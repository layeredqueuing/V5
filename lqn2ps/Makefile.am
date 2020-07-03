# Makefile for .
# Copyright (C) 2003 Greg Franks <rgfranks@alumni.uwaterloo.ca>
# $Id: Makefile.am 13477 2020-02-08 23:14:37Z greg $
# ----------------------------------------------------------------------
ACLOCAL_AMFLAGS=-I ../config 
SUBDIRS		= 
if COND_PNG
  MAYBE_PNG = lqn2png
endif
if COND_JPEG
  MAYBE_JPEG = lqn2jpeg
endif
if COND_GIF
  MAYBE_GIF = lqn2gif
endif

AM_CPPFLAGS	= -I.. -I../lqiolib/src/headers -I../lqx/libsrc/headers
bin_PROGRAMS 	= lqn2ps 
OTHEREXE	= lqn2emf lqn2fig lqn2out lqn2lqn lqn2svg lqn2xml $(MAYBE_PNG) $(MAYBE_JPEG) $(MAYBE_GIF) rep2flat 
lqn2ps_SOURCES 	= activity.cc actlayer.cc actlist.cc arc.cc call.cc share.cc element.cc entity.cc entry.cc errmsg.cc \
		getopt2.cc graphic.cc group.cc help.cc key.cc label.cc layer.cc lqn2ps.cc main.cc model.cc node.cc \
		open.cc phase.cc point.cc processor.cc runlqx.cc task.cc

noinst_HEADERS 	= actlayer.h activity.h actlist.h arc.h call.h share.h element.h entity.h entry.h errmsg.h \
		getopt2.h graphic.h group.h help.h key.h label.h layer.h lqn2ps.h model.h node.h \
		open.h phase.h point.h processor.h runlqx.h task.h

EXTRA_DIST	= lqn2ps.1 

lqn2ps_CXXFLAGS = -Wall -Wno-parentheses -Wno-switch -Wno-unknown-pragmas -Wno-overloaded-virtual
lqn2ps_LDADD	= -L../lqx/libsrc/.libs -llqx -L../lqiolib/src/.libs -llqio 
lqn2ps_LDFLAGS	= @STATIC@

man1_MANS	= lqn2ps.1 

clean-local:

distclean-local:
	rm -f config.cache

install-exec-hook:
	for i in $(OTHEREXE); do \
	  $(RM) $(DESTDIR)$(bindir)/$$i$(EXEEXT); \
	  (cd $(DESTDIR)$(bindir); $(LN_S) lqn2ps$(EXEEXT) $$i$(EXEEXT)); \
	done

install-data-hook:
	for i in $(OTHEREXE); do \
	  $(RM) $(DESTDIR)$(mandir)/man1/$$i.1; \
	  (cd $(DESTDIR)$(mandir)/man1; $(LN_S) lqn2ps.1 $$i.1); \
	done

install-data-local: lqn2ps.pdf 

lqn2ps.ps: lqn2ps.1
	cat lqn2ps.1 | groff -man > lqn2ps.ps

lqn2ps.pdf: lqn2ps.ps
	ps2pdf lqn2ps.ps

lqn2ps.txt: lqn2ps.1
	nroff -man lqn2ps.1 | col -b > lqn2ps.txt

