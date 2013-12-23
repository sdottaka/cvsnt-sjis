int
WINAPI
wvsprintfU(
    OUT LPSTR,
    IN LPCSTR,
    IN va_list arglist);

int
WINAPIV
wsprintfU(
    OUT LPSTR,
    IN LPCSTR,
    ...);

HKL
WINAPI
LoadKeyboardLayoutU(
    IN LPCSTR pwszKLID,
    IN UINT Flags);

BOOL
WINAPI
GetKeyboardLayoutNameU(
    OUT LPSTR pwszKLID);

HDESK
WINAPI
CreateDesktopU(
    IN LPCSTR lpszDesktop,
    IN LPCSTR lpszDevice,
    IN LPDEVMODEA pDevmode,
    IN DWORD dwFlags,
    IN ACCESS_MASK dwDesiredAccess,
    IN LPSECURITY_ATTRIBUTES lpsa);

HDESK
WINAPI
OpenDesktopU(
    IN LPCSTR lpszDesktop,
    IN DWORD dwFlags,
    IN BOOL fInherit,
    IN ACCESS_MASK dwDesiredAccess);

BOOL
WINAPI
EnumDesktopsU(
    IN HWINSTA hwinsta,
    IN DESKTOPENUMPROCA lpEnumFunc,
    IN LPARAM lParam);

HWINSTA
WINAPI
CreateWindowStationU(
    IN LPCSTR              lpwinsta,
    IN DWORD                 dwReserved,
    IN ACCESS_MASK           dwDesiredAccess,
    IN LPSECURITY_ATTRIBUTES lpsa);

HWINSTA
WINAPI
OpenWindowStationU(
    IN LPCSTR lpszWinSta,
    IN BOOL fInherit,
    IN ACCESS_MASK dwDesiredAccess);

BOOL
WINAPI
EnumWindowStationsU(
    IN WINSTAENUMPROCA lpEnumFunc,
    IN LPARAM lParam);

BOOL
WINAPI
GetUserObjectInformationU(
    IN HANDLE hObj,
    IN int nIndex,
    OUT PVOID pvInfo,
    IN DWORD nLength,
    OUT LPDWORD lpnLengthNeeded);

BOOL
WINAPI
SetUserObjectInformationU(
    IN HANDLE hObj,
    IN int nIndex,
    IN PVOID pvInfo,
    IN DWORD nLength);

UINT
WINAPI
RegisterWindowMessageU(
    IN LPCSTR lpString);

BOOL
WINAPI
GetMessageU(
    OUT LPMSG lpMsg,
    IN HWND hWnd,
    IN UINT wMsgFilterMin,
    IN UINT wMsgFilterMax);

LRESULT
WINAPI
DispatchMessageU(
    IN CONST MSG *lpMsg);

BOOL
WINAPI
PeekMessageU(
    OUT LPMSG lpMsg,
    IN HWND hWnd,
    IN UINT wMsgFilterMin,
    IN UINT wMsgFilterMax,
    IN UINT wRemoveMsg);

LRESULT
WINAPI
SendMessageU(
    IN HWND hWnd,
    IN UINT Msg,
    IN WPARAM wParam,
    IN LPARAM lParam);

LRESULT
WINAPI
SendMessageTimeoutU(
    IN HWND hWnd,
    IN UINT Msg,
    IN WPARAM wParam,
    IN LPARAM lParam,
    IN UINT fuFlags,
    IN UINT uTimeout,
    OUT PDWORD_PTR lpdwResult);

BOOL
WINAPI
SendNotifyMessageU(
    IN HWND hWnd,
    IN UINT Msg,
    IN WPARAM wParam,
    IN LPARAM lParam);

BOOL
WINAPI
SendMessageCallbackU(
    IN HWND hWnd,
    IN UINT Msg,
    IN WPARAM wParam,
    IN LPARAM lParam,
    IN SENDASYNCPROC lpResultCallBack,
    IN ULONG_PTR dwData);

/*
long
WINAPI
BroadcastSystemMessageExU(
    IN DWORD,
    IN LPDWORD,
    IN UINT,
    IN WPARAM,
    IN LPARAM,
    OUT PBSMINFO);
*/
long
WINAPI
BroadcastSystemMessageU(
    IN DWORD,
    IN LPDWORD,
    IN UINT,
    IN WPARAM,
    IN LPARAM);

HDEVNOTIFY
WINAPI
RegisterDeviceNotificationU(
    IN HANDLE hRecipient,
    IN LPVOID NotificationFilter,
    IN DWORD Flags
    );

BOOL
WINAPI
PostMessageU(
    IN HWND hWnd,
    IN UINT Msg,
    IN WPARAM wParam,
    IN LPARAM lParam);

BOOL
WINAPI
PostThreadMessageU(
    IN DWORD idThread,
    IN UINT Msg,
    IN WPARAM wParam,
    IN LPARAM lParam);

LRESULT
CALLBACK
DefWindowProcU(
    IN HWND hWnd,
    IN UINT Msg,
    IN WPARAM wParam,
    IN LPARAM lParam);

LRESULT
WINAPI
CallWindowProcU(
    IN WNDPROC lpPrevWndFunc,
    IN HWND hWnd,
    IN UINT Msg,
    IN WPARAM wParam,
    IN LPARAM lParam);

LRESULT
WINAPI
CallWindowProcU(
    IN FARPROC lpPrevWndFunc,
    IN HWND hWnd,
    IN UINT Msg,
    IN WPARAM wParam,
    IN LPARAM lParam);

ATOM
WINAPI
RegisterClassU(
    IN CONST WNDCLASSA *lpWndClass);

BOOL
WINAPI
UnregisterClassU(
    IN LPCSTR lpClassName,
    IN HINSTANCE hInstance);

BOOL
WINAPI
GetClassInfoU(
    IN HINSTANCE hInstance,
    IN LPCSTR lpClassName,
    OUT LPWNDCLASSA lpWndClass);

ATOM
WINAPI
RegisterClassExU(
    IN CONST WNDCLASSEXA *);

BOOL
WINAPI
GetClassInfoExU(
    IN HINSTANCE,
    IN LPCSTR,
    OUT LPWNDCLASSEXA);

HWND
WINAPI
CreateWindowExU(
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
    IN LPVOID lpParam);

HWND
WINAPI
CreateDialogParamU(
    IN HINSTANCE hInstance,
    IN LPCSTR lpTemplateName,
    IN HWND hWndParent,
    IN DLGPROC lpDialogFunc,
    IN LPARAM dwInitParam);

HWND
WINAPI
CreateDialogIndirectParamU(
    IN HINSTANCE hInstance,
    IN LPCDLGTEMPLATEA lpTemplate,
    IN HWND hWndParent,
    IN DLGPROC lpDialogFunc,
    IN LPARAM dwInitParam);

INT_PTR
WINAPI
DialogBoxParamU(
    IN HINSTANCE hInstance,
    IN LPCSTR lpTemplateName,
    IN HWND hWndParent,
    IN DLGPROC lpDialogFunc,
    IN LPARAM dwInitParam);

INT_PTR
WINAPI
DialogBoxIndirectParamU(
    IN HINSTANCE hInstance,
    IN LPCDLGTEMPLATEA hDialogTemplate,
    IN HWND hWndParent,
    IN DLGPROC lpDialogFunc,
    IN LPARAM dwInitParam);

BOOL
WINAPI
SetDlgItemTextU(
    IN HWND hDlg,
    IN int nIDDlgItem,
    IN LPCSTR lpString);

UINT
WINAPI
GetDlgItemTextU(
    IN HWND hDlg,
    IN int nIDDlgItem,
    OUT LPSTR lpString,
    IN int nMaxCount);

LRESULT
WINAPI
SendDlgItemMessageU(
    IN HWND hDlg,
    IN int nIDDlgItem,
    IN UINT Msg,
    IN WPARAM wParam,
    IN LPARAM lParam);

LRESULT
CALLBACK
DefDlgProcU(
    IN HWND hDlg,
    IN UINT Msg,
    IN WPARAM wParam,
    IN LPARAM lParam);

BOOL
WINAPI
CallMsgFilterU(
    IN LPMSG lpMsg,
    IN int nCode);

UINT
WINAPI
RegisterClipboardFormatU(
    IN LPCSTR lpszFormat);

int
WINAPI
GetClipboardFormatNameU(
    IN UINT format,
    OUT LPSTR lpszFormatName,
    IN int cchMaxCount);

BOOL
WINAPI
CharToOemU(
    IN LPCSTR lpszSrc,
    OUT LPSTR lpszDst);

BOOL
WINAPI
OemToCharU(
    IN LPCSTR lpszSrc,
    OUT LPSTR lpszDst);

BOOL
WINAPI
CharToOemBuffU(
    IN LPCSTR lpszSrc,
    OUT LPSTR lpszDst,
    IN DWORD cchDstLength);

BOOL
WINAPI
OemToCharBuffU(
    IN LPCSTR lpszSrc,
    OUT LPSTR lpszDst,
    IN DWORD cchDstLength);

LPSTR
WINAPI
CharUpperU(
    IN OUT LPSTR lpsz);

DWORD
WINAPI
CharUpperBuffU(
    IN OUT LPSTR lpsz,
    IN DWORD cchLength);

LPSTR
WINAPI
CharLowerU(
    IN OUT LPSTR lpsz);

DWORD
WINAPI
CharLowerBuffU(
    IN OUT LPSTR lpsz,
    IN DWORD cchLength);

LPSTR
WINAPI
CharNextU(
    IN LPCSTR lpsz);

LPSTR
WINAPI
CharPrevU(
    IN LPCSTR lpszStart,
    IN LPCSTR lpszCurrent);

LPSTR
WINAPI
CharNextExU(
     IN WORD CodePage,
     IN LPCSTR lpCurrentChar,
     IN DWORD dwFlags);

LPSTR
WINAPI
CharPrevExU(
     IN WORD CodePage,
     IN LPCSTR lpStart,
     IN LPCSTR lpCurrentChar,
     IN DWORD dwFlags);

BOOL
WINAPI
IsCharAlphaU(
    IN CHAR ch);

BOOL
WINAPI
IsCharAlphaNumericU(
    IN CHAR ch);

BOOL
WINAPI
IsCharUpperU(
    IN CHAR ch);

BOOL
WINAPI
IsCharLowerU(
    IN CHAR ch);

int
WINAPI
GetKeyNameTextU(
    IN LONG lParam,
    OUT LPSTR lpString,
    IN int nSize
    );

SHORT
WINAPI
VkKeyScanU(
    IN CHAR ch);

SHORT
WINAPI
VkKeyScanExU(
    IN CHAR  ch,
    IN HKL   dwhkl);

UINT
WINAPI
MapVirtualKeyU(
    IN UINT uCode,
    IN UINT uMapType);

UINT
WINAPI
MapVirtualKeyExU(
    IN UINT uCode,
    IN UINT uMapType,
    IN HKL dwhkl);

HACCEL
WINAPI
LoadAcceleratorsU(
    IN HINSTANCE hInstance,
    IN LPCSTR lpTableName);

HACCEL
WINAPI
CreateAcceleratorTableU(
    IN LPACCEL, IN int);

int
WINAPI
CopyAcceleratorTableU(
    IN HACCEL hAccelSrc,
    OUT LPACCEL lpAccelDst,
    IN int cAccelEntries);

int
WINAPI
TranslateAcceleratorU(
    IN HWND hWnd,
    IN HACCEL hAccTable,
    IN LPMSG lpMsg);

HMENU
WINAPI
LoadMenuU(
    IN HINSTANCE hInstance,
    IN LPCSTR lpMenuName);

HMENU
WINAPI
LoadMenuIndirectU(
    IN CONST MENUTEMPLATEA *lpMenuTemplate);

BOOL
WINAPI
ChangeMenuU(
    IN HMENU hMenu,
    IN UINT cmd,
    IN LPCSTR lpszNewItem,
    IN UINT cmdInsert,
    IN UINT flags);

int
WINAPI
GetMenuStringU(
    IN HMENU hMenu,
    IN UINT uIDItem,
    OUT LPSTR lpString,
    IN int nMaxCount,
    IN UINT uFlag);

BOOL
WINAPI
InsertMenuU(
    IN HMENU hMenu,
    IN UINT uPosition,
    IN UINT uFlags,
    IN UINT_PTR uIDNewItem,
    IN LPCSTR lpNewItem
    );

BOOL
WINAPI
AppendMenuU(
    IN HMENU hMenu,
    IN UINT uFlags,
    IN UINT_PTR uIDNewItem,
    IN LPCSTR lpNewItem
    );

BOOL
WINAPI
ModifyMenuU(
    IN HMENU hMnu,
    IN UINT uPosition,
    IN UINT uFlags,
    IN UINT_PTR uIDNewItem,
    IN LPCSTR lpNewItem
    );

BOOL
WINAPI
InsertMenuItemU(
    IN HMENU,
    IN UINT,
    IN BOOL,
    IN LPCMENUITEMINFOA
    );

BOOL
WINAPI
GetMenuItemInfoU(
    IN HMENU,
    IN UINT,
    IN BOOL,
    IN OUT LPMENUITEMINFOA
    );

BOOL
WINAPI
SetMenuItemInfoU(
    IN HMENU,
    IN UINT,
    IN BOOL,
    IN LPCMENUITEMINFOA
    );

int
WINAPI
DrawTextU(
    IN HDC hDC,
    IN LPCSTR lpString,
    IN int nCount,
    IN OUT LPRECT lpRect,
    IN UINT uFormat);

int
WINAPI
DrawTextExU(
    IN HDC,
    IN OUT LPSTR,
    IN int,
    IN OUT LPRECT,
    IN UINT,
    IN LPDRAWTEXTPARAMS);

BOOL
WINAPI
GrayStringU(
    IN HDC hDC,
    IN HBRUSH hBrush,
    IN GRAYSTRINGPROC lpOutputFunc,
    IN LPARAM lpData,
    IN int nCount,
    IN int X,
    IN int Y,
    IN int nWidth,
    IN int nHeight);

BOOL
WINAPI
DrawStateU(
    IN HDC,
    IN HBRUSH,
    IN DRAWSTATEPROC,
    IN LPARAM,
    IN WPARAM,
    IN int,
    IN int,
    IN int,
    IN int,
    IN UINT);

LONG
WINAPI
TabbedTextOutU(
    IN HDC hDC,
    IN int X,
    IN int Y,
    IN LPCSTR lpString,
    IN int nCount,
    IN int nTabPositions,
    IN CONST INT *lpnTabStopPositions,
    IN int nTabOrigin);

DWORD
WINAPI
GetTabbedTextExtentU(
    IN HDC hDC,
    IN LPCSTR lpString,
    IN int nCount,
    IN int nTabPositions,
    IN CONST INT *lpnTabStopPositions);

BOOL
WINAPI
SetPropU(
    IN HWND hWnd,
    IN LPCSTR lpString,
    IN HANDLE hData);

HANDLE
WINAPI
GetPropU(
    IN HWND hWnd,
    IN LPCSTR lpString);

HANDLE
WINAPI
RemovePropU(
    IN HWND hWnd,
    IN LPCSTR lpString);

int
WINAPI
EnumPropsExU(
    IN HWND hWnd,
    IN PROPENUMPROCEXA lpEnumFunc,
    IN LPARAM lParam);

int
WINAPI
EnumPropsU(
    IN HWND hWnd,
    IN PROPENUMPROCA lpEnumFunc);

BOOL
WINAPI
SetWindowTextU(
    IN HWND hWnd,
    IN LPCSTR lpString);

int
WINAPI
GetWindowTextU(
    IN HWND hWnd,
    OUT LPSTR lpString,
    IN int nMaxCount);

int
WINAPI
GetWindowTextLengthU(
    IN HWND hWnd);

int
WINAPI
MessageBoxU(
    IN HWND hWnd,
    IN LPCSTR lpText,
    IN LPCSTR lpCaption,
    IN UINT uType);

int
WINAPI
MessageBoxExU(
    IN HWND hWnd,
    IN LPCSTR lpText,
    IN LPCSTR lpCaption,
    IN UINT uType,
    IN WORD wLanguageId);

int
WINAPI
MessageBoxIndirectU(
    IN CONST MSGBOXPARAMSA *);

LONG
WINAPI
GetWindowLongU(
    IN HWND hWnd,
    IN int nIndex);

LONG
WINAPI
SetWindowLongU(
    IN HWND hWnd,
    IN int nIndex,
    IN LONG dwNewLong);

LONG_PTR
WINAPI
GetWindowLongPtrU(
    HWND hWnd,
    int nIndex);

LONG_PTR
WINAPI
SetWindowLongPtrU(
    HWND hWnd,
    int nIndex,
    LONG_PTR dwNewLong);

DWORD
WINAPI
GetClassLongU(
    IN HWND hWnd,
    IN int nIndex);

DWORD
WINAPI
SetClassLongU(
    IN HWND hWnd,
    IN int nIndex,
    IN LONG dwNewLong);

ULONG_PTR
WINAPI
GetClassLongPtrU(
    IN HWND hWnd,
    IN int nIndex);

ULONG_PTR
WINAPI
SetClassLongPtrU(
    IN HWND hWnd,
    IN int nIndex,
    IN LONG_PTR dwNewLong);

HWND
WINAPI
FindWindowU(
    IN LPCSTR lpClassName,
    IN LPCSTR lpWindowName);


int
WINAPI
GetClassNameU(
    IN HWND hWnd,
    OUT LPSTR lpClassName,
    IN int nMaxCount);

HHOOK
WINAPI
SetWindowsHookU(
    IN int nFilterType,
    IN HOOKPROC pfnFilterProc);

HHOOK
WINAPI
SetWindowsHookExU(
    IN int idHook,
    IN HOOKPROC lpfn,
    IN HINSTANCE hmod,
    IN DWORD dwThreadId);

HBITMAP
WINAPI
LoadBitmapU(
    IN HINSTANCE hInstance,
    IN LPCSTR lpBitmapName);

HCURSOR
WINAPI
LoadCursorU(
    IN HINSTANCE hInstance,
    IN LPCSTR lpCursorName);

HCURSOR
WINAPI
LoadCursorFromFileU(
    IN LPCSTR lpFileName);

HICON
WINAPI
LoadIconU(
    IN HINSTANCE hInstance,
    IN LPCSTR lpIconName);

HANDLE
WINAPI
LoadImageU(
    IN HINSTANCE,
    IN LPCSTR,
    IN UINT,
    IN int,
    IN int,
    IN UINT);

int
WINAPI
LoadStringU(
    IN HINSTANCE hInstance,
    IN UINT uID,
    OUT LPSTR lpBuffer,
    IN int nBufferMax);

BOOL
WINAPI
IsDialogMessageU(
    IN HWND hDlg,
    IN LPMSG lpMsg);

int
WINAPI
DlgDirListU(
    IN HWND hDlg,
    IN OUT LPSTR lpPathSpec,
    IN int nIDListBox,
    IN int nIDStaticPath,
    IN UINT uFileType);

BOOL
WINAPI
DlgDirSelectExU(
    IN HWND hDlg,
    OUT LPSTR lpString,
    IN int nCount,
    IN int nIDListBox);

int
WINAPI
DlgDirListComboBoxU(
    IN HWND hDlg,
    IN OUT LPSTR lpPathSpec,
    IN int nIDComboBox,
    IN int nIDStaticPath,
    IN UINT uFiletype);

BOOL
WINAPI
DlgDirSelectComboBoxExU(
    IN HWND hDlg,
    OUT LPSTR lpString,
    IN int nCount,
    IN int nIDComboBox);

LRESULT
WINAPI
DefFrameProcU(
    IN HWND hWnd,
    IN HWND hWndMDIClient,
    IN UINT uMsg,
    IN WPARAM wParam,
    IN LPARAM lParam);

LRESULT
CALLBACK
DefMDIChildProcU(
    IN HWND hWnd,
    IN UINT uMsg,
    IN WPARAM wParam,
    IN LPARAM lParam);

HWND
WINAPI
CreateMDIWindowU(
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
    );

BOOL
WINAPI
WinHelpU(
    IN HWND hWndMain,
    IN LPCSTR lpszHelp,
    IN UINT uCommand,
    IN ULONG_PTR dwData
    );

LONG
WINAPI
ChangeDisplaySettingsU(
    IN LPDEVMODEA  lpDevMode,
    IN DWORD       dwFlags);

LONG
WINAPI
ChangeDisplaySettingsExU(
    IN LPCSTR    lpszDeviceName,
    IN LPDEVMODEA  lpDevMode,
    IN HWND        hwnd,
    IN DWORD       dwflags,
    IN LPVOID      lParam);

BOOL
WINAPI
EnumDisplaySettingsU(
    IN LPCSTR lpszDeviceName,
    IN DWORD iModeNum,
    OUT LPDEVMODEA lpDevMode);

BOOL
WINAPI
EnumDisplaySettingsExU(
    IN LPCSTR lpszDeviceName,
    IN DWORD iModeNum,
    OUT LPDEVMODEA lpDevMode,
    IN DWORD dwFlags);

BOOL
WINAPI
EnumDisplayDevicesU(
    IN LPCSTR lpDevice,
    IN DWORD iDevNum,
    OUT PDISPLAY_DEVICEA lpDisplayDevice,
    IN DWORD dwFlags);

BOOL
WINAPI
SystemParametersInfoU(
    IN UINT uiAction,
    IN UINT uiParam,
    IN OUT PVOID pvParam,
    IN UINT fWinIni);

UINT
WINAPI
GetWindowModuleFileNameU(
    IN HWND     hwnd,
    OUT LPSTR pszFileName,
    IN UINT     cchFileNameMax);

UINT
WINAPI
RealGetWindowClassU(
    IN HWND  hwnd,
    OUT LPSTR pszType,
    IN UINT  cchType
);

BOOL
WINAPI
GetAltTabInfoU(
    IN HWND hwnd,
    IN int iItem,
    OUT PALTTABINFO pati,
    OUT LPSTR pszItemText,
    IN UINT cchItemText
);

UINT
WINAPI
GetRawInputDeviceInfoU(
    IN HANDLE hDevice,
    IN UINT uiCommand,
    OUT LPVOID pData,
    IN OUT PUINT pcbSize);

