# Makefile for regression test
# Copyright (C) 2005 Greg Franks <greg@sce.carleton.ca>
#
# ------------------------------------------------------------------------
# $Id$
# ------------------------------------------------------------------------

SRVN=	../lqns
OPTS=	-Pvariance=mol,threads=mak -w -M 

SANITY=	01-sanity.lqxo \
	02-sanity.lqxo \
	03-sanity.lqxo \
	04-sanity.lqxo \
	05-sanity.lqxo \
	06-sanity.lqxo \
	07-sanity.lqxo 
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
REPL=	50-replication.lqxo \
	51-replication.lqxo \
	55-replication.lqxo \
	56-replication.lqxo
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
	87-fork.lqxo 
QUORUM=	88-quorum.lqxo \
	89-quorum.lqxo 
OTHER=	90-A01.lqxo \
	90-B07.lqxo \
	91-cs3-1.lqxo \
	92-open.lqxo \
	93-simple-ucm.lqxo \
	94-5101-a2-q2b1.lqxo

# Can't do: $(SYNC)  Should omit $(SIM) to, as the results aren't meaningful.
OBJS=	$(SANITY) $(INTER) $(MULTI) $(ACT) $(REPL) $(FORK) $(OTHER)
if QUORUM
#OBJS +=	$(QUORUM)
endif
OBJS2=	01L-sanity.d 20L-multiserver.d

SRCS=	$(OBJS:.lqxo=.lqnx)
SRCS2=	$(OBJS2:.d=.lqnx)

EXTRA_DIST = $(SRCS) $(SRCS2)

SUFFIXES = .diff .d .lqxo .lqnx 

.lqnx.lqxo:
	$(SRVN) $(OPTS) $<
	@srvndiff -Q -S0.001 $@ ../../models/regression/lqns-results/$*.p


.lqnx.d:
	$(SRVN) $(OPTS) $<

.lqxo.diff:
	-srvndiff $< ../../models/regression/lqns-results/$*.p > $@

check:	$(OBJS)
	echo $(OBJS) | tr -s " " "\n" | srvndiff -Q -S0.001 -@- . ../../models/regression/lqns-results

check-syntax:
	$(MAKE) OPTS=-n check

check-verbose:
	FILES=`echo $(OBJS) | tr -s " " "\n" | srvndiff -Q -S0.001 -@- . ../../models/regression/lqns-results`; \
	for i in $$FILES; do j=`echo $$i | sed -e's/lqxo/p/'`; srvndiff -A +t ../../models/regression/lqns-results/$$j $$i; done

all:

clean:
	@-rm -f *.lqxo *.out *.diff
	@-rm -rf $(OBJS2)

distclean:	clean 
	-rm -f Makefile

sanity:	$(SANITY)
	echo $(SANITY) | tr -s " " "\n" | srvndiff -Q -@- . ../../models/regression/lqns-results

multi:	$(MULTI)
	echo $(MULTI) | tr -s " " "\n" | srvndiff -Q -@- -S0.01 . ../../models/regression/lqns-results

act:	$(ACT)
	echo $(ACT) | tr -s " " "\n" | srvndiff -Q -@- -S0.01 . ../../models/regression/lqns-results

fork:	$(FORK)
	echo $(FORK) | tr -s " " "\n" | srvndiff -Q -@- -S0.01 . ../../models/regression/lqns-results

other:	$(OTHER)
	echo $(OTHER) | tr -s " " "\n" | srvndiff -Q -@- -S0.02 . ../../models/regression/lqns-results

repl:	$(REPL)
	echo $(REPL) | tr -s " " "\n" | srvndiff -Q -@- -S0.02 . ../../models/regression/lqns-results
