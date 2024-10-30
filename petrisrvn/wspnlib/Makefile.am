#
# Makefile for libwspn.a
#
# $HeadURL: http://rads-svn.sce.carleton.ca:8080/svn/lqn/trunk/petrisrvn/wspnlib/Makefile.am $
#
#----------------------------------------------------------------------
# $Id: Makefile.am 11142 2012-10-06 16:22:24Z greg $
#----------------------------------------------------------------------

AM_CPPFLAGS	= -I.
noinst_LIBRARIES = libwspn.a
libwspn_a_SOURCES = change.c\
	alloc.c\
	global.c\
	load.c\
	res.c\
	save.c\
	net.c\
	netdir.c \
	show.c\
	value.c\
	solve.c

noinst_HEADERS=	const.h \
	global.h \
	object.h \
	wspn.h 

check_PROGRAMS = wspn
wspn_SOURCES =  wspn.c read.c
wspn_LDADD= -L. -lwspn
man3_MANS	= wspn.3
EXTRA_DIST	= wspn.3

