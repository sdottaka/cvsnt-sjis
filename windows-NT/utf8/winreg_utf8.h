LONG
APIENTRY
RegConnectRegistryU(
    IN LPCSTR lpMachineName,
    IN HKEY hKey,
    OUT PHKEY phkResult
    );

LONG
APIENTRY
RegCreateKeyU(
    IN HKEY hKey,
    IN LPCSTR lpSubKey,
    OUT PHKEY phkResult
    );

LONG
APIENTRY
RegCreateKeyExU(
    IN HKEY hKey,
    IN LPCSTR lpSubKey,
    IN DWORD Reserved,
    IN LPSTR lpClass,
    IN DWORD dwOptions,
    IN REGSAM samDesired,
    IN LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    OUT PHKEY phkResult,
    OUT LPDWORD lpdwDisposition
    );

LONG
APIENTRY
RegDeleteKeyU(
    IN HKEY hKey,
    IN LPCSTR lpSubKey
    );

LONG
APIENTRY
RegDeleteValueU(
    IN HKEY hKey,
    IN LPCSTR lpValueName
    );

LONG
APIENTRY
RegEnumKeyU(
    IN HKEY hKey,
    IN DWORD dwIndex,
    OUT LPSTR lpName,
    IN DWORD cbName
    );

LONG
APIENTRY
RegEnumKeyExU(
    IN HKEY hKey,
    IN DWORD dwIndex,
    OUT LPSTR lpName,
    IN OUT LPDWORD lpcbName,
    IN LPDWORD lpReserved,
    IN OUT LPSTR lpClass,
    IN OUT LPDWORD lpcbClass,
    OUT PFILETIME lpftLastWriteTime
    );

LONG
APIENTRY
RegEnumValueU(
    IN HKEY hKey,
    IN DWORD dwIndex,
    OUT LPSTR lpValueName,
    IN OUT LPDWORD lpcbValueName,
    IN LPDWORD lpReserved,
    OUT LPDWORD lpType,
    OUT LPBYTE lpData,
    IN OUT LPDWORD lpcbData
    );

LONG
APIENTRY
RegLoadKeyU(
    IN HKEY    hKey,
    IN LPCSTR  lpSubKey,
    IN LPCSTR  lpFile
    );

LONG
APIENTRY
RegOpenKeyU(
    IN HKEY hKey,
    IN LPCSTR lpSubKey,
    OUT PHKEY phkResult
    );

LONG
APIENTRY
RegOpenKeyExU(
    IN HKEY hKey,
    IN LPCSTR lpSubKey,
    IN DWORD ulOptions,
    IN REGSAM samDesired,
    OUT PHKEY phkResult
    );

LONG
APIENTRY
RegQueryInfoKeyU(
    IN HKEY hKey,
    OUT LPSTR lpClass,
    IN OUT LPDWORD lpcbClass,
    IN LPDWORD lpReserved,
    OUT LPDWORD lpcSubKeys,
    OUT LPDWORD lpcbMaxSubKeyLen,
    OUT LPDWORD lpcbMaxClassLen,
    OUT LPDWORD lpcValues,
    OUT LPDWORD lpcbMaxValueNameLen,
    OUT LPDWORD lpcbMaxValueLen,
    OUT LPDWORD lpcbSecurityDescriptor,
    OUT PFILETIME lpftLastWriteTime
    );

LONG
APIENTRY
RegQueryValueU(
    IN HKEY hKey,
    IN LPCSTR lpSubKey,
    OUT LPSTR lpValue,
    IN OUT PLONG   lpcbValue
    );

LONG
APIENTRY
RegQueryMultipleValuesU(
    IN HKEY hKey,
    OUT PVALENTA val_list,
    IN DWORD num_vals,
    OUT LPSTR lpValueBuf,
    IN OUT LPDWORD ldwTotsize
    );

LONG
APIENTRY
RegQueryValueExU(
    IN HKEY hKey,
    IN LPCSTR lpValueName,
    IN LPDWORD lpReserved,
    OUT LPDWORD lpType,
    IN OUT LPBYTE lpData,
    IN OUT LPDWORD lpcbData
    );

LONG
APIENTRY
RegReplaceKeyU(
    IN HKEY     hKey,
    IN LPCSTR  lpSubKey,
    IN LPCSTR  lpNewFile,
    IN LPCSTR  lpOldFile
    );

LONG
APIENTRY
RegRestoreKeyU(
    IN HKEY hKey,
    IN LPCSTR lpFile,
    IN DWORD   dwFlags
    );

LONG
APIENTRY
RegSaveKeyU(
    IN HKEY hKey,
    IN LPCSTR lpFile,
    IN LPSECURITY_ATTRIBUTES lpSecurityAttributes
    );

LONG
APIENTRY
RegSetValueU(
    IN HKEY hKey,
    IN LPCSTR lpSubKey,
    IN DWORD dwType,
    IN LPCSTR lpData,
    IN DWORD cbData
    );

LONG
APIENTRY
RegSetValueExU(
    IN HKEY hKey,
    IN LPCSTR lpValueName,
    IN DWORD Reserved,
    IN DWORD dwType,
    IN CONST BYTE* lpData,
    IN DWORD cbData
    );

LONG
APIENTRY
RegUnLoadKeyU(
    IN HKEY    hKey,
    IN LPCSTR lpSubKey
    );

BOOL
APIENTRY
InitiateSystemShutdownU(
    IN LPSTR lpMachineName,
    IN LPSTR lpMessage,
    IN DWORD dwTimeout,
    IN BOOL bForceAppsClosed,
    IN BOOL bRebootAfterShutdown
    );

BOOL
APIENTRY
AbortSystemShutdownU(
    IN LPSTR lpMachineName
    );

BOOL
APIENTRY
InitiateSystemShutdownExU(
    IN LPSTR lpMachineName,
    IN LPSTR lpMessage,
    IN DWORD dwTimeout,
    IN BOOL bForceAppsClosed,
    IN BOOL bRebootAfterShutdown,
    IN DWORD dwReason
    );

LONG
APIENTRY
RegSaveKeyExU(
    IN HKEY hKey,
    IN LPCSTR lpFile,
    IN LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    IN DWORD Flags
    );

