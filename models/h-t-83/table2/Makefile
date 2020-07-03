# Makefile for
# Copyright (C) 2003 Greg Franks <greg@sce.carleton.ca>
# $HeadURL: http://rads-svn.sce.carleton.ca:8080/svn/lqn/trunk-V5/models/h-t-83/table2/Makefile $
# ------------------------------------------------------------------------
# $Id: Makefile 7730 2008-02-28 03:55:37Z greg $
# ------------------------------------------------------------------------

CPU1=case-01-01-01-01.p case-01-01-01-02.p case-01-01-02-01.p case-01-01-02-02.p case-01-02-01-01.p case-01-02-01-02.p case-01-02-02-01.p case-01-02-02-02.p case-01-03-01-01.p case-01-03-01-02.p case-01-03-02-01.p case-01-03-02-02.p
CPU2=case-02-01-01-01.p case-02-01-01-02.p case-02-01-02-01.p case-02-01-02-02.p case-02-02-01-01.p case-02-02-01-02.p case-02-02-02-01.p case-02-02-02-02.p case-02-03-01-01.p case-02-03-01-02.p case-02-03-02-01.p case-02-03-02-02.p
SETS=set1 set2 set3 set4 set5

all:	1-cpu.tput 3-cpu.tput
	@echo "1 CPU  LQNS"
	@awk '{print $$4}' < 1-cpu.tput | sort -nb | awk -f stats.awk
	@echo "1 CPU  NOML"
	@awk '{print $$6}' < 1-cpu.tput | sort -nb | awk -f stats.awk
	@echo "1 CPU  NEW"
	@awk '{print $$8}' < 1-cpu.tput | sort -nb | awk -f stats.awk
	@echo "3 CPU  LQNS"
	@awk '{print $$4}' < 3-cpu.tput | sort -nb | awk -f stats.awk
	@echo "3 CPU  NOML"
	@awk '{print $$6}' < 3-cpu.tput | sort -nb | awk -f stats.awk
	@echo "3 CPU  NEW"
	@awk '{print $$8}' < 3-cpu.tput | sort -nb | awk -f stats.awk

sim:
	for i in $(SETS); do \
	  (cd $$i; if test ! -r sim/case-01-01-01-01.in; then MultiSRVN -n -e sim $$i.exp; fi); \
	  (cd $$i/sim; $(MAKE)); \
	  (cd $$i; MultiSRVN -r -e sim $$i.exp); \
	done

noml:
	for i in $(SETS); do \
	  (cd $$i; MultiSRVN -e noml $$i.exp); \
	done

new:
	for i in $(SETS); do \
	  (cd $$i; MultiSRVN -e new $$i.exp); \
	done

lqns:
	for i in $(SETS); do \
	  (cd $$i; MultiSRVN -e lqns $$i.exp); \
	done

1-cpu.tput: 
	for i in $(SETS); do \
	  for j in $(CPU1); do \
	    srvndiff -AC +t $$i/sim/$$j $$i/lqns/$$j $$i/noml/$$j $$i/new/$$j | grep 't1 ' >> $@; \
	  done; \
	done;

3-cpu.tput:
	for i in $(SETS); do \
	  for j in $(CPU2); do \
	    srvndiff -AC +t $$i/sim/$$j $$i/lqns/$$j $$i/noml/$$j $$i/new/$$j | grep 't1 ' >> $@; \
	  done; \
	done;

clean:
	-rm 1-cpu.tput 3-cpu.tput


high-error:
	@for i in $(SETS); do \
	    srvndiff -AICT +t -S10.0 $$i/sim $$i/lqns $$i/new; \
	done;
