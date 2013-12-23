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
#define _WIN32_WINNT 0x0500
#define WIN32_NO_STATUS
#include <windows.h>
#undef WIN32_NO_STATUS
#include <ntstatus.h>
#include <lm.h>
#include <lmcons.h>
#include <winsock.h>
#include <dbghelp.h>
#include <objbase.h>
#include <ntdsapi.h>
#include <dsgetdc.h>

#include <config.h>
#include <stdlib.h>
#include <process.h>
#include <winternl.h>
#define _NTDEF_
#include <ntsecapi.h>
#include <tchar.h>

#ifndef CVS95
#include "sid.h"
static MAKE_SID1(sidEveryone, 1, 0);
#endif

/* MS BUG:  DNLEN hasn't been maintained so when you're on a legacy-free win2k domain
    you can apparently get a domain that's >DNLEN in size */
#undef DNLEN
#define DNLEN 256

#include "cvs.h"
#include "library.h"

#include "../version.h"
#include "../version_no.h"

#ifdef SERVER_SUPPORT
void nt_setuid_init();
int nt_setuid(LPCWSTR szMachine, LPCWSTR szUser, HANDLE *phToken);
int nt_s4u(LPCWSTR wszMachine, LPCWSTR wszUser, HANDLE *phToken);
#include "setuid/libsuid/suid.h"
#endif

/* Try to work out if this is a GMT FS.  There is probably
   a way of doing this that works for all FS's, but this
   detects FAT at least */
#define GMT_FS(_s) (!strstr(_s,"FAT"))

#ifndef CVS95
ITypeLib *myTypeLib;
#endif

int bIsWin95, bIsNt4, bIsWin2k;

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
int isDomainMember(wchar_t *wszDomain);

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

static int use_ntsec, use_ntea;

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
	OSVERSIONINFO osv;
	char buffer[1024];

#ifdef SERVER_SUPPORT

	impersonate = 1;
	force_local_machine = 0;

	if(!get_global_config_data("PServer","Impersonation",buffer,sizeof(buffer)))
		impersonate = atoi(buffer);

	if(!get_global_config_data("PServer","DontUseDomain",buffer,sizeof(buffer)))
		force_local_machine = atoi(buffer);
#endif

	osv.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	GetVersionEx(&osv);
	if (osv.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS)
		bIsWin95 = 1;
	else if(osv.dwMajorVersion==4)
		bIsNt4=1;
	else
		bIsWin2k=1;

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

	/* Cygwin default nsec is on, ntea is off */
	/* We default ntea for minimum impact at the client side.  The CYGWIN variable
	   will override this for us */
	use_ntsec = 0;
	use_ntea = 1;

	if(GetEnvironmentVariable("CYGWIN",buffer,sizeof(buffer)))
	{
		if(strstr(buffer,"ntsec"))
		{ 
			use_ntsec = !strstr(buffer,"nontsec");
			if(use_ntsec) use_ntea=0;
		}
		if(strstr(buffer,"ntea"))
		{
			use_ntea = !strstr(buffer,"nontea");
			if(use_ntea) use_ntsec=0;
		}
	}

	if(GetEnvironmentVariable("CVSNT",buffer,sizeof(buffer)))
	{
		if(strstr(buffer,"ntsec"))
		{
			use_ntsec = !strstr(buffer,"nontsec");
			if(use_ntsec) use_ntea=0;
		}
		if(strstr(buffer,"ntea"))
		{
			use_ntea = !strstr(buffer,"nontea");
			if(use_ntea) use_ntsec=0;
		}
	}
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
    for (i = 0; i < max_length - 1;)
    {
		c=0;
		c = read_key();
		if(c==27 || c=='\r' || c=='\n')
			break;
		else if(c==8 && i)
		{
		  fputs("\b \b",stdout);
		  --i; 
		}
		else if(c>31)
		{
		  password[i++]=c;
		  fputc('*',stdout);
		}
		fflush (stderr);
		fflush (stdout);
    }
	SetConsoleMode(hInput,dwMode);
	FlushConsoleInputBuffer(hInput);
    password[i] = '\0';
    fputs ("\n", stderr);
	return c==27?NULL:password;
}

#ifndef CVS95
DWORD BreakNameIntoParts(LPCSTR name, LPWSTR w_name, LPWSTR w_domain, LPWSTR w_pdc)
{
	static wchar_t *pw_pdc;
    char *ptr;
	wchar_t w_defaultdomain[DNLEN+1]={0};

	// only fetch a domain controller if the machine is a domain member
	if(isDomainMember(w_defaultdomain))
	{
		TRACE(3,"Machine is domain member");
		ptr=strchr(name, '\\');
  		if (ptr)
  		{
 			w_name[MultiByteToWideChar(CP_ACP,0,ptr+1,-1,w_name,UNLEN+1)]='\0';
			w_domain[MultiByteToWideChar(CP_ACP,0,name,ptr-name,w_domain,DNLEN)]='\0';
   		}
		else
		{
 			w_name[MultiByteToWideChar(CP_ACP,0,name,-1,w_name,UNLEN+1)]='\0';
			wcscpy(w_domain,w_defaultdomain);
		}
	}
  	else
	{
		TRACE(3,"Machine is standalone");
		if(strchr(name,'\\'))
		{
			fprintf(stderr,"error 0 Invalid username - cannot specify domain as server is not acting as a domain member\n");
			fflush(stderr);
			return ERROR_INVALID_PARAMETER;
		}
 		w_name[MultiByteToWideChar(CP_ACP,0,name,-1,w_name,UNLEN+1)]='\0';
		*w_domain='\0';
	}

	if(w_pdc)
	{
		DWORD (WINAPI *pDsGetDcNameW)(LPCWSTR ComputerName,LPCWSTR DomainName,GUID *DomainGuid,LPCWSTR SiteName,ULONG Flags,PDOMAIN_CONTROLLER_INFOW *DomainControllerInfo);
		pDsGetDcNameW=GetProcAddress(GetModuleHandle("netapi32"),"DsGetDcNameW");

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
#endif

#ifdef SERVER_SUPPORT
int win32_valid_user(const char *username, const char *password, const char *domain, user_handle_t *user_handle)
{
	HANDLE handle = NULL;
    wchar_t User[UNLEN+1];
    wchar_t Domain[DNLEN+1];
	wchar_t Password[UNLEN+1];
	char user[UNLEN+DNLEN+2];

    memset(User, '\0', sizeof User);
    memset(Domain, '\0', sizeof Domain);

	if(domain)
		sprintf(user,"%s\\%s",domain,username);
	else
		strcpy(user,username);

	if(BreakNameIntoParts(user, User, Domain, NULL))
		return 0;

	Password[MultiByteToWideChar(CP_UTF8,0, password, -1, Password, (sizeof(Password)/sizeof(TCHAR))-1)]='\0';

	if(!LogonUserW(User,Domain,Password,LOGON32_LOGON_NETWORK,LOGON32_PROVIDER_DEFAULT,&handle))
	{
		switch(GetLastError())
		{
			case ERROR_SUCCESS:
				break;
			case ERROR_NO_SUCH_DOMAIN:
			case ERROR_NO_SUCH_USER:
			case ERROR_ACCOUNT_DISABLED:
			case ERROR_PASSWORD_EXPIRED:
			case ERROR_ACCOUNT_RESTRICTION:
				/* Do we want to print something here? */
				break;
			case ERROR_PRIVILEGE_NOT_HELD:
			case ERROR_ACCESS_DENIED:
				/* Technically can't happen (due to check for seTcbName above) */
				cvs_outerr("error 0 Cannot login: Server has insufficient rights to validate user account - contact your system administrator\n",0);
				cvs_flusherr();
				break;
			default:
				break;
		}
	}
	if(user_handle)
		*user_handle=handle;
	else if(user)
		CloseHandle(handle);
	return handle?1:0;
}
#endif

struct passwd *win32getpwnam(const char *name)
{
#ifdef WINNT_VERSION // Win95 doesn't support this...  Client only mode...
	static struct passwd pw;
	USER_INFO_1 *pinfo = NULL;
	static wchar_t w_name[UNLEN+1];
	static wchar_t w_domain[DNLEN+1];
	static wchar_t w_pdc[DNLEN+1];
	static char homedir[1024];
	NET_API_STATUS res;

	memset(&pw,0,sizeof(pw));

	TRACE(3,"win32getpwnam(%s)",name);

	BreakNameIntoParts(name,w_name,w_domain,w_pdc);

	// If by sad chance this gets called on Win95, there is
	// no way of verifying user info, and you always get 'Not implemented'.
	// We are stuck in this case...
	res=NetUserGetInfo(w_pdc,w_name,1,(BYTE**)&pinfo);
	if(res==NERR_UserNotFound)
	{
		TRACE(3,"NetUserGetInfo returned NERR_UserNotFound - failing");
		return NULL;
	}

	pw.pw_uid=0;
	pw.pw_gid=0;
	pw.pw_name=name;
	pw.pw_passwd="secret";
	pw.pw_shell="cmd.exe";
	pw.pw_pdc_w=w_pdc;
	pw.pw_domain_w=w_domain;
	pw.pw_name_w = w_name;

	if(res==NERR_Success)
	{
		WideCharToMultiByte(CP_ACP,0,pinfo->usri1_home_dir,-1,homedir,sizeof(homedir),NULL,NULL);
		if(*homedir) pw.pw_dir=homedir;
	}
	else
		pw.pw_dir=get_homedir();

	if(pinfo)
		NetApiBufferFree(pinfo);
	return &pw;
#else // Win95 broken version.  Rely on the HOME environment variable...
	static struct passwd pw = {0};
	pw.pw_name=(char*)name;
	pw.pw_passwd="secret";
	pw.pw_shell="cmd.exe";
	pw.pw_dir=get_homedir();
#endif
	return &pw;
}

char *win32getlogin()
{
	static char UserName[UNLEN+1];
	DWORD len=sizeof(UserName);

	if(!GetUserNameA(UserName,&len))
		return NULL;

	/* Patch for cygwin sshd suggested by Markus Kuehni */
	if(!strcmp(UserName,"SYSTEM"))
	{
		/* First try logname, and if that fails try user */
		if(!GetEnvironmentVariable("LOGNAME",UserName,sizeof(UserName)))
			GetEnvironmentVariable("USER",UserName,sizeof(UserName));
	}
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

int trys4u(const struct passwd *pw, user_handle_t *user_handle)
{
	/* XP actually implements this but drops out early in the processing
		because it's in workstation mode not server */
	/* Also for this to succeed your PDC needs to be a Win2k3 machine */
	TRACE(3,"Trying S4u...\n");
	switch(nt_s4u(pw->pw_domain_w,pw->pw_name_w,user_handle))
	{
		case ERROR_SUCCESS:
			return 0;
		case ERROR_NO_SUCH_DOMAIN:
		case ERROR_NO_SUCH_USER:
			return 2;
		case ERROR_ACCOUNT_DISABLED:
		case ERROR_PASSWORD_EXPIRED:
		case ERROR_ACCOUNT_RESTRICTION:
			return 3;
		case ERROR_PRIVILEGE_NOT_HELD:
		case ERROR_ACCESS_DENIED:
			return 1;
		default:
			return -1;
	}
}

int trysuid(const struct passwd *pw, user_handle_t *user_handle)
{
	TRACE(3,"Trying Setuid helper...\n");
	switch(SuidGetImpersonationTokenW(pw->pw_name_w,pw->pw_domain_w,LOGON32_LOGON_NETWORK,user_handle))
	{
		case ERROR_SUCCESS:
			return 0;
		case ERROR_NO_SUCH_DOMAIN:
		case ERROR_NO_SUCH_USER:
			return 2;
		case ERROR_ACCOUNT_DISABLED:
		case ERROR_PASSWORD_EXPIRED:
		case ERROR_ACCOUNT_RESTRICTION:
			return 3;
		case ERROR_PRIVILEGE_NOT_HELD:
		case ERROR_ACCESS_DENIED:
			return 1;
		default:
			return -1;
	}
}

int trytoken(const struct passwd *pw, user_handle_t *user_handle)
{
	TRACE(3,"Trying NTCreateToken...\n");
	if(nt_setuid(pw->pw_pdc_w,pw->pw_name_w,user_handle))
		return 1;
	return 0;
}
/*
  Returns:
     0 = ok
	 1 = user found, impersonation failed
	 2 = user not found
	 3 = Account disabled
*/

int win32switchtouser(const char *username, user_handle_t *user_handle)
{
	int ret=-1;

	/* Store the user for later */
	current_username = xstrdup(username);

	TRACE(3,"win32switchtouser(%s), impersonate=%d",username,impersonate);
	if(impersonate)
	{
#ifdef _DEBUG
		*user_handle=NULL;
#endif
		if(!*user_handle)
		{
			const struct passwd *pw;

			pw = win32getpwnam(username);

			if(!pw)
				return 2;

			if(bIsWin2k && (ret = trys4u(pw,user_handle))>0)
				return ret;
			if(bIsWin2k && ret && (ret = trysuid(pw,user_handle))>0)
				return ret;
			if(ret && (ret = trytoken(pw,user_handle))>0)
				return ret;
			if(ret<0)
				return 1;
		}
		/* If we haven't bailed out above, user_handle will be set */
		return ImpersonateLoggedOnUser(*user_handle)?0:1;
	}
	else
	{
		char user[100];
		DWORD dw = sizeof(user);
		GetUserName(user,&dw);
		TRACE(3,"My username=%s",user);
		return 0;
	}
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
		wchar_t w_name[UNLEN+1];
		wchar_t w_domain[DNLEN+1];
		wchar_t w_pdc[DNLEN+1];
		DWORD priv;
		BOOL check_local = FALSE; /* If the domain wasn't specified, try checking for local admin too */

		if(BreakNameIntoParts(CVS_Username,w_name,w_domain,w_pdc))
			return 0;

		if(NetUserGetInfo(w_pdc,w_name,1,(LPBYTE*)&info)!=NERR_Success)
			return 0;

		priv=info->usri1_priv;
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

int isDomainMember(wchar_t *wszDomain)
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

int win32_makepipe(long hPipe)
{
	return _open_osfhandle(hPipe,_O_RDWR|_O_BINARY);
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
#ifdef UTF8
	const char *src = slash_convert(filename,fname);
	wchar_t dst[MAX_PATH],wmode[32];
	FILE *f;
	MultiByteToWideChar(CP_UTF8,0,src,-1,dst,sizeof(dst));
	MultiByteToWideChar(CP_UTF8,0,mode,-1,wmode,sizeof(wmode));
	f = _wfopen(dst,wmode);
#else
	FILE *f = fopen(slash_convert(filename,fname),mode);
#endif
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

		itime.QuadPart -= (bias * 60) *systemtime_second;
    }

	*(ULARGE_INTEGER*)pft=itime;

    return success;
}

#ifndef CVS95
static BOOL GetFileSec(LPCTSTR strPath, HANDLE hFile, SECURITY_INFORMATION requestedInformation, PSECURITY_DESCRIPTOR *ppSecurityDescriptor)
{
	if(hFile)
	{
		DWORD dwLen = 0;
		GetKernelObjectSecurity(hFile,requestedInformation,NULL,0,&dwLen);
		if(!dwLen)
			return FALSE;
		*ppSecurityDescriptor = (PSECURITY_DESCRIPTOR)xmalloc(dwLen);
		if(!GetKernelObjectSecurity(hFile,requestedInformation,*ppSecurityDescriptor,dwLen,&dwLen))
		{
			xfree(*ppSecurityDescriptor);
			return FALSE;
		}
		return TRUE;
	}
	else
	{
		DWORD dwLen = 0;
		GetFileSecurity(strPath,requestedInformation,NULL,0,&dwLen);
		if(!dwLen)
			return FALSE;
		*ppSecurityDescriptor = (PSECURITY_DESCRIPTOR)xmalloc(dwLen);
		if(!GetFileSecurity(strPath,requestedInformation,*ppSecurityDescriptor,dwLen,&dwLen))
		{
			xfree(*ppSecurityDescriptor);
			return FALSE;
		}
		return TRUE;
	}
}

static mode_t GetUnixFileModeNtSec(LPCTSTR strPath, HANDLE hFile)
{
	PSECURITY_DESCRIPTOR pSec;
	PACL pAcl;
	mode_t mode;
	int index;
	PSID pUserSid,pGroupSid;
	int haveuser,havegroup,haveworld;
	BOOL bPresent,bDefaulted;

	if(!GetFileSec(strPath,hFile,DACL_SECURITY_INFORMATION|OWNER_SECURITY_INFORMATION|GROUP_SECURITY_INFORMATION,&pSec))
		return 0;

	if(!GetSecurityDescriptorDacl(pSec,&bPresent,&pAcl,&bDefaulted))
	{
		xfree(pSec);
		return 0;
	}

	if(!pAcl)
	{
		xfree(pSec);
		if(bPresent) /* Null DACL */
			return 0755;
		else
			return 0; /* No DACL/No Access */
	}

	if(!GetSecurityDescriptorOwner(pSec,&pUserSid,&bDefaulted))
	{
		xfree(pSec);
		return 0;
	}
	if(!GetSecurityDescriptorGroup(pSec,&pGroupSid,&bDefaulted))
	{
		xfree(pSec);
		return 0;
	}

	mode=0;
	haveuser=havegroup=haveworld=0;
	for (index = 0; index < pAcl->AceCount && haveuser+havegroup+haveworld<3; ++index)
	{
		PACCESS_ALLOWED_ACE pACE;
		PSID pSid;
		mode_t mask,mask2;
		if (!GetAce(pAcl, index, (LPVOID*)&pACE))
			continue;
		pSid = (PSID)&pACE->SidStart;
		if(!haveuser && pUserSid && EqualSid(pSid,pUserSid))
		{
			mask=0700;
			haveuser=1;
		}
		else if(!havegroup && pGroupSid && EqualSid(pSid,pGroupSid))
		{
			mask=0070;
			havegroup=1;
		}
		else if(!haveworld && EqualSid(pSid,&sidEveryone))
		{
			mask=0007;
			haveworld=1;
		}
		else continue;

		mask2=0;
		if((pACE->Mask&FILE_GENERIC_READ)==FILE_GENERIC_READ)
			mask2|=0444;
		if((pACE->Mask&FILE_GENERIC_WRITE)==FILE_GENERIC_WRITE)
			mask2|=0222;
		if((pACE->Mask&FILE_GENERIC_EXECUTE)==FILE_GENERIC_EXECUTE)
			mask2|=0111;

		if(pACE->Header.AceType==ACCESS_ALLOWED_ACE_TYPE)
			mode|=(mask2&mask);
		else if(pACE->Header.AceType==ACCESS_DENIED_ACE_TYPE)
			mode&=~(mask2&mask);
	}

	xfree(pSec);

	/* We could use GetEffectiveRightsFromAcl here but don't want to as it takes
	   into account annoying things like inheritance.  All we want to do is transport
	   the mode bits to/from a unix server relatively unmolested */
	/* If there's no group ownership, make group = world, and if there's no owner ownership,
	   make owner=group.  This should only normally happen on 'legacy' checkins as the
	   files in the sandbox will have both set by SetUnixFileMode. */
	if(!havegroup)
		mode|=(mode&0007)<<3;
	if(!haveuser)
		mode|=(mode&0070)<<3;

	TRACE(3,"GetUnixFileModeNtSec(%s,%p) returns %04o",strPath,hFile,mode);

	return mode;
}

static BOOL SetUnixFileModeNtSec(LPCTSTR strPath, mode_t mode)
{
	PSECURITY_DESCRIPTOR pSec;
	SECURITY_DESCRIPTOR NewSec;
	PACL pAcl,pNewAcl;
	PSID pUserSid,pGroupSid;
	int n;
	BOOL bPresent,bDefaulted;

	TRACE(3,"SetUnixFileModeNtSec(%s,%04o)",strPath,mode);

	if(!GetFileSec(strPath,NULL,DACL_SECURITY_INFORMATION|OWNER_SECURITY_INFORMATION|GROUP_SECURITY_INFORMATION,&pSec))
		return 0;
	if(!GetSecurityDescriptorDacl(pSec,&bPresent,&pAcl,&bDefaulted))
	{
		xfree(pSec);
		return 0;
	}
	if(!GetSecurityDescriptorOwner(pSec,&pUserSid,&bDefaulted))
	{
		xfree(pSec);
		return 0;
	}
	if(!GetSecurityDescriptorGroup(pSec,&pGroupSid,&bDefaulted))
	{
		xfree(pSec);
		return 0;
	}

	if(pAcl)
	{
		pNewAcl=(PACL)xmalloc(pAcl->AclSize+256);
		InitializeAcl(pNewAcl,pAcl->AclSize+256,ACL_REVISION);
		/* Delete any ACEs that equal our data */
		for(n=0; n<(int)pAcl->AceCount; n++)
		{
			ACCESS_ALLOWED_ACE *pAce;
			GetAce(pAcl,n,(LPVOID*)&pAce);
			if(!((pUserSid && EqualSid((PSID)&pAce->SidStart,pUserSid)) ||
				(pGroupSid && EqualSid((PSID)&pAce->SidStart,pGroupSid)) ||
			   (EqualSid((PSID)&pAce->SidStart,&sidEveryone))))
			{
				AddAce(pNewAcl,ACL_REVISION,MAXDWORD,pAce,pAce->Header.AceSize);
			}
		}
	}
	else
	{
		pNewAcl=xmalloc(1024);
		InitializeAcl(pNewAcl,1024,ACL_REVISION);
	}

	/* Poor mans exception handling...  better to go C++ really */
	do
	{
		/* Always grant rw + control to the owner, no matter what else happens */
		if(pUserSid && !AddAccessAllowedAce(pNewAcl,ACL_REVISION,STANDARD_RIGHTS_ALL|FILE_GENERIC_READ|FILE_GENERIC_WRITE|((mode&0100)?FILE_GENERIC_EXECUTE:0),pUserSid))
			break;
		if(pGroupSid && !AddAccessAllowedAce(pNewAcl,ACL_REVISION,((mode&0040)?FILE_GENERIC_READ:0)|((mode&0020)?(FILE_GENERIC_WRITE|DELETE):0)|((mode&0010)?FILE_GENERIC_EXECUTE:0),pGroupSid))
			break;
		if(!AddAccessAllowedAce(pNewAcl,ACL_REVISION,((mode&0004)?FILE_GENERIC_READ:0)|((mode&0002)?(FILE_GENERIC_WRITE|DELETE):0)|((mode&0001)?FILE_GENERIC_EXECUTE:0),&sidEveryone))
			break;

		if(!InitializeSecurityDescriptor(&NewSec,SECURITY_DESCRIPTOR_REVISION))
			break;

		if(!SetSecurityDescriptorDacl(&NewSec,TRUE,pNewAcl,FALSE))
			break;

		if(!SetFileSecurity(strPath,DACL_SECURITY_INFORMATION|PROTECTED_DACL_SECURITY_INFORMATION,&NewSec))
			break;
	} while(0);

	xfree(pSec);
	xfree(pNewAcl);

	return TRUE;
}

typedef NTSTATUS (NTAPI *NtQueryEaFile_t)(HANDLE FileHandle, PIO_STATUS_BLOCK IoStatusBlock, PVOID Buffer, ULONG Length, BOOL ReturnSingleEntry, PVOID EaList, ULONG EaListLength,PULONG EaIndex, IN BOOL RestartScan);
typedef NTSTATUS (NTAPI *NtSetEaFile_t)(HANDLE FileHandle, PIO_STATUS_BLOCK IoStatusBlock, PVOID EaBuffer,ULONG EaBufferSize);

static NtQueryEaFile_t pNtQueryEaFile;
static NtSetEaFile_t pNtSetEaFile;

typedef struct _FILE_FULL_EA_INFORMATION
{
	ULONG NextEntryOffset;
	BYTE Flags;
	BYTE EaNameLength;
	USHORT EaValueLength;
	CHAR EaName[1];
} FILE_FULL_EA_INFORMATION, *PFILE_FULL_EA_INFORMATION;

typedef struct _FILE_GET_EA_INFORMATION
{
	ULONG	NextEntryOffset;
	BYTE	EaNameLength;
	CHAR	EaName[1];
} FILE_GET_EA_INFORMATION, *PFILE_GET_EA_INFORMATION;

static mode_t GetUnixFileModeNtEA(LPCTSTR strPath, HANDLE hFile)
{
	mode_t mode = 0;

	if(!pNtQueryEaFile)
		pNtQueryEaFile=(NtQueryEaFile_t)GetProcAddress(GetModuleHandle("NTDLL"),"NtQueryEaFile");
	if(pNtQueryEaFile)
	{
		if(strPath)
			hFile = CreateFile(strPath, FILE_READ_EA, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING,FILE_FLAG_BACKUP_SEMANTICS, NULL);

		if(hFile != INVALID_HANDLE_VALUE)
		{
			IO_STATUS_BLOCK io = {0};
			BYTE buffer[256];
			NTSTATUS status;
			FILE_GET_EA_INFORMATION *list;
			FILE_FULL_EA_INFORMATION *pea;
			static const char *Attr = ".UNIXATTR";

			list=(FILE_GET_EA_INFORMATION*)buffer;
			list->NextEntryOffset=0;
			list->EaNameLength=strlen(Attr);
			memcpy(list->EaName,Attr,list->EaNameLength+1);

			status = pNtQueryEaFile(hFile, &io, buffer, sizeof(buffer), TRUE, buffer, sizeof(buffer), NULL, TRUE);
			switch(status)
			{
			case STATUS_SUCCESS:
				pea = (FILE_FULL_EA_INFORMATION*)buffer;
				mode=*(mode_t*)&pea->EaName[pea->EaNameLength+1];
				break;
			case STATUS_NONEXISTENT_EA_ENTRY:
				TRACE(3,"error: STATUS_NONEXISTENT_EA_ENTRY");
				break;
			case STATUS_NO_EAS_ON_FILE:
				TRACE(3,"error: STATUS_NO_EAS_ON_FILE");
				break;
			case STATUS_EA_CORRUPT_ERROR:
				TRACE(3,"error: STATUS_EA_CORRUPT_ERROR");
				break;
			default:
				TRACE(3,"error %08x",error);
				break;
			}

			if(strPath)
				CloseHandle(hFile);
		}
	}

	TRACE(3,"GetUnixFileModeNtEA(%s,%p) returns %04o",strPath,hFile,mode);
	return mode;
}

static BOOL SetUnixFileModeNtEA(LPCTSTR strPath, mode_t mode)
{
	HANDLE hFile = INVALID_HANDLE_VALUE;

	TRACE(3,"SetUnixFileModeNtEA(%s,%04o)",strPath,mode);

	if(!pNtSetEaFile)
		pNtSetEaFile=(NtSetEaFile_t)GetProcAddress(GetModuleHandle("NTDLL"),"NtSetEaFile");

	if(pNtSetEaFile)
	{
		if(strPath)
			hFile = CreateFile(strPath, FILE_READ_EA|FILE_WRITE_EA, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING,FILE_FLAG_BACKUP_SEMANTICS, NULL);

		if(hFile != INVALID_HANDLE_VALUE)
		{
			IO_STATUS_BLOCK io = {0};
			BYTE buffer[256];
			NTSTATUS status;
			static const char *Attr = ".UNIXATTR";
			FILE_FULL_EA_INFORMATION *pea;

			pea = (FILE_FULL_EA_INFORMATION*)buffer;

			pea->NextEntryOffset=0;
			pea->Flags=0;
			pea->EaNameLength=strlen(Attr);
			pea->EaValueLength=sizeof(mode);
			memcpy(pea->EaName,Attr,pea->EaNameLength+1);
			*(mode_t*)(pea->EaName+pea->EaNameLength+1)=mode;

			status = pNtSetEaFile(hFile, &io, buffer, sizeof(buffer));

			if(strPath)
				CloseHandle(hFile);
		}
	}

	return FALSE;
}

#endif

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

	if(!(bhfi.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
	{
#ifndef CVS95
		if(is_gmt_fs) /* No point on doing this in FAT */
		{
			/* We're assuming a reasonable failure mode on FAT32 here */
			if(use_ntsec)
				buf->st_mode = GetUnixFileModeNtSec(filename,hFile);
			else if(use_ntea)
				buf->st_mode = GetUnixFileModeNtEA(filename,hFile);
			else
				buf->st_mode = 0;
			if(!buf->st_mode)
				buf->st_mode = 0644;
		}
		else
#else
		{
			buf->st_mode = 0644;
		}
#endif
		if ( bhfi.dwFileAttributes & FILE_ATTRIBUTE_READONLY)
			buf->st_mode &=~ (_S_IWRITE + (_S_IWRITE >> 3) + (_S_IWRITE >> 6));
		else
			buf->st_mode |= _S_IWRITE;
	}
	else
	{
		/* We always assume under NT that directories are 0755.  Since Unix permissions don't
		   map properly to NT directories anyway it's probably sensible */
		buf->st_mode = 0755;
	}
	
    // Potential, but unlikely problem... casting DWORD to short.
    // Reported by Jerzy Kaczorowski, addressed by Jonathan Gilligan
    // if the number of links is greater than 0x7FFF
    // there would be an overflow problem.
    // This is a problem inherent in the struct stat, and hence
    // in the design of the C library.
    if (bhfi.nNumberOfLinks > SHRT_MAX)
	{
        error(0,0,"Internal error: too many links to a file.");
        buf->st_nlink = SHRT_MAX;
    }
    else
	{
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
				// UNC pathname: Extract server and share and pass it to GVI
				char szRootPath[MAX_PATH + 1] = "\\\\";
				const char *p = &filename[2];
				char *q = &szRootPath[2];
				int n;
				for (n = 0; n < 2; n++)
				{
					// Get n-th path element
					while (*p != 0 && *p != '/' && *p != '\\')
					{
						*q = *p;
						p++;
						q++;
					}
					// Add separator
					if (*p != 0)
					{
						*q = '\\';
						p++;
						q++;
					}
				}
				*q = 0;

				*szFs='\0';
				GetVolumeInformationA(szRootPath,NULL,0,NULL,NULL,NULL,szFs,sizeof(szFs));
				is_gmt_fs = GMT_FS(szFs);
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
#undef chmod
#undef asctime
#undef ctime
#undef utime

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

int wnt_chmod (const char *name, mode_t mode)
{
	TRACE(3,"wnt_chmod(%s,%04o)",name,mode);
	if(chmod(name,mode)<0)
		return -1;

#ifndef CVS95
	if(!(GetFileAttributes(name)&FILE_ATTRIBUTE_DIRECTORY))
	{
		if(use_ntsec)
			SetUnixFileModeNtSec(name,mode);
		else if(use_ntea)
			SetUnixFileModeNtEA(name,mode);
	}
#endif
	return 0;
}

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
			// UNC pathname: Extract server and share and pass it to GVI
			char szRootPath[MAX_PATH + 1] = "\\\\";
			const char *p = &filename[2];
			char *q = &szRootPath[2];
			int n;
			for (n = 0; n < 2; n++)
			{
				// Get n-th path element
				while (*p != 0 && *p != '/' && *p != '\\')
				{
					*q = *p;
					p++;
					q++;
				}
				// Add separator
				if (*p != 0)
				{
					*q = '\\';
					p++;
					q++;
				}
			}
			*q = 0;

			*szFs='\0';
			GetVolumeInformationA(szRootPath,NULL,0,NULL,NULL,NULL,szFs,sizeof(szFs));
			is_gmt_fs = GMT_FS(szFs);
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
		LocalFileTimeToFileTime  ( &fAt, &At );
		LocalFileTimeToFileTime  ( &fWt, &Wt );
	}
	h = CreateFileA(f,GENERIC_READ|GENERIC_WRITE,0,NULL,OPEN_EXISTING,0,NULL);
	if(h==INVALID_HANDLE_VALUE)
	{
		DWORD dwErr = GetLastError();
		return -1;
	}
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
			if (MessageBox( NULL, _T("Unfortunately CVSNT has crashed.  Would you like to produce a crash dump?"), _T("cvsnt"), MB_YESNO|MB_SERVICE_NOTIFICATION )==IDYES)
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
		MessageBox( NULL, szResult, _T("cvsnt"), MB_OK|MB_SERVICE_NOTIFICATION );
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

int getmode(int fd)
{
	unsigned char mode = _osfile(fd);
	if(mode&FTEXT)
		return _O_TEXT;
	else
		return _O_BINARY;
}

#ifndef CVS95
#undef main
int main(int argc, char **argv);

int wmain(int argc, wchar_t **argv)
{
	char **newargv = (char**)xmalloc(sizeof(char*)*(argc+1)),*buf, *p;
	int n, ret, len;

#ifdef UTF8
	int cp = CP_UTF8;
#else
	int cp = CP_ACP;
#endif

	len=0;
	for(n=0; n<argc; n++)
		len+= WideCharToMultiByte(cp,0,argv[n],-1,NULL,0,NULL,NULL);
	buf=p=xmalloc(len);
	for(n=0; n<argc; n++)
	{
		newargv[n]=p;
		p+=WideCharToMultiByte(cp,0,argv[n],-1,newargv[n],len,NULL,NULL);
	}
	newargv[n]=NULL;
	ret = main(argc,newargv);
	xfree(buf);
	xfree(newargv);
	return ret;
}
#endif

int w32_is_network_share(const char *directory)
{
	char drive[]="z:\\";
	char dir[MAX_PATH];

	if(strlen(directory)<2)
		return 0;

	if(ISDIRSEP(directory[0]) && ISDIRSEP(directory[1]))
		return 1;

	if(directory[1]==':')
	{
		drive[0]=directory[0];
		if(GetDriveType(drive)==DRIVE_REMOTE)
			return 1;
		return 0;
	}

	GetCurrentDirectory(sizeof(dir),dir);
	if(dir[1]==':')
	{
		drive[0]=dir[0];
		if(GetDriveType(drive)==DRIVE_REMOTE)
			return 1;
	}
	return 0;
}

/* Courtesy of anonymous, via google */
char *ConvertFilespecToCorrectCase(char *aFullFileSpec)
// aFullFileSpec must be a modifiable string
// since it will be converted to proper case.
// Returns aFullFileSpec, the contents of which
// have been converted to the case used by the
// file system.  Note: The trick of changing
// the current directory to be that of
// aFullFileSpec and then calling GetFullPathName()
// doesn't always work.  So perhaps the
// only easy way is to call FindFirstFile() on each
// directory that composes aFullFileSpec, which
// is what is done here.
{
 // Longer in case of UNCs and such,
 // which might be longer than MAX_PATH:
 #define WORK_AREA_SIZE (MAX_PATH * 2)

	size_t length;
    char built_filespec[WORK_AREA_SIZE], *dir_start, *dir_end, *dir_last;
	int chars_to_copy;
	WIN32_FIND_DATA found_file;
	HANDLE file_search;

	if (!aFullFileSpec || !*aFullFileSpec)
		return aFullFileSpec;
	length = strlen(aFullFileSpec);

	if (length < 2 || length >= WORK_AREA_SIZE)
		return aFullFileSpec;

	// Start with something easy, the drive letter:
	if (aFullFileSpec[1] == ':')
		aFullFileSpec[0] = toupper(aFullFileSpec[0]);
	// else it might be a UNC that has no drive letter.
	if (dir_start = strchr(aFullFileSpec, ':'))
	// Skip over 1st backslash that goes with drive letter.
		dir_start += 2;
	else // it's probably a UNC (handling of UNCs hasn't been tested)
	{
		if (!strncmp(aFullFileSpec, "//", 2))
			// I think MS says you can't use FindFirstFile() directly
			// on a share name, so we want to omit that from consideration
			// (i.e. we don't attempt to find its proper case):
			dir_start = aFullFileSpec + 2;
		else if(aFullFileSpec[0]=='/')
			dir_start = aFullFileSpec + 1;
		else
			dir_start = aFullFileSpec;
	}
	// Init the new string (the filespec we're building),
	// e.g. copy just the "c:\\" part.
	chars_to_copy = dir_start - aFullFileSpec;
	memcpy(built_filespec, aFullFileSpec, chars_to_copy);
	built_filespec[chars_to_copy] = '\0';

	dir_last = NULL;
	for (dir_end = dir_start; dir_end = strchr(dir_end, '/'); dir_last = ++dir_end)
	{
		*dir_end = '\0';  // Temporarily terminate.
		if(dir_last && !strcmp(dir_last,"."))
		{
			*dir_end='/';
			continue;
		}
		if(dir_last && !strcmp(dir_last,".."))
		{
			built_filespec[strlen(built_filespec)-1]='\0';
			dir_last = strrchr(built_filespec,'/');
			if(dir_last > (built_filespec+(dir_start - aFullFileSpec)))
				*dir_last = '\0';
			else
				built_filespec[dir_start - aFullFileSpec]='\0';
			continue;
		}
		file_search = FindFirstFile(aFullFileSpec, &found_file);
		if (file_search == INVALID_HANDLE_VALUE)
		{
			DWORD dwError = GetLastError();
			if(dwError==ERROR_ACCESS_DENIED)
				error(0,0,"Access denied looking up %s - results may not be correct",aFullFileSpec);
			*dir_end = '/'; // Restore it before we do anything else.
			return aFullFileSpec;
		}
		*dir_end = '/'; // Restore it before we do anything else.
		FindClose(file_search);
		// Append the case-corrected version of this directory name:
		strcat(built_filespec, found_file.cFileName);
		strcat(built_filespec, "/");
	}

	if(dir_last && !strcmp(dir_last,".."))
	{
		built_filespec[strlen(built_filespec)-1]='\0';
		dir_last = strrchr(built_filespec,'/');
		if(dir_last >= (built_filespec+(dir_start - aFullFileSpec)))
			*dir_last = '\0';
		else
			built_filespec[dir_start - aFullFileSpec]='\0';
		dir_last=".";
	}
	if(!dir_last || (*dir_last && strcmp(dir_last,".")))
	{
		// Now do the filename itself:
		if (   (file_search = FindFirstFile(aFullFileSpec, &found_file)) == INVALID_HANDLE_VALUE   )
		{
			DWORD dwError = GetLastError();
			if(dwError==ERROR_ACCESS_DENIED)
				error(0,0,"Access denied looking up %s - results may not be correct",aFullFileSpec);
			return aFullFileSpec;
		}
		strcat(built_filespec, found_file.cFileName);
	}
	FindClose(file_search);
 // It might be possible for the new one to be longer than the old,
 // e.g. if some 8.3 short names were converted to long names by the
 // process.  Thus, the caller should ensure that aFullFileSpec is
 // large enough:
	strcpy(aFullFileSpec, built_filespec);
	return aFullFileSpec;
}
