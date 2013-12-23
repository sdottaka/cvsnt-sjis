/* CVS ntserver auth interface

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
#include <winsock.h>
#include <io.h>

#include "protocol_interface.h"
#include "common.h"
#include "../version.h"

static void ntserver_destroy(const struct protocol_interface *protocol);
static int ntserver_connect(const struct protocol_interface *protocol, int verify_only);
static int ntserver_disconnect(const struct protocol_interface *protocol);
static int ntserver_read_data(const struct protocol_interface *protocol, void *data, int length);
static int ntserver_write_data(const struct protocol_interface *protocol, const void *data, int length);
static int ntserver_flush_data(const struct protocol_interface *protocol);
static int ntserver_shutdown(const struct protocol_interface *protocol);
static int ntserver_impersonate(const struct protocol_interface *protocol, const char *username, void *user_handle);
static int handle_printf(char *fmt, ...);

struct protocol_interface ntserver_protocol_interface =
{
	PROTOCOL_INTERFACE_VERSION,

	"ntserver",
	"ntserver "CVSNT_PRODUCTVERSION_STRING,
	":ntserver[;keyword=value...]:host[:]/path",

	ntserver_destroy,

	elemHostname,	/* Required elements */
	elemHostname,	/* Valid elements */

	NULL, /* validate */
	ntserver_connect,
	ntserver_disconnect,
	NULL, /* login */
	NULL, /* logout */
	NULL, /* start_encryption */
	NULL, /* auth_protocol_connect */
	ntserver_read_data,
	ntserver_write_data,
	ntserver_flush_data,
	ntserver_shutdown,
	NULL, /* impersonate */
	NULL, /* validate_keyword */
	NULL /* get_keyword_help */
};

static HANDLE hPipe;

struct protocol_interface *get_protocol_interface(const struct server_interface *server)
{
	current_server = server;
	return &ntserver_protocol_interface;
}

void ntserver_destroy(const struct protocol_interface *protocol)
{
	free(protocol->auth_username);
	free(protocol->auth_repository);
}

int ntserver_connect(const struct protocol_interface *protocol, int verify_only)
{
	const char *port = current_server->current_root->port;
	const char *begin_request = "BEGIN NTSERVER";
	char tmp[_MAX_PATH];

	if(current_server->current_root->username || !current_server->current_root->hostname || current_server->current_root->port || !current_server->current_root->directory)
		return CVSPROTO_BADPARMS;


	snprintf(tmp,sizeof(tmp),"\\\\%s\\pipe\\CVS_PIPE",current_server->current_root->hostname);
	hPipe=CreateFile(tmp,GENERIC_READ|GENERIC_WRITE,0,NULL,OPEN_EXISTING,0,NULL);
	if(hPipe==INVALID_HANDLE_VALUE)
	{
		DWORD dwErr = GetLastError();

		server_error(1,"Couldn't connect to named pipe on remote machine: Error %d",dwErr);
	}

	if(handle_printf("%s\n%s\n",begin_request,current_server->current_root->directory)<0)
		return CVSPROTO_FAIL;
	return CVSPROTO_SUCCESS;
}

int ntserver_disconnect(const struct protocol_interface *protocol)
{
	CloseHandle(hPipe);
	return CVSPROTO_SUCCESS;
}

int ntserver_read_data(const struct protocol_interface *protocol, void *data, int length)
{
	DWORD dwRead = 0;
	if(!ReadFile(hPipe,data,length,&dwRead,NULL))
		return -1;
	return (int)dwRead;
}

int ntserver_write_data(const struct protocol_interface *protocol, const void *data, int length)
{
	DWORD dwWritten = 0;
	if(!WriteFile(hPipe,data,length,&dwWritten,NULL))
		return -1;
	return (int)dwWritten;
}

int ntserver_flush_data(const struct protocol_interface *protocol)
{
	FlushFileBuffers(hPipe);
	return 0;
}

int ntserver_shutdown(const struct protocol_interface *protocol)
{
	FlushFileBuffers(hPipe);
	return 0;
}

int handle_printf(char *fmt, ...)
{
	char temp[1024];
	va_list va;

	va_start(va,fmt);

	_vsnprintf(temp,sizeof(temp),fmt,va);

	va_end(va);

	return ntserver_write_data(&ntserver_protocol_interface,temp,strlen(temp));
}

