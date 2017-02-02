/*@ Docs Online - A Google Docs Backup Applicaiotn                          */
/*                                                                          */
/*--------------------------------------------------------------------------*/
/* Written and Designed by Michael Haephrati                                */
/* COPYRIGHT ©2008 by Michael Haephrati    haephrati@gmail.com              */
/* All rights reserved.                                                     */
/* -------------------------------------------------------------------------*/

#include "StdAfx.h"
#include "Application.h"
#include "Utils.h"
#include "..\common\common.h"
#include "AppConstants.h"

CApplication *AppInstance()
{
	static CApplication application;
	return &application;
}

CApplication::CApplication(void)
{
}

CApplication::~CApplication(void)
{
}

void CApplication::Init(HINSTANCE hInstance, LPTSTR lpCmdLine, int nCmdShow)
{
  HKEY key = NULL;
  DWORD disposition;
  wchar_t *executablePath;

  static const wchar_t autoStart[] = AUTOSTART_PARAMETER;

  m_instance = hInstance;
	m_commandLine = lpCmdLine;

  // Set "Executable" registry value for the shell extension
  if (_get_wpgmptr(&executablePath) == 0)
  {
    if (RegCreateKeyEx(HKEY_CURRENT_USER, APPLICATION_REGISTRY_KEY, 0,
      NULL, 0, KEY_CREATE_SUB_KEY | KEY_SET_VALUE, NULL, &key, &disposition) == ERROR_SUCCESS)
    {
      reg::SetString(key, REG_EXECUTABLE, executablePath);
      RegCloseKey(key);
    }
  }

  m_autostart = FALSE;
  if (m_commandLine.length() >= ARRAYDIM(autoStart)-1 && m_commandLine.substr(m_commandLine.length() - (ARRAYDIM(autoStart)-1)) == autoStart)
    m_autostart = TRUE;
}

