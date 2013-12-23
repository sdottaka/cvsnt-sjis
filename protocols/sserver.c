/* CVS sserver auth interface

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
#include <io.h>
#else
#include <netdb.h>
#include <pwd.h>
#include <unistd.h>
#endif

/* Requires openssl installed */
#include <openssl/ssl.h>
#include <openssl/err.h>

#ifndef MAX_PATH
#define MAX_PATH 1024
#endif

#include "protocol_interface.h"
#include "common.h"
#include "scramble.h"
#include "../version.h"

static void sserver_destroy(const struct protocol_interface *protocol);
static int sserver_connect(const struct protocol_interface *protocol, int verify_only);
static int sserver_disconnect(const struct protocol_interface *protocol);
static int sserver_login(const struct protocol_interface *protocol, char *password);
static int sserver_logout(const struct protocol_interface *protocol);
static int sserver_auth_protocol_connect(const struct protocol_interface *protocol, const char *auth_string);
static int sserver_get_user_password(const char *username, const char *server, const char *port, const char *directory, char *password, int password_len);
static int sserver_set_user_password(const char *username, const char *server, const char *port, const char *directory, const char *password);
static int sserver_read_data(const struct protocol_interface *protocol, void *data, int length);
static int sserver_write_data(const struct protocol_interface *protocol, const void *data, int length);
static int sserver_flush_data(const struct protocol_interface *protocol);
static int sserver_shutdown(const struct protocol_interface *protocol);
static int sserver_wrap(const struct protocol_interface *protocol, int unwrap, int encrypt, const void *input, int size, void *output, int *newsize);
static int sserver_validate_keyword(const struct protocol_interface *protocol, cvsroot_t *root, const char *keyword, const char *value);
static const char *sserver_get_keyword_help(const struct protocol_interface *protocol);


static int sserver_printf(char *fmt, ...);
static void sserver_error(const char *txt, int err);

static SSL *ssl;
static SSL_CTX *ctx;

#define SSERVER_INIT_STRING "SSERVER 1.0 READY\n"

struct protocol_interface sserver_protocol_interface =
{
	PROTOCOL_INTERFACE_VERSION,

	"sserver",
	"sserver "CVSNT_PRODUCTVERSION_STRING,
	":sserver[;keyword=value...]:[username[:password]@]host[:port][:]/path",

	sserver_destroy,

	elemHostname, /* Required elements */
	elemUsername|elemPassword|elemHostname|elemPort|elemTunnel, /* Valid elements */

	NULL, /* validate */
	sserver_connect,
	sserver_disconnect,
	sserver_login,
	sserver_logout,
	sserver_wrap,
	sserver_auth_protocol_connect,
	sserver_read_data,
	sserver_write_data,
	sserver_flush_data,
	sserver_shutdown,
	NULL, /* impersonate */
	sserver_validate_keyword, 
	sserver_get_keyword_help,
	sserver_read_data,
	sserver_write_data,
	sserver_flush_data,
	sserver_shutdown,
	NULL /* negotiate encryption */
};

struct protocol_interface *get_protocol_interface(const struct server_interface *server)
{
	current_server = server;
	return &sserver_protocol_interface;
}

void sserver_destroy(const struct protocol_interface *protocol)
{
	free(protocol->auth_username);
	free(protocol->auth_password);
	free(protocol->auth_repository);

	if(ssl)
	{
	  SSL_free (ssl);
      	  SSL_CTX_free (ctx);
	  ssl=NULL;
	  ctx=NULL;
	}
}

int sserver_connect(const struct protocol_interface *protocol, int verify_only)
{
	char crypt_password[64];
	char server_version[128];
	const char *begin_request = "BEGIN SSL AUTH REQUEST";
	const char *end_request = "END SSL AUTH REQUEST";
	const char *username = NULL;
	int err,l;
	int sserver_version = 0;
	char certs[4096];
	int strict = 0;

	snprintf(certs,sizeof(certs),"%sca.pem",current_server->library_dir);

	if(current_server->current_root->optional_1)
	{
	  sserver_version = atoi(current_server->current_root->optional_1);
	  if(sserver_version != 0 && sserver_version != 1)
	  {
	    server_error(0,"version must be one of:");
	    server_error(0,"0 - All CVSNT-type servers");
	    server_error(0,"1 - Unix server using Corey Minards' sserver patches");
	    server_error(1,"Please specify a valid value");
	  }
	}

	if(!get_user_local_config_data("sserver","StrictChecking",server_version,sizeof(server_version)))
	{
	  strict = atoi(server_version);
	}
	
	if(current_server->current_root->optional_2)
		strict = atoi(current_server->current_root->optional_2);

	if(sserver_version == 1) /* Old sserver */
	{
	  begin_request = verify_only?"BEGIN SSL VERIFICATION REQUEST":"BEGIN SSL REQUEST";
	  end_request = verify_only?"END SSL VERIFICATION REQUEST":"END SSL REQUEST";
	}
	else if(verify_only)
	{
		begin_request = "BEGIN SSL VERIFICATION REQUEST";
		end_request = "END SSL VERIFICATION REQUEST";
	}
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
		if(sserver_get_user_password(username,current_server->current_root->hostname,current_server->current_root->port,current_server->current_root->directory,crypt_password,sizeof(crypt_password)))
		{
			/* Using null password - trace something out here */
			server_error(0,"Empty password used - try 'cvs login' with a real password\n"); 
			pserver_crypt_password("",crypt_password,sizeof(crypt_password));
		}
	}


	if(sserver_version == 0) /* Pre-CVSNT had no version check */
	{
	  if(tcp_printf("%s\n",begin_request)<0)
		return CVSPROTO_FAIL;
	  for(;;)
	  {
		*server_version='\0';
		if((l=tcp_readline(server_version,sizeof(server_version))<0))
			return CVSPROTO_FAIL;
		if(*server_version)
			break;
		usleep(100);
	  }
	}

	SSL_library_init();
	SSL_load_error_strings ();
	if(sserver_version == 0)
	  ctx = SSL_CTX_new (SSLv23_client_method ());
	else
	  ctx = SSL_CTX_new (SSLv3_client_method ()); /* Corey minyards' version forces v3 (in fact it locks up if you send it a v2 greeting! */
	SSL_CTX_load_verify_locations(ctx,certs,NULL);
	SSL_CTX_set_verify(ctx,SSL_VERIFY_NONE,NULL); /* Check verify result below */
    ssl = SSL_new (ctx);

	SSL_set_fd (ssl, get_tcp_fd());
    if((err=SSL_connect (ssl))<1)
	{
		sserver_error("SSL conneciton failed", err);
		return CVSPROTO_FAIL;
	}

	switch(err=SSL_get_verify_result(ssl))
	{
		case X509_V_OK:
		/* Valid certificate */
		break;
		case X509_V_ERR_DEPTH_ZERO_SELF_SIGNED_CERT:
		/* Self signed certificate */
		if(!strict)
			break;
		/* Fall through */
		default:
			server_error(1,"Server certificate verification failed (error %d)",err);
	}
	
	{
		X509 *cert;
		char buf[1024];

		if(!(cert=SSL_get_peer_certificate(ssl)))
			server_error(1,"Server did not present a valid certificate");
		
		buf[0]='\0';
		if(strict)
		{
		  X509_NAME_get_text_by_NID(X509_get_subject_name(cert), NID_commonName, buf, sizeof(buf));
		  if(strcasecmp(buf,current_server->current_root->hostname))
			server_error(1, "Certificate CommonName '%s' does not match server name '%s'",buf,current_server->current_root->hostname);
		}
	}

    if(sserver_version == 1)
	{
	  if(sserver_printf("%s\n",begin_request)<0)
		return CVSPROTO_FAIL;
	}

	if(sserver_printf("%s\n%s\n%s\n%s\n",current_server->current_root->directory,username,crypt_password,end_request)<0)
		return CVSPROTO_FAIL;
	return CVSPROTO_SUCCESS;
}

int sserver_disconnect(const struct protocol_interface *protocol)
{
	if(tcp_disconnect())
		return CVSPROTO_FAIL;
	return CVSPROTO_SUCCESS;
}

int sserver_login(const struct protocol_interface *protocol, char *password)
{
	char crypt_password[64];
	const char *username = NULL;

	username = get_username(current_server->current_root);

	/* Store username & encrypted password in password store */
	pserver_crypt_password(password,crypt_password,sizeof(crypt_password));
	if(sserver_set_user_password(username,current_server->current_root->hostname,current_server->current_root->port,current_server->current_root->directory,crypt_password))
	{
		server_error(1,"Failed to store password");
	}

	return CVSPROTO_SUCCESS;
}

int sserver_logout(const struct protocol_interface *protocol)
{
	const char *username = NULL;

	username = get_username(current_server->current_root);
	if(sserver_set_user_password(username,current_server->current_root->hostname,current_server->current_root->port,current_server->current_root->directory,NULL))
	{
		server_error(1,"Failed to delete password");
	}
	return CVSPROTO_SUCCESS;
}

int sserver_auth_protocol_connect(const struct protocol_interface *protocol, const char *auth_string)
{
	char *tmp;
	int err;
	char certfile[1024];
	char keyfile[1024];
	char certs[4096];

	snprintf(certs,sizeof(certs),"%sca.pem",current_server->library_dir);

    if (!strcmp (auth_string, "BEGIN SSL VERIFICATION REQUEST"))
		sserver_protocol_interface.verify_only = 1;
    else if (!strcmp (auth_string, "BEGIN SSL AUTH REQUEST"))
		sserver_protocol_interface.verify_only = 0;
	else
		return CVSPROTO_NOTME;

	write(1,SSERVER_INIT_STRING,sizeof(SSERVER_INIT_STRING)-1);

	if(current_server->get_global_config_data(current_server,"PServer","CertificateFile",certfile,sizeof(certfile)))
		server_error(1,"Couldn't get certificate file");
	if(current_server->get_global_config_data(current_server,"PServer","PrivateKeyFile",keyfile,sizeof(keyfile)))
		strncpy(keyfile,certfile,sizeof(keyfile));

	SSL_library_init();
	SSL_load_error_strings ();
	ctx = SSL_CTX_new (SSLv23_server_method ());
	SSL_CTX_load_verify_locations(ctx,certs,NULL);
	if((err=SSL_CTX_use_certificate_file(ctx, certfile ,SSL_FILETYPE_PEM))<1)
	{
		sserver_error("Couldn't read certificate", err);
		return CVSPROTO_FAIL;
	}
	if((err=SSL_CTX_use_PrivateKey_file(ctx, keyfile ,SSL_FILETYPE_PEM))<1)
	{
		sserver_error("Couldn't read certificate", err);
		return CVSPROTO_FAIL;
	}
	if(!SSL_CTX_check_private_key(ctx))
	{
		sserver_error("Certificate verification failed", err);
		return CVSPROTO_FAIL;
	}

    ssl = SSL_new (ctx);
#ifdef _WIN32 /* Win32 is stupid... */
    SSL_set_rfd (ssl, _get_osfhandle(0));
    SSL_set_wfd (ssl, _get_osfhandle(1));
#else
    SSL_set_rfd (ssl, 0);
    SSL_set_wfd (ssl, 1);
#endif
    if((err=SSL_accept(ssl))<1)
	{
		sserver_error("SSL connection failed", err);
		return CVSPROTO_FAIL;
	}
    set_encrypted_channel(1); /* Error must go through us now */
    /* Get the three important pieces of information in order. */
    /* See above comment about error handling.  */
    server_getline (protocol, &sserver_protocol_interface.auth_repository, MAX_PATH);
    server_getline (protocol, &sserver_protocol_interface.auth_username, MAX_PATH);
    server_getline (protocol, &sserver_protocol_interface.auth_password, MAX_PATH);

    /* ... and make sure the protocol ends on the right foot. */
    /* See above comment about error handling.  */
    server_getline(protocol, &tmp, MAX_PATH);
    if (strcmp (tmp,
		sserver_protocol_interface.verify_only ?
		"END SSL VERIFICATION REQUEST" : "END SSL AUTH REQUEST")
	!= 0)
    {
		server_error (1, "bad auth protocol end: %s", tmp);
		free(tmp);
    }

	pserver_decrypt_password(sserver_protocol_interface.auth_password, tmp, MAX_PATH);
	strcpy(sserver_protocol_interface.auth_password, tmp);

	free(tmp);
	return CVSPROTO_SUCCESS;
}

int sserver_get_user_password(const char *username, const char *server, const char *port, const char *directory, char *password, int password_len)
{
	char tmp[1024];

	if(port)
		snprintf(tmp,sizeof(tmp),":sserver:%s@%s:%s:%s",username,server,port,directory);
	else
		snprintf(tmp,sizeof(tmp),":sserver:%s@%s:%s",username,server,directory);
	if(!get_user_local_config_data("cvspass",tmp,password,password_len))
		return CVSPROTO_SUCCESS;
	else
		return CVSPROTO_FAIL;
}

int sserver_set_user_password(const char *username, const char *server, const char *port, const char *directory, const char *password)
{
	char tmp[1024];

	if(port)
		snprintf(tmp,sizeof(tmp),":sserver:%s@%s:%s:%s",username,server,port,directory);
	else
		snprintf(tmp,sizeof(tmp),":sserver:%s@%s:%s",username,server,directory);
	if(!set_user_local_config_data("cvspass",tmp,password))
		return CVSPROTO_SUCCESS;
	else
		return CVSPROTO_FAIL;
}

int sserver_read_data(const struct protocol_interface *protocol, void *data, int length)
{
	int r,e;
	r=SSL_read(ssl,data,length);
	switch(e=SSL_get_error(ssl,r))
	{
		case SSL_ERROR_NONE:
			return r;
		case SSL_ERROR_ZERO_RETURN:
			return 0;
		default:
			sserver_error("Read data failed", e);
			return -1;
	}
}

int sserver_write_data(const struct protocol_interface *protocol, const void *data, int length)
{
	int offset=0,r,e;
	while(length)
	{	
		r=SSL_write(ssl,((const char *)data)+offset,length);
		switch(e=SSL_get_error(ssl,r))
		{
		case SSL_ERROR_NONE:
			length -= r;
			offset += r;
			break;
		case SSL_ERROR_WANT_WRITE:
			break;
		default:
			sserver_error("Write data failed", e);
			return -1;
		}
	}
	return offset;
}

int sserver_flush_data(const struct protocol_interface *protocol)
{
	return 0; // TCP/IP is always flushed
}

int sserver_shutdown(const struct protocol_interface *protocol)
{
	SSL_shutdown(ssl);
	return 0;
}

int sserver_printf(char *fmt, ...)
{
	char temp[1024];
	va_list va;

	va_start(va,fmt);

	vsnprintf(temp,sizeof(temp),fmt,va);

	va_end(va);

	return sserver_write_data(NULL,temp,strlen(temp));
}

int sserver_wrap(const struct protocol_interface *protocol, int unwrap, int encrypt, const void *input, int size, void *output, int *newsize)
{
	memcpy(output,input,size);
	*newsize=size;
	return 0;
}

int sserver_validate_keyword(const struct protocol_interface *protocol, cvsroot_t *root, const char *keyword, const char *value)
{
   if(!strcasecmp(keyword,"version") || !strcasecmp(keyword,"ver"))
   {
      root->optional_1 = strdup(value);
      return CVSPROTO_SUCCESS;
   }
   if(!strcasecmp(keyword,"strict"))
   {
      root->optional_2 = strdup(value);
      return CVSPROTO_SUCCESS;
   }
   return CVSPROTO_FAIL;
}

const char *sserver_get_keyword_help(const struct protocol_interface *protocol)
{
	return "version\0Server implementation (default: 0) (alias: ver)\0strict\0Strict certificate checks (default: 0)\0";
}

void sserver_error(const char *txt, int err)
{
	char errbuf[1024];
	ERR_error_string_n(ERR_get_error(), errbuf, sizeof(errbuf));
	server_error(0, "%s: %s",txt,errbuf);
}
