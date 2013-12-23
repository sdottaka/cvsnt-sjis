dnl Process this file with autoconf to produce a configure script.

dnl Copyright (C) 2002 Simon Josefsson.
dnl
dnl This file is part of Libntlm.
dnl
dnl Libntlm is free software; you can redistribute it and/or modify
dnl it under the terms of the GNU Lesser General Public License as
dnl published by the Free Software Foundation; either version 2.1 of
dnl the License, or (at your option) any later version.
dnl
dnl Libntlm is distributed in the hope that it will be useful,
dnl but WITHOUT ANY WARRANTY; without even the implied warranty of
dnl MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
dnl GNU Lesser General Public License for more details.
dnl
dnl You should have received a copy of the GNU Lesser General Public
dnl License along with Libntlm; if not, write to the Free Software
dnl Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA

AC_INIT(libntlm, 0.3.2, bug-libntlm@josefsson.org)
AC_CONFIG_SRCDIR(ntlm.h.in)

# Interfaces removed:    CURRENT++, AGE=0, REVISION=0
# Interfaces added:      CURRENT++, AGE++, REVISION=0
# No interfaces changed:                   REVISION++
LT_CURRENT=0
LT_AGE=0
LT_REVISION=1
AC_SUBST(LT_CURRENT)
AC_SUBST(LT_AGE)
AC_SUBST(LT_REVISION)

AM_INIT_AUTOMAKE
AM_CONFIG_HEADER(config.h)

# Checks for programs.
AC_PROG_LIBTOOL
AC_PROG_CC

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS(unistd.h)

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_SIZE_T

# Checks for library functions.
AC_CHECK_FUNCS([memset strchr strdup])

AC_CONFIG_FILES([Makefile libntlm.pc ntlm.h])
AC_OUTPUT
