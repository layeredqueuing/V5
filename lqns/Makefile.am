# 
# $Id: Makefile.am 14322 2021-01-02 22:44:33Z greg $
#
#----------------------------------------------------------------------

ACLOCAL_AMFLAGS=-I ../config
SUBDIRS		= regression 
AM_CPPFLAGS	= -I.. -I../lqiolib/src/headers -I../lqx/libsrc/headers -I../libmva/src/headers
bin_PROGRAMS 	= lqns
lqns_SOURCES	= activity.cc actlist.cc call.cc entity.cc entry.cc \
		  entrythread.cc errmsg.cc gamma.cc generate.cc group.cc help.cc interlock.cc \
		  model.cc option.cc overtake.cc  \
		  phase.cc pragma.cc processor.cc randomvar.cc report.cc runlqx.cc \
		  slice.cc lqns.cc submodel.cc  \
		  synmodel.cc task.cc variance.cc
noinst_HEADERS	= activity.h actlist.h call.h dim.h entity.h entry.h entrythread.h \
		  errmsg.h gamma.h generate.h group.h help.h interlock.h model.h \
		  option.h overtake.h phase.h pragma.h \
		  processor.h randomvar.h report.h slice.h lqns.h runlqx.h \
		  submodel.h  synmodel.h task.h variance.h 

EXTRA_DIST	= lqns.1 

lqns_CXXFLAGS   = -Wall -Wno-parentheses -Wno-unknown-pragmas -Wno-overloaded-virtual
lqns_LDADD	= -L../lqx/libsrc/.libs -llqx -L../lqiolib/src/.libs -llqio  -L../libmva/src/.libs -lmva
lqns_LDFLAGS	= @STATIC@
man1_MANS	= lqns.1

clean-local:

install-data-local:	lqns.pdf

distclean-local:
		rm -f config.cache

lqns.ps:	$(man1_MANS)
		tbl < $(man1_MANS)  | groff -man > lqns.ps

lqns.pdf:	lqns.ps
		ps2pdf lqns.ps

lqns.txt:	$(man1_MANS)
		tbl < $(man1_MANS)  | nroff -man | col -b > lqns.txt
