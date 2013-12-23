#ifndef NDIR__H
#define NDIR__H
/*  ndir.c - portable directory routines
    Copyright (C) 1990 by Thorsten Ohl, td12@ddagsi3.bitnet

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 1, or (at your option)
    any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.  */

/* Everything non trivial in this code is taken from: @(#)msd_dir.c 1.4
   87/11/06.  A public domain implementation of BSD directory routines
   for MS-DOS.  Written by Michael Rendell ({uunet,utai}michael@garfield),
   August 1897 */

/* Win32 port for CVS/NT by Tony Hoyle, January 2000 */

#include <sys/types.h>	/* ino_t definition */

#define	rewinddir(dirp)	seekdir(dirp, 0L)

/* 255 is said to be big enough for Windows NT.  The more elegant
   solution would be declaring d_name as one byte long and allocating
   it to the actual size needed.  */
#define	MAXNAMELEN	255

struct direct
{
  _ino_t d_ino;			/* a bit of a farce */
  int d_reclen;			/* more farce */
  int d_namlen;			/* length of d_name */
  char d_name[MAXNAMELEN];			/* garentee null termination */
};

typedef struct 
{
	int current;
	int max;
	struct direct *ptr;
} DIR;

extern void seekdir (DIR *, long);
extern long telldir (DIR *);
extern DIR *opendir (const char *);
extern void closedir (DIR *);
extern struct direct *readdir (DIR *);

/* 
 * Local Variables:
 * mode:C
 * ChangeLog:ChangeLog
 * compile-command:make
 * End:
 */
#endif