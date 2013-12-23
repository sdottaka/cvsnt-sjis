## Process this file with automake to produce Makefile.in
# Makefile for library files used by GNU CVS.
# Copyright (C) 1986, 1988-1994, 2000 Free Software Foundation, Inc.

# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# For now we need to include $(top_srcdir)/src because some systems
# (at least 'AIX rioscpu2 3 4 000030498200',
# 'HP-UX hp60 B.10.20 A 9000/770 hp60 two-user license', &
# 'IRIX64 sgiop110 6.5 07151433 IP30') have trouble finding error.h
# when compiling savecwd.c
#
# $(includeopt) is CVS specific and set by configure
AM_CPPFLAGS = -I$(top_srcdir)/src -I$(top_srcdir)/lib $(includeopt)
AM_CFLAGS=-DCVS_CONFIG_DIR=\"$(cvs_config_dir)\" -DCVS_LIBRARY_DIR=\"$(cvs_library_dir)\"

libdir=$(cvs_library_dir)

if PSERVER_PROTOCOL_SUPPORT

pserver_target = pserver_protocol.la

pserver_protocol_la_SOURCES = \
	pserver.c \
	scramble.c \
	common.c 

pserver_protocol_la_LDFLAGS = -module -avoid-version

else

pserver_target = 

endif 

if SSERVER_PROTOCOL_SUPPORT

sserver_target = sserver_protocol.la

sserver_protocol_la_SOURCES = \
	sserver.c \
	scramble.c \
	common.c 

sserver_protocol_la_LDFLAGS = -module -avoid-version

else

sserver_target = 

endif 

if SSPI_PROTOCOL_SUPPORT

sspi_target = sspi_protocol.la
ntlm_dir = ntlm

sspi_protocol_la_SOURCES = \
	sspi_unix.c \
	scramble.c \
	common.c 

sspi_protocol_la_LDFLAGS = -module -avoid-version ntlm/libntlm.la

else

sspi_target = 
ntlm_dir =

endif 

if EXT_PROTOCOL_SUPPORT

ext_target = ext_protocol.la

ext_protocol_la_SOURCES = \
	ext.c \
	common.c 

ext_protocol_la_LDFLAGS = -module -avoid-version

else

ext_target = 

endif 

if FORK_PROTOCOL_SUPPORT

fork_target = fork_protocol.la

fork_protocol_la_SOURCES = \
	fork.c \
	common.c 

fork_protocol_la_LDFLAGS = -module -avoid-version

else

fork_target = 

endif 

if SERVER_PROTOCOL_SUPPORT

server_target = server_protocol.la

server_protocol_la_SOURCES = \
	server.c \
	common.c 

server_protocol_la_LDFLAGS = -module -avoid-version

else

server_target = 

endif 

if GSERVER_PROTOCOL_SUPPORT

gserver_target = gserver_protocol.la

gserver_protocol_la_SOURCES = \
	gserver.c \
	common.c

gserver_protocol_la_LDFLAGS = -module -avoid-version

else

gssapi_target = 

endif

SUBDIRS = $(ntlm_dir)

lib_LTLIBRARIES = $(pserver_target) \
	      $(sserver_target) \
	      $(sspi_target) \
	      $(gserver_target) \
	      $(ext_target) \
	      $(fork_target) \
	      $(server_target)

EXTRA_DIST = \
	.cvsignore 

# for backwards compatibility with the old makefiles
realclean: maintainer-clean
.PHONY: realclean
