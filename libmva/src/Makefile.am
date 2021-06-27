# $Id: Makefile.am 14863 2021-06-26 01:36:42Z greg $
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
libmva_la_CXXFLAGS = -std=c++11 -Wall -Wno-unknown-pragmas
libmva_la_LDFLAGS =  -no-undefined -version-info 0:1:0
