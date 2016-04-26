/* $Id: test07.c 9210 2010-02-24 18:59:24Z greg $ */
/************************************************************************/
/* test07.c:	PARASOL test program					*/
/*									*/
/* Created: 	07/06/95 (PRM)						*/
/*									*/
/* Description:	Tests semaphores					*/
/*									*/
/* Notes:								*/
/*									*/
/************************************************************************/
#include <parasol.h>
#include "test.h"

#define BUFSIZE 20

#define MUTEX0 0
#define COUNTER1 1
  
long shared0 = 1;
long shared1 = 5;
 
void ps_genesis(void * arg)
{
	void locker(void *);
	void critical(void *);
	long i;

	for (i = 0; i < 20; i++)
		ps_resume(ps_create("Locker", 0, ANY_HOST, locker, 1));

	ps_reset_semaphore(COUNTER1, 5);
	for (i = 0; i < 100; i++)
		ps_resume(ps_create("Other", 0, ANY_HOST, critical, 1));
		
	ps_suspend(ps_myself);
}
 

void locker(void * arg)
{
	while (TRUE) {
		ps_wait_semaphore(MUTEX0);
		if (shared0 != 1)
			ERRABORT("Mutual exclusion semaphores are not working");
		shared0--;
		ps_sleep (ps_exponential(1.0));
		shared0++;
		ps_signal_semaphore(MUTEX0);
	}
}

void critical(void * arg)
{
	while (TRUE) {
		ps_wait_semaphore(COUNTER1);
		shared1--;
		if (shared1 < 0 || shared1 > 5)
			ERRABORT("Counting Semaphores are not working");
		ps_sleep(ps_exponential(1.0));
		shared1++;
		ps_signal_semaphore(COUNTER1);
	}
}

