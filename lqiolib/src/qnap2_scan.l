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
#include "common_io.h"
    
extern YYSTYPE qnap2lval;

#if	defined(__cplusplus)
#define	INPUT	qnap2input
#else
#define	INPUT	input
#endif

static int return_code( int code );
%}

D	[0-9]
E	[DEde][-+]?{D}+
C	[a-zA-Z]
U	[_]
S	[().,:;"]

%%
%{
%}

[ \t\r\n]+		;

"&"(.*)			;	/* Comment */

"<="			return return_code( QNAP_LESS_EQUAL );
">="			return return_code( QNAP_GREATER_EQUAL );
"<>"			return return_code( QNAP_NOT_EQUAL );
"<"			return return_code( QNAP_LESS );
">"			return return_code( QNAP_GREATER );
"="			return return_code( QNAP_EQUAL );
":="	 		return return_code( QNAP_ASSIGNMENT );
"+"			return return_code( QNAP_PLUS );
"-"			return return_code( QNAP_MINUS );
"*"			return return_code( QNAP_MULTIPLY );
"/"			return return_code( QNAP_DIVIDE );
"%"			return return_code( QNAP_MODULUS );
"**"			return return_code( QNAP_POWER );

\"[^"]*			{
    			    if ( qnap2text[qnap2leng-1] == '\\' ) {
				yymore();
			    } else {
				char c = INPUT();
				if ( c != '"' ) {
				    unput( c );
				}
				qnap2lval.aString = lqio_duplicate_comment( &qnap2text[1], qnap2leng-1 );
				return STRING;
			    }
			}

"/control/"		return QNAP_CONTROL;
"/declare/"		return QNAP_DECLARE;
"/end/"			return QNAP_END_PROGRAM;
"/exec/"		return QNAP_EXEC;
"/restart/"		return QNAP_RESTART;
"/reboot/"		return QNAP_REBOOT;
"/station/"		return QNAP_STATION;
"/terminal/"		return QNAP_TERMINAL;

all			return QNAP_ALL;	/* Reserved words */
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
is			return QNAP_IS;
nil			return QNAP_NIL;
not			return QNAP_NOT;
object			return QNAP_OBJECT;
or			return QNAP_OR;
ref			return return_code( QNAP_REF );
repeat			return QNAP_REPEAT;
step			return QNAP_STEP;
then			return QNAP_THEN;
true			return QNAP_TRUE;
until			return QNAP_UNTIL;
var			return QNAP_VAR;
watched			return QNAP_WATCHED;
while			return QNAP_WHILE;
with			return QNAP_WITH;

boolean			return return_code( QNAP_BOOLEAN );
class			return return_code( QNAP_CLASS );	/* Other */
cst			return return_code( QNAP_CST );
entry			return return_code( QNAP_ENTRY );
erlang			return return_code( QNAP_ERLANG );
exit			return return_code( QNAP_EXIT );
exp			return return_code( QNAP_EXP );
hexp			return return_code( QNAP_HEXP );
infinite 		return return_code( QNAP_INFINITE );
init			return QNAP_INIT;
integer			return return_code( QNAP_INTEGER );
multiple 		return return_code( QNAP_MULTIPLE );
name			return QNAP_NAME;
option			return return_code( QNAP_OPTION );
prio			return QNAP_PRIO;
quantum			return QNAP_QUANTUM;
queue			return return_code( QNAP_QUEUE );
rate			return QNAP_RATE;
real			return return_code( QNAP_REAL );
sched			return QNAP_SCHED;
server 			return return_code( QNAP_SERVER );
service			return QNAP_SERVICE;
single 			return return_code( QNAP_SINGLE );
source 			return return_code( QNAP_SOURCE );
string			return return_code( QNAP_STRING );
transit			return QNAP_TRANSIT;
type			return QNAP_TYPE;

{S}			return *qnap2text;
{D}+			{
			    qnap2lval.aLong = strtoul( (const char *)&qnap2text[0], 0, 10 );
			    if ( errno == ERANGE ) {
				errno = 0; /* Reset for next conversion */
				qnap2lval.aString = lqio_duplicate_string( &qnap2text[0], qnap2leng-1 );
				return RANGE_ERR;
			    } else {
				return LONG;
			    }
			}

{D}+"."{D}*({E})?	|
{D}*"."{D}+({E})?	|
{D}+{E}			{
			    qnap2lval.aReal = strtod( (const char *)&qnap2text[0], 0 );
			    if ( errno == ERANGE ) {
				errno = 0;	/* Reset for next conversion */
				qnap2lval.aString = lqio_duplicate_string( &qnap2text[0], qnap2leng-1 );
				return RANGE_ERR;
			    } else {
				return DOUBLE;
			    }
			}

({C}|{U})({C}|{D}|{U})*	{ qnap2lval.aString = lqio_duplicate_string( &qnap2text[0], qnap2leng ); return IDENTIFIER; }
%%
static int return_code( int code )
{
    qnap2lval.aCode = code;
    return code;
}
