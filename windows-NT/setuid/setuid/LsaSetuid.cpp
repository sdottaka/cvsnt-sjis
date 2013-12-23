#include "stdafx.h"
#include "setuid.h"
#include "sid.h"
#include "LsaSetuid.h"

DEFINE_SID1(sidEveryone, 1, 0);
DEFINE_SID1(sidLocal, 2, 0);
DEFINE_SID1(sidNetwork, 5, 2);
DEFINE_SID1(sidBatch, 5, 3);
DEFINE_SID1(sidService, 5, 6);
DEFINE_SID1(sidAnonymous, 5, 7);
DEFINE_SID1(sidInteractive, 5, 4);
DEFINE_SID1(sidAuthenticated, 5, 11);
DEFINE_SID1(sidSystem, 5, 18);
DEFINE_SID2(sidAdministrators, 5, 32, 544);

static bool LookupSid(LPCWSTR szSid, PSID* pUserSid, SID_NAME_USE* Use);
static bool GetPrimaryGroup(LPCWSTR wszMachine, LPCWSTR wszUser, PSID UserSid, PSID* PrimarySid);
static bool GetDacl(PACL* pAcl, PSID UserSid, PTOKEN_GROUPS Groups);

LPBYTE g_HeapPtr, g_HeapBase;
#define HEAP_SIZE 4096

/*
	This is Win2K LSA Setuid code, based on CVSNT Setuid, which is based on Cygwin Setuid.
*/

void SuidDebug(const wchar_t *fmt, ...)
{
	static wchar_t buf[4096]={0};
	va_list va;
	va_start(va,fmt);
	buf[_vsnwprintf(buf,sizeof(buf),fmt,va)]='\0';
	va_end(va);
	OutputDebugString(buf);
}

LPVOID LsaAllocateLsa(ULONG size)
{
	return g_pSec->AllocateLsaHeap(size);
}

LPVOID LsaAllocateHeap(ULONG size)
{
	LPVOID p;

	if(g_HeapPtr + size > g_HeapBase + HEAP_SIZE)
	{
		DEBUG(L"Out of reserved heap space.  Increase HEAP_SIZE and recompile.");
		return NULL;
	}
	p = g_HeapPtr;
	g_HeapPtr += size;
	DEBUG(L"LsaAllocateHeap(%d) - Heapsize %d\n",size,g_HeapPtr-g_HeapBase);
	return p; 
}

void LsaFreeLsa(LPVOID p)
{
	g_pSec->FreeLsaHeap(p);
}

LSA_STRING *AllocateLsaStringLsa(LPCSTR szString)
{
	LSA_STRING *s;
	size_t len = strlen(szString);

	s = (LSA_STRING *) LsaAllocateLsa(sizeof(LSA_STRING));
	if(!s)
		return NULL;
	s->Buffer = (char *) LsaAllocateLsa((ULONG) len+1);
	s->Length = (USHORT)len;
	s->MaximumLength = (USHORT)len+1;
	strcpy(s->Buffer,  szString);
	return s;
}

UNICODE_STRING *AllocateUnicodeStringHeap(LPCWSTR szString)
{
	UNICODE_STRING *s;
	size_t len = wcslen(szString)*sizeof(wchar_t);

	s = (UNICODE_STRING *) LsaAllocateHeap((ULONG)sizeof(UNICODE_STRING)+len+sizeof(wchar_t));
	if(!s)
		return NULL;
	s->Buffer = (wchar_t *) (s+1);
	s->Length = (USHORT)len;
	s->MaximumLength = (USHORT)len+sizeof(wchar_t);
	wcscpy(s->Buffer,  szString);
	return s;
}

UNICODE_STRING *AllocateUnicodeStringLsa(LPCWSTR szString)
{
	UNICODE_STRING *s;
	size_t len = wcslen(szString)*sizeof(wchar_t);

	s = (UNICODE_STRING *) LsaAllocateLsa((ULONG)sizeof(UNICODE_STRING));
	if(!s)
		return NULL;
	s->Buffer = (wchar_t *) LsaAllocateLsa((ULONG)len+sizeof(wchar_t));
	s->Length = (USHORT)len;
	s->MaximumLength = (USHORT)len+sizeof(wchar_t);
	wcscpy(s->Buffer,  szString);
	return s;
}

bool LookupSid(LPCWSTR szSid, PSID* pUserSid, SID_NAME_USE* Use)
{
	DWORD UserSidSize=0,DomainSize=0;
	wchar_t szDomain[DNLEN];

	*Use=SidTypeInvalid;

	LookupAccountNameW(NULL,szSid,NULL,&UserSidSize,NULL,&DomainSize,NULL);
	if(!UserSidSize)
		return false;
	*pUserSid=(PSID)LsaAllocateHeap(UserSidSize);
	if(!LookupAccountNameW(NULL,szSid,*pUserSid,&UserSidSize,szDomain,&DomainSize,Use))
	{
		free(pUserSid);
		return false;
	}
	return true;
}

bool GetPrimaryGroup (LPCWSTR wszMachine, LPCWSTR wszUser, PSID UserSid, PSID* PrimarySid)
{
  LPUSER_INFO_3 buf;
  bool ret = false;
  UCHAR count;
  ULONG size = GetLengthSid(UserSid);
  DWORD err;

  if ((err=NetUserGetInfo(wszMachine, wszUser, 3, (LPBYTE *) &buf))!=0)
  {
		DEBUG(L"NetUserGetInfo failed - error %d\n",err);
		return false;
  }
  *PrimarySid=(PSID)LsaAllocateHeap(size);
  CopySid(size,*PrimarySid,UserSid);
  if (IsValidSid(*PrimarySid) && (count = *GetSidSubAuthorityCount (*PrimarySid)) > 1)
      *GetSidSubAuthority (*PrimarySid, count - 1) = buf->usri3_primary_group_id;
  ret = true;
  NetApiBufferFree (buf);
  return ret;
}

bool GetDacl(PACL* pAcl, PSID UserSid, PTOKEN_GROUPS Groups)
{
	int n;
	ULONG daclSize = sizeof(ACL)+sizeof(ACCESS_ALLOWED_ACE)*3+GetLengthSid(UserSid)+sizeof(sidAdministrators)+sizeof(sidSystem)+64;

	*pAcl = (PACL)LsaAllocateHeap(daclSize);
	if (!InitializeAcl(*pAcl, daclSize, ACL_REVISION))
	{
		return false;
	}
	for(n=0; n<(int)Groups->GroupCount; n++)
	{
		if(EqualSid(Groups->Groups[n].Sid,&sidAdministrators))
		{
			if (!AddAccessAllowedAce(*pAcl, ACL_REVISION, GENERIC_ALL, &sidAdministrators))
			{
				return false;
			}
			break;
		}
    }
	if (!AddAccessAllowedAce(*pAcl, ACL_REVISION, GENERIC_ALL, UserSid))
	{
		return false;
	}

	if (!AddAccessAllowedAce(*pAcl, ACL_REVISION, GENERIC_ALL, &sidSystem))
	{
		return false;
	}
	return true;
}

/* We must use LSA_TOKEN_INFORMATION_V2 because in XP there's a bug in the LSA that corrupts
   its own heap if you use the _V1 variant.  _V2 expects all its information in a single block of
   data, which it frees all at once - the structure is otherwise identical */
NTSTATUS GetTokenInformationv2(LPCWSTR wszMachine,LPCWSTR wszDomain, LPCWSTR wszUser,LSA_TOKEN_INFORMATION_V2** TokenInformation, PLUID LogonId)
{
	int retval;
	PSID UserSid = NULL, pTmpSid;
	LPGROUP_USERS_INFO_0 GlobalGroups = NULL;
	LPGROUP_USERS_INFO_0 LocalGroups = NULL;
	DWORD NumGlobalGroups=0,TotalGlobalGroups=0;
	DWORD NumLocalGroups=0,TotalLocalGroups=0;
	PTOKEN_GROUPS pTokenGroups = NULL;
	PTOKEN_PRIVILEGES TokenPrivs = NULL;
	wchar_t grName[256+256];
	int n,j,p,q;
	SID_NAME_USE Use;
	LSA_OBJECT_ATTRIBUTES lsa = { sizeof(LSA_OBJECT_ATTRIBUTES) };
	PUNICODE_STRING lsaUserRights;
	DWORD NumUserRights;
	PUNICODE_STRING lsaMachine;
	LSA_HANDLE hLsa=NULL;
	SID_IDENTIFIER_AUTHORITY nt = SECURITY_NT_AUTHORITY;

	try
	{
		*TokenInformation = (LSA_TOKEN_INFORMATION_V2*)LsaAllocateLsa(sizeof(LSA_TOKEN_INFORMATION_V2)+HEAP_SIZE);
		g_HeapPtr = g_HeapBase = (LPBYTE)((*TokenInformation)+1);

		g_pSec->ImpersonateClient();
		if(wszMachine && wszMachine[0])
		{
			lsaMachine = AllocateUnicodeStringLsa(wszMachine);
			DEBUG(L"Querying LSA database on %s\n",wszMachine);
			if((retval=LsaOpenPolicy(lsaMachine,&lsa,POLICY_EXECUTE,&hLsa))!=STATUS_SUCCESS)
			{
				DEBUG(L"LsaOpenPolicy (%s) failed (%08x:%d)\n",wszMachine,retval,LsaNtStatusToWinError(retval));
				LsaFreeLsa(lsaMachine->Buffer);
				LsaFreeLsa(lsaMachine);
				throw retval;
			}
		}
		else 
		{
			DEBUG(L"Querying local LSA database\n");
			if((retval=LsaOpenPolicy(NULL,&lsa,POLICY_EXECUTE,&hLsa))!=STATUS_SUCCESS)
			{
				DEBUG(L"LsaOpenPolicy (local) failed (%08x:%d)\n",retval,LsaNtStatusToWinError(retval));
				throw retval;
			}
		}

		if(!wszDomain || !wszDomain[0])
			wcscpy(grName,wszUser);
		else
			wsprintfW(grName,L"%s\\%s",wszDomain,wszUser); // Used for domain/user clashes

		/* Search on the specified PDC, then on the local domain, for the user.
		This allows for trusted domains to work */
		if(!LookupSid(grName,&UserSid,&Use) || Use!=SidTypeUser)
		{
			DEBUG(L"LookupSid\n");
			retval = STATUS_NO_SUCH_USER;
			throw retval;
		}

		(*TokenInformation)->Owner.Owner=NULL;
		(*TokenInformation)->User.User.Attributes=0;
		(*TokenInformation)->User.User.Sid=UserSid;

		NetUserGetGroups(wszMachine,wszUser,0,(LPBYTE*)&GlobalGroups,MAX_PREFERRED_LENGTH,&NumGlobalGroups,&TotalGlobalGroups);
		NetUserGetLocalGroups(wszMachine,wszUser,0,0,(LPBYTE*)&LocalGroups,MAX_PREFERRED_LENGTH,&NumLocalGroups,&TotalLocalGroups);

		pTokenGroups = (PTOKEN_GROUPS)LsaAllocateHeap(sizeof(TOKEN_GROUPS)+sizeof(SID_AND_ATTRIBUTES)*(NumGlobalGroups + NumLocalGroups + NumGlobalGroups));
		pTokenGroups->GroupCount = NumGlobalGroups + NumLocalGroups;

		j=0;
		for(n=0; n<(int)NumLocalGroups; n++)
		{
			if(LookupSid(LocalGroups[n].grui0_name,&pTmpSid,&Use))
			{
				if(memcmp(GetSidIdentifierAuthority(pTmpSid),&nt,sizeof(nt)) ||
				   *GetSidSubAuthority(pTmpSid,0)!=SECURITY_BUILTIN_DOMAIN_RID)
				{
					pTokenGroups->Groups[j].Attributes=SE_GROUP_ENABLED|SE_GROUP_ENABLED_BY_DEFAULT|SE_GROUP_RESOURCE;
				}
				else
					pTokenGroups->Groups[j].Attributes=0;
				pTokenGroups->Groups[j].Sid=pTmpSid;
				DEBUG(L"Adding local group (%d) %s\n",j,LocalGroups[n].grui0_name);
				j++;
			}
		}

		for(n=0; n<(int)NumGlobalGroups; n++)
		{
			if(LookupSid(GlobalGroups[n].grui0_name,&pTmpSid,&Use))
			{
				for(q=0; q<j; q++)
					if(EqualSid(pTokenGroups->Groups[q].Sid,pTmpSid))
						break;
				if(q==j)
				{
					if(memcmp(GetSidIdentifierAuthority(pTmpSid),&nt,sizeof(nt)) ||
					*GetSidSubAuthority(pTmpSid,0)!=SECURITY_BUILTIN_DOMAIN_RID)
					{
						pTokenGroups->Groups[j].Attributes=SE_GROUP_ENABLED|SE_GROUP_ENABLED_BY_DEFAULT;
					}
					else
						pTokenGroups->Groups[j].Attributes=0;
					pTokenGroups->Groups[j].Sid=pTmpSid;
					DEBUG(L"Adding global group (%d) %s\n",j,GlobalGroups[n].grui0_name);
					j++;
				}
			}
		}

		pTokenGroups->GroupCount = j;
		DEBUG(L"Added %d groups\n",j);

		if(!GetPrimaryGroup(wszMachine,wszUser,UserSid,&(*TokenInformation)->PrimaryGroup.PrimaryGroup))
		{
			DEBUG(L"GetPrimaryGroup failed\n");
			retval = STATUS_NO_SUCH_GROUP;
			throw retval;
		}

		j=0;
		lsaUserRights=NULL;
		NumUserRights=0;
		if((retval=LsaEnumerateAccountRights(hLsa,UserSid,&lsaUserRights,&NumUserRights))==STATUS_SUCCESS)
		{
			DEBUG(L"LsaEnumerateAccountRights (user) returned %d rights\n",NumUserRights);
			j+=NumUserRights;
			NetApiBufferFree(lsaUserRights);
		}
		else
		{
			if(LsaNtStatusToWinError(retval)!=2)
				DEBUG(L"LsaEnumerateAccountRights (user) failed (%08x:%d)\n",retval,LsaNtStatusToWinError(retval));
		}

		for(n=0; n<(int)pTokenGroups->GroupCount; n++)
		{
			lsaUserRights=NULL;
			NumUserRights=0;
			if((retval=LsaEnumerateAccountRights(hLsa,pTokenGroups->Groups[n].Sid,&lsaUserRights,&NumUserRights))==STATUS_SUCCESS)
			{
				DEBUG(L"LsaEnumerateAccountRights (group) returned %d rights\n",NumUserRights);
				j+=NumUserRights;
				NetApiBufferFree(lsaUserRights);
			}
			else
			{
				if(LsaNtStatusToWinError(retval)!=2)
					DEBUG(L"LsaEnumerateAccountRights (group) failed (%08x:%d)\n",retval,LsaNtStatusToWinError(retval));
			}
		}
		DEBUG(L"Possible %d group rights\n",j);

		TokenPrivs=(PTOKEN_PRIVILEGES)LsaAllocateHeap(sizeof(TOKEN_PRIVILEGES)+sizeof(LUID_AND_ATTRIBUTES)*j);
		TokenPrivs->PrivilegeCount=j;
		j=0;
		if((retval=LsaEnumerateAccountRights(hLsa,UserSid,&lsaUserRights,&NumUserRights))==STATUS_SUCCESS)
		{
			for(n=0; n<(int)NumUserRights; n++)
			{
				TokenPrivs->Privileges[j].Attributes=SE_PRIVILEGE_ENABLED | SE_PRIVILEGE_ENABLED_BY_DEFAULT;
				if(!LookupPrivilegeValueW(wszMachine,lsaUserRights[n].Buffer,&TokenPrivs->Privileges[j].Luid))
				{
					DEBUG(L"LookupPrivilegeValue failed (%d)\n",GetLastError());
					continue;
				}
				DEBUG(L"User: Adding (%d) %s\n",j,lsaUserRights[n].Buffer);
				j++;
			}
			NetApiBufferFree(lsaUserRights);
		}

		for(n=0; n<(int)pTokenGroups->GroupCount; n++)
		{
			if((retval=LsaEnumerateAccountRights(hLsa,pTokenGroups->Groups[n].Sid,&lsaUserRights,&NumUserRights))==STATUS_SUCCESS)
			{
				for(p=0; p<(int)NumUserRights; p++)
				{
					LUID luid;
					if(!LookupPrivilegeValueW(wszMachine,lsaUserRights[p].Buffer,&luid))
					{
						DEBUG(L"LookupPrivilegeValue failed (%d)\n",GetLastError());
						continue;
					}
					for(q=0; q<j; q++)
						if(!memcmp(&luid,&TokenPrivs->Privileges[q].Luid,sizeof(luid)))
							break;
					if(q==j)
					{
						DEBUG(L"Group: Adding (%d) %s\n",j,lsaUserRights[p].Buffer);
						TokenPrivs->Privileges[j].Attributes=SE_PRIVILEGE_ENABLED | SE_PRIVILEGE_ENABLED_BY_DEFAULT;
						TokenPrivs->Privileges[j].Luid=luid;
						j++;
					}
				}
				NetApiBufferFree(lsaUserRights);
			}
		}

		TokenPrivs->PrivilegeCount=j;
		DEBUG(L"Added %d rights\n",j);

		/* Strip out the BUILTIN stuff */
		for(n=pTokenGroups->GroupCount; n>=0; --n)
		{
			if(pTokenGroups->Groups[n].Attributes==0)
			{
				if((int)pTokenGroups->GroupCount>n)
					memcpy(pTokenGroups->Groups+n,pTokenGroups->Groups+n+1,sizeof(pTokenGroups->Groups[0])*(pTokenGroups->GroupCount-n));
				pTokenGroups->GroupCount--;
			}
		}

		DEBUG(L"%d groups after cleanup\n",pTokenGroups->GroupCount);

		GetDacl(&(*TokenInformation)->DefaultDacl.DefaultDacl,UserSid,pTokenGroups);

		(*TokenInformation)->Groups = pTokenGroups;
		(*TokenInformation)->Privileges = TokenPrivs;

		retval = STATUS_SUCCESS;
	}
	catch(NTSTATUS /*status*/)
	{
		LsaFreeLsa(*TokenInformation);
		*TokenInformation=NULL;
	}

	if(GlobalGroups)
		NetApiBufferFree(GlobalGroups);
	if(LocalGroups)
		NetApiBufferFree(LocalGroups);
	if(hLsa)
		LsaClose(hLsa);

	RevertToSelf();

	return retval;
}

