/* yesno.c -- read a yes/no response from stdin
   Copyright (C) 1990 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.  */

#include "cvs.h"

#include <stdio.h>
#if defined(CVSGUI_PIPE)
#include <string.h>
#endif /* CVSGUI_PIPE */

/* Read one line from standard input
   and return nonzero if that line begins with y or Y,
   otherwise return 0. */

int
yesno ()
{
  int c;
  int rv;

#ifdef CVSGUI_PIPE
	char *response;

	cvs_output("\n", 1);
	response = getenv("CVSLIB_YESNO");
	if(response != 0L)
		return strcmp(response, "yes") == 0;
#endif /* CVSGUI_PIPE*/

  fflush (stderr);
  fflush (stdout);
  c = getchar ();
  rv = (c == 'y') || (c == 'Y');
  while (c != EOF && c != '\n')
    c = getchar ();

  return rv;
}
