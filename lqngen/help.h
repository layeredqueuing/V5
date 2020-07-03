/* help.h	-- Greg Franks
 *
 * $Id: help.h 13477 2020-02-08 23:14:37Z greg $
 */

#ifndef _HELP_H
#define _HELP_H

void invalid_option( int );
void invalid_argument( int, const std::string&, const std::string& s="" );
void usage();
void man();
void help();

#endif
