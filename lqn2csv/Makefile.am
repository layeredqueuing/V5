#----------------------------------------------------------------------
# $Id: Makefile.am 16118 2022-11-17 18:45:33Z greg $
#----------------------------------------------------------------------

AM_CPPFLAGS	= -I..  -I../lqiolib/src/headers -I../lqx/libsrc/headers
lqn2csv_CXXFLAGS = -std=c++11 -Wall -Wno-unknown-pragmas
bin_PROGRAMS 	= lqn2csv
lqn2csv_SOURCES = gnuplot.cc lqn2csv.cc model.cc
noinst_HEADERS  = gnuplot.h lqn2csv.h model.h
lqn2csv_LDADD	= -L../lqiolib/src/.libs -llqio

lqn2csv_LDFLAGS= @STATIC@

EXTRA_DIST	= lqn2csv.1

man1_MANS	= lqn2csv.1

if HAVE_PS2PDF
install-data-local:	lqn2csv.pdf

lqn2csv.pdf:	$(man1_MANS)
		groff -man $(man1_MANS)  > $<.ps
		ps2pdf $<.ps $@
		rm $<.ps
endif

