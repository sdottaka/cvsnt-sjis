/* run.c --- routines for executing subprocesses.
   
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

#include <signal.h>

#ifndef HAVE_UNISTD_H
extern int execvp(char *file, char **argv);
#endif

static void run_add_arg(const char *s);

extern char *strtok ();

static pid_t pipe_pid;

/*
 * To exec a program under CVS, first call run_setup() to setup initial
 * arguments.  The argument to run_setup will be parsed into whitespace 
 * separated words and added to the global run_argv list.
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

/* VARARGS */
void 
run_setup (prog)
    const char *prog;
{
    char *cp;
    int i;
    char *run_prog;

    /* clean out any malloc'ed values from run_argv */
    for (i = 0; i < run_argc; i++)
    {
	if (run_argv[i])
	{
	    xfree (run_argv[i]);
	    run_argv[i] = (char *) 0;
	}
    }
    run_argc = 0;

    run_prog = xstrdup (prog);

    /* put each word into run_argv, allocating it as we go */
    for (cp = strtok (run_prog, " \t"); cp; cp = strtok ((char *) NULL, " \t"))
	run_add_arg (cp);
    xfree (run_prog);
}

void
run_arg (s)
    const char *s;
{
    run_add_arg (s);
}

/* Return a malloc'd copy of s, with double quotes around it.  */
static char *
quote (const char *s, int wrap)
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
    if(wrap)
    	*scan++ = '"';
    while (*s)
    {
        if ('"' == *s)
            *scan++ = '\\';
        *scan++ = *s++;
    }
    /* ending quote and closing zero */
    if(wrap)
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
        run_argv[run_argc] = (space ? quote (s,1) : xstrdup (s));
        run_argc++;
    }
    else
        run_argv[run_argc] = (char *) 0;        /* not post-incremented on purpose! */
}

int run_exec (int flags)
{
  pid_t pid;
  int status;
  
  cvs_flusherr();
  cvs_flushout();
  run_add_arg(NULL);

#ifdef SERVER_SUPPORT
  if(server_active)
  {
    int stdout_pipe[2];
    int stderr_pipe[2];
    int stdout_fd, stderr_fd;
    int size,stat;
    char buf[BUFSIZ];
    int w;

    if (pipe (stdout_pipe) < 0)
        error (1, 0, "cannot create pipe");
    if (pipe (stderr_pipe) < 0)
        error (1, 0, "cannot create pipe");

#ifdef USE_SETMODE_BINARY
    setmode (stdout_pipe[0], O_BINARY);
    setmode (stdout_pipe[1], O_BINARY);
    setmode (stderr_pipe[0], O_BINARY);
    setmode (stderr_pipe[1], O_BINARY);
#endif

#ifdef HAVE_VFORK
    pid = vfork ();
#else
    pid = fork ();
#endif
    if (pid < 0)
        error (1, 0, "cannot fork");
    if (pid == 0)
    {
        if (dup2 (stdout_pipe[1], 1) < 0)
          error (1, 0, "cannot dup2 pipe");
        if (close (stdout_pipe[0]) < 0)
          error (1, 0, "cannot close pipe");
        if (dup2 (stderr_pipe[1], 2) < 0)
          error (1, 0, "cannot dup2 pipe");
        if (close (stderr_pipe[0]) < 0)
          error (1, 0, "cannot close pipe");

        execvp (run_argv[0], run_argv);
        error (1, errno, "cannot exec %s", run_argv[0]);
    }
    if (close (stdout_pipe[1]) < 0)
      error (1, 0, "cannot close pipe");
    if (close (stderr_pipe[1]) < 0)
      error (1, 0, "cannot close pipe");

    stdout_fd = stdout_pipe[0];
    stderr_fd = stderr_pipe[0];

    fcntl(stdout_fd,F_SETFL, O_NONBLOCK);
    fcntl(stderr_fd,F_SETFL, O_NONBLOCK);
    do
    {
      status = 0;
      w = waitpid(pid, &status, WNOHANG); 
      usleep(200);
      while(stderr_fd && (size=read(stderr_fd,buf,BUFSIZ))>0)
        cvs_outerr(buf,size);
      if(size<0 && errno!=EAGAIN)
      {
        close(stderr_fd);
        stderr_fd = 0;
      }
      while(stdout_fd && (size=read(stdout_fd,buf,BUFSIZ))>0)
        cvs_output(buf,size);
      if(size<0 && errno!=EAGAIN)
      {
        close(stdout_fd);
        stdout_fd = 0;
      }
    } while(w==0);
  }
  else
#endif
  {
#ifdef HAVE_VFORK
     pid = vfork ();
#else
     pid = fork ();
#endif
    if (pid < 0)
        error (1, 0, "cannot fork");
     if(pid == 0)
     {
       execvp(run_argv[0],run_argv);
       error (1, errno, "cannot exec %s", run_argv[0]);
     }
     waitpid(pid, &status, 0);
  }

  cvs_flusherr();
  cvs_flushout();

  return WEXITSTATUS(status);
}

void
run_print (fp)
    FILE *fp;
{
    int i;
    void (*outfn)(const char *, size_t);

    if (fp == stderr)
	outfn = cvs_outerr;
    else if (fp == stdout)
	outfn = cvs_output;
    else
    {
	error (1, 0, "internal error: bad argument to run_print");
	/* Solely to placate gcc -Wall.
	   FIXME: it'd be better to use a function named `fatal' that
	   is known never to return.  Then kludges wouldn't be necessary.  */
	outfn = NULL;
    }

    for (i = 0; i < run_argc; i++)
    {
	(*outfn) ("'", 1);
	(*outfn) (run_argv[i], 0);
	(*outfn) ("'", 1);
	if (i != run_argc - 1)
	    (*outfn) (" ", 1);
    }
}

static void tokenise(char *argbuf, char **argv)
{
  char *p,*q;
  int in_quote=0,escape=0;

  for(p=argbuf;*p;p++)
  {
    while(isspace(*p)) p++;
    *(argv++)=p;
    q=p;
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
          continue;
        }
        if(*p=='\'' || *p=='"')
        {
          in_quote=*p;
	  q--;
          continue;
        }
        if(isspace(*p))
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
    if(!*p)
      break;
    *q='\0';
  }
  *argv=NULL;
}

static int run_command2(char **argv, int to_child_pipe[2], int from_child_pipe[2], int err_child_pipe[2], int *child_proc)
{
  int pid;

#ifdef USE_SETMODE_BINARY
  setmode (to_child_pipe[0], O_BINARY);
  setmode (to_child_pipe[1], O_BINARY);
  setmode (from_child_pipe[0], O_BINARY);
  setmode (from_child_pipe[1], O_BINARY);
  setmode (err_child_pipe[0], O_BINARY);
  setmode (err_child_pipe[1], O_BINARY);
#endif


#ifdef HAVE_VFORK
  pid = vfork ();
#else
  pid = fork ();
#endif
  if (pid < 0)
      error(1, errno, "cannot fork");
  if (pid == 0)
  {
      if (close (to_child_pipe[1]) < 0)
          error(1, errno, "cannot close pipe");
      if (dup2 (to_child_pipe[0], 0) < 0)
          error(1, errno, "cannot dup2 pipe");
      if (close (from_child_pipe[0]) < 0)
          error(1, errno, "cannot close pipe");
      if (dup2 (from_child_pipe[1], 1) < 0)
          error(1, errno, "cannot dup2 pipe");
      if (close (err_child_pipe[0]) < 0)
          error(1, errno, "cannot close pipe");
      if (dup2 (err_child_pipe[1], 2) < 0)
          error(1, errno, "cannot dup2 pipe");

      execvp (argv[0], argv);
      error(1, errno, "cannot exec %s", argv[0]);
  }
  if (close (to_child_pipe[0]) < 0)
      error(1, errno, "cannot close pipe");
  if (close (from_child_pipe[1]) < 0)
      error(1, errno, "cannot close pipe");
  if (close (err_child_pipe[1]) < 0)
      error(1, errno, "cannot close pipe");

  if(child_proc)
	  *child_proc = pid;

  return 0;
}

static int run_command(const char *cmd, int* in_fd, int* out_fd, int* err_fd, int *child_proc)
{
  char **argv;    
  int ret;
  int to_child_pipe[2];
  int from_child_pipe[2];
  int err_child_pipe[2];

  TRACE(1,"run_command(%s)",cmd);

  if (noexec)
	return 0;

  argv=(char**)xmalloc(4*sizeof(char*));
  argv[0]="/bin/sh";
  argv[1]="-c";
  argv[2]=(char*)cmd;
  argv[3]=NULL;

  if (pipe (to_child_pipe) < 0)
      error(1, errno, "cannot create pipe");
  if (pipe (from_child_pipe) < 0)
      error(1, errno, "cannot create pipe");
  if (err_fd && pipe (err_child_pipe) <0)
      error(1, errno, "cannot create pipe");

  ret = run_command2(argv,to_child_pipe,from_child_pipe,err_fd?err_child_pipe:from_child_pipe, child_proc);

  xfree(argv);
  if(ret)
    return ret;

  if(in_fd)
	*in_fd = to_child_pipe[1];
  else
	  close(to_child_pipe[1]);
  if(out_fd)
	*out_fd = from_child_pipe[0];
  else
	  close(from_child_pipe[0]);
  if(err_fd)
	*err_fd = err_child_pipe[0];
  else
	  close(err_child_pipe[0]);

  return ret;
}

/* Return value is NULL for error, or if noexec was set.  If there was an
   error, return NULL and I'm not sure whether errno was set (the Red Hat
   Linux 4.1 popen manpage was kind of vague but discouraging; and the noexec
   case complicates this even aside from popen behavior).  */

FILE *run_popen (const char *cmd)
{
  char **argv;    
  int ret;
  int to_child_pipe[2];
  int from_child_pipe[2];
  int err_child_pipe[2];
  pid_t pid;

  TRACE(1,"run_popen(%s)",cmd);

  if (noexec)
	return (NULL);

  argv=(char**)xmalloc(4*sizeof(char*));
  argv[0]="/bin/sh";
  argv[1]="-c";
  argv[2]=(char*)cmd;
  argv[3]=NULL;

  if (pipe (to_child_pipe) < 0)
      error(1, errno, "cannot create pipe");
  if (pipe (from_child_pipe) < 0)
      error(1, errno, "cannot create pipe");
  if (pipe (err_child_pipe) <0)
      error(1, errno, "cannot create pipe");

  pid = fork();
  if(!pid)
  {
    /* I am the child */
    char buf[BUFSIZ];
    int size;
    int status;
    int last_time=0;
    pid_t child_proc;
    int w;

    ret = run_command2(argv,to_child_pipe,from_child_pipe,err_child_pipe, &child_proc);
    xfree(argv);
    if(ret)
	exit(ret); 

    close(to_child_pipe[1]);
    fcntl(from_child_pipe[0],F_SETFL, O_NONBLOCK);
    fcntl(err_child_pipe[0],F_SETFL, O_NONBLOCK);
    do
    {
        status = 0;
        w = waitpid(child_proc, &status, WNOHANG); 
	usleep(200);
        while(err_child_pipe[0] && (size=read(err_child_pipe[0],buf,BUFSIZ))>0)
          cvs_outerr(buf,size);
      	if(err_child_pipe[0] && (size<0 && errno!=EAGAIN))
        {
       	  close(err_child_pipe[0]);
          err_child_pipe[0] = 0;
      	}
      	while(from_child_pipe[0] && (size=read(from_child_pipe[0],buf,BUFSIZ))>0)
          cvs_output(buf,size);
      	if(from_child_pipe[0] && (size<0 && errno!=EAGAIN))
      	{
          close(from_child_pipe[0]);
          from_child_pipe[0] = 0;
      	}
    } while(w==0);
    exit(WEXITSTATUS(status)); 
  }
  close(from_child_pipe[0]);
  close(err_child_pipe[0]);
  xfree(argv);
  pipe_pid = pid;
  return fdopen(to_child_pipe[1],"w");
}

int run_pclose(FILE *pipe)
{
	int status;

	fclose(pipe);
	waitpid(pipe_pid, &status, 0);

	return WEXITSTATUS(status);
}

void
close_on_exec (fd)
     int fd;
{
#ifdef F_SETFD
    if (fcntl (fd, F_SETFD, 1))
	error (1, errno, "can't set close-on-exec flag on %d", fd);
#endif
}
