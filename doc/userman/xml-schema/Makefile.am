# Makefile for bookstore
# Copyright (C) 2005 Greg Franks <rgfranks@alumni.uwaterloo.ca>
# ------------------------------------------------------------------------
# $Id: Makefile.am 14618 2021-04-21 14:24:22Z greg $
# ------------------------------------------------------------------------

SRCS=	activity-schema.fig \
	call-schema.fig \
	distribution-schema.fig \
	entry-schema.fig \
	lqn-model-schema.fig \
	precedence-schema.fig \
	processor-schema.fig \
	result-join-delay-schema.fig \
	result-schema.fig \
	schema.fig \
	task-schema.fig

EXTRA_DIST = $(SRCS)

SUFFIXES = .pdf .ps .dvi .bbl .eps .pstex .eps .fig .tex

.fig.eps:
	fig2dev -L eps -m 0.75 $< $@

noinst_DATA = $(SRCS:.fig=.eps) 

distclean-local:
	rm -f *.bak

