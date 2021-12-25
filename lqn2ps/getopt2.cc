/*
 * Copyright (c) 1987 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#if defined(LIBC_SCCS) && !defined(lint)
/* static char sccsid[] = "@(#)getopt.c	4.13 (Berkeley) 2/23/91"; */
static char RcsId[] = "$Header$";
#endif /* LIBC_SCCS and not lint */

#if HAVE_CONFIG_H
#include <config.h>
#endif
#include <cstdio>
#include <cstdlib>
#include <cstring>
#if defined(HAVE_UNISTD_H) || defined(nt)
#include <unistd.h>
#endif
#include "getopt2.h"

extern "C" char *optarg;
extern "C" int optind, opterr, optopt;
char    optsign;		/* '-' || '+'. */

#define	BADCH	(int)'?'
#define	EMSG	""

int
getopt2( int nargc, char * const * nargv, const char *ostr )
{
    return getopt2_long( nargc, nargv, ostr, 0, 0 );
}

int
getopt2_long( int nargc, char * const * nargv, const char *ostr, const struct option * longopts, int * longindex )
{
    static const char *place = EMSG;	/* option letter processing */
    char *oli;		/* option letter list index */
    char *p;
    if (!(p = strrchr(*nargv, '/')))
	p = *nargv;
    else
	++p;

    if (*place == '\0') {				/* update scanning pointer */
	if (optind >= nargc || (*(place = nargv[optind]) != '-' && *place != '+' ) ) {
	    place = EMSG;
	    return EOF;
	}
	optsign = *place;
	if ( place[1] == '-' ) {
	    if (!place[2] || place[2] == ' ' || place[2] == '\t' || !longopts) {	/* found "--" */
		++optind;
		place = EMSG;
		return EOF;
	    } else {
		unsigned count = 0;
		int has_arg = no_argument;
		char * q = const_cast<char *>(strchr(&place[2],'='));
		if ( q ) *q++ = '\0';		/* Point to =arg... */
		else q = nargv[optind+1];
		const unsigned len = strlen( &place[2] );
		for ( const struct option *o = longopts; o->name || o->val; ++o ) {
		    if ( strncmp( &place[2], o->name, len ) == 0 ) {
			count += 1;
			if ( (o->val & 0xff00) == 0x0100 ) {
			    optopt = o->val & 0x00ff;
			    optsign = '+';		/* Flag '+' version of opt. */
			} else {
			    optopt = o->val;
			}
			has_arg = o->has_arg;
		    }
		}
		if ( count == 0 ) {
		    if ( opterr ) {
			(void)fprintf(stderr, "%s: illegal option -- %s\n", p, &place[2]);
		    }
		    return BADCH;
		} else if ( count > 1 ) {
		    (void)fprintf(stderr, "%s: ambiguous option -- %s\n", p, &place[2]);
		    return BADCH;
		} else {
		    /* Need optarg eventually */
		    place = EMSG;
		    if ( has_arg != no_argument ) {
			optarg = q;
			if ( q == nargv[optind] ) ++optind;
		    } else {
			optarg = NULL;
		    }
		    ++optind;
		    return optopt;
		}
	    }
	} else {
	    ++place;
	}
    }					/* option letter okay? */
    if ((optopt = (int)*place++) == (int)':' ||
	!(oli = strchr(const_cast<char *>(ostr), optopt))) {
	/*
	 * if the user didn't specify '-' as an option,
	 * assume it means EOF.
	 */
	if (optopt == (int)'-')
	    return EOF;
	if (!*place)
	    ++optind;
	if (opterr) {
	    (void)fprintf(stderr, "%s: illegal option -- %c\n", p, optopt);
	}
	return BADCH;
    }
    if (*++oli != ':') {			/* don't need argument */
	optarg = NULL;
	if (!*place)
	    ++optind;
    }
    else {					/* need an argument */
	if (*place)			/* no white space */
	    optarg = const_cast<char *>(place);
	else if (nargc <= ++optind) {	/* no arg */
	    place = EMSG;
	    if (opterr) {
		(void)fprintf(stderr, "%s: option requires an argument -- %c\n", p, optopt);
	    }
	    return BADCH;
	}
	else				/* white space */
	    optarg = nargv[optind];
	place = EMSG;
	++optind;
    }
    return optopt;				/* dump back option letter */
}
