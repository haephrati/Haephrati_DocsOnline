// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "targetver.h"

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files:
#include <windows.h>

// C RunTime Header Files
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>


// TODO: reference additional headers your program requires here
#include <wininet.h>
#include <ole2.h>
#include <Shellapi.h>
#include <shlobj.h>
#include <Wincrypt.h>
#include <commctrl.h>
#include <windowsx.h>
#include <msxml.h>
#include <objsafe.h>
#include <objbase.h>
#include <atlbase.h>

#include <assert.h>
#define ASSERT assert

#include <string>
#include <algorithm>
#include <vector>
#include <map>
#include <stdexcept>
#include <sstream>
#include <fstream>
using namespace std;

#include "Buffer.h"
#include "SystemTraySDK.h"
#include "hashstring.h"
