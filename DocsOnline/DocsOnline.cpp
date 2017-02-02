/*@ Docs Online - A Google Docs Backup Applicaiotn                          */
/*                                                                          */
/*--------------------------------------------------------------------------*/
/* Written and Designed by Michael Haephrati                                */
/* COPYRIGHT ©2008 by Michael Haephrati    haephrati@gmail.com              */
/* All rights reserved.                                                     */
/* -------------------------------------------------------------------------*/
#include "stdafx.h"
#include "..\common\common.h"
#include "DocsOnline.h"
#include "Application.h"
#include "MainWindow.h"
#include "AppSettings.h"
#include "Utils.h"
#include "AppConstants.h"

int APIENTRY _tWinMain(HINSTANCE hInstance,
                       HINSTANCE hPrevInstance,
                       LPTSTR    lpCmdLine,
                       int       nCmdShow)
{
  UNREFERENCED_PARAMETER(hPrevInstance);
  UNREFERENCED_PARAMETER(lpCmdLine);
  MSG msg;
  HWND window; 
  BOOL alreadyRunning;
  int result;

  alreadyRunning = FALSE;
  if (CreateMutex(NULL, TRUE, APPLICATION_RUNNING_MUTEX) != NULL) 
  {
    if (GetLastError() == ERROR_ALREADY_EXISTS) 
    {
      result = 1;
      alreadyRunning = TRUE;
      window = FindWindowEx(NULL, NULL, APPLICATION_MAINWND_CLASS_NAME, NULL);
      if (window != NULL)
      {
        SetForegroundWindow(window);
        ShowWindow(window, SW_SHOW);
      }
    }
  }
  if (!alreadyRunning)  
  {
    OleInitialize(NULL);
    AppInstance()->Init(hInstance, lpCmdLine, nCmdShow);
    if (AppInstance()->AutostartParam())
      nCmdShow = SW_HIDE;
    AppSettings()->LoadSettings();
    AppWindow()->CreateShow(APPLICATION_MAINWND_CLASS_NAME, nCmdShow);
    // Main message loop:
    while (GetMessage(&msg, NULL, 0, 0))
    {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }
    OleUninitialize();
    result = (int) msg.wParam;
  }
  return result;
}