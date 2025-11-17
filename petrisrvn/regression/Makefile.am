# Makefile for regression test
# Copyright (C) 2005 Greg Franks <greg@sce.carleton.ca>
#
# ------------------------------------------------------------------------
# $Id: Makefile.am 17586 2025-11-12 19:51:48Z greg $
# ------------------------------------------------------------------------


SRVN=	../petrisrvn
OPTS=	-w
DIFFOPTS= -Q -S0.1

SANITY=	00-sanity.lqxo \
	01-sanity.lqxo \
	02-sanity.lqxo \
	03-sanity.lqxo \
	04-sanity.lqxo \
	05-sanity.lqxo \
	06-sanity.lqxo \
	07-sanity.lqxo \
	08-sanity.lqxo \
	09-sanity.lqxo
INTER=	10-interlock.lqxo \
	11-interlock.lqxo \
	12-interlock.lqxo \
	13-interlock.lqxo \
	14-interlock.lqxo \
	15-split-interlock.lqxo \
	16-split-interlock.lqxo \
	18-interlock.lqxo \
	19-interlock.lqxo
MULTI=	20-multiserver.lqxo \
	21-multiserver.lqxo \
	22-multiserver.lqxo \
	23-multiserver.lqxo 
OVER=	31-overtaking.lqxo \
	33-spex-convergence.lqxo \
	34-priority.lqxo 
ACT=	40-activities.lqxo \
	41-activities.lqxo \
	42-activities.lqxo \
	43-activities.lqxo \
	44-activities.lqxo \
	45-activities.lqxo \
	46-activities.lqxo 
REPL=	50-replication.lqxo \
	52-replication.lqxo \
	55-replication.lqxo \
	56-replication.lqxo 
SEMA =  60-semaphore.lqxo \
	61-semaphore.lqxo \
	62-semaphore.lqxo \
	63-semaphore.lqxo \
	65-buffer.lqxo 
SYNC=	80-sync.lqxo \
	81-sync.lqxo \
	82-sync.lqxo \
	83-sync.lqxo \
	84-sync.lqxo
FORK=	85-fork.lqxo \
	86-fork.lqxo \
	87-fork.lqxo 
QUORUM=	88-quorum.lqxo \
	89-quorum.lqxo 
OTHER=	90-A01.lqxo \
	90-B07.lqxo \
	91-cs3-1.lqxo \
	92-open.lqxo \
	93-simple-ucm.lqxo

OBJS=	$(SANITY) $(INTER) $(MULTI) $(OVER) $(ACT) $(REPL) $(SEMA) $(SYNC) $(FORK) $(OTHER)

SRCS=	$(OBJS:.lqxo=.lqnx)

EXTRA_DIST = $(SRCS)

SUFFIXES = .diff .d .lqxo .lqnx 

.lqnx.lqxo:
	$(SRVN) $(OPTS) $<
	@srvndiff $(DIFFOPTS) $@ ../../models/regression/petri-results/$@

.lqnx.d:
	$(SRVN) $(OPTS) $<

.lqxo.diff:
	-srvndiff -I ../../models/regression/{petri,sim}-results/$< $< > $@

check:	$(OBJS)
	echo $(OBJS) | tr -s " " "\n" | srvndiff -QI $(DIFFOPTS) -@- . ../../models/regression/petri-results/

check-verbose:
	FILES=`echo $(OBJS) | tr -s " " "\n" | srvndiff -QI $(DIFFOPTS) -@- . ../../models/regression/petri-results`; \
	for i in $$FILES; do srvndiff ../../models/regression-xml/petri-results/$$i $$i; done

all:

clean-am:
	@-rm -rf $(OBJS) *.out nets empty

sanity:	$(SANITY)
	echo $(SANITY) | tr -s " " "\n" | srvndiff $(DIFFOPTS) -Q -@- . ../../models/regression/petri-results

inter:	$(INTER)
	echo $(INTER) | tr -s " " "\n" | srvndiff $(DIFFOPTS) -@- . ../../models/regression/petri-results

multi:	$(MULTI)
	echo $(MULTI) | tr -s " " "\n" | srvndiff $(DIFFOPTS) -@- . ../../models/regression/petri-results

act:	$(ACT)
	echo $(ACT) | tr -s " " "\n" | srvndiff $(DIFFOPTS) -@- . ../../models/regression/petri-results

fork:	$(FORK)
	echo $(FORK) | tr -s " " "\n" | srvndiff -Q -@- . ../../models/regression/petri-results

sema:	$(SEMA)
	echo $(SEMA) | tr -s " " "\n" | srvndiff $(DIFFOPTS) -Q  . ../../models/regression/petri-results

sync:	$(SYNC)
	echo $(SYNC) | tr -s " " "\n" | srvndiff $(DIFFOPTS) -Q  . ../../models/regression/petri-results

other:	$(OTHER)
	echo $(OTHER) | tr -s " " "\n" | srvndiff $(DIFFOPTS) -Q  . ../../models/regression/petri-results

09-sanity.lqxo:	09-sanity.lqnx
	$(SRVN) $(OPTS) -Q5 $<
	@srvndiff -q $(DIFFOPTS) $@ ../../models/regression/petri-results/$@

33-spex-convergence.lqxo: 33-spex-convergence.lqnx
	$(SRVN) -w -o $*.out $<
	@mv $*.d/$*-007.lqxo $@
	@srvndiff $(DIFFOPTS) $@ ../../models/regression/petri-results/$@
	@rm -rf $*.d

50-replication.lqxo:	50-replication.lqnx
	@rep2flat -Oxml -o$*-flat.lqnx $<
	$(SRVN) $(OPTS) $*-flat.lqnx
	@lqn2ps -Oxml -o$*.lqxo --merge-replicas $*-flat.lqxo
	@rm $*-flat.*
	@srvndiff $(DIFFOPTS) $@ ../../models/regression/petri-results/$@

52-replication.lqxo:	52-replication.lqnx
	@rep2flat -Oxml -o$*-flat.lqnx $<
	$(SRVN) $(OPTS) $*-flat.lqnx
	@lqn2ps -Oxml -o$*.lqxo --merge-replicas $*-flat.lqxo
	@rm $*-flat.*
	@srvndiff $(DIFFOPTS) $@ ../../models/regression/petri-results/$@

53-replication.lqxo:	53-replication.lqnx
	@lqn2ps --run-lqx -Oxml -o$*.temp $<	# instantiate variables
	@rep2flat -Oxml -o$*-flat.lqnx $*.temp
	$(SRVN) $(OPTS) $*-flat.lqnx
	@lqn2ps -Oxml -o$*.lqxo --merge-replicas $*-flat.lqxo
	@rm $*-flat.* $*.temp
	@srvndiff $(DIFFOPTS) $@ ../../models/regression/petri-results/$@

55-replication.lqxo:	55-replication.lqnx
	@rep2flat -Oxml -o$*-flat.lqnx $<
	$(SRVN) $(OPTS) $*-flat.lqnx
	@lqn2ps -Oxml -o$*.lqxo --merge-replicas $*-flat.lqxo
	@rm $*-flat.*
	@srvndiff $(DIFFOPTS) $@ ../../models/regression/petri-results/$@

56-replication.lqxo:	56-replication.lqnx
	@rep2flat -Oxml -o$*-flat.lqnx $<
	$(SRVN) $(OPTS) $*-flat.lqnx
	@lqn2ps -Oxml -o$*.lqxo --merge-replicas $*-flat.lqxo
	@rm $*-flat.*
	@srvndiff $(DIFFOPTS) $@ ../../models/regression/petri-results/$@

90-B07.lqxo: 90-B07.lqnx
	$(SRVN) $(OPTS) $<
	@srvndiff -q $(DIFFOPTS) $@ ../../models/regression/petri-results/$@
