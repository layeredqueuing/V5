%{ /* -*- c -*- */
/************************************************************************/
/* Copyright the Real-Time and Distributed Systems Group,		*/
/* Department of Systems and Computer Engineering,			*/
/* Carleton University, Ottawa, Ontario, Canada. K1S 5B6		*/
/* 									*/
/* Novemeber 1990.							*/
/************************************************************************/

/*
 * $Id: srvn_result_scan.l 13204 2018-03-06 22:52:04Z greg $
 */

/* 
 * This file has been modified to use xx rather than yy as the 
 * designator for global variables.  A sed command of the form
 * "sed -e /s/yy/xx/g" must be applied to the lex.yy.c file
 * before compilation in order to be compatible.  Note that the
 * same change must be made to the yacc files as well.
 */

#include <stdio.h>
#include <stdlib.h>
#include "srvn_results.h"
#include "srvn_result_gram.h"

/* extern int atoi(); */
/* extern double atof(); */
extern YYSTYPE resultlval;

#undef YY_INPUT
#define YY_INPUT(buf,result,max_size) { \
	int c = getc(resultin); *buf =c; \
	result = c == EOF ? YY_NULL : 1; \
	if (c == '\n') resultlinenumber++; }

#undef resultwrap
#if	!defined(__cplusplus)
extern int resultwrap();
#endif

%}

D	[0-9]
E	[DEde][-+]?{D}+
C	[a-zA-Z]
U	"_"

%%

[ \n\t\r]		;

^"#VAR"[ \t](.)*        {add_output_pragma( &resulttext[0], resultleng);}
^"#!Real:"		return(REAL_TIME);
^"#!User:"		return(USER_TIME);
^"#!Sys:"		return(SYST_TIME);
^"#!MaxRSS":		return(MAX_RSS);

^"#!Solver:"		return(SOLVER);
"#"[^!](.)*             |
"#!"                    |
"#"                     ;

B			return(BOUND_FLAG);
C			return(CONV_FLAG);
D			return(DISTRIBUTION_FLAG);
DP			return(DROP_PROBABILITY_FLAG);
E			return(SERVICE_EXCEEDED_FLAG);
FQ			return(THPT_UT_FLAG);
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
VARW			return(WAITING_VARIANCE_FLAG);
VARZ			return(SNR_WAITING_VARIANCE_FLAG);
W			return(WAITING_FLAG);
X			return(SERVICE_FLAG);
Z			return(SNR_WAITING_FLAG);
%			return(CONF_INT_FLAG);
:			return(TASK_ENTRY_FLAG);

{D}+			{
				resultlval.anInt = atoi((char *)&resulttext[0]);	
				return(INTEGER);
			}

{D}+"."{D}*({E})?	|
{D}*"."{D}+({E})?	|
{D}+{E}			{
				resultlval.aFloat = atof((char *)&resulttext[0]);
				return(FLOAT);
			}

{D}+":"{D}{D}":"{D}{D}("."{D}{D})? {
    				resultlval.aString = (char *)&resulttext[0];
				return(TIME);
			}

-1			return(ENDLIST);

{C}			{
				resultlval.aChar = *resulttext;
				return(CHAR);
			}

infinity		{
				resultlval.aString = (char *)&resulttext[0];
				return(INFTY);
			}

[.iI][nN][fF]		{
				resultlval.aString = (char *)&resulttext[0];
				return(INFTY);
			}

({U})*{C}({C}|{D}|{U})+	{
				resultlval.aString = (char *)&resulttext[0];
				return(SYMBOL);
			}

.			{
				resultlval.aString = (char *)&resulttext[0];
				return(TEXT);
			}

%%
int
resultwrap()
{
	return 1;
}
