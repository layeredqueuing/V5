# Makefile for regression test
# Copyright (C) 2005 Greg Franks <greg@sce.carleton.ca>
#
# ------------------------------------------------------------------------
# $Id: Makefile.am 16199 2022-12-26 15:04:39Z greg $
# ------------------------------------------------------------------------

SRVN=	../lqns
OPTS=	-Pvariance=mol,threads=hyper -w
DIFFOPTS= -Q -S0.001 

SANITY=	01-sanity.lqxo \
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
	18-interlock.lqxo
MULTI=	20-multiserver.lqxo \
	21-multiserver.lqxo \
	22-multiserver.lqxo \
	23-multiserver.lqxo \
	24-multiserver.lqxo
OVER=	31-overtaking.lqxo \
	32-async-inf.lqxo
ACT=	40-activities.lqxo \
	41-activities.lqxo \
	42-activities.lqxo \
	43-activities.lqxo \
	44-activities.lqxo \
	45-activities.lqxo \
	46-activities.lqxo \
	47-activities.lqxo \
	48-activities.lqxo
REPL=	50-replication.lqxo \
	51-replication.lqxo \
	52-replication.lqxo \
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

VAR=	90-A01-novar.lqxo \
	90-A01-stoch.lqxo

DISTTEST=disttest


# Can't do: $(SYNC)  Should omit $(SIM) to, as the results aren't meaningful.
OBJS=	$(SANITY) $(INTER) $(MULTI) $(OVER) $(ACT) $(REPL) $(FORK) $(OTHER)
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
	@srvndiff $(DIFFOPTS) $@ ../../models/regression/lqns-results/$@


.lqnx.d:
	$(SRVN) $(OPTS) $<

.lqxo.diff:
	-if test -f ../../models/regression/petri-results/$<; \
	then srvndiff --print-comment -IC -o $@ ../../models/regression/{petri,sim,lqns}-results/$< $<; \
	else srvndiff --print-comment -IC -o $@ ../../models/regression/{sim,lqns}-results/$< $<; \
	fi

all:

check:	$(OBJS) $(VAR)
	echo $(OBJS) $(VAR) | tr -s " " "\n" | srvndiff $(DIFFOPTS) -@- . ../../models/regression/lqns-results

recheck:
	rm -f $(OBJS) $(VAR)
	$(MAKE) -k check

summary:	$(OBJS)
	for i in $(OBJS); do srvndiff -Q --ignore-errors -S0.001 -o /dev/null ../../models/regression-xml/lqns-results/$$i $$i; \
	if test $$? == 0; then result="pass"; else result="fail"; fi; \
	echo \"$$i\",$$result; done

check-syntax:
	$(MAKE) OPTS=-n check

check-verbose:
	@-echo $(OBJS) | tr -s " " "\n" > .files
	srvndiff -a +t -@.files ../../models/regression/{sim,lqns}-results .
	@-rm .files

check-summary:	$(OBJS)
	@-echo $(OBJS) | tr -s " " "\n" > .files
	srvndiff -Ia -S10. -T +t -@.files ../../models/regression/{sim,lqns}-results .
	@-rm .files

disttest: disttest.o randomvar.o
	$(CXX) -L../../libmva/src/.libs -lmva -o $@ disttest.o randomvar.o

randomvar.o:	../randomvar.cc ../randomvar.h
	$(CXX) -DTESTDIST -I.. -I../../libmva/src/headers -c -o randomvar.o ../randomvar.cc

disttest.o:	disttest.cc
	$(CXX) -I.. -I../../libmva/src/headers -c -o disttest.o disttest.cc

clean:
	@-rm -f *.lqxo *.out *.diff .files
	@-rm -rf $(OBJS2)

distclean:	clean 
	@-rm -f Makefile

# 48-activities.lqxo: 48-activities.lqnx
# 	$(SRVN) $(OPTS) -a $<
# 	@srvndiff $(DIFFOPTS) $@ ../../models/regression/lqns-results/$@

90-A01-novar.lqxo: 90-A01.lqnx
	$(SRVN) -Pvariance=none -a -xo$@ 90-A01.lqnx
	@srvndiff $(DIFFOPTS) $@ ../../models/regression/lqns-results/$@

90-A01-stoch.lqxo: 90-A01.lqnx
	$(SRVN) -Pvariance=stochastic -a -xo$@ 90-A01.lqnx
	@srvndiff $(DIFFOPTS) $@ ../../models/regression/lqns-results/$@

94-5101-a2-q2b1.lqxo: 94-5101-a2-q2b1.lqnx
	$(SRVN) $(OPTS) -a $<
	@srvndiff $(DIFFOPTS) $@ ../../models/regression/lqns-results/$@

sanity:	$(SANITY)
	echo $(SANITY) | tr -s " " "\n" | srvndiff -Q -@- . ../../models/regression/lqns-results

interlock:	$(INTER)
	echo $(INTER) | tr -s " " "\n" | srvndiff -Q -@- . ../../models/regression/lqns-results

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
