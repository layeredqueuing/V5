/************************************************************************/
/* Copyright the Real-Time and Distributed Systems Group,		*/
/* Department of Systems and Computer Engineering,			*/
/* Carleton University, Ottawa, Ontario, Canada. K1S 5B6		*/
/* 									*/
/* October 1991.							*/
/************************************************************************/

/*
 * $Id: getsbopt.h 9748 2010-08-18 19:38:02Z greg $
 * This exists only if getsubopt isn't found in cstdlib (like on windoze)
 */

#if	!defined(SRVNIOLIB_GETSUBOPT_H)
#define	SRVNIOLIB_GETSUBOPT_H

#if	defined(__cplusplus)
extern "C" {
#endif

/* getsubopt.c */

int getsubopt (char **optionp, char * const *, char **valuep);

#if	defined(__cplusplus)
}
#endif
#endif	/* Solaris */
