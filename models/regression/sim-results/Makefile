# Makefile for 
# Copyright (C) 2003 Greg Franks <greg@sce.carleton.ca>
# ------------------------------------------------------------------------
# $Id: Makefile 17565 2025-11-06 16:53:07Z greg $
# ------------------------------------------------------------------------

SRVN=	lqsim
OPTS=	-C1.0,1000 -x
SEED=	-S1049217653 
#OPTS=	-B3,10000,1000 -M -S1049217653

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
DST =	$(SANITY) \
	10-interlock.lqxo \
	11-interlock.lqxo \
	12-interlock.lqxo \
	13-interlock.lqxo \
	14-interlock.lqxo \
	15-split-interlock.lqxo \
	16-split-interlock.lqxo \
	18-interlock.lqxo \
	20-multiserver.lqxo \
	21-multiserver.lqxo \
	22-multiserver.lqxo \
	23-multiserver.lqxo \
	24-multiserver.lqxo \
	31-overtaking.lqxo \
	32-async-inf.lqxo \
	34-priority.lqxo \
	40-activities.lqxo \
	41-activities.lqxo \
	42-activities.lqxo \
	43-activities.lqxo \
	44-activities.lqxo \
	45-activities.lqxo \
	46-activities.lqxo \
	47-activities.lqxo \
	48-activities.lqxo \
	50-replication.lqxo \
	51-replication.lqxo \
	52-replication.lqxo \
	53-replication.lqxo \
	55-replication.lqxo \
	56-replication.lqxo \
	60-semaphore.lqxo \
	61-semaphore.lqxo \
	62-semaphore.lqxo \
	63-semaphore.lqxo \
	65-buffer.lqxo \
	66-rwlock.lqxo \
	66W-rwlock.lqxo \
	67-rwlock.lqxo \
	68-rwlock.lqxo \
	69-rwlock.lqxo \
	72-fair.lqxo \
	75-distributions.lqxo \
	76-distributions.lqxo \
	80-sync.lqxo \
	81-sync.lqxo \
	82-sync.lqxo \
	83-sync.lqxo \
	84-sync.lqxo \
	85-fork.lqxo \
	86-fork.lqxo \
	87-fork.lqxo \
	88-quorum.lqxo \
	89-quorum.lqxo \
	90-A01.lqxo \
	90-B07.lqxo \
	91-cs3-1.lqxo \
	92-open.lqxo \
	93-simple-ucm.lqxo \
	95-async-inf.lqxo \
	94-5101-a2-q2b1.lqxo 

SRC = $(DST:.lqxo=.lqnx)

.SUFFIXES:
.SUFFIXES: .diff .lqxo .lqnx
.PHONY:	.files

.lqnx.lqxo:
	$(SRVN) $(OPTS) $(SEED) -o $*.lqxo $<

all:	$(DST) .files

sanity: $(SANITY)

diff:
	srvndiff -@.files ../petri-results ../lqns-results .

.files:	
	@-rm .files
	for i in $(DST); do echo $$i >> .files; done

# ----

50-replication.lqxo:	50-replication.lqnx
	rep2flat -Oxml -o$*-flat.lqnx $<
	lqn2ps -Oxml -o$*.lqxo --merge-replicas $*-flat.lqxo
	rm $*-flat.lqnx $*-flat.lqxo

51-replication.lqxo:	51-replication.lqnx
	rep2flat -Oxml -o$*-flat.lqnx $<
	lqn2ps -Oxml -o$*.lqxo --merge-replicas $*-flat.lqxo
	rm $*-flat.lqnx $*-flat.lqxo

52-replication.lqxo:	52-replication.lqnx
	rep2flat -Oxml -o$*-flat.lqnx $<
	lqn2ps -Oxml -o$*.lqxo --merge-replicas $*-flat.lqxo
	rm $*-flat.lqnx $*-flat.lqxo

53-replication.lqxo:	53-replication.lqnx
	rep2flat -Oxml -o$*-flat.lqnx $<
	lqn2ps -Oxml -o$*.lqxo --merge-replicas $*-flat.lqxo
	rm $*-flat.lqnx $*-flat.lqxo

55-replication.lqxo:	55-replication.lqnx
	rep2flat -Oxml -o$*-flat.lqnx $<
	lqn2ps -Oxml -o$*.lqxo --merge-replicas $*-flat.lqxo
	rm $*-flat.lqnx $*-flat.lqxo

56-replication.lqxo:	56-replication.lqnx
	rep2flat -Oxml -o$*-flat.lqnx $<
	lqn2ps -Oxml -o$*.lqxo --merge-replicas $*-flat.lqxo
	rm $*-flat.lqnx $*-flat.lqxo

94-5101-a2-q2b1.lqxo: 	94-5101-a2-q2b1.lqnx
	$(SRVN) -C1.0,10000 $(SEED) -xo $*.lqxo $<

repl:	50-replication.lqxo 51-replication.lqxo 52-replication.lqxo 55-replication.lqxo 56-replication.lqxo 
