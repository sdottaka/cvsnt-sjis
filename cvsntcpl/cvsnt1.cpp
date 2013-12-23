// cvsnt1.cpp: implementation of the CcvsntCPL class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "cvsnt.h"
#include "cvsnt1.h"
#include "serverPage.h"
#include "RepositoryPage.h"
#include "AdvancedPage.h"
#include "SslSettingPage.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CcvsntCPL::CcvsntCPL()
{
	CoInitialize(NULL);
}

CcvsntCPL::~CcvsntCPL()
{
}

BOOL CcvsntCPL::DoubleClick(UINT uAppNum, LONG lData)
{
	CPropertySheet sheet(_T("CVSNT"));

	sheet.AddPage(new CserverPage);
	sheet.AddPage(new CRepositoryPage);
	sheet.AddPage(new CAdvancedPage);
	sheet.AddPage(new CSslSettingPage);
	sheet.DoModal();
	return TRUE;
}

BOOL CcvsntCPL::Exit()
{
	return TRUE;
}

LONG CcvsntCPL::GetCount()
{
	return 1;
}

BOOL CcvsntCPL::Init()
{
	return TRUE;
}

BOOL CcvsntCPL::Inquire(UINT uAppNum, LPCPLINFO lpcpli)
{
	lpcpli->idIcon=IDI_ICON1;
	lpcpli->idInfo=IDS_DESCRIPTION;
	lpcpli->idName=IDS_NAME;
	return TRUE;
}

BOOL CcvsntCPL::Stop(UINT uAppNum, LONG lData)
{
	return TRUE;
}

/* Generic SHBrowseForFolder callback */
int CALLBACK BrowseValid(HWND hWnd, UINT uMsg, LPARAM lParam, LPARAM lpData)
{
	switch(uMsg)
	{
	case BFFM_INITIALIZED:
		return 0;
	case BFFM_SELCHANGED:
		{
			TCHAR fn[MAX_PATH];
			TCHAR shortfn[4];
			LPITEMIDLIST idl = (LPITEMIDLIST)lParam;
			BOOL bOk = SHGetPathFromIDList(idl,fn);
			_tcsncpy(shortfn,fn,3);
			shortfn[3]='\0';
			if(bOk && (!_tcsnicmp(fn,_T("\\\\"),2) || !_tcsnicmp(fn,_T("//"),2)))
			{
				bOk=FALSE;
				SendMessage(hWnd,BFFM_SETSTATUSTEXT,NULL,(LPARAM)"UNC Paths are not allowed");
			}
			if(bOk && GetDriveType(shortfn)==DRIVE_REMOTE)
			{
				bOk=FALSE;
#ifdef JP_STRING
				SendMessage(hWnd,BFFM_SETSTATUSTEXT,NULL,(LPARAM)"ネットワークドライブは使用できません");
#else
				SendMessage(hWnd,BFFM_SETSTATUSTEXT,NULL,(LPARAM)"Network drives are not allowed");
#endif
			}
			SendMessage(hWnd,BFFM_ENABLEOK,NULL,bOk);
		}
		return 0;
	case BFFM_VALIDATEFAILED:
		return -1;
	}
	return 0;
}
