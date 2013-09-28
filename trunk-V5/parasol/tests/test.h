/* @(#)test.h	1.1 12:15:52 8/22/95 */
#ifndef __TEST_H
#define __TEST_H

#include <stdlib.h>

/* We use ERRABORT whenever we encounter an error, and we want ot abort	*/
/* with a message to the user					 	*/

#define ERRABORT(msg){ ERRREPORT(msg); exit(1); }

/* We use ERRREPORT when we encounter and error and we want to report 	*/
/* it to the user without aborting					*/

#if (defined __FILE__) && (defined __LINE__)
#define ERRREPORT(msg)\
fprintf(stderr,"file: %s line: %d - %s\n", __FILE__, __LINE__, msg)
#else
#define ERRREPORT(msg)\
fprintf(stderr,"%s\n", msg)
#endif /*(defined __FILE__) && (defined __LINE__)*/


/* TIME_TOLERANCE is the amount of floating point error we allow in	*/
/* time calculations.							*/

#define TIME_TOLERANCE 1.0e-4

/* BIG_TIME should be larger than any simulation duration, and should	*/
/* be used instead of HUGE_VAL.						*/

#define BIG_TIME 1.0e9

/* define abs if it's not already defined because it's handy for 	*/
/* comparing times.							*/

#ifndef abs
#define abs(x) (((x) < 0) ? -(x) : (x))
#endif /*abs*/

#endif /*__TEST_H*/
