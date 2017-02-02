#pragma once

class CAppSettings
{
public:
  BOOL m_firstStart;
  // General
  wstring m_googleUser;
  wstring m_googlePassword;
  wstring m_googleAuth;
  BOOL m_googlePremier;
  wstring m_installExecutable;
  BOOL m_enableMirrorFolders;
  BOOL m_rootUpload;
  int m_maxRetries; // a const at the moment
  // changes from the main window
  BOOL m_convertDocs;
  BOOL m_minimizeBalloonShowed;
  // saved only on change
  vector<wstring> m_mirrorFolders;

  CAppSettings(void);
  ~CAppSettings(void);
  void LoadSettings();
  void SaveGeneralSettings();
  void SaveOnExit();
  void SaveMirrorFolders();
};

CAppSettings *AppSettings();