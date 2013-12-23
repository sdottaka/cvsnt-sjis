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

static void fork_destroy(const struct protocol_interface *protocol);
static int fork_connect(const struct protocol_interface *protocol, int verify_only);
static int fork_disconnect(const struct protocol_interface *protocol);
static int fork_read_data(const struct protocol_interface *protocol, void *data, int length);
static int fork_write_data(const struct protocol_interface *protocol, const void *data, int length);
static int fork_flush_data(const struct protocol_interface *protocol);
static int fork_shutdown(const struct protocol_interface *protocol);

static int current_in=-1, current_out=-1;

struct protocol_interface fork_protocol_interface =
{
	PROTOCOL_INTERFACE_VERSION,

	"fork",
	"fork "CVSNT_PRODUCTVERSION_STRING,
	":fork[;keyword=value...]:/path",

	fork_destroy,

	elemNone,		/* Required elements */
	elemNone,		/* Valid elements */

	NULL, /* validate */
	fork_connect,
	fork_disconnect,
	NULL, /* login */
	NULL, /* logout */
	NULL, /* start_encryption */
	NULL, /* auth_protocol_connect */
	fork_read_data,
	fork_write_data,
	fork_flush_data,
	fork_shutdown,
	NULL, /* impersonate */
	NULL, /* validate_keyword */
	NULL /* get_keyword_help */
};

struct protocol_interface *get_protocol_interface(const struct server_interface *server)
{
	current_server = server;
	return &fork_protocol_interface;
}

void fork_destroy(const struct protocol_interface *protocol)
{
	free(protocol->auth_username);
	free(protocol->auth_password);
	free(protocol->auth_repository);
}

int fork_connect(const struct protocol_interface *protocol, int verify_only)
{
	char command_line[256];

	if(current_server->current_root->username || current_server->current_root->hostname || !current_server->current_root->directory || current_server->current_root->port)
		return CVSPROTO_BADPARMS;

	snprintf(command_line,sizeof(command_line),"%s server",current_server->cvs_command);

	if(run_command(command_line,&current_in, &current_out, NULL))
		return CVSPROTO_FAIL;

	return CVSPROTO_SUCCESS_NOPROTOCOL; /* :fork: doesn't need login response */
}

int fork_disconnect(const struct protocol_interface *protocol)
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

int fork_read_data(const struct protocol_interface *protocol, void *data, int length)
{
	int res = read(current_out,data,length);
	return res;
}

int fork_write_data(const struct protocol_interface *protocol, const void *data, int length)
{
	int res = write(current_in,data,length);
	return res;
}

int fork_flush_data(const struct protocol_interface *protocol)
{
	return 0;
}

int fork_shutdown(const struct protocol_interface *protocol)
{
	return 0;
}

