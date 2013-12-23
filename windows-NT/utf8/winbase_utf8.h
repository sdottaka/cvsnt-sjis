#ifndef WINBASE_UTF8__H
#define WINBASE_UTF8__H

BOOL
WINAPI
GetBinaryTypeU(
    IN LPCSTR lpApplicationName,
    OUT LPDWORD lpBinaryType
    );

#undef GetBinaryType
#define GetBinaryType GetBinaryTypeU

DWORD
WINAPI
GetShortPathNameU(
    IN LPCSTR lpszLongPath,
    OUT LPSTR  lpszShortPath,
    IN DWORD    cchBuffer
    );

#undef GetShortPathName
#define GetShortPathName GetShortPathNameU

DWORD
WINAPI
GetLongPathNameU(
    IN LPCSTR lpszShortPath,
    OUT LPSTR  lpszLongPath,
    IN DWORD    cchBuffer
    );

#undef GetLongPathName
#define GetLongPathName GetLongPathNameU

BOOL
WINAPI
FreeEnvironmentStringsU(
    IN LPSTR
    );

#undef FreeEnvironmentStrings
#define FreeEnvironmentStrings FreeEnvironmentStringsU

BOOL
WINAPI
SetFileShortNameU(
    IN HANDLE hFile,
    IN LPCSTR lpShortName
    );

#undef SetFileShortName
#define SetFileShortName SetFileShortNameU

DWORD
WINAPI
FormatMessageU(
    IN DWORD dwFlags,
    IN LPCVOID lpSource,
    IN DWORD dwMessageId,
    IN DWORD dwLanguageId,
    OUT LPSTR lpBuffer,
    IN DWORD nSize,
    IN va_list *Arguments
    );

#undef FormatMessage
#define FormatMessage FormatMessageU

HANDLE
WINAPI
CreateMailslotU(
    IN LPCSTR lpName,
    IN DWORD nMaxMessageSize,
    IN DWORD lReadTimeout,
    IN LPSECURITY_ATTRIBUTES lpSecurityAttributes
    );

#undef CreateMailslot
#define CreateMailslot CreateMailslotU

BOOL
WINAPI
EncryptFileU(
    IN LPCSTR lpFileName
    );

#undef EncryptFile
#define EncryptFile EncryptFileU

BOOL
WINAPI
DecryptFileU(
    IN LPCSTR lpFileName,
    IN DWORD    dwReserved
    );

#undef DecryptFile
#define DecryptFile DecryptFileU

BOOL
WINAPI
FileEncryptionStatusU(
    LPCSTR lpFileName,
    LPDWORD  lpStatus
    );

#undef FileEncryptionStatus
#define FileEncryptionStatus FileEncryptionStatusU

DWORD
WINAPI
OpenEncryptedFileRawU(
    IN LPCSTR        lpFileName,
    IN ULONG           ulFlags,
    IN PVOID *         pvContext
    );

#undef OpenEncryptedFile
#define OpenEncryptedFile OpenEncryptedFileU

int
WINAPI
lstrcmpU(
    IN LPCSTR lpString1,
    IN LPCSTR lpString2
    );

#undef lstrcmp
#define lstrcmp lstrcmpU

int
WINAPI
lstrcmpiU(
    IN LPCSTR lpString1,
    IN LPCSTR lpString2
    );

#undef lstrcmpi
#define lstrcmpi lstrcmpiU

void /* Was LPSTR */
WINAPI
lstrcpynU(
    OUT LPSTR lpString1,
    IN LPCSTR lpString2,
    IN int iMaxLength
    );

#undef lstrcpyn
#define lstrcpyn lstrcpynU

void /* Was LPSTR */
WINAPI
lstrcpyU(
    OUT LPSTR lpString1,
    IN LPCSTR lpString2
    );

#undef lstrcpy
#define lstrcpy lstrcpyU

void /* Was LPSTR */
WINAPI
lstrcatU(
    IN OUT LPSTR lpString1,
    IN LPCSTR lpString2
    );

#undef lstrcat
#define lstrcat lstrcatU

int
WINAPI
lstrlenU(
    IN LPCSTR lpString
    );

#undef lstrlen
#define lstrlen lstrlenU

HANDLE
WINAPI
CreateMutexU(
    IN LPSECURITY_ATTRIBUTES lpMutexAttributes,
    IN BOOL bInitialOwner,
    IN LPCSTR lpName
    );

#undef CreateMutex
#define CreateMutex CreateMutexU

HANDLE
WINAPI
OpenMutexU(
    IN DWORD dwDesiredAccess,
    IN BOOL bInheritHandle,
    IN LPCSTR lpName
    );

#undef OpenMutex
#define OpenMutex OpenMutexU

HANDLE
WINAPI
CreateEventU(
    IN LPSECURITY_ATTRIBUTES lpEventAttributes,
    IN BOOL bManualReset,
    IN BOOL bInitialState,
    IN LPCSTR lpName
    );

#undef CreateEvent
#define CreateEvent CreateEventU

HANDLE
WINAPI
OpenEventU(
    IN DWORD dwDesiredAccess,
    IN BOOL bInheritHandle,
    IN LPCSTR lpName
    );

#undef OpenEvent
#define OpenEvent OpenEventU

HANDLE
WINAPI
CreateSemaphoreU(
    IN LPSECURITY_ATTRIBUTES lpSemaphoreAttributes,
    IN LONG lInitialCount,
    IN LONG lMaximumCount,
    IN LPCSTR lpName
    );

#undef CreateSemaphore
#define CreateSemaphore CreateSemaphoreU

HANDLE
WINAPI
OpenSemaphoreU(
    IN DWORD dwDesiredAccess,
    IN BOOL bInheritHandle,
    IN LPCSTR lpName
    );

#undef OpenSemaphore
#define OpenSemaphore OpenSemaphoreU

HANDLE
WINAPI
CreateWaitableTimerU(
    IN LPSECURITY_ATTRIBUTES lpTimerAttributes,
    IN BOOL bManualReset,
    IN LPCSTR lpTimerName
    );

#undef CreateWaitableTimer
#define CreateWaitableTimer CreateWaitableTimerU

HANDLE
WINAPI
OpenWaitableTimerU(
    IN DWORD dwDesiredAccess,
    IN BOOL bInheritHandle,
    IN LPCSTR lpTimerName
    );

#undef OpenWaitableTimer
#define OpenWaitableTimer OpenWaitableTimerU

HANDLE
WINAPI
CreateFileMappingU(
    IN HANDLE hFile,
    IN LPSECURITY_ATTRIBUTES lpFileMappingAttributes,
    IN DWORD flProtect,
    IN DWORD dwMaximumSizeHigh,
    IN DWORD dwMaximumSizeLow,
    IN LPCSTR lpName
    );

#undef CreateFileMapping
#define CreateFileMapping CreateFileMappingU

HANDLE
WINAPI
OpenFileMappingU(
    IN DWORD dwDesiredAccess,
    IN BOOL bInheritHandle,
    IN LPCSTR lpName
    );

#undef OpenFileMapping
#define OpenFileMapping OpenFileMappingU

DWORD
WINAPI
GetLogicalDriveStringsU(
    IN DWORD nBufferLength,
    OUT LPSTR lpBuffer
    );

#undef GetLogicalDriveStrings
#define GetLogicalDriveStrings GetLogicalDriveStringsU

HMODULE
WINAPI
LoadLibraryU(
    IN LPCSTR lpLibFileName
    );

#undef LoadLibrary
#define LoadLibrary LoadLibraryU

HMODULE
WINAPI
LoadLibraryExU(
    IN LPCSTR lpLibFileName,
    IN HANDLE hFile,
    IN DWORD dwFlags
    );

#undef LoadLibraryEx
#define LoadLibraryEx LoadLibraryExU

DWORD
WINAPI
GetModuleFileNameU(
    IN HMODULE hModule,
    OUT LPSTR lpFilename,
    IN DWORD nSize
    );

HMODULE
WINAPI
GetModuleHandleU(
    IN LPCSTR lpModuleName
    );

BOOL
WINAPI
GetModuleHandleExU(
    IN DWORD        dwFlags,
    IN LPCSTR     lpModuleName,
    OUT HMODULE*    phModule
    );

BOOL
WINAPI
CreateProcessU(
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
    );

VOID
WINAPI
FatalAppExitU(
    IN UINT uAction,
    IN LPCSTR lpMessageText
    );

VOID
WINAPI
GetStartupInfoU(
    OUT LPSTARTUPINFOA lpStartupInfo
    );

LPSTR
WINAPI
GetCommandLineU(
    VOID
    );

DWORD
WINAPI
GetEnvironmentVariableU(
    IN LPCSTR lpName,
    OUT LPSTR lpBuffer,
    IN DWORD nSize
    );

BOOL
WINAPI
SetEnvironmentVariableU(
    IN LPCSTR lpName,
    IN LPCSTR lpValue
    );

DWORD
WINAPI
ExpandEnvironmentStringsU(
    IN LPCSTR lpSrc,
    OUT LPSTR lpDst,
    IN DWORD nSize
    );

DWORD
WINAPI
GetFirmwareEnvironmentVariableU(
    IN LPCSTR lpName,
    IN LPCSTR lpGuid,
    OUT PVOID   pBuffer,
    IN DWORD    nSize
    );

BOOL
WINAPI
SetFirmwareEnvironmentVariableU(
    IN LPCSTR lpName,
    IN LPCSTR lpGuid,
    IN PVOID    pValue,
    IN DWORD    nSize
    );

VOID
WINAPI
OutputDebugStringU(
    IN LPCSTR lpOutputString
    );

HRSRC
WINAPI
FindResourceU(
    IN HMODULE hModule,
    IN LPCSTR lpName,
    IN LPCSTR lpType
    );

HRSRC
WINAPI
FindResourceExU(
    IN HMODULE hModule,
    IN LPCSTR lpType,
    IN LPCSTR lpName,
    IN WORD    wLanguage
    );

BOOL
WINAPI
EnumResourceTypesU(
    IN HMODULE hModule,
    IN ENUMRESTYPEPROCW lpEnumFunc,
    IN LONG_PTR lParam
    );

BOOL
WINAPI
EnumResourceNamesU(
    IN HMODULE hModule,
    IN LPCSTR lpType,
    IN ENUMRESNAMEPROCW lpEnumFunc,
    IN LONG_PTR lParam
    );

BOOL
WINAPI
EnumResourceLanguagesU(
    IN HMODULE hModule,
    IN LPCSTR lpType,
    IN LPCSTR lpName,
    IN ENUMRESLANGPROCW lpEnumFunc,
    IN LONG_PTR lParam
    );

HANDLE
WINAPI
BeginUpdateResourceU(
    IN LPCSTR pFileName,
    IN BOOL bDeleteExistingResources
    );

BOOL
WINAPI
UpdateResourceU(
    IN HANDLE      hUpdate,
    IN LPCSTR     lpType,
    IN LPCSTR     lpName,
    IN WORD        wLanguage,
    IN LPVOID      lpData,
    IN DWORD       cbData
    );

BOOL
WINAPI
EndUpdateResourceU(
    IN HANDLE      hUpdate,
    IN BOOL        fDiscard
    );

ATOM
WINAPI
GlobalAddAtomU(
    IN LPCSTR lpString
    );

ATOM
WINAPI
GlobalFindAtomU(
    IN LPCSTR lpString
    );

UINT
WINAPI
GlobalGetAtomNameU(
    IN ATOM nAtom,
    OUT LPSTR lpBuffer,
    IN int nSize
    );

ATOM
WINAPI
AddAtomU(
    IN LPCSTR lpString
    );

ATOM
WINAPI
FindAtomU(
    IN LPCSTR lpString
    );

UINT
WINAPI
GetAtomNameU(
    IN ATOM nAtom,
    OUT LPSTR lpBuffer,
    IN int nSize
    );

UINT
WINAPI
GetProfileIntU(
    IN LPCSTR lpAppName,
    IN LPCSTR lpKeyName,
    IN INT nDefault
    );

DWORD
WINAPI
GetProfileStringU(
    IN LPCSTR lpAppName,
    IN LPCSTR lpKeyName,
    IN LPCSTR lpDefault,
    OUT LPSTR lpReturnedString,
    IN DWORD nSize
    );

BOOL
WINAPI
WriteProfileStringU(
    IN LPCSTR lpAppName,
    IN LPCSTR lpKeyName,
    IN LPCSTR lpString
    );

DWORD
WINAPI
GetProfileSectionU(
    IN LPCSTR lpAppName,
    OUT LPSTR lpReturnedString,
    IN DWORD nSize
    );

BOOL
WINAPI
WriteProfileSectionU(
    IN LPCSTR lpAppName,
    IN LPCSTR lpString
    );

UINT
WINAPI
GetPrivateProfileIntU(
    IN LPCSTR lpAppName,
    IN LPCSTR lpKeyName,
    IN INT nDefault,
    IN LPCSTR lpFileName
    );

DWORD
WINAPI
GetPrivateProfileStringU(
    IN LPCSTR lpAppName,
    IN LPCSTR lpKeyName,
    IN LPCSTR lpDefault,
    OUT LPSTR lpReturnedString,
    IN DWORD nSize,
    IN LPCSTR lpFileName
    );

BOOL
WINAPI
WritePrivateProfileStringU(
    IN LPCSTR lpAppName,
    IN LPCSTR lpKeyName,
    IN LPCSTR lpString,
    IN LPCSTR lpFileName
    );

DWORD
WINAPI
GetPrivateProfileSectionU(
    IN LPCSTR lpAppName,
    OUT LPSTR lpReturnedString,
    IN DWORD nSize,
    IN LPCSTR lpFileName
    );

BOOL
WINAPI
WritePrivateProfileSectionU(
    IN LPCSTR lpAppName,
    IN LPCSTR lpString,
    IN LPCSTR lpFileName
    );

DWORD
WINAPI
GetPrivateProfileSectionNamesU(
    OUT LPSTR lpszReturnBuffer,
    IN DWORD nSize,
    IN LPCSTR lpFileName
    );

BOOL
WINAPI
GetPrivateProfileStructU(
    IN LPCSTR lpszSection,
    IN LPCSTR lpszKey,
    OUT LPVOID   lpStruct,
    IN UINT     uSizeStruct,
    IN LPCSTR szFile
    );

BOOL
WINAPI
WritePrivateProfileStructU(
    IN LPCSTR lpszSection,
    IN LPCSTR lpszKey,
    IN LPVOID   lpStruct,
    IN UINT     uSizeStruct,
    IN LPCSTR szFile
    );

UINT
WINAPI
GetDriveTypeU(
    IN LPCSTR lpRootPathName
    );

UINT
WINAPI
GetSystemDirectoryU(
    OUT LPSTR lpBuffer,
    IN UINT uSize
    );

DWORD
WINAPI
GetTempPathU(
    IN DWORD nBufferLength,
    OUT LPSTR lpBuffer
    );

UINT
WINAPI
GetTempFileNameU(
    IN LPCSTR lpPathName,
    IN LPCSTR lpPrefixString,
    IN UINT uUnique,
    OUT LPSTR lpTempFileName
    );

UINT
WINAPI
GetWindowsDirectoryU(
    OUT LPSTR lpBuffer,
    IN UINT uSize
    );

UINT
WINAPI
GetSystemWindowsDirectoryU(
    OUT LPSTR lpBuffer,
    IN UINT uSize
    );

UINT
WINAPI
GetSystemWow64DirectoryU(
    OUT LPSTR lpBuffer,
    IN UINT uSize
    );

BOOL
WINAPI
SetCurrentDirectoryU(
    IN LPCSTR lpPathName
    );

DWORD
WINAPI
GetCurrentDirectoryU(
    IN DWORD nBufferLength,
    OUT LPSTR lpBuffer
    );

BOOL
WINAPI
GetDiskFreeSpaceU(
    IN LPCSTR lpRootPathName,
    OUT LPDWORD lpSectorsPerCluster,
    OUT LPDWORD lpBytesPerSector,
    OUT LPDWORD lpNumberOfFreeClusters,
    OUT LPDWORD lpTotalNumberOfClusters
    );

BOOL
WINAPI
GetDiskFreeSpaceExU(
    IN LPCSTR lpDirectoryName,
    OUT PULARGE_INTEGER lpFreeBytesAvailableToCaller,
    OUT PULARGE_INTEGER lpTotalNumberOfBytes,
    OUT PULARGE_INTEGER lpTotalNumberOfFreeBytes
    );

BOOL
WINAPI
CreateDirectoryU(
    IN LPCSTR lpPathName,
    IN LPSECURITY_ATTRIBUTES lpSecurityAttributes
    );

BOOL
WINAPI
CreateDirectoryExU(
    IN LPCSTR lpTemplateDirectory,
    IN LPCSTR lpNewDirectory,
    IN LPSECURITY_ATTRIBUTES lpSecurityAttributes
    );

BOOL
WINAPI
RemoveDirectoryU(
    IN LPCSTR lpPathName
    );

DWORD
WINAPI
GetFullPathNameU(
    IN LPCSTR lpFileName,
    IN DWORD nBufferLength,
    OUT LPSTR lpBuffer,
    OUT LPSTR *lpFilePart
    );

BOOL
WINAPI
DefineDosDeviceU(
    IN DWORD dwFlags,
    IN LPCSTR lpDeviceName,
    IN LPCSTR lpTargetPath
    );

DWORD
WINAPI
QueryDosDeviceU(
    IN LPCSTR lpDeviceName,
    OUT LPSTR lpTargetPath,
    IN DWORD ucchMax
    );

HANDLE
WINAPI
CreateFileU(
    IN LPCSTR lpFileName,
    IN DWORD dwDesiredAccess,
    IN DWORD dwShareMode,
    IN LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    IN DWORD dwCreationDisposition,
    IN DWORD dwFlagsAndAttributes,
    IN HANDLE hTemplateFile
    );

BOOL
WINAPI
SetFileAttributesU(
    IN LPCSTR lpFileName,
    IN DWORD dwFileAttributes
    );

DWORD
WINAPI
GetFileAttributesU(
    IN LPCSTR lpFileName
    );

BOOL
WINAPI
GetFileAttributesExU(
    IN LPCSTR lpFileName,
    IN GET_FILEEX_INFO_LEVELS fInfoLevelId,
    OUT LPVOID lpFileInformation
    );

DWORD
WINAPI
GetCompressedFileSizeU(
    IN LPCSTR lpFileName,
    OUT LPDWORD lpFileSizeHigh
    );

BOOL
WINAPI
DeleteFileU(
    IN LPCSTR lpFileName
    );

HANDLE
WINAPI
FindFirstFileExU(
    IN LPCSTR lpFileName,
    IN FINDEX_INFO_LEVELS fInfoLevelId,
    OUT LPVOID lpFindFileData,
    IN FINDEX_SEARCH_OPS fSearchOp,
    IN LPVOID lpSearchFilter,
    IN DWORD dwAdditionalFlags
    );

HANDLE
WINAPI
FindFirstFileU(
    IN LPCSTR lpFileName,
    OUT LPWIN32_FIND_DATAA lpFindFileData
    );

BOOL
WINAPI
FindNextFileU(
    IN HANDLE hFindFile,
    OUT LPWIN32_FIND_DATAA lpFindFileData
    );

DWORD
WINAPI
SearchPathU(
    IN LPCSTR lpPath,
    IN LPCSTR lpFileName,
    IN LPCSTR lpExtension,
    IN DWORD nBufferLength,
    OUT LPSTR lpBuffer,
    OUT LPSTR *lpFilePart
    );

BOOL
WINAPI
CopyFileU(
    IN LPCSTR lpExistingFileName,
    IN LPCSTR lpNewFileName,
    IN BOOL bFailIfExists
    );

BOOL
WINAPI
CopyFileExU(
    IN LPCSTR lpExistingFileName,
    IN LPCSTR lpNewFileName,
    IN LPPROGRESS_ROUTINE lpProgressRoutine OPTIONAL,
    IN LPVOID lpData OPTIONAL,
    IN LPBOOL pbCancel OPTIONAL,
    IN DWORD dwCopyFlags
    );

BOOL
WINAPI
MoveFileU(
    IN LPCSTR lpExistingFileName,
    IN LPCSTR lpNewFileName
    );

BOOL
WINAPI
MoveFileExU(
    IN LPCSTR lpExistingFileName,
    IN LPCSTR lpNewFileName,
    IN DWORD dwFlags
    );

BOOL
WINAPI
MoveFileWithProgressU(
    IN LPCSTR lpExistingFileName,
    IN LPCSTR lpNewFileName,
    IN LPPROGRESS_ROUTINE lpProgressRoutine OPTIONAL,
    IN LPVOID lpData OPTIONAL,
    IN DWORD dwFlags
    );

BOOL
WINAPI
ReplaceFileU(
    LPCSTR  lpReplacedFileName,
    LPCSTR  lpReplacementFileName,
    LPCSTR  lpBackupFileName,
    DWORD   dwReplaceFlags,
    LPVOID  lpExclude,
    LPVOID  lpReserved
    );

BOOL
WINAPI
CreateHardLinkU(
    IN LPCSTR lpFileName,
    IN LPCSTR lpExistingFileName,
    IN LPSECURITY_ATTRIBUTES lpSecurityAttributes
    );

HANDLE
WINAPI
CreateNamedPipeU(
    IN LPCSTR lpName,
    IN DWORD dwOpenMode,
    IN DWORD dwPipeMode,
    IN DWORD nMaxInstances,
    IN DWORD nOutBufferSize,
    IN DWORD nInBufferSize,
    IN DWORD nDefaultTimeOut,
    IN LPSECURITY_ATTRIBUTES lpSecurityAttributes
    );

BOOL
WINAPI
GetNamedPipeHandleStateU(
    IN HANDLE hNamedPipe,
    OUT LPDWORD lpState,
    OUT LPDWORD lpCurInstances,
    OUT LPDWORD lpMaxCollectionCount,
    OUT LPDWORD lpCollectDataTimeout,
    OUT LPSTR lpUserName,
    IN DWORD nMaxUserNameSize
    );

BOOL
WINAPI
CallNamedPipeU(
    IN LPCSTR lpNamedPipeName,
    IN LPVOID lpInBuffer,
    IN DWORD nInBufferSize,
    OUT LPVOID lpOutBuffer,
    IN DWORD nOutBufferSize,
    OUT LPDWORD lpBytesRead,
    IN DWORD nTimeOut
    );

BOOL
WINAPI
WaitNamedPipeU(
    IN LPCSTR lpNamedPipeName,
    IN DWORD nTimeOut
    );

BOOL
WINAPI
SetVolumeLabelU(
    IN LPCSTR lpRootPathName,
    IN LPCSTR lpVolumeName
    );

BOOL
WINAPI
GetVolumeInformationU(
    IN LPCSTR lpRootPathName,
    OUT LPSTR lpVolumeNameBuffer,
    IN DWORD nVolumeNameSize,
    OUT LPDWORD lpVolumeSerialNumber,
    OUT LPDWORD lpMaximumComponentLength,
    OUT LPDWORD lpFileSystemFlags,
    OUT LPSTR lpFileSystemNameBuffer,
    IN DWORD nFileSystemNameSize
    );

BOOL
WINAPI
ClearEventLogU(
    IN HANDLE hEventLog,
    IN LPCSTR lpBackupFileName
    );

BOOL
WINAPI
BackupEventLogU(
    IN HANDLE hEventLog,
    IN LPCSTR lpBackupFileName
    );

HANDLE
WINAPI
OpenEventLogU(
    IN LPCSTR lpUNCServerName,
    IN LPCSTR lpSourceName
    );

HANDLE
WINAPI
RegisterEventSourceU(
    IN LPCSTR lpUNCServerName,
    IN LPCSTR lpSourceName
    );

HANDLE
WINAPI
OpenBackupEventLogU(
    IN LPCSTR lpUNCServerName,
    IN LPCSTR lpFileName
    );

BOOL
WINAPI
ReadEventLogU(
     IN HANDLE     hEventLog,
     IN DWORD      dwReadFlags,
     IN DWORD      dwRecordOffset,
     OUT LPVOID     lpBuffer,
     IN DWORD      nNumberOfBytesToRead,
     OUT DWORD      *pnBytesRead,
     OUT DWORD      *pnMinNumberOfBytesNeeded
    );

BOOL
WINAPI
ReportEventU(
     IN HANDLE     hEventLog,
     IN WORD       wType,
     IN WORD       wCategory,
     IN DWORD      dwEventID,
     IN PSID       lpUserSid,
     IN WORD       wNumStrings,
     IN DWORD      dwDataSize,
     IN LPCSTR   *lpStrings,
     IN LPVOID     lpRawData
    );

BOOL
WINAPI
AccessCheckAndAuditAlarmU(
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
    );

BOOL
WINAPI
AccessCheckByTypeAndAuditAlarmU(
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
    );

BOOL
WINAPI
AccessCheckByTypeResultListAndAuditAlarmU(
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
    );

BOOL
WINAPI
AccessCheckByTypeResultListAndAuditAlarmByHandleU(
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
    );

BOOL
WINAPI
ObjectOpenAuditAlarmU(
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
    );

BOOL
WINAPI
ObjectPrivilegeAuditAlarmU(
    IN LPCSTR SubsystemName,
    IN LPVOID HandleId,
    IN HANDLE ClientToken,
    IN DWORD DesiredAccess,
    IN PPRIVILEGE_SET Privileges,
    IN BOOL AccessGranted
    );

BOOL
WINAPI
ObjectCloseAuditAlarmU(
    IN LPCSTR SubsystemName,
    IN LPVOID HandleId,
    IN BOOL GenerateOnClose
    );

BOOL
WINAPI
ObjectDeleteAuditAlarmU(
    IN LPCSTR SubsystemName,
    IN LPVOID HandleId,
    IN BOOL GenerateOnClose
    );

BOOL
WINAPI
PrivilegedServiceAuditAlarmU(
    IN LPCSTR SubsystemName,
    IN LPCSTR ServiceName,
    IN HANDLE ClientToken,
    IN PPRIVILEGE_SET Privileges,
    IN BOOL AccessGranted
    );

BOOL
WINAPI
SetFileSecurityU(
    IN LPCSTR lpFileName,
    IN SECURITY_INFORMATION SecurityInformation,
    IN PSECURITY_DESCRIPTOR pSecurityDescriptor
    );

BOOL
WINAPI
GetFileSecurityU(
    IN LPCSTR lpFileName,
    IN SECURITY_INFORMATION RequestedInformation,
    OUT PSECURITY_DESCRIPTOR pSecurityDescriptor,
    IN DWORD nLength,
    OUT LPDWORD lpnLengthNeeded
    );

HANDLE
WINAPI
FindFirstChangeNotificationU(
    IN LPCSTR lpPathName,
    IN BOOL bWatchSubtree,
    IN DWORD dwNotifyFilter
    );

BOOL
WINAPI
IsBadStringPtrU(
    IN LPCSTR lpsz,
    IN UINT_PTR ucchMax
    );

BOOL
WINAPI
LookupAccountSidU(
    IN LPCSTR lpSystemName,
    IN PSID Sid,
    OUT LPSTR Name,
    IN OUT LPDWORD cbName,
    OUT LPSTR ReferencedDomainName,
    IN OUT LPDWORD cbReferencedDomainName,
    OUT PSID_NAME_USE peUse
    );

BOOL
WINAPI
LookupAccountNameU(
    IN LPCSTR lpSystemName,
    IN LPCSTR lpAccountName,
    OUT PSID Sid,
    IN OUT LPDWORD cbSid,
    OUT LPSTR ReferencedDomainName,
    IN OUT LPDWORD cbReferencedDomainName,
    OUT PSID_NAME_USE peUse
    );

BOOL
WINAPI
LookupPrivilegeValueU(
    IN LPCSTR lpSystemName,
    IN LPCSTR lpName,
    OUT PLUID   lpLuid
    );

BOOL
WINAPI
LookupPrivilegeNameU(
    IN LPCSTR lpSystemName,
    IN PLUID   lpLuid,
    OUT LPSTR lpName,
    IN OUT LPDWORD cbName
    );

BOOL
WINAPI
LookupPrivilegeDisplayNameU(
    IN LPCSTR lpSystemName,
    IN LPCSTR lpName,
    OUT LPSTR lpDisplayName,
    IN OUT LPDWORD cbDisplayName,
    OUT LPDWORD lpLanguageId
    );

BOOL
WINAPI
BuildCommDCBU(
    IN LPCSTR lpDef,
    OUT LPDCB lpDCB
    );

BOOL
WINAPI
BuildCommDCBAndTimeoutsU(
    IN LPCSTR lpDef,
    OUT LPDCB lpDCB,
    IN LPCOMMTIMEOUTS lpCommTimeouts
    );

BOOL
WINAPI
CommConfigDialogU(
    IN LPCSTR lpszName,
    IN HWND hWnd,
    IN OUT LPCOMMCONFIG lpCC
    );

BOOL
WINAPI
GetDefaultCommConfigU(
    IN LPCSTR lpszName,
    OUT LPCOMMCONFIG lpCC,
    IN OUT LPDWORD lpdwSize
    );

BOOL
WINAPI
SetDefaultCommConfigU(
    IN LPCSTR lpszName,
    IN LPCOMMCONFIG lpCC,
    IN DWORD dwSize
    );

BOOL
WINAPI
GetComputerNameU(
    OUT LPSTR lpBuffer,
    IN OUT LPDWORD nSize
    );

BOOL
WINAPI
SetComputerNameU(
    IN LPCSTR lpComputerName
    );

/*BOOL
WINAPI
GetComputerNameExU(
    IN COMPUTER_NAME_FORMAT NameType,
    OUT LPSTR lpBuffer,
    IN OUT LPDWORD nSize
    );

BOOL
WINAPI
SetComputerNameExU(
    IN COMPUTER_NAME_FORMAT NameType,
    IN LPCSTR lpBuffer
    );

DWORD
WINAPI
AddLocalAlternateComputerNameU(
    IN LPCSTR lpDnsFQHostname,
    IN ULONG    ulFlags
    );

DWORD
WINAPI
RemoveLocalAlternateComputerNameU(
    IN LPCSTR lpAltDnsFQHostname,
    IN ULONG    ulFlags
    );

DWORD
WINAPI
SetLocalPrimaryComputerNameU(
    IN LPCSTR  lpAltDnsFQHostname,
    IN ULONG     ulFlags
    );

DWORD
WINAPI
EnumerateLocalComputerNamesU(
    IN COMPUTER_NAME_TYPE        NameType,
    IN ULONG                     ulFlags,
    IN OUT LPSTR               lpDnsFQHostname,
    IN OUT LPDWORD               nSize
    );

BOOL
WINAPI
DnsHostnameToComputerNameU(
    IN LPCSTR Hostname,
    OUT LPSTR ComputerName,
    IN OUT LPDWORD nSize
    );
*/
BOOL
WINAPI
GetUserNameU(
    OUT LPSTR lpBuffer,
    IN OUT LPDWORD nSize
    );

BOOL
WINAPI
LogonUserU(
    IN LPSTR lpszUsername,
    IN LPSTR lpszDomain,
    IN LPSTR lpszPassword,
    IN DWORD dwLogonType,
    IN DWORD dwLogonProvider,
    OUT PHANDLE phToken
    );

BOOL
WINAPI
LogonUserExU(
    IN LPSTR lpszUsername,
    IN LPSTR lpszDomain,
    IN LPSTR lpszPassword,
    IN DWORD dwLogonType,
    IN DWORD dwLogonProvider,
    OUT PHANDLE phToken           OPTIONAL,
    OUT PSID  *ppLogonSid       OPTIONAL,
    OUT PVOID *ppProfileBuffer  OPTIONAL,
    OUT LPDWORD pdwProfileLength  OPTIONAL,
    OUT PQUOTA_LIMITS pQuotaLimits OPTIONAL
    );

BOOL
WINAPI
CreateProcessAsUserU(
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
    );

BOOL
WINAPI
GetCurrentHwProfileU(
    OUT LPHW_PROFILE_INFOA  lpHwProfileInfo
    );

BOOL
WINAPI
GetVersionExU(
    IN OUT LPOSVERSIONINFOA lpVersionInformation
    );

BOOL
WINAPI
VerifyVersionInfoU(
    IN LPOSVERSIONINFOEXA lpVersionInformation,
    IN DWORD dwTypeMask,
    IN DWORDLONG dwlConditionMask
    );

HANDLE
WINAPI
CreateJobObjectU(
    IN LPSECURITY_ATTRIBUTES lpJobAttributes,
    IN LPCSTR lpName
    );

HANDLE
WINAPI
OpenJobObjectU(
    IN DWORD dwDesiredAccess,
    IN BOOL bInheritHandle,
    IN LPCSTR lpName
    );

HANDLE
WINAPI
FindFirstVolumeU(
    LPSTR lpszVolumeName,
    DWORD cchBufferLength
    );

BOOL
WINAPI
FindNextVolumeU(
    HANDLE hFindVolume,
    LPSTR lpszVolumeName,
    DWORD cchBufferLength
    );

HANDLE
WINAPI
FindFirstVolumeMountPointU(
    LPCSTR lpszRootPathName,
    LPSTR lpszVolumeMountPoint,
    DWORD cchBufferLength
    );

BOOL
WINAPI
FindNextVolumeMountPointU(
    HANDLE hFindVolumeMountPoint,
    LPSTR lpszVolumeMountPoint,
    DWORD cchBufferLength
    );

BOOL
WINAPI
SetVolumeMountPointU(
    LPCSTR lpszVolumeMountPoint,
    LPCSTR lpszVolumeName
    );

BOOL
WINAPI
DeleteVolumeMountPointU(
    LPCSTR lpszVolumeMountPoint
    );

BOOL
WINAPI
GetVolumeNameForVolumeMountPointU(
    LPCSTR lpszVolumeMountPoint,
    LPSTR lpszVolumeName,
    DWORD cchBufferLength
    );

BOOL
WINAPI
GetVolumePathNameU(
    LPCSTR lpszFileName,
    LPSTR lpszVolumePathName,
    DWORD cchBufferLength
    );

BOOL
WINAPI
GetVolumePathNamesForVolumeNameU(
    LPCSTR lpszVolumeName,
    LPSTR lpszVolumePathNames,
    DWORD cchBufferLength,
    PDWORD lpcchReturnLength
    );

/*HANDLE
WINAPI
CreateActCtxU(
    PCACTCTXA pActCtx
    );

BOOL
WINAPI
FindActCtxSectionStringU(
    DWORD dwFlags,
    const GUID *lpExtensionGuid,
    ULONG ulSectionId,
    LPCSTR lpStringToFind,
    PACTCTX_SECTION_KEYED_DATA ReturnedData
    );
*/
#endif
