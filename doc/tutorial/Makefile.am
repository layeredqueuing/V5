# Copyright the Real-Time and Distributed Systems Group,
# Department of Systems and Computer Engineering,
# Carleton University, Ottawa, Ontario, Canada. K1S 5B6
#
#----------------------------------------------------------------------
# $Id: Makefile.am 16326 2023-01-12 23:07:03Z greg $
#----------------------------------------------------------------------

if HAVE_LATEX
LATEX = latex -shell-escape -interaction=nonstopmode -file-line-error
FILTER= grep -i ".*:[0-9]*:.*\|warning"

SUBDIRS = model calls activity-templ model web-server pseudo-tasks
DST = 	tutorial
SRCS =	schema.fig activity-schema.fig
OBJS = $(SRCS:.fig=.eps) 
OTHER =	plainurl.bst

pkgdata_DATA = $(DST).pdf
EXTRA_DIST = $(DST).tex $(DST).bib $(SRCS) $(OTHER)

SUFFIXES = .pdf .ps .dvi .bbl .eps .pstex .eps .fig .tex

.ps.pdf:
	ps2pdf $< $@

.fig.eps:
if HAVE_FIG2DEV
	fig2dev -L eps -m 0.7 $< $@
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

$(DST).ps: $(DST).dvi 

$(DST).dvi: $(DST).tex $(DST).bib $(OBJS)

$(DST).txt: $(DST).dvi
	dvi2tty -w132 $(DST).dvi > $(DST).txt

clean-local:
	rm -f $(DST).aux $(DST).bbl $(DST).blg $(DST).dvi $(DST).log $(DST).pdf $(DST).ps $(DST).out *.fig.bak 
else
all:
	echo LaTeX is required to make the tutorial.
endif
