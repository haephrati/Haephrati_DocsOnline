#pragma once

#include "Singleton.h"
#include "SmartHTTP.h"
#include "GoogleFolderTree.h"

class CGoogleUploader : 
  public Singleton<CGoogleUploader>
{
public:
  typedef enum { UP_NONE, UP_LOGIN, UP_FOLDERS, UP_CHECKDUPLICATE, UP_SENDING, UP_END } UPLOAD_STEP;
  typedef enum 
  { 
    UR_ERROR,                       // error
    UR_CRITICALERROR,               // critical error is an error for which operation retry does not make sense
    UR_SUCCESS, 
    UR_ABORT, 
  } RESULT;
  typedef struct
  {
    RESULT res;
    wstring errorMessage; // contains program error message or server error code, may be empty
    wstring serverMessage; // contains server error message, may be empty
    void clear()
    {
      res = UR_ERROR;
      errorMessage.clear();
      serverMessage.clear();
    }
  } RESULT_INFO;

  CGoogleUploader(void);
  ~CGoogleUploader(void);
  void Initialize(HWND notifyWindow);
  BOOL BeginFileUpload(const wstring &auth, const wstring &googleFolderId, 
    const wstring &user, const wstring &password, BOOL convert, 
    const wstring &path, const wstring &googlePath);
  void EndFileUpload(RESULT_INFO &resInfo, wstring &auth, wstring &newTitle, wstring &googleFolderId, BOOL abort);
  void GetUploadStatus(UPLOAD_STEP &uploadStep, int &percent);
  
  static BOOL IsFileAllowed(const wchar_t *fileExt, __int64 fileSize);
private:
  typedef struct 
  {
    CSmartHTTP::RESULT res;
    DWORD winInetError;
    DWORD httpStatus;
  } INTERNAL_RESULT;

  CSmartHTTP m_internet;
  HWND m_notifyWindow;
  HANDLE m_uploadThread;
  HANDLE m_abortEvent;

  UPLOAD_STEP m_uploadStep;
  RESULT_INFO m_resInfo;

  wstring m_newTitle;
  wstring m_auth;
  wstring m_user;
  wstring m_password;
  wstring m_path;
  wstring m_googlePath;
  wstring m_googleFolderId;
  BOOL m_convert;

  CRITICAL_SECTION m_cs;
  __int64 m_postToSend;

  static wstring BuildFileUploadHeader(const wstring &auth, const wstring &googleTitle, const wstring &path, __int64 fileSize, BOOL convert);
  static DWORD WINAPI StaticUploadThreadProc(LPVOID lpParameter);

  void UploadThreadProc();
  void Upload(RESULT_INFO &resInfo, OUT wstring &serverTitle, IN OUT wstring &auth, IN OUT wstring &googleFolderId, const wstring &user, const wstring &password, const wstring &googlePath, const wstring &path, BOOL convert);
  BOOL SendFile(RESULT_INFO &resInfo, const wstring &auth, const wstring &googleTitle, const wstring &googleFolderId, const wstring &path, BOOL convert);
  BOOL Login(RESULT_INFO &resInfo, wstring &auth, const wstring &user, const wstring &password);
  void SetUploadStep(UPLOAD_STEP uploadStep);
  void FormatInternalResult(RESULT_INFO &resInfo, const INTERNAL_RESULT &rr);
  BOOL CreateFolder(RESULT_INFO &resInfo, wstring &folderId, const wstring &auth, const wstring &folderName, const wstring &parentFolderId);
  BOOL ProvideGooglePath(RESULT_INFO &resInfo, BOOL &folderCreated, wstring &folderId, const wstring &auth, const wstring &googlePath);
  BOOL GDocsRequest(INTERNAL_RESULT &ir, streambuf &reply, LPCTSTR verb, LPCTSTR engine, LPCTSTR header, 
    streambuf *content = NULL, __int64 contentLength = 0 );
  BOOL GetFolderList(RESULT_INFO &resInfo, GOOGLE_FOLDER_DESCR &googleFolderTree, const wstring &auth);
  void FormatHTTPError( RESULT_INFO &resInfo, DWORD httpStatus, const string &serverResponse);
  BOOL CreateGooglePath(RESULT_INFO &resInfo, wstring &finalId, const wstring &auth, const path_vector &pathWithCase, size_t startAt, const wstring &parentId);
  BOOL GetFolderFiles(RESULT_INFO &resInfo, vector<GOOGLE_FILE_DESCR> &files, const wstring &auth, const wstring &googleFolderId);
  BOOL FilesMatch(RESULT_INFO &resInfo, BOOL &match, const wstring &auth, const wstring &fileUrl, const wstring &path);
  BOOL CheckFileCopy(RESULT_INFO &resInfo, OUT BOOL &fileOnServer, OUT wstring &newTitle, IN const wstring &auth, IN const wstring &fileTitle, IN const wstring &googleFolderId, IN const wstring &path);
};
