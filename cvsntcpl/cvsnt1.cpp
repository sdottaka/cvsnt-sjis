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

bool g_bPrivileged;
HKEY g_hServerKey;

#ifndef Button_SetElevationRequiredState
#define BCM_SETSHIELD            (0x1600/*BCM_FIRST*/ + 0x000C)
#define Button_SetElevationRequiredState(hwnd, fRequired) \
	(LRESULT)::SendMessage((hwnd), BCM_SETSHIELD, 0, (LPARAM)fRequired)
#endif

/*-------------------------------------------------------------------------

from http://support.microsoft.com/kb/118626/ja

IsCurrentUserLocalAdministrator ()

This function checks the token of the calling thread to see if the caller
belongs to the Administrators group.

Return Value:
   TRUE if the caller is an administrator on the local machine.
   Otherwise, FALSE.
--------------------------------------------------------------------------*/
BOOL IsCurrentUserLocalAdministrator(void)
{
   BOOL   fReturn         = FALSE;
   DWORD  dwStatus;
   DWORD  dwAccessMask;
   DWORD  dwAccessDesired;
   DWORD  dwACLSize;
   DWORD  dwStructureSize = sizeof(PRIVILEGE_SET);
   PACL   pACL            = NULL;
   PSID   psidAdmin       = NULL;

   HANDLE hToken              = NULL;
   HANDLE hImpersonationToken = NULL;

   PRIVILEGE_SET   ps;
   GENERIC_MAPPING GenericMapping;

   PSECURITY_DESCRIPTOR     psdAdmin           = NULL;
   SID_IDENTIFIER_AUTHORITY SystemSidAuthority = SECURITY_NT_AUTHORITY;


   /*
      Determine if the current thread is running as a user that is a member of
      the local admins group.  To do this, create a security descriptor that
      has a DACL which has an ACE that allows only local aministrators access.
      Then, call AccessCheck with the current thread's token and the security
      descriptor.  It will say whether the user could access an object if it
      had that security descriptor.  Note: you do not need to actually create
      the object.  Just checking access against the security descriptor alone
      will be sufficient.
   */
   const DWORD ACCESS_READ  = 1;
   const DWORD ACCESS_WRITE = 2;


   __try
   {

      /*
         AccessCheck() requires an impersonation token.  We first get a primary
         token and then create a duplicate impersonation token.  The
         impersonation token is not actually assigned to the thread, but is
         used in the call to AccessCheck.  Thus, this function itself never
         impersonates, but does use the identity of the thread.  If the thread
         was impersonating already, this function uses that impersonation context.
      */
      if (!OpenThreadToken(GetCurrentThread(), TOKEN_DUPLICATE|TOKEN_QUERY, TRUE, &hToken))
      {
         if (GetLastError() != ERROR_NO_TOKEN)
            __leave;

         if (!OpenProcessToken(GetCurrentProcess(), TOKEN_DUPLICATE|TOKEN_QUERY, &hToken))
            __leave;
      }

      if (!DuplicateToken (hToken, SecurityImpersonation, &hImpersonationToken))
          __leave;


      /*
        Create the binary representation of the well-known SID that
        represents the local administrators group.  Then create the security
        descriptor and DACL with an ACE that allows only local admins access.
        After that, perform the access check.  This will determine whether
        the current user is a local admin.
      */
      if (!AllocateAndInitializeSid(&SystemSidAuthority, 2,
                                    SECURITY_BUILTIN_DOMAIN_RID,
                                    DOMAIN_ALIAS_RID_ADMINS,
                                    0, 0, 0, 0, 0, 0, &psidAdmin))
         __leave;

      psdAdmin = LocalAlloc(LPTR, SECURITY_DESCRIPTOR_MIN_LENGTH);
      if (psdAdmin == NULL)
         __leave;

      if (!InitializeSecurityDescriptor(psdAdmin, SECURITY_DESCRIPTOR_REVISION))
         __leave;

      // Compute size needed for the ACL.
      dwACLSize = sizeof(ACL) + sizeof(ACCESS_ALLOWED_ACE) +
                  GetLengthSid(psidAdmin) - sizeof(DWORD);

      pACL = (PACL)LocalAlloc(LPTR, dwACLSize);
      if (pACL == NULL)
         __leave;

      if (!InitializeAcl(pACL, dwACLSize, ACL_REVISION2))
         __leave;

      dwAccessMask= ACCESS_READ | ACCESS_WRITE;

      if (!AddAccessAllowedAce(pACL, ACL_REVISION2, dwAccessMask, psidAdmin))
         __leave;

      if (!SetSecurityDescriptorDacl(psdAdmin, TRUE, pACL, FALSE))
         __leave;

      /*
         AccessCheck validates a security descriptor somewhat; set the 

group
         and owner so that enough of the security descriptor is filled out 

to
         make AccessCheck happy.
      */
      SetSecurityDescriptorGroup(psdAdmin, psidAdmin, FALSE);
      SetSecurityDescriptorOwner(psdAdmin, psidAdmin, FALSE);

      if (!IsValidSecurityDescriptor(psdAdmin))
         __leave;

      dwAccessDesired = ACCESS_READ;

      /*
         Initialize GenericMapping structure even though you
         do not use generic rights.
      */
      GenericMapping.GenericRead    = ACCESS_READ;
      GenericMapping.GenericWrite   = ACCESS_WRITE;
      GenericMapping.GenericExecute = 0;
      GenericMapping.GenericAll     = ACCESS_READ | ACCESS_WRITE;

      if (!AccessCheck(psdAdmin, hImpersonationToken, dwAccessDesired,
                       &GenericMapping, &ps, &dwStructureSize, &dwStatus,
                       &fReturn))
      {
         fReturn = FALSE;
         __leave;
      }
   }
   __finally
   {
      // Clean up.
      if (pACL) LocalFree(pACL);
      if (psdAdmin) LocalFree(psdAdmin);
      if (psidAdmin) FreeSid(psidAdmin);
      if (hImpersonationToken) CloseHandle (hImpersonationToken);
      if (hToken) CloseHandle (hToken);
   }

   return fReturn;
}

class CCplPropertySheet : public CPropertySheet
{
public:
	CCplPropertySheet(LPCTSTR szTitle) : CPropertySheet(szTitle) { }
	virtual ~CCplPropertySheet() { }
protected:
	virtual BOOL OnInitDialog()
	{
		BOOL bRet=CPropertySheet::OnInitDialog();

		OSVERSIONINFO vi = { sizeof(OSVERSIONINFO) };
		if(GetVersionEx(&vi) && (vi.dwMajorVersion>5 || (vi.dwMajorVersion==5 && vi.dwMinorVersion>2)))
		{
			CRect rect;
			GetDlgItem(IDOK)->GetWindowRect(&rect);
			ScreenToClient(&rect);
#ifdef JP_STRING
			btAdmin.Create(_T("設定を変更"),WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON,CRect(6,rect.top,rect.left - 6,rect.bottom),this,1111);
#else
			btAdmin.Create(_T("Change Settings"),WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON,CRect(6,rect.top,rect.left - 6,rect.bottom),this,1111);
#endif
			btAdmin.SetFont(CFont::FromHandle((HFONT)GetStockObject(DEFAULT_GUI_FONT)));
			Button_SetElevationRequiredState(btAdmin.m_hWnd,TRUE);
			if(IsCurrentUserLocalAdministrator())
				btAdmin.EnableWindow(FALSE);
		}
		return bRet;
	}

	void OnElevate()
	{
		// This isn't documented but seems to work in the beta2.
		TCHAR fn[MAX_PATH],cmd[MAX_PATH*2];
		GetModuleFileName(AfxGetApp()->m_hInstance,fn,MAX_PATH);
		_sntprintf(cmd,sizeof(cmd)/sizeof(cmd[0]),_T("shell32.dll,Control_RunDLL %s"),fn);
		if(ShellExecute(m_hWnd,_T("open"),_T("RunLegacyCPLElevated.exe"),cmd,NULL,SW_SHOWNORMAL)>(HINSTANCE)32)
			PostMessage(WM_COMMAND,IDCANCEL);
	}

	CButton btAdmin;

	DECLARE_MESSAGE_MAP()
};
BEGIN_MESSAGE_MAP(CCplPropertySheet,CPropertySheet)
	ON_BN_CLICKED(1111,OnElevate)
END_MESSAGE_MAP()


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
	CString tmp;
	
	if(IsCurrentUserLocalAdministrator())
	{
		if(RegCreateKeyEx(HKEY_LOCAL_MACHINE,_T("Software\\CVS\\Pserver"),NULL,_T(""),REG_OPTION_NON_VOLATILE,KEY_ALL_ACCESS,NULL,&g_hServerKey,NULL))
		{ 
			tmp.Format(_T("Couldn't create HKLM\\Software\\CVS\\Pserver key, error %d\n"),GetLastError());
			AfxMessageBox(tmp,MB_ICONSTOP);
			return FALSE;
		}
		g_bPrivileged=true;
	}
	else
	{
		if(RegOpenKeyEx(HKEY_LOCAL_MACHINE,_T("Software\\CVS\\Pserver"),0,KEY_READ,&g_hServerKey))
		{ 
			tmp.Format(_T("CVSNT Server registry entries cannot be opened.  Quitting.\n"),GetLastError());
			AfxMessageBox(tmp,MB_ICONSTOP);
			return FALSE;
		}
		g_bPrivileged=false;
	}

	CCplPropertySheet sheet(g_bPrivileged?_T("CVSNT"):_T("CVSNT (Read Only)"));

	sheet.m_psh.hIcon = AfxGetApp()->LoadIcon(IDI_ICON1);
	sheet.m_psh.dwFlags |= PSH_USEHICON;

	sheet.AddPage(new CserverPage);
	sheet.AddPage(new CRepositoryPage);
	sheet.AddPage(new CAdvancedPage);
	sheet.AddPage(new CSslSettingPage);
	sheet.DoModal();

	RegCloseKey(g_hServerKey);
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
