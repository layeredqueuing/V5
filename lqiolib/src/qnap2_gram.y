/*
 * $id: srvn_gram.y 14381 2021-01-19 18:52:02Z greg $ 
 */

%{
#include <stdio.h>
#include <strings.h>
#include "qnap2_document.h"

extern void qnap2error( const char * fmt, ... );
extern int qnap2lex();

static void * curr_station = NULL;
static bool station_found = false;
%}

%token QNAP_ASSIGNMENT QNAP_LESS_EQUAL QNAP_GREATER_EQUAL QNAP_POWER 
%token QNAP_CONTROL QNAP_DECLARE QNAP_END QNAP_EXEC QNAP_RESTART QNAP_REBOOT QNAP_STATION QNAP_TERMINAL 

%token QNAP_ALL QNAP_AND QNAP_ANY QNAP_BEGIN QNAP_DO QNAP_ELSE QNAP_FALSE QNAP_FOR QNAP_FORWARD QNAP_GENERIC
%token QNAP_GOTO QNAP_IF QNAP_IN  QNAP_IS QNAP_NIL QNAP_NOT QNAP_OBJECT QNAP_OR 
%token QNAP_REF QNAP_REPEAT QNAP_STEP QNAP_THEN QNAP_TRUE QNAP_UNTIL QNAP_VAR QNAP_WATCHED
%token QNAP_WHILE QNAP_WITH
%token RANGE_ERR 

%token <aString>	STRING QNAP_IDENTIFIER 
%token <aPointer>	
%token <aCode>		QNAP_BOOLEAN QNAP_REAL QNAP_INTEGER QNAP_STRING		/* scanner codes */
%token <aReal>		DOUBLE
%token <aLong>		LONG
%token QNAP_QUEUE QNAP_CLASS
%token QNAP_NAME QNAP_INIT QNAP_PRIO QNAP_QUANTUM QNAP_RATE QNAP_SCHED QNAP_SERVICE QNAP_TRANSIT QNAP_TYPE

%union {
    int aCode;
    long aLong;
    double aReal;
    char * aString;
    void * aPointer;
}

%type <aPointer>	identifier_list variable arrayref power factor term expression expression_list transit station_type_pair 
%type <aCode>		variable_type
%type <aString>		identifier transit_class
%%

qnap2			: declare { qnap2_default_class(); } station_list opt_control exec QNAP_END
			;

/*
command_list		: command_list command
			| command
			;

command			| QNAP_TERMINAL
			| QNAP_REBOOT
			| QNAP_RESTART
			;
*/

/* ------------------------------------------------------------------------ */

declare			: QNAP_DECLARE declare_list
			;

declare_list		: declare_statement
			| declare_list declare_statement
			;

declare_statement	: QNAP_QUEUE identifier_list ';'	{ qnap2_add_queue( $2 ); }
			| QNAP_CLASS identifier_list ';'	{ qnap2_add_class( $2 ); }
			| QNAP_CLASS variable_type identifier_list ';'	{ qnap2_add_field( $2, $3 ); }
			| variable_type identifier_list ';'	{ qnap2_add_variable( $1, $2 ); }
			;

variable_type 		: QNAP_INTEGER 				{ $$ = $1; }
			| QNAP_REAL 				{ $$ = $1; }
			| QNAP_BOOLEAN 				{ $$ = $1; }
			| QNAP_STRING 				{ $$ = $1; }

identifier_list		: identifier				{ $$ = qnap2_append_identifier( NULL, $1 ); free( $1 ); }
			| identifier_list ',' identifier	{ $$ = qnap2_append_identifier( $1, $3 ); free( $3 ); }
			;

/*
sublist			: simple_sublist
			| sublist ',' simple_sublist
			;

simple_sublist		: expression
			;
*/


/* ------------------------------------------------------------------------ */

station_list		: station
			| station_list station
			;

station			: QNAP_STATION parameter_list 		{ qnap2_define_station(); }
			;

parameter_list		: parameter
			| parameter_list parameter
			;

parameter		: QNAP_NAME '=' identifier ';'		{ qnap2_set_station_name( $3 ); free( $3 ); }
			| QNAP_INIT '=' variable ';'		{ qnap2_set_station_init( $3 ); }
			| QNAP_PRIO '=' variable ';'		{ qnap2_set_station_prio( $3 ); }
			| QNAP_QUANTUM '=' variable ';'		{ qnap2_set_station_quantum( $3 ); }
			| QNAP_RATE '=' variable ';'		{ qnap2_set_station_rate( $3 ); }
			| QNAP_SCHED '=' identifier ';'		{ qnap2_set_station_sched( $3 ); free( $3 ); }
			| QNAP_SERVICE '=' factor ';'		{ qnap2_set_station_service( $3 ); }
			| QNAP_TRANSIT transit_class '=' transit ';'	
								{ qnap2_set_station_transit( $2, $4 ); }
			| QNAP_TYPE '=' station_type ';'
			;

transit_class		: 					{ $$ = NULL; }
			| '(' QNAP_ALL QNAP_CLASS ')'		{ $$ = NULL; }
			| '(' identifier ')' 			{ $$ = $2; }
			;

transit			: identifier
			| transit_list 
			;

transit_list		: transit_pair
			| transit_list ',' transit_pair
			;

transit_pair		: identifier ',' variable
			;


station_type		: identifier				{ qnap2_set_station_type( $1, 1 ); free( $1 ); }
			| identifier '(' QNAP_INTEGER ')'	{ qnap2_set_station_type( $1, $3 ); free( $1 ); }
			| station_type_pair			{ qnap2_set_station_type( $1, 1 ); free( $1 ); }
			| station_type_pair '(' QNAP_INTEGER ')'{ qnap2_set_station_type( $1, $3 ); free( $1 ); }
			;

station_type_pair	: identifier ',' identifier		{ $$ = qnap2_get_station_type( $1, $3 ); free( $1 ); free( $3 ); }
			;

/* ------------------------------------------------------------------------ */

opt_control		:
			| QNAP_CONTROL control_list
			;

control_list		: control
			| control_list control
			;

control			: QNAP_CLASS '=' QNAP_ALL QNAP_QUEUE ';'
			;

/* ------------------------------------------------------------------------ */

exec 			: QNAP_EXEC statement
			;

statement		: QNAP_BEGIN compound_statement QNAP_END ';'
			| identifier ';'
			| identifier QNAP_ASSIGNMENT expression ';'
			| identifier '.' identifier QNAP_ASSIGNMENT expression ';'
			;

compound_statement	: statement
			| compound_statement statement
			;


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

factor			: '(' expression ')'			{ $$ = $2; }
			| identifier '(' ')'
			| identifier '(' expression_list ')'
			| variable				{ $$ = $1; }
			;

expression_list		: expression
			| expression_list ',' expression
			;

variable		: identifier				{ $$ = qnap2_get_variable( $1 ); }
			| LONG 					{ $$ = qnap2_get_integer( $1 ); }
			| DOUBLE				{ $$ = qnap2_get_real( $1 ); }
			| STRING				{ $$ = qnap2_get_string( $1 ); }
			;

identifier		: QNAP_IDENTIFIER			{ $$ = $1; }
			| QNAP_CLASS   				{ $$ = strdup( "class" ); }
			| QNAP_INIT    				{ $$ = strdup( "init" ); }
			| QNAP_INTEGER 				{ $$ = strdup( "integer" ); }
			| QNAP_NAME    				{ $$ = strdup( "name" ); }
			| QNAP_PRIO    				{ $$ = strdup( "prio" ); }
			| QNAP_QUANTUM 				{ $$ = strdup( "quantum" ); }
			| QNAP_QUEUE   				{ $$ = strdup( "queue" ); }
			| QNAP_RATE    				{ $$ = strdup( "rate" ); }
			| QNAP_REAL    				{ $$ = strdup( "real" ); }
			| QNAP_SCHED   				{ $$ = strdup( "sched" ); }
			| QNAP_SERVICE 				{ $$ = strdup( "service" ); }
			| QNAP_TRANSIT 				{ $$ = strdup( "transit" ); }
			| QNAP_TYPE 				{ $$ = strdup( "type" ); }
			;
%%
