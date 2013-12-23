// SslSettingPage.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "SslSettingPage.h"


// CSslSettingPage dialog

IMPLEMENT_DYNAMIC(CSslSettingPage, CTooltipPropertyPage)
CSslSettingPage::CSslSettingPage()
	: CTooltipPropertyPage(CSslSettingPage::IDD)
{
}

CSslSettingPage::~CSslSettingPage()
{
}

void CSslSettingPage::DoDataExchange(CDataExchange* pDX)
{
	CTooltipPropertyPage::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_EDIT2, m_edCertificateFile);
	DDX_Control(pDX, IDC_EDIT3, m_edPrivateKeyFile);
}


BEGIN_MESSAGE_MAP(CSslSettingPage, CTooltipPropertyPage)
	ON_BN_CLICKED(IDC_SSLCERT, OnBnClickedSslcert)
	ON_BN_CLICKED(IDC_PRIVATEKEY, OnBnClickedPrivatekey)
END_MESSAGE_MAP()

// CSslSettingPage message handlers

BOOL CSslSettingPage::OnInitDialog()
{
	TCHAR buf[4096];
	DWORD bufLen,dwType=REG_SZ;

	CTooltipPropertyPage::OnInitDialog();

	bufLen=sizeof(buf);
	if(RegQueryValueEx(g_hServerKey,_T("CertificateFile"),NULL,&dwType,(BYTE*)buf,&bufLen))
	{
		// Not set
		*buf='\0';
	}

	TCHAR *p = buf;
	while((p=_tcschr(p,'/'))!=NULL)
		*p='\\';


	m_edCertificateFile.SetWindowText((LPCTSTR)buf);

	bufLen=sizeof(buf);
	if(RegQueryValueEx(g_hServerKey,_T("PrivateKeyFile"),NULL,&dwType,(BYTE*)buf,&bufLen))
	{
		// Not set
		*buf='\0';
	}
	p = buf;
	while((p=_tcschr(p,'/'))!=NULL)
		*p='\\';

	m_edPrivateKeyFile.SetWindowText(buf);

	if (!g_bPrivileged)
	{
		GetDlgItem(IDC_SSLCERT)->EnableWindow(FALSE);
		GetDlgItem(IDC_PRIVATEKEY)->EnableWindow(FALSE);
	}

	return TRUE;
}

BOOL CSslSettingPage::OnApply()
{
	TCHAR fn[4096];
	m_edCertificateFile.GetWindowText(fn,sizeof(fn)/sizeof(fn[0]));
	if(RegSetValueEx(g_hServerKey,_T("CertificateFile"),NULL,REG_EXPAND_SZ,(BYTE*)fn,(_tcslen(fn)+1)*sizeof(TCHAR)))
		AfxMessageBox(_T("RegSetValueEx failed"),MB_ICONSTOP);
	m_edPrivateKeyFile.GetWindowText(fn,sizeof(fn)/sizeof(fn[0]));
	if(RegSetValueEx(g_hServerKey,_T("PrivateKeyFile"),NULL,REG_EXPAND_SZ,(BYTE*)fn,(_tcslen(fn)+1)*sizeof(TCHAR)))
		AfxMessageBox(_T("RegSetValueEx failed"),MB_ICONSTOP);
	return CTooltipPropertyPage::OnApply();
}

void CSslSettingPage::OnBnClickedSslcert()
{
	TCHAR fn[4096];
#ifdef JP_STRING
	OPENFILENAME ofn = { sizeof(OPENFILENAME), m_hWnd, NULL, _T("PEM �ؖ��� �t�@�C��\0*.pem\0"), NULL, 0, 0, fn, sizeof(fn), NULL, 0, NULL, NULL, OFN_FILEMUSTEXIST|OFN_PATHMUSTEXIST|OFN_HIDEREADONLY, 0, 0, _T("pem") };
#else
	OPENFILENAME ofn = { sizeof(OPENFILENAME), m_hWnd, NULL, _T("PEM Private key files\0*.pem\0"), NULL, 0, 0, fn, sizeof(fn), NULL, 0, NULL, NULL, OFN_FILEMUSTEXIST|OFN_PATHMUSTEXIST|OFN_HIDEREADONLY, 0, 0, _T("pem") };
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
	OPENFILENAME ofn = { sizeof(OPENFILENAME), m_hWnd, NULL, _T("PEM �閧�� �t�@�C��\0*.pem\0"), NULL, 0, 0, fn, sizeof(fn), NULL, 0, NULL, NULL, OFN_FILEMUSTEXIST|OFN_PATHMUSTEXIST|OFN_HIDEREADONLY, 0, 0, _T("pem") };
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
