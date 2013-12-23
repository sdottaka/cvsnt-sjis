#pragma once
#include "afxwin.h"


// CSslSettingPage dialog

class CSslSettingPage : public CPropertyPage
{
	DECLARE_DYNAMIC(CSslSettingPage)

public:
	CSslSettingPage();
	virtual ~CSslSettingPage();

// Dialog Data
	enum { IDD = IDD_PAGE4 };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	HKEY m_hServerKey;

	CEdit m_edCertificateFile;
	CEdit m_edPrivateKeyFile;
	virtual BOOL OnInitDialog();
	virtual BOOL OnApply();
	afx_msg void OnBnClickedSslcert();
	afx_msg void OnBnClickedPrivatekey();
};
