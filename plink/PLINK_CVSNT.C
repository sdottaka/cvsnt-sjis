/*
 * PLink - a command-line (stdin/stdout) variant of PuTTY.
 *
 * Modified for using as CVSNT Protocol
 *
 */

#ifndef AUTO_WINSOCK
#include <winsock2.h>
#endif
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdarg.h>
#include <process.h>

#define PUTTY_DO_GLOBALS	       /* actually _define_ globals */
#define PLINK_EXPORT __declspec(dllexport)
#include "putty/putty.h"
#include "putty/storage.h"
#include "putty/tree234.h"

#include "plink_cvsnt.h"

extern int fatal_exit;
extern int fatal_exit_code;

#define MAX_STDIN_BACKLOG 4096

static Backend *back;
static void *backhandle;
static Config cfg;
extern putty_callbacks *callbacks;

void fatalbox(char *p, ...)
{
    va_list ap;
    fprintf(stderr, "FATAL ERROR: ");
    va_start(ap, p);
    vfprintf(stderr, p, ap);
    va_end(ap);
    fputc('\n', stderr);
    cleanup_exit(1);
}

void modalfatalbox(char *p, ...)
{
    va_list ap;
    fprintf(stderr, "FATAL ERROR: ");
    va_start(ap, p);
    vfprintf(stderr, p, ap);
    va_end(ap);
    fputc('\n', stderr);
    cleanup_exit(1);
}

void connection_fatal(void *frontend, char *p, ...)
{
    va_list ap;
    fprintf(stderr, "FATAL ERROR: ");
    va_start(ap, p);
    vfprintf(stderr, p, ap);
    va_end(ap);
    fputc('\n', stderr);
    cleanup_exit(1);
}
void cmdline_error(char *p, ...)
{
    va_list ap;
    fprintf(stderr, "plink: ");
    va_start(ap, p);
    vfprintf(stderr, p, ap);
    va_end(ap);
    fputc('\n', stderr);
    exit(1);
}

static char *password = NULL;

HANDLE inhandle, outhandle, errhandle;
DWORD orig_console_mode;
HANDLE handles[4];

WSAEVENT netevent;

char *cmdline_password = NULL;

static int cmdline_get_line(const char *prompt, char *str,
                            int maxlen, int is_pw)
{
    static int tried_once = 0;

    assert(is_pw && cmdline_password);

    if (tried_once) {
	return 0;
    } else {
	strncpy(str, cmdline_password, maxlen);
	str[maxlen - 1] = '\0';
	tried_once = 1;
	return 1;
    }
}

int term_ldisc(Terminal *term, int mode)
{
    return FALSE;
}

void ldisc_update(void *frontend, int echo, int edit)
{
    /* Update stdin read mode to reflect changes in line discipline. */
    DWORD mode;

    mode = ENABLE_PROCESSED_INPUT;
    if (echo)
	mode = mode | ENABLE_ECHO_INPUT;
    else
	mode = mode & ~ENABLE_ECHO_INPUT;
    if (edit)
	mode = mode | ENABLE_LINE_INPUT;
    else
	mode = mode & ~ENABLE_LINE_INPUT;
    SetConsoleMode(inhandle, mode);
}

struct input_data {
    DWORD len;
    char buffer[4096];
    HANDLE event, eventback;
};

static struct input_data *global_idata;

int plink_write_data(const void *buffer, int length)
{
    struct input_data *idata = global_idata;
	int l;

	if(fatal_exit)
		return -1;

	for(l=length; l>0; l-=sizeof(idata->buffer))
	{
		idata->len = (length>sizeof(idata->buffer))?sizeof(idata->buffer):length;
		memcpy(idata->buffer,buffer,idata->len);
		SetEvent(idata->event);
		WaitForSingleObject(idata->eventback, INFINITE);
	}
	return length;
}

struct output_data {
    DWORD len, lenwritten;
    int writeret;
    char *buffer;
    int is_stderr, done;
    HANDLE event, eventback;
    int busy,wrap;
};

static struct output_data *global_odata;

int plink_read_data(void *buffer, int max_length)
{
    struct output_data *odata = global_odata;
	int todo = max_length,l;
	char *ptr = buffer;

	if(fatal_exit)
		return -1;

	if(odata->wrap)
	{
		l = odata->len - odata->lenwritten;

		if(l <= todo)
		{
			todo -= l;
			memcpy(ptr,odata->buffer+odata->lenwritten,l);
			ptr+=l;
			odata->lenwritten=odata->len;
			odata->writeret = odata->lenwritten;
			odata->wrap=0;
			SetEvent(odata->event);
			if(!todo)
				return 0;
		}
		else
		{
			/* Actually, I'm not sure that this case ever happens.. */
			memcpy(ptr,odata->buffer+l,todo);
			ptr+=todo;
			odata->lenwritten+=todo;
			return 0;
		}
	}
			
	WaitForSingleObject(odata->eventback, INFINITE);
	while(todo)
	{
		if (odata->done)
			break;
		
		if(odata->len <= (size_t)todo)
		{
			todo -= odata->len;
			memcpy(ptr, odata->buffer, odata->len);
			ptr+=odata->len;
			odata->lenwritten = odata->len;
			odata->writeret = odata->len;
		}
		else
		{
			memcpy(ptr, odata->buffer, todo);
			ptr += todo;
			odata->lenwritten = todo;
			odata->wrap=1;
			break;
		}
		SetEvent(odata->event);
		if(todo && WaitForSingleObject(odata->eventback, 40)!=WAIT_OBJECT_0)
			break;
	}

    return ptr - (char*)buffer;
}

static DWORD WINAPI stderr_write_thread(void *param)
{
    struct output_data *odata = (struct output_data *) param;
    HANDLE errhandle;

    errhandle = GetStdHandle(STD_ERROR_HANDLE);

    while (1)
	{
		WaitForSingleObject(odata->eventback, INFINITE);
		if (odata->done)
			break;
		odata->writeret =
			WriteFile(errhandle,
				odata->buffer, odata->len, &odata->lenwritten, NULL);
		SetEvent(odata->event);
    }
	return 0;
}

bufchain stdout_data, stderr_data;
struct output_data odata, edata;

void try_output(int is_stderr)
{
    struct output_data *data = (is_stderr ? &edata : &odata);
    void *senddata;
    int sendlen;

    if (!data->busy) {
	bufchain_prefix(is_stderr ? &stderr_data : &stdout_data,
			&senddata, &sendlen);
	data->buffer = senddata;
	data->len = sendlen;
	SetEvent(data->eventback);
	data->busy = 1;
    }
}

int from_backend(void *frontend, int is_stderr, const char *data, int len)
{
    HANDLE h = (is_stderr ? errhandle : outhandle);
    int osize, esize;

    assert(len > 0);

    if (is_stderr) {
	bufchain_add(&stderr_data, data, len);
	try_output(1);
    } else {
	bufchain_add(&stdout_data, data, len);
	try_output(0);
    }

    osize = bufchain_size(&stdout_data);
    esize = bufchain_size(&stderr_data);

    return osize + esize;
}

char *do_select(SOCKET skt, int startup)
{
    int events;
    if (startup) {
	events = (FD_CONNECT | FD_READ | FD_WRITE |
		  FD_OOB | FD_CLOSE | FD_ACCEPT);
    } else {
	events = 0;
    }
    if (WSAEventSelect(skt, netevent, events) == SOCKET_ERROR) {
	switch (WSAGetLastError()) {
	  case WSAENETDOWN:
	    return "Network is down";
	  default:
	    return "WSAAsyncSelect(): unknown error";
	}
    }
    return NULL;
}

void listen_thread(void* param)
{
    WSAEVENT stdinevent;
    SOCKET *sklist;
    int reading = 0;
    int sending = 0;
	struct input_data idata = {0};
    int skcount, sksize;
    int connopen = 1;

    stdinevent = CreateEvent(NULL, FALSE, FALSE, NULL);
    handles[1] = stdinevent;
    sklist = NULL;
    skcount = sksize = 0;

	while (1) {
	int n;

	if (!sending && back->sendok(backhandle)) {
	    /*
	     * Create a separate thread to read from stdin. This is
	     * a total pain, but I can't find another way to do it:
	     *
	     *  - an overlapped ReadFile or ReadFileEx just doesn't
	     *    happen; we get failure from ReadFileEx, and
	     *    ReadFile blocks despite being given an OVERLAPPED
	     *    structure. Perhaps we can't do overlapped reads
	     *    on consoles. WHY THE HELL NOT?
	     * 
	     *  - WaitForMultipleObjects(netevent, console) doesn't
	     *    work, because it signals the console when
	     *    _anything_ happens, including mouse motions and
	     *    other things that don't cause data to be readable
	     *    - so we're back to ReadFile blocking.
	     */
	    idata.event = stdinevent;
	    idata.eventback = CreateEvent(NULL, FALSE, FALSE, NULL);
	    sending = TRUE;
		global_idata = &idata;
	}

	n = WaitForMultipleObjects(4, handles, FALSE, INFINITE);
	if (n == 0) {
	    WSANETWORKEVENTS things;
	    SOCKET socket;
	    extern SOCKET first_socket(int *), next_socket(int *);
	    extern int select_result(WPARAM, LPARAM);
	    int i, socketstate;

	    /*
	     * We must not call select_result() for any socket
	     * until we have finished enumerating within the tree.
	     * This is because select_result() may close the socket
	     * and modify the tree.
	     */
	    /* Count the active sockets. */
	    i = 0;
	    for (socket = first_socket(&socketstate);
		 socket != INVALID_SOCKET;
		 socket = next_socket(&socketstate)) i++;

	    /* Expand the buffer if necessary. */
	    if (i > sksize) {
		sksize = i + 16;
		sklist = srealloc(sklist, sksize * sizeof(*sklist));
	    }

	    /* Retrieve the sockets into sklist. */
	    skcount = 0;
	    for (socket = first_socket(&socketstate);
		 socket != INVALID_SOCKET;
		 socket = next_socket(&socketstate)) {
		sklist[skcount++] = socket;
	    }

	    /* Now we're done enumerating; go through the list. */
	    for (i = 0; i < skcount; i++) {
		WPARAM wp;
		socket = sklist[i];
		wp = (WPARAM) socket;
		if (!WSAEnumNetworkEvents(socket, NULL, &things)) {
                    static const struct { int bit, mask; } eventtypes[] = {
                        {FD_CONNECT_BIT, FD_CONNECT},
                        {FD_READ_BIT, FD_READ},
                        {FD_CLOSE_BIT, FD_CLOSE},
                        {FD_OOB_BIT, FD_OOB},
                        {FD_WRITE_BIT, FD_WRITE},
                        {FD_ACCEPT_BIT, FD_ACCEPT},
                    };
                    int e;

		    noise_ultralight(socket);
		    noise_ultralight(things.lNetworkEvents);

                    for (e = 0; e < lenof(eventtypes); e++)
                        if (things.lNetworkEvents & eventtypes[e].mask) {
                            LPARAM lp;
                            int err = things.iErrorCode[eventtypes[e].bit];
                            lp = WSAMAKESELECTREPLY(eventtypes[e].mask, err);
                            connopen &= select_result(wp, lp);
                        }
		}
	    }
	} else if (n == 1) {
	    reading = 0;
	    noise_ultralight(idata.len);
	    if (connopen && back->socket(backhandle) != NULL) {
		if (idata.len > 0) {
		    back->send(backhandle, idata.buffer, idata.len);
		} else {
		    back->special(backhandle, TS_EOF);
		}
	    }
	} else if (n == 2) {
	    odata.busy = 0;
	    if (!odata.writeret) {
		fprintf(stderr, "Unable to write to standard output\n");
		cleanup_exit(0);
	    }
	    bufchain_consume(&stdout_data, odata.lenwritten);
	    if (bufchain_size(&stdout_data) > 0)
		try_output(0);
	    if (connopen && back->socket(backhandle) != NULL) {
		back->unthrottle(backhandle, bufchain_size(&stdout_data) +
				 bufchain_size(&stderr_data));
	    }
	} else if (n == 3) {
	    edata.busy = 0;
	    if (!edata.writeret) {
		fprintf(stderr, "Unable to write to standard output\n");
		cleanup_exit(0);
	    }
	    bufchain_consume(&stderr_data, edata.lenwritten);
	    if (bufchain_size(&stderr_data) > 0)
		try_output(1);
	    if (connopen && back->socket(backhandle) != NULL) {
		back->unthrottle(backhandle, bufchain_size(&stdout_data) +
				 bufchain_size(&stderr_data));
	    }
	}
	if (!reading && back->sendbuffer(backhandle) < MAX_STDIN_BACKLOG) {
	    SetEvent(idata.eventback);
	    reading = 1;
	}
	if ((!connopen || back->socket(backhandle) == NULL) &&
	    bufchain_size(&stdout_data) == 0 &&
	    bufchain_size(&stderr_data) == 0)
	    break;		       /* we closed the connection */
    }
	fatal_exit = TRUE;
}

int plink_connect(const char *username, const char *password, const char *keyfile, const char *host, unsigned port, char version, const char *cmd)
{
	DWORD err_threadid;
    WSAEVENT stdoutevent, stderrevent;
    int reading = 0;
    int sending = 0;

    ssh_get_line = console_get_line;

    /*
     * Initialise port and protocol to sensible defaults. (These
     * will be overridden by more or less anything.)
     */
    default_protocol = PROT_SSH;
    default_port = 22;

    flags = FLAG_STDERR;
    /*
     * Process the command line.
     */
    do_defaults(NULL, &cfg);
	strncpy(cfg.host,host,sizeof(cfg.host));
	if(port>0)
		cfg.port = port;
	strncpy(cfg.username,username,sizeof(cfg.username));
	cfg.nopty = 1;

	if(cmd)
	{
		strncpy(cfg.remote_cmd,cmd,sizeof(cfg.remote_cmd));
		cfg.remote_cmd_ptr = cfg.remote_cmd;
	}

	switch(version)
	{
	case 1:
		cfg.sshprot=0;
		break;
	case 2:
		cfg.sshprot=3;
		break;
	}

	if(keyfile)
	{
		cfg.keyfile=filename_from_str(keyfile);
	}
	else if(password)
	{
		cmdline_password = (char*)password;
		ssh_get_line = cmdline_get_line;
		ssh_getline_pw_only = TRUE;
	}

	if (!*cfg.remote_cmd_ptr)
		flags |= FLAG_INTERACTIVE;

    /*
     * Select protocol. This is farmed out into a table in a
     * separate file to enable an ssh-free variant.
     */
    {
	int i;
	back = NULL;
	for (i = 0; backends[i].backend != NULL; i++)
	    if (backends[i].protocol == cfg.protocol) {
		back = backends[i].backend;
		break;
	    }
	if (back == NULL) {
	    fprintf(stderr,
		    "Internal fault: Unsupported protocol found\n");
	    return 1;
	}
    }

    sk_init();

    /*
     * Start up the connection.
     */
    netevent = CreateEvent(NULL, FALSE, FALSE, NULL);
    {
	const char *error;
	char *realhost;
	/* nodelay is only useful if stdin is a character device (console) */
	int nodelay = cfg.tcp_nodelay &&
	    (GetFileType(GetStdHandle(STD_INPUT_HANDLE)) == FILE_TYPE_CHAR);

	error = back->init(NULL, &backhandle, &cfg, cfg.host, cfg.port, &realhost, nodelay, cfg.tcp_keepalives);
	if (error) {
	    fprintf(stderr, "Unable to open connection:\n%s", error);
	    return 1;
	}
	sfree(realhost);
    }

    stdoutevent = CreateEvent(NULL, FALSE, FALSE, NULL);
    stderrevent = CreateEvent(NULL, FALSE, FALSE, NULL);

    inhandle = GetStdHandle(STD_INPUT_HANDLE);
    outhandle = GetStdHandle(STD_OUTPUT_HANDLE);
    errhandle = GetStdHandle(STD_ERROR_HANDLE);
    GetConsoleMode(inhandle, &orig_console_mode);
    SetConsoleMode(inhandle, ENABLE_PROCESSED_INPUT);

    /*
     * Turn off ECHO and LINE input modes. We don't care if this
     * call fails, because we know we aren't necessarily running in
     * a console.
     */
    handles[0] = netevent;
    handles[2] = stdoutevent;
    handles[3] = stderrevent;
    sending = FALSE;

    /*
     * Create spare threads to write to stdout and stderr, so we
     * can arrange asynchronous writes.
     */
    odata.event = stdoutevent;
    odata.eventback = CreateEvent(NULL, FALSE, FALSE, NULL);
    odata.busy = odata.done = 0;

	global_odata = &odata;

    edata.event = stderrevent;
    edata.eventback = CreateEvent(NULL, FALSE, FALSE, NULL);
    edata.is_stderr = 1;
    edata.busy = edata.done = 0;
    if (!CreateThread(NULL, 0, stderr_write_thread,
		&edata, 0, &err_threadid))
	{
 		fprintf(stderr, "Unable to create error output thread\n");
 		cleanup_exit(1);
    }

	if(!_beginthread(listen_thread,0,NULL))
	{
		fprintf(stderr, "Unable to create background thread\n");
		cleanup_exit(1);
	}

	while(!global_idata)
	{
		Sleep(100);
		if(fatal_exit)
		{
			int exitcode = back->exitcode(backhandle);
			if (exitcode < 0) {
				exitcode = fatal_exit_code;		       /* this is an error condition */
			}
			return exitcode; 
		}
	}
	return 0;
}

putty_callbacks PLINK_EXPORT *plink_set_callbacks(putty_callbacks *new_callbacks)
{
	putty_callbacks *old = callbacks;
	callbacks = new_callbacks;
	return old;
}
