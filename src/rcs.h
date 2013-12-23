/*
 * Copyright (c) 1992, Brian Berliner and Jeff Polk
 * Copyright (c) 1989-1992, Brian Berliner
 * 
 * You may distribute under the terms of the GNU General Public License as
 * specified in the README file that comes with the CVS source distribution.
 * 
 * RCS source control definitions needed by rcs.c and friends
 */

#include "unicode_stuff.h"

/* Strings which indicate a conflict if they occur at the start of a line.  */
#define	RCS_MERGE_PAT_1 "<<<<<<< "
#define	RCS_MERGE_PAT_2 "=======\n"
#define	RCS_MERGE_PAT_3 ">>>>>>> "

#define	RCSEXT		",v"
#define RESREVEXT	",rv"
#define RCSPAT		"*,v"
#define RCSREVPAT	"*,rv"
#define	RCSHEAD		"head"
#define	RCSBRANCH	"branch"
#define	RCSSYMBOLS	"symbols"
#define	RCSDATE		"date"
#define	RCSDESC		"desc"
#define RCSEXPAND	"expand"

#define PROTO(x) x

/* Used by the version of death support which resulted from old
   versions of CVS (e.g. 1.5 if you define DEATH_SUPPORT and not
   DEATH_STATE).  Only a hacked up RCS (used by those old versions of
   CVS) will put this into RCS files.  Considered obsolete.  */
#define RCSDEAD		"dead"

#define	DATEFORM	"%02d.%02d.%02d.%02d.%02d.%02d"
#define	SDATEFORM	"%d.%d.%d.%d.%d.%d"

/*
 * Opaque structure definitions used by RCS specific lookup routines
 */
#define VALID	0x1			/* flags field contains valid data */
#define	INATTIC	0x2			/* RCS file is located in the Attic */
#define PARTIAL 0x4			/* RCS file not completly parsed */

/* A structure we use to buffer the contents of an RCS file.  The
   various fields are only referenced directly by the rcsbuf_*
   functions.  */

struct rcsbuffer
{
    /* Points to the current position in the buffer.  */
    char *ptr;
    /* Points just after the last valid character in the buffer.  */
    char *ptrend;
    /* The file.  */
    FILE *fp;
    /* The name of the file, used for error messages.  */
    const char *filename;
    /* The starting file position of the data in the buffer.  */
    off_t pos;
    /* The length of the value.  */
    size_t vlen;
    /* Whether the value contains an '@' string.  If so, we can not
       compress whitespace characters.  */
    int at_string;
    /* The number of embedded '@' characters in an '@' string.  If
       this is non-zero, we must search the string for pairs of '@'
       and convert them to a single '@'.  */
    int embedded_at;

	/* Buffer used by this rcs buffer object */
	char *buffer;
	size_t buffer_size;

	/* Space for allocated strings, one per token */
	char **alloc_string_buffer;
	size_t alloc_string_buffer_len;
	char **alloc_string_ptr;

	/* File lock ID */
	size_t lockId;
};

/* All the "char *" fields in RCSNode, Deltatext, and RCSVers are
   '\0'-terminated (except "text" in Deltatext).  This means that we
   can't deal with fields containing '\0', which is a limitation that
   RCS does not have.  Would be nice to fix this some day.  */

struct rcsnode
{
    /* Reference count for this structure.  Used to deal with the
       fact that there might be a pointer from the Vers_TS or might
       not.  Callers who increment this field are responsible for
       calling freercsnode when they are done with their reference.  */
    int refcount;

    /* Flags (INATTIC, PARTIAL, &c), see above.  */
    int flags;

    /* File name of the RCS file.  This is not necessarily the name
       as specified by the user, but it is a name which can be passed to
       system calls and a name which is OK to print in error messages
       (the various names might differ in case).  */
    char *path;

    /* Value for head keyword from RCS header, or NULL if empty.  */
    char *head;

    /* Value for branch keyword from RCS header, or NULL if omitted.  */
    char *branch;

    /* Raw data on symbolic revisions.  The first time that RCS_symbols is
       called, we parse these into ->symbols, and free ->symbols_data.  */
    char *symbols_data;

    /* Value for expand keyword from RCS header, or NULL if omitted.  */
    char *expand;

    /* List of nodes, the key of which is the symbolic name and the data
       of which is the numeric revision that it corresponds to (malloc'd).  */
    List *symbols;

    /* List of nodes (type RCSVERS), the key of which the numeric revision
       number, and the data of which is an RCSVers * for the revision.  */
    List *versions;

    /* Value for access keyword from RCS header, or NULL if empty.
       FIXME: RCS_delaccess would also seem to use "" for empty.  We
       should pick one or the other.  */
    char *access;

    /* Raw data on locked revisions.  The first time that RCS_getlocks is
       called, we parse these into ->locks, and free ->locks_data.  */
    char *locks_data;

    /* List of nodes, the key of which is the numeric revision and the
       data of which is the user that it corresponds to (malloc'd).  */
    List *locks;

    /* Set for the strict keyword from the RCS header.  */
    int strict_locks;

    /* Value for the comment keyword from RCS header (comment leader), or
       NULL if omitted.  */
    char *comment;

    /* Value for the desc field in the RCS file, or NULL if empty.  */
    char *desc;

    /* File offset of the first deltatext node, so we can seek there.  */
    off_t delta_pos;

    /* Newphrases from the RCS header.  List of nodes, the key of which
       is the "id" which introduces the newphrase, and the value of which
       is the value from the newphrase.  */
    List *other;

	/* Versions returned by lock server when full lock is held, or NULL if file is current */
	/* Format HEAD:version BRANCH:version ... */
	const char *lock_version;

	/* rcs buffer for this rcs node */
	struct rcsbuffer rcsbuf;
};

typedef struct rcsnode RCSNode;

struct deltatext {
    char *version;

    /* Log message, or NULL if we do not intend to change the log message
       (that is, RCS_copydeltas should just use the log message from the
       file).  */
    char *log;

    /* Change text, or NULL if we do not intend to change the change text
       (that is, RCS_copydeltas should just use the change text from the
       file).  Note that it is perfectly legal to have log be NULL and
       text non-NULL, or vice-versa.  */
    char *text;
    size_t len;

    /* Newphrase fields from deltatext nodes.  FIXME: duplicates the
       other field in the rcsversnode, I think.  */
    List *other;
};
typedef struct deltatext Deltatext;

struct rcsversnode
{
    /* Duplicate of the key by which this structure is indexed.  */
    char *version;

    char *date;
    char *author;
    char *state;
    char *next;
	char *previous;
	char *type;
    int dead;
    int outdated;
    Deltatext *text;
    List *branches;
    /* Newphrase fields from deltatext nodes.  Also contains ";add" and
       ";delete" magic fields (see rcs.c, log.c).  I think this is
       only used by log.c (where it looks up "log").  Duplicates the
       other field in struct deltatext, I think.  */
    List *other;
    /* Newphrase fields from delta nodes.  */
    List *other_delta;
};
typedef struct rcsversnode RCSVers;

/*
 * CVS reserves all even-numbered branches for its own use.  "magic" branches
 * (see rcs.c) are contained as virtual revision numbers (within symbolic
 * tags only) off the RCS_MAGIC_BRANCH, which is 0.  CVS also reserves the
 * ".1" branch for vendor revisions.  So, if you do your own branching, you
 * should limit your use to odd branch numbers starting at 3.
 */
#define	RCS_MAGIC_BRANCH	0

/* The type of a function passed to RCS_checkout.  */
typedef void (*RCSCHECKOUTPROC) (void *, const char *, size_t);

#ifdef __STDC__
struct rcsbuffer;
#endif

/* What RCS_deltas is supposed to do.  */
enum rcs_delta_op {RCS_ANNOTATE, RCS_FETCH};

/*
 * exported interfaces
 */
RCSNode *RCS_parse (const char *file, const char *repos);
RCSNode *RCS_parsercsfile (char *rcsfile);
void RCS_fully_parse (RCSNode *);
void RCS_reparsercsfile (RCSNode *);
extern int RCS_setattic (RCSNode *, int);

enum
{
	/* Encodings */
	KFLAG_TEXT           = 0x00000000,
	KFLAG_BINARY         = 0x00000001,
	KFLAG_ENCODED		 = 0x00000002,

	/* Expansions */
	KFLAG_KEYWORD        = 0x00001000,
	KFLAG_VALUE_LOGONLY  = 0x00002000, /* Internal use only */
	KFLAG_VALUE          = 0x00004000,
	KFLAG_LOCKER         = 0x00008000,
	KFLAG_PRESERVE       = 0x00010000,

	/* Extra */
	KFLAG_UNIX           = 0x01000000,
	KFLAG_BINARY_DELTA   = 0x02000000,
	KFLAG_COMPRESS_DELTA = 0x04000000,
	KFLAG_RESERVED_EDIT  = 0x08000000,

	KFLAG_ENCODINGS      = 0x00000FFF,
	KFLAG_EXPANSIONS     = 0x00FFF000,
	KFLAG_EXTRA		     = 0xFF000000,

	/* Flag types */
	KFLAG_LEGACY		 = 0x00000001, /* On Unix CVS */
	KFLAG_CVSNT		     = 0x00000002, /* CVSNT Only */
	KFLAG_ESSENTIAL		 = 0x00000100, /* If the client can't handle this then fail */
};

typedef struct
{
	const char flag; /* Flag as used in -k option */
	const char *keyword; /* Keyword for -kx */
	const char *altkeyword; /* Alternate keyword */
	encoding_type encoding; /* Resultant encoding, or 0 */
	unsigned bitmask; /* Effect(s) of flag */
	unsigned type; /* Type of flag - cvsnt, legacy, etc. */
	const char alternate; /* Legacy alternate char to send to client if not supported */
} kflag_t;

extern const kflag_t kflag_encoding[];
extern const kflag_t kflag_flags[];

typedef int (*RCSCHECKINPROC)(const char *filename, char **buffer, size_t *buflen, char **displayname);

typedef struct
{
	encoding_type encoding;
	unsigned flags;
} kflag;

char *RCS_check_kflag(const char *arg);
kflag RCS_get_kflags(const char *arg, int error);
char *RCS_getdate (RCSNode * rcs, char *date, int force_tag_match);
char *RCS_gettag (RCSNode * rcs, char *symtag, int force_tag_match,
			int *simple_tag);
int RCS_isfloating(RCSNode *rcs, const char *rev);
int RCS_exist_rev (RCSNode *rcs, const char *rev);
int RCS_exist_tag (RCSNode *rcs, char *tag);
char *RCS_tag2rev (RCSNode *rcs, const char *tag);
char *RCS_getversion (RCSNode * rcs, char *tag, char *date,
		      int force_tag_match, int *simple_tag);
char *RCS_magicrev (RCSNode *rcs, char *rev);
int RCS_isbranch (RCSNode *rcs, const char *rev);
int RCS_nodeisbranch (RCSNode *rcs, const char *tag);
char *RCS_whatbranch (RCSNode *rcs, const char *tag);
char *RCS_branchfromversion(RCSNode *rcs, const char *rev);
char *RCS_head (RCSNode * rcs);
int RCS_datecmp (char *date1, char *date2);
time_t RCS_getrevtime (RCSNode * rcs, char *rev, char *date, int fudge);
List *RCS_symbols (RCSNode *rcs);
void RCS_check_tag (const char *tag);
int RCS_valid_rev (const char *rev);
List *RCS_getlocks (RCSNode *rcs);
void rcsbuf_close (struct rcsbuffer *rcsbuf);
void freercsnode (RCSNode ** rnodep);
char *RCS_getbranch (RCSNode * rcs, char *tag, int force_tag_match);
char *RCS_branch_head (RCSNode *rcs, char *rev);
char *RCS_getfilename (RCSNode *rcs, char *rev);
char *RCS_getbranchpoint (RCSNode *rcs, char *target);

int RCS_isdead (RCSNode *, const char *);
char *RCS_getexpand (RCSNode *);
void RCS_setexpand (RCSNode *, char *);
int RCS_checkout (RCSNode *, char *, char *, char *, char *, char *,
			 RCSCHECKOUTPROC, void *, mode_t*);
int RCS_checkin (RCSNode *rcs, char *workfile, char *message, char *rev, int flags,
			 const char *merge_from_tag1, const char *merge_from_tag2, RCSCHECKINPROC callback, char **pnewversion);
int RCS_cmp_file (RCSNode *, char *, char *, const char *);
int RCS_settag (RCSNode *, const char *, const char *, const char *);
int RCS_deltag (RCSNode *, const char *);
int RCS_setbranch (RCSNode *, const char *);
int RCS_lock (RCSNode *, char *, int);
int RCS_unlock (RCSNode *, char *, int);
int RCS_delete_revs (RCSNode *, char *, char *, int);
void RCS_addaccess (RCSNode *, char *);
void RCS_delaccess (RCSNode *, char *);
char *RCS_getaccess (RCSNode *);
RETSIGTYPE rcs_cleanup (void);
void RCS_rewrite (RCSNode *rcs, Deltatext *newdtext, char *insertpt, int compress_new_delta);
void RCS_abandon (RCSNode *);
int rcs_change_text (const char *, char *, size_t, const char *,
			    size_t, char **, size_t *);
void RCS_deltas (RCSNode *, FILE *, struct rcsbuffer *, char *,
			enum rcs_delta_op, char **, size_t *,
			char **, size_t *);
char *make_file_label (const char *, char *, RCSNode *);
void rcsbuf_cache_close (void);

/* From import.c.  */
extern int add_rcs_file(char *, char *, char *, char *, char *,
				char *, char *, int, char **,
				char *, size_t, FILE *, RCSCHECKINPROC);

RCSNode *RCS_fopen(const char *filename);
