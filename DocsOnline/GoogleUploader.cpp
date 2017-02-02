/*@ Docs Online - A Google Docs Backup Applicaiotn                          */
/*                                                                          */
/*--------------------------------------------------------------------------*/
/* Written and Designed by Michael Haephrati                                */
/* COPYRIGHT ©2008 by Michael Haephrati    haephrati@gmail.com              */
/* All rights reserved.                                                     */
/* -------------------------------------------------------------------------*/
#include "StdAfx.h"
#include "GoogleUploader.h"
#include "SmartHTTP.h"
#include "Utils.h"
#include "AppConstants.h"
#include "UIStrings.h"
#include "myxml.h"
#include "hashstring.h"
#include "GoogleFolderTree.h"

//////////////////////////////////////////////////////////////////////////
// Google Data Protocol constants

// login
#define GOOGLE_LOGIN_SERVER                 L"www.google.com"
#define GOOGLE_LOGIN_ENGINE                 L"accounts/ClientLogin"
#define FIELD_ACCOUNT_TYPE                  L"accountType"
#define VALUE_ACCOUNT_TYPE                  L"HOSTED_OR_GOOGLE"
#define FIELD_SERVICE                       L"service"
#define VALUE_SERVICE                       L"writely"
#define FIELD_EMAIL                         L"Email"
#define FIELD_PASSWD                        L"Passwd"
#define FIELD_SOURCE                        L"source"

#define GOOGLE_AUTH_PARAM                   "Auth"
#define GOOGLE_ERROR_PARAM                  "Error"

// docs
#define GDOCS_SERVER                        L"docs.google.com"
#define GDOCS_API_ENGINE                    L"feeds/default/private/full"
#define GDOCS_API_UPLOADNOCONVERT           L"?convert=false"
#define GDOCS_HOST_KEY                      L"Host"
#define GDOCS_HOST_VALUE                    L"docs.google.com"
#define AUTHORIZATION_KEY                   L"Authorization"
#define GOOGLELOGIN_AUTH_FORMAT             L"GoogleLogin auth="
#define GDATA_VERSION_KEY                   L"GData-Version"
#define GDATA_VERSION_VALUE                 L"3.0"
#define FILENAME_KEY                        L"Slug"
#define CONVERT_KEY                         L"convert"
#define GDOCS_RESOURCE_FILE                 L"file"
#define GDOCS_RESOURCE_PDF                  L"pdf"

#define GOOGLE_XML_ERRORS                   L"errors"
#define GOOGLE_XML_ERROR                    L"error"

//////////////////////////////////////////////////////////////////////////

BOOL HttpsHrefSplit(wstring &server, wstring &engine, const wstring &href)
{
  static const wstring httpsPrefix(L"https://");
  size_t dividerIndex;

  server.clear();
  engine.clear();
  if (str::StartingWithI(href, httpsPrefix) && href.length() > httpsPrefix.length())
  {
    dividerIndex = href.find('/', httpsPrefix.length());
    if (dividerIndex == wstring::npos)
      server = href;
    else
    {
      server = href.substr(httpsPrefix.length(), dividerIndex-httpsPrefix.length());
      if (dividerIndex != wstring::npos && href.length() > dividerIndex)
        engine = href.substr(dividerIndex + 1);
    }
  }
  return !server.empty();
}

typedef struct
{
  const wchar_t *ext;
  const wchar_t *mimeType;
  __int64 maxSize;
} FILETYPE;

const int KB = 1024;
const int MB = KB * KB;

FILETYPE fileTypes[] = 
{
  { L"CSV", L"text/csv", 1*MB },
  { L"TSV", L"text/tab-separated-values", 1*MB },
  { L"TAB", L"text/tab-separated-values", 1*MB },
  { L"HTML", L"text/html", 500*KB },
  { L"HTM", L"text/html", 500*KB },
  { L"DOC", L"application/msword", -1 },
  { L"DOCX", L"application/vnd.openxmlformats-officedocument.wordprocessingml.document", -1 },
  { L"ODS", L"application/x-vnd.oasis.opendocument.spreadsheet", 1*MB },
  { L"ODT", L"application/vnd.oasis.opendocument.text", -1 },
  { L"RTF", L"application/rtf", -1 },
  { L"SXW", L"application/vnd.sun.xml.writer", -1 },
  { L"TXT", L"text/plain", 500*KB },
  { L"XLS", L"application/vnd.ms-excel", 1*MB },
  { L"XLSX", L"application/vnd.openxmlformats-officedocument.spreadsheetml.sheet", 1*MB },
  { L"PDF", L"application/pdf", -1 },
  { L"PPT", L"application/vnd.ms-powerpoint", 10*MB },
  { L"PPS", L"application/vnd.ms-powerpoint", 10*MB },
};

BOOL CGoogleUploader::IsFileAllowed(const wchar_t *fileExt, __int64 fileSize)
{
  BOOL result;
  size_t i;

  result = FALSE;
  for (i = 0; i < ARRAYDIM(fileTypes); i++)
  {
    if (_wcsicmp(fileTypes[i].ext, fileExt) == 0) // ASCII case insensitive comparison
      break;
  }
  if (i < ARRAYDIM(fileTypes))
  {
    if (fileSize == -1 || fileTypes[i].maxSize == -1 || fileSize <= fileTypes[i].maxSize)
      result = TRUE;
  }
  return result;
}

CGoogleUploader::CGoogleUploader(void)
{
  m_notifyWindow = NULL;
  m_uploadThread = NULL;
  m_abortEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
}

CGoogleUploader::~CGoogleUploader(void)
{
}

void CGoogleUploader::Initialize( HWND notifyWindow )
{
  // supposed to be called only once 
  ASSERT(m_notifyWindow == NULL);
  m_notifyWindow = notifyWindow;
}

void URLEncodeToAnsi(string &urlEncoded, const wstring &unicodeString)
{
  static const char hexToChar[16] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 
    'A', 'B', 'C', 'D', 'E', 'F'};
  static char encoded[4] = { '%', '0', '0', '\0' };

  vector<char> utf8;
  size_t i;
  char c;

  urlEncoded.clear();
  str::Unicode2MB(utf8, unicodeString.c_str(), CP_UTF8);
  for (i = 0; i < utf8.size(); i++)
  {
    c = utf8[i];
    if ((c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c == '-' || c == '_' || c == '.')
      urlEncoded += c;
    else
    {
      encoded[1] = hexToChar[(c >> 4) & 0x0F];
      encoded[2] = hexToChar[c & 0x0F];
      urlEncoded += encoded;
    }
  }
}

void URLEncode(wstring &urlEncoded, const wstring &unicodeString)
{
  string buf;

  URLEncodeToAnsi(buf, unicodeString);
  str::MB2Unicode(urlEncoded, buf.c_str(), buf.length());
}

void AddFormField(string &urlEncodedForm, const wchar_t *field, const wstring &value)
{
  string encodedField;
  string encodedValue;

  URLEncodeToAnsi(encodedField, field);
  URLEncodeToAnsi(encodedValue, value);
  if (!urlEncodedForm.empty())
    urlEncodedForm += '&';
  urlEncodedForm += encodedField;
  urlEncodedForm += '=';
  urlEncodedForm += encodedValue;
}

void GetHTTPStatusString(wstring &descr, DWORD httpStatus)
{
  switch (httpStatus)
  {
  case HTTP_STATUS_DENIED:
    descr = UIFormat(msg_accessdenied);
    break;
  case HTTP_STATUS_FORBIDDEN:
    descr = UIFormat(msg_forbidden);
    break;
  case HTTP_STATUS_UNSUPPORTED_MEDIA:
    descr = UIFormat(msg_unsupportedmedia);
    break;
  default:
    descr = UIFormat(msg_httperror, str::IntToStr(httpStatus).c_str());
  }
}

void FormatWinInetError( wstring &message, DWORD errorCode)
{
  switch (errorCode)
  {
  case ERROR_INTERNET_TIMEOUT:
    message = UIFormat(msg_requesttimeout);
    break;
  case ERROR_INTERNET_NAME_NOT_RESOLVED:
    message = UIFormat(msg_srvnamenotresolved);
    break;
  case ERROR_INTERNET_CANNOT_CONNECT:
    message = UIFormat(msg_cannotconnect);
    break;
  case ERROR_INTERNET_CONNECTION_ABORTED:
    message = UIFormat(msg_connectionaborted);
    break;
  case ERROR_INTERNET_CONNECTION_RESET:
    message = UIFormat(msg_connectionreset);
    break;
  case ERROR_HTTP_INVALID_SERVER_RESPONSE:
    message = UIFormat(msg_invalidserverresponse);
    break;
  case ERROR_INTERNET_SECURITY_CHANNEL_ERROR:
    message = UIFormat(msg_securitychannelerror);
    break;
  default:
    message = UIFormat(msg_winineterror, str::IntToStr(errorCode).c_str());
    break;
  }
}

BOOL ExtractParam(wstring &paramValue, const char *paramName, const string &params)
{
  size_t start;
  size_t end;
  string name(paramName);
  
  name += '=';
  if (params.compare(0, name.length(), name) == 0)
    start = name.length();
  else
  {
    name.insert(0, 1, '\n');
    start = params.find(name);
    if (start != string::npos)
      start += name.length();
  }
  if (start != string::npos)
  {
    end = params.find('\n', start);
    if (end == string::npos)
      end = params.length();
    str::MB2Unicode(paramValue, params.c_str() + start, end - start);
    return TRUE;
  }
  paramValue.clear();
  return FALSE;
}

BOOL CGoogleUploader::Login(RESULT_INFO &resInfo, wstring &auth, const wstring &user, const wstring &password)
{
  BOOL result;
  INTERNAL_RESULT ir;
  stringbuf reply;
  wstring header;
  string content;

  result = FALSE;

  AddFormField(content, FIELD_ACCOUNT_TYPE, VALUE_ACCOUNT_TYPE);
  AddFormField(content, FIELD_SERVICE, VALUE_SERVICE);
  AddFormField(content, FIELD_EMAIL, user);
  AddFormField(content, FIELD_PASSWD, password);
  AddFormField(content, FIELD_SOURCE, AGENT_NAME);

  CSmartHTTP::AddHeader(header, HTTP_CONTENT_LENGTH, str::Int64ToStr(content.length()).c_str());
  CSmartHTTP::AddHeader(header, HTTP_CONTENT_TYPE, HTTP_MIME_FORM_URL_ENCODED);
  
  stringbuf contentBuf(content, ios_base::in);
  if (!m_internet.Request(ir.res, ir.winInetError, ir.httpStatus, reply, 
    CSmartHTTP::POST, GOOGLE_LOGIN_SERVER, GOOGLE_LOGIN_ENGINE, 
    header.c_str(), &contentBuf, content.length(), TRUE, m_abortEvent))
    FormatInternalResult(resInfo, ir); // sets UR_ERROR which allows retries for connectivity problems
  else // we got a http response
  {
    const string &googleReply = reply.str();
    if (ir.httpStatus == HTTP_STATUS_OK)
    {
      if (ExtractParam(auth, GOOGLE_AUTH_PARAM, googleReply))
        result = TRUE;
      else
        resInfo.errorMessage = UIFormat(msg_unexpectedserverreply);
    }
    else
    {
      if (!ExtractParam(resInfo.serverMessage, GOOGLE_ERROR_PARAM, googleReply))
        GetHTTPStatusString(resInfo.errorMessage, ir.httpStatus);
    }
    if (!result)
      resInfo.res = UR_CRITICALERROR; // stop retries
  }
  return result;
}

// build common gdocs header
wstring BuildGDocsHeader(const wstring &auth, const wchar_t *contentType = NULL, __int64 contentLength = 0, const wchar_t *host = GDOCS_HOST_VALUE)
{
  wstring result;
  wstring googleLoginAuth(GOOGLELOGIN_AUTH_FORMAT);
  googleLoginAuth += auth; 
  CSmartHTTP::AddHeader(result, GDOCS_HOST_KEY, host);
  CSmartHTTP::AddHeader(result, AUTHORIZATION_KEY, googleLoginAuth.c_str());
  CSmartHTTP::AddHeader(result, GDATA_VERSION_KEY, GDATA_VERSION_VALUE);
  if (contentType != NULL && contentLength > 0)
    CSmartHTTP::AddHeader(result, HTTP_CONTENT_LENGTH, str::Int64ToStr(contentLength).c_str());
  if (contentType != NULL)
    CSmartHTTP::AddHeader(result, HTTP_CONTENT_TYPE, contentType);
  return result;
}

std::wstring CGoogleUploader::BuildFileUploadHeader( const wstring &auth, const wstring &googleTitle, const wstring &path, __int64 fileSize, BOOL convert )
{
  wstring result;
  wstring googleLoginAuth(GOOGLELOGIN_AUTH_FORMAT);
  wstring fileTitle;
  wstring fileExtention;
  string urlEncodedFileName;

  ExtractFileExt(fileExtention, path);
  result = BuildGDocsHeader(auth, CSmartHTTP::GetMIMETypeOrBinary(fileExtention.c_str()).c_str(), fileSize);
  
  fileTitle = googleTitle;
  // Google removes extension when importing files
  // appending an empty extension will preserve original extension in Google document name
  if (convert)
    fileTitle += '.';
  URLEncode(fileTitle, fileTitle);
  CSmartHTTP::AddHeader(result, FILENAME_KEY, fileTitle.c_str());
  return result;
}

void CGoogleUploader::SetUploadStep(UPLOAD_STEP uploadStep)
{
  m_uploadStep = uploadStep;
}

BOOL CGoogleUploader::BeginFileUpload(const wstring &auth, const wstring &googleFolderId, 
                                      const wstring &user, const wstring &password, BOOL convert, 
                                      const wstring &path, const wstring &googlePath)
{
  DWORD threadId;

  // make sure not uploading at the moment
  ASSERT(m_uploadThread == NULL);

  ResetEvent(m_abortEvent);
  m_auth = auth;
  m_user = user;
  m_password = password;
  m_path = path;
  m_googlePath = googlePath;
  m_googleFolderId = googleFolderId;
  m_convert = convert;
  SetUploadStep(UP_NONE);
  m_resInfo.clear();
  m_postToSend = 0;
  m_uploadThread = CreateThread(NULL, 0, &StaticUploadThreadProc, this, 0, &threadId);
  return (m_uploadThread != NULL);
}

void CGoogleUploader::EndFileUpload(RESULT_INFO &resInfo, wstring &auth, wstring &newTitle, wstring &googleFolderId, BOOL abort)
{
  MSG msg;

  ASSERT(m_uploadThread != NULL);

  if (abort)
  {
    SetEvent(m_abortEvent);
    WaitForSingleObject(m_uploadThread, INFINITE);
    // remove notification if the upload thread was in time send one
    if (m_notifyWindow != NULL)
      PeekMessage(&msg, m_notifyWindow, AM_FILE_UPLOAD_END, AM_FILE_UPLOAD_END, TRUE);
  }
  m_uploadThread = NULL;
  resInfo = m_resInfo;
  auth = m_auth;
  newTitle = m_newTitle;
  googleFolderId = m_googleFolderId;
}

void CGoogleUploader::GetUploadStatus( UPLOAD_STEP &uploadStep, int &percent )
{
  __int64 postSent;
  __int64 postToSend;

  uploadStep = m_uploadStep;
  postToSend = m_postToSend;
  postSent = m_internet.GetPostSent();

  if (uploadStep != UP_SENDING || postToSend == 0)
    percent = 0;
  else
  {
    percent = (int)(postSent * 100 / postToSend);
    if (percent == 0 && postSent > 0)
      percent = 1;
    else
    {
      if (percent == 100 && postSent < postToSend)
        percent = 99;
    }
  }
}

DWORD WINAPI CGoogleUploader::StaticUploadThreadProc(LPVOID lpParameter)
{
  ((CGoogleUploader *)lpParameter)->UploadThreadProc();
  return 0;
}

BOOL CGoogleUploader::GDocsRequest(INTERNAL_RESULT &ir, streambuf &reply,
                                   LPCTSTR verb, LPCTSTR engine, LPCTSTR header, 
                                   streambuf *content, __int64 contentLength )
{
  return m_internet.Request(ir.res, ir.winInetError, ir.httpStatus, reply, 
    verb, GDOCS_SERVER, engine, 
    header, content, contentLength, TRUE, m_abortEvent);
}

void CGoogleUploader::FormatInternalResult(RESULT_INFO &resInfo, const INTERNAL_RESULT &rr)
{
  switch (rr.res)
  {
  case CSmartHTTP::RES_ABORT:
    resInfo.res = UR_ABORT;
    break;
  case CSmartHTTP::RES_EWININET:
    resInfo.res = UR_ERROR;
    FormatWinInetError(resInfo.errorMessage, m_internet.GetError());
    break;
  case CSmartHTTP::RES_EINPUTSTREAM:
    resInfo.res = UR_ERROR;
    resInfo.errorMessage = UIFormat(msg_postinputstreamerror);
    break;
  case CSmartHTTP::RES_EOUTPUTSTREAM:
    resInfo.res = UR_ERROR;
    resInfo.errorMessage = UIFormat(msg_postoutputstreamerror);
    break;
  default:
    ASSERT(FALSE);
    resInfo.res = UR_ERROR;
  }
}

size_t WriteXml(streambuf &destination, IXMLDOMDocument *xmlDoc)
{
  static const char xmlVersionEncoding[] = "<?xml version='1.0' encoding='UTF-8'?>";
  const size_t len = ARRAYDIM(xmlVersionEncoding) - 1;
  CComBSTR xmlString;
  vector<char> utf8;

  myxml::GetXml(&xmlString, xmlDoc);
  str::Unicode2MB(utf8, xmlString, CP_UTF8);
  destination.sputn(xmlVersionEncoding, len);
  destination.sputn(&utf8[0], utf8.size());
  return (len + utf8.size());
};

size_t WriteCreateFolderXml(streambuf &destination, const wchar_t *folderName)
{
  CComPtr<IXMLDOMDocument> xmlDoc;
  myxml::CreateDocInstance(xmlDoc);
  CComPtr<IXMLDOMNode> entryNode;
  myxml::AppendNewNode(&entryNode, xmlDoc, NULL, L"entry");
  CComQIPtr<IXMLDOMElement> entryElement(entryNode);
  myxml::SetElementAttribute(entryElement, L"xmlns", L"http://www.w3.org/2005/Atom");
  CComPtr<IXMLDOMNode> categoryNode;
  myxml::AppendNewNode(&categoryNode, xmlDoc, entryNode, L"category");
  CComQIPtr<IXMLDOMElement> categoryElement(categoryNode);
  myxml::SetElementAttribute(categoryElement, L"scheme", L"http://schemas.google.com/g/2005#kind");
  myxml::SetElementAttribute(categoryElement, L"term", L"http://schemas.google.com/docs/2007#folder");
  myxml::AppendTextElement(xmlDoc, entryNode, L"title", folderName);
  return WriteXml(destination, xmlDoc);
}

// http://code.google.com/intl/ru/apis/documents/docs/3.0/developers_guide_protocol.html#CreateFolders
BOOL CGoogleUploader::CreateFolder(RESULT_INFO &resInfo, wstring &folderId, const wstring &auth, const wstring &folderName, const wstring &parentFolderId)
{
  BOOL result;
  wstring engine;
  INTERNAL_RESULT ir;
  stringbuf xmlStream;
  wstring header;
  stringbuf reply;
  size_t xmlLength;

  result = FALSE;
  folderId.clear();
  engine = GDOCS_API_ENGINE;
  if (parentFolderId != CGoogleFolderTree::RootFolderId())
  {
    engine += L"/folder%3A";
    engine += parentFolderId;
    engine += L"/contents";
  }
  xmlLength = WriteCreateFolderXml(xmlStream, folderName.c_str());
  header = BuildGDocsHeader(auth, L"application/atom+xml", xmlLength);
  if (!GDocsRequest(ir, reply, CSmartHTTP::POST, engine.c_str(), header.c_str(), &xmlStream, xmlLength))
    FormatInternalResult(resInfo, ir);
  else if (ir.httpStatus != HTTP_STATUS_CREATED) 
    FormatHTTPError(resInfo, ir.httpStatus, reply.str());
  else // ok
  {
    xml_document xmlDoc;
    if (xmlDoc.load(reply.str()))
    {
      if (xmlDoc.root().name() == GOOGLE_XML_ENTRY)
      {
        xml_element id = xmlDoc.root().subnode(GOOGLE_XML_ID);
        folderId = CGoogleFolderTree::ExtractIdFromIdRef(id.val());
      }
    }
    if (folderId.empty())
      resInfo.errorMessage = UIFormat(msg_unexpectedserverreply);
    else
      result = TRUE;
  }
  return result;
}

BOOL HandleErrorXml(wstring &code, wstring &internalReason, const string &xmlAnsi)
{
  BOOL result;

  result = FALSE;
  xml_document xmlDoc;
  if (xmlDoc.load(xmlAnsi))
  {
    if (xmlDoc.root().name() == GOOGLE_XML_ERRORS)
    {
      xml_element error = xmlDoc.root().subnode(GOOGLE_XML_ERROR);
      if (!error.empty())
      {
        result = TRUE;
        code = error.subval(L"code");
        internalReason = error.subval(L"internalReason");
      }
    }
  }
  if (!result)
  {
    code.clear();
    internalReason.clear();
  }
  return result;
}

void CGoogleUploader::FormatHTTPError( RESULT_INFO &resInfo, DWORD httpStatus, const string &serverResponse)
{
  if (!HandleErrorXml(resInfo.errorMessage, resInfo.serverMessage, serverResponse))
    GetHTTPStatusString(resInfo.errorMessage, httpStatus);
}

BOOL CGoogleUploader::GetFolderFiles(RESULT_INFO &resInfo, vector<GOOGLE_FILE_DESCR> &files, const wstring &auth, const wstring &googleFolderId)
{
  wstring header;
  INTERNAL_RESULT ir;
  BOOL result;
  wstring server;
  wstring engine;
  BOOL cycleResult;
  size_t entriesProcessed;
  wstring nextHref;

  result = FALSE;
  files.clear();
  header = BuildGDocsHeader(auth);
  engine = L"/feeds/default/private/full/folder%3A";
  if (googleFolderId.empty())
    engine += L"root";
  else
    engine += googleFolderId;
  engine += L"/contents";
  // iterate over link rel="next"
  do 
  {
    cycleResult = -1;
    stringbuf reply;
    if (!GDocsRequest(ir, reply, CSmartHTTP::GET, engine.c_str(), header.c_str()))
      FormatInternalResult(resInfo, ir);
    else if (ir.httpStatus != HTTP_STATUS_OK)
      FormatHTTPError(resInfo, ir.httpStatus, reply.str());
    else
    {
      if (!CGoogleFolderTree::AddFiles(entriesProcessed, files, nextHref, reply.str(), googleFolderId))
        resInfo.errorMessage = UIFormat(msg_unexpectedserverreply);
      else
      {
        if (entriesProcessed == 0 || nextHref.empty())
          cycleResult = 1; // successful end of the loop
        else
        {
          if (!HttpsHrefSplit(server, engine, nextHref))
            resInfo.errorMessage = UIFormat(msg_unexpectedserverreply);
          else
            cycleResult = 0; // continue the loop
        }
      }
    }
  } while (cycleResult == 0);
  return (cycleResult > 0);
}

BOOL CGoogleUploader::SendFile( RESULT_INFO &resInfo, const wstring &auth, const wstring &googleTitle, const wstring &googleFolderId, const wstring &path, BOOL convert )
{
  BOOL result;
  stringbuf reply;
  filebuf file;
  wstring header;
  __int64 fileSize;
  wstring descr;
  wstring engine;
  INTERNAL_RESULT ir;
  BOOL isHttp;

  result = FALSE;
  if (!GetFileSizeByName(fileSize, path.c_str()) || (file.open(path.c_str(), ios_base::in | ios_base::binary) == NULL))
    resInfo.errorMessage = UIFormat(msg_fileopenerror);
  else
  {
    m_postToSend = fileSize;
    // preserve file extension if document is converted into the google format
    header = BuildFileUploadHeader(auth, googleTitle, path, fileSize, convert);
    engine = GDOCS_API_ENGINE;
    if (!googleFolderId.empty())
    {
      engine += L"/folder%3A";
      engine += googleFolderId;
      engine += L"/contents";
    }
    if (!convert)
      engine += GDOCS_API_UPLOADNOCONVERT;
    isHttp = GDocsRequest(ir, reply, CSmartHTTP::POST, engine.c_str(), header.c_str(), &file, fileSize);
    file.close();
    if (!isHttp)
    {
      // replace message for input stream error: show a file read error
      if (ir.res == CSmartHTTP::RES_EINPUTSTREAM)
        resInfo.errorMessage = UIFormat(msg_filereaderror);
      else
        FormatInternalResult(resInfo, ir);
    }
    else
    { // we received a http response
      if (ir.httpStatus == HTTP_STATUS_OK || ir.httpStatus == HTTP_STATUS_CREATED)
        result = TRUE;
      else
      {
        FormatHTTPError(resInfo, ir.httpStatus, reply.str());
        // allow to continue for all errors but authorization
        resInfo.res = ir.httpStatus == HTTP_STATUS_DENIED ? UR_CRITICALERROR : UR_ERROR;
      }
    }
  }
  return result;
}

// http://code.google.com/intl/ru/apis/documents/docs/3.0/developers_guide_protocol.html#ListDocs
BOOL CGoogleUploader::GetFolderList(RESULT_INFO &resInfo, GOOGLE_FOLDER_DESCR &googleFolderTree, const wstring &auth)
{
  wstring header;
  INTERNAL_RESULT ir;
  stringbuf reply;
  BOOL result;
  vector<CGoogleFolderTree::GOOGLE_FOLDER_BD> folders;
  wstring server;
  wstring engine;
  BOOL cycleResult;
  size_t entriesProcessed;
  wstring nextHref;

  result = FALSE;
  header = BuildGDocsHeader(auth);

  engine = L"/feeds/default/private/full/-/folder";
  do 
  {
    cycleResult = -1;
    stringbuf reply;
    if (!GDocsRequest(ir, reply, CSmartHTTP::GET, engine.c_str(), header.c_str()))
      FormatInternalResult(resInfo, ir);
    else if (ir.httpStatus != HTTP_STATUS_OK)
      FormatHTTPError(resInfo, ir.httpStatus, reply.str());
    else
    {
      if (!CGoogleFolderTree::AddFolders(entriesProcessed, folders, nextHref, reply.str()))
        resInfo.errorMessage = UIFormat(msg_unexpectedserverreply);
      else
      {
        if (entriesProcessed == 0 || nextHref.empty())
          cycleResult = 1; // successful end of the loop
        else
        {
          if (!HttpsHrefSplit(server, engine, nextHref) || !str::EqualI(server, GDOCS_SERVER))
            resInfo.errorMessage = UIFormat(msg_unexpectedserverreply);
          else
            cycleResult = 0; // continue the loop
        }
      }
    }
  } while (cycleResult == 0);
  if (cycleResult == 1)
  {
    result = TRUE;
    CGoogleFolderTree::BuildFolderTree(googleFolderTree, folders);
  }
  return result;
}

BOOL IsPathUnder(const wstring &path, const wstring &pathHi)
{
  return (path.length() >= pathHi.length() && _wcsnicmp(path.c_str(), pathHi.c_str(), pathHi.length()) == 0);
}

BOOL CGoogleUploader::CreateGooglePath(RESULT_INFO &resInfo, wstring &finalId, 
                                       const wstring &auth, const path_vector &pathWithCase, size_t startAt, const wstring &parentId)
{
  BOOL result;
  size_t i;
  wstring currentFolder;
  wstring newFolder;

  result = TRUE;
  currentFolder = parentId;
  for (i = startAt; result && i < pathWithCase.size(); i++)
  {
    result = CreateFolder(resInfo, newFolder, auth, pathWithCase[i], currentFolder);
    currentFolder = newFolder;
  }
  if (result)
    finalId = newFolder;
  return result;
}

BOOL CGoogleUploader::ProvideGooglePath(RESULT_INFO &resInfo, BOOL &folderCreated, wstring &folderId, const wstring &auth, const wstring &googlePath)
{
  path_vector googlePathVector;
  folder_list::const_iterator folder;
  BOOL result;

  result = FALSE;
  folderCreated = FALSE;
  if (googlePath.empty())
  {
    result = TRUE;
    folderId = CGoogleFolderTree::RootFolderId();
  }
  else
  {
    GOOGLE_FOLDER_DESCR googleFolderTree;
    if (GetFolderList(resInfo, googleFolderTree, auth))
    {
      str::split(googlePathVector, googlePath, '\\');
      size_t level = 0;
      const GOOGLE_FOLDER_DESCR *folderDescr = &googleFolderTree;
      // tree search
      while (level < googlePathVector.size())
      {
        const wstring &folderName = googlePathVector[level];
        for (folder = folderDescr->children.begin(); folder != folderDescr->children.end() 
          && !str::EqualI(folder->title, folderName); folder++);
        if (folder == folderDescr->children.end())
          break; // no match at the current level -> finish the tree search
        // match found, advance to the next level in the tree
        folderDescr = &(*folder);
        level++; 
      }
      // at the end of the tree search folderDescr contains pointer to the parent folder 
      // and level contains index in googlePathUpcase to start folder creation from
      if (level == googlePathVector.size())
      {
        result = TRUE;
        folderId = folderDescr->id; // return the deepest level found (will return "" for the google root folder id)
      }
      else
      {
        result = CreateGooglePath(resInfo, folderId, auth, googlePathVector, level, folderDescr->id);
        if (result)
          folderCreated = TRUE;
      }
    }
  }
  return result;
}

void CGoogleUploader::UploadThreadProc()
{
  CoInitialize(NULL);
  try
  {
    Upload(m_resInfo, m_newTitle, m_auth, m_googleFolderId, m_user, m_password, m_googlePath, m_path, m_convert);
  }
  catch (exception &e)
  {
    m_resInfo.res = UR_CRITICALERROR;
    str::MB2Unicode(m_resInfo.errorMessage, e.what());
  }
  catch (...)
  {
    m_resInfo.res = UR_CRITICALERROR;
    m_resInfo.errorMessage = L"Generic exception";
  }
  CoUninitialize();
  PostMessage(m_notifyWindow, AM_FILE_UPLOAD_END, 0, 0);
}

class CFileComparator
{
public:
  CFileComparator()
  {
  }
  BOOL Open(const wchar_t *path, streambuf *httpErrorStream)
  {
    m_differenceFound = FALSE;
    m_httpErrorStream = httpErrorStream;
    return (GetFileSizeByName(m_fileSize, path) && (m_file.open(path, ios_base::in | ios_base::binary) != NULL));
  }
  void Close()
  {
    m_file.close();
  }
  static size_t Callback(DWORD httpStatus, __int64 contentLength, const void *buf, size_t toRead, void *userParam)
  {
    if (httpStatus != HTTP_STATUS_OK) 
      return ((CFileComparator*)userParam)->m_httpErrorStream->sputn((const char*)buf, toRead);
    return ((CFileComparator*)userParam)->Compare(contentLength, buf, toRead);
  }
  BOOL DifferenceFound() { return m_differenceFound; }
protected:
  streambuf *m_httpErrorStream;
  __int64 m_fileSize;
  filebuf m_file;
  BOOL m_differenceFound;
  vector<char> m_filedata;

  size_t Compare(__int64 contentLength, const void *buf, size_t toRead)
  {
    size_t result;

    result = 0;
    if (contentLength != -1 && contentLength != m_fileSize)
      m_differenceFound = TRUE;
    else
    {
      m_filedata.resize(toRead);
      if (m_file.sgetn(&m_filedata[0], toRead) == toRead)
      {
        if (memcmp(&m_filedata[0], buf, toRead) != 0)
          m_differenceFound = TRUE;
        else
          result = toRead;
      }
    }
    return result;
  }
};

BOOL CGoogleUploader::FilesMatch(RESULT_INFO &resInfo, BOOL &match, const wstring &auth, const wstring &fileUrl, const wstring &path)
{
  wstring header;
  INTERNAL_RESULT ir;
  BOOL result;
  wstring server;
  wstring engine;
  stringbuf errorReply;
  CFileComparator fileComparator;
  BOOL requestResult;

  result = FALSE;
  match = FALSE;
  if (!fileComparator.Open(path.c_str(), &errorReply))
    resInfo.errorMessage = UIFormat(msg_fileopenerror);
  else
  {
    HttpsHrefSplit(server, engine, fileUrl);
    header = BuildGDocsHeader(auth, NULL, 0, server.c_str());
    requestResult = m_internet.RequestEx(ir.res, ir.winInetError, ir.httpStatus, CFileComparator::Callback, &fileComparator,
      CSmartHTTP::GET, server.c_str(), engine.c_str(), 
      header.c_str(), NULL, 0, TRUE, m_abortEvent);
    fileComparator.Close();
    if (!requestResult)
    {
      if (ir.res == CSmartHTTP::RES_EOUTPUTSTREAM)
      {
        if (fileComparator.DifferenceFound())
          result = TRUE; // RES_EOUTPUTSTREAM raised because of file difference
        else
          resInfo.errorMessage = UIFormat(msg_filereaderror);
      }
      else
        FormatInternalResult(resInfo, ir);
    }
    else if (ir.httpStatus != HTTP_STATUS_OK)
      FormatHTTPError(resInfo, ir.httpStatus, errorReply.str());
    else
    {
      ASSERT(!fileComparator.DifferenceFound());
      match = TRUE; 
      result = TRUE;
    }
  }
  return result;
}

BOOL CGoogleUploader::CheckFileCopy(RESULT_INFO &resInfo,
                                    OUT BOOL &fileOnServer,
                                    OUT wstring &newTitle,
                                    IN const wstring &auth,
                                    IN const wstring &fileTitle,
                                    IN const wstring &googleFolderId,
                                    IN const wstring &path)
{
  const long file_not_found = -1;
  const long file_exact_name = 0;

  BOOL result;
  vector<GOOGLE_FILE_DESCR> files;
  vector<GOOGLE_FILE_DESCR>::iterator fileIter;
  wstring googleName;
  wstring googleExt;
  wstring fileName;
  wstring fileExt;
  vector<GOOGLE_FILE_DESCR>::iterator maxGoogleFile;
  long maxSuffix;
  long suffix;
  wchar_t *endPtr;
  BOOL needNewTitle;

  fileOnServer = FALSE;
  newTitle.clear();
  result = GetFolderFiles(resInfo, files, auth, googleFolderId);
  if (result)
  {
    ExtractFileExt(fileExt, fileTitle);
    StripFileExt(fileName, fileTitle);
    maxSuffix = file_not_found;
    for (fileIter = files.begin(); fileIter != files.end(); fileIter++)
    {
      ExtractFileExt(googleExt, fileIter->title);
      if (str::EqualI(fileExt, googleExt))
      {
        suffix = file_not_found;
        StripFileExt(googleName, fileIter->title);
        if (str::EqualI(fileName, googleName))
          suffix = file_exact_name;
        else if (googleName.length() > fileName.length() && str::StartingWithI(googleName, fileName) && googleName[fileName.length()] == '_') 
        {
          // check if googleName looks like "fileName_NNN"
          suffix = wcstol(googleName.c_str() + fileName.length() + 1, &endPtr, 10);
          if (!(suffix > 0 && suffix < LONG_MAX && *endPtr == '\0')) // successful NNN conversion 
            suffix = file_not_found;
        }
        if (suffix > maxSuffix)
        {
          maxSuffix = suffix;
          maxGoogleFile = fileIter;
        }
      }
    }
    if (maxSuffix != file_not_found)
    {
      GOOGLE_FILE_DESCR &fileDescr = *maxGoogleFile;
      needNewTitle = FALSE;
      if (fileDescr.gdResourceType != GDOCS_RESOURCE_FILE && fileDescr.gdResourceType != GDOCS_RESOURCE_PDF) 
        needNewTitle = TRUE; // we can't compare a disk file to a google document
      else // only to a google file/pdf (located at the googleusercontent.com server)
      {
        result = FilesMatch(resInfo, fileOnServer, auth, fileDescr.contentSrc, path);
        if (result)
        {
          if (fileOnServer)
          {
            if (maxSuffix != file_exact_name)
              newTitle = fileDescr.title;
          }
          else
            needNewTitle = TRUE;
        }
      }
      if (needNewTitle)
      {
        newTitle = fileName;
        newTitle += '_';
        newTitle += str::IntToStr(maxSuffix + 1);
        newTitle += '.';
        newTitle += fileExt;
      }
    }
  }
  return result;
}

void CGoogleUploader::Upload(RESULT_INFO &resInfo,
                             OUT wstring &serverTitle,
                             IN OUT wstring &auth,
                             IN OUT wstring &googleFolderId,
                             const wstring &user, const wstring &password,
                             const wstring &googlePath, 
                             const wstring &path, BOOL convert)
{
  BOOL authOK;
  BOOL folderOK;
  BOOL folderCreated;
  BOOL checkFileCopyOK;
  wstring fileTitle;
  BOOL fileOnServer;

  serverTitle.clear();
  ExtractFileName(fileTitle, path);
  if (auth.empty())
  {
    SetUploadStep(UP_LOGIN);
    authOK = Login(resInfo, auth, user, password);
  }
  else
    authOK = TRUE;
  if (authOK)
  {
    folderCreated = FALSE;
    if (googleFolderId.empty())
    {
      SetUploadStep(UP_FOLDERS);
      folderOK = ProvideGooglePath(resInfo, folderCreated, googleFolderId, auth, googlePath);
    }
    else
      folderOK = TRUE;
    if (folderOK)
    {
      fileOnServer = FALSE;
      if (!folderCreated)
      {
        SetUploadStep(UP_CHECKDUPLICATE);
        checkFileCopyOK = CheckFileCopy(resInfo, fileOnServer, serverTitle, auth, fileTitle, googleFolderId, path);
        if (checkFileCopyOK)
        {
          if (fileOnServer)
            resInfo.res = UR_SUCCESS;
          else if (!serverTitle.empty())
            fileTitle = serverTitle;
        }
      }
      else
        checkFileCopyOK = TRUE;
      if (checkFileCopyOK && !fileOnServer)
      {
        SetUploadStep(UP_SENDING);
        if (SendFile(resInfo, auth, fileTitle, googleFolderId, path, convert))
          resInfo.res = UR_SUCCESS;
      }
    }
  }
}


