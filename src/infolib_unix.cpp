/* CVS info library interface

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.  */


#include "cvs.h"
#include <config.h>

#include <ltdl.h>
#include <map>
#include <string>

#include "infolib.h"

static std::map<std::string, library_callback *> infolib_list;

int isinfolibrary(const char *filter)
{
	if(filter[0]=='@')
	{
		return 1; /* Shared library */
	}
	return 0;
}

library_callback *open_infolibrary(const char *filter)
{
	library_callback *cb = NULL;
	const char *p = filter+1;
	char *copy;
	char *q=copy=xstrdup(filter+1);
	int quote=0;
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
	if(filter[0]=='@')
	{
	    lt_dlhandle handle;
        lt_dlinit();
    	handle = lt_dlopenext(copy);
		if(!handle)
			return NULL;
		CVSINFOPROC pCvsInfo = (CVSINFOPROC)lt_dlsym(handle,"GetCvsInfo");
		if(!pCvsInfo)
		{
       		        lt_dlclose(handle);
			return NULL;
		}
		cb = pCvsInfo();
	}
	if(cb)
	{
		infolib_list[copy]=cb;
		if(cb->init)
			cb->init(cb,current_parsed_root->unparsed_directory,current_parsed_root->username,"",global_session_id,remote_host_name?remote_host_name:hostname);
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
