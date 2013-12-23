/* CVS server (rsh) auth interface

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
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <io.h>
#else
#include <pwd.h>
#include <unistd.h>
#endif

#include "protocol_interface.h"
#include "common.h"
#include "../version.h"

static void server_destroy(const struct protocol_interface *protocol);
static int server_connect(const struct protocol_interface *protocol, int verify_only);
static int server_disconnect(const struct protocol_interface *protocol);
static int server_read_data(const struct protocol_interface *protocol, void *data, int length);
static int server_write_data(const struct protocol_interface *protocol, const void *data, int length);
static int server_flush_data(const struct protocol_interface *protocol);
static int server_shutdown(const struct protocol_interface *protocol);

struct protocol_interface server_protocol_interface =
{
	PROTOCOL_INTERFACE_VERSION,

	"server",
	"server "CVSNT_PRODUCTVERSION_STRING,
	":server[;keyword=value...]:[username[:password]@]host[:]/path",

	server_destroy,
	
	elemHostname, /* Required elements */
	elemUsername|elemPassword|elemHostname, /* Valid elements */

	NULL, /* validate */
	server_connect,
	server_disconnect,
	NULL, /* login */
	NULL, /* logout */
	NULL, /* start_encryption */
	NULL, /* auth_protocol_connect */
	server_read_data,
	server_write_data,
	server_flush_data,
	server_shutdown,
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
	return &server_protocol_interface;
}

void server_destroy(const struct protocol_interface *protocol)
{
	free(protocol->auth_username);
	free(protocol->auth_password);
	free(protocol->auth_repository);
}

int server_connect(const struct protocol_interface *protocol, int verify_only)
{
	char current_user[256];
	char remote_user[256];
	char tmp[32];
	unsigned char c;
	int listen_port=0;
	if(!current_server->current_root->hostname || !current_server->current_root->directory || current_server->current_root->port)
		return CVSPROTO_BADPARMS;

	if(tcp_connect_bind(current_server->current_root->hostname,"514",512,1023)<1)
		return CVSPROTO_FAIL;

#ifdef _WIN32
	{
		DWORD dwLen = 256;
		GetUserNameA(current_user,&dwLen);
	}
#else
	strncpy(current_user,getpwuid(geteuid())->pw_name,sizeof(current_user));
#endif

	if(current_server->current_root->username)
		strncpy(remote_user,current_server->current_root->username,sizeof(remote_user));
	else
		strncpy(remote_user,current_user,sizeof(remote_user));

	snprintf(tmp,sizeof(tmp),"%d",listen_port);
	if(tcp_write(tmp,strlen(tmp)+1)<1)
		return CVSPROTO_FAIL;
	if(tcp_write(current_user,strlen(current_user)+1)<1)
		return CVSPROTO_FAIL;
	if(tcp_write(remote_user,strlen(remote_user)+1)<1)
		return CVSPROTO_FAIL;

#define CMD "cvs server"

	if(tcp_write(CMD,sizeof(CMD))<1)
		return CVSPROTO_FAIL;
	
	if(tcp_read(&c,1)<1)
		return CVSPROTO_FAIL;

	if(c)
	{
		char msg[257];
		if((c=tcp_read(msg,256))<1)
			return CVSPROTO_FAIL;
		msg[c]='\0';
		server_error(0,"rsh server reported: %s",msg);
		return CVSPROTO_FAIL;
	}

	return CVSPROTO_SUCCESS_NOPROTOCOL; /* :server: doesn't need login response */
}

int server_disconnect(const struct protocol_interface *protocol)
{
	tcp_disconnect();
	return CVSPROTO_SUCCESS;
}

int server_read_data(const struct protocol_interface *protocol, void *data, int length)
{
	return tcp_read(data,length);
}

int server_write_data(const struct protocol_interface *protocol, const void *data, int length)
{
	return tcp_write(data,length);
}

int server_flush_data(const struct protocol_interface *protocol)
{
	return 0;
}

int server_shutdown(const struct protocol_interface *protocol)
{
	tcp_shutdown();
	return 0;
}

