# Makefile for 
# Copyright (C) 2003 Greg Franks <greg@sce.carleton.ca>
# ------------------------------------------------------------------------
# $Id: Makefile 13875 2020-09-25 20:02:38Z greg $
# ------------------------------------------------------------------------

SRVN=	petrisrvn
OPTS=	
SANITY=	01-sanity.lqxo \
	02-sanity.lqxo \
	03-sanity.lqxo \
	04-sanity.lqxo \
	05-sanity.lqxo \
	06-sanity.lqxo \
	07-sanity.lqxo \
	08-sanity.lqxo 
DST=	$(SANITY) \
	10-interlock.lqxo \
	11-interlock.lqxo \
	12-interlock.lqxo \
	13-interlock.lqxo \
	15-split-interlock.lqxo \
	16-split-interlock.lqxo \
	20-multiserver.lqxo \
	21-multiserver.lqxo \
	22-multiserver.lqxo \
	23-multiserver.lqxo \
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
	52-replication.lqxo \
	60-semaphore.lqxo \
	61-semaphore.lqxo \
	62-semaphore.lqxo \
	63-sync.lqxo \
	70-priority.lqxo \
	77-semaphore.lqxo \
	80-sync.lqxo \
	81-sync.lqxo \
	82-sync.lqxo \
	83-sync.lqxo \
	85-fork.lqxo \
	86-fork.lqxo \
	87-fork.lqxo \
	88-quorum.lqxo \
	89-quorum.lqxo \
	90-A01.lqxo \
	90-B07.lqxo \
	91-cs3-1.lqxo \
	92-open.lqxo \
	93-simple-ucm.lqxo

SRC = $(DST:.lqxo=.lqnx)

.SUFFIXES:
.SUFFIXES: .diff .lqxo .lqnx
.PHONY:	.files

.lqnx.lqxo:
	$(SRVN) $(OPTS) -xo $@ $<

all:	$(DST) .files

sanity:	$(SANITY)

diff:
	srvndiff -@.files ../petri-results ../lqns-results .

.files:	
	@-rm .files
	for i in $(DST); do echo $$i >> .files; done

50-replication-flat.lqnx:	50-replication.lqnx
	rep2flat -Oxml $<

50-replication.lqxo:	50-replication-flat.lqxo
	lqn2ps -Oxml -o$*.lqxo --merge-replicas 50-replication-flat.lqxo

52-replication-flat.lqnx:	52-replication.lqnx
	rep2flat -Oxml $<

52-replication.lqxo:	52-replication-flat.lqxo
	lqn2ps -Oxml -o$*.lqxo --merge-replicas 52-replication-flat.lqxo

