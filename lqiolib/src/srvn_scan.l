%{ /* -*- c -*- */
/************************************************************************/
/* Copyright the Real-Time and Distributed Systems Group,		*/
/* Department of Systems and Computer Engineering,			*/
/* Carleton University, Ottawa, Ontario, Canada. K1S 5B6		*/
/* 									*/
/* Novemeber 1990.							*/
/************************************************************************/

/*
 * $Id$
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include "srvn_input.h"
#include "srvn_gram.h"

extern YYSTYPE srvnlval;

#if	defined(__cplusplus)
#define	INPUT	srvninput
#else
#define	INPUT	input
#endif
#undef YY_INPUT

#define YY_INPUT(buf,result,max_size) \
	{ register int c = getc(srvnin); *buf = c; \
	result = c == EOF ? YY_NULL : 1; \
	if (c == '\n') LQIO_line_number++; }


#undef srvnwrap
#if	!defined(__cplusplus)
int srvnwrap();
#endif
unsigned LQIO_line_number = 1;

static char * dup_srvntext( char * str, int len );
static char * dup_comment( char * str, int len );
%}

D	[0-9]
E	[DEde][-+]?{D}+
C	[a-zA-Z]
U	[_]
V	[$]
K	[%]
S	[,:;+\-*/%^&\[\](){}=?]
P	[0123]?

%%
[ \n\t\r]		;

^"#pragma"[ \t](.)*	{
				srvn_pragma( &srvntext[0] );		/* Note... must copy string... */
			}

"#"(.)*                 ;

\"[^"]*			{
				if ( srvntext[srvnleng-1] == '\\' ) {
				    yymore();
				} else {
				    	char c = INPUT();
 					if ( c != '"' ) {
						unput( c );
					}
					srvnlval.aString = dup_comment( &srvntext[1], srvnleng-1 );
					return(TEXT);
				}
			}

-1			return END_LIST;

"->"                    return TRANSITION;
"!"			return TOK_LOGIC_NOT;
"!="			return TOK_NOT_EQUALS;
"&&"			return TOK_LOGIC_AND;
"<"			return TOK_LESS_THAN;
"<="			return TOK_LESS_EQUAL;
"=="			return TOK_EQUALS;
">"			return TOK_GREATER_THAN;
">="			return TOK_GREATER_EQUAL;
"||"			return TOK_LOGIC_OR;
"**"			return TOK_POWER;

{D}+			{
				srvnlval.anInt = strtoul( (const char *)&srvntext[0], 0, 10 );
				if ( errno == ERANGE ) {
				    errno = 0; /* Reset for next conversion */
				    srvnlval.aString = dup_comment( &srvntext[0], srvnleng-1 );
				    return RANGE_ERR;
				} else {
				    return INTEGER;
				}
			}

{D}+"."{D}*({E})?	|
{D}*"."{D}+({E})?	|
{D}+{E}			{
				srvnlval.aFloat = strtod( (const char *)&srvntext[0], 0 );
				if ( errno == ERANGE ) {
				    errno = 0;	/* Reset for next conversion */
				    srvnlval.aString = dup_comment( &srvntext[0], srvnleng-1 );
				    return RANGE_ERR;
				} else {
				    return FLOAT;
				}
			}


({C}|{U})({C}|{D}|{U})+	{ srvnlval.aString = dup_srvntext( &srvntext[0], srvnleng ); return SYMBOL; }

{V}"solver".*		{ srvnlval.aString = dup_srvntext( &srvntext[0], srvnleng ); return SOLVER; }
{V}({C}|{D}|{U})+	{ srvnlval.aString = dup_srvntext( &srvntext[0], srvnleng ); return VARIABLE; }

{K}u{P}			{ srvnlval.anInt = (int)(srvntext[2] - '0'); return KEY_UTILIZATION; }
{K}f			{ srvnlval.anInt = 0; return KEY_THROUGHPUT; }
{K}pu			{ srvnlval.anInt = 0; return KEY_PROCESSOR_UTILIZATION; }
{K}s{P}			{ srvnlval.anInt = (int)(srvntext[2] - '0'); return KEY_SERVICE_TIME; }
{K}v{P}			{ srvnlval.anInt = (int)(srvntext[2] - '0'); return KEY_VARIANCE; }
{K}fb			{ srvnlval.anInt = 0; return KEY_THROUGHPUT_BOUND; }
{K}pw{P}		{ srvnlval.anInt = (int)(srvntext[3] - '0'); return KEY_PROCESSOR_WAITING; }
{K}w{P}			{ srvnlval.anInt = (int)(srvntext[2] - '0'); return KEY_WAITING; }
{K}wv{P}		{ srvnlval.anInt = (int)(srvntext[2] - '0'); return KEY_WAITING_VARIANCE; }

({C}|{S})		return *srvntext;

.			{ srvnlval.aString = dup_srvntext( &srvntext[0], srvnleng ); return TEXT; }

%%

int
srvnwrap()
{
    return 1;
}

/*
 * Copy str to mallocated storage.
 */

static char *
dup_srvntext( char * str, int len )
{
    char * p = (char *)malloc( (unsigned)len+1 );

    if ( p ) {
	(void) strncpy( p, (char *)str, len );
	/* strip trailing whitespace */
	for ( ; len > 0 && isspace( p[len-1] ); len-- );
	p[len] = '\0';
    }
    return p;
}


/*
 * Copy str to mallocated storage.
 * Strip out '\' sequences.
 */

static char *
dup_comment( char * str, int len )
{
    char * p = (char *)malloc( (unsigned)len+1 );
    char * q = p;

    if ( p ) {
	while ( *str && len ) {
	    if ( *str != '\\' ) {
		*p++ = *str;
	    }
	    ++str;
	    --len;
	}
	*p = '\0';
    }
    return q;
}


