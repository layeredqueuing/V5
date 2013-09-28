/* $Id$ */
/************************************************************************/
/* test08.c:	PARASOL test program					*/
/*									*/
/* Created: 	07/06/95 (PRM)						*/
/*									*/
/* Description:	Tests ps_pass_port when passing empty ports, ports with	*/
/*		messages, and ports with someone blocked on them	*/
/*									*/
/*									*/
/************************************************************************/
#include <parasol.h>
#include "test.h"

#define NMESS 20
#define ATYPE 1
#define CTYPE 4
#define ATEXT ((char*)0x3b875)
#define CTEXT ((char*)0x83827)
#define AACK NULL_PORT
#define CACK NULL_PORT
 

long aport, cport;
long nmessages;
long ta, tb, tc, td;

void ps_genesis(void * arg)
{
	long	i;
	void	receiver_a(void *);
	void	receiver_b(void *);
	void	receiver_c(void *);
	void	receiver_d(void *);

	ps_resume (ta = ps_create("", 0, ANY_HOST, receiver_a, 1));
	ps_resume (tb = ps_create("", 0, ANY_HOST, receiver_b, 1));
	ps_resume (tc = ps_create("", 0, ANY_HOST, receiver_c, 1));
	ps_resume (td = ps_create("", 0, ANY_HOST, receiver_d, 1));

	aport = ps_allocate_port("", ta);
	cport = ps_allocate_port("", tc);
	
	ps_sleep(1.0);
	ps_pass_port(cport, td);
	ps_send(cport, CTYPE, CTEXT, CACK);

	while(TRUE) {
		for (i = 0; i < NMESS; i++)
			ps_send(aport, ATYPE, ATEXT, AACK);
		ps_sleep(1.0);
	}
}

void receiver_a(void * arg)
{
	long	type, ack_port, count, i;
	double 	ts;
	char	*text;

 	while(TRUE) {
		count = ps_choice(NMESS)+1;
		for (i = 0; i < count; i++) {
			ps_receive(aport, NEVER, &type, &ts, &text, &ack_port);
			if (type != ATYPE)
				ERRABORT("Invalid Type");
			if (text != ATEXT)
				ERRABORT("Invalid Text");
			if (ack_port != AACK)
				ERRABORT("Invalid ack port");
		}
		nmessages = NMESS - count;
		ps_pass_port(aport, tb);
		while (ps_owner(aport) != ps_myself)
			ps_sleep(.1);
	}
} 

void receiver_b(void * arg)
{
	long	type, ack_port, i;
	double 	ts;
	char	*text;

 	while(TRUE) {
		while (ps_owner(aport) != ps_myself)
			ps_sleep(.1);
		for (i = 0; i < nmessages; i++) {
			if (ps_receive(aport, IMMEDIATE, &type, &ts, &text, 
			    &ack_port) < 1)
				ERRABORT("ps_receive failed");
			if (type != ATYPE)
				ERRABORT("Invalid Type");
			if (text != ATEXT)
				ERRABORT("Invalid Text");
			if (ack_port != AACK)
				ERRABORT("Invalid ack port");
		}
 		ps_pass_port(aport, ta);
	}
} 

void receiver_c (void * arg)
{
	long	type, ack_port;
	double 	ts;
	char	*text;

	if (ps_receive(cport, NEVER, &type, &ts, &text, &ack_port) != SYSERR)
		ERRREPORT("Passing a block-on port not working");
}

void receiver_d (void * arg)
{
	long	type, ack_port;
	double 	ts;
	char	*text;

	while (ps_owner(cport) != ps_myself)
		ps_sleep(.1);
	if (ps_receive(cport, NEVER, &type, &ts, &text, &ack_port) == SYSERR)
		ERRABORT("ps_receive failed");
	if (type != CTYPE)
		ERRABORT("Invalid Type");
	if (text != CTEXT)
		ERRABORT("Invalid Text");
	if (ack_port != CACK)
		ERRABORT("Invalid ack port");
}

