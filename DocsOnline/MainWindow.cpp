/*@ Docs Online - A Google Docs Backup Applicaiotn                          */
/*                                                                          */
/*--------------------------------------------------------------------------*/
/* Written and Designed by Michael Haephrati                                */
/* COPYRIGHT ©2008 by Michael Haephrati    haephrati@gmail.com              */
/* All rights reserved.                                                     */
/* -------------------------------------------------------------------------*/
#include "StdAfx.h"
#include "MainWindow.h"
#include "AppConstants.h"
#include "Application.h"
#include "Resource.h"
#include "GoogleUploader.h"
#include "Utils.h"
#include "SettingsDialog.h"
#include "AppSettings.h"
#include "UIStrings.h"
#include "..\common\common.h"

//////////////////////////////////////////////////////////////////////////

// Window
#define DEF_WINDOWWIDTH                   550
#define DEF_WINDOWHEIGHT                  300

// Toolbar
#define TOOLBAR_HEIGHT                    28
#define TOOLBAR_IUPLOAD                   0
#define TOOLBAR_ISTOP                     1
#define TOOLBAR_ICLEARLIST                2
#define TOOLBAR_ISETTINGS                 3
#define TOOLBAR_IEXIT                     4

// Misc
#define UPDATESTATUS_TIMERID              12345
#define UPDATESTATUS_INTERVAL             250

//////////////////////////////////////////////////////////////////////////

CMainWindow *AppWindow()
{
  static CMainWindow mainWindow;
  return &mainWindow;
}

CMainWindow::CMainWindow(void)
{
  m_handle = NULL;
}

CMainWindow::~CMainWindow(void)
{
}

BOOL CMainWindow::CreateShow(const wchar_t *windowClassName, int nCmdShow)
{
  m_queueFilesMessage = RegisterWindowMessage(MESSAGE_QUEUE_FILES);

  WNDCLASSEX wcex;

  wcex.cbSize = sizeof(WNDCLASSEX);
  wcex.style			= CS_HREDRAW | CS_VREDRAW;
  wcex.lpfnWndProc	= StaticWindowProc;
  wcex.cbClsExtra		= 0;
  wcex.cbWndExtra		= 0;
  wcex.hInstance		= AppInstance()->GetInstance();
  wcex.hIcon			  = LoadIcon(AppInstance()->GetResInstance(), MAKEINTRESOURCE(IDI_DOCSONLINE));
  wcex.hCursor		  = LoadCursor(NULL, IDC_ARROW);
  wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
  wcex.lpszMenuName	= NULL;
  wcex.lpszClassName	= windowClassName;
  wcex.hIconSm		  = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));
  RegisterClassEx(&wcex);

  m_handle = CreateWindow(windowClassName, APPLICATION_TITLE, WS_OVERLAPPEDWINDOW,
    CW_USEDEFAULT, CW_USEDEFAULT, DEF_WINDOWWIDTH, DEF_WINDOWHEIGHT, NULL, NULL, AppInstance()->GetInstance(), NULL);
  if (m_handle)
  {
    OnAfterCreate();
    ShowWindow(m_handle, nCmdShow);
    UpdateWindow(m_handle);
  }
  return (m_handle != NULL);
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND	- process the application menu
//  WM_PAINT	- Paint the main window
//  WM_DESTROY	- post a quit message and return
//
//
LRESULT CALLBACK CMainWindow::StaticWindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  return AppWindow()->WindowProc(hWnd, message, wParam, lParam);
}

void CMainWindow::UpdateTitle()
{
  wstring title;
  
  title = APPLICATION_TITLE;
  title += L" - ";
  title += AppSettings()->m_googleUser;
  SetWindowText(GetHandle(), title.c_str());
}

void CMainWindow::OnFirstStart()
{
  TCHAR myDocumentsPath[MAX_PATH];
  if (AppSettings()->m_mirrorFolders.size() == 0)
  {
    if ( SUCCEEDED(SHGetSpecialFolderPath(NULL, myDocumentsPath, CSIDL_PERSONAL, FALSE)) ) 
    {
      AppSettings()->m_mirrorFolders.push_back(myDocumentsPath);
      AppSettings()->SaveMirrorFolders();
    }
  }
}

void CMainWindow::OnAfterCreate()
{
  CGoogleUploader::Instance().Initialize(GetHandle());
  m_uploading = FALSE;
  DragAcceptFiles(GetHandle(), TRUE);
  // Create the tray icon
  m_trayIcon.Create(AppInstance()->GetInstance(),
    GetHandle(),                            // Parent window
    AM_ICON_NOTIFY,                  // Icon notify message to use
    APPLICATION_TITLE,  // tooltip
    ::LoadIcon(AppInstance()->GetResInstance(), (LPCTSTR)IDI_SMALL),
    IDR_APP_POPUP_MENU);
  UpdateTitle();
  CreateToolbar();
  m_currentFile = CUploadList::npos;
  m_uploadList.Create(GetHandle());
  if (AppSettings()->m_firstStart)
    OnFirstStart();
}

BOOL CMainWindow::SetToolbarButton(DWORD buttonId, const wstring &text, int image) 
{
  TBBUTTONINFO tbi;

  tbi.cbSize = sizeof(TBBUTTONINFO);
  tbi.dwMask = TBIF_TEXT;
  if (image != -1)
  {
    tbi.dwMask |= TBIF_IMAGE;
    tbi.iImage = image;
  }
  tbi.pszText = (LPWSTR)text.c_str();
  tbi.cchText = 0;
  return SendMessage(m_toolbar, TB_SETBUTTONINFO, (WPARAM) buttonId, (LPARAM)&tbi); 
}

void CMainWindow::SetConvertDocs(BOOL premier, BOOL checked)
{
  TBBUTTONINFO tbi;

  if (!premier)
    checked = TRUE;
  ASSERT(premier || (checked == TRUE));
  tbi.cbSize = sizeof(TBBUTTONINFO);
  tbi.dwMask = TBIF_STATE;
  tbi.fsState = TBSTATE_ENABLED; // disabled can't display pressed state; commented out: (premier ? TBSTATE_ENABLED : 0);
  tbi.fsState |= checked ? TBSTATE_CHECKED : 0;
  SendMessage(m_toolbar, TB_SETBUTTONINFO, ID_CONVERTDOCS, (LPARAM)&tbi); 
}

BOOL CMainWindow::GetConvertDocs()
{
  TBBUTTONINFO tbi;

  tbi.cbSize = sizeof(TBBUTTONINFO);
  tbi.dwMask = TBIF_STATE;
  SendMessage(m_toolbar, TB_GETBUTTONINFO, ID_CONVERTDOCS, (LPARAM)&tbi);
  return (tbi.fsState & TBSTATE_CHECKED) != 0;
}

void CMainWindow::CreateToolbar()
{
  TBBUTTON tbButtons[] = 
  {
    { TOOLBAR_IUPLOAD, ID_STARTSTOP, 0, BTNS_BUTTON | BTNS_AUTOSIZE, 0L, 0},
    { TOOLBAR_ICLEARLIST, ID_CLEARLIST, TBSTATE_ENABLED, BTNS_BUTTON | BTNS_AUTOSIZE, 0L, 0},
    { TOOLBAR_ISETTINGS, ID_SETTINGS, TBSTATE_ENABLED, BTNS_BUTTON | BTNS_AUTOSIZE, 0L, 0},
    { 0, 0, TBSTATE_ENABLED, TBSTYLE_SEP, 0L, -1},
    { I_IMAGENONE, ID_CONVERTDOCS, TBSTATE_ENABLED | TBSTATE_CHECKED, BTNS_CHECK | BTNS_AUTOSIZE, 0L, 0},
    { 0, 0, TBSTATE_ENABLED, TBSTYLE_SEP, 0L, -1},
    { TOOLBAR_IEXIT, IDM_EXIT, TBSTATE_ENABLED, BTNS_BUTTON | BTNS_AUTOSIZE, 0L, 0},
  };

  m_toolbar = CreateWindowEx(0, TOOLBARCLASSNAME, L"", 
    WS_CHILD | WS_VISIBLE | TBSTYLE_FLAT | TBSTYLE_LIST | TBSTYLE_TRANSPARENT | TBSTYLE_TOOLTIPS,
    0, 0, 100, TOOLBAR_HEIGHT,           
    GetHandle(),              
    (HMENU)0,                
    AppInstance()->GetInstance(),
    NULL); 

  m_toolbarImages = ImageList_LoadImage(AppInstance()->GetResInstance(), (LPCWSTR)IDB_MAINTB, 
    15, 1, CLR_DEFAULT, IMAGE_BITMAP, LR_CREATEDIBSECTION);

  SendMessage(m_toolbar, TB_SETIMAGELIST, 0, (LPARAM)m_toolbarImages);
  SendMessage(m_toolbar, TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);
  SendMessage(m_toolbar, TB_SETBUTTONSIZE, 0, (LPARAM)MAKELONG(100, 0)); 
  SendMessage(m_toolbar, TB_ADDBUTTONS, (WPARAM) ARRAYDIM(tbButtons), (LPARAM) tbButtons); 
  
  SetToolbarButton(ID_STARTSTOP, UIFormat(btn_upload));
  SetToolbarButton(ID_CLEARLIST, UIFormat(btn_clearlist));
  SetToolbarButton(ID_SETTINGS, UIFormat(btn_settings));
  SetToolbarButton(ID_CONVERTDOCS, UIFormat(chk_convertdocs));
  SetToolbarButton(IDM_EXIT, UIFormat(btn_exit));

  SetConvertDocs(AppSettings()->m_googlePremier, AppSettings()->m_convertDocs);
  m_hintConvertDocsButton = UIFormat(hint_convertdocs);
}


void CMainWindow::OnDestroy()
{
  AppSettings()->m_convertDocs = GetConvertDocs();
  AppSettings()->SaveOnExit();
  PostQuitMessage(0);
}

LRESULT CMainWindow::OnNotify(NMHDR *pnm)
{
  LPTOOLTIPTEXT toolTipText;

  if (pnm->hwndFrom == m_uploadList.GetHandle())
  {
    switch (pnm->code)
    {
    case LVN_GETDISPINFO:
      m_uploadList.OnGetDispInfo((NMLVDISPINFO*) pnm);
      break;
    case LVN_COLUMNCLICK:
      m_uploadList.OnColumnClick(((NMLISTVIEW*)pnm)->iSubItem, m_currentFile);
      break;
    default:
      //OutputDebugString(str::Format1K(L"code: %d\n", pnm->code+100).c_str());
      break;
    }
  }
  else 
  {
    switch (pnm->code)
    {
    case TTN_GETDISPINFO:
      toolTipText = (LPTOOLTIPTEXT)pnm;
      if (toolTipText->hdr.idFrom == ID_CONVERTDOCS)
      {
        toolTipText->lpszText = (LPWSTR)m_hintConvertDocsButton.c_str();
        toolTipText->hinst = NULL;
      }
      break;
    default:
      //OutputDebugString(str::Format1K(L"code: %d\n", pnm->code+100).c_str());
      break;
    }
  }
  return 0;
}

BOOL CMainWindow::OnContextMenu( HWND window, int x, int y )
{
  if (window == m_uploadList.GetHandle())
  {
    m_uploadList.OnContextMenu(x, y);
    return TRUE;
  }
  return FALSE;
}

BOOL CMainWindow::OnCommand(DWORD id, DWORD event)
{
  size_t fileToUpload;

  switch (id)
  {
  case ID_STARTSTOP:
    if (m_uploading)
      AbortUpload();
    else if (m_uploadList.FindPos1(fileToUpload))
    {
      BeforeUploadSeriesStart();
      StartUpload(fileToUpload);
    }
    break;
  case ID_CLEARLIST:
    OnClearList();
    break;
  case ID_SETTINGS:
    SettingsDlg();
    break;
  case IDM_EXIT:
    OnExit();
    break;
  case IDM_UPLOADREMOVE:
    m_uploadList.RemoveSelectedItems(m_currentFile);
    break;
  case IDM_UPLOADRETRY:
    OnUploadRetry();
    break;
  case ID_CONVERTDOCS:
    if (!AppSettings()->m_googlePremier)
      SetConvertDocs(FALSE, TRUE); // keep checked for non-premium accounts
    break;
  default:
    return FALSE;
  }
  return TRUE;
}

LRESULT CMainWindow::WindowProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
{
  switch (message)
  {
  case WM_CONTEXTMENU:
    if (!OnContextMenu((HWND)wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)))
      return DefWindowProc(hWnd, message, wParam, lParam);
    break;
  case WM_NOTIFY:
    return OnNotify((NMHDR*)lParam);
  case AM_ICON_NOTIFY:
    return m_trayIcon.OnTrayNotification(wParam, lParam);
  case AM_FILE_UPLOAD_END:
    EndFileUpload(FALSE);
    break;
  case AM_DELAYED_QUEUEFILES:
    OnDelayedQueueFiles(wParam, lParam);
    break;
  case WM_TIMER:
    OnTimer(wParam);
    break;
  case WM_DESTROY:
    OnDestroy();
    break;
  case WM_DROPFILES:
    OnDropFiles((HDROP)wParam);
    break;
  case WM_CLOSE:
    OnClose();
    break;
  case WM_SIZE:
    OnSize();
    break;
  case WM_COMMAND:
    if (!OnCommand(LOWORD(wParam), HIWORD(wParam)))
      return DefWindowProc(hWnd, message, wParam, lParam);
    break;
  default:
    if (message == m_queueFilesMessage)
    {
      OnQueueFiles();
      break;
    }
    return DefWindowProc(hWnd, message, wParam, lParam);
  }
  return 0;
}

void CMainWindow::OnClose()
{
  ShowWindow(GetHandle(), SW_HIDE);
  if (!AppSettings()->m_minimizeBalloonShowed)
  {
    if (m_trayIcon.ShowBalloon(UIFormat(msg_uploaderkeepsworking).c_str(), APPLICATION_TITLE, NIIF_INFO))
      AppSettings()->m_minimizeBalloonShowed = TRUE;
  }
}

BOOL CMainWindow::ProvideNoUploadState(const wchar_t *uploadInProgressWarning)
{
  BOOL result;

  if (!m_uploading)
    result = TRUE;
  else
  {
    result = (MessageBox(GetHandle(), uploadInProgressWarning, APPLICATION_TITLE, MB_ICONWARNING | MB_YESNOCANCEL) == IDYES);
    if (result)
      AbortUpload();
  }
  return result;
}

void CMainWindow::OnClearList()
{
  if (ProvideNoUploadState(UIFormat(msg_clearlistinupload).c_str()))
  {
    ASSERT(!m_uploading);
    ClearFilesLV();
  }
}

//////////////////////////////////////////////////////////////////////////
// Reads content of a memory block from the shell extension and returns 
// immediately to avoid explorer hang up. The block is handled later on
// posted AM_DELAYED_QUEUEFILES message

void CMainWindow::OnQueueFiles()
{
  BOOL result;
  HANDLE fileMapping;
  LPVOID fileView;
  MEMORY_MAPPED_INFO *queueInfo;
  vector<wchar_t> *filesList;

  result = FALSE;
  fileMapping = OpenFileMapping(FILE_MAP_READ, FALSE, QUEUE_MEMORY_FILE_NAME);
  if (fileMapping != NULL)
  {
    fileView = MapViewOfFile(fileMapping, FILE_MAP_READ, 0, 0, 0);
    if (fileView != NULL)
    {
      result = TRUE;
      queueInfo = new MEMORY_MAPPED_INFO;
      *queueInfo = *((MEMORY_MAPPED_INFO*)fileView);
      filesList = new vector<wchar_t>();
      filesList->resize(queueInfo->listChars);
      memcpy(vector_ptr(*filesList), (LPBYTE)fileView + sizeof(MEMORY_MAPPED_INFO), vector_bytes(*filesList));
      PostMessage(GetHandle(), AM_DELAYED_QUEUEFILES, (WPARAM)queueInfo, (LPARAM)filesList);
      UnmapViewOfFile(fileView);
    }
    CloseHandle(fileMapping);
  }
}

void CMainWindow::OnDelayedQueueFiles(WPARAM wParam, LPARAM lParam)
{
  MEMORY_MAPPED_INFO *queueInfo;
  vector<wchar_t> *filesList;
  vector<wstring> files;
  wstring fileName;
  size_t off;
  size_t maxChars;

  queueInfo = (MEMORY_MAPPED_INFO*)wParam;
  filesList = (vector<wchar_t>*)lParam;

  off = 0;
  while (off < filesList->size())
  {
    maxChars = filesList->size() - off;
    str::AssignSZ(fileName, const_vector_ptr(*filesList) + off, maxChars);
    files.push_back(fileName);
    off += fileName.size() + 1; // move to the next string stored
  }

  delete queueInfo;
  delete filesList;

  if (files.size() > 0)
    QueueFiles(files);
}

void CMainWindow::OnSize()
{
  RECT rect;

  GetClientRect(GetHandle(), &rect);
  MoveWindow(m_toolbar, 0, 0, rect.right, TOOLBAR_HEIGHT, TRUE);
  MoveWindow(m_uploadList.GetHandle(), 0, TOOLBAR_HEIGHT, rect.right, rect.bottom-TOOLBAR_HEIGHT, TRUE);
}

void CMainWindow::OnDropFiles(HDROP drop)
{
  unsigned int i;
  unsigned int count;
  vector<wstring> files;

  CBuffer<wchar_t> buffer(MAX_PATH + 1);
  count = DragQueryFile(drop, (UINT)-1, NULL, 0);
  for (i = 0; i < count; i++)
  {
    buffer.ProvideCapacity(DragQueryFile(drop, i, NULL, 0) + 1);
    if (DragQueryFile(drop, i, buffer, buffer.Size()) > 0)
    {
      files.push_back(wstring(buffer));
    }
  }
  if (files.size() > 0)
    QueueFiles(files);
}

int CMainWindow::SettingsDlg()
{
  int result;
  CSettingsDialog& settingsDlg = CSettingsDialog::Instance();
  BOOL autostart;

  autostart = CSettingsDialog::IsAutoStartOn();
  settingsDlg.m_user = AppSettings()->m_googleUser;
  settingsDlg.m_password = AppSettings()->m_googlePassword;
  settingsDlg.m_premier = AppSettings()->m_googlePremier;
  settingsDlg.m_autostart = autostart;
  settingsDlg.m_enableMirrorFolders = AppSettings()->m_enableMirrorFolders;
  settingsDlg.m_mirrorFolders = AppSettings()->m_mirrorFolders;
  settingsDlg.m_rootUpload = AppSettings()->m_rootUpload;
  result = settingsDlg.DoModal(GetHandle());
  if (result == IDOK)
  {
    if (settingsDlg.m_user != AppSettings()->m_googleUser)
    {
      AppSettings()->m_googleUser = settingsDlg.m_user;
      AppSettings()->m_googleAuth.clear();
      UpdateTitle();
    }
    if (!AppSettings()->m_googlePremier && settingsDlg.m_premier) // switch to premier from free
      SetConvertDocs(settingsDlg.m_premier, FALSE); // set no for docs conversation
    else
      SetConvertDocs(settingsDlg.m_premier, GetConvertDocs());
    if (autostart != settingsDlg.m_autostart)
      CSettingsDialog::ChangeAutoStart(settingsDlg.m_autostart, AppSettings()->m_installExecutable);
    AppSettings()->m_googlePassword = settingsDlg.m_password;
    AppSettings()->m_googlePremier = settingsDlg.m_premier;
    AppSettings()->m_enableMirrorFolders = settingsDlg.m_enableMirrorFolders;
    AppSettings()->m_rootUpload = settingsDlg.m_rootUpload;
    AppSettings()->SaveGeneralSettings();
    if (settingsDlg.m_mirrorFoldersChanged)
    {
      AppSettings()->m_mirrorFolders = settingsDlg.m_mirrorFolders;
      AppSettings()->SaveMirrorFolders();
    }
  }
  return result;
}

BOOL CMainWindow::QueueFile(const wstring &path, size_t googlePathOffset)
{
  wstring fileNameOnly;
  __int64 fileSize;
  wstring sizeString;
  wstring fileExt;
  BOOL result;

  result = FALSE;
  if (!GetFileSizeByName(fileSize, path.c_str()))
    fileSize = -1;
  ExtractFileExt(fileExt, path);
  if (AppSettings()->m_googlePremier || CGoogleUploader::IsFileAllowed(fileExt.c_str(), fileSize))
  {
    result = TRUE;
    m_uploadList.AddFile(path, googlePathOffset, fileSize);
  }
  return result;
}

typedef struct
{
  size_t googlePathOffset;
  int skippedCount;
} QUEUING_DATA;

BOOL CMainWindow::QueueScanCallback(void *param, const wstring &fileName)
{
  if (!AppWindow()->QueueFile(fileName, ((QUEUING_DATA*)param)->googlePathOffset))
    ((QUEUING_DATA*)param)->skippedCount++;
  return TRUE;
}

void CMainWindow::QueueFiles(const vector<wstring> &pathes)
{
  int modalResult;
  int queuedCount;
  vector<wstring>::const_iterator iter;
  wstring message;
  DWORD icon;
  BOOL listChanged;
  size_t fileToUpload;
  DWORD fileAttr;
  wstring topFolder;
  QUEUING_DATA queuingData;

  listChanged = FALSE;
  if (!m_uploading)
  {
    ClearFilesLV();
    listChanged = TRUE;
  }
  if (pathes.size() == 0)
    return;
  iter = pathes.begin();
  ExtractFilePath(topFolder, *iter);
  queuingData.googlePathOffset = topFolder.length();
  if (queuingData.googlePathOffset > 0)
    queuingData.googlePathOffset++; // to advance over '\'
  queuingData.skippedCount = 0;
  m_uploadList.AddFilesBegin();
  for (; iter != pathes.end(); iter++)
  {
    fileAttr = GetFileAttributes(iter->c_str());
    if (fileAttr == INVALID_FILE_ATTRIBUTES)
      queuingData.skippedCount++;
    else if ((fileAttr & FILE_ATTRIBUTE_DIRECTORY) != 0)
      ScanPath(iter->c_str(), QueueScanCallback, &queuingData); // a directory is scanned for files
    else
    { // a normal file
      if (!QueueFile(*iter, queuingData.googlePathOffset))
        queuingData.skippedCount++;
    }
  }
  queuedCount = m_uploadList.AddFilesEnd(m_currentFile);
  int skippedCount = queuingData.skippedCount;
  if (queuedCount == 0)
  {
    icon = NIIF_WARNING;
    message = UIFormat(msg_nonequeued, str::IntToStr(skippedCount).c_str());
  }
  else if (skippedCount > 0)
  {
    icon = NIIF_WARNING;
    message = UIFormat(msg_xfilesofyqueued, str::IntToStr(queuedCount).c_str(),
      str::IntToStr(skippedCount + queuedCount).c_str(),
      str::IntToStr(skippedCount).c_str());
  }
  else
  {
    icon = NIIF_INFO;
    message = UIFormat(msg_xfilesqueued, str::IntToStr(queuedCount).c_str());
  }
  m_trayIcon.ShowBalloon(message.c_str(), APPLICATION_TITLE, icon);
  if (queuedCount > 0)
  {
    modalResult = IDOK;
    if (AppSettings()->m_googleUser.empty() || AppSettings()->m_googlePassword.empty())
    {
      modalResult = SettingsDlg();
    }
    if (modalResult == IDOK)
    {
      if (!m_uploading && m_uploadList.FindPos1(fileToUpload))
      {
        BeforeUploadSeriesStart();
        StartUpload(fileToUpload);
      }
      else
        UpdateUploadButton();
    }
  }
}

void CMainWindow::UpdateUploadButton()
{
  BOOL enableUpload;

  if (m_uploading)
  {
    SetToolbarButton(ID_STARTSTOP, UIFormat(btn_stop), TOOLBAR_ISTOP);
    enableUpload = TRUE;
  }
  else
  {
    SetToolbarButton(ID_STARTSTOP, UIFormat(btn_upload), TOOLBAR_IUPLOAD);
    enableUpload = m_currentFile != CUploadList::npos;
  }
  SendMessage(m_toolbar, TB_ENABLEBUTTON, ID_STARTSTOP, MAKELONG(enableUpload, 0));
}

void CMainWindow::OnExit()
{
  if (ProvideNoUploadState(UIFormat(msg_exitinupload).c_str()))
  {
    ASSERT(!m_uploading);
    DestroyWindow(GetHandle());
  }
}

void CMainWindow::ClearFilesLV()
{
  ASSERT(!m_uploading);
  m_uploadList.Clear();
  m_currentFile = CUploadList::npos;
  UpdateUploadButton();
}

void CMainWindow::StartUpload(size_t fileToUpload)
{
  wstring path;
  size_t googlePathOffset;
  wstring googlePath;
  BOOL inMirrorFolder;
  size_t maxLen;

  ASSERT(!m_uploading);
  m_currentFile = fileToUpload;
  if (fileToUpload != CUploadList::npos)
  {
    SetCurrentStatus(CUploadItem::ACTIVE, str::EmptyString());
    m_uploadList.GetUploadItemParams(m_currentFile, path, googlePathOffset);
    inMirrorFolder = FALSE;
    if (AppSettings()->m_enableMirrorFolders)
    {
      vector<wstring>::const_iterator iter;
      maxLen = 0;
      for (iter = AppSettings()->m_mirrorFolders.begin(); iter != AppSettings()->m_mirrorFolders.end(); iter++)
      {
        size_t len = iter->length();
        if (len > maxLen && path.length() > len && path[len] == '\\' && str::EqualI(path.substr(0, len), *iter))
          maxLen = len;
      }
      if (maxLen > 0)
      {
        inMirrorFolder = TRUE;
        googlePathOffset = path.rfind('\\', maxLen-1);
        if (googlePathOffset == wstring::npos)
          googlePathOffset = 0;
        else
          googlePathOffset++;
      }
    }
    if (inMirrorFolder || !AppSettings()->m_rootUpload)
    {
      // take into account the googlePathOffset 
      wstring pathOnly;
      ExtractFilePath(pathOnly, path);
      if (googlePathOffset < pathOnly.length())
        googlePath = pathOnly.substr(googlePathOffset);
    }
    if (m_saveGooglePath.empty() || !str::EqualI(googlePath, m_saveGooglePath))
      m_googleFolderId.clear(); // clear folder id if path changed
    m_saveGooglePath = googlePath;
    if (CGoogleUploader::Instance().BeginFileUpload(AppSettings()->m_googleAuth, m_googleFolderId,
      AppSettings()->m_googleUser, AppSettings()->m_googlePassword, GetConvertDocs(), 
      path, googlePath))
    {
      m_uploading = TRUE;
      SetTimer(GetHandle(), UPDATESTATUS_TIMERID, UPDATESTATUS_INTERVAL, NULL);
    }
    else
    {
      m_countErrors++;
      SetCurrentStatus(CUploadItem::ITEMERROR, UIFormat(msg_outofresources));
      AfterUploadSeriesEnd();
    }
  }
  else
  {
    ASSERT(FALSE);
  }
  UpdateUploadButton();
}

void CMainWindow::AbortUpload()
{
  if (m_uploading)
  {
    EndFileUpload(TRUE);
  }
}

wstring FormatUploadStatus(CGoogleUploader::UPLOAD_STEP uploadStep, int percent)
{
  wstring result;

  switch (uploadStep)
  {
  case CGoogleUploader::UP_NONE:
    return str::EmptyString();
  case CGoogleUploader::UP_LOGIN:
    return UIFormat(msg_authorizing);
  case CGoogleUploader::UP_FOLDERS:
    return UIFormat(msg_processingfolders);
  case CGoogleUploader::UP_CHECKDUPLICATE:
    return UIFormat(msg_checkingforfilecopy);
  case CGoogleUploader::UP_SENDING:
    return percent == 0 ? UIFormat(msg_uploading) : 
      UIFormat(msg_uploadingprogress, str::IntToStr(percent).c_str());
  default:
    ASSERT(FALSE);
    return str::EmptyString();
  }
}

wstring CMainWindow::GetFormattedUploadStatus()
{
  CGoogleUploader::UPLOAD_STEP uploadStep;
  int percent;

  CGoogleUploader::Instance().GetUploadStatus(uploadStep, percent);
  return FormatUploadStatus(uploadStep, percent);
}

void CMainWindow::EndFileUpload( BOOL abort )
{
  wstring auth;
  CUploadItem::ITEMSTATE state;
  CGoogleUploader::UPLOAD_STEP uploadStep;
  CGoogleUploader::RESULT_INFO resInfo;
  int percent;
  size_t nextToUpload;
  wstring newTitle;
  wstring statusString;

  m_uploading = FALSE;
  KillTimer(GetHandle(), UPDATESTATUS_TIMERID);
  CGoogleUploader::Instance().EndFileUpload(resInfo, auth, newTitle, m_googleFolderId, abort);
  if (!auth.empty())
  {
    AppSettings()->m_googleAuth = auth;
  }
  CGoogleUploader::Instance().GetUploadStatus(uploadStep, percent);
  switch (resInfo.res)
  {
  case CGoogleUploader::UR_ABORT:
    ASSERT(abort);
    AfterUploadSeriesEnd(FALSE);
    SetCurrentStatus(CUploadItem::STOPPED, UIFormat(msg_stopped));
    m_currentFile = CUploadList::npos;
    break;
  case CGoogleUploader::UR_SUCCESS:
    m_countSuccess++;
    m_uploadList.NullPos1();
    if (uploadStep == CGoogleUploader::UP_CHECKDUPLICATE)
    {
      statusString = newTitle.empty() ? UIFormat(msg_filemathesservercopy) : UIFormat(msg_filemathesservercopyas, newTitle.c_str());
    }
    else
    {
      ASSERT(uploadStep == CGoogleUploader::UP_SENDING);
      statusString = newTitle.empty() ? UIFormat(msg_uploadcompleted) : UIFormat(msg_uploadedas, newTitle.c_str());
    }
    SetCurrentStatus(CUploadItem::SUCCESS, statusString);
    // success result could also come if abort button was hit too late so check if we need to go further
    if (!abort && m_uploadList.FindPos1(nextToUpload))
      StartUpload(nextToUpload);
    else
    {
      m_currentFile = CUploadList::npos;
      AfterUploadSeriesEnd();
    }
    break;
  default:
    ASSERT(resInfo.res == CGoogleUploader::UR_ERROR || resInfo.res == CGoogleUploader::UR_CRITICALERROR);
    state = CUploadItem::ITEMERROR;

    wstring displayMessage;
    if (!resInfo.serverMessage.empty())
      displayMessage = resInfo.serverMessage;
    else if (!resInfo.errorMessage.empty())
      displayMessage = resInfo.errorMessage;
    else
      displayMessage = UIFormat(msg_failed);
    if (uploadStep == CGoogleUploader::UP_SENDING && percent == 100)
      SetCurrentStatus(state, displayMessage); // display "XXX" instead of "Uploading: 100% - XXX"
    else
    {
      SetCurrentStatus(state, UIFormat(msg_steperrorformat, 
        FormatUploadStatus(uploadStep, percent).c_str(),
        displayMessage.c_str()));
    }
    nextToUpload = CUploadList::npos;
    if (!abort && (resInfo.res != CGoogleUploader::UR_CRITICALERROR))
    {
      if (m_currentRetry < AppSettings()->m_maxRetries)
      {
        m_currentRetry++;
        nextToUpload = m_currentFile;
      }
      else
      { // advance to the next upload item
        m_currentRetry = 0;
        m_countErrors++;
        m_uploadList.NullPos1();
        m_uploadList.FindPos1(nextToUpload);
      }
    }
    else
      m_countErrors++;
    if (nextToUpload != CUploadList::npos)
      StartUpload(nextToUpload);
    else
    {
      m_currentFile = CUploadList::npos; // to disable "Upload" button
      AfterUploadSeriesEnd();
    }
  }
}

void CMainWindow::SetCurrentStatus(CUploadItem::ITEMSTATE state, const wstring &msg)
{
  ASSERT(m_currentFile != CUploadList::npos);
  if (m_currentRetry == 0)
    m_uploadList.SetCurrentStatus(m_currentFile, state, msg);
  else
    m_uploadList.SetCurrentStatus(m_currentFile, state, UIFormat(msg_retrymessage_fmt, str::IntToStr(m_currentRetry).c_str(), msg.c_str()));
}

void CMainWindow::OnTimer(WPARAM timerId)
{
  if (timerId == UPDATESTATUS_TIMERID && m_uploading)
  {
    SetCurrentStatus(CUploadItem::ACTIVE, GetFormattedUploadStatus());
  }
}

void CMainWindow::BeforeUploadSeriesStart()
{
  m_countErrors = 0;
  m_countSuccess = 0;
  m_currentRetry = 0;
}

void CMainWindow::AfterUploadSeriesEnd(BOOL showMessage)
{
  wstring message;
  DWORD icon;

  ASSERT(!m_uploading);

  UpdateUploadButton();
  if (showMessage)
  {
    if (m_countSuccess == 0)
    {
      icon = NIIF_ERROR;
      message = UIFormat(msg_uploadstoperror);
    }
    else if (m_countErrors > 0)
    {
      icon = NIIF_WARNING;
      message = UIFormat(msg_uploadwitherrors);
    }
    else
    {
      icon = NIIF_INFO;
      message = UIFormat(msg_uploadsuccess);
    }
    m_trayIcon.ShowBalloon(message.c_str(), APPLICATION_TITLE, icon);
  }
}

void CMainWindow::OnUploadRetry()
{
  size_t pos1_index;

  if (m_uploadList.RetrySelectedItems(pos1_index) && !m_uploading)
  {
    if (pos1_index == CUploadList::npos)
      m_uploadList.FindPos1(pos1_index);
    if (pos1_index != CUploadList::npos)
    {
      BeforeUploadSeriesStart();
      StartUpload(pos1_index);
    }
    else
    {
      ASSERT(FALSE);
    }
  }
}
