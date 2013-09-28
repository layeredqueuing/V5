/* $Id$ */
/************************************************************************/
/* test04.c:	PARASOL test program					*/
/*									*/
/* Created: 	07/06/95 (PRM)						*/
/*									*/
/* Description:	Tests shared ports including:				*/
/*		- Correct sending of messages				*/
/*		- Correct FCFS queueing of messages			*/
/*		- timeout values of IMMEDIATE, NEVER, and .1		*/
/*									*/
/* Notes:	This program should produce no output if it completes	*/
/*		successfully.						*/
/*									*/
/************************************************************************/

#include <parasol.h>
#include "test.h"

#define ATYPE 1
#define BTYPE 3
#define CTYPE 4
#define ATEXT ((char*)0x3b875)
#define BTEXT ((char*)0x20392)
#define CTEXT ((char*)0x83827)
#define AACK NULL_PORT
#define BACK NULL_PORT
#define CACK NULL_PORT
 
long aport, bport, cport;
double alast = -1.0, blast = -1.0, clast = -1.0;

void ps_genesis(void * arg)
{
	void sender_a(void *);
	void sender_b(void *);
	void sender_c(void *);
 	void receiver_a(void *);
	void receiver_b(void *);
	void receiver_c(void *);
 	long i;

	aport = ps_allocate_shared_port("Shared Port A");
	bport = ps_allocate_shared_port("Shared Port B");
	cport = ps_allocate_shared_port("Shared Port C");

	for (i = 0; i < 10; i++)
		ps_resume (ps_create("R A", 0, ANY_HOST, receiver_a, 1));
	for (i = 0; i < 10; i++)
		ps_resume (ps_create("R B", 0, ANY_HOST, receiver_b, 1));
	for (i = 0; i < 10; i++)
		ps_resume (ps_create("R C", 0, ANY_HOST, receiver_c, 1));

	for (i = 0; i < 10; i++)
		ps_resume (ps_create("S A", 0, ANY_HOST, sender_a, 1));
	for (i = 0; i < 10; i++)
		ps_resume (ps_create("S B", 0, ANY_HOST, sender_b, 1));
	for (i = 0; i < 10; i++)
		ps_resume (ps_create("S C", 0, ANY_HOST, sender_c, 1));

	ps_suspend(ps_myself);
}

void sender_a (void * arg)
{
	while (TRUE) {
 		if (ps_send(aport, ATYPE, ATEXT, AACK) == SYSERR)
			ERRABORT("Send Failed");
		ps_sleep(ps_exponential(1.0));
	}
}

void receiver_a(void * arg)
{
	long type, ack_port;
	double ts;
	char *text;

	ps_sleep(ps_random);
 	while (TRUE) {
		if (ps_receive_shared(aport, NEVER, &type, &ts, &text, 
		    &ack_port) == SYSERR)
			ERRABORT("Receive failed");
		if (type != ATYPE)
			ERRABORT("Invalid type");
 		if (ts < alast)
			ERRABORT("ps_receive_shared is not FCFS");
		alast = ts;
		if (text != ATEXT)
			ERRABORT("Invalid text");
		if (ack_port != AACK)
			ERRABORT("Invalid acknowledgement port");
		ps_sleep (ps_exponential(1.0));
 	}
 }

void sender_b (void * arg)
{
	while (TRUE) {
 		if (ps_send(bport, BTYPE, BTEXT, BACK) == SYSERR)
			ERRABORT("Send Failed");
		ps_sleep(ps_exponential(1.0));
	}
}

void receiver_b(void * arg)
{
	long type, ack_port;
	double ts;
	char *text;

	ps_sleep (ps_random);
 	while (TRUE) {
		while (ps_receive_shared(bport, IMMEDIATE, &type, &ts, &text,
		    &ack_port)) {
 			if (type != BTYPE)
				ERRABORT("Invalid type");
 			if (blast > ts)
				ERRABORT("ps_receive_shared is not FCFS");
			blast = ts;
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
	while (TRUE) {
		if (ps_send(cport, CTYPE, CTEXT, CACK) == SYSERR)
			ERRABORT("Send Failed");
		ps_sleep(ps_exponential(1.0));
	}
}

void receiver_c(void * arg)
{
	long type, ack_port;
	double ts;
	char *text;

	ps_sleep (ps_random);
 	while (TRUE) {
		while (ps_receive_shared(cport, .1, &type, &ts, &text,
		    &ack_port)) {
 			if (type != CTYPE)
				ERRABORT("Invalid type");
 			if (clast > ts)
				ERRABORT("ps_receive_shared is not FCFS");
			clast = ts;
			if (text != CTEXT)
				ERRABORT("Invalid text");
			if (ack_port != CACK)
				ERRABORT("Invalid acknowledgement port");
		}
		ps_sleep (1.0);
 	}
}

