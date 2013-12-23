/* CVS ext auth interface

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.  */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#endif

#include "protocol_interface.h"
#include "common.h"
#include "../version.h"

static void ext_destroy(const struct protocol_interface *protocol);
static int ext_connect(const struct protocol_interface *protocol, int verify_only);
static int ext_disconnect(const struct protocol_interface *protocol);
static int ext_read_data(const struct protocol_interface *protocol, void *data, int length);
static int ext_write_data(const struct protocol_interface *protocol, const void *data, int length);
static int ext_flush_data(const struct protocol_interface *protocol);
static int ext_shutdown(const struct protocol_interface *protocol);
static int ext_validate_keyword(const struct protocol_interface *protocol, cvsroot_t *root, const char *keyword, const char *value);
static const char *ext_get_keyword_help(const struct protocol_interface *protocol);

static int expand_command_line(char *result, int length, const char *command, const cvsroot_t* root);

static int current_in=-1, current_out=-1;

struct protocol_interface ext_protocol_interface =
{
	PROTOCOL_INTERFACE_VERSION,

	"ext",
	"ext "CVSNT_PRODUCTVERSION_STRING,
	":ext[{program}][;keyword=value...]:[user@]host[:]/path",

	ext_destroy,

	elemHostname,				/* Required elements */
	elemUsername|elemHostname,	/* Valid elements */

	NULL, /* validate_details */
	ext_connect,
	ext_disconnect,
	NULL, /* login */
	NULL, /* logout */
	NULL, /* start_encryption */
	NULL, /* auth_protocol_connect */
	ext_read_data,
	ext_write_data,
	ext_flush_data,
	ext_shutdown,
	NULL, /* impersonate */
	ext_validate_keyword,
	ext_get_keyword_help,
	NULL, /* read_data */
	NULL, /* write_data */
	NULL, /* flush_data */
	NULL /* shutdown */
};

struct protocol_interface *get_protocol_interface(const struct server_interface *server)
{
	current_server = server;
	return &ext_protocol_interface;
}

void ext_destroy(const struct protocol_interface *protocol)
{
	free(protocol->auth_username);
	free(protocol->auth_password);
	free(protocol->auth_repository);
}

int ext_connect(const struct protocol_interface *protocol, int verify_only)
{
	char command[256],command_line[1024];
		char *env;

	if(!current_server->current_root->hostname || !current_server->current_root->directory || current_server->current_root->port)
		return CVSPROTO_BADPARMS;

	if(current_server->current_root->optional_1)
		expand_command_line(command_line,sizeof(command_line),current_server->current_root->optional_1,current_server->current_root); // CVSROOT parameter
	else if(!get_user_local_config_data("ext","command",command,sizeof(command)))
		expand_command_line(command_line,sizeof(command_line),command,current_server->current_root); // CVSROOT parameter
	else if((env = getenv("CVS_EXT"))!=NULL) // cvsnt environment
		expand_command_line(command_line,sizeof(command_line),env,current_server->current_root);
	else if((env = getenv("CVS_RSH"))!=NULL) // legacy cvs environment
	{
		//  People are amazingly anal about the handling of this env. so we try to make
		//  it as unixy as possible.  Personally I'd rather ditch it as it's so limiting.
		if(env[0]=='"') // Strip quotes.  It's incorrect to put quotes around a CVS_RSH but it does happen
			env++;
		if(env[strlen(env)-1]=='"')
			env[strlen(env)-1]='\0';
		snprintf(command_line,sizeof(command_line),"\"%s\" %s -l \"%s\"",env,current_server->current_root->hostname,get_username(current_server->current_root));
	}
	else
		expand_command_line(command_line,sizeof(command_line),"ssh -l \"%u\" %h",current_server->current_root);

	strcat(command_line," ");
	if((env=getenv("CVS_SERVER"))!=NULL)
		strcat(command_line,env);
	else
		strcat(command_line,"cvs");
	strcat(command_line," server");
	if(run_command(command_line,&current_in, &current_out, NULL))
		return CVSPROTO_FAIL;

	return CVSPROTO_SUCCESS_NOPROTOCOL; /* :ext: doesn't need login response */
}

int ext_disconnect(const struct protocol_interface *protocol)
{
	if(current_in>0)
	{
		close(current_in);
		current_in=-1;
	}
	if(current_out>0)
	{
		close(current_out);
		current_in=-1;
	}
	return CVSPROTO_SUCCESS;
}

int ext_read_data(const struct protocol_interface *protocol, void *data, int length)
{
	return read(current_out,data,length);
}

int ext_write_data(const struct protocol_interface *protocol, const void *data, int length)
{
	return write(current_in,data,length);
}

int ext_flush_data(const struct protocol_interface *protocol)
{
	return 0;
}

int ext_shutdown(const struct protocol_interface *protocol)
{
	return 0;
}

int expand_command_line(char *result, int length, const char *command, const cvsroot_t* root)
{
	const char *p;
	char *q;

	q=result;
	for(p=command; *p && (q-result)<length; p++)
	{
		if(*p=='%')
		{
			switch(*(p+1))
			{
			case 0:
				p--; // Counteract p++ below, so we end on NULL
				break;
			case '%':
				*(q++)='%';
				break;
			case 'u':
				strcpy(q, get_username(root));
				q+=strlen(q);
				break;
			case 'h':
				strcpy(q, root->hostname);
				q+=strlen(q);
				break;
			}
			p++;
		}
		else
			*(q++)=*p;
	}		
	*(q++)='\0';
	return 0;
}

int ext_validate_keyword(const struct protocol_interface *protocol, cvsroot_t *root, const char *keyword, const char *value)
{
	if(!strcasecmp(keyword,"command"))
	{
		root->optional_1 = strdup(value);
		return CVSPROTO_SUCCESS;
	}
	return CVSPROTO_FAIL;
}

const char *ext_get_keyword_help(const struct protocol_interface *protocol)
{
	return "command\0Command to execute\0";
}

