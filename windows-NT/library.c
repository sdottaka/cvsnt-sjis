/* CVS auth library interface

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.  */

#define WIN32_LEAN_AND_MEAN
#define _WIN32_WINNT 0x0400
#include <windows.h>
#include <config.h>
#include "cvs.h"

#include "..\protocols\protocol_interface.h"
#include "library.h"

typedef struct protocol_interface *(*tGPI)(const struct server_interface *server);

static int server_get_config_data(const struct server_interface *server, const char *key, const char *value, char *buffer, int buffer_len);
static int server_set_config_data(const struct server_interface *server, const char *key, const char *value, const char *buffer);
static int server_get_global_config_data(const struct server_interface *server, const char *key, const char *value, char *buffer, int buffer_len);
static int server_set_global_config_data(const struct server_interface *server, const char *key, const char *value, const char *buffer);
static int server_error(const struct server_interface *server, int fatal, const char *text);
static int server_getpass(char *password, int max_length, const char *prompt);
static int server_prompt(const char *message, const char *title, int withcancel);
static int server_set_encrypted_channel(int enctypt);

static char cvs_dir[_MAX_PATH] = {0};
static char cvs_command[_MAX_PATH] = {0};

struct server_interface cvs_interface = 
{
	NULL, /* Current root */
	NULL, /* Library directory */
	NULL, /* cvs command */
	
	server_get_config_data,
	server_set_config_data,
	server_get_global_config_data,
	server_set_global_config_data,
	server_error,
	server_getpass,
	server_prompt,
	server_set_encrypted_channel
};

static void get_cvs_dir()
{
	if(!*cvs_dir) /* Only do this once */
	{
		if (GetModuleFileNameA(NULL, cvs_dir, sizeof(cvs_dir)))
		{
			char *p;
			GetShortPathNameA(cvs_dir,cvs_dir,sizeof(cvs_dir));
#ifdef SJIS
			p = _mbsrchr(cvs_dir, '\\');
#else
			p = strrchr(cvs_dir, '\\');
#endif
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
}

const struct protocol_interface *load_protocol(const char *protocol)
{
	char fn[_MAX_PATH];
	HMODULE hLib;
	tGPI get_protocol_interface;
	struct protocol_interface *proto_interface;
	UINT oldMode;

	get_cvs_dir();
	_snprintf(fn,sizeof(fn),"%sprotocol_map.ini",cvs_dir);
	if(GetPrivateProfileStringA("cvsnt",protocol,NULL,fn,sizeof(fn),fn))
	{
		if(!stricmp(fn,"none") || !stricmp(fn,"off") || !stricmp(fn,"disable"))
			return NULL;
		if(!isabsolute(fn))
		{
			char fn2[_MAX_PATH];
			_snprintf(fn2,sizeof(fn2),"%s%s",cvs_dir,fn);
			strcpy(fn,fn2);
		}
	}
	else
		_snprintf(fn,sizeof(fn),"%s%s_protocol.dll",cvs_dir,protocol);

    oldMode=SetErrorMode(SEM_NOOPENFILEERRORBOX|SEM_FAILCRITICALERRORS);
   	hLib = LoadLibraryA(fn);
	SetErrorMode(oldMode);

	if(!hLib)
		return NULL; // Couldn't find protocol - not supported, or supporting DLLs missing

	get_protocol_interface = (tGPI)GetProcAddress(hLib,"get_protocol_interface");
	if(!get_protocol_interface)
	{
		error(0,0,"%s protocol library is corrupt",protocol);
		FreeLibrary(hLib);
		return NULL; // Couldn't find protocol - bad DLL
	}

	proto_interface = get_protocol_interface(&cvs_interface);
	proto_interface->__reserved=(void*)hLib;
	if(proto_interface->interface_version!=PROTOCOL_INTERFACE_VERSION)
	{
		TRACE(1,"Not loading %s - wrong version",protocol);
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
	error(fatal,0,"%s",text);
	return 0;
}

const char *enumerate_protocols(int *context)
{
	static HANDLE hFind=INVALID_HANDLE_VALUE;
	static char fn[_MAX_PATH];
	WIN32_FIND_DATAA wfd;

	if(!*context)
	{
		get_cvs_dir();
		_snprintf(fn,sizeof(fn),"%s*_protocol.dll",cvs_dir);
		hFind=FindFirstFileA(fn,&wfd);
		if(hFind==INVALID_HANDLE_VALUE)
			return NULL;
		*context=1;
	}
	else
	{
		if(hFind==INVALID_HANDLE_VALUE)
			return NULL;
		if(!FindNextFileA(hFind,&wfd))
		{
			FindClose(hFind);
			hFind=INVALID_HANDLE_VALUE;
			*context=2;
			return NULL;
		}
	}
	strcpy(fn,wfd.cFileName);
	*strchr(fn,'_')='\0';
	return fn;
}

const struct protocol_interface *find_authentication_mechanism(const char *tagline)
{
	int context;
	const char *proto;
	const struct protocol_interface *protocol;
	int res;

	context=0;
	while((proto=enumerate_protocols(&context))!=NULL)
	{
		protocol=load_protocol(proto);
		if(!protocol)
			continue;
		if(protocol->auth_protocol_connect)
		{
			setup_server_interface(NULL);
			res = protocol->auth_protocol_connect(protocol,tagline);
			if(res==CVSPROTO_SUCCESS)
				return protocol; /* Correctly authenticated */
			unload_protocol(protocol);
			if(res !=CVSPROTO_NOTME && res!=CVSPROTO_NOTIMP)
			{
				error(1, 0, "Authentication protocol rejected access");
				return NULL; /* Protocol was recognised, but failed */
			}
		}
	}
	return NULL;
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
	char *pw = getpass(prompt);
	if(!pw)
		return 0;
	strncpy(password,pw,max_length);
	return 1;
}

int server_prompt(const char *message, const char *title, int withcancel)
{
	return yesno_prompt(message,title,withcancel);
}

int server_set_encrypted_channel(int encrypt)
{
#ifdef SERVER_SUPPORT
        encrypted_channel = encrypt;
#endif
        return 0;
}

