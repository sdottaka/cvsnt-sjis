#include "stdafx.h"
#include "winbase_utf8.h"

BOOL WINAPI GetBinaryTypeU(
 IN LPCSTR lpApplicationName,
 OUT LPDWORD lpBinaryType
 )
{
return GetBinaryTypeW( UTF8_C_MAP(lpApplicationName),lpBinaryType );
}

DWORD WINAPI GetShortPathNameU(
 IN LPCSTR lpszLongPath,
 OUT LPSTR lpszShortPath,
 IN DWORD cchBuffer
 )
{
return GetShortPathNameW( UTF8_C_MAP(lpszLongPath), UTF8_MAP(lpszShortPath,cchBuffer),cchBuffer );
}

DWORD WINAPI GetLongPathNameU(
 IN LPCSTR lpszShortPath,
 OUT LPSTR lpszLongPath,
 IN DWORD cchBuffer
 )
{
return GetLongPathNameW( UTF8_C_MAP(lpszShortPath), UTF8_MAP(lpszLongPath,cchBuffer),cchBuffer );
}

BOOL WINAPI FreeEnvironmentStringsU(
 IN LPSTR lpszEnvironmentBlock
 )
{
return FreeEnvironmentStringsW( (LPWSTR)UTF8_C_MAP(lpszEnvironmentBlock) );
}

BOOL WINAPI SetFileShortNameU(
 IN HANDLE hFile,
 IN LPCSTR lpShortName
 )
{
return SetFileShortNameW(hFile, UTF8_C_MAP(lpShortName) );
}

DWORD WINAPI FormatMessageU(
 IN DWORD dwFlags,
 IN LPCVOID lpSource,
 IN DWORD dwMessageId,
 IN DWORD dwLanguageId,
 OUT LPSTR lpBuffer,
 IN DWORD nSize,
 IN va_list *Arguments
 )
{
return FormatMessageW(dwFlags,lpSource,dwMessageId,dwLanguageId, UTF8_MAP(lpBuffer,nSize),nSize,Arguments );
}

HANDLE WINAPI CreateMailslotU(
 IN LPCSTR lpName,
 IN DWORD nMaxMessageSize,
 IN DWORD lReadTimeout,
 IN LPSECURITY_ATTRIBUTES lpSecurityAttributes
 )
{
return CreateMailslotW( UTF8_C_MAP(lpName),nMaxMessageSize,lReadTimeout,lpSecurityAttributes );
}

BOOL WINAPI EncryptFileU(
 IN LPCSTR lpFileName
 )
{
return EncryptFileW( UTF8_C_MAP(lpFileName) );
}

BOOL WINAPI DecryptFileU(
 IN LPCSTR lpFileName,
 IN DWORD dwReserved
 )
{
return DecryptFileW( UTF8_C_MAP(lpFileName),dwReserved );
}

BOOL WINAPI FileEncryptionStatusU(
 LPCSTR lpFileName,
 LPDWORD lpStatus
 )
{
return FileEncryptionStatusW( UTF8_C_MAP(lpFileName),lpStatus );
}

DWORD WINAPI OpenEncryptedFileRawU(
 IN LPCSTR lpFileName,
 IN ULONG ulFlags,
 IN PVOID * pvContext
 )
{
return OpenEncryptedFileRawW( UTF8_C_MAP(lpFileName),ulFlags,pvContext );
}

int WINAPI lstrcmpU(
 IN LPCSTR lpString1,
 IN LPCSTR lpString2
 )
{
return lstrcmpW( UTF8_C_MAP(lpString1), UTF8_C_MAP(lpString2) );
}

int WINAPI lstrcmpiU(
 IN LPCSTR lpString1,
 IN LPCSTR lpString2
 )
{
return lstrcmpiW( UTF8_C_MAP(lpString1), UTF8_C_MAP(lpString2) );
}

void WINAPI lstrcpynU(
 OUT LPSTR lpString1,
 IN LPCSTR lpString2,
 IN int iMaxLength
 )
{
	lstrcpynW( UTF8_MAP(lpString1,iMaxLength), UTF8_C_MAP(lpString2),iMaxLength );
}

void WINAPI lstrcpyU(
 OUT LPSTR lpString1,
 IN LPCSTR lpString2
 )
{
	lstrcpyW( UTF8_MAP(lpString1,strlen(lpString2)+1), UTF8_C_MAP(lpString2) );
}

void WINAPI lstrcatU(
 IN OUT LPSTR lpString1,
 IN LPCSTR lpString2
 )
{
	lstrcatW( UTF8_MAP(lpString1,strlen(lpString1)+strlen(lpString2)+1), UTF8_C_MAP(lpString2) );
}

int WINAPI lstrlenU(
 IN LPCSTR lpString
 )
{
return lstrlenW( UTF8_C_MAP(lpString) );
}

HANDLE WINAPI CreateMutexU(
 IN LPSECURITY_ATTRIBUTES lpMutexAttributes,
 IN BOOL bInitialOwner,
 IN LPCSTR lpName
 )
{
return CreateMutexW(lpMutexAttributes,bInitialOwner, UTF8_C_MAP(lpName) );
}

HANDLE WINAPI OpenMutexU(
 IN DWORD dwDesiredAccess,
 IN BOOL bInheritHandle,
 IN LPCSTR lpName
 )
{
return OpenMutexW(dwDesiredAccess,bInheritHandle, UTF8_C_MAP(lpName) );
}

HANDLE WINAPI CreateEventU(
 IN LPSECURITY_ATTRIBUTES lpEventAttributes,
 IN BOOL bManualReset,
 IN BOOL bInitialState,
 IN LPCSTR lpName
 )
{
return CreateEventW(lpEventAttributes,bManualReset,bInitialState, UTF8_C_MAP(lpName) );
}

HANDLE WINAPI OpenEventU(
 IN DWORD dwDesiredAccess,
 IN BOOL bInheritHandle,
 IN LPCSTR lpName
 )
{
return OpenEventW(dwDesiredAccess,bInheritHandle, UTF8_C_MAP(lpName) );
}

HANDLE WINAPI CreateSemaphoreU(
 IN LPSECURITY_ATTRIBUTES lpSemaphoreAttributes,
 IN LONG lInitialCount,
 IN LONG lMaximumCount,
 IN LPCSTR lpName
 )
{
return CreateSemaphoreW(lpSemaphoreAttributes,lInitialCount,lMaximumCount, UTF8_C_MAP(lpName) );
}

HANDLE WINAPI OpenSemaphoreU(
 IN DWORD dwDesiredAccess,
 IN BOOL bInheritHandle,
 IN LPCSTR lpName
 )
{
return OpenSemaphoreW(dwDesiredAccess,bInheritHandle, UTF8_C_MAP(lpName) );
}

HANDLE WINAPI CreateWaitableTimerU(
 IN LPSECURITY_ATTRIBUTES lpTimerAttributes,
 IN BOOL bManualReset,
 IN LPCSTR lpTimerName
 )
{
return CreateWaitableTimerW(lpTimerAttributes,bManualReset, UTF8_C_MAP(lpTimerName) );
}

HANDLE WINAPI OpenWaitableTimerU(
 IN DWORD dwDesiredAccess,
 IN BOOL bInheritHandle,
 IN LPCSTR lpTimerName
 )
{
return OpenWaitableTimerW(dwDesiredAccess,bInheritHandle, UTF8_C_MAP(lpTimerName) );
}

HANDLE WINAPI CreateFileMappingU(
 IN HANDLE hFile,
 IN LPSECURITY_ATTRIBUTES lpFileMappingAttributes,
 IN DWORD flProtect,
 IN DWORD dwMaximumSizeHigh,
 IN DWORD dwMaximumSizeLow,
 IN LPCSTR lpName
 )
{
return CreateFileMappingW(hFile,lpFileMappingAttributes,flProtect,dwMaximumSizeHigh,dwMaximumSizeLow, UTF8_C_MAP(lpName) );
}

HANDLE WINAPI OpenFileMappingU(
 IN DWORD dwDesiredAccess,
 IN BOOL bInheritHandle,
 IN LPCSTR lpName
 )
{
return OpenFileMappingW(dwDesiredAccess,bInheritHandle, UTF8_C_MAP(lpName) );
}

DWORD WINAPI GetLogicalDriveStringsU(
 IN DWORD nBufferLength,
 OUT LPSTR lpBuffer
 )
{
return GetLogicalDriveStringsW(nBufferLength, UTF8_MAP(lpBuffer,nBufferLength) );
}

HMODULE WINAPI LoadLibraryU(
 IN LPCSTR lpLibFileName
 )
{
return LoadLibraryW( UTF8_C_MAP(lpLibFileName) );
}

HMODULE WINAPI LoadLibraryExU(
 IN LPCSTR lpLibFileName,
 IN HANDLE hFile,
 IN DWORD dwFlags
 )
{
return LoadLibraryExW( UTF8_C_MAP(lpLibFileName),hFile,dwFlags );
}

DWORD WINAPI GetModuleFileNameU(
 IN HMODULE hModule,
 OUT LPSTR lpFilename,
 IN DWORD nSize
 )
{
return GetModuleFileNameW(hModule, UTF8_MAP(lpFilename,nSize),nSize );
}

HMODULE WINAPI GetModuleHandleU(
 IN LPCSTR lpModuleName
 )
{
return GetModuleHandleW( UTF8_C_MAP(lpModuleName) );
}

/*
BOOL WINAPI GetModuleHandleExU(
 IN DWORD dwFlags,
 IN LPCSTR lpModuleName,
 OUT HMODULE* phModule
 )
{
return GetModuleHandleExW(dwFlags, UTF8_C_MAP(lpModuleName),phModule );
}
*/

BOOL WINAPI CreateProcessU(
 IN LPCSTR lpApplicationName,
 IN LPSTR lpCommandLine,
 IN LPSECURITY_ATTRIBUTES lpProcessAttributes,
 IN LPSECURITY_ATTRIBUTES lpThreadAttributes,
 IN BOOL bInheritHandles,
 IN DWORD dwCreationFlags,
 IN LPVOID lpEnvironment,
 IN LPCSTR lpCurrentDirectory,
 IN LPSTARTUPINFOA lpStartupInfo,
 OUT LPPROCESS_INFORMATION lpProcessInformation
 )
{
	UTF8_STRING(lpwReserved,lpStartupInfo->lpReserved);
	UTF8_STRING(lpwDesktop,lpStartupInfo->lpDesktop);
	UTF8_STRING(lpwTitle,lpStartupInfo->lpTitle);

	STARTUPINFOW wStartupInfo =
	{
		sizeof(STARTUPINFOW),
		lpwReserved,
		lpwDesktop,
		lpwTitle,
		lpStartupInfo->dwX,
		lpStartupInfo->dwY,
		lpStartupInfo->dwXSize,
		lpStartupInfo->dwYSize,
		lpStartupInfo->dwXCountChars, 
		lpStartupInfo->dwYCountChars, 
		lpStartupInfo->dwFillAttribute, 
		lpStartupInfo->dwFlags, 
		lpStartupInfo->wShowWindow, 
		lpStartupInfo->cbReserved2, 
		lpStartupInfo->lpReserved2, 
		lpStartupInfo->hStdInput, 
		lpStartupInfo->hStdOutput, 
		lpStartupInfo->hStdError
	};

	return CreateProcessW( UTF8_C_MAP(lpApplicationName), (LPWSTR)UTF8_C_MAP(lpCommandLine),lpProcessAttributes,lpThreadAttributes,bInheritHandles,dwCreationFlags,lpEnvironment, UTF8_C_MAP(lpCurrentDirectory),&wStartupInfo,lpProcessInformation );
}

VOID WINAPI FatalAppExitU(
 IN UINT uAction,
 IN LPCSTR lpMessageText
 )
{
return FatalAppExitW(uAction, UTF8_C_MAP(lpMessageText) );
}

VOID WINAPI GetStartupInfoU(
 OUT LPSTARTUPINFOA lpStartupInfo
 )
{
	STARTUPINFOW wStartupInfo = { sizeof(STARTUPINFOW) };
	GetStartupInfoW(&wStartupInfo );

	UTF8_REVERSE_STRING(lpReserved,wStartupInfo.lpReserved);
	UTF8_REVERSE_STRING(lpDesktop,wStartupInfo.lpDesktop);
	UTF8_REVERSE_STRING(lpTitle,wStartupInfo.lpTitle);

	lpStartupInfo->cb = sizeof(STARTUPINFOA);
	lpStartupInfo->lpReserved = lpReserved;
	lpStartupInfo->lpDesktop = lpDesktop;
	lpStartupInfo->lpTitle = lpTitle;
	lpStartupInfo->dwX = wStartupInfo.dwX;
	lpStartupInfo->dwY = wStartupInfo.dwY;
	lpStartupInfo->dwXSize = wStartupInfo.dwXSize;
	lpStartupInfo->dwYSize = wStartupInfo.dwYSize;
	lpStartupInfo->dwXCountChars = wStartupInfo.dwXCountChars;
	lpStartupInfo->dwYCountChars = wStartupInfo.dwYCountChars;
	lpStartupInfo->dwFillAttribute = wStartupInfo.dwFillAttribute;
	lpStartupInfo->dwFlags = wStartupInfo.dwFlags;
	lpStartupInfo->wShowWindow = wStartupInfo.wShowWindow;
	lpStartupInfo->cbReserved2 = wStartupInfo.cbReserved2;
	lpStartupInfo->lpReserved2 = wStartupInfo.lpReserved2;
	lpStartupInfo->hStdInput = wStartupInfo.hStdInput;
	lpStartupInfo->hStdOutput = wStartupInfo.hStdOutput;
	lpStartupInfo->hStdError = wStartupInfo.hStdError;
}

void WINAPI GetCommandLineU( OUT LPSTR lpCmdLine, UINT nCmdLine
 )
{
	strncpy(lpCmdLine,UTF8_REVERSE_C_MAP(GetCommandLineW()),nCmdLine);
}

DWORD WINAPI GetEnvironmentVariableU(
 IN LPCSTR lpName,
 OUT LPSTR lpBuffer,
 IN DWORD nSize
 )
{
return GetEnvironmentVariableW( UTF8_C_MAP(lpName), UTF8_MAP(lpBuffer,nSize),nSize );
}

BOOL WINAPI SetEnvironmentVariableU(
 IN LPCSTR lpName,
 IN LPCSTR lpValue
 )
{
return SetEnvironmentVariableW( UTF8_C_MAP(lpName), UTF8_C_MAP(lpValue) );
}

DWORD WINAPI ExpandEnvironmentStringsU(
 IN LPCSTR lpSrc,
 OUT LPSTR lpDst,
 IN DWORD nSize
 )
{
return ExpandEnvironmentStringsW( UTF8_C_MAP(lpSrc), UTF8_MAP(lpDst,nSize),nSize );
}

DWORD WINAPI GetFirmwareEnvironmentVariableU(
 IN LPCSTR lpName,
 IN LPCSTR lpGuid,
 OUT PVOID pBuffer,
 IN DWORD nSize
 )
{
return GetFirmwareEnvironmentVariableW( UTF8_C_MAP(lpName), UTF8_C_MAP(lpGuid),pBuffer,nSize );
}

BOOL WINAPI SetFirmwareEnvironmentVariableU(
 IN LPCSTR lpName,
 IN LPCSTR lpGuid,
 IN PVOID pValue,
 IN DWORD nSize
 )
{
return SetFirmwareEnvironmentVariableW( UTF8_C_MAP(lpName), UTF8_C_MAP(lpGuid),pValue,nSize );
}

VOID WINAPI OutputDebugStringU(
 IN LPCSTR lpOutputString
 )
{
return OutputDebugStringW( UTF8_C_MAP(lpOutputString) );
}

HRSRC WINAPI FindResourceU(
 IN HMODULE hModule,
 IN LPCSTR lpName,
 IN LPCSTR lpType
 )
{
return FindResourceW(hModule, UTF8_C_MAP(lpName), UTF8_C_MAP(lpType) );
}

HRSRC WINAPI FindResourceExU(
 IN HMODULE hModule,
 IN LPCSTR lpType,
 IN LPCSTR lpName,
 IN WORD wLanguage
 )
{
return FindResourceExW(hModule, UTF8_C_MAP(lpType), UTF8_C_MAP(lpName),wLanguage );
}

BOOL WINAPI EnumResourceTypesU(
 IN HMODULE hModule,
 IN ENUMRESTYPEPROCW lpEnumFunc,
 IN LONG_PTR lParam
 )
{
return EnumResourceTypesW(hModule,lpEnumFunc,lParam );
}

BOOL WINAPI EnumResourceNamesU(
 IN HMODULE hModule,
 IN LPCSTR lpType,
 IN ENUMRESNAMEPROCW lpEnumFunc,
 IN LONG_PTR lParam
 )
{
return EnumResourceNamesW(hModule, UTF8_C_MAP(lpType),lpEnumFunc,lParam );
}

BOOL WINAPI EnumResourceLanguagesU(
 IN HMODULE hModule,
 IN LPCSTR lpType,
 IN LPCSTR lpName,
 IN ENUMRESLANGPROCW lpEnumFunc,
 IN LONG_PTR lParam
 )
{
return EnumResourceLanguagesW(hModule, UTF8_C_MAP(lpType), UTF8_C_MAP(lpName),lpEnumFunc,lParam );
}

HANDLE WINAPI BeginUpdateResourceU(
 IN LPCSTR pFileName,
 IN BOOL bDeleteExistingResources
 )
{
return BeginUpdateResourceW( UTF8_C_MAP(pFileName),bDeleteExistingResources );
}

BOOL WINAPI UpdateResourceU(
 IN HANDLE hUpdate,
 IN LPCSTR lpType,
 IN LPCSTR lpName,
 IN WORD wLanguage,
 IN LPVOID lpData,
 IN DWORD cbData
 )
{
return UpdateResourceW(hUpdate, UTF8_C_MAP(lpType), UTF8_C_MAP(lpName),wLanguage,lpData,cbData );
}

BOOL WINAPI EndUpdateResourceU(
 IN HANDLE hUpdate,
 IN BOOL fDiscard
 )
{
return EndUpdateResourceW(hUpdate,fDiscard );
}

ATOM WINAPI GlobalAddAtomU(
 IN LPCSTR lpString
 )
{
return GlobalAddAtomW( UTF8_C_MAP(lpString) );
}

ATOM WINAPI GlobalFindAtomU(
 IN LPCSTR lpString
 )
{
return GlobalFindAtomW( UTF8_C_MAP(lpString) );
}

UINT WINAPI GlobalGetAtomNameU(
 IN ATOM nAtom,
 OUT LPSTR lpBuffer,
 IN int nSize
 )
{
return GlobalGetAtomNameW(nAtom, UTF8_MAP(lpBuffer,nSize),nSize );
}

ATOM WINAPI AddAtomU(
 IN LPCSTR lpString
 )
{
return AddAtomW( UTF8_C_MAP(lpString) );
}

ATOM WINAPI FindAtomU(
 IN LPCSTR lpString
 )
{
return FindAtomW( UTF8_C_MAP(lpString) );
}

UINT WINAPI GetAtomNameU(
 IN ATOM nAtom,
 OUT LPSTR lpBuffer,
 IN int nSize
 )
{
return GetAtomNameW(nAtom, UTF8_MAP(lpBuffer,nSize),nSize );
}

UINT WINAPI GetProfileIntU(
 IN LPCSTR lpAppName,
 IN LPCSTR lpKeyName,
 IN INT nDefault
 )
{
return GetProfileIntW( UTF8_C_MAP(lpAppName), UTF8_C_MAP(lpKeyName),nDefault );
}

DWORD WINAPI GetProfileStringU(
 IN LPCSTR lpAppName,
 IN LPCSTR lpKeyName,
 IN LPCSTR lpDefault,
 OUT LPSTR lpReturnedString,
 IN DWORD nSize
 )
{
return GetProfileStringW( UTF8_C_MAP(lpAppName), UTF8_C_MAP(lpKeyName), UTF8_C_MAP(lpDefault), UTF8_MAP(lpReturnedString,nSize),nSize );
}

BOOL WINAPI WriteProfileStringU(
 IN LPCSTR lpAppName,
 IN LPCSTR lpKeyName,
 IN LPCSTR lpString
 )
{
return WriteProfileStringW( UTF8_C_MAP(lpAppName), UTF8_C_MAP(lpKeyName), UTF8_C_MAP(lpString) );
}

DWORD WINAPI GetProfileSectionU(
 IN LPCSTR lpAppName,
 OUT LPSTR lpReturnedString,
 IN DWORD nSize
 )
{
return GetProfileSectionW( UTF8_C_MAP(lpAppName), UTF8_MAP(lpReturnedString,nSize),nSize );
}

BOOL WINAPI WriteProfileSectionU(
 IN LPCSTR lpAppName,
 IN LPCSTR lpString
 )
{
return WriteProfileSectionW( UTF8_C_MAP(lpAppName), UTF8_C_MAP(lpString) );
}

UINT WINAPI GetPrivateProfileIntU(
 IN LPCSTR lpAppName,
 IN LPCSTR lpKeyName,
 IN INT nDefault,
 IN LPCSTR lpFileName
 )
{
return GetPrivateProfileIntW( UTF8_C_MAP(lpAppName), UTF8_C_MAP(lpKeyName),nDefault, UTF8_C_MAP(lpFileName) );
}

DWORD WINAPI GetPrivateProfileStringU(
 IN LPCSTR lpAppName,
 IN LPCSTR lpKeyName,
 IN LPCSTR lpDefault,
 OUT LPSTR lpReturnedString,
 IN DWORD nSize,
 IN LPCSTR lpFileName
 )
{
return GetPrivateProfileStringW( UTF8_C_MAP(lpAppName), UTF8_C_MAP(lpKeyName), UTF8_C_MAP(lpDefault), UTF8_MAP(lpReturnedString,nSize),nSize, UTF8_C_MAP(lpFileName) );
}

BOOL WINAPI WritePrivateProfileStringU(
 IN LPCSTR lpAppName,
 IN LPCSTR lpKeyName,
 IN LPCSTR lpString,
 IN LPCSTR lpFileName
 )
{
return WritePrivateProfileStringW( UTF8_C_MAP(lpAppName), UTF8_C_MAP(lpKeyName), UTF8_C_MAP(lpString), UTF8_C_MAP(lpFileName) );
}

DWORD WINAPI GetPrivateProfileSectionU(
 IN LPCSTR lpAppName,
 OUT LPSTR lpReturnedString,
 IN DWORD nSize,
 IN LPCSTR lpFileName
 )
{
return GetPrivateProfileSectionW( UTF8_C_MAP(lpAppName), UTF8_MAP(lpReturnedString,nSize),nSize, UTF8_C_MAP(lpFileName) );
}

BOOL WINAPI WritePrivateProfileSectionU(
 IN LPCSTR lpAppName,
 IN LPCSTR lpString,
 IN LPCSTR lpFileName
 )
{
return WritePrivateProfileSectionW( UTF8_C_MAP(lpAppName), UTF8_C_MAP(lpString), UTF8_C_MAP(lpFileName) );
}

DWORD WINAPI GetPrivateProfileSectionNamesU(
 OUT LPSTR lpszReturnBuffer,
 IN DWORD nSize,
 IN LPCSTR lpFileName
 )
{
return GetPrivateProfileSectionNamesW( UTF8_MAP(lpszReturnBuffer,nSize),nSize, UTF8_C_MAP(lpFileName) );
}

BOOL WINAPI GetPrivateProfileStructU(
 IN LPCSTR lpszSection,
 IN LPCSTR lpszKey,
 OUT LPVOID lpStruct,
 IN UINT uSizeStruct,
 IN LPCSTR szFile
 )
{
return GetPrivateProfileStructW( UTF8_C_MAP(lpszSection), UTF8_C_MAP(lpszKey),lpStruct,uSizeStruct, UTF8_C_MAP(szFile) );
}

BOOL WINAPI WritePrivateProfileStructU(
 IN LPCSTR lpszSection,
 IN LPCSTR lpszKey,
 IN LPVOID lpStruct,
 IN UINT uSizeStruct,
 IN LPCSTR szFile
 )
{
return WritePrivateProfileStructW( UTF8_C_MAP(lpszSection), UTF8_C_MAP(lpszKey),lpStruct,uSizeStruct, UTF8_C_MAP(szFile) );
}

UINT WINAPI GetDriveTypeU(
 IN LPCSTR lpRootPathName
 )
{
return GetDriveTypeW( UTF8_C_MAP(lpRootPathName) );
}

UINT WINAPI GetSystemDirectoryU(
 OUT LPSTR lpBuffer,
 IN UINT uSize
 )
{
return GetSystemDirectoryW( UTF8_MAP(lpBuffer,uSize),uSize );
}

DWORD WINAPI GetTempPathU(
 IN DWORD nBufferLength,
 OUT LPSTR lpBuffer
 )
{
return GetTempPathW(nBufferLength, UTF8_MAP(lpBuffer,nBufferLength) );
}

UINT WINAPI GetTempFileNameU(
 IN LPCSTR lpPathName,
 IN LPCSTR lpPrefixString,
 IN UINT uUnique,
 OUT LPSTR lpTempFileName
 )
{
return GetTempFileNameW( UTF8_C_MAP(lpPathName), UTF8_C_MAP(lpPrefixString),uUnique, UTF8_MAP(lpTempFileName,MAX_PATH) );
}

UINT WINAPI GetWindowsDirectoryU(
 OUT LPSTR lpBuffer,
 IN UINT uSize
 )
{
return GetWindowsDirectoryW( UTF8_MAP(lpBuffer,uSize),uSize );
}

UINT WINAPI GetSystemWindowsDirectoryU(
 OUT LPSTR lpBuffer,
 IN UINT uSize
 )
{
return GetSystemWindowsDirectoryW( UTF8_MAP(lpBuffer,uSize),uSize );
}

/*
UINT WINAPI GetSystemWow64DirectoryU(
 OUT LPSTR lpBuffer,
 IN UINT uSize
 )
{
return GetSystemWow64DirectoryW( UTF8_MAP(lpBuffer),uSize );
}
*/
BOOL WINAPI SetCurrentDirectoryU(
 IN LPCSTR lpPathName
 )
{
return SetCurrentDirectoryW( UTF8_C_MAP(lpPathName) );
}

DWORD WINAPI GetCurrentDirectoryU(
 IN DWORD nBufferLength,
 OUT LPSTR lpBuffer
 )
{
return GetCurrentDirectoryW(nBufferLength, UTF8_MAP(lpBuffer,nBufferLength) );
}

BOOL WINAPI GetDiskFreeSpaceU(
 IN LPCSTR lpRootPathName,
 OUT LPDWORD lpSectorsPerCluster,
 OUT LPDWORD lpBytesPerSector,
 OUT LPDWORD lpNumberOfFreeClusters,
 OUT LPDWORD lpTotalNumberOfClusters
 )
{
return GetDiskFreeSpaceW( UTF8_C_MAP(lpRootPathName),lpSectorsPerCluster,lpBytesPerSector,lpNumberOfFreeClusters,lpTotalNumberOfClusters );
}

BOOL WINAPI GetDiskFreeSpaceExU(
 IN LPCSTR lpDirectoryName,
 OUT PULARGE_INTEGER lpFreeBytesAvailableToCaller,
 OUT PULARGE_INTEGER lpTotalNumberOfBytes,
 OUT PULARGE_INTEGER lpTotalNumberOfFreeBytes
 )
{
return GetDiskFreeSpaceExW( UTF8_C_MAP(lpDirectoryName),lpFreeBytesAvailableToCaller,lpTotalNumberOfBytes,lpTotalNumberOfFreeBytes );
}

BOOL WINAPI CreateDirectoryU(
 IN LPCSTR lpPathName,
 IN LPSECURITY_ATTRIBUTES lpSecurityAttributes
 )
{
return CreateDirectoryW( UTF8_C_MAP(lpPathName),lpSecurityAttributes );
}

BOOL WINAPI CreateDirectoryExU(
 IN LPCSTR lpTemplateDirectory,
 IN LPCSTR lpNewDirectory,
 IN LPSECURITY_ATTRIBUTES lpSecurityAttributes
 )
{
return CreateDirectoryExW( UTF8_C_MAP(lpTemplateDirectory), UTF8_C_MAP(lpNewDirectory),lpSecurityAttributes );
}

BOOL WINAPI RemoveDirectoryU(
 IN LPCSTR lpPathName
 )
{
return RemoveDirectoryW( UTF8_C_MAP(lpPathName) );
}

DWORD WINAPI GetFullPathNameU(
 IN LPCSTR lpFileName,
 IN DWORD nBufferLength,
 OUT LPSTR lpBuffer,
 OUT LPSTR *lpFilePart
 )
{
	LPWSTR lpwFilePart;
	UTF8_STRING(lpwBuffer,lpBuffer);

	DWORD dwRet = GetFullPathNameW( UTF8_C_MAP(lpFileName),nBufferLength, lpwBuffer, &lpwFilePart );
	if(lpFilePart)
		*lpFilePart = lpBuffer + (lpwFilePart - lpwBuffer);
	return dwRet;
}

BOOL WINAPI DefineDosDeviceU(
 IN DWORD dwFlags,
 IN LPCSTR lpDeviceName,
 IN LPCSTR lpTargetPath
 )
{
return DefineDosDeviceW(dwFlags, UTF8_C_MAP(lpDeviceName), UTF8_C_MAP(lpTargetPath) );
}

DWORD WINAPI QueryDosDeviceU(
 IN LPCSTR lpDeviceName,
 OUT LPSTR lpTargetPath,
 IN DWORD ucchMax
 )
{
return QueryDosDeviceW( UTF8_C_MAP(lpDeviceName), UTF8_MAP(lpTargetPath,ucchMax),ucchMax );
}

HANDLE WINAPI CreateFileU(
 IN LPCSTR lpFileName,
 IN DWORD dwDesiredAccess,
 IN DWORD dwShareMode,
 IN LPSECURITY_ATTRIBUTES lpSecurityAttributes,
 IN DWORD dwCreationDisposition,
 IN DWORD dwFlagsAndAttributes,
 IN HANDLE hTemplateFile
 )
{
return CreateFileW( UTF8_C_MAP(lpFileName),dwDesiredAccess,dwShareMode,lpSecurityAttributes,dwCreationDisposition,dwFlagsAndAttributes,hTemplateFile );
}

BOOL WINAPI SetFileAttributesU(
 IN LPCSTR lpFileName,
 IN DWORD dwFileAttributes
 )
{
return SetFileAttributesW( UTF8_C_MAP(lpFileName),dwFileAttributes );
}

DWORD WINAPI GetFileAttributesU(
 IN LPCSTR lpFileName
 )
{
return GetFileAttributesW( UTF8_C_MAP(lpFileName) );
}

BOOL WINAPI GetFileAttributesExU(
 IN LPCSTR lpFileName,
 IN GET_FILEEX_INFO_LEVELS fInfoLevelId,
 OUT LPVOID lpFileInformation
 )
{
return GetFileAttributesExW( UTF8_C_MAP(lpFileName),fInfoLevelId,lpFileInformation );
}

DWORD WINAPI GetCompressedFileSizeU(
 IN LPCSTR lpFileName,
 OUT LPDWORD lpFileSizeHigh
 )
{
return GetCompressedFileSizeW( UTF8_C_MAP(lpFileName),lpFileSizeHigh );
}

BOOL WINAPI DeleteFileU(
 IN LPCSTR lpFileName
 )
{
return DeleteFileW( UTF8_C_MAP(lpFileName) );
}

HANDLE WINAPI FindFirstFileExU(
 IN LPCSTR lpFileName,
 IN FINDEX_INFO_LEVELS fInfoLevelId,
 OUT LPVOID lpFindFileData,
 IN FINDEX_SEARCH_OPS fSearchOp,
 IN LPVOID lpSearchFilter,
 IN DWORD dwAdditionalFlags
 )
{
	WIN32_FIND_DATAW wFindData = { 0 };
	LPWIN32_FIND_DATAA lpFindData = (LPWIN32_FIND_DATAA)lpFindFileData;

	HANDLE hRet = FindFirstFileExW( UTF8_C_MAP(lpFileName),fInfoLevelId,&wFindData,fSearchOp,lpSearchFilter,dwAdditionalFlags );
	lpFindData->dwFileAttributes = wFindData.dwFileAttributes;
	lpFindData->ftCreationTime = wFindData.ftCreationTime;
	lpFindData->ftLastAccessTime = wFindData.ftLastAccessTime;
	lpFindData->ftLastWriteTime = wFindData.ftLastWriteTime;
	lpFindData->nFileSizeHigh = wFindData.nFileSizeHigh;
	lpFindData->nFileSizeLow = wFindData.nFileSizeLow;
	lpFindData->dwReserved0 = wFindData.dwReserved0;
	lpFindData->dwReserved1 = wFindData.dwReserved1;
	strncpy(lpFindData->cFileName, UTF8_REVERSE_C_MAP(wFindData.cFileName), sizeof(lpFindData->cFileName));
	strncpy(lpFindData->cAlternateFileName, UTF8_REVERSE_C_MAP(wFindData.cAlternateFileName), sizeof(lpFindData->cAlternateFileName));
	return hRet;
}

HANDLE WINAPI FindFirstFileU(
 IN LPCSTR lpFileName,
 OUT LPWIN32_FIND_DATAA lpFindFileData
 )
{
	WIN32_FIND_DATAW wFindData = { 0 };
	HANDLE hRet = FindFirstFileW( UTF8_C_MAP(lpFileName),&wFindData );
	lpFindFileData->dwFileAttributes = wFindData.dwFileAttributes;
	lpFindFileData->ftCreationTime = wFindData.ftCreationTime;
	lpFindFileData->ftLastAccessTime = wFindData.ftLastAccessTime;
	lpFindFileData->ftLastWriteTime = wFindData.ftLastWriteTime;
	lpFindFileData->nFileSizeHigh = wFindData.nFileSizeHigh;
	lpFindFileData->nFileSizeLow = wFindData.nFileSizeLow;
	lpFindFileData->dwReserved0 = wFindData.dwReserved0;
	lpFindFileData->dwReserved1 = wFindData.dwReserved1;
	strncpy(lpFindFileData->cFileName, UTF8_REVERSE_C_MAP(wFindData.cFileName), sizeof(lpFindFileData->cFileName));
	strncpy(lpFindFileData->cAlternateFileName, UTF8_REVERSE_C_MAP(wFindData.cAlternateFileName), sizeof(lpFindFileData->cAlternateFileName));
	return hRet;
}

BOOL WINAPI FindNextFileU(
 IN HANDLE hFindFile,
 OUT LPWIN32_FIND_DATAA lpFindFileData
 )
{
	WIN32_FIND_DATAW wFindData = { 0 };
	BOOL bRet = FindNextFileW( hFindFile,&wFindData );
	lpFindFileData->dwFileAttributes = wFindData.dwFileAttributes;
	lpFindFileData->ftCreationTime = wFindData.ftCreationTime;
	lpFindFileData->ftLastAccessTime = wFindData.ftLastAccessTime;
	lpFindFileData->ftLastWriteTime = wFindData.ftLastWriteTime;
	lpFindFileData->nFileSizeHigh = wFindData.nFileSizeHigh;
	lpFindFileData->nFileSizeLow = wFindData.nFileSizeLow;
	lpFindFileData->dwReserved0 = wFindData.dwReserved0;
	lpFindFileData->dwReserved1 = wFindData.dwReserved1;
	strncpy(lpFindFileData->cFileName, UTF8_REVERSE_C_MAP(wFindData.cFileName), sizeof(lpFindFileData->cFileName));
	strncpy(lpFindFileData->cAlternateFileName, UTF8_REVERSE_C_MAP(wFindData.cAlternateFileName), sizeof(lpFindFileData->cAlternateFileName));
	return bRet;
}

DWORD WINAPI SearchPathU(
 IN LPCSTR lpPath,
 IN LPCSTR lpFileName,
 IN LPCSTR lpExtension,
 IN DWORD nBufferLength,
 OUT LPSTR lpBuffer,
 OUT LPSTR *lpFilePart
 )
{
	UTF8_STRING(lpwBuffer,lpBuffer);
	LPWSTR lpwFilePart;
	DWORD dwRet = SearchPathW( UTF8_C_MAP(lpPath), UTF8_C_MAP(lpFileName), UTF8_C_MAP(lpExtension),nBufferLength, lpwBuffer, &lpwFilePart );
	if(lpFilePart)
		*lpFilePart=lpBuffer + (lpwFilePart - lpwBuffer);
	return dwRet;
}

BOOL WINAPI CopyFileU(
 IN LPCSTR lpExistingFileName,
 IN LPCSTR lpNewFileName,
 IN BOOL bFailIfExists
 )
{
return CopyFileW( UTF8_C_MAP(lpExistingFileName), UTF8_C_MAP(lpNewFileName),bFailIfExists );
}

BOOL WINAPI CopyFileExU(
 IN LPCSTR lpExistingFileName,
 IN LPCSTR lpNewFileName,
 IN LPPROGRESS_ROUTINE lpProgressRoutine OPTIONAL,
 IN LPVOID lpData OPTIONAL,
 IN LPBOOL pbCancel OPTIONAL,
 IN DWORD dwCopyFlags
 )
{
return CopyFileExW( UTF8_C_MAP(lpExistingFileName), UTF8_C_MAP(lpNewFileName),lpProgressRoutine,lpData,pbCancel,dwCopyFlags );
}

BOOL WINAPI MoveFileU(
 IN LPCSTR lpExistingFileName,
 IN LPCSTR lpNewFileName
 )
{
return MoveFileW( UTF8_C_MAP(lpExistingFileName), UTF8_C_MAP(lpNewFileName) );
}

BOOL WINAPI MoveFileExU(
 IN LPCSTR lpExistingFileName,
 IN LPCSTR lpNewFileName,
 IN DWORD dwFlags
 )
{
return MoveFileExW( UTF8_C_MAP(lpExistingFileName), UTF8_C_MAP(lpNewFileName),dwFlags );
}
/*
BOOL WINAPI MoveFileWithProgressU(
 IN LPCSTR lpExistingFileName,
 IN LPCSTR lpNewFileName,
 IN LPPROGRESS_ROUTINE lpProgressRoutine OPTIONAL,
 IN LPVOID lpData OPTIONAL,
 IN DWORD dwFlags
 )
{
return MoveFileWithProgressW( UTF8_C_MAP(lpExistingFileName), UTF8_C_MAP(lpNewFileName),lpProgressRoutine,lpData,dwFlags );
}


BOOL WINAPI ReplaceFileU(
 LPCSTR lpReplacedFileName,
 LPCSTR lpReplacementFileName,
 LPCSTR lpBackupFileName,
 DWORD dwReplaceFlags,
 LPVOID lpExclude,
 LPVOID lpReserved
 )
{
return ReplaceFileW( UTF8_C_MAP(lpReplacedFileName), UTF8_C_MAP(lpReplacementFileName), UTF8_C_MAP(lpBackupFileName),dwReplaceFlags,lpExclude,lpReserved );
}

BOOL WINAPI CreateHardLinkU(
 IN LPCSTR lpFileName,
 IN LPCSTR lpExistingFileName,
 IN LPSECURITY_ATTRIBUTES lpSecurityAttributes
 )
{
return CreateHardLinkW( UTF8_C_MAP(lpFileName), UTF8_C_MAP(lpExistingFileName),lpSecurityAttributes );
}
*/

HANDLE WINAPI CreateNamedPipeU(
 IN LPCSTR lpName,
 IN DWORD dwOpenMode,
 IN DWORD dwPipeMode,
 IN DWORD nMaxInstances,
 IN DWORD nOutBufferSize,
 IN DWORD nInBufferSize,
 IN DWORD nDefaultTimeOut,
 IN LPSECURITY_ATTRIBUTES lpSecurityAttributes
 )
{
return CreateNamedPipeW( UTF8_C_MAP(lpName),dwOpenMode,dwPipeMode,nMaxInstances,nOutBufferSize,nInBufferSize,nDefaultTimeOut,lpSecurityAttributes );
}

BOOL WINAPI GetNamedPipeHandleStateU(
 IN HANDLE hNamedPipe,
 OUT LPDWORD lpState,
 OUT LPDWORD lpCurInstances,
 OUT LPDWORD lpMaxCollectionCount,
 OUT LPDWORD lpCollectDataTimeout,
 OUT LPSTR lpUserName,
 IN DWORD nMaxUserNameSize
 )
{
return GetNamedPipeHandleStateW(hNamedPipe,lpState,lpCurInstances,lpMaxCollectionCount,lpCollectDataTimeout, UTF8_MAP(lpUserName,nMaxUserNameSize),nMaxUserNameSize );
}

BOOL WINAPI CallNamedPipeU(
 IN LPCSTR lpNamedPipeName,
 IN LPVOID lpInBuffer,
 IN DWORD nInBufferSize,
 OUT LPVOID lpOutBuffer,
 IN DWORD nOutBufferSize,
 OUT LPDWORD lpBytesRead,
 IN DWORD nTimeOut
 )
{
return CallNamedPipeW( UTF8_C_MAP(lpNamedPipeName),lpInBuffer,nInBufferSize,lpOutBuffer,nOutBufferSize,lpBytesRead,nTimeOut );
}

BOOL WINAPI WaitNamedPipeU(
 IN LPCSTR lpNamedPipeName,
 IN DWORD nTimeOut
 )
{
return WaitNamedPipeW( UTF8_C_MAP(lpNamedPipeName),nTimeOut );
}

BOOL WINAPI SetVolumeLabelU(
 IN LPCSTR lpRootPathName,
 IN LPCSTR lpVolumeName
 )
{
return SetVolumeLabelW( UTF8_C_MAP(lpRootPathName), UTF8_C_MAP(lpVolumeName) );
}

BOOL WINAPI GetVolumeInformationU(
 IN LPCSTR lpRootPathName,
 OUT LPSTR lpVolumeNameBuffer,
 IN DWORD nVolumeNameSize,
 OUT LPDWORD lpVolumeSerialNumber,
 OUT LPDWORD lpMaximumComponentLength,
 OUT LPDWORD lpFileSystemFlags,
 OUT LPSTR lpFileSystemNameBuffer,
 IN DWORD nFileSystemNameSize
 )
{
return GetVolumeInformationW( UTF8_C_MAP(lpRootPathName), UTF8_MAP(lpVolumeNameBuffer,nVolumeNameSize),nVolumeNameSize,lpVolumeSerialNumber,lpMaximumComponentLength,lpFileSystemFlags, UTF8_MAP(lpFileSystemNameBuffer,nFileSystemNameSize),nFileSystemNameSize );
}

BOOL WINAPI ClearEventLogU(
 IN HANDLE hEventLog,
 IN LPCSTR lpBackupFileName
 )
{
return ClearEventLogW(hEventLog, UTF8_C_MAP(lpBackupFileName) );
}

BOOL WINAPI BackupEventLogU(
 IN HANDLE hEventLog,
 IN LPCSTR lpBackupFileName
 )
{
return BackupEventLogW(hEventLog, UTF8_C_MAP(lpBackupFileName) );
}

HANDLE WINAPI OpenEventLogU(
 IN LPCSTR lpUNCServerName,
 IN LPCSTR lpSourceName
 )
{
return OpenEventLogW( UTF8_C_MAP(lpUNCServerName), UTF8_C_MAP(lpSourceName) );
}

HANDLE WINAPI RegisterEventSourceU(
 IN LPCSTR lpUNCServerName,
 IN LPCSTR lpSourceName
 )
{
return RegisterEventSourceW( UTF8_C_MAP(lpUNCServerName), UTF8_C_MAP(lpSourceName) );
}

HANDLE WINAPI OpenBackupEventLogU(
 IN LPCSTR lpUNCServerName,
 IN LPCSTR lpFileName
 )
{
return OpenBackupEventLogW( UTF8_C_MAP(lpUNCServerName), UTF8_C_MAP(lpFileName) );
}

BOOL WINAPI ReadEventLogU(
 IN HANDLE hEventLog,
 IN DWORD dwReadFlags,
 IN DWORD dwRecordOffset,
 OUT LPVOID lpBuffer,
 IN DWORD nNumberOfBytesToRead,
 OUT DWORD *pnBytesRead,
 OUT DWORD *pnMinNumberOfBytesNeeded
 )
{
return ReadEventLogW(hEventLog,dwReadFlags,dwRecordOffset,lpBuffer,nNumberOfBytesToRead,pnBytesRead,pnMinNumberOfBytesNeeded );
}

BOOL WINAPI ReportEventU(
 IN HANDLE hEventLog,
 IN WORD wType,
 IN WORD wCategory,
 IN DWORD dwEventID,
 IN PSID lpUserSid,
 IN WORD wNumStrings,
 IN DWORD dwDataSize,
 IN LPCSTR *lpStrings,
 IN LPVOID lpRawData
 )
{
	LPCWSTR *lpwStrings;
	if(wNumStrings)
	{
		lpwStrings = (LPCWSTR*)malloc(sizeof(LPCWSTR)*wNumStrings);
		for(WORD w=0; w<wNumStrings; w++)
			lpwStrings[w]=UTF8_ALLOC_STRING(lpStrings[w]);
	}
	BOOL bRet = ReportEventW(hEventLog,wType,wCategory,dwEventID,lpUserSid,wNumStrings,dwDataSize, lpwStrings,lpRawData );
	if(wNumStrings)
	{
		for(WORD w=0; w<wNumStrings; w++)
			free((void*)lpwStrings[w]);
		free(lpwStrings);
	}
	return bRet;
}

BOOL WINAPI AccessCheckAndAuditAlarmU(
 IN LPCSTR SubsystemName,
 IN LPVOID HandleId,
 IN LPSTR ObjectTypeName,
 IN LPSTR ObjectName,
 IN PSECURITY_DESCRIPTOR SecurityDescriptor,
 IN DWORD DesiredAccess,
 IN PGENERIC_MAPPING GenericMapping,
 IN BOOL ObjectCreation,
 OUT LPDWORD GrantedAccess,
 OUT LPBOOL AccessStatus,
 OUT LPBOOL pfGenerateOnClose
 )
{
return AccessCheckAndAuditAlarmW( UTF8_C_MAP(SubsystemName),HandleId, (LPWSTR)UTF8_C_MAP(ObjectTypeName), (LPWSTR)UTF8_C_MAP(ObjectName),SecurityDescriptor,DesiredAccess,GenericMapping,ObjectCreation,GrantedAccess,AccessStatus,pfGenerateOnClose );
}
/*
BOOL WINAPI AccessCheckByTypeAndAuditAlarmU(
 IN LPCSTR SubsystemName,
 IN LPVOID HandleId,
 IN LPCSTR ObjectTypeName,
 IN LPCSTR ObjectName,
 IN PSECURITY_DESCRIPTOR SecurityDescriptor,
 IN PSID PrincipalSelfSid,
 IN DWORD DesiredAccess,
 IN AUDIT_EVENT_TYPE AuditType,
 IN DWORD Flags,
 IN POBJECT_TYPE_LIST ObjectTypeList,
 IN DWORD ObjectTypeListLength,
 IN PGENERIC_MAPPING GenericMapping,
 IN BOOL ObjectCreation,
 OUT LPDWORD GrantedAccess,
 OUT LPBOOL AccessStatus,
 OUT LPBOOL pfGenerateOnClose
 )
{
return AccessCheckByTypeAndAuditAlarmW( UTF8_C_MAP(SubsystemName),HandleId, UTF8_C_MAP(ObjectTypeName), UTF8_C_MAP(ObjectName),SecurityDescriptor,PrincipalSelfSid,DesiredAccess,AuditType,Flags,ObjectTypeList,ObjectTypeListLength,GenericMapping,ObjectCreation,GrantedAccess,AccessStatus,pfGenerateOnClose );
}
*/
/*
BOOL WINAPI AccessCheckByTypeResultListAndAuditAlarmU(
 IN LPCSTR SubsystemName,
 IN LPVOID HandleId,
 IN LPCSTR ObjectTypeName,
 IN LPCSTR ObjectName,
 IN PSECURITY_DESCRIPTOR SecurityDescriptor,
 IN PSID PrincipalSelfSid,
 IN DWORD DesiredAccess,
 IN AUDIT_EVENT_TYPE AuditType,
 IN DWORD Flags,
 IN POBJECT_TYPE_LIST ObjectTypeList,
 IN DWORD ObjectTypeListLength,
 IN PGENERIC_MAPPING GenericMapping,
 IN BOOL ObjectCreation,
 OUT LPDWORD GrantedAccess,
 OUT LPDWORD AccessStatusList,
 OUT LPBOOL pfGenerateOnClose
 )
{
return AccessCheckByTypeResultListAndAuditAlarmW( UTF8_C_MAP(SubsystemName),HandleId, UTF8_C_MAP(ObjectTypeName), UTF8_C_MAP(ObjectName),SecurityDescriptor,PrincipalSelfSid,DesiredAccess,AuditType,Flags,ObjectTypeList,ObjectTypeListLength,GenericMapping,ObjectCreation,GrantedAccess,AccessStatusList,pfGenerateOnClose );
}
*/
/*
BOOL WINAPI AccessCheckByTypeResultListAndAuditAlarmByHandleU(
 IN LPCSTR SubsystemName,
 IN LPVOID HandleId,
 IN HANDLE ClientToken,
 IN LPCSTR ObjectTypeName,
 IN LPCSTR ObjectName,
 IN PSECURITY_DESCRIPTOR SecurityDescriptor,
 IN PSID PrincipalSelfSid,
 IN DWORD DesiredAccess,
 IN AUDIT_EVENT_TYPE AuditType,
 IN DWORD Flags,
 IN POBJECT_TYPE_LIST ObjectTypeList,
 IN DWORD ObjectTypeListLength,
 IN PGENERIC_MAPPING GenericMapping,
 IN BOOL ObjectCreation,
 OUT LPDWORD GrantedAccess,
 OUT LPDWORD AccessStatusList,
 OUT LPBOOL pfGenerateOnClose
 )
{
return AccessCheckByTypeResultListAndAuditAlarmByHandleW( UTF8_C_MAP(SubsystemName),HandleId,ClientToken, UTF8_C_MAP(ObjectTypeName), UTF8_C_MAP(ObjectName),SecurityDescriptor,PrincipalSelfSid,DesiredAccess,AuditType,Flags,ObjectTypeList,ObjectTypeListLength,GenericMapping,ObjectCreation,GrantedAccess,AccessStatusList,pfGenerateOnClose );
}
*/
BOOL WINAPI ObjectOpenAuditAlarmU(
 IN LPCSTR SubsystemName,
 IN LPVOID HandleId,
 IN LPSTR ObjectTypeName,
 IN LPSTR ObjectName,
 IN PSECURITY_DESCRIPTOR pSecurityDescriptor,
 IN HANDLE ClientToken,
 IN DWORD DesiredAccess,
 IN DWORD GrantedAccess,
 IN PPRIVILEGE_SET Privileges,
 IN BOOL ObjectCreation,
 IN BOOL AccessGranted,
 OUT LPBOOL GenerateOnClose
 )
{
return ObjectOpenAuditAlarmW( UTF8_C_MAP(SubsystemName),HandleId, (LPWSTR)UTF8_C_MAP(ObjectTypeName), (LPWSTR)UTF8_C_MAP(ObjectName),pSecurityDescriptor,ClientToken,DesiredAccess,GrantedAccess,Privileges,ObjectCreation,AccessGranted,GenerateOnClose );
}

BOOL WINAPI ObjectPrivilegeAuditAlarmU(
 IN LPCSTR SubsystemName,
 IN LPVOID HandleId,
 IN HANDLE ClientToken,
 IN DWORD DesiredAccess,
 IN PPRIVILEGE_SET Privileges,
 IN BOOL AccessGranted
 )
{
return ObjectPrivilegeAuditAlarmW( UTF8_C_MAP(SubsystemName),HandleId,ClientToken,DesiredAccess,Privileges,AccessGranted );
}

BOOL WINAPI ObjectCloseAuditAlarmU(
 IN LPCSTR SubsystemName,
 IN LPVOID HandleId,
 IN BOOL GenerateOnClose
 )
{
return ObjectCloseAuditAlarmW( UTF8_C_MAP(SubsystemName),HandleId,GenerateOnClose );
}

BOOL WINAPI ObjectDeleteAuditAlarmU(
 IN LPCSTR SubsystemName,
 IN LPVOID HandleId,
 IN BOOL GenerateOnClose
 )
{
return ObjectDeleteAuditAlarmW( UTF8_C_MAP(SubsystemName),HandleId,GenerateOnClose );
}

BOOL WINAPI PrivilegedServiceAuditAlarmU(
 IN LPCSTR SubsystemName,
 IN LPCSTR ServiceName,
 IN HANDLE ClientToken,
 IN PPRIVILEGE_SET Privileges,
 IN BOOL AccessGranted
 )
{
return PrivilegedServiceAuditAlarmW( UTF8_C_MAP(SubsystemName), UTF8_C_MAP(ServiceName),ClientToken,Privileges,AccessGranted );
}

BOOL WINAPI SetFileSecurityU(
 IN LPCSTR lpFileName,
 IN SECURITY_INFORMATION SecurityInformation,
 IN PSECURITY_DESCRIPTOR pSecurityDescriptor
 )
{
return SetFileSecurityW( UTF8_C_MAP(lpFileName),SecurityInformation,pSecurityDescriptor );
}

BOOL WINAPI GetFileSecurityU(
 IN LPCSTR lpFileName,
 IN SECURITY_INFORMATION RequestedInformation,
 OUT PSECURITY_DESCRIPTOR pSecurityDescriptor,
 IN DWORD nLength,
 OUT LPDWORD lpnLengthNeeded
 )
{
return GetFileSecurityW( UTF8_C_MAP(lpFileName),RequestedInformation,pSecurityDescriptor,nLength,lpnLengthNeeded );
}

HANDLE WINAPI FindFirstChangeNotificationU(
 IN LPCSTR lpPathName,
 IN BOOL bWatchSubtree,
 IN DWORD dwNotifyFilter
 )
{
return FindFirstChangeNotificationW( UTF8_C_MAP(lpPathName),bWatchSubtree,dwNotifyFilter );
}

BOOL WINAPI IsBadStringPtrU(
 IN LPCSTR lpsz,
 IN UINT_PTR ucchMax
 )
{
return IsBadStringPtrW( UTF8_C_MAP(lpsz),ucchMax );
}

BOOL WINAPI LookupAccountSidU(
 IN LPCSTR lpSystemName,
 IN PSID Sid,
 OUT LPSTR Name,
 IN OUT LPDWORD cbName,
 OUT LPSTR ReferencedDomainName,
 IN OUT LPDWORD cbReferencedDomainName,
 OUT PSID_NAME_USE peUse
 )
{
return LookupAccountSidW( UTF8_C_MAP(lpSystemName),Sid, UTF8_MAP(Name,*cbName),cbName, UTF8_MAP(ReferencedDomainName,*cbReferencedDomainName),cbReferencedDomainName,peUse );
}

BOOL WINAPI LookupAccountNameU(
 IN LPCSTR lpSystemName,
 IN LPCSTR lpAccountName,
 OUT PSID Sid,
 IN OUT LPDWORD cbSid,
 OUT LPSTR ReferencedDomainName,
 IN OUT LPDWORD cbReferencedDomainName,
 OUT PSID_NAME_USE peUse
 )
{
return LookupAccountNameW( UTF8_C_MAP(lpSystemName), UTF8_C_MAP(lpAccountName),Sid,cbSid, UTF8_MAP(ReferencedDomainName,*cbReferencedDomainName),cbReferencedDomainName,peUse );
}

BOOL WINAPI LookupPrivilegeValueU(
 IN LPCSTR lpSystemName,
 IN LPCSTR lpName,
 OUT PLUID lpLuid
 )
{
return LookupPrivilegeValueW( UTF8_C_MAP(lpSystemName), UTF8_C_MAP(lpName),lpLuid );
}

BOOL WINAPI LookupPrivilegeNameU(
 IN LPCSTR lpSystemName,
 IN PLUID lpLuid,
 OUT LPSTR lpName,
 IN OUT LPDWORD cbName
 )
{
return LookupPrivilegeNameW( UTF8_C_MAP(lpSystemName),lpLuid, UTF8_MAP(lpName,*cbName),cbName );
}

BOOL WINAPI LookupPrivilegeDisplayNameU(
 IN LPCSTR lpSystemName,
 IN LPCSTR lpName,
 OUT LPSTR lpDisplayName,
 IN OUT LPDWORD cbDisplayName,
 OUT LPDWORD lpLanguageId
 )
{
return LookupPrivilegeDisplayNameW( UTF8_C_MAP(lpSystemName), UTF8_C_MAP(lpName), UTF8_MAP(lpDisplayName,*cbDisplayName),cbDisplayName,lpLanguageId );
}

BOOL WINAPI BuildCommDCBU(
 IN LPCSTR lpDef,
 OUT LPDCB lpDCB
 )
{
return BuildCommDCBW( UTF8_C_MAP(lpDef),lpDCB );
}

BOOL WINAPI BuildCommDCBAndTimeoutsU(
 IN LPCSTR lpDef,
 OUT LPDCB lpDCB,
 IN LPCOMMTIMEOUTS lpCommTimeouts
 )
{
return BuildCommDCBAndTimeoutsW( UTF8_C_MAP(lpDef),lpDCB,lpCommTimeouts );
}

BOOL WINAPI CommConfigDialogU(
 IN LPCSTR lpszName,
 IN HWND hWnd,
 IN OUT LPCOMMCONFIG lpCC
 )
{
return CommConfigDialogW( UTF8_C_MAP(lpszName),hWnd,lpCC );
}

BOOL WINAPI GetDefaultCommConfigU(
 IN LPCSTR lpszName,
 OUT LPCOMMCONFIG lpCC,
 IN OUT LPDWORD lpdwSize
 )
{
return GetDefaultCommConfigW( UTF8_C_MAP(lpszName),lpCC,lpdwSize );
}

BOOL WINAPI SetDefaultCommConfigU(
 IN LPCSTR lpszName,
 IN LPCOMMCONFIG lpCC,
 IN DWORD dwSize
 )
{
return SetDefaultCommConfigW( UTF8_C_MAP(lpszName),lpCC,dwSize );
}

BOOL WINAPI GetComputerNameU(
 OUT LPSTR lpBuffer,
 IN OUT LPDWORD nSize
 )
{
return GetComputerNameW( UTF8_MAP(lpBuffer,*nSize),nSize );
}

BOOL WINAPI SetComputerNameU(
 IN LPCSTR lpComputerName
 )
{
return SetComputerNameW( UTF8_C_MAP(lpComputerName) );
}

/*
BOOL WINAPI GetComputerNameExU(
 IN COMPUTER_NAME_FORMAT NameType,
 OUT LPSTR lpBuffer,
 IN OUT LPDWORD nSize
 )
{
return GetComputerNameExW(NameType, UTF8_MAP(lpBuffer),nSize );
}

BOOL WINAPI SetComputerNameExU(
 IN COMPUTER_NAME_FORMAT NameType,
 IN LPCSTR lpBuffer
 )
{
return SetComputerNameExW(NameType, UTF8_C_MAP(lpBuffer) );
}

DWORD WINAPI AddLocalAlternateComputerNameU(
 IN LPCSTR lpDnsFQHostname,
 IN ULONG ulFlags
 )
{
return AddLocalAlternateComputerNameW( UTF8_C_MAP(lpDnsFQHostname),ulFlags );
}

DWORD WINAPI RemoveLocalAlternateComputerNameU(
 IN LPCSTR lpAltDnsFQHostname,
 IN ULONG ulFlags
 )
{
return RemoveLocalAlternateComputerNameW( UTF8_C_MAP(lpAltDnsFQHostname),ulFlags );
}

DWORD WINAPI SetLocalPrimaryComputerNameU(
 IN LPCSTR lpAltDnsFQHostname,
 IN ULONG ulFlags
 )
{
return SetLocalPrimaryComputerNameW( UTF8_C_MAP(lpAltDnsFQHostname),ulFlags );
}

DWORD WINAPI EnumerateLocalComputerNamesU(
 IN COMPUTER_NAME_TYPE NameType,
 IN ULONG ulFlags,
 IN OUT LPSTR lpDnsFQHostname,
 IN OUT LPDWORD nSize
 )
{
return EnumerateLocalComputerNamesW(NameType,ulFlags, UTF8_MAP(lpDnsFQHostname),nSize );
}

BOOL WINAPI DnsHostnameToComputerNameU(
 IN LPCSTR Hostname,
 OUT LPSTR ComputerName,
 IN OUT LPDWORD nSize
 )
{
return DnsHostnameToComputerNameW( UTF8_C_MAP(Hostname), UTF8_MAP(ComputerName),nSize );
}
*/
BOOL WINAPI GetUserNameU(
 OUT LPSTR lpBuffer,
 IN OUT LPDWORD nSize
 )
{
return GetUserNameW( UTF8_MAP(lpBuffer,*nSize),nSize );
}

BOOL WINAPI LogonUserU(
 IN LPSTR lpszUsername,
 IN LPSTR lpszDomain,
 IN LPSTR lpszPassword,
 IN DWORD dwLogonType,
 IN DWORD dwLogonProvider,
 OUT PHANDLE phToken
 )
{
return LogonUserW( (LPWSTR)UTF8_C_MAP(lpszUsername), (LPWSTR)UTF8_C_MAP(lpszDomain), (LPWSTR)UTF8_C_MAP(lpszPassword),dwLogonType,dwLogonProvider,phToken );
}

BOOL WINAPI LogonUserExU(
 IN LPSTR lpszUsername,
 IN LPSTR lpszDomain,
 IN LPSTR lpszPassword,
 IN DWORD dwLogonType,
 IN DWORD dwLogonProvider,
 OUT PHANDLE phToken OPTIONAL,
 OUT PSID *ppLogonSid OPTIONAL,
 OUT PVOID *ppProfileBuffer OPTIONAL,
 OUT LPDWORD pdwProfileLength OPTIONAL,
 OUT PQUOTA_LIMITS pQuotaLimits OPTIONAL
 )
{
return LogonUserExW( (LPWSTR)UTF8_C_MAP(lpszUsername), (LPWSTR)UTF8_C_MAP(lpszDomain), (LPWSTR)UTF8_C_MAP(lpszPassword),dwLogonType,dwLogonProvider,phToken,ppLogonSid,ppProfileBuffer,pdwProfileLength,pQuotaLimits );
}

BOOL WINAPI CreateProcessAsUserU(
 IN HANDLE hToken,
 IN LPCSTR lpApplicationName,
 IN LPSTR lpCommandLine,
 IN LPSECURITY_ATTRIBUTES lpProcessAttributes,
 IN LPSECURITY_ATTRIBUTES lpThreadAttributes,
 IN BOOL bInheritHandles,
 IN DWORD dwCreationFlags,
 IN LPVOID lpEnvironment,
 IN LPCSTR lpCurrentDirectory,
 IN LPSTARTUPINFOA lpStartupInfo,
 OUT LPPROCESS_INFORMATION lpProcessInformation
 )
{
	UTF8_STRING(lpwReserved,lpStartupInfo->lpReserved);
	UTF8_STRING(lpwDesktop,lpStartupInfo->lpDesktop);
	UTF8_STRING(lpwTitle,lpStartupInfo->lpTitle);

	STARTUPINFOW wStartupInfo =
	{
		sizeof(STARTUPINFOW),
		lpwReserved,
		lpwDesktop,
		lpwTitle,
		lpStartupInfo->dwX,
		lpStartupInfo->dwY,
		lpStartupInfo->dwXSize,
		lpStartupInfo->dwYSize,
		lpStartupInfo->dwXCountChars, 
		lpStartupInfo->dwYCountChars, 
		lpStartupInfo->dwFillAttribute, 
		lpStartupInfo->dwFlags, 
		lpStartupInfo->wShowWindow, 
		lpStartupInfo->cbReserved2, 
		lpStartupInfo->lpReserved2, 
		lpStartupInfo->hStdInput, 
		lpStartupInfo->hStdOutput, 
		lpStartupInfo->hStdError
	};
	return CreateProcessAsUserW(hToken, UTF8_C_MAP(lpApplicationName), (LPWSTR)UTF8_C_MAP(lpCommandLine),lpProcessAttributes,lpThreadAttributes,bInheritHandles,dwCreationFlags,lpEnvironment, UTF8_C_MAP(lpCurrentDirectory),&wStartupInfo,lpProcessInformation );
}

BOOL WINAPI GetCurrentHwProfileU(
 OUT LPHW_PROFILE_INFOA lpHwProfileInfo
 )
{
	HW_PROFILE_INFOW wHwProfileInfo;
	BOOL bRet = GetCurrentHwProfileW( &wHwProfileInfo );
	lpHwProfileInfo->dwDockInfo = wHwProfileInfo.dwDockInfo;
	strncpy(lpHwProfileInfo->szHwProfileGuid,UTF8_REVERSE_C_MAP(wHwProfileInfo.szHwProfileGuid),sizeof(lpHwProfileInfo->szHwProfileGuid));
	strncpy(lpHwProfileInfo->szHwProfileName,UTF8_REVERSE_C_MAP(wHwProfileInfo.szHwProfileName),sizeof(lpHwProfileInfo->szHwProfileName));
	return bRet;
}

BOOL WINAPI GetVersionExU(
 IN OUT LPOSVERSIONINFOA lpVersionInformation
 )
{
	OSVERSIONINFOW wVersionInformation = { sizeof(OSVERSIONINFOW) };
	BOOL bRet = GetVersionExW(&wVersionInformation);
	lpVersionInformation->dwOSVersionInfoSize = sizeof(OSVERSIONINFOA);
	lpVersionInformation->dwMajorVersion = wVersionInformation.dwMajorVersion;
	lpVersionInformation->dwMinorVersion = wVersionInformation.dwMinorVersion;
	lpVersionInformation->dwBuildNumber = wVersionInformation.dwBuildNumber;
	lpVersionInformation->dwPlatformId = wVersionInformation.dwPlatformId;
	strncpy(lpVersionInformation->szCSDVersion,UTF8_REVERSE_C_MAP(wVersionInformation.szCSDVersion), sizeof(lpVersionInformation->szCSDVersion));

	return bRet;
}

BOOL WINAPI VerifyVersionInfoU(
 IN LPOSVERSIONINFOEXA lpVersionInformation,
 IN DWORD dwTypeMask,
 IN DWORDLONG dwlConditionMask
 )
{
	OSVERSIONINFOEXW wVersionInformation = { sizeof(OSVERSIONINFOEXW) };
	wVersionInformation.dwMajorVersion = lpVersionInformation->dwMajorVersion;
	wVersionInformation.dwMinorVersion = lpVersionInformation->dwMinorVersion;
	wVersionInformation.dwBuildNumber = lpVersionInformation->dwBuildNumber;
	wVersionInformation.dwPlatformId = lpVersionInformation->dwPlatformId;
	wcsncpy(wVersionInformation.szCSDVersion,UTF8_C_MAP(lpVersionInformation->szCSDVersion), sizeof(wVersionInformation.szCSDVersion)/sizeof(WCHAR));
	wVersionInformation.wServicePackMajor = lpVersionInformation->wServicePackMajor;
	wVersionInformation.wServicePackMinor = lpVersionInformation->wServicePackMinor;
	wVersionInformation.wSuiteMask = lpVersionInformation->wSuiteMask;
	wVersionInformation.wProductType = lpVersionInformation->wProductType;
	wVersionInformation.wReserved = lpVersionInformation->wReserved;
	return VerifyVersionInfoW(&wVersionInformation,dwTypeMask,dwlConditionMask );
}

/*
HANDLE WINAPI CreateJobObjectU(
 IN LPSECURITY_ATTRIBUTES lpJobAttributes,
 IN LPCSTR lpName
 )
{
return CreateJobObjectW(lpJobAttributes, UTF8_C_MAP(lpName) );
}

HANDLE WINAPI OpenJobObjectU(
 IN DWORD dwDesiredAccess,
 IN BOOL bInheritHandle,
 IN LPCSTR lpName
 )
{
return OpenJobObjectW(dwDesiredAccess,bInheritHandle, UTF8_C_MAP(lpName) );
}

HANDLE WINAPI FindFirstVolumeU(
 LPSTR lpszVolumeName,
 DWORD cchBufferLength
 )
{
return FindFirstVolumeW( UTF8_MAP(lpszVolumeName),cchBufferLength );
}

BOOL WINAPI FindNextVolumeU(
 HANDLE hFindVolume,
 LPSTR lpszVolumeName,
 DWORD cchBufferLength
 )
{
return FindNextVolumeW(hFindVolume, UTF8_MAP(lpszVolumeName),cchBufferLength );
}

HANDLE WINAPI FindFirstVolumeMountPointU(
 LPCSTR lpszRootPathName,
 LPSTR lpszVolumeMountPoint,
 DWORD cchBufferLength
 )
{
return FindFirstVolumeMountPointW( UTF8_C_MAP(lpszRootPathName), UTF8_MAP(lpszVolumeMountPoint),cchBufferLength );
}

BOOL WINAPI FindNextVolumeMountPointU(
 HANDLE hFindVolumeMountPoint,
 LPSTR lpszVolumeMountPoint,
 DWORD cchBufferLength
 )
{
return FindNextVolumeMountPointW(hFindVolumeMountPoint, UTF8_MAP(lpszVolumeMountPoint),cchBufferLength );
}

BOOL WINAPI SetVolumeMountPointU(
 LPCSTR lpszVolumeMountPoint,
 LPCSTR lpszVolumeName
 )
{
return SetVolumeMountPointW( UTF8_C_MAP(lpszVolumeMountPoint), UTF8_C_MAP(lpszVolumeName) );
}

BOOL WINAPI DeleteVolumeMountPointU(
 LPCSTR lpszVolumeMountPoint
 )
{
return DeleteVolumeMountPointW( UTF8_C_MAP(lpszVolumeMountPoint) );
}

BOOL WINAPI GetVolumeNameForVolumeMountPointU(
 LPCSTR lpszVolumeMountPoint,
 LPSTR lpszVolumeName,
 DWORD cchBufferLength
 )
{
return GetVolumeNameForVolumeMountPointW( UTF8_C_MAP(lpszVolumeMountPoint), UTF8_MAP(lpszVolumeName),cchBufferLength );
}

BOOL WINAPI GetVolumePathNameU(
 LPCSTR lpszFileName,
 LPSTR lpszVolumePathName,
 DWORD cchBufferLength
 )
{
return GetVolumePathNameW( UTF8_C_MAP(lpszFileName), UTF8_MAP(lpszVolumePathName),cchBufferLength );
}

BOOL WINAPI GetVolumePathNamesForVolumeNameU(
 LPCSTR lpszVolumeName,
 LPSTR lpszVolumePathNames,
 DWORD cchBufferLength,
 PDWORD lpcchReturnLength
 )
{
return GetVolumePathNamesForVolumeNameW( UTF8_C_MAP(lpszVolumeName), UTF8_MAP(lpszVolumePathNames),cchBufferLength,lpcchReturnLength );
}

HANDLE WINAPI CreateActCtxU(
 PCACTCTXA pActCtx
 )
{
return CreateActCtxW(pActCtx );
}

BOOL WINAPI FindActCtxSectionStringU(
 DWORD dwFlags,
 const GUID *lpExtensionGuid,
 ULONG ulSectionId,
 LPCSTR lpStringToFind,
 PACTCTX_SECTION_KEYED_DATA ReturnedData
 )
{
return FindActCtxSectionStringW(dwFlags,lpExtensionGuid,ulSectionId, UTF8_C_MAP(lpStringToFind),ReturnedData );
}
*/