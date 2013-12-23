/* CVS sspi auth interface - unix (client only)

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

#include "protocol_interface.h"
#include "common.h"
#include "scramble.h"
#include "ntlm/ntlm.h"
#include "../version.h"

static void sspi_destroy(const struct protocol_interface *protocol);
static int sspi_connect(const struct protocol_interface *protocol, int verify_only);
static int sspi_disconnect(const struct protocol_interface *protocol);
static int sspi_login(const struct protocol_interface *protocol, char *password);
static int sspi_logout(const struct protocol_interface *protocol);
static int sspi_read_data(const struct protocol_interface *protocol, void *data, int length);
static int sspi_write_data(const struct protocol_interface *protocol, const void *data, int length);
static int sspi_flush_data(const struct protocol_interface *protocol);
static int sspi_shutdown(const struct protocol_interface *protocol);
static int ClientAuthenticate(const char *protocol, const char *name, const char *pwd, const char *domain);
static int sspi_get_user_password(const char *username, const char *server, const char *port, const char *directory, char *password, int password_len);
static int sspi_set_user_password(const char *username, const char *server, const char *port, const char *directory, const char *password);

struct protocol_interface sspi_protocol_interface =
{
	PROTOCOL_INTERFACE_VERSION,

	"sspi",
	"sspi "CVSNT_PRODUCTVERSION_STRING,
	":sspi[;keyword=value...]:[username[:password]@]host[:port][:]/path",

	sspi_destroy,

	elemHostname, /* Required elements */
	elemUsername|elemPassword|elemHostname|elemPort|elemTunnel, /* Valid elements */

	NULL, /* validate */
	sspi_connect,
	sspi_disconnect,
	sspi_login,
	sspi_logout,
	NULL, /* wrap */
	NULL, /* auth_protocol_connect */
	sspi_read_data,
	sspi_write_data,
	sspi_flush_data,
	sspi_shutdown,
	NULL, /* impersonate */
	NULL, /* validate_keyword */
	NULL, /* get_keyword_help */
	NULL, /* server_read_data */
	NULL, /* server_write_data */
	NULL, /* server_flush_data */
	NULL /* server_shutdown */
};

struct protocol_interface *get_protocol_interface(const struct server_interface *server)
{
	current_server = server;

	return &sspi_protocol_interface;
}

void sspi_destroy(const struct protocol_interface *protocol)
{
	free(protocol->auth_username);
	free(protocol->auth_repository);
}

int sspi_connect(const struct protocol_interface *protocol, int verify_only)
{
	char crypt_password[64],real_password[64],*password;
	char domain_buffer[128],*domain;
	char user_buffer[128],*p;
	const char *user;
	const char *begin_request = "BEGIN SSPI";
	char protocols[1024];
	const char *proto;

	if(!current_server->current_root->hostname || !current_server->current_root->directory)
		return CVSPROTO_BADPARMS;

	if(tcp_connect(current_server->current_root))
		return CVSPROTO_FAIL;

	user = get_username(current_server->current_root);
	password = current_server->current_root->password;
	domain = NULL;

	if(!current_server->current_root->password)
	{
		if(!sspi_get_user_password(user,current_server->current_root->hostname,current_server->current_root->port,current_server->current_root->directory,crypt_password,sizeof(crypt_password)))
		{
			pserver_decrypt_password(crypt_password, real_password, sizeof(real_password));
			password = real_password;
		}
	}

	if(strchr(user,'\\'))
	{
		strncpy(domain_buffer,user,sizeof(domain_buffer));
		domain_buffer[sizeof(domain_buffer)-1]='\0';
		domain=strchr(domain_buffer,'\\');
		if(domain)
		{
			*domain = '\0';
			strncpy(user_buffer,domain+1,sizeof(user_buffer));
			domain = domain_buffer;
			user = user_buffer;
		}
	}

	if(tcp_printf("%s\nNTLM\n",begin_request)<0)
		return CVSPROTO_FAIL;

	tcp_readline(protocols, sizeof(protocols));
	if((p=strstr(protocols,"[server aborted"))!=NULL)
	{
		server_error(1, p);
	}

	if(strstr(protocols,"NTLM"))
		proto="NTLM";
	else
	    server_error(1, "Can't authenticate - server and client cannot agree on an authentication scheme (got '%s')",protocols);

	if(!ClientAuthenticate(proto,user,password,domain))
	{
		/* Actually we never get here... NTLM seems to allow the client to
		   authenticate then fails at the server end.  Wierd huh? */

		if(user)
			server_error(1, "Can't authenticate - Username, Password or Domain is incorrect");
		else
			server_error(1, "Can't authenticate - perhaps you need to login first?");

		return CVSPROTO_FAIL;
	}

	if(tcp_printf("%s\n",current_server->current_root->directory)<0)
		return CVSPROTO_FAIL;
	return CVSPROTO_SUCCESS;
}

int sspi_disconnect(const struct protocol_interface *protocol)
{
	if(tcp_disconnect())
		return CVSPROTO_FAIL;
	return CVSPROTO_SUCCESS;
}

int sspi_login(const struct protocol_interface *protocol, char *password)
{
	char crypt_password[64];
	char *user = get_username(current_server->current_root);
	
	/* Store username & encrypted password in password store */
	pserver_crypt_password(password,crypt_password,sizeof(crypt_password));
	if(sspi_set_user_password(user,current_server->current_root->hostname,current_server->current_root->port,current_server->current_root->directory,crypt_password))
	{
		server_error(1,"Failed to store password");
	}

	return CVSPROTO_SUCCESS;
}

int sspi_logout(const struct protocol_interface *protocol)
{
	char *user = get_username(current_server->current_root);
	
	if(sspi_set_user_password(user,current_server->current_root->hostname,current_server->current_root->port,current_server->current_root->directory,NULL))
	{
		server_error(1,"Failed to delete password");
	}
	return CVSPROTO_SUCCESS;
}

int sspi_read_data(const struct protocol_interface *protocol, void *data, int length)
{
	return tcp_read(data,length);
}

int sspi_write_data(const struct protocol_interface *protocol, const void *data, int length)
{
	return tcp_write(data,length);
}

int sspi_flush_data(const struct protocol_interface *protocol)
{
	return 0; // TCP/IP is always flushed
}

int sspi_shutdown(const struct protocol_interface *protocol)
{
	return tcp_shutdown();
}

int ClientAuthenticate(const char *protocol, const char *name, const char *pwd, const char *domain)
{
	tSmbNtlmAuthRequest request;
	tSmbNtlmAuthChallenge challenge;
	tSmbNtlmAuthResponse response;
	short len;
	
	buildSmbNtlmAuthRequest(&request,name?(char*)name:"",domain?(char*)domain:"");
	len=htons(SmbLength(&request));
	if(tcp_write(&len,sizeof(len))<0)
	  return 0;
	if(tcp_write(&request,SmbLength(&request))<0)
	  return 0;
	if(tcp_read(&len,2)!=2)
	  return 0;
	if(tcp_read(&challenge,ntohs(len))!=ntohs(len))
	  return 0;
	buildSmbNtlmAuthResponse(&challenge, &response, name?(char*)name:"", pwd?(char*)pwd:"");
	len=htons(SmbLength(&response));
	if(tcp_write(&len,sizeof(len))<0)
	  return 0;
	if(tcp_write(&response, SmbLength(&response))<0)
	  return 0;
	return 1;
}

int sspi_get_user_password(const char *username, const char *server, const char *port, const char *directory, char *password, int password_len)
{
	char tmp[1024];

	if(port)
		snprintf(tmp,sizeof(tmp),":sspi:%s@%s:%s:%s",username,server,port,directory);
	else
		snprintf(tmp,sizeof(tmp),":sspi:%s@%s:%s",username,server,directory);
	if(!get_user_local_config_data("cvspass",tmp,password,password_len))
		return CVSPROTO_SUCCESS;
	else
		return CVSPROTO_FAIL;
}

int sspi_set_user_password(const char *username, const char *server, const char *port, const char *directory, const char *password)
{
	char tmp[1024];

	if(port)
		snprintf(tmp,sizeof(tmp),":sspi:%s@%s:%s:%s",username,server,port,directory);
	else
		snprintf(tmp,sizeof(tmp),":sspi:%s@%s:%s",username,server,directory);
	if(!set_user_local_config_data("cvspass",tmp,password))
		return CVSPROTO_SUCCESS;
	else
		return CVSPROTO_FAIL;
}

