// RepositoryPage.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "RepositoryPage.h"
#include "NewRootDialog.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define MAX_REPOSITORIES 1024

/////////////////////////////////////////////////////////////////////////////
// CRepositoryPage property page

IMPLEMENT_DYNCREATE(CRepositoryPage, CPropertyPage)

CRepositoryPage::CRepositoryPage() : CPropertyPage(CRepositoryPage::IDD)
{
	//{{AFX_DATA_INIT(CRepositoryPage)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	m_hServerKey = NULL;
}

CRepositoryPage::~CRepositoryPage()
{
}

void CRepositoryPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CRepositoryPage)
	DDX_Control(pDX, IDC_CHANGEPREFIX, m_btChangePrefix);
	DDX_Control(pDX, IDC_EDIT2, m_edPrefix);
	DDX_Control(pDX, IDC_REPOSITORYPREFIX, m_btRepoPrefix);
	DDX_Control(pDX, IDC_DELETEROOT, m_btDelete);
	DDX_Control(pDX, IDC_ADDROOT, m_btAdd);
	DDX_Control(pDX, IDC_ROOTLIST, m_listRoot);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CRepositoryPage, CPropertyPage)
	//{{AFX_MSG_MAP(CRepositoryPage)
	ON_LBN_SELCHANGE(IDC_ROOTLIST, OnSelchangeRootlist)
	ON_LBN_SELCANCEL(IDC_ROOTLIST, OnSelcancelRootlist)
	ON_BN_CLICKED(IDC_ADDROOT, OnAddroot)
	ON_BN_CLICKED(IDC_DELETEROOT, OnDeleteroot)
	ON_BN_CLICKED(IDC_REPOSITORYPREFIX, OnRepositoryprefix)
	ON_BN_CLICKED(IDC_CHANGEPREFIX, OnChangeprefix)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CRepositoryPage message handlers

BOOL CRepositoryPage::OnInitDialog() 
{
	DWORD bufLen,dwType;
	TCHAR buf[_MAX_PATH];

	CPropertyPage::OnInitDialog();
	
	if(!m_hServerKey && RegCreateKeyEx(HKEY_LOCAL_MACHINE,_T("Software\\CVS\\Pserver"),NULL,_T(""),REG_OPTION_NON_VOLATILE,KEY_ALL_ACCESS,NULL,&m_hServerKey,NULL))
	{ 
		fprintf(stderr,"Couldn't create HKLM\\Software\\CVS\\Pserver key, error %d\n",GetLastError());
		return -1;
	}

	GetRootList();
	DrawRootList();
	OnSelchangeRootlist();

	bufLen=sizeof(buf);
	if(RegQueryValueEx(m_hServerKey,_T("RepositoryPrefix"),NULL,&dwType,(BYTE*)buf,&bufLen))
	{
		// Not set
		*buf='\0';
	}
	TCHAR *p = buf;
	while((p=_tcschr(p,'\\'))!=NULL)
		*p='/';


	m_edPrefix.SetWindowText((LPCTSTR)buf);
	if(buf[0])
		m_btRepoPrefix.SetCheck(TRUE);
	else
		m_btRepoPrefix.SetCheck(FALSE);
	OnRepositoryprefix();

	bufLen=sizeof(buf);
	if(RegQueryValueEx(m_hServerKey,_T("InstallPath"),NULL,&dwType,(BYTE*)buf,&bufLen))
	{
		// Not set
		*buf='\0';
	}
	m_szInstallPath=buf;

	return TRUE;
}

void CRepositoryPage::OnSelchangeRootlist() 
{
	m_btAdd.EnableWindow(m_listRoot.GetCount()<MAX_REPOSITORIES);
	m_btDelete.EnableWindow(m_listRoot.GetCount()>0 && m_listRoot.GetCurSel()>=0);
}

void CRepositoryPage::OnSelcancelRootlist() 
{
	m_btAdd.EnableWindow(m_listRoot.GetCount()<MAX_REPOSITORIES);
	m_btDelete.EnableWindow(m_listRoot.GetCount()>0 && m_listRoot.GetCurSel()>=0);
}

BOOL CRepositoryPage::OnApply() 
{
	RebuildRootList();
	TCHAR fn[_MAX_PATH];

	if(m_btRepoPrefix.GetCheck())
		m_edPrefix.GetWindowText(fn,sizeof(fn));
	else
		fn[0]='\0';
	if(RegSetValueEx(m_hServerKey,_T("RepositoryPrefix"),NULL,REG_SZ,(BYTE*)fn,(_tcslen(fn)+1)*sizeof(TCHAR)))
		AfxMessageBox(_T("RegSetValueEx failed"),MB_ICONSTOP);

	return CPropertyPage::OnApply();
}

void CRepositoryPage::GetRootList()
{
	TCHAR buf[MAX_PATH];
	DWORD bufLen;
	DWORD dwType;
	CString tmp;

	for(int n=0; n<MAX_REPOSITORIES; n++)
	{
		tmp.Format(_T("Repository%d"),n);
		bufLen=sizeof(buf);
		if(RegQueryValueEx(m_hServerKey,tmp,NULL,&dwType,(BYTE*)buf,&bufLen))
			continue;
		if(dwType!=REG_SZ)
			continue;

		TCHAR *p = buf;
		while((p=_tcschr(p,'\\'))!=NULL)
			*p='/';

		m_Roots.push_back(buf);
	}
}

void CRepositoryPage::DrawRootList()
{
	CString tmp,prefix;

	m_edPrefix.GetWindowText(prefix);
	prefix.Replace('\\','/');
	m_listRoot.ResetContent();
	for(size_t n=0; n<m_Roots.size(); n++)
	{
		LPCTSTR buf=(LPCTSTR)m_Roots[n];
		if(!*buf)
			continue;
		if(!prefix.GetLength())
		{
			tmp=buf;
		}
		else if(!_tcsnicmp(prefix,buf,prefix.GetLength()))
		{
			tmp.Format(_T("%s"),buf+prefix.GetLength(),buf);
			if(tmp.Left(1)!="/")
				tmp="/"+tmp;
		}
		else
		{
			tmp.Format(_T("(%s)"),buf);
		}

		if(m_listRoot.FindStringExact(-1,tmp)>=0)
		{
			m_Roots[n]="";
			continue;
		}

		m_listRoot.SetItemData(m_listRoot.AddString(tmp),n);
	}
}

void CRepositoryPage::RebuildRootList()
{
	CString tmp,tmp2,path,prefix;
	int j;

	for(size_t n=0; n<MAX_REPOSITORIES; n++)
	{
		tmp.Format(_T("Repository%d"),n);
		RegDeleteValue(m_hServerKey,tmp);
	}

	m_edPrefix.GetWindowText(prefix);
	for(n=0,j=0; n<m_Roots.size(); n++)
	{
		path=m_Roots[n];
		if(path.GetLength())
		{
			tmp.Format(_T("Repository%d"),j++);
			RegSetValueEx(m_hServerKey,tmp,NULL,REG_SZ,(BYTE*)(LPCTSTR)path,(path.GetLength()+1)*sizeof(TCHAR));
		}
	}
}

void CRepositoryPage::OnAddroot() 
{
	CNewRootDialog dlg;
	m_edPrefix.GetWindowText(dlg.m_RepoPrefix);
	if(m_szInstallPath.GetLength())
		dlg.m_szInstallPath=m_szInstallPath+"\\";
	if(dlg.DoModal()==IDOK)
	{
		m_Roots.push_back(dlg.m_szRoot);
		DrawRootList();
	}
	SetModified();
}

void CRepositoryPage::OnDeleteroot() 
{
	int nSel = m_listRoot.GetCurSel();

	if(nSel<0) return;
	m_Roots[m_listRoot.GetItemData(nSel)]="";
	m_listRoot.DeleteString(nSel);
	m_btDelete.EnableWindow(false);
	SetModified();
}

void CRepositoryPage::OnRepositoryprefix() 
{
	if(m_btRepoPrefix.GetCheck())
		m_btChangePrefix.EnableWindow(TRUE);
	else
	{
		m_btChangePrefix.EnableWindow(FALSE);
		m_edPrefix.SetWindowText(_T(""));
	}
	DrawRootList();
	SetModified();
}

void CRepositoryPage::OnChangeprefix() 
{
	TCHAR fn[MAX_PATH];
	LPITEMIDLIST idl,idlroot;
	IMalloc *mal;
	
	SHGetSpecialFolderLocation(m_hWnd, CSIDL_DRIVES, &idlroot);
	SHGetMalloc(&mal);
#ifdef JP_STRING
	BROWSEINFO bi = { m_hWnd, idlroot, fn, _T("CVS プリフィックス ディレクトリを選択してください。"), BIF_NEWDIALOGSTYLE|BIF_RETURNONLYFSDIRS|BIF_RETURNFSANCESTORS, BrowseValid };
#else
	BROWSEINFO bi = { m_hWnd, idlroot, fn, _T("Select CVS prefix directory."), BIF_NEWDIALOGSTYLE|BIF_RETURNONLYFSDIRS|BIF_RETURNFSANCESTORS, BrowseValid };
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

	TCHAR *p = fn;
	while((p=_tcschr(p,'\\'))!=NULL)
		*p='/';

	m_edPrefix.SetWindowText(fn);

	DrawRootList();
	SetModified();
}




