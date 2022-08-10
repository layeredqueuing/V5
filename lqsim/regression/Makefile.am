# Makefile for regression test
# Copyright (C) 2005 Greg Franks <greg@sce.carleton.ca>
#
# ------------------------------------------------------------------------
# $Id: Makefile.am 15801 2022-08-10 13:26:50Z greg $
# ------------------------------------------------------------------------

SRVN=	../lqsim
OPTS=	-C1.0,1000 -S1049217653
#OPTS=	-B3,10000,1000 -M -S1049217653
EPSILON = '-S1.0'

SANITY=	01-sanity.lqxo \
	02-sanity.lqxo \
	03-sanity.lqxo \
	04-sanity.lqxo \
	05-sanity.lqxo \
	06-sanity.lqxo \
	07-sanity.lqxo \
	09-sanity.lqxo \
	31-overtaking.lqxo
INTER=	10-interlock.lqxo \
	11-interlock.lqxo \
	12-interlock.lqxo \
	13-interlock.lqxo 
MULTI=	20-multiserver.lqxo \
	21-multiserver.lqxo \
	22-multiserver.lqxo \
	23-multiserver.lqxo 
ACT=	40-activities.lqxo \
	41-activities.lqxo \
	42-activities.lqxo \
	43-activities.lqxo \
	44-activities.lqxo \
	45-activities.lqxo \
	46-activities.lqxo 
OVER=	31-overtaking.lqxo \
	32-async-inf.lqxo 
REPL=	50-replication-flat.lqxo \
	51-replication-flat.lqxo \
	52-replication-flat.lqxo \
	55-replication-flat.lqxo \
	56-replication-flat.lqxo 
SEMA =  60-semaphore.lqxo \
	61-semaphore.lqxo \
	62-semaphore.lqxo \
	65-buffer.lqxo \
	66-rwlock.lqxo \
	67-rwlock.lqxo \
	68-rwlock.lqxo \
	69-rwlock.lqxo
#	63-semaphore.lqxo 
SIM=	70-priority.lqxo \
	71-fair.lqxo \
	72-fair.lqxo \
	75-distributions.lqxo \
	76-distributions.lqxo 
SYNC=	80-sync.lqxo \
	81-sync.lqxo \
	82-sync.lqxo \
	83-sync.lqxo 
FORK=	85-fork.lqxo \
	86-fork.lqxo \
	87-fork.lqxo \
	88-quorum.lqxo \
	89-quorum.lqxo 
OTHER=	90-A01.lqxo \
	90-B07.lqxo \
	91-cs3-1.lqxo \
	92-open.lqxo \
	93-simple-ucm.lqxo

OBJS=	$(SANITY) $(INTER) $(MULTI) $(OVER) $(ACT) $(REPL) $(SEMA) $(SIM) $(SYNC) $(FORK) $(OTHER)
OBJS2=  01L-sanity.d 20L-multiserver.d 71L-fair.d

SRCS=	$(OBJS:.lqxo=.lqnx)
SRCS2=	$(OBJS2:.d=.lqnx)

EXTRA_DIST = $(SRCS) $(SRCS2)

SUFFIXES = .diff .d .lqxo .lqnx 

.lqnx.lqxo:
	$(SRVN) $(OPTS) $<
	@srvndiff -Q $(EPSILON) ../../models/regression/sim-results/$@ $@ 

.lqnx.d:
	$(SRVN) $(OPTS) $<

.lqxo.diff:
	-srvndiff ../../models/regression/sim-results/$< $< > $@

check:	$(OBJS)
	echo $(OBJS) | tr -s " " "\n" | srvndiff -Q $(EPSILON) -@- . ../../models/regression/sim-results

check-syntax:
	$(MAKE) OPTS=-n check

quick-check:
	$(MAKE) OPTS=-S1049217653 check

check-verbose:
	FILES=`echo $(OBJS) | tr -s " " "\n" | srvndiff -Q $(EPSILON) -@- . ../../models/regression/sim-results`; \
	for i in $$FILES; do srvndiff ../../models/regression-xml/sim-results/$$i $$i; done

all:	

clean:
	@-rm -f $(OBJS) *.out 
	@-rm -rf $(OBJS2)

distclean:	clean 
	-rm -f Makefile

sanity:	$(SANITY)
	echo $(SANITY) | tr -s " " "\n" | srvndiff -S0.01 -Q -@- . ../../models/regression/sim-results

multi:	$(MULTI)
	echo $(MULTI) | tr -s " " "\n" | srvndiff -Q -@- . ../../models/regression/sim-results

act:	$(ACT)
	echo $(ACT) | tr -s " " "\n" | srvndiff -Q -@- -S0.01 . ../../models/regression/sim-results

fork:	$(FORK)
	echo $(FORK) | tr -s " " "\n" | srvndiff -Q -@- . ../../models/regression/sim-results

repl:	$(REPL)
	echo $(REPL) | tr -s " " "\n" | srvndiff -Q -@- -S0.02 . ../../models/regression/repl-results

sim:	$(SIM)
	echo $(SIM) | tr -s " " "\n" | srvndiff -Q -@- -S0.02 . ../../models/regression/sim-results

sema:	$(SEMA)
	echo $(SEMA) | tr -s " " "\n" | srvndiff -Q -@- -S0.02 . ../../models/regression/sim-results

sync:	$(SYNC)
	echo $(SYNC) | tr -s " " "\n" | srvndiff -Q -@- -S0.02 . ../../models/regression/sim-results

other:	$(OTHER)
	echo $(OTHER) | tr -s " " "\n" | srvndiff -Q -@- -S0.02 . ../../models/regression/sim-results

quick:
	$(MAKE) OPTS='-S123456 -T10000' EPSILON='-A +t -S10.0' check

32-async-inf.lqxo: 32-async-inf.lqnx
	$(SRVN) -C1.0,250000,1000000 -Pstop-on-message-loss=f -w -S1049217653 32-async-inf.lqnx

50-replication.lqxo:	50-replication.lqnx
	rep2flat -Oxml -o$*-flat.lqnx $<
	$(SRVN) $(OPTS) $*-flat.lqnx
	lqn2ps -Oxml -o$*.lqxo --merge-replicas $*-flat.lqxo

51-replication.lqxo:	51-replication.lqnx
	rep2flat -Oxml -o$*-flat.lqnx $<
	$(SRVN) $(OPTS) $*-flat.lqnx
	lqn2ps -Oxml -o$*.lqxo --merge-replicas $*-flat.lqxo

52-replication.lqxo:	52-replication.lqnx
	rep2flat -Oxml -o$*-flat.lqnx $<
	$(SRVN) $(OPTS) $*-flat.lqnx
	lqn2ps -Oxml -o$*.lqxo --merge-replicas $*-flat.lqxo

55-replication.lqxo:	55-replication.lqnx
	rep2flat -Oxml -o$*-flat.lqnx $<
	$(SRVN) $(OPTS) $*-flat.lqnx
	lqn2ps -Oxml -o$*.lqxo --merge-replicas $*-flat.lqxo

56-replication.lqxo:	56-replication.lqnx
	rep2flat -Oxml -o$*-flat.lqnx $<
	$(SRVN) $(OPTS) $*-flat.lqnx
	lqn2ps -Oxml -o$*.lqxo --merge-replicas $*-flat.lqxo

