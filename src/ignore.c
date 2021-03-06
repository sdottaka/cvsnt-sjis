/* This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.  */

/*
 * .cvsignore file support contributed by David G. Grubbs <dgg@odi.com>
 */

#include "cvs.h"
#include "getline.h"

/*
 * Ignore file section.
 * 
 *	"!" may be included any time to reset the list (i.e. ignore nothing);
 *	"*" may be specified to ignore everything.  It stays as the first
 *	    element forever, unless a "!" clears it out.
 */

static const char **ign_list;			/* List of files to ignore in update
					 * and import */
static const char **s_ign_list = NULL;
static int ign_count;			/* Number of active entries */
static int s_ign_count = 0;
static int ign_size;			/* This many slots available (plus
					 * one for a NULL) */
static int ign_hold = -1;		/* Index where first "temporary" item
					 * is held */

const char *ign_default = ". .. core RCSLOG tags TAGS RCS SCCS .make.state\
 .nse_depinfo #* .#* cvslog.* ,* CVS CVS.adm .del-* *.a *.olb *.o *.obj\
 *.so *.Z *~ *.old *.elc *.ln *.bak *.BAK *.orig *.rej *.exe *.dll *.pdb *.lib *.exp _$* *$";

#define IGN_GROW 16			/* grow the list by 16 elements at a
					 * time */

/* Nonzero if we have encountered an -I ! directive, which means one should
   no longer ask the server about what is in CVSROOTADM_IGNORE.  */
static int ign_inhibit_server;

/*
 * To the "ignore list", add the hard-coded default ignored wildcards above,
 * the wildcards found in $CVSROOT/CVSROOT/cvsignore, the wildcards found in
 * ~/.cvsignore and the wildcards found in the CVSIGNORE environment
 * variable.
 */
void
ign_setup ()
{
    char *home_dir;
    char *tmp, *ptr, *server_line;
	int len;

    ign_inhibit_server = 0;

    /* Start with default list and special case */
    tmp = xstrdup (ign_default);
    ign_add (tmp, 0);
    xfree (tmp);

    /* The client handles another way, by (after it does its own ignore file
       processing, and only if !ign_inhibit_server), letting the server
       know about the files and letting it decide whether to ignore
       them based on CVSROOOTADM_IGNORE.  */
#ifdef CLIENT_SUPPORT
    if (current_parsed_root && !current_parsed_root->isremote)
#endif
    {
	char *file = xmalloc (strlen (current_parsed_root->directory) + sizeof (CVSROOTADM)
			      + sizeof (CVSROOTADM_IGNORE) + 10);
	/* Then add entries found in repository, if it exists */
	sprintf (file, "%s/%s/%s", current_parsed_root->directory,
			CVSROOTADM, CVSROOTADM_IGNORE);
	ign_add_file (file, 0);
	xfree (file);
    }
#ifdef CLIENT_SUPPORT
	else if(supported_request("read-cvsignore"))
	{
		TRACE(1,"Requesting server cvsignore");
		send_to_server ("read-cvsignore\012", 0);
		read_line(&server_line);
		if(server_line[0]=='E' && server_line[1]==' ')
		{
			fprintf (stderr, "%s\n", server_line + 2);
			error_exit();
		}
		len = atoi(server_line);
		tmp = xmalloc(len+1);
		ptr = tmp;
		while (len > 0)
		{
			size_t n;

			n = try_read_from_server (ptr, len);
			len -= n;
			ptr += n;
		}
		*ptr = '\0';
		ptr=strtok(tmp,"\n");
		while(ptr && *ptr)
		{
			ign_add(ptr,0);
			ptr=strtok(NULL,"\n");
		}
		xfree(tmp);
		xfree(server_line);
		ign_inhibit_server = 1; /* We have all the server-side ignore stuff... ignore any more */
	}
#endif

    /* Then add entries found in home dir, (if user has one) and file exists */
    home_dir = get_homedir ();
    /* If we can't find a home directory, ignore ~/.cvsignore.  This may
       make tracking down problems a bit of a pain, but on the other
       hand it might be obnoxious to complain when CVS will function
       just fine without .cvsignore (and many users won't even know what
       .cvsignore is).  */
    if (home_dir)
    {
	char *file = xmalloc (strlen (home_dir) + sizeof (CVSDOTIGNORE) + 10);
	sprintf (file, "%s/%s", home_dir, CVSDOTIGNORE);
	ign_add_file (file, 0);
	xfree (file);
    }

    /* Then add entries found in CVSIGNORE environment variable. */
    ign_add (getenv (IGNORE_ENV), 0);

    /* Later, add ignore entries found in -I arguments */
}

/*
 * Open a file and read lines, feeding each line to a line parser. Arrange
 * for keeping a temporary list of wildcards at the end, if the "hold"
 * argument is set.
 */
void ign_add_file (const char *file, int hold)
{
    FILE *fp;
    char *line = NULL;
    size_t line_allocated = 0;

    /* restore the saved list (if any) */
    if (s_ign_list != NULL)
    {
	int i;

	for (i = 0; i < s_ign_count; i++)
	    ign_list[i] = s_ign_list[i];
	ign_count = s_ign_count;
	ign_list[ign_count] = NULL;

	s_ign_count = 0;
	xfree (s_ign_list);
	s_ign_list = NULL;
    }

    /* is this a temporary ignore file? */
    if (hold)
    {
	/* re-set if we had already done a temporary file */
	if (ign_hold >= 0)
	{
	    int i;

	    for (i = ign_hold; i < ign_count; i++)
			xfree (ign_list[i]);
	    ign_count = ign_hold;
	    if(ign_list)
	      ign_list[ign_count] = NULL;
	}
	else
	{
	    ign_hold = ign_count;
	}
    }

    /* load the file */
    fp = fopen (file, "r");
    if (fp == NULL)
    {
		return; // Fail silently
    }
    while (getline (&line, &line_allocated, fp) >= 0)
		ign_add (line, hold);
    if (ferror (fp))
		error (0, errno, "cannot read %s", file);
    if (fclose (fp) < 0)
		error (0, errno, "cannot close %s", file);
    xfree (line);
}

static char *next_token(const char **line)
{
	const char *cp;
	char *ptr,*np;
	int in_string,esc;

	if(!**line)
		return NULL;

	cp=*line;
	ptr=np=xmalloc(strlen(cp)+1);
	in_string=0;
	esc=0;
	for(;*cp && (in_string || esc || !isspace(*(unsigned char *)cp));cp++)
	{
		if(!esc)
		{
#ifdef SJIS
			if(_ismbblead(*cp))
			{
				*(np++)=*(cp++);
			}
			else if(*cp=='\\')
#else
			if(*cp=='\\')
#endif
			{
				esc=1;
				continue;
			}
			else if(*cp=='"')
			{
				in_string=!in_string;
				continue;
			}
		}
		*(np++)=*(cp);
		esc=0;
	}
	*(np++)='\0';
	while(*cp && isspace(*cp))
		cp++;
	*line=cp;

	return xrealloc(ptr,strlen(ptr)+1);
}

/* Parse a line of space-separated wildcards and add them to the list. */
void ign_add (const char *ign, int hold)
{
	const char *ptr;

    if (!ign || !*ign)
		return;

    for (; *ign; )
    {
		ptr = next_token(&ign);
		if(!ptr)
			break; /* Shouldn't happen */
		/*
		* if we find a single character !, we must re-set the ignore list
		* (saving it if necessary).  We also catch * as a special case in a
		* global ignore file as an optimization
		*/
		if((ptr[0]=='!' || ptr[0]=='*') && !ptr[1])
		{
		    if (!hold)
		    {
				/* permanently reset the ignore list */
				int i;

				for (i = 0; i < ign_count; i++)
					xfree (ign_list[i]);
				ign_count = 0;
				ign_list[0] = NULL;

				/* if we are doing a '!', continue; otherwise add the '*' */
				if (ptr[0] == '!')
				{
					ign_inhibit_server = 1;
					continue;
				}
		    }
		}
	    else if (ptr[0] == '!')
	    {
			/* temporarily reset the ignore list */
			int i;

			if (ign_hold >= 0)
			{
				for (i = ign_hold; i < ign_count; i++)
				xfree (ign_list[i]);
				ign_hold = -1;
			}
			s_ign_list = (const char **) xmalloc (ign_count * sizeof (char *));
			for (i = 0; i < ign_count; i++)
				s_ign_list[i] = ign_list[i];
			s_ign_count = ign_count;
			ign_count = 0;
			ign_list[0] = NULL;
			continue;
	    }

		/* If we have used up all the space, add some more */
		if (ign_count >= ign_size)
		{
			ign_size += IGN_GROW;
			ign_list = (const char **) xrealloc ((void*)ign_list,
						(ign_size + 1) * sizeof (char *));
		}

		ign_list[ign_count++] = ptr;
		ign_list[ign_count] = NULL;
    }
}

/* Return 1 if the given filename should be ignored by update or import. */
int
ign_name (name)
    char *name;
{
    const char **cpp = ign_list;

    if (cpp == NULL)
		return (0);

	while (*cpp)
	{
	    if (CVS_FNMATCH (*cpp++, name, 0) == 0)
			return 1;
	}
	return 0;
}

/* FIXME: This list of dirs to ignore stuff seems not to be used.
   Really?  send_dirent_proc and update_dirent_proc both call
   ignore_directory and do_module calls ign_dir_add.  No doubt could
   use some documentation/testsuite work.  */

static char **dir_ign_list = NULL;
static int dir_ign_max = 0;
static int dir_ign_current = 0;

/* Add a directory to list of dirs to ignore.  */
void
ign_dir_add (name)
    char *name;
{
    /* Make sure we've got the space for the entry.  */
    if (dir_ign_current <= dir_ign_max)
    {
	dir_ign_max += IGN_GROW;
	dir_ign_list =
	    (char **) xrealloc (dir_ign_list,
				(dir_ign_max + 1) * sizeof (char *));
    }

    dir_ign_list[dir_ign_current++] = xstrdup (name);
}


/* Return nonzero if NAME is part of the list of directories to ignore.  */

int
ignore_directory (name)
    char *name;
{
    int i;

    if (!dir_ign_list)
	return 0;

    i = dir_ign_current;
    while (i--)
    {
	if (pathncmp (name, dir_ign_list[i], strlen (dir_ign_list[i]),NULL) == 0)
	    return 1;
    }

    return 0;
}

/*
 * Process the current directory, looking for files not in ILIST and
 * not on the global ignore list for this directory.  If we find one,
 * call PROC passing it the name of the file and the update dir.
 * ENTRIES is the entries list, which is used to identify known
 * directories.  ENTRIES may be NULL, in which case we assume that any
 * directory with a CVS administration directory is known.
 */
void
ignore_files (ilist, entries, update_dir, proc)
    List *ilist;
    List *entries;
    char *update_dir;
    Ignore_proc proc;
{
    int subdirs;
    DIR *dirp;
    struct dirent *dp;
    struct stat sb;
    char *file;
    char *xdir;
    List *files;
    Node *p;

    /* Set SUBDIRS if we have subdirectory information in ENTRIES.  */
    if (entries == NULL)
	subdirs = 0;
    else
    {
	struct stickydirtag *sdtp;

	sdtp = (struct stickydirtag *) entries->list->data;
	subdirs = sdtp == NULL || sdtp->subdirs;
    }

    /* we get called with update_dir set to "." sometimes... strip it */
    if (strcmp (update_dir, ".") == 0)
	xdir = "";
    else
	xdir = update_dir;

    dirp = opendir (".");
    if (dirp == NULL)
    {
	error (0, errno, "cannot open current directory");
	return;
    }

    ign_add_file (CVSDOTIGNORE, 1);
    wrap_add_file (CVSDOTWRAPPER, 1);

    /* Make a list for the files.  */
    files = getlist ();

    while (errno = 0, (dp = readdir (dirp)) != NULL)
    {
	file = dp->d_name;
	if (strcmp (file, ".") == 0 || strcmp (file, "..") == 0)
	    continue;
	if (findnode_fn (ilist, file) != NULL)
	    continue;
	if (subdirs)
	{
	    Node *node;

	    node = findnode_fn (entries, file);
	    if (node != NULL
		&& ((Entnode *) node->data)->type == ENT_SUBDIR)
	    {
		char *p;
		int dir;

		/* For consistency with past behaviour, we only ignore
		   this directory if there is a CVS subdirectory.
		   This will normally be the case, but the user may
		   have messed up the working directory somehow.  */
		p = xmalloc (strlen (file) + sizeof CVSADM + 10);
		sprintf (p, "%s/%s", file, CVSADM);
		dir = isdir (p);
		xfree (p);
		if (dir)
		    continue;
	    }
	}

	/* We could be ignoring FIFOs and other files which are neither
	   regular files nor directories here.  */
	if (ign_name (file))
	    continue;

	if (
		CVS_LSTAT(file, &sb) != -1) 
	{

	    if (
		S_ISDIR (sb.st_mode)
		)
	    {
		if (! subdirs)
		{
		    char *temp;

		    temp = xmalloc (strlen (file) + sizeof (CVSADM) + 10);
		    sprintf (temp, "%s/%s", file, CVSADM);
		    if (isdir (temp))
		    {
			xfree (temp);
			continue;
		    }
		    xfree (temp);
		}
	    }
#ifdef S_ISLNK
	    else if (
		     S_ISLNK (sb.st_mode)
		     )
	    {
		continue;
	    }
#endif
	}

	p = getnode ();
	p->type = FILES;
	p->key = xstrdup (file);
	addnode (files, p);
    }
    if (errno != 0)
	error (0, errno, "error reading current directory");
    closedir (dirp);

    sortlist (files, fsortcmp);
    for (p = files->list->next; p != files->list; p = p->next)
	(*proc) (p->key, xdir);
    dellist (&files);
}

#ifdef CLIENT_SUPPORT
/* Send -I arguments for the cvsignore to the server.  The command must
   be one that accepts them (e.g. update, import).  */
void ign_send ()
{
    int i;

	send_to_server ("Argument -I\012Argument ", 0);
	if(ign_inhibit_server)
		send_to_server("! ",2);
	if(ign_list)
	{
		for(i=0; i<ign_count; i++)
		{
			if(ign_list[i])
			{
				send_to_server (ign_list[i], 0);
				send_to_server (" ",1);
			}
		}
	}
	send_to_server ("\012", 0);
}
#endif /* CLIENT_SUPPORT */

int ign_close()
{
	if(ign_list)
	{
		int i;
		for(i=0; i<ign_count; i++)
			xfree(ign_list[i]);
		xfree(ign_list);
		ign_count=0;
	}
	return 0;
}

void ign_display()
{
    int i;

	if(ign_inhibit_server)
	  printf("! ");
	if(ign_list)
	{
		for(i=0; i<ign_count; i++)
		{
			if(ign_list[i])
			{
				printf("%s ",ign_list[i]);
			}
		}
	}
	printf("\n");
}
