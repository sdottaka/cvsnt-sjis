#include "stdafx.h"
#include "cvsnt1.h"

CcvsntCPL* g_cpl=NULL;

extern "C" LONG APIENTRY EXPORT CPlApplet(HWND hwndCPl, UINT uMsg, LONG lParam1, LONG lParam2)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());  // *MUST* be first line of function

	BOOL bRet;

	switch(uMsg)
	{
		case CPL_DBLCLK:  // Sent to notify CPlApplet that the user has chosen the icon associated with a given dialog box. CPlApplet should display the corresponding dialog box and carry out any user-specified tasks.  
			return !g_cpl->DoubleClick((UINT)lParam1,(LONG)lParam2); // uAppNum, lData
		case CPL_EXIT:  // Sent after the last CPL_STOP message and immediately before the controlling application uses the FreeLibrary function to free the DLL containing the Control Panel application. CPlApplet should free any remaining memory and prepare to close.  
			bRet=!g_cpl->Exit();
			delete g_cpl;
			g_cpl=NULL;
			return bRet;
		case CPL_GETCOUNT:  // Sent after the CPL_INIT message to prompt CPlApplet to return a number that indicates how many dialog boxes it supports.  
			return g_cpl->GetCount();
		case CPL_INIT:  // Sent immediately after the DLL containing the Control Panel application is loaded. The message prompts CPlApplet to perform initialization procedures, including memory allocation.  
			g_cpl=new CcvsntCPL;
			return g_cpl->Init();
		case CPL_INQUIRE:  // Sent after the CPL_GETCOUNT message to prompt CPlApplet to provide information about a specified dialog box. The lParam2 parameter of CPlApplet points to a CPLINFO structure.  
			return !g_cpl->Inquire((UINT)lParam1, (LPCPLINFO)lParam2);
		case CPL_STOP:	// Sent once for each dialog box before the controlling application closes. CPlApplet should free any memory associated with the given dialog box. 
			return !g_cpl->Stop((UINT)lParam1,(LONG)lParam2); // uAppNum, lData
	}
	return 0;
}

