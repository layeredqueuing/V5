# -*- Makefile -*-
# $HeadURL: svn://192.168.2.10/lqn/trunk-V5/parasol/src/Makefile.am $
# Makefile for Parasol libraries.
#
# ------------------------------------------------------------------------
# $Id: Makefile.am 11142 2012-10-06 16:22:24Z greg $
# ------------------------------------------------------------------------

AM_CPPFLAGS	= -I../include
noinst_LIBRARIES =  libparasol.a libparasolc.a libparasol++.a

libparasol_a_SOURCES = para_library.c standalone.c 
EXTRA_libparasol_a_SOURCES = drand48.c
libparasol_a_DEPENDENCIES = @DRAND48@
libparasol_a_LIBADD = @DRAND48@
libparasolc_a_SOURCES = para_library.c 
EXTRA_libparasolc_a_SOURCES = drand48.c
libparasolc_a_LIBADD = @DRAND48@
libparasolc_a_DEPENDENCIES = @DRAND48@

libparasol___a_SOURCES = carrier.cc node.cc semaphore.cc statistic.cc spinlock.cc para_entity.cc \
	task.cc port.cc genesis.cc

