/* $Id: test05.c 9210 2010-02-24 18:59:24Z greg $ */
/************************************************************************/
/* test04.c:	PARASOL test program					*/
/*									*/
/* Created: 	07/06/95 (PRM)						*/
/*									*/
/* Description:	Tests statistics functions				*/
/*									*/
/* Notes:	Just make sure the expected values match closely with	*/
/*		the observed values.					*/
/*									*/
/************************************************************************/
#include <parasol.h>
#include "test.h"

#define NSAMPLES 10000
#define MEAN 223.5
 
void ps_genesis(void * arg)
{
	double mean, other;
	long i, stat1, stat2, stat3;

	stat1 = ps_open_stat("Stat 1", SAMPLE);
	for (i = 0; i < NSAMPLES; i++)
		ps_record_stat(stat1, ps_exponential(MEAN));
	ps_get_stat(stat1, &mean, &other);
	printf ("\nExpected Mean = %g, Observed Mean = %g, #obs = %g\n", MEAN, 
	    mean, other);

	stat2 = ps_open_stat("Stat 2", VARIABLE);
	ps_record_stat (stat2, 1.0);
	ps_sleep(1.0);
	ps_record_stat (stat2, 0.0);
	ps_sleep(.5);
	ps_get_stat(stat2, &mean, &other);
	printf ("\nExpected Value = %g, Observed Value = %g, #obs = %g\n", 
	    (1.0*1+0.0*.5)/1.5, mean, other);

	stat3 = ps_open_stat("Rate Stat", RATE);
	while (TRUE) {
		ps_sleep(0.5);
		ps_record_rate_stat(stat3);
	}
}

