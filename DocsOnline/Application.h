#pragma once

// Encapsulates global application properties and methods
class CApplication
{
public:
	CApplication(void);
	~CApplication(void);
	void Init(HINSTANCE hInstance, LPTSTR lpCmdLine, int nCmdShow);
	HINSTANCE GetInstance() { return m_instance; }
	HINSTANCE GetResInstance() { return m_instance; }
	const wstring &GetParams() { return m_commandLine; }
  BOOL AutostartParam() { return m_autostart; }
private:
	HINSTANCE m_instance;
	wstring m_commandLine;
  BOOL m_autostart;
};

CApplication *AppInstance();