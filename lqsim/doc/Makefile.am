# Makefile for documentation
# Copyright (C) 2004 Greg Franks <greg@sce.carleton.ca>
#
# ------------------------------------------------------------------------
# $Id: Makefile.am 9228 2010-03-03 15:57:42Z greg $
# ------------------------------------------------------------------------

SRCS=	client-server-seq.fig	lqsim-front.fig		lqsim-seq.fig \
	lqiolib.fig		lqsim-seq-2.fig		client-multiserver-seq.fig

EXTRA_DIST = $(SRCS)

SUFFIXES = .pdf .ps .svg .dvi .bbl .eps .pstex .eps .fig .tex

.fig.eps:
	fig2dev -L eps -m 0.75 $< $@

.fig.pdf:
	fig2dev -L pdf -m 0.75 $< $@

.fig.svg:
	fig2dev -L svg $< $@

.fig.png:
	fig2dev -L png -m2.5 $< $@


noinst_DATA = $(SRCS:.fig=.eps) 

distclean-local:
	rm -f *.bak *~

