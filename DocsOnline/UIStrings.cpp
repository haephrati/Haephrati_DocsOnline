/*@ Docs Online - A Google Docs Backup Applicaiotn                          */
/*                                                                          */
/*--------------------------------------------------------------------------*/
/* Written and Designed by Michael Haephrati                                */
/* COPYRIGHT ©2008 by Michael Haephrati    haephrati@gmail.com              */
/* All rights reserved.                                                     */
/* -------------------------------------------------------------------------*/
#include "stdafx.h"
#include "UIStrings.h"

#define MAX_UIARGS                       16
#define MAX_ARGLENGTH                    1024

wstring UIFormatVA(const wchar_t *id, va_list marker) {
  wstring result;
  const wchar_t *args[MAX_UIARGS];
  const wchar_t *arg;
  int argc;
  int index;
  wchar_t c;
  int i;

  argc = 0;
  for (i = 0; (c = id[i]) != '\0'; ) 
  {
    i++;
    if (c != '%')
      result += c;
    else 
    {
      c = id[i]; // grab the char after %
      i++;
      if (c >= '1' && c <= '9')
        index = c - '0';
      else if (c >= 'A' && c <= 'F')
        index = 0x10 + c - 'A';
      else if (c >= 'a' && c <= 'f')
        index = 0x10 + c - 'a';
      else {
        result += '%';
        if (c == '\0')
          break;
        if (c != '%') // %% -> %
          result += c;
        continue;
      }
      while (index > argc) 
        args[argc++] = va_arg(marker, const wchar_t *);
      arg = args[index-1];
      if (IsBadStringPtr(arg, MAX_ARGLENGTH)) {
        if (arg == NULL)
          result += L"<null string>";
        else
          result += L"<bad string>";
      }
      else
        result += arg;
    }
  }
  return result;
}

wstring UIFormat(const wchar_t *id, ...) 
{
  wstring result;
  va_list marker;

  va_start( marker, id );
  result = UIFormatVA(id, marker);
  va_end( marker );
  return result;
}
