/*
 * win32.c
 * - utility functions for cvs under win32
 *
 */

#include <ctype.h>
#include <stdio.h>
#include <conio.h>
#include <share.h>

#define WIN32_LEAN_AND_MEAN
#define _WIN32_WINNT 0x0400
#include <windows.h>
#include <lm.h>
#include <lmcons.h>
#include <winsock.h>
#include <dbghelp.h>
#include <objbase.h>

#include <config.h>
#include <stdlib.h>
#include <process.h>
#include <ntsecapi.h>
#include <tchar.h>

#include "cvs.h"
#include "library.h"

#include "../version.h"
#include "../version_no.h"

#ifdef SERVER_SUPPORT
void nt_setuid_init(); 
int nt_setuid(LPCTSTR szMachine, LPCTSTR szUser);
#endif

/* Try to work out if this is a GMT FS.  There is probably
   a way of doing this that works for all FS's, but this
   detects FAT at least */
#define GMT_FS(_s) (!strstr(_s,"FAT"))

#ifndef CVS95
ITypeLib *myTypeLib;
#endif

static const char *current_username=NULL;
#ifdef SERVER_SUPPORT

static int impersonate;
static int force_local_machine;
#endif

#ifdef SERVER_SUPPORT
static void (*thread_exit_handler)(int);
#endif

static int win32_errno;

static struct { DWORD err; int dos;} errors[] =
{
        {  ERROR_INVALID_FUNCTION,       EINVAL    },  /* 1 */
        {  ERROR_FILE_NOT_FOUND,         ENOENT    },  /* 2 */
        {  ERROR_PATH_NOT_FOUND,         ENOENT    },  /* 3 */
        {  ERROR_TOO_MANY_OPEN_FILES,    EMFILE    },  /* 4 */
        {  ERROR_ACCESS_DENIED,          EACCES    },  /* 5 */
        {  ERROR_INVALID_HANDLE,         EBADF     },  /* 6 */
        {  ERROR_ARENA_TRASHED,          ENOMEM    },  /* 7 */
        {  ERROR_NOT_ENOUGH_MEMORY,      ENOMEM    },  /* 8 */
        {  ERROR_INVALID_BLOCK,          ENOMEM    },  /* 9 */
        {  ERROR_BAD_ENVIRONMENT,        E2BIG     },  /* 10 */
        {  ERROR_BAD_FORMAT,             ENOEXEC   },  /* 11 */
        {  ERROR_INVALID_ACCESS,         EINVAL    },  /* 12 */
        {  ERROR_INVALID_DATA,           EINVAL    },  /* 13 */
        {  ERROR_INVALID_DRIVE,          ENOENT    },  /* 15 */
        {  ERROR_CURRENT_DIRECTORY,      EACCES    },  /* 16 */
        {  ERROR_NOT_SAME_DEVICE,        EXDEV     },  /* 17 */
        {  ERROR_NO_MORE_FILES,          ENOENT    },  /* 18 */
        {  ERROR_LOCK_VIOLATION,         EACCES    },  /* 33 */
        {  ERROR_BAD_NETPATH,            ENOENT    },  /* 53 */
        {  ERROR_NETWORK_ACCESS_DENIED,  EACCES    },  /* 65 */
        {  ERROR_BAD_NET_NAME,           ENOENT    },  /* 67 */
        {  ERROR_FILE_EXISTS,            EEXIST    },  /* 80 */
        {  ERROR_CANNOT_MAKE,            EACCES    },  /* 82 */
        {  ERROR_FAIL_I24,               EACCES    },  /* 83 */
        {  ERROR_INVALID_PARAMETER,      EINVAL    },  /* 87 */
        {  ERROR_NO_PROC_SLOTS,          EAGAIN    },  /* 89 */
        {  ERROR_DRIVE_LOCKED,           EACCES    },  /* 108 */
        {  ERROR_BROKEN_PIPE,            EPIPE     },  /* 109 */
        {  ERROR_DISK_FULL,              ENOSPC    },  /* 112 */
        {  ERROR_INVALID_TARGET_HANDLE,  EBADF     },  /* 114 */
        {  ERROR_INVALID_HANDLE,         EINVAL    },  /* 124 */
        {  ERROR_WAIT_NO_CHILDREN,       ECHILD    },  /* 128 */
        {  ERROR_CHILD_NOT_COMPLETE,     ECHILD    },  /* 129 */
        {  ERROR_DIRECT_ACCESS_HANDLE,   EBADF     },  /* 130 */
        {  ERROR_NEGATIVE_SEEK,          EINVAL    },  /* 131 */
        {  ERROR_SEEK_ON_DEVICE,         EACCES    },  /* 132 */
        {  ERROR_DIR_NOT_EMPTY,          ENOTEMPTY },  /* 145 */
        {  ERROR_NOT_LOCKED,             EACCES    },  /* 158 */
        {  ERROR_BAD_PATHNAME,           ENOENT    },  /* 161 */
        {  ERROR_MAX_THRDS_REACHED,      EAGAIN    },  /* 164 */
        {  ERROR_LOCK_FAILED,            EACCES    },  /* 167 */
        {  ERROR_ALREADY_EXISTS,         EEXIST    },  /* 183 */
        {  ERROR_FILENAME_EXCED_RANGE,   ENOENT    },  /* 206 */
        {  ERROR_NESTING_NOT_ALLOWED,    EAGAIN    },  /* 215 */
        {  ERROR_NOT_ENOUGH_QUOTA,       ENOMEM    }    /* 1816 */
};

#define IOINFO_L2E          5
#define IOINFO_ARRAY_ELTS   (1 << IOINFO_L2E)

typedef struct {
        long osfhnd;
        char osfile;
        char pipech;    
#ifdef _MT
        int lockinitflag;
        CRITICAL_SECTION lock;
#endif  /* _MT */
    }   ioinfo;


#define _pioinfo(i) ( __pioinfo[(i) >> IOINFO_L2E] + ((i) & (IOINFO_ARRAY_ELTS - 1)) )
#define _osfile(i)  ( _pioinfo(i)->osfile )
extern __declspec(dllimport) ioinfo * __pioinfo[];

#define FOPEN           0x01    /* file handle open */
#define FEOFLAG         0x02    /* end of file has been encountered */
#define FCRLF           0x04    /* CR-LF across read buffer (in text mode) */
#define FPIPE           0x08    /* file handle refers to a pipe */
#define FNOINHERIT      0x10    /* file handle opened _O_NOINHERIT */
#define FAPPEND         0x20    /* file handle opened O_APPEND */
#define FDEV            0x40    /* file handle refers to device */
#define FTEXT           0x80    /* file handle is in text mode */

static const WORD month_len [12] = 
{
    31, /* Jan */
    28, /* Feb */
    31, /* Mar */
    30, /* Apr */
    31, /* May */
    30, /* Jun */
    31, /* Jul */
    31, /* Aug */
    30, /* Sep */
    31, /* Oct */
    30, /* Nov */
    31  /* Dec */
};

/* One second = 10,000,000 * 100 nsec */
    static const ULONGLONG systemtime_second = 10000000L;

// IsDomainMember patch from John Anderson <panic@semiosix.com>
int isDomainMember();

typedef BOOL (WINAPI *MINIDUMPWRITEDUMP)(
    IN HANDLE hProcess,
    IN DWORD ProcessId,
    IN HANDLE hFile,
    IN MINIDUMP_TYPE DumpType,
    IN CONST PMINIDUMP_EXCEPTION_INFORMATION ExceptionParam, OPTIONAL
    IN CONST PMINIDUMP_USER_STREAM_INFORMATION UserStreamParam, OPTIONAL
    IN CONST PMINIDUMP_CALLBACK_INFORMATION CallbackParam OPTIONAL
    );
static LONG WINAPI MiniDumper(PEXCEPTION_POINTERS pExceptionInfo);

static int tcp_init()
{
    WSADATA data;

    if (WSAStartup (MAKEWORD (1, 1), &data))
    {
		fprintf (stderr, "cvs: unable to initialize winsock\n");
		return -1;
    }
	return 0;
}

static int tcp_close()
{
    WSACleanup ();
	return 0;
}

void win32init(int mode)
{
#ifdef SERVER_SUPPORT
	char buffer[1024];

	impersonate = 1;
	force_local_machine = 0;

	if(!get_global_config_data("PServer","Impersonation",buffer,sizeof(buffer)))
		impersonate = atoi(buffer);

	if(!get_global_config_data("PServer","DontUseDomain",buffer,sizeof(buffer)))
		force_local_machine = atoi(buffer);
#endif

	_tzset(); // Set the timezone from the system, or 'TZ' if specified.

#if !defined(_CVS95) && !defined(_DEBUG)
	SetUnhandledExceptionFilter(MiniDumper);
#endif

#ifndef CVS95
	CoInitializeEx(NULL,0);
	{
		wchar_t mod[1024];
		GetModuleFileNameW(NULL,mod,1024);
		LoadTypeLibEx(mod,REGKIND_NONE,&myTypeLib); /* This is called too often to register here... let innosetup do it */
	}
#endif

	tcp_init();

#ifdef SERVER_SUPPORT
	if(impersonate)
		nt_setuid_init();
#endif

}

void wnt_cleanup (void)
{
	if(!server_active)
		tcp_close();

	free((void*)current_username);
#ifndef CVS95
	myTypeLib->lpVtbl->Release(myTypeLib);
#endif
}

unsigned sleep(unsigned seconds)
{
	Sleep(1000*seconds);
	return 0;
}

/*
 * Sleep at least useconds microseconds.
 */
int usleep(unsigned long useconds)
{
    /* Not very accurate, but it gets the job done */
    Sleep(useconds/1000 + (useconds%1000 ? 1 : 0));
    return 0;
}

pid_t wnt_getpid (void)
{
    return (pid_t) GetCurrentProcessId();
}

#ifdef CVSGUI_PIPE
#	undef getpass
#endif

char *getpass (const char *prompt)
{
    static char password[128];
	static const int max_length = sizeof(password);
    int i;
	char c;
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

#ifdef SERVER_SUPPORT
int win32_valid_user(char *username, char *password, char *domain, user_handle_t *user_handle)
{
	HANDLE user = NULL;
    TCHAR User[UNLEN+1];
    TCHAR Domain[UNLEN+1];
	TCHAR Password[UNLEN+1];
    char *ptr;

    memset(User, '\0', sizeof User);
    memset(Domain, '\0', sizeof Domain);

	if(!domain)       /* No domain specified, look for one in username */
	{
#ifdef SJIS
      ptr=_mbschr(username, '\\');
#else
      ptr=strchr(username, '\\');
#endif
      if (ptr) 
      {
		/* Use repository password file domain */
		if(!isDomainMember())
		{
			cvs_outerr("error 0 Cannot login: Domain specified but server is not acting as domain member.\n",0);
			cvs_flusherr();
			return 0;
		}

#ifndef _UNICODE
        strncpy(Domain, username, min(ptr-username, sizeof Domain-1));
        strncpy(User, ptr+1, sizeof User-1);
#else
		MultiByteToWideChar(CP_UTF8,0,username, ptr-username, Domain, (sizeof(Domain)/sizeof(TCHAR))-1);
		MultiByteToWideChar(CP_UTF8,0, ptr+1, strlen(ptr+1), User, (sizeof(User)/sizeof(TCHAR))-1);
#endif
      }
	  else
	  {
#ifndef _UNICODE
        strncpy(User, username, sizeof User-1);
#else
		MultiByteToWideChar(CP_UTF8,0,username, strlen(username), User, (sizeof(User)/sizeof(TCHAR))-1);
#endif
      }
    }
	else 
	{
#ifndef _UNICODE
		strncpy(User, username, sizeof User -1); // TH Fix if domain specified
        strncpy(Domain, domain, sizeof Domain-1);
#else
		MultiByteToWideChar(CP_UTF8,0, username, strlen(username), User, (sizeof(User)/sizeof(TCHAR))-1);
		MultiByteToWideChar(CP_UTF8,0,domain, strlen(domain), Domain, (sizeof(Domain)/sizeof(TCHAR))-1);
#endif
    }

#ifndef _UNICODE
		strncpy(Password, password, sizeof Password -1); 
#else
		MultiByteToWideChar(CP_UTF8,0, password, strlen(password), Password, (sizeof(Password)/sizeof(TCHAR))-1);
#endif

	if(!LogonUser(User,Domain,Password,LOGON32_LOGON_NETWORK,LOGON32_PROVIDER_DEFAULT,&user))
	{
		switch(GetLastError())
		{
		case ERROR_PRIVILEGE_NOT_HELD:
			cvs_outerr("error 0 Cannot login: Server has insufficient rights to validate user account - contact your system administrator\n",0);
			cvs_flusherr();
			break;
		default:
			break;
		}

		return 0;
	}
	if(user_handle)
		*user_handle=user;
	else
		CloseHandle(user);
	return 1;
}
#endif

NET_API_STATUS CVSNetGetAnyDCName(LPCWSTR servername, LPCWSTR domainname, LPWSTR buf)
{
	wchar_t* pbuf=NULL;
	NET_API_STATUS ret;

	ret=NetGetAnyDCName(servername,domainname,(LPBYTE*)&pbuf);
	if(pbuf)
	{
		wcscpy(buf,pbuf);
		NetApiBufferFree(pbuf);
	}
	return ret;
}

struct passwd *win32getpwnam(const char *name)
{
#ifdef WINNT_VERSION // Win95 doesn't support this...  Client only mode...
	static struct passwd pw;
	USER_INFO_1 *pinfo = NULL;
    WKSTA_USER_INFO_1 *wk_info = NULL;
	static wchar_t w_name[UNLEN+1];
	static wchar_t w_domain[DNLEN+1];
	static wchar_t w_pdc[2048];
	static char homedir[1024];
	static char pdc[1024];
	NET_API_STATUS res;
    char *ptr;

    // only fetch a domain controller if the machine is a domain member
	if(isDomainMember())
	{
#ifdef SJIS
		ptr=_mbschr(name, '\\');
#else
		ptr=strchr(name, '\\');
#endif
  		if (ptr)
  		{
 			int numchars;
 
 			MultiByteToWideChar(CP_ACP,0,ptr+1,-1,w_name,UNLEN+1);
 
 			// Since -1 wasn't specified for the cbMultiByte (size) parameter and
 			// the input string isn't NULL terminated, the output won't be NULL 
 			// terminated.  We have to do that ourselves.  Note that we leave space
 			// for the NULL byte.
 			numchars = MultiByteToWideChar(CP_ACP,0,name,ptr-name,w_domain,DNLEN);
 
 			if (numchars >= 0)
 				w_domain[numchars] = 0;
  
  			// May fail for workgroup-only NT boxen (Patch from jonathan.gilligan@vanderbilt.edu)
  			CVSNetGetAnyDCName(NULL,w_domain,w_pdc); 
			name = ptr+1;
  		} 
  		else 
  		{
			if(NetWkstaUserGetInfo(NULL,1,(LPBYTE*)&wk_info)==NERR_Success)
				wcscpy(w_pdc,wk_info->wkui1_logon_server);
			else
				CVSNetGetAnyDCName(NULL,NULL,w_pdc); 
			MultiByteToWideChar(CP_ACP,0,name,-1,w_name,UNLEN+1);
  		} 
  	}
  	else
	{
#ifdef SJIS
		if(_mbschr(name,'\\'))
#else
		if(strchr(name,'\\'))
#endif
		{
			fprintf(stderr,"error 0 Invalid username - cannot specify domain as server is not acting as a domain member\n");
			fflush(stderr);
			return NULL;
		}
		MultiByteToWideChar(CP_ACP,0,name,-1,w_name,UNLEN+1); // Initialise buffer...  (Patch from Brian Moran)
	}

	// If by sad chance this gets called on Win95, there is
	// no way of verifying user info, and you always get 'Not implemented'.
	// We are stuck in this case...
	res=NetUserGetInfo(w_pdc,w_name,1,(BYTE**)&pinfo);
	if(res==NERR_UserNotFound)
	{
		if(w_pdc)
			NetApiBufferFree(w_pdc);
		return NULL;
	}

	if(w_pdc)
		WideCharToMultiByte(0,0,w_pdc,-1,pdc,sizeof(pdc),0,0);

	pw.pw_uid=0;
	pw.pw_gid=0;
	pw.pw_name=name;
	pw.pw_passwd="secret";
	pw.pw_shell="cmd.exe";
#ifdef _UNICODE
	pw.pw_pdc =w_pdc;
	pw.pw_name_t = w_name;
#else
	pw.pw_pdc=w_pdc?pdc:NULL;
	pw.pw_name_t =name;
#endif

	if(res==NERR_Success)
	{
		WideCharToMultiByte(CP_ACP,0,pinfo->usri1_home_dir,-1,homedir,sizeof(homedir),NULL,NULL);
		if(*homedir) pw.pw_dir=homedir;
	}
	else
		pw.pw_dir=get_homedir();
		
	if(pinfo)
		NetApiBufferFree(pinfo);
	if(wk_info)
		NetApiBufferFree(wk_info);
	else if(w_pdc)
		NetApiBufferFree(w_pdc);
	return &pw;
#else // Win95 broken version.  Rely on the HOME environment variable...
	static struct passwd pw;
	pw.pw_uid=0;
	pw.pw_gid=0;
	pw.pw_name=(char*)name;
	pw.pw_passwd="secret";
	pw.pw_shell="cmd.exe";
	pw.pw_dir=get_homedir();
	pw.pw_pdc=NULL;
#endif
	return &pw;
}

char *win32getlogin()
{
	static char UserName[UNLEN+1];
	DWORD len=sizeof(UserName);

	if(!GetUserNameA(UserName,&len))
		return NULL;
	return UserName;
}

void win32flush(int fd)
{
	FlushFileBuffers((HANDLE)_get_osfhandle(fd));
}

void win32setblock(int fd, int block)
{
	DWORD mode = block?PIPE_WAIT:PIPE_NOWAIT;
	SetNamedPipeHandleState((HANDLE)_get_osfhandle(fd),&mode,NULL,NULL);
}

#if defined(SERVER_SUPPORT)
int win32setuser(const char *username)
{
	/* Store the user for later */
	current_username = xstrdup(username);
	return 0;
}

/*
  Returns:
     0 = ok
	 1 = user found, impersonation failed
	 2 = user not found
*/

int win32switchtouser(const char *username, user_handle_t user_handle)
{
	/* Store the user for later */
	current_username = xstrdup(username);

	if(impersonate)
	{
		if(!user_handle)
		{
			const struct passwd *pw;

			pw = win32getpwnam(username);
			
			if(!pw)
				return 2;

			return nt_setuid(pw->pw_pdc,pw->pw_name_t)?1:0;
		}
		else
		{
			return ImpersonateLoggedOnUser(user_handle)?0:1;
		}
	}
	else
		return 0;
}

#endif

char *win32getfileowner(const char *file)
{
	static char szName[64];

	/* This is called with a file called '#cvs.[rw]fl.{machine}({user}).pid */
	const char *p=file;
	const char *name=file;

	while(*p)
	{
#ifdef SJIS
		if(_ismbblead(*p))
			p++;
#endif
		if(*p=='\\' || *p=='/')
			name=p+1;
		p++;
	}

	p=strchr(name,'(');
	if(!p)
		return "Unknown User";
	name=p+1;
	p=strchr(name,')');
	if(!p)
		return "Unknown User";
	strncpy(szName,name,p-name);
	szName[p-name]='\0';
	return szName;
}

/* NT won't let you delete a readonly file, even if you have write perms on the directory */
int wnt_unlink(const char *file)
{
	SetFileAttributesA(file,FILE_ATTRIBUTE_NORMAL);
	return unlink(file);
}

/* Based on fix by jmg.  Test whether user is an administrator & can do 'cvs admin' */

int win32_isadmin()
{
#ifndef WINNT_VERSION
	return 1; // Always succeed in the Win95 case...  The server should catch this in client/server, otherwise it's irrelevant.
#else
	/* If Impersonation is enabled, then check the local thread for administrator access,
	   otherwise do the domain checks, as before.  This code is basically Microsoft KB 118626  */
	/* Also use this if we're in client mode */
	if((!current_username || !stricmp(current_username,CVS_Username)) && (impersonate || !server_active)) 
	{
		BOOL   fReturn         = FALSE;
		DWORD  dwStatus;
		DWORD  dwAccessMask;
		DWORD  dwAccessDesired;
		DWORD  dwACLSize;
		DWORD  dwStructureSize = sizeof(PRIVILEGE_SET);
		PACL   pACL            = NULL;
		PSID   psidAdmin       = NULL;

		HANDLE hToken              = NULL;
		HANDLE hImpersonationToken = NULL;

		PRIVILEGE_SET   ps;
		GENERIC_MAPPING GenericMapping;

		PSECURITY_DESCRIPTOR     psdAdmin           = NULL;
		SID_IDENTIFIER_AUTHORITY SystemSidAuthority = SECURITY_NT_AUTHORITY;


   /* Determine if the current thread is running as a user that is a member of
      the local admins group.  To do this, create a security descriptor that
      has a DACL which has an ACE that allows only local aministrators access.
      Then, call AccessCheck with the current thread's token and the security
      descriptor.  It will say whether the user could access an object if it
      had that security descriptor.  Note: you do not need to actually create
      the object.  Just checking access against the security descriptor alone
      will be sufficient.
   */
		__try
		{
		/* AccessCheck() requires an impersonation token.  We first get a primary
			token and then create a duplicate impersonation token.  The
			impersonation token is not actually assigned to the thread, but is
			used in the call to AccessCheck.  Thus, this function itself never
			impersonates, but does use the identity of the thread.  If the thread
			was impersonating already, this function uses that impersonation context.
		*/
			if (!OpenThreadToken(GetCurrentThread(), TOKEN_DUPLICATE|TOKEN_QUERY, TRUE, &hToken))
			{
				if (GetLastError() != ERROR_NO_TOKEN)
					__leave;

				if (!OpenProcessToken(GetCurrentProcess(), TOKEN_DUPLICATE|TOKEN_QUERY, &hToken))
					__leave;
			}

			if (!DuplicateToken (hToken, SecurityImpersonation, &hImpersonationToken))
				__leave;

		/*
			Create the binary representation of the well-known SID that
			represents the local administrators group.  Then create the security
			descriptor and DACL with an ACE that allows only local admins access.
			After that, perform the access check.  This will determine whether
			the current user is a local admin.
		*/
			if (!AllocateAndInitializeSid(&SystemSidAuthority, 2,
										SECURITY_BUILTIN_DOMAIN_RID,
										DOMAIN_ALIAS_RID_ADMINS,
										0, 0, 0, 0, 0, 0, &psidAdmin))
				__leave;

			psdAdmin = LocalAlloc(LPTR, SECURITY_DESCRIPTOR_MIN_LENGTH);
			if (psdAdmin == NULL)
				__leave;

			if (!InitializeSecurityDescriptor(psdAdmin, SECURITY_DESCRIPTOR_REVISION))
				__leave;

			// Compute size needed for the ACL.
			dwACLSize = sizeof(ACL) + sizeof(ACCESS_ALLOWED_ACE) + GetLengthSid(psidAdmin) - sizeof(DWORD);

			pACL = (PACL)LocalAlloc(LPTR, dwACLSize);
			if (pACL == NULL)
				__leave;

			if (!InitializeAcl(pACL, dwACLSize, ACL_REVISION2))
				__leave;

			dwAccessMask= ACCESS_READ | ACCESS_WRITE;

			if (!AddAccessAllowedAce(pACL, ACL_REVISION2, dwAccessMask, psidAdmin))
				__leave;

			if (!SetSecurityDescriptorDacl(psdAdmin, TRUE, pACL, FALSE))
				__leave;

		/* AccessCheck validates a security descriptor somewhat; set the group
			and owner so that enough of the security descriptor is filled out to
			make AccessCheck happy.
		*/
			SetSecurityDescriptorGroup(psdAdmin, psidAdmin, FALSE);
			SetSecurityDescriptorOwner(psdAdmin, psidAdmin, FALSE);

			if (!IsValidSecurityDescriptor(psdAdmin))
				__leave;

			dwAccessDesired = ACCESS_READ;

		/*
			Initialize GenericMapping structure even though you
			do not use generic rights.
		*/
			GenericMapping.GenericRead    = ACCESS_READ;
			GenericMapping.GenericWrite   = ACCESS_WRITE;
			GenericMapping.GenericExecute = 0;
			GenericMapping.GenericAll     = ACCESS_READ | ACCESS_WRITE;

			if (!AccessCheck(psdAdmin, hImpersonationToken, dwAccessDesired,
						&GenericMapping, &ps, &dwStructureSize, &dwStatus,
						&fReturn))
			{
				fReturn = FALSE;
				__leave;
			}
		}
		__finally
		{
			// Clean up.
			if (pACL) LocalFree(pACL);
			if (psdAdmin) LocalFree(psdAdmin);
			if (psidAdmin) FreeSid(psidAdmin);
			if (hImpersonationToken) CloseHandle (hImpersonationToken);
			if (hToken) CloseHandle (hToken);
		}
		return fReturn;
	}
	else
	{
		/* Contact the domain controller to decide whether the user is an adminstrator.  This
		   is about 90-95% accurate */
		USER_INFO_1 *info;
		WKSTA_USER_INFO_1 *wk_info = NULL;
		wchar_t w_name[UNLEN+1];
		wchar_t w_domain[DNLEN+1];
		wchar_t *w_pdc = NULL;
		char *ptr;
		DWORD priv;
		BOOL check_local = FALSE; /* If the domain wasn't specified, try checking for local admin too */
   
		// only fetch a domain controller if the machine is a domain member
		if(isDomainMember())
		{
#ifdef SJIS
  			ptr=_mbschr(CVS_Username, '\\');
#else
  			ptr=strchr(CVS_Username, '\\');
#endif
  			if (ptr)
  			{
 				int numchars;
	 
 				MultiByteToWideChar(CP_ACP,0,ptr+1,-1,w_name,UNLEN+1);
 				numchars = MultiByteToWideChar(CP_ACP,0,CVS_Username,ptr-CVS_Username,w_domain,DNLEN);
	 
				if (numchars >= 0)
 					w_domain[numchars] = 0;
	 
  				// May fail for workgroup-only NT boxen (Patch from jonathan.gilligan@vanderbilt.edu)
  				NetGetAnyDCName(NULL,w_domain,(LPBYTE*)&w_pdc); 
  			} 
 			else
  			{
 			MultiByteToWideChar(CP_ACP,0,CVS_Username,-1,w_name,UNLEN+1);
	  
			if(NetWkstaUserGetInfo(NULL,1,(LPBYTE*)&wk_info)==NERR_Success)
				w_pdc = wk_info->wkui1_logon_server;
			else
				NetGetAnyDCName(NULL,NULL,(LPBYTE*)&w_pdc); 

			check_local = TRUE;
			}
		}
		else    // bugfix 2001-04-25: jonathan.gilligan@vanderbilt.edu: w_name wasn't set in 
				// client mode. Caused failure for cvs admin, even if the user is in the 
				// Administrators group.
		{
			MultiByteToWideChar(CP_ACP,0,CVS_Username,-1,w_name,UNLEN+1);
		}

		if(NetUserGetInfo(w_pdc,w_name,1,(LPBYTE*)&info)!=NERR_Success)
		{
			if(wk_info)
				NetApiBufferFree(wk_info);
			else if(w_pdc)
				NetApiBufferFree(w_pdc);

			return 0;
		}
		priv=info->usri1_priv;
		if(wk_info)
			NetApiBufferFree(wk_info);
		else if(w_pdc)
			NetApiBufferFree(w_pdc);
		NetApiBufferFree(info);
		if(priv==USER_PRIV_ADMIN)
			return 1;

		if(check_local)
		{
			/* No domain specified.  Check local admin privs for user.  This assumes the local
			usernames match the domain usernames, which isn't always the best thing to do,
			however people seem to want it that way. */
			if(NetUserGetInfo(NULL,w_name,1,(LPBYTE*)&info)!=NERR_Success)
				return 0;
			priv=info->usri1_priv;
			NetApiBufferFree(info);
			if(priv==USER_PRIV_ADMIN)
				return 1;
		}
		return 0;
	}
#endif // WINNT_VERSION
}

#ifdef WINNT_VERSION
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
     StringLength = wcslen(String);
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

int isDomainMember()
{
     PPOLICY_PRIMARY_DOMAIN_INFO ppdiDomainInfo=NULL;
     PPOLICY_DNS_DOMAIN_INFO pddiDomainInfo=NULL;
     LSA_HANDLE PolicyHandle;
     NTSTATUS status;
     BOOL retval = FALSE;
     
	if(force_local_machine)
		return 0;

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
			    LsaFreeMemory( (LPVOID)ppdiDomainInfo );
			}
		}
    }
    // Clean up all the memory buffers created by the LSA calls
	LsaClose(PolicyHandle);
    return retval;
}
#endif

int win32_openpipe(const char *pipe)
{
	HANDLE h;

	h = CreateFileA(pipe,GENERIC_READ|GENERIC_WRITE,0,NULL,OPEN_EXISTING,0,NULL);
	if(h==INVALID_HANDLE_VALUE)
	{
		errno=EBADF;
		win32_errno=GetLastError();
		return -1;
	}
	return _open_osfhandle((long)h,_O_RDWR|_O_BINARY);
}

void win32_perror(int quit, const char *prefix, ...)
{
	char tmp[128];
	LPVOID buf=NULL;
	va_list va;

	va_start(va,prefix);
	vsprintf(tmp,prefix,va);
	va_end(va);

	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM,NULL,win32_errno,0,(LPTSTR)&buf,0,NULL);
	error(quit,0,"%s: %s",tmp,buf);
	LocalFree((HLOCAL)buf);
}

#undef fopen
#undef open
#undef fclose
#undef close
#undef access

static char fname[_MAX_PATH*2];

static const char *slash_convert(const char *filename, char *buffer)
{
	char *dest = buffer;
	for(;*filename;filename++)
	{
		if(*filename=='/')
			*(dest++)='\\';
		else
			*(dest++)=*filename;
	}
	*dest='\0';
	return buffer;
}

FILE *wnt_fopen(const char *filename, const char *mode)
{
	FILE *f = fopen(slash_convert(filename,fname),mode);
//	printf("fopen %s = %p\n",filename,f);
	return f;
		
}

int wnt_open(const char *filename, int oflag , int pmode)
{
	int f = open(slash_convert(filename,fname),oflag,pmode);
	return f;
}

int wnt_access(const char *path, int mode)
{
	return access(slash_convert(path,fname),mode);
}

int wnt_fclose(FILE *file)
{
	assert(file);
	assert(file->_flag);

//	printf("fclose %p\n",file);

	//FlushFileBuffers((HANDLE)_get_osfhandle(file->_file));

	return fclose(file);
}

int wnt_close(int file)
{
	assert(file>0);
	return close(file);
}

void _dosmaperr(DWORD dwErr)
{
	int n;
	for(n=0; n<sizeof(errors)/sizeof(errors[0]); n++)
	{
		if(errors[n].err==dwErr)
		{
			errno=errors[n].dos;
			return;
		}
	}
	errno=EFAULT;
}

/* Is the year a leap year?
 * 
 * Use standard Gregorian: every year divisible by 4
 * except centuries indivisible by 400.
 *
 * INPUTS: WORD year the year (AD)
 *
 * OUTPUTS: TRUE if it's a leap year.
 */
BOOL IsLeapYear ( WORD year )
{
    return ( ((year & 3u) == 0)
            && ( (year % 100u == 0)
                || (year % 400u == 0) ));
}

/* A fairly complicated comparison:
 *
 * Compares a test date against a target date.
 * If the test date is earlier, return a negative number
 * If the test date is later, return a positive number
 * If the two dates are equal, return zero.
 *
 * The comparison is complicated by the way we specify
 * TargetDate.
 *
 * TargetDate is assumed to be the kind of date used in 
 * TIME_ZONE_INFORMATION.DaylightDate: If wYear is 0,
 * then it's a kind of code for things like 1st Sunday 
 * of the month. As described in the Windows API docs,
 * wDay is an index of week: 
 *      1 = first of month
 *      2 = second of month
 *      3 = third of month
 *      4 = fourth of month
 *      5 = last of month (even if there are only four such days).
 *
 * Thus, if wYear = 0, wMonth = 4, wDay = 2, wDayOfWeek = 4
 * it specifies the second Thursday of April.
 *
 * INPUTS: SYSTEMTIME * p_test_date     The date to be tested
 *
 *         SYSTEMTIME * p_target_date   The target date. This should be in
 *                                      the format for a TIME_ZONE_INFORMATION
 *                                      DaylightDate or StandardDate.
 *
 * OUTPUT:  -4/+4 if test month is less/greater than target month
 *          -2/+2 if test day is less/greater than target day
 *          -1/+2 if test hour:minute:seconds.milliseconds less/greater than target
 *          0     if both dates/times are equal.
 * 
 */
static int CompareTargetDate (
    const SYSTEMTIME * p_test_date,
    const SYSTEMTIME * p_target_date
    )
{
    WORD first_day_of_month; /* day of week of the first. Sunday = 0 */
    WORD end_of_month;       /* last day of month */
    WORD temp_date;
    int test_milliseconds, target_milliseconds;

    /* Check that p_target_date is in the correct foramt: */
    if (p_target_date->wYear) 
    {
        error(0,0,"Internal error: p_target_date is not in TIME_ZONE_INFORMATION format.");
        return 0;
    }
    if (!p_test_date->wYear) 
    {
        error(0,0,"Internal error: p_test_date must be an actual date, not TIME_ZONE_INFORMAATION format.");
        return 0;
    }

    /* Don't waste time calculating if we can shortcut the comparison... */
    if (p_test_date->wMonth != p_target_date->wMonth) 
    {
        return (p_test_date->wMonth > p_target_date->wMonth) ? 4 : -4;
    }

    /* Months equal. Now we neet to do some calculation.
     * If we know that y is the day of the week for some arbitrary date x, 
     * then the day of the week of the first of the month is given by 
     * (1 + y - x) mod 7.
     * 
     * For instance, if the 19th is a Wednesday (day of week = 3), then
     * the date of the first Wednesday of the month is (19 mod 7) = 5.
     * If the 5th is a Wednesday (3), then the first of the month is 
     * four days earlier (it's the first, not the zeroth):
     * (3 - 4) = -1; -1 mod 7 = 6. The first is a Saturday.
     *
     * Check ourselves: The 19th falls on a (6 + 19 - 1) mod 7 
     * = 24 mod 7 = 3: Wednesday, as it should be.
     */
    first_day_of_month = (WORD)( (1u + p_test_date->wDayOfWeek - p_test_date->wDay) % 7u);

    /* If the first of the month comes on day y, then the first day of week z falls on
     * (z - y + 1) mod 7. 
     *
     * For instance, if the first is a Saturday (6), then the first Tuesday (2) falls on a
     * (2 - 6 + 1) mod 7 = -3 mod 7 = 4: The fourth. This is correct (see above).
     *
     * temp_date gets the first <target day of week> in the month.
     */
    temp_date = (WORD)( (1u + p_target_date->wDayOfWeek - first_day_of_month) % 7u);
    /* If we're looking for the third Tuesday in the month, find the date of the first
     * Tuesday and add (3 - 1) * 7. In the example, it's the 4 + 14 = 18th.
     *
     * temp_date now should hold the date for the wDay'th wDayOfWeek of the month.
     * we only need to handle the special case of the last <DayOfWeek> of the month.
     */
    temp_date = (WORD)( temp_date + 7 * p_target_date->wDay );

    /* what's the last day of the month? */
    end_of_month = month_len [p_target_date->wMonth - 1];
    /* Correct if it's February of a leap year? */
    if ( p_test_date->wMonth == 2 && IsLeapYear(p_test_date->wYear) ) 
    {
        ++ end_of_month;
    }

    /* if we tried to calculate the fifth Tuesday of the month
     * we may well have overshot. Correct for that case.
     */
    while ( temp_date > end_of_month)
        temp_date -= 7;

    /* At long last, we're ready to do the comparison. */
    if ( p_test_date->wDay != temp_date ) 
    {
        return (p_test_date->wDay > temp_date) ? 2 : -2;
    }
    else 
    {
        test_milliseconds = ((p_test_date->wHour * 60 + p_test_date->wMinute) * 60 
                                + p_test_date->wSecond) * 1000 + p_test_date->wMilliseconds;
        target_milliseconds = ((p_target_date->wHour * 60 + p_target_date->wMinute) * 60 
                                + p_target_date->wSecond) * 1000 + p_target_date->wMilliseconds;
        test_milliseconds -= target_milliseconds;
        return (test_milliseconds > 0) ? 1 : (test_milliseconds < 0) ? -1 : 0;
    }

}

//
//  Get time zone bias for local time *pst.
//
//  UTC time = *pst + bias.
//
static int GetTimeZoneBias( const SYSTEMTIME * pst )
{
    TIME_ZONE_INFORMATION tz;
    int n, bias;
    
    GetTimeZoneInformation ( &tz );

    /*  I only deal with cases where we look at 
     *  a "last sunday" kind of thing.
     */
    if (tz.DaylightDate.wYear || tz.StandardDate.wYear) 
    {
        error(0,0, "Cannont handle year-specific DST clues in TIME_ZONE_INFORMATION");
        return 0;
    }

    bias = tz.Bias;

    n = CompareTargetDate ( pst, & tz.DaylightDate );
    if (n < 0)
        bias += tz.StandardBias;
    else 
    {
        n = CompareTargetDate ( pst, & tz.StandardDate );
        if (n < 0)
            bias += tz.DaylightBias;
        else
            bias += tz.StandardBias;
    }
    return bias;
}

//
//  Is the (local) time in DST?
//
static int IsDST( const SYSTEMTIME * pst )
{
    TIME_ZONE_INFORMATION tz;
    
    GetTimeZoneInformation ( &tz );

    /*  I only deal with cases where we look at 
     *  a "last sunday" kind of thing.
     */
    if (tz.DaylightDate.wYear || tz.StandardDate.wYear) 
    {
        error(0,0, "Cannont handle year-specific DST clues in TIME_ZONE_INFORMATION");
        return 0;
    }

	return (CompareTargetDate ( pst, & tz.DaylightDate ) >= 0)
			&& (CompareTargetDate ( pst, & tz.StandardDate ) < 0);
}

/* Convert a system time from local time to UTC time, correctly
 * taking DST into account (sort of).
 *
 * INPUTS:
 *      const SYSTEMTIME * p_local: 
 *              A file time. It may be in UTC or in local 
 *              time (see local_time, below, for details).
 *
 *      SYSTEMTIME * p_utc:
 *              The destination for the converted time.
 *
 * OUTPUTS:
 *      SYSTEMTIME * p_utc:         
 *              The destination for the converted time.
 */

void LocalSystemTimeToUtcSystemTime( const SYSTEMTIME * p_local, SYSTEMTIME * p_utc)
{
    TIME_ZONE_INFORMATION tz;
    FILETIME ft;
    ULARGE_INTEGER itime, delta;
    int bias;

    * p_utc = * p_local;

    GetTimeZoneInformation(&tz);
    bias = tz.Bias;
    if ( CompareTargetDate(p_local, & tz.DaylightDate) < 0
            || CompareTargetDate(p_local, & tz.StandardDate) >= 0)
        bias += tz.StandardBias;
    else
        bias += tz.DaylightBias;

    SystemTimeToFileTime(p_local, & ft);
    itime.QuadPart = ((ULARGE_INTEGER *) &ft)->QuadPart;
    delta.QuadPart = systemtime_second;
    delta.QuadPart *= 60;   // minute
    delta.QuadPart *= bias;
    itime.QuadPart += delta.QuadPart;
    ((ULARGE_INTEGER *) & ft)->QuadPart = itime.QuadPart;
    FileTimeToSystemTime(& ft, p_utc);
}


/* Convert a file time to a Unix time_t structure. This function is as 
 * complicated as it is because it needs to ask what time system the 
 * filetime describes.
 * 
 * INPUTS:
 *      const FILETIME * ft: A file time. It may be in UTC or in local 
 *                           time (see local_time, below, for details).
 *
 *      time_t * ut:         The destination for the converted time.
 *
 *      BOOL local_time:     TRUE if the time in *ft is in local time 
 *                           and I need to convert to a real UTC time.
 *
 * OUTPUTS:
 *      time_t * ut:         Store the result in *ut.
 */
static BOOL FileTimeToUnixTime ( const FILETIME* pft, time_t* put, BOOL local_time )
{
    BOOL success = FALSE;

   /* FILETIME = number of 100-nanosecond ticks since midnight 
    * 1 Jan 1601 UTC. time_t = number of 1-second ticks since 
    * midnight 1 Jan 1970 UTC. To translate, we subtract a
    * FILETIME representation of midnight, 1 Jan 1970 from the
    * time in question and divide by the number of 100-ns ticks
    * in one second.
    */

    SYSTEMTIME base_st = 
    {
        1970,   /* wYear            */
        1,      /* wMonth           */
        0,      /* wDayOfWeek       */
        1,      /* wDay             */
        0,      /* wHour            */
        0,      /* wMinute          */
        0,      /* wSecond          */
        0       /* wMilliseconds    */
    };
    
    ULARGE_INTEGER itime;
    FILETIME base_ft;
    int bias = 0;

    if (local_time)
    {
        SYSTEMTIME temp_st;
        success = FileTimeToSystemTime(pft, & temp_st);
        bias =  GetTimeZoneBias(& temp_st);
    }

    success = SystemTimeToFileTime ( &base_st, &base_ft );
    if (success) 
    {
        itime.QuadPart = ((ULARGE_INTEGER *)pft)->QuadPart;

        itime.QuadPart -= ((ULARGE_INTEGER *)&base_ft)->QuadPart;
        itime.QuadPart /= systemtime_second;	// itime is now in seconds.
        itime.QuadPart += bias * 60;    // bias is in minutes.

        *put = itime.LowPart;
    }

    if (!success)
    {
        *put = -1;   /* error value used by mktime() */
    }
    return success;
}

/* Create a FileTime from a time_t, taking into account timezone if required */
static BOOL UnixTimeToFileTime ( time_t ut, FILETIME* pft, BOOL local_time )
{
    BOOL success = FALSE;

   /* FILETIME = number of 100-nanosecond ticks since midnight 
    * 1 Jan 1601 UTC. time_t = number of 1-second ticks since 
    * midnight 1 Jan 1970 UTC. To translate, we subtract a
    * FILETIME representation of midnight, 1 Jan 1970 from the
    * time in question and divide by the number of 100-ns ticks
    * in one second.
    */

    SYSTEMTIME base_st = 
    {
        1970,   /* wYear            */
        1,      /* wMonth           */
        0,      /* wDayOfWeek       */
        1,      /* wDay             */
        0,      /* wHour            */
        0,      /* wMinute          */
        0,      /* wSecond          */
        0       /* wMilliseconds    */
    };
    
    ULARGE_INTEGER itime;
    FILETIME base_ft;
    int bias = 0;

    SystemTimeToFileTime ( &base_st, &base_ft );
	itime.HighPart=0;
	itime.LowPart = ut;
	itime.QuadPart *= systemtime_second;
	itime.QuadPart += ((ULARGE_INTEGER *)&base_ft)->QuadPart;

    if (local_time)
    {
        SYSTEMTIME temp_st;
        success = FileTimeToSystemTime((FILETIME*)&itime, & temp_st);
        bias =  GetTimeZoneBias(& temp_st);

		itime.QuadPart += (bias * 60) *systemtime_second;
    }

	*(ULARGE_INTEGER*)pft=itime;

    return success;
}

static int _statcore(HANDLE hFile, const char *filename, struct stat *buf)
{
    BY_HANDLE_FILE_INFORMATION bhfi;
	int isdev;
	char szFs[32];
	int is_gmt_fs = -1;

	if(hFile)
	{
		/* Find out what kind of handle underlies filedes
		*/
		isdev = GetFileType(hFile) & ~FILE_TYPE_REMOTE;

		if ( isdev != FILE_TYPE_DISK )
		{
			/* not a disk file. probably a device or pipe
			 */
			if ( (isdev == FILE_TYPE_CHAR) || (isdev == FILE_TYPE_PIPE) )
			{
				/* treat pipes and devices similarly. no further info is
				 * available from any API, so set the fields as reasonably
				 * as possible and return.
				 */
				if ( isdev == FILE_TYPE_CHAR )
					buf->st_mode = _S_IFCHR;
				else
					buf->st_mode = _S_IFIFO;

				buf->st_rdev = buf->st_dev = (_dev_t)hFile;
				buf->st_nlink = 1;
				buf->st_uid = buf->st_gid = buf->st_ino = 0;
				buf->st_atime = buf->st_mtime = buf->st_ctime = 0;
				if ( isdev == FILE_TYPE_CHAR )
					buf->st_size = 0;
				else
				{
					unsigned long ulAvail;
					int rc;
					rc = PeekNamedPipe(hFile,NULL,0,NULL,&ulAvail,NULL);
					if (rc)
						buf->st_size = (_off_t)ulAvail;
					else 
						buf->st_size = (_off_t)0;
				}

				return 0;
			}
			else if ( isdev == FILE_TYPE_UNKNOWN )
			{
				// Network device
				errno = EBADF;
				return -1;
			}
		}
	}

    /* set the common fields
     */
    buf->st_ino = buf->st_uid = buf->st_gid = buf->st_mode = 0;
    buf->st_nlink = 1;

	if(hFile)
	{
		/* use the file handle to get all the info about the file
		 */
		if ( !GetFileInformationByHandle(hFile, &bhfi) )
		{
			_dosmaperr(GetLastError());
			return -1;
		}
	}
	else
	{
		WIN32_FIND_DATAA fd;
		HANDLE hFind;
		
		hFind=FindFirstFileA(filename,&fd);
		memset(&bhfi,0,sizeof(bhfi));
		if(hFind==INVALID_HANDLE_VALUE)
		{
			// If we don't have permissions to view the directory content (required by FindFirstFile,
			// maybe we can open the file directly
			hFile = CreateFileA(filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
				FILE_ATTRIBUTE_NORMAL, 0);

			if(hFile != INVALID_HANDLE_VALUE)
			{
				GetFileInformationByHandle(hFile, &bhfi);
				CloseHandle(hFile);
			}
			else
			{
				// Can't do anything about the root directory...
				// Win2k and Win98 can return info, but Win95 can't, so
				// it's best to do nothing.
				bhfi.dwFileAttributes=GetFileAttributesA(filename);
				if(bhfi.dwFileAttributes==0xFFFFFFFF)
				{
					_dosmaperr(GetLastError());
					return -1;
				}
				bhfi.nNumberOfLinks=1;
 			}
		}
		else
		{
			FindClose(hFind);
			bhfi.dwFileAttributes=fd.dwFileAttributes;
			bhfi.ftCreationTime=fd.ftCreationTime;
			bhfi.ftLastAccessTime=fd.ftLastAccessTime;
			bhfi.ftLastWriteTime=fd.ftLastWriteTime;
			bhfi.nFileSizeLow=fd.nFileSizeLow;
			bhfi.nFileSizeHigh=fd.nFileSizeHigh;
			bhfi.nNumberOfLinks=1;
		}
	}

    if ( bhfi.dwFileAttributes & FILE_ATTRIBUTE_READONLY )
        buf->st_mode |= (_S_IREAD + (_S_IREAD >> 3) + (_S_IREAD >> 6));
    else
        buf->st_mode |= ((_S_IREAD|_S_IWRITE) + ((_S_IREAD|_S_IWRITE) >> 3)
          + ((_S_IREAD|_S_IWRITE) >> 6));
	if (bhfi.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
        buf->st_mode |= (_S_IEXEC + (_S_IEXEC >> 3) + (_S_IEXEC >> 6));

    // Potential, but unlikely problem... casting DWORD to short.
    // Reported by Jerzy Kaczorowski, addressed by Jonathan Gilligan
    // if the number of links is greater than 0x7FFF
    // there would be an overflow problem.
    // This is a problem inherent in the struct stat, and hence
    // in the design of the C library.
    if (bhfi.nNumberOfLinks > SHRT_MAX) {
        error(0,0,"Internal error: too many links to a file.");
        buf->st_nlink = SHRT_MAX;
        }
    else {
	    buf->st_nlink=(short)bhfi.nNumberOfLinks;
        }

	if(!filename)
	{
		// We have to assume here that the repository doesn't span drives, so the
		// current directory is correct.
		*szFs='\0';
		GetVolumeInformationA(NULL,NULL,0,NULL,NULL,NULL,szFs,32);
		
		is_gmt_fs = GMT_FS(szFs);
	}
	else
	{
		if(filename[1]!=':')
		{
			if((filename[0]=='\\' || filename[0]=='/') && (filename[1]=='\\' || filename[1]=='/'))
			{
				// UNC pathname, assumed to be on an NTFS server.  No way to
				// check, actually (GVI doesn't return anything useful).
				is_gmt_fs = 1;
			}
			else
			{
				// Relative path, treat as local
				GetVolumeInformationA(NULL,NULL,0,NULL,NULL,NULL,szFs,sizeof(szFs));
				is_gmt_fs = GMT_FS(szFs);
			}
		}
		else
		{
			// Drive specified...
			char szRootPath[4] = "?:\\";
			szRootPath[0]=filename[0];
			*szFs='\0';
			GetVolumeInformationA(szRootPath,NULL,0,NULL,NULL,NULL,szFs,sizeof(szFs));
			is_gmt_fs = GMT_FS(szFs);
		}
	}

	if(is_gmt_fs) // NTFS or similar - everything is in GMT already
	{
		FileTimeToUnixTime ( &bhfi.ftLastAccessTime, &buf->st_atime, FALSE );
		FileTimeToUnixTime ( &bhfi.ftLastWriteTime, &buf->st_mtime, FALSE );
		FileTimeToUnixTime ( &bhfi.ftCreationTime, &buf->st_ctime, FALSE );
	}
	else 
	{
        // FAT - timestamps are in incorrectly translated local time.
        // translate them back and let FileTimeToUnixTime() do the
        // job properly.

        FILETIME At,Wt,Ct;

		FileTimeToLocalFileTime ( &bhfi.ftLastAccessTime, &At );
		FileTimeToLocalFileTime ( &bhfi.ftLastWriteTime, &Wt );
		FileTimeToLocalFileTime ( &bhfi.ftCreationTime, &Ct );
		FileTimeToUnixTime ( &At, &buf->st_atime, TRUE );
		FileTimeToUnixTime ( &Wt, &buf->st_mtime, TRUE );
		FileTimeToUnixTime ( &Ct, &buf->st_ctime, TRUE );
	}

    buf->st_size = bhfi.nFileSizeLow;
	if(bhfi.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)
		buf->st_mode |= _S_IFDIR;
	else
		buf->st_mode |= _S_IFREG;

    /* On DOS, this field contains the drive number, but
     * the drive number is not available on this platform.
     * Also, for UNC network names, there is no drive number.
     */
    buf->st_rdev = buf->st_dev = 0;
	return 0;
}

#undef stat
#undef fstat
#undef lstat

int wnt_stat(const char *name, struct wnt_stat *buf)
{
	return _statcore(NULL,slash_convert(name,fname),buf);
}

int wnt_fstat (int fildes, struct wnt_stat *buf)
{
	return _statcore((HANDLE)_get_osfhandle(fildes),NULL,buf);
}

int wnt_lstat (const char *name, struct wnt_stat *buf)
{
	return _statcore(NULL,slash_convert(name,fname),buf);
}

#undef asctime
#undef ctime
/* ANSI C compatibility for timestamps - VC pads with zeros, C standard pads with spaces */

char *wnt_asctime(const struct tm *tm)
{
	char *buf = asctime(tm);
	if(buf[8]=='0') buf[8]=' ';
	return buf;
}

char *wnt_ctime(const time_t *t)
{
	static char future[] = "Fri Dec 31 23:59:59 1999";
#ifdef TIME_64BIT
	char *buf = _ctime64(t);
#else
	char *buf = ctime(t);
#endif
	if(!buf) /* Y2.038K bug in 32bit... */
		buf=future;
	else if(buf[8]=='0')
		buf[8]=' ';
	return buf;
}

int wnt_utime(const char *filename, struct utimbuf *uf)
{
	const char *f = slash_convert(filename,fname);
	HANDLE h;
	int is_gmt_fs;
	char szFs[32];
	FILETIME At,Wt;

	if(filename[1]!=':')
	{
		if((filename[0]=='\\' || filename[0]=='/') && (filename[1]=='\\' || filename[1]=='/'))
		{
			// UNC pathname, assumed to be on an NTFS server.  No way to
			// check, actually (GVI doesn't return anything useful).
			is_gmt_fs = 1;
		}
		else
		{
			// Relative path, treat as local
			GetVolumeInformationA(NULL,NULL,0,NULL,NULL,NULL,szFs,sizeof(szFs));
			is_gmt_fs = GMT_FS(szFs);
		}
	}
	else
	{
		// Drive specified...
		char szRootPath[4] = "?:\\";
		szRootPath[0]=filename[0];
		*szFs='\0';
		GetVolumeInformationA(szRootPath,NULL,0,NULL,NULL,NULL,szFs,sizeof(szFs));
		is_gmt_fs = GMT_FS(szFs);
	}

	if(is_gmt_fs) // NTFS or similar - everything is in GMT already
	{
		UnixTimeToFileTime ( uf->actime, &At, FALSE);
		UnixTimeToFileTime ( uf->modtime, &Wt, FALSE);
	}
	else 
	{
		// FAT or similar.  Convert to local time
		FILETIME fAt,fWt;

		UnixTimeToFileTime ( uf->actime, &fAt, TRUE);
		UnixTimeToFileTime ( uf->modtime, &fWt, TRUE);
		FileTimeToLocalFileTime ( &fAt, &At ); // Was LocalFileTimeFileTime... Not sure about this change.
		FileTimeToLocalFileTime ( &fWt, &Wt );
	}
	h = CreateFileA(f,GENERIC_READ|GENERIC_WRITE,0,NULL,OPEN_EXISTING,0,NULL);
	if(h==INVALID_HANDLE_VALUE)
		return -1;
	SetFileTime(h,NULL,&At,&Wt);
	CloseHandle(h);
	return 0;
}

void tidy_path(char *path)
{
	GetShortPathNameA(path,path,strlen(path)+1);
}

void wnt_putenv(const char *variable, const char *value)
{
	assert(variable);
	assert(value);
	SetEnvironmentVariableA(variable,value);
}

LONG WINAPI MiniDumper(PEXCEPTION_POINTERS pExceptionInfo)
{
	LONG retval = EXCEPTION_CONTINUE_SEARCH;
	HWND hParent = NULL;						// find a better value for your app
	LPCTSTR szResult = NULL;

	// firstly see if dbghelp.dll is around and has the function we need
	// look next to the EXE first, as the one in System32 might be old 
	// (e.g. Windows 2000)
	HMODULE hDll = NULL;
	TCHAR szDbgHelpPath[_MAX_PATH];

	if (GetModuleFileName( NULL, szDbgHelpPath, _MAX_PATH ))
	{
		TCHAR *pSlash = _tcsrchr( szDbgHelpPath, '\\' );
		if (pSlash)
		{
			_tcscpy( pSlash+1, _T("DBGHELP.DLL") );
			hDll = LoadLibrary( szDbgHelpPath );
		}
	}

	if (hDll==NULL)
	{
		// load any version we can
		hDll = LoadLibrary( _T("DBGHELP.DLL") );
	}

	if (hDll)
	{
		MINIDUMPWRITEDUMP pDump = (MINIDUMPWRITEDUMP)GetProcAddress( hDll, "MiniDumpWriteDump" );
		if (pDump)
		{
			TCHAR szDumpPath[_MAX_PATH];
			TCHAR szScratch [_MAX_PATH];

			// work out a good place for the dump file
			if (!GetTempPath( _MAX_PATH, szDumpPath ))
				_tcscpy( szDumpPath, _T("c:\\") );

			_tcscat( szDumpPath, _T("cvsnt-") T_CVSNT_PRODUCTVERSION_SHORT _T(".dmp") );

			// ask the user if they want to save a dump file
			if (MessageBox( NULL, _T("Something bad happened to CVSNT, and it crashed.  Would you like to produce a crash dump?"), _T("cvsnt"), MB_YESNO )==IDYES)
			{
				// create the file
				HANDLE hFile = CreateFile( szDumpPath, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS,
											FILE_ATTRIBUTE_NORMAL, NULL );

				if (hFile!=INVALID_HANDLE_VALUE)
				{
					struct _MINIDUMP_EXCEPTION_INFORMATION ExInfo;
					BOOL bOK;

					ExInfo.ThreadId = GetCurrentThreadId();
					ExInfo.ExceptionPointers = pExceptionInfo;
					ExInfo.ClientPointers = FALSE;

					// write the dump
					bOK = pDump( GetCurrentProcess(), GetCurrentProcessId(), hFile, MiniDumpWithFullMemory, &ExInfo, NULL, NULL );
					if (bOK)
					{
						_stprintf( szScratch, _T("Saved dump file to '%s'.  Please email this file to cvsnt-crashdumps@cvsnt.org with an explanation of what you did to cause the fault."), szDumpPath );
						szResult = szScratch;
						retval = EXCEPTION_EXECUTE_HANDLER;
					}
					else
					{
						_stprintf( szScratch, _T("Failed to save dump file to '%s' (error %d)"), szDumpPath, GetLastError() );
						szResult = szScratch;
					}
					CloseHandle(hFile);
				}
				else
				{
					_stprintf( szScratch, _T("Failed to create dump file '%s' (error %d)"), szDumpPath, GetLastError() );
					szResult = szScratch;
				}
			}
		}
		else
		{
			szResult = _T("Something bad happened to CVSNT, and it crashed.  Unfortunately DBGHELP.DLL is too old for us to produce a crash dump.");
		}
	}
	else
	{
			szResult = _T("Something bad happened to CVSNT, and it crashed.  Unfortunately DBGHELP.DLL is missing so we can't produce a crash dump.");
	}

	if (szResult)
		MessageBox( NULL, szResult, _T("cvsnt"), MB_OK );
	return retval;
}

int wnt_link(const char *oldpath, const char *newpath)
{
	static BOOL (WINAPI *pCreateHardLinkA)(LPCSTR lpFileName, LPCSTR lpExistingFileName, LPSECURITY_ATTRIBUTES lpSecurityAttributes);
	if(!pCreateHardLinkA)
	{
		HANDLE hLib = GetModuleHandle(_T("Kernel32"));
		((PROC)pCreateHardLinkA)=GetProcAddress(hLib,"CreateHardLinkA");
	}
	if(!pCreateHardLinkA)
	{
		errno=ENOSYS;
		return -1;
	}
	if(!pCreateHardLinkA(newpath,oldpath,NULL))
	{
		_dosmaperr(GetLastError());
		return -1;
	}
	return 0;
}

int wnt_fseek64(FILE *fp, off_t pos, int whence)
{
	/* Win32 does in fact have a 64bit fseek but it's buried in the CRT and not
	   exported... */
	switch(whence)
	{
	case SEEK_SET:
		if(pos>0x7FFFFFFF)
		{
			if(fseek(fp,0x7FFFFFFF,whence))
				return -1;
			whence=SEEK_CUR;
			pos-=0x7FFFFFFFF;
			while(pos>0x7FFFFFFF && !fseek(fp,0x7FFFFFFF,SEEK_CUR))
				pos-=0x7FFFFFFFF;
			if(pos>0x7FFFFFFF)
				return -1;
		}
		return fseek(fp,(long)pos,whence);
	case SEEK_CUR:
		if(pos>0x7FFFFFFF)
		{
			while(pos>0x7FFFFFFF && !fseek(fp,0x7FFFFFFF,SEEK_CUR))
				pos-=0x7FFFFFFFF;
			if(pos>0x7FFFFFFF)
				return -1;
		}
		return fseek(fp,(long)pos,whence);
	case SEEK_END:
		if(pos>0x7FFFFFFF)
		{
			if(fseek(fp,0x7FFFFFFF,whence))
				return -1;
			whence=SEEK_CUR;
			pos-=0x7FFFFFFFF;
			while(pos>0x7FFFFFFF && !fseek(fp,-0x7FFFFFFF,SEEK_CUR))
				pos-=0x7FFFFFFFF;
			if(pos>0x7FFFFFFF)
				return -1;
			pos=-pos;
		}
		return fseek(fp,(long)pos,whence);
	}
	return -1;
}

off_t wnt_ftell64(FILE *fp)
{
	off_t o=-1;
	fgetpos(fp,&o);
	return o;
}

void wnt_hide_file(const char *fn)
{
	SetFileAttributesA(fn,FILE_ATTRIBUTE_HIDDEN);
}

#ifdef _UNICODE
int real_main(int argc, char *argv[]);

int __cdecl wmain(int argc, wchar_t **wargv, wchar_t **wenvp)
{
	char **argv;
//	char **envp;
	int i,ret;

	argv=(char**)xmalloc(argc*sizeof(char*));
/*	for(i=0;wenvp[i];i++)
		;
	envp=(char**)xmalloc((i+1)*sizeof(char*));*/

	for(i=0; i<argc; i++)
	{
		int l = wcslen(wargv[i])+1;
		argv[i]=(char*)xmalloc(l);
		WideCharToMultiByte(CP_UTF8,0,wargv[i],l,argv[i],l,NULL,NULL);
	}

/*	for(i=0; wenvp[i]; i++)
	{
		int l = wcslen(wenvp[i])+1;
		envp[i]=(char*)xmalloc(l);
		WideCharToMultiByte(CP_UTF8,0,wenvp[i],l,argv[i],l,NULL,NULL);
	}
	envp[i]=NULL; */

	ret = real_main(argc,argv/*,envp*/);

	for(i=0; i<argc; i++)
		xfree(argv[i]);
	xfree(argv);

/*	for(i=0;envp[i];i++)
		xfree(envp[i]);
	xfree(envp);*/

	return ret;
}
#endif

int getmode(int fd)
{
	unsigned char mode = _osfile(fd);
	if(mode&FTEXT)
		return _O_TEXT;
	else
		return _O_BINARY;
}

