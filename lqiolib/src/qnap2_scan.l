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
#include "qnap2_gram.h"
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
"<="		return QNAP_LESS_EQUAL;
">="		return QNAP_GREATER_EQUAL;
"**"		return QNAP_POWER;
":="	 	return QNAP_ASSIGNMEMT;

"/control/"	return QNAP_CONTROL;
"/declare/"	return QNAP_DECLARE;
"/end/"		return QNAP_END;
"/exec/"	return QNAP_EXEC;
"/restart/"	return QNAP_RESTART;
"/reboot/"	return QNAP_REBOOT;
"/station/"	return QNAP_STATION;
"/terminal/"	return QNAP_TERMINAL;

all		return QNAP_ALL;
and		return QNAP_AND;
any		return QNAP_ANY;
begin		return QNAP_BEGIN;
do		return QNAP_DO;
end		return QNAP_END;
false		return QNAP_FALSE;
for		return QNAP_FOR;
forward		return QNAP_FORWARD;
generic		return QNAP_GENERIC;
goto		return QNAP_GOTO;
if		return QNAP_IF;
in		return QNAP_IN;
init		return QNAP_INIT;
integer		return QNAP_INTEGER;
is		return QNAP_IS;
name		return QNAP_NAME;
nil		return QNAP_NIL;
not		return QNAP_NOT;
object		return QNAP_OBJECT;
or		return QNAP_OR;
prio		return QNAP_PRIO;
quantum		return QNAP_QUANTUM;
rate		return QNAP_RATE;
real		return QNAP_REAL;
ref		return QNAP_REF;
repeat		return QNAP_REPEAT;
sched		return QNAP_SCHED;
se		return QNAP_ELSE;
service		return QNAP_SERVICE;
step		return QNAP_STEP;
then		return QNAP_THEN;
transit		return QNAP_TRANSIT;
true		return QNAP_TRUE;
type		return QNAP_TYPE;
until		return QNAP_UNTIL;
var		return QNAP_VAR;
watched		return QNAP_WATCHED;
while		return QNAP_WHILE;
with		return QNAP_WITH;


{D}+			{ return 0;
			}

{D}+"."{D}*({E})?	|
{D}*"."{D}+({E})?	|
{D}+{E}			{ return QNAP_CONSTANT; }

({C}|{U})({C}|{D}|{U})*	{ return QNAP_IDENTIFIER; }
%%
