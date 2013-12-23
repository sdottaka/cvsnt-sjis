// setuid.cpp : Defines the entry point for the DLL application.
//

// Add the dll name (setuid) to
// HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\Lsa\Authentication Packages
// and copy to the windows system directory.

#include "stdafx.h"
#include "setuid.h"
#include "LsaSetuid.h"

PLSA_SECPKG_FUNCTION_TABLE g_pSec;

BOOL APIENTRY DllMain( HANDLE hModule, 
                       DWORD  ul_reason_for_call, 
                       LPVOID lpReserved
					 )
{
    return TRUE;
}

NTSTATUS NTAPI LsaApCallPackage(
  PLSA_CLIENT_REQUEST ClientRequest,
  PVOID ProtocolAuthenticationInformation,
  PVOID ClientBufferBase,
  ULONG AuthenticationInformationLength,
  PVOID* ProtocolReturnBuffer,
  PULONG ReturnBufferLength,
  PNTSTATUS ProtocolStatus
)
{
	return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS NTAPI LsaApCallPackageUntrusted(
  PLSA_CLIENT_REQUEST ClientRequest,
  PVOID ProtocolAuthenticationInformation,
  PVOID ClientBufferBase,
  ULONG AuthenticationInformationLength,
  PVOID* ProtocolReturnBuffer,
  PULONG ReturnBufferLength,
  PNTSTATUS ProtocolStatus
)
{
	return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS NTAPI LsaApCallPackagePassthrough(
  PLSA_CLIENT_REQUEST ClientRequest,
  PVOID ProtocolAuthenticationInformation,
  PVOID ClientBufferBase,
  ULONG AuthenticationInformationLength,
  PVOID* ProtocolReturnBuffer,
  PULONG ReturnBufferLength,
  PNTSTATUS ProtocolStatus
)
{
	return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS NTAPI LsaApInitializePackage(
  ULONG AuthenticationPackageId,
  PLSA_DISPATCH_TABLE LsaDispatchTable,
  PLSA_STRING Database,
  PLSA_STRING Confidentiality,
  PLSA_STRING* AuthenticationPackageName
)
{
	/* The cast isn't documented, but it's really the same structure */
	g_pSec = (PLSA_SECPKG_FUNCTION_TABLE)LsaDispatchTable;

	(*AuthenticationPackageName) = AllocateLsaStringLsa(SETUID_PACKAGENAME);
	return STATUS_SUCCESS;
}

NTSTATUS NTAPI LsaApLogonUser(
  PLSA_CLIENT_REQUEST ClientRequest,
  SECURITY_LOGON_TYPE LogonType,
  PVOID AuthenticationInformation,
  PVOID ClientAuthenticationBase,
  ULONG AuthenticationInformationLength,
  PVOID* ProfileBuffer,
  PULONG ProfileBufferLength,
  PLUID LogonId,
  PNTSTATUS SubStatus,
  PLSA_TOKEN_INFORMATION_TYPE TokenInformationType,
  PVOID* TokenInformation,
  PUNICODE_STRING* AccountName,
  PUNICODE_STRING* AuthenticatingAuthority
)
{
	PLSA_TOKEN_INFORMATION_V2 LocalTokenInformation;
	NTSTATUS err;
	wchar_t wszDomain[DNLEN+1] ={0};
	wchar_t wszUser[UNLEN+1];
	USER_INFO_1 *ui1 = NULL;
	PDOMAIN_CONTROLLER_INFOW pdi = NULL;

	DEBUG(L"Setuid (%S) started\n", __DATE__);
	if(AuthenticationInformationLength!=sizeof(SetUidParms))
	{
		DEBUG(L"AuthenticationInformationLength != sizeof(SetUidParms_In): INVALID_PARAMETER\n");
		return STATUS_INVALID_PARAMETER;
	}

	SetUidParms *in = (SetUidParms *)AuthenticationInformation;

	if(in->Command!=SETUID_BECOME_USER)
	{
		DEBUG(L"in->Command!=SETUID_BECOME_USER: INVALID_PARAMETER\n");
		return STATUS_INVALID_PARAMETER;
	}

	/* Username is always domain\user */
	if(!in->BecomeUser.Username || !wcschr(in->BecomeUser.Username, '\\'))
	{
		DEBUG(L"in->Username invalid (%s): INVALID_PARAMETER\n",in->BecomeUser.Username?in->BecomeUser.Username:L"null");
		return STATUS_INVALID_PARAMETER;
	}

	wcsncpy(wszDomain,in->BecomeUser.Username,wcschr(in->BecomeUser.Username,'\\')-in->BecomeUser.Username);
	wcscpy(wszUser,wcschr(in->BecomeUser.Username,'\\')+1);

	if(AccountName)
		*AccountName = AllocateUnicodeStringLsa(wszUser);

	if(AuthenticatingAuthority)
	{
		if(wszDomain[0])
			*AuthenticatingAuthority = AllocateUnicodeStringLsa(wszDomain);
		else
		{
			DWORD dwLen = sizeof(wszDomain);
			GetComputerNameW(wszDomain,&dwLen);
			*AuthenticatingAuthority = AllocateUnicodeStringLsa(wszDomain);
			wszDomain[0]='\0';
		}
	}

	if(wszDomain[0] && DsGetDcNameW(NULL,wszDomain,NULL,NULL,DS_IS_FLAT_NAME,&pdi) && DsGetDcNameW(NULL,wszDomain,NULL,NULL,DS_IS_DNS_NAME,&pdi))
	{
		DEBUG(L"Domain not found (%s): NO_SUCH_DOMAIN\n",wszDomain);
		if(pdi) NetApiBufferFree(pdi);
		return STATUS_NO_SUCH_DOMAIN;
	}

	if(NetUserGetInfo(pdi?pdi->DomainControllerName:NULL, wszUser, 1, (LPBYTE*)&ui1))
	{
		DEBUG(L"User not found (%s): NO_SUCH_USER\n",wszUser);
		if(pdi) NetApiBufferFree(pdi);
		if(ui1) NetApiBufferFree(ui1);
		return STATUS_NO_SUCH_USER;
	}

	if(ui1->usri1_flags&UF_ACCOUNTDISABLE)
	{
		DEBUG(L"Account disabled: ACCOUNT_DISABLED\n");
		*SubStatus = STATUS_ACCOUNT_DISABLED;
		if(pdi) NetApiBufferFree(pdi);
		if(ui1) NetApiBufferFree(ui1);
		return STATUS_ACCOUNT_RESTRICTION;
	}
	if(ui1->usri1_flags&UF_PASSWORD_EXPIRED)
	{
		DEBUG(L"Password expired: PASSWORD_EXPIRED\n");
		*SubStatus = STATUS_PASSWORD_EXPIRED;
		if(pdi) NetApiBufferFree(pdi);
		if(ui1) NetApiBufferFree(ui1);
		return STATUS_ACCOUNT_RESTRICTION;
	}
	if(ui1->usri1_flags&(UF_WORKSTATION_TRUST_ACCOUNT|UF_SERVER_TRUST_ACCOUNT|UF_INTERDOMAIN_TRUST_ACCOUNT))
	{
		DEBUG(L"Is a trust account: ACCOUNT_RESTRICTION\n");
		*SubStatus = STATUS_ACCOUNT_DISABLED;
		if(pdi) NetApiBufferFree(pdi);
		if(ui1) NetApiBufferFree(ui1);
		return STATUS_ACCOUNT_RESTRICTION;
	}
	NetApiBufferFree(ui1);

	if(!AllocateLocallyUniqueId(LogonId))
	{
		DEBUG(L"AllocateLocallyUniqueId failed (%d)\n",GetLastError());
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	err = GetTokenInformationv2(pdi?pdi->DomainControllerName:NULL,wszDomain,wszUser,&LocalTokenInformation,LogonId);

	if(err) 
	{
		DEBUG(L"GetTokenInformationv2 failed (%08x)\n",err);
		if(pdi) NetApiBufferFree(pdi);
		return err;
	}
	if(pdi) NetApiBufferFree(pdi);

	err = g_pSec->CreateLogonSession(LogonId);
	if(err)
	{
		DEBUG(L"CreateLogonSession failed (%d)\n",err);
		if(LocalTokenInformation)
			NetApiBufferFree(LocalTokenInformation);
		return err;
	}

	if(ProfileBuffer)
	{
		*ProfileBuffer=NULL;
		*ProfileBufferLength=0;
	}

	(*TokenInformationType)=LsaTokenInformationV2;
	(*TokenInformation)=LocalTokenInformation;

	DEBUG(L"Setuid Logon for %s\\%s returned OK\n", wszDomain, wszUser);

	return STATUS_SUCCESS;
}

void NTAPI LsaApLogonTerminated(
  PLUID LogonId
)
{
	DEBUG(L"Setuid logon deleted\n");
}
