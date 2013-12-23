/* CVS auth common routines

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.  */

#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <stdarg.h>
#ifdef _WIN32
#include <io.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <lmcons.h>
#else
#include <pwd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>
#include <fcntl.h>
#endif
#include <errno.h>
#include <ctype.h>
#include <stdlib.h>

#include "protocol_interface.h"
#include "common.h"

#ifdef _WIN32
#define socket_errno WSAGetLastError()
#else
#define closesocket close
#define socket_errno errno
#endif

enum PIPES { PIPE_READ = 0, PIPE_WRITE = 1 };

const struct server_interface *current_server;
static int tcp_fd;
static struct addrinfo *tcp_addrinfo, *tcp_active_addrinfo;

int get_tcp_fd()
{
	return tcp_fd;
}

int get_user_local_config_data(const char *key, const char *value, char *buffer, int buffer_len)
{
	return current_server->get_config_data(current_server,key,value,buffer,buffer_len);
}

int set_user_local_config_data(const char *key, const char *value, const char *buffer)
{
	return current_server->set_config_data(current_server,key,value,buffer);
}

int set_encrypted_channel(int encrypt)
{
	return current_server->set_encrypted_channel(encrypt);
}

int server_error(int fatal, char *fmt, ...)
{
	char temp[1024];
	va_list va;

	va_start(va,fmt);

	vsnprintf(temp,sizeof(temp),fmt,va);

	va_end(va);

	return current_server->error(current_server, fatal, temp);
}

static int tcp_connect_direct(const cvsroot_t *cvsroot)
{
	const char *port;
	int sock;

	port = get_default_port(cvsroot);
	sock = tcp_connect_bind(cvsroot->hostname, port, 0, 0);

	if(sock<0)
		return sock;

	return 0;
}

static int tcp_connect_http(const cvsroot_t *cvsroot)
{
	int sock;
	const char *port;
	char line[1024],*p;

	port = cvsroot->proxyport;
	if(!port)
		port="3128"; // Some people like 8080...  No real standard here

	if(!cvsroot->proxy)
		server_error(1,"Proxy name must be specified for HTTP tunnelling");
	
	sock = tcp_connect_bind(cvsroot->proxy, port, 0, 0);
	if(sock<0)
		return sock;

	port = get_default_port(cvsroot);

	tcp_printf("CONNECT %s:%s HTTP/1.0\n\n",cvsroot->hostname,port);
	tcp_readline(line, sizeof(line));
	
	p=strchr(line,' ');
	if(!p || (atoi(p+1)/100)!=2)
		server_error(1,"Proxy server %s does not support HTTP tunnelling",cvsroot->proxy);

	/* Eat the remaining output */
	do
	{
		tcp_readline(line,sizeof(line));
	} while(strlen(line)>1);

	return 0;
}

int tcp_connect(const cvsroot_t *cvsroot)
{
	char *protocol = cvsroot->proxyprotocol;
	if(!protocol && cvsroot->proxy)
		protocol = "HTTP"; // Compatibility case, where proxy is specified but no tunnelling protocol

	if(!protocol)
		return tcp_connect_direct(cvsroot);
	if(!strcasecmp(protocol,"HTTP"))
		return tcp_connect_http(cvsroot);
	tcp_fd = -1;
	server_error(1, "Unsupported tunnelling protocol '%s' specified",protocol);
	return -1;
}

int tcp_connect_bind(const char *servername, const char *remote_port, int min_local_port, int max_local_port)
{
	struct addrinfo hint = {0}, *localinfo;
	int res,sock,localport,last_error;

	hint.ai_socktype=SOCK_STREAM;
	if((res=getaddrinfo(servername,remote_port,&hint,&tcp_addrinfo))!=0)
	{
		fprintf(stderr, "Error connecting to host %s: %s\n", servername, gai_strerror(socket_errno));
		return -1;
	}

	for(tcp_active_addrinfo = tcp_addrinfo; tcp_active_addrinfo; tcp_active_addrinfo=tcp_active_addrinfo->ai_next)
	{
#ifdef _WIN32
win32_port_reuse_hack:
#endif

		sock = socket(tcp_active_addrinfo->ai_family, tcp_active_addrinfo->ai_socktype, tcp_active_addrinfo->ai_protocol);
		if (sock == -1)
			server_error (1, "cannot create socket: %s", gai_strerror(socket_errno));

		if(min_local_port || max_local_port)
		{
			for(localport=min_local_port; localport<max_local_port; localport++)
			{
				char lport[32];
				snprintf(lport,sizeof(lport),"%d",localport);
				hint.ai_flags=AI_PASSIVE;
				hint.ai_protocol=tcp_active_addrinfo->ai_protocol;
				hint.ai_socktype=tcp_active_addrinfo->ai_socktype;
				hint.ai_family=tcp_active_addrinfo->ai_family;
				localinfo=NULL;
				if((res=getaddrinfo(NULL,lport,&hint,&localinfo))!=0)
				{
					server_error (1,"Error connecting to host %s: %s\n", servername, gai_strerror(socket_errno));
					return -1;
				}
				if(bind(sock, (struct sockaddr *)localinfo->ai_addr, localinfo->ai_addrlen) ==0)
					break;
				freeaddrinfo(localinfo);
			}
			freeaddrinfo(localinfo);
			if(localport==max_local_port)
				server_error (1, "Couldn't bind to local port - %s",gai_strerror(socket_errno));
		}

		if(connect (sock, (struct sockaddr *) tcp_active_addrinfo->ai_addr, tcp_active_addrinfo->ai_addrlen)==0)
			break;
		last_error = socket_errno;
		closesocket(sock);
#ifdef _WIN32
		if(last_error == 10048 && (min_local_port || max_local_port))
			// Win32 bug - should have been caught by 'bind' above, but instead gets caught by 'connect'
		{
			min_local_port = localport+1;
			goto win32_port_reuse_hack;
		}
#endif
	}
	if(!tcp_active_addrinfo)
		server_error (1, "connect to %s:%s failed: %s", servername, remote_port, gai_strerror(last_error));

	tcp_fd = sock;

	return sock;
}
const struct addrinfo *get_addrinfo(int get_canonical_name)
{
	if(get_canonical_name && tcp_addrinfo->ai_canonname==NULL)
	{
		char host[256];
		getnameinfo(tcp_addrinfo->ai_addr,tcp_addrinfo->ai_addrlen,host,sizeof(host),NULL,0,0);
		tcp_addrinfo->ai_canonname=strdup(host);
	}
	return tcp_addrinfo;
}

int tcp_setblock(int block)
{
#ifndef _WIN32
  if(tcp_fd != -1)
  {
    int flags;
    fcntl(tcp_fd, F_GETFL, &flags);
    if(block)
	flags&=~O_NONBLOCK;
    else
	flags!=O_NONBLOCK;
    fcntl(tcp_fd, F_SETFL, flags);
    return 0;
  }
#else
  return -1;
#endif

}

int tcp_disconnect()
{
	if (tcp_fd != -1)
	{
		if(closesocket(tcp_fd))
			return -1;
	    tcp_fd = -1;
		freeaddrinfo(tcp_addrinfo);
	}
	return 0;
}

int tcp_read(void *data, int length)
{
	if(!tcp_fd) /* Using stdin/out, probably */
		return read(0,data,length);
	else
		return recv(tcp_fd,data,length,0);
}

int tcp_write(const void *data, int length)
{
	if(!tcp_fd) /* Using stdin/out, probably */
		return write(1,data,length);
	else
		return send(tcp_fd,data,length,0);
}

int tcp_shutdown()
{
	if(!tcp_fd)
		return 0;
	if(shutdown(tcp_fd,0))
	{
		server_error (1, "shutting down connection to %s: %s", current_server->current_root->hostname, gai_strerror(socket_errno));
		return -1;
	}
	return 0;
}

int tcp_printf(char *fmt, ...)
{
	char temp[1024];
	va_list va;

	va_start(va,fmt);

	vsnprintf(temp,sizeof(temp),fmt,va);

	va_end(va);

	return tcp_write(temp,strlen(temp));
}

int tcp_readline(char* buffer, int buffer_len)
{
	char *p,c;
	int l;

	l=0;
	p=buffer;
	while(l<buffer_len-1 && tcp_read(&c,1)>0)
	{
		if(c=='\012')
			break;
		*(p++)=(char)c;
		l++;
	}
	*p='\0';
	return l;
}

int server_getc(const struct protocol_interface *protocol)
{
	char c;

	if(protocol->server_read_data)
	{
		if(protocol->server_read_data(protocol,&c,1)<1)
			return EOF;
		return c;
	}
	else
		return getc(stdin);
}

int server_getline(const struct protocol_interface *protocol, char** buffer, int buffer_max)
{
	char *p;
	int l,c=0;

	*buffer=malloc(buffer_max);
	if(!*buffer)
		return -1;

	l=0;
	p=*buffer;
	*p='\0';
	while(l<buffer_max-1 && (c=server_getc(protocol))!=EOF)
	{
		if(c=='\012')
			break;
		*(p++)=(char)c;
		l++;
	}
	if(l==0 && c==EOF)
		return -1; /* EOF */
	*p='\0';
	return l;
	
}

#ifdef _WIN32
int run_command(const char *cmd, int* in_fd, int* out_fd, int *err_fd)
{
	TCHAR *c;
	TCHAR szComSpec[_MAX_PATH];
	OSVERSIONINFO osv;
	STARTUPINFO si= { sizeof(STARTUPINFO) };
	SECURITY_ATTRIBUTES sa = { sizeof(SECURITY_ATTRIBUTES), NULL, TRUE };
	PROCESS_INFORMATION pi = { 0 };
	HANDLE hReadPipeClient,hWritePipeClient,hErrorPipeClient;
	HANDLE hReadPipeServer,hWritePipeServer,hErrorPipeServer;
	HANDLE hReadPipeTmp,hWritePipeTmp,hErrorPipeTmp;

	CreatePipe(&hReadPipeClient,&hWritePipeTmp,&sa,0);
	CreatePipe(&hReadPipeTmp,&hWritePipeClient,&sa,0);
	if(err_fd)
		CreatePipe(&hErrorPipeTmp,&hErrorPipeClient,&sa,0);

	DuplicateHandle(GetCurrentProcess(),hReadPipeTmp,GetCurrentProcess(),&hReadPipeServer,0,FALSE,DUPLICATE_SAME_ACCESS);
	DuplicateHandle(GetCurrentProcess(),hWritePipeTmp,GetCurrentProcess(),&hWritePipeServer,0,FALSE,DUPLICATE_SAME_ACCESS);
	if(err_fd)
		DuplicateHandle(GetCurrentProcess(),hErrorPipeTmp,GetCurrentProcess(),&hErrorPipeServer,0,FALSE,DUPLICATE_SAME_ACCESS);

	CloseHandle(hReadPipeTmp);
	CloseHandle(hWritePipeTmp);
	if(err_fd)
		CloseHandle(hErrorPipeTmp);
	else
		DuplicateHandle(GetCurrentProcess(),hWritePipeClient,GetCurrentProcess(),&hErrorPipeClient,0,TRUE,DUPLICATE_SAME_ACCESS);

	si.dwFlags=STARTF_USESTDHANDLES|STARTF_USESHOWWINDOW;
	si.hStdInput=hReadPipeClient;
	si.hStdOutput=hWritePipeClient;
	si.hStdError=hErrorPipeClient;
	si.wShowWindow=SW_HIDE;

	c=malloc(strlen(cmd)+128);
	osv.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	GetVersionEx(&osv);
	if (osv.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS)
		strcpy(c,cmd); // In Win9x we have to execute directly
	else
	{
		if(!GetEnvironmentVariable("COMSPEC",szComSpec,sizeof(szComSpec)))
			strcpy(szComSpec,"cmd.exe"); 
		sprintf(c,"%s /c \"%s\"",szComSpec,cmd);
	}

	if(!CreateProcess(NULL,c,NULL,NULL,TRUE,0,NULL,NULL,&si,&pi))
		return -1;

	free(c);

	CloseHandle(hReadPipeClient);
	CloseHandle(hWritePipeClient);
	CloseHandle(hErrorPipeClient);

	WaitForInputIdle(pi.hProcess,100);

	CloseHandle(pi.hThread);
	CloseHandle(pi.hProcess);

	if(out_fd)
		*out_fd = _open_osfhandle((long)hReadPipeServer, O_RDONLY|O_BINARY);
	else
		CloseHandle(hReadPipeServer);
	if(in_fd)
		*in_fd = _open_osfhandle((long)hWritePipeServer, O_WRONLY|O_BINARY);
	else
		CloseHandle(hWritePipeServer);
	if(err_fd)
		*err_fd = _open_osfhandle((long)hErrorPipeServer, O_RDONLY|O_BINARY);

	return 0;
}
#else
static void tokenise(char *argbuf, char **argv)
{
  char *p,*q;
  int in_quote=0,escape=0;

  for(p=argbuf;*p;p++)
  {
    while(isspace(*p)) p++;
    *(argv++)=p;
    q=p;
    for(;*p;p++)
    {
      *(q++)=*p;
      if(!in_quote)
      {
        if(escape)
        {
          escape=0;
          continue;
        }
        if(*p=='\\')
        {
          escape=1;
          continue;
        }
        if(*p=='\'' || *p=='"')
        {
          in_quote=*p;
          q--;
          continue;
        }
        if(isspace(*p))
        {
          q--;
          break;
        }
      }
      else
      {
        if(*p==in_quote)
        {
          in_quote=0;
          q--;
          continue;
        }
      }
    }
    if(!*p)
      break;
    *q='\0';
  }
  *argv=NULL;
}

int run_command(const char *cmd, int* in_fd, int* out_fd, int* err_fd)
{
  char **argv;    
  char *argbuf;
  int pid;
  int to_child_pipe[2];
  int from_child_pipe[2];
  int err_child_pipe[2];

  argv=(char**)malloc(256*sizeof(char*));
  argbuf=(char*)malloc(strlen(cmd)+128); 
  argv[0]="/bin/sh";
  argv[1]="-c";
  argv[2]=(char*)cmd;
  argv[3]=NULL;

  if (pipe (to_child_pipe) < 0)
      server_error (1, "cannot create pipe");
  if (pipe (from_child_pipe) < 0)
      server_error (1, "cannot create pipe");
  if (pipe (err_child_pipe) <0)
      server_error (1, "cannot create pipe");

#ifdef USE_SETMODE_BINARY
  setmode (to_child_pipe[0], O_BINARY);
  setmode (to_child_pipe[1], O_BINARY);
  setmode (from_child_pipe[0], O_BINARY);
  setmode (from_child_pipe[1], O_BINARY);
  setmode (err_child_pipe[0], O_BINARY);
  setmode (err_child_pipe[1], O_BINARY);
#endif

#ifdef HAVE_VFORK
  pid = vfork ();
#else
  pid = fork ();
#endif
  if (pid < 0)
      server_error (1, "cannot fork");
  if (pid == 0)
  {
      if (close (to_child_pipe[1]) < 0)
          server_error (1, "cannot close pipe");
      if (in_fd && dup2 (to_child_pipe[0], 0) < 0)
          server_error (1, "cannot dup2 pipe");
      if (close (from_child_pipe[0]) < 0)
          server_error (1, "cannot close pipe");
      if (out_fd && dup2 (from_child_pipe[1], 1) < 0)
          server_error (1, "cannot dup2 pipe");
      if (close (err_child_pipe[0]) < 0)
          server_error (1, "cannot close pipe");
      if (err_fd && dup2 (err_child_pipe[1], 2) < 0)
          server_error (1, "cannot dup2 pipe");

      execvp (argv[0], argv);
      server_error (1, "cannot exec %s", cmd);
  }
  if (close (to_child_pipe[0]) < 0)
      server_error (1, "cannot close pipe");
  if (close (from_child_pipe[1]) < 0)
      server_error (1, "cannot close pipe");
  if (close (err_child_pipe[1]) < 0)
      server_error (1, "cannot close pipe");

  if(in_fd)
	*in_fd = to_child_pipe[1];
  else
	  close(to_child_pipe[1]);
  if(out_fd)
	*out_fd = from_child_pipe[0];
  else
	  close(from_child_pipe[0]);
  if(err_fd)
	*err_fd = err_child_pipe[0];
  else
	  close(err_child_pipe[0]);

  free(argv);
  free(argbuf);

  return 0;
}
#endif

#ifdef _WIN32
const char* get_username(const cvsroot_t* current_root)
{
    const char* username;
    static char username_buffer[UNLEN+1];
    DWORD buffer_length=UNLEN+1;
    username=current_root->username;
    if(!username)
    {
		if(GetUserNameA(username_buffer,&buffer_length))
		{
			username=username_buffer;
		}
    }
    return username;
}
#else
const char* get_username(const cvsroot_t* current_root)
{
    const char* username;
    username=current_root->username;
    if(!username)
    {
		username = getpwuid(getuid())->pw_name;
    }
    return username;
}
#endif

const char *get_default_port(const cvsroot_t *root)
{
	struct servent *ent;
	static char p[32];

	if(root->port)
		return root->port;

	if((ent=getservbyname("cvspserver","tcp"))!=NULL)
	{
		sprintf(p,"%u",ntohs(ent->s_port));
		return p;
	}

	return "2401";
}

#if _WIN32
int usleep(unsigned long useconds)
{
	Sleep(useconds);
	return 0;
}
#endif
