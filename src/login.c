/*
 * Copyright (c) 1995, Cyclic Software, Bloomington, IN, USA
 * 
 * You may distribute under the terms of the GNU General Public License as
 * specified in the README file that comes with CVS.
 * 
 * Allow user to log in for an authenticating server.
 */

#include "cvs.h"
#include "getline.h"
#include "library.h"

#ifdef CLIENT_SUPPORT   /* This covers the rest of the file. */

#ifdef HAVE_GETPASSPHRASE
#define GETPASS getpassphrase
#else
#define GETPASS getpass
#endif

/* There seems to be very little agreement on which system header
   getpass is declared in.  With a lot of fancy autoconfiscation,
   we could perhaps detect this, but for now we'll just rely on
   _CRAY, since Cray is perhaps the only system on which our own
   declaration won't work (some Crays declare the 2#$@% thing as
   varadic, believe it or not).  On Cray, getpass will be declared
   in either stdlib.h or unistd.h.  */
#ifndef _CRAY
extern char *GETPASS ();
#endif

/* Prompt for a password, and store it in the file "CVS/.cvspass".
 */

static const char *const login_usage[] =
{
    "Usage: %s %s\n",
    "(Specify the --help global option for a list of other help options)\n",
    NULL
};

int
login (argc, argv)
    int argc;
    char **argv;
{
    char *typed_password;
    char *cvsroot_canonical;

    if (argc < 0)
		usage (login_usage);

	if(!client_protocol || !client_protocol->login)
	{
		error(1,0,"The :%s: protocol does not support the login command",(current_parsed_root&&current_parsed_root->method)?current_parsed_root->method:"local");
	}

    cvsroot_canonical = normalize_cvsroot(current_parsed_root);
    printf ("Logging in to %s\n", cvsroot_canonical);
    fflush (stderr);
    fflush (stdout);
    xfree (cvsroot_canonical);

    if (current_parsed_root->password)
		typed_password = current_parsed_root->password;
    else
		typed_password = GETPASS ("CVS password: ");

	if(client_protocol->login(client_protocol,typed_password))
		return 1;
	if(start_server(1)) /* Verify the new password */
		return 1;
	return 0;
}

static const char *const logout_usage[] =
{
    "Usage: %s %s\n",
    "(Specify the --help global option for a list of other help options)\n",
    NULL
};

/* Remove any entry for the CVSRoot repository found in .cvspass. */
int logout (argc, argv)
    int argc;
    char **argv;
{
    char *cvsroot_canonical;

    if (argc < 0)
	usage (logout_usage);

	if(!client_protocol || !client_protocol->logout)
	{
		error(1,0,"The :%s: protocol does not support the logout command",(current_parsed_root&&current_parsed_root->method)?current_parsed_root->method:"local");
	}

    /* Hmm.  Do we want a variant of this command which deletes _all_
       the entries from the current .cvspass?  Might be easier to
       remember than "rm ~/.cvspass" but then again if people are
       mucking with HOME (common in Win95 as the system doesn't set
       it), then this variant of "cvs logout" might give a false sense
       of security, in that it wouldn't delete entries from any
       .cvspass files but the current one.  */

    if (!quiet)
    {
		cvsroot_canonical = normalize_cvsroot(current_parsed_root);
		printf ("Logging out of %s\n", cvsroot_canonical);
		fflush (stderr);
		fflush (stdout);
		xfree (cvsroot_canonical);
    }

	if(client_protocol->logout(client_protocol))
		return 1;
    return 0;
}

#endif /* CLIENT_SUPPORT from beginning of file. */
