/* help.h	-- Greg Franks
 *
 * $Id: help.h 13477 2020-02-08 23:14:37Z greg $
 */

#ifndef _HELP_H
#define _HELP_H

void usage( const bool = true );
void invalid_option( char c, char * optarg );
void man();

#endif
