# Makefile for Userman.
# Copyright (C) 2005 Greg Franks <rgfranks@alumni.uwaterloo.ca>
#
# ------------------------------------------------------------------------
# $Id: Makefile.am 11431 2013-06-17 14:18:13Z greg $
# ------------------------------------------------------------------------

SUBDIRS = 
DST = 	parasol
LATEX = latex -shell-escape -interaction=nonstopmode -file-line-error
FILTER= grep -i ".*:[0-9]*:.*\|warning"

SRCS=	

TEXS=	$(DST).tex
CLSS =	
BIBS = 	$(DST).bib


noinst_DATA = $(DST).pdf
EXTRA_DIST = $(BIBS) $(SRCS) $(TEXS) $(CLSS)

SUFFIXES = .pdf .ps .dvi .ind .bbl .aux .tex .eps .fig 

OBJS = $(SRCS:.fig=.eps) 

.ps.pdf:
	ps2pdf $< $@

.fig.eps:
	fig2dev -L eps -m 0.75 $< $@

.dvi.ps:
	dvips -q -tletter -Ppdf -G0 -o $@ $< 

.tex.dvi:
	-$(LATEX) $* | $(FILTER) 
	-makeindex $*
	-grep 'Citation .* undefined' $*.log > /dev/null; \
	if [ $$? -eq 0 ]; then bibtex -terse $*; $(LATEX) $* | $(FILTER); fi
	-grep 'Rerun to get' $*.log > /dev/null; \
	if [ $$? -eq 0 ]; then $(LATEX) $< | $(FILTER); fi

$(DST).pdf: $(DST).ps

$(DST).dvi: $(DST).aux $(DST).bbl 

$(DST).bbl: $(TEXS)

$(DST).aux: $(OBJS) $(TEXS)

$(DST).txt: $(DST).dvi
	dvi2tty -w132 $(DST).dvi > $(DST).txt

clean-local:
	rm -f $(OBJS)  $(DST).aux $(DST).blg $(DST).dvi $(DST).idx $(DST).ind $(DST).ing $(DST).lof $(DST).log $(DST).lot $(DST).pdf $(DST).ps $(DST).toc $(DST).out *.fig.bak *.eps

