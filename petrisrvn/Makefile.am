#
#----------------------------------------------------------------------
# $Id$
#----------------------------------------------------------------------

ACLOCAL_AMFLAGS=-I ../config
SUBDIRS=	wspnlib regression
AM_CPPFLAGS =	-I.. -I../lqiolib/src/headers -I../lqx/libsrc/headers/
bin_PROGRAMS=	petrisrvn
petrisrvn_SOURCES = actlist.cc \
	activity.cc \
	entry.cc \
	errmsg.cc \
	makeobj.cc \
	model.cc \
	petrisrvn.cc \
	phase.cc \
	place.cc \
	pragma.cc \
	processor.cc \
	results.cc \
	runlqx.cc \
	task.cc

noinst_HEADERS=	activity.h \
	actlist.h \
	config.h \
	entry.h \
	errmsg.h \
	makeobj.h \
	model.h \
	petrisrvn.h \
	phase.h \
	place.h \
	pragma.h \
	processor.h \
	stack.h \
	results.h \
	runlqx.h \
	task.h

EXTRA_DIST 	= README NEWS ChangeLog AUTHORS COPYING

petrisrvn_CXXFLAGS  = -Wall -Wno-parentheses -Wno-unknown-pragmas
petrisrvn_LDADD	    = -L../lqx/libsrc/.libs -llqx -L../lqiolib/src/.libs -llqio -Lwspnlib -lwspn
petrisrvn_LDFLAGS   = @STATIC@
man1_MANS	    = petrisrvn.1
