# 
# $Id: Makefile.am 14321 2021-01-02 15:35:47Z greg $
#
#----------------------------------------------------------------------

ACLOCAL_AMFLAGS=-I ../config
AM_CPPFLAGS	= -I.. -I../lqiolib/src/headers -I../lqx/libsrc/headers -I../libmva/src/headers
bin_PROGRAMS 	= qnsolver
qnsolver_SOURCES= bcmpmodel.cc bcmpresult.cc qnsolver.cc
noinst_HEADERS	= bcmpmodel.h bcmpresult.h

#EXTRA_DIST	= qnsolver.1 

qnsolver_CXXFLAGS= -Wall -Wno-parentheses -Wno-unknown-pragmas -Wno-overloaded-virtual
qnsolver_LDADD	= -L../lqiolib/src/.libs -llqio -L../libmva/src/.libs -lmva
qnsolver_LDFLAGS= @STATIC@
#man1_MANS	= qnsolver.1

clean-local:

#install-data-local:	qnsolver.pdf

distclean-local:
		rm -f config.cache

lqns.ps:	$(man1_MANS)
		tbl < $(man1_MANS)  | groff -man > lqns.ps

lqns.pdf:	qnsolver.ps
		ps2pdf qnsolver.ps

lqns.txt:	$(man1_MANS)
		tbl < $(man1_MANS)  | nroff -man | col -b > lqns.txt
