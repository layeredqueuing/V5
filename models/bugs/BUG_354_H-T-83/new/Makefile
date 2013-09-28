# Makefile for regression test
# Copyright (C) 2005 Greg Franks <greg@sce.carleton.ca>
#
# ------------------------------------------------------------------------
# $Id: Makefile.am 7509 2007-07-04 14:37:00Z greg $
# ------------------------------------------------------------------------

SRVN=	lqns
OPTS=	-Pvariance=mol -w

SRCS=	repeat.in linear.in

OBJS=	$(SRCS:.in=.p)
OUTS=	$(SRCS:.in=.out)

.SUFFIXES:
.SUFFIXES: .p .in

.in.p:
	$(SRVN) $(OPTS) -p -o $*.out $<

all:	$(OBJS)

check:
	echo $(OBJS) | tr -s " " "\n" | srvndiff -Q -@- . ../../models/regression/lqns-results/

clean:
	-rm -f $(OBJS) $(OUTS)

distclean:	clean

