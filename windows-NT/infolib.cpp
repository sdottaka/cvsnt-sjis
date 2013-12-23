/* CVS info library interface

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

#include <map>
#include <string>

#include "cvs.h"

#include "infolib.h"

#ifndef CVS95

#include <comdef.h>
#include <objbase.h>

#include "cvscom_h.h"

struct InfoStruct
{
	ICvsInfo *i1;
	ICvsInfo2 *i2;
};

#endif

static std::map<std::string, library_callback *> infolib_list;

int isinfolibrary(const char *filter)
{
#ifndef CVS95
	if(filter[0]=='{')
	{
		/* COM filter */
		CLSID id;
		wchar_t str[128];
		char *p = strchr(filter,'}');
		if(!p)
			return 0;

		MultiByteToWideChar(CP_ACP,0,filter,(p-filter)+1,str,sizeof(str)/sizeof(wchar_t));
		str[(p-filter)+1]='\0';
		HRESULT hr = CLSIDFromString(str,&id);
		if(hr!=NOERROR)
			return 0;
		return 1;
	}
#endif
	if(filter[0]=='@')
	{
		return 1; /* DLL */
	}
	return 0;
}

#ifndef CVS95
int COM_init(struct library_callback_t* cb, const char *repository, const char *username, const char *prefix, const char *sessionid, const char *hostname)
{
	ICvsInfo *i = ((InfoStruct*)cb->_reserved)->i1;
	return i->init(_bstr_t(repository),_bstr_t(username),_bstr_t(prefix),_bstr_t(sessionid),_bstr_t(hostname));
}

int COM_pretag(struct library_callback_t* cb, const char *tag, const char *action,  const char *repository,  int pretag_list_count, const char **pretag_list)
{
	ICvsInfo *i = ((InfoStruct*)cb->_reserved)->i1;
	SAFEARRAY *sa_pretag_list = SafeArrayCreateVector(VT_BSTR, 0, pretag_list_count);
	for(long n=0; n<pretag_list_count; n++)
		SafeArrayPutElement(sa_pretag_list, &n, _bstr_t(pretag_list[n]).Detach());
	int ret = i->pretag(_bstr_t(tag),_bstr_t(action),_bstr_t(repository),sa_pretag_list);
	SafeArrayDestroy(sa_pretag_list);
	return ret;
}

int COM_verifymsg(struct library_callback_t* cb, const char *filename)
{
	ICvsInfo *i = ((InfoStruct*)cb->_reserved)->i1;
	return i->verifymsg(_bstr_t(filename));
}

int COM_loginfo(struct library_callback_t* cb, const char *repository, const char *hostname, const char *directory, const char *message, const char *status, int change_list_count, change_info *change_list)
{
	ICvsInfo *i = ((InfoStruct*)cb->_reserved)->i1;
	ChangeInfoStruct *pData;
	IRecordInfo *pRI;
	HRESULT hr;
	hr = GetRecordInfoFromGuids(LIBID_CVSNT, 1, 0, 0x409, __uuidof(ChangeInfoStruct), &pRI);
	if(FAILED(hr))
	{
		error(1,0,"GetRecordInfoFromGuids returned 0x%x\n",hr);
	}
	SAFEARRAY *sa_change_list = SafeArrayCreateVectorEx(VT_RECORD, 0, change_list_count,pRI);
	pRI->Release();
	hr = SafeArrayAccessData(sa_change_list, (void**)&pData);
	if(FAILED(hr))
	{
		error(1,0,"SafeArrayAccessData returned 0x%x\n",hr);
	}
	for(long n=0; n<change_list_count; n++)
	{
		pData[n].filename=_bstr_t(change_list[n].filename).Detach();
		pData[n].rev_new=_bstr_t(change_list[n].rev_new).Detach();
		pData[n].rev_old=_bstr_t(change_list[n].rev_old).Detach();
		pData[n].tag=_bstr_t(change_list[n].tag).Detach();
		pData[n].type=(ChangeType)change_list[n].type;
	}
	hr = SafeArrayUnaccessData(sa_change_list);
	int ret = i->loginfo(_bstr_t(repository),_bstr_t(hostname),_bstr_t(directory),_bstr_t(message),_bstr_t(status),sa_change_list);
	SafeArrayDestroy(sa_change_list);
	return ret;
}

int COM_history(struct library_callback_t* cb, const char *repository, const char *history_line)
{
	ICvsInfo *i = ((InfoStruct*)cb->_reserved)->i1;
	return i->history(_bstr_t(repository),_bstr_t(history_line));
}

int COM_notify(struct library_callback_t* cb, const char *short_repository, const char *file, const char *type, const char *repository, const char *who)
{
	ICvsInfo *i = ((InfoStruct*)cb->_reserved)->i1;
	return i->notify(_bstr_t(short_repository),_bstr_t(file),_bstr_t(type),_bstr_t(repository),_bstr_t(who));
}

int COM_precommit(struct library_callback_t* cb, const char *repository, int precommit_list_count, const char **precommit_list)
{
	ICvsInfo *i = ((InfoStruct*)cb->_reserved)->i1;
	TRACE(3,"COM_precommit count=%d\n",precommit_list_count);
	SAFEARRAY *sa_precommit_list = SafeArrayCreateVector(VT_BSTR, 0, precommit_list_count);
	for(long n=0; n<precommit_list_count; n++)
		SafeArrayPutElement(sa_precommit_list, &n, (BSTR)_bstr_t(precommit_list[n]));
	int ret = i->precommit(_bstr_t(repository),sa_precommit_list);
	SafeArrayDestroy(sa_precommit_list);
	return ret;
}

int COM_postcommit(struct library_callback_t* cb, const char *repository)
{
	ICvsInfo *i = ((InfoStruct*)cb->_reserved)->i1;
	return i->postcommit(_bstr_t(repository));
}

int COM_close(struct library_callback_t *cb)
{
	int ret;
	InfoStruct *is = (InfoStruct*)cb->_reserved;
	if(is->i2)
		ret = is->i2->close();
	else
		ret = 0;
	if(is->i2)
		is->i2->Release();
	is->i1->Release();
	delete is;
	delete cb;
	return 0;
}

#endif

library_callback *open_infolibrary(const char *filter)
{
	library_callback *cb = NULL;
	const char *p = filter+1;
	char *copy;
	char *q=copy=xstrdup(filter+1);
	int quote=0;

	/* We do this here so it's done before the CRT in the DLL/COM object initialises */
	if(server_io_socket)
	{
		SetStdHandle(STD_INPUT_HANDLE,(HANDLE)_get_osfhandle(server_io_socket));
		SetStdHandle(STD_OUTPUT_HANDLE,(HANDLE)_get_osfhandle(server_io_socket));
		SetStdHandle(STD_ERROR_HANDLE,(HANDLE)_get_osfhandle(server_io_socket));
	}

	while(*p && (quote || !isspace(*p)))
	{
		if(!quote && (*p=='"' || *p=='\'' || *p=='\\'))
		{
				quote=*p;
		}
		else if(quote=='\\')
		{
			quote=0;
			*(q++)=*(p++);
		}
		else if(*p==quote)
		{
			quote=0;
		}
		else
		{
			*(q++)=*(p++);
		}
	}
	*q='\0';
	if(infolib_list.find(copy)!=infolib_list.end())
	{
		cb = infolib_list[copy];
		xfree(copy);
		return cb;
	}

#ifndef CVS95
	if(filter[0]=='{')
	{
		/* COM filter */
		CLSID id;
		wchar_t str[128];
		char *p = strchr(filter,'}');
		if(!p)
			return 0;

		MultiByteToWideChar(CP_ACP,0,filter,(p-filter)+1,str,sizeof(str)/sizeof(wchar_t));
		str[(p-filter)+1]='\0';
		HRESULT hr = CLSIDFromString(str,&id);
		if(hr!=NOERROR)
			return NULL;
		ICvsInfo *i1;
		ICvsInfo2 *i2;
		hr = CoCreateInstance(id,NULL,CLSCTX_ALL,IID_ICvsInfo,(void**)&i1);
		if(FAILED(hr))
		{
			TRACE(0,"CoCreateInstance failed : %08x",hr);
			return NULL;
		}
		hr = i1->QueryInterface(IID_ICvsInfo2,(void**)&i2);
		if(FAILED(hr))
		{
			i2=NULL;
		}
		cb =  new library_callback;
		InfoStruct *is = new InfoStruct;
		is->i1 = i1;
		is->i2 = i2;
		cb->_reserved=(void*)is;
		cb->init=COM_init;
		cb->close=COM_close;
		cb->history=COM_history;
		cb->notify=COM_notify;
		cb->postcommit=COM_postcommit;
		cb->precommit=COM_precommit;
		cb->pretag=COM_pretag;
		cb->verifymsg=COM_verifymsg;
		cb->loginfo=COM_loginfo;
	}
#endif
	if(filter[0]=='@')
	{
		HMODULE hLib = LoadLibraryA(copy);
		if(!hLib)
			return NULL;
		CVSINFOPROC pCvsInfo = (CVSINFOPROC)GetProcAddress(hLib,"GetCvsInfo");
		if(!pCvsInfo)
			return NULL;
		cb = pCvsInfo();
	}
	if(cb)
	{
		infolib_list[copy]=cb;
		if(cb->init)
			cb->init(cb,current_parsed_root->unparsed_directory,CVS_Username,getcaller(),global_session_id,remote_host_name?remote_host_name:hostname);
	}
	xfree(copy);
	return cb;
}

int close_infolibrary(library_callback *cb)
{
	return 0;
};

int shutdown_infolib()
{
	std::map<std::string, library_callback *>::const_iterator i;
	for(i=infolib_list.begin();i!=infolib_list.end();i++)
	{
		if(i->second->close)
			i->second->close(i->second);
	}
	infolib_list.clear();
	return 0;
}
