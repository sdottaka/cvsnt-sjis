/*
** The cvsgui protocol used by WinCvs
**
** This library is free software; you can redistribute it and/or
** modify it under the terms of the GNU Lesser General Public
** License as published by the Free Software Foundation; either
** version 2.1 of the License, or (at your option) any later version.
** 
** This library is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
** Lesser General Public License for more details.
** 
** You should have received a copy of the GNU Lesser General Public
** License along with this library; if not, write to the Free Software
** Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

/*!
	\file cvsgui.c
	\brief Glue code implementation for communicating with CVS over pipes

	Glue code to intercept some low level I/O calls and tunnel them inside the communication pipes

	\author Alexandre Parenteau <aubonbeurre@hotmail.com> --- November 1999
	\note To be used by CVS client
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#ifndef WIN32
#include <unistd.h>
#endif

#include <limits.h>

#if defined(WIN32) && !defined(PATH_MAX)
#	ifdef _MAX_PATH
#		define PATH_MAX _MAX_PATH
#	else 
#		define PATH_MAX 512
#	endif
#endif

#include "cvsgui.h"
#include "cvsgui_process.h"
#include "cvsgui_protocol.h"

#undef getenv
#undef getpass
#undef main
#undef exit

#ifdef WIN32
	/// \copydoc cvsguiglue_getpass(const char* prompt)
	extern char* getpass(const char* prompt);
#endif

pipe_t _cvsgui_readfd = 0;	/*!< Read pipe file descriptor */
pipe_t _cvsgui_writefd = 0;	/*!< Write pipe file descriptor */

#if !defined(WIN32) && !defined(__GNUC__)
static off_t outseek;					/*!< Output file position */
static off_t errseek;					/*!< Error file position */
static char outname[PATH_MAX] = {0};	/*!< Output file name  */
static char errname[PATH_MAX] = {0};	/*!< Error file name */
static FILE* outlog;					/*!< Output file */
static FILE* errlog;					/*!< Error file */
#endif

/*!
	Get the environmental variable
	\param env Name of the environmental variable to get
	\return The value of the environmental variable
*/
char* cvsguiglue_getenv(const char* env)
{
	char* res = 0L;

	if( _cvsgui_readfd == 0 )
		return getenv(env);

	cvsguiglue_flushconsole(0);

	if( env && gp_getenv_write(_cvsgui_writefd, env) )
	{
		res = gp_getenv_read(_cvsgui_readfd);;
	}

	return res;
}

/*!
	Get the password
	\param prompt Password prompt message
	\return Password string
	\note Queries for environmental variable <b>CVS_GETPASS</b>
*/
char* cvsguiglue_getpass(const char* prompt)
{
	cvsguiglue_flushconsole(0);
	
	if( _cvsgui_readfd == 0 )
		return getpass(prompt);

	return cvsguiglue_getenv("CVS_GETPASS");
}

/*!
	Report the exit code and exit
	\param code Exit code
*/
void cvsguiglue_exit(int code)
{
	cvsguiglue_flushconsole(1);
	
	if( _cvsgui_writefd != 0 )
		gp_quit_write(_cvsgui_writefd, code);

	exit(code);
}

/*!
	Reopen the <b>stdout</b> and <b>stderr</b> consoles
*/
static void reopen_consoles(void)
{
#if !defined(WIN32) && !defined(__GNUC__)
    strcpy(outname, tempnam(0L, 0L));
    strcpy(errname, tempnam(0L, 0L));
    strcat(outname, ".out");
    strcat(errname, ".err");

	outlog = freopen(outname, "w+", stdout);
	if( outlog == 0L )
	{
		fprintf(stderr, "Unable to reopen stdout !\n");
		exit(1);
	}

	errlog = freopen(errname, "w+", stderr);
	if( errlog == 0L )
	{
		fprintf(stderr, "Unable to reopen stderr !\n");
		exit(1);
	}
	
	outseek = 0;
	errseek = 0;
#endif
}

#if !defined(WIN32) && !defined(__GNUC__)
/*!
	Close consoles
*/
static void close_consoles(void)
{
	if( outlog != 0L )
		fclose(outlog);

	if( errlog != 0L )
		fclose(errlog);

	if( outname[0] )
		unlink(outname);
	
	if( errname[0] )
		unlink(errname);

	outlog = 0L;
	errlog = 0L;
	outname[0] = '\0';
	errname[0] = '\0';
}

#if !defined(CVS_FSEEK)
#	define CVS_FSEEK fseek
#endif

#if !defined(CVS_FTELL)
#	define CVS_FTELL ftell
#endif

/*!
	Flush console
	\param log Console file
	\param oldpos Position in the console file
*/
static void myflush(FILE* log, off_t* oldpos)
{
	off_t newpos, pos;

#	define BUF_SIZE 8000
	char buf[BUF_SIZE];

	size_t len;

	pos = CVS_FSEEK(log, *oldpos, SEEK_SET);
	if( pos != 0 )
		goto fail;

	while( !feof(log) && !ferror(log) )
	{
		len = fread(buf, sizeof(char), BUF_SIZE, log);
		if( len > 0 )
		{
			if( log == outlog )
				gp_console_write(_cvsgui_writefd, buf, len, 0, 0);

			if( log == errlog )
				gp_console_write(_cvsgui_writefd, buf, len, 1, 0);
		}
	}

	if( ferror(log) )
		goto fail;

	newpos = CVS_FTELL(log);
	if( newpos < 0 )
		goto fail;

	*oldpos = newpos;

	return;

fail:
	fprintf(stderr, "Unable to redirect stdout/stderr !\n");
}
#endif

/*!
	Flush consoles
	\param closeit Set to non-zero to close consoles
*/
void cvsguiglue_flushconsole(int closeit)
{
	fflush(stdout);
	fflush(stderr);
	
#if !defined(WIN32) && !defined(__GNUC__)
	if( outlog != 0L )
		myflush(outlog, &outseek);
	
	if( errlog != 0L )
		myflush(errlog, &errseek);
	
	if( closeit )
		close_consoles();
#endif
}

/*!
	Main function
	\param argc Arguments count
	\param argv Arguments
	\return The exit code
	\note Detects whether the cvsgui protocol should be activated
*/
#ifdef WINCVS_GOTTANI_TEMPORARILY_FIX_COMMANDLINE_PROBLEM
#include "URI_encode.h"
#endif /* WINCVS_GOTTANI_TEMPORARILY_FIX_COMMANDLINE_PROBLEM */
#ifdef _UNICODE
int real_main(int argc, char* argv[])
#else
int main(int argc, char* argv[])
#endif
{
	int res;
	char** tmparg = 0L;
#ifdef WINCVS_GOTTANI_TEMPORARILY_FIX_COMMANDLINE_PROBLEM
	int commandline_URI_encode = 0; /* bool, false */
	int j;
#endif /* WINCVS_GOTTANI_TEMPORARILY_FIX_COMMANDLINE_PROBLEM */

	if( argc >= 4 && strcmp(argv[1], "-cvsgui") == 0 )
	{
		int i;

		_cvsgui_readfd = (pipe_t)atoi(argv[2]);
		_cvsgui_writefd = (pipe_t)atoi(argv[3]);

		tmparg = (char**)malloc((argc - 2) * sizeof(char*));
		if( tmparg == 0L )
			goto ignore;

		tmparg[0] = argv[0];

#ifdef WINCVS_GOTTANI_TEMPORARILY_FIX_COMMANDLINE_PROBLEM
		j = 4;
#endif /* WINCVS_GOTTANI_TEMPORARILY_FIX_COMMANDLINE_PROBLEM */
		for(i = 4; i < argc; i++)
		{
#ifdef WINCVS_GOTTANI_TEMPORARILY_FIX_COMMANDLINE_PROBLEM
#if defined(_DEBUG) && defined(WIN32)
			OutputDebugString(argv[i]), OutputDebugString("\n");
#endif /* defined(_DEBUG) && defined(WIN32) */
			if(!commandline_URI_encode && strcmp(argv[i], "--cvsgui-cmdline-encoded") == 0)
			{
				commandline_URI_encode = 1; /* true */
			}
			else
			{
				tmparg[j - 3] = commandline_URI_encode ? URI_decode(argv[i]) : argv[i];
				j++;
			}
#else  /* WINCVS_GOTTANI_TEMPORARILY_FIX_COMMANDLINE_PROBLEM */
			tmparg[i - 3] = argv[i];
#endif /* WINCVS_GOTTANI_TEMPORARILY_FIX_COMMANDLINE_PROBLEM */
		}

#ifdef WINCVS_GOTTANI_TEMPORARILY_FIX_COMMANDLINE_PROBLEM
		tmparg[argc - (3 + commandline_URI_encode)] = 0L;
		argc -= (3 + commandline_URI_encode);
#else  /* WINCVS_GOTTANI_TEMPORARILY_FIX_COMMANDLINE_PROBLEM */
		tmparg[argc - 3] = 0L;
		argc -= 3;
#endif /* WINCVS_GOTTANI_TEMPORARILY_FIX_COMMANDLINE_PROBLEM */

		cvs_process_init();
		reopen_consoles();
	}
	
ignore:
	res = cvsguiglue_main(argc, tmparg != 0L ? tmparg :  argv);

	if( tmparg != 0L )
	{
		cvsguiglue_flushconsole(1);

		if( _cvsgui_writefd != 0 )
			gp_quit_write(_cvsgui_writefd, res);

		free(tmparg);
	}

	return res;
}
