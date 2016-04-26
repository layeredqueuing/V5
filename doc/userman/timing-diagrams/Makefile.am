# Makefile for bookstore
# Copyright (C) 2005 Greg Franks <rgfranks@alumni.uwaterloo.ca>
# ------------------------------------------------------------------------
# $Id: Makefile.am 11963 2014-04-10 14:36:42Z greg $
# ------------------------------------------------------------------------

SRCS=	join-time.fig \
	service-time.fig \
	holding-time.fig

EXTRA_DIST = $(SRCS)

SUFFIXES = .pdf .ps .dvi .bbl .eps .pstex .eps .fig .tex

.fig.eps:
	fig2dev -L eps -m 0.75 $< $@

noinst_DATA = $(SRCS:.fig=.eps) 

distclean-local:
	rm -f $(noinst_DATA)

