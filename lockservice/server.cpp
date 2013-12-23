#include "config.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define STRICT
#define FD_SETSIZE 1024
#include <windows.h>
#include <stdio.h>
#include <tchar.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#define SOCKET_ERRNO WSAGetLastError()
#else
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>
#include <unistd.h>
#define SOCKET int
#define SOCKET_ERRNO errno
#define SOCKET_ERROR -1
#define closesocket close
#endif

#include <vector>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

#include "LockService.h"

enum SocketType { ListenSocket, LockSocket };

struct SocketState
{
	SocketState(SOCKET _s, SocketType _t = LockSocket) { s=_s; type=_t; line_len=0; }
	SOCKET s;
	SocketType type;
	char line_buf[1024];
	int line_len;
};

static std::vector<SocketState> g_Sockets;

bool g_bStop = false;
extern bool g_bTestMode;

#ifndef _WIN32
#ifndef FALSE
enum { FALSE, TRUE };
#endif
void ReportError(int error, char *fmt,...)
{
  char buf[512];
  va_list va;

  va_start(va,fmt);
  vsnprintf(buf,sizeof(buf),fmt,va);
  fprintf(stderr,"%s%s\n",error?"Error: ":"",buf);
}
#else
void ReportError(BOOL bError, LPCSTR szError, ...);
BOOL NotifySCM(DWORD dwState, DWORD dwWin32ExitCode, DWORD dwProgress);
#endif

void run_server(int port, int seq, int local_only)
{
	int err;
    addrinfo *pAddrInfo;
    char szLockServer[64];

	sprintf(szLockServer,"%d",port);

	addrinfo hint = {0};
	hint.ai_family=PF_UNSPEC;
	hint.ai_socktype=SOCK_STREAM;
	hint.ai_protocol=IPPROTO_TCP;
	hint.ai_flags=local_only?0:AI_PASSIVE;
	pAddrInfo=NULL;
	if(g_bTestMode)
	{
		printf("Initialising socket...");
	}
	err=getaddrinfo(NULL,szLockServer,&hint,&pAddrInfo);
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

#ifdef _WIN32
	if(!g_bTestMode)
		NotifySCM(SERVICE_START_PENDING, 0, seq++);
#endif

	if(g_bTestMode)
			printf("Starting lock server on port %d/tcp...\n",port);


	addrinfo* ai;
	for(ai=pAddrInfo;ai;ai=ai->ai_next)
	{
#ifdef _WIN32
		SOCKET s = WSASocket(ai->ai_family,ai->ai_socktype,ai->ai_protocol,NULL,0,0);
#else
		SOCKET s = socket(ai->ai_family,ai->ai_socktype,ai->ai_protocol);
#endif

		if(s!=-1 && !bind(s,ai->ai_addr,ai->ai_addrlen))
		{
			if(listen(s,50)==SOCKET_ERROR)
			{
				ReportError(TRUE,"Listen on socket failed: %s\n",gai_strerror(SOCKET_ERRNO));
#ifdef _WIN32
				if(!g_bTestMode)
					NotifySCM(SERVICE_STOPPED,0,1);
#endif
				freeaddrinfo(pAddrInfo);
				return;
			}
			SocketState st(s, ListenSocket);
			g_Sockets.push_back(st);
		}
		else
		{
//			if(g_bTestMode)
//				printf("Socket Failed (Handle=%08x Family=%d,Socktype=%d,Protocol=%d): %s (not fatal)\n",s,ai->ai_family,ai->ai_socktype,ai->ai_protocol, gai_strerror(SOCKET_ERRNO));
			closesocket(s);
		}
	}
	freeaddrinfo(pAddrInfo);

	if(!g_Sockets.size())
	{
		ReportError(TRUE,"All socket binds failed.");
#ifdef _WIN32
		if(!g_bTestMode)
			NotifySCM(SERVICE_STOPPED,0,1);
#endif
		return;
	}

	// Process running, wait for closedown
	ReportError(FALSE,"CVS Lock service initialised successfully");
#ifdef _WIN32
	if(!g_bTestMode)
		NotifySCM(SERVICE_RUNNING, 0, 0);
#endif

	g_bStop=FALSE;

	do
	{
		fd_set rfd;
		sockaddr_storage sin;
		size_t n;
#ifndef __hpux__ // hpux has a broken accept() definition
		socklen_t addrlen;
#else
		int addrlen;
#endif
 		int maxdesc = -1;

		FD_ZERO(&rfd);
		for(n=0; n<g_Sockets.size(); n++)
		{
			FD_SET(g_Sockets[n].s,&rfd);
			if(((int)g_Sockets[n].s)>maxdesc)
			  maxdesc=g_Sockets[n].s;
		}
		struct timeval tv = { 5, 0 }; // 5 seconds max wait
		int sel=select(maxdesc+1,&rfd,NULL,NULL,&tv);
		if(g_bStop || sel==SOCKET_ERROR) break; // Error on socket, or stopped
		size_t size = g_Sockets.size();
		for(n=0; n<size; n++)
		{
			if(FD_ISSET(g_Sockets[n].s,&rfd))
			{
				if(g_Sockets[n].type==ListenSocket && g_Sockets.size()<FD_SETSIZE) // Socket listening on port 2401
				{
					addrlen=sizeof(sockaddr_storage);
					SocketState st(accept(g_Sockets[n].s,(struct sockaddr*)&sin,&addrlen),LockSocket);
					g_Sockets.push_back(st);
					OpenLockClient(st.s,&sin,addrlen);
					continue;
				} else if(g_Sockets[n].type==LockSocket)
				{
					char buf[sizeof(g_Sockets[n].line_buf)];
					int len;

//					if(g_bTestMode)
//						printf("Socket #%d Sent data\n",g_Sockets[n].s);
					len=recv(g_Sockets[n].s,buf,sizeof(buf)-1,0);
					if(len<=0)
					{
						CloseLockClient(g_Sockets[n].s);
						closesocket(g_Sockets[n].s);
						g_Sockets[n].s=SOCKET_ERROR;
					}
					else
					{
						char *p=g_Sockets[n].line_buf+g_Sockets[n].line_len;
						char *q=buf;

						if(!p)
							p=g_Sockets[n].line_buf;
						buf[len]='\0';

						while(*q && p<g_Sockets[n].line_buf+sizeof(g_Sockets[n].line_buf))
						{
							if(*q=='\r')
							{
								q++;
								continue;
							}
							*(p++)=*(q++);
							if(*(p-1)=='\n')
							{
								*(p-1)='\0';
								ParseLockCommand(g_Sockets[n].s,g_Sockets[n].line_buf);
								p=g_Sockets[n].line_buf;
							}
						}
						if(p>=g_Sockets[n].line_buf+sizeof(g_Sockets[n].line_buf))
						{
							if(g_bTestMode)
							{
								printf("Overlong line ignored on Socket #%d\n",g_Sockets[n].s);
							}
							p=g_Sockets[n].line_buf;
						}
						g_Sockets[n].line_len=p-g_Sockets[n].line_buf;
					}
				}
			}
		}
		// Cleanup any closed sockets
		for(n=g_Sockets.size()-1; n>0; --n)
			if(g_Sockets[n].s==SOCKET_ERROR)
				g_Sockets.erase(g_Sockets.begin()+n);
	} while(!g_bStop);

#ifdef _WIN32
	NotifySCM(SERVICE_STOPPED, 0, 0);
#endif
	ReportError(FALSE,"CVS Lock service stopped successfully");
}
