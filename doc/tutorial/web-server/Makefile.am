# Copyright the Real-Time and Distributed Systems Group,
# Department of Systems and Computer Engineering,
# Carleton University, Ottawa, Ontario, Canada. K1S 5B6
#
#----------------------------------------------------------------------
# $Id: Makefile.am 15463 2022-03-10 23:32:21Z greg $
#----------------------------------------------------------------------

SUBDIRS = 
SRCS =	web-server-class.fig	web-server-deploy.fig	web-server.fig
OBS = $(SRCS:.fig=.eps) 

EXTRA_DIST = $(SRCS) reserve-templ.lqnx reserve-templ.lqn
pkgdata_DATA = $(OBJS) 

SUFFIXES = .pdf .ps .dvi .bbl .eps .pstex .eps .fig .tex

.fig.eps:
if HAVE_FIG2DEV
	fig2dev -L eps $< $@
endif

clean-local:
	rm -f $(OBJS)  $(DST).aux $(DST).bbl $(DST).blg $(DST).dvi $(DST).log $(DST).pdf $(DST).ps $(DST).out *.fig.bak
