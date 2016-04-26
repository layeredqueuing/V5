/*
 * $Id: getopt2.h 9767 2010-08-26 02:51:21Z greg $
 *
 */

#ifndef SRVNIOLIB_GETOPT_H
#define SRVNIOLIB_GETOPT_H 1

extern char  optsign;		/* '-' || '+'. */

#if HAVE_CONFIG_H
#include <config.h>
#endif
#include <stdlib.h>
#if HAVE_GETOPT_H
#include <getopt.h>
#endif
	
int getopt2( int argc, char * const * argv, const char *optstr );
#if HAVE_GETOPT_H
int getopt2_long( int argc, char * const * argv, const char * optstr, const struct option * longopts, int * longindex );
#endif

#endif /* SRVNIOLIB_GETOPT_H */
