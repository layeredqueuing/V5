/*
 *  $Id: alloc.c 10972 2012-06-19 01:12:22Z greg $
 *  Prentiss Riddle
 *
 *  Front-ends for the Unix(tm) malloc routines, with rudimentary error
 *  checking, an droutines to free the data structures once they are allocated.
 *
 */

#include <../config.h>
#include <stdio.h>
#include <stdlib.h>
#include "const.h"
#include "global.h"

/*
 *  Malloc with error checking.
 */

char *
emalloc( unsigned nbytes )		       /* no. of bytes of storage requested */
{
	char           *mallptr;       /* pointer returned by malloc */

	if ((mallptr = malloc(nbytes)) == NULL) {
		fprintf(stderr, "emalloc: couldn't fill request for %d bytes\n",
			nbytes);
		exit(2);
	}
	return (mallptr);
}


#if 0
/*
 *  Realloc with error checking.
 */

char *
erealloc(pointer, nbytes)
char           *pointer;	       /* pointer of block to be reallocated */
unsigned        nbytes;		       /* no. of bytes of storage requested */
{
	char           *reallptrr;     /* pointer returned by malloc */

	if ((reallptrr = realloc(pointer, nbytes)) == NULL) {
		fprintf(stderr, "erealloc: couldn't fill request for %d bytes\n",
			nbytes);
		exit(2);
	}
	return (reallptrr);
}
#endif
