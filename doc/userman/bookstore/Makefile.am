# Makefile for bookstore
# Copyright (C) 2005 Greg Franks <rgfranks@alumni.uwaterloo.ca>
# $Id: Makefile.am 10133 2011-01-03 19:31:58Z greg $

SRCS=	bookstore-result.fig     bookstore-service.fig 		bookstore.fig

EXTRA_DIST = $(SRCS)

SUFFIXES = .pdf .ps .dvi .bbl .eps .pstex .eps .fig .tex

.fig.eps:
	fig2dev -L eps -m 0.5 $< $@

noinst_DATA = $(SRCS:.fig=.eps) 

distclean-local:
	rm -f $(noinst_DATA)

bookstore-result.eps: bookstore-result.fig
	fig2dev -L eps -m 0.75 $< $@

bookstore-service.eps: bookstore-service.fig
	fig2dev -L eps -m 0.75 $< $@
