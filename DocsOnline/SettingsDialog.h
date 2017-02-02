#pragma once

#include "Singleton.h"

class CSettingsDialog :
  public Singleton<CSettingsDialog>
{
public:
  wstring m_user;
  wstring m_password;
  BOOL m_premier;
  BOOL m_enableMirrorFolders;
  vector<wstring> m_mirrorFolders;
  BOOL m_mirrorFoldersChanged;
  BOOL m_autostart;
  BOOL m_rootUpload;

  CSettingsDialog(void);
  ~CSettingsDialog(void);
  int DoModal(HWND parentWindow);
  HWND GetHandle() const { return m_handle; }

  static BOOL IsAutoStartOn();
  static BOOL ChangeAutoStart(BOOL enable, const wstring &path);
private:
  HWND m_handle;
  HWND m_foldersList;
  
  static INT_PTR CALLBACK StaticDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
  INT_PTR DlgProc(UINT message, WPARAM wParam, LPARAM lParam);
  void UpdateData();
  void OnInitDialog();
  void OnGetDispInfo(NMLVDISPINFO *dispInfo);
  void OnAddFolder();
  void OnDeleteFolder();
  void CheckButton(DWORD idc, BOOL check);
  BOOL IsButtonChecked(DWORD idc);
};
