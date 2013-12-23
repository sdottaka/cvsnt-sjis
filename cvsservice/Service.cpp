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

namespace 
{
const char ntservice_version_string[] = 
    "CVSNT Service " CVSNT_PRODUCTVERSION_STRING "\n";
}

static void CALLBACK ServiceMain(DWORD dwArgc, LPTSTR *lpszArgv);
static void CALLBACK ServiceHandler(DWORD fdwControl);
static BOOL NotifySCM(DWORD dwState, DWORD dwWin32ExitCode, DWORD dwProgress);
static char* basename(const char* str);
static LPCSTR GetErrorString();
static void AddEventSource(LPCTSTR szService, LPCTSTR szModule);
static void ReportError(BOOL bError, LPCSTR szError, ...);
static DWORD CALLBACK DoCvsThread(LPVOID lpParam);
static DWORD CALLBACK DoPipeThread(LPVOID lpParam);

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
	if(argc<2 || (strcmp(argv[1],"-i") && strcmp(argv[1],"-u") && strcmp(argv[1],"-test") && strcmp(argv[1],"-v") ))
	{
		fprintf(stderr, "CVSNT Service Handler\n\n"
                        "Arguments:\n"
                        "\t%s -i [cvsroot]\tInstall\n"
                        "\t%s -u\tUninstall\n"
                        "\t%s -test\tInteractive run\n"
                        "\t%s -v\tReport version number\n",
                        basename(argv[0]),basename(argv[0]),
                        basename(argv[0]), basename(argv[0]) 
                        );
		return -1;
	}

	if(RegCreateKeyEx(HKEY_LOCAL_MACHINE,_T("Software\\CVS\\Pserver"),NULL,_T(""),REG_OPTION_NON_VOLATILE,KEY_ALL_ACCESS,NULL,&hk,NULL))
	{ 
		fprintf(stderr,"Couldn't create HKLM\\Software\\CVS\\Pserver key, error %d\n",GetLastError());
		return -1;
	}

    if (!strcmp(argv[1],"-v")) {
        puts(ntservice_version_string);
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
						STANDARD_RIGHTS_REQUIRED, SERVICE_WIN32_OWN_PROCESS|SERVICE_INTERACTIVE_PROCESS,
						SERVICE_AUTO_START, SERVICE_ERROR_NORMAL,
						szImagePath, NULL, NULL, NULL, NULL, NULL)) == NULL)
		{
			fprintf(stderr,"CreateService Failed: %s\n",GetErrorString());
			return -1;
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
			NotifySCM(SERVICE_STOPPED,0,1);
		return;
	}

	dwTmp=sizeof(szTmp);
	if(RegQueryValueEx(hk,_T("PATH"),NULL,&dwType,(BYTE*)szTmp,&dwTmp))
	{
		ReportError(TRUE,"Unable to start "SERVICE_NAMEA" - PATH environment variable not defined in system environment");
		if(!g_bTestMode)
			NotifySCM(SERVICE_STOPPED,0,1);
		return;
	}
	ExpandEnvironmentStrings(szTmp,szTmp2,sizeof(szTmp));
	SetEnvironmentVariable(_T("PATH"),szTmp2);

	RegCloseKey(hk);

	if(RegOpenKeyEx(HKEY_LOCAL_MACHINE,_T("Software\\CVS\\Pserver"),NULL,KEY_QUERY_VALUE,&hk))
	{
		ReportError(TRUE,"Unable to start "SERVICE_NAMEA" - Couldn't open HKLM\\Software\\CVS\\Pserver key");
		if(!g_bTestMode)
			NotifySCM(SERVICE_STOPPED,0,1);
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

	bool NTServer = true;
	dwTmp=sizeof(DWORD);
	if(!RegQueryValueEx(hk,_T("StartNTServer"),NULL,&dwType,(BYTE*)szTmp,&dwTmp))
	{
		if(!*(DWORD*)szTmp)
			NTServer=false;
	}
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
			NotifySCM(SERVICE_STOPPED,0,1);
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
					NotifySCM(SERVICE_STOPPED,0,1);
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
			NotifySCM(SERVICE_STOPPED,0,1);
		return;
	}

	// Startup the named pipe listener for ntserver mode
	if(NTServer)
	{
		if(g_bTestMode)
			printf("Starting named pipe server...\n");
		CloseHandle(CreateThread(NULL,0,DoPipeThread,NULL,0,NULL));
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
		NotifySCM(SERVICE_STOP_PENDING, 0, 1);
		g_bStop=TRUE;
		break;
    case SERVICE_CONTROL_INTERROGATE:
		OutputDebugString(SERVICE_NAME _T(": Interrogate\n"));
		NotifySCM(g_dwCurrentState, 0, 0);
		break;
   }
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
	while(p>str && *p!='\\')
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

	STARTUPINFO si= { sizeof(STARTUPINFO) };
	PROCESS_INFORMATION pi = { 0 };
	HANDLE hReadPipeClient,hWritePipeClient,hErrorPipeClient;

	DuplicateHandle(GetCurrentProcess(),hConn,GetCurrentProcess(),&hErrorPipeClient,0,TRUE,DUPLICATE_SAME_ACCESS);
	DuplicateHandle(GetCurrentProcess(),hConn,GetCurrentProcess(),&hReadPipeClient,0,TRUE,DUPLICATE_SAME_ACCESS);
	DuplicateHandle(GetCurrentProcess(),hConn,GetCurrentProcess(),&hWritePipeClient,0,TRUE,DUPLICATE_SAME_ACCESS);

	si.dwFlags=STARTF_USESTDHANDLES|STARTF_USESHOWWINDOW;
	si.hStdInput=hReadPipeClient;
	si.hStdOutput=hWritePipeClient;
	si.hStdError=hErrorPipeClient;
	si.wShowWindow=SW_HIDE;

	if(!CreateProcess(NULL,_T("cvs authserver"),NULL,NULL,TRUE,0,NULL,NULL,&si,&pi))
	{
		ReportError(TRUE,"Couldn't start cvs.exe.  Error %d\n",GetLastError());
		return -1;
	}

	if(g_bTestMode)
		printf("%08x: Process %08x started\n",GetTickCount(),pi.hProcess);

	CloseHandle(hReadPipeClient);
	CloseHandle(hWritePipeClient);
	CloseHandle(hErrorPipeClient);

	while(!g_bStop && (WaitForSingleObject(pi.hProcess,200)==WAIT_TIMEOUT))
		;
	if(g_bStop)
		TerminateProcess(pi.hProcess,-1);

	if(g_bTestMode)
		printf("%08x: Process %08x terminated\n",GetTickCount(),pi.hProcess);

	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);

	DWORD flags,ob,ib,mi;
	if(!GetNamedPipeInfo(hConn,&flags,&ob,&ib,&mi))
	{
		shutdown((SOCKET)hConn,SD_BOTH);
		closesocket((SOCKET)hConn);
	}
	else
	{
		FlushFileBuffers(hConn);
		CloseHandle(hConn);
	}

	return 0;
}

DWORD CALLBACK DoPipeThread(LPVOID lpParam)
{
	SECURITY_ATTRIBUTES sa;
	PSECURITY_DESCRIPTOR pSD;
 
    // create a security descriptor that allows anyone to write to 
    //  the pipe...
	// 
    pSD = (PSECURITY_DESCRIPTOR) malloc( SECURITY_DESCRIPTOR_MIN_LENGTH );  
    if (pSD == NULL)
		return FALSE;
    if (!InitializeSecurityDescriptor(pSD, SECURITY_DESCRIPTOR_REVISION)) 
		return FALSE;
	// add a NULL disc. ACL to the security descriptor. 
    //
	if (!SetSecurityDescriptorDacl(pSD, TRUE, (PACL) NULL, FALSE)) 
		return FALSE;

	sa.nLength = sizeof(sa);
    sa.lpSecurityDescriptor = pSD;
	sa.bInheritHandle = TRUE;

	HANDLE hPipe = CreateNamedPipe(_T("\\\\.\\pipe\\CVS_PIPE"),PIPE_ACCESS_DUPLEX,PIPE_TYPE_BYTE|PIPE_READMODE_BYTE|PIPE_WAIT,PIPE_UNLIMITED_INSTANCES,0,0,5000,&sa);
	if(hPipe==INVALID_HANDLE_VALUE)
	{
		ReportError(TRUE,"Couldn't create named pipe - error %d\n",GetLastError());
		return -1;
	}

	while(!g_bStop)
	{
		BOOL bRes = ConnectNamedPipe(hPipe,NULL);
		DWORD dwErr = 0;

		// Change submitted by Egor Shokurov on 8/31/2001
		//
		// We don't need error code if function succeed
		if (!bRes)
			dwErr = GetLastError();
		if(bRes || dwErr==ERROR_PIPE_CONNECTED)
		{
			// Here we store the handle and create new one before forking cvs.exe
			// to prevent two concurrent connections to the same pipe.

			HANDLE hOldPipe = hPipe;
			hPipe = CreateNamedPipe(_T("\\\\.\\pipe\\CVS_PIPE"),PIPE_ACCESS_DUPLEX,PIPE_TYPE_BYTE|PIPE_READMODE_BYTE|PIPE_WAIT,PIPE_UNLIMITED_INSTANCES,0,0,5000,&sa);
			if(hPipe==INVALID_HANDLE_VALUE)
			{
				ReportError(TRUE, "Couldn't create named pipe - error %d\n",GetLastError());
				return -1;
			}
	
			CloseHandle(CreateThread(NULL,0,DoCvsThread,(void*)hOldPipe,0,NULL));
		}
		// Here we check for disconnected pipe
		// This code is not needed in normal function just paranoid check
		// 
		else if (dwErr == ERROR_NO_DATA)
		{
			DisconnectNamedPipe(hPipe);
			CloseHandle(hPipe);
			hPipe = CreateNamedPipe(_T("\\\\.\\pipe\\CVS_PIPE"),PIPE_ACCESS_DUPLEX,PIPE_TYPE_BYTE|PIPE_READMODE_BYTE|PIPE_WAIT,PIPE_UNLIMITED_INSTANCES,0,0,5000,&sa);
			if(hPipe==INVALID_HANDLE_VALUE)
			{
				ReportError(TRUE,"Couldn't create named pipe - error %d\n",GetLastError());
				return -1;
			}
			continue;
		}
		else
		{
			ReportError(TRUE,"ConnectNamedPipe returned error %d\n",dwErr);
		}
		Sleep(1000);
	}
	return 0;
}

