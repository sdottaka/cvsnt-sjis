// PasswordDialog.cpp : implementation file
//

#include "stdafx.h"
#include "cvsagent.h"
#include "PasswordDialog.h"
#include ".\passworddialog.h"

void DDX_Text(CDataExchange *pDX, int nIDC, std::string& value)
{
	if(pDX->m_bSaveAndValidate)
	{
		int len = ::GetWindowTextLength(::GetDlgItem(pDX->m_pDlgWnd->GetSafeHwnd(),nIDC));
		value.resize(len);
		::GetDlgItemTextA(pDX->m_pDlgWnd->GetSafeHwnd(),nIDC,(char*)value.data(),len+1);
	}
	else
	{
		::SetDlgItemTextA(pDX->m_pDlgWnd->GetSafeHwnd(),nIDC,value.c_str());
	}
}

// CPasswordDialog dialog

IMPLEMENT_DYNAMIC(CPasswordDialog, CDialog)
CPasswordDialog::CPasswordDialog(CWnd* pParent /*=NULL*/)
	: CDialog(CPasswordDialog::IDD, pParent)
{
}

CPasswordDialog::~CPasswordDialog()
{
}

void CPasswordDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_CVSROOT, m_szCvsRoot);
	DDX_Text(pDX, IDC_PASSWORD, m_szPassword);
}


BEGIN_MESSAGE_MAP(CPasswordDialog, CDialog)
END_MESSAGE_MAP()


// CPasswordDialog message handlers

BOOL CPasswordDialog::OnInitDialog()
{
	CDialog::OnInitDialog();

	if(g_bTopmost)
		SetWindowPos(&wndTopMost, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
	return TRUE;  
}

