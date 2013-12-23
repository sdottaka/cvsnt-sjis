// Service.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "ServiceMsg.h"
#include <malloc.h>
#include <io.h>
#include <vector>
#include <shlwapi.h>

#include <winsock2.h>
#include <ws2tcpip.h>
#include <io.h>

#define SECURITY_WIN32
#include <security.h>
#include <ntsecapi.h>
#include <ntdsapi.h>
#include <dsgetdc.h>
#include <lm.h>

#include "../version_no.h"
#include "../version_fu.h"

#define SERVICE_NAMEA "CVS"
#define SERVICE_NAME _T("CVS")
#define DISPLAY_NAMEA "CVSNT"
#define DISPLAY_NAME _T("CVSNT")
#define NTSERVICE_VERSION_STRING "CVSNT Service " CVSNT_PRODUCTVERSION_STRING

static void CALLBACK ServiceMain(DWORD dwArgc, LPTSTR *lpszArgv);
static void CALLBACK ServiceHandler(DWORD fdwControl);
static BOOL NotifySCM(DWORD dwState, DWORD dwWin32ExitCode, DWORD dwProgress);
static char* basename(const char* str);
static LPCSTR GetErrorString();
static void AddEventSource(LPCTSTR szService, LPCTSTR szModule);
static void ReportError(BOOL bError, LPCSTR szError, ...);
static DWORD CALLBACK DoCvsThread(LPVOID lpParam);

static DWORD   g_dwCurrentState;
static SERVICE_STATUS_HANDLE  g_hService;
static std::vector<SOCKET> g_Sockets;
static BOOL g_bStop = FALSE;
static BOOL g_bTestMode = FALSE;
static int authserver_port = 2401;

int main(int argc, char* argv[])
{
    SC_HANDLE  hSCManager = NULL, hService = NULL;
    TCHAR szImagePath[MAX_PATH];
	HKEY hk;
	DWORD dwType;
	SERVICE_TABLE_ENTRY ServiceTable[] =
	{
		{ SERVICE_NAME, ServiceMain },
		{ NULL, NULL }
	};
	LPSTR szRoot;

	if(argc==1)
	{
		// Attempt to start service.  If this fails we're probably
		// not running as a service
		if(!StartServiceCtrlDispatcher(ServiceTable)) return 0;
	}
	if(argc<2 || (strcmp(argv[1],"-i") && strcmp(argv[1],"-reglsa") && strcmp(argv[1],"-u") && strcmp(argv[1],"-unreglsa") && strcmp(argv[1],"-test") && strcmp(argv[1],"-v") ))
	{
		fprintf(stderr, "CVSNT Service Handler\n\n"
                        "Arguments:\n"
                        "\t%s -i [cvsroot]\tInstall\n"
                        "\t%s -reglsa\tRegister LSA helper\n"
                        "\t%s -u\tUninstall\n"
                        "\t%s -unreglsa\tUnregister LSA helper\n"
                        "\t%s -test\tInteractive run\n"
                        "\t%s -v\tReport version number\n",
                        basename(argv[0]),basename(argv[0]),
                        basename(argv[0]), basename(argv[0]), 
                        basename(argv[0]), basename(argv[0]) 
                        );
		return -1;
	}

	if(!strcmp(argv[1],"-reglsa"))
	{
		TCHAR lsaBuf[10240];
		DWORD dwLsaBuf;

		if(RegOpenKeyEx(HKEY_LOCAL_MACHINE,_T("SYSTEM\\CurrentControlSet\\Control\\Lsa"),0,KEY_ALL_ACCESS,&hk))
		{
			fprintf(stderr,"Couldn't open LSA registry key, error %d\n",GetLastError());
			return -1;
		}
		dwLsaBuf=sizeof(lsaBuf);
		if(RegQueryValueEx(hk,_T("Authentication Packages"),NULL,&dwType,(BYTE*)lsaBuf,&dwLsaBuf))
		{
			fprintf(stderr,"Couldn't read LSA registry key, error %d\n",GetLastError());
			return -1;
		}
		if(dwType!=REG_MULTI_SZ)
		{
			fprintf(stderr,"LSA key isn't REG_MULTI_SZ!!!\n");
			return -1;
		}
		lsaBuf[dwLsaBuf]='\0';
		TCHAR *p = lsaBuf;
		while(*p)
		{
			if(!_tcscmp(p,"setuid"))
				break;
			p+=strlen(p)+1;
		}
		if(!*p)
		{
			strcpy(p,"setuid");
			dwLsaBuf+=strlen(p)+1;
			lsaBuf[dwLsaBuf]='\0';
			if(RegSetValueEx(hk,_T("Authentication Packages"),NULL,dwType,(BYTE*)lsaBuf,dwLsaBuf))
			{
				fprintf(stderr,"Couldn't write LSA registry key, error %d\n",GetLastError());
				return -1;
			}
		}
		return 0;
	}

	if(!strcmp(argv[1],"-unreglsa"))
	{
		TCHAR lsaBuf[10240];
		DWORD dwLsaBuf;

		if(RegOpenKeyEx(HKEY_LOCAL_MACHINE,_T("SYSTEM\\CurrentControlSet\\Control\\Lsa"),0,KEY_ALL_ACCESS,&hk))
		{
			fprintf(stderr,"Couldn't open LSA registry key, error %d\n",GetLastError());
			return -1;
		}
		dwLsaBuf=sizeof(lsaBuf);
		if(RegQueryValueEx(hk,_T("Authentication Packages"),NULL,&dwType,(BYTE*)lsaBuf,&dwLsaBuf))
		{
			fprintf(stderr,"Couldn't read LSA registry key, error %d\n",GetLastError());
			return -1;
		}
		if(dwType!=REG_MULTI_SZ)
		{
			fprintf(stderr,"LSA key isn't REG_MULTI_SZ!!!\n");
			return -1;
		}
		lsaBuf[dwLsaBuf]='\0';
		TCHAR *p = lsaBuf;
		while(*p)
		{
			if(!_tcscmp(p,"setuid"))
				break;
			p+=strlen(p)+1;
		}
		if(*p)
		{
			size_t l = strlen(p)+1;
			memcpy(p,p+l,(dwLsaBuf-((p+l)-lsaBuf))+1);
			dwLsaBuf-=l;
			if(RegSetValueEx(hk,_T("Authentication Packages"),NULL,dwType,(BYTE*)lsaBuf,dwLsaBuf))
			{
				fprintf(stderr,"Couldn't write LSA registry key, error %d\n",GetLastError());
				return -1;
			}
		}
		return 0;
	}

	if(RegCreateKeyEx(HKEY_LOCAL_MACHINE,_T("Software\\CVS\\Pserver"),NULL,_T(""),REG_OPTION_NON_VOLATILE,KEY_ALL_ACCESS,NULL,&hk,NULL))
	{ 
		fprintf(stderr,"Couldn't create HKLM\\Software\\CVS\\Pserver key, error %d\n",GetLastError());
		return -1;
	}

    if (!strcmp(argv[1],"-v")) {
        puts(NTSERVICE_VERSION_STRING);
        return 0;
        }

	if(!strcmp(argv[1],"-i"))
	{
		if(argc==3)
		{
			szRoot = argv[2];
			if(GetFileAttributesA(szRoot)==(DWORD)-1)
			{
				fprintf(stderr,"Repository directory '%s' not found\n",szRoot);
				return -1;
			}
			dwType=REG_SZ;
			RegSetValueExA(hk,"Repository0",NULL,dwType,(BYTE*)szRoot,strlen(szRoot)+1);
		}
		// connect to  the service control manager
		if((hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_CREATE_SERVICE)) == NULL)
		{
			fprintf(stderr,"OpenSCManager Failed\n");
			return -1;
		}

		if((hService=OpenService(hSCManager,SERVICE_NAME,DELETE))!=NULL)
		{
			DeleteService(hService);
			CloseServiceHandle(hService);
		}

		GetModuleFileName(NULL,szImagePath,MAX_PATH);
		if ((hService = CreateService(hSCManager,SERVICE_NAME,DISPLAY_NAME,
						STANDARD_RIGHTS_REQUIRED|SERVICE_CHANGE_CONFIG, SERVICE_WIN32_OWN_PROCESS|SERVICE_INTERACTIVE_PROCESS,
						SERVICE_AUTO_START, SERVICE_ERROR_NORMAL,
						szImagePath, NULL, NULL, NULL, NULL, NULL)) == NULL)
		{
			fprintf(stderr,"CreateService Failed: %s\n",GetErrorString());
			return -1;
		}
		{
			BOOL (WINAPI *pChangeServiceConfig2)(SC_HANDLE,DWORD,LPVOID);
			pChangeServiceConfig2=(BOOL (WINAPI *)(SC_HANDLE,DWORD,LPVOID))GetProcAddress(GetModuleHandle("advapi32"),"ChangeServiceConfig2A");
			if(pChangeServiceConfig2)
			{
				SERVICE_DESCRIPTION sd = { NTSERVICE_VERSION_STRING };
				if(!pChangeServiceConfig2(hService,SERVICE_CONFIG_DESCRIPTION,&sd))
				{
					0;
				}
			}
		}
		CloseServiceHandle(hService);
		CloseServiceHandle(hSCManager);
		ReportError(FALSE,DISPLAY_NAMEA " installed successfully");
		printf(DISPLAY_NAMEA " installed successfully\n");
	}
	
	RegCloseKey(hk);

	if(!strcmp(argv[1],"-u"))
	{
		// connect to  the service control manager
		if((hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_CREATE_SERVICE)) == NULL)
		{
			fprintf(stderr,"OpenSCManager Failed\n");
			return -1;
		}

		if((hService=OpenService(hSCManager,SERVICE_NAME,DELETE))==NULL)
		{
			fprintf(stderr,"OpenService Failed: %s\n",GetErrorString());
			return -1;
		}
		if(!DeleteService(hService))
		{
			fprintf(stderr,"DeleteService Failed: %s\n",GetErrorString());
			return -1;
		}
		CloseServiceHandle(hService);
		CloseServiceHandle(hSCManager);
		ReportError(FALSE,DISPLAY_NAMEA " uninstalled successfully");
		printf(DISPLAY_NAMEA " uninstalled successfully\n");
	}	
	else if(!strcmp(argv[1],"-test"))
	{
		ServiceMain(999,NULL);
	}
	return 0;
}

void CALLBACK ServiceMain(DWORD dwArgc, LPTSTR *lpszArgv)
{
	TCHAR szTmp[8192];
	char szTmpA[8192];
	TCHAR szTmp2[8192];
	char szAuthServer[32];
	DWORD dwTmp,dwType;
	HKEY hk;
	int seq=1,err;
	LPCSTR szNode;
	addrinfo *pAddrInfo;

	if(dwArgc!=999)
	{
		if (!(g_hService = RegisterServiceCtrlHandler(SERVICE_NAME,ServiceHandler))) { ReportError(TRUE,"Unable to start "SERVICE_NAMEA" - RegisterServiceCtrlHandler failed"); return; }
		NotifySCM(SERVICE_START_PENDING, 0, seq++);
	}
	else
	{
		g_bTestMode=TRUE;
		printf(SERVICE_NAMEA" " CVSNT_PRODUCTVERSION_STRING " ("__DATE__") starting in test mode.\n");
	}

	if(RegOpenKeyEx(HKEY_LOCAL_MACHINE,_T("SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment"),NULL,KEY_QUERY_VALUE,&hk))
	{ 
		ReportError(TRUE,"Unable to start "SERVICE_NAMEA" - Couldn't open environment key"); 
		if(!g_bTestMode)
			NotifySCM(SERVICE_STOPPED,0,0);
		return;
	}

	dwTmp=sizeof(szTmp);
	if(RegQueryValueEx(hk,_T("PATH"),NULL,&dwType,(BYTE*)szTmp,&dwTmp))
	{
		ReportError(TRUE,"Unable to start "SERVICE_NAMEA" - PATH environment variable not defined in system environment");
		if(!g_bTestMode)
			NotifySCM(SERVICE_STOPPED,0,0);
		return;
	}
	ExpandEnvironmentStrings(szTmp,szTmp2,sizeof(szTmp));
	SetEnvironmentVariable(_T("PATH"),szTmp2);

	RegCloseKey(hk);

	if(RegOpenKeyEx(HKEY_LOCAL_MACHINE,_T("Software\\CVS\\Pserver"),NULL,KEY_QUERY_VALUE,&hk))
	{
		ReportError(TRUE,"Unable to start "SERVICE_NAMEA" - Couldn't open HKLM\\Software\\CVS\\Pserver key");
		if(!g_bTestMode)
			NotifySCM(SERVICE_STOPPED,0,0);
		return;
	}

	dwTmp=sizeof(szTmp);
	if(RegQueryValueEx(hk,_T("TempDir"),NULL,&dwType,(LPBYTE)szTmp,&dwTmp) &&
	   SHRegGetUSValue(_T("SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment"),_T("TEMP"),NULL,(LPVOID)szTmp,&dwTmp,TRUE,NULL,0) &&
	   !GetEnvironmentVariable(_T("TEMP"),(LPTSTR)szTmp,sizeof(szTmp)) &&
	   !GetEnvironmentVariable(_T("TMP"),(LPTSTR)szTmp,sizeof(szTmp)))
	{
		_tcscpy(szTmp,_T("C:\\"));
	}

	SetEnvironmentVariable(_T("TEMP"),szTmp);
	SetEnvironmentVariable(_T("TMP"),szTmp);
	if(g_bTestMode)
		_tprintf(_T("TEMP/TMP currently set to %s\n"),szTmp);

	dwTmp=sizeof(DWORD);
	if(!RegQueryValueEx(hk,_T("PServerPort"),NULL,&dwType,(BYTE*)szTmp,&dwTmp))
	{
		authserver_port=*(DWORD*)szTmp;
	}
	itoa(authserver_port,szAuthServer,10);

// Initialisation
    WSADATA data;

    if(WSAStartup (MAKEWORD (1, 1), &data))
	{
		ReportError(TRUE,"WSAStartup failed... aborting - Error %d\n",WSAGetLastError());
		if(!g_bTestMode)
			NotifySCM(SERVICE_STOPPED,0,0);
		return;
	}

	dwTmp=sizeof(szTmpA);
	szNode = NULL;
	if(!RegQueryValueExA(hk,"BindAddress",NULL,&dwType,(BYTE*)szTmpA,&dwTmp))
	{
		if(stricmp(szTmpA,"*"))
			szNode = szTmpA;
	}

	addrinfo hint = {0};
	hint.ai_family=PF_UNSPEC;
	hint.ai_socktype=SOCK_STREAM;
	hint.ai_protocol=IPPROTO_TCP;
	hint.ai_flags=AI_PASSIVE;
	pAddrInfo=NULL;
	if(g_bTestMode)
	{
		printf("Initialising socket...");
	}
	err=getaddrinfo(szNode,szAuthServer,&hint,&pAddrInfo);
	if(g_bTestMode)
	{
		if(err)
			printf("failed (%s)\n",gai_strerror(err));
		else
		{
			if(!pAddrInfo)
			{
				printf("This server doesn't know how to bind tcp sockets!!!  Your sockets layer is broken!!!\n");
			}
			else
				printf("ok\n");
		}
	}
	if(err)
		ReportError(FALSE,"Failed to get ipv4 socket details: %s",gai_strerror(err));

	RegCloseKey(hk);

	if(!g_bTestMode)
		NotifySCM(SERVICE_START_PENDING, 0, seq++);

	int impersonate=1;
	if(!RegOpenKeyEx(HKEY_LOCAL_MACHINE,_T("Software\\CVS\\Pserver"),0,KEY_QUERY_VALUE,&hk))
	{
		DWORD dwType,dwImpersonate;
		DWORD dwTmp=sizeof(dwTmp);
		if(!RegQueryValueEx(hk,_T("Impersonation"),NULL,&dwType,(BYTE*)&dwImpersonate,&dwTmp))
			impersonate = dwImpersonate;
        RegCloseKey(hk);
        hk = 0;
	
	}
	if(impersonate)
		printf("Impersonation is enabled\n");
	else
		printf("*WARNING* Impersonation is disabled - all file access will be done as System user\n");

	if(g_bTestMode)
		printf("Starting auth server on port %d/tcp...\n",authserver_port);

	addrinfo* ai;
	for(ai=pAddrInfo;ai;ai=ai->ai_next)
	{
		SOCKET s = WSASocket(ai->ai_family,ai->ai_socktype,ai->ai_protocol,NULL,0,0);

		if(s!=-1 && !bind(s,ai->ai_addr,ai->ai_addrlen))
		{
			if(listen(s,50)==SOCKET_ERROR)
			{
				ReportError(TRUE,"Listen on socket failed: %s\n",gai_strerror(WSAGetLastError()));
				if(!g_bTestMode)
					NotifySCM(SERVICE_STOPPED,0,0);
				freeaddrinfo(pAddrInfo);
				return;
			} 
			g_Sockets.push_back(s);
		}
		else
		{
			if(g_bTestMode)
				printf("Socket Failed (Handle=%08x Family=%d,Socktype=%d,Protocol=%d): %s (not fatal)\n",s,ai->ai_family,ai->ai_socktype,ai->ai_protocol, gai_strerror(WSAGetLastError()));
			closesocket(s);
		}
	}
	freeaddrinfo(pAddrInfo);

	if(!g_Sockets.size())
	{
		ReportError(TRUE,"All socket binds failed.");
		if(!g_bTestMode)
			NotifySCM(SERVICE_STOPPED,0,0);
		return;
	}

	DWORD (WINAPI *pDsServerRegisterSpn)(DS_SPN_WRITE_OP Operation, LPCTSTR ServiceClass, LPCTSTR UserObjectDN);

    UINT oldMode=SetErrorMode(SEM_NOOPENFILEERRORBOX|SEM_FAILCRITICALERRORS);
	pDsServerRegisterSpn = (DWORD (WINAPI*)(DS_SPN_WRITE_OP,LPCTSTR,LPCTSTR))GetProcAddress(LoadLibrary("ntdsapi.dll"),"DsServerRegisterSpnA");
	SetErrorMode(oldMode);
	if(pDsServerRegisterSpn)
	{
		if(g_bTestMode)
			printf("Registering service SPN... ");
		pDsServerRegisterSpn(DS_SPN_DELETE_SPN_OP,"cvs",NULL);
		pDsServerRegisterSpn(DS_SPN_DELETE_SPN_OP,"cvs",NULL);
		if((dwTmp=pDsServerRegisterSpn(DS_SPN_ADD_SPN_OP,"cvs",NULL))!=0)
		{
			if(g_bTestMode)
				printf("failed (Error %d)\n",dwTmp);
			//ReportError(TRUE,"Registering cvs service SPN failed (error %d)", dwTmp);
		}
		else
		{
			if(g_bTestMode)
				printf("ok\n");
		}
	}

	// Process running, wait for closedown
	ReportError(FALSE,SERVICE_NAMEA" initialised successfully");
	if(!g_bTestMode)
		NotifySCM(SERVICE_RUNNING, 0, 0);

	g_bStop=FALSE;

	do
	{
		fd_set rfd;
		sockaddr_storage sin;
		size_t n;

		FD_ZERO(&rfd);
		for(n=0; n<g_Sockets.size(); n++)
			FD_SET(g_Sockets[n],&rfd);
		TIMEVAL tv = { 5, 0 }; // 5 seconds max wait
		int sel=select(1,&rfd,NULL,NULL,&tv);
		if(g_bStop || sel==SOCKET_ERROR) break; // Error on socket, or stopped
		for(n=0; n<g_Sockets.size(); n++)
		{
			if(FD_ISSET(g_Sockets[n],&rfd))
			{
				HANDLE hConn=(HANDLE)accept(g_Sockets[n],(struct sockaddr*)&sin,NULL);
				CloseHandle(CreateThread(NULL,0,DoCvsThread,(void*)hConn,0,NULL));
			}
		}
	} while(!g_bStop);

	if(pDsServerRegisterSpn)
	{
		if(g_bTestMode)
			printf("Unregistering service SPN...\n");
		pDsServerRegisterSpn(DS_SPN_DELETE_SPN_OP,"cvs",NULL);
	}
	NotifySCM(SERVICE_STOPPED, 0, 0);
	ReportError(FALSE,SERVICE_NAMEA" stopped successfully");
}

void CALLBACK ServiceHandler(DWORD fdwControl)
{
	switch(fdwControl)
	{      
	case SERVICE_CONTROL_STOP:
		OutputDebugString(SERVICE_NAME _T(": Stop\n"));
		NotifySCM(SERVICE_STOP_PENDING, 0, 0);
		g_bStop=TRUE;
		return;
	case SERVICE_CONTROL_INTERROGATE:
	default:
		break;
	}
	OutputDebugString(SERVICE_NAME _T(": Interrogate\n"));
	NotifySCM(g_dwCurrentState, 0, 0);
}

BOOL NotifySCM(DWORD dwState, DWORD dwWin32ExitCode, DWORD dwProgress)
{
	SERVICE_STATUS ServiceStatus;

	// fill in the SERVICE_STATUS structure
	ServiceStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
	ServiceStatus.dwCurrentState = g_dwCurrentState = dwState;
	ServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;
	ServiceStatus.dwWin32ExitCode = dwWin32ExitCode;
	ServiceStatus.dwServiceSpecificExitCode = 0;
	ServiceStatus.dwCheckPoint = dwProgress;
	ServiceStatus.dwWaitHint = 3000;

	// send status to SCM
	return SetServiceStatus(g_hService, &ServiceStatus);
}

char* basename(const char* str)
{
	char*p = ((char*)str)+strlen(str)-1;
#ifdef SJIS
	while(p>str && (*p!='\\' || !_ismbstrail((unsigned char *)str, (unsigned char *)p)))
#else
	while(p>str && *p!='\\')
#endif
		p--;
	if(p>str) return (p+1);
	else return p;
}

LPCSTR GetErrorString()
{
	static char ErrBuf[1024];

	FormatMessageA(
    FORMAT_MESSAGE_FROM_SYSTEM |
	FORMAT_MESSAGE_IGNORE_INSERTS,
    NULL,
    GetLastError(),
    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
    (LPSTR) ErrBuf,
    sizeof(ErrBuf),
    NULL );
	return ErrBuf;
};

void ReportError(BOOL bError, LPCSTR szError, ...)
{
	static BOOL bEventSourceAdded = FALSE;
	char buf[512];
	const char *bufp = buf;
	va_list va;

	va_start(va,szError);
	vsprintf(buf,szError,va);
	va_end(va);
	if(g_bTestMode)
	{
		printf("%s%s\n",bError?"Error: ":"",buf);
	}
	else
	{
		if(!bEventSourceAdded)
		{
			TCHAR szModule[MAX_PATH];
			GetModuleFileName(NULL,szModule,MAX_PATH);
			AddEventSource(SERVICE_NAME,szModule);
			bEventSourceAdded=TRUE;
		}

		HANDLE hEvent = RegisterEventSource(NULL,  SERVICE_NAME);
		ReportEventA(hEvent,bError?EVENTLOG_ERROR_TYPE:EVENTLOG_INFORMATION_TYPE,0,MSG_STRING,NULL,1,0,&bufp,NULL);
		DeregisterEventSource(hEvent);
	}
}

void AddEventSource(LPCTSTR szService, LPCTSTR szModule)
{
	HKEY hk;
	DWORD dwData;
	TCHAR szKey[1024];

	_tcscpy(szKey,_T("SYSTEM\\CurrentControlSet\\Services\\EventLog\\Application\\"));
	_tcscat(szKey,szService);

    // Add your source name as a subkey under the Application 
    // key in the EventLog registry key.  
    if (RegCreateKey(HKEY_LOCAL_MACHINE, szKey, &hk))
		return; // Fatal error, no key and no way of reporting the error!!!

    // Add the name to the EventMessageFile subkey.  
    if (RegSetValueEx(hk,             // subkey handle 
            _T("EventMessageFile"),       // value name 
            0,                        // must be zero 
            REG_EXPAND_SZ,            // value type 
            (LPBYTE) szModule,           // pointer to value data 
            _tcslen(szModule) + 1))       // length of value data 
			return; // Couldn't set key

    // Set the supported event types in the TypesSupported subkey.  
    dwData = EVENTLOG_ERROR_TYPE | EVENTLOG_WARNING_TYPE | 
        EVENTLOG_INFORMATION_TYPE;  
    if (RegSetValueEx(hk,      // subkey handle 
            _T("TypesSupported"),  // value name 
            0,                 // must be zero 
            REG_DWORD,         // value type 
            (LPBYTE) &dwData,  // pointer to value data 
            sizeof(DWORD)))    // length of value data 
			return; // Couldn't set key
	RegCloseKey(hk); 
} 

DWORD CALLBACK DoCvsThread(LPVOID lpParam)
{
	HANDLE hConn = (HANDLE)lpParam;
	TCHAR buf[1024];

	STARTUPINFO si= { sizeof(STARTUPINFO) };
	PROCESS_INFORMATION pi = { 0 };

	si.dwFlags = STARTF_USESHOWWINDOW;
	si.wShowWindow = SW_HIDE;

	if(g_bTestMode)
		printf("I/O Socket is %p\n",hConn);

	_sntprintf(buf,sizeof(buf),_T("cvs --win32_socket_io=%ld authserver"),(long)hConn);
	if(!CreateProcess(NULL,buf,NULL,NULL,TRUE,0,NULL,NULL,&si,&pi))
	{
		ReportError(TRUE,"Couldn't start cvs.exe.  Error %d\n",GetLastError());
		return -1;
	}

	if(g_bTestMode)
		printf("%08x: Process %08x started\n",GetTickCount(),pi.hProcess);

	while(!g_bStop && (WaitForSingleObject(pi.hProcess,200)==WAIT_TIMEOUT))
		;
	if(g_bStop)
		TerminateProcess(pi.hProcess,-1);

	if(g_bTestMode)
		printf("%08x: Process %08x terminated\n",GetTickCount(),pi.hProcess);

	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);

	shutdown((SOCKET)hConn,SD_BOTH);
	closesocket((SOCKET)hConn);

	return 0;
}
