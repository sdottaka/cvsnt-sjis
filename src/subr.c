/*
 * Copyright (c) 1992, Brian Berliner and Jeff Polk
 * Copyright (c) 1989-1992, Brian Berliner
 *
 * You may distribute under the terms of the GNU General Public License as
 * specified in the README file that comes with the CVS source distribution.
 *
 * Various useful functions for the CVS support code.
 */

#include "cvs.h"
#include "getline.h"

#ifdef _WIN32
#include <io.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <sys/socket.h>
#include <netdb.h>
#endif
#include <errno.h>

#include <stdarg.h>

#ifdef _WIN32
#define socket_errno WSAGetLastError()
#define EWOULDBLOCK WSAEWOULDBLOCK
#else
#define closesocket close
#define socket_errno errno
#endif

#ifdef HAVE_NANOSLEEP
# include "xtime.h"
#else /* HAVE_NANOSLEEP */
# if !defined HAVE_USLEEP && defined HAVE_SELECT
    /* use select as a workaround */
#   include "xselect.h"
# endif /* !defined HAVE_USLEEP && defined HAVE_SELECT */
#endif /* !HAVE_NANOSLEEP */

extern char *getlogin ();

#ifdef CVSGUI_PIPE
#include "cvsgui_protocol.h"
#endif

/*
 * malloc some data and die if it fails
 */
#ifdef _DEBUG
void *dbg_xmalloc(size_t bytes, const char *file, int line)
#else
void *xmalloc (size_t bytes)
#endif
{
    char *cp;

    /* Parts of CVS try to xmalloc zero bytes and then free it.  Some
       systems have a malloc which returns NULL for zero byte
       allocations but a free which can't handle NULL, so compensate. */
    if (bytes == 0)
		bytes = 1;

#ifdef _DEBUG
    cp = _malloc_dbg(bytes,_NORMAL_BLOCK,file,line);
#else
    cp = malloc (bytes);
#endif
    if (cp == NULL)
    {
		char buf[80];
		sprintf (buf, "out of memory; can not allocate %lu bytes",
			(unsigned long) bytes);
		error (1, 0, buf);
    }
    return (cp);
}

/*
 * realloc data and die if it fails [I've always wanted to have "realloc" do
 * a "malloc" if the argument is NULL, but you can't depend on it.  Here, I
 * can *force* it.
 */
#ifdef _DEBUG
void *dbg_xrealloc(void *ptr, size_t bytes, const char *file, int line)
#else
void *xrealloc (void *ptr, size_t bytes)
#endif
{
    char *cp;

#ifdef _DEBUG
    if (!ptr)
		cp = _malloc_dbg(bytes?bytes:1,_NORMAL_BLOCK,file,line);
	else
		cp = _realloc_dbg(ptr, bytes,_NORMAL_BLOCK,file,line);
#else
    if (!ptr)
		cp = malloc (bytes?bytes:1);
	else
		cp = realloc(ptr, bytes);
#endif

    if (cp == NULL)
    {
		char buf[80];
		sprintf (buf, "out of memory; can not reallocate %lu bytes",
			(unsigned long) bytes);
		error (1, 0, buf);
    }
    return (cp);
}

/* This is #defined from xfree()
   Automatically null the pointer after freeing it */
void xfree_s(void **ptr)
{
	if(*ptr)
	{
		free(*ptr);
	}
	*ptr=NULL;
}


/* Two constants which tune expand_string.  Having MIN_INCR as large
   as 1024 might waste a bit of memory, but it shouldn't be too bad
   (CVS used to allocate arrays of, say, 3000, PATH_MAX (8192, often),
   or other such sizes).  Probably anything which is going to allocate
   memory which is likely to get as big as MAX_INCR shouldn't be doing
   it in one block which must be contiguous, but since getrcskey does
   so, we might as well limit the wasted memory to MAX_INCR or so
   bytes.

   MIN_INCR and MAX_INCR should both be powers of two and we generally
   try to keep our allocations to powers of two for the most part.
   Most malloc implementations these days tend to like that.  */

#define MIN_INCR BUFSIZ
#define MAX_INCR (2*1024*1024)

/* *STRPTR is a pointer returned from xmalloc (or NULL), pointing to *N
   characters of space.  Reallocate it so that points to at least
   NEWSIZE bytes of space.  Gives a fatal error if out of memory;
   if it returns it was successful.  */
void expand_string (char **strptr, size_t *n, size_t newsize)
{
    if (*n < newsize)
    {
	while (*n < newsize)
	{
	    if (*n < MIN_INCR)
		*n = MIN_INCR;
	    else if (*n >= MAX_INCR)
		*n += MAX_INCR;
	    else
	    {
		*n *= 2;
		if (*n > MAX_INCR)
		    *n = MAX_INCR;
	    }
	}
	*strptr = xrealloc (*strptr, *n);
    }
}

/* *STR is a pointer to a malloc'd string.  *LENP is its allocated
   length.  Add SRC to the end of it, reallocating if necessary.  */
void
allocate_and_strcat (str, lenp, src)
    char **str;
    size_t *lenp;
    const char *src;
{

    expand_string (str, lenp, strlen (*str) + strlen (src) + 1);
    strcat (*str, src);
}

/*
 * Duplicate a string, calling xmalloc to allocate some dynamic space
 */
char *xstrdup (const char *str)
{
    char *s;

    if (str == NULL)
		return ((char *) NULL);
    s = xmalloc (strlen (str) + 1);
    strcpy (s, str);
    return s;
}

/* Remove trailing newlines from STRING, destructively. */
void
strip_trailing_newlines (str)
     char *str;
{
    int len;
    len = strlen (str) - 1;

    while (str[len] == '\n')
	str[len--] = '\0';
}

/* Return the number of levels that path ascends above where it starts.
   For example:
   "../../foo" -> 2
   "foo/../../bar" -> 1
   */
/* FIXME: Should be using ISDIRSEP, last_component, or some other
   mechanism which is more general than just looking at slashes,
   particularly for the client.c caller.  The server.c caller might
   want something different, so be careful.  */
int
pathname_levels (path)
    const char *path;
{
    const char *p;
    const char *q;
#ifdef _WIN32
	const char *q1,*q2;
#endif
    int level;
    int max_level;

    max_level = 0;
    p = path;
    level = 0;
    do
    {
#ifdef _WIN32
	q1 = strchr (p, '/');
	q2 = strchr (p, '\\');
	if(q1!=NULL && (q1<q2))
		q=q1;
	else if(q1==NULL && (q1>q2))
		q=q2;
	else
		q=q1; /* Probably NULL */
#else
	q = strchr (p, '/');
#endif

	if (q != NULL)
	    ++q;
	if (p[0] == '.' && p[1] == '.' && (p[2] == '\0' || ISDIRSEP(p[2])))
	{
	    --level;
	    if (-level > max_level)
		max_level = -level;
	}
	else if (p[0] == '\0' || ISDIRSEP(p[0]) ||
		 (p[0] == '.' && (p[1] == '\0' || ISDIRSEP(p[1]))))
	    ;
	else
	    ++level;
	p = q;
    } while (p != NULL);
    return max_level;
}


/* Free a vector, where (*ARGV)[0], (*ARGV)[1], ... (*ARGV)[*PARGC - 1]
   are malloc'd and so is *ARGV itself.  Such a vector is allocated by
   line2argv or expand_wild, for example.  */
void
free_names (pargc, argv)
    int *pargc;
    char **argv;
{
    register int i;

    for (i = 0; i < *pargc; i++)
    {					/* only do through *pargc */
	xfree (argv[i]);
    }
    xfree (argv);
    *pargc = 0;				/* and set it to zero when done */
}

/* Convert LINE into arguments separated by SEPCHARS.  Set *ARGC
   to the number of arguments found, and (*ARGV)[0] to the first argument,
   (*ARGV)[1] to the second, etc.  *ARGV is malloc'd and so are each of
   (*ARGV)[0], (*ARGV)[1], ...  Use free_names() to return the memory
   allocated here back to the free pool.  */
void line2argv (int *pargc, char ***argv, const char *line, const char *sepchars)
{
  const char *p;
  char *q,*qstart;
  int in_quote=0,escape=0;
  int argc_allocated = 10;
  *argv=(char**)xmalloc(sizeof(char*)*argc_allocated);
  *pargc=0;

  for(p=line;*p;p++)
  {
    while(strchr(sepchars,*p))
		p++;

	qstart=q=(char*)xstrdup(p);
    for(;*p;p++)
    {
      *(q++)=*p;
      if(!in_quote)
      {
        if(escape)
        {
          escape=0;
          continue;
        }
        if(*p=='\\')
        {
          escape=1;
		  q--;
          continue;
        }
        if(*p=='\'' || *p=='"')
        {
          in_quote=*p;
		  q--;
          continue;
        }
        if(strchr(sepchars,*p))
		{
		  q--;
		  break;
		}
      }
      else
      {
        if(*p==in_quote)
		{
          in_quote=0;
		  q--;
		  continue;
 		}
      }
    }
    *q='\0';
	(*argv)[(*pargc)++]=(char*)xrealloc(qstart,strlen(qstart)+1);
	if(*pargc==argc_allocated)
	{
		argc_allocated*=2;
		*argv=(char**)xrealloc(*argv,sizeof(char*)*argc_allocated);
	}
    if(!*p)
      break;
  }
  (*argv)[*pargc]=NULL;
}

/*
 * Returns the number of dots ('.') found in an RCS revision number
 */
int
numdots (s)
    const char *s;
{
    int dots = 0;

    for (; *s; s++)
    {
	if (*s == '.')
	    dots++;
    }
    return (dots);
}

/* Compare revision numbers REV1 and REV2 by consecutive fields.
   Return negative, zero, or positive in the manner of strcmp.  The
   two revision numbers must have the same number of fields, or else
   compare_revnums will return an inaccurate result. */
int
compare_revnums (rev1, rev2)
    const char *rev1;
    const char *rev2;
{
    const char *s, *sp;
    const char *t, *tp;
    char *snext, *tnext;
    int result = 0;

    sp = s = rev1;
    tp = t = rev2;
    while (result == 0)
    {
	result = strtoul (sp, &snext, 10) - strtoul (tp, &tnext, 10);
	if (*snext == '\0' || *tnext == '\0')
	    break;
	sp = snext + 1;
	tp = tnext + 1;
    }

    return result;
}

char *
increment_revnum (rev)
    const char *rev;
{
    char *newrev, *p;
    int lastfield;
    size_t len = strlen (rev);

    newrev = (char *) xmalloc (len + 2);
    memcpy (newrev, rev, len + 1);
    p = strrchr (newrev, '.');
    if (p == NULL)
    {
	xfree (newrev);
	return NULL;
    }
    lastfield = atoi (++p);
    sprintf (p, "%d", lastfield + 1);

    return newrev;
}

/* Return the username by which the caller should be identified in
   CVS, in contexts such as the author field of RCS files, various
   logs, etc.  */
const char *
getcaller ()
{
#ifndef SYSTEM_GETCALLER
    static char *cache;
#ifndef _WIN32
    struct passwd *pw;
    uid_t uid;
#endif
#endif

    /* If there is a CVS username, return it.  */
#ifdef SERVER_SUPPORT
    if (CVS_Username != NULL)
	return CVS_Username;
#endif

#ifdef SYSTEM_GETCALLER
    return SYSTEM_GETCALLER ();
#else
    /* Get the caller's login from his uid.  If the real uid is "root"
       try LOGNAME USER or getlogin(). If getlogin() and getpwuid()
       both fail, return the uid as a string.  */

    if (cache != NULL)
	return cache;

#ifndef _WIN32
    uid = getuid ();
    if (uid == (uid_t) 0)
#endif
    {
	char *name;

	/* super-user; try getlogin() to distinguish */
	if (((name = getlogin ()) || (name = getenv("LOGNAME")) ||
	     (name = getenv("USER"))) && *name)
	{
	    cache = xstrdup (name);
	    return cache;
	}
    }
#ifndef _WIN32
    if ((pw = (struct passwd *) getpwuid (uid)) == NULL)
    {
	char uidname[20];

	(void) sprintf (uidname, "uid%lu", (unsigned long) uid);
	cache = xstrdup (uidname);
	return cache;
    }
    cache = xstrdup (pw->pw_name);
    return cache;
#else
	/* Shouldn't get here... */
	cache=xstrdup("unknown-user");
	return cache;
#endif
#endif
}

#ifdef lint
#ifndef __GNUC__
/* ARGSUSED */
time_t
get_date (date, now)
    char *date;
    struct timeb *now;
{
    time_t foo = 0;

    return (foo);
}
#endif
#endif

/* Given two revisions, find their greatest common ancestor.  If the
   two input revisions exist, then rcs guarantees that the gca will
   exist.  */

char *
gca (rev1, rev2)
    const char *rev1;
    const char *rev2;
{
    int dots;
    char *gca;
    const char *p[2];
    int j[2];
    char *retval;

    if (rev1 == NULL || rev2 == NULL)
    {
	error (0, 0, "sanity failure in gca");
	abort();
    }

    /* The greatest common ancestor will have no more dots, and numbers
       of digits for each component no greater than the arguments.  Therefore
       this string will be big enough.  */
    gca = xmalloc (strlen (rev1) + strlen (rev2) + 100);

    /* walk the strings, reading the common parts. */
    gca[0] = '\0';
    p[0] = rev1;
    p[1] = rev2;
    do
    {
	int i;
	char c[2];
	char *s[2];

	for (i = 0; i < 2; ++i)
	{
	    /* swap out the dot */
	    s[i] = strchr (p[i], '.');
	    if (s[i] != NULL) {
		c[i] = *s[i];
	    }

	    /* read an int */
	    j[i] = atoi (p[i]);

	    /* swap back the dot... */
	    if (s[i] != NULL) {
		*s[i] = c[i];
		p[i] = s[i] + 1;
	    }
	    else
	    {
		/* or mark us at the end */
		p[i] = NULL;
	    }

	}

	/* use the lowest. */
	(void) sprintf (gca + strlen (gca), "%d.",
			j[0] < j[1] ? j[0] : j[1]);

    } while (j[0] == j[1]
	     && p[0] != NULL
	     && p[1] != NULL);

    /* back up over that last dot. */
    gca[strlen(gca) - 1] = '\0';

    /* numbers differ, or we ran out of strings.  we're done with the
       common parts.  */

    dots = numdots (gca);
    if (dots == 0)
    {
	/* revisions differ in trunk major number.  */

	char *q;
	const char *s;

	s = (j[0] < j[1]) ? p[0] : p[1];

	if (s == NULL)
	{
	    /* we only got one number.  this is strange.  */
	    error (0, 0, "bad revisions %s or %s", rev1, rev2);
	    abort();
	}
	else
	{
	    /* we have a minor number.  use it.  */
	    q = gca + strlen (gca);

	    *q++ = '.';
	    for ( ; *s != '.' && *s != '\0'; )
		*q++ = *s++;

	    *q = '\0';
	}
    }
    else if ((dots & 1) == 0)
    {
	/* if we have an even number of dots, then we have a branch.
	   remove the last number in order to make it a revision.  */

	char *s;

	s = strrchr(gca, '.');
	*s = '\0';
    }

    retval = xstrdup (gca);
    xfree (gca);
    return retval;
}

/* Give fatal error if REV is numeric and ARGC,ARGV imply we are
   planning to operate on more than one file.  The current directory
   should be the working directory.  Note that callers assume that we
   will only be checking the first character of REV; it need not have
   '\0' at the end of the tag name and other niceties.  Right now this
   is only called from admin.c, but if people like the concept it probably
   should also be called from diff -r, update -r, get -r, and log -r.  */

void
check_numeric (rev, argc, argv)
    const char *rev;
    int argc;
    char **argv;
{
    if (rev == NULL || !isdigit ((unsigned char) *rev))
	return;

    /* Note that the check for whether we are processing more than one
       file is (basically) syntactic; that is, we don't behave differently
       depending on whether a directory happens to contain only a single
       file or whether it contains more than one.  I strongly suspect this
       is the least confusing behavior.  */
    if (argc != 1
	|| isdir (argv[0]))
    {
	error (0, 0, "while processing more than one file:");
	error (1, 0, "attempt to specify a numeric revision");
    }
}

/*
 *  Sanity checks and any required fix-up on message passed to RCS via '-m'.
 *  RCS 5.7 requires that a non-total-whitespace, non-null message be provided
 *  with '-m'.  Returns a newly allocated, non-empty buffer with whitespace
 *  stripped from end of lines and end of buffer.
 *
 *  TODO: We no longer use RCS to manage repository files, so maybe this
 *  nonsense about non-empty log fields can be dropped.
 */
char *
make_message_rcslegal (message)
     char *message;
{
    char *dst, *dp, *mp;

    if (message == NULL) message = "";

    /* Strip whitespace from end of lines and end of string. */
    dp = dst = (char *) xmalloc (strlen (message) + 1);
    for (mp = message; *mp != '\0'; ++mp)
    {
	if (*mp == '\n')
	{
	    /* At end-of-line; backtrack to last non-space. */
	    while (dp > dst && (dp[-1] == ' ' || dp[-1] == '\t'))
		--dp;
	}
	*dp++ = *mp;
    }

    /* Backtrack to last non-space at end of string, and truncate. */
    while (dp > dst && isspace ((unsigned char) dp[-1]))
	--dp;
    *dp = '\0';

    /* After all that, if there was no non-space in the string,
       substitute a non-empty message. */
    if (*dst == '\0')
    {
	xfree (dst);
	dst = xstrdup ("*** empty log message ***");
    }

    return dst;
}

/* Does the file FINFO contain conflict markers?  The whole concept
   of looking at the contents of the file to figure out whether there are
   unresolved conflicts is kind of bogus (people do want to manage files
   which contain those patterns not as conflict markers), but for now it
   is what we do.  */
int file_has_markers (const struct file_info *finfo)
{
    FILE *fp;
    char *line = NULL;
    size_t line_allocated = 0;
    int result;

    result = 0;
    fp = CVS_FOPEN (finfo->file, "r");
    if (fp == NULL)
	error (1, errno, "cannot open %s", fn_root(finfo->fullname));
    while (getline (&line, &line_allocated, fp) > 0)
    {
		if (strncmp (line, RCS_MERGE_PAT_1, sizeof RCS_MERGE_PAT_1 - 1) == 0 ||
			strncmp (line, RCS_MERGE_PAT_2, sizeof RCS_MERGE_PAT_2 - 1) == 0 ||
			strncmp (line, RCS_MERGE_PAT_3, sizeof RCS_MERGE_PAT_3 - 1) == 0)
		{
			result = 1;
			goto out;
		}
    }
    if (ferror (fp))
		error (0, errno, "cannot read %s", fn_root(finfo->fullname));
out:
    if (fclose (fp) < 0)
		error (0, errno, "cannot close %s", fn_root(finfo->fullname));
    if (line != NULL)
		xfree (line);
    return result;
}

/* Read the entire contents of the file NAME into *BUF.
   If NAME is NULL, read from stdin.  *BUF
   is a pointer returned from xmalloc (or NULL), pointing to *BUFSIZE
   bytes of space.  The actual size is returned in *LEN.  On error,
   give a fatal error.  The name of the file to use in error messages
   (typically will include a directory if we have changed directory)
   is FULLNAME.  MODE is "r" for text or "rb" for binary.  */

void get_file (const char *name, const char *fullname, const char *mode,
			char **buf, size_t *bufsize, size_t *len, kflag kf)
{
    struct stat s;
    size_t nread;
    char *tobuf;
    FILE *e;
    size_t filesize;
	char tmp[256];
	size_t siz;
	int unicode=0;

	TRACE(2,"get_file(%s,%s,%s,%04x)",name?name:"stdin",PATCH_NULL(fullname),PATCH_NULL(mode),kf);

    if (name == NULL)
    {
		e = stdin;
		filesize = 100;	/* force allocation of minimum buffer */
    }
    else
    {
	/* Although it would be cleaner in some ways to just read
	   until end of file, reallocating the buffer, this function
	   does get called on files in the working directory which can
	   be of arbitrary size, so I think we better do all that
	   extra allocation.  */

		if (CVS_STAT (name, &s) < 0)
			error (1, errno, "can't stat %s", fullname);

		/* Convert from signed to unsigned.  */
		filesize = s.st_size;

		/* We don't have to do this test on the server, as it should already
		   be handling UTF8 */
		if(!server_active && !strcasecmp(mode,"r") && !(filesize&1))
		{
			/* Text file with even number of characters, might be unicode */
			e = open_file (name, "rb");
			siz = fread(tmp,1,sizeof(tmp),e);
			if(file_encoding(tmp,siz,NULL,(kf.flags&KFLAG_ENCODED)?kf.encoding:ENC_UNKNOWN))
			{
				mode="rb";
				unicode=1;
			}
			fclose(e);
		}

		e = open_file (name, mode);
    }

    if (*buf == NULL || *bufsize <= filesize)
    {
		*bufsize = filesize + 1;
		*buf = xrealloc (*buf, *bufsize);
    }

    tobuf = *buf;
    nread = 0;
    while (1)
    {
		size_t got;

		got = fread (tobuf, 1, *bufsize - (tobuf - *buf), e);
		if (ferror (e))
			error (1, errno, "can't read %s", fullname);

		nread += got;
		tobuf += got;

		if (feof (e))
			break;

		/* Allocate more space if needed.  */
		if (tobuf == *buf + *bufsize)
		{
			int c;
			long off;

			c = getc (e);
			if (c == EOF)
			break;
			off = tobuf - *buf;
			expand_string (buf, bufsize, *bufsize + 100);
			tobuf = *buf + off;
			*tobuf++ = c;
			++nread;
		}
	}

	if (e != stdin && fclose (e) < 0)
		error (0, errno, "cannot close %s", fullname);

	/* If this is a unicode file, convert it to utf8 */
	/* Also converts crlf->lf if required as the routine above will not
	have done it */
	if(unicode)
		nread=convert_encoding_to_utf8(buf,nread,bufsize,ENC_UCS2LE);

	*len = nread;

	/* Force *BUF to be large enough to hold a null terminator. */
	if (nread == *bufsize)
		expand_string (buf, bufsize, *bufsize + 1);
	(*buf)[nread] = '\0';

	if(trace>1)
	{
		TRACE(2,"get_file -> %s",PATCH_NULL(*buf));
	}
}


/* Follow a chain of symbolic links to its destination.  FILENAME
   should be a handle to a malloc'd block of memory which contains the
   beginning of the chain.  This routine will replace the contents of
   FILENAME with the destination (a real file).  */

void
resolve_symlink (filename)
     char **filename;
{
    if ((! filename) || (! *filename))
	return;

    while (islink (*filename))
    {
#ifdef HAVE_READLINK
	char *newname;
	/* The clean thing to do is probably to have each filesubr.c
	   implement this (with an error if not supported by the
	   platform, in which case islink would presumably return 0).
	   But that would require editing each filesubr.c and so the
	   expedient hack seems to be looking at HAVE_READLINK.  */
	newname = xreadlink (*filename);
#else
	error (1, 0, "internal error: islink doesn't like readlink");
#endif

#ifdef HAVE_READLINK
	if (isabsolute (newname))
	{
	    xfree (*filename);
	    *filename = newname;
	}
	else
	{
	    const char *oldname = last_component (*filename);
	    int dirlen = oldname - *filename;
	    char *fullnewname = xmalloc (dirlen + strlen (newname) + 1);
	    strncpy (fullnewname, *filename, dirlen);
	    strcpy (fullnewname + dirlen, newname);
	    xfree (newname);
	    xfree (*filename);
	    *filename = fullnewname;
	}
#endif
    }
}

/*
 * Rename a file to an appropriate backup name based on BAKPREFIX.
 * If suffix non-null, then ".<suffix>" is appended to the new name.
 *
 * Returns the new name, which caller may xfree() if desired.
 */
char *
backup_file (filename, suffix)
     const char *filename;
     const char *suffix;
{
    char *backup_name;

    if (suffix == NULL)
    {
        backup_name = xmalloc (sizeof (BAKPREFIX) + strlen (filename) + 1);
        sprintf (backup_name, "%s%s", BAKPREFIX, filename);
    }
    else
    {
        backup_name = xmalloc (sizeof (BAKPREFIX)
                               + strlen (filename)
                               + strlen (suffix)
                               + 2);  /* one for dot, one for trailing '\0' */
        sprintf (backup_name, "%s%s.%s", BAKPREFIX, filename, suffix);
    }

    if (isfile (filename))
        copy_file (filename, backup_name, 0, 1);

    return backup_name;
}

/*
 * Copy a string into a buffer escaping any shell metacharacters.  The
 * buffer should be at least twice as long as the string.
 *
 * Returns a pointer to the terminating NUL byte in buffer.
 */

char *shell_escape(char *buf, const char *str)
{
    static const char meta[] = "$`\\\" ";
    const char *p;
	char *buf_in = buf;

    for (;;)
    {
	p = strpbrk(str, meta);
	if (!p) p = str + strlen(str);
	if (p > str)
	{
	    memcpy(buf, str, p - str);
	    buf += p - str;
	}
	if (!*p) break;
	*buf++ = '\\';
	*buf++ = *p++;
	str = p;
    }
    *buf = '\0';
    return buf_in;
}

/*
 * We can only travel forwards in time, not backwards.  :)
 */
void
sleep_past (desttime)
    time_t desttime;
{
    time_t t;
    long s;
    long us;

    while (time (&t) <= desttime)
    {
#ifdef HAVE_GETTIMEOFDAY
	struct timeval tv;
	gettimeofday (&tv, NULL);
	if (tv.tv_sec > desttime)
	    break;
	s = desttime - tv.tv_sec;
	if (tv.tv_usec > 0)
	    us = 1000000 - tv.tv_usec;
	else
	{
	    s++;
	    us = 0;
	}
#else
	/* default to 20 ms increments */
	s = desttime - t;
	us = 20000;
#endif

#if defined(HAVE_NANOSLEEP)
	{
	    struct timespec ts;
	    ts.tv_sec = s;
	    ts.tv_nsec = us * 1000;
	    (void)nanosleep (&ts, NULL);
	}
#elif defined(HAVE_USLEEP)
	if (s > 0)
	    (void)sleep (s);
	else
	    (void)usleep (us);
#elif defined(HAVE_SELECT)
	{
	    /* use select instead of sleep since it is a fairly portable way of
	     * sleeping for ms.
	     */
	    struct timeval tv;
	    tv.tv_sec = s;
	    tv.tv_usec = us;
	    (void)select (0, (fd_set *)NULL, (fd_set *)NULL, (fd_set *)NULL, &tv);
	}
#else
	if (us > 0) s++;
	(void)sleep(s);
#endif
    }
}

/* Like strtok, but can return empty tokens */
char *cvs_strtok(char *buffer, const char *tokens)
{
	static char *lastptr = NULL;
	char *p;

	if(!buffer)
		buffer=lastptr;
	if(!buffer)
		return NULL;

	p=buffer;
	while(*p && !strchr(tokens,*p))
		p++;
	if(*p)
	{
		*p='\0';
		lastptr=p+1;
	}
	else
		lastptr=NULL;
	return buffer;
}

void cvs_trace(int level, const char *fmt, ...)
{
	if((!server_active || allow_trace) && trace >= level)
	{
		va_list va;
		char str[1024];

		va_start(va, fmt);
		fprintf(stderr,"%c -> ",server_active?'S':' ');
		vsnprintf(str, 1024, fmt, va);
		fprintf(stderr,"%s\n",str);
#ifdef _DEBUG
		OutputDebugString(str);
		OutputDebugString("\n");
#endif
		va_end(va);
	}
}

#ifdef SERVER_SUPPORT
#undef printf
#undef fprintf
#undef vfprintf
#undef putchar
#undef puts
#undef fputs

void cvs_printf(const char *fmt, ...)
{
	static char line_buffer[4096];

	va_list va;
	va_start(va,fmt);
	vsprintf(line_buffer,fmt,va);

	cvs_output(line_buffer,0);
}

void cvs_putchar(int c)
{
	char s[2]={c,0};
	cvs_output(s,1);
}

void cvs_puts(const char *s)
{
	cvs_output(s,0);
}

int cvs_vfprintf(FILE*f, const char *fmt, va_list va)
{
	static char line_buffer[4096];

	if(!server_active || (f!=stdout && f!=stderr))
		return vfprintf(f,fmt,va);

	vsprintf(line_buffer,fmt,va);
	if(f==stdout)
		cvs_output(line_buffer,0);
	else if(f==stderr)
		cvs_outerr(line_buffer,0);
	return strlen(line_buffer);
}

int cvs_fprintf(FILE*f, const char *fmt, ...)
{
	int ret;

	va_list va;
	va_start(va,fmt);

	ret = cvs_vfprintf(f,fmt,va);

	va_end(va);

	return ret;
}

int cvs_fputs(const char *s, FILE *f)
{
	if(!server_active || (f!=stdout && f!=stderr))
		return fputs(s,f);

	if(f==stdout)
		cvs_output(s,0);
	else if(f==stderr)
		cvs_outerr(s,0);
	return strlen(s);
}

#endif

#if !defined(_WIN32) && defined(HAVE_PUTENV)
void cvs_putenv(const char *variable, const char *value)
{
	char *tmp = xmalloc(strlen(variable)+strlen(value)+2);
	sprintf(tmp,"%s=%s",variable,value);
	putenv(tmp);
	/* No need to free tmp as it's owned by the putenv function */
}
#endif

int cvs_tcp_connect(const char *servername, const char *port, int supress_errors)
{
	struct addrinfo hint = {0};
	struct addrinfo *tcp_addrinfo;
	int res,sock;
	size_t b;
	int err;

	hint.ai_flags=supress_errors?0:AI_CANONNAME;
	hint.ai_socktype=SOCK_STREAM;
	if((res=getaddrinfo(servername,port,&hint,&tcp_addrinfo))!=0)
	{
		if(!supress_errors)
			error(0,0, "Error connecting to host %s: %s\n", servername, gai_strerror(socket_errno));
		return -1;
	}

    sock = socket(tcp_addrinfo->ai_family, tcp_addrinfo->ai_socktype, tcp_addrinfo->ai_protocol);
    if (sock == -1)
	{
		if(!supress_errors)
			error(0,0, "cannot create socket: %s", gai_strerror(socket_errno));
		return -1;
	}

	if(supress_errors)
	{
#ifdef _WIN32
		b=1;
		ioctlsocket(sock,FIONBIO,&b);
#else	
		b=fcntl(sock,F_GETFL,0);
		fcntl(sock,F_SETFL,b|O_NONBLOCK);
#endif
	}

	/* If errors are supressed we use a nonblocking connect with a 1000us select... this is enough
	   for a connect to localhost (used by the agent code) and isn't going to be noticed by the user */
	if(connect (sock, (struct sockaddr *) tcp_addrinfo->ai_addr, tcp_addrinfo->ai_addrlen) <0)
	{
		err = socket_errno;
		if(err==EWOULDBLOCK)
		{
			fd_set fds;
			struct timeval tv = { 0,1000 };
			FD_ZERO(&fds);
			FD_SET(sock,&fds);
			err = select(sock,NULL,&fds,NULL,&tv);
			if(err!=1)
			{
				if(!supress_errors)
					error(0,0, "connect to %s(%s):%s failed: %s", servername, tcp_addrinfo->ai_canonname, port, gai_strerror(socket_errno));
				closesocket(sock);
				return -1;
			}
		}
		else
		{
			if(!supress_errors)
				error(0,0, "connect to %s(%s):%s failed: %s", servername, tcp_addrinfo->ai_canonname, port, gai_strerror(socket_errno));
			closesocket(sock);
			return -1;
		}
	}

	if(supress_errors)
	{
#ifdef _WIN32
		b=0;
		ioctlsocket(sock,FIONBIO,&b);
#else	
		b=fcntl(sock,F_GETFL,0);
		fcntl(sock,F_SETFL,b&~O_NONBLOCK);
#endif
	}

	freeaddrinfo(tcp_addrinfo);

	return sock;
}

int cvs_tcp_close(int sock)
{
	return closesocket(sock);
}

/* Remove repository prefix from displayed paths */
const char *fn_root(const char *path)
{
	const char *endp;
	static char fnroot[MAX_PATH]; 

	if(!path)
	  return NULL;

	if(!current_parsed_root || !current_parsed_root->directory || !current_parsed_root->unparsed_directory)
	  return path;

	if(!pathncmp(path,current_parsed_root->directory,strlen(current_parsed_root->directory),&endp))
	{
		strcpy(fnroot,current_parsed_root->unparsed_directory);
		strcat(fnroot,endp);
		return fnroot;
	}
	else
		return path;
}

const char *client_where(const char *path)
{
		return path;
}

char read_key()
{
#ifdef _WIN32
  INPUT_RECORD buffer;
  DWORD EventsRead;
  char ch;
  HANDLE hInput = GetStdHandle(STD_INPUT_HANDLE);
  BOOL CharRead = FALSE;

  /* loop until we find a valid keystroke (a KeyDown event, and NOT a
     SHIFT, ALT, or CONTROL keypress by itself) */
  while(!CharRead)
  {
	while( !CharRead && ReadConsoleInput(hInput, &buffer, 1, &EventsRead ) &&
			EventsRead > 0 )
	{
		if( buffer.EventType == KEY_EVENT &&
			buffer.Event.KeyEvent.bKeyDown )
		{
			ch = buffer.Event.KeyEvent.uChar.AsciiChar;
			if(ch)
				CharRead = TRUE;
		}
	}
  }

  return ch;
#else
  return getchar();
#endif
}

int yesno_prompt(const char *message, const char *title, int withcancel)
{
#ifdef CVSGUI_PIPE
	if(_cvsgui_readfd != 0)
	{
		char *response;

		/* Send to cvsgui in a format that it'll understand */
		fflush (stderr);
		fflush (stdout);

		printf("Question: %s\n",title);
		printf("%s\n",message);
		printf("Enter: Yes/No%s\n",withcancel?"/Cancel":"");
		fflush(stdout);

		response = getenv("CVSLIB_YESNO");
		if(!response)
		{
			printf("CVSGUI protocol error - null response\n");
			return -1; /* Something went wrong... assume Cancel */
		}

		switch(tolower(response[0]))
		{
		case 'y':
			return 1;
		case 'n':
			return 0;
		case 'c':
		case 'q':
			return -1;
		default:
			printf("CVSGUI protocol error - don't understand '%s\n",response);
			return -1;
		}
	}
	else
#endif /* CVSGUI_PIPE*/
	{
		char c;

		fflush (stderr);
		fflush (stdout);
		fflush (stdin);

		printf("%s",message);
		fflush(stdout);
		for(;;)
		{
			c=getchar();
			if(tolower(c)=='y' || c=='\n' || c=='\r')
			{
				fflush (stdin);
				return 1;
			}
			if(withcancel && (c==27 || tolower(c)=='c'))
			{
				fflush (stdin);
				return -1;
			}
			if(tolower(c)=='n' || (!withcancel && c==27))
			{
				fflush (stdin);
				return 0;
			}
		}
	}
}

/* Get the full pathname of file (assuming the working directory) */
char *fullpathname(const char *name, const char **shortname)
{
	char *path;

    if (isabsolute(name))
		path = xstrdup (name);
    else
    {
		char *dir = xgetwd_mapped();
		path = (char *) xmalloc (strlen(dir) + strlen(name) + 2);
		sprintf (path, "%s/%s", dir, name);
		xfree (dir);
    }

	if(shortname)
	{
		char *p = path;
		*shortname = p;
		for(;*p;p++)
			if(ISDIRSEP(*p))
				*shortname = p+1;
	}
	return path;
}

char *find_rcs_filename(const char *path)
{
	int found = 0;
	char *tmp=xmalloc(strlen(path)+32);
	const char *p = strrchr(path,'/');
#ifdef _WIN32
	const char *q = strrchr(path,'\\');
	if(q>p)
		p=q;
#endif

	do
	{
		sprintf(tmp,"%s,v",path);
		if(isfile(tmp))
			break;
		sprintf(tmp,"%s",path);
		if(isfile(tmp))
			break;
		sprintf(tmp,"RCS/%s",path);
		if(isfile(tmp))
			break;
		sprintf(tmp,"RCS/%s,v",path);
		if(isfile(tmp))
			break;
		if(p)
		{
			sprintf(tmp,"%-*.*s/Attic/%s,v",(int)(p-path),(int)(p-path),path,p+1);
			if(isfile(tmp))
				break;
			sprintf(tmp,"%-*.*s/Attic/%s",(int)(p-path),(int)(p-path),path,p);
			if(isfile(tmp))
				break;
			sprintf(tmp,"RCS/%-*.*s/Attic/%s,v",(int)(p-path),(int)(p-path),path,p);
			if(isfile(tmp))
				break;
			sprintf(tmp,"RCS/%-*.*s/Attic/%s",(int)(p-path),(int)(p-path),path,p);
			if(isfile(tmp))
				break;
		}

		xfree(tmp);
	} while(0);

	if(tmp)
		tmp=normalize_path(tmp);
	return tmp;
}

int case_isfile(char *name, char **realname)
{
    struct dirent *dp;
    DIR *dirp;
    char *dir;
    char *fname;
	int ret=0;

    /* Separate NAME into directory DIR and filename within the directory
       FNAME.  */
    dir = xstrdup (name);
    fname = strrchr (dir, '/');
    if (fname == NULL)
	{
		xfree(dir);
		dir = xstrdup(".");
		fname = name;
	}
	else
	{
	    *fname++ = '\0';
	}

    dirp = CVS_OPENDIR (dir);
    if (dirp == NULL)
    {
	    /* Give a fatal error; that way the error message can be
	       more specific than if we returned the error to the caller.  */
	    error (1, errno, "cannot read directory %s", dir);
	}

	errno = 0;
    while ((dp = CVS_READDIR (dirp)) != NULL)
    {
		if (strcasecmp (dp->d_name, fname) == 0)
		{
			if(realname)
				*realname = xstrdup(dp->d_name);
			ret = !strcmp(dp->d_name,name);
			break;
		}
    }

	if(!dp)
	{
		if(realname)
			*realname=NULL;
		ret=0;
	}
    if (errno != 0)
		error (1, errno, "cannot read directory %s", dir);

	CVS_CLOSEDIR (dirp);
	xfree(dir);
	return ret;
}

int get_local_time_offset()
{
	time_t t;
	struct tm l,g;

	time(&t);
	l = *localtime (&t);
	g = *gmtime (&t);
	return difftm (&l, &g);
}

char *xgetwd_mapped()
{
	char *tmp = xgetwd(),*dir;
	if(!current_parsed_root || !current_parsed_root->mapped_directory)
		return tmp;

	if(fnncmp(tmp,current_parsed_root->mapped_directory,strlen(current_parsed_root->mapped_directory)))
		return tmp;
	dir = xmalloc(strlen(current_parsed_root->mapped_directory)+strlen(tmp)+2);
	strcpy(dir,current_parsed_root->directory);
	strcat(dir,tmp+strlen(current_parsed_root->mapped_directory));
	xfree(tmp);
	return dir;
}

int get_cached_password(const char *key, char *buffer, int buffer_len)
{
	int sock;

#ifdef SERVER_SUPPORT
	/* This should never get called on the server...  just in case */
	if(server_active)
		return -1;
#endif
	/* No song and dance.. if there's no server listening, do nothing */
	if((sock = cvs_tcp_connect("127.0.0.1","32401", 1))<0)
		return -1;
	if(send(sock,key,strlen(key),0)<=0)
	{
		TRACE(1,"Error sending to passwd agent");
		return -1;
	}
	if(recv(sock,buffer,buffer_len,0)<=0)
	{
		TRACE(1,"Error receiving from passwd agent");
		return -1;
	}
	if(buffer[0]==-1) /* No passwd */
	{
		TRACE(2,"No password stored in passwd agent");
		return -1;
	}
	closesocket(sock);
	return 0;
}

const char *relative_repos(const char *directory)
{
	if(!strncmp(directory,current_parsed_root->directory,strlen(current_parsed_root->directory)))
		directory+=strlen(current_parsed_root->directory);
	if(*directory)
		directory++;
	return directory;
}
