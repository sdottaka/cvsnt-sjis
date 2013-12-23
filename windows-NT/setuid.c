#define WIN32_LEAN_AND_MEAN
#define _WIN32_WINNT 0x0400
#include <windows.h>
#include <tchar.h>
#include <ntsecapi.h>
#include <lm.h>
#include <malloc.h>

#include "sid.h"

static MAKE_SID1(sidEveryone, 1, 0);
static MAKE_SID1(sidLocal, 2, 0);
static MAKE_SID1(sidNetwork, 5, 2);
static MAKE_SID1(sidBatch, 5, 3);
static MAKE_SID1(sidService, 5, 6);
static MAKE_SID1(sidAnonymous, 5, 7);
static MAKE_SID1(sidInteractive, 5, 4);
static MAKE_SID1(sidAuthenticated, 5, 11);
static MAKE_SID1(sidSystem, 5, 18);
static MAKE_SID2(sidAdministrators, 5, 32, 544);

/*

  This is a funny bit of code... it emulates setuid on NT.
  Any process with 'create token' can become any other user, with a bit of magic.
  (This is normally just LocalSystem).

  I did actually think of this independently, but realised very quickly that the
  cygwin folks had already done something like this.  This is based mostly on
  their implementation, with a bit of re-interpretation of MSDN by me.

  It leaks a bit (not 100% sure why) but it's only called a couple of times
  during the execution of cvs so that's not a problem.

  Prototype:
	void nt_setuid_init();  -- Initialise
	int nt_setuid(LPCSTR szMachine, LPCSTR szUser); -- Do the stuff

*/

typedef enum { false, true } bool;

typedef NTSTATUS (NTAPI *NtCreateToken_t)
		(PHANDLE, /* Address of created token */
		 ACCESS_MASK, /* Access granted to object (TOKEN_ALL_ACCESS) */
		 PLSA_OBJECT_ATTRIBUTES,
	     TOKEN_TYPE, /* Token type (Primary/Impersonation) */
		 PLUID, /* Authentication Id (From GetTokenInformation) or SYSTEM_LUID */
		 PLARGE_INTEGER, /* exp - unknown 0x7FFFFFFFFFFFFFFFLL */
		 PTOKEN_USER,	/* Owner SID for this token */
		 PTOKEN_GROUPS, /* Group SIDs in this token */
		 PTOKEN_PRIVILEGES, /* List of rights for this user */
		 PTOKEN_OWNER, /* Default SID for created objects */
		 PTOKEN_PRIMARY_GROUP, /* Primary group for created objects */
		 PTOKEN_DEFAULT_DACL, /* Default DACL for created objects */
		 PTOKEN_SOURCE /* Source of this token (App name) */
		 );

static NtCreateToken_t NtCreateToken=NULL;

static bool LookupSid(LPCTSTR szMachine, LPCTSTR szUser, PSID* pUserSid, SID_NAME_USE* Use)
{
	DWORD UserSidSize=0,DomainSize=0;
	TCHAR *szDomain = NULL;

	*Use=SidTypeInvalid;

	LookupAccountName(szMachine,szUser,NULL,&UserSidSize,NULL,&DomainSize,NULL);
	if(!UserSidSize || !DomainSize)
		return false;
	*pUserSid=(PSID)malloc(UserSidSize);
	szDomain=(TCHAR*)malloc(DomainSize);
	if(!LookupAccountName(szMachine,szUser,*pUserSid,&UserSidSize,szDomain,&DomainSize,Use))
	{
		free(szDomain);
		free(pUserSid);
		return false;
	}
	free(szDomain);
	return true;
}

void nt_setuid_init()
{
	HMODULE hNtDll;

	hNtDll=LoadLibrary(_T("ntdll.dll"));
	NtCreateToken = (NtCreateToken_t)GetProcAddress(hNtDll,"NtCreateToken");
}

static bool GetPrimaryGroup (LPCWSTR wszMachine, LPCWSTR wszUser, PSID UserSid, PSID* PrimarySid)
{
  LPUSER_INFO_3 buf;
  bool ret = false;
  UCHAR count;

  if (NetUserGetInfo (wszMachine, wszUser, 3, (LPBYTE *) &buf))
    return false;
  *PrimarySid=malloc(_msize(UserSid));
  CopySid(_msize(UserSid),*PrimarySid,UserSid);
  if (IsValidSid (*PrimarySid) && (count = *GetSidSubAuthorityCount (*PrimarySid)) > 1)
  {
      *GetSidSubAuthority (*PrimarySid, count - 1) = buf->usri3_primary_group_id;
      ret = true;
  }
  NetApiBufferFree (buf);
  return ret;
}

static bool GetDacl(PACL* pAcl, PSID UserSid, PTOKEN_GROUPS Groups)
{
	int n;

	*pAcl = (PACL)malloc(4096);
	if (!InitializeAcl(*pAcl, 4096, ACL_REVISION))
	{
		free(*pAcl);
		return false;
	}
	for(n=0; n<(int)Groups->GroupCount; n++)
	{
		if(EqualSid(Groups->Groups[n].Sid,&sidAdministrators))
		{
			if (!AddAccessAllowedAce(*pAcl, ACL_REVISION, GENERIC_ALL, &sidAdministrators))
			{
				free(pAcl);
				return false;
			}
			break;
		}
    }
	if (!AddAccessAllowedAce(*pAcl, ACL_REVISION, GENERIC_ALL, UserSid))
	{
		free(pAcl);
		return false;
	}

	if (!AddAccessAllowedAce(*pAcl, ACL_REVISION, GENERIC_ALL, &sidSystem))
	{
		free(pAcl);
		return false;
	}
	return true;
}

static bool GetAuthLuid(LUID* pLuid)
{
	HANDLE hToken;
	TOKEN_STATISTICS stats;
	DWORD size;

	if (!OpenProcessToken (GetCurrentProcess (), TOKEN_QUERY, &hToken))
		return false;
	if (!GetTokenInformation (hToken, TokenStatistics, &stats, sizeof stats, &size))
		return false; 

	*pLuid=stats.AuthenticationId; 
	return true;
}

#ifndef _UNICODE
int nt_setuid(LPCTSTR szMachine, LPCTSTR szUser)
#else
int nt_setuid(LPCTSTR wszMachine, LPCTSTR wszUser)
#endif
{
	int retval = -1;
	PSID UserSid = NULL, pTmpSid;
#ifndef _UNICODE
	LPWSTR wszMachine=NULL,wszUser=NULL;
#define tszMachine szMachine
#define tszUser szUser
#else
#define tszMachine wszMachine
#define tszUser wszUser
#endif
	LPGROUP_USERS_INFO_0 GlobalGroups = NULL;
	LPGROUP_USERS_INFO_0 LocalGroups = NULL;
	DWORD NumGlobalGroups=0,TotalGlobalGroups=0;
	DWORD NumLocalGroups=0,TotalLocalGroups=0;
	TOKEN_USER _TokenUser = {0};
	TOKEN_OWNER _TokenOwner = {0};
	PTOKEN_GROUPS pTokenGroups = NULL;
	PTOKEN_PRIVILEGES TokenPrivs = NULL;
	TOKEN_PRIMARY_GROUP _TokenPrimaryGroup = {0};
	TOKEN_SOURCE _TokenSource = {0};
	TOKEN_DEFAULT_DACL TokenDacl = {0};
	TCHAR grName[128];
	int n,j,p,q;
	SID_NAME_USE Use;
	LSA_OBJECT_ATTRIBUTES lsa = { sizeof(LSA_OBJECT_ATTRIBUTES) };
	LSA_HANDLE hLsa=NULL;
	LSA_UNICODE_STRING lsaMachine;
	PLSA_UNICODE_STRING lsaUserRights;
	DWORD NumUserRights;
	LUID AuthLuid;
	HANDLE hToken = INVALID_HANDLE_VALUE;
	HANDLE hPrimaryToken = INVALID_HANDLE_VALUE;
	HANDLE hProcessToken = INVALID_HANDLE_VALUE;
	int max_sid,identifier_sid;
	
	SECURITY_QUALITY_OF_SERVICE sqos =
		{ sizeof sqos, SecurityImpersonation, SECURITY_STATIC_TRACKING, FALSE };
	LSA_OBJECT_ATTRIBUTES oa =
		{ sizeof oa, 0, 0, 0, 0, &sqos };
	SECURITY_ATTRIBUTES sa = { sizeof sa, NULL, TRUE };
	LARGE_INTEGER exp = { 0xffffffff,0x7fffffff } ;

	PTOKEN_PRIVILEGES NewToken;
	DWORD NewTokenLength;
	LUID TempLuid;

	/* If init failed, or not called, then exit immediately */
	if(!NtCreateToken)
	   return -1;

#ifdef _UNICODE
	if(wszMachine)
	{
		lsaMachine.Length=_tcslen(wszMachine);
		lsaMachine.MaximumLength=_tcslen(wszMachine);
		lsaMachine.Buffer=(TCHAR*)wszMachine;
#else
	if(szMachine)
	{
		wszMachine=(LPWSTR)malloc(strlen(szMachine)*2+2);
		MultiByteToWideChar(0,0,szMachine,-1,wszMachine,strlen(szMachine)+1);

		lsaMachine.Length=strlen(szMachine)*2+2;
		lsaMachine.MaximumLength=strlen(szMachine)*2+2;
		lsaMachine.Buffer=wszMachine;
#endif
	}
#ifndef _UNICODE
	wszUser=(LPWSTR)malloc(strlen(szUser)*2+2);
	MultiByteToWideChar(0,0,szUser,-1,wszUser,strlen(szUser)+1);
#endif

	if(!LookupPrivilegeValue(NULL,SE_CREATE_TOKEN_NAME,&TempLuid))
	{
		goto nt_setuid_out;
	}
	if(!OpenProcessToken(GetCurrentProcess(),TOKEN_ADJUST_PRIVILEGES|TOKEN_QUERY,&hProcessToken))
	{
		goto nt_setuid_out;
	}
	GetTokenInformation(hProcessToken,TokenPrivileges,NULL,0,&NewTokenLength);
	NewToken=(PTOKEN_PRIVILEGES)malloc(NewTokenLength);
	GetTokenInformation(hProcessToken,TokenPrivileges,NewToken,NewTokenLength,&NewTokenLength);
	for(n=0; n<(int)NewToken->PrivilegeCount; n++)
	{
		if(!memcmp(&NewToken->Privileges[n].Luid,&TempLuid,sizeof(LUID)))
		{
			NewToken->Privileges[n].Attributes=SE_PRIVILEGE_ENABLED;
			break;
		}
	}
	if(n==(int)NewToken->PrivilegeCount)
	{
		goto nt_setuid_out;
	}
	if(!AdjustTokenPrivileges(hProcessToken,FALSE,NewToken,0,0,NULL))
	{
		goto nt_setuid_out;
	}

	if(LsaOpenPolicy(wszMachine?&lsaMachine:NULL,&lsa,POLICY_LOOKUP_NAMES,&hLsa)!=ERROR_SUCCESS)
	{
		goto nt_setuid_out;
	}

	/* From Q185246 - username = domain name */
	wsprintf(grName,_T("%s\\%s"),tszUser,tszUser); // Used for domain/user clashes

	/* Search on the specified PDC, then on the local domain, for the user.
	   This allows for trusted domains to work */
	if((!LookupSid(tszMachine,tszUser,&UserSid,&Use) || Use!=SidTypeUser) &&
	   (!LookupSid(tszMachine,grName,&UserSid,&Use) || Use!=SidTypeUser) &&
	   (!LookupSid(NULL,tszUser,&UserSid,&Use) || Use!=SidTypeUser) &&
	   (!LookupSid(NULL,grName,&UserSid,&Use) || Use!=SidTypeUser))
	{
		goto nt_setuid_out;
	}

	_TokenOwner.Owner=UserSid;
	_TokenUser.User.Attributes=0;
	_TokenUser.User.Sid=UserSid;


	NetUserGetGroups(wszMachine,wszUser,0,(LPBYTE*)&GlobalGroups,MAX_PREFERRED_LENGTH,&NumGlobalGroups,&TotalGlobalGroups);
	NetUserGetLocalGroups(wszMachine,wszUser,0,0,(LPBYTE*)&LocalGroups,MAX_PREFERRED_LENGTH,&NumLocalGroups,&TotalLocalGroups);

	pTokenGroups = (PTOKEN_GROUPS)malloc(sizeof(TOKEN_GROUPS)+sizeof(SID_AND_ATTRIBUTES)*(NumGlobalGroups + NumLocalGroups + NumGlobalGroups + 5));
	pTokenGroups->GroupCount = NumGlobalGroups + NumLocalGroups + 5;

	for(n=0,j=0; n<(int)NumGlobalGroups; n++)
	{
#ifndef _UNICODE
		WideCharToMultiByte(0,0,GlobalGroups[n].grui0_name,-1,grName,sizeof(grName),NULL,NULL);
#else
		_tcsncpy(grName,GlobalGroups[n].grui0_name,sizeof(grName));
#endif
		if(LookupSid(tszMachine,grName,&pTmpSid,&Use))
		{
			pTokenGroups->Groups[j].Attributes=SE_GROUP_ENABLED|SE_GROUP_ENABLED_BY_DEFAULT;
			pTokenGroups->Groups[j].Sid=pTmpSid;
			j++;
		}
	}
	for(n=0; n<(int)NumLocalGroups; n++)
	{
#ifndef _UNICODE
		WideCharToMultiByte(0,0,LocalGroups[n].grui0_name,-1,grName,sizeof(grName),NULL,NULL);
#else
		_tcsncpy(grName,LocalGroups[n].grui0_name,sizeof(grName));
#endif
		if(LookupSid(tszMachine,grName,&pTmpSid,&Use))
		{
			pTokenGroups->Groups[j].Attributes=SE_GROUP_ENABLED|SE_GROUP_ENABLED_BY_DEFAULT|SE_GROUP_MANDATORY;
			pTokenGroups->Groups[j].Sid=pTmpSid;
			j++;
		}
	}

	max_sid=j;

	if(!GetAuthLuid(&AuthLuid))
	{
		goto nt_setuid_out;
	}
	if(!GetPrimaryGroup(wszMachine,wszUser,UserSid,&_TokenPrimaryGroup.PrimaryGroup))
	{
		goto nt_setuid_out;
	}

	pTokenGroups->Groups[j].Attributes=SE_GROUP_ENABLED|SE_GROUP_ENABLED_BY_DEFAULT|SE_GROUP_MANDATORY;
	pTokenGroups->Groups[j].Sid=&sidLocal;
	j++;
	pTokenGroups->Groups[j].Attributes=SE_GROUP_ENABLED|SE_GROUP_ENABLED_BY_DEFAULT|SE_GROUP_MANDATORY;
	pTokenGroups->Groups[j].Sid=&sidInteractive;
	j++;
	pTokenGroups->Groups[j].Attributes=SE_GROUP_ENABLED|SE_GROUP_ENABLED_BY_DEFAULT|SE_GROUP_MANDATORY;
	pTokenGroups->Groups[j].Sid=&sidAuthenticated;
	j++;
	pTokenGroups->Groups[j].Attributes=SE_GROUP_ENABLED|SE_GROUP_ENABLED_BY_DEFAULT|SE_GROUP_MANDATORY;
	pTokenGroups->Groups[j].Sid=&sidEveryone;
	j++;
	{
		PSID pUserSid;
		SID_IDENTIFIER_AUTHORITY nt = SECURITY_NT_AUTHORITY;
		AllocateAndInitializeSid(&nt,3,SECURITY_LOGON_IDS_RID,AuthLuid.HighPart,AuthLuid.LowPart,0,0,0,0,0,&pUserSid);
		pTokenGroups->Groups[j].Attributes=SE_GROUP_LOGON_ID|SE_GROUP_ENABLED|SE_GROUP_ENABLED_BY_DEFAULT|SE_GROUP_MANDATORY;
		pTokenGroups->Groups[j].Sid=pUserSid;
		identifier_sid = j;
		j++;
	}


	pTokenGroups->GroupCount = j;

	TokenPrivs=(PTOKEN_PRIVILEGES)calloc(1,sizeof(TOKEN_PRIVILEGES));
	if(LsaEnumerateAccountRights(hLsa,UserSid,&lsaUserRights,&NumUserRights)==ERROR_SUCCESS)
	{
		TokenPrivs->PrivilegeCount=NumUserRights;
		TokenPrivs=(PTOKEN_PRIVILEGES)realloc(TokenPrivs,sizeof(TOKEN_PRIVILEGES)+sizeof(LUID_AND_ATTRIBUTES)*TokenPrivs->PrivilegeCount);

		for(n=0,j=0; n<(int)NumUserRights; n++)
		{
			TokenPrivs->Privileges[j].Attributes=SE_PRIVILEGE_ENABLED | SE_PRIVILEGE_ENABLED_BY_DEFAULT;
#ifndef _UNICODE
			WideCharToMultiByte(0,0,lsaUserRights->Buffer,-1,grName,sizeof(grName),NULL,NULL);
#else
			_tcsncpy(grName,lsaUserRights->Buffer,sizeof(grName));
#endif
			LookupPrivilegeValue(tszMachine,grName,&TokenPrivs->Privileges[j].Luid);
			j++;
		}
		NetApiBufferFree(lsaUserRights);
	}

	for(n=0; n<(int)pTokenGroups->GroupCount; n++)
	{
		if(LsaEnumerateAccountRights(hLsa,pTokenGroups->Groups[n].Sid,&lsaUserRights,&NumUserRights)==ERROR_SUCCESS)
		{
			TokenPrivs->PrivilegeCount+=NumUserRights;
			TokenPrivs=(PTOKEN_PRIVILEGES)realloc(TokenPrivs,sizeof(TOKEN_PRIVILEGES)+sizeof(LUID_AND_ATTRIBUTES)*TokenPrivs->PrivilegeCount);
			for(p=0; p<(int)NumUserRights; p++)
			{
				LUID luid;
				TokenPrivs->Privileges[p].Attributes=SE_PRIVILEGE_ENABLED | SE_PRIVILEGE_ENABLED_BY_DEFAULT;
#ifndef _UNICODE
				WideCharToMultiByte(0,0,lsaUserRights[p].Buffer,-1,grName,sizeof(grName),NULL,NULL);
#else
				_tcsncpy(grName,lsaUserRights[p].Buffer,sizeof(grName));
#endif
				if(!LookupPrivilegeValue(tszMachine,grName,&luid))
					continue;
				for(q=0; q<j; q++)
					if(!memcmp(&luid,&TokenPrivs->Privileges[q].Luid,sizeof(luid)))
						break;
				if(q==j)
				{
					TokenPrivs->Privileges[p].Luid=luid;
					j++;
				}
			}
			NetApiBufferFree(lsaUserRights);
		}
	}

	TokenPrivs->PrivilegeCount=j;
	TokenPrivs=(PTOKEN_PRIVILEGES)realloc(TokenPrivs,sizeof(TOKEN_PRIVILEGES)+sizeof(LUID_AND_ATTRIBUTES)*TokenPrivs->PrivilegeCount);

	memset(&_TokenSource,0,sizeof(_TokenSource));
	strcpy(_TokenSource.SourceName,"cvsnt");
	_TokenSource.SourceIdentifier.HighPart = 0;
	_TokenSource.SourceIdentifier.LowPart = 0x0101;

	if(!GetDacl(&TokenDacl.DefaultDacl,UserSid,pTokenGroups))
	{
		goto nt_setuid_out;
	}

	if(NtCreateToken (&hToken, TOKEN_ALL_ACCESS, &oa, TokenImpersonation,
		   &AuthLuid, &exp, &_TokenUser, pTokenGroups, TokenPrivs, &_TokenOwner, &_TokenPrimaryGroup,
		   &TokenDacl, &_TokenSource)!=ERROR_SUCCESS)
	{
		goto nt_setuid_out;
	}

	if(!DuplicateTokenEx(hToken,TOKEN_ALL_ACCESS,&sa,SecurityImpersonation,TokenPrimary,&hPrimaryToken))
	{
		goto nt_setuid_out;
	}

	RevertToSelf();
	if(!ImpersonateLoggedOnUser(hPrimaryToken))
	{
		goto nt_setuid_out;
	}

	retval = 0;

nt_setuid_out:
	free(UserSid);
#ifndef _UNICODE
	free(wszMachine);
	free(wszUser);
#endif
	NetApiBufferFree(GlobalGroups);
	NetApiBufferFree(LocalGroups);
	if(pTokenGroups)
	{
		for(n=0; n<(int)max_sid; n++)
			free(pTokenGroups->Groups[n].Sid);
		FreeSid(pTokenGroups->Groups[identifier_sid].Sid);
	} 
	free(pTokenGroups);
	free(TokenPrivs); 
	free(_TokenPrimaryGroup.PrimaryGroup); 
	free(NewToken);
	if(hToken!=INVALID_HANDLE_VALUE)
		CloseHandle(hToken);
	if(hProcessToken!=INVALID_HANDLE_VALUE)
		CloseHandle(hProcessToken);
	if(hPrimaryToken!=INVALID_HANDLE_VALUE)
		CloseHandle(hPrimaryToken);
	free(TokenDacl.DefaultDacl);
	LsaClose(hLsa); 
	return retval;
}



