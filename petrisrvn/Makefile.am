#----------------------------------------------------------------------
# $Id: Makefile.am 17584 2025-11-12 17:06:47Z greg $
#----------------------------------------------------------------------

SUBDIRS=	wspnlib regression
AM_CPPFLAGS =	-I../lqiolib/src/headers -I../lqx/libsrc/headers/
bin_PROGRAMS=	petrisrvn
petrisrvn_SOURCES = actlist.cc \
	activity.cc \
	call.cc \
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
	call.h \
	entry.h \
	errmsg.h \
	makeobj.h \
	model.h \
	petrisrvn.h \
	phase.h \
	place.h \
	pragma.h \
	processor.h \
	results.h \
	runlqx.h \
	task.h

EXTRA_DIST 	= petrisrvn.1

petrisrvn_CXXFLAGS  = -std=gnu++17 -Wall -Wno-parentheses -Wno-unknown-pragmas
petrisrvn_LDADD	    = -L../lqx/libsrc/.libs -llqx -L../lqiolib/src/.libs -llqio -Lwspnlib -lwspn
petrisrvn_LDFLAGS   = @STATIC@
man1_MANS	    = petrisrvn.1

if HAVE_PS2PDF
all-local:	petrisrvn.pdf

install-data-local:	petrisrvn.pdf

petrisrvn.pdf:	$(man1_MANS)
		groff -t -man $(man1_MANS)  > $<.ps
		ps2pdf $<.ps $@
		rm $<.ps
endif
