#
# $Id: Makefile.am 13188 2018-03-02 17:44:42Z greg $
#----------------------------------------------------------------------

AUTOMAKE_OPTIONS = foreign
ACLOCAL_AMFLAGS=-I ../config
AM_CPPFLAGS	= -I..
srvndiff_CXXFLAGS = -Wall -Wno-format -Wno-format-security
bin_PROGRAMS 	= srvndiff
srvndiff_SOURCES = srvndiff.cc symtbl.cc getopt2.cc expat_document.cc srvn_result_scan.l srvn_result_gram.y commandline.cc confidence_intervals.cc error.cc srvn_results.cc getsbopt.cc parseable.cc 
noinst_HEADERS  = confidence_intervals.h commandline.h config.h error.h expat_document.h getopt2.h getsbopt.h srvndiff.h srvn_result_gram.h srvn_results.h symtbl.h parseable.h 

EXTRA_DIST	= srvndiff.1

srvndiff_LDADD	= 
srvndiff_LDFLAGS= @STATIC@

man1_MANS	= srvndiff.1

install-data-local:	srvndiff.pdf

srvndiff.ps:	$(man1_MANS)
		cat $(man1_MANS) | groff -man  > srvndiff.ps

srvndiff.pdf:	srvndiff.ps
		ps2pdf srvndiff.ps

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
