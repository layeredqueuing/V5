# Makefile for lqns regression tests
# Copyright (C) 2003 Greg Franks <greg@sce.carleton.ca>
# ------------------------------------------------------------------------
# $Id: Makefile 15540 2022-04-17 13:26:30Z greg $
# ------------------------------------------------------------------------

SRVN=	lqns 
OPTS=	-Pvariance=mol,threads=hyper

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
	14-interlock.p \
	15-split-interlock.p \
	16-split-interlock.p \
	18-interlock.p \
	20-multiserver.p \
	21-multiserver.p \
	22-multiserver.p \
	23-multiserver.p \
	31-overtaking.p \
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
	51-replication.p \
	52-replication.p \
	55-replication.p \
	56-replication.p \
	70-priority.p \
	75-distributions.p \
	76-distributions.p \
	85-fork.p \
	86-fork.p \
	87-fork.p \
	88-quorum.p \
	89-quorum.p \
	90-A01.p \
	90-A01-novar.p \
	90-A01-stoch.p \
	90-B07.p \
	91-cs3-1.p \
	92-open.p \
	93-simple-ucm.p \
	94-5101-a2-q2b1.p

SRC = $(DST:.p=.in)

.SUFFIXES:
.SUFFIXES: .diff .p .xml .in
.PHONY:	.files

.in.p:
	$(SRVN) $(OPTS) -p  $<

.files:	
	@-rm .files
	for i in $(DST); do echo $$i >> .files; done


all:	$(DST) .files

diff:
	srvndiff -@.files ../petri-results ../sim-results .

90-A01-novar.p:	90-A01.in
	$(SRVN) -Pvariance=none -p -o$@ $<

90-A01-stoch.p: 90-A01.in
	$(SRVN) -Pvariance=stochastic -p -o$@ $<

