#include "stdafx.h"
#include "winuser_utf8.h"

int WINAPI wvsprintfU(
 OUT LPSTR szTarget,
 IN LPCSTR szFmt,
 IN va_list arglist)
{
return wvsprintfW(UTF8_MAP(szTarget),UTF8_MAP(szFmt,arglist);
}

int WINAPIV wsprintfU(
 OUT LPSTR,
 IN LPCSTR,
 ...)
{
return wsprintfW(LPSTR,LPCSTR, ...);
}

HKL WINAPI LoadKeyboardLayoutU(
 IN LPCSTR pwszKLID,
 IN UINT Flags)
{
return LoadKeyboardLayoutW( UTF8_C_MAP(pwszKLID), UINT Flags);
}

BOOL WINAPI GetKeyboardLayoutNameU(
 OUT LPSTR pwszKLID)
{
return GetKeyboardLayoutNameW( UTF8_MAP(pwszKLID));
}

HDESK WINAPI CreateDesktopU(
 IN LPCSTR lpszDesktop,
 IN LPCSTR lpszDevice,
 IN LPDEVMODEA pDevmode,
 IN DWORD dwFlags,
 IN ACCESS_MASK dwDesiredAccess,
 IN LPSECURITY_ATTRIBUTES lpsa)
{
return CreateDesktopW( UTF8_C_MAP(lpszDesktop), UTF8_C_MAP(lpszDevice),pDevmode,dwFlags,dwDesiredAccess, LPSECURITY_ATTRIBUTES lpsa);
}

HDESK WINAPI OpenDesktopU(
 IN LPCSTR lpszDesktop,
 IN DWORD dwFlags,
 IN BOOL fInherit,
 IN ACCESS_MASK dwDesiredAccess)
{
return OpenDesktopW( UTF8_C_MAP(lpszDesktop),dwFlags,fInherit, ACCESS_MASK dwDesiredAccess);
}

BOOL WINAPI EnumDesktopsU(
 IN HWINSTA hwinsta,
 IN DESKTOPENUMPROCA lpEnumFunc,
 IN LPARAM lParam)
{
return EnumDesktopsW(hwinsta,lpEnumFunc, LPARAM lParam);
}

HWINSTA WINAPI CreateWindowStationU(
 IN LPCSTR lpwinsta,
 IN DWORD dwReserved,
 IN ACCESS_MASK dwDesiredAccess,
 IN LPSECURITY_ATTRIBUTES lpsa)
{
return CreateWindowStationW( UTF8_C_MAP(lpwinsta),dwReserved,dwDesiredAccess, LPSECURITY_ATTRIBUTES lpsa);
}

HWINSTA WINAPI OpenWindowStationU(
 IN LPCSTR lpszWinSta,
 IN BOOL fInherit,
 IN ACCESS_MASK dwDesiredAccess)
{
return OpenWindowStationW( UTF8_C_MAP(lpszWinSta),fInherit, ACCESS_MASK dwDesiredAccess);
}

BOOL WINAPI EnumWindowStationsU(
 IN WINSTAENUMPROCA lpEnumFunc,
 IN LPARAM lParam)
{
return EnumWindowStationsW(lpEnumFunc, LPARAM lParam);
}

BOOL WINAPI GetUserObjectInformationU(
 IN HANDLE hObj,
 IN int nIndex,
 OUT PVOID pvInfo,
 IN DWORD nLength,
 OUT LPDWORD lpnLengthNeeded)
{
return GetUserObjectInformationW(hObj,nIndex,pvInfo,nLength, LPDWORD lpnLengthNeeded);
}

BOOL WINAPI SetUserObjectInformationU(
 IN HANDLE hObj,
 IN int nIndex,
 IN PVOID pvInfo,
 IN DWORD nLength)
{
return SetUserObjectInformationW(hObj,nIndex,pvInfo, DWORD nLength);
}

UINT WINAPI RegisterWindowMessageU(
 IN LPCSTR lpString)
{
return RegisterWindowMessageW( UTF8_C_MAP(lpString));
}

BOOL WINAPI GetMessageU(
 OUT LPMSG lpMsg,
 IN HWND hWnd,
 IN UINT wMsgFilterMin,
 IN UINT wMsgFilterMax)
{
return GetMessageW(lpMsg,hWnd,wMsgFilterMin, UINT wMsgFilterMax);
}

LRESULT WINAPI DispatchMessageU(
 IN CONST MSG *lpMsg)
{
return DispatchMessageW( CONST MSG lpMsg);
}

BOOL WINAPI PeekMessageU(
 OUT LPMSG lpMsg,
 IN HWND hWnd,
 IN UINT wMsgFilterMin,
 IN UINT wMsgFilterMax,
 IN UINT wRemoveMsg)
{
return PeekMessageW(lpMsg,hWnd,wMsgFilterMin,wMsgFilterMax, UINT wRemoveMsg);
}

LRESULT WINAPI SendMessageU(
 IN HWND hWnd,
 IN UINT Msg,
 IN WPARAM wParam,
 IN LPARAM lParam)
{
return SendMessageW(hWnd,Msg,wParam, LPARAM lParam);
}

LRESULT WINAPI SendMessageTimeoutU(
 IN HWND hWnd,
 IN UINT Msg,
 IN WPARAM wParam,
 IN LPARAM lParam,
 IN UINT fuFlags,
 IN UINT uTimeout,
 OUT PDWORD_PTR lpdwResult)
{
return SendMessageTimeoutW(hWnd,Msg,wParam,lParam,fuFlags,uTimeout, PDWORD_PTR lpdwResult);
}

BOOL WINAPI SendNotifyMessageU(
 IN HWND hWnd,
 IN UINT Msg,
 IN WPARAM wParam,
 IN LPARAM lParam)
{
return SendNotifyMessageW(hWnd,Msg,wParam, LPARAM lParam);
}

BOOL WINAPI SendMessageCallbackU(
 IN HWND hWnd,
 IN UINT Msg,
 IN WPARAM wParam,
 IN LPARAM lParam,
 IN SENDASYNCPROC lpResultCallBack,
 IN ULONG_PTR dwData)
{
return SendMessageCallbackW(hWnd,Msg,wParam,lParam,lpResultCallBack, ULONG_PTR dwData);
}

long WINAPI BroadcastSystemMessageExU(
 IN DWORD,
 IN LPDWORD,
 IN UINT,
 IN WPARAM,
 IN LPARAM,
 OUT PBSMINFO)
{
return BroadcastSystemMessageExW(DWORD,LPDWORD,UINT,WPARAM,LPARAM, PBSMINFO);
}

long WINAPI BroadcastSystemMessageU(
 IN DWORD,
 IN LPDWORD,
 IN UINT,
 IN WPARAM,
 IN LPARAM)
{
return BroadcastSystemMessageW(DWORD,LPDWORD,UINT,WPARAM, LPARAM);
}

HDEVNOTIFY WINAPI RegisterDeviceNotificationU(
 IN HANDLE hRecipient,
 IN LPVOID NotificationFilter,
 IN DWORD Flags
 )
{
return RegisterDeviceNotificationW(hRecipient,NotificationFilter,Flags );
}

BOOL WINAPI PostMessageU(
 IN HWND hWnd,
 IN UINT Msg,
 IN WPARAM wParam,
 IN LPARAM lParam)
{
return PostMessageW(hWnd,Msg,wParam, LPARAM lParam);
}

BOOL WINAPI PostThreadMessageU(
 IN DWORD idThread,
 IN UINT Msg,
 IN WPARAM wParam,
 IN LPARAM lParam)
{
return PostThreadMessageW(idThread,Msg,wParam, LPARAM lParam);
}

LRESULT CALLBACK DefWindowProcU(
 IN HWND hWnd,
 IN UINT Msg,
 IN WPARAM wParam,
 IN LPARAM lParam)
{
return DefWindowProcW(hWnd,Msg,wParam, LPARAM lParam);
}

LRESULT WINAPI CallWindowProcU(
 IN WNDPROC lpPrevWndFunc,
 IN HWND hWnd,
 IN UINT Msg,
 IN WPARAM wParam,
 IN LPARAM lParam)
{
return CallWindowProcW(lpPrevWndFunc,hWnd,Msg,wParam, LPARAM lParam);
}

LRESULT WINAPI CallWindowProcU(
 IN FARPROC lpPrevWndFunc,
 IN HWND hWnd,
 IN UINT Msg,
 IN WPARAM wParam,
 IN LPARAM lParam)
{
return CallWindowProcW(lpPrevWndFunc,hWnd,Msg,wParam, LPARAM lParam);
}

ATOM WINAPI RegisterClassU(
 IN CONST WNDCLASSA *lpWndClass)
{
return RegisterClassW( CONST WNDCLASSA lpWndClass);
}

BOOL WINAPI UnregisterClassU(
 IN LPCSTR lpClassName,
 IN HINSTANCE hInstance)
{
return UnregisterClassW( UTF8_C_MAP(lpClassName), HINSTANCE hInstance);
}

BOOL WINAPI GetClassInfoU(
 IN HINSTANCE hInstance,
 IN LPCSTR lpClassName,
 OUT LPWNDCLASSA lpWndClass)
{
return GetClassInfoW(hInstance, UTF8_C_MAP(lpClassName), LPWNDCLASSA lpWndClass);
}

ATOM WINAPI RegisterClassExU(
 IN CONST WNDCLASSEXA *)
{
return RegisterClassExW( CONST WNDCLASSEXA );
}

BOOL WINAPI GetClassInfoExU(
 IN HINSTANCE,
 IN LPCSTR,
 OUT LPWNDCLASSEXA)
{
return GetClassInfoExW(HINSTANCE,LPCSTR, LPWNDCLASSEXA);
}

HWND WINAPI CreateWindowExU(
 IN DWORD dwExStyle,
 IN LPCSTR lpClassName,
 IN LPCSTR lpWindowName,
 IN DWORD dwStyle,
 IN int X,
 IN int Y,
 IN int nWidth,
 IN int nHeight,
 IN HWND hWndParent,
 IN HMENU hMenu,
 IN HINSTANCE hInstance,
 IN LPVOID lpParam)
{
return CreateWindowExW(dwExStyle, UTF8_C_MAP(lpClassName), UTF8_C_MAP(lpWindowName),dwStyle,X,Y,nWidth,nHeight,hWndParent,hMenu,hInstance, LPVOID lpParam);
}

HWND WINAPI CreateDialogParamU(
 IN HINSTANCE hInstance,
 IN LPCSTR lpTemplateName,
 IN HWND hWndParent,
 IN DLGPROC lpDialogFunc,
 IN LPARAM dwInitParam)
{
return CreateDialogParamW(hInstance, UTF8_C_MAP(lpTemplateName),hWndParent,lpDialogFunc, LPARAM dwInitParam);
}

HWND WINAPI CreateDialogIndirectParamU(
 IN HINSTANCE hInstance,
 IN LPCDLGTEMPLATEA lpTemplate,
 IN HWND hWndParent,
 IN DLGPROC lpDialogFunc,
 IN LPARAM dwInitParam)
{
return CreateDialogIndirectParamW(hInstance,lpTemplate,hWndParent,lpDialogFunc, LPARAM dwInitParam);
}

INT_PTR WINAPI DialogBoxParamU(
 IN HINSTANCE hInstance,
 IN LPCSTR lpTemplateName,
 IN HWND hWndParent,
 IN DLGPROC lpDialogFunc,
 IN LPARAM dwInitParam)
{
return DialogBoxParamW(hInstance, UTF8_C_MAP(lpTemplateName),hWndParent,lpDialogFunc, LPARAM dwInitParam);
}

INT_PTR WINAPI DialogBoxIndirectParamU(
 IN HINSTANCE hInstance,
 IN LPCDLGTEMPLATEA hDialogTemplate,
 IN HWND hWndParent,
 IN DLGPROC lpDialogFunc,
 IN LPARAM dwInitParam)
{
return DialogBoxIndirectParamW(hInstance,hDialogTemplate,hWndParent,lpDialogFunc, LPARAM dwInitParam);
}

BOOL WINAPI SetDlgItemTextU(
 IN HWND hDlg,
 IN int nIDDlgItem,
 IN LPCSTR lpString)
{
return SetDlgItemTextW(hDlg,nIDDlgItem, UTF8_C_MAP(lpString));
}

UINT WINAPI GetDlgItemTextU(
 IN HWND hDlg,
 IN int nIDDlgItem,
 OUT LPSTR lpString,
 IN int nMaxCount)
{
return GetDlgItemTextW(hDlg,nIDDlgItem, UTF8_MAP(lpString), int nMaxCount);
}

LRESULT WINAPI SendDlgItemMessageU(
 IN HWND hDlg,
 IN int nIDDlgItem,
 IN UINT Msg,
 IN WPARAM wParam,
 IN LPARAM lParam)
{
return SendDlgItemMessageW(hDlg,nIDDlgItem,Msg,wParam, LPARAM lParam);
}

LRESULT CALLBACK DefDlgProcU(
 IN HWND hDlg,
 IN UINT Msg,
 IN WPARAM wParam,
 IN LPARAM lParam)
{
return DefDlgProcW(hDlg,Msg,wParam, LPARAM lParam);
}

BOOL WINAPI CallMsgFilterU(
 IN LPMSG lpMsg,
 IN int nCode)
{
return CallMsgFilterW(lpMsg, int nCode);
}

UINT WINAPI RegisterClipboardFormatU(
 IN LPCSTR lpszFormat)
{
return RegisterClipboardFormatW( UTF8_C_MAP(lpszFormat));
}

int WINAPI GetClipboardFormatNameU(
 IN UINT format,
 OUT LPSTR lpszFormatName,
 IN int cchMaxCount)
{
return GetClipboardFormatNameW(format, UTF8_MAP(lpszFormatName), int cchMaxCount);
}

BOOL WINAPI CharToOemU(
 IN LPCSTR lpszSrc,
 OUT LPSTR lpszDst)
{
return CharToOemW( UTF8_C_MAP(lpszSrc), UTF8_MAP(lpszDst));
}

BOOL WINAPI OemToCharU(
 IN LPCSTR lpszSrc,
 OUT LPSTR lpszDst)
{
return OemToCharW( UTF8_C_MAP(lpszSrc), UTF8_MAP(lpszDst));
}

BOOL WINAPI CharToOemBuffU(
 IN LPCSTR lpszSrc,
 OUT LPSTR lpszDst,
 IN DWORD cchDstLength)
{
return CharToOemBuffW( UTF8_C_MAP(lpszSrc), UTF8_MAP(lpszDst), DWORD cchDstLength);
}

BOOL WINAPI OemToCharBuffU(
 IN LPCSTR lpszSrc,
 OUT LPSTR lpszDst,
 IN DWORD cchDstLength)
{
return OemToCharBuffW( UTF8_C_MAP(lpszSrc), UTF8_MAP(lpszDst), DWORD cchDstLength);
}

LPSTR WINAPI CharUpperU(
 IN OUT LPSTR lpsz)
{
return CharUpperW( UTF8_MAP(lpsz));
}

DWORD WINAPI CharUpperBuffU(
 IN OUT LPSTR lpsz,
 IN DWORD cchLength)
{
return CharUpperBuffW( UTF8_MAP(lpsz), DWORD cchLength);
}

LPSTR WINAPI CharLowerU(
 IN OUT LPSTR lpsz)
{
return CharLowerW( UTF8_MAP(lpsz));
}

DWORD WINAPI CharLowerBuffU(
 IN OUT LPSTR lpsz,
 IN DWORD cchLength)
{
return CharLowerBuffW( UTF8_MAP(lpsz), DWORD cchLength);
}

LPSTR WINAPI CharNextU(
 IN LPCSTR lpsz)
{
return CharNextW( UTF8_C_MAP(lpsz));
}

LPSTR WINAPI CharPrevU(
 IN LPCSTR lpszStart,
 IN LPCSTR lpszCurrent)
{
return CharPrevW( UTF8_C_MAP(lpszStart), UTF8_C_MAP(lpszCurrent));
}

LPSTR WINAPI CharNextExU(
 IN WORD CodePage,
 IN LPCSTR lpCurrentChar,
 IN DWORD dwFlags)
{
return CharNextExW(CodePage, UTF8_C_MAP(lpCurrentChar), DWORD dwFlags);
}

LPSTR WINAPI CharPrevExU(
 IN WORD CodePage,
 IN LPCSTR lpStart,
 IN LPCSTR lpCurrentChar,
 IN DWORD dwFlags)
{
return CharPrevExW(CodePage, UTF8_C_MAP(lpStart), UTF8_C_MAP(lpCurrentChar), DWORD dwFlags);
}

BOOL WINAPI IsCharAlphaU(
 IN CHAR ch)
{
return IsCharAlphaW( CHAR ch);
}

BOOL WINAPI IsCharAlphaNumericU(
 IN CHAR ch)
{
return IsCharAlphaNumericW( CHAR ch);
}

BOOL WINAPI IsCharUpperU(
 IN CHAR ch)
{
return IsCharUpperW( CHAR ch);
}

BOOL WINAPI IsCharLowerU(
 IN CHAR ch)
{
return IsCharLowerW( CHAR ch);
}

int WINAPI GetKeyNameTextU(
 IN LONG lParam,
 OUT LPSTR lpString,
 IN int nSize
 )
{
return GetKeyNameTextW(lParam, UTF8_MAP(lpString),nSize );
}

SHORT WINAPI VkKeyScanU(
 IN CHAR ch)
{
return VkKeyScanW( CHAR ch);
}

SHORT WINAPI VkKeyScanExU(
 IN CHAR ch,
 IN HKL dwhkl)
{
return VkKeyScanExW(ch, HKL dwhkl);
}

UINT WINAPI MapVirtualKeyU(
 IN UINT uCode,
 IN UINT uMapType)
{
return MapVirtualKeyW(uCode, UINT uMapType);
}

UINT WINAPI MapVirtualKeyExU(
 IN UINT uCode,
 IN UINT uMapType,
 IN HKL dwhkl)
{
return MapVirtualKeyExW(uCode,uMapType, HKL dwhkl);
}

HACCEL WINAPI LoadAcceleratorsU(
 IN HINSTANCE hInstance,
 IN LPCSTR lpTableName)
{
return LoadAcceleratorsW(hInstance, UTF8_C_MAP(lpTableName));
}

HACCEL WINAPI CreateAcceleratorTableU(
 IN LPACCEL, IN int)
{
return CreateAcceleratorTableW(LPACCEL, IN int);
}

int WINAPI CopyAcceleratorTableU(
 IN HACCEL hAccelSrc,
 OUT LPACCEL lpAccelDst,
 IN int cAccelEntries)
{
return CopyAcceleratorTableW(hAccelSrc,lpAccelDst, int cAccelEntries);
}

int WINAPI TranslateAcceleratorU(
 IN HWND hWnd,
 IN HACCEL hAccTable,
 IN LPMSG lpMsg)
{
return TranslateAcceleratorW(hWnd,hAccTable, LPMSG lpMsg);
}

HMENU WINAPI LoadMenuU(
 IN HINSTANCE hInstance,
 IN LPCSTR lpMenuName)
{
return LoadMenuW(hInstance, UTF8_C_MAP(lpMenuName));
}

HMENU WINAPI LoadMenuIndirectU(
 IN CONST MENUTEMPLATEA *lpMenuTemplate)
{
return LoadMenuIndirectW( CONST MENUTEMPLATEA lpMenuTemplate);
}

BOOL WINAPI ChangeMenuU(
 IN HMENU hMenu,
 IN UINT cmd,
 IN LPCSTR lpszNewItem,
 IN UINT cmdInsert,
 IN UINT flags)
{
return ChangeMenuW(hMenu,cmd, UTF8_C_MAP(lpszNewItem),cmdInsert, UINT flags);
}

int WINAPI GetMenuStringU(
 IN HMENU hMenu,
 IN UINT uIDItem,
 OUT LPSTR lpString,
 IN int nMaxCount,
 IN UINT uFlag)
{
return GetMenuStringW(hMenu,uIDItem, UTF8_MAP(lpString),nMaxCount, UINT uFlag);
}

BOOL WINAPI InsertMenuU(
 IN HMENU hMenu,
 IN UINT uPosition,
 IN UINT uFlags,
 IN UINT_PTR uIDNewItem,
 IN LPCSTR lpNewItem
 )
{
return InsertMenuW(hMenu,uPosition,uFlags,uIDNewItem, UTF8_C_MAP(lpNewItem) );
}

BOOL WINAPI AppendMenuU(
 IN HMENU hMenu,
 IN UINT uFlags,
 IN UINT_PTR uIDNewItem,
 IN LPCSTR lpNewItem
 )
{
return AppendMenuW(hMenu,uFlags,uIDNewItem, UTF8_C_MAP(lpNewItem) );
}

BOOL WINAPI ModifyMenuU(
 IN HMENU hMnu,
 IN UINT uPosition,
 IN UINT uFlags,
 IN UINT_PTR uIDNewItem,
 IN LPCSTR lpNewItem
 )
{
return ModifyMenuW(hMnu,uPosition,uFlags,uIDNewItem, UTF8_C_MAP(lpNewItem) );
}

BOOL WINAPI InsertMenuItemU(
 IN HMENU,
 IN UINT,
 IN BOOL,
 IN LPCMENUITEMINFOA
 )
{
return InsertMenuItemW(HMENU,UINT,BOOL,LPCMENUITEMINFOA );
}

BOOL WINAPI GetMenuItemInfoU(
 IN HMENU,
 IN UINT,
 IN BOOL,
 IN OUT LPMENUITEMINFOA
 )
{
return GetMenuItemInfoW(HMENU,UINT,BOOL,LPMENUITEMINFOA );
}

BOOL WINAPI SetMenuItemInfoU(
 IN HMENU,
 IN UINT,
 IN BOOL,
 IN LPCMENUITEMINFOA
 )
{
return SetMenuItemInfoW(HMENU,UINT,BOOL,LPCMENUITEMINFOA );
}

int WINAPI DrawTextU(
 IN HDC hDC,
 IN LPCSTR lpString,
 IN int nCount,
 IN OUT LPRECT lpRect,
 IN UINT uFormat)
{
return DrawTextW(hDC, UTF8_C_MAP(lpString),nCount,lpRect, UINT uFormat);
}

int WINAPI DrawTextExU(
 IN HDC,
 IN OUT LPSTR,
 IN int,
 IN OUT LPRECT,
 IN UINT,
 IN LPDRAWTEXTPARAMS)
{
return DrawTextExW(HDC,LPSTR,int,LPRECT,UINT, LPDRAWTEXTPARAMS);
}

BOOL WINAPI GrayStringU(
 IN HDC hDC,
 IN HBRUSH hBrush,
 IN GRAYSTRINGPROC lpOutputFunc,
 IN LPARAM lpData,
 IN int nCount,
 IN int X,
 IN int Y,
 IN int nWidth,
 IN int nHeight)
{
return GrayStringW(hDC,hBrush,lpOutputFunc,lpData,nCount,X,Y,nWidth, int nHeight);
}

BOOL WINAPI DrawStateU(
 IN HDC,
 IN HBRUSH,
 IN DRAWSTATEPROC,
 IN LPARAM,
 IN WPARAM,
 IN int,
 IN int,
 IN int,
 IN int,
 IN UINT)
{
return DrawStateW(HDC,HBRUSH,DRAWSTATEPROC,LPARAM,WPARAM,int,int,int,int, UINT);
}

LONG WINAPI TabbedTextOutU(
 IN HDC hDC,
 IN int X,
 IN int Y,
 IN LPCSTR lpString,
 IN int nCount,
 IN int nTabPositions,
 IN CONST INT *lpnTabStopPositions,
 IN int nTabOrigin)
{
return TabbedTextOutW(hDC,X,Y, UTF8_C_MAP(lpString),nCount,nTabPositions,lpnTabStopPositions, int nTabOrigin);
}

DWORD WINAPI GetTabbedTextExtentU(
 IN HDC hDC,
 IN LPCSTR lpString,
 IN int nCount,
 IN int nTabPositions,
 IN CONST INT *lpnTabStopPositions)
{
return GetTabbedTextExtentW(hDC, UTF8_C_MAP(lpString),nCount,nTabPositions, CONST INT lpnTabStopPositions);
}

BOOL WINAPI SetPropU(
 IN HWND hWnd,
 IN LPCSTR lpString,
 IN HANDLE hData)
{
return SetPropW(hWnd, UTF8_C_MAP(lpString), HANDLE hData);
}

HANDLE WINAPI GetPropU(
 IN HWND hWnd,
 IN LPCSTR lpString)
{
return GetPropW(hWnd, UTF8_C_MAP(lpString));
}

HANDLE WINAPI RemovePropU(
 IN HWND hWnd,
 IN LPCSTR lpString)
{
return RemovePropW(hWnd, UTF8_C_MAP(lpString));
}

int WINAPI EnumPropsExU(
 IN HWND hWnd,
 IN PROPENUMPROCEXA lpEnumFunc,
 IN LPARAM lParam)
{
return EnumPropsExW(hWnd,lpEnumFunc, LPARAM lParam);
}

int WINAPI EnumPropsU(
 IN HWND hWnd,
 IN PROPENUMPROCA lpEnumFunc)
{
return EnumPropsW(hWnd, PROPENUMPROCA lpEnumFunc);
}

BOOL WINAPI SetWindowTextU(
 IN HWND hWnd,
 IN LPCSTR lpString)
{
return SetWindowTextW(hWnd, UTF8_C_MAP(lpString));
}

int WINAPI GetWindowTextU(
 IN HWND hWnd,
 OUT LPSTR lpString,
 IN int nMaxCount)
{
return GetWindowTextW(hWnd, UTF8_MAP(lpString), int nMaxCount);
}

int WINAPI GetWindowTextLengthU(
 IN HWND hWnd)
{
return GetWindowTextLengthW( HWND hWnd);
}

int WINAPI MessageBoxU(
 IN HWND hWnd,
 IN LPCSTR lpText,
 IN LPCSTR lpCaption,
 IN UINT uType)
{
return MessageBoxW(hWnd, UTF8_C_MAP(lpText), UTF8_C_MAP(lpCaption), UINT uType);
}

int WINAPI MessageBoxExU(
 IN HWND hWnd,
 IN LPCSTR lpText,
 IN LPCSTR lpCaption,
 IN UINT uType,
 IN WORD wLanguageId)
{
return MessageBoxExW(hWnd, UTF8_C_MAP(lpText), UTF8_C_MAP(lpCaption),uType, WORD wLanguageId);
}

int WINAPI MessageBoxIndirectU(
 IN CONST MSGBOXPARAMSA *)
{
return MessageBoxIndirectW( CONST MSGBOXPARAMSA );
}

LONG WINAPI GetWindowLongU(
 IN HWND hWnd,
 IN int nIndex)
{
return GetWindowLongW(hWnd, int nIndex);
}

LONG WINAPI SetWindowLongU(
 IN HWND hWnd,
 IN int nIndex,
 IN LONG dwNewLong)
{
return SetWindowLongW(hWnd,nIndex, LONG dwNewLong);
}

LONG_PTR WINAPI GetWindowLongPtrU(
 HWND hWnd,
 int nIndex)
{
return GetWindowLongPtrW(hWnd, int nIndex);
}

LONG_PTR WINAPI SetWindowLongPtrU(
 HWND hWnd,
 int nIndex,
 LONG_PTR dwNewLong)
{
return SetWindowLongPtrW(hWnd,nIndex, LONG_PTR dwNewLong);
}

DWORD WINAPI GetClassLongU(
 IN HWND hWnd,
 IN int nIndex)
{
return GetClassLongW(hWnd, int nIndex);
}

DWORD WINAPI SetClassLongU(
 IN HWND hWnd,
 IN int nIndex,
 IN LONG dwNewLong)
{
return SetClassLongW(hWnd,nIndex, LONG dwNewLong);
}

ULONG_PTR WINAPI GetClassLongPtrU(
 IN HWND hWnd,
 IN int nIndex)
{
return GetClassLongPtrW(hWnd, int nIndex);
}

ULONG_PTR WINAPI SetClassLongPtrU(
 IN HWND hWnd,
 IN int nIndex,
 IN LONG_PTR dwNewLong)
{
return SetClassLongPtrW(hWnd,nIndex, LONG_PTR dwNewLong);
}

HWND WINAPI FindWindowU(
 IN LPCSTR lpClassName,
 IN LPCSTR lpWindowName)
{
return FindWindowW( UTF8_C_MAP(lpClassName), UTF8_C_MAP(lpWindowName));
}

int WINAPI GetClassNameU(
 IN HWND hWnd,
 OUT LPSTR lpClassName,
 IN int nMaxCount)
{
return GetClassNameW(hWnd, UTF8_MAP(lpClassName), int nMaxCount);
}

HHOOK WINAPI SetWindowsHookU(
 IN int nFilterType,
 IN HOOKPROC pfnFilterProc)
{
return SetWindowsHookW(nFilterType, HOOKPROC pfnFilterProc);
}

HOOKPROC WINAPI SetWindowsHookU(
 IN int nFilterType,
 IN HOOKPROC pfnFilterProc)
{
return SetWindowsHookW(nFilterType, HOOKPROC pfnFilterProc);
}

HHOOK WINAPI SetWindowsHookExU(
 IN int idHook,
 IN HOOKPROC lpfn,
 IN HINSTANCE hmod,
 IN DWORD dwThreadId)
{
return SetWindowsHookExW(idHook,lpfn,hmod, DWORD dwThreadId);
}

HBITMAP WINAPI LoadBitmapU(
 IN HINSTANCE hInstance,
 IN LPCSTR lpBitmapName)
{
return LoadBitmapW(hInstance, UTF8_C_MAP(lpBitmapName));
}

HCURSOR WINAPI LoadCursorU(
 IN HINSTANCE hInstance,
 IN LPCSTR lpCursorName)
{
return LoadCursorW(hInstance, UTF8_C_MAP(lpCursorName));
}

HCURSOR WINAPI LoadCursorFromFileU(
 IN LPCSTR lpFileName)
{
return LoadCursorFromFileW( UTF8_C_MAP(lpFileName));
}

HICON WINAPI LoadIconU(
 IN HINSTANCE hInstance,
 IN LPCSTR lpIconName)
{
return LoadIconW(hInstance, UTF8_C_MAP(lpIconName));
}

HANDLE WINAPI LoadImageU(
 IN HINSTANCE,
 IN LPCSTR,
 IN UINT,
 IN int,
 IN int,
 IN UINT)
{
return LoadImageW(HINSTANCE,LPCSTR,UINT,int,int, UINT);
}

int WINAPI LoadStringU(
 IN HINSTANCE hInstance,
 IN UINT uID,
 OUT LPSTR lpBuffer,
 IN int nBufferMax)
{
return LoadStringW(hInstance,uID, UTF8_MAP(lpBuffer), int nBufferMax);
}

BOOL WINAPI IsDialogMessageU(
 IN HWND hDlg,
 IN LPMSG lpMsg)
{
return IsDialogMessageW(hDlg, LPMSG lpMsg);
}

int WINAPI DlgDirListU(
 IN HWND hDlg,
 IN OUT LPSTR lpPathSpec,
 IN int nIDListBox,
 IN int nIDStaticPath,
 IN UINT uFileType)
{
return DlgDirListW(hDlg, UTF8_MAP(lpPathSpec),nIDListBox,nIDStaticPath, UINT uFileType);
}

BOOL WINAPI DlgDirSelectExU(
 IN HWND hDlg,
 OUT LPSTR lpString,
 IN int nCount,
 IN int nIDListBox)
{
return DlgDirSelectExW(hDlg, UTF8_MAP(lpString),nCount, int nIDListBox);
}

int WINAPI DlgDirListComboBoxU(
 IN HWND hDlg,
 IN OUT LPSTR lpPathSpec,
 IN int nIDComboBox,
 IN int nIDStaticPath,
 IN UINT uFiletype)
{
return DlgDirListComboBoxW(hDlg, UTF8_MAP(lpPathSpec),nIDComboBox,nIDStaticPath, UINT uFiletype);
}

BOOL WINAPI DlgDirSelectComboBoxExU(
 IN HWND hDlg,
 OUT LPSTR lpString,
 IN int nCount,
 IN int nIDComboBox)
{
return DlgDirSelectComboBoxExW(hDlg, UTF8_MAP(lpString),nCount, int nIDComboBox);
}

LRESULT WINAPI DefFrameProcU(
 IN HWND hWnd,
 IN HWND hWndMDIClient,
 IN UINT uMsg,
 IN WPARAM wParam,
 IN LPARAM lParam)
{
return DefFrameProcW(hWnd,hWndMDIClient,uMsg,wParam, LPARAM lParam);
}

CALLBACK #endif DefMDIChildProcU(
 IN HWND hWnd,
 IN UINT uMsg,
 IN WPARAM wParam,
 IN LPARAM lParam)
{
return DefMDIChildProcW(hWnd,uMsg,wParam, LPARAM lParam);
}

HWND WINAPI CreateMDIWindowU(
 IN LPCSTR lpClassName,
 IN LPCSTR lpWindowName,
 IN DWORD dwStyle,
 IN int X,
 IN int Y,
 IN int nWidth,
 IN int nHeight,
 IN HWND hWndParent,
 IN HINSTANCE hInstance,
 IN LPARAM lParam
 )
{
return CreateMDIWindowW( UTF8_C_MAP(lpClassName), UTF8_C_MAP(lpWindowName),dwStyle,X,Y,nWidth,nHeight,hWndParent,hInstance,lParam );
}

BOOL WINAPI WinHelpU(
 IN HWND hWndMain,
 IN LPCSTR lpszHelp,
 IN UINT uCommand,
 IN ULONG_PTR dwData
 )
{
return WinHelpW(hWndMain, UTF8_C_MAP(lpszHelp),uCommand,dwData );
}

LONG WINAPI ChangeDisplaySettingsU(
 IN LPDEVMODEA lpDevMode,
 IN DWORD dwFlags)
{
return ChangeDisplaySettingsW(lpDevMode, DWORD dwFlags);
}

LONG WINAPI ChangeDisplaySettingsExU(
 IN LPCSTR lpszDeviceName,
 IN LPDEVMODEA lpDevMode,
 IN HWND hwnd,
 IN DWORD dwflags,
 IN LPVOID lParam)
{
return ChangeDisplaySettingsExW( UTF8_C_MAP(lpszDeviceName),lpDevMode,hwnd,dwflags, LPVOID lParam);
}

BOOL WINAPI EnumDisplaySettingsU(
 IN LPCSTR lpszDeviceName,
 IN DWORD iModeNum,
 OUT LPDEVMODEA lpDevMode)
{
return EnumDisplaySettingsW( UTF8_C_MAP(lpszDeviceName),iModeNum, LPDEVMODEA lpDevMode);
}

BOOL WINAPI EnumDisplaySettingsExU(
 IN LPCSTR lpszDeviceName,
 IN DWORD iModeNum,
 OUT LPDEVMODEA lpDevMode,
 IN DWORD dwFlags)
{
return EnumDisplaySettingsExW( UTF8_C_MAP(lpszDeviceName),iModeNum,lpDevMode, DWORD dwFlags);
}

BOOL WINAPI EnumDisplayDevicesU(
 IN LPCSTR lpDevice,
 IN DWORD iDevNum,
 OUT PDISPLAY_DEVICEA lpDisplayDevice,
 IN DWORD dwFlags)
{
return EnumDisplayDevicesW( UTF8_C_MAP(lpDevice),iDevNum,lpDisplayDevice, DWORD dwFlags);
}

BOOL WINAPI SystemParametersInfoU(
 IN UINT uiAction,
 IN UINT uiParam,
 IN OUT PVOID pvParam,
 IN UINT fWinIni)
{
return SystemParametersInfoW(uiAction,uiParam,pvParam, UINT fWinIni);
}

UINT WINAPI GetWindowModuleFileNameU(
 IN HWND hwnd,
 OUT LPSTR pszFileName,
 IN UINT cchFileNameMax)
{
return GetWindowModuleFileNameW(hwnd, UTF8_MAP(pszFileName), UINT cchFileNameMax);
}

UINT WINAPI RealGetWindowClassU(
 IN HWND hwnd,
 OUT LPSTR pszType,
 IN UINT cchType
)
{
return RealGetWindowClassW(hwnd, UTF8_MAP(pszType),cchType);
}

BOOL WINAPI GetAltTabInfoU(
 IN HWND hwnd,
 IN int iItem,
 OUT PALTTABINFO pati,
 OUT LPSTR pszItemText,
 IN UINT cchItemText
)
{
return GetAltTabInfoW(hwnd,iItem,pati, UTF8_MAP(pszItemText),cchItemText);
}

UINT WINAPI GetRawInputDeviceInfoU(
 IN HANDLE hDevice,
 IN UINT uiCommand,
 OUT LPVOID pData,
 IN OUT PUINT pcbSize)
{
return GetRawInputDeviceInfoW(hDevice,uiCommand,pData, PUINT pcbSize);
}

