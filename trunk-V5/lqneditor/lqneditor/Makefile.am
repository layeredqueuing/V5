# Makefile for lqneditor.
# Copyright (C) 2012 Greg Franks <rgfranks@alumni.uwaterloo.ca>
# $Id: Makefile.am 11142 2012-10-06 16:22:24Z greg $
# ----------------------------------------------------------------------
AM_CPPFLAGS	= -I../../lqiolib/src/headers  -I../../lqx/libsrc/headers -Wall -Wno-unknown-pragmas
bin_PROGRAMS	= lqneditor
lqneditor_SOURCES=lqneditor.cpp frame.cpp canvas.cpp dialog.cpp model.cpp node.cpp processor.cpp task.cpp entry.cpp phase.cpp arc.cpp call.cpp label.cpp
noinst_HEADERS	= lqneditor.h frame.h canvas.h dialog.h model.h node.h processor.h task.h entry.h phase.h arc.h call.h label.h
lqneditor_LDADD	= -llqio 

