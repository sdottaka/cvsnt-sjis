// AboutDialog.cpp : implementation file
//

#include "stdafx.h"
#include "cvsagent.h"
#include "AboutDialog.h"
#include ".\aboutdialog.h"


// CAboutDialog dialog

IMPLEMENT_DYNAMIC(CAboutDialog, CDialog)
CAboutDialog::CAboutDialog(CWnd* pParent /*=NULL*/)
	: CDialog(CAboutDialog::IDD, pParent)
{
}

CAboutDialog::~CAboutDialog()
{
}

void CAboutDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CAboutDialog, CDialog)
END_MESSAGE_MAP()


// CAboutDialog message handlers

BOOL CAboutDialog::OnInitDialog()
{
	CDialog::OnInitDialog();

	::SetDlgItemTextA(GetSafeHwnd(),IDC_VERSION,"Version "CVSNT_PRODUCTVERSION_STRING);
	return TRUE; 
}

