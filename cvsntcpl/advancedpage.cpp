// AdvancedPage.cpp : implementation file
//

#include "stdafx.h"
#include "cvsnt.h"
#include "AdvancedPage.h"
#include ".\advancedpage.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CAdvancedPage property page

IMPLEMENT_DYNCREATE(CAdvancedPage, CTooltipPropertyPage)

CAdvancedPage::CAdvancedPage() : CTooltipPropertyPage(CAdvancedPage::IDD)
{
	//{{AFX_DATA_INIT(CAdvancedPage)
	//}}AFX_DATA_INIT
	m_hServerKey=NULL;
}

CAdvancedPage::~CAdvancedPage()
{
}

void CAdvancedPage::DoDataExchange(CDataExchange* pDX)
{
	CTooltipPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAdvancedPage)
	DDX_Control(pDX, IDC_NODOMAIN, m_btNoDomain);
	DDX_Control(pDX, IDC_IMPERSONATE, m_btImpersonate);
	DDX_Control(pDX, IDC_EDIT1, m_edTempDir);
	DDX_Control(pDX, IDC_LOCKSERVER, m_edLockServer);
	DDX_Control(pDX, IDC_ENCRYPTION, m_cbEncryption);
	DDX_Control(pDX, IDC_COMPRESSION, m_cbCompression);
	DDX_Control(pDX, IDC_NOREVERSEDNS, m_cbNoReverseDns);
	DDX_Control(pDX, IDC_SPIN1, m_sbServerPort);
	DDX_Control(pDX, IDC_SPIN2, m_sbLockPort);
	DDX_Control(pDX, IDC_LOCKSERVERLOCAL, m_btLockServerLocal);
	//}}AFX_DATA_MAP
	DDX_Control(pDX, IDC_FAKEUNIX, m_btFakeUnix);
	DDX_Control(pDX, IDC_ENABLERENAME, m_btEnableRename);
	DDX_Control(pDX, IDC_ALLOWTRACE, m_btAllowTrace);
}


BEGIN_MESSAGE_MAP(CAdvancedPage, CTooltipPropertyPage)
	//{{AFX_MSG_MAP(CAdvancedPage)
	ON_BN_CLICKED(IDC_CHANGETEMP, OnChangetemp)
	ON_BN_CLICKED(IDC_IMPERSONATE, OnImpersonate)
	ON_EN_CHANGE(IDC_PSERVERPORT, OnChangePserverport)
	ON_EN_CHANGE(IDC_LOCKSERVERPORT, OnChangeLockserverport)
	ON_BN_CLICKED(IDC_NODOMAIN, OnNodomain)
	ON_CBN_SELENDOK(IDC_ENCRYPTION, OnCbnSelendokEncryption)
	ON_CBN_SELENDOK(IDC_COMPRESSION, OnCbnSelendokCompression)
	ON_BN_CLICKED(IDC_NOREVERSEDNS, OnBnClickedNoreversedns)
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(IDC_FAKEUNIX, OnBnClickedFakeunix)
	ON_BN_CLICKED(IDC_ENABLERENAME, OnBnClickedEnablerename)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CAdvancedPage message handlers

BOOL CAdvancedPage::OnInitDialog() 
{
	int t;
	BYTE buf[_MAX_PATH*sizeof(TCHAR)];
	DWORD bufLen;
	DWORD dwType;

	CTooltipPropertyPage::OnInitDialog();
	
	if(!m_hServerKey && RegCreateKeyEx(HKEY_LOCAL_MACHINE,_T("Software\\CVS\\Pserver"),NULL,_T(""),REG_OPTION_NON_VOLATILE,KEY_ALL_ACCESS,NULL,&m_hServerKey,NULL))
	{ 
		fprintf(stderr,"Couldn't create HKLM\\Software\\CVS\\Pserver key, error %d\n",GetLastError());
		return -1;
	}
	
	m_btImpersonate.SetCheck((t=QueryDword(_T("Impersonation")))>=0?t:1);
	m_btNoDomain.SetCheck((t=QueryDword(_T("DontUseDomain")))>=0?t:0);
	m_cbNoReverseDns.SetCheck((t=QueryDword(_T("NoReverseDns")))>=0?t:0);
	m_btLockServerLocal.SetCheck((t=QueryDword(_T("LockServerLocal")))>=0?t:1);
	m_btFakeUnix.SetCheck((t=QueryDword(_T("FakeUnixCvs")))>=0?t:0);
	m_btEnableRename.SetCheck((t=QueryDword(_T("EnableRename")))>=0?t:0);
	m_btAllowTrace.SetCheck((t=QueryDword(_T("AllowTrace")))>=0?t:0);
	SetDlgItemInt(IDC_PSERVERPORT,(t=QueryDword(_T("PServerPort")))>=0?t:2401,FALSE);
	bufLen=sizeof(buf);
	if(RegQueryValueEx(m_hServerKey,_T("LockServer"),NULL,&dwType,buf,&bufLen))
	{
		SetDlgItemText(IDC_LOCKSERVER,_T("localhost"));
		SetDlgItemInt(IDC_LOCKSERVERPORT,(t=QueryDword(_T("LockServerPort")))>=0?t:2402,FALSE);
	}
	else
	{
		RegDeleteValue(m_hServerKey,_T("LockServerPort"));
		TCHAR *p=_tcschr((TCHAR*)buf,':');
		if(p)
			*p='\0';
		m_edLockServer.SetWindowText((LPCTSTR)buf);
		SetDlgItemInt(IDC_LOCKSERVERPORT,p?_tstoi(p+1):2402,FALSE);
	}

	SendDlgItemMessage(IDC_PSERVERPORT,EM_LIMITTEXT,4);
	SendDlgItemMessage(IDC_LOCKSERVERPORT,EM_LIMITTEXT,4);

	m_sbServerPort.SetRange32(1,65535);
	m_sbLockPort.SetRange32(1,65535);

	bufLen=sizeof(buf);
	if(RegQueryValueEx(m_hServerKey,_T("TempDir"),NULL,&dwType,buf,&bufLen) &&
	   SHRegGetUSValue(_T("SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment"),_T("TEMP"),NULL,(LPVOID)buf,&bufLen,TRUE,NULL,0) &&
	   !GetEnvironmentVariable(_T("TEMP"),(LPTSTR)buf,sizeof(buf)) &&
	   !GetEnvironmentVariable(_T("TMP"),(LPTSTR)buf,sizeof(buf)))
		{
			// Not set
			*buf='\0';
		}

	m_edTempDir.SetWindowText((LPCTSTR)buf);

	m_cbEncryption.ResetContent();
	m_cbEncryption.SetItemData(m_cbEncryption.AddString(_T("Optional")),0);
	m_cbEncryption.SetItemData(m_cbEncryption.AddString(_T("Request Authentication")),1);
	m_cbEncryption.SetItemData(m_cbEncryption.AddString(_T("Request Encryption")),2);
	m_cbEncryption.SetItemData(m_cbEncryption.AddString(_T("Require Authentication")),3);
	m_cbEncryption.SetItemData(m_cbEncryption.AddString(_T("Require Encryption")),4);
	m_cbCompression.ResetContent();
	m_cbCompression.SetItemData(m_cbCompression.AddString(_T("Optional")),0);
	m_cbCompression.SetItemData(m_cbCompression.AddString(_T("Request Compression")),1);
	m_cbCompression.SetItemData(m_cbCompression.AddString(_T("Require Compression")),2);

	m_cbEncryption.SetCurSel((t=QueryDword(_T("EncryptionLevel")))>=0?t:0);
	m_cbCompression.SetCurSel((t=QueryDword(_T("CompressionLevel")))>=0?t:0);

	return TRUE;  
}

void CAdvancedPage::OnChangetemp() 
{
	TCHAR fn[MAX_PATH];
	LPITEMIDLIST idl,idlroot;
	IMalloc *mal;
	
	SHGetSpecialFolderLocation(m_hWnd, CSIDL_DRIVES, &idlroot);
	SHGetMalloc(&mal);
	BROWSEINFO bi = { m_hWnd, idlroot, fn, _T("Select folder for CVS temporary files.  This folder must be writeable by all users that wish to use CVS."), BIF_NEWDIALOGSTYLE|BIF_RETURNONLYFSDIRS|BIF_RETURNFSANCESTORS, BrowseValid };
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

	m_edTempDir.SetWindowText(fn);

	SetModified();
}

BOOL CAdvancedPage::OnApply() 
{
	DWORD dwVal;
	TCHAR fn[MAX_PATH];

	dwVal=m_btImpersonate.GetCheck()?1:0;

	if(RegSetValueEx(m_hServerKey,_T("Impersonation"),NULL,REG_DWORD,(BYTE*)&dwVal,sizeof(DWORD)))
		AfxMessageBox(_T("RegSetValueEx failed"),MB_ICONSTOP);

	dwVal=m_btNoDomain.GetCheck()?1:0;

	if(RegSetValueEx(m_hServerKey,_T("DontUseDomain"),NULL,REG_DWORD,(BYTE*)&dwVal,sizeof(DWORD)))
		AfxMessageBox(_T("RegSetValueEx failed"),MB_ICONSTOP);

	dwVal=m_cbNoReverseDns.GetCheck()?1:0;

	if(RegSetValueEx(m_hServerKey,_T("NoReverseDns"),NULL,REG_DWORD,(BYTE*)&dwVal,sizeof(DWORD)))
		AfxMessageBox(_T("RegSetValueEx failed"),MB_ICONSTOP);

	dwVal=m_btLockServerLocal.GetCheck()?1:0;

	if(RegSetValueEx(m_hServerKey,_T("LockServerLocal"),NULL,REG_DWORD,(BYTE*)&dwVal,sizeof(DWORD)))
		AfxMessageBox(_T("RegSetValueEx failed"),MB_ICONSTOP);

	dwVal=m_btFakeUnix.GetCheck()?1:0;

	if(RegSetValueEx(m_hServerKey,_T("FakeUnixCvs"),NULL,REG_DWORD,(BYTE*)&dwVal,sizeof(DWORD)))
		AfxMessageBox(_T("RegSetValueEx failed"),MB_ICONSTOP);

	dwVal=m_btEnableRename.GetCheck()?1:0;

	if(RegSetValueEx(m_hServerKey,_T("EnableRename"),NULL,REG_DWORD,(BYTE*)&dwVal,sizeof(DWORD)))
		AfxMessageBox(_T("RegSetValueEx failed"),MB_ICONSTOP);

	dwVal=m_btAllowTrace.GetCheck()?1:0;

	if(RegSetValueEx(m_hServerKey,_T("AllowTrace"),NULL,REG_DWORD,(BYTE*)&dwVal,sizeof(DWORD)))
		AfxMessageBox(_T("RegSetValueEx failed"),MB_ICONSTOP);

	dwVal = GetDlgItemInt(IDC_PSERVERPORT,NULL,FALSE);
	if(RegSetValueEx(m_hServerKey,_T("PServerPort"),NULL,REG_DWORD,(BYTE*)&dwVal,sizeof(DWORD)))
		AfxMessageBox(_T("RegSetValueEx failed"),MB_ICONSTOP);

	m_edLockServer.GetWindowText(fn,sizeof(fn)-8);
	dwVal = GetDlgItemInt(IDC_LOCKSERVERPORT,NULL,FALSE);
	_sntprintf(fn+_tcslen(fn),8,_T(":%d"),dwVal);
	if(RegSetValueEx(m_hServerKey,_T("LockServer"),NULL,REG_SZ,(BYTE*)fn,(_tcslen(fn)+1)*sizeof(TCHAR)))
		AfxMessageBox(_T("RegSetValueEx failed"),MB_ICONSTOP);

	m_edTempDir.GetWindowText(fn,sizeof(fn));
	if(RegSetValueEx(m_hServerKey,_T("TempDir"),NULL,REG_SZ,(BYTE*)fn,(_tcslen(fn)+1)*sizeof(TCHAR)))
		AfxMessageBox(_T("RegSetValueEx failed"),MB_ICONSTOP);

	dwVal = m_cbCompression.GetItemData(m_cbCompression.GetCurSel());
	if(RegSetValueEx(m_hServerKey,_T("CompressionLevel"),NULL,REG_DWORD,(BYTE*)&dwVal,sizeof(DWORD)))
		AfxMessageBox(_T("RegSetValueEx failed"),MB_ICONSTOP);

	dwVal = m_cbEncryption.GetItemData(m_cbEncryption.GetCurSel());
	if(RegSetValueEx(m_hServerKey,_T("EncryptionLevel"),NULL,REG_DWORD,(BYTE*)&dwVal,sizeof(DWORD)))
		AfxMessageBox(_T("RegSetValueEx failed"),MB_ICONSTOP);

	return CTooltipPropertyPage::OnApply();
}

DWORD CAdvancedPage::QueryDword(LPCTSTR szKey)
{
	BYTE buf[64];
	DWORD bufLen=sizeof(buf);
	DWORD dwType;
	if(RegQueryValueEx(m_hServerKey,szKey,NULL,&dwType,buf,&bufLen))
		return -1;
	if(dwType!=REG_DWORD)
		return -1;
	return *(DWORD*)buf;
}

void CAdvancedPage::OnImpersonate() 
{
	SetModified();
}

void CAdvancedPage::OnChangePserverport() 
{
	SetModified();
}

void CAdvancedPage::OnChangeLockserverport() 
{
	SetModified();
}

void CAdvancedPage::OnNodomain() 
{
	SetModified();
}


void CAdvancedPage::OnCbnSelendokEncryption()
{
	SetModified();
}

void CAdvancedPage::OnCbnSelendokCompression()
{
	SetModified();
}

void CAdvancedPage::OnBnClickedNoreversedns()
{
	SetModified();
}

void CAdvancedPage::OnBnClickedFakeunix()
{
	SetModified();
}

void CAdvancedPage::OnBnClickedEnablerename()
{
	SetModified();
}
