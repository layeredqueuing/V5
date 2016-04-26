/* $Id: test02.c 9210 2010-02-24 18:59:24Z greg $ */
/************************************************************************/
/* test02.c:	PARASOL test program					*/
/*									*/
/* Created: 	07/06/95 (PRM)						*/
/*									*/
/* Description:	Tests the following functionality of buses		*/
/*		- Messages are transmitted at correct rate		*/
/*		- Messages types, text and ack ports are sent correctly	*/
/*		- Bus Usage statistics work correctly			*/
/*		- Buses using FCFS to resolve contention work properly	*/
/*		- Buses using RAND to resolve contention work properly	*/
/*									*/
/* Notes:	Bus A tests that messages are transmitted correctly	*/
/*		Bus B tests the transmission rate and utilization	*/
/*		Bus C tests FCFS contention resolution discipline	*/
/*		Bus D tests RAND contention resolution discipline	*/
/*		Usage of bus a should be around 95%			*/
/*		Usage of bus b should be exactly 50%			*/
/*		Usage of bus c should be around 90%			*/
/*		Usage of bus d should be around 90%			*/
/*									*/
/* Bugs:	This file does not test if the contention resolution	*/
/*		strategy is indeed random.  		 		*/
/*									*/
/************************************************************************/

#include <parasol.h>
#include "test.h"

#define ARATE 100
#define BRATE 1000
#define CRATE 10000
#define DRATE 100000

#define A1TYPE 1
#define A2TYPE 2
#define BTYPE 3
#define CTYPE 4
#define DTYPE 5
#define A1TEXT ((char*)0x3b875)
#define A2TEXT ((char*)0x11019)
#define BTEXT ((char*)0x20392)
#define CTEXT ((char*)0x83827)
#define DTEXT ((char*)0x9878d)
#define A1ACK NULL_PORT
#define A2ACK NULL_PORT
#define BACK NULL_PORT
#define CACK NULL_PORT
#define DACK NULL_PORT

long busa, busb, busc, busd;
long a1_port;
long a2_port;
long b_port;
long c_port;
long d_port;
long cserial = 0;
 
void ps_genesis (void * arg)
{
	void sender_a1(void *);
	void sender_a2(void *);
	void sender_b(void *);
	void sender_c(void *);
	void sender_d(void *);
	void receiver_a1(void *);
	void receiver_a2(void *);
	void receiver_b(void *);
	void receiver_c(void *);
	void receiver_d(void *);

	long tid, i;
	long nodes[10];

	for (i = 0; i < 10; i++)
		nodes[i] = ps_build_node("", 1, 1.0, 0.0, FCFS, FALSE);
	busa = ps_build_bus ("Bus A", 10, nodes, ARATE, FCFS, TRUE);
	busb = ps_build_bus ("Bus B", 10, nodes, BRATE, FCFS, TRUE);
	busc = ps_build_bus ("Bus C", 10, nodes, CRATE, FCFS, TRUE);
	busd = ps_build_bus ("Bus D", 10, nodes, DRATE, RAND, TRUE);

	ps_resume(ps_create("S A1", nodes[ps_choice(10)], ANY_HOST, sender_a1, 1));
	ps_resume(ps_create("S A2", nodes[ps_choice(10)], ANY_HOST, sender_a2, 1));
	ps_resume(ps_create("S B", nodes[ps_choice(10)], ANY_HOST, sender_b, 1));
	for (i = 0; i < 10; i++)
		ps_resume(ps_create("S C", nodes[ps_choice(10)], ANY_HOST, sender_c, 1));
	for (i = 0; i < 10; i++)
		ps_resume(ps_create("S D", nodes[ps_choice(10)], ANY_HOST, sender_d, 1));

	ps_resume(tid=ps_create("R A1", nodes[ps_choice(10)], ANY_HOST, receiver_a1, 1));
	a1_port = ps_std_port(tid);
	ps_resume(tid=ps_create("R A2", nodes[ps_choice(10)], ANY_HOST, receiver_a2, 1));
	a2_port = ps_std_port(tid);
	ps_resume(tid=ps_create("R B", nodes[ps_choice(10)], ANY_HOST, receiver_b, 1));
	b_port = ps_std_port(tid);
	ps_resume(tid=ps_create("R C", nodes[ps_choice(10)], ANY_HOST, receiver_c, 1));
	c_port = ps_std_port(tid);
	ps_resume(tid=ps_create("R D", nodes[ps_choice(10)], ANY_HOST, receiver_d, 1));
	d_port = ps_std_port(tid);

	ps_suspend(ps_myself);
}

void sender_a1(void * arg)
{
	while (TRUE) {
		if (ps_bus_send(busa, a1_port, A1TYPE, ARATE, A1TEXT, A1ACK)
		    == SYSERR)
			ERRABORT("Error on bus send");
		ps_sleep(ps_exponential(2.0));
	}
}

void sender_a2(void * arg)
{
	while (TRUE) {
		if (ps_bus_send(busa, a2_port, A2TYPE, ARATE, A2TEXT, A2ACK)
		    == SYSERR)
			ERRABORT("Error on bus send");
		ps_sleep(ps_exponential(2.0));
	}
}

void sender_b(void * arg)
{
	while (TRUE) {
		if (ps_bus_send(busb, b_port, BTYPE, BRATE, BTEXT, BACK)
		    == SYSERR)
			ERRABORT("Error on bus send");
		ps_sleep(2.0);
	}
}

void sender_c(void * arg)
{
	long temp;

	while (TRUE) {
		temp = cserial++;
		if (ps_bus_send(busc, c_port, temp, CRATE/10, CTEXT, CACK)
		    == SYSERR)
			ERRABORT("Error on bus send");
		ps_sleep(ps_exponential(1.1));
	}
}

void sender_d(void * arg)
{
	while (TRUE) {
		if (ps_bus_send(busd, d_port, DTYPE, DRATE/10, DTEXT, DACK)
		    == SYSERR)
			ERRABORT("Error on bus send");
		ps_sleep(ps_exponential(1.1));
	}
}

void receiver_a1(void * arg)
{
	long type, ack_port;
	double ts;
	char *text;

	while (TRUE) {
		if (ps_receive(a1_port, NEVER, &type, &ts, &text, &ack_port)
		    == SYSERR)
			ERRABORT("Error on receive");
		if (type != A1TYPE)
			ERRABORT("Invalid type");
		if (ts > ps_now - 0.999999)
			ERRABORT("Invalid time stamp or send rate");
		if (text != A1TEXT)
			ERRABORT("Invalid text");
		if (ack_port != A1ACK)
			ERRABORT("Invalid acknowledgement port");
	}
}

void receiver_a2(void * arg)
{
	long type, ack_port;
	double ts;
	char *text;

	while (TRUE) {
		if (ps_receive(a2_port, NEVER, &type, &ts, &text, &ack_port)
		    == SYSERR)
			ERRABORT("Error on receive");
		if (type != A2TYPE)
			ERRABORT("Invalid type");
		if (ts > ps_now - 0.999999)
			ERRABORT("Invalid time stamp or link send rate");
		if (text != A2TEXT)
			ERRABORT("Invalid text");
		if (ack_port != A2ACK)
			ERRABORT("Invalid acknowledgement port");
	}
}

void receiver_b(void * arg)
{
	long type, ack_port;
	double ts;
	char *text;

	while (TRUE) {
		if (ps_receive(b_port, NEVER, &type, &ts, &text, &ack_port)
		    == SYSERR)
			ERRABORT("Error on receive");
		if (type != BTYPE)
			ERRABORT("Invalid type");
		if (ps_now - ts - 1.0 > 0.00001)
			ERRABORT("Invalid time stamp or link send rate");
		if (text != BTEXT)
			ERRABORT("Invalid text");
		if (ack_port != BACK)
			ERRABORT("Invalid acknowledgement port");
	}
}

void receiver_c(void * arg)
{
	long type, ack_port;
	double ts;
	char *text;
	long last;

	last = -1;
	while (TRUE) {
		if (ps_receive(c_port, NEVER, &type, &ts, &text, &ack_port)
		    == SYSERR)
			ERRABORT("Error on receive");
		if (type <= last)
			ERRABORT("Bus is not FCFS");
		last = type;
		if (ps_now <= ts)
			ERRABORT("Invalid time stamp or link send rate");
		if (text != CTEXT)
			ERRABORT("Invalid text");
		if (ack_port != CACK)
			ERRABORT("Invalid acknowledgement port");
	}
}


void receiver_d(void * arg)
{
	long type, ack_port;
	double ts;
	char *text;
	long last;

	last = -1;
	while (TRUE) {
		if (ps_receive(d_port, NEVER, &type, &ts, &text, &ack_port)
		    == SYSERR)
			ERRABORT("Error on receive");
		if (type != DTYPE)
			ERRABORT("Invalid Type");
		if (ps_now <= ts)
			ERRABORT("Invalid time stamp or link send rate");
		if (text != DTEXT)
			ERRABORT("Invalid text");
		if (ack_port != DACK)
			ERRABORT("Invalid acknowledgement port");
	}
}

