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
	m_hServerKey=NULL;
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
		
	m_hSCManager=OpenSCManager(NULL,NULL,GENERIC_EXECUTE);
	if(!m_hSCManager)
	{
		CString tmp;
		DWORD e=GetLastError();

		if(e==5)
			tmp.Format(_T("Couldn't open service control manager - Access Denied"));
		else
			tmp.Format(_T("Couldn't open service control manager - error %d"),GetLastError());
		AfxMessageBox(tmp,MB_ICONSTOP);
		PostMessage(WM_CLOSE);
	}
	
	m_hService=OpenService(m_hSCManager,ServiceName,SERVICE_QUERY_STATUS|SERVICE_START|SERVICE_STOP);
	m_hLockService=OpenService(m_hSCManager,ServiceName2,SERVICE_QUERY_STATUS|SERVICE_START|SERVICE_STOP);

	if(!m_hServerKey && RegCreateKeyEx(HKEY_LOCAL_MACHINE,_T("Software\\CVS\\Pserver"),NULL,_T(""),REG_OPTION_NON_VOLATILE,KEY_ALL_ACCESS,NULL,&m_hServerKey,NULL))
	{ 
		fprintf(stderr,"Couldn't create HKLM\\Software\\CVS\\Pserver key, error %d\n",GetLastError());
		return -1;
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
		m_szStatus="Service not installed";
		m_btStart.EnableWindow(FALSE);
		m_btStop.EnableWindow(FALSE);
	}
	else
	{
		QueryServiceStatus(m_hService,&stat);
		switch(stat.dwCurrentState)
		{
			case SERVICE_STOPPED:
				m_szStatus="Stopped";
				m_btStart.EnableWindow(TRUE);
				m_btStop.EnableWindow(FALSE);
				break;
			case SERVICE_START_PENDING:
				m_szStatus="Starting";
				m_btStart.EnableWindow(FALSE);
				m_btStop.EnableWindow(FALSE);
				break;
			case SERVICE_STOP_PENDING:
				m_szStatus="Stopping";
				m_btStart.EnableWindow(FALSE);
				m_btStop.EnableWindow(FALSE);
				break;
			case SERVICE_RUNNING:
				m_szStatus="Running";
				m_btStart.EnableWindow(FALSE);
				m_btStop.EnableWindow(TRUE);
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
		m_szLockStatus="Service not installed";
		m_btLockStart.EnableWindow(FALSE);
		m_btLockStop.EnableWindow(FALSE);
	}
	else
	{
		QueryServiceStatus(m_hLockService,&stat);
		switch(stat.dwCurrentState)
		{
			case SERVICE_STOPPED:
				m_szLockStatus="Stopped";
				m_btLockStart.EnableWindow(TRUE);
				m_btLockStop.EnableWindow(FALSE);
				break;
			case SERVICE_START_PENDING:
				m_szLockStatus="Starting";
				m_btLockStart.EnableWindow(FALSE);
				m_btLockStop.EnableWindow(FALSE);
				break;
			case SERVICE_STOP_PENDING:
				m_szLockStatus="Stopping";
				m_btLockStart.EnableWindow(FALSE);
				m_btLockStop.EnableWindow(FALSE);
				break;
			case SERVICE_RUNNING:
				m_szLockStatus="Running";
				m_btLockStart.EnableWindow(FALSE);
				m_btLockStop.EnableWindow(TRUE);
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
		tmp.Format(_T("Couldn't start service: %s"),GetErrorString());
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
		tmp.Format(_T("Couldn't stop service: %s"),GetErrorString());
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
		tmp.Format(_T("Couldn't start service: %s"),GetErrorString());
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
		tmp.Format(_T("Couldn't stop service: %s"),GetErrorString());
		AfxMessageBox(tmp,MB_ICONSTOP);
	}
	UpdateStatus();
}
