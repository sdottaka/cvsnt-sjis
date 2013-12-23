/*
 * Copyright (c) 1992, Brian Berliner and Jeff Polk
 * Copyright (c) 1989-1992, Brian Berliner
 * 
 * You may distribute under the terms of the GNU General Public License as
 * specified in the README file that comes with the CVS 1.4 kit.
 * 
 * lsacl
 * 
 * Shows the current permissions
 */

#include "cvs.h"

static const char *const lsacl_usage[] =
{
    "Usage: %s %s [directory...]\n",
    NULL
};

int
lsacl (argc, argv)
   int argc;
   char **argv;
{
#ifdef SERVER_SUPPORT
   int i;
#endif
   char *repository;
   int c;
   int err = 0;
   char *new_argv[1]; /* Used if no filenames are specified. */
   int local = 1;

   if (argc == -1)
      usage (lsacl_usage);

   optind = 0;
   while ((c = getopt (argc, argv, "")) != -1)
   {
      switch (c)
      {
      case '?':
      default:
	 usage (lsacl_usage);
	 break;
      }
   }
   argc -= optind;
   argv += optind;

   if (argc < 0)
      usage (lsacl_usage);

   if (argc == 0)
   {
       /* We have no filenames specified, default to the current directory. */

       new_argv[0] = ".";
       argv = new_argv;
       argc++;
   }

   /* find the repository associated with our current dir */
   repository = Name_Repository ((char *) NULL, (char *) NULL);

#ifdef CLIENT_SUPPORT
   if (current_parsed_root->isremote)
   {
	  if (!supported_request ("lsacl"))
	    error (1, 0, "server does not support lsacl");

      send_file_names (argc, argv, SEND_EXPAND_WILD);
      send_files (argc, argv, local, 0, SEND_NO_CONTENTS);
      send_to_server ("lsacl\012", 0);
      return get_responses_and_close ();
   }
#endif

#ifdef SERVER_SUPPORT
   /* walk the arg list checking files/dirs */
   for (i = 0; i < argc; i++)
   {
      char *dname;
      dname = xmalloc(strlen(repository)
		      + strlen(argv[i])
		      + 2);
      (void) sprintf (dname, "%s/%s", repository, argv[i]);
      if (!isdir (dname))
      {
	 error (0, 0, "`%s' is not a directory", fn_root(dname));
	 err++;
      }
      else
      {
	 if (!verify_owner(dname) && !verify_read (dname,NULL))
	 {
	    error (0, 0, "'%s' cannot list '%s'\n", CVS_Username?CVS_Username:getlogin(), argv[i]);
	    err++;
	 }
	 else
	 {
	    /*
	     * Changed it so that the output makes more sense ... permissions
	     * exist on Directories .. not files.
 	     */
	    cvs_output ("Directory: ", 0);
	    cvs_output (argv[i], 0);
	    cvs_output ("\n", 0);
	    list_owner (dname);
	    list_perms (dname);
	 }
      }
      xfree(dname);
   }

   return (err);
#else
   error (1, 0, "Server support is not available");
   return (0);
#endif /* SERVER_SUPPORT */
}
