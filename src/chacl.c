/*
 * Copyright (c) 1992, Brian Berliner and Jeff Polk
 * Copyright (c) 1989-1992, Brian Berliner
 * 
 * You may distribute under the terms of the GNU General Public License as
 * specified in the README file that comes with the CVS 1.4 kit.
 * 
 * chacl
 * 
 * Sets the permission for the specified user for the directory
 */

#include "cvs.h"

static const char *const chacl_usage[] =
{
    "Usage: %s %s -R [-r tag] {user|default}:[{[r][w][c]|[n]}] [directory...]\n",
    "\t-R\tRecursively set permissions\n",
    "\t-r\tSet permissions on specific branch\n",
    NULL
};

#ifdef SERVER_SUPPORT

static char *g_rtag;
static char *g_user;
static char *g_permptr;

static Dtype chacl_dirproc (void *callerdat, char *dir, char *repos, char *update_dir,  List *entries, const char *virtual_repository, Dtype hint)
{
	int err;

	if(hint!=R_PROCESS)
		return hint;

	if (!verify_owner (repos))
	{
		error (0, 0, "'%s' does not own '%s'\n", CVS_Username, dir);
		err = 1;
	}
	else
	{
		err = change_perms (repos, g_user, g_permptr, g_rtag);
	}
	return err;
}

#endif

int
check_perms(perm)
   char *perm;
{
   int foundc, foundr, foundw, foundn;

   if (strlen(perm) > 3)
      return 0;

   foundc = 0,
   foundr = 0;
   foundw = 0;
   foundn = 0;
   while (*perm != '\0')
   {
      switch (*perm)
      {
      case 'c':
	 if (foundn)
		 return 0;
	 if (foundc)
	    return 0;
	 foundc = 1;
	 break;

      case 'r':
	 if (foundn)
		 return 0;
	 if (foundr)
	    return 0;
	 foundr = 1;
	 break;

      case 'w':
	 if (foundn)
		 return 0;
	 if (foundw)
	    return 0;
	 foundw = 1;
	 break;
	 case 'n':
	 if (foundn)
		 return 0;
	 if(foundc|foundr|foundw)
		 return 0;
	 foundn=1;
	 break;
      default:
	 return 0;
      }
      perm++;
   }

   return 1;
}

int
chacl (argc, argv)
   int argc;
   char **argv;
{
#ifdef SERVER_SUPPORT
   char *user;
   char *permptr;
#endif
   char *repository;
   int c;
   int err = 0;
   char *new_argv[2]; /* Used if no filenames are specified. */
   int usetag = 0;
   int recurse = 0;
   char * rtag = NULL;

   if (argc == 1 || argc == -1)
      usage (chacl_usage);

   optind = 0;
   while ((c = getopt (argc, argv, "Rr:")) != -1)
   {
      switch (c)
      {
	  case 'r':
		  rtag = xstrdup(optarg);
		  usetag = 1;
		  break;
	  case 'R':
		  recurse = 1;
		  break;
      case '?':
	  default:
		 usage (chacl_usage);
		 break;
      }
   }
   argc -= optind;
   argv += optind;

   if (argc <= 0)
      usage (chacl_usage);

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
      if (!supported_request ("chacl"))
           error (1, 0, "server does not support chacl");

	  if(recurse)
		  send_arg("-R");
	  if(usetag)
	  {
		  send_arg("-r");
		  send_arg(rtag);
	  }
      send_arg (argv[0]); /* Send the user name and permissions */
      argc--;
      argv++;
      send_files (argc, argv, !recurse, 0, SEND_NO_CONTENTS | SEND_DIRECTORIES_ONLY);
      send_file_names (argc, argv, SEND_EXPAND_WILD | SEND_DIRECTORIES_ONLY);
      send_to_server ("chacl\012", 0);
      return get_responses_and_close ();
   }
#endif

#ifdef SERVER_SUPPORT
   user = strtok (argv[0], ":");
   permptr = strtok (NULL, "");

   if ((permptr != NULL) && (strlen(permptr) == 0))
   {
      permptr = NULL;
   }

   if (permptr == NULL)
   {
      /* Ok, nothing to do. */
   }
   else if  (! check_perms(permptr))
   {
      error(1, 0, "Invalid permissions: '%s', can only have r, w, and c",
	    permptr);
   }

   g_rtag = rtag;
   g_permptr = permptr;
   g_user = user;
    err = start_recursion (NULL, NULL, (PREDIRENTPROC) NULL, chacl_dirproc, NULL, (void*)NULL,
		argc, argv, !recurse, W_LOCAL, 0, 0, (char*)NULL, 1, NULL);

   xfree(rtag);

   return (err);
#else
   error (1, 0, "Server support is not available");
   return (0);
#endif /* SERVER_SUPPORT */
}
