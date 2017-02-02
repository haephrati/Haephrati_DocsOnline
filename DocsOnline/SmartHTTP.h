#pragma once

#define HTTP_CONTENT_LENGTH                 L"Content-Length"
#define HTTP_CONTENT_TYPE                   L"Content-Type"
#define HTTP_MIME_BINARY                    L"application/octet-stream"
#define HTTP_MIME_FORM_URL_ENCODED          L"application/x-www-form-urlencoded"
                                                
class CSmartHTTP
{
public:
  static const wchar_t *GET;
  static const wchar_t *POST;

  // contentLength may be -1 in case the corresponding http header is not available
  typedef size_t (*READCALLBACK)(DWORD httpStatus, __int64 contentLength, const void *buf, size_t toRead, void *userParam);

  typedef enum { RES_SUCCESS, RES_ABORT, RES_EWININET, RES_EINPUTSTREAM, RES_EOUTPUTSTREAM } RESULT;
  CSmartHTTP();
  ~CSmartHTTP(void);
  BOOL Connect(LPCTSTR szServerName, INTERNET_PORT nServerPort = INTERNET_DEFAULT_HTTP_PORT);
  void Disconnect();
  BOOL Request( RESULT &res, 
    DWORD &winInetCode, // receives last api error ( from GetError() member ) when res = RES_EWININET
    DWORD &httpStatus, streambuf &reply, 
    LPCTSTR verb, LPCTSTR server, LPCTSTR engine, 
    LPCTSTR header, 
    streambuf *content = NULL, 
    __int64 contentLength = 0, 
    BOOL secure = FALSE, // TRUE: use https, FALSE: use http
    HANDLE abortEvent = NULL, // setting this event aborts POST, can be NULL
    int portOverride = -1 // port to connect to if differs from default http / https ports
    );
  BOOL RequestEx( RESULT &res, DWORD &winInetCode, DWORD &httpStatus, READCALLBACK readCallback, void *userParam, LPCTSTR verb, LPCTSTR server, LPCTSTR engine, LPCTSTR header, 
    streambuf *content = NULL, 
    __int64 contentLength = 0, 
    BOOL secure = FALSE, // TRUE: use https, FALSE: use http
    HANDLE abortEvent = NULL, // setting this event aborts POST, can be NULL
    int portOverride = -1 // port to connect to if differs from default http / https ports
    );
  __int64 GetPostSent() { return m_postSent; }
  DWORD GetError() { return m_error; }

  static void AddHeader(wstring &header, LPCTSTR key, LPCTSTR value);
  static wstring GetMIMETypeOrBinary( const wstring extention );

private:
  static const map<wstring, wstring> m_ext2MIME;
  CRITICAL_SECTION m_cs;
  __int64 m_postSent;

  HINTERNET m_hInet;
  HINTERNET m_hConnection;
  DWORD m_error;
  HANDLE m_events[2];
  int m_eventsCount;

  static size_t ReadIntoStreambuf(DWORD httpStatus, __int64 contentLength, const void *buf, size_t toRead, void *userParam);
  static void PrepareInetBuffer(INTERNET_BUFFERS *ib);
  BOOL Initialize();
  void Uninitialize();
  void AsyncPrepare();
  BOOL AsyncEnd(RESULT &res);
  BOOL AsyncResult(RESULT &res, BOOL bAsyncResult);
  static void __stdcall InternetCallback(HINTERNET hInternet,
    DWORD dwContext,
    DWORD dwInternetStatus,
    LPVOID lpvStatusInformation,
    DWORD dwStatusInformationLength);
  BOOL SendRequest(RESULT &res, HINTERNET hRequestSession, LPCTSTR header, 
    streambuf *content = NULL, // no additional request content
    __int64 contentLength = 0);
  BOOL CheckResult(BOOL syncResult);
  BOOL IsAbort();
  BOOL ReadReplyEx(RESULT &res, DWORD httpStatus, READCALLBACK readCallback, void *userParam, HINTERNET hRequestSession);
  BOOL DoRequestEx( RESULT &res, DWORD &httpStatus, READCALLBACK readCallback, void *userParam, LPCTSTR verb, LPCTSTR engine, LPCTSTR header, streambuf *content, __int64 contentLength, BOOL secure, HANDLE abortEvent );
};
