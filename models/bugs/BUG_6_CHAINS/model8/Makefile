# Makefile for interlocking
# Copyright (C) 2012 Greg Franks <rgfranks@alumni.uwaterloo.ca>
#
# $HeadURL: http://franks.dnsalias.com/svn/papers/trunk/valuetools-2013-interlocking/model2/Fixed/Makefile $
#
# $Id: Makefile 3816 2013-11-26 19:03:03Z greg $
# ------------------------------------------------------------------------
include .common.mk

MVA =

SUBDIRS = 
DST = 	
MODEL = model8
SRCS1 = 
SRCS2 = 

OBJS1 = $(SRCS1:.fig=.eps)
OBJS2 = $(SRCS2:.gnuplot=.eps)
BAKS =  $(SRCS1:.fig=.fig.bak)

.PRECIOUS:	$(SRCS1) $(SRCS2)

all-am:	$(OBJS1) $(OBJS2)  

install-am:

clean-am:
	rm -rf $(BAKS) empty nets 

distclean-am:
	$(MAKE) clean-am
	rm -rf $(OBJS1) $(OBJS2) results results.dat

# ------------------------------------------------------------------------

# results:
# 	@cd ..; $(MAKE) diff
# 	../diff $(MODEL).sim $(MODEL).none $(MODEL).lqns $(MODEL).new > results

# results.dat:
# 	@cd ..; $(MAKE) diff
# 	../diff --no-headers --no-summary --infinity=99999 $(MODEL).sim $(MODEL).none $(MODEL).lqns $(MODEL).new > results.dat

new-results:
	@$(MAKE) distclean
	-@rm -f $(MODEL).new
	$(MAKE) $(MODEL).new
	$(MAKE)

none-error.eps:	results.dat none-error.gnuplot 

lqns-error.eps:	results.dat lqns-error.gnuplot

new-error.eps:	results.dat new-error.gnuplot

diff:	diff.c
	$(CC) -std=c99 -Wall -O2 -o $@ -lm $<

