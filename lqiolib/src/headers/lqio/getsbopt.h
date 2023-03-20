/************************************************************************/
/* Copyright the Real-Time and Distributed Systems Group,		*/
/* Department of Systems and Computer Engineering,			*/
/* Carleton University, Ottawa, Ontario, Canada. K1S 5B6		*/
/* 									*/
/* October 1991.							*/
/************************************************************************/

/*
 * $Id: getsbopt.h 16546 2023-03-18 22:32:16Z greg $
 * This exists only if getsubopt isn't found in cstdlib (like on windoze)
 */

#ifndef LQIOLIB_GETSUBOPT_H
#define	LQIOLIB_GETSUBOPT_H

#if	defined(__cplusplus)
extern "C" {
#endif

/* getsubopt.c */

int getsubopt (char **optionp, char * const *, char **valuep);

#if	defined(__cplusplus)
}
#endif
#endif	/* Solaris */
