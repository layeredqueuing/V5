# 
# $Id: Makefile.am 14823 2021-06-15 18:07:36Z greg $
#
#----------------------------------------------------------------------

ACLOCAL_AMFLAGS=-I ../config
SUBDIRS		= regression 
AM_CPPFLAGS	= -I.. -I../lqiolib/src/headers -I../lqx/libsrc/headers -I../libmva/src/headers
bin_PROGRAMS 	= lqns
lqns_SOURCES	= activity.cc actlist.cc call.cc entity.cc entry.cc \
		  entrythread.cc errmsg.cc gamma.cc generate.cc group.cc help.cc interlock.cc \
		  lqns.cc model.cc option.cc overtake.cc  \
		  phase.cc pragma.cc processor.cc randomvar.cc report.cc runlqx.cc \
		  slice.cc submodel.cc synmodel.cc task.cc variance.cc
noinst_HEADERS	= activity.h actlist.h call.h entity.h entry.h \
		  entrythread.h errmsg.h gamma.h generate.h group.h help.h interlock.h \
		  lqns.h model.h option.h overtake.h
		  phase.h pragma.h processor.h randomvar.h report.h runlqx.h \
		  slice.h submodel.h synmodel.h task.h variance.h 

EXTRA_DIST	= lqns.1 

lqns_CXXFLAGS   = -std=c++11 -Wall -Wno-parentheses -Wno-unknown-pragmas -Wno-overloaded-virtual
lqns_LDADD	= -L../lqx/libsrc/.libs -llqx -L../lqiolib/src/.libs -llqio  -L../libmva/src/.libs -lmva
lqns_LDFLAGS	= @STATIC@
man1_MANS	= lqns.1

clean-local:

if HAVE_PS2PDF
install-data-local:	lqns.pdf
endif

distclean-local:
		rm -f config.cache

lqns.ps:	$(man1_MANS)
		groff -t -man  $(man1_MANS) > lqns.ps

if HAVE_PS2PDF
lqns.pdf:	lqns.ps
		ps2pdf lqns.ps
endif

lqns.html:	$(man1_MANS)
		groff -t -man -Thtml $(man1_MANS) > lqns.html
