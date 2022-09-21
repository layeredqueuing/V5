/*
 * $id: srvn_gram.y 14381 2021-01-19 18:52:02Z greg $ 
 */

%{
#include <stdio.h>
#include "qnap2_document.h"

static void qnap2error( const char * fmt );
extern int qnap2lex();

static void * curr_station = NULL;
static bool station_found = false;
%}

%token QNAP_ASSIGNMEMT QNAP_LESS_EQUAL QNAP_GREATER_EQUAL QNAP_POWER
%token QNAP_CONTROL QNAP_DECLARE QNAP_END QNAP_EXEC QNAP_RESTART QNAP_REBOOT QNAP_STATION QNAP_TERMINAL 

%token QNAP_ALL QNAP_AND QNAP_ANY QNAP_BEGIN QNAP_DO QNAP_ELSE QNAP_FALSE QNAP_FOR QNAP_FORWARD QNAP_GENERIC
%token QNAP_GOTO QNAP_IF QNAP_IN  QNAP_IS QNAP_NIL QNAP_NOT QNAP_OBJECT QNAP_OR 
%token QNAP_REF QNAP_REPEAT QNAP_STEP QNAP_THEN QNAP_TRUE QNAP_UNTIL QNAP_VAR QNAP_WATCHED
%token QNAP_WHILE QNAP_WITH

%token <aString>	QNAP_IDENTIFIER QNAP_STRING
%token QNAP_BOOLEAN QNAP_INTEGER QNAP_QUEUE QNAP_REAL 
%token QNAP_CONSTANT
%token QNAP_NAME QNAP_INIT QNAP_PRIO QNAP_QUANTUM QNAP_RATE QNAP_SCHED QNAP_SERVICE QNAP_TRANSIT QNAP_TYPE

%union {
    long anInt;
    double aReal;
    char * aString;
    void * aPointer;
}

%type <aPointer>	identifier_list

%%

qnap2			: command_list QNAP_END
			;

command_list		: command_list command
			| command
			;

command			: declare
			| station
			| QNAP_CONTROL
			| QNAP_TERMINAL
			| QNAP_EXEC
			| QNAP_REBOOT
			| QNAP_RESTART
			;

declare			: QNAP_DECLARE declare_list
			;

declare_list		: declare_list ';' declare_statement
			| declare_statement ';'
			;

declare_statement	: QNAP_QUEUE identifier_list		{ qnap_add_queue( $2 ); }
			| variable_type identifier_list
			;

variable_type		: QNAP_INTEGER
			| QNAP_REAL
			| QNAP_BOOLEAN
			| QNAP_STRING
			;

identifier_list		: identifier_list ',' identifier
			| identifier
			;

identifier		: QNAP_IDENTIFIER
/*			| QNAP_IDENTIFIER '=' sublist */
			;

/*
sublist			: simple_sublist
			| sublist ',' simple_sublist
			;

simple_sublist		: expression
			;
*/

expression		: expression '+' term
			| expression '-' term
			| term
			;

term			: term '*' power
			| term '/' power
			| term '%' power
			| power
			;

power			: arrayref QNAP_POWER power
			| arrayref
			;

/*
prefix			: TOK_LOGIC_NOT arrayref
			| arrayref
			;
*/


arrayref		: arrayref '[' expression ']'
			| factor
			;

factor			: '(' expression ')'
			| identifier '(' ')'
			| identifier '(' expression_list ')'
			| variable
			;

expression_list		: expression
			| expression_list ',' expression
			;

variable		: identifier
			| QNAP_CONSTANT

/* Station */

station			: QNAP_STATION { curr_station = qnap_add_station(); } station_list { curr_station = 0; station_found = false; }
			;

station_list		: station_list ';' station_statement
			| station_statement ';'
			;

station_statement	: QNAP_NAME '=' QNAP_IDENTIFIER	{ station_found = qnap_set_station_name( curr_station, $3 ); }
			| QNAP_INIT '=' variable
			| QNAP_PRIO
			| QNAP_QUANTUM
			| QNAP_RATE
			| QNAP_SCHED
			| QNAP_SERVICE '=' factor
			| QNAP_TRANSIT
			| QNAP_TYPE
			;

%%


static void
qnap2error( const char * fmt )
{
}
