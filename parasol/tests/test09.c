/* $Id$ */
/************************************************************************/
/* test09.c:	PARASOL test program					*/
/*									*/
/* Created: 	07/06/95 (PRM)						*/
/*									*/
/* Description:	Tests port set functions including			*/
/*		- Correct sending of messages				*/
/*		- Correct FCFS queueing of messages			*/
/*		- timeout values of IMMEDIATE, NEVER, and .1		*/
/*		- the use of port set to implement prioritized messages.*/
/*									*/
/* Notes:	This program should produce no output if it completes	*/
/*		successfully. This test is very slow because of all the	*/
/*		dynamic task creation going on.				*/
/*									*/
/* Bugs:						 		*/
/*									*/
/************************************************************************/

#include <parasol.h>
#include "test.h"

#define NPORTS 20

#define ATYPE 1
#define BTYPE 3
#define CTYPE 4
#define DTYPE 5
#define ATEXT ((char*)0x3b875)
#define BTEXT ((char*)0x20392)
#define CTEXT ((char*)0x83827)
#define DTEXT ((char*)0x9abcd)
#define AACK NULL_PORT
#define BACK NULL_PORT
#define CACK NULL_PORT
#define DACK NULL_PORT
 
long aports[NPORTS];
long bports[NPORTS];
long cports[NPORTS];
long dports[NPORTS];
 

void ps_genesis(void * arg)
{
	void sender_a(void *);
	void sender_b(void *);
	void sender_c(void *);
	void sender_d(void *);
	void receiver_a(void *);
	void receiver_b(void *);
	void receiver_c(void *);
	void receiver_d(void *);
	long i;

	ps_resume (ps_create("R A", 0, ANY_HOST, receiver_a, 1));
 	ps_resume (ps_create("R B", 0, ANY_HOST, receiver_b, 1));
 	ps_resume (ps_create("R C", 0, ANY_HOST, receiver_c, 1));
	ps_resume (ps_create("R D", 0, ANY_HOST, receiver_d, 1));
  	for (i = 0; i < 10; i++)
		ps_resume (ps_create("S A", 0, ANY_HOST, sender_a, 1));
	for (i = 0; i < 10; i++)
		ps_resume (ps_create("S B", 0, ANY_HOST, sender_b, 1));
	for (i = 0; i < 10; i++)
		ps_resume (ps_create("S C", 0, ANY_HOST, sender_c, 1));
	for (i = 0; i < 10; i++)
		ps_resume (ps_create("S D", 0, ANY_HOST, sender_d, 1));

	ps_suspend(ps_myself);
}

void sender_a (void * arg)
{
	ps_sleep(ps_exponential(1.0));
	while (TRUE) {
 		if (ps_send(aports[ps_choice(NPORTS)], ATYPE, ATEXT, AACK) 
		    == SYSERR)
			ERRABORT("Send Failed");
		ps_sleep(1.0);
	}
}

void receiver_a(void * arg)
{
	long type, ack_port;
	double ts;
	char *text;
	double last = 0.0;
	long ps, i;

	ps = ps_allocate_port_set ("Port Set A");
	for (i = 0; i < NPORTS; i++) {
		aports[i] = ps_allocate_port("Port A", ps_myself);
		ps_join_port_set(ps, aports[i]);
	}

 	while (TRUE) {
		if (ps_receive(ps, NEVER, &type, &ts, &text, &ack_port) 
		    == SYSERR)
			ERRABORT("Receive failed");
		if (type != ATYPE)
			ERRABORT("Invalid Type");
 		if (last > ts)
			ERRABORT("Port set is not FCFS");
		ts = last;
		if (text != ATEXT)
			ERRABORT("Invalid text");
		if (ack_port != AACK)
			ERRABORT("Invalid acknowledgement port");
		ps_sleep (.099);
 	}
 }
 
void sender_b (void * arg)
{
	ps_sleep(ps_exponential(1.0));
	while (TRUE) {
 		if (ps_send(bports[ps_choice(NPORTS)], BTYPE, BTEXT, BACK) 
		    == SYSERR)
			ERRABORT("Send Failed");
		ps_sleep(1.0);
	}
}

void receiver_b(void * arg)
{
	long type, ack_port;
	double ts, last = 0.0;
	char *text;
	long ps, i;

	ps = ps_allocate_port_set ("Port Set B");
	for (i = 0; i < NPORTS; i++) {
		bports[i] = ps_allocate_port("Port B", ps_myself);
		ps_join_port_set(ps, bports[i]);
	}

 	while (TRUE) {
		while (ps_receive(ps, IMMEDIATE, &type, &ts, &text,
		    &ack_port)) {
 			if (type != BTYPE)
				ERRABORT("Invalid type");
 			if (last > ts)
				ERRABORT("Port set is not FCFS");
			last = ts;
			if (text != BTEXT)
				ERRABORT("Invalid text");
			if (ack_port != BACK)
				ERRABORT("Invalid acknowledgement port");
		}
		ps_sleep (1.0);
 	}
 }

void sender_c (void * arg)
{
 	ps_sleep(ps_exponential(1.0));
	while (TRUE) {
 		if (ps_send(cports[ps_choice(NPORTS)], CTYPE, CTEXT, CACK) 
		    == SYSERR)
			ERRABORT("Send Failed");
		ps_sleep(ps_exponential(1.0));
	}
}

void receiver_c(void * arg)
{
	long type, ack_port;
	double ts, last = 0.0;
	char *text;
	long ps, i;

	ps = ps_allocate_port_set ("Port Set C");
	for (i = 0; i < NPORTS; i++) {
		cports[i] = ps_allocate_port("Port C", ps_myself);
		ps_join_port_set(ps, cports[i]);
	}

 	while (TRUE) {
		while (ps_receive(ps, .1, &type, &ts, &text,
		    &ack_port)) {
 			if (type != CTYPE)
				ERRABORT("Invalid type");
 			if (last > ts)
				ERRABORT("Port set is not FCFS");
			last = ts;
			if (text != CTEXT)
				ERRABORT("Invalid text");
			if (ack_port != CACK)
				ERRABORT("Invalid acknowledgement port");
		}
		ps_sleep (1.0);
 	}
 }

void sender_d(void * arg)
{
 	ps_sleep(ps_exponential(1.0));
	while (TRUE) {
 		if (ps_send(dports[ps_choice(NPORTS)], DTYPE, DTEXT, DACK) 
		    == SYSERR)
			ERRABORT("Send Failed");
		ps_sleep(ps_exponential(1.0));
	}
}

void receiver_d(void * arg)
{
	long type, ack_port;
	double ts;
	char *text;
	long ps, i, remove;

	ps = ps_allocate_port_set ("Port Set D");
	for (i = 0; i < NPORTS; i++)
		dports[i] = ps_allocate_port("Port D", ps_myself);


 	while (TRUE) {
		for (i = 0; i < NPORTS; i++)
			if (ps_receive(dports[i], IMMEDIATE, &type, &ts, &text,
			    &ack_port))
				break;
		remove = FALSE;
		if (i == NPORTS) {
			for (i = 0; i < NPORTS; i++)
				ps_join_port_set(ps, dports[i]);
			
			if (ps_receive(ps, NEVER, &type, &ts, &text, &ack_port)
			    == SYSERR)
				ERRABORT ("Error on ps_receive");
			remove = TRUE;
		}
 		if (type != DTYPE)
			ERRABORT("Invalid type");
 		if (ps_now < ts)
			ERRABORT("Invalid time stamp");
 		if (text != DTEXT)
			ERRABORT("Invalid text");
		if (ack_port != DACK)
			ERRABORT("Invalid acknowledgement port");
		if (remove) {
			for (i = 0; i < NPORTS; i++)
				ps_leave_port_set(ps, dports[i]);
		}
		ps_sleep(.099);
	}
}
