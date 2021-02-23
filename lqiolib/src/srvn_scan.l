/* -*- c -*- */
/************************************************************************/
/* Copyright the Real-Time and Distributed Systems Group,		*/
/* Department of Systems and Computer Engineering,			*/
/* Carleton University, Ottawa, Ontario, Canada. K1S 5B6		*/
/* 									*/
/* Novemeber 1990.							*/
/************************************************************************/

/*
 * $Id: srvn_scan.l 14480 2021-02-23 02:07:21Z greg $
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

extern YYSTYPE LQIO_lval;

#if	defined(__cplusplus)
#define	INPUT	LQIO_input
#else
#define	INPUT	input
#endif

unsigned LQIO_linenno = 1;
int srvn_start_token = 0;

extern char * dup_LQIO_text( char * str, int len );
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
    if ( srvn_start_token ) {
	int t = srvn_start_token;
	srvn_start_token = 0;
	return t;
    }
%}

[ \t\r\n]+		;

^"#pragma"[ \t](.)*	{
				srvn_pragma( &LQIO_text[0] );		/* Note... must copy string... */
			}

"#"(.)*                 ;

\"[^"]*			{
				if ( LQIO_text[LQIO_leng-1] == '\\' ) {
				    yymore();
				} else {
				    	char c = INPUT();
 					if ( c != '"' ) {
						unput( c );
					}
					LQIO_lval.aString = dup_comment( &LQIO_text[1], LQIO_leng-1 );
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
				LQIO_lval.anInt = strtoul( (const char *)&LQIO_text[0], 0, 10 );
				if ( errno == ERANGE ) {
				    errno = 0; /* Reset for next conversion */
				    LQIO_lval.aString = dup_comment( &LQIO_text[0], LQIO_leng-1 );
				    return RANGE_ERR;
				} else {
				    return INTEGER;
				}
			}

{D}+"."{D}*({E})?	|
{D}*"."{D}+({E})?	|
{D}+{E}			{
				LQIO_lval.aFloat = strtod( (const char *)&LQIO_text[0], 0 );
				if ( errno == ERANGE ) {
				    errno = 0;	/* Reset for next conversion */
				    LQIO_lval.aString = dup_comment( &LQIO_text[0], LQIO_leng-1 );
				    return RANGE_ERR;
				} else {
				    return FLOAT;
				}
			}


({C}|{U})({C}|{D}|{U})+	{ LQIO_lval.aString = dup_LQIO_text( &LQIO_text[0], LQIO_leng ); return SYMBOL; }

{V}solver.*		{ LQIO_lval.aString = dup_LQIO_text( &LQIO_text[0], LQIO_leng ); return SOLVER; }
{V}({C}|{D}|{U})+	{ LQIO_lval.aString = dup_LQIO_text( &LQIO_text[0], LQIO_leng ); return VARIABLE; }
{X}infinity		{ return CONST_INFINITY; }

{K}f			{ LQIO_lval.anInt = 0; return KEY_THROUGHPUT; }
{K}fb			{ LQIO_lval.anInt = 0; return KEY_THROUGHPUT_BOUND; }
{K}i			{ LQIO_lval.anInt = 0; return KEY_ITERATIONS; }
{K}pu			{ LQIO_lval.anInt = 0; return KEY_PROCESSOR_UTILIZATION; }
{K}pw{P}		{ LQIO_lval.anInt = get_result_phase( LQIO_text[3] ); return KEY_PROCESSOR_WAITING; }
{K}s{P}			{ LQIO_lval.anInt = get_result_phase( LQIO_text[2] ); return KEY_SERVICE_TIME; }
{K}sys			{ LQIO_lval.anInt = 0; return KEY_SYSTEM_TIME; }
{K}time			{ LQIO_lval.anInt = 0; return KEY_ELAPSED_TIME; }
{K}u{P}			{ LQIO_lval.anInt = get_result_phase( LQIO_text[2] ); return KEY_UTILIZATION; }
{K}usr			{ LQIO_lval.anInt = 0; return KEY_USER_TIME; }
{K}v{P}			{ LQIO_lval.anInt = get_result_phase( LQIO_text[2] ); return KEY_VARIANCE; }
{K}wv{P}		{ LQIO_lval.anInt = get_result_phase( LQIO_text[3] ); return KEY_WAITING_VARIANCE; }
{K}w{P}			{ LQIO_lval.anInt = get_result_phase( LQIO_text[2] ); return KEY_WAITING; }
{K}x{P}			{ LQIO_lval.anInt = get_result_phase( LQIO_text[2] ); return KEY_EXCEEDED_TIME; }

({C}|{S})		return *LQIO_text;

.			{ LQIO_lval.aString = dup_LQIO_text( &LQIO_text[0], LQIO_leng ); return TEXT; }

%%
/*
 * Copy str to mallocated storage.
 */

char *
dup_LQIO_text( char * str, int len )
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

int LQIO_parse_string( int start_token, const char * s )
{
    extern int LQIO_parse();
    
    int rc = 0;
    srvn_start_token = start_token;
    YY_BUFFER_STATE bp = LQIO__scan_string( s );
    rc = LQIO_parse();
    LQIO__delete_buffer( bp );
    return rc;
}
