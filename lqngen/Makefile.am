# Makefile for .
# Copyright (C) 2003 Greg Franks <rgfranks@alumni.uwaterloo.ca>
# $Id: Makefile.am 12297 2015-11-17 21:10:22Z greg $
# ----------------------------------------------------------------------
ACLOCAL_AMFLAGS=-I ../config
SUBDIRS		= 

AM_CPPFLAGS	= -I.. -I../lqiolib/src/headers -I../lqx/libsrc/headers
bin_PROGRAMS 	= lqngen
OTHEREXE	= lqn2lqx
lqngen_SOURCES  = lqngen.cc generate.cc help.cc randomvar.cc 
noinst_HEADERS 	= generate.h help.h lqngen.h randomvar.h 

EXTRA_DIST	= lqngen.1 lqn2lqx.1

lqngen_CXXFLAGS = -Wall -Wno-parentheses -Wno-switch -Wno-unknown-pragmas -Wno-overloaded-virtual
lqngen_LDADD 	= -L../lqiolib/src/.libs -llqio -L../lqx/libsrc/.libs -llqx 
lqngen_LDFLAGS	= @STATIC@

man1_MANS	= lqngen.1 lqn2lqx.1

clean-local:

distclean-local:
	rm -f config.cache

install-exec-hook:
	for i in $(OTHEREXE); do \
	  $(RM) $(DESTDIR)$(bindir)/$$i$(EXEEXT); \
	  (cd $(DESTDIR)$(bindir); $(LN_S) lqngen$(EXEEXT) $$i$(EXEEXT)); \
	done

install-data-local: lqngen.pdf lqn2lqx.pdf

lqngen.ps: lqngen.1
	cat lqngen.1  | groff -man > lqngen.ps

lqngen.pdf: lqngen.ps
	ps2pdf lqngen.ps

lqn2lqx.ps: lqn2lqx.1
	cat lqn2lqx.1  | groff -man > lqn2lqx.ps

lqn2lqx.pdf: lqn2lqx.ps
	ps2pdf lqn2lqx.ps
