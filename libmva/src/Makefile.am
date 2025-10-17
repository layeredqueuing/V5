# $Id: Makefile.am 17546 2025-10-15 23:09:02Z greg $
# -------------------------------------------------------------------- [Libtool libsrvnio2]

SUBDIRS = headers/mva
lib_LTLIBRARIES = libmva.la
libmva_la_SOURCES = fpgoop.cc \
	multserv.cc \
	mva.cc \
	open.cc \
	ph2serv.cc \
	pop.cc \
	prob.cc \
	server.cc 

libmva_la_CPPFLAGS = -I headers/mva
libmva_la_CXXFLAGS = -std=gnu++17 -Wall -Wno-unknown-pragmas
libmva_la_LDFLAGS =  -no-undefined -version-info 0:1:0
