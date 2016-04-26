# Copyright the Real-Time and Distributed Systems Group,
# Department of Systems and Computer Engineering,
# Carleton University, Ottawa, Ontario, Canada. K1S 5B6
#
#----------------------------------------------------------------------
# $Id: Makefile.am 11470 2013-08-12 19:51:25Z greg $
#----------------------------------------------------------------------

SUBDIRS = 
SRCS =  activity-model.fig	flat-model.fig		phase-entry.fig \
	basic-lqn.fig		layered-model.fig	phase-seq.fig \
	buffer-seq.fig		multi-servers.fig	service-time.fig \
	buffer.fig		notation.fig		task-queue.fig \
	elements.fig		phase-activity.fig      service-time-seq.fig 

EXTRA_DIST = $(SRCS)

SUFFIXES = .pdf .ps .dvi .bbl .eps .pstex .eps .fig .tex

noinst_DATA = $(SRCS:.fig=.eps) 

.fig.eps:
	fig2dev -L eps $< $@

clean-local:
	rm -f $(OBJS)  $(DST).aux $(DST).bbl $(DST).blg $(DST).dvi $(DST).log $(DST).pdf $(DST).ps $(DST).out *.fig.bak *.eps
