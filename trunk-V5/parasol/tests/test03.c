/* $Id$ */
/************************************************************************/
/* test03.c:	PARASOL test program					*/
/*									*/
/* Created: 	07/06/95 (PRM)						*/
/*									*/
/* Description:	Tests message sending and receiving functions including	*/
/*		- Correct sending of messages				*/
/*		- Correct FCFS queueing of messages			*/
/*		- ps_receive_last works in a LIFO manner		*/
/*		- ps_receive_random works				*/
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
#define DTYPE 5
#define ETYPE 6
#define FTYPE 7
#define ATEXT ((char*)0x3b875)
#define BTEXT ((char*)0x20392)
#define CTEXT ((char*)0x83827)
#define DTEXT ((char*)0x9878d)
#define ETEXT ((char*)0x8322e)
#define FTEXT ((char*)0x23484)
#define AACK NULL_PORT
#define BACK NULL_PORT
#define CACK NULL_PORT
#define DACK NULL_PORT
#define EACK NULL_PORT
#define FACK NULL_PORT

long bpool;

long aport, bport, cport, dport, eport, fport, gport;
 
void ps_genesis(void * arg)
{
	void sender_a(void *);
	void sender_b(void *);
	void sender_c(void *);
	void sender_d(void *);
	void sender_e(void *);
	void sender_f(void *);
	void receiver_a(void *);
	void receiver_b(void *);
	void receiver_c(void *);
	void receiver_d(void *);
	void receiver_e(void *);
	void receiver_f(void *);
	void receiver_g(void *);
	long i, tid;

	bpool = ps_build_buffer_pool(30);
	ps_resume (tid = ps_create("R A", 0, ANY_HOST, receiver_a, 1));
	aport = ps_std_port(tid);
	ps_resume (tid = ps_create("R B", 0, ANY_HOST, receiver_b, 1));
	bport = ps_std_port(tid);
	ps_resume (tid = ps_create("R C", 0, ANY_HOST, receiver_c, 1));
	cport = ps_std_port(tid);
	ps_resume (tid = ps_create("R D", 0, ANY_HOST, receiver_d, 1));
	dport = ps_std_port(tid);
	ps_resume (tid = ps_create("R E", 0, ANY_HOST, receiver_e, 1));
	eport = ps_std_port(tid);
	ps_resume (tid = ps_create("R F", 0, ANY_HOST, receiver_f, 1));
	fport = ps_std_port(tid);
	ps_resume (tid = ps_create("R G", 0, ANY_HOST, receiver_g, 1));
	gport = ps_std_port(tid);
	for (i = 0; i < 10; i++)
		ps_resume (ps_create("S A", 0, ANY_HOST, sender_a, 1));
	for (i = 0; i < 10; i++)
		ps_resume (ps_create("S B", 0, ANY_HOST, sender_b, 1));
	for (i = 0; i < 10; i++)
		ps_resume (ps_create("S C", 0, ANY_HOST, sender_c, 1));
	for (i = 0; i < 10; i++)
		ps_resume (ps_create("S D", 0, ANY_HOST, sender_d, 1));
	for (i = 0; i < 10; i++)
		ps_resume (ps_create("S E", 0, ANY_HOST, sender_e, 1));
	ps_resume (ps_create("S F", 0, ANY_HOST, sender_f, 1));

	ps_suspend(ps_myself);
}

void sender_a (void * arg)
{
	while (TRUE) {
 		if (ps_send(aport, ATYPE, ATEXT, AACK) == SYSERR)
			ERRABORT("Send Failed");
		ps_sleep(1.0);
	}
}

void receiver_a(void * arg)
{
	long type, ack_port;
	double ts;
	char *text;
	double last = -1.0;

 	while (TRUE) {
		if (ps_receive(aport, NEVER, &type, &ts, &text, &ack_port) 
		    == SYSERR)
			ERRABORT("Receive failed");
		if (type != ATYPE)
			ERRABORT("Invalid Type");
 		if (ts < last)
			ERRABORT("ps_receive is not FCFS");
		last = ts;
		if (text != ATEXT)
			ERRABORT("Invalid text");
		if (ack_port != AACK)
			ERRABORT("Invalid acknowledgement port");
		ps_sleep (.099);
 	}
 }

void sender_b (void * arg)
{
	while (TRUE) {
 		if (ps_send(bport, BTYPE, BTEXT, BACK) == SYSERR)
			ERRABORT("Send Failed");
		ps_sleep(1.0);
	}
}

void receiver_b(void * arg)
{
	long type, ack_port, i;
	double ts;
	char *text;
	double last;

	ps_sleep (0.1);
 	while (TRUE) {
		ps_sleep(1.0);
		for (i = 0; i < 10; i++) {
			if (ps_receive_last(bport, NEVER, &type, &ts, &text, 
			    &ack_port) == SYSERR)
				ERRABORT("Receive failed");
			if (type != BTYPE)
				ERRABORT("Invalid Type");
 			if (i != 0 && ts > last)
				ERRABORT("Invalid time stamp");
			last = ts;
			if (text != BTEXT)
				ERRABORT("Invalid text");
			if (ack_port != BACK)
				ERRABORT("Invalid acknowledgement port");
		}
 	}
 }

void sender_c (void * arg)
{
	while (TRUE) {
		if (ps_send(cport, CTYPE, CTEXT, CACK) == SYSERR)
			ERRABORT("Send Failed");
		ps_sleep(1.0);
	}
}

void receiver_c(void * arg)
{
	long type, ack_port;
	double ts;
	char *text;
	long i;

	ps_sleep (0.1);
 	while (TRUE) {
		ps_sleep(1.0);
		for (i = 0; i < 10; i++) {
			if (ps_receive_random(cport, NEVER, &type, &ts, &text, 
			    &ack_port) == SYSERR)
				ERRABORT("Receive failed");
			if (type != CTYPE)
				ERRABORT("Invalid type");
			if (ps_now < ts)
				ERRABORT("Invalid time stamp");
			if (text != CTEXT)
				ERRABORT("Invalid text");
			if (ack_port != CACK)
				ERRABORT("Invalid acknowledgement port");
		}
 	}
 }

void sender_d (void * arg)
{
	while (TRUE) {
 		if (ps_send(dport, DTYPE, DTEXT, DACK) == SYSERR)
			ERRABORT("Send Failed");
		ps_sleep(ps_exponential(1.0));
	}
}

void receiver_d(void * arg)
{
	long type, ack_port;
	double ts;
	char *text;
 
 	while (TRUE) {
		while (ps_receive(dport, IMMEDIATE, &type, &ts, &text,
		    &ack_port)) {
 			if (type != DTYPE)
				ERRABORT("Invalid type");
 			if (ps_now < ts)
				ERRABORT("Invalid time stamp");
			if (text != DTEXT)
				ERRABORT("Invalid text");
			if (ack_port != DACK)
				ERRABORT("Invalid acknowledgement port");
		}
		ps_sleep (1.0);
 	}
 }

void sender_e (void * arg)
{
	while (TRUE) {
		if (ps_send(eport, ETYPE, ETEXT, EACK) == SYSERR)
			ERRABORT("Send Failed");
		ps_sleep(ps_exponential(1.0));
	}
}

void receiver_e(void * arg)
{
	long type, ack_port;
	double ts;
	char *text;
 
 	while (TRUE) {
		while (ps_receive(eport, .1, &type, &ts, &text,
		    &ack_port)) {
 			if (type != ETYPE)
				ERRABORT("Invalid type");
 			if (ps_now < ts)
				ERRABORT("Invalid time stamp");
			if (text != ETEXT)
				ERRABORT("Invalid text");
			if (ack_port != EACK)
				ERRABORT("Invalid acknowledgement port");
		}
		ps_sleep (1.0);
 	}
 }

void sender_f (void * arg)
{
	char *text;

	while(TRUE) {
		text = ps_get_buffer(bpool);
		sprintf(text, "%g", ps_now);
		if (ps_send(fport, FTYPE, text, FACK) == SYSERR)
			ERRABORT("Send Failed");
		ps_sleep(ps_exponential(1.0));
	}
}

void receiver_f (void * arg) 
{
	long type, ack_port;
	double ts, time;
	char *text;

	while (TRUE) {
		ps_receive(fport, NEVER, &type, &ts, &text, &ack_port);
		sscanf (text, "%lg", &time);
		if (abs(time - ts) > .001)
			ERRABORT("Invalid time stamp");
		ps_sleep(ps_exponential(0.9));
		ps_resend(gport, type, ts, text, ack_port);
	}
}

void receiver_g (void * arg)
{
	long type, ack_port;
	double ts, time;
	char *text;

	while (TRUE) {
		ps_receive(gport, NEVER, &type, &ts, &text, &ack_port);
		if (type != FTYPE)
			ERRABORT("Invalid type");
		sscanf(text, "%lg", &time);
		ps_free_buffer(text);
		if (abs(ts - time) > .001)
			ERRABORT("ps_resend not preserving time stamp");
		if (ack_port != FACK)
			ERRABORT("Invalid acknowledgement port");
	}
}

