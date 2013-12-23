/* Copyright (C) 1992 Free Software Foundation, Inc.
This file is part of the GNU C Library.

The GNU C Library is free software; you can redistribute it and/or
modify it under the terms of the GNU Library General Public License as
published by the Free Software Foundation; either version 2 of the
License, or (at your option) any later version.

The GNU C Library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Library General Public License for more details.  */

/* Modified slightly by Brian Berliner <berliner@sun.com> and
   Jim Blandy <jimb@cyclic.com> for CVS use */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "system.h"

/* IGNORE(@ */
/* #include <ansidecl.h> */
/* @) */
#include <errno.h>
#include "fnmatch.h"

/*
 * Modified for Shift-JIS support by
 * Atsuo Ishimoto <ishimoto@axissoft.co.jp>,
 * Satoshi YAMADA <slakichi@kmc.kyoto-u.ac.jp>
 */

#ifdef SJIS
#include <mbctype.h>
#define GET_NEXT_CHAR(n) ((unsigned short)(_ismbblead(*((unsigned char *)(n)))? \
	((n)+=2,(*(((unsigned char *)(n)-2))<<8)|(*((unsigned char *)(n)-1)&0xFF)): \
	((n)++,*((unsigned char *)(n)-1))))
#endif

#if !defined(__GNU_LIBRARY__) && !defined(STDC_HEADERS)
extern int errno;
#endif

/* Match STRING against the filename pattern PATTERN, returning zero if
   it matches, nonzero if not.  */
int
#if __STDC__
fnmatch (const char *pattern, const char *string, int flags)
#else
fnmatch (pattern, string, flags)
    char *pattern;
    char *string;
    int flags;
#endif
{
  register const char *p = pattern, *n = string;
#ifdef SJIS
  register unsigned short c, cn;
#else
  register char c;
#endif

  if ((flags & ~__FNM_FLAGS) != 0)
    {
      errno = EINVAL;
      return -1;
    }

#ifdef SJIS
  while ((c = ((*p++)&0xFF)) != '\0')
#else
  while ((c = *p++) != '\0')
#endif
    {
#ifdef SJIS
		if (_ismbblead((unsigned char)c))
		  c = (c<<8)|((*p++)&0xFF);
#endif
      switch (c)
	{
	case '?':
	  if (*n == '\0')
	    return FNM_NOMATCH;
	  else if ((flags & FNM_PATHNAME) && *n == '/')
	    return FNM_NOMATCH;
	  else if ((flags & FNM_PERIOD) && *n == '.' &&
		   (n == string || ((flags & FNM_PATHNAME) && n[-1] == '/')))
	    return FNM_NOMATCH;
#ifdef SJIS
          if(_ismbblead((unsigned char)(*n)))
            n++;
#endif
	  break;
	  
	case '\\':
#ifdef SJIS
	    c = GET_NEXT_CHAR(p);
	  cn = GET_NEXT_CHAR(n);
	  n--;
	  if(cn < 0x100 && c < 0x100) {
	    if (FOLD_FN_CHAR (c) != FOLD_FN_CHAR (cn))
	      return FNM_NOMATCH;
	} else if(cn != c)
#else
	    c = *p++;
	  if (FOLD_FN_CHAR (*n) != FOLD_FN_CHAR (c))
#endif
	    return FNM_NOMATCH;
	  break;
	  
	case '*':
	  if ((flags & FNM_PERIOD) && *n == '.' &&
	      (n == string || ((flags & FNM_PATHNAME) && n[-1] == '/')))
	    return FNM_NOMATCH;
	  
	  for (c = *p++; c == '?' || c == '*'; c = *p++, ++n)
	    if (((flags & FNM_PATHNAME) && *n == '/') ||
		(c == '?' && *n == '\0'))
	      return FNM_NOMATCH;
	  
	  if (c == '\0')
	    return 0;
	  
#ifdef SJIS
	  if (_ismbblead((unsigned char)c))
	    c = (c<<8)|((*p++)&0xFF);
#endif

	  {
#ifdef SJIS
	    unsigned short c1 = (!(flags & FNM_NOESCAPE) && c == '\\') 
	    ? ((_ismbblead(*p))?((*p<<8)|(*p++)):(*p)) : c;
#else
	    char c1 = (!(flags & FNM_NOESCAPE) && c == '\\') ? *p : c;
#endif
	    for (--p; *n != '\0'; ++n)
#ifdef SJIS
	    {
	      cn = GET_NEXT_CHAR(n);
	      n--;
	      if ((c == '['
	         || ((cn > 0xFF || c1 > 0xFF)?
	            (cn == c1):(FOLD_FN_CHAR (*n) == FOLD_FN_CHAR (c1)))) &&
		  fnmatch(p-(c>0xFF||c1>0xFF), n-(cn>0xFF),
		          flags & ~FNM_PERIOD) == 0)
		return 0;
	    }
#else
	      if ((c == '[' || FOLD_FN_CHAR (*n) == FOLD_FN_CHAR (c1)) &&
		  fnmatch(p, n, flags & ~FNM_PERIOD) == 0)
		return 0;
#endif
	    return FNM_NOMATCH;
	  }
	  
	case '[':
	  {
	    /* Nonzero if the sense of the character class is inverted.  */
	    register int not;
	    
	    if (*n == '\0')
	      return FNM_NOMATCH;
	    
	    if ((flags & FNM_PERIOD) && *n == '.' &&
		(n == string || ((flags & FNM_PATHNAME) && n[-1] == '/')))
	      return FNM_NOMATCH;
	    
	    not = (*p == '!' || *p == '^');
	    if (not)
	      ++p;
	    
#ifdef SJIS
	    c = GET_NEXT_CHAR(p);
#else
	    c = *p++;
#endif
	    for (;;)
	      {
#ifdef SJIS
		register unsigned short cstart = c, cend = c;
#else
		register char cstart = c, cend = c;
#endif
		
		if (!(flags & FNM_NOESCAPE) && c == '\\')
#ifdef SJIS
		  cstart = cend = GET_NEXT_CHAR(p);
#else
		  cstart = cend = *p++;
#endif
		
		if (c == '\0')
		  /* [ (unterminated) loses.  */
		  return FNM_NOMATCH;
		
#ifdef SJIS
		c = GET_NEXT_CHAR(p);
#else
		c = *p++;
#endif
		
		if ((flags & FNM_PATHNAME) && c == '/')
		  /* [/] can never match.  */
		  return FNM_NOMATCH;
		
		if (c == '-' && *p != ']')
		  {
#ifdef SJIS
		    cend = GET_NEXT_CHAR(p);
		    if (!(flags & FNM_NOESCAPE) &&
		        cend == '\\')
		      cend = GET_NEXT_CHAR(p);
		    if (cend == '\0')
		      return FNM_NOMATCH;
		    c = GET_NEXT_CHAR(p);
#else
		    cend = *p++;
		    if (!(flags & FNM_NOESCAPE) && cend == '\\')
		      cend = *p++;
		    if (cend == '\0')
		      return FNM_NOMATCH;
		    c = *p++;
#endif
		  }
		
#ifdef SJIS
		cn = GET_NEXT_CHAR(n);
		n--;
		if (cn >= cstart && cn <= cend)
#else
		if (*n >= cstart && *n <= cend)
#endif
		  goto matched;
		
		if (c == ']')
		  break;
	      }
	    if (!not)
	      return FNM_NOMATCH;
	    break;
	    
	  matched:;
	    /* Skip the rest of the [...] that already matched.  */
	    while (c != ']')
	      {
		if (c == '\0')
		  /* [... (unterminated) loses.  */
		  return FNM_NOMATCH;
		
#ifdef SJIS
		c = GET_NEXT_CHAR(p);
#else
		c = *p++;
#endif
		if (!(flags & FNM_NOESCAPE) && c == '\\')
		  /* 1003.2d11 is unclear if this is right.  %%% */
#ifdef SJIS
		  (void)GET_NEXT_CHAR(p);
#else
		  ++p;
#endif
	      }
	    if (not)
	      return FNM_NOMATCH;
	  }
	  break;
	  
	default:
#ifdef SJIS
	  cn = GET_NEXT_CHAR(n);
	  n--;
	  if(cn < 0x100 && c < 0x100) {
	    if (FOLD_FN_CHAR (c) != FOLD_FN_CHAR (*n))
	      return FNM_NOMATCH;
	  } else {
	    if(cn != c)
	      return FNM_NOMATCH;
	  }
#else
	  if (FOLD_FN_CHAR (c) != FOLD_FN_CHAR (*n))
	    return FNM_NOMATCH;
#endif
	}
      
      ++n;
    }

  if (*n == '\0')
    return 0;

  return FNM_NOMATCH;
}
