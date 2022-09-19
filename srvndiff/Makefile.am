#----------------------------------------------------------------------
# $Id: Makefile.am 15865 2022-09-15 19:28:53Z greg $
#----------------------------------------------------------------------

AM_CPPFLAGS	= -I..
srvndiff_CXXFLAGS = -std=c++11 -Wall -Wno-format -Wno-format-security
bin_PROGRAMS 	= srvndiff
srvndiff_SOURCES = srvndiff.cc symtbl.cc getopt2.cc srvn_result_scan.l srvn_result_gram.y commandline.cc confidence_intervals.cc error.cc srvn_results.cc getsbopt.cc parseable.cc json_document.cc
noinst_HEADERS  = confidence_intervals.h commandline.h error.h expat_document.h getopt2.h getsbopt.h srvndiff.h srvn_result_gram.h srvn_results.h symtbl.h parseable.h json_document.h

srvndiff_LDFLAGS= @STATIC@

if EXPAT
srvndiff_SOURCES += expat_document.cc
endif

EXTRA_DIST	= srvndiff.1

man1_MANS	= srvndiff.1

if HAVE_PS2PDF
all-local:	srvndiff.pdf

install-data-local:	srvndiff.pdf

srvndiff.pdf:	$(man1_MANS)
		groff -man $(man1_MANS) | ps2pdf - $@
endif

srvndiff.txt:	$(man1_MANS)
		nroff -man $(man1_MANS) | col -b > srvndiff.txt

# Force rename of variables.  For FLEX -- should check. :-)
srvn_result_scan.c: srvn_result_scan.l srvn_result_gram.h
	$(LEX) -t $(LFLAGS) -Presult srvn_result_scan.l > srvn_result_scan.c

# Force rename of variables.  For BISON -- should check. :-)
srvn_result_gram.c srvn_result_gram.h: srvn_result_gram.y
	$(YACC) -dt -p result -o srvn_result_gram.c srvn_result_gram.y

# Files to remove during cleanup
MOSTLYCLEANFILES = srvn_result_gram.h srvn_result_gram.c srvn_result_scan.c
