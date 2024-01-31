# Makefile for .
# Copyright (C) 2003 Greg Franks <rgfranks@alumni.uwaterloo.ca>
# $Id: Makefile.am 16989 2024-01-31 11:40:29Z greg $
# ----------------------------------------------------------------------

AM_CPPFLAGS	= -I.. -I../lqiolib/src/headers -I../lqx/libsrc/headers 
bin_PROGRAMS 	= lqngen
OTHEREXE	= lqn2lqx
lqngen_SOURCES  = lqngen.cc generate.cc help.cc randomvar.cc 
noinst_HEADERS 	= generate.h help.h lqngen.h randomvar.h 

EXTRA_DIST	= lqngen.1 lqn2lqx.1

lqngen_CXXFLAGS = -std=c++17 -Wall -Wno-parentheses -Wno-switch -Wno-unknown-pragmas -Wno-overloaded-virtual
lqngen_LDADD 	= -L../lqiolib/src/.libs -llqio -L../lqx/libsrc/.libs -llqx 
lqngen_LDFLAGS	= @STATIC@

man1_MANS	= lqngen.1 lqn2lqx.1

clean-local:

distclean-local:
	rm -f config.cache

install-exec-hook:
	for i in $(OTHEREXE); do \
	  (cd $(DESTDIR)$(bindir); rm -f $$i$(EXEEXT); $(LN_S) lqngen$(EXEEXT) $$i$(EXEEXT)); \
	done

if HAVE_PS2PDF
all-local: lqngen.pdf lqn2lqx.pdf

install-data-local: lqngen.pdf lqn2lqx.pdf

lqngen.pdf: 	lqngen.1
		groff -man lqngen.1  > $<.ps
		ps2pdf $<.ps $@
		rm $<.ps

lqn2lqx.pdf: 	lqn2lqx.1
		groff -man lqn2lqx.1  > $<.ps
		ps2pdf $<.ps $@
		rm $<.ps
endif
