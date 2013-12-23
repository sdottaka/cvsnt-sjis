/*
 * Copyright (c) 1992, Brian Berliner and Jeff Polk
 *
 * You may distribute under the terms of the GNU General Public License as
 * specified in the README file that comes with the CVS source distribution.
 *
 * The routines contained in this file do all the rcs file parsing and
 * manipulation
 */

#include "cvs.h"
#include "edit.h"
#include "hardlink.h"
#include "../cvsdelta/cvsdelta.h"
#include <zlib.h>

/* for ntohl */
#if defined(_WIN32)
  #include <winsock2.h>
#else
  #include <netinet/in.h>
#endif

/* The RCS -k options */
/* General form is [encoding][flags...] */
const kflag_t kflag_encoding[] =
{
  'b', NULL, NULL, 0, KFLAG_BINARY, KFLAG_LEGACY, 0,
  'B', NULL, NULL, 0, KFLAG_BINARY|KFLAG_BINARY_DELTA, KFLAG_CVSNT, 'b',
  'u', "ucs2le_bom", "utf16le_bom", ENC_UCS2LE_BOM, KFLAG_ENCODED, KFLAG_CVSNT|KFLAG_ESSENTIAL, 0, /* If this is on a file, a legacy client can't handle it */
  't', NULL, NULL, 0, KFLAG_TEXT, KFLAG_CVSNT, 0, /* This is never actually sent */
  -1, "ucs2le", "utf16le", ENC_UCS2LE, KFLAG_ENCODED, KFLAG_CVSNT|KFLAG_ESSENTIAL, 0, 
  -1, "ucs2be_bom", "utf16be_bom", ENC_UCS2BE_BOM, KFLAG_ENCODED, KFLAG_CVSNT|KFLAG_ESSENTIAL, 0, 
  -1, "ucs2be", "utf16be", ENC_UCS2BE, KFLAG_ENCODED, KFLAG_CVSNT|KFLAG_ESSENTIAL, 0, 
  -1, "ucs4le", "utf32le", ENC_UCS4LE, KFLAG_ENCODED, KFLAG_CVSNT|KFLAG_ESSENTIAL, 0, 
  -1, "ucs4be", "utf32be", ENC_UCS4BE, KFLAG_ENCODED, KFLAG_CVSNT|KFLAG_ESSENTIAL, 0, 
  -1, "ucs4le_bom", "utf32le_bom", ENC_UCS4LE_BOM, KFLAG_ENCODED, KFLAG_CVSNT|KFLAG_ESSENTIAL, 0, 
  -1, "ucs4be_bom", "utf32be_bom", ENC_UCS4BE_BOM, KFLAG_ENCODED, KFLAG_CVSNT|KFLAG_ESSENTIAL, 0, 
  -1, "shiftjis", NULL, ENC_SHIFTJIS, KFLAG_ENCODED, KFLAG_CVSNT|KFLAG_ESSENTIAL, 0,
  '{', NULL, NULL, 0, 0, KFLAG_CVSNT|KFLAG_ESSENTIAL, 0,
  0
};

const kflag_t kflag_flags[] =
{
  'c', NULL, NULL, 0, KFLAG_RESERVED_EDIT, KFLAG_CVSNT, 0,
  'k', NULL, NULL, 0, KFLAG_KEYWORD, KFLAG_LEGACY, 0,
  'v', NULL, NULL, 0, KFLAG_VALUE, KFLAG_LEGACY, 0,
  'V', NULL, NULL, 0, KFLAG_VALUE_LOGONLY, KFLAG_CVSNT, 0, /* internal use only */
  'l', NULL, NULL, 0, KFLAG_LOCKER, KFLAG_LEGACY, 0,
  'L', NULL, NULL, 0, KFLAG_UNIX, KFLAG_CVSNT, 0,
  'o', NULL, NULL, 0, KFLAG_PRESERVE, KFLAG_LEGACY, 0,
  'z', NULL, NULL, 0, KFLAG_COMPRESS_DELTA, KFLAG_CVSNT, 0,
  0
};

static void rcsbuf_seek(struct rcsbuffer *rcsbuf, off_t pos);
static RCSNode *RCS_parsercsfile_i (FILE * fp, const char *rcsfile);
static char *RCS_getdatebranch (RCSNode * rcs, char *date, char *branch);
static void rcsbuf_open(struct rcsbuffer *rcsbuf, FILE *fp, const char *filename, off_t pos);
static int rcsbuf_getkey (struct rcsbuffer *, char **keyp,
				 char **valp);
static int rcsbuf_getrevnum (struct rcsbuffer *, char **revp);
static char *rcsbuf_fill(struct rcsbuffer *, char *ptr, char **ptr1, char **ptr2);
static int rcsbuf_valcmp (struct rcsbuffer *);
static char *rcsbuf_valcopy (struct rcsbuffer *, char *val, int polish,
				    size_t *lenp);
static void rcsbuf_valpolish (struct rcsbuffer *, char *val, int polish,
				     size_t *lenp);
static void rcsbuf_valpolish_internal (struct rcsbuffer *, char *to,
					      const char *from, size_t *lenp);
static off_t rcsbuf_ftell (struct rcsbuffer *);
static void rcsbuf_get_buffered (struct rcsbuffer *, char **datap,
					size_t *lenp);
static int checkmagic_proc (Node *p, void *closure);
static void do_branches (List * list, char *val);
static void do_symbols (List * list, char *val);
static void do_locks (List * list, char *val);
static void free_rcsnode_contents (RCSNode *);
static void free_rcsvers_contents (RCSVers *);
static void rcsvers_delproc (Node * p);
static char *translate_symtag (RCSNode *, const char *);
static char *RCS_addbranch (RCSNode *, const char *);
static char *truncate_revnum_in_place (char *);
static char *truncate_revnum (const char *);
static char *printable_date (const char *);
static char *escape_keyword_value (const char *, int *);
static void expand_keywords (RCSNode *, RCSVers *, const char *,
				   const char *, size_t, kflag, char *,
				   size_t, char **, size_t *);
static void cmp_file_buffer (void *, const char *, size_t);

/* Routines for reading, parsing and writing RCS files. */
static RCSVers *getdelta (struct rcsbuffer *, char *, char **,
				 char **);
static Deltatext *RCS_getdeltatext (RCSNode *, FILE *,
					   struct rcsbuffer *);
static void freedeltatext (Deltatext *);

static void RCS_putadmin (RCSNode *, FILE *);
static void RCS_putdtree (RCSNode *rcs, char *rev, FILE *fp);
static void RCS_putdesc (RCSNode *, FILE *);
static void putdelta(RCSVers *vers, FILE *fp);
static int putrcsfield_proc (Node *, void *);
static int putsymbol_proc (Node *symnode, void *fparg);
static void RCS_copydeltas(RCSNode *rcs, FILE *fin, struct rcsbuffer *rcsbufin,
    FILE *fout, Deltatext *newdtext, char *insertpt, int compress_new_delta);
static int count_delta_actions (Node *, void *);
static void putdeltatext (FILE *fp, Deltatext *d, int compress);

static FILE *rcs_internal_lockfile (const char *rcsfile, size_t *lockId);
static void rcs_internal_unlockfile (FILE *fp, char *rcsfile, size_t lockId);
static char *rcs_lockfilename (const char *filename);
static int binary_delta(char **buf1, size_t buf1_len, char **buf2, size_t buf2_len, char **delta, size_t *length);
static int RCS_create_reverse(RCSNode *rcs);
static void RCS_putrtree (RCSNode *rcs, char *rev, FILE *fp, const char *basefilename);
static void putrdelta(RCSVers *vers, FILE *fp, const char *basefilename, const char *rcsfilename);
static int putrbranch_proc (Node *symnode, void *fparg);

/* The RCS file reading functions are called a lot, and they do some
   string comparisons.  This macro speeds things up a bit by skipping
   the function call when the first characters are different.  It
   evaluates its arguments multiple times.  */
#define STREQ(a, b) ((a)[0] == (b)[0] && strcmp ((a), (b)) == 0)

/*
 * We don't want to use isspace() from the C library because:
 *
 * 1. The definition of "whitespace" in RCS files includes ASCII
 *    backspace, but the C locale doesn't.
 * 2. isspace is an very expensive function call in some implementations
 *    due to the addition of wide character support.
 */
static const char spacetab[] = {
        0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0,	/* 0x00 - 0x0f */
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 0x10 - 0x1f */
        1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 0x20 - 0x2f */
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 0x30 - 0x3f */
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 0x40 - 0x4f */
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 0x50 - 0x5f */
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 0x60 - 0x8f */
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 0x70 - 0x7f */
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 0x80 - 0x8f */
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 0x90 - 0x9f */
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 0xa0 - 0xaf */
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 0xb0 - 0xbf */
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 0xc0 - 0xcf */
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 0xd0 - 0xdf */
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 0xe0 - 0xef */
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0  /* 0xf0 - 0xff */
};

#define whitespace(c)	(spacetab[(unsigned char)c] != 0)

static char *rcs_lockfile;
static int rcs_lockfd = -1;

static char *rcsbuf_strdup(struct rcsbuffer *rcs, const char *start, const char *end)
{
	char *p;
	if(end==start)
		return NULL;
	assert(end>start);
	p = (char*)xmalloc(end - start + 1);
	memcpy(p,start,end - start);
	p[end - start]='\0';
	if(!rcs->alloc_string_buffer || (rcs->alloc_string_ptr-rcs->alloc_string_buffer)>=rcs->alloc_string_buffer_len)
	{
		int offset = rcs->alloc_string_ptr-rcs->alloc_string_buffer;
		if(!rcs->alloc_string_buffer_len)
			rcs->alloc_string_buffer_len=128;
		else
			rcs->alloc_string_buffer_len*=2;
		rcs->alloc_string_buffer=xrealloc(rcs->alloc_string_buffer,sizeof(char*)*rcs->alloc_string_buffer_len);
		rcs->alloc_string_ptr=rcs->alloc_string_buffer+offset;
	}
	*(rcs->alloc_string_ptr++)=p;
	return p;
}

/* Open a random RCS file */
RCSNode *RCS_fopen(const char *filename)
{
	FILE *fp;
	RCSNode *rcs = NULL;

	fp = CVS_FOPEN(filename, "rb");
	if (fp)
	{
		rcs = RCS_parsercsfile_i (fp, filename);
		if (rcs != NULL)
			rcs->flags |= VALID;
	}
	else if (! existence_error (errno))
	{
		error (1, errno, "cannot open %s", filename);
	}
	return rcs;
}

/* A few generic thoughts on error handling, in particular the
   printing of unexpected characters that we find in the RCS file
   (that is, why we use '\x%x' rather than %c or some such).

   * Avoiding %c means we don't have to worry about what is printable
   and other such stuff.  In error handling, often better to keep it
   simple.

   * Hex rather than decimal or octal because character set standards
   tend to use hex.

   * Saying "character 0x%x" might make it sound like we are printing
   a file offset.  So we use '\x%x'.

   * Would be nice to print the offset within the file, but I can
   imagine various portability hassles (in particular, whether
   unsigned long is always big enough to hold file offsets).  */

/* Parse an rcsfile given a user file name and a repository.  If there is
   an error, we print an error message and return NULL.  If the file
   does not exist, we return NULL without printing anything (I'm not
   sure this allows the caller to do anything reasonable, but it is
   the current behavior).  */
RCSNode *
RCS_parse (file, repos)
    const char *file;
    const char *repos;
{
    RCSNode *rcs;
    FILE *fp;
    RCSNode *retval;
    char *rcsfile;

	if(!file)
		return NULL;

    rcsfile = xmalloc (strlen (repos) + strlen (file)
		       + sizeof (RCSEXT) + sizeof (CVSATTIC) + 10);

    sprintf (rcsfile, "%s/%s%s", repos, file, RCSEXT);

    if ((fp = CVS_FOPEN (rcsfile, FOPEN_BINARY_READ)) != NULL)
    {
        rcs = RCS_parsercsfile_i(fp, rcsfile);
		if (rcs != NULL)
			rcs->flags |= VALID;

		retval = rcs;
		goto out;
    }
    else if (! existence_error (errno))
    {
		error (0, errno, "cannot open %s", rcsfile);
		retval = NULL;
		goto out;
    }

    (void) sprintf (rcsfile, "%s/%s/%s%s", repos, CVSATTIC, file, RCSEXT);
    if ((fp = CVS_FOPEN (rcsfile, FOPEN_BINARY_READ)) != NULL)
    {
        rcs = RCS_parsercsfile_i(fp, rcsfile);
		if (rcs != NULL)
		{
			rcs->flags |= INATTIC;
			rcs->flags |= VALID;
		}

		retval = rcs;
		goto out;
    }
    else if (! existence_error (errno))
    {
		error (0, errno, "cannot open %s", rcsfile);
		retval = NULL;
		goto out;
    }

    retval = NULL;

 out:
    xfree (rcsfile);

    return retval;
}

/*
 * Parse a specific rcsfile.
 */
RCSNode *
RCS_parsercsfile (rcsfile)
    char *rcsfile;
{
    FILE *fp;
    RCSNode *rcs;

    /* open the rcsfile */
    if ((fp = CVS_FOPEN (rcsfile, FOPEN_BINARY_READ)) == NULL)
    {
		error (0, errno, "Couldn't open rcs file `%s'", rcsfile);
		return (NULL);
    }

    rcs = RCS_parsercsfile_i (fp, rcsfile);

    return (rcs);
}


/*
 */
static RCSNode *RCS_parsercsfile_i (FILE *fp, const char *rcsfile)
{
    RCSNode *rdata;
    char *key, *value;

    /* make a node */
    rdata = (RCSNode *) xmalloc (sizeof (RCSNode));
    memset ((char *) rdata, 0, sizeof (RCSNode));
    rdata->refcount = 1;
    rdata->path = xstrdup (rcsfile);

    /* Process HEAD, BRANCH, and EXPAND keywords from the RCS header.

       Most cvs operations on the main branch don't need any more
       information.  Those that do call RCS_reparsercsfile to parse
       the rest of the header and the deltas.  */
	rcsbuf_open(&rdata->rcsbuf, fp, rcsfile, 0);

    if (! rcsbuf_getkey (&rdata->rcsbuf, &key, &value))
	goto l_error;
    if (STREQ (key, RCSDESC))
	goto l_error;

    if (STREQ (RCSHEAD, key) && value != NULL)
	rdata->head = rcsbuf_valcopy (&rdata->rcsbuf, value, 0, (size_t *) NULL);

    if (! rcsbuf_getkey (&rdata->rcsbuf, &key, &value))
	goto l_error;
    if (STREQ (key, RCSDESC))
	goto l_error;

    if (STREQ (RCSBRANCH, key) && value != NULL)
    {
	char *cp;

	rdata->branch = rcsbuf_valcopy (&rdata->rcsbuf, value, 0, (size_t *) NULL);
	if ((numdots (rdata->branch) & 1) != 0)
	{
	    /* turn it into a branch if it's a revision */
	    cp = strrchr (rdata->branch, '.');
	    *cp = '\0';
	}
    }

    /* Look ahead for expand, stopping when we see desc or a revision
       number.  */
    while (1)
    {
	char *cp;

	if (STREQ (RCSEXPAND, key))
	{
	    rdata->expand = rcsbuf_valcopy (&rdata->rcsbuf, value, 0,
					    (size_t *) NULL);
	    break;
	}

	for (cp = key;
	     (isdigit ((unsigned char) *cp) || *cp == '.') && *cp != '\0';
	     cp++)
	    /* do nothing */ ;
	if (*cp == '\0')
	    break;

	if (STREQ (RCSDESC, key))
	    break;

	if (! rcsbuf_getkey (&rdata->rcsbuf, &key, &value))
	    break;
    }

    rdata->flags |= PARTIAL;

    return rdata;

l_error:
    error (0, 0, "`%s' does not appear to be a valid rcs file",
	   rcsfile);
    freercsnode (&rdata);
    return (NULL);
}


/* Do the real work of parsing an RCS file.

   On error, die with a fatal error; if it returns at all it was successful.

   If PFP is NULL, close the file when done.  Otherwise, leave it open
   and store the FILE * in *PFP.  */
void RCS_reparsercsfile (RCSNode *rdata)
{
    char *rcsfile;
    Node *q, *kv;
    RCSVers *vnode;
    int gotkey;
    char *cp;
    char *key, *value;

    if (!(rdata->flags & PARTIAL))
	{
		rcsbuf_seek(&rdata->rcsbuf,rdata->delta_pos);
		return; // Already fully parsed
	}

	assert (rdata != NULL);
    rcsfile = rdata->path;

	rcsbuf_seek (&rdata->rcsbuf, 0);

    /* make a node */
    /* This probably shouldn't be done until later: if a file has an
       empty revision tree (which is permissible), rdata->versions
       should be NULL. -twp */
    rdata->versions = getlist ();

    /*
     * process all the special header information, break out when we get to
     * the first revision delta
     */
    gotkey = 0;
    for (;;)
    {
	/* get the next key/value pair */
	if (!gotkey)
	{
	    if (! rcsbuf_getkey (&rdata->rcsbuf, &key, &value))
	    {
		error (1, 0, "`%s' does not appear to be a valid rcs file",
		       rcsfile);
	    }
	}

	gotkey = 0;

	/* Skip head, branch and expand tags; we already have them. */
	if (STREQ (key, RCSHEAD)
	    || STREQ (key, RCSBRANCH)
	    || STREQ (key, RCSEXPAND))
	{
	    continue;
	}

	if (STREQ (key, "access"))
	{
	    if (value != NULL)
	    {
		/* We pass the POLISH parameter as 1 because
                   RCS_addaccess expects nothing but spaces.  FIXME:
                   It would be easy and more efficient to change
                   RCS_addaccess.  */
		rdata->access = rcsbuf_valcopy (&rdata->rcsbuf, value, 1,
						(size_t *) NULL);
	    }
	    continue;
	}

	/* We always save lock information, so that we can handle
           -kkvl correctly when checking out a file. */
	if (STREQ (key, "locks"))
	{
	    if (value != NULL)
		rdata->locks_data = rcsbuf_valcopy (&rdata->rcsbuf, value, 0,
						    (size_t *) NULL);
	    if (! rcsbuf_getkey (&rdata->rcsbuf, &key, &value))
	    {
		error (1, 0, "premature end of file reading %s", rcsfile);
	    }
	    if (STREQ (key, "strict") && value == NULL)
	    {
		rdata->strict_locks = 1;
	    }
	    else
		gotkey = 1;
	    continue;
	}

	if (STREQ (RCSSYMBOLS, key))
	{
	    if (value != NULL)
		rdata->symbols_data = rcsbuf_valcopy (&rdata->rcsbuf, value, 0,
						      (size_t *) NULL);
	    continue;
	}

	/*
	 * check key for '.''s and digits (probably a rev) if it is a
	 * revision or `desc', we are done with the headers and are down to the
	 * revision deltas, so we break out of the loop
	 */
	for (cp = key;
	     (isdigit ((unsigned char) *cp) || *cp == '.') && *cp != '\0';
	     cp++)
	     /* do nothing */ ;
	/* Note that when comparing with RCSDATE, we are not massaging
           VALUE from the string found in the RCS file.  This is OK
           since we know exactly what to expect.  */
	if (*cp == '\0' && strncmp (RCSDATE, value, (sizeof RCSDATE) - 1) == 0)
	    break;

	if (STREQ (key, RCSDESC))
	    break;

	if (STREQ (key, "comment"))
	{
	    rdata->comment = rcsbuf_valcopy (&rdata->rcsbuf, value, 0,
					     (size_t *) NULL);
	    continue;
	}
	if (rdata->other == NULL)
	    rdata->other = getlist ();
	kv = getnode ();
	kv->type = rcsbuf_valcmp (&rdata->rcsbuf) ? RCSCMPFLD : RCSFIELD;
	kv->key = xstrdup (key);
	kv->data = rcsbuf_valcopy (&rdata->rcsbuf, value, kv->type == RCSFIELD,
				   (size_t *) NULL);
	if (addnode (rdata->other, kv) != 0)
	{
	    error (0, 0, "warning: duplicate key `%s' in RCS file `%s'",
		   key, rcsfile);
	    freenode (kv);
	}

	/* if we haven't grabbed it yet, we didn't want it */
    }

    /* We got out of the loop, so we have the first part of the first
       revision delta in KEY (the revision) and VALUE (the date key
       and its value).  This is what getdelta expects to receive.  */

    while ((vnode = getdelta (&rdata->rcsbuf, rcsfile, &key, &value)) != NULL)
    {
		/* get the node */
		q = getnode ();
		q->type = RCSVERS;
		q->delproc = rcsvers_delproc;
		q->data = (char *) vnode;
		q->key = vnode->version;

		/* add the nodes to the list */
		addnode (rdata->versions, q);
    }

    /* Here KEY and VALUE are whatever caused getdelta to return NULL.  */

    if (STREQ (key, RCSDESC))
    {
	if (rdata->desc != NULL)
	{
	    error (0, 0,
		   "warning: duplicate key `%s' in RCS file `%s'",
		   key, rcsfile);
	    xfree (rdata->desc);
	}
	rdata->desc = rcsbuf_valcopy (&rdata->rcsbuf, value, 1, (size_t *) NULL);
    }

    rdata->delta_pos = rcsbuf_ftell (&rdata->rcsbuf);

    rdata->flags &= ~PARTIAL;
}

/* Move RCS into or out of the Attic, depending on TOATTIC.  If the
   file is already in the desired place, return without doing
   anything.  At some point may want to think about how this relates
   to RCS_rewrite but that is a bit hairy (if one wants renames to be
   atomic, or that kind of thing).  If there is an error, print a message
   and return 1.  On success, return 0.  */
int RCS_setattic (RCSNode *rcs, int toattic)
{
    char *newpath;
    const char *p;
    char *q;

    /* Could make the pathname computations in this file, and probably
       in other parts of rcs.c too, easier if the REPOS and FILE
       arguments to RCS_parse got stashed in the RCSNode.  */

    if (toattic)
    {
		error(0,0,"Setting to attic is depreciated");
    }
    else
    {
		if (!(rcs->flags & INATTIC))
			return 0;

		newpath = xmalloc (strlen (rcs->path));

		/* Example: rcs->path is "/foo/bar/Attic/baz,v".  */
		p = last_component (rcs->path);
		strncpy (newpath, rcs->path, p - rcs->path - 1);
		newpath[p - rcs->path - 1] = '\0';
		q = newpath + (p - rcs->path - 1) - (sizeof CVSATTIC - 1);
		assert (strncmp (q, CVSATTIC, sizeof CVSATTIC - 1) == 0);
		strcpy (q, p);

		rcsbuf_close (&rcs->rcsbuf);
		if (CVS_RENAME (rcs->path, newpath) < 0)
		{
			error (0, errno, "failed to move `%s' out of the attic",
			fn_root(rcs->path));
			xfree (newpath);
			return 1;
		}
    }

    xfree (rcs->path);
    rcs->path = newpath;
	xfree (rcs->rcsbuf.filename);
	rcs->rcsbuf.filename = xstrdup(newpath);

    return 0;
}

/*
 * Fully parse the RCS file.  Store all keyword/value pairs, fetch the
 * log messages for each revision, and fetch add and delete counts for
 * each revision (we could fetch the entire text for each revision,
 * but the only caller, log_fileproc, doesn't need that information,
 * so we don't waste the memory required to store it).  The add and
 * delete counts are stored on the OTHER field of the RCSVERSNODE
 * structure, under the names ";add" and ";delete", so that we don't
 * waste the memory space of extra fields in RCSVERSNODE for code
 * which doesn't need this information.
 */

void RCS_fully_parse (RCSNode *rcs)
{
    RCS_reparsercsfile (rcs);

    while (1)
    {
	char *key, *value;
	Node *vers;
	RCSVers *vnode;

	/* Rather than try to keep track of how much information we
           have read, just read to the end of the file.  */
	if (! rcsbuf_getrevnum (&rcs->rcsbuf, &key))
	    break;

	vers = findnode (rcs->versions, key);
	if (vers == NULL)
	    error (1, 0,
		   "mismatch in rcs file %s between deltas and deltatexts",
		   fn_root(rcs->path));

	vnode = (RCSVers *) vers->data;

	while (rcsbuf_getkey (&rcs->rcsbuf, &key, &value))
	{
	    if (! STREQ (key, "text"))
	    {
		Node *kv;

		if (vnode->other == NULL)
		    vnode->other = getlist ();
		kv = getnode ();
		kv->type = rcsbuf_valcmp (&rcs->rcsbuf) ? RCSCMPFLD : RCSFIELD;
		kv->key = xstrdup (key);
		kv->data = rcsbuf_valcopy (&rcs->rcsbuf, value, kv->type == RCSFIELD,
					   (size_t *) NULL);
		if (addnode (vnode->other, kv) != 0)
		{
		    error (0, 0,
			   "\
warning: duplicate key `%s' in version `%s' of RCS file `%s'",
			   key, vnode->version, fn_root(rcs->path));
		    freenode (kv);
		}

		continue;
	    }

	    if (! STREQ (vnode->version, rcs->head))
	    {
			if(vnode->type && (STREQ(vnode->type,"binary") || STREQ(vnode->type,"compressed_binary")))
			{
			}
			else
			{
				unsigned long add=0, del=0;
				char buf[50];
				Node *kv;
				/* This is a change text.  Store the add and delete
						counts.  */
				add = 0;
				del = 0;
				if (value != NULL)
				{
					size_t vallen;
					const char *cp;
					char *zbuf = NULL;

					rcsbuf_valpolish (&rcs->rcsbuf, value, 0, &vallen);

					if(vnode->type && STREQ(vnode->type,"compressed_text"))
					{
						uLong zlen;

						z_stream stream = {0};
						inflateInit(&stream);
						zlen = ntohl(*(unsigned long *)value);
						if(zlen)
						{
							stream.avail_in = vallen-4;
							stream.next_in = value+4;
							stream.avail_out = zlen;
							stream.next_out = zbuf = xmalloc(zlen);
							if(inflate(&stream, Z_FINISH)!=Z_STREAM_END)
							{
								error(1,0,"internal error: inflate failed");
							}
						}
						vallen=zlen;
						value = zbuf;
					}
					cp = value;
					while (cp < value + vallen)
					{
					char op;
					unsigned long count;

					op = *cp++;
					if (op != 'a' && op  != 'd')
						error (1, 0, "\
		unrecognized operation '\\x%x' in %s",
						op, fn_root(rcs->path));
					(void) strtoul (cp, (char **) &cp, 10);
					if (*cp++ != ' ')
						error (1, 0, "space expected in %s",
						fn_root(rcs->path));
					count = strtoul (cp, (char **) &cp, 10);
					if (*cp++ != '\012')
						error (1, 0, "linefeed expected in %s (got %d)",
						fn_root(rcs->path),(int)*(cp-1));

					if (op == 'd')
						del += count;
					else
					{
						add += count;
						while (count != 0)
						{
						if (*cp == '\012')
							--count;
						else if (cp == value + vallen)
						{
							if (count != 1)
							error (1, 0, "\
		invalid rcs file %s: premature end of value",
								fn_root(rcs->path));
							else
							break;
						}
						++cp;
						}
					}
					}
				xfree(zbuf);
				}
			sprintf (buf, "%lu", add);
			kv = getnode ();
			kv->type = RCSFIELD;
			kv->key = xstrdup (";add");
			kv->data = xstrdup (buf);
			if (addnode (vnode->other, kv) != 0)
			{
				error (0, 0,
				"\
	warning: duplicate key `%s' in version `%s' of RCS file `%s'",
				key, vnode->version, fn_root(rcs->path));
				freenode (kv);
			}

			sprintf (buf, "%lu", del);
			kv = getnode ();
			kv->type = RCSFIELD;
			kv->key = xstrdup (";delete");
			kv->data = xstrdup (buf);
			if (addnode (vnode->other, kv) != 0)
			{
				error (0, 0,
				"\
	warning: duplicate key `%s' in version `%s' of RCS file `%s'",
				key, vnode->version, fn_root(rcs->path));
				freenode (kv);
			}
			}

	    }

	    /* We have found the "text" key which ends the data for
               this revision.  Break out of the loop and go on to the
               next revision.  */
	    break;
	}
    }
}

/*
 * freercsnode - free up the info for an RCSNode
 */
void
freercsnode (rnodep)
    RCSNode **rnodep;
{
    if (rnodep == NULL || *rnodep == NULL)
	return;

    ((*rnodep)->refcount)--;
    if ((*rnodep)->refcount != 0)
    {
	*rnodep = (RCSNode *) NULL;
	return;
    }
    xfree ((*rnodep)->path);
	xfree ((*rnodep)->head);
	xfree ((*rnodep)->branch);

	if((*rnodep)->rcsbuf.lockId)
		do_unlock_file((*rnodep)->rcsbuf.lockId);

	rcsbuf_close(&(*rnodep)->rcsbuf);
	xfree((*rnodep)->rcsbuf.filename);
    free_rcsnode_contents (*rnodep);
    xfree (*rnodep);
    *rnodep = (RCSNode *) NULL;
}

/*
 * free_rcsnode_contents - free up the contents of an RCSNode without
 * freeing the node itself, or the file name, or the head, or the
 * path.  This returns the RCSNode to the state it is in immediately
 * after a call to RCS_parse.
 */
static void
free_rcsnode_contents (rnode)
    RCSNode *rnode;
{
    dellist (&rnode->versions);
    if (rnode->symbols != (List *) NULL)
		dellist (&rnode->symbols);
	xfree (rnode->symbols_data);
	xfree (rnode->expand);
	dellist (&rnode->other);
	xfree (rnode->access);
	xfree (rnode->locks_data);
    if (rnode->locks != (List *) NULL)
		dellist (&rnode->locks);
	xfree (rnode->comment);
	xfree (rnode->desc);
	xfree (rnode->lock_version);
}

/* free_rcsvers_contents -- free up the contents of an RCSVers node,
   but also free the pointer to the node itself. */
/* Note: The `hardlinks' list is *not* freed, since it is merely a
   pointer into the `hardlist' structure (defined in hardlink.c), and
   that structure is freed elsewhere in the program. */

static void
free_rcsvers_contents (RCSVers *rnode)
{
    if (rnode->branches != (List *) NULL)
	dellist (&rnode->branches);
	xfree (rnode->date);
	xfree (rnode->next);
	xfree (rnode->author);
	xfree (rnode->state);
	xfree (rnode->type);
    if (rnode->other != (List *) NULL)
	dellist (&rnode->other);
    if (rnode->other_delta != NULL)
	dellist (&rnode->other_delta);
    if (rnode->text != NULL)
	freedeltatext (rnode->text);
    xfree (rnode);
}

/*
 * rcsvers_delproc - free up an RCSVers type node
 */
static void rcsvers_delproc (Node *p)
{
    free_rcsvers_contents ((RCSVers *) p->data);
}

/* These functions retrieve keys and values from an RCS file using a
   buffer.  We use this somewhat complex approach because it turns out
   that for many common operations, CVS spends most of its time
   reading keys, so it's worth doing some fairly hairy optimization.  */

/* The number of bytes we try to read each time we need more data.  */

#define RCSBUF_BUFSIZE (BUFSIZ*10)

/* Set up to start gathering keys and values from an RCS file.  This
   initializes RCSBUF.  */

static void rcsbuf_open(struct rcsbuffer *rcsbuf, FILE *fp, const char *filename, off_t pos)
{
	if(!filename)
		filename = rcsbuf->filename;
	if(!fp)
	{
		fp = CVS_FOPEN(filename, FOPEN_BINARY_READ);
		if(!fp)
			error(1,errno,"Couldn't open %s",filename);
	}
	assert(filename);
	if(!rcsbuf->lockId)
		rcsbuf->lockId=do_lock_file(filename, NULL, 0);

	rcsbuf->buffer_size = 0;
	rcsbuf->buffer = NULL;
    rcsbuf->ptr = rcsbuf->buffer;
    rcsbuf->ptrend = rcsbuf->buffer;

	if(!rcsbuf->filename)
		rcsbuf->filename = xstrdup(filename);
    rcsbuf->fp = fp;
    rcsbuf->pos = pos;
    rcsbuf->vlen = 0;
    rcsbuf->at_string = 0;
    rcsbuf->embedded_at = 0;

	if(fp)
		CVS_FSEEK(fp, pos, SEEK_SET);
}

/* Stop gathering keys from an RCS file.  */

void rcsbuf_close (struct rcsbuffer *rcsbuf)
{
	xfree(rcsbuf->buffer);
	if(rcsbuf->fp)
		fclose(rcsbuf->fp);
	rcsbuf->fp=NULL;
	rcsbuf->buffer=NULL;
	rcsbuf->ptr=NULL;
	rcsbuf->ptrend=NULL;
	rcsbuf->vlen=0;
	rcsbuf->at_string=0;
	rcsbuf->embedded_at=0;
	if(rcsbuf->alloc_string_buffer)
	{
		char **p = rcsbuf->alloc_string_buffer;
		while(p<rcsbuf->alloc_string_ptr)
			xfree(*(p++));
		xfree(rcsbuf->alloc_string_buffer);
		rcsbuf->alloc_string_ptr=NULL;
		rcsbuf->alloc_string_buffer_len=0;
	}
}

/* Read a key/value pair from an RCS file.  This sets *KEYP to point
   to the key, and *VALUEP to point to the value.  A missing or empty
   value is indicated by setting *VALUEP to NULL.

   This function returns 1 on success, or 0 on EOF.  If there is an
   error reading the file, or an EOF in an unexpected location, it
   gives a fatal error.

   This sets *KEYP and *VALUEP to point to storage managed by
   rcsbuf_getkey.  Moreover, *VALUEP has not been massaged from the
   RCS format: it may contain embedded whitespace and embedded '@'
   characters.  Call rcsbuf_valcopy or rcsbuf_valpolish to do
   appropriate massaging.  */

/* Note that the extreme hair in rcsbuf_getkey is because profiling
   statistics show that it was worth it. */

static int
rcsbuf_getkey (rcsbuf, keyp, valp)
    struct rcsbuffer *rcsbuf;
    char **keyp;
    char **valp;
{
    const char * const my_spacetab = spacetab;
    char *ptr, *ptrend, *keystart;
    char c;
	char *pat;
	size_t vlen;

#define my_whitespace(c)	(my_spacetab[(unsigned char)c] != 0)

    rcsbuf->vlen = 0;
    rcsbuf->at_string = 0;
    rcsbuf->embedded_at = 0;

    ptr = rcsbuf->ptr;
    ptrend = rcsbuf->ptrend;

    /* Sanity check.  */
    if (ptr < rcsbuf->buffer || ptr > rcsbuf->buffer + rcsbuf->buffer_size)
	abort ();

    /* If the pointer is more than RCSBUF_BUFSIZE bytes into the
       buffer, move back to the start of the buffer.  This keeps the
       buffer from growing indefinitely.  */
    if (ptr - rcsbuf->buffer >= RCSBUF_BUFSIZE)
    {
	int len;

	len = ptrend - ptr;

	/* Sanity check: we don't read more than RCSBUF_BUFSIZE bytes
           at a time, so we can't have more bytes than that past PTR.  */
	if (len > RCSBUF_BUFSIZE)
	    abort ();

	/* Update the POS field, which holds the file offset of the
           first byte in the rcsbuf->buffer buffer.  */
	rcsbuf->pos += ptr - rcsbuf->buffer;

	memcpy (rcsbuf->buffer, ptr, len);
	ptr = rcsbuf->buffer;
	ptrend = ptr + len;
	rcsbuf->ptrend = ptrend;
    }

    /* Skip leading whitespace.  */

    while (1)
    {
		if (ptr >= ptrend)
		{
			ptr = rcsbuf_fill (rcsbuf, ptr, (char **) NULL, (char **) NULL);
			if (ptr == NULL)
			return 0;
			ptrend = rcsbuf->ptrend;
		}

		c = *ptr;
		if (! my_whitespace (c))
			break;

		++ptr;
    }

    /* We've found the start of the key.  */

    keystart = ptr;

    if (c != ';')
    {
	while (1)
	{
	    ++ptr;
	    if (ptr >= ptrend)
	    {
			ptr = rcsbuf_fill (rcsbuf, ptr, &keystart, (char **) NULL);
			if (ptr == NULL)
				error (1, 0, "EOF in key in RCS file %s",
				rcsbuf->filename);
			ptrend = rcsbuf->ptrend;
	    }
	    c = *ptr;
	    if (c == ';' || my_whitespace (c))
		break;
	}
    }

    /* Here *KEYP points to the key in the buffer, C is the character
       we found at the of the key, and PTR points to the location in
       the buffer where we found C.  We must set *PTR to \0 in order
       to terminate the key.  If the key ended with ';', then there is
       no value.  */

    *keyp=rcsbuf_strdup(rcsbuf,keystart,ptr);
    ++ptr;

    if (c == ';')
    {
	*valp = NULL;
	rcsbuf->ptr = ptr;
	return 1;
    }

    /* C must be whitespace.  Skip whitespace between the key and the
       value.  If we find ';' now, there is no value.  */

    while (1)
    {
	if (ptr >= ptrend)
	{
	    ptr = rcsbuf_fill (rcsbuf, ptr, NULL, (char **) NULL);
	    if (ptr == NULL)
		error (1, 0, "EOF while looking for value in RCS file %s",
		       rcsbuf->filename);
	    ptrend = rcsbuf->ptrend;
	}
	c = *ptr;
	if (c == ';')
	{
	    *valp = NULL;
	    rcsbuf->ptr = ptr + 1;
	    return 1;
	}
	if (! my_whitespace (c))
	    break;
	++ptr;
    }

    /* Now PTR points to the start of the value, and C is the first
       character of the value.  */

    if (c != '@')
		keystart = ptr;
    else
    {

	/* Optimize the common case of a value composed of a single
	   '@' string.  */

	rcsbuf->at_string = 1;

	++ptr;

	keystart = ptr;

	while (1)
	{
	    while ((pat = memchr (ptr, '@', ptrend - ptr)) == NULL)
	    {
		/* Note that we pass PTREND as the PTR value to
                   rcsbuf_fill, so that we will wind up setting PTR to
                   the location corresponding to the old PTREND, so
                   that we don't search the same bytes again.  */
		ptr = rcsbuf_fill (rcsbuf, ptrend, &keystart, NULL);
		if (ptr == NULL)
		    error (1, 0,
			   "EOF while looking for end of string in RCS file %s",
			   rcsbuf->filename);
		ptrend = rcsbuf->ptrend;
	    }

	    /* Handle the special case of an '@' right at the end of
               the known bytes.  */
	    if (pat + 1 >= ptrend)
	    {
			/* Note that we pass PAT, not PTR, here.  */
			pat = rcsbuf_fill (rcsbuf, pat, &keystart, NULL);
			if (pat == NULL)
			{
				/* EOF here is OK; it just means that the last
				character of the file was an '@' terminating a
				value for a key type which does not require a
				trailing ';'.  */
				pat = rcsbuf->ptrend - 1;

			}
			ptrend = rcsbuf->ptrend;

			/* Note that the value of PTR is bogus here.  This is
			OK, because we don't use it.  */
	    }

	    if (pat + 1 >= ptrend || pat[1] != '@')
		break;

	    /* We found an '@' pair in the string.  Keep looking.  */
	    ++rcsbuf->embedded_at;
	    ptr = pat + 2;
	}

	/* Here PAT points to the final '@' in the string.  */

	vlen = pat - keystart;

	if (vlen == 0)
	    *valp = NULL;
	else
		*valp = rcsbuf_strdup(rcsbuf,keystart,pat);
	rcsbuf->vlen = vlen;

	ptr = pat + 1;
    }

    /* Certain keywords only have a '@' string.  If there is no '@'
       string, then the old getrcskey function assumed that they had
       no value, and we do the same.  */

    {
	char *k;

	k = *keyp;
	if (STREQ (k, RCSDESC)
	    || STREQ (k, "text")
	    || STREQ (k, "log"))
	{
	    if (c != '@')
		*valp = NULL;
	    rcsbuf->ptr = ptr;
	    return 1;
	}
    }

    /* If we've already gathered a '@' string, try to skip whitespace
       and find a ';'.  */
    if (c == '@')
    {
	while (1)
	{
	    char n;

	    if (ptr >= ptrend)
	    {
			ptr = rcsbuf_fill (rcsbuf, ptr, &keystart, NULL);
			if (ptr == NULL)
				error (1, 0, "EOF in value in RCS file %s",
				rcsbuf->filename);
			ptrend = rcsbuf->ptrend;
	    }
	    n = *ptr;
	    if (n == ';')
	    {
		/* We're done.  We already set everything up for this
                   case above.  */
		rcsbuf->ptr = ptr + 1;
		return 1;
	    }
	    if (! my_whitespace (n))
			break;
	    ++ptr;
	}

	/* The value extends past the '@' string.  We need to undo the
           '@' stripping done in the default case above.  This
           case never happens in a plain RCS file, but it can happen
           if user defined phrases are used.  */
//	((*valp)--)[rcsbuf->vlen++] = '@';
		error(1,0,"Unhandled keyword overrun");
    }

    /* Here we have a value which is not a simple '@' string.  We need
       to gather up everything until the next ';', including any '@'
       strings.  *VALP points to the start of the value.  If
       RCSBUF->VLEN is not zero, then we have already read an '@'
       string, and PTR points to the data following the '@' string.
       Otherwise, PTR points to the start of the value.  */

    while (1)
    {
	char *start, *psemi, *pat;

	/* Find the ';' which must end the value.  */
	start = ptr;
	while ((psemi = memchr (ptr, ';', ptrend - ptr)) == NULL)
	{
	    /* Note that we pass PTREND as the PTR value to
	       rcsbuf_fill, so that we will wind up setting PTR to the
	       location corresponding to the old PTREND, so that we
	       don't search the same bytes again.  */
	    ptr = rcsbuf_fill (rcsbuf, ptrend, &keystart, &start);
	    if (ptr == NULL)
			error (1, 0, "EOF in value in RCS file %s", rcsbuf->filename);
	    ptrend = rcsbuf->ptrend;
	}

	/* See if there are any '@' strings in the value.  */
	pat = memchr (start, '@', psemi - start);

	if (pat == NULL)
	{
	    size_t vlen;

	    /* We're done with the value.  Trim any trailing
               whitespace.  */

	    rcsbuf->ptr = psemi + 1;

	    start = keystart;
	    while (psemi > start && my_whitespace (psemi[-1]))
		--psemi;

	    vlen = psemi - start;
	    if (vlen == 0)
			*valp = NULL;
		else
			*valp = rcsbuf_strdup(rcsbuf,start,psemi);
	    rcsbuf->vlen = vlen;

	    return 1;
	}

	/* We found an '@' string in the value.  We set RCSBUF->AT_STRING
	   and RCSBUF->EMBEDDED_AT to indicate that we won't be able to
	   compress whitespace correctly for this type of value.
	   Since this type of value never arises in a normal RCS file,
	   this should not be a big deal.  It means that if anybody
	   adds a phrase which can have both an '@' string and regular
	   text, they will have to handle whitespace compression
	   themselves.  */

	rcsbuf->at_string = 1;
	rcsbuf->embedded_at = -1;

	ptr = pat + 1;

	while (1)
	{
	    while ((pat = memchr (ptr, '@', ptrend - ptr)) == NULL)
	    {
			/* Note that we pass PTREND as the PTR value to
					rcsbuff_fill, so that we will wind up setting PTR
					to the location corresponding to the old PTREND, so
					that we don't search the same bytes again.  */
			ptr = rcsbuf_fill (rcsbuf, ptrend, NULL, NULL);
			if (ptr == NULL)
				error (1, 0,
				"EOF while looking for end of string in RCS file %s",
				rcsbuf->filename);
			ptrend = rcsbuf->ptrend;
	    }

	    /* Handle the special case of an '@' right at the end of
               the known bytes.  */
	    if (pat + 1 >= ptrend)
	    {
			ptr = rcsbuf_fill (rcsbuf, ptr, NULL, NULL);
			if (ptr == NULL)
				error (1, 0, "EOF in value in RCS file %s",
				rcsbuf->filename);
			ptrend = rcsbuf->ptrend;
	    }

	    if (pat[1] != '@')
		break;

	    /* We found an '@' pair in the string.  Keep looking.  */
	    ptr = pat + 2;
	}

	/* Here PAT points to the final '@' in the string.  */
	ptr = pat + 1;
    }

#undef my_whitespace
}

/* Read an RCS revision number from an RCS file.  This sets *REVP to
   point to the revision number; it will point to space that is
   managed by the rcsbuf functions, and is only good until the next
   call to rcsbuf_getkey or rcsbuf_getrevnum.

   This function returns 1 on success, or 0 on EOF.  If there is an
   error reading the file, or an EOF in an unexpected location, it
   gives a fatal error.  */

static int
rcsbuf_getrevnum (rcsbuf, revp)
    struct rcsbuffer *rcsbuf;
    char **revp;
{
    char *ptr, *ptrend, *revstart;
    char c;

    ptr = rcsbuf->ptr;
    ptrend = rcsbuf->ptrend;

    *revp = NULL;

    /* Skip leading whitespace.  */

    while (1)
    {
	if (ptr >= ptrend)
	{
	    ptr = rcsbuf_fill (rcsbuf, ptr, (char **) NULL, (char **) NULL);
	    if (ptr == NULL)
			return 0;
	    ptrend = rcsbuf->ptrend;
	}

	c = *ptr;
	if (! whitespace (c))
	    break;

	++ptr;
    }

    if (! isdigit ((unsigned char) c) && c != '.')
	{
	error (1, 0,
	       "unexpected '\\x%x' reading revision number in RCS file %s",
	       c, rcsbuf->filename);
	}

    revstart = ptr;

    do
    {
	++ptr;
	if (ptr >= ptrend)
	{
	    ptr = rcsbuf_fill (rcsbuf, ptr, &revstart, NULL);
	    if (ptr == NULL)
		error (1, 0,
		       "unexpected EOF reading revision number in RCS file %s",
		       rcsbuf->filename);
	    ptrend = rcsbuf->ptrend;
	}

	c = *ptr;
    }
    while (isdigit ((unsigned char) c) || c == '.');

    if (! whitespace (c))
	error (1, 0, "\
unexpected '\\x%x' reading revision number in RCS file %s",
	       c, rcsbuf->filename);

	*revp=rcsbuf_strdup(rcsbuf,revstart,ptr);

    rcsbuf->ptr = ptr + 1;

    return 1;
}

/* Fill rcsbuf->buffer with bytes from the file associated with RCSBUF,
   updating PTR and the PTREND field.  If KEYP and *KEYP are not NULL,
   then *KEYP points into the buffer, and must be adjusted if the
   buffer is changed.  Likewise for VALP.  Returns the new value of
   PTR, or NULL on error.  */

static char *rcsbuf_fill (struct rcsbuffer *rcsbuf, char *ptr, char **ptr1, char **ptr2)
{
	int got;

	if(!rcsbuf->fp)
		rcsbuf_open(rcsbuf,NULL,NULL,rcsbuf->pos);

	do
	{
		if (rcsbuf->ptrend - rcsbuf->buffer + RCSBUF_BUFSIZE > rcsbuf->buffer_size)
		{
			int poff, peoff, koff, voff, ptroff;

			poff = ptr - rcsbuf->buffer;
			peoff = rcsbuf->ptrend - rcsbuf->buffer;
			ptroff = rcsbuf->ptr - rcsbuf->buffer;
			if (ptr1 != NULL && *ptr1 != NULL)
				koff = *ptr1 - rcsbuf->buffer;
			if (ptr2 != NULL && *ptr2 != NULL)
				voff = *ptr2 - rcsbuf->buffer;

			expand_string (&rcsbuf->buffer, &rcsbuf->buffer_size,
					rcsbuf->buffer_size + RCSBUF_BUFSIZE);

			ptr = rcsbuf->buffer + poff;
			rcsbuf->ptrend = rcsbuf->buffer + peoff;
			rcsbuf->ptr = rcsbuf->buffer + ptroff;
			if (ptr1 != NULL && *ptr1 != NULL)
				*ptr1 = rcsbuf->buffer + koff;
			if (ptr2 != NULL && *ptr2 != NULL)
				*ptr2 = rcsbuf->buffer + voff;
		}

		got = fread (rcsbuf->ptrend, 1, RCSBUF_BUFSIZE, rcsbuf->fp);
		if (got == 0)
		{
			if (ferror (rcsbuf->fp))
				error (1, errno, "cannot read %s", rcsbuf->filename);
			return NULL;
		}
	    rcsbuf->ptrend += got;
	} while(ptr >= rcsbuf->ptrend);

    return ptr;
}

/* Test whether the last value returned by rcsbuf_getkey is a composite
   value or not. */

static int
rcsbuf_valcmp (rcsbuf)
    struct rcsbuffer *rcsbuf;
{
    return rcsbuf->at_string && rcsbuf->embedded_at < 0;
}

/* Copy the value VAL returned by rcsbuf_getkey into a memory buffer,
   returning the memory buffer.  Polish the value like
   rcsbuf_valpolish, q.v.  */

static char *
rcsbuf_valcopy (rcsbuf, val, polish, lenp)
    struct rcsbuffer *rcsbuf;
    char *val;
    int polish;
    size_t *lenp;
{
    size_t vlen;
    int embedded_at;
    char *ret;

    if (val == NULL)
    {
	if (lenp != NULL)
	    *lenp = 0;
	return NULL;
    }

    vlen = rcsbuf->vlen;
    embedded_at = rcsbuf->embedded_at < 0 ? 0 : rcsbuf->embedded_at;

    ret = xmalloc (vlen - embedded_at + 1);

    if (rcsbuf->at_string ? embedded_at == 0 : ! polish)
    {
	/* No special action to take.  */
	memcpy (ret, val, vlen + 1);
	if (lenp != NULL)
	    *lenp = vlen;
	return ret;
    }

    rcsbuf_valpolish_internal (rcsbuf, ret, val, lenp);
    return ret;
}

/* Polish the value VAL returned by rcsbuf_getkey.  The POLISH
   parameter is non-zero if multiple embedded whitespace characters
   should be compressed into a single whitespace character.  Note that
   leading and trailing whitespace was already removed by
   rcsbuf_getkey.  Within an '@' string, pairs of '@' characters are
   compressed into a single '@' character regardless of the value of
   POLISH.  If LENP is not NULL, set *LENP to the length of the value.  */

static void
rcsbuf_valpolish (rcsbuf, val, polish, lenp)
    struct rcsbuffer *rcsbuf;
    char *val;
    int polish;
    size_t *lenp;
{
    if (val == NULL)
    {
	if (lenp != NULL)
	    *lenp= 0;
	return;
    }

    if (rcsbuf->at_string ? rcsbuf->embedded_at == 0 : ! polish)
    {
	/* No special action to take.  */
	if (lenp != NULL)
	    *lenp = rcsbuf->vlen;
	return;
    }

    rcsbuf_valpolish_internal (rcsbuf, val, val, lenp);
}

/* Internal polishing routine, called from rcsbuf_valcopy and
   rcsbuf_valpolish.  */

static void
rcsbuf_valpolish_internal (rcsbuf, to, from, lenp)
    struct rcsbuffer *rcsbuf;
    char *to;
    const char *from;
    size_t *lenp;
{
    size_t len;

    len = rcsbuf->vlen;

    if (! rcsbuf->at_string)
    {
	char *orig_to;
	size_t clen;

	orig_to = to;

	for (clen = len; clen > 0; ++from, --clen)
	{
	    char c;

	    c = *from;
	    if (whitespace (c))
	    {
		/* Note that we know that clen can not drop to zero
                   while we have whitespace, because we know there is
                   no trailing whitespace.  */
		while (whitespace (from[1]))
		{
		    ++from;
		    --clen;
		}
		c = ' ';
	    }
	    *to++ = c;
	}

	*to = '\0';

	if (lenp != NULL)
	    *lenp = to - orig_to;
    }
    else
    {
	const char *orig_from;
	char *orig_to;
	int embedded_at;
	size_t clen;

	orig_from = from;
	orig_to = to;

	embedded_at = rcsbuf->embedded_at;
	assert (embedded_at > 0);

	if (lenp != NULL)
	    *lenp = len - embedded_at;

	for (clen = len; clen > 0; ++from, --clen)
	{
	    char c;

	    c = *from;
	    *to++ = c;
	    if (c == '@')
	    {
		++from;

		/* Sanity check.  */
		if (*from != '@' || clen == 0)
		    abort ();

		--clen;

		--embedded_at;
		if (embedded_at == 0)
		{
		    /* We've found all the embedded '@' characters.
                       We can just memcpy the rest of the buffer after
                       this '@' character.  */
		    if (orig_to != orig_from)
			memcpy (to, from + 1, clen - 1);
		    else
			memmove (to, from + 1, clen - 1);
		    from += clen;
		    to += clen - 1;
		    break;
		}
	    }
	}

	/* Sanity check.  */
	if (from != orig_from + len
	    || to != orig_to + (len - rcsbuf->embedded_at))
	{
	    abort ();
	}

	*to = '\0';
    }
}

/* Copy the next word from the value VALP returned by rcsbuf_getkey into a
   memory buffer, updating VALP and returning the memory buffer.  Return
   NULL when there are no more words. */

static char *
rcsbuf_valword (rcsbuf, valp)
    struct rcsbuffer *rcsbuf;
    char **valp;
{
    register const char * const my_spacetab = spacetab;
    register char *ptr, *pat;
    char c;

#define my_whitespace(c)	(my_spacetab[(unsigned char)c] != 0)

    if (*valp == NULL)
	return NULL;

    for (ptr = *valp; my_whitespace (*ptr); ++ptr) ;
    if (*ptr == '\0')
    {
	assert (ptr - *valp == rcsbuf->vlen);
	*valp = NULL;
	rcsbuf->vlen = 0;
	return NULL;
    }

    /* PTR now points to the start of a value.  Find out whether it is
       a num, an id, a string or a colon. */
    c = *ptr;
    if (c == ':')
    {
	rcsbuf->vlen -= ++ptr - *valp;
	*valp = ptr;
	return xstrdup (":");
    }

    if (c == '@')
    {
	int embedded_at = 0;
	size_t vlen;

	pat = ++ptr;
	while ((pat = strchr (pat, '@')) != NULL)
	{
	    if (pat[1] != '@')
		break;
	    ++embedded_at;
	    pat += 2;
	}

	/* Here PAT points to the final '@' in the string.  */
	*pat++ = '\0';
	assert (rcsbuf->at_string);
	vlen = rcsbuf->vlen - (pat - *valp);
	rcsbuf->vlen = pat - ptr - 1;
	rcsbuf->embedded_at = embedded_at;
	ptr = rcsbuf_valcopy (rcsbuf, ptr, 0, (size_t *) NULL);
	*valp = pat;
	rcsbuf->vlen = vlen;
	if (strchr (pat, '@') == NULL)
	    rcsbuf->at_string = 0;
	else
	    rcsbuf->embedded_at = -1;
	return ptr;
    }

    /* *PTR is neither `:', `;' nor `@', so it should be the start of a num
       or an id.  Make sure it is not another special character. */
    if (c == '$' || c == '.' || c == ',')
    {
	error (1, 0, "illegal special character in RCS field in %s",
	       rcsbuf->filename);
    }

    pat = ptr;
    while (1)
    {
	/* Legitimate ID characters are digits, dots and any `graphic
           printing character that is not a special.' This test ought
	   to do the trick. */
	c = *++pat;
	if (!isprint ((unsigned char) c) ||
	    c == ';' || c == '$' || c == ',' || c == '@' || c == ':')
	    break;
    }

    /* PAT points to the last non-id character in this word, and C is
       the character in its memory cell.  Check to make sure that it
       is a legitimate word delimiter -- whitespace or end. */
    if (c != '\0' && !my_whitespace (c))
	error (1, 0, "illegal special character in RCS field in %s",
	       rcsbuf->filename);

    *pat = '\0';
    rcsbuf->vlen -= pat - *valp;
    *valp = pat;
    return xstrdup (ptr);

#undef my_whitespace
}

/* Return the current position of an rcsbuf.  */

static off_t rcsbuf_ftell (struct rcsbuffer *rcsbuf)
{
    return rcsbuf->pos + (rcsbuf->ptr - rcsbuf->buffer);
}

/* Return a pointer to any data buffered for RCSBUF, along with the
   length.  */

static void
rcsbuf_get_buffered (rcsbuf, datap, lenp)
    struct rcsbuffer *rcsbuf;
    char **datap;
    size_t *lenp;
{
    *datap = rcsbuf->ptr;
    *lenp = rcsbuf->ptrend - rcsbuf->ptr;
}


/*
 * process the symbols list of the rcs file
 */
static void
do_symbols (list, val)
    List *list;
    char *val;
{
    Node *p;
    char *cp = val, *cp2;
    char *tag, *rev,*eol;
	int in_string;

    for (;;)
    {
	/* skip leading whitespace */
	while (whitespace (*cp))
	    cp++;

	/* if we got to the end, we are done */
	if (*cp == '\0')
	    break;

	/* split it up into tag and rev */
	tag = cp;
	eol=strchr(cp,'\n');
	cp = strchr (cp, ':');
	*cp++ = '\0';
	rev = cp;
	cp2 = strchr (cp, ':');
	if(cp2 && cp2<eol)
	{
		/* TagDate */
		cp=cp2;
		*cp++ = '\0';
		cp2 = strchr (cp, ':');
		if(cp2)
		{
			/* TagComment */
			cp=cp2;
			*cp++ = '\0';
		}
	}
	in_string=0;
	while ((in_string || !whitespace (*cp)) && *cp != '\0')
	{
		if(*cp=='@')
			in_string=!in_string;
		cp++;
	}
	if (*cp != '\0')
		*cp++ = '\0';

	/* make a new node and add it to the list */
	p = getnode ();
	p->key = xstrdup (tag);
	p->data = xstrdup (rev);
	(void) addnode (list, p);
    }
}

/*
 * process the locks list of the rcs file
 * Like do_symbols, but hash entries are keyed backwards: i.e.
 * an entry like `user:rev' is keyed on REV rather than on USER.
 */
static void
do_locks (list, val)
    List *list;
    char *val;
{
    Node *p;
    char *cp = val;
    char *user, *rev;

    for (;;)
    {
	/* skip leading whitespace */
	while (whitespace (*cp))
	    cp++;

	/* if we got to the end, we are done */
	if (*cp == '\0')
	    break;

	/* split it up into user and rev */
	user = cp;
	cp = strchr (cp, ':');
	*cp++ = '\0';
	rev = cp;
	while (!whitespace (*cp) && *cp != '\0')
	    cp++;
	if (*cp != '\0')
	    *cp++ = '\0';

	/* make a new node and add it to the list */
	p = getnode ();
	p->key = xstrdup (rev);
	p->data = xstrdup (user);
	(void) addnode (list, p);
    }
}

/*
 * process the branches list of a revision delta
 */
static void
do_branches (list, val)
    List *list;
    char *val;
{
    Node *p;
    char *cp = val;
    char *branch;

    for (;;)
    {
	/* skip leading whitespace */
	while (whitespace (*cp))
	    cp++;

	/* if we got to the end, we are done */
	if (*cp == '\0')
	    break;

	/* find the end of this branch */
	branch = cp;
	while (!whitespace (*cp) && *cp != '\0')
	    cp++;
	if (*cp != '\0')
	    *cp++ = '\0';

	/* make a new node and add it to the list */
	p = getnode ();
	p->key = xstrdup (branch);
	(void) addnode (list, p);
    }
}

/*
 * Version Number
 *
 * Returns the requested version number of the RCS file, satisfying tags and/or
 * dates, and walking branches, if necessary.
 *
 * The result is returned; null-string if error.
 */
char *RCS_getversion (RCSNode *rcs, char *tag, char *date, int force_tag_match, int *simple_tag)
{
    if (simple_tag != NULL)
		*simple_tag = 0;

    /* make sure we have something to look at... */
    assert (rcs != NULL);

    if (tag && date)
    {
		char *branch, *rev;

		if (! RCS_nodeisbranch (rcs, tag))
		{
			/* We can't get a particular date if the tag is not a
				branch.  */
			return NULL;
		}

		/* Work out the branch.  */
		if (! isdigit ((unsigned char) tag[0]))
			branch = RCS_whatbranch (rcs, tag);
		else
			branch = xstrdup (tag);

		/* Fetch the revision of branch as of date.  */
		rev = RCS_getdatebranch (rcs, date, branch);
		xfree (branch);
		return (rev);
    }
    else if (tag)
		return (RCS_gettag (rcs, tag, force_tag_match, simple_tag));
    else if (date)
		return (RCS_getdate (rcs, date, force_tag_match));
    else 
		return (RCS_head (rcs));
}

/*
 * Get existing revision number corresponding to tag or revision.
 * Similar to RCS_gettag but less interpretation imposed.
 * For example:
 * -- If tag designates a magic branch, RCS_tag2rev
 *    returns the magic branch number.
 * -- If tag is a branch tag, returns the branch number, not
 *    the revision of the head of the branch.
 * If tag or revision is not valid or does not exist in file,
 * return NULL.
 */
char *RCS_tag2rev (RCSNode *rcs, const char *tag)
{
    char *rev, *pa, *pb;
    int i;

    assert (rcs != NULL);

    if (rcs->flags & PARTIAL)
		RCS_reparsercsfile (rcs);

    /* If a valid revision, try to look it up */
    if ( RCS_valid_rev (tag) )
    {
	/* Make a copy so we can scribble on it */
	rev =  xstrdup (tag);

	/* If revision exists, return the copy */
	if (RCS_exist_rev (rcs, tag))
	    return rev;

	/* Nope, none such. If tag is not a branch we're done. */
	i = numdots (rev);
	if ((i & 1) == 1 )
	{
	    pa = strrchr (rev, '.');
	    if (i == 1 || *(pa-1) != RCS_MAGIC_BRANCH || *(pa-2) != '.')
	    {
		xfree (rev);
		error (1, 0, "revision `%s' does not exist", tag);
	    }
	}

	/* Try for a real (that is, exists in the RCS deltas) branch
	   (RCS_exist_rev just checks for real revisions and revisions
	   which have tags pointing to them).  */
	pa = RCS_getbranch (rcs, rev, 1);
	if (pa != NULL)
	{
	    xfree (pa);
	    return rev;
	}

       /* Tag is branch, but does not exist, try corresponding
	* magic branch tag.
	*
	* FIXME: assumes all magic branches are of
	* form "n.n.n ... .0.n".  I'll fix if somebody can
	* send me a method to get a magic branch tag with
	* the 0 in some other position -- <dan@gasboy.com>
	*/
	pa = strrchr (rev, '.');
	pb = xmalloc (strlen (rev) + 3);
	*pa++ = 0;
	(void) sprintf (pb, "%s.%d.%s", rev, RCS_MAGIC_BRANCH, pa);
	xfree (rev);
	rev = pb;
	if (RCS_exist_rev (rcs, rev))
	    return rev;
	error (1, 0, "revision `%s' does not exist", tag);
    }


    RCS_check_tag (tag); /* exit if not a valid tag */

    /* If tag is "HEAD", special case to get head RCS revision */
    if (tag && STREQ (tag, TAG_HEAD))
        return (RCS_head (rcs));

    /* If valid tag let translate_symtag say yea or nay. */
    rev = translate_symtag (rcs, tag);

    if (rev)
        return rev;

    /* Trust the caller to print warnings. */
    return NULL;
}

/*
 * Find the revision for a specific tag.
 * If force_tag_match is set, return NULL if an exact match is not
 * possible otherwise return RCS_head ().  We are careful to look for
 * and handle "magic" revisions specially.
 *
 * If the matched tag is a branch tag, find the head of the branch.
 *
 * Returns pointer to newly malloc'd string, or NULL.
 */
char *
RCS_gettag (rcs, symtag, force_tag_match, simple_tag)
    RCSNode *rcs;
    char *symtag;
    int force_tag_match;
    int *simple_tag;
{
    char *tag = symtag;
    int tag_allocated = 0;

    if (simple_tag != NULL)
	*simple_tag = 0;

    /* make sure we have something to look at... */
    assert (rcs != NULL);

    /* XXX this is probably not necessary, --jtc */
    if (rcs->flags & PARTIAL)
		RCS_reparsercsfile (rcs);

    /* If tag is "HEAD", special case to get head RCS revision */
    if (tag && (STREQ (tag, TAG_HEAD) || *tag == '\0'))
#if 0 /* This #if 0 is only in the Cygnus code.  Why?  Death support?  */
	if (force_tag_match && (rcs->flags & VALID) && (rcs->flags & INATTIC))
	    return ((char *) NULL);	/* head request for removed file */
	else
#endif
	    return (RCS_head (rcs));

    if (!isdigit ((unsigned char) tag[0]))
    {
	char *version;

	/* If we got a symbolic tag, resolve it to a numeric */
	version = translate_symtag (rcs, tag);
	if (version != NULL)
	{
	    int dots;
	    char *magic, *branch, *cp;

	    tag = version;
	    tag_allocated = 1;

	    /*
	     * If this is a magic revision, we turn it into either its
	     * physical branch equivalent (if one exists) or into
	     * its base revision, which we assume exists.
	     */
	    dots = numdots (tag);
	    if (dots > 2 && (dots & 1) != 0)
	    {
		branch = strrchr (tag, '.');
		cp = branch++ - 1;
		while (*cp != '.')
		    cp--;

		/* see if we have .magic-branch. (".0.") */
		magic = xmalloc (strlen (tag) + 1);
		(void) sprintf (magic, ".%d.", RCS_MAGIC_BRANCH);
		if (strncmp (magic, cp, strlen (magic)) == 0)
		{
		    /* it's magic.  See if the branch exists */
		    *cp = '\0';		/* turn it into a revision */
		    (void) sprintf (magic, "%s.%s", tag, branch);
		    branch = RCS_getbranch (rcs, magic, 1);
		    xfree (magic);
		    if (branch != NULL)
		    {
			xfree (tag);
			return (branch);
		    }
		    return (tag);
		}
		xfree (magic);
	    }
	}
	else
	{
	    /* The tag wasn't there, so return the head or NULL */
	    if (force_tag_match)
		return (NULL);
	    else
		return (RCS_head (rcs));
	}
    }

    /*
     * numeric tag processing:
     *		1) revision number - just return it
     *		2) branch number   - find head of branch
     */

    /* strip trailing dots */
    while (tag[strlen (tag) - 1] == '.')
	tag[strlen (tag) - 1] = '\0';

    if ((numdots (tag) & 1) == 0)
    {
	char *branch;

	/* we have a branch tag, so we need to walk the branch */
	branch = RCS_getbranch (rcs, tag, force_tag_match);
	if (tag_allocated)
	    xfree (tag);
	return branch;
    }
    else
    {
	Node *p;

	/* we have a revision tag, so make sure it exists */
	p = findnode (rcs->versions, tag);
	if (p != NULL)
	{
	    /* We have found a numeric revision for the revision tag.
	       To support expanding the RCS keyword Name, if
	       SIMPLE_TAG is not NULL, tell the the caller that this
	       is a simple tag which co will recognize.  FIXME: Are
	       there other cases in which we should set this?  In
	       particular, what if we expand RCS keywords internally
	       without calling co?  */
	    if (simple_tag != NULL)
		*simple_tag = 1;
	    if (! tag_allocated)
		tag = xstrdup (tag);
	    return (tag);
	}
	else
	{
	    /* The revision wasn't there, so return the head or NULL */
	    if (tag_allocated)
		xfree (tag);
	    if (force_tag_match)
		return (NULL);
	    else
		return (RCS_head (rcs));
	}
    }
}

int RCS_isfloating(RCSNode *rcs, const char *rev)
{
	int ret;
	char *bra = RCS_tag2rev(rcs,rev), *p;
	if(!bra)
		return 0;
	if(numdots(bra)<3)
		return 0;
	p=strrchr(bra,'.');
	for(--p;*p!='.';--p)
		;
	for(--p;*p!='.';--p)
		;
	ret = !strncmp(p,".0.0.",5);
	xfree(bra);
	return ret;
}


/*
 * Return a "magic" revision as a virtual branch off of REV for the RCS file.
 * A "magic" revision is one which is unique in the RCS file.  By unique, I
 * mean we return a revision which:
 *	- has a branch of 0 (see rcs.h RCS_MAGIC_BRANCH)
 *	- has a revision component which is not an existing branch off REV
 *	- has a revision component which is not an existing magic revision
 *	- is an even-numbered revision, to avoid conflicts with vendor branches
 * The first point is what makes it "magic".
 *
 * As an example, if we pass in 1.37 as REV, we will look for an existing
 * branch called 1.37.2.  If it did not exist, we would look for an
 * existing symbolic tag with a numeric part equal to 1.37.0.2.  If that
 * didn't exist, then we know that the 1.37.2 branch can be reserved by
 * creating a symbolic tag with 1.37.0.2 as the numeric part.
 *
 * This allows us to fork development with very little overhead -- just a
 * symbolic tag is used in the RCS file.  When a commit is done, a physical
 * branch is dynamically created to hold the new revision.
 *
 * Note: We assume that REV is an RCS revision and not a branch number.
 */
static char *check_rev;
char *
RCS_magicrev (rcs, rev)
    RCSNode *rcs;
    char *rev;
{
    int rev_num;
    char *xrev, *test_branch;

    xrev = xmalloc (strlen (rev) + 14); /* enough for .0.number */
    check_rev = xrev;

    /* only look at even numbered branches */
    for (rev_num = 2; ; rev_num += 2)
    {
	/* see if the physical branch exists */
	(void) sprintf (xrev, "%s.%d", rev, rev_num);
	test_branch = RCS_getbranch(rcs, xrev, 2);
	if (test_branch != NULL)	/* it did, so keep looking */
	{
	    xfree (test_branch);
	    continue;
	}

	/* now, create a "magic" revision */
	(void) sprintf (xrev, "%s.%d.%d", rev, RCS_MAGIC_BRANCH, rev_num);

	/* walk the symbols list to see if a magic one already exists */
	if (walklist (RCS_symbols(rcs), checkmagic_proc, NULL) != 0)
	    continue;

	/* we found a free magic branch.  Claim it as ours */
	return (xrev);
    }
}

/*
 * walklist proc to look for a match in the symbols list.
 * Returns 0 if the symbol does not match, 1 if it does.
 */
static int
checkmagic_proc (p, closure)
    Node *p;
    void *closure;
{
    if (STREQ (check_rev, p->data))
	return (1);
    else
	return (0);
}

/*
 * Given an RCSNode, returns non-zero if the specified revision number
 * or symbolic tag resolves to a "branch" within the rcs file.
 *
 * FIXME: this is the same as RCS_nodeisbranch except for the special
 *        case for handling a null rcsnode.
 */
int
RCS_isbranch (rcs, rev)
    RCSNode *rcs;
    const char *rev;
{
    /* numeric revisions are easy -- even number of dots is a branch */
    if (isdigit ((unsigned char) *rev))
	return ((numdots (rev) & 1) == 0);

    /* assume a revision if you can't find the RCS info */
    if (rcs == NULL)
	return (0);

    /* now, look for a match in the symbols list */
    return (RCS_nodeisbranch (rcs, rev));
}

/*
 * Given an RCSNode, returns non-zero if the specified revision number
 * or symbolic tag resolves to a "branch" within the rcs file.  We do
 * take into account any magic branches as well.
 */
int
RCS_nodeisbranch (rcs, rev)
    RCSNode *rcs;
    const char *rev;
{
    int dots;
    char *version;

    assert (rcs != NULL);

    /* numeric revisions are easy -- even number of dots is a branch */
    if (isdigit ((unsigned char) *rev))
	return ((numdots (rev) & 1) == 0);

    version = translate_symtag (rcs, rev);
    if (version == NULL)
	return (0);
    dots = numdots (version);
    if ((dots & 1) == 0)
    {
	xfree (version);
	return (1);
    }

    /* got a symbolic tag match, but it's not a branch; see if it's magic */
    if (dots > 2)
    {
	char *magic;
	char *branch = strrchr (version, '.');
	char *cp = branch - 1;
	while (*cp != '.')
	    cp--;

	/* see if we have .magic-branch. (".0.") */
	magic = xmalloc (strlen (version) + 1);
	(void) sprintf (magic, ".%d.", RCS_MAGIC_BRANCH);
	if (strncmp (magic, cp, strlen (magic)) == 0)
	{
	    xfree (magic);
	    xfree (version);
	    return (1);
	}
	xfree (magic);
    }
    xfree (version);
    return (0);
}

/*
 * Returns a pointer to malloc'ed memory which contains the branch
 * for the specified *symbolic* tag.  Magic branches are handled correctly.
 */
char *RCS_whatbranch (RCSNode *rcs, const char *rev)
{
    char *version;
    int dots;

    /* assume no branch if you can't find the RCS info */
    if (rcs == NULL)
	return ((char *) NULL);

    /* now, look for a match in the symbols list */
    version = translate_symtag (rcs, rev);
    if (version == NULL)
	return ((char *) NULL);
    dots = numdots (version);
    if ((dots & 1) == 0)
	return (version);

    /* got a symbolic tag match, but it's not a branch; see if it's magic */
    if (dots > 2)
    {
	char *magic;
	char *branch = strrchr (version, '.');
	char *cp = branch++ - 1;
	while (*cp != '.')
	    cp--;

	/* see if we have .magic-branch. (".0.") */
	magic = xmalloc (strlen (version) + 1);
	(void) sprintf (magic, ".%d.", RCS_MAGIC_BRANCH);
	if (strncmp (magic, cp, strlen (magic)) == 0)
	{
	    /* yep.  it's magic.  now, construct the real branch */
	    *cp = '\0';			/* turn it into a revision */
	    (void) sprintf (magic, "%s.%s", version, branch);
	    xfree (version);
	    return (magic);
	}
	xfree (magic);
    }
    xfree (version);
    return ((char *) NULL);
}

/* Return branch name from numeric version */
char *RCS_branchfromversion (RCSNode *rcs, const char *rev)
{
    int dots;
	char *version;
	char *cp;

    /* assume no branch if you can't find the RCS info */
    if (rcs == NULL)
		return NULL;

	version = xmalloc(strlen(rev)+32);
	strcpy(version,rev);
    dots = numdots (version);
	/* If version is not branch already */
    if ((dots & 1))
	{
		char *cp = strrchr(version, '.');
		*cp='\0';
		dots--;
	}
	if(!dots)
	{
		xfree(version);
		return NULL; /* HEAD */
	}

	cp = strrchr(version,'.');
	memmove(cp+2,cp,strlen(cp)+1);
	memcpy(cp,".0",2);

    if (rcs->symbols_data != NULL)
    {
		char *cp = rcs->symbols_data;
		char *cq;

		while((cp=strstr(cp,version))!=NULL)
		{
			cq=cp+strlen(version);
			if(cp[-1]==':' && (isspace(*cq) || !(*cq) || *cq==':'))
			{
				char *branch;
				cq=cp-1;
				cp--;
				cp--;
				while((cp>rcs->symbols_data-1) && !isspace(*cp) && *cp!=':')
					cp--;
				cp++;
				branch=xmalloc(cq-cp+1);
				strncpy(branch,cp,cq-cp);
				branch[cq-cp]='\0';
				return branch;
			}
			else
				cp+=strlen(version);
			continue;
		}
	}
	xfree(version);
	return NULL;
}

/*
 * Get the head of the specified branch.  If the branch does not exist,
 * return NULL or RCS_head depending on force_tag_match.
 * Returns NULL or a newly malloc'd string.
 */
char *RCS_getbranch (RCSNode *rcs, char *tag, int force_tag_match)
{
    Node *p, *head;
    RCSVers *vn;
    char *xtag;
    char *nextvers;
    char *cp;
	char *v,*branch;

    /* make sure we have something to look at... */
    assert (rcs != NULL);

	RCS_reparsercsfile (rcs);

	if(!tag)
		tag="HEAD";

	/* If the file is stale, get the version we locked, not HEAD */
	assert(rcs->rcsbuf.lockId);

	if(strchr(tag,'.')) /* This is a dotted name.  Convert to branch name */
	{
		branch = RCS_branchfromversion(rcs,tag);
		if(!branch)
			branch=xstrdup("HEAD");
	}
	else
		branch = xstrdup(tag);

	do_lock_version(rcs->rcsbuf.lockId, branch, &v);
	xfree(branch);
	if(v && v[0])
		return xstrdup(v);

	/* find out if the tag contains a dot, or is on the trunk */
    cp = strrchr (tag, '.');

    /* trunk processing is the special case */
    if (cp == NULL)
    {
	xtag = xmalloc (strlen (tag) + 1 + 1);	/* +1 for an extra . */
	(void) strcpy (xtag, tag);
	(void) strcat (xtag, ".");
	for (cp = rcs->head; cp != NULL;)
	{
	    if (strncmp (xtag, cp, strlen (xtag)) == 0)
		break;
	    p = findnode (rcs->versions, cp);
	    if (p == NULL)
	    {
		xfree (xtag);
		if (force_tag_match)
		    return (NULL);
		else
		    return (RCS_head (rcs));
	    }
	    vn = (RCSVers *) p->data;
	    cp = vn->next;
	}
	xfree (xtag);
	if (cp == NULL)
	{
	    if (force_tag_match)
		return (NULL);
	    else
		return (RCS_head (rcs));
	}
	return (xstrdup (cp));
    }

    /* if it had a `.', terminate the string so we have the base revision */
    *cp = '\0';

    /* look up the revision this branch is based on */
    p = findnode (rcs->versions, tag);

    /* put the . back so we have the branch again */
    *cp = '.';

    if (p == NULL)
    {
	/* if the base revision didn't exist, return head or NULL */
	if (force_tag_match)
	    return (NULL);
	else
	    return (RCS_head (rcs));
    }

    /* find the first element of the branch we are looking for */
    vn = (RCSVers *) p->data;
    if (vn->branches == NULL)
	return (NULL);
    xtag = xmalloc (strlen (tag) + 1 + 1);	/* 1 for the extra '.' */
    (void) strcpy (xtag, tag);
    (void) strcat (xtag, ".");
    head = vn->branches->list;
    for (p = head->next; p != head; p = p->next)
	if (strncmp (p->key, xtag, strlen (xtag)) == 0)
	    break;
    xfree (xtag);

    if (p == head)
    {
	/* we didn't find a match so return head or NULL */
	if (force_tag_match)
	    return (NULL);
	else
	    return (RCS_head (rcs));
    }

    /* now walk the next pointers of the branch */
    nextvers = p->key;
    do
    {
	p = findnode (rcs->versions, nextvers);
	if (p == NULL)
	{
	    /* a link in the chain is missing - return head or NULL */
	    if (force_tag_match)
		return (NULL);
	    else
		return (RCS_head (rcs));
	}
	vn = (RCSVers *) p->data;
	nextvers = vn->next;
    } while (nextvers != NULL);

    /* we have the version in our hand, so go for it */
    return (xstrdup (vn->version));
}

/* Returns the head of the branch which REV is on.  REV can be a
   branch tag or non-branch tag; symbolic or numeric.

   Returns a newly malloc'd string.  Returns NULL if a symbolic name
   isn't found.  */

char *
RCS_branch_head (rcs, rev)
    RCSNode *rcs;
    char *rev;
{
    char *num;
    char *br;
    char *retval;

    assert (rcs != NULL);

    if (RCS_nodeisbranch (rcs, rev))
	return RCS_getbranch (rcs, rev, 1);

    if (isdigit ((unsigned char) *rev))
	num = xstrdup (rev);
    else
    {
	num = translate_symtag (rcs, rev);
	if (num == NULL)
	    return NULL;
    }
    br = truncate_revnum (num);
    retval = RCS_getbranch (rcs, br, 1);
    xfree (br);
    xfree (num);
    return retval;
}

/* Get the branch point for a particular branch, that is the first
   revision on that branch.  For example, RCS_getbranchpoint (rcs,
   "1.3.2") will normally return "1.3.2.1".  TARGET may be either a
   branch number or a revision number; if a revnum, find the
   branchpoint of the branch to which TARGET belongs.

   Return RCS_head if TARGET is on the trunk or if the root node could
   not be found (this is sort of backwards from our behavior on a branch;
   the rationale is that the return value is a revision from which you
   can start walking the next fields and end up at TARGET).
   Return NULL on error.  */

char *RCS_getbranchpoint (RCSNode *rcs, char *target)
{
    char *branch, *bp;
    Node *vp;
    RCSVers *rev;
    int dots, isrevnum, brlen;

    dots = numdots (target);
    isrevnum = dots & 1;

    if (dots == 1)
	/* TARGET is a trunk revision; return rcs->head. */
	return (RCS_head (rcs));

    /* Get the revision number of the node at which TARGET's branch is
       rooted.  If TARGET is a branch number, lop off the last field;
       if it's a revision number, lop off the last *two* fields. */
    branch = xstrdup (target);
    bp = strrchr (branch, '.');
    if (bp == NULL)
	error (1, 0, "%s: confused revision number %s",
	       fn_root(rcs->path), target);
    if (isrevnum)
	while (*--bp != '.')
	    ;
    *bp = '\0';

    vp = findnode (rcs->versions, branch);
    if (vp == NULL)
    {
	error (0, 0, "%s: can't find branch point %s", fn_root(rcs->path), target);
	return NULL;
    }
    rev = (RCSVers *) vp->data;

    *bp++ = '.';
    while (*bp && *bp != '.')
	++bp;
    brlen = bp - branch;

    vp = rev->branches->list->next;
    while (vp != rev->branches->list)
    {
	/* BRANCH may be a genuine branch number, e.g. `1.1.3', or
	   maybe a full revision number, e.g. `1.1.3.6'.  We have
	   found our branch point if the first BRANCHLEN characters
	   of the revision number match, *and* if the following
	   character is a dot. */
	if (strncmp (vp->key, branch, brlen) == 0 && vp->key[brlen] == '.')
	    break;
	vp = vp->next;
    }

    xfree (branch);
    if (vp == rev->branches->list)
    {
	error (0, 0, "%s: can't find branch point %s", fn_root(rcs->path), target);
	return NULL;
    }
    else
	return (xstrdup (vp->key));
}

/*
 * Get the head of the RCS file.  If branch is set, this is the head of the
 * branch, otherwise the real head.
 * Returns NULL or a newly malloc'd string.
 */
char *RCS_head (RCSNode *rcs)
{
	char *v;
    /* make sure we have something to look at... */
    assert (rcs != NULL);
	assert (rcs->rcsbuf.lockId);

	/* If the file is stale, get the version we locked, not HEAD */
	do_lock_version(rcs->rcsbuf.lockId, rcs->branch?rcs->branch:"HEAD", &v);
	if(v && v[0])
		return xstrdup(v);

	/*
	* NOTE: we call getbranch with force_tag_match set to avoid any
	* possibility of recursion
	*/
	if (rcs->branch)
		return (RCS_getbranch (rcs, rcs->branch, 1));
	else
		return (xstrdup (rcs->head));
}

/*
 * Get the most recent revision, based on the supplied date, but use some
 * funky stuff and follow the vendor branch maybe
 */
char *
RCS_getdate (rcs, date, force_tag_match)
    RCSNode *rcs;
    char *date;
    int force_tag_match;
{
    char *cur_rev = NULL;
    char *retval = NULL;
    Node *p;
    RCSVers *vers = NULL;

    /* make sure we have something to look at... */
    assert (rcs != NULL);

    if (rcs->flags & PARTIAL)
		RCS_reparsercsfile (rcs);

    /* if the head is on a branch, try the branch first */
    if (rcs->branch != NULL)
	retval = RCS_getdatebranch (rcs, date, rcs->branch);

    /* if we found a match, we are done */
    if (retval != NULL)
	return (retval);

    /* otherwise if we have a trunk, try it */
    if (rcs->head)
    {
	p = findnode (rcs->versions, rcs->head);
	while (p != NULL)
	{
	    /* if the date of this one is before date, take it */
	    vers = (RCSVers *) p->data;
	    if (RCS_datecmp (vers->date, date) <= 0)
	    {
		cur_rev = vers->version;
		break;
	    }

	    /* if there is a next version, find the node */
	    if (vers->next != NULL)
		p = findnode (rcs->versions, vers->next);
	    else
		p = (Node *) NULL;
	}
    }

    /*
     * at this point, either we have the revision we want, or we have the
     * first revision on the trunk (1.1?) in our hands
     */

    /* if we found what we're looking for, and it's not 1.1 return it */
    if (cur_rev != NULL)
    {
	if (! STREQ (cur_rev, "1.1"))
	    return (xstrdup (cur_rev));

	/* This is 1.1;  if the date of 1.1 is not the same as that for the
	   1.1.1.1 version, then return 1.1.  This happens when the first
	   version of a file is created by a regular cvs add and commit,
	   and there is a subsequent cvs import of the same file.  */
	p = findnode (rcs->versions, "1.1.1.1");
	if (p)
	{
	    vers = (RCSVers *) p->data;
	    if (RCS_datecmp (vers->date, date) != 0)
		return xstrdup ("1.1");
	}
    }

    /* look on the vendor branch */
    retval = RCS_getdatebranch (rcs, date, CVSBRANCH);

    /*
     * if we found a match, return it; otherwise, we return the first
     * revision on the trunk or NULL depending on force_tag_match and the
     * date of the first rev
     */
    if (retval != NULL)
	return (retval);

    if (!force_tag_match || RCS_datecmp (vers->date, date) <= 0)
	return (xstrdup (vers->version));
    else
	return (NULL);
}

/*
 * Look up the last element on a branch that was put in before the specified
 * date (return the rev or NULL)
 */
static char *
RCS_getdatebranch (rcs, date, branch)
    RCSNode *rcs;
    char *date;
    char *branch;
{
    char *cur_rev = NULL;
    char *cp;
    char *xbranch, *xrev;
    Node *p;
    RCSVers *vers;

    /* look up the first revision on the branch */
    xrev = xstrdup (branch);
    cp = strrchr (xrev, '.');
    if (cp == NULL)
    {
	xfree (xrev);
	return (NULL);
    }
    *cp = '\0';				/* turn it into a revision */

    assert (rcs != NULL);

    if (rcs->flags & PARTIAL)
		RCS_reparsercsfile (rcs);

    p = findnode (rcs->versions, xrev);
    xfree (xrev);
    if (p == NULL)
	return (NULL);
    vers = (RCSVers *) p->data;

    /* Tentatively use this revision, if it is early enough.  */
    if (RCS_datecmp (vers->date, date) <= 0)
	cur_rev = vers->version;

    /* If no branches list, return now.  This is what happens if the branch
       is a (magic) branch with no revisions yet.  */
    if (vers->branches == NULL)
	return xstrdup (cur_rev);

    /* walk the branches list looking for the branch number */
    xbranch = xmalloc (strlen (branch) + 1 + 1); /* +1 for the extra dot */
    (void) strcpy (xbranch, branch);
    (void) strcat (xbranch, ".");
    for (p = vers->branches->list->next; p != vers->branches->list; p = p->next)
	if (strncmp (p->key, xbranch, strlen (xbranch)) == 0)
	    break;
    xfree (xbranch);
    if (p == vers->branches->list)
    {
	/* This is what happens if the branch is a (magic) branch with
	   no revisions yet.  Similar to the case where vers->branches ==
	   NULL, except here there was a another branch off the same
	   branchpoint.  */
	return xstrdup (cur_rev);
    }

    p = findnode (rcs->versions, p->key);

    /* walk the next pointers until you find the end, or the date is too late */
    while (p != NULL)
    {
	vers = (RCSVers *) p->data;
	if (RCS_datecmp (vers->date, date) <= 0)
	    cur_rev = vers->version;
	else
	    break;

	/* if there is a next version, find the node */
	if (vers->next != NULL)
	    p = findnode (rcs->versions, vers->next);
	else
	    p = (Node *) NULL;
    }

    /* Return whatever we found, which may be NULL.  */
    return xstrdup (cur_rev);
}

/*
 * Compare two dates in RCS format. Beware the change in format on January 1,
 * 2000, when years go from 2-digit to full format.
 */
int
RCS_datecmp (date1, date2)
    char *date1, *date2;
{
    int length_diff = strlen (date1) - strlen (date2);

    return (length_diff ? length_diff : strcmp (date1, date2));
}

/* Return the filename for the given revision, or NULL */
char *RCS_getfilename (RCSNode *rcs, char *rev)
{
	return NULL;
}

/* Look up revision REV in RCS and return the date specified for the
   revision minus FUDGE seconds (FUDGE will generally be one, so that the
   logically previous revision will be found later, or zero, if we want
   the exact date).

   The return value is the date being returned as a time_t, or (time_t)-1
   on error (previously was documented as zero on error; I haven't checked
   the callers to make sure that they really check for (time_t)-1, but
   the latter is what this function really returns).  If DATE is non-NULL,
   then it must point to MAXDATELEN characters, and we store the same
   return value there in DATEFORM format.  */
time_t
RCS_getrevtime (rcs, rev, date, fudge)
    RCSNode *rcs;
    char *rev;
    char *date;
    int fudge;
{
    char tdate[MAXDATELEN];
    struct tm xtm, *ftm;
    time_t revdate = 0;
    Node *p;
    RCSVers *vers;

    /* make sure we have something to look at... */
    assert (rcs != NULL);

    if (rcs->flags & PARTIAL)
		RCS_reparsercsfile (rcs);

    /* look up the revision */
    p = findnode (rcs->versions, rev);
    if (p == NULL)
	return (-1);
    vers = (RCSVers *) p->data;

    /* split up the date */
    ftm = &xtm;
    (void) sscanf (vers->date, SDATEFORM, &ftm->tm_year, &ftm->tm_mon,
		   &ftm->tm_mday, &ftm->tm_hour, &ftm->tm_min,
		   &ftm->tm_sec);

    /* If the year is from 1900 to 1999, RCS files contain only two
       digits, and sscanf gives us a year from 0-99.  If the year is
       2000+, RCS files contain all four digits and we subtract 1900,
       because the tm_year field should contain years since 1900.  */

    if (ftm->tm_year > 1900)
	ftm->tm_year -= 1900;

    /* put the date in a form getdate can grok */
    (void) sprintf (tdate, "%d/%d/%d GMT %d:%d:%d", ftm->tm_mon,
		    ftm->tm_mday, ftm->tm_year + 1900, ftm->tm_hour,
		    ftm->tm_min, ftm->tm_sec);

    /* turn it into seconds since the epoch */
    revdate = get_date (tdate, (struct timeb *) NULL);
    if (revdate != (time_t) -1)
    {
	revdate -= fudge;		/* remove "fudge" seconds */
	if (date)
	{
	    /* put an appropriate string into ``date'' if we were given one */
	    ftm = gmtime (&revdate);
	    (void) sprintf (date, DATEFORM,
			    ftm->tm_year + (ftm->tm_year < 100 ? 0 : 1900),
			    ftm->tm_mon + 1, ftm->tm_mday, ftm->tm_hour,
			    ftm->tm_min, ftm->tm_sec);
	}
    }
    return (revdate);
}

List *
RCS_getlocks (rcs)
    RCSNode *rcs;
{
    assert(rcs != NULL);

    if (rcs->flags & PARTIAL)
		RCS_reparsercsfile (rcs);

    if (rcs->locks_data) {
	rcs->locks = getlist ();
	do_locks (rcs->locks, rcs->locks_data);
	xfree(rcs->locks_data);
	rcs->locks_data = NULL;
    }

    return rcs->locks;
}

List *
RCS_symbols(rcs)
    RCSNode *rcs;
{
    assert(rcs != NULL);

    if (rcs->flags & PARTIAL)
		RCS_reparsercsfile (rcs);

    if (rcs->symbols_data) {
	rcs->symbols = getlist ();
	do_symbols (rcs->symbols, rcs->symbols_data);
	xfree(rcs->symbols_data);
	rcs->symbols_data = NULL;
    }

    return rcs->symbols;
}

/*
 * Return the version associated with a particular symbolic tag.
 * Returns NULL or a newly malloc'd string.
 */
static char *
translate_symtag (rcs, tag)
    RCSNode *rcs;
    const char *tag;
{
    if (rcs->flags & PARTIAL)
		RCS_reparsercsfile (rcs);

	if(tag[0]=='@')
	{
		/* We're asking for a commit id.  This is tricky... search the RCS file for
		   the commit */
		RCSVers *vers;
	    Node *head, *p, *info;

	    head = rcs->versions->list;
	    for (p = head->next; p != head; p = p->next)
		{
			vers = (RCSVers *) p->data;

			info = findnode (vers->other_delta, "commitid");
			if (info != NULL)
				if(!strcmp(info->data, tag+1))
					return xstrdup(vers->version);
		}
		return NULL;
	}

    if (rcs->symbols != NULL)
    {
	Node *p;

	/* The symbols have already been converted into a list.  */
	p = findnode (rcs->symbols, tag);
	if (p == NULL)
	    return NULL;

	return xstrdup (p->data);
    }

    if (rcs->symbols_data != NULL)
    {
	size_t len;
	char *cp;

	/* Look through the RCS symbols information.  This is like
           do_symbols, but we don't add the information to a list.  In
           most cases, we will only be called once for this file, so
           generating the list is unnecessary overhead.  */

	len = strlen (tag);
	cp = rcs->symbols_data;
	while ((cp = strchr (cp, tag[0])) != NULL)
	{
	    if ((cp == rcs->symbols_data || whitespace (cp[-1]))
		&& strncmp (cp, tag, len) == 0
		&& cp[len] == ':')
	    {
		char *v, *r;

		/* We found the tag.  Return the version number.  */

		cp += len + 1;
		v = cp;
		while (! whitespace (*cp) && *cp != '\0')
		    ++cp;
		r = xmalloc (cp - v + 1);
		strncpy (r, v, cp - v);
		r[cp - v] = '\0';
		return r;
	    }

	    while (! whitespace (*cp) && *cp != '\0')
		++cp;
	}
    }

    return NULL;
}

/*
 * The argument ARG is the getopt remainder of the -k option specified on the
 * command line.  This function returns malloc'ed space that can be used
 * directly in calls to RCS V5, with the -k flag munged correctly.
 */
char *RCS_check_kflag (const char *arg)
{
	RCS_get_kflags(arg, 1);
	if(arg[0]=='-' && arg[1]=='k')
		return xstrdup(arg);
	else
	{
		char *ret = (char*)xmalloc(strlen(arg)+3);
		strcpy(ret,"-k");
		strcat(ret,arg);
		return ret;
	}
}

kflag RCS_get_kflags(const char *arg, int err)
{
    static const char *const  keyword_usage[] =
    {
      "%s %s: invalid RCS expansion flags\n",
	  "Valid flags are one of:\n",
	  "t\tText file (default)\n",
      "b\tBinary file (merges not allowed).\n",
      "B\tBinary file using binary deltas (merges not allowed).\n",
      "u\tUnicode (UCS-2) file with BOM.\n",
	  "{encoding}\tExtended encoding type\n",
      "Followed by any of:\n",
	  "c\tForce reserved edit.\n",
      "k\tSubstitute keyword.\n",
      "v\tSubstiture value.\n",
      "l\tGenerate lockers name.\n",
	  "L\tGenerate Unix line endings on checkout.\n",
	  "o\tDon't change keywords.\n",
	  "z\tCompress deltas within RCS files.\n",
      "(Specify the --help global option for a list of other help options)\n",
      NULL,
    };
	const kflag_t *flag;
	int pos = 0;
	kflag result = {0};

	if(!arg)
	{
		result.flags=KFLAG_TEXT|KFLAG_KEYWORD|KFLAG_VALUE, ENC_UNKNOWN;
		return result; /* Default -kkv */
	}

	if(arg[0]=='-' && arg[1]=='k') /* -k */
		pos=2;

	if(arg[pos]=='{')
	{
		if(!strchr(arg+pos,'}'))
		{
			error(0,0,"Bad '-k{}' keyword syntax");
			if(err)
				usage(keyword_usage);
			pos=(strchr(arg+pos,'}')-arg)+1;
		}
		else
		{
			pos++;
			for(flag=kflag_encoding; flag->flag; flag++)
			{
				if(flag->keyword && !strncmp(arg+pos,flag->keyword,strlen(flag->keyword)) && strlen(arg+pos)>strlen(flag->keyword) && arg[pos+strlen(flag->keyword)]=='}')
				{
					result.flags = flag->bitmask;
					if(flag->encoding)
						result.encoding=flag->encoding;
					pos+=strlen(flag->keyword)+1;
					break;
				}
				if(flag->altkeyword && !strncmp(arg+pos,flag->altkeyword,strlen(flag->altkeyword)) && strlen(arg+pos)>strlen(flag->altkeyword) && arg[pos+strlen(flag->altkeyword)]=='}')
				{
					result.flags = flag->bitmask;
					if(flag->encoding)
						result.encoding=flag->encoding;
					pos+=strlen(flag->altkeyword)+1;
					break;
				}
			}
			if(!flag->flag)
			{
				error(0,0,"Unknown '-k{}' keyword");
				if(err)
					usage(keyword_usage);
				pos=(strchr(arg+pos,'}')-arg)+1;
			}
		}
	}
	else
	{
		for(flag=kflag_encoding; flag->flag; flag++)
		{
			if(flag->flag>0 && arg[pos]==flag->flag)
			{
				result.flags = flag->bitmask;
				if(flag->encoding)
					result.encoding=flag->encoding;
				pos++;
				break;
			}
		}
		if(!flag->flag)
			result.flags = KFLAG_TEXT;
	}

	while(arg[pos])
	{
		for(flag=kflag_flags; flag->flag; flag++)
		{
			if(flag->flag>0 && arg[pos]==flag->flag)
			{
				result.flags |= flag->bitmask;
				break;
			}
		}
		if(!flag->flag)
		{
			error(0,0,"Unknown expansion option '%c'.",arg[pos]);
			if(err)
				usage(keyword_usage);
		}
		pos++;
	};

	/* Set default expansions.  -kbkv is possible but probably not useful... */
	if(!(result.flags&KFLAG_EXPANSIONS))
	{
		if(result.flags&(KFLAG_TEXT|KFLAG_ENCODED))
			result.flags|=KFLAG_KEYWORD|KFLAG_VALUE;
		else if(result.flags&KFLAG_BINARY)
			result.flags|=KFLAG_PRESERVE;
	}

	/* Compatibility - -kl to -kL for those that used the old version */
	if((result.flags&KFLAG_EXPANSIONS)==KFLAG_LOCKER)
		result.flags=(result.flags&~KFLAG_EXPANSIONS)|KFLAG_KEYWORD|KFLAG_VALUE|KFLAG_UNIX;

    return result;
}

/*
 * Do some consistency checks on the symbolic tag... These should equate
 * pretty close to what RCS checks, though I don't know for certain.
 */
void RCS_check_tag (const char *tag)
{
    char *invalid = "$,.:;@";		/* invalid RCS tag characters */
    const char *cp;

    /*
     * The first character must be an alphabetic letter. The remaining
     * characters cannot be non-visible graphic characters, and must not be
     * in the set of "invalid" RCS identifier characters.
     */
    if (isalpha ((unsigned char) *tag))
    {
	for (cp = tag; *cp; cp++)
	{
	    if (!isgraph ((unsigned char) *cp))
		error (1, 0, "tag `%s' has non-visible graphic characters",
		       tag);
	    if (strchr (invalid, *cp))
		error (1, 0, "tag `%s' must not contain the characters `%s'",
		       tag, invalid);
	}
    }
    else
		error (1, 0, "tag `%s' must start with a letter", tag);
}

/*
 * TRUE if argument has valid syntax for an RCS revision or
 * branch number.  All characters must be digits or dots, first
 * and last characters must be digits, and no two consecutive
 * characters may be dots.
 *
 * Intended for classifying things, so this function doesn't
 * call error.
 */
int RCS_valid_rev (const char *rev)
{
   char last, c;
   last = *rev++;
   if (!isdigit ((unsigned char) last))
       return 0;
   while ((c = *rev++))   /* Extra parens placate -Wall gcc option */
   {
       if (c == '.')
       {
           if (last == '.')
               return 0;
           continue;
       }
       last = c;
       if (!isdigit ((unsigned char) c))
           return 0;
   }
   if (!isdigit ((unsigned char) last))
       return 0;
   return 1;
}

/*
 * Return true if RCS revision with TAG is a dead revision.
 */
int
RCS_isdead (rcs, tag)
    RCSNode *rcs;
    const char *tag;
{
    Node *p;
    RCSVers *version;

    if (rcs->flags & PARTIAL)
		RCS_reparsercsfile (rcs);

    p = findnode (rcs->versions, tag);
    if (p == NULL)
	return (0);

    version = (RCSVers *) p->data;
    return (version->dead);
}

/* Return the RCS keyword expansion mode.  For example "b" for binary.
   Returns a pointer into storage which is allocated and freed along with
   the rest of the RCS information; the caller should not modify this
   storage.  Returns NULL if the RCS file does not specify a keyword
   expansion mode; for all other errors, die with a fatal error.  */
char *
RCS_getexpand (rcs)
    RCSNode *rcs;
{
    /* Since RCS_parsercsfile_i now reads expand, don't need to worry
       about RCS_reparsercsfile.  */
    assert (rcs != NULL);
    return rcs->expand;
}

/* Set keyword expansion mode to EXPAND.  For example "b" for binary.  */
void
RCS_setexpand (rcs, expand)
    RCSNode *rcs;
    char *expand;
{
    /* Since RCS_parsercsfile_i now reads expand, don't need to worry
       about RCS_reparsercsfile.  */
    assert (rcs != NULL);
    if (rcs->expand != NULL)
	xfree (rcs->expand);
    rcs->expand = xstrdup (expand);
}

/* RCS keywords, and a matching enum.  */
struct rcs_keyword
{
    const char *string;
    size_t len;
};
#define KEYWORD_INIT(s) (s), sizeof (s) - 1
static const struct rcs_keyword keywords[] =
{
    { KEYWORD_INIT ("Author") },
    { KEYWORD_INIT ("Date") },
    { KEYWORD_INIT ("Header") },
    { KEYWORD_INIT ("Id") },
    { KEYWORD_INIT ("Locker") },
    { KEYWORD_INIT ("Log") },
    { KEYWORD_INIT ("Name") },
    { KEYWORD_INIT ("RCSfile") },
    { KEYWORD_INIT ("Revision") },
    { KEYWORD_INIT ("Source") },
    { KEYWORD_INIT ("State") },
    { NULL, 0 }
};
enum keyword
{
    KEYWORD_AUTHOR = 0,
    KEYWORD_DATE,
    KEYWORD_HEADER,
    KEYWORD_ID,
    KEYWORD_LOCKER,
    KEYWORD_LOG,
    KEYWORD_NAME,
    KEYWORD_RCSFILE,
    KEYWORD_REVISION,
    KEYWORD_SOURCE,
    KEYWORD_STATE
};

/* Convert an RCS date string into a readable string.  This is like
   the RCS date2str function.  */

static char *
printable_date (rcs_date)
     const char *rcs_date;
{
    int year, mon, mday, hour, min, sec;
    char buf[100];

    (void) sscanf (rcs_date, SDATEFORM, &year, &mon, &mday, &hour, &min,
		   &sec);
    if (year < 1900)
	year += 1900;
    sprintf (buf, "%04d/%02d/%02d %02d:%02d:%02d", year, mon, mday,
	     hour, min, sec);
    return xstrdup (buf);
}

/* Escape the characters in a string so that it can be included in an
   RCS value.  */

static char *
escape_keyword_value (value, free_value)
     const char *value;
     int *free_value;
{
    char *ret, *t;
    const char *s;

    for (s = value; *s != '\0'; s++)
    {
	char c;

	c = *s;
	if (c == '\t'
	    || c == '\n'
	    || c == '\\'
	    || c == ' '
	    || c == '$')
	{
	    break;
	}
    }

    if (*s == '\0')
    {
	*free_value = 0;
	return (char *) value;
    }

    ret = xmalloc (strlen (value) * 4 + 1);
    *free_value = 1;

    for (s = value, t = ret; *s != '\0'; s++, t++)
    {
	switch (*s)
	{
	default:
	    *t = *s;
	    break;
	case '\t':
	    *t++ = '\\';
	    *t = 't';
	    break;
	case '\n':
	    *t++ = '\\';
	    *t = 'n';
	    break;
	case '\\':
	    *t++ = '\\';
	    *t = '\\';
	    break;
	case ' ':
	    *t++ = '\\';
	    *t++ = '0';
	    *t++ = '4';
	    *t = '0';
	    break;
	case '$':
	    *t++ = '\\';
	    *t++ = '0';
	    *t++ = '4';
	    *t = '4';
	    break;
	}
    }

    *t = '\0';

    return ret;
}

/* Expand RCS keywords in the memory buffer BUF of length LEN.  This
   applies to file RCS and version VERS.  If NAME is not NULL, and is
   not a numeric revision, then it is the symbolic tag used for the
   checkout.  EXPAND indicates how to expand the keywords.  This
   function sets *RETBUF and *RETLEN to the new buffer and length.
   This function may modify the buffer BUF.  If BUF != *RETBUF, then
   RETBUF is a newly allocated buffer.  */

static void
expand_keywords (RCSNode *rcs, RCSVers *ver, const char *name, const char *log,
     size_t loglen, kflag expand, char *buf, size_t len, char **retbuf,
     size_t *retlen)
{
    struct expand_buffer
    {
	struct expand_buffer *next;
	char *data;
	size_t len;
	int free_data;
    } *ebufs = NULL;
    struct expand_buffer *ebuf_last = NULL;
    size_t ebuf_len = 0;
    char *locker;
    char *srch, *srch_next;
    size_t srch_len;

    if (expand.flags&KFLAG_PRESERVE)
    {
	*retbuf = buf;
	*retlen = len;
	return;
    }

    /* If we are using -kkvl, dig out the locker information if any.  */
    locker = NULL;
    if (expand.flags&KFLAG_LOCKER)
    {
	Node *lock;
	lock = findnode (RCS_getlocks(rcs), ver->version);
	if (lock != NULL)
	    locker = xstrdup (lock->data);
    }

    /* RCS keywords look like $STRING$ or $STRING: VALUE$.  */
    srch = buf;
    srch_len = len;
    while ((srch_next = memchr (srch, '$', srch_len)) != NULL)
    {
	char *s, *send;
	size_t slen;
	const struct rcs_keyword *keyword;
	enum keyword kw;
	char *value;
	int free_value;
	char *sub;
	size_t sublen;

	srch_len -= (srch_next + 1) - srch;
	srch = srch_next + 1;

	/* Look for the first non alphabetic character after the '$'.  */
	send = srch + srch_len;
	for (s = srch; s < send; s++)
	    if (! isalpha ((unsigned char) *s))
		break;

	/* If the first non alphabetic character is not '$' or ':',
           then this is not an RCS keyword.  */
	if (s == send || (*s != '$' && *s != ':'))
	    continue;

	/* See if this is one of the keywords.  */
	slen = s - srch;
	for (keyword = keywords; keyword->string != NULL; keyword++)
	{
	    if (keyword->len == slen
		&& strncmp (keyword->string, srch, slen) == 0)
	    {
		break;
	    }
	}
	if (keyword->string == NULL)
	    continue;

	kw = (enum keyword) (keyword - keywords);

	/* If the keyword ends with a ':', then the old value consists
           of the characters up to the next '$'.  If there is no '$'
           before the end of the line, though, then this wasn't an RCS
           keyword after all.  */
	if (*s == ':')
	{
	    for (; s < send; s++)
		if (*s == '$' || *s == '\n')
		    break;
	    if (s == send || *s != '$')
		continue;
	}

	/* At this point we must replace the string from SRCH to S
           with the expansion of the keyword KW.  */

	/* Get the value to use.  */
	free_value = 0;
	if ((!((kw==KEYWORD_LOG) && (expand.flags&KFLAG_VALUE_LOGONLY))) && !(expand.flags&KFLAG_VALUE))
	    value = NULL;
	else
	{
	    switch (kw)
	    {
	    default:
		abort ();

	    case KEYWORD_AUTHOR:
		value = ver->author;
		break;

	    case KEYWORD_DATE:
		value = printable_date (ver->date);
		free_value = 1;
		break;

	    case KEYWORD_HEADER:
	    case KEYWORD_ID:
		{
		    const char *path;
		    int free_path;
		    char *date;

		    if (kw == KEYWORD_HEADER)
				path = rcs->path;
		    else
				path = last_component (rcs->path);
		    path = escape_keyword_value (path, &free_path);
		    date = printable_date (ver->date);
		    value = xmalloc (strlen (path)
				     + strlen (ver->version)
				     + strlen (date)
				     + strlen (ver->author)
				     + strlen (ver->state)
				     + (locker == NULL ? 0 : strlen (locker))
				     + 20);

		    sprintf (value, "%s %s %s %s %s%s%s",
			     fn_root(path), ver->version, date, ver->author,
			     ver->state,
			     locker != NULL ? " " : "",
			     locker != NULL ? locker : "");
		    if (free_path)
			xfree (path);
		    xfree (date);
		    free_value = 1;
		}
		break;

	    case KEYWORD_LOCKER:
		value = locker;
		break;

	    case KEYWORD_LOG:
	    case KEYWORD_RCSFILE:
		value = escape_keyword_value (last_component (rcs->path),
					      &free_value);
		break;

	    case KEYWORD_NAME:
		if (name != NULL && ! isdigit ((unsigned char) *name))
		    value = (char *) name;
		else
		    value = NULL;
		break;

	    case KEYWORD_REVISION:
		value = ver->version;
		break;

	    case KEYWORD_SOURCE:
		value = escape_keyword_value (rcs->path, &free_value);
		break;

	    case KEYWORD_STATE:
		value = ver->state;
		break;
	    }
	}

	sub = xmalloc (keyword->len
		       + (value == NULL ? 0 : strlen (value))
		       + MAX_PATH);
	if (!(expand.flags&KFLAG_KEYWORD))
	{
	    /* Decrement SRCH and increment S to remove the $
               characters.  */
	    --srch;
	    ++srch_len;
	    ++s;
	    sublen = 0;
	}
	else
	{
	    strcpy (sub, keyword->string);
	    sublen = strlen (keyword->string);
	    if (((kw==KEYWORD_LOG) && (expand.flags&KFLAG_VALUE_LOGONLY)) || expand.flags&KFLAG_VALUE)
	    {
		sub[sublen] = ':';
		sub[sublen + 1] = ' ';
		sublen += 2;
	    }
	}
	if (value != NULL)
	{
	    strcpy (sub + sublen, value);
	    sublen += strlen (value);
	}
	if (((expand.flags&(KFLAG_VALUE|KFLAG_KEYWORD)) == (KFLAG_VALUE|KFLAG_KEYWORD))
		|| (kw==KEYWORD_LOG && (expand.flags&(KFLAG_VALUE_LOGONLY|KFLAG_KEYWORD)) == (KFLAG_VALUE_LOGONLY|KFLAG_KEYWORD)))
	{
	    sub[sublen] = ' ';
	    ++sublen;
	    sub[sublen] = '\0';
	}

	if (free_value)
	    xfree (value);

	/* The Log keyword requires special handling.  This behaviour
           is taken from RCS 5.7.  The special log message is what RCS
           uses for ci -k.  */
	if (kw == KEYWORD_LOG
	    && (sizeof "checked in with -k by " <= loglen
		|| log == NULL
		|| strncmp (log, "checked in with -k by ",
			    sizeof "checked in with -k by " - 1) != 0))
	{
	    char *start;
	    char *leader;
	    size_t leader_len, leader_sp_len;
	    const char *logend;
	    const char *snl;
	    int cnl;
	    char *date;
	    const char *sl;

	    /* We are going to insert the trailing $ ourselves, before
               the log message, so we must remove it from S, if we
               haven't done so already.  */
	    if (expand.flags&KFLAG_KEYWORD)
			++s;

	    /* CVS never has empty log messages, but old RCS files might.  */
	    if (log == NULL)
		log = "";

	    /* Find the start of the line.  */
	    start = srch;
	    while (start > buf && start[-1] != '\n')
		--start;

	    /* Copy the start of the line to use as a comment leader.  */
	    leader_len = srch - start;
	    if (expand.flags&KFLAG_KEYWORD)
			--leader_len;
	    leader = xmalloc (leader_len);
	    memcpy (leader, start, leader_len);
	    leader_sp_len = leader_len;
	    while (leader_sp_len > 0 && leader[leader_sp_len - 1] == ' ')
		--leader_sp_len;

	    /* RCS does some checking for an old style of Log here,
	       but we don't bother.  RCS issues a warning if it
	       changes anything.  */

	    /* Count the number of newlines in the log message so that
	       we know how many copies of the leader we will need.  */
	    cnl = 0;
	    logend = log + loglen;
	    for (snl = log; snl < logend; snl++)
		if (*snl == '\n')
		    ++cnl;

	    date = printable_date (ver->date);
	    sub = xrealloc (sub,
			    (sublen
			     + sizeof "Revision"
			     + strlen (ver->version)
			     + strlen (date)
			     + strlen (ver->author)
			     + loglen
			     + (cnl + 2) * leader_len
			     + MAX_PATH));
	    if (expand.flags&KFLAG_KEYWORD)
	    {
		sub[sublen] = '$';
		++sublen;
	    }
	    sub[sublen] = '\n';
	    ++sublen;
	    memcpy (sub + sublen, leader, leader_len);
	    sublen += leader_len;
	    sprintf (sub + sublen, "Revision %s  %s  %s\n",
		     ver->version, date, ver->author);
	    sublen += strlen (sub + sublen);
	    xfree (date);

	    sl = log;
	    while (sl < logend)
	    {
		if (*sl == '\n')
		{
		    memcpy (sub + sublen, leader, leader_sp_len);
		    sublen += leader_sp_len;
		    sub[sublen] = '\n';
		    ++sublen;
		    ++sl;
		}
		else
		{
		    const char *slnl;

		    memcpy (sub + sublen, leader, leader_len);
		    sublen += leader_len;
		    for (slnl = sl; slnl < logend && *slnl != '\n'; ++slnl)
			;
		    if (slnl < logend)
			++slnl;
		    memcpy (sub + sublen, sl, slnl - sl);
		    sublen += slnl - sl;
		    sl = slnl;
		}
	    }

	    memcpy (sub + sublen, leader, leader_sp_len);
	    sublen += leader_sp_len;

	    xfree (leader);
	}

	/* Now SUB contains a string which is to replace the string
	   from SRCH to S.  SUBLEN is the length of SUB.  */

	if (srch + sublen == s)
	{
	    memcpy (srch, sub, sublen);
	    xfree (sub);
	}
	else
	{
	    struct expand_buffer *ebuf;

	    /* We need to change the size of the buffer.  We build a
               list of expand_buffer structures.  Each expand_buffer
               structure represents a portion of the final output.  We
               concatenate them back into a single buffer when we are
               done.  This minimizes the number of potentially large
               buffer copies we must do.  */

	    if (ebufs == NULL)
	    {
		ebufs = (struct expand_buffer *) xmalloc (sizeof *ebuf);
		ebufs->next = NULL;
		ebufs->data = buf;
		ebufs->free_data = 0;
		ebuf_len = srch - buf;
		ebufs->len = ebuf_len;
		ebuf_last = ebufs;
	    }
	    else
	    {
		assert (srch >= ebuf_last->data);
		assert (srch <= ebuf_last->data + ebuf_last->len);
		ebuf_len -= ebuf_last->len - (srch - ebuf_last->data);
		ebuf_last->len = srch - ebuf_last->data;
	    }

	    ebuf = (struct expand_buffer *) xmalloc (sizeof *ebuf);
	    ebuf->data = sub;
	    ebuf->len = sublen;
	    ebuf->free_data = 1;
	    ebuf->next = NULL;
	    ebuf_last->next = ebuf;
	    ebuf_last = ebuf;
	    ebuf_len += sublen;

	    ebuf = (struct expand_buffer *) xmalloc (sizeof *ebuf);
	    ebuf->data = s;
	    ebuf->len = srch_len - (s - srch);
	    ebuf->free_data = 0;
	    ebuf->next = NULL;
	    ebuf_last->next = ebuf;
	    ebuf_last = ebuf;
	    ebuf_len += srch_len - (s - srch);
	}

	srch_len -= (s - srch);
	srch = s;
    }

    if (locker != NULL)
	xfree (locker);

    if (ebufs == NULL)
    {
	*retbuf = buf;
	*retlen = len;
    }
    else
    {
	char *ret;

	ret = xmalloc (ebuf_len);
	*retbuf = ret;
	*retlen = ebuf_len;
	while (ebufs != NULL)
	{
	    struct expand_buffer *next;

	    memcpy (ret, ebufs->data, ebufs->len);
	    ret += ebufs->len;
	    if (ebufs->free_data)
		xfree (ebufs->data);
	    next = ebufs->next;
	    xfree (ebufs);
	    ebufs = next;
	}
    }
}

/* Check out a revision from an RCS file.

   If PFN is not NULL, then ignore WORKFILE and SOUT.  Call PFN zero
   or more times with the contents of the file.  CALLERDAT is passed,
   uninterpreted, to PFN.  (The current code will always call PFN
   exactly once for a non empty file; however, the current code
   assumes that it can hold the entire file contents in memory, which
   is not a good assumption, and might change in the future).

   Otherwise, if WORKFILE is not NULL, check out the revision to
   WORKFILE.  However, if WORKFILE is not NULL, and noexec is set,
   then don't do anything.

   Otherwise, if WORKFILE is NULL, check out the revision to SOUT.  If
   SOUT is RUN_TTY, then write the contents of the revision to
   standard output.  When using SOUT, the output is generally a
   temporary file; don't bother to get the file modes correct.

   REV is the numeric revision to check out.  It may be NULL, which
   means to check out the head of the default branch.

   If NAMETAG is not NULL, and is not a numeric revision, then it is
   the tag that should be used when expanding the RCS Name keyword.

   OPTIONS is a string such as "-kb" or "-kv" for keyword expansion
   options.  It may be NULL to use the default expansion mode of the
   file, typically "-kkv".

   On an error which prevented checking out the file, either print a
   nonfatal error and return 1, or give a fatal error.  On success,
   return 0.  */

/* This function mimics the behavior of `rcs co' almost exactly.  The
   chief difference is in its support for preserving file ownership,
   permissions, and special files across checkin and checkout -- see
   comments in RCS_checkin for some issues about this. -twp */

int
RCS_checkout (RCSNode *rcs, char *workfile, char *rev, char *nametag, char *options,
     char *sout, RCSCHECKOUTPROC pfn, void *callerdat, mode_t *pmode)
{
    int free_rev = 0;
	kflag expand = {0};
    FILE *fp, *ofp;
    char *key;
    char *value;
    size_t len;
    int free_value = 0;
    char *log = NULL;
    size_t loglen;
    Node *vp = NULL;
    uid_t rcs_owner = (uid_t) -1;
    gid_t rcs_group = (gid_t) -1;
    mode_t rcs_mode;
	void *zbuf = NULL;

	TRACE(1,"RCS_checkout (%s, %s, %s, %s)",
			PATCH_NULL(fn_root(rcs->path)),
			rev != NULL ? rev : "",
			options != NULL ? options : "",
			(pfn != NULL ? "(function)"
			 : (workfile != NULL
			    ? workfile
			    : (sout != RUN_TTY ? sout : "(stdout)"))));

    assert (rev == NULL || isdigit ((unsigned char) *rev));

    if (noexec && workfile != NULL)
		return 0;

    assert (sout == RUN_TTY || workfile == NULL);
    assert (pfn == NULL || (sout == RUN_TTY && workfile == NULL));

    /* Some callers, such as Checkin or remove_file, will pass us a
       branch.  */
    if (rev != NULL && (numdots (rev) & 1) == 0)
    {
	rev = RCS_getbranch (rcs, rev, 1);
	if (rev == NULL)
	    error (1, 0, "internal error: bad branch tag in checkout");
	free_rev = 1;
    }

    if (rev == NULL || STREQ (rev, rcs->head))
    {
	int gothead;

	/* We want the head revision.  Try to read it directly.  */

    RCS_reparsercsfile (rcs);

	gothead = 0;
	if (! rcsbuf_getrevnum (&rcs->rcsbuf, &key))
	    error (1, 0, "unexpected EOF reading %s", fn_root(rcs->path));
	while (rcsbuf_getkey (&rcs->rcsbuf, &key, &value))
	{
	    if (STREQ (key, "log"))
		log = rcsbuf_valcopy (&rcs->rcsbuf, value, 0, &loglen);
	    else if (STREQ (key, "text"))
	    {
		gothead = 1;
		break;
	    }
	}

	if (! gothead)
	{
	    error (0, 0, "internal error: cannot find head text");
	    if (free_rev)
		xfree (rev);
	    return 1;
	}

	rcsbuf_valpolish (&rcs->rcsbuf, value, 0, &len);

	/* Handle zip expansion of head */
	{
		RCSVers *vers;

		vp = findnode (rcs->versions, rcs->head);
		if (vp == NULL)
			error (1, 0, "internal error: no revision information for %s",
			rev == NULL ? rcs->head : rev);
		vers = (RCSVers *) vp->data;
		if(vers->type && (STREQ(vers->type,"compressed_text") || STREQ(vers->type,"compressed_binary")))
		{
			uLong zlen;

			z_stream stream = {0};
			inflateInit(&stream);
			zlen = ntohl(*(unsigned long *)value);
			if(zlen)
			{
				stream.avail_in = len-4;
				stream.next_in = value+4;
				stream.avail_out = zlen;
				stream.next_out = zbuf = xmalloc(zlen);
				if(inflate(&stream, Z_FINISH)!=Z_STREAM_END)
				{
					error(1,0,"internal error: inflate failed");
				}
			}
			len = zlen;
			value = zbuf;
		}
	}
	}
	else
	{
 	struct rcsbuffer *rcsbufp;

	/* It isn't the head revision of the trunk.  We'll need to
	   walk through the deltas.  */

	fp = NULL;
	if (rcs->flags & PARTIAL)
	    RCS_reparsercsfile (rcs);

	if (fp == NULL)
	    rcsbufp = NULL;
	else
	    rcsbufp = &rcs->rcsbuf;

	RCS_deltas (rcs, fp, rcsbufp, rev, RCS_FETCH, &value, &len,
		    &log, &loglen);
	free_value = 1;
    }

	/* If OPTIONS is NULL or the empty string, then the old code would
       invoke the RCS co program with no -k option, which means that
       co would use the string we have stored in vers->expand.  */

	/* Handle version specific expansion */
	if(options == NULL || options[0] == '\0')
    {
		RCSVers *vers;
		Node *info;

		vp = findnode (rcs->versions, rev == NULL ? rcs->head : rev);
		if (vp == NULL)
			error (1, 0, "internal error: no revision information for %s",
			rev == NULL ? rcs->head : rev);
		vers = (RCSVers *) vp->data;

		info = findnode (vers->other_delta, "kopt");
		if (info != NULL)
			options = info->data;
    }

    if ((options == NULL || options[0] == '\0') && rcs->expand == NULL)
		expand.flags = KFLAG_TEXT|KFLAG_KEYWORD|KFLAG_VALUE;
    else
    {
		const char *ouroptions;

		if (options != NULL && options[0] != '\0')
		{
			if(options[0]=='-' && options[1]=='k')
				ouroptions = options + 2;
			else
				ouroptions = options;
		}
		else
			ouroptions = rcs->expand;

		expand = RCS_get_kflags(ouroptions, 1);
    }

    /* Handle permissions */
    {
		RCSVers *vers;
		Node *info;

		vp = findnode (rcs->versions, rev == NULL ? rcs->head : rev);
		if (vp == NULL)
			error (1, 0, "internal error: no revision information for %s",
			rev == NULL ? rcs->head : rev);
		vers = (RCSVers *) vp->data;

		info = findnode (vers->other_delta, "permissions");
		if (info != NULL)
			rcs_mode = (mode_t) strtoul (info->data, NULL, 8);
		else
			rcs_mode = 0666;
		TRACE(3,"rcs_mode = %04o",rcs_mode);
    }

    if (!(expand.flags&KFLAG_PRESERVE))
    {
		char *newvalue;

		/* Don't fetch the delta node again if we already have it. */
		if (vp == NULL)
		{
			vp = findnode (rcs->versions, rev == NULL ? rcs->head : rev);
			if (vp == NULL)
			error (1, 0, "internal error: no revision information for %s",
				rev == NULL ? rcs->head : rev);
		}

		expand_keywords (rcs, (RCSVers *) vp->data, nametag, log, loglen,
				expand, value, len, &newvalue, &len);

		if (newvalue != value)
		{
			if (free_value)
				xfree (value);
			value = newvalue;
			free_value = 1;
		}
    }

    if (free_rev)
		xfree (rev);

	xfree (log);

    if (pfn != NULL)
    {
		/* The PFN interface is very simple to implement right now, as
			we always have the entire file in memory.  */
		pfn(callerdat, len?value:"", len);
    }
    else
    {
	/* Not a special file: write to WORKFILE or SOUT. */
	if (workfile == NULL)
	{
	    if (sout == RUN_TTY)
		ofp = stdout;
	    else
	    {
		/* Symbolic links should be removed before replacement, so that
		   `fopen' doesn't follow the link and open the wrong file. */
		if (islink (sout))
		    if (unlink_file (sout) < 0)
			error (1, errno, "cannot remove %s", sout);
		ofp = CVS_FOPEN (sout, ((expand.flags&KFLAG_BINARY) || (expand.flags&KFLAG_UNIX)) ? "wb" : "w");
		if (ofp == NULL)
		    error (1, errno, "cannot open %s", sout);
	    }
	}
	else
	{
	    /* Output is supposed to go to WORKFILE, so we should open that
	       file.  Symbolic links should be removed first (see above). */
	    if (islink (workfile))
		if (unlink_file (workfile) < 0)
		    error (1, errno, "cannot remove %s", workfile);

		ofp = CVS_FOPEN (workfile, ((expand.flags&KFLAG_BINARY) || (expand.flags&KFLAG_UNIX)) ? "wb" : "w");

	    /* If the open failed because the existing workfile was not
	       writable, try to chmod the file and retry the open.  */
	    if (ofp == NULL && errno == EACCES
		&& isfile (workfile) && !iswritable (workfile))
	    {
		xchmod (workfile, 1);
		ofp = CVS_FOPEN (workfile, ((expand.flags&KFLAG_BINARY) || (expand.flags&KFLAG_UNIX)) ? "wb" : "w");
	    }

	    if (ofp == NULL)
	    {
		error (0, errno, "cannot open %s", workfile);
		if (free_value)
		    xfree (value);
		xfree(zbuf);
		return 1;
	    }
	}

	if (workfile == NULL && sout == RUN_TTY)
	{
	    if (expand.flags&KFLAG_BINARY)
			cvs_output_binary (value, len);
	    else
	    {
			/* cvs_output requires the caller to check for zero
			length.  */
			if (len > 0)
			{
				if(!server_active)
					cvs_output(value,len);
				else
				{
					char *p;
					value=xrealloc(value,len+1);
					value[len]='\0'; 
					p=strrchr(value,'\n');
					if(!p)
						cvs_output_tagged("text",value);
					else if(!*(p+1))
						cvs_output(value,len);
					else
					{
						/* If the file doesn't end in '\n' then send a special 'MT text' output */
						cvs_output(value,(p-value)+1);
						cvs_output_tagged("text",p+1);
					}
				}
			}
	    }
	}
	else if((expand.flags&KFLAG_ENCODED) && !server_active) // In server mode we never use unicode
	{
		if(output_utf8_as_encoding(fileno(ofp),value,len,expand.encoding))
		{
		    error (0, errno, "cannot write %s",
			   (workfile != NULL
			    ? workfile
			    : (sout != RUN_TTY ? sout : "stdout")));
		    if (free_value)
				xfree (value);
			xfree(zbuf);
		    return 1;
		}
	}
	else
	{
	    /* NT 4.0 is said to have trouble writing 2099999 bytes
	       (for example) in a single fwrite.  So break it down
	       (there is no need to be writing that much at once
	       anyway; it is possible that LARGEST_FWRITE should be
	       somewhat larger for good performance, but for testing I
	       want to start with a small value until/unless a bigger
	       one proves useful).  */
#define LARGEST_FWRITE 102400
	    size_t nleft = len;
	    size_t nstep = (len < LARGEST_FWRITE ? len : LARGEST_FWRITE);
	    char *p = value;

	    while (nleft > 0)
	    {
		if (fwrite (p, 1, nstep, ofp) != nstep)
		{
		    error (0, errno, "cannot write %s",
			   (workfile != NULL
			    ? workfile
			    : (sout != RUN_TTY ? sout : "stdout")));
		    if (free_value)
				xfree (value);
			xfree(zbuf);
		    return 1;
		}
		p += nstep;
		nleft -= nstep;
		if (nleft < nstep)
		    nstep = nleft;
	    }
	}
    }

    if (free_value)
		xfree (value);
	xfree(zbuf);

    if(pmode)
      *pmode = rcs_mode & ~(S_IWRITE | S_IWGRP | S_IWOTH);

    if (workfile != NULL)
    {
		int ret;

		if (fclose (ofp) < 0)
		{
			error (0, errno, "cannot close %s", workfile);
			return 1;
		}

		ret = chmod (workfile, rcs_mode & ~(S_IWRITE | S_IWGRP | S_IWOTH));
		if (ret < 0)
		{
			error (0, errno, "cannot change mode of file %s",
			fn_root(workfile));
		}
    }
    else if (sout != RUN_TTY)
    {
		if (fclose (ofp) < 0)
		{
			error (0, errno, "cannot close %s", sout);
			return 1;
		}
    }

	/* We only trace the first 256 chars */
	if(trace>1 && sout!=RUN_TTY)
	{
		int len;
		char buf[256];
		FILE *f;

		f=fopen(sout,"r");
		if(f==NULL)
			TRACE(1,"checkout didn't generate a file... something wierd happened");
		else
		{
			len = fread(buf,255,1,f);
			fclose(f);
			buf[len]='\0';
			TRACE(1,"checkout -> %s",PATCH_NULL(buf));
		}
	}
    return 0;
}

static RCSVers *RCS_findlock_or_tip PROTO ((RCSNode *rcs));

/* Find the delta currently locked by the user.  From the `ci' man page:

	"If rev is omitted, ci tries to  derive  the  new  revision
	 number  from  the  caller's  last lock.  If the caller has
	 locked the tip revision of a branch, the new  revision  is
	 appended  to  that  branch.   The  new  revision number is
	 obtained by incrementing the tip revision number.  If  the
	 caller  locked a non-tip revision, a new branch is started
	 at that revision by incrementing the highest branch number
	 at  that  revision.   The default initial branch and level
	 numbers are 1.

	 If rev is omitted and the caller has no lock, but owns the
	 file  and  locking is not set to strict, then the revision
	 is appended to the default branch (normally the trunk; see
	 the -b option of rcs(1))."

   RCS_findlock_or_tip finds the unique revision locked by the caller
   and returns its delta node.  If the caller has not locked any
   revisions (and is permitted to commit to an unlocked delta, as
   described above), return the tip of the default branch. */

static RCSVers *
RCS_findlock_or_tip (rcs)
    RCSNode *rcs;
{
    const char *user = getcaller();
    Node *lock, *p;
    List *locklist;

    /* Find unique delta locked by caller. This code is very similar
       to the code in RCS_unlock -- perhaps it could be abstracted
       into a RCS_findlock function. */
    locklist = RCS_getlocks (rcs);
    lock = NULL;
	if(locklist)
	{
		for (p = locklist->list->next; p != locklist->list; p = p->next)
		{
		if (STREQ (p->data, user))
		{
			if (lock != NULL)
			{
			error (0, 0, "\
	%s: multiple revisions locked by %s; please specify one", fn_root(rcs->path), user);
			return NULL;
			}
			lock = p;
		}
		}
	}

    if (lock != NULL)
    {
	/* Found an old lock, but check that the revision still exists. */
	p = findnode (rcs->versions, lock->key);
	if (p == NULL)
	{
	    error (0, 0, "%s: can't unlock nonexistent revision %s",
		   fn_root(rcs->path),
		   lock->key);
	    return NULL;
	}
	return (RCSVers *) p->data;
    }

    /* No existing lock.  The RCS rule is that this is an error unless
       locking is nonstrict AND the file is owned by the current
       user.  Trying to determine the latter is a portability nightmare
       in the face of NT, VMS, AFS, and other systems with non-unix-like
       ideas of users and owners.  In the case of CVS, we should never get
       here (as long as the traditional behavior of making sure to call
       RCS_lock persists).  Anyway, we skip the RCS error checks
       and just return the default branch or head.  The reasoning is that
       those error checks are to make users lock before a checkin, and we do
       that in other ways if at all anyway (e.g. rcslock.pl).  */

    p = findnode (rcs->versions, RCS_getbranch (rcs, rcs->branch, 0));
    return (RCSVers *) p->data;
}

/* Revision number string, R, must contain a `.'.
   Return a newly-malloc'd copy of the prefix of R up
   to but not including the final `.'.  */

static char *
truncate_revnum (r)
    const char *r;
{
    size_t len;
    char *new_r;
    char *dot = strrchr (r, '.');

    assert (dot);
    len = dot - r;
    new_r = xmalloc (len + 1);
    memcpy (new_r, r, len);
    *(new_r + len) = '\0';
    return new_r;
}

/* Revision number string, R, must contain a `.'.
   R must be writable.  Replace the rightmost `.' in R with
   the NUL byte and return a pointer to that NUL byte.  */

static char *
truncate_revnum_in_place (r)
    char *r;
{
    char *dot = strrchr (r, '.');
    assert (dot);
    *dot = '\0';
    return dot;
}

/* Revision number strings, R and S, must each contain a `.'.
   R and S must be writable and must have the same number of dots.
   Truncate R and S for the comparison, then restored them to their
   original state.
   Return the result (see compare_revnums) of comparing R and S
   ignoring differences in any component after the rightmost `.'.  */

static int
compare_truncated_revnums (r, s)
    char *r;
    char *s;
{
    char *r_dot = truncate_revnum_in_place (r);
    char *s_dot = truncate_revnum_in_place (s);
    int cmp;

    assert (numdots (r) == numdots (s));

    cmp = compare_revnums (r, s);

    *r_dot = '.';
    *s_dot = '.';

    return cmp;
}

/* Return a malloc'd copy of the string representing the highest branch
   number on BRANCHNODE.  If there are no branches on BRANCHNODE, return NULL.
   FIXME: isn't the max rev always the last one?
   If so, we don't even need a loop.  */

static char *max_rev PROTO ((const RCSVers *));

static char *max_rev (const RCSVers *branchnode)
{
    Node *head;
    Node *bp;
    char *max;

    if (branchnode->branches == NULL)
    {
        return NULL;
    }

    max = NULL;
    head = branchnode->branches->list;
    for (bp = head->next; bp != head; bp = bp->next)
    {
	if (max == NULL || compare_truncated_revnums (max, bp->key) < 0)
	{
	    max = bp->key;
	}
    }
    assert (max);

    return truncate_revnum (max);
}

/* Create BRANCH in RCS's delta tree.  BRANCH may be either a branch
   number or a revision number.  In the former case, create the branch
   with the specified number; in the latter case, create a new branch
   rooted at node BRANCH with a higher branch number than any others.
   Return the number of the tip node on the new branch. */

static char *
RCS_addbranch (rcs, branch)
    RCSNode *rcs;
    const char *branch;
{
    char *branchpoint, *newrevnum;
    Node *nodep, *bp;
    Node *marker;
    RCSVers *branchnode;

    /* Append to end by default.  */
    marker = NULL;

    branchpoint = xstrdup (branch);
    if ((numdots (branchpoint) & 1) == 0)
    {
	truncate_revnum_in_place (branchpoint);
    }

    /* Find the branch rooted at BRANCHPOINT. */
    nodep = findnode (rcs->versions, branchpoint);
    if (nodep == NULL)
    {
	error (0, 0, "%s: can't find branch point %s", fn_root(rcs->path), branchpoint);
	xfree (branchpoint);
	return NULL;
    }
    xfree (branchpoint);
    branchnode = (RCSVers *) nodep->data;

    /* If BRANCH was a full branch number, make sure it is higher than MAX. */
    if ((numdots (branch) & 1) == 1)
    {
	if (branchnode->branches == NULL)
	{
	    /* We have to create the first branch on this node, which means
	       appending ".2" to the revision number. */
	    newrevnum = (char *) xmalloc (strlen (branch) + 3);
	    strcpy (newrevnum, branch);
	    strcat (newrevnum, ".2");
	}
	else
	{
	    char *max = max_rev (branchnode);
	    assert (max);
	    newrevnum = increment_revnum (max);
	    xfree (max);
	}
    }
    else
    {
	newrevnum = xstrdup (branch);

	if (branchnode->branches != NULL)
	{
	    Node *head;
	    Node *bp;

	    /* Find the position of this new branch in the sorted list
	       of branches.  */
	    head = branchnode->branches->list;
	    for (bp = head->next; bp != head; bp = bp->next)
	    {
		char *dot;
		int found_pos;

		/* The existing list must be sorted on increasing revnum.  */
		assert (bp->next == head
			|| compare_truncated_revnums (bp->key,
						      bp->next->key) < 0);
		dot = truncate_revnum_in_place (bp->key);
		found_pos = (compare_revnums (branch, bp->key) < 0);
		*dot = '.';

		if (found_pos)
		{
		    break;
		}
	    }
	    marker = bp;
	}
    }

    newrevnum = (char *) xrealloc (newrevnum, strlen (newrevnum) + 3);
    strcat (newrevnum, ".1");

    /* Add this new revision number to BRANCHPOINT's branches list. */
    if (branchnode->branches == NULL)
	branchnode->branches = getlist();
    bp = getnode();
    bp->key = xstrdup (newrevnum);

    /* Append to the end of the list by default, that is, just before
       the header node, `list'.  */
    if (marker == NULL)
	marker = branchnode->branches->list;

    {
	int fail;
	fail = insert_before (branchnode->branches, marker, bp);
	assert (!fail);
    }

    return newrevnum;
}

/* Check in to RCSFILE with revision REV (which must be greater than
   the largest revision) and message MESSAGE (which is checked for
   legality).  If FLAGS & RCS_FLAGS_DEAD, check in a dead revision.
   If FLAGS & RCS_FLAGS_QUIET, tell ci to be quiet.  If FLAGS &
   RCS_FLAGS_MODTIME, use the working file's modification time for the
   checkin time.  WORKFILE is the working file to check in from, or
   NULL to use the usual RCS rules for deriving it from the RCSFILE.
   If FLAGS & RCS_FLAGS_KEEPFILE, don't unlink the working file;
   unlinking the working file is standard RCS behavior, but is rarely
   appropriate for CVS.

   This function should almost exactly mimic the behavior of `rcs ci'.  The
   principal point of difference is the support here for preserving file
   ownership and permissions in the delta nodes.  This is not a clean
   solution -- precisely because it diverges from RCS's behavior -- but
   it doesn't seem feasible to do this anywhere else in the code. [-twp]

   Return value is -1 for error (and errno is set to indicate the
   error), positive for error (and an error message has been printed),
   or zero for success.  */

int RCS_checkin (RCSNode *rcs, char *workfile, char *message, char *rev, int flags,
			 const char *merge_from_tag1, const char *merge_from_tag2, RCSCHECKINPROC callback, char **pnewversion)
{
    RCSVers *delta, *commitpt;
    Deltatext *dtext;
    Node *nodep;
    char *tmpfile, *changefile, *chtext;
    char *diffopts;
    size_t bufsize;
    int buflen, chtextlen;
    int status, checkin_quiet, allocated_workfile;
    struct tm *ftm;
    time_t modtime;
    int adding_branch = 0;
    struct stat sb;
	Node *np;
	char *workfilename;
	char buf[64];	/* static buffer should be safe: see usage. -twp */
	kflag kf = RCS_get_kflags(rcs->expand, 1);
	const char *deltat;
	kflag kf_empty = {0};
	size_t lockId_temp;

    commitpt = NULL;

    if (rcs->flags & PARTIAL)
		RCS_reparsercsfile (rcs);

    /* Get basename of working file.  Is there a library function to
       do this?  I couldn't find one. -twp */
    allocated_workfile = 0;
	workfilename = NULL;
    if (workfile == NULL)
    {
		if(callback)
		{
			char *text;
			size_t len;
			FILE *fp;

			workfile = cvs_temp_name();
			if(callback(workfile, &text, &len, &workfilename))
				error(1,0,"Checkin callback failed");
			fp = CVS_FOPEN(workfile,"wb");
			if(!fp)
				error(1,errno,"Couldn't create working file");
			if(len)
			{
				if(fwrite(text,1,len,fp)!=len)
					error(1,errno,"Couldn't write to working file");
			}
			fclose(fp);
			xfree(text);
		}
		else
		{
			char *p;
			int extlen = strlen (RCSEXT);
			workfile = xstrdup (last_component (rcs->path));
			p = workfile + (strlen (workfile) - extlen);
			assert (strncmp (p, RCSEXT, extlen) == 0);
			*p = '\0';
		}
		allocated_workfile = 1;
    }

    /* If the filename is a symbolic link, follow it and replace it
       with the destination of the link.  We need to do this before
       calling rcs_internal_lockfile, or else we won't put the lock in
       the right place. */
    resolve_symlink (&(rcs->path));

    checkin_quiet = flags & RCS_FLAGS_QUIET;
    if (!checkin_quiet)
    {
	cvs_output (fn_root(rcs->path), 0);
	cvs_output ("  <--  ", 7);
	cvs_output (workfilename?workfilename:workfile, 0);
	cvs_output ("\n", 1);
    }

    /* Create new delta node. */
    delta = (RCSVers *) xmalloc (sizeof (RCSVers));
    memset (delta, 0, sizeof (RCSVers));
    delta->author = xstrdup(getcaller ());
	/* Deltatype is text unless specified otherwise */
	if(kf.flags&KFLAG_BINARY_DELTA)
		delta->type = xstrdup((kf.flags&KFLAG_COMPRESS_DELTA)?"compressed_binary":"binary");
	else
		delta->type = xstrdup((kf.flags&KFLAG_COMPRESS_DELTA)?"compressed_text":"text");
    if (flags & RCS_FLAGS_MODTIME)
    {
	struct stat ws;
	if (stat (workfile, &ws) < 0)
	{
	    error (1, errno, "cannot stat %s", workfile);
	}
	modtime = ws.st_mtime;
    }
    else
	(void) time (&modtime);
    ftm = gmtime (&modtime);
    delta->date = (char *) xmalloc (MAXDATELEN);
    (void) sprintf (delta->date, DATEFORM,
		    ftm->tm_year + (ftm->tm_year < 100 ? 0 : 1900),
		    ftm->tm_mon + 1, ftm->tm_mday, ftm->tm_hour,
		    ftm->tm_min, ftm->tm_sec);
    if (flags & RCS_FLAGS_DEAD)
    {
	delta->state = xstrdup (RCSDEAD);
	delta->dead = 1;
    }
    else
	delta->state = xstrdup ("Exp");

	delta->other_delta = getlist();

    /*  save the permission info. */
	if(callback)
	{
		strcpy(buf,"0644");
	}
	else
	{
		if (CVS_LSTAT (workfile, &sb) < 0)
			error (1, 1, "cannot lstat %s", workfile);

		(void) sprintf (buf, "%o", (int)(sb.st_mode & 0777));
	}
	np = getnode();
	np->type = RCSFIELD;
	np->key = xstrdup ("permissions");
	np->data = xstrdup (buf);
	addnode (delta->other_delta, np);

	/* save the commit ID */
	np = getnode();
	np->type = RCSFIELD;
	np->key = xstrdup ("commitid");
	np->data = xstrdup(global_session_id);
	addnode (delta->other_delta, np);

	/* save the keyword expansion mode */
	np = getnode();
	np->type = RCSFIELD;
	np->key = xstrdup ("kopt");
	np->data = xstrdup(rcs->expand?rcs->expand:"kv");
	addnode (delta->other_delta, np);

	/* Record the mergepoints */
	/* We didn't used to record mergepoints unless they were single ones - I think this behaviour
	   is/was incorrect */
	if(merge_from_tag1 && merge_from_tag1[0])
	{
		np = getnode();
		np->type = RCSFIELD;
		np->key = xstrdup ("mergepoint1");
		if(merge_from_tag2 && merge_from_tag2[0]) /* For a two revision merge, record the second target */
			np->data = xstrdup(merge_from_tag2); 
		else
			np->data = xstrdup(merge_from_tag1);
		addnode (delta->other_delta, np);
	}

	/* save the current filename */
	np = getnode();
	np->type = RCSFIELD;
	np->key = xstrdup ("filename");
	np->data = xstrdup(workfile);
	addnode (delta->other_delta, np);

	/* Create a new deltatext node. */
    dtext = (Deltatext *) xmalloc (sizeof (Deltatext));
    memset (dtext, 0, sizeof (Deltatext));

    dtext->log = make_message_rcslegal (message);

    /* If the delta tree is empty, then there's nothing to link the
       new delta into.  So make a new delta tree, snarf the working
       file contents, and just write the new RCS file. */
    if (rcs->head == NULL)
    {
	char *newrev;
	FILE *fout;

	/* Figure out what the first revision number should be. */
	if (rev == NULL || *rev == '\0')
	    newrev = xstrdup ("1.1");
	else if (numdots (rev) == 0)
	{
	    newrev = (char *) xmalloc (strlen (rev) + 3);
	    strcpy (newrev, rev);
	    strcat (newrev, ".1");
	}
	else
	    newrev = xstrdup (rev);

	/* Don't need to xstrdup NEWREV because it's already dynamic, and
	   not used for anything else.  (Don't need to free it, either.) */
	rcs->head = newrev;
	delta->version = xstrdup (newrev);
	nodep = getnode();
	nodep->type = RCSVERS;
	nodep->delproc = rcsvers_delproc;
	nodep->data = (char *) delta;
	nodep->key = delta->version;
	(void) addnode (rcs->versions, nodep);

	dtext->version = xstrdup (newrev);
	bufsize = 0;
	get_file (workfile, workfile,
		(kf.flags&KFLAG_BINARY) ? "rb" : "r",
		&dtext->text, &bufsize, &dtext->len, kf);

	if (!checkin_quiet)
	{
	    cvs_output ("initial revision: ", 0);
	    cvs_output (rcs->head, 0);
	    cvs_output ("\n", 1);
	}

	fout = rcs_internal_lockfile (rcs->path, &lockId_temp);
	RCS_putadmin (rcs, fout);
	RCS_putdtree (rcs, rcs->head, fout);
	RCS_putdesc (rcs, fout);
	rcs->delta_pos = CVS_FTELL (fout);
	if (rcs->delta_pos == -1)
	    error (1, errno, "cannot ftell for %s", fn_root(rcs->path));
	putdeltatext (fout, dtext, kf.flags&KFLAG_COMPRESS_DELTA);
	rcsbuf_close(&rcs->rcsbuf);
	{
		char *branch = RCS_branchfromversion(rcs,dtext->version);
		do_modified(lockId_temp,dtext->version,"",branch?branch:"HEAD",'A');
		xfree(branch);
	}
	rcs_internal_unlockfile(fout, rcs->path, lockId_temp);

	if ((flags & RCS_FLAGS_KEEPFILE) == 0)
	{
	    if (unlink_file (workfile) < 0)
		/* FIXME-update-dir: message does not include update_dir.  */
		error (0, errno, "cannot remove %s", workfile);
	}

	if (!checkin_quiet)
	    cvs_output ("done\n", 5);

	status = 0;
	goto checkin_done;
    }

    /* Derive a new revision number.  From the `ci' man page:

	 "If rev  is  a revision number, it must be higher than the
	 latest one on the branch to which  rev  belongs,  or  must
	 start a new branch.

	 If  rev is a branch rather than a revision number, the new
	 revision is appended to that branch.  The level number  is
	 obtained  by  incrementing the tip revision number of that
	 branch.  If rev  indicates  a  non-existing  branch,  that
	 branch  is  created  with  the  initial  revision numbered
	 rev.1."

       RCS_findlock_or_tip handles the case where REV is omitted.
       RCS 5.7 also permits REV to be "$" or to begin with a dot, but
       we do not address those cases -- every routine that calls
       RCS_checkin passes it a numeric revision. */

    if (rev == NULL || *rev == '\0')
    {
	/* Figure out where the commit point is by looking for locks.
	   If the commit point is at the tip of a branch (or is the
	   head of the delta tree), then increment its revision number
	   to obtain the new revnum.  Otherwise, start a new
	   branch. */
	commitpt = RCS_findlock_or_tip (rcs);
	if (commitpt == NULL)
	{
	    status = 1;
	    goto checkin_done;
	}
	else if (commitpt->next == NULL
		 || STREQ (commitpt->version, rcs->head))
	    delta->version = increment_revnum (commitpt->version);
	else
	    delta->version = RCS_addbranch (rcs, commitpt->version);
    }
    else
    {
	/* REV is either a revision number or a branch number.  Find the
	   tip of the target branch. */
	char *branch, *tip, *newrev, *p;
	int dots, isrevnum;

	assert (isdigit ((unsigned char) *rev));

	newrev = xstrdup (rev);
	dots = numdots (newrev);
	isrevnum = dots & 1;

	branch = xstrdup (rev);
	if (isrevnum)
	{
	    p = strrchr (branch, '.');
	    *p = '\0';
	}

	/* Find the tip of the target branch.  If we got a one- or two-digit
	   revision number, this will be the head of the tree.  Exception:
	   if rev is a single-field revision equal to the branch number of
	   the trunk (usually "1") then we want to treat it like an ordinary
	   branch revision. */
	if (dots == 0)
	{
	    tip = xstrdup (rcs->head);
	    if (atoi (tip) != atoi (branch))
	    {
		newrev = (char *) xrealloc (newrev, strlen (newrev) + 3);
		strcat (newrev, ".1");
		dots = isrevnum = 1;
	    }
	}
	else if (dots == 1)
	    tip = xstrdup (rcs->head);
	else
	    tip = RCS_getbranch (rcs, branch, 1);

	/* If the branch does not exist, and we were supplied an exact
	   revision number, signal an error.  Otherwise, if we were
	   given only a branch number, create it and set COMMITPT to
	   the branch point. */
	if (tip == NULL)
	{
	    if (isrevnum)
	    {
		error (0, 0, "%s: can't find branch point %s",
		       fn_root(rcs->path), branch);
		xfree (branch);
		xfree (newrev);
		status = 1;
		goto checkin_done;
	    }
	    delta->version = RCS_addbranch (rcs, branch);
	    if (!delta->version)
	    {
		xfree (branch);
		xfree (newrev);
		status = 1;
		goto checkin_done;
	    }
	    adding_branch = 1;
	    p = strrchr (branch, '.');
	    *p = '\0';
	    tip = xstrdup (branch);
	}
	else
	{
	    if (isrevnum)
	    {
		/* NEWREV must be higher than TIP. */
		if (compare_revnums (tip, newrev) >= 0)
		{
		    error (0, 0,
			   "%s: revision %s too low; must be higher than %s",
			   fn_root(rcs->path),
			   newrev, tip);
		    xfree (branch);
		    xfree (newrev);
		    xfree (tip);
		    status = 1;
		    goto checkin_done;
		}
		delta->version = xstrdup (newrev);
	    }
	    else
		/* Just increment the tip number to get the new revision. */
		delta->version = increment_revnum (tip);
	}

	nodep = findnode (rcs->versions, tip);
	commitpt = (RCSVers *) nodep->data;

	xfree (branch);
	xfree (newrev);
	xfree (tip);
    }

    assert (delta->version != NULL);

    /* If COMMITPT is locked by us, break the lock.  If it's locked
       by someone else, signal an error. */
    nodep = findnode (RCS_getlocks (rcs), commitpt->version);
    if (nodep != NULL)
    {
	if (! STREQ (nodep->data, delta->author))
	{
	    /* If we are adding a branch, then leave the old lock around.
	       That is sensible in the sense that when adding a branch,
	       we don't need to use the lock to tell us where to check
	       in.  It is fishy in the sense that if it is our own lock,
	       we break it.  However, this is the RCS 5.7 behavior (at
	       the end of addbranch in ci.c in RCS 5.7, it calls
	       removelock only if it is our own lock, not someone
	       else's).  */

	    if (!adding_branch)
	    {
		error (0, 0, "%s: revision %s locked by %s",
		       fn_root(rcs->path),
		       nodep->key, nodep->data);
		status = 1;
		goto checkin_done;
	    }
	}
	else
	    delnode (nodep);
    }

    dtext->version = xstrdup (delta->version);

    /* Obtain the change text for the new delta.  If DELTA is to be the
       new head of the tree, then its change text should be the contents
       of the working file, and LEAFNODE's change text should be a diff.
       Else, DELTA's change text should be a diff between LEAFNODE and
       the working file. */

    tmpfile = cvs_temp_name();
    status = RCS_checkout (rcs, NULL, commitpt->version, NULL,
			   (kf.flags&KFLAG_BINARY)
			    ? "-kb"
			    : "-ko",
			   tmpfile,
			   (RCSCHECKOUTPROC)0, NULL, NULL);

    if (status != 0)
	error (1, 0,
	       "could not check out revision %s of `%s'",
	       commitpt->version, fn_root(rcs->path));

    bufsize = buflen = 0;
    chtext = NULL;
    chtextlen = 0;
    changefile = cvs_temp_name();

    /* Diff options should include --binary if the RCS file has -kb set
       in its `expand' field. */
	if(kf.flags&KFLAG_BINARY)
		diffopts = "-a -n --binary";
	else if(kf.flags&KFLAG_ENCODED && !server_active)
	{
		static char __diffopts[32] = "-a -n --encoding=";
		sprintf(__diffopts+strlen(__diffopts),"%d",kf.encoding);
		diffopts = __diffopts;
	}
	else
		diffopts = "-a -n";

    if (STREQ (commitpt->version, rcs->head) && numdots (delta->version) == 1)
    {
		TRACE(2,"Insert delta at head (%s,%s)",PATCH_NULL(commitpt->version),PATCH_NULL(delta->version));

		/* If this revision is being inserted on the trunk, the change text
		for the new delta should be the contents of the working file ... */
		bufsize = 0;
		get_file (workfile, workfile,
			(kf.flags&KFLAG_BINARY) ? "rb" : "r",
			&dtext->text, &bufsize, &dtext->len, kf);

		/* ... and the change text for the old delta should be a diff. */
		commitpt->text = (Deltatext *) xmalloc (sizeof (Deltatext));
		memset (commitpt->text, 0, sizeof (Deltatext));

		if(kf.flags&KFLAG_BINARY_DELTA)
		{
			char *tmptxt = NULL;
			size_t tmplen;

			bufsize = 0;
			get_file(tmpfile,tmpfile, (kf.flags&KFLAG_BINARY)? "rb" : "r", &tmptxt, &bufsize, &tmplen, kf);

			if(binary_delta(&dtext->text,dtext->len,&tmptxt,tmplen,&commitpt->text->text,&commitpt->text->len))
			{
				error(1, errno, "error binary diffing %s", workfile);
			}
			xfree(tmptxt);

			deltat = (kf.flags&KFLAG_COMPRESS_DELTA)?"compressed_binary":"binary";
			if(!commitpt->type || strcmp(commitpt->type,deltat))
			{
				/* Patch up the existing delta as it is converted to binary if not already set */
				xfree(commitpt->type);
				commitpt->type = xstrdup(deltat);
			}
		}
		else
		{
			bufsize = 0;
			switch (diff_exec (workfile, tmpfile, NULL, NULL, diffopts, changefile))
			{
				case 0:
				case 1:
				break;
				case -1:
				/* FIXME-update-dir: message does not include update_dir.  */
				error (1, errno, "error diffing %s", workfile);
				break;
				default:
				/* FIXME-update-dir: message does not include update_dir.  */
				error (1, 0, "error diffing %s", workfile);
				break;
			}

			/* OK, the text file case here is really dumb.  Logically
			speaking we want diff to read the files in text mode,
			convert them to the canonical form found in RCS files
			(which, we hope at least, is independent of OS--always
			bare linefeeds), and then work with change texts in that
			format.  However, diff_exec both generates change
			texts and produces output for user purposes (e.g. patch.c),
			and there is no way to distinguish between the two cases.
			So we actually implement the text file case by writing the
			change text as a text file, then reading it as a text file.
			This should cause no harm, but doesn't strike me as
			immensely clean.  */
			get_file (changefile, changefile,
				(kf.flags&KFLAG_BINARY) ? "rb" : "r",
				&commitpt->text->text, &bufsize, &commitpt->text->len, kf);

			/* If COMMITPT->TEXT->TEXT is NULL, it means that CHANGEFILE
			was empty and that there are no differences between revisions.
			In that event, we want to force RCS_rewrite to write an empty
			string for COMMITPT's change text.  Leaving the change text
			field set NULL won't work, since that means "preserve the original
			change text for this delta." */
			if (commitpt->text->text == NULL)
			{
				commitpt->text->text = xstrdup ("");
				commitpt->text->len = 0;
			}

			/* Patch up the existing delta as it is converted to text if not already set */
			deltat = (kf.flags&KFLAG_COMPRESS_DELTA)?"compressed_text":"text";
			if(!commitpt->type || strcmp(commitpt->type,deltat))
			{
				/* Patch up the existing delta as it is converted to binary if not already set */
				xfree(commitpt->type);
				commitpt->type = xstrdup(deltat);
			}
		}

		if(kf.flags&KFLAG_COMPRESS_DELTA)
		{
			/* We need to compress the delta here, because it won't be compressed by RCS_rewrite */
			uLong zlen;
			void *zbuf;

			z_stream stream = {0};
			deflateInit(&stream,Z_DEFAULT_COMPRESSION);
			zlen = deflateBound(&stream, commitpt->text->len);
			stream.avail_in = commitpt->text->len;
			stream.next_in = commitpt->text->text;
			stream.avail_out = zlen;
			zbuf = xmalloc(zlen+4);
			stream.next_out = ((char*)zbuf)+4;
			*(unsigned long *)zbuf=htonl(commitpt->text->len);
			if(deflate(&stream, Z_FINISH)!=Z_STREAM_END)
			{
				error(1,0,"internal error: deflate failed");
			}
			deflateEnd(&stream);
			xfree(commitpt->text->text);
			commitpt->text->text = zbuf;
			commitpt->text->len = zlen+4;
		}
    }
    else
    {
		TRACE(2,"Insert delta at branch (%s,%s)",PATCH_NULL(commitpt->version),PATCH_NULL(delta->version));

		/* This file is not being inserted at the head, but on a side
		branch somewhere.  Make a diff from the previous revision
		to the working file. */

		if(kf.flags&KFLAG_BINARY_DELTA)
		{
			char *tmptxt = NULL, *worktxt = NULL;
			size_t tmplen,worklen;

			bufsize = 0;
			get_file(tmpfile,tmpfile, (kf.flags&KFLAG_BINARY)? "rb" : "r", &tmptxt, &bufsize, &tmplen, kf);

			bufsize = 0;
			get_file(workfile,workfile, (kf.flags&KFLAG_BINARY)? "rb" : "r", &worktxt, &bufsize, &worklen, kf);

			if(binary_delta(&tmptxt,tmplen,&worktxt,worklen,&dtext->text, &dtext->len))
			{
				error(1, errno, "error binary diffing %s", workfile);
			}
			xfree(tmptxt);
			xfree(worktxt);
			bufsize=dtext->len; /* I don't think this is used but set it anyway */
		}
		else
		{
			switch (diff_exec (tmpfile, workfile, NULL, NULL, diffopts, changefile))
			{
				case 0:
				case 1:
				break;
				case -1:
				/* FIXME-update-dir: message does not include update_dir.  */
				error (1, errno, "error diffing %s", workfile);
				break;
				default:
				/* FIXME-update-dir: message does not include update_dir.  */
				error (1, 0, "error diffing %s", workfile);
				break;
			}
			/* See the comment above, at the other get_file invocation,
			regarding binary vs. text.  */
			get_file (changefile, changefile,
				(kf.flags&KFLAG_BINARY) ? "rb" : "r",
				&dtext->text, &bufsize,
				&dtext->len, kf_empty);
		}
		if (dtext->text == NULL)
		{
			dtext->text = xstrdup ("");
			dtext->len = 0;
		}
    }

    /* Update DELTA linkage.  It is important not to do this before
       the very end of RCS_checkin; if an error arises that forces
       us to abort checking in, we must not have malformed deltas
       partially linked into the tree.

       If DELTA and COMMITPT are on different branches, do nothing --
       DELTA is linked to the tree through COMMITPT->BRANCHES, and we
       don't want to change `next' pointers.

       Otherwise, if the nodes are both on the trunk, link DELTA to
       COMMITPT; otherwise, link COMMITPT to DELTA. */

    if (numdots (commitpt->version) == numdots (delta->version))
    {
	if (STREQ (commitpt->version, rcs->head))
	{
	    delta->next = rcs->head;
	    rcs->head = xstrdup (delta->version);
	}
	else
	    commitpt->next = xstrdup (delta->version);
    }

    /* Add DELTA to RCS->VERSIONS. */
    if (rcs->versions == NULL)
	rcs->versions = getlist();
    nodep = getnode();
    nodep->type = RCSVERS;
    nodep->delproc = rcsvers_delproc;
    nodep->data = (char *) delta;
    nodep->key = delta->version;
    (void) addnode (rcs->versions, nodep);

    /* Write the new RCS file, inserting the new delta at COMMITPT. */
    if (!checkin_quiet)
    {
		cvs_output ("new revision: ", 14);
		cvs_output (delta->version, 0);
		cvs_output ("; previous revision: ", 21);
		cvs_output (commitpt->version, 0);
		cvs_output ("\n", 1);
    }

    RCS_rewrite (rcs, dtext, commitpt->version, kf.flags&KFLAG_COMPRESS_DELTA);

    if ((flags & RCS_FLAGS_KEEPFILE) == 0)
    {
		if (unlink_file (workfile) < 0)
			/* FIXME-update-dir: message does not include update_dir.  */
			error (1, errno, "cannot remove %s", workfile);
    }
    if (unlink_file (tmpfile) < 0)
		error (0, errno, "cannot remove %s", tmpfile);
    xfree (tmpfile);
    if (unlink_file (changefile) < 0)
		error (0, errno, "cannot remove %s", changefile);
    xfree (changefile);

    if (!checkin_quiet)
	cvs_output ("done\n", 5);

 checkin_done:
    if (allocated_workfile)
	{
		if(callback)
			CVS_UNLINK(workfile);
		xfree (workfile);
	}
	xfree(workfilename);

    if (commitpt != NULL && commitpt->text != NULL)
    {
		freedeltatext (commitpt->text);
		commitpt->text = NULL;
    }

    freedeltatext (dtext);

	if(pnewversion)
		*pnewversion=xstrdup(delta->version);

    if (status != 0)
		free_rcsvers_contents (delta);

    return status;
}

/* This structure is passed between RCS_cmp_file and cmp_file_buffer.  */

struct cmp_file_data
{
    const char *filename;
    FILE *fp;
    int different;
	kflag expand;
};

/* Compare the contents of revision REV of RCS file RCS with the
   contents of the file FILENAME.  OPTIONS is a string for the keyword
   expansion options.  Return 0 if the contents of the revision are
   the same as the contents of the file, 1 if they are different.  */

int
RCS_cmp_file (rcs, rev, options, filename)
     RCSNode *rcs;
     char *rev;
     char *options;
     const char *filename;
{
    int binary;
    FILE *fp;
    struct cmp_file_data data;
    int retcode;
	kflag expand;

	expand = RCS_get_kflags(RCS_getexpand (rcs), 1);
	binary = expand.flags&(KFLAG_BINARY|KFLAG_ENCODED);

    {
        fp = CVS_FOPEN (filename, binary ? FOPEN_BINARY_READ : "r");
	if (fp == NULL)
	    /* FIXME-update-dir: should include update_dir in message.  */
	    error (1, errno, "cannot open file %s for comparing", filename);

        data.filename = filename;
        data.fp = fp;
        data.different = 0;
		data.expand = expand;

        retcode = RCS_checkout (rcs, (char *) NULL, rev, (char *) NULL,
				options, RUN_TTY, cmp_file_buffer,
				(void *) &data, NULL);

        /* If we have not yet found a difference, make sure that we are at
           the end of the file.  */
        if (! data.different)
        {
	    if (getc (fp) != EOF)
		data.different = 1;
        }

        fclose (fp);

	if (retcode != 0)
	    return 1;

        return data.different;
    }
}

/* This is a subroutine of RCS_cmp_file.  It is passed to
   RCS_checkout.  */

#define CMP_BUF_SIZE (8 * 1024)

static void cmp_file_buffer (void *callerdat, const char *buffer, size_t len)
{
    struct cmp_file_data *data = (struct cmp_file_data *) callerdat;
    char *filebuf,*unibuf;
	int unicode=0;
	size_t unilen;
	int first;
	encoding_type type;

    /* If we've already found a difference, we don't need to check
       further.  */
    if (data->different)
		return;

	if((!server_active) && (data->expand.flags&KFLAG_ENCODED))
	{
		unicode=1;
		type=data->expand.encoding;
	}

	if(unicode)
	{
		/* Fixup the file length to cope with the unicode->utf8 translation */
		fseek(data->fp,0,SEEK_END);
		len = ftell(data->fp);
		fseek(data->fp,0,SEEK_SET);

		unibuf = xmalloc (len > CMP_BUF_SIZE*3 ? CMP_BUF_SIZE*3 : len*3);
	}
	else
		unibuf = NULL;

	filebuf = xmalloc (len > CMP_BUF_SIZE ? CMP_BUF_SIZE : len);

	first = 1;
	while (len > 0)
    {
	size_t checklen;

	checklen = len > CMP_BUF_SIZE ? CMP_BUF_SIZE : len;
	if (fread (filebuf, 1, checklen, data->fp) != checklen)
	{
	    if (ferror (data->fp))
		error (1, errno, "cannot read file %s for comparing",
		       data->filename);
	    data->different = 1;
	    xfree (filebuf);
		xfree (unibuf);
	    return;
	}

	if(unicode)
	{
		convert_encoding_buffer_to_utf8(filebuf,checklen,unibuf,&unilen,first,&type);
		if (memcmp (unibuf, buffer, unilen) != 0)
		{
			data->different = 1;
		    xfree (filebuf);
			xfree (unibuf);
			return;
		}
		buffer += unilen;
		len -= checklen;
	}
	else
	{
		if (memcmp (filebuf, buffer, checklen) != 0)
		{
			data->different = 1;
			xfree (filebuf);
			return;
		}
		buffer += checklen;
		len -= checklen;
	}

	first = 0;
    }

    xfree (filebuf);
	xfree (unibuf);
}

/* For RCS file RCS, make symbolic tag TAG point to revision REV.
   This validates that TAG is OK for a user to use.  Return value is
   -1 for error (and errno is set to indicate the error), positive for
   error (and an error message has been printed), or zero for success.  */

int
RCS_settag (RCSNode *rcs, const char *tag, const char *rev, const char *date)
{
    List *symbols;
    Node *node;

    if (rcs->flags & PARTIAL)
		RCS_reparsercsfile (rcs);

    /* FIXME: This check should be moved to RCS_check_tag.  There is no
       reason for it to be here.  */
    if (STREQ (tag, TAG_BASE)
	|| STREQ (tag, TAG_HEAD))
    {
	/* Print the name of the tag might be considered redundant
	   with the caller, which also prints it.  Perhaps this helps
	   clarify why the tag name is considered reserved, I don't
	   know.  */
	error (0, 0, "Attempt to add reserved tag name %s", tag);
	return 1;
    }

	if(tag[0]=='@')
	{
		error(0,0, "Invalid tag name %s", tag);
		return 1;
	}

    /* A revision number of NULL means use the head or default branch.
       If rev is not NULL, it may be a symbolic tag or branch number;
       expand it to the correct numeric revision or branch head. */
    if (rev == NULL)
	rev = rcs->branch ? rcs->branch : rcs->head;

    /* At this point rcs->symbol_data may not have been parsed.
       Calling RCS_symbols will force it to be parsed into a list
       which we can easily manipulate.  */
    symbols = RCS_symbols (rcs);
    if (symbols == NULL)
    {
	symbols = getlist ();
	rcs->symbols = symbols;
    }
    node = findnode (symbols, tag);
    if (node != NULL)
    {
	xfree (node->data);
	node->data = xstrdup (rev);
    }
    else
    {
	node = getnode ();
	node->key = xstrdup (tag);
	node->data = xstrdup (rev);
	(void) addnode_at_front (symbols, node);
    }

    return 0;
}

/* Delete the symbolic tag TAG from the RCS file RCS.  Return 0 if
   the tag was found (and removed), or 1 if it was not present.  (In
   either case, the tag will no longer be in RCS->SYMBOLS.) */

int
RCS_deltag (rcs, tag)
    RCSNode *rcs;
    const char *tag;
{
    List *symbols;
    Node *node;
    if (rcs->flags & PARTIAL)
		RCS_reparsercsfile (rcs);

    symbols = RCS_symbols (rcs);
    if (symbols == NULL)
	return 1;

    node = findnode (symbols, tag);
    if (node == NULL)
	return 1;

    delnode (node);

    return 0;
}

/* Set the default branch of RCS to REV.  */

int
RCS_setbranch (rcs, rev)
     RCSNode *rcs;
     const char *rev;
{
    if (rcs->flags & PARTIAL)
		RCS_reparsercsfile (rcs);

    if (rev && ! *rev)
	rev = NULL;

    if (rev == NULL && rcs->branch == NULL)
	return 0;
    if (rev != NULL && rcs->branch != NULL && STREQ (rev, rcs->branch))
	return 0;

    if (rcs->branch != NULL)
	xfree (rcs->branch);
    rcs->branch = xstrdup (rev);

    return 0;
}

/* Lock revision REV.  LOCK_QUIET is 1 to suppress output.  FIXME:
   Most of the callers only call us because RCS_checkin still tends to
   like a lock (a relic of old behavior inherited from the RCS ci
   program).  If we clean this up, only "cvs admin -l" will still need
   to call RCS_lock.  */

/* FIXME-twp: if a lock owned by someone else is broken, should this
   send mail to the lock owner?  Prompt user?  It seems like such an
   obscure situation for CVS as almost not worth worrying much
   about. */

int
RCS_lock (rcs, rev, lock_quiet)
     RCSNode *rcs;
     char *rev;
     int lock_quiet;
{
    List *locks;
    Node *p;
    const char *user;
    char *xrev = NULL;

    if (rcs->flags & PARTIAL)
		RCS_reparsercsfile (rcs);

    locks = RCS_getlocks (rcs);
    if (locks == NULL)
	locks = rcs->locks = getlist();
    user = getcaller();

    /* A revision number of NULL means lock the head or default branch. */
    if (rev == NULL)
	xrev = RCS_head (rcs);
    else
	xrev = RCS_gettag (rcs, rev, 1, (int *) NULL);

    /* Make sure that the desired revision exists.  Technically,
       we can update the locks list without even checking this,
       but RCS 5.7 did this.  And it can't hurt. */
    if (xrev == NULL || findnode (rcs->versions, xrev) == NULL)
    {
	if (!lock_quiet)
	    error (0, 0, "%s: revision %s absent", fn_root(rcs->path), rev);
	xfree (xrev);
	return 1;
    }

    /* Is this rev already locked? */
    p = findnode (locks, xrev);
    if (p != NULL)
    {
	if (STREQ (p->data, user))
	{
	    /* We already own the lock on this revision, so do nothing. */
	    xfree (xrev);
	    return 0;
	}

#if 0
	/* Well, first of all, "rev" below should be "xrev" to avoid
	   core dumps.  But more importantly, should we really be
	   breaking the lock unconditionally?  What CVS 1.9 does (via
	   RCS) is to prompt "Revision 1.1 is already locked by fred.
	   Do you want to break the lock? [ny](n): ".  Well, we don't
	   want to interact with the user (certainly not at the
	   server/protocol level, and probably not in the command-line
	   client), but isn't it more sensible to give an error and
	   let the user run "cvs admin -u" if they want to break the
	   lock?  */

	/* Break the lock. */
	if (!lock_quiet)
	{
	    cvs_output (rev, 0);
	    cvs_output (" unlocked\n", 0);
	}
	delnode (p);
#else
	error (1, 0, "Revision %s is already locked by %s", xrev, p->data);
#endif
    }

    /* Create a new lock. */
    p = getnode();
    p->key = xrev;	/* already xstrdupped */
    p->data = xstrdup (getcaller());
    (void) addnode_at_front (locks, p);

    if (!lock_quiet)
    {
	cvs_output (xrev, 0);
	cvs_output (" locked\n", 0);
    }

    return 0;
}

/* Unlock revision REV.  UNLOCK_QUIET is 1 to suppress output.  FIXME:
   Like RCS_lock, this can become a no-op if we do the checkin
   ourselves.

   If REV is not null and is locked by someone else, break their
   lock and notify them.  It is an open issue whether RCS_unlock
   queries the user about whether or not to break the lock. */

int
RCS_unlock (rcs, rev, unlock_quiet)
     RCSNode *rcs;
     char *rev;
     int unlock_quiet;
{
    Node *lock;
    List *locks;
    const char *user;
    char *xrev = NULL;

    user = getcaller();
    if (rcs->flags & PARTIAL)
		RCS_reparsercsfile (rcs);

    /* If rev is NULL, unlock the latest revision (first in
       rcs->locks) held by the caller. */
    if (rev == NULL)
    {
	Node *p;

	/* No-ops: attempts to unlock an empty tree or an unlocked file. */
	if (rcs->head == NULL)
	{
	    if (!unlock_quiet)
		cvs_outerr ("can't unlock an empty tree\n", 0);
	    return 0;
	}

	locks = RCS_getlocks (rcs);
	if (locks == NULL)
	{
	    if (!unlock_quiet)
		cvs_outerr ("No locks are set.\n", 0);
	    return 0;
	}

	lock = NULL;
	for (p = locks->list->next; p != locks->list; p = p->next)
	{
	    if (lock != NULL)
	    {
		if (!unlock_quiet)
		    error (0, 0, "\
%s: multiple revisions locked by %s; please specify one", fn_root(rcs->path), user);
		return 1;
	    }
	    lock = p;
	}
	if (lock == NULL)
	    return 0;	/* no lock found, ergo nothing to do */
	xrev = xstrdup (lock->key);
    }
    else
    {
	xrev = RCS_gettag (rcs, rev, 1, (int *) NULL);
	if (xrev == NULL)
	{
	    error (0, 0, "%s: revision %s absent", fn_root(rcs->path), rev);
	    return 1;
	}
    }

    lock = findnode (RCS_getlocks (rcs), xrev);
    if (lock == NULL)
    {
	/* This revision isn't locked. */
	xfree (xrev);
	return 0;
    }

    if (! STREQ (lock->data, user))
    {
        /* If the revision is locked by someone else, notify
	   them.  Note that this shouldn't ever happen if RCS_unlock
	   is called with a NULL revision, since that means "whatever
	   revision is currently locked by the caller." */
	char *repos, *workfile;
	repos = xstrdup (rcs->path);
	workfile = strrchr (repos, '/');
	*workfile++ = '\0';
	notify_do ('C', workfile, user, NULL, NULL, repos);
	xfree (repos);
    }

    delnode (lock);
    if (!unlock_quiet)
    {
	cvs_output (xrev, 0);
	cvs_output (" unlocked\n", 0);
    }

    xfree (xrev);
    return 0;
}

/* Add USER to the access list of RCS.  Do nothing if already present.
   FIXME-twp: check syntax of USER to make sure it's a valid id. */

void
RCS_addaccess (rcs, user)
    RCSNode *rcs;
    char *user;
{
    char *access, *a;

    if (rcs->flags & PARTIAL)
	RCS_reparsercsfile (rcs);

    if (rcs->access == NULL)
	rcs->access = xstrdup (user);
    else
    {
	access = xstrdup (rcs->access);
	for (a = strtok (access, " "); a != NULL; a = strtok (NULL, " "))
	{
	    if (STREQ (a, user))
	    {
		xfree (access);
		return;
	    }
	}
	xfree (access);
	rcs->access = (char *) xrealloc
	    (rcs->access, strlen (rcs->access) + strlen (user) + 2);
	strcat (rcs->access, " ");
	strcat (rcs->access, user);
    }
}

/* Remove USER from the access list of RCS. */

void
RCS_delaccess (rcs, user)
    RCSNode *rcs;
    char *user;
{
    char *p, *s;
    int ulen;

    if (rcs->flags & PARTIAL)
	RCS_reparsercsfile (rcs);

    if (rcs->access == NULL)
	return;

    if (user == NULL)
    {
        xfree (rcs->access);
        rcs->access = NULL;
        return;
    }

    p = rcs->access;
    ulen = strlen (user);
    while (p != NULL)
    {
	if (strncmp (p, user, ulen) == 0 && (p[ulen] == '\0' || p[ulen] == ' '))
	    break;
	p = strchr (p, ' ');
	if (p != NULL)
	    ++p;
    }

    if (p == NULL)
	return;

    s = p + ulen;
    while (*s != '\0')
	*p++ = *s++;
    *p = '\0';
}

char *
RCS_getaccess (rcs)
    RCSNode *rcs;
{
    if (rcs->flags & PARTIAL)
	RCS_reparsercsfile (rcs);

    return rcs->access;
}

/* Return a nonzero value if the revision specified by ARG is found.  */

static int findtag (Node *node, void *arg)
{
    char *rev = (char *)arg;

    if (STREQ (node->data, rev))
	return 1;
    else
	return 0;
}

/* Delete revisions between REV1 and REV2.  The changes between the two
   revisions must be collapsed, and the result stored in the revision
   immediately preceding the lower one.  Return 0 for successful completion,
   1 otherwise.

   Solution: check out the revision preceding REV1 and the revision
   following REV2.  Use call_diff to find aggregate diffs between
   these two revisions, and replace the delta text for the latter one
   with the new aggregate diff.  Alternatively, we could write a
   function that takes two change texts and combines them to produce a
   new change text, without checking out any revs or calling diff.  It
   would be hairy, but so, so cool.

   If INCLUSIVE is set, then TAG1 and TAG2, if non-NULL, tell us to
   delete that revision as well (cvs admin -o tag1:tag2).  If clear,
   delete up to but not including that revision (cvs admin -o tag1::tag2).
   This does not affect TAG1 or TAG2 being NULL; the meaning of the start
   point in ::tag2 and :tag2 is the same and likewise for end points.  */

int
RCS_delete_revs (rcs, tag1, tag2, inclusive)
    RCSNode *rcs;
    char *tag1;
    char *tag2;
    int inclusive;
{
    char *next;
    Node *nodep;
    RCSVers *revp = NULL;
    RCSVers *beforep;
    int status, found;
    int save_noexec;
	char *diffopts,*checkoutopts;

    char *branchpoint = NULL;
    char *rev1 = NULL;
    char *rev2 = NULL;
    int rev1_inclusive = inclusive;
    int rev2_inclusive = inclusive;
    char *before = NULL;
    char *after = NULL;
    char *beforefile = NULL;
    char *afterfile = NULL;
    char *outfile = NULL;
	kflag kf = RCS_get_kflags(rcs->expand, 1);

    if (tag1 == NULL && tag2 == NULL)
	return 0;

    /* Assume error status until everything is finished. */
    status = 1;

    /* Make sure both revisions exist. */
    if (tag1 != NULL)
    {
	rev1 = RCS_gettag (rcs, tag1, 1, NULL);
	if (rev1 == NULL || (nodep = findnode (rcs->versions, rev1)) == NULL)
	{
	    error (0, 0, "%s: Revision %s doesn't exist.", fn_root(rcs->path), tag1);
	    goto delrev_done;
	}
    }
    if (tag2 != NULL)
    {
	rev2 = RCS_gettag (rcs, tag2, 1, NULL);
	if (rev2 == NULL || (nodep = findnode (rcs->versions, rev2)) == NULL)
	{
	    error (0, 0, "%s: Revision %s doesn't exist.", fn_root(rcs->path), tag2);
	    goto delrev_done;
	}
    }

    /* If rev1 is on the trunk and rev2 is NULL, rev2 should be
       RCS->HEAD.  (*Not* RCS_head(rcs), which may return rcs->branch
       instead.)  We need to check this special case early, in order
       to make sure that rev1 and rev2 get ordered correctly. */
    if (rev2 == NULL && numdots (rev1) == 1)
    {
	rev2 = xstrdup (rcs->head);
	rev2_inclusive = 1;
    }

    if (rev2 == NULL)
	rev2_inclusive = 1;

    if (rev1 != NULL && rev2 != NULL)
    {
	/* A range consisting of a branch number means the latest revision
	   on that branch. */
	if (RCS_isbranch (rcs, rev1) && STREQ (rev1, rev2))
	    rev1 = rev2 = RCS_getbranch (rcs, rev1, 0);
	else
	{
	    /* Make sure REV1 and REV2 are ordered correctly (in the
	       same order as the next field).  For revisions on the
	       trunk, REV1 should be higher than REV2; for branches,
	       REV1 should be lower.  */
	    /* Shouldn't we just be giving an error in the case where
	       the user specifies the revisions in the wrong order
	       (that is, always swap on the trunk, never swap on a
	       branch, in the non-error cases)?  It is not at all
	       clear to me that users who specify -o 1.4:1.2 really
	       meant to type -o 1.2:1.4, and the out of order usage
	       has never been documented, either by cvs.texinfo or
	       rcs(1).  */
	    char *temp;
	    int temp_inclusive;
	    if (numdots (rev1) == 1)
	    {
		if (compare_revnums (rev1, rev2) <= 0)
		{
		    temp = rev2;
		    rev2 = rev1;
		    rev1 = temp;

		    temp_inclusive = rev2_inclusive;
		    rev2_inclusive = rev1_inclusive;
		    rev1_inclusive = temp_inclusive;
		}
	    }
	    else if (compare_revnums (rev1, rev2) > 0)
	    {
		temp = rev2;
		rev2 = rev1;
		rev1 = temp;

		temp_inclusive = rev2_inclusive;
		rev2_inclusive = rev1_inclusive;
		rev1_inclusive = temp_inclusive;
	    }
	}
    }

    /* Basically the same thing; make sure that the ordering is what we
       need.  */
    if (rev1 == NULL)
    {
	assert (rev2 != NULL);
	if (numdots (rev2) == 1)
	{
	    /* Swap rev1 and rev2.  */
	    int temp_inclusive;

	    rev1 = rev2;
	    rev2 = NULL;

	    temp_inclusive = rev2_inclusive;
	    rev2_inclusive = rev1_inclusive;
	    rev1_inclusive = temp_inclusive;
	}
    }

    /* Put the revision number preceding the first one to delete into
       BEFORE (where "preceding" means according to the next field).
       If the first revision to delete is the first revision on its
       branch (e.g. 1.3.2.1), BEFORE should be the node on the trunk
       at which the branch is rooted.  If the first revision to delete
       is the head revision of the trunk, set BEFORE to NULL.

       Note that because BEFORE may not be on the same branch as REV1,
       it is not very handy for navigating the revision tree.  It's
       most useful just for checking out the revision preceding REV1. */
    before = NULL;
    branchpoint = RCS_getbranchpoint (rcs, rev1 != NULL ? rev1 : rev2);
    if (rev1 == NULL)
    {
	rev1 = xstrdup (branchpoint);
	if (numdots (branchpoint) > 1)
	{
	    char *bp;
	    bp = strrchr (branchpoint, '.');
	    while (*--bp != '.')
		;
	    *bp = '\0';
	    /* Note that this is exclusive, always, because the inclusive
	       flag doesn't affect the meaning when rev1 == NULL.  */
	    before = xstrdup (branchpoint);
	    *bp = '.';
	}
    }
    else if (! STREQ (rev1, branchpoint))
    {
	/* Walk deltas from BRANCHPOINT on, looking for REV1. */
	nodep = findnode (rcs->versions, branchpoint);
	revp = (RCSVers *) nodep->data;
	while (revp->next != NULL && ! STREQ (revp->next, rev1))
	{
	    revp = (RCSVers *) nodep->data;
	    nodep = findnode (rcs->versions, revp->next);
	}
	if (revp->next == NULL)
	{
	    error (0, 0, "%s: Revision %s doesn't exist.", fn_root(rcs->path), rev1);
	    goto delrev_done;
	}
	if (rev1_inclusive)
	    before = xstrdup (revp->version);
	else
	{
	    before = rev1;
	    nodep = findnode (rcs->versions, before);
	    rev1 = xstrdup (((RCSVers *)nodep->data)->next);
	}
    }
    else if (!rev1_inclusive)
    {
	before = rev1;
	nodep = findnode (rcs->versions, before);
	rev1 = xstrdup (((RCSVers *)nodep->data)->next);
    }
    else if (numdots (branchpoint) > 1)
    {
	/* Example: rev1 is "1.3.2.1", branchpoint is "1.3.2.1".
	   Set before to "1.3".  */
	char *bp;
	bp = strrchr (branchpoint, '.');
	while (*--bp != '.')
	    ;
	*bp = '\0';
	before = xstrdup (branchpoint);
	*bp = '.';
    }

    /* If any revision between REV1 and REV2 is locked or is a branch point,
       we can't delete that revision and must abort. */
    after = NULL;
    next = rev1;
    found = 0;
    while (!found && next != NULL)
    {
	nodep = findnode (rcs->versions, next);
	revp = (RCSVers *) nodep->data;

	if (rev2 != NULL)
	    found = STREQ (revp->version, rev2);
	next = revp->next;

	if ((!found && next != NULL) || rev2_inclusive || rev2 == NULL)
	{
	    if (findnode (RCS_getlocks (rcs), revp->version))
	    {
		error (0, 0, "%s: can't remove locked revision %s",
		       fn_root(rcs->path),
		       revp->version);
		goto delrev_done;
	    }
	    if (revp->branches != NULL)
	    {
		error (0, 0, "%s: can't remove branch point %s",
		       fn_root(rcs->path),
		       revp->version);
		goto delrev_done;
	    }

	    /* Doing this only for the :: syntax is for compatibility.
	       See cvs.texinfo for somewhat more discussion.  */
	    if (!inclusive
		&& walklist (RCS_symbols (rcs), findtag, revp->version))
	    {
		/* We don't print which file this happens to on the theory
		   that the caller will print the name of the file in a
		   more useful fashion (fullname not rcs->path).  */
		error (0, 0, "cannot remove revision %s because it has tags",
		       revp->version);
		goto delrev_done;
	    }

	    /* It's misleading to print the `deleting revision' output
	       here, since we may not actually delete these revisions.
	       But that's how RCS does it.  Bleah.  Someday this should be
	       moved to the point where the revs are actually marked for
	       deletion. -twp */
	    cvs_output ("deleting revision ", 0);
	    cvs_output (revp->version, 0);
	    cvs_output ("\n", 1);
	}
    }

    if (rev2 == NULL)
	;
    else if (found)
    {
	if (rev2_inclusive)
	    after = xstrdup (next);
	else
	    after = xstrdup (revp->version);
    }
    else if (!inclusive)
    {
	/* In the case of an empty range, for example 1.2::1.2 or
	   1.2::1.3, we want to just do nothing.  */
	status = 0;
	goto delrev_done;
    }
    else
    {
	/* This looks fishy in the cases where tag1 == NULL or tag2 == NULL.
	   Are those cases really impossible?  */
	assert (tag1 != NULL);
	assert (tag2 != NULL);

	error (0, 0, "%s: invalid revision range %s:%s", fn_root(rcs->path),
	       tag1, tag2);
	goto delrev_done;
    }

    if (after == NULL && before == NULL)
    {
	/* The user is trying to delete all revisions.  While an
	   RCS file without revisions makes sense to RCS (e.g. the
	   state after "rcs -i"), CVS has never been able to cope with
	   it.  So at least for now we just make this an error.

	   We don't include rcs->path in the message since "cvs admin"
	   already printed "RCS file:" and the name.  */
	error (1, 0, "attempt to delete all revisions");
    }

    /* The conditionals at this point get really hairy.  Here is the
       general idea:

       IF before != NULL and after == NULL
         THEN don't check out any revisions, just delete them
       IF before == NULL and after != NULL
         THEN only check out after's revision, and use it for the new deltatext
       ELSE
         check out both revisions and diff -n them.  This could use
	 RCS_exec_rcsdiff with some changes, like being able
	 to suppress diagnostic messages and to direct output. */

    if (after != NULL)
    {
	char *diffbuf;
	size_t bufsize, len;

	checkoutopts = (kf.flags&KFLAG_BINARY)
		? "-kb"
		: "-ko";

	afterfile = cvs_temp_name();
	status = RCS_checkout (rcs, NULL, after, NULL, checkoutopts, afterfile,
			       (RCSCHECKOUTPROC)0, NULL, NULL);
	if (status > 0)
	    goto delrev_done;

	if (before == NULL)
	{
	    /* We are deleting revisions from the head of the tree,
	       so must create a new head. */
	    diffbuf = NULL;
	    bufsize = 0;
	    get_file (afterfile, afterfile, "r", &diffbuf, &bufsize, &len, kf);

	    save_noexec = noexec;
	    noexec = 0;
	    if (unlink_file (afterfile) < 0)
		error (0, errno, "cannot remove %s", afterfile);
	    noexec = save_noexec;

	    xfree (afterfile);
	    afterfile = NULL;

	    xfree (rcs->head);
	    rcs->head = xstrdup (after);
	}
	else
	{
	    beforefile = cvs_temp_name();
	    status = RCS_checkout (rcs, NULL, before, NULL, checkoutopts, beforefile,
				   (RCSCHECKOUTPROC)0, NULL, NULL);
	    if (status > 0)
		goto delrev_done;

	    outfile = cvs_temp_name();
		/* Diff options should include --binary if the RCS file has -kb set
		   in its `expand' field. */
		diffopts = (kf.flags&KFLAG_BINARY)
			? "-a -n --binary"
			: "-a -n";
	    status = diff_exec (beforefile, afterfile, NULL, NULL, diffopts, outfile);

	    if (status == 2)
	    {
		/* Not sure we need this message; will diff_exec already
		   have printed an error?  */
		error (0, 0, "%s: could not diff", fn_root(rcs->path));
		status = 1;
		goto delrev_done;
	    }

	    diffbuf = NULL;
	    bufsize = 0;
	    get_file (outfile, outfile, "r", &diffbuf, &bufsize, &len, kf);
	}

	/* Save the new change text in after's delta node. */
	nodep = findnode (rcs->versions, after);
	revp = (RCSVers *) nodep->data;

	assert (revp->text == NULL);

	revp->text = (Deltatext *) xmalloc (sizeof (Deltatext));
	memset ((Deltatext *) revp->text, 0, sizeof (Deltatext));
	revp->text->version = xstrdup (revp->version);
	revp->text->text = diffbuf;
	revp->text->len = len;

	/* If DIFFBUF is NULL, it means that OUTFILE is empty and that
	   there are no differences between the two revisions.  In that
	   case, we want to force RCS_copydeltas to write an empty string
	   for the new change text (leaving the text field set NULL
	   means "preserve the original change text for this delta," so
	   we don't want that). */
	if (revp->text->text == NULL)
	    revp->text->text = xstrdup ("");
    }

    /* Walk through the revisions (again) to mark each one as
       outdated.  (FIXME: would it be safe to use the `dead' field for
       this?  Doubtful.) */
    for (next = rev1;
	 next != NULL && (after == NULL || ! STREQ (next, after));
	 next = revp->next)
    {
	nodep = findnode (rcs->versions, next);
	revp = (RCSVers *) nodep->data;
	revp->outdated = 1;
    }

    /* Update delta links.  If BEFORE == NULL, we're changing the
       head of the tree and don't need to update any `next' links. */
    if (before != NULL)
    {
	/* If REV1 is the first node on its branch, then BEFORE is its
	   root node (on the trunk) and we have to update its branches
	   list.  Otherwise, BEFORE is on the same branch as AFTER, and
	   we can just change BEFORE's `next' field to point to AFTER.
	   (This should be safe: since findnode manages its lists via
	   the `hashnext' and `hashprev' fields, rather than `next' and
	   `prev', mucking with `next' and `prev' should not corrupt the
	   delta tree's internal structure.  Much. -twp) */

	if (rev1 == NULL)
	    /* beforep's ->next field already should be equal to after,
	       which I think is always NULL in this case.  */
	    ;
	else if (STREQ (rev1, branchpoint))
	{
	    nodep = findnode (rcs->versions, before);
	    revp = (RCSVers *) nodep->data;
	    nodep = revp->branches->list->next;
	    while (nodep != revp->branches->list &&
		   ! STREQ (nodep->key, rev1))
		nodep = nodep->next;
	    assert (nodep != revp->branches->list);
	    if (after == NULL)
		delnode (nodep);
	    else
	    {
		xfree (nodep->key);
		nodep->key = xstrdup (after);
	    }
	}
	else
	{
	    nodep = findnode (rcs->versions, before);
	    beforep = (RCSVers *) nodep->data;
	    xfree (beforep->next);
	    beforep->next = xstrdup (after);
	}
    }

    status = 0;

 delrev_done:
    if (rev1 != NULL)
	xfree (rev1);
    if (rev2 != NULL)
	xfree (rev2);
    if (branchpoint != NULL)
	xfree (branchpoint);
    if (before != NULL)
	xfree (before);
    if (after != NULL)
	xfree (after);

    save_noexec = noexec;
    noexec = 0;
    if (beforefile != NULL)
    {
	if (unlink_file (beforefile) < 0)
	    error (0, errno, "cannot remove %s", beforefile);
	xfree (beforefile);
    }
    if (afterfile != NULL)
    {
	if (unlink_file (afterfile) < 0)
	    error (0, errno, "cannot remove %s", afterfile);
	xfree (afterfile);
    }
    if (outfile != NULL)
    {
	if (unlink_file (outfile) < 0)
	    error (0, errno, "cannot remove %s", outfile);
	xfree (outfile);
    }
    noexec = save_noexec;

    return status;
}

/*
 * TRUE if there exists a symbolic tag "tag" in file.
 */
int
RCS_exist_tag (rcs, tag)
    RCSNode *rcs;
    char *tag;
{

    assert (rcs != NULL);

    if (findnode (RCS_symbols (rcs), tag))
    return 1;
    return 0;

}

/*
 * TRUE if RCS revision number "rev" exists.
 * This includes magic branch revisions, not found in rcs->versions,
 * but only in rcs->symbols, requiring a list walk to find them.
 * Take advantage of list walk callback function already used by
 * RCS_delete_revs, above.
 */
int RCS_exist_rev (RCSNode *rcs, const char *rev)
{

    assert (rcs != NULL);

    if (rcs->flags & PARTIAL)
	RCS_reparsercsfile (rcs);

    if (findnode(rcs->versions, rev) != 0)
	return 1;

    if (walklist (RCS_symbols(rcs), findtag, (void*)rev) != 0)
	return 1;

    return 0;

}


/* RCS_deltas and friends.  Processing of the deltas in RCS files.  */

struct line
{
    /* Text of this line.  Part of the same malloc'd block as the struct
       line itself (we probably should use the "struct hack" (char text[1])
       and save ourselves sizeof (char *) bytes).  Does not include \n;
       instead has_newline indicates the presence or absence of \n.  */
    char *text;
    /* Length of this line, not counting \n if has_newline is true.  */
    size_t len;
    /* Version in which it was introduced.  */
    RCSVers *vers;
    /* Nonzero if this line ends with \n.  This will always be true
       except possibly for the last line.  */
    int has_newline;
    /* Number of pointers to this struct line.  */
    int refcount;
};

struct binbuffer
{
		/* length of binary buffer */
		size_t length;
		size_t reserved;
		/* memory for buffer */
		void *buffer;
		/* number of pointers to this buffer */
		int refcount;
};

struct _linevector_binary
{
  /* nonzero if this contains binary data */
  char is_binary;
  /* binary buffer */
  struct binbuffer *bb;
};

struct _linevector_text
{
  /* nonzero if this contains binary data */
  char is_binary;
  /* How many lines in use for this linevector?  */
  unsigned int nlines;
  /* How many lines allocated for this linevector?  */
  unsigned int lines_alloced;
  /* Pointer to array containing a pointer to each line.  */
  struct line **vector;
};

typedef union _linevector
{
  /* nonzero if this contains binary data */
  char is_binary;
  struct _linevector_binary binary;
  struct _linevector_text text;
} linevector;

static void linevector_init (linevector *vec);
static void linevector_free (linevector *vec);

/* call cvsdelta to create a binary delta */
/* This automatically adds padding required by cvsdelta_diff (which always works to block sizes) */
static int binary_delta(char **buf1, size_t buf1_len, char **buf2, size_t buf2_len, char **delta, size_t *length)
{
	if(buf1_len%CVSDELTA_BLOCKPAD)
	{
		*buf1=xrealloc(*buf1,buf1_len+CVSDELTA_BLOCKPAD);
		memset((*buf1)+buf1_len,0,CVSDELTA_BLOCKPAD);
	}
	if(buf2_len%CVSDELTA_BLOCKPAD)
	{
		*buf2=xrealloc(*buf2,buf2_len+CVSDELTA_BLOCKPAD);
		memset((*buf2)+buf2_len,0,CVSDELTA_BLOCKPAD);
	}
	return cvsdelta_diff(*buf1, buf1_len, *buf2, buf2_len, (void**)delta, length);
}

/* Apply binary changes.  Call cvsdelta with the binary data + the change text, and
   updated the linevector with the new data */
static int apply_binary_changes (linevector *lines, linevector *tmpbuf, const char *diffbuf, size_t difflen)
{
	linevector t;

	assert(tmpbuf->is_binary);

	if(!lines->is_binary)
	{
		linevector lv = {0};
		int n = 0, ln;
		char *p;

		for (ln = 0; ln < lines->text.nlines; ++ln)
		{
			n += lines->text.vector[ln]->len;
			if (lines->text.vector[ln]->has_newline)
				n++;
		}

		lv.is_binary = 1;
		lv.binary.bb=(struct binbuffer*)xmalloc(sizeof(struct binbuffer));
		memset(lv.binary.bb,0,sizeof(struct binbuffer));
		p = lv.binary.bb->buffer = xmalloc (n);
		lv.binary.bb->refcount=1;
		lv.binary.bb->length=n;

		for (ln = 0; ln < lines->text.nlines; ++ln)
		{
			memcpy (p, lines->text.vector[ln]->text,
				lines->text.vector[ln]->len);
			p += lines->text.vector[ln]->len;
			if (lines->text.vector[ln]->has_newline)
			*p++ = '\n';
		}
		assert ((p-((char*)lv.binary.bb->buffer))==n);
		linevector_free(lines);
		memcpy(lines,&lv,sizeof(linevector));
	}

	if(cvsdelta_patch(lines->binary.bb->buffer,lines->binary.bb->length,diffbuf,difflen,&tmpbuf->binary.bb->buffer,&tmpbuf->binary.bb->length,&tmpbuf->binary.bb->reserved))
		error(1,0,"Binary patch failed");

	t = *lines;
	*lines = *tmpbuf;
	*tmpbuf = t;
	return 1;
}

/* Initialize *VEC to be a linevector with no lines.  */
static void linevector_init (linevector *vec)
{
	memset(vec,0,sizeof(*vec));
}

/* Given some text TEXT, add each of its lines to VEC before line POS
   (where line 0 is the first line).  The last line in TEXT may or may
   not be \n terminated.
   Set the version for each of the new lines to VERS.  This
   function returns non-zero for success.  It returns zero if the line
   number is out of range.

   Each of the lines in TEXT are copied to space which is managed with
   the linevector (and freed by linevector_free).  So the caller doesn't
   need to keep TEXT around after the call to this function.  */
static int linevector_add(linevector *vec, const char *text,
				  size_t len, RCSVers *vers,
				  unsigned int pos, char is_binary)
{
    const char *textend;
    unsigned int i;
    unsigned int nnew;
    const char *p;
    const char *nextline_text;
    size_t nextline_len;
    int nextline_newline;
    struct line *q;

    if (len == 0)
	return 1;

	if(is_binary)
	{
		assert(!pos);
		if(vec->text.vector || vec->text.lines_alloced)
		{
			error(1,0,"Attempting binary operation on text linevector");
		}

		vec->is_binary=1;
		vec->binary.bb=(struct binbuffer *)xmalloc(sizeof(struct binbuffer));
		vec->binary.bb->buffer = xmalloc(len);
		vec->binary.bb->length = len;
		vec->binary.bb->reserved = len;
		vec->binary.bb->refcount=1;
		memcpy(vec->binary.bb->buffer,text,len);
		return 1;
	}

	if(vec->is_binary)
	{
		error(1,0,"Attempting text operation on binary linevector");
	}

    textend = text + len;

    /* Count the number of lines we will need to add.  */
    nnew = 1;
    for (p = text; p && (p < textend); p=memchr(p+1, '\n', textend-(p+1)))
	 	if (*p == '\n' && p + 1 < textend)
 		    ++nnew;

    /* Expand VEC->VECTOR if needed.  */
    if (vec->text.nlines + nnew >= vec->text.lines_alloced)
    {
	if (vec->text.lines_alloced == 0)
	    vec->text.lines_alloced = 10;
	while (vec->text.nlines + nnew >= vec->text.lines_alloced)
	    vec->text.lines_alloced *= 3;
	vec->text.vector = xrealloc (vec->text.vector,
				vec->text.lines_alloced * sizeof (*vec->text.vector));
    }

    /* Make room for the new lines in vec->text.VECTOR.  */
   if(vec->text.nlines && pos < vec->text.nlines)
     memmove(vec->text.vector + pos + nnew, vec->text.vector + pos, sizeof(vec->text.vector[0]) * (vec->text.nlines - pos));
/*    for (i = vec->text.nlines + nnew - 1; i >= pos + nnew; --i)
	vec->text.vector[i] = vec->text.vector[i - nnew];
*/
    if (pos > vec->text.nlines)
    {
	error(0,0,"Dropping data: pos>vec->text.nlines");
	return 0;
    }

    /* Actually add the lines, to vec->text.VECTOR.  */
    i = pos;
    nextline_text = text;
    nextline_newline = 0;
    for (p = text; p < textend; ++p)
	{
	if (*p == '\n')
	{
	    nextline_newline = 1;
	    if (p + 1 == textend)
		/* If there are no characters beyond the last newline, we
		   don't consider it another line.  */
		break;
	    nextline_len = p - nextline_text;
	    q = (struct line *) xmalloc (sizeof (struct line) + nextline_len);
	    q->vers = vers;
	    q->text = (char *)q + sizeof (struct line);
	    q->len = nextline_len;
	    q->has_newline = nextline_newline;
	    q->refcount = 1;
	    memcpy (q->text, nextline_text, nextline_len);
	    vec->text.vector[i++] = q;

	    nextline_text = (char *)p + 1;
	    nextline_newline = 0;
	}
	}
    nextline_len = p - nextline_text;
    q = (struct line *) xmalloc (sizeof (struct line) + nextline_len);
    q->vers = vers;
    q->text = (char *)q + sizeof (struct line);
    q->len = nextline_len;
    q->has_newline = nextline_newline;
    q->refcount = 1;
    memcpy (q->text, nextline_text, nextline_len);
    vec->text.vector[i] = q;

    vec->text.nlines += nnew;

    return 1;
}

static void linevector_delete PROTO ((linevector *, unsigned int,
				      unsigned int));

/* Remove NLINES lines from VEC at position POS (where line 0 is the
   first line).  */
static void linevector_delete (linevector *vec, unsigned int pos, unsigned int nlines)
{
    register unsigned int i;
    unsigned int last;

    last = vec->text.nlines - nlines;
    for (i = pos; i < pos + nlines; ++i)
    {
	if (--vec->text.vector[i]->refcount == 0)
	    xfree (vec->text.vector[i]);
    }
    if(nlines && (i < vec->text.nlines))
       memmove(vec->text.vector + pos, vec->text.vector + pos + nlines, sizeof(vec->text.vector[0]) * (vec->text.nlines - i));
    vec->text.nlines -= nlines;
}


/* Copy FROM to TO, copying the vectors but not the lines pointed to.  */
static void linevector_copy(linevector *to, linevector *from)
{
    unsigned int ln;

    if(to->is_binary && !from->is_binary)
		linevector_free(to);
    if(from->is_binary)
    {
		from->binary.bb->refcount++;
		linevector_free(to);
		to->is_binary=1;
		to->binary.bb=from->binary.bb;
    }
    else
    {
		to->is_binary=0;
      for (ln = 0; ln < to->text.nlines; ++ln)
      {
	if (--to->text.vector[ln]->refcount == 0)
	    xfree (to->text.vector[ln]);
      }
      if (from->text.nlines > to->text.lines_alloced)
      {
	if (to->text.lines_alloced == 0)
	    to->text.lines_alloced = 10;
	while (from->text.nlines > to->text.lines_alloced)
	    to->text.lines_alloced *= 2;
	to->text.vector = (struct line **)
	    xrealloc (to->text.vector, to->text.lines_alloced * sizeof (*to->text.vector));
      }
      memcpy (to->text.vector, from->text.vector,
	    from->text.nlines * sizeof (*to->text.vector));
      to->text.nlines = from->text.nlines;
      for (ln = 0; ln < to->text.nlines; ++ln)
	++to->text.vector[ln]->refcount;
    }
}

/* Free storage associated with linevector.  */
static void linevector_free (linevector *vec)
{
    unsigned int ln;

	if(vec->is_binary)
	{
		if(vec->binary.bb && !vec->binary.bb->refcount--)
		{
			xfree(vec->binary.bb->buffer);
			xfree(vec->binary.bb);
		}
		else
			vec->binary.bb=NULL;
	}
	else
	{
		if (vec->text.vector != NULL)
		{
			for (ln = 0; ln < vec->text.nlines; ++ln)
				if (--vec->text.vector[ln]->refcount == 0)
				xfree (vec->text.vector[ln]);

			xfree (vec->text.vector);
		}
	}
}

static char *month_printname PROTO ((char *));

/* Given a textual string giving the month (1-12), terminated with any
   character not recognized by atoi, return the 3 character name to
   print it with.  I do not think it is a good idea to change these
   strings based on the locale; they are standard abbreviations (for
   example in rfc822 mail messages) which should be widely understood.
   Returns a pointer into static readonly storage.  */
static char *
month_printname (month)
    char *month;
{
    static const char *const months[] =
      {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
	 "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
    int mnum;

    mnum = atoi (month);
    if (mnum < 1 || mnum > 12)
	return "???";
    return (char *)months[mnum - 1];
}

/* Apply changes to the line vector LINES.  DIFFBUF is a buffer of
   length DIFFLEN holding the change text from an RCS file (the output
   of diff -n).  NAME is used in error messages.  The VERS field of
   any line added is set to ADDVERS.  The VERS field of any line
   deleted is set to DELVERS, unless DELVERS is NULL, in which case
   the VERS field of deleted lines is unchanged.  The function returns
   non-zero if the change text is applied successfully.  It returns
   zero if the change text does not appear to apply to LINES (e.g., a
   line number is invalid).  If the change text is improperly
   formatted (e.g., it is not the output of diff -n), the function
   calls error with a status of 1, causing the program to exit.  */

static int apply_rcs_changes (linevector *lines, const char *diffbuf,
     size_t difflen, const char *name, RCSVers *addvers, RCSVers *delvers)
{
    const char *p;
    const char *q;
    int op;
    /* The RCS format throws us for a loop in that the deltafrags (if
       we define a deltafrag as an add or a delete) need to be applied
       in reverse order.  So we stick them into a linked list.  */
    struct deltafrag {
	enum {FRAG_ADD, FRAG_DELETE} type;
	unsigned long pos;
	unsigned long nlines;
	const char *new_lines;
	size_t len;
	struct deltafrag *next;
    };
    struct deltafrag *dfhead;
    struct deltafrag *df;

	/* At the boundary between text and binary delta.  Hopefully we don't do this too often.. */
	if(lines->is_binary)
	{
		linevector lv = {0};
		linevector_add(&lv,lines->binary.bb->buffer,lines->binary.bb->length,delvers,0,0);
		linevector_free(lines);
		memcpy(lines,&lv,sizeof(lv));
	}

    dfhead = NULL;
    for (p = diffbuf; p != NULL && p < diffbuf + difflen; )
    {
	op = *p++;
	if (op != 'a' && op != 'd')
	    /* Can't just skip over the deltafrag, because the value
	       of op determines the syntax.  */
	    error (1, 0, "unrecognized operation '\\x%x' in %s",
		   op, name);
	df = (struct deltafrag *) xmalloc (sizeof (struct deltafrag));
	df->next = dfhead;
	dfhead = df;
	df->pos = strtoul (p, (char **) &q, 10);

	if (p == q)
	    error (1, 0, "number expected in %s", name);
	p = q;
	if (*p++ != ' ')
	    error (1, 0, "space expected in %s", name);
	df->nlines = strtoul (p, (char **) &q, 10);
	if (p == q)
	    error (1, 0, "number expected in %s", name);
	p = q;
	if (*p++ != '\012')
	    error (1, 0, "linefeed expected in %s (got %d)", name, (int)*(p-1));

	if (op == 'a')
	{
	    unsigned int i;

	    df->type = FRAG_ADD;
	    i = df->nlines;
	    /* The text we want is the number of lines specified, or
	       until the end of the value, whichever comes first (it
	       will be the former except in the case where we are
	       adding a line which does not end in newline).  */
	    for (q = p; i != 0; ++q)
		if (*q == '\n')
		    --i;
		else if (q == diffbuf + difflen)
		{
		    if (i != 1)
			error (1, 0, "premature end of change in %s", name);
		    else
			break;
		}

	    /* Stash away a pointer to the text we are adding.  */
	    df->new_lines = p;
	    df->len = q - p;

	    p = q;
	}
	else
	{
	    /* Correct for the fact that line numbers in RCS files
	       start with 1.  */
	    --df->pos;

	    assert (op == 'd');
	    df->type = FRAG_DELETE;
	}
    }

    for (df = dfhead; df != NULL;)
    {
	unsigned int ln;

	switch (df->type)
	{
	case FRAG_ADD:
	    if (! linevector_add (lines, df->new_lines, df->len, addvers,
				  df->pos, 0))
		return 0;
	    break;
	case FRAG_DELETE:
	    if (df->pos > lines->text.nlines
		|| df->pos + df->nlines > lines->text.nlines)
		return 0;
	    if (delvers != NULL)
		for (ln = df->pos; ln < df->pos + df->nlines; ++ln)
		    lines->text.vector[ln]->vers = delvers;
	    linevector_delete (lines, df->pos, df->nlines);
	    break;
	}
	df = df->next;
	xfree (dfhead);
	dfhead = df;
    }

    return 1;
}

/* Apply an RCS change text to a buffer.  The function name starts
   with rcs rather than RCS because this does not take an RCSNode
   argument.  NAME is used in error messages.  TEXTBUF is the text
   buffer to change, and TEXTLEN is the size.  DIFFBUF and DIFFLEN are
   the change buffer and size.  The new buffer is returned in *RETBUF
   and *RETLEN.  The new buffer is allocated by xmalloc.

   Return 1 for success.  On failure, call error and return 0.  */

int
rcs_change_text (name, textbuf, textlen, diffbuf, difflen, retbuf, retlen)
     const char *name;
     char *textbuf;
     size_t textlen;
     const char *diffbuf;
     size_t difflen;
     char **retbuf;
     size_t *retlen;
{
    linevector lines;
    int ret;

    *retbuf = NULL;
    *retlen = 0;

    linevector_init (&lines);

    if (! linevector_add (&lines, textbuf, textlen, NULL, 0, 0))
	error (1, 0, "cannot initialize line vector");

    if (! apply_rcs_changes (&lines, diffbuf, difflen, name, NULL, NULL))
    {
	error (0, 0, "invalid change text in %s", name);
	ret = 0;
    }
    else
    {
	char *p;
	size_t n;
	unsigned int ln;

	n = 0;
	for (ln = 0; ln < lines.text.nlines; ++ln)
	    /* 1 for \n */
	    n += lines.text.vector[ln]->len + 1;

	p = xmalloc (n);
	*retbuf = p;

	for (ln = 0; ln < lines.text.nlines; ++ln)
	{
	    memcpy (p, lines.text.vector[ln]->text, lines.text.vector[ln]->len);
	    p += lines.text.vector[ln]->len;
	    if (lines.text.vector[ln]->has_newline)
		*p++ = '\n';
	}

	*retlen = p - *retbuf;
	assert (*retlen <= n);

	ret = 1;
    }

    linevector_free (&lines);

    return ret;
}

/* Walk the deltas in RCS to get to revision VERSION.

   If OP is RCS_ANNOTATE, then write annotations using cvs_output.

   If OP is RCS_FETCH, then put the contents of VERSION into a
   newly-malloc'd array and put a pointer to it in *TEXT.  Each line
   is \n terminated; the caller is responsible for converting text
   files if desired.  The total length is put in *LEN.

   If FP is non-NULL, it should be a file descriptor open to the file
   RCS with file position pointing to the deltas.  We close the file
   when we are done.

   If LOG is non-NULL, then *LOG is set to the log message of VERSION,
   and *LOGLEN is set to the length of the log message.

   On error, give a fatal error.  */

void
RCS_deltas (rcs, fp, rcsbuf, version, op, text, len, log, loglen)
    RCSNode *rcs;
    FILE *fp;
    struct rcsbuffer *rcsbuf;
    char *version;
    enum rcs_delta_op op;
    char **text;
    size_t *len;
    char **log;
    size_t *loglen;
{
    char *branchversion;
    char *cpversion;
    char *key;
    char *value;
    size_t vallen;
    RCSVers *vers;
    RCSVers *prev_vers;
    RCSVers *trunk_vers;
    char *next;
    int ishead, isnext, isversion, onbranch;
    Node *node;
    linevector headlines;
    linevector curlines;
    linevector trunklines;
	linevector binbuf;
    int foundhead;
	const char *deltatype;
	char *zbuf=NULL;
	size_t zbuflen=0;

    if (fp == NULL)
		rcsbuf = &rcs->rcsbuf;

	rcsbuf_seek(rcsbuf, rcs->delta_pos);

    ishead = 1;
    vers = NULL;
    prev_vers = NULL;
    trunk_vers = NULL;
    next = NULL;
    onbranch = 0;
    foundhead = 0;

    linevector_init (&curlines);
    linevector_init (&headlines);
    linevector_init (&trunklines);
	linevector_init (&binbuf);
	binbuf.is_binary=1;

    /* We set BRANCHVERSION to the version we are currently looking
       for.  Initially, this is the version on the trunk from which
       VERSION branches off.  If VERSION is not a branch, then
       BRANCHVERSION is just VERSION.  */
    branchversion = xstrdup (version);
    cpversion = strchr (branchversion, '.');
    if (cpversion != NULL)
        cpversion = strchr (cpversion + 1, '.');
    if (cpversion != NULL)
        *cpversion = '\0';

    do {
	if (! rcsbuf_getrevnum (rcsbuf, &key))
	    error (1, 0, "unexpected EOF reading RCS file %s", fn_root(rcs->path));

	if (next != NULL && ! STREQ (next, key))
	{
	    /* This is not the next version we need.  It is a branch
               version which we want to ignore.  */
	    isnext = 0;
	    isversion = 0;
	}
	else
	{
	    isnext = 1;

	    /* look up the revision */
	    node = findnode (rcs->versions, key);
	    if (node == NULL)
	        error (1, 0,
		       "mismatch in rcs file %s between deltas and deltatexts",
		       fn_root(rcs->path));

	    /* Stash the previous version.  */
	    prev_vers = vers;

	    vers = (RCSVers *) node->data;
	    next = vers->next;
		deltatype = vers->type?vers->type:"text";

		/* Hack */
		if(STREQ(deltatype,"(null)"))
			deltatype="text";

	    /* Compare key and trunkversion now, because key points to
	       storage controlled by rcsbuf_getkey.  */
	    if (STREQ (branchversion, key))
	        isversion = 1;
	    else
	        isversion = 0;
	}

	while (1)
	{
	    if (! rcsbuf_getkey (rcsbuf, &key, &value))
		error (1, 0, "%s does not appear to be a valid rcs file",
		       fn_root(rcs->path));

	    if (log != NULL
		&& isversion
		&& STREQ (key, "log")
		&& STREQ (branchversion, version))
	    {
			*log = rcsbuf_valcopy (rcsbuf, value, 0, loglen);
	    }

	    if (STREQ (key, "text"))
	    {
			rcsbuf_valpolish (rcsbuf, value, 0, &vallen);
			if(STREQ(deltatype,"compressed_text") || STREQ(deltatype,"compressed_binary"))
			{
				uLong zlen;

				z_stream stream = {0};
				inflateInit(&stream);
				zlen = ntohl(*(unsigned long *)value);
				if(zlen)
				{
					stream.avail_in = vallen-4;
					stream.next_in = value+4;
					stream.avail_out = zlen;
					if(zlen>zbuflen)
					{
						zbuf=xrealloc(zbuf,zlen);
						zbuflen=zlen;
					}
					stream.next_out = zbuf;
					if(inflate(&stream, Z_FINISH)!=Z_STREAM_END)
					{
						error(1,0,"internal error: inflate failed");
					}
				}
				vallen = zlen;
				value = zbuf;
				inflateEnd(&stream);
			}
			if (ishead)
			{
				if(STREQ(deltatype,"text") || STREQ(deltatype,"compressed_text"))
				{
					if (! linevector_add (&curlines, value, vallen, NULL, 0, 0))
						error (1, 0, "invalid rcs file %s", fn_root(rcs->path));
				}
				else if(STREQ(deltatype,"binary") || STREQ(deltatype,"compressed_binary"))
				{
					if (!linevector_add(&curlines, value, vallen, NULL, 0, 1))
						error (1, 0, "invalid rcs file %s", fn_root(rcs->path));
				}
				else
					error(1,0,"invalid rcs delta type %s in %s",deltatype,fn_root(rcs->path));

				ishead = 0;
			}
			else if (isnext)
			{
				if(STREQ(deltatype,"text") || STREQ(deltatype,"compressed_text"))
				{
					if (! apply_rcs_changes (&curlines, value, vallen,
								rcs->path,
								onbranch ? vers : NULL,
								onbranch ? NULL : prev_vers))
					{
						error (1, 0, "invalid change text in %s", fn_root(rcs->path));
					}
				}
				else if(STREQ(deltatype,"binary") || STREQ(deltatype,"compressed_binary"))
				{
					/* If we've been copied, disconnect the copy */
					if(!binbuf.binary.bb || binbuf.binary.bb->refcount>1)
					{
						if(binbuf.binary.bb)
							binbuf.binary.bb->refcount--;
						binbuf.binary.bb=(struct binbuffer*)xmalloc(sizeof(struct binbuffer));
						memset(binbuf.binary.bb,0,sizeof(struct binbuffer));
						binbuf.binary.bb->refcount=1;
					}
					if (! apply_binary_changes(&curlines, &binbuf, value, vallen))
						error(1,0,"invalid binary delta text in %s", fn_root(rcs->path));
				}
				else
					error(1,0,"invalid rcs delta type %s in %s",deltatype,fn_root(rcs->path));
			}
			break;
	    }
	}

	if (isversion)
	{
	    /* This is either the version we want, or it is the
               branchpoint to the version we want.  */
	    if (STREQ (branchversion, version))
	    {
	        /* This is the version we want.  */
		linevector_copy (&headlines, &curlines);
		foundhead = 1;
		if (onbranch)
		{
		    /* We have found this version by tracking up a
                       branch.  Restore back to the lines we saved
                       when we left the trunk, and continue tracking
                       down the trunk.  */
		    onbranch = 0;
		    vers = trunk_vers;
		    next = vers->next;
		    linevector_copy (&curlines, &trunklines);
		}
	    }
	    else
	    {
	        Node *p;

	        /* We need to look up the branch.  */
	        onbranch = 1;

		if (numdots (branchversion) < 2)
		{
		    unsigned int ln;

		    /* We are leaving the trunk; save the current
                       lines so that we can restore them when we
                       continue tracking down the trunk.  */
		    trunk_vers = vers;
		    linevector_copy (&trunklines, &curlines);

		    if(!curlines.is_binary)
		    {
		      /* Reset the version information we have
                         accumulated so far.  It only applies to the
                         changes from the head to this version.  */
		      for (ln = 0; ln < curlines.text.nlines; ++ln)
		        curlines.text.vector[ln]->vers = NULL;
		    }
		}

		/* The next version we want is the entry on
                   VERS->branches which matches this branch.  For
                   example, suppose VERSION is 1.21.4.3 and
                   BRANCHVERSION was 1.21.  Then we look for an entry
                   starting with "1.21.4" and we'll put it (probably
                   1.21.4.1) in NEXT.  We'll advance BRANCHVERSION by
                   two dots (in this example, to 1.21.4.3).  */

		if (vers->branches == NULL)
		    error (1, 0, "missing expected branches in %s",
			   fn_root(rcs->path));
		*cpversion = '.';
		++cpversion;
		cpversion = strchr (cpversion, '.');
		if (cpversion == NULL)
		    error (1, 0, "version number confusion in %s",
			   fn_root(rcs->path));
		for (p = vers->branches->list->next;
		     p != vers->branches->list;
		     p = p->next)
		    if (strncmp (p->key, branchversion,
				 cpversion - branchversion) == 0)
			break;
		if (p == vers->branches->list)
		    error (1, 0, "missing expected branch in %s",
			   fn_root(rcs->path));

		next = p->key;

		cpversion = strchr (cpversion + 1, '.');
		if (cpversion != NULL)
		    *cpversion = '\0';
	    }
	}
	if (op == RCS_FETCH && foundhead)
	    break;
    } while (next != NULL);

    xfree (branchversion);

    if (! foundhead)
	{
        error (1, 0, "could not find desired version %s in %s",
	       version, fn_root(rcs->path));
	}

    /* Now print out or return the data we have just computed.  */
    switch (op)
    {
	case RCS_ANNOTATE:
	    {
		unsigned int ln;

		if(headlines.is_binary)
			error(1,0,"Cannot annotate a binary delta");

		for (ln = 0; ln < headlines.text.nlines; ++ln)
		{
		    char buf[80];
		    /* Period which separates year from month in date.  */
		    char *ym;
		    /* Period which separates month from day in date.  */
		    char *md;
		    RCSVers *prvers;

		    prvers = headlines.text.vector[ln]->vers;
		    if (prvers == NULL)
			prvers = vers;

		    sprintf (buf, "%-12s (%-8.8s ",
			     prvers->version,
			     prvers->author);
		    cvs_output (buf, 0);

		    /* Now output the date.  */

			ym = strchr (prvers->date, '.');
		    if (ym == NULL)
		    {
			/* ??- is an ANSI trigraph.  The ANSI way to
			   avoid it is \? but some pre ANSI compilers
			   complain about the unrecognized escape
			   sequence.  Of course string concatenation
			   ("??" "-???") is also an ANSI-ism.  Testing
			   __STDC__ seems to be a can of worms, since
			   compilers do all kinds of things with it.  */
			cvs_output ("??", 0);
			cvs_output ("-???", 0);
			cvs_output ("-??", 0);
		    }
		    else
		    {
			md = strchr (ym + 1, '.');
			if (md == NULL)
			    cvs_output ("??", 0);
			else
			    cvs_output (md + 1, 2);

			cvs_output ("-", 1);
			cvs_output (month_printname (ym + 1), 0);
			cvs_output ("-", 1);
			/* Only output the last two digits of the year.  Our output
			   lines are long enough as it is without printing the
			   century.  */
			cvs_output (ym - 2, 2);
		    }
		    cvs_output ("): ", 0);
		    if (headlines.text.vector[ln]->len != 0)
			cvs_output (headlines.text.vector[ln]->text,
				    headlines.text.vector[ln]->len);
		    cvs_output ("\n", 1);
		}
	    }
	    break;
	case RCS_FETCH:
	    {
		char *p;
		size_t n;
		unsigned int ln;

		assert (text != NULL);
		assert (len != NULL);

		if(headlines.is_binary)
		{
			p = xmalloc(headlines.binary.bb->length);
			*text = p;
			*len = headlines.binary.bb->length;
			memcpy(p, headlines.binary.bb->buffer,headlines.binary.bb->length);
		}
		else
		{
			n = 0;
			for (ln = 0; ln < headlines.text.nlines; ++ln)
				/* 1 for \n */
				n += headlines.text.vector[ln]->len + 1;
			p = xmalloc (n);
			*text = p;
			for (ln = 0; ln < headlines.text.nlines; ++ln)
			{
				memcpy (p, headlines.text.vector[ln]->text,
					headlines.text.vector[ln]->len);
				p += headlines.text.vector[ln]->len;
				if (headlines.text.vector[ln]->has_newline)
				*p++ = '\n';
			}
			*len = p - *text;
			assert (*len <= n);
		}
		}
	    break;
    }

    linevector_free (&curlines);
    linevector_free (&headlines);
    linevector_free (&trunklines);
	linevector_free (&binbuf);
	xfree(zbuf);

    return;
}

/* Read the information for a single delta from the RCS buffer RCSBUF,
   whose name is RCSFILE.  *KEYP and *VALP are either NULL, or the
   first key/value pair to read, as set by rcsbuf_getkey. Return NULL
   if there are no more deltas.  Store the key/value pair which
   terminated the read in *KEYP and *VALP.  */

static RCSVers *
getdelta (rcsbuf, rcsfile, keyp, valp)
    struct rcsbuffer *rcsbuf;
    char *rcsfile;
    char **keyp;
    char **valp;
{
    RCSVers *vnode;
    char *key, *value, *cp;
    Node *kv;

    /* Get revision number if it wasn't passed in. This uses
       rcsbuf_getkey because it doesn't croak when encountering
       unexpected input.  As a result, we have to play unholy games
       with `key' and `value'. */
    if (*keyp != NULL)
    {
	key = *keyp;
	value = *valp;
    }
    else
    {
	if (! rcsbuf_getkey (rcsbuf, &key, &value))
	    error (1, 0, "%s: unexpected EOF", rcsfile);
    }

    /* Make sure that it is a revision number and not a cabbage
       or something. */
    for (cp = key;
	 (isdigit ((unsigned char) *cp) || *cp == '.') && *cp != '\0';
	 cp++)
	/* do nothing */ ;
    /* Note that when comparing with RCSDATE, we are not massaging
       VALUE from the string found in the RCS file.  This is OK since
       we know exactly what to expect.  */
    if (*cp != '\0' || strncmp (RCSDATE, value, (sizeof RCSDATE) - 1) != 0)
    {
	*keyp = key;
	*valp = value;
	return NULL;
    }

    vnode = (RCSVers *) xmalloc (sizeof (RCSVers));
    memset (vnode, 0, sizeof (RCSVers));

    vnode->version = xstrdup (key);

    /* Grab the value of the date from value.  Note that we are not
       massaging VALUE from the string found in the RCS file.  */
    cp = value + (sizeof RCSDATE) - 1;	/* skip the "date" keyword */
    while (whitespace (*cp))		/* take space off front of value */
	cp++;

    vnode->date = xstrdup (cp);

    /* Get author field.  */
    if (! rcsbuf_getkey (rcsbuf, &key, &value))
    {
	error (1, 0, "unexpected end of file reading %s", rcsfile);
    }
    if (! STREQ (key, "author"))
	error (1, 0, "\
unable to parse %s; `author' not in the expected place", rcsfile);
    vnode->author = rcsbuf_valcopy (rcsbuf, value, 0, (size_t *) NULL);

    /* Get state field.  */
    if (! rcsbuf_getkey (rcsbuf, &key, &value))
    {
	error (1, 0, "unexpected end of file reading %s", rcsfile);
    }
    if (! STREQ (key, "state"))
	error (1, 0, "\
unable to parse %s; `state' not in the expected place", rcsfile);
    vnode->state = rcsbuf_valcopy (rcsbuf, value, 0, (size_t *) NULL);
    /* The value is optional, according to rcsfile(5).  */
    if (value != NULL && STREQ (value, "dead"))
    {
	vnode->dead = 1;
    }

    /* Note that "branches" and "next" are in fact mandatory, according
       to doc/RCSFILES.  */

    /* fill in the branch list (if any branches exist) */
    if (! rcsbuf_getkey (rcsbuf, &key, &value))
    {
	error (1, 0, "unexpected end of file reading %s", rcsfile);
    }
    if (STREQ (key, RCSDESC))
    {
	*keyp = key;
	*valp = value;
	/* Probably could/should be a fatal error.  */
	error (0, 0, "warning: 'branches' keyword missing from %s", rcsfile);
	return vnode;
    }
    if (value != (char *) NULL)
    {
	vnode->branches = getlist ();
	/* Note that we are not massaging VALUE from the string found
           in the RCS file.  */
	do_branches (vnode->branches, value);
    }

    /* fill in the next field if there is a next revision */
    if (! rcsbuf_getkey (rcsbuf, &key, &value))
    {
	error (1, 0, "unexpected end of file reading %s", rcsfile);
    }
    if (STREQ (key, RCSDESC))
    {
		*keyp = key;
		*valp = value;
		/* Probably could/should be a fatal error.  */
		error (0, 0, "warning: 'next' keyword missing from %s", rcsfile);
		return vnode;
    }
    if (value != (char *) NULL)
		vnode->next = rcsbuf_valcopy (rcsbuf, value, 0, (size_t *) NULL);

    /*
     * XXX - this is where we put the symbolic link stuff???
     * (into newphrases in the deltas).
     */
    while (1)
    {
	if (! rcsbuf_getkey (rcsbuf, &key, &value))
	    error (1, 0, "unexpected end of file reading %s", rcsfile);

	/* The `desc' keyword is the end of the deltas. */
	if (strcmp (key, RCSDESC) == 0)
	    break;

	/* Enable use of repositories created by certain obsolete
	   versions of CVS.  This code should remain indefinately;
	   there is no procedure for converting old repositories, and
	   checking for it is harmless.  */
	if (STREQ (key, RCSDEAD))
	{
	    vnode->dead = 1;
	    if (vnode->state != NULL)
		xfree (vnode->state);
	    vnode->state = xstrdup ("dead");
	    continue;
	}

	if (STREQ (key, "deltatype"))
	{
		xfree(vnode->type);
		vnode->type = rcsbuf_valcopy (rcsbuf, value, 1,NULL);
		continue;
	}

	/* if we have a new revision number, we're done with this delta */
	for (cp = key;
	     (isdigit ((unsigned char) *cp) || *cp == '.') && *cp != '\0';
	     cp++)
	    /* do nothing */ ;
	/* Note that when comparing with RCSDATE, we are not massaging
	   VALUE from the string found in the RCS file.  This is OK
	   since we know exactly what to expect.  */
	if (*cp == '\0' && strncmp (RCSDATE, value, strlen (RCSDATE)) == 0)
	    break;

	/* At this point, key and value represent a user-defined field
	   in the delta node. */
	if (vnode->other_delta == NULL)
	    vnode->other_delta = getlist ();
	kv = getnode ();
	kv->type = rcsbuf_valcmp (rcsbuf) ? RCSCMPFLD : RCSFIELD;
	kv->key = xstrdup (key);
	kv->data = rcsbuf_valcopy (rcsbuf, value, kv->type == RCSFIELD,
				   (size_t *) NULL);
	if (addnode (vnode->other_delta, kv) != 0)
	{
	    /* Complaining about duplicate keys in newphrases seems
	       questionable, in that we don't know what they mean and
	       doc/RCSFILES has no prohibition on several newphrases
	       with the same key.  But we can't store more than one as
	       long as we store them in a List *.  */
	    error (0, 0, "warning: duplicate key `%s' in RCS file `%s'",
		   key, rcsfile);
	    freenode (kv);
	}
    }

    /* Return the key which caused us to fail back to the caller.  */
    *keyp = key;
    *valp = value;

    return vnode;
}

static void
freedeltatext (d)
    Deltatext *d;
{
    if (d->version != NULL)
	xfree (d->version);
    if (d->log != NULL)
	xfree (d->log);
    if (d->text != NULL)
	xfree (d->text);
    if (d->other != (List *) NULL)
	dellist (&d->other);
    xfree (d);
}

static Deltatext *
RCS_getdeltatext (rcs, fp, rcsbuf)
    RCSNode *rcs;
    FILE *fp;
    struct rcsbuffer *rcsbuf;
{
    char *num;
    char *key, *value;
    Node *p;
    Deltatext *d;

    /* Get the revision number. */
    if (! rcsbuf_getrevnum (rcsbuf, &num))
    {
	/* If num == NULL, it means we reached EOF naturally.  That's
	   fine. */
	if (num == NULL)
	    return NULL;
	else
	    error (1, 0, "%s: unexpected EOF", fn_root(rcs->path));
    }

    p = findnode (rcs->versions, num);
    if (p == NULL)
	error (1, 0, "mismatch in rcs file %s between deltas and deltatexts",
	       fn_root(rcs->path));

    d = (Deltatext *) xmalloc (sizeof (Deltatext));
    d->version = xstrdup (num);

    /* Get the log message. */
    if (! rcsbuf_getkey (rcsbuf, &key, &value))
	error (1, 0, "%s, delta %s: unexpected EOF", fn_root(rcs->path), num);
    if (! STREQ (key, "log"))
	error (1, 0, "%s, delta %s: expected `log', got `%s'",
	       rcs->path, num, key);
    d->log = rcsbuf_valcopy (rcsbuf, value, 0, (size_t *) NULL);

    /* Get random newphrases. */
    d->other = getlist();
    while (1)
    {
	if (! rcsbuf_getkey (rcsbuf, &key, &value))
	    error (1, 0, "%s, delta %s: unexpected EOF", rcs->path, num);

	if (STREQ (key, "text"))
	    break;

	p = getnode();
	p->type = rcsbuf_valcmp (rcsbuf) ? RCSCMPFLD : RCSFIELD;
	p->key = xstrdup (key);
	p->data = rcsbuf_valcopy (rcsbuf, value, p->type == RCSFIELD,
				  (size_t *) NULL);
	if (addnode (d->other, p) < 0)
	{
	    error (0, 0, "warning: %s, delta %s: duplicate field `%s'",
		   rcs->path, num, key);
	}
    }

    /* Get the change text. We already know that this key is `text'. */
    d->text = rcsbuf_valcopy (rcsbuf, value, 0, &d->len);

    return d;
}

/* RCS output functions, for writing RCS format files from RCSNode
   structures.

   For most of this work, RCS 5.7 uses an `aprintf' function which aborts
   program upon error.  Instead, these functions check the output status
   of the stream right before closing it, and aborts if an error condition
   is found.  The RCS solution is probably the better one: it produces
   more overhead, but will produce a clearer diagnostic in the case of
   catastrophic error.  In either case, however, the repository will probably
   not get corrupted. */

static int putsymbol_proc (Node *symnode, void *fparg)
{
    FILE *fp = (FILE *) fparg;

    /* A fiddly optimization: this code used to just call fprintf, but
       in an old repository with hundreds of tags this can get called
       hundreds of thousands of times when doing a cvs tag.  Since
       tagging is a relatively common operation, and using putc and
       fputs is just as comprehensible, the change is worthwhile.  */
    putc ('\n', fp);
    putc ('\t', fp);
    fputs (symnode->key, fp);
    putc (':', fp);
    fputs (symnode->data, fp);
    return 0;
}

static int putlock_proc PROTO ((Node *, void *));

/* putlock_proc is like putsymbol_proc, but key and data are reversed. */

static int
putlock_proc (symnode, fp)
    Node *symnode;
    void *fp;
{
    return fprintf ((FILE *) fp, "\n\t%s:%s", symnode->data, symnode->key);
}

static int
putrcsfield_proc (node, vfp)
    Node *node;
    void *vfp;
{
    FILE *fp = (FILE *) vfp;

    /* Some magic keys used internally by CVS start with `;'. Skip them. */
    if (node->key[0] == ';')
	return 0;

    fprintf (fp, "\n%s\t", node->key);
    if (node->data != NULL)
    {
	/* If the field's value contains evil characters,
	   it must be stringified. */
	/* FIXME: This does not quite get it right.  "7jk8f" is not a legal
	   value for a value in a newpharse, according to doc/RCSFILES,
	   because digits are not valid in an "id".  We might do OK by
	   always writing strings (enclosed in @@).  Would be nice to
	   explicitly mention this one way or another in doc/RCSFILES.
	   A case where we are wrong in a much more clear-cut way is that
	   we let through non-graphic characters such as whitespace and
	   control characters.  */

	if ((node->type == RCSCMPFLD || strpbrk (node->data, "$,.:;@") == NULL) && strlen(node->data)>0)
	    fputs (node->data, fp);
	else
	{
	    putc ('@', fp);
	    expand_at_signs (node->data, strlen (node->data), fp);
	    putc ('@', fp);
	}
    }

    /* desc, log and text fields should not be terminated with semicolon;
       all other fields should be. */
    if (! STREQ (node->key, "desc") &&
	! STREQ (node->key, "log") &&
	! STREQ (node->key, "text"))
    {
	putc (';', fp);
    }
    return 0;
}

/* Seek within an rcs file */
static void rcsbuf_seek(struct rcsbuffer *rcsbuf, off_t pos)
{
	if(rcsbuf->pos>pos)
	{
		rcsbuf->pos = pos;
		rcsbuf->ptrend = rcsbuf->ptr = rcsbuf->buffer;
		if(rcsbuf->fp)
			CVS_FSEEK(rcsbuf->fp,pos,SEEK_SET);
	}
	rcsbuf->ptr = rcsbuf->buffer + (pos - rcsbuf->pos);
	if(rcsbuf->ptr >= rcsbuf->ptrend)
		rcsbuf->ptr = rcsbuf_fill(rcsbuf,rcsbuf->ptr, NULL, NULL);
}

/* Output the admin node for RCS into stream FP. */

static void
RCS_putadmin (rcs, fp)
    RCSNode *rcs;
    FILE *fp;
{
    fprintf (fp, "%s\t%s;\n", RCSHEAD, rcs->head ? rcs->head : "");
    if (rcs->branch)
	fprintf (fp, "%s\t%s;\n", RCSBRANCH, rcs->branch);

    fputs ("access", fp);
    if (rcs->access)
    {
	char *p, *s;
	s = xstrdup (rcs->access);
	for (p = strtok (s, " \n\t"); p != NULL; p = strtok (NULL, " \n\t"))
	    fprintf (fp, "\n\t%s", p);
	xfree (s);
    }
    fputs (";\n", fp);

    fputs (RCSSYMBOLS, fp);
    /* If we haven't had to convert the symbols to a list yet, don't
       force a conversion now; just write out the string.  */
    if (rcs->symbols == NULL && rcs->symbols_data != NULL)
    {
	fputs ("\n\t", fp);
	fputs (rcs->symbols_data, fp);
    }
    else
	walklist (RCS_symbols (rcs), putsymbol_proc, (void *) fp);
    fputs (";\n", fp);

    fputs ("locks", fp);
    if (rcs->locks_data)
	fprintf (fp, "\t%s", rcs->locks_data);
    else if (rcs->locks)
	walklist (rcs->locks, putlock_proc, (void *) fp);
    if (rcs->strict_locks)
	fprintf (fp, "; strict");
    fputs (";\n", fp);

    if (rcs->comment)
    {
	fprintf (fp, "comment\t@");
	expand_at_signs (rcs->comment, strlen (rcs->comment), fp);
	fputs ("@;\n", fp);
    }
    if (rcs->expand && ! STREQ (rcs->expand, "kv"))
	fprintf (fp, "%s\t@%s@;\n", RCSEXPAND, rcs->expand);

    walklist (rcs->other, putrcsfield_proc, (void *) fp);

    putc ('\n', fp);
}

static void putdelta(RCSVers *vers, FILE *fp)
{
    Node *bp, *start;

    /* Skip if no revision was supplied, or if it is outdated (cvs admin -o) */
    if (vers == NULL || vers->outdated)
	return;

    fprintf (fp, "\n%s\n%s\t%s;\t%s ",
	     vers->version,
	     RCSDATE, vers->date,
	     "author");

    if(strpbrk(vers->author,"$,.:;@"))
    {
	fputs("@", fp);
        expand_at_signs (vers->author, strlen(vers->author), fp);
	fputs ("@", fp);
    }
    else
	fputs(vers->author,fp);

    fprintf (fp, ";\t%s %s;\nbranches",
	     "state", vers->state ? vers->state : "");

    if (vers->branches != NULL)
    {
	start = vers->branches->list;
	for (bp = start->next; bp != start; bp = bp->next)
	    fprintf (fp, "\n\t%s", bp->key);
    }

    fprintf (fp, ";\nnext\t%s;", vers->next ? vers->next : "");

	fprintf (fp, "\ndeltatype\t%s;", vers->type?vers->type:"text");

    walklist (vers->other_delta, putrcsfield_proc, fp);

    putc ('\n', fp);
}

static void no_del(Node *p)
{
}

static void RCS_putdtree (RCSNode *rcs, char *rev, FILE *fp)
{
    RCSVers *versp;
    Node *p, *q, *branch;
	List *revs = getlist();

	char *orev = rev;

	do
	{
		/* Find the delta node for this revision. */
		p = findnode (rcs->versions, rev);
		if (p == NULL)
		{
			error (1, 0,
				"error parsing repository file %s, file may be corrupt.",
				rcs->path);
		}

		versp = (RCSVers *) p->data;

		/* Print the delta node and recurse on its `next' node.  This prints
		the trunk.  If there are any branches printed on this revision,
		print those trunks as well. */
		putdelta (versp, fp);

		if(versp->branches)
		{
			p = getnode();
			p->key = xstrdup(rev);
			p->data = (char*)versp;
			p->delproc = no_del;
			addnode_at_front(revs,p);
		}

		rev = versp->next;
	} while(rev);

	for(p=revs->list->next; p!=revs->list;p=p->next)
	{
		versp = (RCSVers *) p->data;

		/* Recurse into this routine to do the branches.  This will eventually fail,
		   but a lot later than the original implementation that blew after 988 versions */
		branch = versp->branches->list;
		for (q = branch->next; q != branch; q = q->next)
			RCS_putdtree (rcs, q->key, fp);
	}
	dellist(&revs);
	fflush(fp);
}

static void
RCS_putdesc (rcs, fp)
    RCSNode *rcs;
    FILE *fp;
{
    fprintf (fp, "\n\n%s\n@", RCSDESC);
    if (rcs->desc != NULL)
    {
	int len = strlen (rcs->desc);
	if (len > 0)
	{
	    expand_at_signs (rcs->desc, len, fp);
	    if (rcs->desc[len-1] != '\n')
		putc ('\n', fp);
	}
    }
    fputs ("@\n", fp);
}

static void putdeltatext (FILE *fp, Deltatext *d, int compress)
{
    fprintf (fp, "\n\n%s\nlog\n@", d->version);
    if (d->log != NULL)
    {
	int loglen = strlen (d->log);
	expand_at_signs (d->log, loglen, fp);
	if (d->log[loglen-1] != '\n')
	    putc ('\n', fp);
    }
    putc ('@', fp);

    walklist (d->other, putrcsfield_proc, fp);

    fputs ("\ntext\n@", fp);
	if (d->text != NULL)
	{
		if(compress)
		{
			uLong zlen;
			void *zbuf;

			z_stream stream = {0};
			deflateInit(&stream,Z_DEFAULT_COMPRESSION);
			zlen = deflateBound(&stream, d->len);
			stream.avail_in = d->len;
			stream.next_in = d->text;
			stream.avail_out = zlen;
			zbuf = xmalloc(zlen+4);
			stream.next_out = ((char*)zbuf)+4;
			*(unsigned long *)zbuf=htonl(d->len);
			if(deflate(&stream, Z_FINISH)!=Z_STREAM_END)
			{
				error(1,0,"internal error: deflate failed");
			}
			expand_at_signs (zbuf, stream.total_out+4, fp);
			deflateEnd(&stream);
			xfree(zbuf);
		}
		else
			expand_at_signs (d->text, d->len, fp);
	}
    fputs ("@\n", fp);
}

/* TODO: the whole mechanism for updating deltas is kludgey... more
   sensible would be to supply all the necessary info in a `newdeltatext'
   field for RCSVers nodes. -twp */

/* Copy delta text nodes from FIN to FOUT.  If NEWDTEXT is non-NULL, it
   is a new delta text node, and should be added to the tree at the
   node whose revision number is INSERTPT.  (Note that trunk nodes are
   written in decreasing order, and branch nodes are written in
   increasing order.) */

static void RCS_copydeltas(RCSNode *rcs, FILE *fin, struct rcsbuffer *rcsbufin,
    FILE *fout, Deltatext *newdtext, char *insertpt, int compress_new_delta)
{
    int actions;
    RCSVers *dadmin;
    Node *np;
    int insertbefore, found;
    char *bufrest;
    int nls;
    size_t buflen;
    char buf[8192];
    int got;

    /* Count the number of versions for which we have to do some
       special operation.  */
    actions = walklist (rcs->versions, count_delta_actions, (void *) NULL);

    /* Make a note of whether NEWDTEXT should be inserted
       before or after its INSERTPT. */
    insertbefore = (newdtext != NULL && numdots (newdtext->version) == 1);

	TRACE(3,"RCS_copydeltas(%s,%s,%d,%d)", PATCH_NULL(insertpt), newdtext?newdtext->version:"", insertbefore, compress_new_delta);

	while (actions != 0 || newdtext != NULL)
    {
	Deltatext *dtext;

	dtext = RCS_getdeltatext (rcs, fin, rcsbufin);

	/* We shouldn't hit EOF here, because that would imply that
           some action was not taken, or that we could not insert
           NEWDTEXT.  */
	if (dtext == NULL)
	    error (1, 0, "internal error: EOF too early in RCS_copydeltas");

	found = (insertpt != NULL && STREQ (dtext->version, insertpt));
	if (found && insertbefore)
	{
	    putdeltatext (fout, newdtext, compress_new_delta);
	    newdtext = NULL;
	    insertpt = NULL;
	}

	np = findnode (rcs->versions, dtext->version);
	dadmin = (RCSVers *) np->data;

	/* If this revision has been outdated, just skip it. */
	if (dadmin->outdated)
	{
	    freedeltatext (dtext);
	    --actions;
	    continue;
	}

	/* Update the change text for this delta.  New change text
	   data may come from cvs admin -m, cvs admin -o, or cvs ci. */
	if (dadmin->text != NULL)
	{
	    if (dadmin->text->log != NULL || dadmin->text->text != NULL)
		--actions;
	    if (dadmin->text->log != NULL)
	    {
		xfree (dtext->log);
		dtext->log = dadmin->text->log;
		dadmin->text->log = NULL;
	    }
	    if (dadmin->text->text != NULL)
	    {
		xfree (dtext->text);
		dtext->text = dadmin->text->text;
		dtext->len = dadmin->text->len;
		dadmin->text->text = NULL;
	    }
	}
	putdeltatext (fout, dtext, 0);
	freedeltatext (dtext);

	if (found && !insertbefore)
	{
	    putdeltatext (fout, newdtext, compress_new_delta);
	    newdtext = NULL;
	    insertpt = NULL;
	}
    }

    /* Copy the rest of the file directly, without bothering to
       interpret it.  The caller will handle error checking by calling
       ferror.

       We just wrote a newline to the file, either in putdeltatext or
       in the caller.  However, we may not have read the corresponding
       newline from the file, because rcsbuf_getkey returns as soon as
       it finds the end of the '@' string for the desc or text key.
       Therefore, we may read three newlines when we should really
       only write two, and we check for that case here.  This is not
       an semantically important issue; we only do it to make our RCS
       files look traditional.  */

    nls = 3;

    rcsbuf_get_buffered (rcsbufin, &bufrest, &buflen);
    if (buflen > 0)
    {
	if (bufrest[0] != '\n'
	    || strncmp (bufrest, "\n\n\n", buflen < 3 ? buflen : 3) != 0)
	{
	    nls = 0;
	}
	else
	{
	    if (buflen < 3)
		nls -= buflen;
	    else
	    {
		++bufrest;
		--buflen;
		nls = 0;
	    }
	}

	fwrite (bufrest, 1, buflen, fout);
    }

    while ((got = fread (buf, 1, sizeof buf, fin)) != 0)
    {
	if (nls > 0
	    && got >= nls
	    && buf[0] == '\n'
	    && strncmp (buf, "\n\n\n", nls) == 0)
	{
	    fwrite (buf + 1, 1, got - 1, fout);
	}
	else
	{
	    fwrite (buf, 1, got, fout);
	}

	nls = 0;
    }
}

/* A helper procedure for RCS_copydeltas.  This is called via walklist
   to count the number of RCS revisions for which some special action
   is required.  */

static int
count_delta_actions (np, ignore)
    Node *np;
    void *ignore;
{
    RCSVers *dadmin;

    dadmin = (RCSVers *) np->data;

    if (dadmin->outdated)
	return 1;

    if (dadmin->text != NULL
	&& (dadmin->text->log != NULL || dadmin->text->text != NULL))
    {
	return 1;
    }

    return 0;
}

/*
 * Clean up temporary files
 */
RETSIGTYPE
rcs_cleanup ()
{
    /* Note that the checks for existence_error are because we are
       called from a signal handler, so we don't know whether the
       files got created.  */

    /* FIXME: Do not perform buffered I/O from an interrupt handler like
       this (via error).  However, I'm leaving the error-calling code there
       in the hope that on the rare occasion the error call is actually made
       (e.g., a fluky I/O error or permissions problem prevents the deletion
       of a just-created file) reentrancy won't be an issue.  */
    if (rcs_lockfile != NULL)
    {
	char *tmp = rcs_lockfile;
	rcs_lockfile = NULL;
	if (rcs_lockfd >= 0)
	{
	    if (close (rcs_lockfd) != 0)
		error (0, errno, "error closing lock file %s", tmp);
	    rcs_lockfd = -1;
	}
	if (unlink_file (tmp) < 0
	    && !existence_error (errno))
	    error (0, errno, "cannot remove %s", tmp);
    }
}

/* RCS_internal_lockfile and RCS_internal_unlockfile perform RCS-style
   locking on the specified RCSFILE: for a file called `foo,v', open
   for writing a file called `,foo,'.

   Note that we what do here is quite different from what RCS does.
   RCS creates the ,foo, file before it reads the RCS file (if it
   knows that it will be writing later), so that it actually serves as
   a lock.  We don't; instead we rely on CVS writelocks.  This means
   that if someone is running RCS on the file at the same time they
   are running CVS on it, they might lose (we read the file,
   then RCS writes it, then we write it, clobbering the
   changes made by RCS).  I believe the current sentiment about this
   is "well, don't do that".

   A concern has been expressed about whether adopting the RCS
   strategy would slow us down.  I don't think so, since we need to
   write the ,foo, file anyway (unless perhaps if O_EXCL is slower or
   something).

   These do not perform quite the same function as the RCS -l option
   for locking files: they are intended to prevent competing RCS
   processes from stomping all over each other's laundry.  Hence,
   they are `internal' locking functions.

   If there is an error, give a fatal error; if we return we always
   return a non-NULL value.  */

static FILE *rcs_internal_lockfile (const char *rcsfile, size_t* lockId)
{
    FILE *fp;
    static int first_call = 1;

    if (first_call)
    {
	first_call = 0;
	/* clean up if we get a signal */
#ifdef SIGABRT
	(void) SIG_register (SIGABRT, rcs_cleanup);
#endif
#ifdef SIGHUP
	(void) SIG_register (SIGHUP, rcs_cleanup);
#endif
#ifdef SIGINT
	(void) SIG_register (SIGINT, rcs_cleanup);
#endif
#ifdef SIGQUIT
	(void) SIG_register (SIGQUIT, rcs_cleanup);
#endif
#ifdef SIGPIPE
	(void) SIG_register (SIGPIPE, rcs_cleanup);
#endif
#ifdef SIGTERM
	(void) SIG_register (SIGTERM, rcs_cleanup);
#endif
    }

    /* Get the lock file name: `,file,' for RCS file `file,v'. */
    assert (rcs_lockfile == NULL);
    assert (rcs_lockfd < 0);
    rcs_lockfile = rcs_lockfilename (rcsfile);

	*lockId = do_lock_file(rcsfile,NULL,1); /* Ask lockserver for an exclusive write lock */

    /* Try to open exclusively.  POSIX.1 guarantees that O_EXCL|O_CREAT
       guarantees an exclusive open.  According to the RCS source, with
       NFS v2 we must also throw in O_TRUNC and use an open mask that makes
       the file unwriteable.  For extensive justification, see the comments for
       rcswriteopen() in rcsedit.c, in RCS 5.7.  This is kind of pointless
       in the CVS case; see comment at the start of this file concerning
       general ,foo, file strategy.

       There is some sentiment that with NFSv3 and such, that one can
       rely on O_EXCL these days.  This might be true for unix (I
       don't really know), but I am still pretty skeptical in the case
       of the non-unix systems.  */

#ifdef _WIN32
    /* For Win32, if the file is open, it is locked.  Try to deleted it, otherwise the following open will fail */
    unlink(rcs_lockfile);
#endif
    rcs_lockfd = open (rcs_lockfile,
		       OPEN_BINARY | O_WRONLY | O_CREAT | O_EXCL | O_TRUNC,
		       S_IRUSR | S_IRGRP | S_IROTH);

    if (rcs_lockfd < 0)
    {
	error (1, errno, "could not open lock file `%s'", fn_root(rcs_lockfile));
    }

    fp = fdopen (rcs_lockfd, FOPEN_BINARY_WRITE);
    if (fp == NULL)
	error (1, errno, "cannot fdopen %s", fn_root(rcs_lockfile));

    return fp;
}

static void rcs_internal_unlockfile (FILE *fp, char *rcsfile, size_t lockId)
{
    assert (rcs_lockfile != NULL);
    assert (rcs_lockfd >= 0);

    /* Abort if we could not write everything successfully to LOCKFILE.
       This is not a great error-handling mechanism, but should prevent
       corrupting the repository. */

    if (ferror (fp))
	/* The only case in which using errno here would be meaningful
	   is if we happen to have left errno unmolested since the call
	   which produced the error (e.g. fprintf).  That is pretty
	   fragile even if it happens to sometimes be true.  The real
	   solution is to check each call to fprintf rather than waiting
	   until the end like this.  */
	error (1, 0, "error writing to lock file %s", rcs_lockfile);
    if (fclose (fp) == EOF)
	error (1, errno, "error closing lock file %s", rcs_lockfile);
    rcs_lockfd = -1;

    rename_file (rcs_lockfile, rcsfile);

	do_unlock_file(lockId);

    {
	/* Use a temporary to make sure there's no interval
	   (after rcs_lockfile has been freed but before it's set to NULL)
	   during which the signal handler's use of rcs_lockfile would
	   reference freed memory.  */
	char *tmp = rcs_lockfile;
	rcs_lockfile = NULL;
	xfree (tmp);
    }
}

static char *rcs_lockfilename (const char *rcsfile)
{
    char *lockfile, *lockp;
    const char *rcsbase, *rcsp, *rcsend;
    int rcslen;

    /* Create the lockfile name. */
    rcslen = strlen (rcsfile);
    lockfile = (char *) xmalloc (rcslen + 10);
    rcsbase = last_component (rcsfile);
    rcsend = rcsfile + rcslen - sizeof(RCSEXT);
    for (lockp = lockfile, rcsp = rcsfile; rcsp < rcsbase; ++rcsp)
		*lockp++ = *rcsp;
    *lockp++ = ',';
    while (rcsp <= rcsend)
	*lockp++ = *rcsp++;
    *lockp++ = ',';
    *lockp = '\0';

    return lockfile;
}

/* Rewrite an RCS file.  The basic idea here is that the caller should
   first call RCS_reparsercsfile, then munge the data structures as
   desired (via RCS_delete_revs, RCS_settag, &c), then call RCS_rewrite.  */

void RCS_rewrite (RCSNode *rcs, Deltatext *newdtext, char *insertpt, int compress_new_delta)
{
    FILE *fout;
	size_t lockId_temp;

	TRACE(2,"RCS_rewrite(%s,%s,%s,%d)",rcs?rcs->path:"",newdtext?newdtext->text:"",insertpt,compress_new_delta);

    if (noexec)
		return;

	/* Make sure we're operating on an actual file and not a symlink.  */
    resolve_symlink (&(rcs->path));

    fout = rcs_internal_lockfile (rcs->path, &lockId_temp);

    RCS_putadmin (rcs, fout);
    RCS_putdtree (rcs, rcs->head, fout);
    RCS_putdesc (rcs, fout);

	rcsbuf_seek(&rcs->rcsbuf, rcs->delta_pos);

    /* Update delta_pos to the current position in the output file.
       Do NOT move these statements: they must be done after fin has
       been positioned at the old delta_pos, but before any delta
       texts have been written to fout. */
	fflush(fout);
    rcs->delta_pos = CVS_FTELL (fout);
    if (rcs->delta_pos == -1)
		error (1, errno, "cannot ftell in RCS file %s", rcs->path);

    RCS_copydeltas (rcs, rcs->rcsbuf.fp, &rcs->rcsbuf, fout, newdtext, insertpt, compress_new_delta);
	fflush(fout);

    rcsbuf_close (&rcs->rcsbuf);
	if(newdtext)
	{
		Node *vers = findnode (rcs->versions, newdtext->version);
		RCSVers *vnode = (RCSVers*)vers->data;
		char *branch = RCS_branchfromversion(rcs,newdtext->version);
		do_modified(lockId_temp,newdtext->version,insertpt,branch?branch:"HEAD",(vnode->state && !strcmp(vnode->state,RCSDEAD))?'D':'M');
		xfree(branch);
	}

    rcs_internal_unlockfile (fout, rcs->path, lockId_temp);
}

/* Abandon changes to an RCS file. */

void
RCS_abandon (rcs)
    RCSNode *rcs;
{
    free_rcsnode_contents (rcs);
    rcs->symbols_data = NULL;
    rcs->expand = NULL;
    rcs->access = NULL;
    rcs->locks_data = NULL;
    rcs->comment = NULL;
    rcs->desc = NULL;
    rcs->flags |= PARTIAL;
}

/*
 * For a given file with full pathname PATH and revision number REV,
 * produce a file label suitable for passing to diff.  The default
 * file label as used by RCS 5.7 looks like this:
 *
 *	FILENAME <tab> YYYY/MM/DD <sp> HH:MM:SS <tab> REVNUM
 *
 * The date and time used are the revision's last checkin date and time.
 * If REV is NULL, use the working copy's mtime instead.
 *
 * /dev/null is not statted but assumed to have been created on the Epoch.
 * At least using the POSIX.2 definition of patch, this should cause creation
 * of files on platforms such as Windoze where the null IO device isn't named
 * /dev/null to be parsed by patch properly.
 */
char *
make_file_label (path, rev, rcs)
    const char *path;
    char *rev;
    RCSNode *rcs;
{
    char datebuf[MAXDATELEN + 1];
    char *label;

    label = (char *) xmalloc (strlen (path)
			      + (rev == NULL ? 0 : strlen (rev) + 1)
			      + MAXDATELEN
			      + 2);

    if (rev)
    {
	char date[MAXDATELEN + 1];
	/* revs cannot be attached to /dev/null ... duh. */
	assert (strcmp(DEVNULL, path));
	RCS_getrevtime (rcs, rev, datebuf, 0);
	(void) date_to_internet (date, datebuf);
	(void) sprintf (label, "-L%s\t%s\t%s", path, date, rev);
    }
    else
    {
	struct stat sb;
	struct tm *wm = NULL;

	if (strcmp(DEVNULL, path))
	{
	    const char *file = last_component ((char*)path);
	    if (CVS_STAT (file, &sb) < 0)
		error (0, 1, "could not get info for `%s'", path);
	    else
		wm = gmtime (&sb.st_mtime);
	}
	else
	{
	    time_t t = 0;
	    wm = gmtime(&t);
	}

	if (wm)
	{
	    (void) tm_to_internet (datebuf, wm);
	    (void) sprintf (label, "-L%s\t%s", path, datebuf);
	}
    }
    return label;
}

struct putrbranch_data
{
	FILE *fp;
	RCSNode *rcs;
};

/* Create the initial reverse map file.  Every RCS file should
   have one of these (since the initial creation of the file
   implies at least one revision with that name). */
int RCS_create_reverse(RCSNode *rcs)
{
	char *path = xmalloc(strlen(rcs->path)+10), *realpath;
	char *basefilename = xmalloc(strlen(rcs->path)+10), *basep;
	FILE *f;
	struct putrbranch_data dat;

	strcpy(path,rcs->path);
	assert(strlen(path)>2 && !strcmp(path+strlen(path)-2,",v"));
	strcpy(path+strlen(rcs->path)-2,",rv");
	if(isfile(path)) /* File exists... don't overwrite it */
	{
		xfree(path);
		return 0;
	}

	/* Calculate base filename */
	basep = strrchr(path,'/');
	if(!basep)
		basep=path;
	else
		basep++;
	strcpy(basefilename,basep);
	basefilename[strlen(basefilename)-3]='\0';

    assert (rcs != NULL);

    if (rcs->flags & PARTIAL)
		RCS_reparsercsfile (rcs);

	realpath=xstrdup(path);
	memmove(basep+1,basep,strlen(basep)+1);
	*basep=',';
	if((f=fopen(path,"wb"))==NULL)
	{
		error(1,errno,"Couldn't create reverse RCS file %s",fn_root(path));
	}
	fprintf(f,"head\t%s;\n",rcs->head);
	fprintf(f,"symbols");
	walklist (RCS_symbols(rcs), putsymbol_proc, f);
	fprintf(f,";\n");
	fprintf(f,"branches");
	dat.fp=f;
	dat.rcs=rcs;
	walklist (RCS_symbols(rcs), putrbranch_proc, &dat);
	fprintf(f,";\n");

	RCS_putrtree(rcs,rcs->head,f,basefilename);
	fclose(f);
	CVS_RENAME(path,realpath);
	xfree(path);
	xfree(realpath);
	return 0;
}

/* Create the reverse delta tree */
static void RCS_putrtree (RCSNode *rcs, char *rev, FILE *fp, const char *basefilename)
{
    RCSVers *versp;
    Node *p, *q, *branch;
	List *revs = getlist();

	char *orev = rev;

	do
	{
		/* Find the delta node for this revision. */
		p = findnode (rcs->versions, rev);
		if (p == NULL)
		{
			error (1, 0,
				"error parsing repository file %s, file may be corrupt.",
				rcs->path);
		}

		versp = (RCSVers *) p->data;

		/* Print the delta node and recurse on its `next' node.  This prints
		the trunk.  If there are any branches printed on this revision,
		print those trunks as well. */
		putrdelta (versp, fp, basefilename, relative_repos(fn_root(rcs->path)));

		if(versp->branches)
		{
			p = getnode();
			p->key = xstrdup(rev);
			p->data = (char*)versp;
			p->delproc = no_del;
			addnode_at_front(revs,p);
		}

		rev = versp->next;
	} while(rev);

	for(p=revs->list->next; p!=revs->list;p=p->next)
	{
		versp = (RCSVers *) p->data;

		/* Recurse into this routine to do the branches.  This will eventually fail,
		   but a lot later than the original implementation that blew after 988 versions */
		branch = versp->branches->list;
		for (q = branch->next; q != branch; q = q->next)
			RCS_putrtree (rcs, q->key, fp, basefilename);
	}
	dellist(&revs);
	fflush(fp);
}

static void putrdelta(RCSVers *vers, FILE *fp, const char *basefilename, const char *rcsfilename)
{
    Node *p;

    /* Skip if no revision was supplied, or if it is outdated (cvs admin -o) */
    if (vers == NULL || vers->outdated)
		return;

	p = findnode(vers->other_delta, "filename");
	/* Skip if this version isn't part of this reverse file */
	if(p && fncmp((const char *)p->data,basefilename))
		return;

    fprintf (fp, "\n%s\n%s %s;\n%s\t%s",
	     vers->version, "filename", rcsfilename,
	     RCSDATE, vers->date);

    fprintf (fp, ";\t%s %s;",
	     "state", vers->state ? vers->state : "");

    putc ('\n', fp);
}

/* Write a line into the reverse branch map.
   This is used to make sure that all revisions are
   unique across all the RCS files that this reverse map references. */
static int putrbranch_proc (Node *symnode, void *fparg)
{
	struct putrbranch_data *dat = (struct putrbranch_data *)fparg;
    FILE *fp = dat->fp;
	RCSNode *rcs = dat->rcs;
	char *sym,*p, *br;

	/* If this isn't a branch, just return */
	p=strstr(symnode->data,".0.");
	if(!p)
		return 0;

	sym = xstrdup(symnode->data);
	p=strstr(sym,".0.");
	strcpy(p+1,p+3);
	br = RCS_getbranch(rcs, sym, 1);
	if(!br)
	{
		/* No branch */
		xfree(sym);
		return 0;
	}
    putc ('\n', fp);
    putc ('\t', fp);
    fputs (br, fp);
	xfree(sym);
	xfree(br);
    return 0;
}
