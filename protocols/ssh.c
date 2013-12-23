/* CVS ssh auth interface

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
#define VC_EXTRALEAN
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>
#endif
#include <time.h>

#include "protocol_interface.h"
#include "common.h"
#include "scramble.h"
#include "../version.h"

#include "..\plink\plink_cvsnt.h"

static int ssh_validate(const struct protocol_interface *protocol, cvsroot_t *newroot);
static void ssh_destroy(const struct protocol_interface *protocol);
static int ssh_connect(const struct protocol_interface *protocol, int verify_only);
static int ssh_disconnect(const struct protocol_interface *protocol);
static int ssh_login(const struct protocol_interface *protocol, char *password);
static int ssh_logout(const struct protocol_interface *protocol);
static int ssh_get_user_password(const char *username, const char *server, const char *port, const char *directory, char *password, int password_len);
static int ssh_set_user_password(const char *username, const char *server, const char *port, const char *directory, const char *password);
static int ssh_read_data(const struct protocol_interface *protocol, void *data, int length);
static int ssh_write_data(const struct protocol_interface *protocol, const void *data, int length);
static int ssh_flush_data(const struct protocol_interface *protocol);
static int ssh_shutdown(const struct protocol_interface *protocol);
static int ssh_validate_keyword(const struct protocol_interface *protocol, cvsroot_t *root, const char *keyword, const char *value);
static int ssh_wrap(const struct protocol_interface *protocol, int unwrap, int encrypt, const void *input, int size, void *output, int *newsize);
static const char *ssh_get_keyword_help(const struct protocol_interface *protocol);
static char *ssh_get_default_port();

struct protocol_interface ssh_protocol_interface =
{
	PROTOCOL_INTERFACE_VERSION,

	"ssh",
	"ssh "CVSNT_PRODUCTVERSION_STRING,
	":ssh[;keyword=value...]:[username[:password]@]host[:port][:]/path",

	ssh_destroy,

	elemHostname, /* Required elements */
	elemUsername|elemPassword|elemHostname|elemPort, /* Valid elements */

	ssh_validate, /* validate */
	ssh_connect,
	ssh_disconnect,
	ssh_login,
	ssh_logout,
	ssh_wrap, /* wrap */
	NULL, /* auth_protocol_connect */
	ssh_read_data,
	ssh_write_data,
	ssh_flush_data,
	ssh_shutdown,
	NULL, /* impersonate */
	ssh_validate_keyword,
	ssh_get_keyword_help,
	NULL, /* server_read_data */
	NULL, /* server_write_data */
	NULL, /* server_flush_data */
	NULL /* server_shutdown */
};

struct protocol_interface *get_protocol_interface(const struct server_interface *server)
{
	static putty_callbacks cb;
	current_server = server;
	cb.getpass = server->getpass;
	cb.yesno = server->yesno;
	plink_set_callbacks(&cb);
	return &ssh_protocol_interface;
}

void ssh_destroy(const struct protocol_interface *protocol)
{
	free(protocol->auth_username);
	free(protocol->auth_password);
	free(protocol->auth_repository);
}

int ssh_validate(const struct protocol_interface *protocol, cvsroot_t *newroot)
{
	if(!newroot->port)
	{
		/* Replace the default port used with the ssh port */
		newroot->port = ssh_get_default_port();
	}
	if(newroot->optional_2) /* Keyfile was specified */
	{
		free(newroot->password);
		newroot->password = strdup("");
	}
	return 0;
}

int ssh_connect(const struct protocol_interface *protocol, int verify_only)
{
	char crypt_password[4096],command_line[1024];
	const char *username;
	const char *password;
	char *key = NULL;
	const char *server;
	char *env;
	const char *version;

	if(!current_server->current_root->hostname || !current_server->current_root->directory)
		return CVSPROTO_BADPARMS;

	username=get_username(current_server->current_root);
	password = current_server->current_root->password;
	if(password && !*password)
		password=NULL;
	version = current_server->current_root->optional_1;
	if(!password) // No password supplied
	{
		if(ssh_get_user_password(username,current_server->current_root->hostname,current_server->current_root->port,current_server->current_root->directory,crypt_password,sizeof(crypt_password)))
		{
			/* In the case where we're using an ssh agent such as pageant, it's legal to have no password */
		}
		else
		{
			if(!strncmp(crypt_password,"KEY;",4))
			{
				key=strchr(crypt_password+4,';');
				if(!key || !*key)
				{
					/* Something wrong - ignore password */
					server_error(1,"No password or key set.  Try 'cvs login'\n"); 
				}
				*key++ = '\0';
				key=strchr(key,';');
				*key++ = '\0';
				if(!key || !*key)
				{
					/* Something wrong - ignore password */
					server_error(1,"No password or key set.  Try 'cvs login'\n"); 
				}
				version = crypt_password+4;			
			}
			else
			{
				pserver_decrypt_password(crypt_password, crypt_password, sizeof(crypt_password));
				password = crypt_password;
			}
		}
	}
	server=current_server->current_root->hostname;

	/* Execute correct command */
	if((env=getenv("CVS_SERVER"))!=NULL)
		strncpy(command_line,env,sizeof(command_line)-sizeof(" server"));
	else
		strcpy(command_line,"cvs");
	strcat(command_line," server");

	if(plink_connect(username, password, key, server, atoi(current_server->current_root->port), version?atoi(version):0, command_line))
	{
		server_error(0,"Couldn't connect to remote server - plink error");
		return CVSPROTO_FAIL;
	}

	return CVSPROTO_SUCCESS_NOPROTOCOL; /* :ssh: doesn't need login response */
}

int ssh_disconnect(const struct protocol_interface *protocol)
{
	return CVSPROTO_SUCCESS;
}

int ssh_login(const struct protocol_interface *protocol, char *password)
{
	char crypt_password[64];
	const char *username = NULL;

	username = get_username(current_server->current_root);

	if(!current_server->current_root->optional_2)
	{
		/* Store username & encrypted password in password store */
		pserver_crypt_password(password,crypt_password,sizeof(crypt_password));
		if(ssh_set_user_password(username,current_server->current_root->hostname,current_server->current_root->port,current_server->current_root->directory,crypt_password))
		{
			server_error(1,"Failed to store password");
		}
	}
	else
	{
		char buf[4096] = "KEY;";

		if(current_server->current_root->optional_1)
			strcat(buf,current_server->current_root->optional_1);
		else
			strcat(buf,"0");
		strcat(buf,";");

		strcat(buf,".;"); // Password field not used any more (use pageant for this kind of thing)

		strcat(buf,current_server->current_root->optional_2);

		if(ssh_set_user_password(username,current_server->current_root->hostname,current_server->current_root->port,current_server->current_root->directory,buf))
		{
			server_error(1,"Failed to store key");
		}
	}

	free(current_server->current_root->optional_2);
	current_server->current_root->optional_2=NULL;
	return CVSPROTO_SUCCESS;
}

int ssh_logout(const struct protocol_interface *protocol)
{
	const char *username = NULL;

	username = get_username(current_server->current_root);
	if(ssh_set_user_password(username,current_server->current_root->hostname,current_server->current_root->port,current_server->current_root->directory,NULL))
	{
		server_error(1,"Failed to delete password");
	}
	return CVSPROTO_SUCCESS;
}

int ssh_get_user_password(const char *username, const char *server, const char *port, const char *directory, char *password, int password_len)
{
	char tmp[1024];

	if(port)
		snprintf(tmp,sizeof(tmp),":ssh:%s@%s:%s:%s",username,server,port,directory);
	else
		snprintf(tmp,sizeof(tmp),":ssh:%s@%s:%s",username,server,directory);
	if(!get_user_local_config_data("cvspass",tmp,password,password_len))
		return CVSPROTO_SUCCESS;
	else
		return CVSPROTO_FAIL;
}

int ssh_set_user_password(const char *username, const char *server, const char *port, const char *directory, const char *password)
{
	char tmp[1024];

	if(port)
		snprintf(tmp,sizeof(tmp),":ssh:%s@%s:%s:%s",username,server,port,directory);
	else
		snprintf(tmp,sizeof(tmp),":ssh:%s@%s:%s",username,server,directory);
	if(!set_user_local_config_data("cvspass",tmp,password))
		return CVSPROTO_SUCCESS;
	else
		return CVSPROTO_FAIL;
}

int ssh_read_data(const struct protocol_interface *protocol, void *data, int length)
{
	return plink_read_data(data, length);
}

int ssh_write_data(const struct protocol_interface *protocol, const void *data, int length)
{
	return plink_write_data(data, length);
}

int ssh_flush_data(const struct protocol_interface *protocol)
{
	return 0;
}

int ssh_shutdown(const struct protocol_interface *protocol)
{
	return 0;
}

const char *ssh_get_keyword_help(const struct protocol_interface *protocol)
{
	return "privatekey\0Use file as private key (aliases: key, rsakey)\0version\0Force SSH version (alias: ver)\0";
}

int ssh_validate_keyword(const struct protocol_interface *protocol, cvsroot_t *root, const char *keyword, const char *value)
{
	if(!strcasecmp(keyword,"version") || !strcasecmp(keyword,"ver"))
	{
		if(!strcmp(value,"1") || !strcmp(value,"2"))
		{
			root->optional_1 = strdup(value);
			return CVSPROTO_SUCCESS;
		}
		else
			server_error(1,"SSH version should be 1 or 2");
	}
	if(!strcasecmp(keyword,"passphrase")) // Synonym for 'password'
	{
		root->password = strdup(value);
	}
	if(!strcasecmp(keyword,"privatekey") || !strcasecmp(keyword,"key") || !strcasecmp(keyword,"rsakey"))
	{
		root->optional_2 = strdup(value);
		return CVSPROTO_SUCCESS;
	}
	return CVSPROTO_FAIL;
}

/* This is a dummy - ssh is always encrypted...  it just allows us to specify the '-x' flag
   and to connect to servers that require encrypted connections */
int ssh_wrap(const struct protocol_interface *protocol, int unwrap, int encrypt, const void *input, int size, void *output, int *newsize)
{
	memcpy(output,input,size);
	*newsize=size;
	return 0;
}

char *ssh_get_default_port()
{
	struct servent *ent;
	char p[32];

	if((ent=getservbyname("ssh","tcp"))!=NULL)
	{
		sprintf(p,"%u",ntohs(ent->s_port));
		return strdup(p);
	}

	return strdup("22");
}

