#include "stdafx.h"
#include "TooltipPropertyPage.h"

IMPLEMENT_DYNAMIC(CTooltipPropertyPage, CPropertyPage)

BEGIN_MESSAGE_MAP(CTooltipPropertyPage, CPropertyPage)
	ON_WM_DESTROY()
END_MESSAGE_MAP()

CTooltipPropertyPage::CTooltipPropertyPage()
{
}

CTooltipPropertyPage::CTooltipPropertyPage(int nID) : CPropertyPage(nID)
{
}

CTooltipPropertyPage::~CTooltipPropertyPage()
{
}


BOOL CTooltipPropertyPage::OnInitDialog()
{
	BOOL bResult = CPropertyPage::OnInitDialog();
	m_wndToolTip.Create(this);
	m_wndToolTip.SetMaxTipWidth(800);
	m_wndToolTip.Activate(TRUE);
	CWnd *pWndChild = GetWindow(GW_CHILD);
	CString strToolTip;
	while (pWndChild)
	{
		int nID = pWndChild->GetDlgCtrlID();
		if (strToolTip.LoadString(nID))
		{
			m_wndToolTip.AddTool(pWndChild, strToolTip);
		}
		pWndChild = pWndChild->GetWindow(GW_HWNDNEXT);
	}
	return bResult;
}

BOOL CTooltipPropertyPage::PreTranslateMessage(MSG *pMsg)
{
	if (pMsg->message >= WM_MOUSEFIRST &&
		pMsg->message <= WM_MOUSELAST)
	{
		MSG msg;
		::CopyMemory(&msg, pMsg, sizeof(MSG));
		HWND hWndParent = ::GetParent(msg.hwnd);
		while (hWndParent && hWndParent != m_hWnd)
		{
			msg.hwnd = hWndParent;
			hWndParent = ::GetParent(hWndParent);
		}
		if (msg.hwnd)
		{
			m_wndToolTip.RelayEvent(&msg);
		}
	}
	return CPropertyPage::PreTranslateMessage(pMsg);
}

void CTooltipPropertyPage::OnDestroy()
{
	m_wndToolTip.DestroyWindow();
}
