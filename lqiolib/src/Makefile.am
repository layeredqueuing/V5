# $Id: Makefile.am 14273 2020-12-27 14:47:06Z greg $
# -------------------------------------------------------------------- [Libtool libsrvnio2]

#AM_CXXFLAGS=-std=c++11

# This project uses Libtool to generate a product
SUBDIRS = headers/lqio
lib_LTLIBRARIES = liblqio.la
liblqio_la_SOURCES = bcmp_document.cpp \
	commandline.cpp \
	common_io.cpp \
	confidence_intervals.cpp \
	dom_activity.cpp \
	dom_actlist.cpp \
	dom_bindings.cpp \
	dom_call.cpp \
	dom_document.cpp \
	dom_entity.cpp \
	dom_entry.cpp \
	dom_extvar.cpp \
	dom_group.cpp \
	dom_histogram.cpp \
	dom_object.cpp \
	dom_phase.cpp \
	dom_pragma.cpp \
	dom_processor.cpp \
	dom_task.cpp \
	error.cpp \
	filename.cpp \
	getsbopt.cc \
	glblerr.cpp \
	jmva_document.cpp \
	labels.cpp \
	srvn_gram.y \
	srvn_input.cpp \
	srvn_output.cpp \
	srvn_result_scan.l \
	srvn_results.cpp \
	srvn_scan.l \
	srvn_spex.cpp \
	srvn_result_gram.y \
	xml_input.cpp \
	xml_output.cpp

noinst_HEADERS = srvn_gram.h  srvn_result_gram.h

#liblqio_la_LIBADD = ../../lqx/libsrc/liblqx.la 
liblqio_la_LIBADD = -L../../lqx/libsrc/.libs -llqx
if EXPAT
liblqio_la_SOURCES += expat_document.cpp
liblqio_la_LIBADD += -lexpat
endif

liblqio_la_CPPFLAGS = -I ../../lqx/libsrc/headers -I headers/lqio
liblqio_la_CXXFLAGS = -Wall -Wno-unknown-pragmas
liblqio_la_LDFLAGS =  -no-undefined -version-info 0:1:0

# Force rename of variables.  For FLEX -- should check. :-)
srvn_scan.c: srvn_scan.l srvn_gram.h headers/lqio/srvn_input.h
	$(LEX) -t $(LFLAGS) -PLQIO_ srvn_scan.l > srvn_scan.c

# Force rename of variables.  For BISON -- should check. :-)
srvn_gram.c srvn_gram.h: srvn_gram.y headers/lqio/srvn_input.h
	$(YACC) -dtv -p LQIO_ -o srvn_gram.c srvn_gram.y

# Force rename of variables.  For FLEX -- should check. :-)
srvn_result_scan.c: srvn_result_scan.l srvn_result_gram.h
	$(LEX) -t $(LFLAGS) -Presult srvn_result_scan.l > srvn_result_scan.c

# Force rename of variables.  For BISON -- should check. :-)
srvn_result_gram.c srvn_result_gram.h: srvn_result_gram.y
	$(YACC) -dtv -p result -o srvn_result_gram.c srvn_result_gram.y

srvn_spex.cpp: srvn_gram.h
srvn_input.cpp: srvn_gram.h

# Files to remove during cleanup
MOSTLYCLEANFILES = srvn_gram.h srvn_gram.c srvn_scan.c srvn_result_gram.h srvn_result_gram.c srvn_result_scan.c

