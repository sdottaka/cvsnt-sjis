## Process this file with automake to produce Makefile.in
# Makefile for GNU CVS program.
# Copyright (C) 1986, 1988-1990, 2000 Free Software Foundation, Inc.

# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

SHELL = /bin/sh

# $(includeopt) is CVS specific and set by configure
# FIXME - This includes line is dependant on its order.  This means there is
# some namespace hackery going on that maybe shouldn't be.  Long term fix is to
# try and remove naming ocnflicts and fix Automake to allow particular includes
# to be attached only to particular object files.  Short term fix is either or.
##INCLUDES = -I. -I.. -I$(srcdir) -I$(top_srcdir)/lib

if WITH_ZLIB
  zlib_inc = 
  zlib_lib =
else
  zlib_inc = -I$(top_srcdir)/zlib
  zlib_lib = ../zlib/libz.a 
endif

if WITH_CVSGUI
  cvsgui_inc = -I$(top_srcdir)/cvsgui
  cvsgui_lib = ../cvsgui/libcvsgui.a
else
  cvsgui_inc = 
  cvsgui_lib =
endif

INCLUDES = -I$(top_srcdir)/lib -I$(top_srcdir)/diff $(cvsgui_inc) $(zlib_inc) $(includeopt) $(INCLTDL)

bin_PROGRAMS = cvs

# The cvs executable
cvs_SOURCES = \
	add.c \
	admin.c \
	annotate.c \
	buffer.c \
	chacl.c \
	chown.c \
	checkin.c \
	checkout.c \
	classify.c \
	client.c \
	commit.c \
	create_adm.c \
	cvsrc.c \
	cvsrcs.c \
	diff.c \
	edit.c \
	entries.c \
	error.c \
	expand_path.c \
	fileattr.c \
	filesubr.c \
	find_names.c \
	hardlink.c \
	hash.c \
	history.c \
	ignore.c \
	info.c \
	infolib_unix.cpp \
	import.c \
	library_unix.c \
	lock.c \
	log.c \
	login.c \
	logmsg.c \
	ls.c \
	lsacl.c \
	main.c \
	mkmodules.c \
	modules.c \
	myndbm.c \
	no_diff.c \
	parseinfo.c \
	passwd.c \
	patch.c \
	perms.c \
	rcs.c \
	rcscmds.c \
	recurse.c \
	release.c \
	remove.c \
	repos.c \
	root.c \
	run.c \
	scramble.c \
	server.c \
	status.c \
	subr.c \
	tag.c \
	update.c \
	version.c \
	vers_ts.c \
	watch.c \
	wrapper.c \
	zlib.c \
	buffer.h \
	client.h \
	cvs.h \
	edit.h \
	error.h \
	fileattr.h \
	hardlink.h \
	hash.h \
	myndbm.h \
	rcs.h \
	server.h \
	update.h \
	watch.h
cvs_LDADD = \
	../diff/libdiff.a \
	../lib/libcvs.a \
	$(cvsgui_lib) \
	$(zlib_lib) \
	$(LIBLTDL)

# extra clean targets
# wish this could be distclean-hdr-local but it's not part of automake
DISTCLEANFILES = options.h-SAVED check.log check.plog

# General
EXTRA_DIST = \
	.cvsignore \
	ChangeLog-9194 \
	ChangeLog-9395 \
	ChangeLog-96 \
	ChangeLog-97 

## MAINTAINER Targets

# for backwards compatibility with the old makefiles
.PHONY: realclean
realclean: maintainer-clean
