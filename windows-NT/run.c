/* run.c --- routines for executing subprocesses under Windows NT.
   
   This file is part of GNU CVS.

   GNU CVS is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by the
   Free Software Foundation; either version 2, or (at your option) any
   later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.  */

#include "cvs.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdlib.h>
#include <process.h>
#include <errno.h>
#include <io.h>
#include <fcntl.h>

enum PIPES { PIPE_READ = 0, PIPE_WRITE = 1 };

static void run_add_arg (const char *s);
static void run_init_prog (void);
static int run_command(const char *cmd, int* in_fd, int* out_fd, int *err_fd, HANDLE *hProcess);

/* Statics used for handing off to thread on popen */
static int thrd_in_fd,thrd_out_fd,thrd_err_fd;
static DWORD thrd_exit;
static HANDLE thrd_hprocess;

/*
 * To exec a program under CVS, first call run_setup() to setup any initial
 * arguments.  The options to run_setup are essentially like printf(). The
 * arguments will be parsed into whitespace separated words and added to the
 * global run_argv list.
 * 
 * Then, optionally call run_arg() for each additional argument that you'd like
 * to pass to the executed program.
 * 
 * Finally, call run_exec() to execute the program with the specified arguments.
 * The execvp() syscall will be used, so that the PATH is searched correctly.
 * File redirections can be performed in the call to run_exec().
 */
static char **run_argv;
static int run_argc;
static int run_argc_allocated;

void
run_setup (const char *prog)
{
    char *cp;
    int i;

    char *run_prog;

    /* clean out any malloc'ed values from run_argv */
    for (i = 0; i < run_argc; i++)
    {
	if (run_argv[i])
	{
	    free (run_argv[i]);
	    run_argv[i] = (char *) 0;
	}
    }
    run_argc = 0;

    run_prog = xstrdup (prog);

    /* put each word into run_argv, allocating it as we go */
    for (cp = strtok (run_prog, " \t"); cp; cp = strtok ((char *) NULL, " \t"))
	run_add_arg (cp);

    free (run_prog);
}

void
run_arg (s)
    const char *s;
{
    run_add_arg (s);
}

/* Return a malloc'd copy of s, with double quotes around it.  */
static char *
quote (const char *s)
{
    size_t s_len = 0;
    char *copy = NULL;
    char *scan = (char *) s;

    /* scan string for extra quotes ... */
    while (*scan)
	if ('"' == *scan++)
	    s_len += 2;   /* one extra for the quote character */
	else
	    s_len++;
    /* allocate length + byte for ending zero + for double quotes around */
    scan = copy = xmalloc(s_len + 3);
    *scan++ = '"';
    while (*s)
    {
	if ('"' == *s)
	    *scan++ = '\\';
	*scan++ = *s++;
    }
    /* ending quote and closing zero */
    *scan++ = '"';
    *scan++ = '\0';
    return copy;
}

static void
run_add_arg (s)
    const char *s;
{
	int space = s && strchr(s,' ')?1:0;
	space |= s && strchr(s,'\t')?1:0;

    /* allocate more argv entries if we've run out */
    if (run_argc >= run_argc_allocated)
    {
	run_argc_allocated += 50;
	run_argv = (char **) xrealloc ((char *) run_argv,
				     run_argc_allocated * sizeof (char **));
    }

    if (s)
    {
	run_argv[run_argc] = (space ? quote (s) : xstrdup (s));
	run_argc++;
    }
    else
	run_argv[run_argc] = (char *) 0;	/* not post-incremented on purpose! */
}

static char *argv_to_command()
{
	int size;
	int n;
	char *cmd;

	size=0;
	for(n=0; n<run_argc; n++)
		size+=strlen(run_argv[n])+1;
	
	cmd = (char*)xmalloc(size+1);
	*cmd='\0';

	for(n=0; n<run_argc; n++)
	{
		if(*cmd)
			strcat(cmd," ");
		strcat(cmd,run_argv[n]);
	}
	return cmd;
}

int run_exec (int flags)
{
	int stdout_fd;
	int stderr_fd;
	char *cmd;
	char buf[BUFSIZ];
	int size;
	int err;
	HANDLE hProcess;
	DWORD dwCode;

    /* make sure that we are null terminated, since we didn't calloc */
    run_add_arg ((char *) 0);

    if (noexec && (flags & RUN_REALLY) == 0) /* if in noexec mode */
		return (0);

	cmd = argv_to_command();

	TRACE(1,"run_exec(%s)",cmd);

	if(!server_active)
		err = run_command(cmd, NULL, NULL, NULL, &hProcess);
	else
		err = run_command(cmd, NULL, &stdout_fd, &stderr_fd, &hProcess);
	if(err<0)
	{
		error(0,errno,"Script execution failed");
		return err;
	}

	cvs_flusherr();
	cvs_flushout();

	if(server_active)
	{
		win32setblock(stdout_fd,0);
		win32setblock(stderr_fd,0);
		do
		{
			while((size=read(stderr_fd,buf,BUFSIZ))>0)
				cvs_outerr(buf,size);
			while((size=read(stdout_fd,buf,BUFSIZ))>0)
				cvs_output(buf,size);
		} while(WaitForSingleObject(hProcess,100)==WAIT_TIMEOUT);

		while((size=read(stderr_fd,buf,BUFSIZ))>0)
			cvs_outerr(buf,size);
		while((size=read(stdout_fd,buf,BUFSIZ))>0)
			cvs_output(buf,size);
	}
	else
	{
		do
		{
		} while(WaitForSingleObject(hProcess,100)==WAIT_TIMEOUT);
	}

	cvs_flusherr();
	cvs_flushout();

	if(server_active)
	{
		close(stdout_fd);
		close(stderr_fd);
	}
	GetExitCodeProcess(hProcess,&dwCode);
	CloseHandle(hProcess);

	return (int)dwCode;
}

int run_command(const char *cmd, int* in_fd, int* out_fd, int *err_fd, HANDLE *hProcess)
{
	STARTUPINFO si = { sizeof(STARTUPINFO) };
	PROCESS_INFORMATION pi;
	BOOL status;
	int fd1[2],fd2[2], fd3[2];
	int fdcp1, fdcp2, fdcp3;
	char *c;
	OSVERSIONINFO osv;
	char szComSpec[_MAX_PATH];

	if(in_fd)
	{
		_pipe(fd1,0,_O_BINARY | _O_NOINHERIT);
		*in_fd=fd1[PIPE_WRITE];
		fdcp1 = _dup(fd1[PIPE_READ]);
	}
	if(out_fd)
	{
		_pipe(fd2,0,_O_BINARY | _O_NOINHERIT);
		*out_fd=fd2[PIPE_READ];
		fdcp2 = _dup(fd2[PIPE_WRITE]);
	}
	if(err_fd)
	{
		_pipe(fd3,0,_O_BINARY | _O_NOINHERIT);
		*err_fd=fd3[PIPE_READ];
		fdcp3 = _dup(fd3[PIPE_WRITE]);
	}

	/* The STARTUPINFO structure can specify handles to pass to the
	 child as its standard input, output, and error.  */
	si.hStdInput =  (HANDLE) (in_fd?_get_osfhandle(fdcp1):_get_osfhandle(fileno(stdin)));
	si.hStdOutput = (HANDLE) (out_fd?_get_osfhandle(fdcp2):_get_osfhandle(fileno(stdout)));
	si.hStdError  = (HANDLE) (err_fd?_get_osfhandle (fdcp3):_get_osfhandle(fileno(stderr)));
	si.dwFlags = STARTF_USESTDHANDLES;

	c=malloc(strlen(cmd)+128);
	osv.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	GetVersionEx(&osv);
	if (osv.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS)
		strcpy(c,cmd); // In Win9x we have to execute directly
	else
	{
		if(!GetEnvironmentVariableA("COMSPEC",szComSpec,sizeof(szComSpec)))
			strcpy(szComSpec,"cmd.exe"); 
		sprintf(c,"%s /c %s",szComSpec,cmd);
	}

	status = CreateProcess ((LPCTSTR) NULL,
						  (LPTSTR) c,
				  (LPSECURITY_ATTRIBUTES) NULL, /* lpsaProcess */
				  (LPSECURITY_ATTRIBUTES) NULL, /* lpsaThread */
				  TRUE, /* fInheritHandles */
				  0,    /* fdwCreate */
				  (LPVOID) 0, /* lpvEnvironment */
				  (LPCTSTR) 0, /* lpszCurDir */
				  &si,  /* lpsiStartInfo */
				  &pi); /* lppiProcInfo */
	free(c);

	if (! status)
	{
	  DWORD error_code = GetLastError ();
	  switch (error_code)
	  {
	  case ERROR_NOT_ENOUGH_MEMORY:
	  case ERROR_OUTOFMEMORY:
		  errno = ENOMEM; break;
	  case ERROR_BAD_EXE_FORMAT:
		  errno = ENOEXEC; break;
	  case ERROR_ACCESS_DENIED:
		  errno = EACCES; break;
	  case ERROR_NOT_READY:
	  case ERROR_FILE_NOT_FOUND:
	  case ERROR_PATH_NOT_FOUND:
	  default:
		  errno = ENOENT; break;
	  }
	  return -1;
	}

	WaitForInputIdle(pi.hProcess,100);
	if(!hProcess)
		CloseHandle(pi.hProcess);
	else
		*hProcess = pi.hProcess;

	if(in_fd)
	{
		_close(fd1[PIPE_READ]);
		_close(fdcp1);
	}
	if(out_fd)
	{
		_close(fd2[PIPE_WRITE]);
		_close(fdcp2);
	}
	if(err_fd)
	{	
		_close(fd3[PIPE_WRITE]);
		_close(fdcp3);
	}

	return 0;
}

void
run_print (fp)
    FILE *fp;
{
    int i;

    for (i = 0; i < run_argc; i++)
    {
	(void) fprintf (fp, "'%s'", run_argv[i]);
	if (i != run_argc - 1)
	    (void) fprintf (fp, " ");
    }
}

static void popen_thread()
{
	int in_fd = thrd_in_fd;
	int out_fd = thrd_out_fd;
	int err_fd = thrd_err_fd;
	HANDLE hProcess = thrd_hprocess;
	char buf[BUFSIZ];
	int size;

	win32setblock(out_fd,0);
	win32setblock(err_fd,0);
	do
	{
		while((size=read(err_fd,buf,BUFSIZ))>0)
			cvs_outerr(buf,size);
		while((size=read(out_fd,buf,BUFSIZ))>0)
			cvs_output(buf,size);
	} while(WaitForSingleObject(hProcess,100)==WAIT_TIMEOUT);

	while((size=read(err_fd,buf,BUFSIZ))>0)
		cvs_outerr(buf,size);
	while((size=read(out_fd,buf,BUFSIZ))>0)
		cvs_output(buf,size);

	cvs_flusherr();
	cvs_flushout();

	close(out_fd);
	close(err_fd);
	GetExitCodeProcess(hProcess,&thrd_exit);
	CloseHandle(hProcess);

	thrd_hprocess = NULL;
}


FILE *run_popen (const char *cmd)
{
	TRACE(1,"run_popen(%s)",cmd);
    if (noexec)
		return (NULL);

    /* If the command string uses single quotes, turn them into
       double quotes.  */
    {
		int in_fd, out_fd, err_fd;
		HANDLE hProcess;
		int ret;

		ret = run_command(cmd, &in_fd, &out_fd, &err_fd, &hProcess);
		if(ret<0)
		{
			error(0,errno,"Script execution failed");
			return NULL;
		}

		thrd_out_fd = out_fd;
		thrd_err_fd = err_fd;
		thrd_hprocess = hProcess;

		_beginthread(popen_thread,0,NULL);

		return fdopen(in_fd,"w");
    }
}

int run_pclose(FILE *pipe)
{
	fclose(pipe);
	while(thrd_hprocess)
		Sleep(200);
	return thrd_exit;
}

/* Given an array of arguments that one might pass to spawnv,
   construct a command line that one might pass to CreateProcess.
   Try to quote things appropriately.  */
static char *
build_command (char **argv)
{
    int len;

    /* Compute the total length the command will have.  */
    {
        int i;

	len = 0;
        for (i = 0; argv[i]; i++)
	{
	    char *p;

	    len += 2;  /* for the double quotes */

	    for (p = argv[i]; *p; p++)
	    {
	        if (*p == '"')
		    len += 2;
		else
		    len++;
	    }
	    len++;  /* for the space or the '\0'  */
	}
    }

    {
	/* The + 10 is in case len is 0.  */
        char *command = (char *) malloc (len + 10);
	int i;
	char *p;

	if (! command)
	{
	    errno = ENOMEM;
	    return command;
	}

	p = command;
        *p = '\0';
	/* copy each element of argv to command, putting each command
	   in double quotes, and backslashing any quotes that appear
	   within an argument.  */
	for (i = 0; argv[i]; i++)
	{
	    char *a;
	    *p++ = '"';
	    for (a = argv[i]; *a; a++)
	    {
	        if (*a == '"')
		    *p++ = '\\', *p++ = '"';
		else
		    *p++ = *a;
	    }
	    *p++ = '"';
	    *p++ = ' ';
	}
	if (p > command)
	    p[-1] = '\0';

        return command;
    }
}

/* Arrange for the file descriptor FD to not be inherited by child
   processes.  At the moment, CVS uses this function only on pipes
   returned by piped_child, and our implementation of piped_child
   takes care of setting the file handles' inheritability, so this
   can be a no-op.  */
void
close_on_exec (int fd)
{
}
