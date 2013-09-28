# Makefile for interlocking
# Copyright (C) 2012 Greg Franks <rgfranks@alumni.uwaterloo.ca>
#
# $HeadURL: http://franks.dnsalias.com/svn/papers/trunk/bug-661-interlocking/model4/Makefile $
#
# $Id: Makefile 3482 2013-03-01 18:43:37Z greg $
# ------------------------------------------------------------------------

SUBDIRS = 
DST = 	

MODEL=  bug-685

OBJS = $(MODEL)-lqns.lqxo $(MODEL)-none.lqxo $(MODEL)-new.lqxo $(MODEL)-sim.lqxo

all:	$(OBJS)

$(MODEL)-lqns.lqxo:	$(MODEL).lqn
	lqns -Pmva=schweitzer -wa -xo $@ $<

$(MODEL)-none.lqxo:	$(MODEL).lqn
	lqns -Pmva=schweitzer -Pinterlocking=none -wa -xo $@ $<

$(MODEL)-new.lqxo:	$(MODEL).lqn
	$$HOME/usr/src-judy/lqns/lqns -Pmva=schweitzer -wa -xo $@ $<

$(MODEL)-sim.lqxo:	$(MODEL).lqn
	lqsim -C1.0 -S123456 -xo $@ $<

