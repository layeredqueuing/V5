/************************************************************************/
/* Copyright the Real-Time and Distributed Systems Group,		*/
/* Department of Systems and Computer Engineering,			*/
/* Carleton University, Ottawa, Ontario, Canada. K1S 5B6		*/
/* 									*/
/* August 1991.								*/
/************************************************************************/

/*
 * Sub option processing.  Like Sun's getsubopt except that it will
 * match substrings.  Besides the latter isn't found on all machine
 * types.  This is a C function.  
 *
 * Written by Greg Franks.  August, 1991.
 *
 * $Id: getsbopt.cc 10091 2010-12-07 15:18:56Z greg $
 */

#if defined(HAVE_CONFIG_H)
#include <config.h>
#endif
#if defined(__cplusplus) && defined(inline)
#undef inline
#endif

#if !defined(HAVE_GETSUBOPT) 
#include "getsbopt.h"
#include <string.h>
#include <ctype.h>

int 
getsubopt (char **optionp, char * const * tokens, char **valuep)
{
  char * p = *optionp;
  char * q;
  char * r;
  int i;

  *valuep = 0;
  if ( !p ) return -1;
		
  while ( isspace( *p ) ) ++p;		/* Skip blanks.		*/
  q = *optionp;

  /* Look for '-', ',' or null. */
		
  for ( ; *p && *p != '=' && *p != ','; ++p );
  if ( *p == '=' ) {
    for ( r = p - 1; r >= *optionp && isspace( (int)*r ); --r ) {
      *r = (char)0;		/* truncate blanks	*/
    }

    *p++ = (char)0;			/* Zap '='		*/
    while ( isspace( *p ) ) ++p;	/* Skip blanks.		*/
    *valuep = p;
			
    for ( ; *p && *p != ','; ++p );	/* Find ','		*/
			
    for ( r = p - 1; r >= *valuep && isspace( (int)*r ); --r ) {
      *r = (char)0;		/* truncate blanks	*/
    }
  } else {
    for ( r = p - 1; r >= *optionp && isspace( (int)*r ); --r ) {
      *r = (char)0;		/* truncate blanks	*/
    }
    *valuep = 0;
  }

		
  /* Skip past ',' and set optionp for scanning next option. */
		
  if ( *p ) {
    *p++ = (char)0;			/* Zap comma...		*/
    while ( isspace( *p ) ) ++p;	/* Skip white space	*/
  }
  *optionp = p;				/* Point to next char.	*/

  /* Search for option string in list of options */
		
  for ( i = 0; tokens[i]; ++i ) {
    if ( strcmp( tokens[i], q ) == 0 ) return i;
  }

  /* No match -- return string as value. */
		
  *valuep = q;
  return -1;
}

#endif
