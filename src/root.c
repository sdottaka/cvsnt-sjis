/*
 * Copyright (c) 1992, Mark D. Baushke
 *
 * You may distribute under the terms of the GNU General Public License as
 * specified in the README file that comes with the CVS source distribution.
 * 
 * Name of Root
 * 
 * Determine the path to the CVSROOT and set "Root" accordingly.
 */

#include "cvs.h"
#include "getline.h"
#include "library.h"
#include "savecwd.h"

#ifdef _WIN32
#include <winsock2.h>
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>
#endif

#ifndef DEBUG

char *
Name_Root (dir, update_dir)
    char *dir;
    char *update_dir;
{
    FILE *fpin;
    char *ret, *xupdate_dir;
    char *root = NULL;
    size_t root_allocated = 0;
    char *tmp;
    char *cvsadm;
    char *cp;

    if (update_dir && *update_dir)
	xupdate_dir = update_dir;
    else
	xupdate_dir = ".";

    if (dir != NULL)
    {
	cvsadm = xmalloc (strlen (dir) + sizeof (CVSADM) + 10);
	(void) sprintf (cvsadm, "%s/%s", dir, CVSADM);
	tmp = xmalloc (strlen (dir) + sizeof (CVSADM_ROOT) + 10);
	(void) sprintf (tmp, "%s/%s", dir, CVSADM_ROOT);
    }
    else
    {
	cvsadm = xstrdup (CVSADM);
	tmp = xstrdup (CVSADM_ROOT);
    }

    /*
     * Do not bother looking for a readable file if there is no cvsadm
     * directory present.
     *
     * It is possible that not all repositories will have a CVS/Root
     * file. This is ok, but the user will need to specify -d
     * /path/name or have the environment variable CVSROOT set in
     * order to continue.  */
    if ((!isdir (cvsadm)) || (!isreadable (tmp)))
    {
	ret = NULL;
	goto out;
    }

    /*
     * The assumption here is that the CVS Root is always contained in the
     * first line of the "Root" file.
     */
    fpin = open_file (tmp, "r");

    if (getline (&root, &root_allocated, fpin) < 0)
    {
	/* FIXME: should be checking for end of file separately; errno
	   is not set in that case.  */
	error (0, 0, "in directory %s:", xupdate_dir);
	error (0, errno, "cannot read %s", CVSADM_ROOT);
	error (0, 0, "please correct this problem");
	ret = NULL;
	goto out;
    }
    (void) fclose (fpin);
    if ((cp = strrchr (root, '\n')) != NULL)
	*cp = '\0';			/* strip the newline */

    /*
     * root now contains a candidate for CVSroot. It must be an
     * absolute pathname or specify a remote server.
     */

    if (
#ifdef CLIENT_SUPPORT
		(strchr (root, ':') == NULL) && (strchr(root, '@') == NULL) &&
#endif
    	! isabsolute (root))
    {
	error (0, 0, "in directory %s:", xupdate_dir);
	error (0, 0,
	       "ignoring %s because it does not contain an absolute pathname.",
	       CVSADM_ROOT);
	ret = NULL;
	goto out;
    }

    {
		const char *tmp;
		
		if(!server_active || !root_allow_ok(root,&tmp))
			tmp = root;
#ifdef CLIENT_SUPPORT
	    if ((strchr (root, ':') == NULL) && (strchr(root, '@') == NULL) && !isdir (tmp))
#else /* ! CLIENT_SUPPORT */
		if (!isdir (tmp))
#endif /* CLIENT_SUPPORT */
		{
			error (0, 0, "in directory %s:", xupdate_dir);
			error (0, 0, "ignoring %s because it specifies a non-existent repository %s", CVSADM_ROOT, root);
			ret = NULL;
			goto out;
		}
    }

    /* allocate space to return and fill it in */
    strip_trailing_slashes (root);
    ret = xstrdup (root);
 out:
    xfree (cvsadm);
    xfree (tmp);
    if (root != NULL)
	xfree (root);
    return (ret);
}

/*
 * Write the CVS/Root file so that the environment variable CVSROOT
 * and/or the -d option to cvs will be validated or not necessary for
 * future work.
 */
void Create_Root (char *dir, char *rootdir)
{
    FILE *fout;
    char *tmp;

    if (noexec)
	return;

    /* record the current cvs root */

    if (rootdir != NULL)
    {
        if (dir != NULL)
	{
	    tmp = xmalloc (strlen (dir) + sizeof (CVSADM_ROOT) + 10);
	    (void) sprintf (tmp, "%s/%s", dir, CVSADM_ROOT);
	}
        else
	    tmp = xstrdup (CVSADM_ROOT);

        fout = open_file (tmp, "w+");
        if (fprintf (fout, "%s\n", rootdir) < 0)
	    error (1, errno, "write to %s failed", tmp);
        if (fclose (fout) == EOF)
	    error (1, errno, "cannot close %s", tmp);
	xfree (tmp);
    }
}

#endif /* ! DEBUG */


/* The root_allow_* stuff maintains a list of legal CVSROOT
   directories.  Then we can check against them when a remote user
   hands us a CVSROOT directory.  */

typedef struct
{
	const char *root;
	const char *name;
} root_allow_struct;

int root_allow_count;
static root_allow_struct *root_allow_vector;
static int root_allow_size;

void root_allow_add(const char *root, const char *name)
{
    if (root_allow_size <= root_allow_count)
    {
		if (root_allow_size == 0)
		{
			root_allow_size = 1;
			root_allow_vector = (root_allow_struct *) xmalloc (root_allow_size * sizeof (root_allow_struct));
		}
		else
		{
			root_allow_size *= 2;
			root_allow_vector = (root_allow_struct *) xrealloc(root_allow_vector,  root_allow_size * sizeof (root_allow_struct));
		}

		if (root_allow_vector == NULL)
		{
	//	no_memory:
			/* Strictly speaking, we're not supposed to output anything
			now.  But we're about to exit(), give it a try.  */
			printf ("E Fatal server error, aborting.\nerror ENOMEM Virtual memory exhausted.\n");

			/* I'm doing this manually rather than via error_exit ()
			because I'm not sure whether we want to call server_cleanup.
			Needs more investigation....  */

#ifdef SYSTEM_CLEANUP
			/* Hook for OS-specific behavior, for example socket
			subsystems on NT and OS2 or dealing with windows
			and arguments on Mac.  */
			SYSTEM_CLEANUP ();
#endif

			exit (EXIT_FAILURE);
		}
    }
    root_allow_vector[root_allow_count].root = xstrdup(root);
    root_allow_vector[root_allow_count++].name = xstrdup(name);
}

void root_allow_free ()
{
	int n;

    if (root_allow_vector != NULL)
	{
		for(n=0; n<root_allow_count; n++)
		{
			xfree(root_allow_vector[n].root);
			xfree(root_allow_vector[n].name);
		}
		xfree(root_allow_vector);
	}
    root_allow_size = root_allow_count = 0;
}

int pathcmp(const char *a, const char *b)
{
	if(isalpha((int)(unsigned char)a[0]) && a[1]==':' && ISDIRSEP(b[0]) && ISDIRSEP(b[2]) && ISDIRSEP(b[3]) && FN_CHAR_EQUAL(a[0],b[1]))
	{
		a+=2;
		b+=3;
	}
	if(isalpha((int)(unsigned char)b[0]) && b[1]==':' && ISDIRSEP(a[0]) && ISDIRSEP(a[2]) && ISDIRSEP(a[3]) && FN_CHAR_EQUAL(b[0],a[1]))
	{
		a+=4;
		b+=2;
	}
	while(*a && *b && (FN_CHAR_EQUAL(*a,*b)))
	{
		a++;
		b++;
	}
	// Strip trailing spaces
	while(*a && isspace((int)(unsigned char)*a))
		a++;
	while(*b && isspace((int)(unsigned char)*b))
		b++;
	return (*a)-(*b);
}

int pathncmp(const char *a, const char *b, size_t len, const char **endpoint)
{
	if(isalpha((int)(unsigned char)a[0]) && a[1]==':' && ISDIRSEP(b[0]) && ISDIRSEP(b[2]) && ISDIRSEP(b[3]) && FN_CHAR_EQUAL(a[0],b[1]))
	{
		a+=2;
		b+=3;
		len-=3;
	}
	if(isalpha((int)(unsigned char)b[0]) && b[1]==':' && ISDIRSEP(a[0]) && ISDIRSEP(a[2]) && ISDIRSEP(a[3]) && FN_CHAR_EQUAL(b[0],a[1]))
	{
		a+=3;
		b+=2;
		len-=2;
	}
	while(len && *a && *b && FN_CHAR_EQUAL(*a,*b))
	{
		a++;
		b++;
		len--;
	}
	// Strip trailing spaces
	while(len && *a && isspace((int)(unsigned char)*a))
		a++;
	while(len && *b && isspace((int)(unsigned char)*b))
		{ b++; len--; }

	if(endpoint)
		*endpoint = a;

	if(!len)
		return 0;

	return (*a)-(*b);
}

int root_allow_ok (const char *root, const char **real_root)
{
    int i;
    for (i = 0; i < root_allow_count; ++i)
		if(!pathcmp(root_allow_vector[i].name, root))
		{
			if(real_root) *real_root=root_allow_vector[i].root;
			break;
		}

    if(i==root_allow_count)
      return 0;
#ifndef _WIN32
    if(chroot_done && chroot_base && real_root)
    {
      if(!fnncmp(*real_root,chroot_base,strlen(chroot_base)) && strlen(chroot_base)<=strlen(*real_root))
	*real_root+=strlen(chroot_base);
    }
#endif
    return 1;
}



/* This global variable holds the global -d option.  It is NULL if -d
   was not used, which means that we must get the CVSroot information
   from the CVSROOT environment variable or from a CVS/Root file.  */

char *CVSroot_cmdline;



/* Parse a CVSROOT variable into its constituent parts -- method,
 * username, hostname, directory.  The prototypical CVSROOT variable
 * looks like:
 *
 * :method:user@host:path
 *
 * Some methods may omit fields; local, for example, doesn't need user
 * and host.
 *
 * Returns pointer to new cvsroot_t on success, NULL on failure. */

cvsroot_t *current_parsed_root = NULL;



/* allocate and initialize a cvsroot_t
 *
 * We must initialize the strings to NULL so we know later what we should
 * free
 *
 * Some of the other zeroes remain meaningful as, "never set, use default",
 * or the like
 */
static cvsroot_t *
new_cvsroot_t (void)
{
    cvsroot_t *newroot;

    /* gotta store it somewhere */
    newroot = (cvsroot_t*)xmalloc(sizeof(cvsroot_t));

    newroot->original = NULL;
    newroot->method = NULL;
    newroot->username = NULL;
    newroot->password = NULL;
    newroot->hostname = NULL;
    newroot->port = NULL;
    newroot->directory = NULL;
	newroot->mapped_directory = NULL;
	newroot->unparsed_directory = NULL;
	newroot->optional_1 = NULL;
	newroot->optional_2 = NULL;
	newroot->optional_3 = NULL;
	newroot->optional_4 = NULL;
	newroot->optional_5 = NULL;
	newroot->optional_6 = NULL;
	newroot->optional_7 = NULL;
	newroot->optional_8 = NULL;
	newroot->proxy = NULL;
	newroot->proxyport = NULL;
	newroot->proxyprotocol = NULL;
	newroot->proxyuser = NULL;
	newroot->proxypassword = NULL;
#ifdef CLIENT_SUPPORT
    newroot->isremote = 0;
#endif /* CLIENT_SUPPORT */

    return newroot;
}



/* Dispose of a cvsroot_t and its component parts */
void free_cvsroot_t (cvsroot_t *root)
{
	if(!root)
		return;

	xfree (root->original);
	xfree (root->username);
	/* I like to be paranoid */
	if(root->password)
		memset (root->password, 0, strlen (root->password));
	xfree (root->password);
	xfree (root->hostname);
	xfree (root->port);
	xfree (root->directory);
	xfree(root->unparsed_directory);
	xfree(root->optional_1);
	xfree(root->optional_2);
	xfree(root->optional_3);
	xfree(root->optional_4);
	xfree(root->optional_5);
	xfree(root->optional_6);
	xfree(root->optional_7);
	xfree(root->optional_8);
	xfree(root->proxy);
	xfree(root->proxyport);
	xfree(root->proxyprotocol);
	xfree(root->proxyuser);
	xfree(root->proxypassword);
	xfree(root->mapped_directory);
    xfree (root);
}

static int get_keyword(char *buffer, int buffer_len, char **ptr)
{
	int len = 0;
	int in_quote = 0; 
	while(**ptr && len<buffer_len && in_quote || (isprint(**ptr) && **ptr!='=' && **ptr!=';' && **ptr!=','))
	{
		if(!in_quote && isspace(**ptr))
		{
			(*ptr)++;
			continue;
		}
		else if(**ptr==in_quote)
		{
			in_quote = 0;
			(*ptr)++;
			continue;
		}
		else if(!in_quote && **ptr=='"' || **ptr=='\'')
		{
			in_quote = **ptr;
			(*ptr)++;
			continue;
		}
		buffer[len++]=*(*ptr)++;
	}
	if(in_quote)
		error(0,0,"Nonterminated quote in cvsroot string");
	buffer[len]='\0';
	return len;
}

/* Parse a keyword in a cvsroot string, either old format (:method;foo=x,bar=x:/stuff) or
   new format [foo=x,bar=x] */
static int parse_keyword(char *keyword, char **p, cvsroot_t *newroot)
{
	char value[256];

	get_keyword(value,sizeof(value),p);
	if(!strcasecmp(keyword,"method") || !strcasecmp(keyword,"protocol"))
	{
		if(*value && strcasecmp(value,"local"))
			newroot->method = xstrdup(value);
		if(newroot->method)
		{
#ifdef CLIENT_SUPPORT
			client_protocol = load_protocol(newroot->method);
			if(client_protocol==NULL)
#endif
			{
				error (0, 0, "the :%s: access method is not installed on this system", newroot->method);
				return -1;
			}
			setup_server_interface(newroot);
		}

#ifdef CLIENT_SUPPORT
		newroot->isremote = (newroot->method==NULL)?0:1;
#endif /* CLIENT_SUPPORT */
	}
	else if(!strcasecmp(keyword,"username") || !strcasecmp(keyword,"user"))
	{
		if(*value)
			newroot->username = xstrdup(value);
	}
	else if(!strcasecmp(keyword,"password") || !strcasecmp(keyword,"pass"))
	{
		newroot->password = xstrdup(value); // Empty password is valid
	}
	else if(!strcasecmp(keyword,"hostname") || !strcasecmp(keyword,"host"))
	{
		if(*value)
			newroot->hostname = xstrdup(value);
	}
	else if(!strcasecmp(keyword,"port"))
	{
		if(*value)
			newroot->port = xstrdup(value);
		if(newroot->port)
		{
			char *q = value;
			while (*q)
			{
				if (!isdigit(*q++))
				{
					error(0, 0, "CVSROOT (\"%s\")", newroot->original);
					error(0, 0, "may only specify a positive, non-zero, integer port (not \"%s\").",
						newroot->port);
					return -1;
				}
			}
		}
	}
	else if(!strcasecmp(keyword,"directory") || !strcasecmp(keyword,"path"))
	{
		if(*value)
			newroot->directory = xstrdup(value);
	}
	else if(!strcasecmp(keyword,"proxy"))
	{
		if(*value)
			newroot->proxy = xstrdup(value);
	}
	else if(!strcasecmp(keyword,"proxyport") || !strcasecmp(keyword,"proxy_port"))
	{
		if(*value)
			newroot->proxyport = xstrdup(value);
	}
	else if(!strcasecmp(keyword,"tunnel") || !strcasecmp(keyword,"proxy_protocol") ||
			!strcasecmp(keyword,"proxyprotocol"))
	{
		if(*value)
			newroot->proxyprotocol = xstrdup(value);
	}
	else if(!strcasecmp(keyword,"proxyuser") || !strcasecmp(keyword,"proxy_user"))
	{
		if(*value)
			newroot->proxyuser = xstrdup(value);
	}
	else if(!strcasecmp(keyword,"proxypassword") || !strcasecmp(keyword,"proxy_password"))
	{
		if(*value)
			newroot->proxypassword = xstrdup(value);
	}
#ifdef CLIENT_SUPPORT
	else if(client_protocol && client_protocol->validate_keyword)
	{
		int res = client_protocol->validate_keyword(client_protocol,newroot,keyword,value);
		if(res==CVSPROTO_FAIL || res==CVSPROTO_BADPARMS || res==CVSPROTO_AUTHFAIL)
		{
			error(0,0,"Bad CVSROOT: Unknown keyword '%s'",keyword);
			return -1;
		}
	}
#endif
	else
	{
		error(0,0,"Bad CVSROOT: Unknown keyword '%s'",keyword);
		return -1;
	}
	return 0;
}
/*
 * parse a CVSROOT string to allocate and return a new cvsroot_t structure
 */
cvsroot_t *
parse_cvsroot (char *root_in)
{
    cvsroot_t *newroot;			/* the new root to be returned */
    char *cvsroot_save;			/* what we allocated so we can dispose
					 * it when finished */
    char *firstslash;			/* save where the path spec starts
					 * while we parse
					 * [[user][:password]@]host[:[port]]
					 */
	char char_at_slash; /* Character might be '/', '\', or a drive letter */
    char *cvsroot_copy, *p, *p1, *q;		/* temporary pointers for parsing */
    int check_hostname, no_port, no_password;

    /* allocate some space */
    newroot = new_cvsroot_t();

    /* save the original string */
    newroot->original = xstrdup (root_in);

    /* and another copy we can munge while parsing */
    cvsroot_save = cvsroot_copy = xstrdup (root_in);

	if (*cvsroot_copy == '[')
	{
		char keyword[256];
		/* Comma separated cvsroot, much better for future expansion */
		p=cvsroot_copy;
		p++;
		while(get_keyword(keyword,sizeof(keyword),&p))
		{
			if(*p!='=')
			{
				error(0,0,"Bad CVSROOT: Specify comma separated keyword=value pairs");
				goto error_exit;
			}
			p++;
			if(parse_keyword(keyword, &p, newroot)<0)
				goto error_exit;
			if(*p==']')
				break;
			if(*p!=',')
			{
				error(0,0,"Bad CVSROOT: Specify comma separated keyword=value pairs");
				goto error_exit;
			}
			p++;
		}
		if(*p!=']')
		{
			error(0,0,"Bad CVSROOT: Specify comma separated keyword=value pairs");
			goto error_exit;
		}
		goto new_root_ok;
	}

	if (*cvsroot_copy == ':' || (!isabsolute(cvsroot_copy) && strchr(cvsroot_copy,'@')))
    {
		char *method;
		char in_quote,escape;

		/* Access method specified, as in
		* "cvs -d :(gserver|kserver|pserver)[;params...]:[[user][:password]@]host[:[port]]/path",
		* "cvs -d [:(ext|server)[;params...]:]{access_method}[[user]@]host[:]/path",
		* "cvs -d :local[;params...]:e:\path",
		* "cvs -d :fork[;params...]:/path".
		* We need to get past that part of CVSroot before parsing the
		* rest of it.
		*/

		if(cvsroot_copy[0]==':')
		{
			method=++cvsroot_copy;
			p=method;
			in_quote=0;
			escape=0;
			while((in_quote || *p!=':') && *p)
			{
				if(escape)
				{
					escape=0;
					p++;
					continue;
				}
				if(in_quote == *p)
					in_quote=0;
				else if(!in_quote && (*p=='"' || *p=='\''))
					in_quote=*p;
				else if(!in_quote && *p=='\\')
					escape=1;
				p++;
			}
			if (!*p)
			{
				error (0, 0, "bad CVSroot: %s", root_in);
				xfree (cvsroot_save);
				goto error_exit;
			}
			*p = '\0';
			cvsroot_copy = ++p;
		}
		else
			method="ext";

		/* Process extra parameters */
		if((p=strchr(method,';'))!=NULL)
		{
			char keyword[256];

			*(p++) = '\0';

			newroot->method = xstrdup(method);

			if(newroot->method && !strcasecmp(newroot->method,"local"))
			{
				xfree(newroot->method);
				newroot->method = NULL;
			}
			else if(newroot->method)
			{
#ifdef CLIENT_SUPPORT
				client_protocol = load_protocol(newroot->method);
				if(client_protocol==NULL)
#endif
				{
					error (1, 0, "the :%s: access method is not installed on this system", newroot->method);
				}
			}

			setup_server_interface(newroot);

			while(*p && get_keyword(keyword,sizeof(keyword),&p))
			{
				if(*p!='=')
				{
					error(0,0,"Bad CVSROOT: Specify semicolon separated keyword=value pairs");
					goto error_exit;
				}
				p++;
				if(parse_keyword(keyword, &p, newroot)<0)
					goto error_exit;
				if(!*p)
					break;
				if(*p!=';')
				{
					error(0,0,"Bad CVSROOT: Specify semicolon separated keyword=value pairs");
					goto error_exit;
				}
				p++;
			}

			if(*p)
			{
				error(0,0,"Bad CVSROOT: Specify semicolon separated keyword=value pairs");
				goto error_exit;
			}

		}
		else
		{
			newroot->method = xstrdup(method);

			if(newroot->method && !strcasecmp(newroot->method,"local"))
			{
				xfree(newroot->method);
				newroot->method = NULL;
			}
			else if(newroot->method)
			{
#ifdef CLIENT_SUPPORT
				client_protocol = load_protocol(newroot->method);
				if(client_protocol==NULL)
#endif
				{
					error (1, 0, "the :%s: access method is not installed on this system", newroot->method);
				}
			}

			setup_server_interface(newroot);
		}
	}
    else
    {
		newroot->method = NULL;
    }

#ifdef CLIENT_SUPPORT
    newroot->isremote = (newroot->method != NULL);
#endif /* CLIENT_SUPPORT */

    if (newroot->method)
    {
	/* split the string into {optional_data}[[user][:password]@]host[:[port]] & /path
	 *
	 * this will allow some characters such as '@' & ':' to remain unquoted
	 * in the path portion of the spec
	 */

	/* Optional data that goes before everything else.  The format is defined only by
	   the protocol. For ext, it's an override for the CVS_RSH variable */
	if(cvsroot_copy[0]=='{')
	{
		p = strchr(cvsroot_copy+1,'}');
		if(p)
		{
			int len = p-cvsroot_copy-1;
			newroot->optional_1 = (char*)xmalloc(len+1);
			strncpy(newroot->optional_1,cvsroot_copy+1,len);
			newroot->optional_1[len]='\0';
			cvsroot_copy = p+1;
		}
	}

	p1=strchr(cvsroot_copy,'@');
	if(!p1) p1=cvsroot_copy;
	if (((p = strchr (p1, '/')) == NULL) && (p = strchr (p1, '\\')) == NULL)
	{
	    error (0, 0, "CVSROOT (\"%s\")", root_in);
	    error (0, 0, "requires a path spec");
#ifdef CLIENT_SUPPORT
	    if(client_protocol)
	    	error (0, 0, "%s",client_protocol->syntax);
#endif
	    xfree (cvsroot_save);
	    goto error_exit;
	}
	if(p>cvsroot_copy+2)
	{
		if(*(p-1)==':' && *(p-3)==':') // host:drive:/path
			p-=2;
	}
	else if(p==cvsroot_copy+2)
	{
		if(*(p-1)==':') // drive:/path
			p-=2;
	}
	firstslash = p;		/* == NULL if '/' not in string */
	char_at_slash = *p;
	*p = '\0';

	/* Check to see if there is a username[:password] in the string. */
	if ((p = strchr (cvsroot_copy, '@')) != NULL)
	{
	    *p = '\0';
	    /* check for a password */
	    if ((q = strchr (cvsroot_copy, ':')) != NULL)
	    {
		*q = '\0';
		newroot->password = xstrdup (++q);
		/* Don't check for *newroot->password == '\0' since
		 * a user could conceivably wish to specify a blank password
		 * (newroot->password == NULL means to use the
		 * password from .cvspass)
		 */
	    }

	    /* copy the username */
	    if (*cvsroot_copy != '\0')
		/* a blank username is impossible, so leave it NULL in that
		 * case so we know to use the default username
		 */
		newroot->username = xstrdup (cvsroot_copy);

	    cvsroot_copy = ++p;
	}

	/* now deal with host[:[port]] */

	/* the port */
	if ((p = strchr (cvsroot_copy, ':')) != NULL)
	{
		
	    *p++ = '\0';
	    if (strlen(p))
	    {
		q = p;
		if (*q == '-') q++;
		while (*q && !(*q==':' && !*(q+1)) && !(isalpha(*q) && *(q+1)==':' && !*(q+2)))
		{
		    if (!isdigit(*q++))
		    {
			error(0, 0, "CVSROOT (\"%s\")", root_in);
			error(0, 0, "may only specify a positive, non-zero, integer port (not \"%s\").",
				p);
			error(0, 0, "perhaps you entered a relative pathname?");
			xfree (cvsroot_save);
			goto error_exit;
		    }
		}
		*q='\0';
		if (atoi(p) <= 0)
		{
		    error (0, 0, "CVSROOT (\"%s\")", root_in);
		    error(0, 0, "may only specify a positive, non-zero, integer port (not \"%s\").",
			    p);
		    error(0, 0, "perhaps you entered a relative pathname?");
		    xfree (cvsroot_save);
		    goto error_exit;
		}
		newroot->port = xstrdup(p);
	    }
	}

	/* copy host */
	if (*cvsroot_copy != '\0')
	    /* blank hostnames are invalid, but for now leave the field NULL
	     * and catch the error during the sanity checks later
	     */
	    newroot->hostname = xstrdup (cvsroot_copy);

	/* restore the '/' */
	cvsroot_copy = firstslash;
	*cvsroot_copy = char_at_slash;
    }

    /* parse the path for all methods */
	if(newroot->isremote)
		newroot->directory = xstrdup (cvsroot_copy);
	else
	{
		const char *tmp;
		if(!root_allow_ok(cvsroot_copy,&tmp))
			tmp = cvsroot_copy;
		newroot->directory = xstrdup(tmp);
	}

new_root_ok:
	if(!newroot->isremote)
		newroot->directory = normalize_path(newroot->directory);
    newroot->unparsed_directory = xstrdup(cvsroot_copy);
    xfree (cvsroot_save);

	/* Get mapped directory, so that we're always talking the root as understood by the 
	   client, not the server */
	if(!newroot->isremote)
	{
	    struct saved_cwd cwd;
		save_cwd(&cwd);
		CVS_CHDIR(newroot->directory);
		newroot->mapped_directory = xgetwd();
		if(!fncmp(newroot->mapped_directory,newroot->directory))
			xfree(newroot->mapped_directory); /* Saves a step if there's no mapping required */
		else
			TRACE(3,"mapping %s -> %s",PATCH_NULL(newroot->mapped_directory),PATCH_NULL(newroot->directory));
		restore_cwd(&cwd, NULL);
		free_cwd(&cwd);
	}

	/* If we are local, set CVS_Username for later */
	if(!newroot->method)
		CVS_Username=xstrdup(getcaller());

    /*
     * Do various sanity checks.
     */

    check_hostname = 0;
    no_password = 0;
    no_port = 0;
    if(!newroot->method)
    {
		if (newroot->username || newroot->hostname)
		{
			error (0, 0, "can't specify hostname and username in CVSROOT");
			error (0, 0, "(\"%s\")", root_in);
			error (0, 0, "when using local access method");
			goto error_exit;
		}
	}

    if (!newroot->directory || *newroot->directory == '\0')
    {
		error (0, 0, "missing directory in CVSROOT: %s", root_in);
		goto error_exit;
    }

	/* cvs.texinfo has always told people that CVSROOT must be an
	   absolute pathname.  Furthermore, attempts to use a relative
	   pathname produced various errors (I couldn't get it to work),
	   so there would seem to be little risk in making this a fatal
	   error.  */
	if (!isabsolute_remote (newroot->directory))
	{
	    error (0, 0, "CVSROOT \"%s\" must be an absolute pathname",
		   newroot->directory);
	    goto error_exit;
	}

#ifdef CLIENT_SUPPORT
	if(client_protocol)
	{
		if((client_protocol->required_elements&elemUsername) && !newroot->username)
		{
			error(0,0,"bad CVSROOT - Username required: %s",root_in);
			goto error_exit;
		}
		if((client_protocol->required_elements&elemHostname) && !newroot->hostname)
		{
			error(0,0,"bad CVSROOT - Hostname required: %s",root_in);
			goto error_exit;
		}
		if((client_protocol->required_elements&elemPort) && !newroot->port)
		{
			error(0,0,"bad CVSROOT - Port required: %s",root_in);
			goto error_exit;
		}
		if((client_protocol->required_elements&elemTunnel) && !(newroot->proxyprotocol||newroot->proxy))
		{
			error(0,0,"bad CVSROOT - Proxy required: %s",root_in);
			goto error_exit;
		}
		if(!(client_protocol->valid_elements&elemUsername) && newroot->username)
		{
			error(0,0,"bad CVSROOT - Cannot specify username: %s",root_in);
			goto error_exit;
		}
		if(!(client_protocol->valid_elements&elemHostname) && newroot->hostname)
		{
			error(0,0,"bad CVSROOT - Cannot specify hostname: %s",root_in);
			goto error_exit;
		}
		if(!(client_protocol->valid_elements&elemPort) && newroot->port)
		{
			error(0,0,"bad CVSROOT - Cannot specify port: %s",root_in);
			goto error_exit;
		}
		if(!(client_protocol->valid_elements&elemTunnel) && (newroot->proxyprotocol||newroot->proxy))
		{
			error(0,0,"bad CVSROOT - Cannot specify proxy: %s",root_in);
			goto error_exit;
		}
		if(client_protocol->validate_details && client_protocol->validate_details(client_protocol, newroot))
		{
			error (0, 0, "bad CVSROOT: %s", root_in);
			goto error_exit;
		}
	}
#endif /* CLIENT_PROTOCOL */

    /* Hooray!  We finally parsed it! */
    return newroot;

error_exit:
    free_cvsroot_t (newroot);
    return NULL;
}

const char *get_default_port(const struct protocol_interface *protocol)
{
	struct servent *ent;
	static char p[32];

	if((ent=getservbyname("cvspserver","tcp"))!=NULL)
	{
		sprintf(p,"%u",ntohs(ent->s_port));
		return p;
	}

	return "2401";
}

#ifdef CLIENT_SUPPORT
/* Use root->username, root->hostname, root->port, and root->directory
 * to create a normalized CVSROOT fit for the .cvspass file
 *
 * username defaults to the result of getcaller()
 * port defaults to the result of get_cvs_port_number()
 *
 * FIXME - we could cache the canonicalized version of a root inside the
 * cvsroot_t, but we'd have to un'const the input here and stop expecting the
 * caller to be responsible for our return value
 */
char *normalize_cvsroot (const cvsroot_t *root)
{
    char *cvsroot_canonical;
    char *p, *hostname;
    const char *username;
    char port_s[64];

    /* get the appropriate port string */
	if(!root->port)
		sprintf (port_s, get_default_port(client_protocol));
	else
		strcpy(port_s,root->port);

    /* use a lower case hostname since we know hostnames are case insensitive */
    /* Some logic says we should be tacking our domain name on too if it isn't
     * there already, but for now this works.  Reverse->Forward lookups are
     * almost certainly too much since that would make CVS immune to some of
     * the DNS trickery that makes life easier for sysadmins when they want to
     * move a repository or the like
     */
    p = hostname = xstrdup(root->hostname);
    while (*p)
    {
	*p = tolower(*p);
	p++;
    }

    /* get the username string */
    username = root->username ? root->username : getcaller();
    cvsroot_canonical = xmalloc ( strlen(root->method) + strlen(username)
				+ strlen(hostname) + strlen(port_s)
				+ strlen(root->directory) + 12);
	sprintf (cvsroot_canonical, ":%s:%s@%s:%s:%s",
		 root->method, username, hostname, port_s, root->directory);

    xfree (hostname);
    return cvsroot_canonical;
}
#endif /* CLIENT_SUPPORT */



/* allocate and return a cvsroot_t structure set up as if we're using the local
 * repository DIR.  */
cvsroot_t *local_cvsroot (const char *dir, const char *real_dir)
{
    cvsroot_t *newroot = new_cvsroot_t();

    newroot->original = xstrdup(dir);
    newroot->method = NULL;

    newroot->directory = xstrdup(real_dir);

    newroot->unparsed_directory = xstrdup(dir);
	newroot->directory = normalize_path(newroot->directory);
	newroot->original = normalize_path(newroot->original);

    {
		struct saved_cwd cwd;
		save_cwd(&cwd);
		if(CVS_CHDIR(newroot->directory)<0)
		   error(1,errno,"Couldn't chdir to working directory %s",newroot->directory);
		newroot->mapped_directory = xgetwd();
		if(!fncmp(newroot->mapped_directory,newroot->directory))
			xfree(newroot->mapped_directory); /* Saves a step if there's no mapping required */
		else
			TRACE(3,"mapping %s -> %s",PATCH_NULL(newroot->mapped_directory),PATCH_NULL(newroot->directory));
		restore_cwd(&cwd, NULL);
    }

    return newroot;
}



#ifdef DEBUG
/* This is for testing the parsing function.  Use

     gcc -I. -I.. -I../lib -DDEBUG root.c -o root

   to compile.  */

#include <stdio.h>

char *program_name = "testing";
char *command_name = "parse_cvsroot";		/* XXX is this used??? */

/* Toy versions of various functions when debugging under unix.  Yes,
   these make various bad assumptions, but they're pretty easy to
   debug when something goes wrong.  */

void
error_exit PROTO ((void))
{
    exit (1);
}

int
isabsolute (dir)
    const char *dir;
{
    return (dir && (ISDIRSEP(*dir) || (*(dir+1)==':'));
}

void
main (argc, argv)
    int argc;
    char *argv[];
{
    program_name = argv[0];

    if (argc != 2)
    {
	fprintf (stderr, "Usage: %s <CVSROOT>\n", program_name);
	exit (2);
    }
  
    if ((current_parsed_root = parse_cvsroot (argv[1])) == NULL)
    {
	fprintf (stderr, "%s: Parsing failed.\n", program_name);
	exit (1);
    }
    printf ("CVSroot: %s\n", argv[1]);
    printf ("current_parsed_root->method: %s\n", method_names[current_parsed_root->method]);
    printf ("current_parsed_root->username: %s\n",
	    current_parsed_root->username ? current_parsed_root->username : "NULL");
    printf ("current_parsed_root->hostname: %s\n",
	    current_parsed_root->hostname ? current_parsed_root->hostname : "NULL");
    printf ("current_parsed_root->directory: %s\n", current_parsed_root->directory);

   exit (0);
   /* NOTREACHED */
}
#endif
