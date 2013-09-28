# $Id$
# ------------------------------------------------------------------------

diff: exact.lqxo sim.lqxo
	srvndiff -t +b exact.lqxo sim.lqxo

exact.lqxo:	model2p-2x-fcfs.lqnx
	petrisrvn -xo $@ $<

sim.lqxo:	model2p-2x-fcfs.lqnx
	lqsim -C1.0 -S123456 -xo $@ $<

diff-1: exact-1.lqxo sim-1.lqxo
	srvndiff  -t +b exact-1.lqxo sim-1.lqxo

exact-1.lqxo:	model2p-2x-fcfs-1.lqnx
	petrisrvn -xo $@ $<

sim-1.lqxo:	model2p-2x-fcfs-1.lqnx
	lqsim -C1.0 -S123456 -xo $@ $<

diff-2: exact-2.lqxo sim-2.lqxo 
	srvndiff exact-2.lqxo sim-2.lqxo

exact-2.lqxo: send-interlock.lqnx
	petrisrvn -xo $@ $<

sim-2.lqxo: send-interlock.lqnx
	lqsim -C1.0 -S123456 -xo $@ $<
