/* help.h	-- Greg Franks
 *
 * $Id: help.h 12412 2016-01-06 17:56:04Z greg $
 */

#ifndef _HELP_H
#define _HELP_H

void invalid_option( int );
void invalid_argument( int, const std::string&, const std::string& s="" );
void usage();
void man();
void help();

#endif
