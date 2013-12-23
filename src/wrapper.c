/* This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.  */

#include "cvs.h"
#include "getline.h"

/*
  Original Author:  athan@morgan.com <Andrew C. Athan> 2/1/94
  Modified By:      vdemarco@bou.shl.com

  This package was written to support the NEXTSTEP concept of
  "wrappers."  These are essentially directories that are to be
  treated as "files."  This package allows such wrappers to be
  "processed" on the way in and out of CVS.  The intended use is to
  wrap up a wrapper into a single tar, such that that tar can be
  treated as a single binary file in CVS.  To solve the problem
  effectively, it was also necessary to be able to prevent rcsmerge
  application at appropriate times.

  ------------------
  Format of wrapper file ($CVSROOT/CVSROOT/cvswrappers or .cvswrappers)

  wildcard	[option value][option value]...

  where option is one of
  -m		update methodology	value: MERGE or COPY
  -k		default -k rcs option to use on import or add

  and value is a single-quote delimited value.

  E.g:
  *.nib		-m 'COPY'
*/


typedef struct {
    char *wildCard;
    char *rcsOption;
    WrapMergeMethod mergeMethod;
	int isRemote; /* Received from CVSROOT/cvswrappers on remote server */
} WrapperEntry;

static const char *wrap_default[] =
{
	"*.gif -kb",
	"*.pdf -kb",
	"*.bmp -kb",
	"*.jpg -kb",
	"*.jpeg -kb",
	"*.png -kb",
	"*.exe -kb",
	"*.dll -kb",
	"*.so -kb",
	"*.a -kb",
	"*.pdb -kb",
	"*.lib -kb",
	"*.o -kb",
	"*.res -kb",
	"*.class -kb",
	"*.ogg -kb",
	"*.mp3 -kb"
};

static WrapperEntry **wrap_list=NULL;
static WrapperEntry **wrap_saved_list=NULL;

static int wrap_size=0;
static int wrap_count=0;
static int wrap_tempcount=0;

/* FIXME: the relationship between wrap_count, wrap_tempcount,
 * wrap_saved_count, and wrap_saved_tempcount is not entirely clear;
 * it is certainly suspicious that wrap_saved_count is never set to a
 * value other than zero!  If the variable isn't being used, it should
 * be removed.  And in general, we should describe how temporary
 * vs. permanent wrappers are implemented, and then make sure the
 * implementation is actually doing that.
 *
 * Right now things seem to be working, but that's no guarantee there
 * isn't a bug lurking somewhere in the murk.
 */

static int wrap_saved_count=0;

static int wrap_saved_tempcount=0;

#define WRAPPER_GROW	8

void wrap_add_entry PROTO((WrapperEntry *e,int temp));
void wrap_kill PROTO((void));
void wrap_kill_temp PROTO((void));
void wrap_free_entry PROTO((WrapperEntry *e));
void wrap_free_entry_internal PROTO((WrapperEntry *e));
void wrap_restore_saved PROTO((void));

void wrap_setup()
{
    char *homedir;
    char *tmp, *ptr, *server_line;
    int len;
    int n;

    /* Add default wrappers */
    tmp = xmalloc(80);
    for(n=0; n<sizeof(wrap_default)/sizeof(wrap_default[0]); n++)
    {
       strncpy(tmp,wrap_default[n],80);
       wrap_add(tmp,0,0);
    }
    xfree(tmp);
#ifdef CLIENT_SUPPORT
    if (current_parsed_root && !current_parsed_root->isremote)
#endif
    {
		char *file;

		file = xmalloc (strlen (current_parsed_root->directory)
				+ sizeof (CVSROOTADM)
				+ sizeof (CVSROOTADM_WRAPPER)
				+ 3);
		/* Then add entries found in repository, if it exists.  */
		sprintf (file, "%s/%s/%s", current_parsed_root->directory, CVSROOTADM,
				CVSROOTADM_WRAPPER);
		if (isfile (file))
		{
			wrap_add_file(file,0);
		}
		xfree (file);
    }
#ifdef CLIENT_SUPPORT
	else if(supported_request("read-cvswrappers"))
	{
		TRACE(1,"Requesting server cvswrappers");
		send_to_server ("read-cvswrappers\012", 0);
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
			wrap_add(ptr,0,1);
			ptr=strtok(NULL,"\n");
		}
		xfree(tmp);
		xfree(server_line);
	}
#endif

    /* Then add entries found in home dir, (if user has one) and file
       exists.  */
    homedir = get_homedir ();
    /* If we can't find a home directory, ignore ~/.cvswrappers.  This may
       make tracking down problems a bit of a pain, but on the other
       hand it might be obnoxious to complain when CVS will function
       just fine without .cvswrappers (and many users won't even know what
       .cvswrappers is).  */
    if (homedir != NULL)
    {
	char *file;

	file = xmalloc (strlen (homedir) + sizeof (CVSDOTWRAPPER) + 10);
	sprintf (file, "%s/%s", homedir, CVSDOTWRAPPER);
	if (isfile (file))
	{
	    wrap_add_file (file, 0);
	}
	xfree (file);
    }

    /* FIXME: calling wrap_add() below implies that the CVSWRAPPERS
     * environment variable contains exactly one "wrapper" -- a line
     * of the form
     * 
     *    FILENAME_PATTERN	FLAG  OPTS [ FLAG OPTS ...]
     *
     * This may disagree with the documentation, which states:
     * 
     *   `$CVSWRAPPERS'
     *      A whitespace-separated list of file name patterns that CVS
     *      should treat as wrappers. *Note Wrappers::.
     *
     * Does this mean the environment variable can hold multiple
     * wrappers lines?  If so, a single call to wrap_add() is
     * insufficient.
     */

    /* Then add entries found in CVSWRAPPERS environment variable. */
    wrap_add (getenv (WRAPPER_ENV), 0, 0);
}

#ifdef CLIENT_SUPPORT
/* Send -W arguments for the wrappers to the server.  The command must
   be one that accepts them (e.g. update, import).  */
void wrap_send ()
{
    int i;

    for (i = 0; i < wrap_count + wrap_tempcount; ++i)
    {
		if(wrap_list[i]->isRemote) /* Don't resend exising wrappers back to the server */
			continue;
	if (wrap_list[i]->mergeMethod == WRAP_COPY)
	    /* For greater studliness we would print the offending option
	       and (more importantly) where we found it.  */
	    error (0, 0, "\
-m wrapper option is not supported remotely; ignored");
	if (wrap_list[i]->rcsOption != NULL)
	{
	    send_to_server ("Argument -W\012Argument ", 0);
	    send_to_server (wrap_list[i]->wildCard, 0);
	    send_to_server (" -k '", 0);
	    send_to_server (wrap_list[i]->rcsOption, 0);
	    send_to_server ("'\012", 0);
	}
    }
}
#endif /* CLIENT_SUPPORT */

#if defined(SERVER_SUPPORT) || defined(CLIENT_SUPPORT)
/* Output wrapper entries in the format of cvswrappers lines.
 *
 * This is useful when one side of a client/server connection wants to
 * send its wrappers to the other; since the receiving side would like
 * to use wrap_add() to incorporate the wrapper, it's best if the
 * entry arrives in this format.
 *
 * The entries are stored in `line', which is allocated here.  Caller
 * can xfree() it.
 *
 * If first_call_p is nonzero, then start afresh.  */
void wrap_unparse_rcs_options (char **line, int first_call_p)
{
    /* FIXME-reentrancy: we should design a reentrant interface, like
       a callback which fgets handed each wrapper (a multithreaded
       server being the most concrete reason for this, but the
       non-reentrant interface is fairly unnecessary/ugly).  */
    static int i;

    if (first_call_p)
        i = 0;

    for (; i < wrap_count + wrap_tempcount; ++i)
    {
	if (wrap_list[i]->rcsOption != NULL)
	{
            *line = xmalloc (strlen (wrap_list[i]->wildCard)
                             + strlen ("\t")
                             + strlen (" -k '")
                             + strlen (wrap_list[i]->rcsOption)
                             + strlen ("'")
                             + 1);  /* leave room for '\0' */
            
            strcpy (*line, wrap_list[i]->wildCard);
            strcat (*line, " -k '");
            strcat (*line, wrap_list[i]->rcsOption);
            strcat (*line, "'");

            /* We're going to miss the increment because we return, so
               do it by hand. */
            ++i;

            return;
	}
    }

    *line = NULL;
    return;
}
#endif /* SERVER_SUPPORT || CLIENT_SUPPORT */

/*
 * Open a file and read lines, feeding each line to a line parser. Arrange
 * for keeping a temporary list of wrappers at the end, if the "temp"
 * argument is set.
 */
void wrap_add_file (const char *file, int temp)
{
    FILE *fp;
    char *line = NULL;
    size_t line_allocated = 0;

    wrap_restore_saved ();
    wrap_kill_temp ();

    /* Load the file.  */
    fp = fopen (file, "r");
    if (fp == NULL)
    {
	if (!existence_error (errno))
	    error (0, errno, "cannot open %s", file);
	return;
    }
    while (getline (&line, &line_allocated, fp) >= 0)
	wrap_add (line, temp, 0);
    if (line)
        xfree (line);
    if (ferror (fp))
	error (0, errno, "cannot read %s", file);
    if (fclose (fp) == EOF)
	error (0, errno, "cannot close %s", file);
}

void wrap_kill()
{
    wrap_kill_temp();
    while(wrap_count)
	wrap_free_entry(wrap_list[--wrap_count]);
}

void wrap_kill_temp()
{
    WrapperEntry **temps=wrap_list+wrap_count;

    while(wrap_tempcount)
	wrap_free_entry(temps[--wrap_tempcount]);
}

void wrap_free_entry(WrapperEntry *e)
{
    wrap_free_entry_internal(e);
    xfree(e);
}

void wrap_free_entry_internal(WrapperEntry *e)
{
    xfree (e->wildCard);
    xfree (e->rcsOption);
}

void wrap_restore_saved()
{
    if(!wrap_saved_list)
	return;

    wrap_kill();

    xfree(wrap_list);

    wrap_list=wrap_saved_list;
    wrap_count=wrap_saved_count;
    wrap_tempcount=wrap_saved_tempcount;

    wrap_saved_list=NULL;
    wrap_saved_count=0;
    wrap_saved_tempcount=0;
}

void wrap_close()
{
	wrap_kill();
	xfree(wrap_list);
}

void wrap_add (char *line, int isTemp, int isRemote)
{
    char *temp;
    char *linetemp = line;
    char ctemp;
    WrapperEntry e;
    char opt;
	int hasQuote = 0;

    if (!line || line[0] == '#')
	return;

    TRACE(3,"wrap_add(%s, %d, %d)",PATCH_NULL(line),isTemp,isRemote);

    memset (&e, 0, sizeof(e));

	/* Search for the wild card */
    while (*line && isspace ((unsigned char) *line))
	++line;
	if((*line)=='"' || (*line)=='\'')
	{
		hasQuote=*line;
		line++;
	}
	for(temp=line;*line;++line)
	{
		if(hasQuote && (*line)==hasQuote)
		{
			hasQuote=0;
			break;
		}
		if(isspace ((unsigned char) *line) && !hasQuote)	// JMG 2000-02-10: Fixed logic type
			break;
	}
    if(temp==line)
	return;

    ctemp=*line;
    *line='\0';

    e.wildCard=xstrdup(temp);
    *line=ctemp;

    while(*line){
	    /* Search for the option */
	while(*line && *line!='-')
	    ++line;
	if(!*line)
	    break;
	++line;
	if(!*line)
	    break;
	opt=*line;

	/* Search for the filter commandline */
	for(++line;*line && isspace(*line); ++line)
	  ;
	hasQuote = (*line=='\'');
	if(hasQuote) line++;	
	
	if(!*line)
	{
	    error(0,0,"Bad cvswrappers line '%s'",linetemp);
	    break;
	}

	for(temp=line;*line && ((hasQuote && *line!='\'') || (!hasQuote && !isspace(*line)));++line)
	    ;

	if(hasQuote && (*line!='\'' || temp==line))
	{
	    error(0,0,"Bad cvswrappers line '%s'",linetemp);
	    break;
	}

	ctemp=*line;
	*line='\0';
	switch(opt){
	case 'm':
	    if(*temp=='C' || *temp=='c')
		e.mergeMethod=WRAP_COPY;
	    else
		e.mergeMethod=WRAP_MERGE;
	    break;
	case 'k':
	    if (e.rcsOption)
		xfree (e.rcsOption);
	    e.rcsOption = xstrdup (temp);
	    break;
	default:
	    break;
	}
	e.isRemote=isRemote;
	*line=ctemp;
	if(!*line)break;
	++line;
    }

    wrap_add_entry(&e, isTemp);
}

void wrap_add_entry(WrapperEntry *e, int temp)
{
    int x;
    if(wrap_count+wrap_tempcount>=wrap_size){
	wrap_size += WRAPPER_GROW;
	wrap_list = (WrapperEntry **) xrealloc ((char *) wrap_list,
						wrap_size *
						sizeof (WrapperEntry *));
    }

    if(!temp && wrap_tempcount){
	for(x=wrap_count+wrap_tempcount-1;x>=wrap_count;--x)
	    wrap_list[x+1]=wrap_list[x];
    }

    x=(temp ? wrap_count+(wrap_tempcount++):(wrap_count++));
    wrap_list[x]=(WrapperEntry *)xmalloc(sizeof(WrapperEntry));
    wrap_list[x]->wildCard=e->wildCard;
    wrap_list[x]->mergeMethod=e->mergeMethod;
    wrap_list[x]->rcsOption = e->rcsOption;
}

/* Return 1 if the given filename is a wrapper filename */
int wrap_name_has(const char   *name, WrapMergeHas  has)
{
    int x,count=wrap_count+wrap_tempcount;
    char *temp;

    for(x=0;x<count;++x)
	if (CVS_FNMATCH (wrap_list[x]->wildCard, name, 0) == 0){
	    switch(has){
	    case WRAP_RCSOPTION:
		temp = wrap_list[x]->rcsOption;
		break;
	    default:
	        abort ();
	    }
	    if(temp==NULL)
		return (0);
	    else
		return (1);
	}
    return (0);
}

static WrapperEntry *wrap_matching_entry (const char *name)
{
    int x,count=wrap_count+wrap_tempcount;

    /* TH: do this backwards, because the local entries should override
     * the global ones */
    for(x=count-1;x>=0;x--)
	if (CVS_FNMATCH (wrap_list[x]->wildCard, name, 0) == 0)
	    return wrap_list[x];
    return (WrapperEntry *)NULL;
}

/* Return the RCS options for FILENAME in a newly malloc'd string.  If
   ASFLAG, then include "-k" at the beginning (e.g. "-kb"), otherwise
   just give the option itself (e.g. "b").  */
char *wrap_rcsoption (const char *filename, int asflag)
{
    WrapperEntry *e = wrap_matching_entry (filename);
    char *buf;

    if (e == NULL || e->rcsOption == NULL || (*e->rcsOption == '\0'))
	return NULL;

    buf = xmalloc (strlen (e->rcsOption) + 3);
    if (asflag)
    {
	strcpy (buf, "-k");
	strcat (buf, e->rcsOption);
    }
    else
    {
	strcpy (buf, e->rcsOption);
    }
    return buf;
}

int wrap_merge_is_copy (const char *fileName)
{
    WrapperEntry *e=wrap_matching_entry(fileName);
    if(e==NULL || e->mergeMethod==WRAP_MERGE)
	return 0;

    return 1;
}

void wrap_display()
{
    int i;

    for (i = 0; i < wrap_count + wrap_tempcount; ++i)
    {
	if (wrap_list[i]->rcsOption != NULL)
	{
	    printf("%s -k '%s'\n",wrap_list[i]->wildCard,wrap_list[i]->rcsOption);
	}
    }
}
