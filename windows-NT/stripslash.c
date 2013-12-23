/* stripslash.c -- remove trailing slashes from a string
   Copyright (C) 1990 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.  */

#include <string.h>
#include <ctype.h>

#ifdef SJIS
#include "sjispath.h"
#endif

/* Remove trailing slashes from PATH. */

void
strip_trailing_slashes (path)
     char *path;
{
#ifdef SJIS
  while(1) {
    char c = sjis_getlastchar(path);
    if(c == '/' || c == '\\')
      path[strlen(path) - 1] = '\0';
    else
      break;
  }
#else
  int last;

  last = strlen (path) - 1;
  while (last > 0 && (path[last] == '/' || path[last] == '\\'))
    path[last--] = '\0';
#endif
}
