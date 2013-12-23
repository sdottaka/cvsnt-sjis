## Process this file with automake to produce Makefile.in
# Makefile for GNU CVS documentation (excluding man pages - see ../man).
# Copyright (C) 1986, 1988-1990, 2000 Free Software Foundation, Inc.

# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# FIXME - these rebuild every 'make dist' with some BSD makes due to a bug
# that prevents make from searching VPATH for targets.  There is some
# machinery in place in configure.in to work around this for some targets
# (see the CVSvn.texi target below) now, but fixing the two info_TEXINFOS
# targets below is probably going to require work on Automake
info_TEXINFOS = cvs.texi cvsclient.texi
cvs_TEXINFOS = CVSvn.texi
cvsclient_TEXINFOS = CVSvn.texi

POSTSCRIPTS = \
	cvs.ps \
	cvsclient.ps

PDFS = \
	cvs.pdf \
	cvsclient.pdf

TXTS = \
	cvs.txt \
	cvsclient.txt

HTMLS = \
	cvs.html \
	cvsclient.html 

EXTRA_DIST = \
	.cvsignore \
	RCSFILES

# Listing cvs.cps & cvs.fns here is a workaround for an Automake deficiency
MOSTLYCLEANFILES = \
	cvs.cps \
	cvs.fns

CLEANFILES = \
	$(PDFS) \
	$(TXTS) \
	$(HTMLS) \
	$(POSTSCRIPTS)

ps: $(POSTSCRIPTS)
.PHONY: ps

pdf: $(PDFS)
.PHONY: pdf

txt: $(TXTS)
.PHONY: txt

html: $(HTMLS)
.PHONY: html

doc: ps html pdf txt
.PHONY: doc

all: doc

SUFFIXES = .txt .pdf .html

# texinfo based targets automake neglects to include
.texi.pdf:
	pdftex $<
	texindex $(basename $<).??
	pdftex $<

.texi.txt:
	TEXINPUTS=$(srcdir):$$TEXINPUTS \
	  MAKEINFO='$(MAKEINFO) -I $(srcdir)' $(MAKEINFO) $< --no-headers -o $@

.texi.html:
	TEXINPUTS=$(srcdir):$$TEXINPUTS \
	  MAKEINFO='$(MAKEINFO) -I $(srcdir)' $(MAKEINFO) $< --html --no-split -o $@

## realclean ##
# for backwards compatibility with the old makefiles
realclean: maintainer-clean
.PHONY: realclean
