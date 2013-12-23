#include "stdafx.h"
#include "winreg_utf8.h"

LONG APIENTRY RegConnectRegistryU(
 IN LPCSTR lpMachineName,
 IN HKEY hKey,
 OUT PHKEY phkResult
 )
{
return RegConnectRegistryW( UTF8_C_MAP(lpMachineName),hKey,phkResult );
}

LONG APIENTRY RegCreateKeyU(
 IN HKEY hKey,
 IN LPCSTR lpSubKey,
 OUT PHKEY phkResult
 )
{
return RegCreateKeyW(hKey, UTF8_C_MAP(lpSubKey),phkResult );
}

LONG APIENTRY RegCreateKeyExU(
 IN HKEY hKey,
 IN LPCSTR lpSubKey,
 IN DWORD Reserved,
 IN LPSTR lpClass,
 IN DWORD dwOptions,
 IN REGSAM samDesired,
 IN LPSECURITY_ATTRIBUTES lpSecurityAttributes,
 OUT PHKEY phkResult,
 OUT LPDWORD lpdwDisposition
 )
{
return RegCreateKeyExW(hKey, UTF8_C_MAP(lpSubKey),Reserved, (LPWSTR)UTF8_C_MAP(lpClass),dwOptions,samDesired,lpSecurityAttributes,phkResult,lpdwDisposition );
}

LONG APIENTRY RegDeleteKeyU(
 IN HKEY hKey,
 IN LPCSTR lpSubKey
 )
{
return RegDeleteKeyW(hKey, UTF8_C_MAP(lpSubKey) );
}

LONG APIENTRY RegDeleteValueU(
 IN HKEY hKey,
 IN LPCSTR lpValueName
 )
{
return RegDeleteValueW(hKey, UTF8_C_MAP(lpValueName) );
}

LONG APIENTRY RegEnumKeyU(
 IN HKEY hKey,
 IN DWORD dwIndex,
 OUT LPSTR lpName,
 IN DWORD cbName
 )
{
return RegEnumKeyW(hKey,dwIndex, UTF8_MAP(lpName,cbName),cbName );
}

LONG APIENTRY RegEnumKeyExU(
 IN HKEY hKey,
 IN DWORD dwIndex,
 OUT LPSTR lpName,
 IN OUT LPDWORD lpcbName,
 IN LPDWORD lpReserved,
 IN OUT LPSTR lpClass,
 IN OUT LPDWORD lpcbClass,
 OUT PFILETIME lpftLastWriteTime
 )
{
return RegEnumKeyExW(hKey,dwIndex, UTF8_MAP(lpName,*lpcbName),lpcbName,lpReserved, UTF8_MAP(lpClass,*lpcbClass),lpcbClass,lpftLastWriteTime );
}

LONG APIENTRY RegEnumValueU(
 IN HKEY hKey,
 IN DWORD dwIndex,
 OUT LPSTR lpValueName,
 IN OUT LPDWORD lpcbValueName,
 IN LPDWORD lpReserved,
 OUT LPDWORD lpType,
 OUT LPBYTE lpData,
 IN OUT LPDWORD lpcbData
 )
{
return RegEnumValueW(hKey,dwIndex, UTF8_MAP(lpValueName,*lpcbValueName),lpcbValueName,lpReserved,lpType,lpData,lpcbData );
}

LONG APIENTRY RegLoadKeyU(
 IN HKEY hKey,
 IN LPCSTR lpSubKey,
 IN LPCSTR lpFile
 )
{
return RegLoadKeyW(hKey, UTF8_C_MAP(lpSubKey), UTF8_C_MAP(lpFile) );
}

LONG APIENTRY RegOpenKeyU(
 IN HKEY hKey,
 IN LPCSTR lpSubKey,
 OUT PHKEY phkResult
 )
{
return RegOpenKeyW(hKey, UTF8_C_MAP(lpSubKey),phkResult );
}

LONG APIENTRY RegOpenKeyExU(
 IN HKEY hKey,
 IN LPCSTR lpSubKey,
 IN DWORD ulOptions,
 IN REGSAM samDesired,
 OUT PHKEY phkResult
 )
{
return RegOpenKeyExW(hKey, UTF8_C_MAP(lpSubKey),ulOptions,samDesired,phkResult );
}

LONG APIENTRY RegQueryInfoKeyU(
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
 )
{
return RegQueryInfoKeyW(hKey, UTF8_MAP(lpClass,*lpcbClass),lpcbClass,lpReserved,lpcSubKeys,lpcbMaxSubKeyLen,lpcbMaxClassLen,lpcValues,lpcbMaxValueNameLen,lpcbMaxValueLen,lpcbSecurityDescriptor,lpftLastWriteTime );
}

LONG APIENTRY RegQueryValueU(
 IN HKEY hKey,
 IN LPCSTR lpSubKey,
 OUT LPSTR lpValue,
 IN OUT PLONG lpcbValue
 )
{
return RegQueryValueW(hKey, UTF8_C_MAP(lpSubKey), UTF8_MAP(lpValue,*lpcbValue),lpcbValue );
}

/*LONG APIENTRY RegQueryMultipleValuesU(
 IN HKEY hKey,
 OUT PVALENTA val_list,
 IN DWORD num_vals,
 OUT LPSTR lpValueBuf,
 IN OUT LPDWORD ldwTotsize
 )
{
return RegQueryMultipleValuesW(hKey,val_list,num_vals, UTF8_MAP(lpValueBuf),ldwTotsize );
}
*/
LONG APIENTRY RegQueryValueExU(
 IN HKEY hKey,
 IN LPCSTR lpValueName,
 IN LPDWORD lpReserved,
 OUT LPDWORD lpType,
 IN OUT LPBYTE lpData,
 IN OUT LPDWORD lpcbData
 )
{
return RegQueryValueExW(hKey, UTF8_C_MAP(lpValueName),lpReserved,lpType,lpData,lpcbData );
}

LONG APIENTRY RegReplaceKeyU(
 IN HKEY hKey,
 IN LPCSTR lpSubKey,
 IN LPCSTR lpNewFile,
 IN LPCSTR lpOldFile
 )
{
return RegReplaceKeyW(hKey, UTF8_C_MAP(lpSubKey), UTF8_C_MAP(lpNewFile), UTF8_C_MAP(lpOldFile) );
}

LONG APIENTRY RegRestoreKeyU(
 IN HKEY hKey,
 IN LPCSTR lpFile,
 IN DWORD dwFlags
 )
{
return RegRestoreKeyW(hKey, UTF8_C_MAP(lpFile),dwFlags );
}

LONG APIENTRY RegSaveKeyU(
 IN HKEY hKey,
 IN LPCSTR lpFile,
 IN LPSECURITY_ATTRIBUTES lpSecurityAttributes
 )
{
return RegSaveKeyW(hKey, UTF8_C_MAP(lpFile),lpSecurityAttributes );
}

LONG APIENTRY RegSetValueU(
 IN HKEY hKey,
 IN LPCSTR lpSubKey,
 IN DWORD dwType,
 IN LPCSTR lpData,
 IN DWORD cbData
 )
{
return RegSetValueW(hKey, UTF8_C_MAP(lpSubKey),dwType, UTF8_C_MAP(lpData),cbData );
}

LONG APIENTRY RegSetValueExU(
 IN HKEY hKey,
 IN LPCSTR lpValueName,
 IN DWORD Reserved,
 IN DWORD dwType,
 IN CONST BYTE* lpData,
 IN DWORD cbData
 )
{
return RegSetValueExW(hKey, UTF8_C_MAP(lpValueName),Reserved,dwType,lpData,cbData );
}

LONG APIENTRY RegUnLoadKeyU(
 IN HKEY hKey,
 IN LPCSTR lpSubKey
 )
{
return RegUnLoadKeyW(hKey, UTF8_C_MAP(lpSubKey) );
}

BOOL APIENTRY InitiateSystemShutdownU(
 IN LPSTR lpMachineName,
 IN LPSTR lpMessage,
 IN DWORD dwTimeout,
 IN BOOL bForceAppsClosed,
 IN BOOL bRebootAfterShutdown
 )
{
return InitiateSystemShutdownW( (LPWSTR)UTF8_C_MAP(lpMachineName), (LPWSTR)UTF8_C_MAP(lpMessage),dwTimeout,bForceAppsClosed,bRebootAfterShutdown );
}

BOOL APIENTRY AbortSystemShutdownU(
 IN LPSTR lpMachineName
 )
{
return AbortSystemShutdownW( (LPWSTR)UTF8_C_MAP(lpMachineName) );
}

BOOL APIENTRY InitiateSystemShutdownExU(
 IN LPSTR lpMachineName,
 IN LPSTR lpMessage,
 IN DWORD dwTimeout,
 IN BOOL bForceAppsClosed,
 IN BOOL bRebootAfterShutdown,
 IN DWORD dwReason
 )
{
return InitiateSystemShutdownExW( (LPWSTR)UTF8_C_MAP(lpMachineName), (LPWSTR)UTF8_C_MAP(lpMessage),dwTimeout,bForceAppsClosed,bRebootAfterShutdown,dwReason );
}

LONG APIENTRY RegSaveKeyExU(
 IN HKEY hKey,
 IN LPCSTR lpFile,
 IN LPSECURITY_ATTRIBUTES lpSecurityAttributes,
 IN DWORD Flags
 )
{
return RegSaveKeyExW(hKey, UTF8_C_MAP(lpFile),lpSecurityAttributes,Flags );
}

