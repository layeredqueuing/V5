/* -*- C++ -*- */
/************************************************************************/
/* Copyright the Real-Time and Distributed Systems Group,		*/
/* Department of Systems and Computer Engineering,			*/
/* Carleton University, Ottawa, Ontario, Canada. K1S 5B6		*/
/* 									*/
/* March 2013.								*/
/************************************************************************/

/*
 * $Id: error.h 11259 2013-01-24 15:36:05Z greg $
 */

#if	!defined(SRVNIO_PARSEABLE_H)
#define	SRVNIO_PARSEABLE_H

#include <cstdio>
#include "commandline.h"

void print_parseable( FILE * output );

extern LQIO::CommandLine command_line;

#endif /* SRVNIO_PARSEABLE_H */
