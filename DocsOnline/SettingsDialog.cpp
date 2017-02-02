/*@ Docs Online - A Google Docs Backup Applicaiotn                          */
/*                                                                          */
/*--------------------------------------------------------------------------*/
/* Written and Designed by Michael Haephrati                                */
/* COPYRIGHT ©2008 by Michael Haephrati    haephrati@gmail.com              */
/* All rights reserved.                                                     */
/* -------------------------------------------------------------------------*/
#include "StdAfx.h"
#include "SettingsDialog.h"
#include "Resource.h"
#include "Application.h"
#include "Utils.h"
#include "UIStrings.h"
#include "AppConstants.h"

#define AUTOSTART_KEY                             L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run"

//////////////////////////////////////////////////////////////////////////

CSettingsDialog::CSettingsDialog(void)
{
}

CSettingsDialog::~CSettingsDialog(void)
{
}

int CSettingsDialog::DoModal(HWND parentWindow)
{
  return DialogBox(AppInstance()->GetInstance(), MAKEINTRESOURCE(IDD_SETTINGS), parentWindow, StaticDlgProc);
}

INT_PTR CALLBACK CSettingsDialog::StaticDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
  UNREFERENCED_PARAMETER(lParam);
  if (message == WM_INITDIALOG)
    CSettingsDialog::Instance().m_handle = hDlg;
  return CSettingsDialog::Instance().DlgProc(message, wParam, lParam);
}

INT_PTR CSettingsDialog::DlgProc( UINT message, WPARAM wParam, LPARAM lParam )
{
  switch (message)
  {
  case WM_NOTIFY:
    if (((LPNMHDR) lParam)->code == LVN_GETDISPINFO)
      OnGetDispInfo((NMLVDISPINFO*)lParam);
    return (INT_PTR)TRUE;
  case WM_INITDIALOG:
    OnInitDialog();
    return (INT_PTR)TRUE;
  case WM_COMMAND:
    switch (LOWORD(wParam))
    {
    case IDOK:
    case IDCANCEL:
      if (LOWORD(wParam) == IDOK)
        UpdateData();
      EndDialog(GetHandle(), LOWORD(wParam));
      return (INT_PTR)TRUE;
    case IDC_ADDFOLDER:
      OnAddFolder();
      return (INT_PTR)TRUE;
    case IDC_DELETEFOLDER:
      OnDeleteFolder();
      return (INT_PTR)TRUE;
    }
    break;
  }
  return (INT_PTR)FALSE;
}

void CSettingsDialog::CheckButton(DWORD idc, BOOL check)
{
  CheckDlgButton(GetHandle(), idc, check ? BST_CHECKED : BST_UNCHECKED);
}

void CSettingsDialog::OnInitDialog()
{
  LVCOLUMN lvc;
  wstring text;
  RECT r;
  LV_ITEM lvi;
  size_t i;

  m_foldersList = GetDlgItem(GetHandle(), IDC_FOLDERS);
  ListView_SetExtendedListViewStyle(m_foldersList, LVS_EX_FULLROWSELECT | LVS_EX_INFOTIP);
  lvc.mask = LVCF_FMT | LVCF_WIDTH;
  lvc.fmt = LVCFMT_LEFT;
  GetClientRect(m_foldersList, &r);
  lvc.cx = r.right - GetSystemMetrics(SM_CXVSCROLL);
  ListView_InsertColumn(m_foldersList, 0, &lvc);

  win::SetDlgItemText(GetHandle(), IDC_USERNAME, m_user);
  win::SetDlgItemText(GetHandle(), IDC_PASSWORD, m_password);
  CheckButton(IDC_PREMIER, m_premier);
  CheckButton(IDC_AUTOSTART, m_autostart);
  CheckButton(IDC_MIRRORFOLDERS, m_enableMirrorFolders);
  CheckButton(IDC_ROOTUPLOAD, m_rootUpload);
  CheckButton(IDC_RECREATESTRUCT, !m_rootUpload);

  lvi.mask = LVIF_TEXT;
  lvi.iSubItem = 0;
  lvi.pszText = LPSTR_TEXTCALLBACK;
  for (i = 0; i < m_mirrorFolders.size(); i++)
  {
    lvi.iItem = i;
    ListView_InsertItem(m_foldersList, &lvi);
  }
  m_mirrorFoldersChanged = FALSE;
  if (!m_user.empty() && m_password.empty())
    SetFocus(GetDlgItem(GetHandle(), IDC_PASSWORD));
}

BOOL CSettingsDialog::IsButtonChecked(DWORD idc)
{
  return IsDlgButtonChecked(GetHandle(), idc) == BST_CHECKED;
}

void CSettingsDialog::UpdateData()
{
  win::GetDlgItemText(m_user, GetHandle(), IDC_USERNAME);
  win::GetDlgItemText(m_password, GetHandle(), IDC_PASSWORD);
  m_premier = IsButtonChecked(IDC_PREMIER);
  m_autostart = IsButtonChecked(IDC_AUTOSTART);
  m_enableMirrorFolders = IsButtonChecked(IDC_MIRRORFOLDERS);
  m_rootUpload = IsButtonChecked(IDC_ROOTUPLOAD);
}

void CSettingsDialog::OnGetDispInfo(NMLVDISPINFO *dispInfo)
{
  ASSERT(dispInfo->item.iSubItem == 0);
  if (dispInfo->item.iItem >= 0 && dispInfo->item.iItem < (int)m_mirrorFolders.size())
    dispInfo->item.pszText = (LPWSTR)m_mirrorFolders[dispInfo->item.iItem].c_str();
}

void CSettingsDialog::OnAddFolder()
{
  LV_ITEM lvi;
  wstring path;

  if (MyBrowseForFolder(path, GetHandle(), UIFormat(msg_selectfolder).c_str(), 
    BIF_NEWDIALOGSTYLE | BIF_NONEWFOLDERBUTTON | BIF_RETURNFSANCESTORS) && !path.empty())
  {
    if (path[path.length()-1] == '\\')
      path.resize(path.length()-1);
    m_mirrorFoldersChanged = TRUE;
    lvi.iItem = ListView_GetSelectionMark(m_foldersList);
    if (lvi.iItem < 0)
      lvi.iItem = m_mirrorFolders.size();
    else
      lvi.iItem++;
    m_mirrorFolders.insert(m_mirrorFolders.begin() + lvi.iItem, path);
    lvi.mask = LVIF_TEXT;
    lvi.iSubItem = 0;
    lvi.pszText = LPSTR_TEXTCALLBACK;
    ListView_InsertItem(m_foldersList, &lvi);
  }
}

void CSettingsDialog::OnDeleteFolder()
{
  int i;

  i = ListView_GetSelectionMark(m_foldersList);
  if (i >= 0)
  {
    ASSERT(i < (int)m_mirrorFolders.size());
    m_mirrorFoldersChanged = TRUE;
    m_mirrorFolders.erase(m_mirrorFolders.begin() + i);
    ListView_DeleteItem(m_foldersList, i);
  }
}

BOOL CSettingsDialog::IsAutoStartOn()
{
  BOOL autostart;
  HKEY key;
  wstring path;

  autostart = FALSE;
  RegOpenKeyEx(HKEY_CURRENT_USER, AUTOSTART_KEY, 0, KEY_QUERY_VALUE, &key); 
  if (key != NULL) 
  {
    if (reg::GetString(path, key, APPLICATION_TITLE))
      if (!path.empty())
        autostart = TRUE;
    RegCloseKey(key);
  }
  return autostart;
}

BOOL CSettingsDialog::ChangeAutoStart(BOOL enable, const wstring &path)
{
  HKEY key;
  DWORD disposition;
  wchar_t *executable;
  wstring commandLine;
  BOOL result;

  result = FALSE;
  if (RegCreateKeyEx(HKEY_CURRENT_USER, AUTOSTART_KEY, 0,
    NULL, 0, KEY_CREATE_SUB_KEY | KEY_SET_VALUE, NULL, &key, &disposition) == ERROR_SUCCESS)
  {
    if (enable)
    {
      if (path.empty()) // looks like the setup did not set the Executable key value
      {
        if (_get_wpgmptr(&executable) == 0)
          commandLine = executable;
      }
      else
        commandLine = path;
      if (!commandLine.empty())
      {
        commandLine = L"\"" + commandLine + L"\"";
        commandLine += L" ";
        commandLine += AUTOSTART_PARAMETER;
        result = reg::SetString(key, APPLICATION_TITLE, commandLine.c_str());
      }
    }
    else
      result = RegDeleteValue(key, APPLICATION_TITLE) == ERROR_SUCCESS;
    RegCloseKey(key);
  }
  return result;
}
