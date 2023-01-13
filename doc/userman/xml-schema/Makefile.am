# Makefile for bookstore
# Copyright (C) 2005 Greg Franks <rgfranks@alumni.uwaterloo.ca>
# ------------------------------------------------------------------------
# $Id: Makefile.am 16324 2023-01-12 17:44:44Z greg $
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

OBJS = $(SRCS:.fig=.eps) 

EXTRA_DIST = $(SRCS) $(OBJS)
noinst_DATA = $(OBJS)

SUFFIXES = .pdf .ps .dvi .bbl .eps .pstex .eps .fig .tex

.fig.eps:
if HAVE_FIG2DEV
	fig2dev -L eps -m 0.75 $< $@
endif


distclean-local:
	rm -f *.bak

