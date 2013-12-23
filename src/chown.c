/*
 * Copyright (c) 1992, Brian Berliner and Jeff Polk
 * Copyright (c) 1989-1992, Brian Berliner
 * 
 * You may distribute under the terms of the GNU General Public License as
 * specified in the README file that comes with the CVS 1.4 kit.
 * 
 * chown
 * 
 * Changes the owner of a directory to the given name.
 */

#include "cvs.h"

static const char *const chown_usage[] =
{
    "Usage: %s %s user directory...\n",
    NULL
};

int
chowner (argc, argv)
   int argc;
   char **argv;
{
   char *user;
   int i;
   char *repository;
   int c;
   int err = 0;
   char *new_argv[2]; /* Used if no filenames are specified. */


   if (argc == 1 || argc == -1)
      usage (chown_usage);

   optind = 0;
   while ((c = getopt (argc, argv, "")) != -1)
   {
      switch (c)
      {
      case '?':
      default:
	 usage (chown_usage);
	 break;
      }
   }
   argc -= optind;
   argv += optind;

   if (argc <= 0)
      usage (chown_usage);

   if (argc == 1)
   {
       /* We have no filenames specified, default to the current directory. */

       new_argv[0] = argv[0];
       new_argv[1] = ".";
       argv = new_argv;
       argc++;
   }

   /* find the repository associated with our current dir */
   repository = Name_Repository ((char *) NULL, (char *) NULL);

#ifdef CLIENT_SUPPORT
   if (current_parsed_root->isremote)
   {
	  if (!supported_request ("chown"))
	    error (1, 0, "server does not support chown");

      send_arg("--");
	  send_arg (argv[0]); /* Send the user name */
      argc--;
      argv++;
      send_file_names (argc, argv, SEND_EXPAND_WILD);
      send_files (argc, argv, 0, 0, SEND_NO_CONTENTS);
      send_to_server ("chown\012", 0);
      return get_responses_and_close ();
   }
#endif

   user = argv[0];

   /* walk the arg list checking files/dirs */
   for (i = 1; i < argc; i++)
   {
      char *dname;
      dname = xmalloc(strlen(repository)
		      + strlen(argv[i])
		      + 2);
      (void) sprintf (dname, "%s/%s", repository, argv[i]);
      if (!isdir (dname))
      {
	 error (0, 0, "`%s' is not a directory", dname);
	 err++;
      }
      else
      {
	 if (!verify_owner (dname))
	 {
	    error (0, 0, "'%s' does not own '%s'\n", CVS_Username, argv[i]);
	    err++;
	 }
	 else
	 {
			char   *linebuf = NULL;

			init_passwd_list();
			read_passwd_list();

			if(!does_passwd_user_exist(user))
			{
			  error (0, 0, "User %s does not exist", user);
			}
			else
			{
			   error (0,0, "Changing owner of %s to %s", argv[i], user);
			   change_owner (dname, user);
			}
			free_passwd_list();
		 }
      }
      xfree(dname);
   }
   return (err);
}
