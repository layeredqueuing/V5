# Copyright the Real-Time and Distributed Systems Group,
# Department of Systems and Computer Engineering,
# Carleton University, Ottawa, Ontario, Canada. K1S 5B6
#
#----------------------------------------------------------------------
# $Id: Makefile.am 15446 2022-03-05 00:01:59Z greg $
#----------------------------------------------------------------------

SUBDIRS = 
SRCS =	fwd-seq.fig rnv-seq.fig snr-seq.fig fwd.fig rnv.fig snr.fig
OBJS = $(SRCS:.fig=.eps) 

EXTRA_DIST = $(SRCS)
pkgdata_DIST = $(OBJS)

SUFFIXES = .pdf .ps .dvi .bbl .eps .pstex .eps .fig .tex

.fig.eps:
if HAVE_FIG2DEV
	fig2dev -L eps $< $@
endif

clean-local:
	rm -f $(OBJS)  $(DST).aux $(DST).bbl $(DST).blg $(DST).dvi $(DST).log $(DST).pdf $(DST).ps $(DST).out *.fig.bak
