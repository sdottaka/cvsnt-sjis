// cvsagent.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "cvsagent.h"
#include "aboutdialog.h"
#include "listenserver.h"
#include "PasswordDialog.h"
#include "..\..\protocols\scramble.h"

std::map<std::string,std::string> g_Passwords;
bool g_bTopmost = true;

class CAgentWnd : public CWnd
{
	void ShowContextMenu();

	afx_msg LRESULT OnTrayMessage(WPARAM wParam,LPARAM lParam);
	afx_msg LRESULT OnPasswordMessage(WPARAM wParam,LPARAM lParam);
	afx_msg void OnAbout();
	afx_msg void OnQuit();
	afx_msg void OnClearPasswords();
	afx_msg void OnAlwaysOnTop();

	DECLARE_MESSAGE_MAP();
};

BEGIN_MESSAGE_MAP(CAgentWnd,CWnd)
ON_MESSAGE(TRAY_MESSAGE,OnTrayMessage)
ON_MESSAGE(PWD_MESSAGE,OnPasswordMessage)
ON_COMMAND(ID_ABOUT,OnAbout)
ON_COMMAND(ID_QUIT,OnQuit)
ON_COMMAND(ID_CLEARPASSWORDS,OnClearPasswords)
ON_COMMAND(ID_ALWAYSONTOP,OnAlwaysOnTop)
END_MESSAGE_MAP()

class CAgentApp : public CWinApp
{
	virtual BOOL InitInstance();
	virtual BOOL ExitInstance();

	static DWORD WINAPI _ThreadProc(LPVOID lpParam);
	DWORD ThreadProc();
};

CAgentApp app;

BOOL CAgentApp::InitInstance()
{
	m_pMainWnd = new CAgentWnd;
	m_pMainWnd->CreateEx(0,_T("static"),NULL,WS_OVERLAPPED,CRect(1,1,1,1),NULL,0);

	NOTIFYICONDATA nid = { sizeof(NOTIFYICONDATA) };
	nid.uFlags=NIF_ICON|NIF_MESSAGE|NIF_TIP;
	nid.uCallbackMessage=TRAY_MESSAGE;
	nid.hIcon=LoadIcon(IDI_CVSAGENT);
	nid.uID=IDI_CVSAGENT;
	_tcscpy(nid.szTip,_T("CVSNT password agent"));
	nid.hWnd=m_pMainWnd->GetSafeHwnd();
	Shell_NotifyIcon(NIM_ADD,&nid);

	CloseHandle(::CreateThread(0,0,_ThreadProc,this,0,NULL));
	return TRUE;
}

DWORD WINAPI CAgentApp::_ThreadProc(LPVOID lpParam)
{
	return ((CAgentApp*)lpParam)->ThreadProc();
}

DWORD CAgentApp::ThreadProc()
{
	CListenServer l;
	l.Listen("32401");
	return 0;
}

BOOL CAgentApp::ExitInstance()
{
	NOTIFYICONDATA nid = { sizeof(NOTIFYICONDATA) };
	nid.hWnd=m_pMainWnd->GetSafeHwnd();
	nid.uID=IDI_CVSAGENT;
	Shell_NotifyIcon(NIM_DELETE,&nid);
	return TRUE;
}

LRESULT CAgentWnd::OnTrayMessage(WPARAM wParam, LPARAM lParam)
{
	switch(lParam)
	{
	case WM_LBUTTONDBLCLK:
		break;
	case WM_RBUTTONDOWN:
	case WM_CONTEXTMENU:
		ShowContextMenu();
		break;
	}
	
	return 0;
}

void CAgentWnd::ShowContextMenu()
{
	CMenu menu;
	CPoint pt;
	menu.LoadMenu(IDR_MENU1);
	SetForegroundWindow();
	GetCursorPos(&pt);
	CMenu *sub = menu.GetSubMenu(0);
	sub->CheckMenuItem(ID_ALWAYSONTOP,g_bTopmost?MF_CHECKED:MF_UNCHECKED);
	sub->TrackPopupMenu(TPM_BOTTOMALIGN|TPM_CENTERALIGN|TPM_LEFTBUTTON,pt.x,pt.y,this);
}

void CAgentWnd::OnAbout()
{
	CAboutDialog dlg;
	dlg.DoModal();
}

void CAgentWnd::OnQuit()
{
	PostQuitMessage(0);
}

void CAgentWnd::OnClearPasswords()
{
	g_Passwords.clear();
}

void CAgentWnd::OnAlwaysOnTop()
{
	g_bTopmost=!g_bTopmost;
}

LRESULT CAgentWnd::OnPasswordMessage(WPARAM wParam,LPARAM lParam)
{
	char *buffer = (char*)wParam;
	int *len = (int*)lParam;

	CPasswordDialog dlg;
	dlg.m_szCvsRoot=buffer;
	if(dlg.DoModal()!=IDOK)
		return 0;
	pserver_crypt_password(dlg.m_szPassword.c_str(),buffer,BUFSIZ);
	*len=(int)strlen(buffer);

	g_Passwords[dlg.m_szCvsRoot]=buffer;
	return 1;
}
