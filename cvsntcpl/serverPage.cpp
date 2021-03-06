// serverPage.cpp : implementation file
//

#include "stdafx.h"
#include "cvsnt.h"
#include "serverPage.h"
#include "NewRootDialog.h"
#include ".\serverpage.h"

#define ServiceName _T("CVS")
#define ServiceName2 _T("CVSLOCK")

/////////////////////////////////////////////////////////////////////////////
// CserverPage property page

IMPLEMENT_DYNCREATE(CserverPage, CTooltipPropertyPage)

CserverPage::CserverPage() : CTooltipPropertyPage(CserverPage::IDD)
//, m_szSshStatus(_T(""))
{
	m_szVersion = "CVSNT " CVSNT_PRODUCTVERSION_STRING;
	//{{AFX_DATA_INIT(CserverPage)
	m_szStatus = _T("");
	m_szLockStatus =_T("");
	 //}}AFX_DATA_INIT
	m_hService=m_hLockService=m_hSCManager=NULL;
}

CserverPage::~CserverPage()
{
}

void CserverPage::DoDataExchange(CDataExchange* pDX)
{
	CTooltipPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CserverPage)
	DDX_Control(pDX, IDC_START, m_btStart);
	DDX_Control(pDX, IDC_STOP, m_btStop);
	DDX_Text(pDX, IDC_VERSION, m_szVersion);
	DDX_Text(pDX, IDC_STATUS, m_szStatus);
	DDX_Text(pDX, IDC_STATUS2, m_szLockStatus);
	DDX_Control(pDX, IDC_START2, m_btLockStart);
	DDX_Control(pDX, IDC_STOP2, m_btLockStop);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CserverPage, CTooltipPropertyPage)
	//{{AFX_MSG_MAP(CserverPage)
	ON_WM_TIMER()
	ON_BN_CLICKED(IDC_START, OnStart)
	ON_BN_CLICKED(IDC_STOP, OnStop)
	ON_BN_CLICKED(IDC_START2, OnBnClickedStart2)
	ON_BN_CLICKED(IDC_STOP2, OnBnClickedStop2)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CserverPage message handlers

BOOL CserverPage::OnInitDialog() 
{
	CTooltipPropertyPage::OnInitDialog();
	CString tmp;

	if(m_hSCManager)
	{
		if(m_hService)
			CloseServiceHandle(m_hService);
		CloseServiceHandle(m_hSCManager);
	}
		
	m_hSCManager=OpenSCManager(NULL,NULL,g_bPrivileged?GENERIC_EXECUTE:GENERIC_READ);
	if(!m_hSCManager)
	{
		CString tmp;
		DWORD e=GetLastError();

		if(e==5)
		{
			tmp.Format(_T("Couldn't open service control manager - Permission Denied"));
			AfxMessageBox(tmp,MB_ICONSTOP);
			GetParent()->PostMessage(WM_CLOSE);
		}
		else
		{
			tmp.Format(_T("Couldn't open service control manager - error %d"),GetLastError());
			AfxMessageBox(tmp,MB_ICONSTOP);
			GetParent()->PostMessage(WM_CLOSE);
		}
	}
	
	if(g_bPrivileged)
	{
		m_hService=OpenService(m_hSCManager,ServiceName,SERVICE_QUERY_STATUS|SERVICE_START|SERVICE_STOP);
		m_hLockService=OpenService(m_hSCManager,ServiceName2,SERVICE_QUERY_STATUS|SERVICE_START|SERVICE_STOP);
	}
	else
	{
		m_hService=OpenService(m_hSCManager,ServiceName,SERVICE_QUERY_STATUS);
		m_hLockService=OpenService(m_hSCManager,ServiceName2,SERVICE_QUERY_STATUS);
	}

	UpdateStatus();
	SetTimer(0,1000,NULL);

	return TRUE; 
}

void CserverPage::UpdateStatus()
{
	SERVICE_STATUS stat = {0};

	if(!m_hService)
	{
#ifdef JP_STRING
		m_szStatus="サービスがインストールされていません";
#else
		m_szStatus="Service not installed";
#endif
		m_btStart.EnableWindow(FALSE);
		m_btStop.EnableWindow(FALSE);
	}
	else
	{
		QueryServiceStatus(m_hService,&stat);
		switch(stat.dwCurrentState)
		{
			case SERVICE_STOPPED:
#ifdef JP_STRING
				m_szStatus="停止";
#else
				m_szStatus="Stopped";
#endif
				m_btStart.EnableWindow(g_bPrivileged?TRUE:FALSE);
				m_btStop.EnableWindow(FALSE);
				break;
			case SERVICE_START_PENDING:
#ifdef JP_STRING
				m_szStatus="開始中";
#else
				m_szStatus="Starting";
#endif
				m_btStart.EnableWindow(FALSE);
				m_btStop.EnableWindow(FALSE);
				break;
			case SERVICE_STOP_PENDING:
#ifdef JP_STRING
				m_szStatus="停止中";
#else
				m_szStatus="Stopping";
#endif
				m_btStart.EnableWindow(FALSE);
				m_btStop.EnableWindow(FALSE);
				break;
			case SERVICE_RUNNING:
#ifdef JP_STRING
				m_szStatus="実行中";
#else
				m_szStatus="Running";
#endif
				m_btStart.EnableWindow(FALSE);
				m_btStop.EnableWindow(g_bPrivileged?TRUE:FALSE);
				break;
			default:
				m_szStatus="Unknown state";
				m_btStart.EnableWindow(FALSE);
				m_btStop.EnableWindow(FALSE);
				break;
		}
	}

	if(!m_hLockService)
	{
#ifdef JP_STRING
		m_szLockStatus="サービスがインストールされていません";
#else
		m_szLockStatus="Service not installed";
#endif
		m_btLockStart.EnableWindow(FALSE);
		m_btLockStop.EnableWindow(FALSE);
	}
	else
	{
		QueryServiceStatus(m_hLockService,&stat);
		switch(stat.dwCurrentState)
		{
			case SERVICE_STOPPED:
#ifdef JP_STRING
				m_szLockStatus="停止";
#else
				m_szLockStatus="Stopped";
#endif

				m_btLockStart.EnableWindow(g_bPrivileged?TRUE:FALSE);
				m_btLockStop.EnableWindow(FALSE);
				break;
			case SERVICE_START_PENDING:
#ifdef JP_STRING
				m_szLockStatus="開始中";
#else
				m_szLockStatus="Starting";
#endif
				m_btLockStart.EnableWindow(FALSE);
				m_btLockStop.EnableWindow(FALSE);
				break;
			case SERVICE_STOP_PENDING:
#ifdef JP_STRING
				m_szLockStatus="停止中";
#else
				m_szLockStatus="Stopping";
#endif
				m_btLockStart.EnableWindow(FALSE);
				m_btLockStop.EnableWindow(FALSE);
				break;
			case SERVICE_RUNNING:
#ifdef JP_STRING
				m_szLockStatus="実行中";
#else
				m_szLockStatus="Running";
#endif
				m_btLockStart.EnableWindow(FALSE);
				m_btLockStop.EnableWindow(g_bPrivileged?TRUE:FALSE);
				break;
			default:
				m_szLockStatus="Unknown state";
				m_btLockStart.EnableWindow(FALSE);
				m_btLockStop.EnableWindow(FALSE);
				break;
		}
	}

	UpdateData(FALSE);
}

void CserverPage::OnTimer(UINT nIDEvent) 
{
	UpdateStatus();
}

void CserverPage::OnStart() 
{
	m_btStart.EnableWindow(FALSE);
	if(!StartService(m_hService,0,NULL))
	{
		CString tmp;
#ifdef JP_STRING
		tmp.Format(_T("サービスが開始できません: %s"),GetErrorString());
#else
		tmp.Format(_T("Couldn't start service: %s"),GetErrorString());
#endif
		AfxMessageBox(tmp,MB_ICONSTOP);
	}
	UpdateStatus();
}

void CserverPage::OnStop() 
{
	SERVICE_STATUS stat = {0};

	m_btStop.EnableWindow(FALSE);
	if(!ControlService(m_hService,SERVICE_CONTROL_STOP,&stat))
	{
		CString tmp;
#ifdef JP_STRING
		tmp.Format(_T("サービスが停止できません: %s"),GetErrorString());
#else
		tmp.Format(_T("Couldn't stop service: %s"),GetErrorString());
#endif
		AfxMessageBox(tmp,MB_ICONSTOP);
	}
	UpdateStatus();
}

LPCTSTR CserverPage::GetErrorString()
{
	static TCHAR ErrBuf[1024];

	FormatMessage(
    FORMAT_MESSAGE_FROM_SYSTEM |
	FORMAT_MESSAGE_IGNORE_INSERTS,
    NULL,
    GetLastError(),
    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
    (LPTSTR) ErrBuf,
    sizeof(ErrBuf),
    NULL );
	return ErrBuf;
};

void CserverPage::OnBnClickedStart2()
{
	m_btLockStart.EnableWindow(FALSE);
	if(!StartService(m_hLockService,0,NULL))
	{
		CString tmp;
#ifdef JP_STRING
		tmp.Format(_T("サービスが開始できません: %s"),GetErrorString());
#else
		tmp.Format(_T("Couldn't start service: %s"),GetErrorString());
#endif
		AfxMessageBox(tmp,MB_ICONSTOP);
	}
	UpdateStatus();
}

void CserverPage::OnBnClickedStop2()
{
	SERVICE_STATUS stat = {0};

	m_btLockStop.EnableWindow(FALSE);
	if(!ControlService(m_hLockService,SERVICE_CONTROL_STOP,&stat))
	{
		CString tmp;
#ifdef JP_STRING
		tmp.Format(_T("サービスが停止できません: %s"),GetErrorString());
#else
		tmp.Format(_T("Couldn't stop service: %s"),GetErrorString());
#endif
		AfxMessageBox(tmp,MB_ICONSTOP);
	}
	UpdateStatus();
}
