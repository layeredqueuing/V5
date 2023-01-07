# Makefile for bookstore
# Copyright (C) 2005 Greg Franks <rgfranks@alumni.uwaterloo.ca>
# ------------------------------------------------------------------------
# $Id: Makefile.am 16299 2023-01-07 17:01:32Z greg $
# ------------------------------------------------------------------------

SRCS=	join-time.fig \
	service-time.fig \
	holding-time.fig
OBJS = $(SRCS:.fig=.eps) 

SUFFIXES = .pdf .ps .dvi .bbl .eps .pstex .eps .fig .tex

EXTRA_DIST = $(SRCS)
noinst_DATA = $(OBJS)

.fig.eps:
if HAVE_FIG2DEV
	fig2dev -L eps -m 0.75 $< $@
endif

distclean-local:
	rm -f *.bak

