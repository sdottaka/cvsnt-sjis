#if !defined(AFX_REPOSITORYPAGE_H__D778308C_9BC8_4C67_AC8B_2ACEE694BC44__INCLUDED_)
#define AFX_REPOSITORYPAGE_H__D778308C_9BC8_4C67_AC8B_2ACEE694BC44__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// RepositoryPage.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CRepositoryPage dialog

class CRepositoryPage : public CPropertyPage
{
	DECLARE_DYNCREATE(CRepositoryPage)

// Construction
public:
	CRepositoryPage();
	~CRepositoryPage();

	HKEY m_hServerKey;
	CString m_szInstallPath;

	void GetRootList();
	void DrawRootList();
	void RebuildRootList();

	vector<CString> m_Roots;

// Dialog Data
	//{{AFX_DATA(CRepositoryPage)
	enum { IDD = IDD_PAGE3 };
	CButton	m_btChangePrefix;
	CEdit	m_edPrefix;
	CButton	m_btRepoPrefix;
	CButton	m_btDelete;
	CButton	m_btAdd;
	CListBox	m_listRoot;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CRepositoryPage)
	public:
	virtual BOOL OnApply();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CRepositoryPage)
	virtual BOOL OnInitDialog();
	afx_msg void OnSelchangeRootlist();
	afx_msg void OnSelcancelRootlist();
	afx_msg void OnAddroot();
	afx_msg void OnDeleteroot();
	afx_msg void OnRepositoryprefix();
	afx_msg void OnChangeprefix();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_REPOSITORYPAGE_H__D778308C_9BC8_4C67_AC8B_2ACEE694BC44__INCLUDED_)
