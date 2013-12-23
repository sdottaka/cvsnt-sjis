#if !defined(AFX_NEWROOTDIALOG_H__1DDA657E_929C_4496_AF11_A7F908CAD40F__INCLUDED_)
#define AFX_NEWROOTDIALOG_H__1DDA657E_929C_4496_AF11_A7F908CAD40F__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// NewRootDialog.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CNewRootDialog dialog

class CNewRootDialog : public CDialog
{
// Construction
public:
	CNewRootDialog(CWnd* pParent = NULL);   // standard constructor

	CString m_RepoPrefix;
	CString m_szInstallPath;

// Dialog Data
	//{{AFX_DATA(CNewRootDialog)
	enum { IDD = IDD_NEWROOT };
	CString	m_szRoot;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CNewRootDialog)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CNewRootDialog)
	afx_msg void OnSelect();
	virtual void OnOK();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_NEWROOTDIALOG_H__1DDA657E_929C_4496_AF11_A7F908CAD40F__INCLUDED_)
