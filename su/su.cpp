// suidtest.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

/*
	 IsDomainMember patch from John Anderson <panic@semiosix.com>

     Find out whether the machine is a member of a domain or not
*/

// build LSA UNICODE strings.
void InitLsaString( PLSA_UNICODE_STRING LsaString, LPWSTR String )
{
    DWORD StringLength;
    if ( String == NULL )
     {
             LsaString->Buffer = NULL;
             LsaString->Length = 0;
             LsaString->MaximumLength = 0;
             return;
     }
     StringLength = (DWORD)wcslen(String);
     LsaString->Buffer = String;
     LsaString->Length = (USHORT) StringLength * sizeof(WCHAR);
     LsaString->MaximumLength=(USHORT)(StringLength+1) * sizeof(WCHAR);
}

NTSTATUS OpenPolicy( LPWSTR ServerName, DWORD DesiredAccess, PLSA_HANDLE PolicyHandle )
{
     LSA_OBJECT_ATTRIBUTES ObjectAttributes;
     LSA_UNICODE_STRING ServerString;
     PLSA_UNICODE_STRING Server = NULL;
     // Always initialize the object attributes to all zeroes.
     ZeroMemory(&ObjectAttributes, sizeof(ObjectAttributes));
     if ( ServerName != NULL )
     {
             // Make a LSA_UNICODE_STRING out of the LPWSTR passed in.
             InitLsaString(&ServerString, ServerName);
             Server = &ServerString;
     }
     // Attempt to open the policy.
     return LsaOpenPolicy( Server, &ObjectAttributes, DesiredAccess, PolicyHandle );
}

int isDomainMember(wchar_t *wszDomain)
{
     PPOLICY_PRIMARY_DOMAIN_INFO ppdiDomainInfo=NULL;
     PPOLICY_DNS_DOMAIN_INFO pddiDomainInfo=NULL;
     LSA_HANDLE PolicyHandle;
     NTSTATUS status;
     BOOL retval = FALSE;
     
     // open the policy object for the local system
     status = OpenPolicy(
             NULL
             , GENERIC_READ | POLICY_VIEW_LOCAL_INFORMATION
             , &PolicyHandle
     );
    // You have a handle to the policy object. Now, get the
    // domain information using LsaQueryInformationPolicy.
    if ( !status )
    {
		/* Based on patch by Valdas Sevelis.  Call PolicyDnsDomainInformation first
		   as Win2K Advanced server is broken w/PolicyPrimaryDomainInformation */
        status = LsaQueryInformationPolicy(
                PolicyHandle,
                PolicyDnsDomainInformation,
                (void**)&pddiDomainInfo);
		if(!status)
		{
			retval = pddiDomainInfo->Sid != 0;
			if(wszDomain && retval)
			{
				wcsncpy(wszDomain,pddiDomainInfo->Name.Buffer,pddiDomainInfo->Name.Length);
				wszDomain[pddiDomainInfo->Name.Length]='\0';
			}
		    LsaFreeMemory( (LPVOID)pddiDomainInfo );
		}
		else
		{
             status = LsaQueryInformationPolicy(
                     PolicyHandle,
                     PolicyPrimaryDomainInformation,
                     (void**)&ppdiDomainInfo);
			if(!status)
			{
				retval = ppdiDomainInfo->Sid != 0;
				if(wszDomain && retval)
				{
					wcsncpy(wszDomain,ppdiDomainInfo->Name.Buffer,ppdiDomainInfo->Name.Length);
					wszDomain[ppdiDomainInfo->Name.Length]='\0';
				}
			    LsaFreeMemory( (LPVOID)ppdiDomainInfo );
			}
		}
    }
    // Clean up all the memory buffers created by the LSA calls
	LsaClose(PolicyHandle);
    return retval;
}

DWORD BreakNameIntoParts(LPCTSTR name, LPWSTR w_name, LPWSTR w_domain, LPWSTR w_pdc)
{
	static wchar_t *pw_pdc;
    TCHAR *ptr;
	wchar_t w_defaultdomain[DNLEN+1]={0};

	// only fetch a domain controller if the machine is a domain member
	if(isDomainMember(w_defaultdomain))
	{
		ptr=_tcschr(name, '\\');
  		if (ptr)
  		{
#ifdef _UNICODE
			_tcscpy(w_name,ptr+1);
			_tcsncpy(w_name,name,int(ptr-name));
			w_name[int(ptr-name)]='\0';
#else
 			w_name[MultiByteToWideChar(CP_ACP,0,ptr+1,-1,w_name,UNLEN+1)]='\0';
			w_domain[MultiByteToWideChar(CP_ACP,0,name,int(ptr-name),w_domain,DNLEN)]='\0';
#endif
   		} 
		else
		{
#ifdef _UNICODE
			_tcscpy(w_name,name);
#else
 			w_name[MultiByteToWideChar(CP_ACP,0,name,-1,w_name,UNLEN+1)]='\0';
#endif
			wcscpy(w_domain,w_defaultdomain);
		}
	}
  	else
	{
		if(_tcsrchr(name,'\\'))
		{
			fprintf(stderr,"Cannot specify domain - Not a domain member\n");
			fflush(stderr);
			return ERROR_INVALID_PARAMETER;
		}
#ifdef _UNICODE
		_tcscpy(w_name,name);
#else
 		w_name[MultiByteToWideChar(CP_ACP,0,name,-1,w_name,UNLEN+1)]='\0';
#endif
		*w_domain='\0';
	}

	if(w_pdc)
	{
		DWORD (WINAPI *pDsGetDcNameW)(LPCWSTR ComputerName,LPCWSTR DomainName,GUID *DomainGuid,LPCWSTR SiteName,ULONG Flags,PDOMAIN_CONTROLLER_INFOW *DomainControllerInfo);
		pDsGetDcNameW=(DWORD (CALLBACK *)(LPCWSTR,LPCWSTR,GUID *,LPCWSTR,ULONG,PDOMAIN_CONTROLLER_INFOW * )) GetProcAddress(GetModuleHandle(L"netapi32"),"DsGetDcNameW");
		
		w_pdc[0]='\0';
		if(w_domain[0] && pDsGetDcNameW)
		{
			PDOMAIN_CONTROLLER_INFOW pdi;

			if(!pDsGetDcNameW(NULL,w_domain,NULL,NULL,DS_IS_FLAT_NAME,&pdi) || !pDsGetDcNameW(NULL,w_domain,NULL,NULL,DS_IS_DNS_NAME,&pdi))
			{
				wcscpy(w_pdc,pdi->DomainControllerName);
				NetApiBufferFree(pdi);
			}
		}
		else if(w_domain[0])
		{
			if(!NetGetAnyDCName(NULL,w_domain,(LPBYTE*)&pw_pdc) || !NetGetDCName(NULL,w_domain,(LPBYTE*)&pw_pdc))
			{
				wcscpy(w_pdc,pw_pdc);
				NetApiBufferFree(pw_pdc);
			}
		}
	}
	return ERROR_SUCCESS;
}

/* Quick and dirty window station security fix */
BOOL SetWinstaDesktopSecurity(void)
{ 
	HWINSTA hWinsta;
	HDESK hDesk;
	SECURITY_INFORMATION si = DACL_SECURITY_INFORMATION;
	SECURITY_DESCRIPTOR sd;

// Get the current window station and desktop

	hWinsta = GetProcessWindowStation();
	if(hWinsta == NULL)
		return FALSE;

	hDesk = GetThreadDesktop(GetCurrentThreadId());
	if (hDesk == NULL)
		return FALSE;

// Create a NULL DACL

	InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION);
	SetSecurityDescriptorDacl(&sd, TRUE, (PACL) NULL, FALSE);

// Set the NULL DACL

	if (!SetUserObjectSecurity(hWinsta, &si, &sd))
	{
		printf("SetUserObjectSecurity on the window station failed: %d",GetLastError());
		return FALSE;
	}

	if (!SetUserObjectSecurity(hDesk, &si, &sd))
	{ 
		printf("SetUserObjectSecurity on the desktop failed: %d",GetLastError());
		return FALSE;
	}

// Return indicating success

	return TRUE;
}

TCHAR read_key()
{
  INPUT_RECORD buffer;
  DWORD EventsRead;
  TCHAR ch;
  HANDLE hInput = GetStdHandle(STD_INPUT_HANDLE);
  BOOL CharRead = FALSE;

  /* loop until we find a valid keystroke (a KeyDown event, and NOT a
     SHIFT, ALT, or CONTROL keypress by itself) */
  while(!CharRead)
  {
	while( !CharRead && ReadConsoleInput(hInput, &buffer, 1, &EventsRead ) &&
			EventsRead > 0 )
	{
		if( buffer.EventType == KEY_EVENT &&
			buffer.Event.KeyEvent.bKeyDown )
		{
#ifdef _UNICODE
			ch = buffer.Event.KeyEvent.uChar.UnicodeChar;
#else
			ch = buffer.Event.KeyEvent.uChar.AsciiChar;
#endif
			if(ch)
				CharRead = TRUE;
		}
	}
  }

  return ch;
}

TCHAR *getpass (const char *prompt)
{
    static TCHAR password[128];
	static const int max_length = sizeof(password);
    int i;
	TCHAR c;
	HANDLE hInput=GetStdHandle(STD_INPUT_HANDLE);
	DWORD dwMode;

    fputs (prompt, stderr);
    fflush (stderr);
    fflush (stdout);
	FlushConsoleInputBuffer(hInput);
	GetConsoleMode(hInput,&dwMode);
	SetConsoleMode(hInput,ENABLE_PROCESSED_INPUT);
    for (i = 0; i < max_length - 1; ++i)
    {
		c=0;
		c = read_key();
		if(c==27 || c=='\r' || c=='\n')
			break;
		password[i]=c;
		fputc('*',stdout);
		fflush (stderr);
		fflush (stdout);
    }
	SetConsoleMode(hInput,dwMode);
	FlushConsoleInputBuffer(hInput);
    password[i] = '\0';
    fputs ("\n", stderr);
	return c==27?NULL:password;
}

char *ErrorToString(DWORD dwError)
{
	static char Buffer[1024];
	
	FormatMessageA(
          FORMAT_MESSAGE_FROM_SYSTEM |FORMAT_MESSAGE_MAX_WIDTH_MASK,
          NULL,
          dwError,
          MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
          Buffer,
          sizeof(Buffer),
          NULL);
	return Buffer;
}

int _tmain(int argc, TCHAR* argv[])
{
	HANDLE hToken;
	wchar_t wszName[256];
	wchar_t wszDomain[256];
	STARTUPINFO si = { sizeof(STARTUPINFO) };
	PROCESS_INFORMATION pi = {0};
	wchar_t wszTitle[256];
	DWORD dwLen,dwErr;

	if(argc!=2)
	{
		fprintf(stderr,"Usage: %S <username>",argv[0]);
		return -1;
	}

	dwLen=sizeof(wszName);
	GetUserName(wszName,&dwLen);
	_snwprintf(wszTitle,sizeof(wszTitle),L"%s impersonating %s",wszName,argv[1]);
	si.lpTitle=wszTitle;
	si.wShowWindow=SW_SHOW;

	if(BreakNameIntoParts(argv[1],wszName,wszDomain,NULL))
		return -1;

	switch((dwErr=SuidGetImpersonationTokenW(wszName,wszDomain,LOGON32_LOGON_INTERACTIVE,&hToken)))
	{
		case ERROR_SUCCESS:
			break;
		case ERROR_PRIVILEGE_NOT_HELD:
		case ERROR_ACCESS_DENIED:
			{
				wchar_t *pw = getpass("Password: ");
				if(!LogonUser(wszName,wszDomain,pw,LOGON32_LOGON_INTERACTIVE,0,&hToken))
				{
					switch((dwErr=GetLastError()))
					{
						case ERROR_SUCCESS:
							break;
						default:
							fprintf(stderr,"%s\n",ErrorToString(dwErr));
							return -1;
					}
				}
			}
			break;
		default:
			fprintf(stderr,"%s\n",ErrorToString(dwErr));
			return -1;
	}

	SetWinstaDesktopSecurity();

	wchar_t wszCommand[]=L"cmd.exe";
	/* Unicode version of CreateProcess modifies its command parameter... Ansi doesn't.
	   Apparently this is not classed as a bug ???? */
	if(!CreateProcessAsUser(hToken,NULL,wszCommand,NULL,NULL,FALSE,CREATE_NEW_CONSOLE,NULL,NULL,&si,&pi))
	{
		CloseHandle(hToken);
		fprintf(stderr,"CreateProcess returned error %d\n",GetLastError());
		return -1;
	}
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
	CloseHandle(hToken);
	return 0;
}

