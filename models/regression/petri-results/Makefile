# Makefile for 
# Copyright (C) 2003 Greg Franks <greg@sce.carleton.ca>
# ------------------------------------------------------------------------
# $Id: Makefile 13923 2020-10-07 14:29:28Z greg $
# ------------------------------------------------------------------------

SRVN=	petrisrvn
OPTS=	
DST =	01-sanity.p \
	02-sanity.p \
	03-sanity.p \
	04-sanity.p \
	05-sanity.p \
	06-sanity.p \
	07-sanity.p \
	08-sanity.p \
	09-sanity.p \
	10-interlock.p \
	11-interlock.p \
	12-interlock.p \
	13-interlock.p \
	15-split-interlock.p \
	16-split-interlock.p \
	20-multiserver.p \
	21-multiserver.p \
	22-multiserver.p \
	23-multiserver.p \
	40-activities.p \
	41-activities.p \
	42-activities.p \
	43-activities.p \
	44-activities.p \
	45-activities.p \
	46-activities.p \
	47-activities.p \
	48-activities.p \
	50-replication.p \
	52-replication.p \
	60-semaphore.p \
	61-semaphore.p \
	62-semaphore.p \
	63-sync.p \
	70-priority.p \
	80-sync.p \
	81-sync.p \
	82-sync.p \
	83-sync.p \
	85-fork.p \
	86-fork.p \
	87-fork.p \
	88-quorum.p \
	89-quorum.p \
	90-A01.p \
	90-B07.p \
	91-cs3-1.p \
	92-open.p \
	93-simple-ucm.p

SRC = $(DST:.p=.in)

.SUFFIXES:
.SUFFIXES: .diff .p .xml .in
.PHONY:	.files

.in.p:
	$(SRVN) $(OPTS) -p -o $*.out $<

all:	$(DST) .files

diff:
	srvndiff -@.files ../sim-results ../lqns-results .

.files:	
	@-rm .files
	for i in $(DST); do echo $$i >> .files; done

50-replication.p:	50-replication.in
	rep2flat 50-replication.in
	$(SRVN) $(OPTS) -p 50-replication-flat.in
	lqn2ps -Oparseable -o$*.p --merge-replicas --parse-file=$*-flat.p 50-replication-flat.in

52-replication.p:	52-replication.in
	rep2flat 52-replication.in
	$(SRVN) $(OPTS) -p 52-replication-flat.in
	lqn2ps -Oparseable -o$*.p --merge-replicas --parse-file=$*-flat.p 52-replication-flat.in

