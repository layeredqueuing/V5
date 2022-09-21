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
#include "qnap2_gram.h"

extern char * dup_qnap2text( char * str, int len );
%}

D	[0-9]
E	[DEde][-+]?{D}+
C	[a-zA-Z]
U	[_]
S	[+-/*().,:;=<>"]

%%
%{
%}

"&"(.*)		;	/* Comment */
"<="			return QNAP_LESS_EQUAL;
">="			return QNAP_GREATER_EQUAL;
"**"			return QNAP_POWER;
":="	 		return QNAP_ASSIGNMEMT;

"/control/"		return QNAP_CONTROL;
"/declare/"		return QNAP_DECLARE;
"/end/"			return QNAP_END;
"/exec/"		return QNAP_EXEC;
"/restart/"		return QNAP_RESTART;
"/reboot/"		return QNAP_REBOOT;
"/station/"		return QNAP_STATION;
"/terminal/"		return QNAP_TERMINAL;

all			return QNAP_ALL;
and			return QNAP_AND;
any			return QNAP_ANY;
begin			return QNAP_BEGIN;
do			return QNAP_DO;
else			return QNAP_ELSE;
end			return QNAP_END;
false			return QNAP_FALSE;
for			return QNAP_FOR;
forward			return QNAP_FORWARD;
generic			return QNAP_GENERIC;
goto			return QNAP_GOTO;
if			return QNAP_IF;
in			return QNAP_IN;
init			return QNAP_INIT;
integer			return QNAP_INTEGER;
is			return QNAP_IS;
name			return QNAP_NAME;
nil			return QNAP_NIL;
not			return QNAP_NOT;
object			return QNAP_OBJECT;
or			return QNAP_OR;
prio			return QNAP_PRIO;
queue			return QNAP_QUEUE;
quantum			return QNAP_QUANTUM;
rate			return QNAP_RATE;
real			return QNAP_REAL;
ref			return QNAP_REF;
repeat			return QNAP_REPEAT;
sched			return QNAP_SCHED;
service			return QNAP_SERVICE;
step			return QNAP_STEP;
then			return QNAP_THEN;
transit			return QNAP_TRANSIT;
true			return QNAP_TRUE;
type			return QNAP_TYPE;
until			return QNAP_UNTIL;
var			return QNAP_VAR;
watched			return QNAP_WATCHED;
while			return QNAP_WHILE;
with			return QNAP_WITH;


{S}			return *qnap2text;
{D}+			{ return 0;
			}

{D}+"."{D}*({E})?	|
{D}*"."{D}+({E})?	|
{D}+{E}			{ return QNAP_CONSTANT; }

({C}|{U})({C}|{D}|{U})*	{ qnap2lval.aString = dup_qnap2text( &qnap2text[0], qnap2leng ); return QNAP_IDENTIFIER; }
%%
/*
 * Copy str to mallocated storage.
 */

char *
dup_qnap2text( char * str, int len )
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
