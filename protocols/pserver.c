/* CVS pserver auth interface

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
#include <winsock.h>
#else
#include <netdb.h>
#include <pwd.h>
#include <unistd.h>
#endif

#ifndef MAX_PATH
#define MAX_PATH 1024
#endif

#include "protocol_interface.h"
#include "common.h"
#include "scramble.h"
#include "../version.h"

static void pserver_destroy(const struct protocol_interface *protocol);
static int pserver_connect(const struct protocol_interface *protocol, int verify_only);
static int pserver_disconnect(const struct protocol_interface *protocol);
static int pserver_login(const struct protocol_interface *protocol, char *password);
static int pserver_logout(const struct protocol_interface *protocol);
static int pserver_auth_protocol_connect(const struct protocol_interface *protocol, const char *auth_string);
static int pserver_get_user_password(const char *username, const char *server, const char *port, const char *directory, char *password, int password_len);
static int pserver_set_user_password(const char *username, const char *server, const char *port, const char *directory, const char *password);
static int pserver_read_data(const struct protocol_interface *protocol, void *data, int length);
static int pserver_write_data(const struct protocol_interface *protocol, const void *data, int length);
static int pserver_flush_data(const struct protocol_interface *protocol);
static int pserver_shutdown(const struct protocol_interface *protocol);

struct protocol_interface pserver_protocol_interface =
{
	PROTOCOL_INTERFACE_VERSION,

	"pserver",
	"pserver " CVSNT_PRODUCTVERSION_STRING,
	":pserver[;keyword=value...]:[username[:password]@]host[:port][:]/path",

	pserver_destroy,

	elemHostname, /* Required elements */
	elemUsername|elemPassword|elemHostname|elemPort|elemTunnel, /* Valid elements */

	NULL, /* validate */
	pserver_connect,
	pserver_disconnect,
	pserver_login,
	pserver_logout,
	NULL, /* start_encryption */
	pserver_auth_protocol_connect,
	pserver_read_data,
	pserver_write_data,
	pserver_flush_data,
	pserver_shutdown,
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
	return &pserver_protocol_interface;
}

void pserver_destroy(const struct protocol_interface *protocol)
{
	free(protocol->auth_username);
	free(protocol->auth_password);
	free(protocol->auth_repository);
}

int pserver_connect(const struct protocol_interface *protocol, int verify_only)
{
	char crypt_password[64];
	const char *begin_request = "BEGIN AUTH REQUEST";
	const char *end_request = "END AUTH REQUEST";
	const char *username = NULL;

	username = get_username(current_server->current_root);

	if(!username || !current_server->current_root->hostname || !current_server->current_root->directory)
		return CVSPROTO_BADPARMS;

	if(tcp_connect(current_server->current_root))
		return CVSPROTO_FAIL;
	if(current_server->current_root->password)
	{
		pserver_crypt_password(current_server->current_root->password,crypt_password,sizeof(crypt_password));
	}
	else
	{
		if(pserver_get_user_password(username,current_server->current_root->hostname,current_server->current_root->port,current_server->current_root->directory,crypt_password,sizeof(crypt_password)))
		{
			/* Using null password - trace something out here */
			server_error(0,"Empty password used - try 'cvs login' with a real password\n"); 
			pserver_crypt_password("",crypt_password,sizeof(crypt_password));
		}
	}

	if(verify_only)
	{
		begin_request = "BEGIN VERIFICATION REQUEST";
		end_request = "END VERIFICATION REQUEST";
	}

	if(tcp_printf("%s\n%s\n%s\n%s\n%s\n",begin_request,current_server->current_root->directory,username,crypt_password,end_request)<0)
		return CVSPROTO_FAIL;
	return CVSPROTO_SUCCESS;
}

int pserver_disconnect(const struct protocol_interface *protocol)
{
	if(tcp_disconnect())
		return CVSPROTO_FAIL;
	return CVSPROTO_SUCCESS;
}

int pserver_login(const struct protocol_interface *protocol, char *password)
{
	char crypt_password[64];
	const char *username = NULL;

	username = get_username(current_server->current_root);

	/* Store username & encrypted password in password store */
	pserver_crypt_password(password,crypt_password,sizeof(crypt_password));
	if(pserver_set_user_password(username,current_server->current_root->hostname,current_server->current_root->port,current_server->current_root->directory,crypt_password))
	{
		server_error(1,"Failed to store password");
	}

	return CVSPROTO_SUCCESS;
}

int pserver_logout(const struct protocol_interface *protocol)
{
	const char *username = NULL;

	username = get_username(current_server->current_root);
	if(pserver_set_user_password(username,current_server->current_root->hostname,current_server->current_root->port,current_server->current_root->directory,NULL))
	{
		server_error(1,"Failed to delete password");
	}
	return CVSPROTO_SUCCESS;
}

int pserver_auth_protocol_connect(const struct protocol_interface *protocol, const char *auth_string)
{
	char *tmp;

    /* The Authentication Protocol.  Client sends:
     *
     *   BEGIN AUTH REQUEST\n
     *   <REPOSITORY>\n
     *   <USERNAME>\n
     *   <PASSWORD>\n
     *   END AUTH REQUEST\n
     *
     * Server uses above information to authenticate, then sends
     *
     *   I LOVE YOU\n
     *
     * if it grants access, else
     *
     *   I HATE YOU\n
     *
     * if it denies access (and it exits if denying).
     *
     * When the client is "cvs login", the user does not desire actual
     * repository access, but would like to confirm the password with
     * the server.  In this case, the start and stop strings are
     *
     *   BEGIN VERIFICATION REQUEST\n
     *
     *            and
     *
     *   END VERIFICATION REQUEST\n
     *
     * On a verification request, the server's responses are the same
     * (with the obvious semantics), but it exits immediately after
     * sending the response in both cases.
     *
     * Why is the repository sent?  Well, note that the actual
     * client/server protocol can't start up until authentication is
     * successful.  But in order to perform authentication, the server
     * needs to look up the password in the special CVS passwd file,
     * before trying /etc/passwd.  So the client transmits the
     * repository as part of the "authentication protocol".  The
     * repository will be redundantly retransmitted later, but that's no
     * big deal.
     */

    if (!strcmp (auth_string, "BEGIN VERIFICATION REQUEST"))
		pserver_protocol_interface.verify_only = 1;
    else if (!strcmp (auth_string, "BEGIN AUTH REQUEST"))
		pserver_protocol_interface.verify_only = 0;
	else
		return CVSPROTO_NOTME;

    /* Get the three important pieces of information in order. */
    /* See above comment about error handling.  */
    server_getline (protocol, &pserver_protocol_interface.auth_repository, MAX_PATH);
    server_getline (protocol, &pserver_protocol_interface.auth_username, MAX_PATH);
    server_getline (protocol, &pserver_protocol_interface.auth_password, MAX_PATH);

    /* ... and make sure the protocol ends on the right foot. */
    /* See above comment about error handling.  */
    server_getline(protocol, &tmp, MAX_PATH);
    if (strcmp (tmp,
		pserver_protocol_interface.verify_only ?
		"END VERIFICATION REQUEST" : "END AUTH REQUEST")
	!= 0)
    {
		server_error (1, "bad auth protocol end: %s", tmp);
		free(tmp);
    }

	pserver_decrypt_password(pserver_protocol_interface.auth_password, tmp, MAX_PATH);
	strcpy(pserver_protocol_interface.auth_password, tmp);

	free(tmp);
	return CVSPROTO_SUCCESS;
}

int pserver_get_user_password(const char *username, const char *server, const char *port, const char *directory, char *password, int password_len)
{
	char tmp[1024];

	if(port)
		snprintf(tmp,sizeof(tmp),":pserver:%s@%s:%s:%s",username,server,port,directory);
	else
		snprintf(tmp,sizeof(tmp),":pserver:%s@%s:%s",username,server,directory);
	if(!get_user_local_config_data("cvspass",tmp,password,password_len))
		return CVSPROTO_SUCCESS;
	else
		return CVSPROTO_FAIL;
}

int pserver_set_user_password(const char *username, const char *server, const char *port, const char *directory, const char *password)
{
	char tmp[1024];

	if(port)
		snprintf(tmp,sizeof(tmp),":pserver:%s@%s:%s:%s",username,server,port,directory);
	else
		snprintf(tmp,sizeof(tmp),":pserver:%s@%s:%s",username,server,directory);
	if(!set_user_local_config_data("cvspass",tmp,password))
		return CVSPROTO_SUCCESS;
	else
		return CVSPROTO_FAIL;
}

int pserver_read_data(const struct protocol_interface *protocol, void *data, int length)
{
	return tcp_read(data,length);
}

int pserver_write_data(const struct protocol_interface *protocol, const void *data, int length)
{
	return tcp_write(data,length);
}

int pserver_flush_data(const struct protocol_interface *protocol)
{
	return 0; // TCP/IP is always flushed
}

int pserver_shutdown(const struct protocol_interface *protocol)
{
	return tcp_shutdown();
}

