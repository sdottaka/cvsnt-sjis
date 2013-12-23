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

#define ELEM_VALID(_elem) protocol->required_elements&_elem?"Required":protocol->valid_elements&_elem?"Optional":"No"

static const char *const info_usage[] =
{
	"Usage: %s %s [protocol]\n",
    "(Specify the --help global option for a list of other help options)\n",
    NULL
};

int info(int argc, char **argv)
{
    int c;
	int context;
	const char *proto;
	const struct protocol_interface *protocol;

    if (argc == -1)
		usage (info_usage);

    optind = 0;
    while ((c = getopt (argc, argv, "c")) != -1)
    {
		switch (c)
		{
		case 'c':
			printf("Crashing...");
			*(int*)0=0;
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

	if(argc==0)
	{

		printf("Available protocols:\n\n");
		TRACE(1,"Interface version is %04x",PROTOCOL_INTERFACE_VERSION);

		context=0;
		while((proto=enumerate_protocols(&context))!=NULL)
		{
			protocol=load_protocol(proto);
			if(!protocol)
				continue;
			printf("%-20.20s%s\n",protocol->name,protocol->version);
			unload_protocol(protocol);
		}
	}
	else
	{
		protocol = load_protocol(argv[0]);
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
		printf("%-20s%s\n","Encryption:",protocol->wrap?"Yes":"No");
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
		unload_protocol(protocol);
	}

    return 0;
}
