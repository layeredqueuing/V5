/* help.h	-- Greg Franks
 *
 * $Id: help.h 15170 2021-12-07 23:33:05Z greg $
 */

#ifndef _HELP_H
#define _HELP_H

void usage( const bool = true );
void invalid_option( char c, char * optarg );
void man();
#endif
