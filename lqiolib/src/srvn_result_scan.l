/* -*- c -*- */
/************************************************************************/
/* Copyright the Real-Time and Distributed Systems Group,		*/
/* Department of Systems and Computer Engineering,			*/
/* Carleton University, Ottawa, Ontario, Canada. K1S 5B6		*/
/* 									*/
/* Novemeber 1990.							*/
/************************************************************************/

/*
 * $Id: srvn_result_scan.l 15906 2022-09-25 12:52:43Z greg $
 */
%option noyywrap yylineno

%{
#include <stdio.h>
#include <stdlib.h>
#include "common_io.h"
#include "srvn_results.h"
#include "srvn_result_gram.h"

extern YYSTYPE resultlval;
static double time_to_double( const char * time );
static char * dup_strip_text( char * src, int len );

#if	defined(__cplusplus)
#define	INPUT	resultinput
#else
#define	INPUT	input
#endif
%}

D	[0-9]
E	[DEde][-+]?{D}+
C	[a-zA-Z]
U	"_"
V	[$]
S	[:%=,]

%%

[ \n\t\r]		;

^"#VAR"[ \t](.)*        { add_output_pragma( &resulttext[0], resultleng); }
^"#!Real:"		return(REAL_TIME);
^"#!User:"		return(USER_TIME);
^"#!Sys:"		return(SYST_TIME);
^"#!MaxRSS":		return(MAX_RSS);
^"#!Solver:"		return(SOLVER);
^"#!Comment:"(.)*	{ resultlval.aString = dup_strip_text( (char *)&resulttext[10], resultleng-10 ); return COMMENT; }
"#"[^!](.)*             |
"#!"                    |
"#"                     ;

B			return(BOUND_FLAG);
C			return(CONV_FLAG);
D			return(DISTRIBUTION_FLAG);
DP			return(DROP_PROBABILITY_FLAG);
E			return(SERVICE_EXCEEDED_FLAG);
F			return(FWD_WAITING_FLAG);
FQ			return(THPT_UT_FLAG);
G			return(GROUP_FLAG);
H			return(HOLD_TIME_FLAG);
I			return(ITERATION_FLAG);
J			return(JOIN_FLAG);
L			return(RWLOCK_HOLD_TIME_FLAG);
NP			return(PHASE_COUNT_FLAG);
OT			return(OVERTAKING_FLAG);
P			return(PROC_FLAG);
PP			return(PROC_COUNT_FLAG);
R			return(OPEN_ARRIV_FLAG);
V			return(VALIDITY_FLAG);
VAR			return(VARIANCE_FLAG);
VARF			return(FWD_WAITING_VARIANCE_FLAG);
VARW			return(WAITING_VARIANCE_FLAG);
VARZ			return(SNR_WAITING_VARIANCE_FLAG);
W			return(WAITING_FLAG);
X			return(SERVICE_FLAG);
Z			return(SNR_WAITING_FLAG);
{S}			return *resulttext;			/* Special character */

{D}+			{ resultlval.anInt = atoi((char *)&resulttext[0]); return INTEGER; }

{D}+"."{D}*({E})?	|
{D}*"."{D}+({E})?	|
{D}+{E}			{ resultlval.aFloat = atof((char *)&resulttext[0]); return FLOAT; }
{D}+":"{D}{D}":"{D}{D}("."{D}{D}*)? { resultlval.aFloat = time_to_double((char *)&resulttext[0]); return TIME; }

-1			return ENDLIST;

{C}			{ resultlval.aChar = *resulttext; return CHAR; }
infinity		{ resultlval.aString = (char *)&resulttext[0]; return INFTY; }
[.iI][nN][fF]		{ resultlval.aString = (char *)&resulttext[0]; return INFTY; }
({U})*{C}({C}|{D}|{U})+	{ resultlval.aString = lqio_duplicate_string( &resulttext[0], resultleng ); return SYMBOL; }
{V}({C}|{D}|{U})+	{ resultlval.aString = lqio_duplicate_string( (char *)&resulttext[0], resultleng ); return VARIABLE; }
.			{ resultlval.aString = (char *)&resulttext[0]; return TEXT; }

%%
static double time_to_double( const char * time ) 
{
    unsigned long hrs   = 0;
    unsigned long mins  = 0;
    unsigned long secs  = 0;
    
    if ( sscanf( time, "%ld:%ld:%ld", &hrs, &mins, &secs ) > 0 ) {
	return hrs * 3600 + mins * 60 + secs;
    } else {
	return 0;
    }
}


static char * dup_strip_text( char * s, int len )
{
    while ( (*s == ' ' || *s == '\t') && len > 0 ) { ++s; --len; }	/* Skip leading white space */
    return lqio_duplicate_string( s, len );
}
