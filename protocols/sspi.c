/* CVS sspi auth interface

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

#include <winsock2.h>
#include <ws2tcpip.h>
#include <io.h>
#define SECURITY_WIN32
#include <security.h>
#include <lm.h>

#ifdef _DEBUG
//#define KERBEROS_TEST
#endif

#include "protocol_interface.h"
#include "common.h"
#include "scramble.h"
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
static int sspi_wrap(const struct protocol_interface *protocol, int unwrap, int encrypt, const void *input, int size, void *output, int *newsize);
static int sspi_unwrap_buffer(const void *buffer, int size, void *output, int *newsize);
static int sspi_wrap_buffer(int encrypt, const void *buffer, int size, void *output, int *newsize);
static int ServerAuthenticate(const char *proto);
static int ClientAuthenticate(const char *protocol, const char *name, const char *pwd, const char *domain);
static int sspi_get_user_password(const char *username, const char *server, const char *port, const char *directory, char *password, int password_len);
static int sspi_set_user_password(const char *username, const char *server, const char *port, const char *directory, const char *password);

static int sspi_auth_protocol_connect(const struct protocol_interface *protocol, const char *auth_string);
static int sspi_impersonate(const struct protocol_interface *protocol, const char *username, void *user_handle);
static void initSecLib();
static int InitProtocol(const char *protocol);

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
	sspi_wrap,
	sspi_auth_protocol_connect,
	sspi_read_data,
	sspi_write_data,
	sspi_flush_data,
	sspi_shutdown,
	sspi_impersonate,
	NULL, /* validate_keyword */
	NULL, /* get_keyword_help */
	NULL, /* server_read_data */
	NULL, /* server_write_data */
	NULL, /* server_flush_data */
	NULL /* server_shutdown */
};

static PSecurityFunctionTable pFunctionTable;
static HINSTANCE hSecurity = NULL;
static CredHandle credHandle;
static CtxtHandle contextHandle;
static SecPkgInfo *secPackInfo = NULL;
static SecPkgContext_Sizes secSizes;
static int negotiate_mode;

struct protocol_interface *get_protocol_interface(const struct server_interface *server)
{
	OSVERSIONINFO osv = { sizeof(OSVERSIONINFO) };
	
	initSecLib();
	current_server = server;

	negotiate_mode = 0;
	/* sspi encryption in Win9x is utterly broken, so we don't support it */
	GetVersionEx(&osv);
	if (osv.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS)
		sspi_protocol_interface.wrap=NULL;
	else
	{
		/* NT4 doesn't support the 'Negotiate' package, so disable it */
		if (osv.dwMajorVersion>=5)
			negotiate_mode = 1;
	}
	return &sspi_protocol_interface;
}

void sspi_destroy(const struct protocol_interface *protocol)
{
	free(protocol->auth_username);
	free(protocol->auth_repository);

	if(pFunctionTable && secPackInfo)
		(pFunctionTable->FreeContextBuffer)( secPackInfo );
	if(hSecurity)
		FreeLibrary(hSecurity);
}

int sspi_connect(const struct protocol_interface *protocol, int verify_only)
{
	char crypt_password[64],real_password[64],*password;
	char domain_buffer[128],*domain;
	char user_buffer[128],*user,*p;
	const char *begin_request = "BEGIN SSPI";
	char protocols[1024];
	const char *proto;

	if(!current_server->current_root->hostname || !current_server->current_root->directory)
		return CVSPROTO_BADPARMS;

	if(tcp_connect(current_server->current_root))
		return CVSPROTO_FAIL;

	user = current_server->current_root->username;
	password = current_server->current_root->password;
	domain = NULL;

	if(user && !current_server->current_root->password)
	{
		if(!sspi_get_user_password(user,current_server->current_root->hostname,current_server->current_root->port,current_server->current_root->directory,crypt_password,sizeof(crypt_password)))
		{
			pserver_decrypt_password(crypt_password, real_password, sizeof(real_password));
			password = real_password;
		}
	}

	if(user && strchr(user,'\\'))
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

	if(negotiate_mode)
	{
#ifdef KERBEROS_TEST
		if(tcp_printf("%s\nKerberos\n",begin_request)<0)
			return CVSPROTO_FAIL;
#else
		if(tcp_printf("%s\nNegotiate,NTLM\n",begin_request)<0)
			return CVSPROTO_FAIL;
#endif
	}
	else
	{
		if(tcp_printf("%s\nNTLM\n",begin_request)<0)
			return CVSPROTO_FAIL;
	}

	tcp_readline(protocols, sizeof(protocols));
	if((p=strstr(protocols,"[server aborted"))!=NULL)
	{
		server_error(1, p);
	}

	/* The server won't send us negotiate if we haven't requested it (above) so no need
	   to check here */
	if(strstr(protocols,"Kerberos"))
		proto="Kerberos";
	else if(strstr(protocols,"Negotiate"))
		proto="Negotiate";
	else if(strstr(protocols,"NTLM"))
		proto="NTLM";
	else
	    server_error(1, "Can't authenticate - server and client cannot agree on an authentication scheme (got '%s')",protocols);

	if(!InitProtocol(proto))
	{
		server_error(1, "Couldn't initialise '%s' package - SSPI broke?",proto);
	}

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

	pFunctionTable->QueryContextAttributes(&contextHandle,SECPKG_ATTR_SIZES,&secSizes);
	
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
	const char *user = get_username(current_server->current_root);
	
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
	const char *user = get_username(current_server->current_root);
	
	if(sspi_set_user_password(user,current_server->current_root->hostname,current_server->current_root->port,current_server->current_root->directory,NULL))
	{
		server_error(1,"Failed to delete password");
	}
	return CVSPROTO_SUCCESS;
}

int sspi_auth_protocol_connect(const struct protocol_interface *protocol, const char *auth_string)
{
	char szUser[128];
	DWORD dwLen;
	int rc;
	char *protocols;
	const char *proto;

    if (!strcmp (auth_string, "BEGIN SSPI"))
		sspi_protocol_interface.verify_only = 0;
	else
		return CVSPROTO_NOTME;

	server_getline(protocol, &protocols, 1024);

	if(!protocols)
	{
		printf("Nope!\n");
		return CVSPROTO_FAIL;
	}
	if(negotiate_mode && strstr(protocols,"Kerberos"))
		proto="Kerberos";
	else if(negotiate_mode && strstr(protocols,"Negotiate"))
		proto="Negotiate";
	else if(strstr(protocols,"NTLM"))
		proto="NTLM";
	else
	{
		printf("Nope!\n");
		return CVSPROTO_FAIL;
	}
	free(protocols);

	
	printf("%s\n",proto); /* We have negotiated NTLM */
	fflush(stdout);

	if(!InitProtocol(proto))
		return CVSPROTO_FAIL;

	if(!ServerAuthenticate(proto))
		return CVSPROTO_FAIL;

	pFunctionTable->QueryContextAttributes(&contextHandle,SECPKG_ATTR_SIZES,&secSizes);

	// now we try to use the context
	rc = (pFunctionTable->ImpersonateSecurityContext)( &contextHandle );
	if ( rc != SEC_E_OK )
		return CVSPROTO_FAIL;

	dwLen=sizeof(szUser);
	GetUserName( szUser, &dwLen );
	pFunctionTable->RevertSecurityContext( &contextHandle );
	
	sspi_protocol_interface.auth_username = strdup(szUser);

	/* Get the repository details for checking */
    server_getline (protocol, &sspi_protocol_interface.auth_repository, 4096);

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

int sspi_wrap(const struct protocol_interface *protocol, int unwrap, int encrypt, const void *input, int size, void *output, int *newsize)
{
	if(unwrap)
		return sspi_unwrap_buffer(input,size,output,newsize);
	else
		return sspi_wrap_buffer(encrypt,input,size,output,newsize);
	return 0;
}

static int sspi_unwrap_buffer(const void *buffer, int size, void *output, int *newsize)
{
	SecBuffer rgsb[] =
	{
        {size - secSizes.cbSecurityTrailer, SECBUFFER_DATA, output},
        {secSizes.cbSecurityTrailer, SECBUFFER_TOKEN,  ((char*)output)+size - secSizes.cbSecurityTrailer}
    };
    SecBufferDesc sbd = {SECBUFFER_VERSION, sizeof rgsb / sizeof *rgsb, rgsb};
	int rc;

	if(stricmp(secPackInfo->Name,"NTLM"))
	{
		/* None-NTLM needs the value of rgsb[1].cbBuffer passed through to the other end.
		   We don't use this in the NTLM case for compatibility with legacy servers */
		rgsb[1].cbBuffer=*(DWORD*)(((char*)buffer)+size-sizeof(DWORD));
		((char*)rgsb[1].pvBuffer)-=sizeof(DWORD);
		rgsb[0].cbBuffer-=sizeof(DWORD);
		size-=sizeof(DWORD);
	}

	memcpy(output,buffer,size);
	rc = pFunctionTable->DecryptMessage(&contextHandle,&sbd,0,0);

	if(rc)
		return -1;

	*newsize = size - secSizes.cbSecurityTrailer;
    return 0;
}

static int sspi_wrap_buffer(int encrypt, const void *buffer, int size, void *output, int *newsize)
{
    SecBuffer rgsb[] =
	{
        {size, SECBUFFER_DATA, output},
        {secSizes.cbSecurityTrailer, SECBUFFER_TOKEN, ((char*)output)+size},
    };
    SecBufferDesc sbd = {SECBUFFER_VERSION, sizeof rgsb / sizeof *rgsb, rgsb};
	int rc;

	/* NTLM doesn't really support wrapping, so we always encrypt */
	/* In theory we could use SignMessage... for the future perhaps */

	memcpy(output, buffer, size);

    // describe our buffer for SSPI
    // encrypt in place

	rc = pFunctionTable->EncryptMessage(&contextHandle, 0, &sbd, 0);

	if(rc)
		return -1;

    *newsize = size + secSizes.cbSecurityTrailer;

	if(stricmp(secPackInfo->Name,"NTLM"))
	{
		/* None-NTLM needs the value of rgsb[1].cbBuffer passed through to the other end.
		   We don't send this in the NTLM case for compatibility with legacy servers */
		*(DWORD*)(((char*)output)+*newsize)=rgsb[1].cbBuffer;
		(*newsize)+=sizeof(DWORD);
	}

	return 0;
}

/*********************************************/
/* From example code on mvps.org */
void initSecLib()
{
	PSecurityFunctionTable (*pSFT)( void );

	hSecurity = LoadLibrary( "secur32.dll" );
	if(!hSecurity)
		hSecurity = LoadLibrary( "security.dll" );
	pSFT = (PSecurityFunctionTable (*)( void )) GetProcAddress( hSecurity, "InitSecurityInterfaceA" );
	if ( pSFT == NULL )
	{
		server_error(1, "Couldn't initialise SSPI:  loading security.dll failed" );
		exit( 1 );
	}

	pFunctionTable = pSFT();
	if ( pFunctionTable == NULL )
	{
		server_error(1, "Couldn't initialise SSPI: no function table?!?" );
	}
}

int InitProtocol(const char *protocol)
{
	if(pFunctionTable->QuerySecurityPackageInfo( (char*)protocol, &secPackInfo ) != SEC_E_OK)
		return 0;

	/* If NTLM on this machine doesn't support encryption disable it.  This should never
	   happen in theory */
	if(!pFunctionTable->EncryptMessage)
		sspi_protocol_interface.wrap=NULL;

	return 1;
}

int ServerAuthenticate(const char *proto)
{
	int rc;
	BOOL haveToken = TRUE;
	int bytesReceived = 0, bytesSent = 0;
	TimeStamp useBefore;
	// input and output buffers
	SecBufferDesc obd, ibd;
	SecBuffer ob, ib;
	BOOL haveContext = FALSE;
	DWORD ctxAttr;
	char *p;
	int n;
	short len;

	rc = (pFunctionTable->AcquireCredentialsHandle)( NULL, (char*)proto, SECPKG_CRED_INBOUND,
		NULL, NULL, NULL, NULL, &credHandle, &useBefore );
	if ( rc != SEC_E_OK )
		haveToken = FALSE;

	while ( 1 )
	{
		// prepare to get the server's response
		ibd.ulVersion = SECBUFFER_VERSION;
		ibd.cBuffers = 1;
		ibd.pBuffers = &ib; // just one buffer
		ib.BufferType = SECBUFFER_TOKEN; // preping a token here

		// receive the client's POD

		rc = read( 0, (char *) &len, sizeof(len) );
		if(rc<=0)
			break;
		ib.cbBuffer = ntohs(len);
		bytesReceived += sizeof ib.cbBuffer;
		ib.pvBuffer = malloc( ib.cbBuffer );

		p = (char *) ib.pvBuffer;
		n = ib.cbBuffer;
		while ( n )
		{
			rc = read( 0, (char *) p, n);
			if(rc<=0)
				break;
			bytesReceived += rc;
			n -= rc;
			p += rc;
		}

		// by now we have an input buffer

		obd.ulVersion = SECBUFFER_VERSION;
		obd.cBuffers = 1;
		obd.pBuffers = &ob; // just one buffer
		ob.BufferType = SECBUFFER_TOKEN; // preping a token here
		ob.cbBuffer = secPackInfo->cbMaxToken;
		ob.pvBuffer = malloc( ob.cbBuffer );

		rc = (pFunctionTable->AcceptSecurityContext)( &credHandle, haveContext? &contextHandle: NULL,
			&ibd, 0, SECURITY_NATIVE_DREP, &contextHandle, &obd, &ctxAttr,
			&useBefore );

		if ( ib.pvBuffer != NULL )
		{
			free( ib.pvBuffer );
			ib.pvBuffer = NULL;
		}

		if ( rc == SEC_I_COMPLETE_AND_CONTINUE || rc == SEC_I_COMPLETE_NEEDED )
		{
			if ( pFunctionTable->CompleteAuthToken != NULL ) // only if implemented
				(pFunctionTable->CompleteAuthToken)( &contextHandle, &obd );
			if ( rc == SEC_I_COMPLETE_NEEDED )
				rc = SEC_E_OK;
			else if ( rc == SEC_I_COMPLETE_AND_CONTINUE )
				rc = SEC_I_CONTINUE_NEEDED;
		}

		// send the output buffer off to the server
		// warning -- this is machine-dependent! FIX IT!
		if ( rc == SEC_E_OK || rc == SEC_I_CONTINUE_NEEDED )
		{
			if ( ob.cbBuffer != 0 )
			{
				len=htons((short)ob.cbBuffer);
				if((n=fwrite(&len,1,sizeof len,stdout))<=0)
					break;
				bytesSent += n;
				if((n=fwrite((const char *) ob.pvBuffer, 1,ob.cbBuffer,stdout))<=0)
					break;
				bytesSent += n;
				fflush(stdout);
			}
			free( ob.pvBuffer );
			ob.pvBuffer = NULL;
		}

		if ( rc != SEC_I_CONTINUE_NEEDED )
			break;

		haveContext = TRUE;
	}

	// we arrive here as soon as InitializeSecurityContext()
	// returns != SEC_I_CONTINUE_NEEDED.

	if ( rc != SEC_E_OK )
	{
		haveToken = FALSE;
	}

	return haveToken;
}

int ClientAuthenticate(const char *protocol, const char *name, const char *pwd, const char *domain)
{
	int rc, rcISC;
	SEC_WINNT_AUTH_IDENTITY nameAndPwd = {0};
	int bytesReceived = 0, bytesSent = 0;
	char myTokenSource[DNLEN*4];
	char username[UNLEN+1];
	TimeStamp useBefore;
	DWORD ctxReq, ctxAttr;
	DWORD dwRead, dwWritten;
	// input and output buffers
	SecBufferDesc obd, ibd;
	SecBuffer ob, ib;
	BOOL haveInbuffer = FALSE;
	BOOL haveContext = FALSE;
	char *p;
	int n;
	short len;
	const struct addrinfo *addrinfo;

	if ( name )
	{
		nameAndPwd.Domain = (byte *) _strdup( domain? domain: "" );
		nameAndPwd.DomainLength = domain? strlen( domain ): 0;
		nameAndPwd.User = (byte *) _strdup( name? name: "" );
		nameAndPwd.UserLength = name? strlen( name ): 0;
		nameAndPwd.Password = (byte *) _strdup( pwd? pwd: "" );
		nameAndPwd.PasswordLength = pwd? strlen( pwd ): 0;
		nameAndPwd.Flags = SEC_WINNT_AUTH_IDENTITY_ANSI;
	}
	else 
	{
		nameAndPwd.UserLength = sizeof(username);
		GetUserName(username,&nameAndPwd.UserLength);
		nameAndPwd.User= (byte *)username;
		nameAndPwd.Flags = SEC_WINNT_AUTH_IDENTITY_ANSI;
	}

	rc = (pFunctionTable->AcquireCredentialsHandle)( NULL, (char*)protocol, SECPKG_CRED_OUTBOUND,
		NULL, name?&nameAndPwd:NULL, NULL, NULL, &credHandle, &useBefore );

	ctxReq = ISC_REQ_REPLAY_DETECT | ISC_REQ_SEQUENCE_DETECT |
		ISC_REQ_CONFIDENTIALITY | ISC_REQ_DELEGATE;

	ib.pvBuffer = NULL;

	addrinfo = get_addrinfo(1);

    sprintf (myTokenSource, "cvs/%s", addrinfo->ai_canonname);

	while ( 1 )
	{
		obd.ulVersion = SECBUFFER_VERSION;
		obd.cBuffers = 1;
		obd.pBuffers = &ob; // just one buffer
		ob.BufferType = SECBUFFER_TOKEN; // preping a token here
		ob.cbBuffer = secPackInfo->cbMaxToken;
		ob.pvBuffer = malloc( ob.cbBuffer );

		rcISC = (pFunctionTable->InitializeSecurityContext)( &credHandle, haveContext? &contextHandle: NULL,
			myTokenSource, ctxReq, 0, SECURITY_NATIVE_DREP, haveInbuffer? &ibd: NULL,
			0, &contextHandle, &obd, &ctxAttr, &useBefore );

		if ( ib.pvBuffer != NULL )
		{
			free( ib.pvBuffer );
			ib.pvBuffer = NULL;
		}

		if ( rcISC == SEC_I_COMPLETE_AND_CONTINUE || rcISC == SEC_I_COMPLETE_NEEDED )
		{
			if ( pFunctionTable->CompleteAuthToken != NULL ) // only if implemented
				(pFunctionTable->CompleteAuthToken)( &contextHandle, &obd );
			if ( rcISC == SEC_I_COMPLETE_NEEDED )
				rcISC = SEC_E_OK;
			else if ( rcISC == SEC_I_COMPLETE_AND_CONTINUE )
				rcISC = SEC_I_CONTINUE_NEEDED;
		}

		// send the output buffer off to the server
		if ( ob.cbBuffer != 0 )
		{
			len=htons((short)ob.cbBuffer);
			if((dwWritten=tcp_write( (const char *) &len, sizeof len))<=0)
				break;
			bytesSent += dwWritten;
			if((dwWritten=tcp_write( (const char *) ob.pvBuffer, ob.cbBuffer))<=0)
				break;
			bytesSent += dwWritten;
		}
		free( ob.pvBuffer );
		ob.pvBuffer = NULL;

		if ( rcISC != SEC_I_CONTINUE_NEEDED )
			break;

		// prepare to get the server's response
		ibd.ulVersion = SECBUFFER_VERSION;
		ibd.cBuffers = 1;
		ibd.pBuffers = &ib; // just one buffer
		ib.BufferType = SECBUFFER_TOKEN; // preping a token here

		// receive the server's response
		if((dwRead=tcp_read((char *) &len, sizeof len))<=0)
			break;
		ib.cbBuffer=ntohs(len);
		bytesReceived += sizeof ib.cbBuffer;
		ib.pvBuffer = malloc( ib.cbBuffer );

		p = (char *) ib.pvBuffer;
		n = ib.cbBuffer;
		while ( n )
		{
			if((dwRead=tcp_read(p,n))<=0)
				break;
			bytesReceived += dwRead;
			n -= dwRead;;
			p += dwRead;
		}

		// by now we have an input buffer and a client context

		haveInbuffer = TRUE;
		haveContext = TRUE;
	}

	// we arrive here as soon as InitializeSecurityContext()
	// returns != SEC_I_CONTINUE_NEEDED.

	if ( rcISC != SEC_E_OK )
		haveContext = FALSE;
	else
		haveContext = TRUE; /* Looopback kerberos needs this */

	if(name)
	{
		free( nameAndPwd.Domain );
		free( nameAndPwd.User );
		free( nameAndPwd.Password );
	}

	return haveContext;
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

int sspi_impersonate(const struct protocol_interface *protocol, const char *username, void *user_handle)
{
	if(pFunctionTable->ImpersonateSecurityContext(&contextHandle)==SEC_E_OK)
		return CVSPROTO_SUCCESS;
	return CVSPROTO_FAIL;
}

