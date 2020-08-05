# Makefile for lqns regression tests
# Copyright (C) 2003 Greg Franks <greg@sce.carleton.ca>
# ------------------------------------------------------------------------
# $Id: Makefile 13729 2020-08-04 20:20:16Z greg $
# ------------------------------------------------------------------------

SRVN=	lqns
OPTS=	-Pvariance=mol,threads=hyper

SRCS =	01-sanity.lqnx \
	02-sanity.lqnx \
	03-sanity.lqnx \
	04-sanity.lqnx \
	05-sanity.lqnx \
	06-sanity.lqnx \
	07-sanity.lqnx \
	08-sanity.lqnx \
	09-sanity.lqnx \
	10-interlock.lqnx \
	11-interlock.lqnx \
	12-interlock.lqnx \
	13-interlock.lqnx \
	15-split-interlock.lqnx \
	16-split-interlock.lqnx \
	20-multiserver.lqnx \
	21-multiserver.lqnx \
	22-multiserver.lqnx \
	23-multiserver.lqnx \
	40-activities.lqnx \
	41-activities.lqnx \
	42-activities.lqnx \
	43-activities.lqnx \
	44-activities.lqnx \
	45-activities.lqnx \
	46-activities.lqnx \
	50-replication.lqnx \
	51-replication.lqnx \
	52-replication.lqnx \
	55-replication.lqnx \
	56-replication.lqnx \
	70-priority.lqnx \
	75-distributions.lqnx \
	76-distributions.lqnx \
	85-fork.lqnx \
	86-fork.lqnx \
	87-fork.lqnx \
	90-A01.lqnx \
	90-B07.lqnx \
	91-cs3-1.lqnx \
	92-open.lqnx \
	93-simple-ucm.lqnx \
	94-5101-a2-q2b1.lqnx \
	95-async-inf.lqnx

QUORUM=	88-quorum.lqnx \
	89-quorum.lqnx 

DSTS = $(SRCS:.lqnx=.lqxo)

.SUFFIXES:
.SUFFIXES: .diff .lqxo .lqnx
.PHONY:	.files

.lqnx.lqxo:
	$(SRVN) $(OPTS) $<

all:	$(DSTS) .files

diff:
	srvndiff -@.files ../petri-results ../sim-results .

.files:	
	@-rm -f .files
	for i in $(DST); do echo $$i >> .files; done
