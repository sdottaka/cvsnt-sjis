/* CVS legacy :ext: interface

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.  */

#include "stdafx.h"

#include "../protocols/protocol_interface.h"

static int server_get_config_data(const struct server_interface *server, const char *key, const char *value, char *buffer, int buffer_len);
static int server_set_config_data(const struct server_interface *server, const char *key, const char *value, const char *buffer);
static int server_get_global_config_data(const struct server_interface *server, const char *key, const char *value, char *buffer, int buffer_len);
static int server_set_global_config_data(const struct server_interface *server, const char *key, const char *value, const char *buffer);
static int server_error(const struct server_interface *server, int fatal, const char *text);
static int server_getpass(char *password, int max_length, const char *prompt);
static int server_prompt(const char *message, const char *title, int withcancel);
static int server_set_encrypted_channel(int enctypt);
static int get_global_config_data(const char *key, const char *value, char *buffer, int buffer_len);
static int set_global_config_data(const char *key, const char *value, const char *buffer);
static const struct protocol_interface *load_protocol(const char *protocol);
static void setup_server_interface(cvsroot_t *root);

static char cvs_dir[MAX_PATH] = {0};
static char cvs_command[MAX_PATH] = {0};

struct server_interface cvs_interface = 
{
	NULL, /* Current root */
	NULL, /* Library directory */
	NULL, /* cvs command */
	0,	  /* Input FD */
	1,	  /* Output FD */
	
	server_get_config_data,
	server_set_config_data,
	server_get_global_config_data,
	server_set_global_config_data,
	server_error,
	server_getpass,
	server_prompt,
	server_set_encrypted_channel
};

static cvsroot_t root;

static void writethread(void *p);

/* Standard :ext: invokes with <hostname> -l <username> cvs server */
int main(int argc, char *argv[])
{
	char protocol[1024],hostname[1024],directory[1024];
	char buf[8000];

	if(argc<6 || strcmp(argv[2],"-l"))
	{
		fprintf(stderr,"This should be invoked as part of an :ext: connection from a cvs client\n");
		return -1;
	}

    WSADATA data;

    if (WSAStartup (MAKEWORD (1, 1), &data))
    {
		fprintf (stderr, "cvs: unable to initialize winsock\n");
		return -1;
    }

	setmode(0,O_BINARY);
	setmode(1,O_BINARY);
	setvbuf(stdin,NULL,_IONBF,0);
	setvbuf(stdout,NULL,_IONBF,0);

	setup_server_interface(&root);

	_snprintf(buf,sizeof(buf),"%s/extnt.ini",cvs_dir);
	if(GetFileAttributes(buf)==0xFFFFFFFF)
	{
		printf("error 0 [extnt] extnt.ini not found\n");
		return -1;
	}

	GetPrivateProfileString(argv[1],"protocol","sspi",protocol,sizeof(protocol),buf);
	GetPrivateProfileString(argv[1],"hostname","",hostname,sizeof(hostname),buf);
	GetPrivateProfileString(argv[1],"directory","",directory,sizeof(directory),buf);

	root.method=protocol;
	root.hostname=hostname;
	root.directory=directory;

	if(!hostname[0] || !directory[0])
	{
		printf("error 0 [extnt] hostname and/or directory not specified in [%s] section of extnt.ini\n",argv[1]);
		return -1;
	}

	const struct protocol_interface *proto = load_protocol(protocol);
	if(!proto)
	{
		printf("error 0 [extnt] Couldn't load procotol %s\n",protocol);
		return -1;
	}

	switch(proto->connect(proto, 0))
	{
	case CVSPROTO_SUCCESS: /* Connect succeeded */
		{
			char line[1024];
			int l = proto->read_data(proto,line,1024);
			line[l]='\0';
			if(!strcmp(line,"I HATE YOU\012"))
			{
				printf("error 0 [extnt] connect aborted: server %s rejeced access to %s\n",hostname,directory);
				return -1;
			}
			if(strcmp(line,"I LOVE YOU\012"))
			{
				printf("error 0 [extnt] Unknown response '%s' from protocol\n", line);
				return -1;
			}
			break;
		}
	case CVSPROTO_SUCCESS_NOPROTOCOL: /* Connect succeeded, don't wait for 'I LOVE YOU' response */
		break;
	case CVSPROTO_FAIL: /* Generic failure (errno set) */
		printf("error 0 [extnt] Connection failed\n");
		return -1;
	case CVSPROTO_BADPARMS: /* (Usually) wrong parameters from cvsroot string */
		printf("error 0 [extnt] Bad parameters\n");
		return -1;
	case CVSPROTO_AUTHFAIL: /* Authorization or login failed */
		printf("error 0 [extnt] Authentication failed\n");
		return -1;
	case CVSPROTO_NOTIMP: /* Not implemented */
		printf("error 0 [extnt] Not implemented\n");
		return -1;
	}

	_beginthread(writethread,0,(void*)proto);
	do
	{
		int len = proto->read_data(proto,buf,sizeof(buf));
		if(len==-1)
		{
			exit(-1); /* dead bidirectionally */
		}
		if(len)
			write(1,buf,len);
		Sleep(50);
	} while(1);

    return 0;
}

void writethread(void *p)
{
	const struct protocol_interface *proto = (const struct protocol_interface *)p;
	char buf[8000];

	do
	{
		int len = read(0,buf,sizeof(buf));
		/* stdin returns 0 when its input pipe goes dead */
		if(len<1)
		{
			exit(0); /* Dead bidirectionally */
		}
		proto->write_data(proto,buf,len);
		Sleep(50);
	} while(1);
}

static void get_cvs_dir()
{
	if(!*cvs_dir) /* Only do this once */
	{
		if (GetModuleFileNameA(NULL, cvs_dir, sizeof(cvs_dir)))
		{
			char *p;
			GetShortPathNameA(cvs_dir,cvs_dir,sizeof(cvs_dir));
			p = strrchr(cvs_dir, '\\');
			strcpy(cvs_command,cvs_dir);
			if(p)
				p[1] = '\0';
			else
				cvs_dir[0] = '\0';
		}
	}
}

struct server_interface *get_server_interface()
{
	return &cvs_interface;
}

void setup_server_interface(cvsroot_t *root)
{
	get_cvs_dir();
	cvs_interface.library_dir = cvs_dir;
	cvs_interface.cvs_command = cvs_command;
	cvs_interface.current_root = root;
	cvs_interface.in_fd = 0;
	cvs_interface.out_fd = 1;
}

const struct protocol_interface *load_protocol(const char *protocol)
{
	char fn[MAX_PATH];
	HMODULE hLib;
	tGPI get_protocol_interface;
	struct protocol_interface *proto_interface;
	UINT oldMode;

	get_cvs_dir();
	_snprintf(fn,sizeof(fn),"%s%s_protocol.dll",cvs_dir,protocol);

    oldMode=SetErrorMode(SEM_NOOPENFILEERRORBOX|SEM_FAILCRITICALERRORS);
   	hLib = LoadLibraryA(fn);
	SetErrorMode(oldMode);

	if(!hLib)
		return NULL; // Couldn't find protocol - not supported, or supporting DLLs missing

	get_protocol_interface = (tGPI)GetProcAddress(hLib,"get_protocol_interface");
	if(!get_protocol_interface)
	{
		printf("error 0 [extnt] %s protocol library is corrupt\n",protocol);
		FreeLibrary(hLib);
		return NULL; // Couldn't find protocol - bad DLL
	}

	proto_interface = get_protocol_interface(&cvs_interface);
	proto_interface->__reserved=(void*)hLib;
	if(proto_interface->interface_version!=PROTOCOL_INTERFACE_VERSION)
	{
		printf("error 0 [extnt] %s protocol library is wrong version\n",protocol);
		if(proto_interface->destroy)
			proto_interface->destroy(proto_interface);
		FreeLibrary(hLib);
		return NULL;
	}
	return proto_interface;
}

int unload_protocol(const struct protocol_interface *protocol)
{
	if(protocol)
	{
		protocol->destroy(protocol);
		FreeLibrary((HMODULE)protocol->__reserved);
	}
	return 0;
}

int server_get_config_data(const struct server_interface *server, const char *key, const char *value, char *buffer, int buffer_len)
{
	HKEY hKey,hSubKey;
	DWORD dwType,dwLen,dw;

	if(RegOpenKeyExA(HKEY_CURRENT_USER,"Software\\Cvsnt",0,KEY_READ,&hKey) &&
	   RegCreateKeyExA(HKEY_CURRENT_USER,"Software\\Cvsnt",0,NULL,0,KEY_READ,NULL,&hKey,NULL))
	{
		return -1; // Couldn't open or create key
	}

	if(key)
	{
		if(RegOpenKeyExA(hKey,key,0,KEY_READ,&hSubKey) &&
		   RegCreateKeyExA(hKey,key,0,NULL,0,KEY_READ,NULL,&hSubKey,NULL))
		{
			RegCloseKey(hKey);
			return -1; // Couldn't open or create key
		}
		RegCloseKey(hKey);
		hKey=hSubKey;
	}

	dwType=REG_SZ;
	dwLen=buffer_len;
	if((dw=RegQueryValueExA(hKey,value,NULL,&dwType,(LPBYTE)buffer,&dwLen))!=0)
	{
		RegCloseKey(hKey);
		return -1;
	}
	RegCloseKey(hKey);
	if(dwType==REG_DWORD)
		sprintf(buffer,"%u",*(DWORD*)buffer);

	return 0;
}

int server_set_config_data(const struct server_interface *server, const char *key, const char *value, const char *buffer)
{
	HKEY hKey,hSubKey;
	DWORD dwLen;

	if(RegOpenKeyExA(HKEY_CURRENT_USER,"Software\\Cvsnt",0,KEY_READ,&hKey) &&
	   RegCreateKeyExA(HKEY_CURRENT_USER,"Software\\Cvsnt",0,NULL,0,KEY_READ,NULL,&hKey,NULL))
	{
		return -1; // Couldn't open or create key
	}

	if(key)
	{
		if(RegOpenKeyExA(hKey,key,0,KEY_WRITE,&hSubKey) &&
		   RegCreateKeyExA(hKey,key,0,NULL,0,KEY_WRITE,NULL,&hSubKey,NULL))
		{
			RegCloseKey(hKey);
			return -1; // Couldn't open or create key
		}
		RegCloseKey(hKey);
		hKey=hSubKey;
	}

	if(!buffer)
	{
		RegDeleteValueA(hKey,value);
	}
	else
	{
		dwLen=strlen(buffer);
		if(RegSetValueExA(hKey,value,0,REG_SZ,(LPBYTE)buffer,dwLen+1))
		{
			RegCloseKey(hKey);
			return -1;
		}
	}
	RegCloseKey(hKey);

	return 0;
}

int server_error(const struct server_interface *server, int fatal, const char *text)
{
	printf("E [extnt] %s",text);
	if(fatal)
		exit(-1);
	return 0;
}


int server_get_global_config_data(const struct server_interface *server, const char *key, const char *value, char *buffer, int buffer_len)
{
	return get_global_config_data(key,value,buffer, buffer_len);
}

int get_global_config_data(const char *key, const char *value, char *buffer, int buffer_len)
{
	HKEY hKey,hSubKey;
	DWORD dwType,dwLen;

	if(RegOpenKeyExA(HKEY_LOCAL_MACHINE,"Software\\CVS",0,KEY_READ,&hKey) &&
	   RegCreateKeyExA(HKEY_LOCAL_MACHINE,"Software\\CVS",0,NULL,0,KEY_READ,NULL,&hKey,NULL))
	{
		return -1; // Couldn't open or create key
	}

	if(key)
	{
		if(RegOpenKeyExA(hKey,key,0,KEY_READ,&hSubKey) &&
		   RegCreateKeyExA(hKey,key,0,NULL,0,KEY_READ,NULL,&hSubKey,NULL))
		{
			RegCloseKey(hKey);
			return -1; // Couldn't open or create key
		}
		RegCloseKey(hKey);
		hKey=hSubKey;
	}

	dwType=REG_SZ;
	dwLen=buffer_len;
	if(RegQueryValueExA(hKey,value,NULL,&dwType,(LPBYTE)buffer,&dwLen))
	{
		RegCloseKey(hKey);
		return -1;
	}
	RegCloseKey(hKey);
	if(dwType==REG_DWORD)
	    sprintf(buffer,"%u",*(DWORD*)buffer);

	return 0;
}

int server_set_global_config_data(const struct server_interface *server, const char *key, const char *value, const char *buffer)
{
	return set_global_config_data(key,value,buffer);
}

int set_global_config_data(const char *key, const char *value, const char *buffer)
{
	HKEY hKey,hSubKey;
	DWORD dwLen;

	if(RegOpenKeyExA(HKEY_LOCAL_MACHINE,"Software\\CVS",0,KEY_READ,&hKey) &&
	   RegCreateKeyExA(HKEY_LOCAL_MACHINE,"Software\\CVS",0,NULL,0,KEY_READ,NULL,&hKey,NULL))
	{
		return -1; // Couldn't open or create key
	}

	if(key)
	{
		if(RegOpenKeyExA(hKey,key,0,KEY_WRITE,&hSubKey) &&
		   RegCreateKeyExA(hKey,key,0,NULL,0,KEY_WRITE,NULL,&hSubKey,NULL))
		{
			RegCloseKey(hKey);
			return -1; // Couldn't open or create key
		}
		RegCloseKey(hKey);
		hKey=hSubKey;
	}

	if(!buffer)
	{
		RegDeleteValueA(hKey,value);
	}
	else
	{
		dwLen=strlen(buffer);
		if(RegSetValueExA(hKey,value,0,REG_SZ,(LPBYTE)buffer,dwLen+1))
		{
			RegCloseKey(hKey);
			return -1;
		}
	}
	RegCloseKey(hKey);

	return 0;
}

int enum_global_config_data(const char *key, int value_num, char *value, int value_len, char *buffer, int buffer_len)
{
	HKEY hKey,hSubKey;
	DWORD dwType,dwLen,dwValLen;
	DWORD dwRes;

	if(RegOpenKeyExA(HKEY_LOCAL_MACHINE,"Software\\CVS",0,KEY_READ,&hKey) &&
	   RegCreateKeyExA(HKEY_LOCAL_MACHINE,"Software\\CVS",0,NULL,0,KEY_READ,NULL,&hKey,NULL))
	{
		return -1; // Couldn't open or create key
	}

	if(key)
	{
		if(RegOpenKeyExA(hKey,key,0,KEY_READ,&hSubKey) &&
		   RegCreateKeyExA(hKey,key,0,NULL,0,KEY_READ,NULL,&hSubKey,NULL))
		{
			RegCloseKey(hKey);
			return -1; // Couldn't open or create key
		}
		RegCloseKey(hKey);
		hKey=hSubKey;
	}

	dwLen=buffer_len;
	dwValLen=value_len;
	if((dwRes=RegEnumValueA(hKey,value_num,value,&dwValLen,NULL,&dwType,(LPBYTE)buffer,&dwLen))!=0 && dwRes!=234)
	{
		RegCloseKey(hKey);
		return -1;
	}
	RegCloseKey(hKey);
	if(dwType==REG_DWORD)
	    sprintf(buffer,"%u",*(DWORD*)buffer);

	return 0;
}

int server_getpass(char *password, int max_length, const char *prompt)
{
	assert(FALSE);
	return 0;
}

int server_prompt(const char *message, const char *title, int withcancel)
{
	assert(FALSE);
	return 0;
}

int server_set_encrypted_channel(int encrypt)
{
	assert(FALSE);
    return 0;
}

