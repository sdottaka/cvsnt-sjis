/*
 *    Copyright (c) 1992, Brian Berliner and Jeff Polk
 *    Copyright (c) 1989-1992, Brian Berliner
 *
 *    You may distribute under the terms of the GNU General Public License
 *    as specified in the README file that comes with the CVS source distribution.
 *
 * This is the main C driver for the CVS system.
 *
 * Credit to Dick Grune, Vrije Universiteit, Amsterdam, for writing
 * the shell-script CVS system that this is based on.
 *
 */

#include "cvs.h"
#include "library.h"
#include "config.h"

#ifdef CVSGUI_PIPE
#include "../cvsgui/cvsgui.h"
#endif

#ifndef HAVE_GETADDRINFO
#include "socket.h"
#include "getnameinfo.h"
#include "getaddrinfo.h"
#endif

#if defined(_WIN32)
  #include <winsock2.h>
  #include <ws2tcpip.h>
  #include <fcntl.h>
  #include <io.h>
  #define SET_BINARY_MODE(file) _setmode(fileno(file), _O_BINARY);
#else
  #include <sys/socket.h>
  #include <netdb.h>
  #define SET_BINARY_MODE(file) do { } while(0);
#endif


char *program_name;
char *program_path;
char *command_name;

char global_session_id[GLOBAL_SESSION_ID_LENGTH]; /* Random session ID */

/* I'd dynamically allocate this, but it seems like gethostname
   requires a fixed size array.  If I'm remembering the RFCs right,
   256 should be enough.  */
#ifndef MAXHOSTNAMELEN
#define MAXHOSTNAMELEN  256
#endif

char hostname[MAXHOSTNAMELEN];

int use_editor = 1;
int use_cvsrc = 1;
int cvswrite = !CVSREAD_DFLT;
int really_quiet = 0;
int quiet = 0;
int trace = 0;
int allow_trace = 0;
int noexec = 0;
int atomic_commits = 0;
int no_reverse_dns = 0;
int server_io_socket = 0;
int force_network_share = 0;

#ifdef SERVER_SUPPORT
/* This is set to request/force encryption/compression */
/* 0=Any, 1=Request auth., 2=Require auth., 3=Require Auth. Request Encr., 4=Request Encr., 5=Require Encr. */
extern int encryption_level; 
/* 0=Any, 1=Request compr., 2=Require compr. */
extern int compression_level;

#endif

/* Set if we should be writing CVSADM directories at top level.  At
   least for now we'll make the default be off (the CVS 1.9, not CVS
   1.9.2, behavior). */
int top_level_admin = 0;

#ifdef SJIS
int compat_2_0_14 = 1;
#endif

mode_t cvsumask = UMASK_DFLT;

char *CurDir;

/*
 * Defaults, for the environment variables that are not set
 */
char *Tmpdir = TMPDIR_DFLT;
char *Editor = EDITOR_DFLT;


/* When our working directory contains subdirectories with different
   values in CVS/Root files, we maintain a list of them.  */
List *root_directories = NULL;

/* We step through the above values.  This variable is set to reflect
 * the currently active value.
 *
 * Now static.  FIXME - this variable should be removable (well, localizable)
 * with a little more work.
 */
static char *current_root = NULL;

/* Hostname of peer, if known */
char *remote_host_name = NULL;

static const struct cmd
{
    char *fullname;		/* Full name of the function (e.g. "commit") */

    /* Synonyms for the command, nick1 and nick2.  We supply them
       mostly for two reasons: (1) CVS has always supported them, and
       we need to maintain compatibility, (2) if there is a need for a
       version which is shorter than the fullname, for ease in typing.
       Synonyms have the disadvantage that people will see "new" and
       then have to think about it, or look it up, to realize that is
       the operation they know as "add".  Also, this means that one
       cannot create a command "cvs new" with a different meaning.  So
       new synonyms are probably best used sparingly, and where used
       should be abbreviations of the fullname (preferably consisting
       of the first 2 or 3 or so letters).

       One thing that some systems do is to recognize any unique
       abbreviation, for example "annotat" "annota", etc., for
       "annotate".  The problem with this is that scripts and user
       habits will expect a certain abbreviation to be unique, and in
       a future release of CVS it may not be.  So it is better to
       accept only an explicit list of abbreviations and plan on
       supporting them in the future as well as now.  */

    char *nick1;
    char *nick2;
    
    int (*func) ();		/* Function takes (argc, argv) arguments. */
    unsigned long attr;		/* Attributes. */
} cmds[] =

{
    { "add",      "ad",       "new",       add,       CVS_CMD_MODIFIES_REPOSITORY | CVS_CMD_USES_WORK_DIR },
    { "admin",    "adm",      "rcs",       admin,     CVS_CMD_MODIFIES_REPOSITORY | CVS_CMD_USES_WORK_DIR },
    { "annotate", "ann",      NULL,        annotate,  CVS_CMD_USES_WORK_DIR },
#if defined(SERVER_SUPPORT)
    { "authserver",  "pserver",   NULL,     server,    CVS_CMD_MODIFIES_REPOSITORY | CVS_CMD_USES_WORK_DIR }, /* placeholder */
#endif
    { "chacl",    "setacl",   "setperm",   chacl,     CVS_CMD_USES_WORK_DIR | CVS_CMD_MODIFIES_REPOSITORY },
    { "checkout", "co",       "get",       checkout,  0 },
    { "chown", 	  "setowner", NULL,		   chowner,   CVS_CMD_USES_WORK_DIR | CVS_CMD_MODIFIES_REPOSITORY },
    { "commit",   "ci",       "com",       commit,    CVS_CMD_MODIFIES_REPOSITORY | CVS_CMD_USES_WORK_DIR },
    { "diff",     "di",       "dif",       diff,      CVS_CMD_USES_WORK_DIR },
    { "edit",     NULL,       NULL,        edit,      CVS_CMD_MODIFIES_REPOSITORY | CVS_CMD_USES_WORK_DIR },
    { "editors",  NULL,       NULL,        editors,   CVS_CMD_USES_WORK_DIR },
    { "export",   "exp",      "ex",        checkout,  CVS_CMD_USES_WORK_DIR },
    { "history",  "hi",       "his",       history,   CVS_CMD_USES_WORK_DIR },
    { "import",   "im",       "imp",       import,    CVS_CMD_MODIFIES_REPOSITORY | CVS_CMD_USES_WORK_DIR | CVS_CMD_IGNORE_ADMROOT},
    { "init",     NULL,       NULL,        init,      CVS_CMD_MODIFIES_REPOSITORY },
	{ "info",	  "inf",	  NULL,		   info, CVS_CMD_NO_CONNECT|CVS_CMD_OPTIONAL_ROOT  },
    { "log",      "lo",       NULL,        cvslog,    CVS_CMD_USES_WORK_DIR },
#ifdef CLIENT_SUPPORT
    { "login",    "logon",    "lgn",       login,     CVS_CMD_NO_CONNECT },
    { "logout",   NULL,       NULL,        logout,    CVS_CMD_NO_CONNECT },
#endif /* CLIENT_SUPPORT */
	{ "ls",		  "dir",      "list",	   ls,		  0 },
    { "lsacl", 	  "lsattr",   "listperm",  lsacl,     CVS_CMD_USES_WORK_DIR },
    { "passwd",  "password",  "setpass",   passwd,    CVS_CMD_USES_WORK_DIR | CVS_CMD_MODIFIES_REPOSITORY },
    { "rannotate","rann",     "ra",        annotate,  0 },
    { "rdiff",    "patch",    "pa",        patch,     0 },
    { "release",  "re",       "rel",       release,   0 },
    { "remove",   "rm",       "delete",    cvsremove, CVS_CMD_MODIFIES_REPOSITORY | CVS_CMD_USES_WORK_DIR },
	{ "rename",	  "ren",	  "mv",		   cvsrename, CVS_CMD_MODIFIES_REPOSITORY | CVS_CMD_USES_WORK_DIR },
	{ "rcsfile",  NULL,		  NULL,		   cvsrcs,	  CVS_CMD_NO_ROOT_NEEDED },
    { "rlog",     "rl",       NULL,        cvslog,    0 },
    { "rtag",     "rt",       "rfreeze",   cvstag,    CVS_CMD_MODIFIES_REPOSITORY },
#ifdef SERVER_SUPPORT
    { "server",   NULL,       NULL,        server,    CVS_CMD_MODIFIES_REPOSITORY | CVS_CMD_USES_WORK_DIR },
#endif
    { "status",   "st",       "stat",      cvsstatus, CVS_CMD_USES_WORK_DIR },
    { "tag",      "ta",       "freeze",    cvstag,    CVS_CMD_MODIFIES_REPOSITORY | CVS_CMD_USES_WORK_DIR },
    { "unedit",   NULL,       NULL,        unedit,    CVS_CMD_MODIFIES_REPOSITORY | CVS_CMD_USES_WORK_DIR },
    { "update",   "up",       "upd",       update,    CVS_CMD_USES_WORK_DIR },
    { "version",  "ve",       "ver",       version,   0 },
    { "watch",    NULL,       NULL,        watch,     CVS_CMD_MODIFIES_REPOSITORY | CVS_CMD_USES_WORK_DIR },
    { "watchers", NULL,       NULL,        watchers,  CVS_CMD_USES_WORK_DIR },
    { NULL, NULL, NULL, NULL, 0 },
};

static const char *const usg[] =
{
    /* CVS usage messages never have followed the GNU convention of
       putting metavariables in uppercase.  I don't know whether that
       is a good convention or not, but if it changes it would have to
       change in all the usage messages.  For now, they consistently
       use lowercase, as far as I know.  Puncutation is pretty funky,
       though.  Sometimes they use none, as here.  Sometimes they use
       single quotes (not the TeX-ish `' stuff), as in --help-options.
       Sometimes they use double quotes, as in cvs -H add.

       Most (not all) of the usage messages seem to have periods at
       the end of each line.  I haven't tried to duplicate this style
       in --help as it is a rather different format from the rest.  */

    "Usage: %s [cvs-options] command [command-options-and-arguments]\n",
    "  where cvs-options are -q, -n, etc.\n",
    "    (specify --help-options for a list of options)\n",
    "  where command is add, admin, etc.\n",
    "    (specify --help-commands for a list of commands\n",
    "     or --help-synonyms for a list of command synonyms)\n",
    "  where command-options-and-arguments depend on the specific command\n",
    "    (specify -H followed by a command name for command-specific help)\n",
    "  Specify --help to receive this message\n",
    "\n",

    /* Some people think that a bug-reporting address should go here.  IMHO,
       the web sites are better because anything else is very likely to go
       obsolete in the years between a release and when someone might be
       reading this help.  Besides, we could never adequately discuss
       bug reporting in a concise enough way to put in a help message.  */

    /* I was going to put this at the top, but usage() wants the %s to
       be in the first line.  */
    "The Concurrent Versions System (CVS) is a tool for version control.\n",
    /* I really don't think I want to try to define "version control"
       in one line.  I'm not sure one can get more concise than the
       paragraph in ../cvs.spec without assuming the reader knows what
       version control means.  */

    "For CVS updates and additional information, see\n",
	"    the CVSNT home page at http://www.cvsnt.org/\n",
    NULL,
};

static const char *const cmd_usage[] =
{
    "CVS commands are:\n",
    "        add          Add a new file/directory to the repository\n",
    "        admin        Administration front end for rcs\n",
    "        annotate     Show last revision where each line was modified\n",
    "        chacl        Change the Access Control List for a directory\n",
    "        checkout     Checkout sources for editing\n",
    "        chown        Change the owner of a directory\n",
    "        commit       Check files into the repository\n",
    "        diff         Show differences between revisions\n",
    "        edit         Get ready to edit a watched file\n",
    "        editors      See who is editing a watched file\n",
    "        export       Export sources from CVS, similar to checkout\n",
    "        history      Show repository access history\n",
    "        import       Import sources into CVS, using vendor branches\n",
    "        init         Create a CVS repository if it doesn't exist\n",
	"        info         Display information about supported protocols\n",
    "        log          Print out history information for files\n",
#ifdef CLIENT_SUPPORT
    "        login        Prompt for password for authenticating server\n",
    "        logout       Removes entry in .cvspass for remote repository\n",
#endif /* CLIENT_SUPPORT */
	"        ls           List files in the repository\n",
    "        lsacl        List the directories Access Control List\n",
    "        passwd       Set the user's password (Admin: Administer users)\n",
#if defined(SERVER_SUPPORT)
    "        authserver   Authentication server mode\n",
#endif
    "        rannotate    Show last revision where each line of module was modified\n",
    "        rdiff        Create 'patch' format diffs between releases\n",
    "        release      Indicate that a Module is no longer in use\n",
    "        remove       Remove an entry from the repository\n",
	"        rename       Rename a file or directory\n",
    "        rlog         Print out history information for a module\n",
    "        rtag         Add a symbolic tag to a module\n",
#ifdef SERVER_SUPPORT
    "        server       Server mode\n",
#endif
    "        status       Display status information on checked out files\n",
    "        tag          Add a symbolic tag to checked out version of files\n",
    "        unedit       Undo an edit command\n",
    "        update       Bring work tree in sync with repository\n",
    "        version      Show current CVS version(s)\n",
    "        watch        Set watches\n",
    "        watchers     See who is watching a file\n",
    "(Specify the --help option for a list of other help options)\n",
    NULL,
};

static const char *const opt_usage[] =
{
    /* Omit -b because it is just for compatibility.  */
    "CVS global options (specified before the command name) are:\n",
    "    -H              Displays usage information for command.\n",
    "    -Q              Cause CVS to be really quiet.\n",
    "    -q              Cause CVS to be somewhat quiet.\n",
    "    -r              Make checked-out files read-only.\n",
    "    -w              Make checked-out files read-write (default).\n",
    "    -n              Do not execute anything that will change the disk.\n",
    "    -t              Show trace of program execution (repeat for more verbosity) -- try with -n.\n",
    "    -v              CVS version and copyright.\n",
    "    -T tmpdir       Use 'tmpdir' for temporary files.\n",
    "    -e editor       Use 'editor' for editing log information.\n",
    "    -d CVS_root     Overrides $CVSROOT as the root of the CVS tree.\n",
    "    -f              Do not use the ~/.cvsrc file.\n",
	"    -F file         Read command arguments from file.\n",
#ifdef CLIENT_SUPPORT
    "    -z #            Use compression level '#' for net traffic.\n",
    "    -x              Encrypt all net traffic (fail if not encrypted).\n",
    "    -y              Encrypt all net traffic (if supported by protocol).\n",
	"    -a              Authenticate all net traffic.\n",
	"	 -N				 Supress network share error.\n",
#endif
    "    -s VAR=VAL      Set CVS user variable.\n",
	"\n",
    "    --version       CVS version and copyright.\n",
    "    --encrypt       Encrypt all net traffic (if supported by protocol).\n",
	"    --authenticate  Authenticate all net traffic (if supported by protocol).\n",
	"\n",
    "(Specify the --help option for a list of other help options)\n",
    NULL
};

static void append_args(const char *file, int *argc, char ***argv)
{
	FILE *f = fopen(file,"r");
	char **newargv;
	int newargc;
	char *buf;
	int buflen;

	if(!f)
		error(1,errno,"Couldn't open args file %s",file);
	fseek(f,0,SEEK_END);
	buflen = ftell(f);
	fseek(f,0,SEEK_SET);
	buf = (char*)xmalloc(buflen+1);
	buf[buflen]='\0';
	if(fread(buf,1,buflen,f)!=buflen)
		error(1,errno,"Couldn't read args file %s",file);

	line2argv(&newargc,&newargv,buf," \t\r\n");

	newargv=(char**)xrealloc(newargv,sizeof(char*)*(newargc+(*argc)));
	memmove(newargv+(*argc),newargv,sizeof(char*)*newargc);
	memcpy(newargv,*argv,sizeof(char*)*(*argc));
	newargc+=(*argc);
	*argc=newargc;
	*argv=newargv;
	xfree(buf);
}

static int
set_root_directory (p, ignored)
    Node *p;
    void *ignored;
{
    if (current_root == NULL && p->data == NULL)
    {
	current_root = p->key;
	return 1;
    }
    return 0;
}


static const char * const*
cmd_synonyms ()
{
    char ** synonyms;
    char ** line;
    const struct cmd *c = &cmds[0];
    /* Three more for title, "specify --help" line, and NULL.  */
    int numcmds = 3;

    while (c->fullname != NULL)
    {
	numcmds++;
	c++;
    }
    
    synonyms = (char **) xmalloc(numcmds * sizeof(char *));
    line = synonyms;
    *line++ = "CVS command synonyms are:\n";
    for (c = &cmds[0]; c->fullname != NULL; c++)
    {
	if (c->nick1 || c->nick2)
	{
	    *line = xmalloc (strlen (c->fullname)
			     + (c->nick1 != NULL ? strlen (c->nick1) : 0)
			     + (c->nick2 != NULL ? strlen (c->nick2) : 0)
			     + 40);
	    sprintf(*line, "        %-12s %s %s\n", c->fullname,
		    c->nick1 ? c->nick1 : "",
		    c->nick2 ? c->nick2 : "");
	    line++;
	}
    }
    *line++ = "(Specify the --help option for a list of other help options)\n";
    *line = NULL;
    
    return (const char * const*) synonyms; /* will never be freed */
}


unsigned long int
lookup_command_attribute (cmd_name)
     char *cmd_name;
{
    const struct cmd *cm;

    for (cm = cmds; cm->fullname; cm++)
    {
	if (strcmp (cmd_name, cm->fullname) == 0)
	    break;
    }

    if (!cm->fullname)
	{
		error (1, 0, "unknown command: %s", cmd_name);
	}

    return cm->attr;
}


static RETSIGTYPE
main_cleanup (sig)
    int sig;
{
#ifndef DONT_USE_SIGNALS
    const char *name;
    char temp[10];

    switch (sig)
    {
#ifdef SIGABRT
    case SIGABRT:
	name = "abort";
	break;
#endif
#ifdef SIGHUP
    case SIGHUP:
	name = "hangup";
	break;
#endif
#ifdef SIGINT
    case SIGINT:
	name = "interrupt";
	break;
#endif
#ifdef SIGQUIT
    case SIGQUIT:
	name = "quit";
	break;
#endif
#ifdef SIGPIPE
    case SIGPIPE:
	name = "broken pipe";
	break;
#endif
#ifdef SIGTERM
    case SIGTERM:
	name = "termination";
	break;
#endif
    default:
	/* This case should never be reached, because we list above all
	   the signals for which we actually establish a signal handler.  */
	sprintf (temp, "%d", sig);
	name = temp;
	break;
    }

    error (1, 0, "received %s signal", name);
#endif /* !DONT_USE_SIGNALS */
}

#if defined(WIN32) && defined(CVSGUI)
static void winUseDosLineFeed(int useDos)
{
   if(useDos)
	_fmode = _O_TEXT;
   else
	_fmode = _O_BINARY;
}
#endif

#ifdef SERVER_SUPPORT
void read_global_server_config(void)
{
	char token[1024];
	char buffer[MAX_PATH],buffer2[MAX_PATH];
	int n;

	if(!get_global_config_data("PServer","EncryptionLevel",buffer,sizeof(buffer)))
		encryption_level = atoi(buffer);

	if(!get_global_config_data("PServer","CompressionLevel",buffer,sizeof(buffer)))
		compression_level = atoi(buffer);
	
	if(!get_global_config_data("PServer","NoReverseDns",buffer,sizeof(buffer)))
		no_reverse_dns = atoi(buffer);

	if(!get_global_config_data("PServer","FakeUnixCvs",buffer,sizeof(buffer)))
		fake_unix_cvs = atoi(buffer);

	if(!get_global_config_data("PServer","LockServer",buffer,sizeof(buffer)))
	{
		xfree(lock_server);
		if(strcasecmp(buffer,"none"))
			lock_server = xstrdup(buffer);
		else
			lock_server = NULL;
	}
	else
		lock_server = xstrdup("127.0.0.1:2402");

	if(!get_global_config_data("PServer","EnableRename",buffer,sizeof(buffer)))
		can_rename = atoi(buffer);
	
	if(!get_global_config_data("PServer","Chroot",buffer,sizeof(buffer)))
		chroot_base = xstrdup(buffer);

	if(!get_global_config_data("PServer","RunAsUser",buffer,sizeof(buffer)))
		runas_user = xstrdup(buffer);

	if(!get_global_config_data("PServer","AllowTrace",buffer,sizeof(buffer)))
		allow_trace = atoi(buffer);

	n=0;
	while(!enum_global_config_data("PServer",n++,token,sizeof(token),buffer,sizeof(buffer)))
	{
		if(!strncasecmp(token,"Repository",10) && isdigit(token[10]) && strcasecmp(token+strlen(token)-4,"Name"))
		{
			char tmp[32];
			int prefixnum = atoi(token+10);
			snprintf(tmp,sizeof(tmp),"Repository%dName",prefixnum);
			if(get_global_config_data("PServer",tmp,buffer2,sizeof(buffer2)))
				strcpy(buffer2,buffer);
#ifdef SJIS
			if(*buffer && ISDIRSEPMB(buffer, buffer + strlen(buffer)-1))
#else
			if(*buffer && ISDIRSEP(buffer[strlen(buffer)-1]))
#endif
				buffer[strlen(buffer)-1]='\0';
#ifdef SJIS
			if(*buffer2 && ISDIRSEPMB(buffer2, buffer2 + strlen(buffer2)-1))
#else
			if(*buffer2 && ISDIRSEP(buffer2[strlen(buffer2)-1]))
#endif
				buffer[strlen(buffer)-1]='\0';
			root_allow_add(buffer,buffer2);
		}
	}
}
#endif

/* Calculate the cvs global session ID */
static void make_session_id()
{
	sprintf(global_session_id,"%x%08lx%04x",(int)getpid(),(long)time(NULL),rand()&0xFFFF);
}

int main (int argc, char **argv)
{
    char *CVSroot = CVSROOT_DFLT;
    char *cp, *end;
    const struct cmd *cm;
    int c, err = 0;
    int tmpdir_update_env, cvs_update_env;
    int free_CVSroot = 0;
    int free_Editor = 0;
    int free_Tmpdir = 0;
	int testserver = 0;
	char *append_file = NULL;
    int seen_root;

    int help = 0;		/* Has the user asked for help?  This
				   lets us support the `cvs -H cmd'
				   convention to give help for cmd. */
	static const char short_options[] = "+Qqrwtnlvb:T:e:d:HfF:z:s:axyN";
    static struct option long_options[] =
    {
		{"help", 0, NULL, 'H'},
		{"version", 0, NULL, 'v'},
		{"encrypt", 0, NULL, 'x'},
		{"authenticate", 0, NULL, 'a'},
		{"help-commands", 0, NULL, 1},
		{"help-synonyms", 0, NULL, 2},
		{"help-options", 0, NULL, 4},
		{"allow-root", required_argument, NULL, 3},
#if defined(WIN32) && defined(CVSGUI)
		{"crlf", 0, NULL, 5},
		{"lf", 0, NULL, 6},
#endif
#if defined(SERVER_SUPPORT)
		{"testserver", 0, NULL, 7 },
#endif
#if defined(WIN32) && defined(SERVER_SUPPORT)
		{"win32_socket_io", required_argument, NULL, 8 },
#endif
        {0, 0, 0, 0}
    };
    /* `getopt_long' stores the option index here, but right now we
        don't use it. */
    int option_index = 0;

#ifdef SYSTEM_INITIALIZE
    /* Hook for OS-specific behavior, for example socket subsystems on
       NT and OS2 or dealing with windows and arguments on Mac.  */
	/* First called with -1 for 'preinitialisation' */
    SYSTEM_INITIALIZE (-1, &argc, &argv);
#endif

	srand(time(NULL));

#ifdef HAVE_TZSET
    /* On systems that have tzset (which is almost all the ones I know
       of), it's a good idea to call it.  */
    tzset ();
#endif

    /*
     * Just save the last component of the path for error messages
     */
    program_path = xstrdup (argv[0]);
    #ifdef _WIN32
	tidy_path(program_path);
    #endif
#ifdef ARGV0_NOT_PROGRAM_NAME
    /* On some systems, e.g. VMS, argv[0] is not the name of the command
       which the user types to invoke the program.  */
    program_name = "cvs";
#else
    program_name = (char*)last_component (argv[0]);
#ifdef _WIN32
	if(strlen(program_name)>4 && !stricmp(program_name+strlen(program_name)-4,".exe"))
		program_name[strlen(program_name)-4]='\0';
#endif
#endif

    /*
     * Query the environment variables up-front, so that
     * they can be overridden by command line arguments
     */
    cvs_update_env = 0;
    tmpdir_update_env = *Tmpdir;	/* TMPDIR_DFLT must be set */

    if ((cp = getenv (TMPDIR_ENV)) != NULL)
    {
	Tmpdir = cp;
	tmpdir_update_env = 0;		/* it's already there */
    }
    if ((cp = getenv (EDITOR1_ENV)) != NULL)
 		Editor = cp;
    else if ((cp = getenv (EDITOR2_ENV)) != NULL)
		Editor = cp;
    else if ((cp = getenv (EDITOR3_ENV)) != NULL)
		Editor = cp;
    if ((cp = getenv (CVSROOT_ENV)) != NULL)
    {
		CVSroot = cp;
		cvs_update_env = 0;		/* it's already there */
    }
    if (getenv (CVSREAD_ENV) != NULL)
		cvswrite = 0;

    /* Set this to 0 to force getopt initialization.  getopt() sets
       this to 1 internally.  */
    optind = 0;

    /* We have to parse the options twice because else there is no
       chance to avoid reading the global options from ".cvsrc".  Set
       opterr to 0 for avoiding error messages about invalid options.
       */
    opterr = 0;

    while ((c = getopt_long
            (argc, argv, short_options, long_options, &option_index))
           != EOF)
    {
		if (c == 'f')
			use_cvsrc = 0;
    }

    /*
     * Scan cvsrc file for global options.
     */
    if (use_cvsrc)
		read_cvsrc (&argc, &argv, "cvs");

#if defined(WIN32) && defined(CVSGUI)
    /* in order to checkout the text files with the Unix Line Feed
    (usefull for Cygwin users). */
    if(getenv("CVSUSEUNIXLF") != 0L)
	winUseDosLineFeed(0);
#endif

    optind = 0;
    opterr = 1;

    while ((c = getopt_long
            (argc, argv, short_options, long_options, &option_index))
           != EOF)
    {
	switch (c)
	{
#if defined(WIN32) && defined(SERVER_SUPPORT)
	case 8:
		/* --win32-socket-io */
		{
			long l;
			sscanf(optarg,"%ld",&l);
			server_io_socket=win32_makepipe(l);
		}
		break;
#endif
#if defined(WIN32) && defined(CVSGUI)
            case 5:
	        /* --crlf */
                winUseDosLineFeed(1);
                break;
            case 6:
	        /* --lf */
                winUseDosLineFeed(0);
                break;
#endif
#if defined(SERVER_SUPPORT)
			case 7:
				testserver = 1;
				break;
#endif
            case 1:
	        /* --help-commands */
                usage (cmd_usage);
                break;
            case 2:
	        /* --help-synonyms */
                usage (cmd_synonyms());
                break;
	    case 4:
		/* --help-options */
		usage (opt_usage);
		break;
	    case 3:
		{
			/* --allow-root */
			char *p = strchr(optarg,',');
			if(p)
				*(p++)='\0';
			else
				p=optarg;
			root_allow_add (optarg,p);
		}
		break;
	    case 'Q':
		really_quiet = 1;
		/* FALL THROUGH */
	    case 'q':
		quiet = 1;
		break;
	    case 'r':
		cvswrite = 0;
		break;
	    case 'w':
		cvswrite = 1;
		break;
	    case 't':
		trace++;
		break;
	    case 'n':
		noexec = 1;
		break;
		case 'N':
		force_network_share = 1;
		break;
	    case 'l':
		/* Ignore global -l */
		break;
	    case 'v':
		printf ("\n");
		version (0, (char **) NULL); 
		fflush(stdout);
		printf ("\n");
		printf ("\
Copyright (c) 1989-2001 Brian Berliner, david d `zoo' zuhn, \n\
                        Jeff Polk, and other authors\n");
 		printf ("CVSNT version ("__DATE__") Copyright (c) 1999-2004 Tony Hoyle and others\n");
		printf ("see http://www.cvsnt.org\n",stdout);
		printf ("\n");
		printf ("CVS may be copied only under the terms of the GNU General Public License,\n");
		printf ("a copy of which can be found with the CVS distribution kit.\n");
		printf ("\n");

		printf ("SSH connectivity provided by PuTTY:\n");
		printf ("  PuTTY is copyright 1997-2001 Simon Tatham.\n");
		printf ("  Portions copyright Robert de Bath, Joris van Rantwijk, Delian\n");
		printf ("  Delchev, Andreas Schultz, Jeroen Massar, Wez Furlong, Nicolas Barry,\n");
		printf ("  Justin Bradford, and CORE SDI S.A.\n");
		printf ("  see http://www.chiark.greenend.org.uk/~sgtatham/putty/\n");
		printf ("\n");

		printf ("Specify the --help option for further information about CVS\n");

		exit (0);
		break;
	    case 'b':
		/* This option used to specify the directory for RCS
		   executables.  But since we don't run them any more,
		   this is a noop.  Silently ignore it so that .cvsrc
		   and scripts and inetd.conf and such can work with
		   either new or old CVS.  */
		break;
	    case 'T':
		Tmpdir = xstrdup (optarg);
		free_Tmpdir = 1;
		tmpdir_update_env = 1;	/* need to update environment */
		break;
	    case 'e':
		Editor = xstrdup (optarg);
		free_Editor = 1;
		break;
	    case 'd':
		if (CVSroot_cmdline != NULL)
		    xfree (CVSroot_cmdline);
		CVSroot_cmdline = xstrdup (optarg);
		if (free_CVSroot)
		    xfree (CVSroot);
		CVSroot = xstrdup (optarg);
		free_CVSroot = 1;
		cvs_update_env = 1;	/* need to update environment */
		break;
	    case 'H':
	        help = 1;
		break;
            case 'f':
		use_cvsrc = 0; /* unnecessary, since we've done it above */
		break;
		case 'F':
			append_file = xstrdup(optarg);
			break;
	    case 'z':
#ifdef CLIENT_SUPPORT
		gzip_level = atoi (optarg);
		if (gzip_level < 0 || gzip_level > 9)
		  error (1, 0,
			 "gzip compression level must be between 0 and 9");
#endif
		/* If no CLIENT_SUPPORT, we just silently ignore the gzip
		   level, so that users can have it in their .cvsrc and not
		   cause any trouble.  */
		break;
	    case 's':
		variable_set (optarg);
		break;
	    case 'x':
#ifdef CLIENT_SUPPORT
			if(cvsauthenticate)
				error(1, 0,
					"Cannot have both -a and -x at the same time");
	        cvsencrypt = 1;
#endif /* CLIENT_SUPPORT */
		/* If no CLIENT_SUPPORT, ignore -x, so that users can
                   have it in their .cvsrc and not cause any trouble.
                   If no ENCRYPTION, we still accept -x, but issue an
                   error if we are being run as a client.  */
		break;
	    case 'y':
#ifdef CLIENT_SUPPORT
			if(cvsauthenticate)
				error(1, 0,
					"Cannot have both -a and -y at the same time");
	        cvsencrypt = -1;
#endif /* CLIENT_SUPPORT */
		/* If no CLIENT_SUPPORT, ignore -x, so that users can
                   have it in their .cvsrc and not cause any trouble.
                   If no ENCRYPTION, we still accept -x, but issue an
                   error if we are being run as a client.  */
		break;
	    case 'a':
#ifdef CLIENT_SUPPORT
			if(cvsencrypt)
				error(1, 0,
					"Cannot have both -a and -x/-y at the same time");
	        cvsauthenticate = 1;
#endif /* CLIENT_SUPPORT */
		/* If no CLIENT_SUPPORT, ignore -a, so that users can
                   have it in their .cvsrc and not cause any trouble.
                   If no ENCRYPTION, we still accept -x, but issue an
                   error if we are being run as a client.  */
		break;
	    case '?':
	    default:
                usage (usg);
	}
    }

    argc -= optind;
    argv += optind;

	if(append_file)
		append_args(append_file,&argc,&argv);

    if (argc < 1)
		usage (usg);

	make_session_id(); /* Calculate the cvs global session ID */

	TRACE(1,"Tracelevel set to %d.  PID is %d",trace,(int)getpid());
	TRACE(1,"Session ID is %s",PATCH_NULL(global_session_id));

    /* Look up the command name. */

    command_name = argv[0];
    for (cm = cmds; cm->fullname; cm++)
    {
	if (cm->nick1 && !strcmp (command_name, cm->nick1))
	    break;
	if (cm->nick2 && !strcmp (command_name, cm->nick2))
	    break;
	if (!strcmp (command_name, cm->fullname))
	    break;
    }

    if (!cm->fullname)
    {
	fprintf (stderr, "Unknown command: `%s'\n\n", command_name);
	usage (cmd_usage);
    }
    else
	command_name = cm->fullname;	/* Global pointer for later use */

    if (help)
    {
	argc = -1;		/* some functions only check for this */
	err = (*(cm->func)) (argc, argv);
    }
    else
    {
	/* The user didn't ask for help, so go ahead and authenticate,
           set up CVSROOT, and the rest of it. */

	/* The UMASK environment variable isn't handled with the
	   others above, since we don't want to signal errors if the
	   user has asked for help.  This won't work if somebody adds
	   a command-line flag to set the umask, since we'll have to
	   parse it before we get here. */

	if ((cp = getenv (CVSUMASK_ENV)) != NULL)
	{
	    /* FIXME: Should be accepting symbolic as well as numeric mask.  */
	    cvsumask = strtol (cp, &end, 8) & 0777;
	    if (*end != '\0')
		error (1, errno, "invalid umask value in %s (%s)",
		       CVSUMASK_ENV, cp);
	}

#if defined(SERVER_SUPPORT) 
	if (strcmp (command_name, "authserver") == 0)
	{
	    /* Pretend we were invoked as a plain server.  */
	    command_name = "server";
		pserver_active = 1;
	}
#endif /* SERVER_SUPPORT  */

#ifdef SERVER_SUPPORT
	server_active = strcmp (command_name, "server") == 0;
	
	if(server_active)
		lock_server = xstrdup("127.0.0.1:2402"); /* default lockserver */
	if(server_active)
	{
		struct sockaddr_storage ss = {0};
		int ss_len = sizeof(ss);
		int flags;

		read_global_server_config();

		if(no_reverse_dns)
		  flags = NI_NUMERICHOST;
		else
		  flags = 0;

#ifdef _WIN32
		if(!getpeername(_get_osfhandle(server_io_socket),(struct sockaddr*)&ss,&ss_len))
#else
		if(!getpeername(0,(struct sockaddr*)&ss,&ss_len))
#endif
		{
			char host[NI_MAXHOST];

			if(!getnameinfo((struct sockaddr*)&ss,ss_len,host,NI_MAXHOST,NULL,0,flags))
				remote_host_name = xstrdup(host);
		}
		else
		{
			remote_host_name=NULL;
		}
	}
#endif

#if defined(_WIN32) && defined(SERVER_SUPPORT)
	if(server_active)
	{
		_fmode = _O_BINARY; /* In Win32 server pretend we are unix by setting the default to binary */
		/* we must assume that the connection is expected to be binary. */
		if(!server_io_socket)
		{
			fflush(stdout);
			fflush(stdin);
			fflush(stderr);
			if(!testserver)
			{
				SET_BINARY_MODE(stdout);
				SET_BINARY_MODE(stdin);
				SET_BINARY_MODE(stderr);
			}
		}
	}
#endif

	/* This is only used for writing into the history file.  For
	   remote connections, it might be nice to have hostname
	   and/or remote path, on the other hand I'm not sure whether
	   it is worth the trouble.  */

	if (server_active)
		CurDir = xstrdup (remote_host_name?remote_host_name:"<remote>");
	else
	{
	    CurDir = xgetwd ();
            if (CurDir == NULL)
		error (1, errno, "cannot get working directory");
	}

	if (Tmpdir == NULL || Tmpdir[0] == '\0')
	    Tmpdir = "/tmp";

#ifdef HAVE_PUTENV
	if (tmpdir_update_env)
		cvs_putenv(TMPDIR_ENV, Tmpdir);
#endif

#ifndef DONT_USE_SIGNALS
	/* make sure we clean up on error */
#ifdef SIGABRT
	(void) SIG_register (SIGABRT, main_cleanup);
#endif
#ifdef SIGHUP
	(void) SIG_register (SIGHUP, main_cleanup);
#endif
#ifdef SIGINT
	(void) SIG_register (SIGINT, main_cleanup);
#endif
#ifdef SIGQUIT
	(void) SIG_register (SIGQUIT, main_cleanup);
#endif
#ifdef SIGPIPE
	(void) SIG_register (SIGPIPE, main_cleanup);
#endif
#ifdef SIGTERM
	(void) SIG_register (SIGTERM, main_cleanup);
#endif
#endif /* !DONT_USE_SIGNALS */

#ifdef SYSTEM_INITIALIZE
    /* Hook for OS-specific behavior, for example socket subsystems on
       NT and OS2 or dealing with windows and arguments on Mac.  */
	/* Second call - server mode is known */
    SYSTEM_INITIALIZE (server_active, &argc, &argv);
#endif

	gethostname(hostname, sizeof (hostname));

#if defined(SERVER_SUPPORT) 
	if(pserver_active)
	{
	    /* Gets username and password from client, authenticates, then
	       switches to run as that user and sends an ACK back to the
	       client. */
	    server_authenticate_connection ();
    }
#endif
	if(!CVS_Username)
	{
		/* In command-line server mode we have to use the system username */
		CVS_Username = xstrdup(getcaller());
	}

	/* Fiddling with CVSROOT doesn't make sense if we're running
	       in server mode, since the client will send the repository
	       directory after the connection is made. */

	if (!server_active)
	{
	    char *CVSADM_Root;
	    
	    /* See if we are able to find a 'better' value for CVSroot
	       in the CVSADM_ROOT directory. */

	    CVSADM_Root = NULL;

	    /* "cvs import" shouldn't check CVS/Root; in general it
	       ignores CVS directories and CVS/Root is likely to
	       specify a different repository than the one we are
	       importing to.  */

	    if (!(cm->attr & CVS_CMD_IGNORE_ADMROOT)

		/* -d overrides CVS/Root, so don't give an error if the
		   latter points to a nonexistent repository.  */
		&& CVSroot_cmdline == NULL)
	    {
		CVSADM_Root = Name_Root((char *) NULL, (char *) NULL);
	    }

	    if (CVSADM_Root != NULL)
	    {
		if (CVSroot == NULL || !cvs_update_env)
		{
		    CVSroot = CVSADM_Root;
		    cvs_update_env = 1;	/* need to update environment */
			free_CVSroot = 1;
		}
	    }

	    /* Now we've reconciled CVSROOT from the command line, the
	       CVS/Root file, and the environment variable.  Do the
	       last sanity checks on the variable. */

	    if (!(cm->attr & CVS_CMD_NO_ROOT_NEEDED))
		{
			if (!(cm->attr & CVS_CMD_OPTIONAL_ROOT) && !CVSroot)
			{
			error (0, 0,
				"No CVSROOT specified!  Please use the `-d' option");
			error (1, 0,
				"or set the %s environment variable.", CVSROOT_ENV);
			}
		    
			if (CVSroot && !*CVSroot)
			{
			error (0, 0,
				"CVSROOT is set but empty!  Make sure that the");
			error (0, 0,
				"specification of CVSROOT is legal, either via the");
			error (0, 0,
				"`-d' option, the %s environment variable, or the",
				CVSROOT_ENV);
			error (1, 0,
				"CVS/Root file (if any).");
			}
	    }
	}

	/* Here begins the big loop over unique cvsroot values.  We
           need to call do_recursion once for each unique value found
           in CVS/Root.  Prime the list with the current value. */

	/* Create the list. */
	assert (root_directories == NULL);
	root_directories = getlist ();

	/* Prime it. */
	if (CVSroot != NULL)
	{
	    Node *n;
	    n = getnode ();
	    n->type = NT_UNKNOWN;
	    n->key = xstrdup (CVSroot);
	    n->data = NULL;

	    if (addnode (root_directories, n))
		error (1, 0, "cannot add initial CVSROOT %s", n->key);
	}

	assert (current_root == NULL);

	/* If we're running the server, we want to execute this main
	   loop once and only once (we won't be serving multiple roots
	   from this connection, so there's no need to do it more than
	   once).  To get out of the loop, we perform a "break" at the
	   end of things.  */

	seen_root = 0;
	while (
	       server_active || (cm->attr & CVS_CMD_NO_ROOT_NEEDED) ||
	       walklist (root_directories, set_root_directory, NULL) ||
	       (!seen_root && (cm->attr & CVS_CMD_OPTIONAL_ROOT) && !current_root) 
	       )
	{
	    /* Fiddling with CVSROOT doesn't make sense if we're running
	       in server mode, since the client will send the repository
	       directory after the connection is made. */

	    if (!server_active && !(cm->attr & CVS_CMD_NO_ROOT_NEEDED) && !((cm->attr & CVS_CMD_OPTIONAL_ROOT) && !current_root))
	    {
		/* Now we're 100% sure that we have a valid CVSROOT
		   variable.  Parse it to see if we're supposed to do
		   remote accesses or use a special access method. */

		seen_root = 1;
		if (current_parsed_root != NULL)
		    free_cvsroot_t (current_parsed_root);
		if ((current_parsed_root = parse_cvsroot (current_root)) == NULL)
		    error (1, 0, "Bad CVSROOT.");

		TRACE(1,"main loop with CVSROOT=%s",PATCH_NULL(current_root));

		/*
		 * Check to see if the repository exists.
		 */
#ifdef CLIENT_SUPPORT
		if (!current_parsed_root->isremote)
#endif	/* CLIENT_SUPPORT */
		{
		    char *path;
		    int save_errno;

		    path = xmalloc (strlen (current_parsed_root->directory)
				    + sizeof (CVSROOTADM)
				    + 2);
		    (void) sprintf (path, "%s/%s", current_parsed_root->directory, CVSROOTADM);
		    if (!isaccessible (path, R_OK | X_OK))
		    {
			save_errno = errno;
			/* If this is "cvs init", the root need not exist yet.  */
			if (strcmp (command_name, "init") != 0)
			{
			    error (1, save_errno, "%s", path);
			}
		    }
		    xfree (path);
		}

#ifdef HAVE_PUTENV
		/* Update the CVSROOT environment variable if necessary. */
		/* FIXME (njc): should we always set this with the CVSROOT from the command line? */
		if (cvs_update_env)
			cvs_putenv(CVSROOT_ENV,CVSroot);
#endif
	    }
	
	    /* Parse the CVSROOT/config file, but only for local.  For the
	       server, we parse it after we know $CVSROOT.  For the
	       client, it doesn't get parsed at all, obviously.  The
	       presence of the parse_config call here is not mean to
	       predetermine whether CVSROOT/config overrides things from
	       read_cvsrc and other such places or vice versa.  That sort
	       of thing probably needs more thought.  */
	    if (!server_active && !(cm->attr & CVS_CMD_NO_ROOT_NEEDED) && !((cm->attr & CVS_CMD_OPTIONAL_ROOT) && !current_parsed_root)
#ifdef CLIENT_SUPPORT
		&& !current_parsed_root->isremote
#endif
		)
	    {
		/* If there was an error parsing the config file, parse_config
		   already printed an error.  We keep going.  Why?  Because
		   if we didn't, then there would be no way to check in a new
		   CVSROOT/config file to fix the broken one!  */
		parse_config (current_parsed_root->directory);
#ifdef SJIS
		if (compat_2_0_14)
			force_network_share = 1;
#endif
#ifdef _WIN32
		if(!force_network_share && w32_is_network_share(current_parsed_root->directory))
		{
			error(1,0,"Local access to network share not supported (Use -N to override this error).");
	    }

		if(!server_active && !lock_server && !local_lockserver() && !w32_is_network_share(current_parsed_root->directory))
		{
			HINSTANCE hInst;
			
			TRACE(3,"Attempting to start local lockserver");
			hInst = ShellExecute(NULL,"open","cvslock.exe","-systray",NULL,SW_HIDE);
			if(hInst)
			{
				WaitForInputIdle(hInst,INFINITE);
				Sleep(200);
			}
		}
#endif

		/* Try to use the lockserver if we can */
		if(!lock_server && local_lockserver())
		{
			lock_server = xstrdup("127.0.0.1:2402");
			TRACE(3,"Using local lockserver on port 2402");
		}

		lock_register_client(getcaller(),current_parsed_root->directory);
		}

#ifdef CLIENT_SUPPORT
	    /* Need to check for current_parsed_root != NULL here since
	     * we could still be in server mode before the server function
	     * gets called below and sets the root
	     */
	    if (current_parsed_root != NULL && current_parsed_root->isremote)
	    {
			/* Create a new list for directory names that we've
			sent to the server. */
			if (dirs_sent_to_server != NULL)
				dellist (&dirs_sent_to_server);
			dirs_sent_to_server = getlist ();

			/* Start the remote server.  Starting here allows us to interrogate the
			server for state information (cvsrc, cvswrappers) prior to executing
			commands */
			if(!(cm->attr&CVS_CMD_NO_CONNECT))
				start_server(0);
	    }
		if(current_parsed_root != NULL)
		{
			ign_setup();
			wrap_setup();
		}
#else
		ign_setup();
		wrap_setup();
#endif

		if (use_cvsrc)
			read_cvsrc (&argc, &argv, command_name);

	    err = (*(cm->func)) (argc, argv);
	
		shutdown_infolib(); /* This command has finished */
	
	    /* Mark this root directory as done.  When the server is
               active, current_root will be NULL -- don't try and
               remove it from the list. */

	    if (current_root != NULL)
	    {
		Node *n = findnode_fn (root_directories, current_root);
		assert (n != NULL);
		n->data = (void *) 1;
	    }
	
	    dellist (&root_directories);

	    if (server_active || (cm->attr & CVS_CMD_NO_ROOT_NEEDED) || ((cm->attr & CVS_CMD_OPTIONAL_ROOT) && !current_root))
	    {
	      current_root = NULL;
	      break;
	    }
	    current_root = NULL;
	} /* end of loop for cvsroot values */

    } /* end of stuff that gets done if the user DOESN'T ask for help */

    Lock_Cleanup ();

    xfree (program_path);
	xfree (CVSroot_cmdline);
    if (free_CVSroot)
		xfree (CVSroot);
    if (free_Editor)
		xfree (Editor);
    if (free_Tmpdir)
		xfree (Tmpdir);
    root_allow_free ();

	free_cvsroot_t (current_parsed_root);

	perms_close();
	free_modules2();
	wrap_close();
	ign_close();
	freenodecache();
	freelistcache();
	close_cvsrc();
	xfree(lock_server);
	xfree(CVS_Username);

#ifdef SYSTEM_CLEANUP
    /* Hook for OS-specific behavior, for example socket subsystems on
       NT and OS2 or dealing with windows and arguments on Mac.  */
    SYSTEM_CLEANUP ();
#endif

    return err ? EXIT_FAILURE : 0;
}

char *Make_Date (char *rawdate)
{
    time_t unixtime;
	int y,m,d,h,mm,s;

	/* If the date is already in internal format, just return */
	if(sscanf(rawdate,"%04d.%02d.%02d.%02d.%02d.%02d",&y,&m,&d,&h,&mm,&s)==6)
		return xstrdup(rawdate);
    unixtime = get_date (rawdate, (struct timeb *) NULL);
    if (unixtime == (time_t) - 1)
	error (1, 0, "Can't parse date/time: %s", rawdate);
    return date_from_time_t (unixtime);
}

/* Convert a time_t to an RCS format date.  This is mainly for the
   use of "cvs history", because the CVSROOT/history file contains
   time_t format dates; most parts of CVS will want to avoid using
   time_t's directly, and instead use RCS_datecmp, Make_Date, &c.
   Assuming that the time_t is in GMT (as it generally should be),
   then the result will be in GMT too.

   Returns a newly malloc'd string.  */

char *
date_from_time_t (unixtime)
    time_t unixtime;
{
    struct tm *ftm;
    char date[MAXDATELEN];
    char *ret;

    ftm = gmtime (&unixtime);
    if (ftm == NULL)
	/* This is a system, like VMS, where the system clock is in local
	   time.  Hopefully using localtime here matches the "zero timezone"
	   hack I added to get_date (get_date of course being the relevant
	   issue for Make_Date, and for history.c too I think).  */
	ftm = localtime (&unixtime);

    (void) sprintf (date, DATEFORM,
		    ftm->tm_year + (ftm->tm_year < 100 ? 0 : 1900),
		    ftm->tm_mon + 1, ftm->tm_mday, ftm->tm_hour,
		    ftm->tm_min, ftm->tm_sec);
    ret = xstrdup (date);
    return (ret);
}

/* Convert a date to RFC822/1123 format.  This is used in contexts like
   dates to send in the protocol; it should not vary based on locale or
   other such conventions for users.  We should have another routine which
   does that kind of thing.

   The SOURCE date is in our internal RCS format.  DEST should point to
   storage managed by the caller, at least MAXDATELEN characters.  */
void
date_to_internet (dest, source)
    char *dest;
    const char *source;
{
    struct tm date;

    date_to_tm (&date, source);
    tm_to_internet (dest, &date);
}

int date_to_tm (struct tm *dest, const char *source)
{
    if (sscanf (source, SDATEFORM,
		&dest->tm_year, &dest->tm_mon, &dest->tm_mday,
		&dest->tm_hour, &dest->tm_min, &dest->tm_sec)
	    != 6)
	return 1;

    if (dest->tm_year > 100)
	dest->tm_year -= 1900;

    dest->tm_mon -= 1;
    return 0;
}

/* Convert a date to RFC822/1123 format.  This is used in contexts like
   dates to send in the protocol; it should not vary based on locale or
   other such conventions for users.  We should have another routine which
   does that kind of thing.

   The SOURCE date is a pointer to a struct tm.  DEST should point to
   storage managed by the caller, at least MAXDATELEN characters.  */
void
tm_to_internet (dest, source)
    char *dest;
    const struct tm *source;
{
    /* Just to reiterate, these strings are from RFC822 and do not vary
       according to locale.  */
    static const char *const month_names[] =
      {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
	 "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
    
    sprintf (dest, "%d %s %d %02d:%02d:%02d -0000", source->tm_mday,
	     source->tm_mon < 0 || source->tm_mon > 11 ? "???" : month_names[source->tm_mon],
	     source->tm_year + 1900, source->tm_hour, source->tm_min, source->tm_sec);
}

void usage (const char *const cpp[])
{
    (void) fprintf (stderr, *cpp++, program_name, command_name);
    for (; *cpp; cpp++)
	(void) fprintf (stderr, *cpp);
    error_exit ();
}
