/* $Id$ */
/************************************************************************/
/* test01.c:	PARASOL test program					*/
/*									*/
/* Created: 	07/06/95 (PRM)						*/
/*									*/
/* Description:	Tests the following functionality of links		*/
/*		- Messages are transmitted at correct rate		*/
/*		- Messages types, text and ack ports are sent correctly	*/
/*		- Link Usage statistics work correctly			*/
/*									*/
/* Notes:	Usage of link a should be around 95%			*/
/*		Usage of link b should be exactly 50%			*/
/*									*/
/************************************************************************/

#include <parasol.h>
#include "test.h"

#define ARATE 100
#define BRATE 1000

#define A1TYPE 1
#define A2TYPE 2
#define BTYPE 3
#define A1TEXT ((char*)0x3b875)
#define A2TEXT ((char*)0x11019)
#define BTEXT ((char*)0x20392)
#define A1ACK NULL_PORT
#define A2ACK NULL_PORT
#define BACK NULL_PORT

long linka, linkb;
long a1_port;
long a2_port;
long b_port;
 
void ps_genesis (void * arg)
{
	void sender_a1(void *);
	void sender_a2(void *);
	void sender_b(void *);
	void receiver_a1(void *);
	void receiver_a2(void *);
	void receiver_b(void *);
	long src, dest, tid;
	
	src = ps_build_node("Source Node", 1, 1.0, 0.0, FCFS, FALSE);
	dest = ps_build_node("Destination Node", 1, 1.0, 0.0, FCFS, FALSE);
	linka = ps_build_link ("Link A", src, dest, ARATE, TRUE);
	linkb = ps_build_link ("Link B", src, dest, BRATE, TRUE);
	ps_resume(ps_create("Sender A1", src, ANY_HOST, sender_a1, 1));
	ps_resume(ps_create("Sender A2", src, ANY_HOST, sender_a2, 1));
	ps_resume(ps_create("Sender B", src, ANY_HOST, sender_b, 1));
	ps_resume(tid=ps_create("Receiver A1", dest, ANY_HOST, receiver_a1, 1));
	a1_port = ps_std_port(tid);
	ps_resume(tid=ps_create("Receiver A2", dest, ANY_HOST, receiver_a2, 1));
	a2_port = ps_std_port(tid);
	ps_resume(tid=ps_create("Receiver B", dest, ANY_HOST, receiver_b, 1));
	b_port = ps_std_port(tid);

	ps_suspend(ps_myself);
}

void sender_a1(void * arg)
{
	while (TRUE) {
		if (ps_link_send(linka, a1_port, A1TYPE, ARATE, A1TEXT, A1ACK)
		    == SYSERR)
			ERRABORT("Error on link send");
		ps_sleep(ps_exponential(2.0));
	}
}

void sender_a2(void * arg)
{
	while (TRUE) {
		if (ps_link_send(linka, a2_port, A2TYPE, ARATE, A2TEXT, A2ACK)
		    == SYSERR)
			ERRABORT("Error on link send");
		ps_sleep(ps_exponential(2.0));
	}
}

void sender_b(void * arg)
{
	while (TRUE) {
		if (ps_link_send(linkb, b_port, BTYPE, BRATE, BTEXT, BACK)
		    == SYSERR)
			ERRABORT("Error on link send");
		ps_sleep(2.0);
	}
}

void receiver_a1(void * arg)
{
	long type, ack_port;
	double ts;
	char *text;

	while (TRUE) {
		ps_receive(a1_port, NEVER, &type, &ts, &text, &ack_port);
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
		ps_receive(a2_port, NEVER, &type, &ts, &text, &ack_port);
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
		ps_receive(b_port, NEVER, &type, &ts, &text, &ack_port);
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
