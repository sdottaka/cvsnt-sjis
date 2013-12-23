// SslSettingPage.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "SslSettingPage.h"


// CSslSettingPage dialog

IMPLEMENT_DYNAMIC(CSslSettingPage, CPropertyPage)
CSslSettingPage::CSslSettingPage()
	: CPropertyPage(CSslSettingPage::IDD)
{
	m_hServerKey=NULL;
}

CSslSettingPage::~CSslSettingPage()
{
	RegCloseKey(m_hServerKey);
}

void CSslSettingPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_EDIT2, m_edCertificateFile);
	DDX_Control(pDX, IDC_EDIT3, m_edPrivateKeyFile);
}


BEGIN_MESSAGE_MAP(CSslSettingPage, CPropertyPage)
	ON_BN_CLICKED(IDC_SSLCERT, OnBnClickedSslcert)
	ON_BN_CLICKED(IDC_PRIVATEKEY, OnBnClickedPrivatekey)
END_MESSAGE_MAP()

// CSslSettingPage message handlers

BOOL CSslSettingPage::OnInitDialog()
{
	TCHAR buf[4096];
	DWORD bufLen,dwType=REG_SZ;

	CPropertyPage::OnInitDialog();

	if(!m_hServerKey && RegCreateKeyEx(HKEY_LOCAL_MACHINE,_T("Software\\CVS\\Pserver"),NULL,_T(""),REG_OPTION_NON_VOLATILE,KEY_ALL_ACCESS,NULL,&m_hServerKey,NULL))
		return FALSE;

	bufLen=sizeof(buf);
	if(RegQueryValueEx(m_hServerKey,_T("CertificateFile"),NULL,&dwType,(BYTE*)buf,&bufLen))
	{
		// Not set
		*buf='\0';
	}

	TCHAR *p = buf;
	while((p=_tcschr(p,'/'))!=NULL)
		*p='\\';


	m_edCertificateFile.SetWindowText((LPCTSTR)buf);

	bufLen=sizeof(buf);
	if(RegQueryValueEx(m_hServerKey,_T("PrivateKeyFile"),NULL,&dwType,(BYTE*)buf,&bufLen))
	{
		// Not set
		*buf='\0';
	}
	p = buf;
	while((p=_tcschr(p,'/'))!=NULL)
		*p='\\';

	m_edPrivateKeyFile.SetWindowText(buf);

	return TRUE;
}

BOOL CSslSettingPage::OnApply()
{
	TCHAR fn[4096];
	m_edCertificateFile.GetWindowText(fn,sizeof(fn));
	if(RegSetValueEx(m_hServerKey,_T("CertificateFile"),NULL,REG_EXPAND_SZ,(BYTE*)fn,(_tcslen(fn)+1)*sizeof(TCHAR)))
		AfxMessageBox(_T("RegSetValueEx failed"),MB_ICONSTOP);
	m_edPrivateKeyFile.GetWindowText(fn,sizeof(fn));
	if(RegSetValueEx(m_hServerKey,_T("PrivateKeyFile"),NULL,REG_EXPAND_SZ,(BYTE*)fn,(_tcslen(fn)+1)*sizeof(TCHAR)))
		AfxMessageBox(_T("RegSetValueEx failed"),MB_ICONSTOP);
	return CPropertyPage::OnApply();
}

void CSslSettingPage::OnBnClickedSslcert()
{
	TCHAR fn[4096];
#ifdef JP_STRING
	OPENFILENAME ofn = { sizeof(OPENFILENAME), m_hWnd, NULL, _T("PEM 証明書 ファイル\0*.pem\0"), NULL, 0, 0, fn, sizeof(fn), NULL, 0, NULL, NULL, OFN_FILEMUSTEXIST|OFN_PATHMUSTEXIST|
OFN_HIDEREADONLY, 0, 0, _T("pem") };
#else
	OPENFILENAME ofn = { sizeof(OPENFILENAME), m_hWnd, NULL, _T("PEM Private key files\0*.pem\0"), NULL, 0, 0, fn, sizeof(fn), NULL, 0, NULL, NULL, OFN_FILEMUSTEXIST|OFN_PATHMUSTEXIST|
OFN_HIDEREADONLY, 0, 0, _T("pem") };
#endif

	m_edCertificateFile.GetWindowText(fn,sizeof(fn));
	if(GetOpenFileName(&ofn))
	{
		m_edCertificateFile.SetWindowText(fn);
		SetModified();
	}
}

void CSslSettingPage::OnBnClickedPrivatekey()
{
	TCHAR fn[4096]={0};
#ifdef JP_STRING
	OPENFILENAME ofn = { sizeof(OPENFILENAME), m_hWnd, NULL, _T("PEM 秘密鍵 ファイル\0*.pem\0"), NULL, 0, 0, fn, sizeof(fn), NULL, 0, NULL, NULL, OFN_FILEMUSTEXIST|OFN_PATHMUSTEXIST|OFN_HIDEREADONLY, 0, 0, _T("pem") };
#else
	OPENFILENAME ofn = { sizeof(OPENFILENAME), m_hWnd, NULL, _T("PEM Private key files\0*.pem\0"), NULL, 0, 0, fn, sizeof(fn), NULL, 0, NULL, NULL, OFN_FILEMUSTEXIST|OFN_PATHMUSTEXIST|OFN_HIDEREADONLY, 0, 0, _T("pem") };
#endif

	m_edPrivateKeyFile.GetWindowText(fn,sizeof(fn));
	if(GetOpenFileName(&ofn))
	{
		m_edPrivateKeyFile.SetWindowText(fn);
		SetModified();
	}	
}
