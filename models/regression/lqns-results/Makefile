# Makefile for lqns regression tests
# Copyright (C) 2003 Greg Franks <greg@sce.carleton.ca>
# ------------------------------------------------------------------------
# $Id: Makefile 17394 2024-10-28 11:31:19Z greg $
# ------------------------------------------------------------------------

SRVN=	lqns
OPTS=	-Pvariance=mol,threads=hyper

DSTS =	00-sanity.lqxo \
	01-sanity.lqxo \
	02-sanity.lqxo \
	03-sanity.lqxo \
	04-sanity.lqxo \
	05-sanity.lqxo \
	06-sanity.lqxo \
	07-sanity.lqxo \
	09-sanity.lqxo \
	10-interlock.lqxo \
	11-interlock.lqxo \
	12-interlock.lqxo \
	13-interlock.lqxo \
	14-interlock.lqxo \
	15-split-interlock.lqxo \
	16-split-interlock.lqxo \
	18-interlock.lqxo \
	19-interlock.lqxo \
	20-multiserver.lqxo \
	21-multiserver.lqxo \
	22-multiserver.lqxo \
	23-multiserver.lqxo \
	24-multiserver.lqxo \
	31-overtaking.lqxo \
	32-async-inf.lqxo \
	33-spex-convergence.lqxo \
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
	50-replication-pan.lqxo \
	51-replication.lqxo \
	51-replication-pan.lqxo \
	52-replication.lqxo \
	52-replication-pan.lqxo \
	55-replication.lqxo \
	55-replication-pan.lqxo \
	56-replication.lqxo \
	56-replication-pan.lqxo \
	75-distributions.lqxo \
	76-distributions.lqxo \
	85-fork.lqxo \
	86-fork.lqxo \
	87-fork.lqxo \
	90-A01.lqxo \
	90-A01-novar.lqxo \
	90-A01-stoch.lqxo \
	90-B07.lqxo \
	91-cs3-1.lqxo \
	92-open.lqxo \
	93-simple-ucm.lqxo \
	94-5101-a2-q2b1.lqxo \
	95-async-inf.lqxo

QUORUM=	88-quorum.lqxo \
	89-quorum.lqxo

SRCS = $(DSTS:.lqxo=.lqnx)

.SUFFIXES:
.SUFFIXES: .diff .out .lqxo .lqnx
.PHONY:	.files

.lqnx.lqxo:
	$(SRVN) $(OPTS) $<

.files:	
	@-rm -f .files
	for i in $(DST); do echo $$i >> .files; done

all:	$(DSTS) .files

33-spex-convergence.lqxo:	33-spex-convergence.lqnx
	$(SRVN) -o $*.out $<
	mv $*.d/$*-002.lqxo $*.lqxo
	rm $*.d

50-replication-pan.lqxo: 50-replication.lqnx
	$(SRVN) $(OPTS) -Preplication=pan -xo $@ $<

51-replication-pan.lqxo: 51-replication.lqnx
	$(SRVN) $(OPTS) -Preplication=pan -xo $@ $<

52-replication-pan.lqxo: 52-replication.lqnx
	$(SRVN) $(OPTS) -Preplication=pan -xo $@ $<

55-replication-pan.lqxo: 55-replication.lqnx
	$(SRVN) $(OPTS) -Preplication=pan -w -xo $@ $<

56-replication-pan.lqxo: 56-replication.lqnx
	$(SRVN) $(OPTS) -Preplication=pan -w -xo $@ $<

90-A01-novar.lqxo:	90-A01.lqnx
	$(SRVN) -Pvariance=none -x -o$@ $<

90-A01-stoch.lqxo: 90-A01.lqnx
	$(SRVN) -Pvariance=stochastic -x -o$@ $<

diff:
	srvndiff -@.files ../petri-results ../sim-results .

