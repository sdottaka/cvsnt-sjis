/*
 * Copyright (c) 1992, Brian Berliner and Jeff Polk
 * Copyright (c) 1989-1992, Brian Berliner
 * 
 * You may distribute under the terms of the GNU General Public License as
 * specified in the README file that comes with the CVS source distribution.
 * 
 * "import" checks in the vendor release located in the current directory into
 * the CVS source repository.  The CVS vendor branch support is utilized.
 * 
 * At least three arguments are expected to follow the options:
 *	repository	Where the source belongs relative to the CVSROOT
 *	VendorTag	Vendor's major tag
 *	VendorReleTag	Tag for this particular release
 *
 * Additional arguments specify more Vendor Release Tags.
 */

#include "cvs.h"
#include "savecwd.h"

#include <zlib.h>
/* for ntohl */
#if defined(_WIN32)
  #include <winsock2.h>
#else
  #include <netinet/in.h>
#endif

static char *get_comment PROTO((char *user));
static int add_rev PROTO((char *message, RCSNode *rcs, char *vfile,
			  char *vers));
static int add_tags PROTO((RCSNode *rcs, char *vfile, char *vtag, int targc,
		     char *targv[]));
static int import_descend PROTO((char *message, char *vtag, int targc, char *targv[]));
static int import_descend_dir PROTO((char *message, char *dir, char *vtag,
			       int targc, char *targv[]));
static int process_import_file PROTO((char *message, char *vfile, char *vtag,
				int targc, char *targv[]));
static int update_rcs_file PROTO((char *message, char *vfile, char *vtag, int targc,
			    char *targv[]));
static void add_log PROTO((int ch, char *fname));
static int do_import_repository(const char *repos);

static int repos_len;
static char *vhead;
static char *vbranch;
static FILE *logfp;
static char *repository;
static int conflicts;
static int use_file_modtime;
static int create_cvs_dirs;
static char *keyword_opt = NULL;
static char *current_date = NULL;
static int force_tags;

static const char *const import_usage[] =
{
    "Usage: %s %s [-C] [-d] [-f] [-k subst] [-I ign] [-m msg] [-b branch]\n",
	"    [-W spec] [-n] repository [vendor-tag] [release-tags...]\n",
    "\t-C\tCreate CVS directories while importing.\n",
    "\t-d\tUse the file's modification time as the time of import.\n",
    "\t-f\tOverwrite existing release tags.\n",
    "\t-k sub\tSet default RCS keyword substitution mode.\n",
    "\t-I ign\tMore files to ignore (! to reset).\n",
    "\t-b bra\tVendor branch id.\n",
    "\t-m msg\tLog message.\n",
    "\t-W spec\tWrappers specification line.\n",
	"\t-n\tDon't create vendor branch or release tags.\n",
    "(Specify the --help global option for a list of other help options)\n",
    NULL
};

int
import (argc, argv)
    int argc;
    char **argv;
{
    char *message = NULL;
    char *tmpfile;
    char *cp;
    int i, c, msglen, err;
    List *ulist;
    Node *p;
#ifdef SERVER_SUPPORT
    char *existing_repos_dir;
#endif
    struct logfile_info *li;
	int vargc;
	char *vtag, **vargv;

    if (argc == -1)
	usage (import_usage);

    vbranch = xstrdup (CVSBRANCH);
    optind = 0;
    while ((c = getopt (argc, argv, "+Qqdb:Cm:I:k:W:nf")) != -1)
    {
	switch (c)
	{
	    case 'Q':
	    case 'q':
		/* The CVS 1.5 client sends these options (in addition to
		   Global_option requests), so we must ignore them.  */
		if (!server_active)
		    error (1, 0,
			   "-q or -Q must be specified before \"%s\"",
			   command_name);
		break;
	    case 'd':
		if (server_active)
		{
		    /* CVS 1.10 and older clients will send this, but it
		       doesn't do any good.  So tell the user we can't
		       cope, rather than silently losing.  */
		    error (0, 0,
			   "warning: not setting the time of import from the file");
		    error (0, 0, "due to client limitations");
		}
		use_file_modtime = 1;
		break;
	    case 'b':
		xfree (vbranch);
		vbranch = xstrdup (optarg);
		break;
		case 'n':
			xfree(vbranch);
		break;
	    case 'm':
#ifdef FORCE_USE_EDITOR
		use_editor = 1;
#else
		use_editor = 0;
#endif
		message = xstrdup(optarg);
		break;
	    case 'I':
		ign_add (optarg, 0);
		break;
            case 'k':
		/* RCS_check_kflag returns strings of the form -kxx.  We
		   only use it for validation, so we can free the value
		   as soon as it is returned. */
		free(RCS_check_kflag (optarg)); /* can't use xfree here */
		keyword_opt = optarg;
		break;
	    case 'W':
		wrap_add (optarg, 0, 0);
		break;
	    case 'C':
		create_cvs_dirs = 1;
		break;
		case 'f':
		force_tags = 1;
		break;
	    case '?':
	    default:
		usage (import_usage);
		break;
	}
    }
    argc -= optind;
    argv += optind;
    if ((vbranch==NULL && argc<1) || (vbranch!=NULL && argc < 2))
		usage (import_usage);

    /* This is for handling the Checkin-time request.  It might seem a
       bit odd to enable the use_file_modtime code even in the case
       where Checkin-time was not sent for a particular file.  The
       effect is that we use the time of upload, rather than the time
       when we call RCS_checkin.  Since those times are both during
       CVS's run, that seems OK, and it is easier to implement than
       putting the "was Checkin-time sent" flag in CVS/Entries or some
       such place.  */

    if (server_active)
		use_file_modtime = 1;

    for (i = 1; i < argc; i++)		/* check the tags for validity */
    {
	int j;

	RCS_check_tag (argv[i]);
	for (j = 1; j < i; j++)
	    if (strcmp (argv[j], argv[i]) == 0)
		error (1, 0, "tag `%s' was specified more than once", argv[i]);
    }

    /* XXX - this should be a module, not just a pathname */
    if (! isabsolute (argv[0])
	&& pathname_levels (argv[0]) == 0)
    {
	if (current_parsed_root == NULL)
	{
	    error (0, 0, "missing CVSROOT environment variable\n");
	    error (1, 0, "Set it or specify the '-d' option to %s.",
		   program_name);
	}
	repository = xmalloc (strlen (current_parsed_root->directory)
			      + strlen (argv[0])
			      + 2);
	sprintf (repository, "%s/%s", current_parsed_root->directory, argv[0]);
	repos_len = strlen (current_parsed_root->directory);
    }
    else
    {
	/* It is somewhere between a security hole and "unexpected" to
	   let the client start mucking around outside the cvsroot
	   (wouldn't get the right CVSROOT configuration, &c).  */
	error (1, 0, "directory %s not relative within the repository",
	       argv[0]);
    }

	if(vbranch)
	{
		/*
		* Consistency checks on the specified vendor branch.  It must be
		* composed of only numbers and dots ('.').  Also, for now we only
		* support branching to a single level, so the specified vendor branch
		* must only have two dots in it (like "1.1.1").
		*/
		for (cp = vbranch; *cp != '\0'; cp++)
			if (!isdigit ((unsigned char) *cp) && *cp != '.')
				error (1, 0, "%s is not a numeric branch", vbranch);
		if (numdots (vbranch) != 2)
			error (1, 0, "Only branches with two dots are supported: %s", vbranch);
		vhead = xstrdup (vbranch);
		cp = strrchr (vhead, '.');
		*cp = '\0';
	}
	else
		vhead = xstrdup("1.1");

    if (use_editor)
    {
		do_editor ((char *) NULL, &message, repository,
		   (List *) NULL);
    }
    do_verify (&message, repository);
    msglen = message == NULL ? 0 : strlen (message);
    if (msglen == 0 || message[msglen - 1] != '\n')
    {
		char *nm = xmalloc (msglen + 2);
		*nm = '\0';
		if (message != NULL)
		{
			strcpy (nm, message);
			xfree (message);
		}
		strcat (nm + msglen, "\n");
		message = nm;
    }

#ifdef CLIENT_SUPPORT
    if (current_parsed_root->isremote)
    {
	int err;

	if(vbranch == NULL)
		send_arg("-n");
	else if (vbranch[0] != '\0')
	    option_with_arg ("-b", vbranch);
	if (message)
#ifdef SJIS
	{
		if (current_parsed_root->message_encoding)
		{
			char *pbuf;
			size_t len;
			if (trace)
				(void) fprintf (stderr, "kanji convert\n");
			transcode_buffer(get_local_charset(),
				current_parsed_root->message_encoding,
				message, 0, &pbuf, &len);
			xfree(message);
			message = pbuf;
		}
#endif
	    option_with_arg ("-m", message);
#ifdef SJIS
	}
#endif
	if (keyword_opt != NULL)
	    option_with_arg ("-k", keyword_opt);
	if(force_tags)
		send_arg("-f");
/*	if (create_cvs_dirs)
		send_arg("-C"); */
	ign_send ();
	wrap_send ();

	{
	    int i;
	    for (i = 0; i < argc; ++i)
		send_arg (argv[i]);
	}

	logfp = stdin;
	client_import_setup (repository);
	err = import_descend (message, argv[1], argc - 2, argv + 2);
	client_import_done ();
	if (message)
	    xfree (message);
	xfree (repository);
	xfree (vbranch);
	xfree (vhead);
	send_to_server ("import\012", 0);
	err += get_responses_and_close ();
	return err;
    }
#endif

    if (!safe_location ())
    {
	error (1, 0, "attempt to import the repository");
    }

#ifdef SERVER_SUPPORT
    /* We search for an existing repository directory when importing, since
       the passed in directories will not exist yet. */
    existing_repos_dir = xstrdup(repository);
    while (! isdir(existing_repos_dir)) {
	cp = strrchr(existing_repos_dir, '/');
	if (cp == NULL || cp == existing_repos_dir)
	    error (1, 0, "User %s cannot create files in %s",
		   CVS_Username, argv[0]);
	*cp = '\0';
    }
    if (! verify_create (existing_repos_dir,NULL))
	error (1, 0, "User %s cannot create files in %s",
	       CVS_Username, argv[0]);
    xfree(existing_repos_dir);
#endif

    /*
     * Make all newly created directories writable.  Should really use a more
     * sophisticated security mechanism here.
     */
    umask (cvsumask);
    make_directories (repository);
	change_owner(repository, CVS_Username);

    /* Create the logfile that will be logged upon completion */
    if ((logfp = cvs_temp_file (&tmpfile)) == NULL)
	error (1, errno, "cannot create temporary file `%s'", tmpfile);
    /* On systems where we can unlink an open file, do so, so it will go
       away no matter how we exit.  FIXME-maybe: Should be checking for
       errors but I'm not sure which error(s) we get if we are on a system
       where one can't unlink open files.  */
    unlink (tmpfile);
	if(vbranch)
		fprintf (logfp, "\nVendor Tag:\t%s\n", argv[1]);
	fprintf (logfp, "Release Tags:\t");
	for (i = vbranch?2:1; i < argc; i++)
 	   fprintf (logfp, "%s\n\t\t", argv[i]);
	fprintf (logfp, "\n");


	current_date = date_from_time_t(time(NULL));
    /* Just Do It.  */
	if(vbranch)
	{
		vtag = argv[1];
		vargc = argc-2;
		vargv = argv+2;
	}
	else
	{
		vtag = NULL;
		vargc=argc-1;
		vargv=argv+1;
	}
    err = import_descend (message, vtag, vargc, vargv);
	xfree(current_date);
    if (conflicts)
    {
	if (!really_quiet)
	{
	    char buf[20];
	    char *buf2;

	    cvs_output_tagged ("+importmergecmd", NULL);
	    cvs_output_tagged ("newline", NULL);
	    sprintf (buf, "%d", conflicts);
	    cvs_output_tagged ("conflicts", buf);
	    cvs_output_tagged ("text", " conflicts created by this import.");
	    cvs_output_tagged ("newline", NULL);
	    cvs_output_tagged ("text",
			       "Use the following command to help the merge:");
	    cvs_output_tagged ("newline", NULL);
	    cvs_output_tagged ("newline", NULL);
	    cvs_output_tagged ("text", "\t");
	    cvs_output_tagged ("text", program_name);
	    if (CVSroot_cmdline != NULL)
	    {
		cvs_output_tagged ("text", " -d ");
		cvs_output_tagged ("text", CVSroot_cmdline);
	    }
	    cvs_output_tagged ("text", " checkout -j");
	    buf2 = xmalloc (strlen (argv[1]) + 20);
	    sprintf (buf2, "%s:yesterday", argv[1]);
	    cvs_output_tagged ("mergetag1", buf2);
	    xfree (buf2);
	    cvs_output_tagged ("text", " -j");
	    cvs_output_tagged ("mergetag2", argv[1]);
	    cvs_output_tagged ("text", " ");
	    cvs_output_tagged ("repository", argv[0]);
	    cvs_output_tagged ("newline", NULL);
	    cvs_output_tagged ("newline", NULL);
	    cvs_output_tagged ("-importmergecmd", NULL);
	}

	/* FIXME: I'm not sure whether we need to put this information
           into the loginfo.  If we do, then note that it does not
           report any required -d option.  There is no particularly
           clean way to tell the server about the -d option used by
           the client.  */
	fprintf (logfp, "\n%d conflicts created by this import.\n",
			conflicts);
	fprintf (logfp,
			"Use the following command to help the merge:\n\n");
	fprintf (logfp, "\t%s checkout ", program_name);
	fprintf (logfp, "-j%s:yesterday -j%s %s\n\n",
			argv[1], argv[1], argv[0]);
    }
    else 
    {	
		if (!really_quiet)
			cvs_output ("\nNo conflicts created by this import\n\n", 0);
		fprintf (logfp, "\nNo conflicts created by this import\n\n");
		if(err)
		{
			if (!really_quiet)
				cvs_output ("\n*** ERRORS DURING THIS IMPORT ***\n\n", 0);
		}
    }

    /*
     * Write out the logfile and clean up.
     */
    ulist = getlist ();
    p = getnode ();
    p->type = UPDATE;
    p->delproc = update_delproc;
    p->key = xstrdup ("- Imported sources");
    li = (struct logfile_info *) xmalloc (sizeof (struct logfile_info));
    li->type = T_TITLE;
	li->tag = vbranch?xstrdup (vbranch):NULL;
    li->rev_old = li->rev_new = NULL;
    p->data = (char *) li;
    addnode (ulist, p);
    Update_Logfile (repository, message, logfp, ulist);
    dellist (&ulist);
    if (fclose (logfp) < 0)
	error (0, errno, "error closing %s", tmpfile);

    /* Make sure the temporary file goes away, even on systems that don't let
       you delete a file that's in use.  */
    if (unlink (tmpfile) < 0 && !existence_error (errno))
	error (0, errno, "cannot remove %s", tmpfile);
    xfree (tmpfile);

    if (message)
	xfree (message);
    xfree (repository);
    xfree (vbranch);
    xfree (vhead);

    return (err);
}

/* Process all the files in ".", then descend into other directories.
   Returns 0 for success, or >0 on error (in which case a message
   will have been printed).  */
static int
import_descend (message, vtag, targc, targv)
    char *message;
    char *vtag;
    int targc;
    char *targv[];
{
    DIR *dirp;
    struct dirent *dp;
    int err = 0;
    List *dirlist = NULL;
    List *filelist = NULL;

    /* first, load up any per-directory ignore lists */
    ign_add_file (CVSDOTIGNORE, 1);
    wrap_add_file (CVSDOTWRAPPER, 1);

    if ((dirp = opendir (".")) == NULL)
    {
	error (0, errno, "cannot open directory");
	err++;
    }
    else
    {
	errno = 0;
	while ((dp = readdir (dirp)) != NULL)
	{
	    if (strcmp (dp->d_name, ".") == 0 || strcmp (dp->d_name, "..") == 0)
		goto one_more_time_boys;
#ifdef SERVER_SUPPORT
	    /* CVS directories are created in the temp directory by
	       server.c because it doesn't special-case import.  So
	       don't print a message about them, regardless of -I!.  */
	    if (server_active && strcmp (dp->d_name, CVSADM) == 0)
		goto one_more_time_boys;
#endif
	    if (ign_name (dp->d_name))
	    {
		add_log ('I', dp->d_name);
		goto one_more_time_boys;
	    }

	    if (isdir (dp->d_name))
	    {
		Node *n;

		if (dirlist == NULL)
		    dirlist = getlist();

		n = getnode();
		n->key = xstrdup (dp->d_name);
		addnode(dirlist, n);
	    }
	    else if (
		     islink (dp->d_name)
		     )
	    {
		add_log ('L', dp->d_name);
		err++;
	    }
	    else
	    {
		Node *n;

#ifdef CLIENT_SUPPORT
		if (current_parsed_root->isremote)
		    err += client_process_import_file (message, dp->d_name,
                                                       vtag, targc, targv,
                                                       repository,
                                                       keyword_opt != NULL &&
						       keyword_opt[0] == 'b',
						       use_file_modtime);
		else
#endif
		    err += process_import_file (message, dp->d_name,
						vtag, targc, targv);
		if(filelist == NULL)
		  filelist = getlist();
		n = getnode();
		n->key=xstrdup(dp->d_name);
		addnode(filelist, n);
	    }
	one_more_time_boys:
	    errno = 0;
	}
	if (errno != 0)
	{
	    error (0, errno, "cannot read directory");
	    ++err;
	}
	closedir (dirp);
       if(create_cvs_dirs)
       {
               FILE *f;
               char *cwd = xgetwd_mapped();
          /* If there is no CVS/Root file, add one */
               if (!isfile (CVSADM_ROOT))
                     Create_Admin (cwd, cwd, repository, NULL, NULL, 1, 0, 1);
               xfree(cwd);
               f=fopen(CVSADM_ENT,"w");
               if (dirlist != NULL)
               {
                       Node *head, *p;

                       head = dirlist->list;
                       for (p = head->next; p != head; p = p->next)
                               fprintf(f,"D/%s////\n",p->key);
	       }
               if (filelist != NULL)
               {
                       Node *head, *p;
                       char *t,*r;

                       head = filelist->list;
                       for (p = head->next; p != head; p = p->next)
                       {
                               t=time_stamp(p->key, 0);
                               r=wrap_rcsoption(p->key,1);
							   fprintf(f,"/%s/%s.1/%s/%s/\n",p->key,vbranch?vbranch:"1",t,r?r:"");
                               xfree(r);
                               xfree(t);
                       }
           	}
               fclose(f);
       }

    }

    if (dirlist != NULL)
    {
	Node *head, *p;

	head = dirlist->list;
	for (p = head->next; p != head; p = p->next)
	{
	    err += import_descend_dir (message, p->key, vtag, targc, targv);
	}

	dellist(&dirlist);
    }
    if(filelist!=NULL)
        dellist(&filelist);

    return (err);
}

/*
 * Process the argument import file.
 */
static int
process_import_file (message, vfile, vtag, targc, targv)
    char *message;
    char *vfile;
    char *vtag;
    int targc;
    char *targv[];
{
    char *rcs;

	if(!verify_write(repository,vtag))
	{
       error (0, 0, "User %s cannot write to %s", CVS_Username, fn_root(repository));
       return (1);
    }

	rcs = xmalloc (strlen (repository) + strlen (vfile) + sizeof (RCSEXT)
		   + 5);
    sprintf (rcs, "%s/%s%s", repository, vfile, RCSEXT);
    if (!isfile (rcs))
    {
	    int retval;
	    char *free_opt = NULL;
	    char *our_opt = keyword_opt;

	    /*
	     * A new import source file; it doesn't exist as a ,v within the
	     * repository nor in the Attic -- create it anew.
	     */
	    add_log ('N', vfile);

#ifdef SERVER_SUPPORT
	    /* The most reliable information on whether the file is binary
	       is what the client told us.  That is because if the client had
	       the wrong idea about binaryness, it corrupted the file, so
	       we might as well believe the client.  */
	    if (server_active)
	    {
		Node *node;
		List *entries;

		/* Reading all the entries for each file is fairly silly, and
		   probably slow.  But I am too lazy at the moment to do
		   anything else.  */
		entries = Entries_Open (0, NULL);
		node = findnode_fn (entries, vfile);
		if (node != NULL)
		{
		    Entnode *entdata = (Entnode *) node->data;
		    if (entdata->type == ENT_FILE)
		    {
			assert (entdata->options[0] == '-'
				&& entdata->options[1] == 'k');
			our_opt = xstrdup (entdata->options + 2);
			free_opt = our_opt;
		    }
		}
		Entries_Close (entries);
	    }
#endif

	    retval = add_rcs_file (message, rcs, vfile, vhead, our_opt,
				   vbranch, vtag, targc, targv,
				   NULL, 0, logfp, NULL);
	    if (free_opt != NULL)
		xfree (free_opt);
	    xfree (rcs);
	    return retval;
    }

    xfree (rcs);
    /*
     * an rcs file exists. have to do things the official, slow, way.
     */
    return (update_rcs_file (message, vfile, vtag, targc, targv));
}

/*
 * The RCS file exists; update it by adding the new import file to the
 * (possibly already existing) vendor branch.
 */
static int update_rcs_file(char *message, char *vfile, char *vtag, int targc, char *targv[])
{
    Vers_TS *vers;
    int letter;
    struct file_info finfo;
	char *mapped_repository;

    memset (&finfo, 0, sizeof finfo);
    finfo.file = vfile;
    /* Not used, so don't worry about it.  */
    finfo.update_dir = NULL;
    finfo.fullname = finfo.file;
	finfo.mapped_file = map_filename(repository,finfo.file,&mapped_repository);
    finfo.repository = mapped_repository;
    finfo.entries = NULL;
    finfo.rcs = NULL;
    vers = Version_TS (&finfo, (char *) NULL, vbranch, (char *) NULL, 1, 0, 0);
    if (vers->vn_rcs != NULL
	&& !RCS_isdead(vers->srcfile, vers->vn_rcs))
    {
	int different;

	/*
	 * The rcs file does have a revision on the vendor branch. Compare
	 * this revision with the import file; if they match exactly, there
	 * is no need to install the new import file as a new revision to the
	 * branch.  Just tag the revision with the new import tags.
	 * 
	 * This is to try to cut down the number of "C" conflict messages for
	 * locally modified import source files.
	 */
	different = RCS_cmp_file (vers->srcfile, vers->vn_rcs, "-ko", vfile);

	if (!different)
	{
	    int retval = 0;

	    /*
	     * The two files are identical.  Just update the tags, print the
	     * "U", signifying that the file has changed, but needs no
	     * attention, and we're done.
	     */
	    if (add_tags (vers->srcfile, vfile, vtag, targc, targv))
			retval = 1;
		else
			add_log ('U', vfile);
	    freevers_ts (&vers);
	    return (retval);
	}
    }

    /* We may have failed to parse the RCS file; check just in case */
    if (vers->srcfile == NULL ||
	add_rev (message, vers->srcfile, vfile, vers->vn_rcs) ||
	add_tags (vers->srcfile, vfile, vtag, targc, targv))
    {
	freevers_ts (&vers);
	return (1);
    }

    if (vers->srcfile->branch == NULL || strcmp (vers->srcfile->branch, vbranch) != 0)
    {
	conflicts++;
	letter = 'C';
    }
    else
	letter = 'U';
    add_log (letter, vfile);

    freevers_ts (&vers);
	xfree(mapped_repository);
    return (0);
}

/*
 * Add the revision to the vendor branch
 */
static int
add_rev (message, rcs, vfile, vers)
    char *message;
    RCSNode *rcs;
    char *vfile;
    char *vers;
{
    int locked, status, ierrno;

    if (noexec)
	return (0);

    locked = 0;
    if (vers != NULL)
    {
	/* Before RCS_lock existed, we were directing cvs_stdout, as well as
	   cvs_stderr, from the RCS command, to DEVNULL.  I wouldn't guess that
	   was necessary, but I don't know for sure.  */
	/* Earlier versions of this function printed a `fork failed' error
	   when RCS_lock returned an error code.  That's not appropriate
	   now that RCS_lock is librarified, but should the error text be
	   preserved? */
	if (RCS_lock (rcs, vbranch, 1) != 0)
	    return 1;
	locked = 1;
	RCS_rewrite (rcs, NULL, NULL, 0);
    }
    status = RCS_checkin (rcs, vfile,
			  message, vbranch,
			  (RCS_FLAGS_QUIET | RCS_FLAGS_KEEPFILE
			   | (use_file_modtime ? RCS_FLAGS_MODTIME : 0)), NULL, NULL, NULL, NULL);
    ierrno = errno;

    if (status)
    {
	if (!noexec)
	{
	    fperrmsg (logfp, 0, status == -1 ? ierrno : 0,
		      "ERROR: Check-in of %s failed", fn_root(rcs->path));
	    error (0, status == -1 ? ierrno : 0,
		   "ERROR: Check-in of %s failed", fn_root(rcs->path));
	}
	if (locked)
	{
	    RCS_unlock(rcs, vbranch, 0);
	    RCS_rewrite (rcs, NULL, NULL, 0);
	}
	return (1);
    }
    return (0);
}

/*
 * Add the vendor branch tag and all the specified import release tags to the
 * RCS file.  The vendor branch tag goes on the branch root (1.1.1) while the
 * vendor release tags go on the newly added leaf of the branch (1.1.1.1,
 * 1.1.1.2, ...).
 */
static int
add_tags (rcs, vfile, vtag, targc, targv)
    RCSNode *rcs;
    char *vfile;
    char *vtag;
    int targc;
    char *targv[];
{
    int i, ierrno;
    Vers_TS *vers;
    int retcode = 0;
    struct file_info finfo;
    char *t;

    if (noexec)
		return (0);

    if (vtag && (t=RCS_tag2rev(rcs, vtag))!=NULL)
    {
		if(strcmp(t,vbranch))
		{
	fperrmsg (logfp, 0, 0,
		"ERROR: tag %s already exists in %s", vtag, fn_root(rcs->path));
	error (0, 0,
		"ERROR: tag %s already exists in %s", vtag, fn_root(rcs->path));
	return (1);
    }
		xfree(t);
    }
      
    if (vtag && (retcode = RCS_settag(rcs, vtag, vbranch, current_date)) != 0)
    {
		ierrno = errno;
		fperrmsg (logfp, 0, retcode == -1 ? ierrno : 0,
			"ERROR: Failed to set tag %s in %s", vtag, fn_root(rcs->path));
		error (0, retcode == -1 ? ierrno : 0,
			"ERROR: Failed to set tag %s in %s", vtag, fn_root(rcs->path));
		return (1);
    }

	if(!targc)
		RCS_rewrite (rcs, NULL, NULL, 0);
	else
	{
		memset (&finfo, 0, sizeof finfo);
		finfo.file = vfile;
		/* Not used, so don't worry about it.  */
		finfo.update_dir = NULL;
		finfo.fullname = finfo.file;
		finfo.repository = repository;
		finfo.entries = NULL;
		finfo.rcs = rcs;
		vers = Version_TS (&finfo, NULL, vtag, NULL, 1, 0, 0);
		for (i = 0; i < targc; i++)
		{
			if (!force_tags && (t=RCS_gettag(rcs, targv[i], 1, NULL))!=NULL)
			{
				fperrmsg (logfp, 0, 0,
					"WARNING: tag %s already exists in %s", targv[i], fn_root(rcs->path));
				error (0, 0,
					"WARNING: tag %s already exists in %s", targv[i], fn_root(rcs->path));
				xfree(t);
			}
			else if ((retcode = RCS_settag (rcs, targv[i], vers->vn_rcs, current_date)) != 0)
			{
				ierrno = errno;
				fperrmsg (logfp, 0, retcode == -1 ? ierrno : 0,	"WARNING: Couldn't add tag %s to %s", targv[i],	fn_root(rcs->path));
				error (0, retcode == -1 ? ierrno : 0, "WARNING: Couldn't add tag %s to %s", targv[i], fn_root(rcs->path));
			}
		}
		freevers_ts (&vers);
	}
	RCS_rewrite (rcs, NULL, NULL, 0);
    return (0);
}

/*
 * Stolen from rcs/src/rcsfnms.c, and adapted/extended.
 */
struct compair
{
    char *suffix, *comlead;
};

static const struct compair comtable[] =
{

/*
 * comtable pairs each filename suffix with a comment leader. The comment
 * leader is placed before each line generated by the $Log keyword. This
 * table is used to guess the proper comment leader from the working file's
 * suffix during initial ci (see InitAdmin()). Comment leaders are needed for
 * languages without multiline comments; for others they are optional.
 *
 * I believe that the comment leader is unused if you are using RCS 5.7, which
 * decides what leader to use based on the text surrounding the $Log keyword
 * rather than a specified comment leader.
 */
    {"a", "-- "},			/* Ada		 */
    {"ada", "-- "},
    {"adb", "-- "},
    {"asm", ";; "},			/* assembler (MS-DOS) */
    {"ads", "-- "},			/* Ada		 */
    {"bas", "' "},    			/* Visual Basic code */
    {"bat", ":: "},			/* batch (MS-DOS) */
    {"body", "-- "},			/* Ada		 */
    {"c", " * "},			/* C		 */
    {"c++", "// "},			/* C++ in all its infinite guises */
    {"cc", "// "},
    {"cpp", "// "},
    {"cxx", "// "},
	{"cs", "// "},
    {"m", "// "},			/* Objective-C */
    {"cl", ";;; "},			/* Common Lisp	 */
    {"cmd", ":: "},			/* command (OS/2) */
    {"cmf", "c "},			/* CM Fortran	 */
    {"cs", " * "},			/* C*		 */
    {"csh", "# "},			/* shell	 */
    {"dlg", " * "},   			/* MS Windows dialog file */
    {"e", "# "},			/* efl		 */
    {"epsf", "% "},			/* encapsulated postscript */
    {"epsi", "% "},			/* encapsulated postscript */
    {"el", "; "},			/* Emacs Lisp	 */
    {"f", "c "},			/* Fortran	 */
    {"for", "c "},
    {"frm", "' "},    			/* Visual Basic form */
    {"h", " * "},			/* C-header	 */
    {"hh", "// "},			/* C++ header	 */
    {"hpp", "// "},
    {"hxx", "// "},
    {"in", "# "},			/* for Makefile.in */
	{"java", "// "},
    {"l", " * "},			/* lex (conflict between lex and
					 * franzlisp) */
    {"mac", ";; "},			/* macro (DEC-10, MS-DOS, PDP-11,
					 * VMS, etc) */
    {"mak", "# "},    			/* makefile, e.g. Visual C++ */
    {"me", ".\\\" "},			/* me-macros	t/nroff	 */
    {"ml", "; "},			/* mocklisp	 */
    {"mm", ".\\\" "},			/* mm-macros	t/nroff	 */
    {"ms", ".\\\" "},			/* ms-macros	t/nroff	 */
    {"man", ".\\\" "},			/* man-macros	t/nroff	 */
    {"1", ".\\\" "},			/* feeble attempt at man pages... */
    {"2", ".\\\" "},
    {"3", ".\\\" "},
    {"4", ".\\\" "},
    {"5", ".\\\" "},
    {"6", ".\\\" "},
    {"7", ".\\\" "},
    {"8", ".\\\" "},
    {"9", ".\\\" "},
    {"p", " * "},			/* pascal	 */
    {"pas", " * "},
    {"pl", "# "},			/* perl	(conflict with Prolog) */
    {"ps", "% "},			/* postscript	 */
    {"psw", "% "},			/* postscript wrap */
    {"pswm", "% "},			/* postscript wrap */
    {"r", "# "},			/* ratfor	 */
    {"rc", " * "},			/* Microsoft Windows resource file */
    {"red", "% "},			/* psl/rlisp	 */
#ifdef sparc
    {"s", "! "},			/* assembler	 */
#endif
#ifdef mc68000
    {"s", "| "},			/* assembler	 */
#endif
#ifdef pdp11
    {"s", "/ "},			/* assembler	 */
#endif
#ifdef vax
    {"s", "# "},			/* assembler	 */
#endif
#ifdef __ksr__
    {"s", "# "},			/* assembler	 */
    {"S", "# "},			/* Macro assembler */
#endif
    {"sh", "# "},			/* shell	 */
    {"sl", "% "},			/* psl		 */
    {"spec", "-- "},			/* Ada		 */
    {"tex", "% "},			/* tex		 */
    {"y", " * "},			/* yacc		 */
    {"ye", " * "},			/* yacc-efl	 */
    {"yr", " * "},			/* yacc-ratfor	 */
    {"", "# "},				/* default for empty suffix	 */
    {NULL, "# "}			/* default for unknown suffix;	 */
/* must always be last		 */
};

static char *
get_comment (user)
    char *user;
{
    char *cp, *suffix;
    char *suffix_path;
    int i;
    char *retval;

    suffix_path = xmalloc (strlen (user) + 5);
    cp = strrchr (user, '.');
    if (cp != NULL)
    {
	cp++;

	/*
	 * Convert to lower-case, since we are not concerned about the
	 * case-ness of the suffix.
	 */
	strcpy (suffix_path, cp);
	for (cp = suffix_path; *cp; cp++)
	    if (isupper ((unsigned char) *cp))
		*cp = tolower (*cp);
	suffix = suffix_path;
    }
    else
	suffix = "";			/* will use the default */
    for (i = 0;; i++)
    {
	if (comtable[i].suffix == NULL)
	{
	    /* Default.  Note we'll always hit this case before we
	       ever return NULL.  */
	    retval = comtable[i].comlead;
	    break;
	}
	if (strcmp (suffix, comtable[i].suffix) == 0)
	{
	    retval = comtable[i].comlead;
	    break;
	}
    }
    xfree (suffix_path);
    return retval;
}

/* Create a new RCS file from scratch.

   This probably should be moved to rcs.c now that it is called from
   places outside import.c.

   Return value is 0 for success, or nonzero for failure (in which
   case an error message will have already been printed).  */
int
add_rcs_file (
    /* Log message for the addition.  Not used if add_vhead == NULL.  */
    char *message,
    /* Filename of the RCS file to create.  */
    char *rcs,
    /* Filename of the file to serve as the contents of the initial
       revision.  Even if add_vhead is NULL, we use this to determine
       the modes to give the new RCS file.  */
    char *userfile,

    /* Revision number of head that we are adding.  Normally 1.1 but
       could be another revision as long as ADD_VBRANCH is a branch
       from it.  If NULL, then just add an empty file without any
       revisions (similar to the one created by "rcs -i").  */
    char *add_vhead,

    /* Keyword expansion mode, e.g., "b" for binary.  NULL means the
       default behavior.  */
    char *key_opt,

    /* Vendor branch to import to, or NULL if none.  If non-NULL, then
       vtag should also be non-NULL.  */
    char *add_vbranch,
    char *vtag,
    int targc,
    char *targv[],

    /* If non-NULL, description for the file.  If NULL, the description
       will be empty.  */
    char *desctext,
    size_t desclen,

    /* Write errors to here as well as via error (), or NULL if we should
       use only error ().  */
    FILE *add_logfp,
	RCSCHECKINPROC callback)
{
    FILE *fprcs, *fpuser;
    struct stat sb;
    struct tm *ftm;
    time_t now;
    char altdate1[MAXDATELEN];
    const char *author;
    int i, ierrno, err = 0;
    char *local_opt = key_opt;
    char *free_opt = NULL;
    mode_t file_type;
	kflag local_opt_flags;

    if (noexec)
	return (0);

    /* Note that as the code stands now, the -k option overrides any
       settings in wrappers (whether CVSROOT/cvswrappers, -W, or
       whatever).  Some have suggested this should be the other way
       around.  As far as I know the documentation doesn't say one way
       or the other.  Before making a change of this sort, should think
       about what is best, document it (in cvs.texinfo and NEWS), &c.  */

    if (local_opt == NULL)
    {
		if (userfile && wrap_name_has (userfile, WRAP_RCSOPTION))
		{
			local_opt = free_opt = wrap_rcsoption (userfile, 0);
		}
    }

    /* Opening in text mode is probably never the right thing for the
       server (because the protocol encodes text files in a fashion
       which does not depend on what the client or server OS is, as
       documented in cvsclient.texi), but as long as the server just
       runs on unix it is a moot point.  */

    /* If PreservePermissions is set, then make sure that the file
       is a plain file before trying to open it.  Longstanding (although
       often unpopular) CVS behavior has been to follow symlinks, so we
       maintain that behavior if PreservePermissions is not on.

       NOTE: this error message used to be `cannot cvs_fstat', but is now
       `cannot lstat'.  I don't see a way around this, since we must
       cvs_stat the file before opening it. -twp */

	local_opt_flags = RCS_get_kflags(local_opt, 1);

	if(callback || !userfile)
	{
		fpuser = NULL;
		memset(&sb,0,sizeof(sb));
		sb.st_mode=0644;
	}
	else 
	{
		if (CVS_LSTAT (userfile, &sb) < 0)
		{
		/* not fatal, continue import */
		if (add_logfp != NULL)
			fperrmsg (add_logfp, 0, errno,
				"ERROR: cannot lstat file %s", userfile);
		error (0, errno, "cannot lstat file %s", userfile);
		goto read_error;
		}
		file_type = sb.st_mode & S_IFMT;

		fpuser = fopen (userfile, (local_opt_flags.flags&(KFLAG_BINARY|KFLAG_ENCODED)) ? "rb" : "r");
		if (fpuser == NULL)
		{
			/* not fatal, continue import */
			if (add_logfp != NULL)
			fperrmsg (add_logfp, 0, errno,
				"ERROR: cannot read file %s", userfile);
			error (0, errno, "ERROR: cannot read file %s", userfile);
			goto read_error;
		}
	}

    fprcs = fopen (rcs, "w+b");
    if (fprcs == NULL)
    {
	ierrno = errno;
	goto write_error_noclose;
    }

    /*
     * putadmin()
     */
    if (add_vhead != NULL)
    {
	if (fprintf (fprcs, "head     %s;\012", add_vhead) < 0)
	    goto write_error;
    }
    else
    {
	if (fprintf (fprcs, "head     ;\012") < 0)
	    goto write_error;
    }

    if (add_vbranch != NULL)
    {
	if (fprintf (fprcs, "branch   %s;\012", add_vbranch) < 0)
	    goto write_error;
    }
    if (fprintf (fprcs, "access   ;\012") < 0 ||
	fprintf (fprcs, "symbols ") < 0)
    {
	goto write_error;
    }

    for (i = targc - 1; i >= 0; i--)
    {
		if (fprintf (fprcs, "\012") < 0)
			goto write_error;
		/* RCS writes the symbols backwards */
		assert (add_vbranch != NULL || add_vhead != NULL);
		if(add_vbranch)
		{
			if (fprintf (fprcs, "\t%s:%s.1", targv[i], add_vbranch) < 0)
				goto write_error;
		}
		else
		{
			if (fprintf (fprcs, "\t%s:%s", targv[i], add_vhead) < 0)
				goto write_error;
		}
    }

    if (add_vbranch != NULL)
    {
		if (fprintf (fprcs, "\012") < 0)
			goto write_error;
		if (fprintf (fprcs, "\t%s:%s", vtag, add_vbranch) < 0)
	    goto write_error;
    }
    if (fprintf (fprcs, ";\012") < 0)
	goto write_error;

    if (fprintf (fprcs, "locks    ; strict;\012") < 0 ||
	/* XXX - make sure @@ processing works in the RCS file */
	fprintf (fprcs, "comment  @%s@;\012", userfile?get_comment (userfile):"new file") < 0)
    {
	goto write_error;
    }

    if (local_opt != NULL)
    {
	if (fprintf (fprcs, "expand   @%s@;\012", local_opt) < 0)
	{
	    goto write_error;
	}
    }

    if (fprintf (fprcs, "\012") < 0)
      goto write_error;

    /* Write the revision(s), with the date and author and so on
       (that is "delta" rather than "deltatext" from rcsfile(5)).  */
    if (add_vhead != NULL)
    {
	if (use_file_modtime)
	    now = sb.st_mtime;
	else
	    time (&now);
	ftm = gmtime (&now);
	sprintf (altdate1, DATEFORM,
			ftm->tm_year + (ftm->tm_year < 100 ? 0 : 1900),
			ftm->tm_mon + 1, ftm->tm_mday, ftm->tm_hour,
			ftm->tm_min, ftm->tm_sec);
	author = getcaller ();

#ifdef SJIS
	if (fprintf (fprcs, "\012%s\012", add_vhead) < 0 ||
	fprintf (fprcs, "date     %s;  author ", altdate1) < 0)
	    goto write_error;
	if (strpbrk (author, "$,.:;@"))
	{
	    fputs ("@", fprcs);
	    expand_at_signs (author, strlen(author), fprcs);
	    fputs ("@", fprcs);
	}
	else
	    fputs(author, fprcs);
	if (fprintf (fprcs, ";  state Exp;\012") < 0)
	goto write_error;
#else
	if (fprintf (fprcs, "\012%s\012", add_vhead) < 0 ||
	fprintf (fprcs, "date     %s;  author %s;  state Exp;\012",
		 altdate1, author) < 0)
	goto write_error;
#endif

	if (fprintf (fprcs, "branches") < 0)
	    goto write_error;
	if (add_vbranch != NULL)
	{
	    if (fprintf (fprcs, " %s.1", add_vbranch) < 0)
		goto write_error;
	}
	if (fprintf (fprcs, ";\012") < 0)
	    goto write_error;

	if (fprintf (fprcs, "next     ;\012") < 0)
	    goto write_error;

	/* kopt */
    if (local_opt != NULL)
    {
		if (fprintf (fprcs, "kopt   @%s@;\012", local_opt) < 0)
		    goto write_error;
    }

	/* deltatype */
	{
		char *dt;

		if(local_opt_flags.flags&KFLAG_COMPRESS_DELTA)
		{
			if(local_opt_flags.flags&KFLAG_BINARY_DELTA)
				dt="compressed_binary";
			else
				dt="compressed_text";
		}
		else
		{
			if(local_opt_flags.flags&KFLAG_BINARY_DELTA)
				dt="binary";
			else
				dt="text";
		}
		
		if (fprintf (fprcs, "deltatype   %s;\012", dt) < 0)
		    goto write_error;
	}

	/* Store initial permissions. */
	if (fprintf (fprcs, "permissions\t%o;\012",
			    sb.st_mode & 07777) < 0)
		goto write_error;

	if (add_vbranch != NULL)
	{
#ifdef SJIS
	    if (fprintf (fprcs, "\012%s.1\012", add_vbranch) < 0)
		goto write_error;
		fprintf (fprcs, "date     %s;  author ", altdate1);
	    if (strpbrk (author, "$,.:;@"))
	    {
		fputs ("@", fprcs);
		expand_at_signs (author, strlen(author), fprcs);
		fputs ("@", fprcs);
	    }
	    else
		fputs(author, fprcs);
	    if (fprintf (fprcs, ";  state Exp;\012",
		     altdate1, author) < 0 ||
	    	fprintf (fprcs, "branches ;\012") < 0 ||
	    	fprintf (fprcs, "next     ;\012") < 0)
		goto write_error;
#else
	    if (fprintf (fprcs, "\012%s.1\012", add_vbranch) < 0 ||
		fprintf (fprcs, "date     %s;  author %s;  state Exp;\012",
			 altdate1, author) < 0 ||
		fprintf (fprcs, "branches ;\012") < 0 ||
		fprintf (fprcs, "next     ;\012") < 0)
		goto write_error;
#endif
	    /* Store initial permissions */
		if (fprintf (fprcs, "permissions\t%o;\012",
				sb.st_mode & 07777) < 0)
		goto write_error;
	    
	    if (fprintf (fprcs, "\012") < 0)
		goto write_error;
	}
    }

    /* Now write the description (possibly empty).  */
    if (fprintf (fprcs, "\012desc\012") < 0 ||
	fprintf (fprcs, "@") < 0)
	goto write_error;
    if (desctext != NULL)
    {
	/* The use of off_t not size_t for the second argument is very
	   strange, since we are dealing with something which definitely
	   fits in memory.  */
	if (expand_at_signs (desctext, desclen, fprcs) < 0)
	    goto write_error;
    }
    if (fprintf (fprcs, "@\012\012\012") < 0)
	goto write_error;

    /* Now write the log messages and contents for the revision(s) (that
       is, "deltatext" rather than "delta" from rcsfile(5)).  */
    if (add_vhead != NULL)
    {
	if (fprintf (fprcs, "\012%s\012", add_vhead) < 0 ||
	    fprintf (fprcs, "log\012@") < 0)
	    goto write_error;
	if (add_vbranch != NULL)
	{
	    /* We are going to put the log message in the revision on the
	       branch.  So putting it here too seems kind of redundant, I
	       guess (and that is what CVS has always done, anyway).  */
	    if (fprintf (fprcs, "Initial revision\012") < 0)
		goto write_error;
	}
	else
	{
	    if (expand_at_signs (message, strlen (message), fprcs) < 0)
		goto write_error;
	}
	if (fprintf (fprcs, "@\012") < 0 ||
	    fprintf (fprcs, "text\012@") < 0)
	{
	    goto write_error;
	}

	/* Now copy over the contents of the file, expanding at signs. */
	if(fpuser)
	{
		char *buf;
	    size_t len;

		fseek(fpuser,0,SEEK_END);
		len = ftell(fpuser);
		fseek(fpuser,0,SEEK_SET);
		buf = xmalloc(len);

		len = fread (buf, 1, len, fpuser);
		if (len == 0)
		{
		    if (ferror (fpuser))
			error (1, errno, "cannot read file %s for copying",
			       userfile);
		}
		if(!server_active)
			len = convert_encoding_to_utf8(&buf,len,&len,local_opt_flags.encoding);

		if(local_opt_flags.flags&KFLAG_COMPRESS_DELTA)
		{
			uLong zlen;
			void *zbuf;

			z_stream stream = {0};
			deflateInit(&stream,Z_DEFAULT_COMPRESSION);
			zlen = deflateBound(&stream, len);
			stream.avail_in = len;
			stream.next_in = buf;
			stream.avail_out = zlen;
			zbuf = xmalloc(zlen+4);
			stream.next_out = ((char*)zbuf)+4;
			*(unsigned long *)zbuf=htonl(len);
			if(deflate(&stream, Z_FINISH)!=Z_STREAM_END)
			{
				error(1,0,"internal error: deflate failed");
			}
			expand_at_signs (zbuf, stream.total_out+4, fprcs);
			deflateEnd(&stream);
			xfree(zbuf);			
		}
		else
		{
			if (expand_at_signs (buf, len, fprcs) < 0)
				goto write_error;
		}

		xfree(buf);
	}
	if (fprintf (fprcs, "@\012\012") < 0)
	    goto write_error;
	if (add_vbranch != NULL)
	{
	    if (fprintf (fprcs, "\012%s.1\012", add_vbranch) < 0 ||
		fprintf (fprcs, "log\012@") < 0 ||
		expand_at_signs (message,
				 strlen (message), fprcs) < 0 ||
		fprintf (fprcs, "@\012text\012") < 0 ||
		fprintf (fprcs, "@@\012") < 0)
		goto write_error;
	}
    }

    if (fclose (fprcs) == EOF)
    {
	ierrno = errno;
	goto write_error_noclose;
    }
    /* Close fpuser only if we opened it to begin with. */
    if (fpuser != NULL)
    {
	if (fclose (fpuser) < 0)
	    error (0, errno, "cannot close %s", userfile);
    }

    /*
     * Fix the modes on the RCS files. 
     */
    xchmod(rcs, 0);
    if (free_opt != NULL)
	xfree (free_opt);
    return (err);

write_error:
    ierrno = errno;
    if (fclose (fprcs) < 0)
	error (0, errno, "cannot close %s", fn_root(rcs));
write_error_noclose:
    if (fclose (fpuser) < 0)
	error (0, errno, "cannot close %s", fn_root(userfile));
    if (add_logfp != NULL)
	fperrmsg (add_logfp, 0, ierrno, "ERROR: cannot write file %s", fn_root(rcs));
    error (0, ierrno, "ERROR: cannot write file %s", fn_root(rcs));
    if (ierrno == ENOSPC)
    {
	if (unlink (rcs) < 0)
	    error (0, errno, "cannot remove %s", rcs);
	if (add_logfp != NULL)
	    fperrmsg (add_logfp, 0, 0, "ERROR: out of space - aborting");
	error (1, 0, "ERROR: out of space - aborting");
    }
read_error:

    if (free_opt != NULL)
	xfree (free_opt);

    return (err + 1);
}

/*
 * Write SIZE bytes at BUF to FP, expanding @ signs into double @
 * signs.  If an error occurs, return a negative value and set errno
 * to indicate the error.  If not, return a nonnegative value.
 */
int expand_at_signs (const char *buf, unsigned int size, FILE *fp)
{
    register const char *cp, *next;

    cp = buf;
    while ((next = memchr (cp, '@', size)) != NULL)
    {
	int len;

	++next;
	len = next - cp;
	if (fwrite (cp, 1, len, fp) != len)
	    return EOF;
	if (fputc ('@', fp) == EOF)
	    return EOF;
	cp = next;
	size -= len;
    }

    if (fwrite (cp, 1, size, fp) != size)
	return EOF;

    return 1;
}

/*
 * Write an update message to (potentially) the screen and the log file.
 */
static void
add_log (ch, fname)
    int ch;
    char *fname;
{
    if (!really_quiet)			/* write to terminal */
    {
	char buf[2];
	buf[0] = ch;
	buf[1] = ' ';
	cvs_output (buf, 2);
	if (repos_len)
	{
	    cvs_output (repository + repos_len + 1, 0);
	    cvs_output ("/", 1);
	}
	else if (repository[0] != '\0')
	{
	    cvs_output (fn_root(repository), 0);
	    cvs_output ("/", 1);
	}
	cvs_output (fname, 0);
	cvs_output ("\n", 1);
    }

    if (repos_len)			/* write to logfile */
	fprintf (logfp, "%c %s/%s\n", ch,
			repository + repos_len + 1, fname);
    else if (repository[0])
	fprintf (logfp, "%c %s/%s\n", ch, fn_root(repository), fname);
    else
	fprintf (logfp, "%c %s\n", ch, fname);
}

/*
 * This is the recursive function that walks the argument directory looking
 * for sub-directories that have CVS administration files in them and updates
 * them recursively.
 * 
 * Note that we do not follow symbolic links here, which is a feature!
 */
static int
import_descend_dir (message, dir, vtag, targc, targv)
    char *message;
    char *dir;
    char *vtag;
    int targc;
    char *targv[];
{
    struct saved_cwd cwd;
    char *cp;
    int ierrno, err;
    char *rcs = NULL;

    if (islink (dir))
	return (0);
    if (save_cwd (&cwd))
    {
	fperrmsg (logfp, 0, 0, "ERROR: cannot get working directory");
	return (1);
    }

    /* Concatenate DIR to the end of REPOSITORY.  */
    if (repository[0] == '\0')
    {
	char *new = xstrdup (dir);
	xfree (repository);
	repository = new;
    }
    else
    {
	char *new = xmalloc (strlen (repository) + strlen (dir) + 10);
	strcpy (new, repository);
	strcat (new, "/");
	strcat (new, dir);
	xfree (repository);
	repository = new;
    }

    /* Verify we have create access in this directory. */
    if (! verify_create (repository,NULL))
	error (1, 0, "User %s cannot create files in %s",
	       CVS_Username, fn_root(repository));

#ifdef CLIENT_SUPPORT
    if (!quiet && !current_parsed_root->isremote)
#else
    if (!quiet)
#endif
	error (0, 0, "Importing %s", repository);

    if ( chdir (dir) < 0)
    {
	ierrno = errno;
	fperrmsg (logfp, 0, ierrno, "ERROR: cannot chdir to %s", repository);
	error (0, ierrno, "ERROR: cannot chdir to %s", repository);
	err = 1;
	goto out;
    }
#ifdef CLIENT_SUPPORT
    if (!current_parsed_root->isremote && !isdir (repository))
#else
    if (!isdir (repository))
#endif
    {
	rcs = xmalloc (strlen (repository) + sizeof (RCSEXT) + 5);
	sprintf (rcs, "%s%s", repository, RCSEXT);
	if (isfile (repository) || isfile(rcs))
	{
	    fperrmsg (logfp, 0, 0,
		      "ERROR: %s is a file, should be a directory!",
		      fn_root(repository));
	    error (0, 0, "ERROR: %s is a file, should be a directory!",
		   fn_root(repository));
	    err = 1;
	    goto out;
	}
	if (noexec == 0 && CVS_MKDIR (repository, 0777) < 0)
	{
	    ierrno = errno;
	    fperrmsg (logfp, 0, ierrno,
		      "ERROR: cannot mkdir %s -- not added", fn_root(repository));
	    error (0, ierrno,
		   "ERROR: cannot mkdir %s -- not added", fn_root(repository));
	    err = 1;
	    goto out;
	}
   change_owner(repository, CVS_Username);
   change_perms(repository, "", NULL, NULL);
    }
    err = import_descend (message, vtag, targc, targv);
  out:
    if (rcs != NULL)
	xfree (rcs);
    if ((cp = strrchr (repository, '/')) != NULL)
	*cp = '\0';
    else
	repository[0] = '\0';
    if (restore_cwd (&cwd, NULL))
	error_exit ();
    free_cwd (&cwd);
    return (err);
}

