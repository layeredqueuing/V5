# Copyright the Real-Time and Distributed Systems Group,
# Department of Systems and Computer Engineering,
# Carleton University, Ottawa, Ontario, Canada. K1S 5B6
#
#----------------------------------------------------------------------
# $Id: Makefile.am 15463 2022-03-10 23:32:21Z greg $
#----------------------------------------------------------------------

SUBDIRS = 
SRCS =  activity-model.fig	flat-model.fig		phase-entry.fig \
	basic-lqn.fig		layered-model.fig	phase-seq.fig \
	buffer-seq.fig		multi-servers.fig	service-time.fig \
	buffer.fig		notation.fig		task-queue.fig \
	elements.fig		phase-activity.fig      service-time-seq.fig 
OBJS = $(SRCS:.fig=.eps) 

EXTRA_DIST = $(SRCS) $(OBJS)
pkgdata_DATA = $(OBJS)

SUFFIXES = .pdf .ps .dvi .bbl .eps .pstex .eps .fig .tex

.fig.eps:
if HAVE_FIG2DEV
	fig2dev -L eps $< $@
endif

clean-local:
	rm -f $(OBJS) $(DST).aux $(DST).bbl $(DST).blg $(DST).dvi $(DST).log $(DST).pdf $(DST).ps $(DST).out *.fig.bak
