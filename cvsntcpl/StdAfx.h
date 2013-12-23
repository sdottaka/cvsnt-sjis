// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//

#if !defined(AFX_STDAFX_H__F52337E9_30FF_11D2_8EED_00A0C94457BF__INCLUDED_)
#define AFX_STDAFX_H__F52337E9_30FF_11D2_8EED_00A0C94457BF__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define VC_EXTRALEAN		// Exclude rarely-used stuff from Windows headers
#define STRICT
#define WINVER 0x0400

#include <afxwin.h>         // MFC core and standard components
#include <afxext.h>         // MFC extensions
#include <afxcmn.h>			// MFC support for Windows Common Controls

#include <cpl.h>
#include <winsvc.h>

#include <shellapi.h>
#include <Shlwapi.h>
#include <shlobj.h>

#include <vector>
using namespace std;

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#include "../version_no.h"
#include "../version_fu.h"
#include <afxdlgs.h>

int CALLBACK BrowseValid(HWND hWnd, UINT uMsg, LPARAM lParam, LPARAM lpData);

#endif // !defined(AFX_STDAFX_H__F52337E9_30FF_11D2_8EED_00A0C94457BF__INCLUDED_)
