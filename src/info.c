/*
 * Copyright (c) 1992, Brian Berliner and Jeff Polk
 * Copyright (c) 1989-1992, Brian Berliner
 * Copyright (c) 2001, Tony Hoyle
 * 
 * You may distribute under the terms of the GNU General Public License as
 * specified in the README file that comes with the CVS source distribution.
 * 
 * Query CVS/Entries from server
 */

#include "cvs.h"
#include "library.h"

struct protocol_interface local_protocol =
{
	PROTOCOL_INTERFACE_VERSION,

	"local",
	"(internal)",
	":local:/path",
};

#define ELEM_VALID(_elem) protocol->required_elements&_elem?"Required":protocol->valid_elements&_elem?"Optional":"No"

static const char *const info_usage[] =
{
	"Usage: %s %s [-c|-s] [cvswrappers|cvsignore|<protocol>]\n",
	"    -c      Describe client (default)\n",
	"    -s      Describe server\n",
    "(Specify the --help global option for a list of other help options)\n",
    NULL
};

int info(int argc, char **argv)
{
    int c;
	int context;
	const char *proto;
	const struct protocol_interface *protocol;
	int use_server = 0;

    if (argc == -1)
		usage (info_usage);

    optind = 0;
    while ((c = getopt (argc, argv, "cs")) != -1)
    {
		switch (c)
		{
		case 'c':
			use_server = 0;
			break;
		case 's':
			use_server = 1;
			break;
		case '?':
		default:
			usage (info_usage);
			break;
		}
    }
    argc -= optind;
    argv += optind;

	if(argc>1)
	{
		usage(info_usage);
		return 1;
	}

#ifdef CLIENT_SUPPORT
	if(use_server && !current_parsed_root)
	{
              error (0, 0, "No CVSROOT specified!  Please use the `-d' option");
              error (1, 0, "or set the %s environment variable.", CVSROOT_ENV);
	}

	if(use_server && current_parsed_root->isremote)
	{
		start_server(0);
		ign_setup();
		wrap_setup();
	
		if(!supported_request("info"))
			error(1,0,"Server does not support %s",command_name);
	
		if(argc)
			send_arg(argv[0]);
		send_to_server("info\012", 0);

		return get_responses_and_close();
	}
#endif
	ign_setup();
	wrap_setup();

	if(argc==0)
	{
		if(!server_active)
			printf("Available protocols:\n\n");
		else
			printf("Available protocols on server:\n\n");
		TRACE(1,"Interface version is %04x",PROTOCOL_INTERFACE_VERSION);
		
		if(!server_active)
			printf("%-20.20s%s\n","local","(internal)");
		context=0;
		while((proto=enumerate_protocols(&context))!=NULL)
		{
			if(!client_protocol || strcmp(proto,client_protocol->name))
				protocol=load_protocol(proto);
			else
				protocol=client_protocol;
			if(protocol && (!server_active || protocol->auth_protocol_connect))
				printf("%-20.20s%s\n",protocol->name,protocol->version);
			if(protocol!=client_protocol)
				unload_protocol(protocol);
		}
	}
	else
	{
		if(!strcmp(argv[0],"cvswrappers"))
		{
		    wrap_display();
		    return 0;
		}
		if(!strcmp(argv[0],"cvsignore"))
		{
		    ign_display();
		    return 0;
		}
		if(!strcmp(argv[0],"local"))
			protocol = &local_protocol;
		else
		{
			if(!client_protocol || strcmp(argv[0],client_protocol->name))
				protocol = load_protocol(argv[0]);
			else
				protocol = client_protocol;
		}
		if(!protocol)
		{
			error(1,0,"Couldn't load protocol '%s'",argv[0]);
			return 1;
		}
		printf("%-20s%s\n","Name:",protocol->name);
		printf("%-20s%s\n","Version:",protocol->version);
		printf("%-20s%s\n","Syntax:",protocol->syntax);
		printf("%-20s%s\n","  Username:",ELEM_VALID(elemUsername));
		printf("%-20s%s\n","  Password:",ELEM_VALID(elemPassword));
		printf("%-20s%s\n","  Hostname:",ELEM_VALID(elemHostname));
		printf("%-20s%s\n","  Port:",ELEM_VALID(elemPort));
		printf("%-20s%s\n","Client:",protocol->connect?"Yes":"No");
		printf("%-20s%s\n","Server:",protocol->auth_protocol_connect?"Yes":"No");
		printf("%-20s%s\n","Login:",protocol->login?"Yes":"No");
		printf("%-20s%s\n","Encryption:",protocol->valid_elements&flagAlwaysEncrypted?"Always":protocol->wrap?"Yes":"No");
		printf("%-20s%s\n","Impersonation:",protocol->impersonate?"Native":"CVS Builtin");

		printf("\nKeywords available:\n\n");
		if(protocol->valid_elements&elemUsername)
			printf("%-20s%s\n","username","Username (alias: user)");
		if(protocol->valid_elements&elemPassword)
			printf("%-20s%s\n","password","Password (alias: pass)");
		if(protocol->valid_elements&elemHostname)
			printf("%-20s%s\n","hostname","Hostname (alias: host)");
		if(protocol->valid_elements&elemPort)
			printf("%-20s%s\n","port","Port");
		if(protocol->valid_elements&elemTunnel)
		{
			printf("%-20s%s\n","proxy","Proxy server");
			printf("%-20s%s\n","proxyport","Proxy server port (alias: proxy_port)");
			printf("%-20s%s\n","tunnel","Proxy protocol (aliases: proxyprotocol,proxy_protocol)");
			printf("%-20s%s\n","proxyuser","Proxy user (alias: proxy_user)");
			printf("%-20s%s\n","proxypassword","Proxy passsord (alias: proxy_password)");
		}
		if(protocol->get_keyword_help)
		{
			const char *p;
			p = protocol->get_keyword_help(protocol);
			while(*p)
			{
				printf("%-20s%s\n",p,p+strlen(p)+1);
				p+=strlen(p)+1;
				p+=strlen(p)+1;
			}
		}
		if(strcmp(argv[0],"local") && protocol!=client_protocol)
			unload_protocol(protocol);
	}

    return 0;
}
