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

static int ls_proc (int argc, char **argv, char *xwhere, char *mwhere, char *mfile, int shorten, int local, char *mname, char *msg);
static int ls_fileproc(void *callerdat, struct file_info *finfo);
static Dtype ls_direntproc(void *callerdat, char *dir,
				      char *repos, char *update_dir,
				      List *entries, const char *virtual_repository, Dtype hint);

static RCSNode *xrcsnode;

static const char *const ls_usage[] =
{
    "Usage: %s %s [-q] [-e] [-l] [-R] [-r rev] [-D date] [-t] [modules...]\n",
	"\t-q\tQuieter output.\n",
    "\t-e\tDisplay in CVS/Entries format.\n",
    "\t-l\tDisplay all details.\n",
	"\t-R\tList recursively.\n",
    "\t-r rev\tShow files with revision or tag.\n",
    "\t-D date\tShow files from date.\n",
    "\t-T\tShow time in local time instead of GMT.\n",
    "(Specify the --help global option for a list of other help options)\n",
    NULL
};

static int entries_format;
static int long_format;
static int quiet;
static char *show_tag;
static char *show_date;
static int tag_validated;
static int recurse;
static char *regexp_match;
static int local_time;
static int local_time_offset;

int
ls(argc, argv)
    int argc;
    char **argv;
{
    int c;
    int err = 0;

    if (argc == -1)
		usage (ls_usage);

	entries_format = 0;
	long_format = 0;
	show_tag = NULL;
	show_date = NULL;
	tag_validated = 0;
	quiet = 0;
	recurse = 0;

    local_time_offset = get_local_time_offset();

    optind = 0;
    while ((c = getopt (argc, argv, "qelr:D:RTo:")) != -1)
    {
	switch (c)
	{
	case 'q':
		quiet = 1;
		break;
	case 'e':
		entries_format = 1;
		break;
	case 'l':
		long_format = 1;
		break;
	case 'r':
		show_tag = optarg;
		break;
	case 'D':
		show_date = Make_Date (optarg);
		break;
	case 'R':
		recurse = 1;
		break;
	case 'T':
		local_time = 1;
		break;
	case 'o':
		local_time_offset = atoi(optarg);
		break;
    case '?':
    default:
		usage (ls_usage);
		break;
	}
    }
    argc -= optind;
    argv += optind;

#ifdef CLIENT_SUPPORT
    if (current_parsed_root->isremote)
    {
	if (!supported_request ("ls"))
		error (1, 0, "server does not support %s",command_name);

	if(quiet)
		send_arg("-q");
	if(entries_format)
		send_arg("-e");
	if(long_format)
		send_arg("-l");
	if(recurse)
		send_arg("-R");
	if(local_time)
	{
		char tmp[64];
		send_arg("-T");
		sprintf(tmp,"%d",local_time_offset);
		option_with_arg("-o",tmp);
	}
	if(show_tag)
		option_with_arg("-r",show_tag);
	if(show_date)
		option_with_arg("-D",show_date);

	send_arg("--");
	{
    int i;
	for (i = 0; i < argc; i++)
#ifdef SJIS
		send_arg_fconv (argv[i]);
#else
		send_arg (argv[i]);
#endif
	}

	send_to_server ("ls\012", 0);

	err = get_responses_and_close ();

	return err;
    }
#endif

    {
	DBM *db;
	int i;
	db = open_module ();
	if(argc)
	{
		for (i = 0; i < argc; i++)
		{
			char *mod = xstrdup(argv[i]);
			char *p;
#ifdef SJIS
			for(p=_mbschr(mod,'\\'); p; p=_mbschr(p,'\\'))
#else
			for(p=strchr(mod,'\\'); p; p=strchr(p,'\\'))
#endif
				*p='/';

			p = strrchr(mod,'/');
			if(p && (strchr(p,'?') || strchr(p,'*')))
			{
				*p='\0';
				regexp_match = p+1;
			}
			else
				regexp_match = NULL;

			/* Frontends like to do 'ls -q /', so we support it explicitly */
			if(!strcmp(mod,"/"))
			{
				*mod='\0';
			}

			err += do_module (db, mod, MISC,
					  "Listing",
					  ls_proc, (char *) NULL, 0, 0, 0, 0, (char*)NULL);

			xfree(mod);
		}
	}
	else
	{
		err += do_module (db, ".", MISC,
				  "Listing",
				  ls_proc, (char *) NULL, 0, 0, 0, 0, (char*)NULL);
	}
	close_module (db);
    }

    return (err);
}

static int
ls_proc (int argc, char **argv, char *xwhere, char *mwhere, char *mfile,
		 int shorten, int local, char *mname, char *msg)
{
    /* Begin section which is identical to patch_proc--should this
       be abstracted out somehow?  */
    char *myargv[2];
    int err = 0;
    int which;
    char *repository, *mapped_repository;
    char *where;

	if(!quiet)
	{
		if(strcmp(mname,"."))
		  {
		   cvs_outerr("Listing module: ", 0);
		   cvs_outerr(mname, 0);
		   cvs_outerr("\n\n", 0);
		  }
	   else
		   cvs_outerr("Listing modules on server\n\n", 0);

	}

	repository = xmalloc (strlen (current_parsed_root->directory) + strlen (argv[0])
			      + (mfile == NULL ? 0 : strlen (mfile) + 1) + 2);
	(void) sprintf (repository, "%s/%s", current_parsed_root->directory, argv[0]);
	where = xmalloc (strlen (argv[0]) + (mfile == NULL ? 0 : strlen (mfile) + 1)
			 + 1);
	(void) strcpy (where, argv[0]);

	/* if mfile isn't null, we need to set up to do only part of the module */
	if (mfile != NULL)
	{
	    char *cp;
	    char *path;

	    /* if the portion of the module is a path, put the dir part on repos */
	    if ((cp = strrchr (mfile, '/')) != NULL)
	    {
		*cp = '\0';
		(void) strcat (repository, "/");
		(void) strcat (repository, mfile);
		(void) strcat (where, "/");
		(void) strcat (where, mfile);
		mfile = cp + 1;
	    }

	    /* take care of the rest */
	    path = xmalloc (strlen (repository) + strlen (mfile) + 5);
	    (void) sprintf (path, "%s/%s", repository, mfile);
	    if (isdir (path))
	    {
		/* directory means repository gets the dir tacked on */
		(void) strcpy (repository, path);
		(void) strcat (where, "/");
		(void) strcat (where, mfile);
	    }
	    else
	    {
		myargv[0] = argv[0];
		myargv[1] = mfile;
		argc = 2;
		argv = myargv;
	    }
	    xfree (path);
	}

	mapped_repository = map_repository(repository);

	/* cd to the starting repository */
	if ( CVS_CHDIR (mapped_repository) < 0)
	{
	    error (0, errno, "cannot chdir to %s", fn_root(repository));
	    xfree (repository);
	    xfree (mapped_repository);
	    return (1);
	}
	xfree (repository);
	xfree (mapped_repository);
	/* End section which is identical to patch_proc.  */

	if (show_tag)
	    which = W_REPOS | W_ATTIC;
	else
	    which = W_REPOS;
	repository = NULL;

    if (show_tag != NULL && !tag_validated)
    {
	tag_check_valid (show_tag, argc - 1, argv + 1, local, 0, repository);
	tag_validated = 1;
    }

    err = start_recursion (ls_fileproc, (FILESDONEPROC) NULL,(PREDIRENTPROC) NULL,
			   ls_direntproc, (DIRLEAVEPROC) NULL, NULL,
			   argc - 1, argv + 1, local, which, 0, 1,
			   where, 1, verify_read);

	if(!strcmp(mname,"."))
	{
		DBM *db;
	    if (db = open_module ())
		{
			datum key = dbm_firstkey(db);
			if(key.dptr)
			{
				cvs_output("\n",1);
				if(!quiet)
					cvs_outerr("Virtual modules on server (CVSROOT/modules file)\n\n",0);
				cat_module(0);
			}
			close_module(db);
		}
	}
    return err;
}


/*
 * display the status of a file
 */
/* ARGSUSED */
static int
ls_fileproc(void *callerdat, struct file_info *finfo)
{
	Vers_TS *vers;
	char outdate[32],tag[64];
	time_t t;

	if(regexp_match && !regex_filename_match(regexp_match,finfo->file))
		return 0;

    vers = Version_TS (finfo, NULL, show_tag, show_date, 1, 0, 0);
	if(!vers->vn_rcs)
	{
		freevers_ts(&vers);
		return 0;
	}

	if(RCS_isdead(finfo->rcs, vers->vn_rcs))
	{
		freevers_ts(&vers);
		return 0;
	}

	t=RCS_getrevtime (finfo->rcs, vers->vn_rcs, 0, 0);
	
	if(local_time)
		t+=local_time_offset;
	strcpy(outdate,asctime(gmtime(&t)));
	outdate[strlen(outdate)-1]='\0';

	if(entries_format)
	{
		tag[0]='\0';
		if(show_tag)
			sprintf(tag,"T%s",show_tag);

		printf("/%s/%s/%s/%s/%s\n",finfo->file,vers->vn_rcs,outdate,vers->options,tag);
	}
	else if(long_format)
	{
		printf("%-32.32s%-8.8s%s %s\n",finfo->file,vers->vn_rcs,outdate,vers->options);
	}
	else
		printf("%s\n",finfo->file);

	freevers_ts(&vers);
	return 0;
}

Dtype ls_direntproc(void *callerdat, char *dir,
				      char *repos, char *update_dir,
				      List *entries, const char *virtual_repository, Dtype hint)
{
	if(hint!=R_PROCESS)
		return hint;

	if(!strcasecmp(dir,"."))
		return R_PROCESS;
	if(recurse)
	{
		printf("\nDirectory %s\n\n",update_dir);
		return R_PROCESS;
	}
	if(entries_format)
	{
		printf("D/%s////\n",dir);
	}
	else if(long_format)
	{
		printf("%-32.32s(directory)\n",dir);
	}
	else
		printf("%s\n",dir);
	return R_SKIP_ALL;
}

