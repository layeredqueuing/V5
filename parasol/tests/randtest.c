/* $Id */
/************************************************************************/
/* randtest.c:	PARASOL test program					*/
/*									*/
/* Created: 	07/06/95 (PRM)						*/
/*									*/
/* Description:	Tests the randomness of ps_receive_random and buses	*/
/*		with the RAND discipline.				*/
/*									*/
/************************************************************************/

#include <parasol.h>
#include "test.h"

#define NTASKS 10
#define NREPS 500
#define EXPECTED (NREPS/NTASKS)
#define CRITICAL 16.919		/* 9 degrees of freedom at the 95% level*/
#define MAX_FAILURES (int)(NTASKS * 2 / 10)
 

long test_randomness(long data[NTASKS][NTASKS]);

long	port;

void ps_genesis (void * arg)
{
	long tid, i;
	void sender(void *);
	void receiver(void *);
 
	ps_resume(tid = ps_create("", 0, ANY_HOST, receiver, 1));
	port = ps_std_port(tid);
	for (i = 0; i < NTASKS; i++)
		ps_resume(ps_create("", 0, ANY_HOST, sender, 1));

	ps_suspend(ps_myself);	
}

void sender(void * arg)
{
	long 	mypos;

	mypos = ps_myself - 3;
	while(TRUE) {
		ps_send(port, mypos, "", NULL_PORT);
		ps_sleep(1.0);
	}
}

void receiver(void * arg)
{
        long type, ack_port;
	long count;
	long i, j;
	long data[NTASKS][NTASKS];
	double ts;
	char *text;

	ps_sleep(0.5);
	while(TRUE) {
		for (i = 0; i < NTASKS; i++)
			for (j = 0; j < NTASKS; j++)
				data[i][j] = 0;

		for (count = 0; count < NREPS; count++) {
			for (i = 0; i < NTASKS; i++) {
				if (ps_receive_random(ps_my_std_port, IMMEDIATE,
				    &type, &ts, &text, &ack_port) <= 0)
					ERRABORT("ps_receive failed");
				data[type][i]++;
			}
			ps_sleep(1.0);
		}

		if (!test_randomness(data)) {
			printf ("**** ps_receive_random Failed Randomness Test ****\n");
			for (i = 0; i < NTASKS; i++) {
				for (j = 0; j < NTASKS; j++)
					printf ("%3ld ", data[i][j]);
				printf ("\n");
			}
		}
		/*ps_kill(ps_myself);*/
	}
}

long test_randomness(long data[NTASKS][NTASKS])
{
	long 	i, j;
	double 	chi_square;
	long	temp;
	long 	nfailures = 0;

	for (i = 0; i < NTASKS; i++) {
		chi_square = 0.0;
		for (j = 0; j < NTASKS; j++) {
			temp = data[i][j] - EXPECTED;
			chi_square += (double)(temp * temp) / (double)EXPECTED;
		}
		if (chi_square > CRITICAL)
			nfailures++;
	}
	for (j = 0; j < NTASKS; j++) {
		chi_square = 0.0;
		for (i = 0; i < NTASKS; i++) {
			temp = data[i][j] - EXPECTED;
			chi_square += (double)(temp * temp) / (double)EXPECTED;
		}
		if (chi_square > CRITICAL)
			nfailures++;
	}
	return (nfailures <= MAX_FAILURES);
}

