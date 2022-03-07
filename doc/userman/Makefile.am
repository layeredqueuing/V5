# Makefile for Userman.
# Copyright (C) 2005 Greg Franks <rgfranks@alumni.uwaterloo.ca>
#
# ------------------------------------------------------------------------
# $Id: Makefile.am 15448 2022-03-05 01:05:41Z greg $
# ------------------------------------------------------------------------

if HAVE_LATEX
SUBDIRS = model bookstore replication srvn-schema timing-diagrams xml-schema

DST = 	userman
INFILE = infile
LATEX = latex -shell-escape -interaction=nonstopmode -file-line-error
FILTER= grep -i ".*:[0-9]*:.*\|warning"

#SRCS=	
#OBJS = $(SRCS:.fig=.eps) 
OBJS = $(SRCS:.fig=.eps) 

TEXS=	defects.tex \
	errors.tex \
	grammar.tex \
	infile.tex \
	input-grammar.tex \
	lqns.tex \
	lqsim.tex \
	lqx.tex \
	model.tex \
	results.tex \
	schema.tex \
	srvn.tex \
	userman.tex
CLSS =	bnf.sty plainurl.bst
BIBS = 	$(DST).bib


pkgdata_DATA = $(DST).pdf

SUFFIXES = .pdf .ps .dvi .ind .bbl .aux .tex .eps .fig 

EXTRA_DIST = $(BIBS) $(SRCS) $(TEXS) $(CLSS) #$(OBJS)

.ps.pdf:
	ps2pdf $< $@

.fig.eps:
if HAVE_FIG2DEV
	fig2dev -L eps -m 0.75 $< $@
endif

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

$(DST).dvi: $(TEXS) $(DST).bib

$(DST).txt: $(DST).dvi
	dvi2tty -w132 $(DST).dvi > $(DST).txt

clean-local:
	rm -f $(DST).blg $(DST).dvi $(DST).log $(DST).pdf $(DST).ps $(DST).out $(DST).toc $(DST).lot $(DST).idx $(DST).ind $(DST).ing $(DST).ilg $(DST).lof *.bak *.aux

distclean-local:
	$(MAKE) clean-local
	rm -f $(OBJS) $(DST).bbl 
else
all:
	echo LaTeX is required to make the user manual.
endif
