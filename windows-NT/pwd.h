#include <tchar.h>
/*  pwd.h - Try to approximate UN*X's getuser...() functions under MS-DOS.
    Copyright (C) 1990 by Thorsten Ohl, td12@ddagsi3.bitnet

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 1, or (at your option)
    any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.  */

/* This 'implementation' is conjectured from the use of this functions in
   the RCS and BASH distributions.  Of course these functions don't do too
   much useful things under MS-DOS, but using them avoids many "#ifdef
   MSDOS" in ported UN*X code ...  */

struct passwd
{
  const char *pw_name;		/* login user id			*/
  const char *pw_dir;			/* home directory			*/
  const char *pw_shell;		/* login shell				*/
  gid_t pw_gid;			/* Group ID					*/
  uid_t pw_uid;			/* User ID					*/
  const char *pw_passwd;		/* Password					*/
  const TCHAR *pw_pdc;			/* Primary domain controller */
  const TCHAR *pw_name_t;		/* login user id (unicode)	*/
};

//extern struct passwd *getpwuid (uid_t);
extern struct passwd *getpwnam (const char *);
extern char *getlogin (void);

/*
 * Local Variables:
 * mode:C
 * ChangeLog:ChangeLog
 * compile-command:make
 * End:
 */
