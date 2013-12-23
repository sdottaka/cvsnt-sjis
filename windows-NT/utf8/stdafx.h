// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#define STRICT
#define WIN32_LEAN_AND_MEAN
#define _WIN32_WINNT 0x0400
#include <windows.h>

#include <winbase.h>
#include <winreg.h>
#include <wingdi.h>
#include <winuser.h>

#include <malloc.h>

class CUtf8Map
{
public:
	CUtf8Map(bool bConst, LPCSTR szConst, int nMaxLen) { m_bConst = bConst; m_szConst = (char*)szConst; if(!nMaxLen) m_nMaxLen=strlen(szConst)+1; else m_nMaxLen=nMaxLen; m_wszConst=new WCHAR[m_nMaxLen]; MultiByteToWideChar(CP_UTF8,0,m_szConst,m_nMaxLen,m_wszConst,m_nMaxLen); }
	~CUtf8Map() { if(!m_bConst) WideCharToMultiByte(CP_UTF8,0,m_wszConst,m_nMaxLen,m_szConst,m_nMaxLen,NULL,NULL); }
	operator LPCWSTR() { return m_wszConst; }
	operator LPWSTR() { return m_wszConst; }
private:
	WCHAR *m_wszConst;
	char *m_szConst;
	int m_nMaxLen;
	bool m_bConst;
};

#define UTF8_MAP(_x,_l) (LPWSTR)CUtf8Map(false,_x,_l)
#define UTF8_C_MAP(_x) (LPCWSTR)CUtf8Map(true,_x,0)

#define	UTF8_C_STRING(_x,_y) CUtf8Map _x(true, _y, 0)
#define	UTF8_STRING(_x,_y) CUtf8Map _x(false, _y, 0)
#define UTF8_ALLOC_STRING(x) (LPWSTR)malloc(strlen(x)*sizeof(TCHAR))

#define UTF8_REVERSE_MAP(x) (LPSTR)x
#define UTF8_REVERSE_C_MAP(x) (LPCSTR)x

#define	UTF8_REVERSE_C_STRING(x,y) LPCSTR x /* =y */
#define	UTF8_REVERSE_STRING(x,y) LPSTR x /* =y */
#define UTF8_REVERSE_ALLOC_STRING(x) (LPSTR)malloc(wcslen(x))
