/*  ndir.c - portable directory routines
    Copyright (C) 2000 by Tony Hoyle, tmh@magenta-logic.com

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 1, or (at your option)
    any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.  */

/* From original DOS version.  Win32 version written by Tony Hoyle, 
   January 2000 */

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include "cvs.h"
#include "ndir.h"

DIR *opendir (const char *name)
{
	WIN32_FIND_DATAA fd;
	HANDLE h;
	DIR *dir = NULL;
	DWORD fa = GetFileAttributesA(name);
	char fn[512];

	if(fa==(DWORD)-1 || !(fa&FILE_ATTRIBUTE_DIRECTORY))
    {
       int err = GetLastError();
       switch (err) {
            case ERROR_NO_MORE_FILES:
            case ERROR_FILE_NOT_FOUND:
            case ERROR_PATH_NOT_FOUND:
                errno = ENOENT;
                break;

            case ERROR_NOT_ENOUGH_MEMORY:
                errno = ENOMEM;
                break;

            default:
                errno = EINVAL;
                break;
        }
		return NULL;
    }

	strcpy(fn,name);
	strcat(fn,"\\*.*");
	h=FindFirstFileA(fn,&fd);
	if(h!=INVALID_HANDLE_VALUE)
    {
		int dirsize=1024;
		dir =malloc(sizeof(DIR));
		dir->current=0;
		dir->max=0;
		dir->ptr=malloc(sizeof(struct direct)*dirsize);
  do
    {
			strcpy(dir->ptr[dir->max].d_name,fd.cFileName);
			dir->ptr[dir->max].d_namlen=strlen(fd.cFileName);
			if(++dir->max==dirsize)
	{
				dirsize*=2;
				dir->ptr=xrealloc(dir->ptr,sizeof(struct direct)*dirsize);
	}
		} while(FindNextFileA(h,&fd));
		FindClose(h);
		dir=xrealloc(dir,sizeof(DIR)*dir->max);
	}
	return dir;
}

void closedir (DIR *dirp)
{
	free(dirp->ptr);
	free(dirp);
}


struct direct *readdir (DIR *dirp)
{
	if(dirp->current==dirp->max) return NULL;
	return &dirp->ptr[dirp->current++];
}

void seekdir (DIR *dirp, long off)
{
	if(off>dirp->max) off=dirp->max;
	dirp->current=off;
}

long telldir (DIR *dirp)
{
  return dirp->current;
}

