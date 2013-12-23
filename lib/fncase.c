/* fncase.c -- CVS support for case insensitive file systems.
   Jim Blandy <jimb@cyclic.com>

   This file is part of GNU CVS.

   GNU CVS is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by the
   Free Software Foundation; either version 2, or (at your option) any
   later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.  */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "system.h"

/* The equivalence class mapping for filenames.
   Windows NT filenames are case-insensitive, but case-preserving.
   Both / and \ are path element separators.
   Thus, this table maps both upper and lower case to lower case, and
   both / and \ to /.  */

#if 0
main ()
{
  int c;

  for (c = 0; c < 256; c++)
    {
      int t;

      if (c == '\\')
        t = '/';
      else
        t = tolower (c);
      
      if ((c & 0x7) == 0x0)
         printf ("    ");
      printf ("0x%02x,", t);
      if ((c & 0x7) == 0x7)
         putchar ('\n');
      else if ((c & 0x7) == 0x3)
         putchar (' ');
    }
}
#endif

/* Like strcmp, but with the appropriate tweaks for file names.
   Under Windows NT, filenames are case-insensitive but case-preserving,
   and both \ and / are path element separators.  */
int fncmp (const char *n1, const char *n2)
{
    while (*n1 && *n2 && FOLD_FN_CHAR(*n1)==FOLD_FN_CHAR(*n2))
        n1++, n2++;
    return FOLD_FN_CHAR(*n1)-FOLD_FN_CHAR(*n2);
}

int
fnncmp (const char *n1, const char *n2, size_t len)
{
    while (*n1 && *n2 && len && FOLD_FN_CHAR(*n1)==FOLD_FN_CHAR(*n2))
        n1++, n2++, len--;
	if(!len) return 0;
	else
    return FOLD_FN_CHAR(*n1)-FOLD_FN_CHAR(*n2);
}

/* Fold characters in FILENAME to their canonical forms.  
   If FOLD_FN_CHAR is not #defined, the system provides a default
   definition for this.  */
void
fnfold (char *filename)
{
    while (*filename)
	{
        *filename = FOLD_FN_CHAR (*filename);
	filename++;
    }
}
