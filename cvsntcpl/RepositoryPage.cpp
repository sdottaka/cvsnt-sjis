// RepositoryPage.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "NewRootDialog.h"
#include "RepositoryPage.h"
#include ".\repositorypage.h"

#include <direct.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define MAX_REPOSITORIES 1024

/////////////////////////////////////////////////////////////////////////////
// CRepositoryPage property page

IMPLEMENT_DYNCREATE(CRepositoryPage, CTooltipPropertyPage)

CRepositoryPage::CRepositoryPage() : CTooltipPropertyPage(CRepositoryPage::IDD)
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
	CTooltipPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CRepositoryPage)
	DDX_Control(pDX, IDC_DELETEROOT, m_btDelete);
	DDX_Control(pDX, IDC_ADDROOT, m_btAdd);
	DDX_Control(pDX, IDC_EDITROOT, m_btEdit);
	DDX_Control(pDX, IDC_ROOTLIST, m_listRoot);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CRepositoryPage, CTooltipPropertyPage)
	//{{AFX_MSG_MAP(CRepositoryPage)
	ON_BN_CLICKED(IDC_ADDROOT, OnAddroot)
	ON_BN_CLICKED(IDC_DELETEROOT, OnDeleteroot)
	ON_BN_CLICKED(IDC_EDITROOT, OnEditroot)
	//}}AFX_MSG_MAP
	ON_NOTIFY(NM_DBLCLK, IDC_ROOTLIST, OnNMDblclkRootlist)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_ROOTLIST, OnLvnItemchangedRootlist)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CRepositoryPage message handlers

BOOL CRepositoryPage::OnInitDialog() 
{
	DWORD bufLen,dwType;
	TCHAR buf[_MAX_PATH];

	CTooltipPropertyPage::OnInitDialog();
	
	if(!m_hServerKey && RegCreateKeyEx(HKEY_LOCAL_MACHINE,_T("Software\\CVS\\Pserver"),NULL,_T(""),REG_OPTION_NON_VOLATILE,KEY_ALL_ACCESS,NULL,&m_hServerKey,NULL))
	{ 
		fprintf(stderr,"Couldn't create HKLM\\Software\\CVS\\Pserver key, error %d\n",GetLastError());
		return -1;
	}

	m_listRoot.InsertColumn(0,_T("Name"),LVCFMT_LEFT,130);
	m_listRoot.InsertColumn(1,_T("Root"),LVCFMT_LEFT,130);

	if(GetRootList())
		GetParent()->PostMessage(PSM_CHANGED, (WPARAM)m_hWnd); /* SetModified happens too early */
	DrawRootList();
	OnLvnItemchangedRootlist(NULL,NULL);

	bufLen=sizeof(buf);
	if(RegQueryValueEx(m_hServerKey,_T("InstallPath"),NULL,&dwType,(BYTE*)buf,&bufLen))
	{
		// Not set
		*buf='\0';
	}
	m_szInstallPath=buf;

	return TRUE;
}

int CRepositoryPage::GetListSelection(CListCtrl& list)
{
    int nItem = -1;
    POSITION nPos = list.GetFirstSelectedItemPosition();
    if (nPos)
        nItem = list.GetNextSelectedItem(nPos);
    return nItem;
}

BOOL CRepositoryPage::OnApply() 
{
	RebuildRootList();

	return CTooltipPropertyPage::OnApply();
}

bool CRepositoryPage::GetRootList()
{
	TCHAR buf[MAX_PATH],buf2[MAX_PATH];
	std::wstring prefix;
	DWORD bufLen;
	DWORD dwType;
	CString tmp;
	int drive;
	bool bModified = false;

	bufLen=sizeof(buf);
	if(!RegQueryValueEx(m_hServerKey,_T("RepositoryPrefix"),NULL,&dwType,(BYTE*)buf,&bufLen))
	{
		TCHAR *p = buf;
		while((p=_tcschr(p,'\\'))!=NULL)
			*p='/';
		p=buf+_tcslen(buf)-1;
		if(*p=='/')
			*p='\0';
		prefix = buf;
		bModified = true; /* Save will delete this value */
	}

	drive = _getdrive() + 'A' - 1;

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

		tmp.Format(_T("Repository%dName"),n);
		bufLen=sizeof(buf2);
		if(RegQueryValueEx(m_hServerKey,tmp,NULL,&dwType,(BYTE*)buf2,&bufLen))
		{
			_tcscpy(buf2,buf);
			if(prefix.size() && !_tcsnicmp(prefix.c_str(),buf,prefix.size()))
				_tcscpy(buf2,&buf[prefix.size()]);
			else
				_tcscpy(buf2,buf);
			if(buf[1]!=':')
				_sntprintf(buf,sizeof(buf),_T("%c:%s"),drive,buf2);
			p=buf2+_tcslen(buf2)-1;
			if(*p=='/')
				*p='\0';
			bModified = true;
		}
		else if(dwType!=REG_SZ)
			continue;

		RootStruct r;
		r.root = buf;
		r.name = buf2;
		r.valid = true;

		m_Roots.push_back(r);
	}
	return bModified;
}

void CRepositoryPage::DrawRootList()
{
	m_listRoot.DeleteAllItems();
	for(size_t n=0; n<m_Roots.size(); n++)
	{
		if(!m_Roots[n].valid)
			continue;

		LV_FINDINFO lvf;

		lvf.flags = LVFI_STRING;
		lvf.psz = m_Roots[n].name.c_str();
		if(m_listRoot.FindItem(&lvf)>=0)
		{
			m_Roots[n].valid=false;
			continue;
		}

		int i = m_listRoot.InsertItem(n,m_Roots[n].name.c_str());
		m_listRoot.SetItem(i,0,LVIF_PARAM,0,0,0,0,n,0);
		m_listRoot.SetItem(i,1,LVIF_TEXT,m_Roots[n].root.c_str(),0,0,0,0,0);
	}
}

void CRepositoryPage::RebuildRootList()
{
	std::wstring path,desc;
	TCHAR tmp[64];
	int j;
	size_t n;

	for(n=0; n<MAX_REPOSITORIES; n++)
	{
		_sntprintf(tmp,sizeof(tmp),_T("Repository%d"),n);
		RegDeleteValue(m_hServerKey,tmp);
	}

	for(n=0,j=0; n<m_Roots.size(); n++)
	{
		path=m_Roots[n].root;
		desc=m_Roots[n].name;
		if(m_Roots[n].valid)
		{
			_sntprintf(tmp,sizeof(tmp),_T("Repository%d"),j);
			RegSetValueEx(m_hServerKey,tmp,NULL,REG_SZ,(BYTE*)path.c_str(),(path.length()+1)*sizeof(TCHAR));
			_sntprintf(tmp,sizeof(tmp),_T("Repository%dName"),j);
			RegSetValueEx(m_hServerKey,tmp,NULL,REG_SZ,(BYTE*)desc.c_str(),(desc.length()+1)*sizeof(TCHAR));
			j++;
		}
	}

	RegDeleteValue(m_hServerKey,_T("RepositoryPrefix"));
}

void CRepositoryPage::OnAddroot() 
{
	CNewRootDialog dlg;
	if(m_szInstallPath.GetLength())
		dlg.m_szInstallPath=m_szInstallPath+"\\";
	if(dlg.DoModal()==IDOK)
	{
		RootStruct r;
		r.valid=true;
		r.name=dlg.m_szName;
		r.root=dlg.m_szRoot;
		m_Roots.push_back(r);
		DrawRootList();
		SetModified();
	}
}

void CRepositoryPage::OnDeleteroot() 
{
	int nSel = GetListSelection(m_listRoot);

	if(nSel<0) return;
	m_Roots[m_listRoot.GetItemData(nSel)].valid=false;
	m_listRoot.DeleteItem(nSel);
	m_btDelete.EnableWindow(false);
	SetModified();
}

void CRepositoryPage::OnEditroot()
{
	int nSel = GetListSelection(m_listRoot);

	if(nSel<0) return;

	RootStruct& r = m_Roots[m_listRoot.GetItemData(nSel)];
	CNewRootDialog dlg;
	if(m_szInstallPath.GetLength())
		dlg.m_szInstallPath=m_szInstallPath+"\\";
	dlg.m_szName = r.name.c_str();
	dlg.m_szRoot = r.root.c_str();
	if(dlg.DoModal()==IDOK)
	{
		r.name=dlg.m_szName;
		r.root=dlg.m_szRoot;
		DrawRootList();
		SetModified();
	}
}

void CRepositoryPage::OnNMDblclkRootlist(NMHDR *pNMHDR, LRESULT *pResult)
{
	OnEditroot();
	*pResult = 0;
}

void CRepositoryPage::OnLvnItemchangedRootlist(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLISTVIEW pNMListView = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	if (!pNMListView || (pNMListView->uChanged & LVIF_STATE && ((pNMListView->uNewState & LVIS_SELECTED) != (pNMListView->uOldState & LVIS_SELECTED))))
	{
		m_btAdd.EnableWindow(m_listRoot.GetItemCount()<MAX_REPOSITORIES);
		m_btDelete.EnableWindow(m_listRoot.GetItemCount()>0 && GetListSelection(m_listRoot)>=0);
		m_btEdit.EnableWindow(m_listRoot.GetItemCount()>0 && GetListSelection(m_listRoot)>=0);
	}

	if(pResult)
		*pResult = 0;
}
