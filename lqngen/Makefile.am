# Makefile for .
# Copyright (C) 2003 Greg Franks <rgfranks@alumni.uwaterloo.ca>
# $Id: Makefile.am 14429 2021-01-29 20:49:19Z greg $
# ----------------------------------------------------------------------
ACLOCAL_AMFLAGS=-I ../config
SUBDIRS		= 

AM_CPPFLAGS	= -I.. -I../lqiolib/src/headers -I../lqx/libsrc/headers 
bin_PROGRAMS 	= lqngen
OTHEREXE	= lqn2lqx
lqngen_SOURCES  = lqngen.cc generate.cc help.cc randomvar.cc 
noinst_HEADERS 	= generate.h help.h lqngen.h randomvar.h 

EXTRA_DIST	= lqngen.1 lqn2lqx.1

lqngen_CXXFLAGS = -std=c++11 -Wall -Wno-parentheses -Wno-switch -Wno-unknown-pragmas -Wno-overloaded-virtual
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

if HAVE_PS2PDF
install-data-local: lqngen.pdf lqn2lqx.pdf
endif

lqngen.ps: lqngen.1
	cat lqngen.1  | groff -man > lqngen.ps

lqn2lqx.ps: lqn2lqx.1
	cat lqn2lqx.1  | groff -man > lqn2lqx.ps

if HAVE_PS2PDF
lqngen.pdf: lqngen.ps
	ps2pdf lqngen.ps

lqn2lqx.pdf: lqn2lqx.ps
	ps2pdf lqn2lqx.ps
endif
