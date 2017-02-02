/*@ Docs Online - A Google Docs Backup Applicaiotn                          */
/*                                                                          */
/*--------------------------------------------------------------------------*/
/* Written and Designed by Michael Haephrati                                */
/* COPYRIGHT ©2008 by Michael Haephrati    haephrati@gmail.com              */
/* All rights reserved.                                                     */
/* -------------------------------------------------------------------------*/
#include "StdAfx.h"
#include "AppSettings.h"
#include "AppConstants.h"
#include "Utils.h"
#include "../common/common.h"

#define REG_FIRST_START                     L"FirstStart"
#define REG_GOOGLE_USER                     L"GoogleUser"
#define REG_GOOGLE_PASSWORD                 L"GoogleAcc"
#define REG_GOOGLE_PREMIER                  L"GooglePremier"
#define REG_CONVERT_DOCS                    L"ConvertDocs"
#define REG_MIRROR_FOLDER_FMT               L"MirrorFolder%d"
#define REG_ENABLE_MIRROR_FOLDERS           L"EnableMirrorFolders"
#define REG_ROOTUPLOAD                      L"RootUpload"
#define REG_MINIMIZEBALLOONSHOWED           L"MinimizeBalloonShowed"

//////////////////////////////////////////////////////////////////////////

CAppSettings *AppSettings()
{
  static CAppSettings appSettings;
  return &appSettings;
}

class CRegistryKey
{
public:
  CRegistryKey() { m_key = NULL; }
  ~CRegistryKey() { if (m_key != NULL) RegCloseKey(m_key); }
  operator HKEY() const { return m_key; }
  BOOL Create()
  {
    DWORD disposition;

    return (RegCreateKeyEx(HKEY_CURRENT_USER, APPLICATION_REGISTRY_KEY, 0,
      NULL, 0, KEY_CREATE_SUB_KEY | KEY_SET_VALUE, NULL, &m_key, &disposition) == ERROR_SUCCESS);
  }
private:
  HKEY m_key;
};

//////////////////////////////////////////////////////////////////////////

CAppSettings::CAppSettings(void)
{
}

CAppSettings::~CAppSettings(void)
{
}


BOOL CryptString(vector<BYTE> &encryptedBLOB, const wstring &inputString)
{
  BOOL result;
  DATA_BLOB plain;
  DATA_BLOB encrypted;

  result = FALSE;
  plain.pbData = (BYTE*)inputString.c_str();    
  plain.cbData = str::SizeInBytes(inputString);
  if (CryptProtectData(&plain, NULL, NULL, NULL, NULL, 0, &encrypted))
  {
    try
    {
      encryptedBLOB.resize(encrypted.cbData);
    }
    catch (...)
    {
      LocalFree(encrypted.pbData);
      throw;
    }
    memcpy(&encryptedBLOB[0], encrypted.pbData, encrypted.cbData);
    LocalFree(encrypted.pbData);
    result = TRUE;
  }
  else
    encryptedBLOB.clear();
  return result;
}

BOOL DecryptString(wstring &decryptedString, const vector<BYTE> &encryptedBLOB)
{
  BOOL result;
  DATA_BLOB encrypted;
  DATA_BLOB plain;

  encrypted.cbData = encryptedBLOB.size();
  encrypted.pbData = (BYTE*)&encryptedBLOB[0];
  result = CryptUnprotectData(&encrypted, NULL, NULL, NULL, NULL, 0, &plain);
  if (result)
  {
    str::AssignSZBytes(decryptedString, plain.pbData, plain.cbData);
    LocalFree(plain.pbData);
  }
  else
    decryptedString.clear();
  return result;
}

void CAppSettings::LoadSettings()
{
  HKEY key;
  vector<BYTE> encryptedPassword;
  wstring folder;
  size_t i;

  m_firstStart = TRUE;
  m_googleUser.clear();
  m_googlePassword.clear();
  m_googleAuth.clear();
  m_googlePremier = FALSE;
  m_convertDocs = TRUE;
  m_installExecutable.clear();
  m_enableMirrorFolders = TRUE;
  m_mirrorFolders.clear();
  m_rootUpload = FALSE;
  m_maxRetries = 3;
  m_minimizeBalloonShowed = FALSE;

  RegOpenKeyEx(HKEY_CURRENT_USER, APPLICATION_REGISTRY_KEY, 0, KEY_QUERY_VALUE, &key); 
  if (key != NULL) 
  {
    reg::GetBool(m_firstStart, key, REG_FIRST_START);
    reg::GetString(m_googleUser, key, REG_GOOGLE_USER);
    if (reg::GetValue(encryptedPassword, key, REG_GOOGLE_PASSWORD))
    {
      DecryptString(m_googlePassword, encryptedPassword);
    }
    reg::GetBool(m_googlePremier, key, REG_GOOGLE_PREMIER);
    reg::GetString(m_installExecutable, key, REG_EXECUTABLE);

    reg::GetBool(m_convertDocs, key, REG_CONVERT_DOCS);
    reg::GetBool(m_enableMirrorFolders, key, REG_ENABLE_MIRROR_FOLDERS);
    reg::GetBool(m_rootUpload, key, REG_ROOTUPLOAD);
    reg::GetBool(m_minimizeBalloonShowed, key, REG_MINIMIZEBALLOONSHOWED);

    for (i = 0; reg::GetString(folder, key, str::Format1K(REG_MIRROR_FOLDER_FMT, i).c_str()) && !folder.empty(); i++)
      m_mirrorFolders.push_back(folder);
    RegCloseKey(key);
  }
}  

void CAppSettings::SaveGeneralSettings()
{
  CRegistryKey key;
  vector<BYTE> encryptedPassword;

  if (key.Create())
  {
    reg::SetString(key, REG_GOOGLE_USER, m_googleUser.c_str());
    CryptString(encryptedPassword, m_googlePassword);
    // write encryptedPassword anyway so BLOB of 0 bytes is written in case of encrypt error
    reg::SetBinary(key, REG_GOOGLE_PASSWORD, &encryptedPassword[0], encryptedPassword.size());
    reg::SetBool(key, REG_GOOGLE_PREMIER, m_googlePremier);
    reg::SetBool(key, REG_CONVERT_DOCS, m_convertDocs);
    reg::SetBool(key, REG_ENABLE_MIRROR_FOLDERS, m_enableMirrorFolders);
    reg::SetBool(key, REG_ROOTUPLOAD, m_rootUpload);
  }
}

void CAppSettings::SaveOnExit()
{
  CRegistryKey key;

  if (key.Create())
  {
    reg::SetBool(key, REG_CONVERT_DOCS, m_convertDocs);
    reg::SetBool(key, REG_FIRST_START, FALSE);
    reg::SetBool(key, REG_MINIMIZEBALLOONSHOWED, m_minimizeBalloonShowed);
  }
}

void CAppSettings::SaveMirrorFolders()
{
  CRegistryKey key;
  size_t i;

  if (key.Create())
  {
    for (i = 0; i < m_mirrorFolders.size(); i++)
      reg::SetString(key, str::Format1K(REG_MIRROR_FOLDER_FMT, i).c_str(), m_mirrorFolders[i].c_str());
    reg::SetString(key, str::Format1K(REG_MIRROR_FOLDER_FMT, i).c_str(), L"");
  }
}

