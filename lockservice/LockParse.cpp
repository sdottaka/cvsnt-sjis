#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define STRICT
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <sys/socket.h>
#include <netdb.h>
#include <errno.h>
#define SOCKET int
#endif

#include <stdio.h>
#include <stdarg.h>
#include <vector>
#include <string>
#include <map>

#ifdef SJIS
#include <mbstring.h>
#endif

#include "LockService.h"

#ifdef _WIN32
#define socket_errno WSAGetLastError()
#define vsnprintf _vsnprintf
#define strcasecmp stricmp
#else
#define closesocket close
#define socket_errno errno
#endif

#ifdef _WIN32
#define isslash(c) ((c=='/') || (c=='\\'))
#define path_equal(a,b) (tolower(a)==tolower(b))
#else
#define isslash(c) (c=='/')
#define path_equal(a,b) (a==b)
#endif

static bool DoClient(SOCKET s, char *param);
static bool DoLock(SOCKET s, char *param);
static bool DoUnlock(SOCKET s, char *param);
static bool DoMonitor(SOCKET s, char *param);
static bool DoClients(SOCKET s, char *param);
static bool DoLocks(SOCKET s, char *param);
static bool request_lock(SOCKET s, const char *path, unsigned flags, int& lock_to_wait_for);

extern bool g_bTestMode;
#define DEBUG if(g_bTestMode) printf

const char *StateString[] = { "Logging in", "Active", "Monitoring", "Closed" };
enum ClientState { lcLogin, lcActive, lcMonitor, lcClosed };
enum LockFlags { lfRead = 0x00, lfWrite = 0x01, lfRecursive=0x08, lfFile=0x10, lfDirectory=0x20, lfDeleted=0x8000 };

struct LockClient
{
	std::string server_host;
	std::string client_host;
	std::string user;
	std::string root;
	ClientState state;
};

struct Lock
{
	SOCKET owner;
	std::string path;
	unsigned flags;
};

std::map<SOCKET,LockClient> LockClientMap;
std::vector<Lock> LockList;

// When we're parsing stuff, it's sometimes handy
// to have strchr return the final token as if it
// had an implicit delimiter at the end.
static char *lock_strchr(const char *s, int ch)
{
	if(!*s)
		return NULL;
#ifdef SJIS
	char *p=(char *)_mbschr((unsigned char *)s,ch);
#else
	char *p=strchr(s,ch);
#endif
	if(p)
		return p;
	return (char*)s+strlen(s);
}

// Compare two paths, accounting for oddities in case/slash recognition.
int pathcmp(const char *a, const char *b)
{
	while(*a && *b && (path_equal(*a,*b) || (isslash(*a) && isslash(*b))))
	{
#ifdef SJIS
		a=(char *)_mbsinc((const unsigned char *)a);
		b=(char *)_mbsinc((const unsigned char *)b);
#else
		a++;
		b++;
#endif
	}
	return (*a)-(*b);
}

int pathncmp(const char *a, const char *b, size_t len)
{
	while(len && *a && *b && (path_equal(*a,*b) || (isslash(*a) && isslash(*b))))
	{
#ifdef SJIS
		if(_ismbblead(*a))
		{
			a+=2;
			b+=2;
			len-=2;
		}
		else
		{
			a++;
			b++;
			len--;
		}
#else
		a++;
		b++;
		len--;
#endif
	}

	if(!len)
		return 0;

	return (*a)-(*b);
}

// Output a string to a socket
int sock_printf(SOCKET s, const char *fmt, ...)
{
	char temp[1024];
	va_list va;

	va_start(va,fmt);

	vsnprintf(temp,sizeof(temp),fmt,va);

	va_end(va);

	return send(s,temp,strlen(temp),0);
}

const char *FlagToString(unsigned flag)
{
	static char flg[1024];
	
	flg[0]='\0';
	if(flag&lfWrite)
		strcat(flg,"Write ");
	else if(!(flag&lfDeleted))
		strcat(flg,"Read ");
	if(flag&lfRecursive)
		strcat(flg,"Recursive ");
	if(flag&lfFile)
		strcat(flg,"File ");
	if(flag&lfDirectory)
		strcat(flg,"Directory ");
	if(flag&lfDeleted)
		strcat(flg,"Deleted ");
	if(flg[0])
		flg[strlen(flg)-1]='\0'; // Chop trailing space
	return flg;
}

bool MonitorUpdateClient(SOCKET s)
{
	LockClient& Client=LockClientMap[s];
	std::map<SOCKET,LockClient>::const_iterator i;
	for(i = LockClientMap.begin(); !(i==LockClientMap.end()); i++)
	{
		if((*i).second.state!=lcMonitor || (*i).first==s)
			continue;
		sock_printf((*i).first,"010 MONITOR Client (#%d) %s|%s|%s|%s|%s\n",
			s,
			Client.server_host.c_str(),
			Client.client_host.c_str(),
			Client.user.c_str(),
			Client.root.c_str(),
			StateString[Client.state]);
	}
	return true;
}

bool MonitorUpdateLock(int lock)
{
	Lock& Lock=LockList[lock];
	std::map<SOCKET,LockClient>::const_iterator i;
	for(i = LockClientMap.begin(); !(i==LockClientMap.end()); i++)
	{
		if((*i).second.state!=lcMonitor)
			continue;
		sock_printf((*i).first,"010 MONITOR Lock (#%d) %s|%s\n",
			Lock.owner,
			Lock.path.c_str(),
			FlagToString(Lock.flags));
	}
	return true;
}

bool OpenLockClient(SOCKET s, const struct sockaddr_storage *sin, int sinlen)
{
	char host[NI_MAXHOST];

	if(getnameinfo((sockaddr*)sin,sinlen,host,NI_MAXHOST,NULL,0,0))
	{
		DEBUG("GetNameInfo failed: %s\n",gai_strerror(socket_errno));
		return false;
	}
	DEBUG("Lock Client #%d(%s) opened\n",s,host);

	LockClient l;
	l.server_host=host;
	l.state=lcLogin;
	
	LockClientMap[s]=l;

	MonitorUpdateClient(s);

	sock_printf(s,"CVSLock 1.0 Ready\n");
	return true;
}

bool CloseLockClient(SOCKET s)
{
	for(size_t n=0; n<LockList.size();)
	{
		if(LockList[n].owner==s)
		{
			DEBUG("(#%d) Destroying lock on %s\n",s,LockList[n].path.c_str());
			LockList[n].flags=lfDeleted;
			MonitorUpdateLock(n);
			LockList.erase(LockList.begin()+n);
		}
		else
			n++;
	}
	LockClientMap[s].state=lcClosed;
	MonitorUpdateClient(s);
	LockClientMap.erase(LockClientMap.find(s));

	DEBUG("Lock Client #%d closed\n",s);
	return true;
}

// Lock commands:
//
// Client <user>'|'<root>['|'<client_host>]
// Lock [Read] [Write] [Recursive] [File] [Directory]'|'<path>
// Unlock <path>
// Monitor
// Clients
// Locks
//
// Errors:
// 000 OK {message}
// 001 FAIL {message}
// 002 WAIT {message}
// 010 MONITOR {message}

bool ParseLockCommand(SOCKET s, const char *command)
{
	char *cmd = strdup(command),*p;
	char *param;
	bool bRet;

	if(!*command)
		return true; // Empty line

	p=lock_strchr(cmd,' ');
	if(!p)
	{
		sock_printf(s,"001 FAIL Syntax error\n");
		bRet=false;
	}
	else
	{
		if(!*p)
			param = p;
		else
			param = p+1;
		*p='\0';
		if(!strcasecmp(cmd,"Client"))
			bRet=DoClient(s,param);
		else if(!strcasecmp(cmd,"Lock"))
			bRet=DoLock(s,param);
		else if(!strcasecmp(cmd,"Unlock"))
			bRet=DoUnlock(s,param);
		else if(!strcasecmp(cmd,"Monitor"))
			bRet=DoMonitor(s,param);
		else if(!strcasecmp(cmd,"Clients"))
			bRet=DoClients(s,param);
		else if(!strcasecmp(cmd,"Locks"))
			bRet=DoLocks(s,param);
		else
		{
			sock_printf(s,"001 FAIL Unknown command '%s'\n",cmd);
			bRet=false;
		}
	}
	free(cmd);
	return bRet;
}

bool DoClient(SOCKET s, char *param)
{
	char *user, *root, *host;
	if(LockClientMap[s].state!=lcLogin)
	{
		sock_printf(s,"001 FAIL Unexpected 'Client' command\n");
		return false;
	}
#ifdef SJIS
	root = (char *)_mbschr((unsigned char *)param,'|');
#else
	root = strchr(param,'|');
#endif
	if(!root)
	{
		sock_printf(s,"001 FAIL Client command expects <user>|<root>[|<client host>]\n");
		return false;
	}
	*(root++)='\0';

#ifdef SJIS
	host = (char *)_mbschr((unsigned char *)root,'|');
#else
	host = strchr(root,'|');
#endif
	if(host)
	{
#ifdef SJIS
		if(_mbschr((unsigned char *)host+1,'|'))
#else
		if(strchr(host+1,'|'))
#endif
		{
			sock_printf(s,"001 FAIL Client command expects <user>|<root>[|<client host>]\n");
			return false;
		}
		if(host)
			*(host++)='\0';
	}

	user = param;

	LockClientMap[s].user=user;
	LockClientMap[s].root=root;
	LockClientMap[s].client_host=host?host:"";
	LockClientMap[s].state=lcActive;

	MonitorUpdateClient(s);

	DEBUG("(#%d) New client %s(%s) root %s\n",s,user,host?host:"unknown",root);
	sock_printf(s,"000 OK Client registered\n");
	return true;
}

bool DoLock(SOCKET s, char *param)
{
	char *flags, *path,*p;
	unsigned uFlags=0;
	int lock_to_wait_for;

	if(LockClientMap[s].state!=lcActive)
	{
		sock_printf(s,"001 FAIL Unexpected 'Lock' command\n");
		return false;
	}
#ifdef SJIS
	path = (char *)_mbschr((unsigned char *)param,'|');
	if(!path || _mbschr((unsigned char *)path+1,'|'))
#else
	path = strchr(param,'|');
	if(!path || strchr(path+1,'|'))
#endif
	{
		sock_printf(s,"001 FAIL Lock command expects <flags>|<path>\n");
		return false;
	}
	*(path++)='\0';
	flags = param;
	while((p=lock_strchr(flags,' '))!=NULL)
	{
		char c=*p;
		*p='\0';
		if(!strcasecmp(flags,"Read"))
			uFlags|=lfRead;
		else if(!strcasecmp(flags,"Write"))
			uFlags|=lfWrite;
		else if(!strcasecmp(flags,"Recursive"))
			uFlags|=lfRecursive;
		else if(!strcasecmp(flags,"File"))
			uFlags|=lfFile;
		else if(!strcasecmp(flags,"Directory"))
			uFlags|=lfDirectory;
		else
		{
			sock_printf(s,"001 FAIL Unknown flag '%s'\n",flags);
			return false;
		}
		if(c) flags=p+1;
		else break;
	}
	if((uFlags&lfRecursive) && (uFlags&lfFile))
	{
		sock_printf(s,"001 FAIL Recursive file locks don't make sense\n");
		return false;
	}
	if((uFlags&lfFile) && (uFlags&lfDirectory))
	{
		sock_printf(s,"001 FAIL Can't be both a file and a directory\n");
		return false;
	}
	if(!(uFlags&lfFile) && !(uFlags&lfDirectory))
	{
		sock_printf(s,"001 FAIL Must specify file or directory\n");
		return false;
	}

	if(pathncmp(path,LockClientMap[s].root.c_str(),LockClientMap[s].root.size()))
	{
		sock_printf(s,"001 FAIL Lock not within repository\n");
		return false;
	}

	DEBUG("(#%d) Lock request on %s (%s)\n",s,path,FlagToString(uFlags));

	if(request_lock(s,path,uFlags,lock_to_wait_for))
		sock_printf(s,"000 OK Lock granted\n");
	else
		sock_printf(s,"002 WAIT Lock busy|%s|%s|%s\n",LockClientMap[LockList[lock_to_wait_for].owner].user.c_str(),LockClientMap[LockList[lock_to_wait_for].owner].client_host.c_str(),LockList[lock_to_wait_for].path.c_str());
	
	return true;
}

bool DoUnlock(SOCKET s, char *param)
{
	char *path;
	if(LockClientMap[s].state!=lcActive)
	{
		sock_printf(s,"001 FAIL Unexpected 'Unlock' command\n");
		return false;
	}
	if(!*param)
	{
		sock_printf(s,"001 FAIL 'Unlock' needs <path> or 'All'\n");
	}
	if(!strcasecmp(param,"All"))
		path = NULL;
	else
		path = param;

	size_t n;
	bool bUnlocked = false;
	for(n=0; n<LockList.size(); )
	{
		if(LockList[n].owner==s && (!path || !pathcmp(path,LockList[n].path.c_str())))
		{
			DEBUG("(#%d) Unlocking %s\n",s,LockList[n].path.c_str());
			LockList[n].flags=lfDeleted;
			MonitorUpdateLock(n);
			LockList.erase(LockList.begin()+n);
			bUnlocked = true;
		}
		else
			n++;
	}
	if(!bUnlocked && path)
	{
		sock_printf(s,"001 FAIL Unknown lock %s\n",path);
		return false;
	}
	else
	{
		sock_printf(s,"000 OK Unlocked\n");
		return true;
	}
}

bool DoMonitor(SOCKET s, char *param)
{
	if(LockClientMap[s].state!=lcLogin)
	{
		sock_printf(s,"001 FAIL Unexpected 'Monitor' command\n");
		return false;
	}
	LockClientMap[s].state=lcMonitor;
	MonitorUpdateClient(s);
	sock_printf(s,"000 OK Entering monitor mode\n");
	return true;
}

bool DoClients(SOCKET s, char *param)
{
	if(LockClientMap[s].state!=lcMonitor)
	{
		sock_printf(s,"001 FAIL Unexpected 'Clients' command\n");
		return false;
	}
	std::map<SOCKET,LockClient>::const_iterator i;
	for(i = LockClientMap.begin(); !(i==LockClientMap.end()); i++)
	{
		sock_printf(s,"(#%d) %s|%s|%s|%s|%s\n",
			(*i).first,
			(*i).second.server_host.c_str(),
			(*i).second.client_host.c_str(),
			(*i).second.user.c_str(),
			(*i).second.root.c_str(),
			StateString[(*i).second.state]);
	}
	sock_printf(s,"000 OK\n");
	return true;
}

bool DoLocks(SOCKET s, char *param)
{
	if(LockClientMap[s].state!=lcMonitor)
	{
		sock_printf(s,"001 FAIL Unexpected 'Locks' command\n");
		return false;
	}
	for(size_t n=0; n<LockList.size(); n++)
	{
		sock_printf(s,"(#%d) %s|%s",LockList[n].owner,LockList[n].path.c_str(),FlagToString(LockList[n].flags));
	}
	sock_printf(s,"000 OK\n");
	return true;
}

bool request_lock(SOCKET s, const char *path, unsigned flags, int& lock_to_wait_for)
{
	size_t n;
	for(n=0; n<LockList.size(); n++)
	{
		if(((LockList[n].flags&lfRecursive) && !pathncmp(LockList[n].path.c_str(),path,LockList[n].path.size())) ||
			((flags&lfRecursive) && !pathncmp(path,LockList[n].path.c_str(),strlen(path))) ||
			(!pathcmp(path,LockList[n].path.c_str())))
		{
			/* Special case - same owner can put multiple locks on the same tree */
			if(LockList[n].owner==s)
				continue;
			// If either is a write lock, we break, otherwise we allow multiple
			// read locks on the same files
			if((LockList[n].flags&lfWrite) || (flags&lfWrite))
				break;
		}
	}
	if(n<LockList.size())
	{
		lock_to_wait_for = n;
		return false;
	}
	LockList.resize(n+1);
	LockList[n].flags=flags;
	LockList[n].path=path;
	LockList[n].owner=s;
	MonitorUpdateLock(n);
	return true;
}
