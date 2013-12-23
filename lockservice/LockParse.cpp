#include "config.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define STRICT
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <errno.h>
#define SOCKET int
#endif

#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <vector>
#include <string>
#include <map>
#include <algorithm>

#include "LockService.h"

#ifdef _WIN32
#define socket_errno WSAGetLastError()
#define vsnprintf _vsnprintf
#else
#define closesocket close
#define socket_errno errno
#endif

static bool DoClient(SOCKET s,size_t client, char *param);
static bool DoLock(SOCKET s,size_t client, char *param);
static bool DoUnlock(SOCKET s,size_t client, char *param);
static bool DoMonitor(SOCKET s,size_t client, char *param);
static bool DoClients(SOCKET s,size_t client, char *param);
static bool DoLocks(SOCKET s,size_t client, char *param);
static bool DoModified(SOCKET s,size_t client, char *param);
static bool DoVersion(SOCKET s,size_t client, char *param);

static bool request_lock(size_t client, const char *path, unsigned flags, size_t& lock_to_wait_for);

extern bool g_bTestMode;
#define DEBUG if(g_bTestMode) printf

static size_t global_lockId;

const char *StateString[] = { "Logging in", "Active", "Monitoring", "Closed" };
enum ClientState { lcLogin, lcActive, lcMonitor, lcClosed };
enum LockFlags { lfRead = 0x01, lfWrite = 0x02, lfAdvisory = 0x04, lfFull = 0x08 };
enum MonitorFlags { lcMonitorClient=0x01, lcMonitorLock=0x02, lcMonitorVersion=0x04 };

typedef std::map<std::string,std::string> VersionMapType;

struct Lock
{
	size_t owner;
	std::string path;
	unsigned flags;
	size_t length; /* length of path */
	VersionMapType versions;
};

struct TransactionStruct
{
	size_t owner;
	std::string path;
	std::string branch;
	std::string version;
	std::string oldversion;
	char type;
};

typedef std::vector<TransactionStruct> TransactionListType;

struct LockClient
{
	SOCKET sock;
	std::string server_host;
	std::string client_host;
	std::string user;
	std::string root;
	ClientState state;
	unsigned flags;
	time_t starttime;
	time_t endtime;
};

typedef std::map<size_t,LockClient> LockClientMapType;
typedef std::map<size_t,Lock> LockMapType;
LockClientMapType LockClientMap;
LockMapType LockMap;
TransactionListType TransactionList;

std::map<SOCKET,size_t> SockToClient;

size_t next_client_id;

/* predicate for partition below */
struct IsWantedTransaction { bool operator()(const TransactionStruct& t) { return LockClientMap.find(t.owner)!=LockClientMap.end(); } };

bool TimeIntersects(time_t start1, time_t end1, time_t start2, time_t end2)
{
   return (start1<=start2 && (!end1 || end1>=start2)) || (start1>=start2 && (!end2 || end2>=start1));
}

// When we're parsing stuff, it's sometimes handy
// to have strchr return the final token as if it
// had an implicit delimiter at the end.
static char *lock_strchr(const char *s, int ch)
{
	if(!*s)
		return NULL;
	char *p=strchr(s,ch);
	if(p)
		return p;
	return (char*)s+strlen(s);
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
	if(flag&lfRead)
		strcat(flg,"Read ");
	if(flag&lfWrite)
		strcat(flg,"Write ");
	if(flag&lfAdvisory)
		strcat(flg,"Advisory ");
	if(flag&lfFull)
		strcat(flg,"Full ");
	if(flg[0])
		flg[strlen(flg)-1]='\0'; // Chop trailing space
	return flg;
}

bool OpenLockClient(SOCKET s, const struct sockaddr_storage *sin, int sinlen)
{
	char host[NI_MAXHOST];

	if(getnameinfo((sockaddr*)sin,sinlen,host,NI_MAXHOST,NULL,0,0))
	{
		DEBUG("GetNameInfo failed: %s\n",gai_strerror(socket_errno));
		return false;
	}

	LockClient l;
	l.server_host=host;
	l.state=lcLogin;
	l.starttime=time(NULL);
	l.endtime=0;
	l.sock = s;

	size_t client = ++next_client_id;
	SockToClient[s]=client;
	LockClientMap[client]=l;

	DEBUG("Lock Client #%d(%s) opened\n",(int)client,host);

	sock_printf(s,"CVSLock 2.0 Ready\n");

	return true;
}

bool CloseLockClient(SOCKET s)
{
	size_t client = SockToClient[s];
	int count=0;
	for(LockMapType::iterator i = LockMap.begin(); i!=LockMap.end();)
	{
		if(i->second.owner == client) 
		{
			count++;
			LockMap.erase(i++);
		}
		else
			++i;
	}
	DEBUG("Destroyed %d locks\n",count);
	SockToClient.erase(SockToClient.find(s));
	LockClientMap[client].state=lcClosed;
	LockClientMap[client].endtime=time(NULL);
	LockClientMap[client].sock=0;
	if(LockClientMap.size()<=1)
	{
		// No clients, just empty the transaction store
		LockClientMap.clear();
		TransactionList.clear();
		DEBUG("No more clients\n");
	}
	else if(TransactionList.size())
	{
		// Find out which stored clients are redundant
		//
		// The rule is that as long as there's an active client
		// that started before our client finished, you can't
		// delete it.  This random overlapping is what makes
		// atomicity hard :)
		//
		// remove_if doesn't work on maps so we end up with trickery
		// like this...
		LockClientMapType::iterator c,d;
		for (c=LockClientMap.begin(); c!=LockClientMap.end();)
		{
		  if(c->second.endtime)
		  {
		    bool can_delete = true;
		    for (d=LockClientMap.begin(); d!=LockClientMap.end(); ++d)
		    {
			if(c->first != d->first && !d->second.endtime &&
			    TimeIntersects(c->second.starttime,c->second.endtime,d->second.starttime,d->second.endtime))
			  can_delete = false;
		    }
		    if(can_delete)
		    {
			LockClientMap.erase(c++);
		    }
		    else
		      ++c;
		  }
		  else
		    ++c;
		}
		DEBUG("%d clients left\n",(int)LockClientMap.size());
		/* Life is much easier on a vector */
		size_t pos = std::partition(TransactionList.begin(), TransactionList.end(), IsWantedTransaction()) - TransactionList.begin();
		TransactionList.resize(pos);
	}
	else
	{
	  /* No transactions, so just erase this client */
	  DEBUG("No pending transactions\n");
	  LockClientMap.erase(LockClientMap.find(client));
	}

	DEBUG("Lock Client #%d closed\n",(int)client);
	return true;
}

// Lock commands:
//
// Client <user>'|'<root>['|'<client_host>]
// Lock [Read] [Write] [Advisory|Full]'|'<path>['|'<branch>]
// Unlock <LockId>
// Version <LockId>|<Branch>
// Monitor [C][L][V]
// Clients
// Locks
// Modified [Added|Deleted]'|'LockId'|'<branch>'|'<version>['|'<oldversion>]
//
// Errors:
// 000 OK {message}
// 001 FAIL {message}
// 002 WAIT {message}
// 003 WARN (message)
// 010 MONITOR {message}

bool ParseLockCommand(SOCKET s, const char *command)
{
	char *cmd = strdup(command),*p;
	char *param;
	bool bRet;
	size_t client;

	if(!*command)
		return true; // Empty line

	client = SockToClient[s];
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
		if(!strcmp(cmd,"Client"))
			bRet=DoClient(s,client,param);
		else if(!strcmp(cmd,"Lock"))
			bRet=DoLock(s,client,param);
		else if(!strcmp(cmd,"Unlock"))
			bRet=DoUnlock(s,client,param);
		else if(!strcmp(cmd,"Version"))
			bRet=DoVersion(s,client,param);
		else if(!strcmp(cmd,"Modified"))
			bRet=DoModified(s,client,param);
		else if(!strcmp(cmd,"Monitor"))
			bRet=DoMonitor(s,client,param);
		else if(!strcmp(cmd,"Clients"))
			bRet=DoClients(s,client,param);
		else if(!strcmp(cmd,"Locks"))
			bRet=DoLocks(s,client,param);
		else
		{
			sock_printf(s,"001 FAIL Unknown command '%s'\n",cmd);
			bRet=false;
		}
	}
	free(cmd);
	return bRet;
}

bool DoClient(SOCKET s,size_t client, char *param)
{
	char *user, *root, *host;
	if(LockClientMap[client].state!=lcLogin)
	{
		sock_printf(s,"001 FAIL Unexpected 'Client' command\n");
		return false;
	}
	root = strchr(param,'|');
	if(!root)
	{
		sock_printf(s,"001 FAIL Client command expects <user>|<root>[|<client host>]\n");
		return false;
	}
	*(root++)='\0';

	host = strchr(root,'|');
	if(host)
	{
		if(strchr(host+1,'|'))
		{
			sock_printf(s,"001 FAIL Client command expects <user>|<root>[|<client host>]\n");
			return false;
		}
		if(host)
			*(host++)='\0';
	}

	user = param;

	LockClientMap[client].user=user;
	LockClientMap[client].root=root;
	LockClientMap[client].client_host=host?host:"";
	LockClientMap[client].state=lcActive;

	DEBUG("(#%d) New client %s(%s) root %s\n",(int)client,user,host?host:"unknown",root);
	sock_printf(s,"000 OK Client registered\n");
	return true;
}

bool DoLock(SOCKET s,size_t client, char *param)
{
	char *flags, *path,*p;
	unsigned uFlags=0;
	size_t lock_to_wait_for;

	if(LockClientMap[client].state!=lcActive)
	{
		sock_printf(s,"001 FAIL Unexpected 'Lock' command\n");
		return false;
	}
	path = strchr(param,'|');
	if(!path)
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
		if(!strcmp(flags,"Read"))
			uFlags|=lfRead;
		else if(!strcmp(flags,"Write"))
			uFlags|=lfWrite;
		else if(!strcmp(flags,"Advisory"))
			uFlags|=lfAdvisory;
		else if(!strcmp(flags,"Full"))
			uFlags|=lfFull;
		else
		{
			sock_printf(s,"001 FAIL Unknown flag '%s'\n",flags);
			return false;
		}
		if(c) flags=p+1;
		else break;
	}
	if(!(uFlags&lfRead) && !(uFlags&lfWrite))
	{
		sock_printf(s,"001 FAIL Must specify read or write\n");
		return false;
	}

	if(strncmp(path,LockClientMap[client].root.c_str(),LockClientMap[client].root.size()))
	{
		DEBUG("(#%d) Lock Fail %s not within %s\n",(int)client,path,LockClientMap[client].root.c_str());
		sock_printf(s,"001 FAIL Lock not within repository\n");
		return false;
	}

	if(request_lock(client,path,uFlags,lock_to_wait_for))
	{
		VersionMapType ver;
		size_t newId = ++global_lockId;
		if((uFlags&lfFull) && (uFlags&lfRead))
		{
			TransactionListType::const_iterator i = TransactionList.begin();
			while(i!=TransactionList.end())
			{
				/* This is where atomicity really 'happens' */
				/* Note we do the strcmp last for speed */
				if(i->owner!=client &&   /* Not us */
				   LockClientMap.find(i->owner)!=LockClientMap.end() && /* Exists */
				   TimeIntersects(LockClientMap[i->owner].starttime,LockClientMap[i->owner].endtime,
					   LockClientMap[client].starttime,LockClientMap[client].endtime) && /* Overlaps us */
 				   !strcmp(i->path.c_str(),path)) /* Our file */
				{
					ver[i->branch]=i->oldversion;
					break;
				}
				++i;
			}
		}
		DEBUG("(#%d) Lock request on %s (%s) (granted %u)\n",(int)client,path,FlagToString(uFlags),(unsigned)newId);
		sock_printf(s,"000 OK Lock granted (%u)\n",(unsigned)newId);
		LockMap[newId].flags=uFlags;
		LockMap[newId].path=path;
		LockMap[newId].length=strlen(path);
		LockMap[newId].owner=client;
		LockMap[newId].versions = ver;
	}
	else
	{
		DEBUG("(#%d) Lock request on %s (%s) (wait for %u)\n",(int)client,path,FlagToString(uFlags),(unsigned)lock_to_wait_for);
		sock_printf(s,"002 WAIT Lock busy|%s|%s|%s\n",LockClientMap[LockMap[lock_to_wait_for].owner].user.c_str(),LockClientMap[LockMap[lock_to_wait_for].owner].client_host.c_str(),LockMap[lock_to_wait_for].path.c_str());
	}

	return true;
}

bool DoUnlock(SOCKET s,size_t client, char *param)
{
	size_t lockId;
	unsigned helper;

	if(LockClientMap[client].state!=lcActive)
	{
		sock_printf(s,"001 FAIL Unexpected 'Unlock' command\n");
		return false;
	}
	sscanf(param,"%u",&helper); /* 64 bit aware */
	lockId = helper;

	if(LockMap.find(lockId)==LockMap.end())
	{
		sock_printf(s,"001 FAIL Unknown lock id %u\n",(unsigned)lockId);
		return false;
	}
	if(LockMap[lockId].owner != client)
	{
		sock_printf(s,"001 FAIL Do not own lock id %u\n",(unsigned)lockId);
		return false;
	}

	LockMap.erase(LockMap.find(lockId));

	DEBUG("(#%d) Unlock request on lock %u\n",(int)client,(unsigned)lockId);

	sock_printf(s,"000 OK Unlocked\n");
	return true;
}

bool DoMonitor(SOCKET s,size_t client, char *param)
{
	if(LockClientMap[client].state!=lcLogin)
	{
		sock_printf(s,"001 FAIL Unexpected 'Monitor' command\n");
		return false;
	}
	LockClientMap[client].state=lcMonitor;
	sock_printf(s,"000 OK Entering monitor mode\n");
	return true;
}

bool DoClients(SOCKET s,size_t client, char *param)
{
	if(LockClientMap[client].state!=lcMonitor)
	{
		sock_printf(s,"001 FAIL Unexpected 'Clients' command\n");
		return false;
	}
	LockClientMapType::const_iterator i;
	for(i = LockClientMap.begin(); !(i==LockClientMap.end()); ++i)
	{
		sock_printf(s,"(#%d) %s|%s|%s|%s|%s\n",
			(int)i->first,
			i->second.server_host.c_str(),
			i->second.client_host.c_str(),
			i->second.user.c_str(),
			i->second.root.c_str(),
			StateString[i->second.state]);
	}
	sock_printf(s,"000 OK\n");
	return true;
}

bool DoLocks(SOCKET s,size_t client, char *param)
{
	if(LockClientMap[client].state!=lcMonitor)
	{
		sock_printf(s,"001 FAIL Unexpected 'Locks' command\n");
		return false;
	}
	for(LockMapType::const_iterator i = LockMap.begin(); i!=LockMap.end(); ++i)
		sock_printf(s,"(#%d) %s|%s (%u)\n",(int)i->second.owner,i->second.path.c_str(),FlagToString(i->second.flags), (unsigned)i->first);

	sock_printf(s,"000 OK\n");
	return true;
}

bool DoModified(SOCKET s,size_t client, char *param)
{
	char *id,*branch,*version,*oldversion;
	char type;
	size_t lockId;
	unsigned helper;

	if(LockClientMap[client].state!=lcActive)
	{
		sock_printf(s,"001 FAIL Unexpected 'Modified' command\n");
		return false;
	}
	id = strchr(param,'|');
	if(!id)
	{
		sock_printf(s,"001 FAIL Modified command expects <flags>|<lockId>|<branch>|<version>|oldversion\n");
		return false;
	}
	(*id++)='\0';
	sscanf(id,"%u",&helper);
	lockId = helper;
	branch = strchr(id,'|');
	if(!branch)
	{
		sock_printf(s,"001 FAIL Modified command expects <flags>|<lockId>|<branch>|<version>|oldversion\n");
		return false;
	}
	*(branch++)='\0';
	version = strchr(branch,'|');
	if(!version)
	{
		sock_printf(s,"001 FAIL Modified command expects <flags>|<lockId>|<branch>|<version>|oldversion\n");
		return false;
	}
	*(version++)='\0';
	oldversion = strchr(version,'|');
	if(strchr(oldversion+1,'|'))
	{
		sock_printf(s,"001 FAIL Modified command expects <flags>|<lockId>|<branch>|<version>|oldversion\n");
		return false;
	}
	if(oldversion)
	  *(oldversion++)='\0';
	if(!*param)
		type='M';
	else if(!strcmp(param,"Added"))
		type='A';
	else  if(!strcmp(param,"Deleted"))
		type='D';
	else
	{
		sock_printf(s,"001 FAIL Modified command expects <flags>|<lockId>|<branch>|<version>\n");
		return false;
	}

	if(LockMap.find(lockId)==LockMap.end())
	{
		sock_printf(s,"001 FAIL Unknown lock id %u\n",(unsigned)lockId);
		return false;
	}
	if(LockMap[lockId].owner != client)
	{
		sock_printf(s,"001 FAIL Do not own lock id %u\n",(unsigned)lockId);
		return false;
	}

	if(!(LockMap[lockId].flags&lfWrite))
	{
		sock_printf(s,"001 FAIL No write lock on file\n");
		return false;
	}

	DEBUG("(#%d) Modified request on lock %u (%s:%s [%c])\n",(int)client,(unsigned)lockId,branch,version,type);

	sock_printf(s,"000 OK\n");

	TransactionStruct t;
	t.owner=client;
	t.path=LockMap[lockId].path;
	t.branch=branch;
	t.version=version;
	t.oldversion=oldversion;
	t.type=type;
	TransactionList.push_back(t);

	return true;
}

bool DoVersion(SOCKET s,size_t client, char *param)
{
	char *branch;
	size_t lockId;
	unsigned helper;

	if(LockClientMap[client].state!=lcActive)
	{
		sock_printf(s,"001 FAIL Unexpected 'Version' command\n");
		return false;
	}
	branch = strchr(param,'|');
	if(!branch)
	{
		sock_printf(s,"001 FAIL Version command expects <lockid>|<branch>\n");
		return false;
	}
	(*branch++)='\0';
	
	sscanf(param,"%u",&helper); /* 64 bit aware */
	lockId = helper;

	if(LockMap.find(lockId)==LockMap.end())
	{
		sock_printf(s,"001 FAIL Unknown lock id %u\n",(unsigned)lockId);
		return false;
	}
	if(LockMap[lockId].owner != client)
	{
		sock_printf(s,"001 FAIL Do not own lock id %u\n",(unsigned)lockId);
		return false;
	}
	
	VersionMapType& ver = LockMap[lockId].versions;

	if(ver.find(branch)==ver.end())
	{
		sock_printf(s,"000 OK\n");
		DEBUG("(#%d) Version request on lock %u (%s)\n",(int)client,(unsigned)lockId,branch);
	}
	else
	{
		sock_printf(s,"000 OK (%s)\n",ver[branch].c_str());
		DEBUG("(#%d) Version request on lock %u (%s) returned %s\n",(int)client,(unsigned)lockId,branch,ver[branch].c_str());
	}

	return true;
}

bool request_lock(size_t client, const char *path, unsigned flags, size_t& lock_to_wait_for)
{
	LockMapType::const_iterator i;
	size_t pathlen = strlen(path);
	for(i=LockMap.begin(); i!=LockMap.end(); ++i)
	{
		size_t locklen = i->second.length;

		if((locklen==pathlen && !strcmp(path,i->second.path.c_str())))
		{
			// Locks are as follows:
			// max. 1 advisory write lock, any number of concurrent advisory read locks.
			// max. 1 full write lock cannot be shared with any read locks
			// any number of advisory read locks (provided it doesn't clash with a full write)
			// any number of full read locks (provided it doesn't clash with a full write)

			// As a special concession allow the same user to add multiple locks

			if(flags&lfWrite) /* Trying to add write lock */
			{
				/* Only one write lock (full or advisory) on any object */
				if(i->second.flags&lfWrite && i->second.owner!=client)
					break;
				/* If there is a full read lock on any object, can't write at the moment */
				if(((i->second.flags&(lfRead|lfFull))==(lfRead|lfFull)) && i->second.owner!=client)
					break;
			}
			else /* read lock */
			{
				/* If there is a full write lock on this object then fail */
				if(((i->second.flags&(lfFull|lfWrite))==(lfFull|lfWrite)) && i->second.owner!=client)
					break;
			}
		}
	}
	if(i!=LockMap.end())
	{
		lock_to_wait_for = i->first;
		return false;
	}
	return true;
}

