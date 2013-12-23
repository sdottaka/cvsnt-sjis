#include "afxwin.h"
#include "afxcmn.h"
#if !defined(AFX_ADVANCEDPAGE_H__D360303C_4B6C_41C1_9261_1D16722BC461__INCLUDED_)
#define AFX_ADVANCEDPAGE_H__D360303C_4B6C_41C1_9261_1D16722BC461__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// AdvancedPage.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CAdvancedPage dialog

class CAdvancedPage : public CPropertyPage
{
	DECLARE_DYNCREATE(CAdvancedPage)

// Construction
public:
	HKEY m_hServerKey;

	CAdvancedPage();
	~CAdvancedPage();

	DWORD QueryDword(LPCTSTR szKey);

// Dialog Data
	//{{AFX_DATA(CAdvancedPage)
	enum { IDD = IDD_PAGE2 };
	CButton	m_btNoDomain;
	CButton	m_btNtServer;
	CButton	m_btImpersonate;
	CEdit	m_edTempDir;
	CComboBox m_cbEncryption;
	CComboBox m_cbCompression;
	CButton m_cbNoReverseDns;
	CSpinButtonCtrl m_sbServerPort;
	CSpinButtonCtrl m_sbLockPort;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CAdvancedPage)
	public:
	virtual BOOL OnApply();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CAdvancedPage)
	afx_msg void OnChangetemp();
	virtual BOOL OnInitDialog();
	afx_msg void OnImpersonate();
	afx_msg void OnNtserver();
	afx_msg void OnChangePserverport();
	afx_msg void OnChangeLockserverport();
	afx_msg void OnNodomain();
	afx_msg void OnCbnSelendokEncryption();
	afx_msg void OnCbnSelendokCompression();
	afx_msg void OnBnClickedNoreversedns();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ADVANCEDPAGE_H__D360303C_4B6C_41C1_9261_1D16722BC461__INCLUDED_)
