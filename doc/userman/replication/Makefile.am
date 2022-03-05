# Makefile for bookstore
# Copyright (C) 2005 Greg Franks <rgfranks@alumni.uwaterloo.ca>
#
# ------------------------------------------------------------------------
# $Id: Makefile.am 15446 2022-03-05 00:01:59Z greg $
# ------------------------------------------------------------------------

SRCS=	flat.fig          multiserver.fig   replicaserver.fig replicated.fig    
OBJS = $(SRCS:.fig=.eps) 

EXTRA_DIST = $(SRCS) $(OBJS)
pkgdata_DIST =  $(OBJS)

SUFFIXES = .pdf .ps .dvi .bbl .eps .pstex .eps .fig .tex

.fig.eps:
if HAVE_FIG2DEV
	fig2dev -L eps -m 0.75 $< $@
endif

distclean-local:
	rm -f *.bak

