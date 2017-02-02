#pragma once

#include "UploadList.h"

class CMainWindow
{
public:
	CMainWindow(void);
	~CMainWindow(void);
  BOOL CreateShow(const wchar_t *windowClassName, int nCmdShow);
  HWND GetHandle() { return m_handle; }

  void OnDropFiles(HDROP drop);

private:
  HWND m_handle;
  unsigned int m_queueFilesMessage;
  CUploadList m_uploadList;
  CSystemTray m_trayIcon;
  BOOL m_uploading;
  HIMAGELIST m_toolbarImages;
  HWND m_toolbar;
  size_t m_currentFile; // index of file being uploaded or the next to upload, the one with GetPos() = 1
  int m_currentRetry;
  int m_countErrors;
  int m_countSuccess;
  wstring m_hintConvertDocsButton;

  // cache for result of folder creation, used if the next file is in the same folder as previously uploaded
  wstring m_googleFolderId; 
  wstring m_saveGooglePath;

  static LRESULT CALLBACK StaticWindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
  static BOOL QueueScanCallback(void *param, const wstring &fileName);
  LRESULT WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
  void OnAfterCreate();
  int SettingsDlg();
  void OnDestroy();
  void QueueFiles(const vector<wstring> &fileNames);
  void OnSize();
  void OnExit();
  void StartUpload(size_t fileToUpload);
  void EndFileUpload( BOOL abort );
  void AbortUpload();
  void SetCurrentStatus(CUploadItem::ITEMSTATE state, const wstring &msg);
  wstring GetFormattedUploadStatus();
  void OnTimer(WPARAM timerId);
  void BeforeUploadSeriesStart();
  void AfterUploadSeriesEnd(BOOL showMessage = TRUE);
  void CreateToolbar();
  BOOL SetToolbarButton(DWORD buttonId, const wstring &text, int image = -1);
  void ClearFilesLV();
  void OnQueueFiles();
  void OnDelayedQueueFiles(WPARAM wParam, LPARAM lParam);
  BOOL ProvideNoUploadState(const wchar_t *uploadInProgressWarning);
  void OnClearList();
  void UpdateUploadButton();
  BOOL QueueFile(const wstring &path, size_t googlePathOffset);
  LRESULT OnNotify(NMHDR *pnm);
  BOOL OnContextMenu(HWND window, int x, int y);
  BOOL OnCommand(DWORD id, DWORD event);
  void SetConvertDocs(BOOL premier, BOOL checked);
  BOOL GetConvertDocs();
  void UpdateTitle();
  void OnUploadRetry();
  void OnClose();
  void OnFirstStart();
};

CMainWindow *AppWindow();