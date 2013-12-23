// NewRootDialog.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "NewRootDialog.h"
#include "cvsnt.h"
#include ".\newrootdialog.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CNewRootDialog dialog


CNewRootDialog::CNewRootDialog(CWnd* pParent /*=NULL*/)
	: CDialog(CNewRootDialog::IDD, pParent)
{
	//{{AFX_DATA_INIT(CNewRootDialog)
	//}}AFX_DATA_INIT
}


void CNewRootDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CNewRootDialog)
	DDX_Text(pDX, IDC_ROOT, m_szRoot);
	DDX_Text(pDX, IDC_NAME, m_szName);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CNewRootDialog, CDialog)
	//{{AFX_MSG_MAP(CNewRootDialog)
	ON_BN_CLICKED(IDC_SELECT, OnSelect)
	//}}AFX_MSG_MAP
	ON_EN_CHANGE(IDC_ROOT, OnEnChangeRoot)
	ON_EN_CHANGE(IDC_NAME, OnEnChangeName)
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
#ifdef JP_STRING
	BROWSEINFO bi = { m_hWnd, idlroot, fn, _T("���|�W�g�����[�g�̃t�H���_��I�����Ă��������B"), BIF_NEWDIALOGSTYLE|BIF_STATUSTEXT|BIF_UAHINT|BIF_RETURNONLYFSDIRS|BIF_RETURNFSANCESTORS, BrowseValid };
#else
	BROWSEINFO bi = { m_hWnd, idlroot, fn, _T("Select folder for repository root."), BIF_NEWDIALOGSTYLE|BIF_STATUSTEXT|BIF_UAHINT|BIF_RETURNONLYFSDIRS|BIF_RETURNFSANCESTORS, BrowseValid };
#endif
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

	m_szRoot =fn;
	m_szRoot .Replace('\\','/');
	UpdateData(FALSE);
	UpdateName();
}

void CNewRootDialog::OnOK() 
{
	CString tmp;
	UpdateData(TRUE);
	bool bCreated=false;
	TCHAR shortfn[4];

	m_szName.Replace(_T("\\"),_T("/"));
	if(m_szName.GetLength()<2 || (m_szName[0]!='/' && m_szName[1]!=':') || m_szName.Left(2)=="//")
	{
#ifdef JP_STRING
		AfxMessageBox(_T("���|�W�g�����[�g�̖��O�́A'/'�Ŏn�܂�Aunix�X�^�C���̐�΃p�X���łȂ���΂Ȃ�܂���"));
#else
		AfxMessageBox(_T("The name of the repository root must be a unix-style absolute pathname starting with '/'"));
#endif
		return;
	}
	m_szRoot .Replace(_T("\\"),_T("/"));
	_tcsncpy(shortfn,m_szRoot ,4);
	shortfn[3]='\0';
	if(m_szRoot [1]!=':')
	{
#ifdef JP_STRING
		AfxMessageBox(_T("�p�X���́A��΃p�X�Ŏw�肵�Ă��������B"),MB_ICONSTOP|MB_OK);
#else
		AfxMessageBox(_T("You must specify an absolute root for the pathname"),MB_ICONSTOP|MB_OK);
#endif
		return;
	}
	if(GetDriveType(shortfn)==DRIVE_REMOTE)
	{
#ifdef JP_STRING
		AfxMessageBox(_T("���|�W�g���̓��[�J���h���C�u��ɂ���K�v������܂��B�l�b�g���[�N�h���C�u�͎w��ł��܂���B"),MB_ICONSTOP|MB_OK);
#else
		AfxMessageBox(_T("You must store the repository on a local drive.  Network drives are not allowed"),MB_ICONSTOP|MB_OK);
#endif
		return;
	}

	if(m_szName[1]==':')
	{
#ifdef JP_STRING
		if(AfxMessageBox(_T("Unix�N���C�A���g�Ƃ̌݊����̖�肪�����邽�߁A���|�W�g�����Ƀh���C�u���^�[���g�p���邱�Ƃ͐����ł��܂���B����ł����s���܂���?"),MB_YESNO|MB_DEFBUTTON2)!=IDYES)
#else
		if(AfxMessageBox(_T("Using drive letters in repository names can create compatibility problems with Unix clients and is not recommended.  Are you sure you want to continue?"),MB_YESNO|MB_DEFBUTTON2)!=IDYES)
#endif
			return;
	}

	for(int n=1; n<m_szName.GetLength(); n++)
	{
#ifdef SJIS
		if(_tcschr(_T("\"+,.;<=>[]|${} '"),m_szName[n]) || (n>1 && m_szName[n]==':'))
#else
		if(strchr("\"+,.;<=>[]|${} '",m_szName[n]) || (n>1 && m_szName[n]==':'))
#endif
		{
#ifdef JP_STRING
			if(AfxMessageBox(_T("�w�肳�ꂽ���|�W�g�����́A����N���C�A���g�Ō݊����̖��𐶂��镶�����܂�ł��܂��B���̂悤�Ȗ��O���g�p���邱�Ƃ͐����ł��܂���B����ł����s���܂���?"),MB_YESNO|MB_DEFBUTTON2)!=IDYES)
#else
			if(AfxMessageBox(_T("The repository name contains characters that may create compatibility problems with certain clients.  Using such names is not recommended.  Are you sure you want to continue?"),MB_YESNO|MB_DEFBUTTON2)!=IDYES)
#endif
				return;
		}
	}

	DWORD dwStatus = GetFileAttributes(m_szRoot );
	if(dwStatus==0xFFFFFFFF)
	{
#ifdef JP_STRING
		tmp.Format(_T("%s �͑��݂��܂���B �쐬���܂���?"),(LPCTSTR)m_szRoot );
#else
		tmp.Format(_T("%s does not exist.  Create it?"),(LPCTSTR)m_szRoot );
#endif
		if(AfxMessageBox(tmp,MB_ICONSTOP|MB_YESNO|MB_DEFBUTTON2)==IDNO)
			return;
		if(!CreateDirectory(m_szRoot ,NULL))
		{
#ifdef JP_STRING
			AfxMessageBox(_T("�f�B���N�g���̍쐬�Ɏ��s���܂����B"),MB_ICONSTOP|MB_OK);
#else
			AfxMessageBox(_T("Couldn't create directory"),MB_ICONSTOP|MB_OK);
#endif
			return;
		}
		bCreated=true;
	}
	if(!bCreated && !(dwStatus&FILE_ATTRIBUTE_DIRECTORY))
	{
#ifdef JP_STRING
		tmp.Format(_T("%s �̓f�B���N�g���ł͂���܂���B"),(LPCTSTR)m_szRoot );
#else
		tmp.Format(_T("%s is not a directory."),(LPCTSTR)m_szRoot );
#endif
		AfxMessageBox(tmp,MB_ICONSTOP|MB_OK);
		return;
	}
	tmp=m_szRoot ;
	tmp+="\\CVSROOT";
	dwStatus = GetFileAttributes(tmp);
	if(dwStatus==0xFFFFFFFF)
	{
#ifdef JP_STRING
		tmp.Format(_T("%s �͑��݂��܂����A�L����CVS���|�W�g���ł͂���܂���B\n\n���̃f�B���N�g�������������܂���?"),(LPCTSTR)m_szRoot );
#else
		tmp.Format(_T("%s exists, but is not a valid CVS repository.\n\nDo you want to initialise it?"),(LPCTSTR)m_szRoot );
#endif
		if(!bCreated && AfxMessageBox(tmp,MB_ICONQUESTION|MB_YESNO|MB_DEFBUTTON2)==IDNO)
			return;
		tmp.Format(_T("%scvs -d \"%s\" init"),(LPCTSTR)m_szInstallPath,(LPCTSTR)m_szRoot );
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
		tmp=m_szRoot ;
		tmp+="/CVSROOT";
		dwStatus = GetFileAttributes(tmp);
		if(dwStatus==0xFFFFFFFF)
		{
#ifdef JP_STRING
			tmp.Format(_T("���|�W�g���̏������Ɏ��s���܂����B �G���[���e�́A���̃R�}���h����͂��邱�ƂŊm�F�ł��܂�: \n\n%scvs -d %s init"),(LPCTSTR)m_szInstallPath,(LPCTSTR)m_szRoot );
#else
			tmp.Format(_T("Repository initialisation failed.  To see errors, type the following at the command line:\n\n%scvs -d %s init"),(LPCTSTR)m_szInstallPath,(LPCTSTR)m_szRoot );
#endif
			AfxMessageBox(tmp,MB_ICONSTOP|MB_OK);
		}
	}

	CDialog::OnOK();
}

BOOL CNewRootDialog::OnInitDialog() 
{
	CDialog::OnInitDialog();

	if(!m_szName.GetLength())
		m_bSyncName = true;
	else
		m_bSyncName = false;
	
	return TRUE;
}

void CNewRootDialog::OnEnChangeRoot()
{
	UpdateName();
}

void CNewRootDialog::UpdateName()
{
	if(m_bSyncName)
	{
		UpdateData();
		if(m_szRoot .GetLength())
		{
			if(m_szRoot .GetLength()>1 && m_szRoot [1]==':')
				m_szName = m_szRoot .Right(m_szRoot .GetLength()-2);
			else if(m_szRoot [0]!='/')
				m_szName = "/" + m_szRoot ;
			else
				m_szName = m_szRoot ;
			m_szName.Replace('\\','/');
			SetDlgItemText(IDC_NAME, m_szName);
			m_bSyncName=true;
		}
		else
			SetDlgItemText(IDC_NAME,_T(""));
	}
}

void CNewRootDialog::OnEnChangeName()
{
	UpdateData();
	if(m_szName.GetLength())
		m_bSyncName=false;
	else
		m_bSyncName=true;
}
