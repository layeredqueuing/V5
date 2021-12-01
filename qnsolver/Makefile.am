# 
# $Id: Makefile.am 15135 2021-12-01 14:07:41Z greg $
#
#----------------------------------------------------------------------

ACLOCAL_AMFLAGS=-I ../config
AM_CPPFLAGS	= -I.. -I../lqiolib/src/headers -I../lqx/libsrc/headers -I../libmva/src/headers
bin_PROGRAMS 	= qnsolver
qnsolver_SOURCES= pragma.cc qnsolver.cc 
if EXPAT
qnsolver_SOURCES+=closedmodel.cc openmodel.cc model.cc runlqx.cc
endif
noinst_HEADERS	= closedmodel.h openmodel.h model.h pragma.h runlqx.h
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

lqns.ps:	$(man1_MANS)
		tbl < $(man1_MANS)  | groff -man > lqns.ps

lqns.pdf:	qnsolver.ps
		ps2pdf qnsolver.ps

lqns.txt:	$(man1_MANS)
		tbl < $(man1_MANS)  | nroff -man | col -b > lqns.txt

if HAVE_PS2PDF
install-data-local:	qnsolver.pdf
endif

qnsolver.ps: $(man1_MANS)
	groff -t -man $(man1_MANS) > qnsolver.ps

if HAVE_PS2PDF
qnsolver.pdf: qnsolver.ps
	ps2pdf qnsolver.ps

endif


