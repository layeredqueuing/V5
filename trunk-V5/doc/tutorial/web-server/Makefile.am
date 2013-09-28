# Copyright the Real-Time and Distributed Systems Group,
# Department of Systems and Computer Engineering,
# Carleton University, Ottawa, Ontario, Canada. K1S 5B6
#
#----------------------------------------------------------------------
# $Id$
#----------------------------------------------------------------------

SUBDIRS = 
SRCS =	web-server-class.fig	web-server-deploy.fig	web-server.fig

EXTRA_DIST = $(SRCS) reserve-templ.lqnx

SUFFIXES = .pdf .ps .dvi .bbl .eps .pstex .eps .fig .tex

noinst_DATA = $(SRCS:.fig=.eps) 

.fig.eps:
	fig2dev -L eps $< $@

clean-local:
	rm -f $(OBJS)  $(DST).aux $(DST).bbl $(DST).blg $(DST).dvi $(DST).log $(DST).pdf $(DST).ps $(DST).out *.fig.bak *.eps
