/* help.h	-- Greg Franks
 *
 * $Id: help.h 11963 2014-04-10 14:36:42Z greg $
 */

#ifndef _HELP_H
#define _HELP_H

void usage( const bool = true );
void invalid_option( char c, char * optarg );
void man();

#endif
