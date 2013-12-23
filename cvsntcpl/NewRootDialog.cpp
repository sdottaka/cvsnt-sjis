// NewRootDialog.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "NewRootDialog.h"
#include "cvsnt.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/* Compatibility with old headers */
#ifndef BIF_NEWDIALOGSTYLE
#define BIF_NEWDIALOGSTYLE 0
#endif

/////////////////////////////////////////////////////////////////////////////
// CNewRootDialog dialog


CNewRootDialog::CNewRootDialog(CWnd* pParent /*=NULL*/)
	: CDialog(CNewRootDialog::IDD, pParent)
{
	//{{AFX_DATA_INIT(CNewRootDialog)
	m_szRoot = _T("");
	//}}AFX_DATA_INIT
}


void CNewRootDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CNewRootDialog)
	DDX_Text(pDX, IDC_ROOT, m_szRoot);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CNewRootDialog, CDialog)
	//{{AFX_MSG_MAP(CNewRootDialog)
	ON_BN_CLICKED(IDC_SELECT, OnSelect)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CNewRootDialog message handlers

void CNewRootDialog::OnSelect() 
{
	TCHAR fn[MAX_PATH];
	LPITEMIDLIST idl,idlroot;
	IMalloc *mal;
	
	SHGetSpecialFolderLocation(m_hWnd, CSIDL_DRIVES, &idlroot);
	SHGetMalloc(&mal);
	BROWSEINFO bi = { m_hWnd, idlroot, fn, _T("Select folder for repository root."), BIF_NEWDIALOGSTYLE|BIF_STATUSTEXT|BIF_UAHINT|BIF_RETURNONLYFSDIRS|BIF_RETURNFSANCESTORS, BrowseValid };
	idl = SHBrowseForFolder(&bi);

	mal->Free(idlroot);
	if(!idl)
	{
		mal->Release();
		return;
	}

	SHGetPathFromIDList(idl,fn);

	mal->Free(idl);
	mal->Release();

	m_szRoot=fn;
	UpdateData(FALSE);
}

void CNewRootDialog::OnOK() 
{
	CString tmp;
	UpdateData(TRUE);
	bool bCreated=false;
	TCHAR shortfn[4];

	m_szRoot.Replace(_T("\\"),_T("/"));
	_tcsncpy(shortfn,m_szRoot,4);
	shortfn[3]='\0';
	if(_tcsnicmp(m_RepoPrefix,m_szRoot,m_RepoPrefix.GetLength()))
	{
		AfxMessageBox(_T("All repositories must be within the current repository prefix"));
		return;
	}
	if(m_szRoot[0]!='/' && m_szRoot[1]!=':')
	{
		AfxMessageBox(_T("You must specify an absolute root for the pathname"),MB_ICONSTOP|MB_OK);
		return;
	}
	if(m_szRoot[0]=='/' && m_szRoot[1]=='/')
	{
		AfxMessageBox(_T("You cannot use a UNC pathname for the root"),MB_ICONSTOP|MB_OK);
		return;
	}
	if(GetDriveType(shortfn)==DRIVE_REMOTE)
	{
		AfxMessageBox(_T("You must store the repository on a local drive.  Network drives are not allowed"),MB_ICONSTOP|MB_OK);
		return;
	}

	DWORD dwStatus = GetFileAttributes(m_szRoot);
	if(dwStatus==0xFFFFFFFF)
	{
		tmp.Format(_T("%s does not exist.  Create it?"),(LPCTSTR)m_szRoot);
		if(AfxMessageBox(tmp,MB_ICONSTOP|MB_YESNO|MB_DEFBUTTON2)==IDNO)
			return;
		if(!CreateDirectory(m_szRoot,NULL))
		{
			AfxMessageBox(_T("Couldn't create directory"),MB_ICONSTOP|MB_OK);
			return;
		}
		bCreated=true;
	}
	if(!bCreated && !(dwStatus&FILE_ATTRIBUTE_DIRECTORY))
	{
		tmp.Format(_T("%s is not a directory."),(LPCTSTR)m_szRoot);
		AfxMessageBox(tmp,MB_ICONSTOP|MB_OK);
		return;
	}
	tmp=m_szRoot;
	tmp+="\\CVSROOT";
	dwStatus = GetFileAttributes(tmp);
	if(dwStatus==0xFFFFFFFF)
	{
		tmp.Format(_T("%s exists, but is not a valid CVS repository.\n\nDo you want to initialise it?"),(LPCTSTR)m_szRoot);
		if(!bCreated && AfxMessageBox(tmp,MB_ICONQUESTION|MB_YESNO|MB_DEFBUTTON2)==IDNO)
			return;
		tmp.Format(_T("%scvs -d \"%s\" init"),(LPCTSTR)m_szInstallPath,(LPCTSTR)m_szRoot);
		{
			CWaitCursor wait;
			STARTUPINFO si = { sizeof(STARTUPINFO) };
			PROCESS_INFORMATION pi = { 0 };
			si.dwFlags = STARTF_USESHOWWINDOW;
			si.wShowWindow = SW_SHOWMINNOACTIVE;
			if(CreateProcess(NULL,(LPTSTR)(LPCTSTR)tmp,NULL,NULL,FALSE,0,NULL,NULL,&si,&pi))
			{
				CloseHandle(pi.hThread);
				WaitForSingleObject(pi.hProcess,INFINITE);
				CloseHandle(pi.hProcess);
			}
		}
		tmp=m_szRoot;
		tmp+="/CVSROOT";
		dwStatus = GetFileAttributes(tmp);
		if(dwStatus==0xFFFFFFFF)
		{
			tmp.Format(_T("Repository initialisation failed.  To see errors, type the following at the command line:\n\n%scvs -d %s init"),(LPCTSTR)m_szInstallPath,(LPCTSTR)m_szRoot);
			AfxMessageBox(tmp,MB_ICONSTOP|MB_OK);
		}
	}

	m_szRoot = m_szRoot.Right(m_szRoot.GetLength()-m_RepoPrefix.GetLength());
	CDialog::OnOK();
}

BOOL CNewRootDialog::OnInitDialog() 
{
	m_szRoot = m_RepoPrefix + "/";

	CDialog::OnInitDialog();
	
	return TRUE;
}
