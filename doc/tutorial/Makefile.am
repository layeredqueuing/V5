# Copyright the Real-Time and Distributed Systems Group,
# Department of Systems and Computer Engineering,
# Carleton University, Ottawa, Ontario, Canada. K1S 5B6
#
#----------------------------------------------------------------------
# $Id: Makefile.am 11483 2013-08-16 12:47:24Z greg $
#----------------------------------------------------------------------

SUBDIRS = model calls activity-templ web-server pseudo-tasks
DST = 	tutorial
SRCS =	schema.fig activity-schema.fig
LATEX = latex -shell-escape -interaction=nonstopmode -file-line-error
FILTER= grep -i ".*:[0-9]*:.*\|warning"

pkgdata_DATA = $(DST).pdf
EXTRA_DIST = $(DST).tex $(DST).bib $(SRCS)

SUFFIXES = .pdf .ps .dvi .bbl .eps .pstex .eps .fig .tex

OBJS = $(SRCS:.fig=.eps) 


.ps.pdf:
	ps2pdf $< $@

.fig.eps:
	fig2dev -L eps -m 0.7 $< $@

.dvi.ps:
	dvips -q -tletter -Ppdf -G0 -o $@ $< 

.tex.dvi: 
	-$(LATEX) $* | $(FILTER) 
	-makeindex -q $*
	-grep 'Citation .* undefined' $*.log > /dev/null; \
	if [ $$? -eq 0 ]; then bibtex -terse $*; $(LATEX) $* | $(FILTER); fi
	-grep 'Rerun to get' $*.log > /dev/null; \
	if [ $$? -eq 0 ]; then $(LATEX) $< | $(FILTER); fi

$(DST).pdf: $(DST).ps

$(DST).ps: $(DST).dvi 

$(DST).dvi: $(DST).tex $(OBJS) $(DST).bib

$(DST).txt: $(DST).dvi
	dvi2tty -w132 $(DST).dvi > $(DST).txt

clean-local:
	rm -f $(OBJS)  $(DST).aux $(DST).bbl $(DST).blg $(DST).dvi $(DST).log $(DST).pdf $(DST).ps $(DST).out *.fig.bak *.eps
