/* -*- c -*- */
/************************************************************************/
/* Copyright the Real-Time and Distributed Systems Group,		*/
/* Department of Systems and Computer Engineering,			*/
/* Carleton University, Ottawa, Ontario, Canada. K1S 5B6		*/
/* 									*/
/* Novemeber 1990.							*/
/************************************************************************/

/*
 * $Id: srvn_scan.l 15880 2022-09-21 12:52:01Z greg $
 */
%option noyywrap yylineno

%{
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

unsigned srvnlinenno = 1;
int srvnstart_token = 0;

extern char * dup_srvntext( char * str, int len );
static char * dup_comment( char * str, int len );
static int get_result_phase( char );
%}

D	[0-9]
E	[DEde][-+]?{D}+
C	[a-zA-Z]
U	[_]
V	[$]
X 	[@]
K	[%]
S	[,:;+\-*/%^&\[\](){}=?]
P	[0123]?

%%
%{
    /* Special code to force starting state of parser. */
    if ( srvnstart_token ) {
	int t = srvnstart_token;
	srvnstart_token = 0;
	return t;
    }
%}

[ \t\r\n]+		;

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

{V}solver.*		{ srvnlval.aString = dup_srvntext( &srvntext[0], srvnleng ); return SOLVER; }
{V}({C}|{D}|{U})+	{ srvnlval.aString = dup_srvntext( &srvntext[0], srvnleng ); return VARIABLE; }
{X}infinity		{ return CONST_INFINITY; }

{K}f			{ srvnlval.anInt = 0; return KEY_THROUGHPUT; }
{K}fb			{ srvnlval.anInt = 0; return KEY_THROUGHPUT_BOUND; }
{K}i			{ srvnlval.anInt = 0; return KEY_ITERATIONS; }
{K}pu			{ srvnlval.anInt = 0; return KEY_PROCESSOR_UTILIZATION; }
{K}pw{P}		{ srvnlval.anInt = get_result_phase( srvntext[3] ); return KEY_PROCESSOR_WAITING; }
{K}s{P}			{ srvnlval.anInt = get_result_phase( srvntext[2] ); return KEY_SERVICE_TIME; }
{K}sys			{ srvnlval.anInt = 0; return KEY_SYSTEM_TIME; }
{K}time			{ srvnlval.anInt = 0; return KEY_ELAPSED_TIME; }
{K}u{P}			{ srvnlval.anInt = get_result_phase( srvntext[2] ); return KEY_UTILIZATION; }
{K}usr			{ srvnlval.anInt = 0; return KEY_USER_TIME; }
{K}v{P}			{ srvnlval.anInt = get_result_phase( srvntext[2] ); return KEY_VARIANCE; }
{K}wv{P}		{ srvnlval.anInt = get_result_phase( srvntext[3] ); return KEY_WAITING_VARIANCE; }
{K}w{P}			{ srvnlval.anInt = get_result_phase( srvntext[2] ); return KEY_WAITING; }
{K}x{P}			{ srvnlval.anInt = get_result_phase( srvntext[2] ); return KEY_EXCEEDED_TIME; }

({C}|{S})		return *srvntext;

.			{ srvnlval.aString = dup_srvntext( &srvntext[0], srvnleng ); return TEXT; }

%%
/*
 * Copy str to mallocated storage.
 */

char *
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


static int
get_result_phase( char c )
{
    switch ( c ) {
    case '1': return 1;
    case '2': return 2;
    case '3': return 3;
    }
    return 0;
}

int srvnparse_string( int start_token, const char * s )
{
    extern int srvnparse();
    
    int rc = 0;
    srvnstart_token = start_token;
    YY_BUFFER_STATE bp = srvn_scan_string( s );
    rc = srvnparse();
    srvn_delete_buffer( bp );
    return rc;
}
