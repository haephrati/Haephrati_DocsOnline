/*@ Docs Online - A Google Docs Backup Applicaiotn                          */
/*                                                                          */
/*--------------------------------------------------------------------------*/
/* Written and Designed by Michael Haephrati                                */
/* COPYRIGHT ©2008 by Michael Haephrati    haephrati@gmail.com              */
/* All rights reserved.                                                     */
/* -------------------------------------------------------------------------*/
#include "stdafx.h"
#include "SmartHTTP.h"
#include "AppConstants.h"
#include "Utils.h"

//////////////////////////////////////////////////////////////////////////

#define INTERNET_READ_CHUNK           1024
#define INTERNET_WRITE_CHUNK          1024

#define EVENT_ASYNC_REQUEST           0
#define EVENT_ABORT                   1

//////////////////////////////////////////////////////////////////////////

const wchar_t *CSmartHTTP::GET = L"GET";
const wchar_t *CSmartHTTP::POST = L"POST";

//////////////////////////////////////////////////////////////////////////

// utility class to ensure request session closure on exception
class CRequestSession
{
public:
  CRequestSession(HINTERNET requestSession) 
  { 
    m_requestSession = requestSession; 
  }
  ~CRequestSession() 
  { 
    Close();
  }
  HINTERNET GetHandle() { return m_requestSession; }
  void Close()
  {
    if (m_requestSession != NULL)
    {
      BOOL closeResult = InternetCloseHandle(m_requestSession);
      ASSERT(closeResult);
      m_requestSession = NULL;
    }
  }

private:
  HINTERNET m_requestSession;
};

//////////////////////////////////////////////////////////////////////////
const map<wstring, wstring> *Ext2MIME()
{
  return new map<wstring, wstring>( MapInitializer<wstring, wstring>()
    .Add(L"CSV", L"text/csv")
    .Add(L"TSV", L"text/tab-separated-values")
    .Add(L"TAB", L"text/tab-separated-values")
    .Add(L"HTML", L"text/html")
    .Add(L"HTM", L"text/html")
    .Add(L"DOC", L"application/msword")
    .Add(L"DOCX", L"application/vnd.openxmlformats-officedocument.wordprocessingml.document")
    .Add(L"ODS", L"application/x-vnd.oasis.opendocument.spreadsheet")
    .Add(L"ODT", L"application/vnd.oasis.opendocument.text")
    .Add(L"RTF", L"application/rtf")
    .Add(L"SXW", L"application/vnd.sun.xml.writer")
    .Add(L"TXT", L"text/plain")
    .Add(L"XLS", L"application/vnd.ms-excel")
    .Add(L"XLSX", L"application/vnd.openxmlformats-officedocument.spreadsheetml.sheet")
    .Add(L"PDF", L"application/pdf")
    .Add(L"PPT", L"application/vnd.ms-powerpoint")
    .Add(L"PPS", L"application/vnd.ms-powerpoint") );
}

const map<wstring, wstring> CSmartHTTP::m_ext2MIME(*Ext2MIME());

wstring CSmartHTTP::GetMIMETypeOrBinary( const wstring extention )
{
  map<wstring, wstring>::const_iterator iter;
  iter = m_ext2MIME.find(str::ToUpper(extention));
  return iter == m_ext2MIME.end() ? HTTP_MIME_BINARY : iter->second;
}

void CSmartHTTP::AddHeader(wstring &header, LPCTSTR key, LPCTSTR value)
{
  header += str::Format1K(L"%s: %s\n", key, value);
}

//////////////////////////////////////////////////////////////////////////

CSmartHTTP::CSmartHTTP(void)
: m_hInet(0), m_hConnection(0)
{
  InitializeCriticalSection(&m_cs);
  m_events[EVENT_ASYNC_REQUEST] = CreateEvent(NULL, TRUE, FALSE, NULL);
  m_events[EVENT_ABORT] = NULL;
}

CSmartHTTP::~CSmartHTTP(void)
{
  Disconnect();
  Uninitialize();
  CloseHandle(m_events[EVENT_ASYNC_REQUEST]);
  DeleteCriticalSection(&m_cs);
}

BOOL CSmartHTTP::Initialize()
{
  if (m_hInet == NULL)
  {
    m_hInet = InternetOpen(AGENT_NAME,
      INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, INTERNET_FLAG_ASYNC);
    if (m_hInet != NULL)
    {
      InternetSetStatusCallback(m_hInet, InternetCallback);
    }
  }
  return (m_hInet != NULL);
}

void CSmartHTTP::Uninitialize()
{
  InternetSetStatusCallback(m_hInet, NULL);
  if (m_hInet != NULL)
  {
    BOOL closeResult = InternetCloseHandle(m_hInet);
    ASSERT(closeResult);
    m_hInet = NULL;
  }
}

BOOL CSmartHTTP::Connect(LPCTSTR serverName, INTERNET_PORT serverPort)
{
  Disconnect();
  if (Initialize())
  {
    m_hConnection = InternetConnect(m_hInet, serverName, serverPort, 
      NULL, NULL, INTERNET_SERVICE_HTTP, 0, (DWORD_PTR) this);
  }
  return (CheckResult(m_hConnection != NULL));
}

void CSmartHTTP::Disconnect()
{
  if (m_hConnection != NULL)
  {
    BOOL closeResult = InternetCloseHandle(m_hConnection);
    ASSERT(closeResult);
    m_hConnection = NULL;
  }
}

void CSmartHTTP::PrepareInetBuffer(INTERNET_BUFFERS *ib)
{
  memset(ib, 0, sizeof(INTERNET_BUFFERS));
  ib->dwStructSize = sizeof(INTERNET_BUFFERS);
}

void __stdcall CSmartHTTP::InternetCallback(HINTERNET hInternet,
    DWORD dwContext,
    DWORD dwInternetStatus,
    LPVOID lpvStatusInformation,
    DWORD dwStatusInformationLength)
{
  CSmartHTTP *pClient = (CSmartHTTP *) dwContext;
  switch (dwInternetStatus)
  {
  case INTERNET_STATUS_REQUEST_COMPLETE:
    pClient->m_error = ((INTERNET_ASYNC_RESULT *)lpvStatusInformation)->dwError;
    ASSERT(pClient->m_error != ERROR_IO_PENDING);
    SetEvent(pClient->m_events[EVENT_ASYNC_REQUEST]);
    break;
  default:
    // Some code.
    break;
  }
}

// checks WinInet function call result and sets m_error to GetLastError when necessary
BOOL CSmartHTTP::CheckResult(BOOL apiResult)
{
  if (!apiResult)
    m_error = GetLastError();
  return apiResult;
}

void CSmartHTTP::AsyncPrepare()
{
  m_error = 0xFFFFFFFF;
  ResetEvent(m_events[EVENT_ASYNC_REQUEST]);
}

BOOL CSmartHTTP::AsyncEnd(RESULT &res)
{
  DWORD waitResult;

  waitResult = WaitForMultipleObjects(m_eventsCount, m_events, FALSE, INFINITE);
  if (waitResult == WAIT_OBJECT_0 + EVENT_ABORT) // abort set
  {
    res = RES_ABORT;
  }
  else // request completion
  {
    ASSERT(m_error != 0xFFFFFFFF);
    res = m_error == ERROR_SUCCESS ? RES_SUCCESS : RES_EWININET;
  }
  return (res == RES_SUCCESS);
}

// m_error is always set after return (to something different then ERROR_IO_PENDING) 
BOOL CSmartHTTP::AsyncResult(RESULT &res, BOOL asyncResult)
{
  if (!asyncResult)
  {
    DWORD error = GetLastError();
    if (error != ERROR_IO_PENDING)
    {
      res = RES_EWININET;
      m_error = error;
    }
    else
    {
      asyncResult = AsyncEnd(res);
    }
  }
  else
    res = RES_SUCCESS;
  return asyncResult;
}

BOOL CSmartHTTP::SendRequest(RESULT &res, HINTERNET hRequestSession, LPCTSTR header, streambuf *content, __int64 contentLength)
{
  char buf[INTERNET_WRITE_CHUNK];
  INTERNET_BUFFERS ibin;
  size_t toSend;
  __int64 totalRead;
  DWORD sent;
  int cycleResult;
  DWORD sendErrorCode;
  RESULT endRequestResult;

  res = RES_SUCCESS;
  PrepareInetBuffer(&ibin);
  ibin.lpcszHeader = header;
  if (header != NULL)
    ibin.dwHeadersLength = wcslen(header);
  ibin.dwBufferTotal = (DWORD) contentLength;

  AsyncPrepare();
  if (AsyncResult(res, HttpSendRequestEx(hRequestSession, &ibin, NULL, HSR_ASYNC, (DWORD_PTR) this)))
  {
    if (content != NULL)
    {
      cycleResult = 0;
      totalRead = 0;
      while (cycleResult == 0 && totalRead < contentLength)
      {
        toSend = content->sgetn(buf, sizeof(buf));
        if (toSend == 0) // end of input
        {
          res = RES_EINPUTSTREAM;
          cycleResult = -1;
        }
        else
        {
          totalRead += toSend;
          AsyncPrepare();
          if (!AsyncResult(res, InternetWriteFile(hRequestSession, buf, toSend, &sent)))
            cycleResult = -1;
          else
          {
            EnterCriticalSection(&m_cs);
            m_postSent += sent;
            LeaveCriticalSection(&m_cs);
            if (IsAbort())
            {
              res = RES_ABORT;
              cycleResult = -1;
            }
          }
        }
      }
    };
    if (res != RES_SUCCESS) // prevent the transfer error from being overwritten by the HttpEndRequest call
      sendErrorCode = GetError();
    AsyncPrepare();
    AsyncResult(endRequestResult, HttpEndRequest(hRequestSession, NULL, HSR_ASYNC, (DWORD_PTR) this));
    if (res != RES_SUCCESS) // if there was send error
      m_error = sendErrorCode; // restore it's code
    else
      res = endRequestResult; // otherwise return HttpEndRequest result
  }
  return (res == RES_SUCCESS);
}

BOOL CSmartHTTP::IsAbort()
{
  BOOL result;

  result = FALSE;
  if (m_events[EVENT_ABORT] != NULL)
  {
    if (WaitForSingleObject(m_events[EVENT_ABORT], 0) == WAIT_OBJECT_0)
      result = TRUE;
  }
  return result;
}

size_t CSmartHTTP::ReadIntoStreambuf(DWORD httpStatus, __int64 contentLength, const void *buf, size_t toRead, void *userParam)
{
  return ((streambuf*)userParam)->sputn((const char*)buf, toRead);
}

BOOL CSmartHTTP::Request( RESULT &res, DWORD &winInetCode, DWORD &httpStatus, streambuf &reply,
                       LPCTSTR verb, LPCTSTR server, LPCTSTR engine, LPCTSTR header, 
                       streambuf *content, __int64 contentLength, 
                       BOOL secure/* = FALSE*/, HANDLE abortEvent/* = NULL*/, int portOverride/* = -1*/ )
{
  return RequestEx(res, winInetCode, httpStatus, ReadIntoStreambuf, &reply, 
    verb, server, engine, header, content, contentLength, secure, abortEvent, portOverride);
}

//////////////////////////////////////////////////////////////////////////

BOOL CSmartHTTP::ReadReplyEx(RESULT &res, DWORD httpStatus, READCALLBACK readCallback, void *userParam, HINTERNET hRequestSession)
{
  char buf[INTERNET_READ_CHUNK];
  INTERNET_BUFFERS ibo;
  BOOL result = FALSE;
  int cycleResult;
  __int64 contentLength;
  DWORD dwContentLength;
  DWORD lenlen;

  lenlen = sizeof(DWORD);
  if (HttpQueryInfo(hRequestSession, HTTP_QUERY_CONTENT_LENGTH | HTTP_QUERY_FLAG_NUMBER, 
    &dwContentLength, &lenlen, NULL))
    contentLength = dwContentLength;
  else
    contentLength = -1;

  res = RES_SUCCESS;
  PrepareInetBuffer(&ibo);
  cycleResult = 0;
  do {
    ibo.lpvBuffer = buf;
    ibo.dwBufferLength = sizeof(buf);
    AsyncPrepare();
    if (!AsyncResult(res, InternetReadFileEx(hRequestSession, &ibo, IRF_ASYNC, (DWORD_PTR) this)))
      cycleResult = -1;
    else
    {
      if (ibo.dwBufferLength == 0)
        cycleResult = 1;
      else
      {
        if (readCallback(httpStatus, contentLength, buf, ibo.dwBufferLength, userParam) != ibo.dwBufferLength)
        {
          res = RES_EOUTPUTSTREAM;
          cycleResult = -1;
        }
        if (IsAbort())
        {
          res = RES_ABORT;
          cycleResult = -1;
        }
      }
    }
  } while (cycleResult == 0);
  if (cycleResult > 0)
    result = TRUE;
  return result;
}

BOOL CSmartHTTP::DoRequestEx( RESULT &res, DWORD &httpStatus, READCALLBACK readCallback, void *userParam, 
                           LPCTSTR verb, LPCTSTR engine, LPCTSTR header, 
                           streambuf *content, __int64 contentLength, BOOL secure, HANDLE abortEvent )
{
  BOOL result = FALSE;
  DWORD flags;
  DWORD len;

  res = RES_SUCCESS;
  if (abortEvent != NULL && abortEvent != INVALID_HANDLE_VALUE)
  {
    m_events[EVENT_ABORT] = abortEvent;
    m_eventsCount = 2;
  }
  else
  {
    m_events[EVENT_ABORT] = NULL;
    m_eventsCount = 1;
  }
  m_postSent = 0;
  flags = INTERNET_FLAG_NO_CACHE_WRITE | INTERNET_FLAG_PRAGMA_NOCACHE;
  if (secure)
    flags |= INTERNET_FLAG_SECURE;
  CRequestSession rs(HttpOpenRequest(m_hConnection, verb, engine, NULL, NULL, NULL, 
    flags, (DWORD_PTR) this));
  if (!CheckResult(rs.GetHandle() != NULL))
    res = RES_EWININET;
  else
  {
    if (SendRequest(res, rs.GetHandle(), header, content, contentLength))
    {
      len = sizeof(httpStatus);
      if (!CheckResult(HttpQueryInfo(rs.GetHandle(), HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER, 
        &httpStatus, &len, NULL)))
      {
        res = RES_EWININET;
      }
      else
      {
        result = ReadReplyEx(res, httpStatus, readCallback, userParam, rs.GetHandle());
      }
    }
    rs.Close();
  }
  m_eventsCount = 1;
  return result;
}

BOOL CSmartHTTP::RequestEx( RESULT &res, DWORD &winInetCode, DWORD &httpStatus, READCALLBACK readCallback, void *userParam,
                         LPCTSTR verb, LPCTSTR server, LPCTSTR engine, LPCTSTR header, 
                         streambuf *content, __int64 contentLength, 
                         BOOL secure/* = FALSE*/, HANDLE abortEvent/* = NULL*/, int portOverride/* = -1*/ )
{
  BOOL result;
  INTERNET_PORT port;

  result = FALSE;
  winInetCode = 0;
  httpStatus = 0;
  if (portOverride >= 0 && portOverride <= 65535)
    port = (INTERNET_PORT)portOverride;
  else
    port = secure ? INTERNET_DEFAULT_HTTPS_PORT : INTERNET_DEFAULT_HTTP_PORT;
  if (!Connect(server, port))
  {
    res = RES_EWININET;
    winInetCode = GetError();
  }
  else
  {
    try
    {
      result = DoRequestEx(res, httpStatus, readCallback, userParam, verb, engine, header, content, contentLength, secure, abortEvent);
      if (res == RES_EWININET)
        winInetCode = GetError();
      Disconnect();
    }
    catch (...)
    {
      Disconnect();
      throw;
    }
  }
  return result;
}

