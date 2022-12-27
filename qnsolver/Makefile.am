#----------------------------------------------------------------------
# $Id: Makefile.am 16204 2022-12-27 15:42:27Z greg $
#----------------------------------------------------------------------

AM_CPPFLAGS	= -I.. -I../lqiolib/src/headers -I../lqx/libsrc/headers -I../libmva/src/headers
bin_PROGRAMS 	= qnsolver
qnsolver_SOURCES= pragma.cc qnsolver.cc boundsmodel.cc closedmodel.cc openmodel.cc model.cc runlqx.cc
if EXPAT
qnsolver_SOURCES+=
endif
noinst_HEADERS	= boundsmodel.h closedmodel.h openmodel.h model.h pragma.h runlqx.h
if EXPAT
endif

EXTRA_DIST	= qnsolver.1 

qnsolver_CXXFLAGS= -std=c++11 -Wall -Wno-parentheses -Wno-unknown-pragmas -Wno-overloaded-virtual
qnsolver_LDADD	= -L../lqx/libsrc/.libs -llqx -L../lqiolib/src/.libs -llqio -L../libmva/src/.libs -lmva 
qnsolver_LDFLAGS= @STATIC@
man1_MANS	= qnsolver.1

clean-local:

distclean-local:
		rm -f config.cache

if HAVE_PS2PDF
all-local:	qnsolver.pdf

install-data-local:	qnsolver.pdf

qnsolver.pdf:	$(man1_MANS)
		groff -man $<  > $<.ps
		ps2pdf $<.ps $@
		rm $<.ps
endif
