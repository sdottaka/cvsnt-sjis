/* CVS auth library interface

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.  */

#include <config.h>
#include "cvs.h"
#include <ltdl.h>
#include <glob.h>

#include <pwd.h>
#include <sys/types.h>

#ifdef HAVE_SYSLOG_H
#include <syslog.h>
#endif

#include "library.h"

static int server_get_config_data(const struct server_interface *server, const char *key, const char *value, char *buffer, int buffer_len);
static int server_set_config_data(const struct server_interface *server, const char *key, const char *value, const char *buffer);
static int server_get_global_config_data(const struct server_interface *server, const char *key, const char *value, char *buffer, int buffer_len);
static int server_set_global_config_data(const struct server_interface *server, const char *key, const char *value, const char *buffer);
static int server_error(const struct server_interface *server, int fatal, const char *text);
static int server_getpass(char *password, int max_length, const char *prompt);
static int server_prompt(const char *message, const char *title, int withcancel);
static int server_set_encrypted_channel(int enctypt);

struct server_interface cvs_interface = 
{
	NULL, /* Current root */
	NULL, /* Library directory */
	NULL, /* cvs command */
	0, /* Server input fd */
	1, /* Server output fd */
	
	server_get_config_data,
	server_set_config_data,
	server_get_global_config_data,
	server_set_global_config_data,
	server_error,
	server_getpass,
	server_prompt,
	server_set_encrypted_channel
};

struct server_interface *get_server_interface()
{
	return &cvs_interface;
}

void setup_server_interface(cvsroot_t *root)
{
	cvs_interface.library_dir = CVS_LIBRARY_DIR"/";
	cvs_interface.cvs_command = program_name; 
	cvs_interface.current_root = root;
}

const struct protocol_interface *load_protocol(const char *protocol)
{
	char fn[512];
	lt_dlhandle handle;
	tGPI get_protocol_interface;
	struct protocol_interface *proto_interface;

	snprintf(fn,sizeof(fn),"%s/%s_protocol",CVS_LIBRARY_DIR,PATCH_NULL(protocol));

	TRACE(1,"Loading protocol %s",PATCH_NULL(fn));

	lt_dlinit();
	handle = lt_dlopenext(fn);

	if(!handle)
		return NULL; // Couldn't find protocol - not supported, or supporting DLLs missing

	get_protocol_interface = (tGPI)lt_dlsym(handle,"get_protocol_interface");
	if(!get_protocol_interface)
	{
		error(0,0,"%s protocol library is corrupt",protocol);
		lt_dlclose(handle);
		return NULL; // Couldn't find protocol - bad DLL
	}

	proto_interface = get_protocol_interface(&cvs_interface);
	proto_interface->__reserved=handle;
	proto_interface->name = xstrdup(protocol);
	return proto_interface;
}

int unload_protocol(const struct protocol_interface *protocol)
{
	if(protocol)
	{
		protocol->destroy(protocol);
		xfree(protocol->name);
		lt_dlclose((lt_dlhandle)protocol->__reserved);
		lt_dlexit();
	}
	return 0;
}

void get_config_file(const char *key, char *fn, int fnlen)
{
  struct passwd *pw = getpwuid(getuid());

  snprintf(fn,fnlen,"%s/.cvs",PATCH_NULL(pw->pw_dir));
  mkdir(fn,0777);
  snprintf(fn,fnlen,"%s/.cvs/%s",PATCH_NULL(pw->pw_dir),PATCH_NULL(key));

  TRACE(1,"Config file name %s",PATCH_NULL(fn));
}

int server_get_config_data(const struct server_interface *server, const char *key, const char *value, char *buffer, int buffer_len)
{
	char fn[512],line[1024];
	FILE *f;
	char *p;

	/* Special case for the 'cvspass' key */
	if(!strcmp(key,"cvspass") && !get_cached_password(value,buffer,buffer_len))
			return 0;

	get_config_file(key,fn,sizeof(fn));
	f=fopen(fn,"r");
	if(f==NULL)
	  return -1;

	/* Read keys */
	while(fgets(line,sizeof(line),f))
	{
	  line[strlen(line)-1]='\0';
	  p=strchr(line,'=');
	  if(p)
	    *p='\0';
	  if(!strcasecmp(value,line))
	  {
	    if(p)
	      strncpy(buffer,p+1,buffer_len);
	    else
	      *buffer='\0';
	    fclose(f);
	    return 0;
	  }
        }
	fclose(f);
	return -1;
}

int server_set_config_data(const struct server_interface *server, const char *key, const char *value, const char *buffer)
{
	char fn[512],fn2[512],line[1024];
	FILE *f,*o;
	char *p;
	int found;

	get_config_file(key,fn,sizeof(fn));
	f=fopen(fn,"r");
	if(f==NULL)
	{
	  f=fopen(fn,"w");
	  if(f==NULL)
	  {
		TRACE(1,"Couldn't create config file %s",PATCH_NULL(fn));
	    return -1;
	  }
	  if(buffer)
	    fprintf(f,"%s=%s\n",value,buffer);
	  fclose(f);
	  return 0;
	}
	sprintf(fn2,"%s.new",fn);
	o=fopen(fn2,"w");
	if(o==NULL)
	{
      TRACE(1,"Couldn't create temporary file %s",PATCH_NULL(fn2));
	  fclose(f);
	  return -1;
	}

	/* Read keys */
	found=0;
	while(fgets(line,sizeof(line),f))
	{
	  line[strlen(line)-1]='\0';
	  p=strchr(line,'=');
	  if(p)
	    *p='\0';
	  if(!strcasecmp(value,line))
	  {
	    if(buffer)
	    {
	      strcat(line,"=");
	      strcat(line,buffer);
	      fprintf(o,"%s\n",line);
	    }
	    found=1;
	  }
	  else
	  {
	    if(p)
		*p='=';
	    fprintf(o,"%s\n",line);
	  }
        }
        if(!found && buffer)
	  fprintf(o,"%s=%s\n",value,buffer);
	fclose(f);
	fclose(o);
	rename(fn2,fn);
	return 0;
}

int server_error(const struct server_interface *server, int fatal, const char *text)
{
	/* If an auth module reports a fatal error, we need to log it 
           as the client probably won't see it */
#ifdef HAVE_SYSLOG_H
	syslog (LOG_DAEMON | (fatal?LOG_ERR:LOG_NOTICE), "%s", text);
#endif
	error(fatal,0,"%s",text);
	return 0;
}

const char *enumerate_protocols(int *context)
{
	static char fn[512];
	static glob_t globbuf;
	char *p;

	if(!*context)
	{
		snprintf(fn,sizeof(fn),"%s/*_protocol.la",CVS_LIBRARY_DIR);
		globbuf.gl_offs = 0;
		if(glob(fn,GLOB_ERR|GLOB_NOSORT,NULL,&globbuf))
		  return NULL;
		*context=1;
	}
	else
	{
		(*context)++;
		if(*context>globbuf.gl_pathc)
		{
		  globfree(&globbuf);
		  *context=-1;
		  return NULL;
		}
	}
	strcpy(fn,globbuf.gl_pathv[(*context)-1]);
	*strrchr(fn,'_')='\0';
	p=strrchr(fn,'/');
	return p?p+1:fn; 
}

const struct protocol_interface *find_authentication_mechanism(const char *tagline)
{
	int context;
	const char *proto;
	const struct protocol_interface *protocol;

	context=0;
	while((proto=enumerate_protocols(&context))!=NULL)
	{
		protocol=load_protocol(proto);
		if(!protocol)
			continue;
		if(protocol->auth_protocol_connect)
		{
			int res;
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
		else
			unload_protocol(protocol);
	}
	return NULL;
}

void get_global_config_file(const char *key, char *fn, int fnlen)
{
  snprintf(fn,fnlen,"%s/%s",CVS_CONFIG_DIR,PATCH_NULL(key));
}

int server_get_global_config_data(const struct server_interface *server, const char *key, const char *value, char *buffer, int buffer_len)
{
	return get_global_config_data(key,value,buffer, buffer_len);
}

int get_global_config_data(const char *key, const char *value, char *buffer, int buffer_len)
{
        char fn[512],line[1024];
        FILE *f;
        char *p;

        get_global_config_file(key,fn,sizeof(fn));
        f=fopen(fn,"r");
        if(f==NULL)
          return -1;

        /* Read keys */
        while(fgets(line,sizeof(line),f))
        {
          line[strlen(line)-1]='\0';
          p=strchr(line,'=');
          if(p)
            *p='\0';
          if(!strcasecmp(value,line))
          {
            if(p)
              strncpy(buffer,p+1,buffer_len);
            else
              *buffer='\0';
            fclose(f);
            return 0;
          }
        }
        fclose(f);
        return -1;
}

int server_set_global_config_data(const struct server_interface *server, const char *key, const char *value, const char *buffer)
{
	return set_global_config_data(key,value,buffer);
}

int set_global_config_data(const char *key, const char *value, const char *buffer)
{
        char fn[512],fn2[512],line[1024];
        FILE *f,*o;
        char *p;
        int found;

        get_global_config_file(key,fn,sizeof(fn));
        f=fopen(fn,"r");
        if(f==NULL)
        {
          f=fopen(fn,"w");
          if(f==NULL)
          {
            if(trace)
             printf("%s -> Couldn't create config file %s\n",server_active?"S":"C",fn);
            return -1;
          }
          if(buffer)
            fprintf(f,"%s=%s\n",value,buffer);
          fclose(f);
          return 0;
        }
        sprintf(fn2,"%s.new",fn);
        o=fopen(fn2,"w");
        if(o==NULL)
        {
          if(trace)
            printf("%s -> Couldn't create temporary file %s\n",server_active?"S":"C",fn2);
          fclose(f);
          return -1;
        }

        /* Read keys */
        found=0;
        while(fgets(line,sizeof(line),f))
        {
          line[strlen(line)-1]='\0';
          p=strchr(line,'=');
          if(p)
            *p='\0';
          if(!strcasecmp(value,line))
          {
            if(buffer)
            {
              strcat(line,"=");
              strcat(line,buffer);
              fprintf(o,"%s\n",line);
            }
            found=1;
          }
	  else
	  {
	    if(p)
		*p='=';
	    fprintf(o,"%s\n",line);
	  }
        }
        if(!found && buffer)
          fprintf(o,"%s=%s\n",value,buffer);
        fclose(f);
        fclose(o);
        rename(fn2,fn);
        return 0;
}

int enum_global_config_data(const char *key, int value_num, char *value, int value_len, char *buffer, int buffer_len)
{
  char fn[512],line[1024];
  FILE *f;
  char *token,*v,*p;

  get_global_config_file(key,fn,sizeof(fn));
  f=fopen(fn,"r");
  if(f==NULL)
     return -1;

  /* Read keys */
  while(fgets(line,sizeof(line),f))
  {
    line[strlen(line)-1]='\0';
    if(line[0]=='#' || !strlen(line))
      continue;
    if(!value_num--)
    {
      for(token=line; isspace(*token); token++)
        ;
      v=p=strchr(token,'=');
      if(!p && !strlen(token))
        continue;
      if(p)
      {
        *p='\0';
        v++;
      }
      for(;isspace(*p); p++)
        *p='\0';
      for(;v && isspace(*v); v++)
        ;
      strncpy(value,token,value_len);
      if(p && v && strlen(v))
        strncpy(buffer,v,buffer_len);
      else
        *buffer='\0';
      fclose(f);
      return 0;
    }
  }
  fclose(f);
  return -1;
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

