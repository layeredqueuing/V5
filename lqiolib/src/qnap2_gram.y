/*
 * $id: srvn_gram.y 14381 2021-01-19 18:52:02Z greg $ 
 */

%{
#include <stdio.h>
#include <string.h>
#include "qnap2_document.h"

extern void qnap2error( const char * fmt, ... );
extern int qnap2lex();

static void * curr_station = NULL;
static bool station_found = false;
%}

%union {
    int aCode;
    long aLong;
    double aReal;
    char * aString;
    void * aPointer;
}

%token QNAP_CONTROL QNAP_DECLARE QNAP_END QNAP_EXEC QNAP_RESTART QNAP_REBOOT QNAP_STATION QNAP_TERMINAL 
%token QNAP_QUEUE QNAP_CLASS							/* /declare/ */
%token QNAP_INIT QNAP_NAME QNAP_PRIO QNAP_QUANTUM QNAP_RATE QNAP_SCHED QNAP_SERVICE QNAP_TRANSIT QNAP_TYPE	/* /station/ */

%token QNAP_ASSIGNMENT QNAP_LESS_EQUAL QNAP_GREATER_EQUAL QNAP_POWER 
%token QNAP_ALL QNAP_AND QNAP_ANY QNAP_BEGIN QNAP_DO QNAP_ELSE QNAP_FALSE QNAP_FOR QNAP_FORWARD QNAP_GENERIC
%token QNAP_GOTO QNAP_IF QNAP_IN  QNAP_IS QNAP_NIL QNAP_NOT QNAP_OBJECT QNAP_OR 
%token QNAP_REF QNAP_REPEAT QNAP_STEP QNAP_THEN QNAP_TRUE QNAP_UNTIL QNAP_VAR QNAP_WATCHED
%token QNAP_WHILE QNAP_WITH
%token RANGE_ERR 

%token <aString>	STRING QNAP_IDENTIFIER 
%token <aCode>		QNAP_BOOLEAN QNAP_REAL QNAP_INTEGER QNAP_STRING		/* variable types */
%token <aCode>		QNAP_CST QNAP_COX QNAP_EXP QNAP_ERLANG QNAP_HEXP	/* service distributions */
%token <aCode>		QNAP_SERVER QNAP_MULTIPLE QNAP_SINGLE QNAP_SOURCE QNAP_INFINITE	/* Station type */
%token <aReal>		DOUBLE
%token <aLong>		LONG

%type <aPointer>	class_list class_reference identifier_list service transit transit_list transit_pair variable variable_list loop_list
%type <aPointer>	assignment_statement closed_statement compound_statement exec expression expression_list factor open_statement power prefix_statement postfix_statement procedure_call simple_statement statement term 
%type <aCode>		compound_station_type simple_station_type variable_type 
%type <aString>		identifier
%%

qnap2			: declare 					{ qnap2_default_class(); }
			  station_list 					{ qnap2_map_transit_to_visits(); }
			  opt_control
			  exec						{ qnap2_set_program( $6 ); }
			  QNAP_END
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

declare_statement	: QNAP_QUEUE variable_list ';'			{ qnap2_add_queue( $2 ); }
			| QNAP_CLASS variable_list ';'			{ qnap2_add_class( $2 ); }
			| QNAP_CLASS variable_type variable_list ';'	{ qnap2_add_field( $2, $3 ); }
			| variable_type variable_list ';'		{ qnap2_add_variable( $1, $2 ); }
			;

variable_list		: variable					{ $$ = qnap2_append_identifier( NULL, $1 ); free( $1 ); }
			| variable_list ',' variable			{ $$ = qnap2_append_identifier( $1, $3 ); free( $3 ); }
			;

variable		: identifier					{ $$ = $1; qnap2_declare_scalar( $1 ); }
			| identifier '(' factor ')'			{ $$ = $1; qnap2_declare_vector( $1, $3 ); }
			;

variable_type 		: QNAP_INTEGER 					{ $$ = $1; }
			| QNAP_REAL 					{ $$ = $1; }
			| QNAP_BOOLEAN 					{ $$ = $1; }
			| QNAP_STRING 					{ $$ = $1; }

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

station			: QNAP_STATION parameter_list 			{ qnap2_define_station(); }
			;

parameter_list		: parameter
			| parameter_list parameter
			;

parameter		: QNAP_NAME '=' identifier ';'			{ qnap2_set_station_name( $3 ); free( $3 ); }
			| QNAP_INIT class_reference '=' factor ';'	{ qnap2_set_station_init( $2, $4 ); }
			| QNAP_PRIO class_reference '=' factor ';'	{ qnap2_set_station_prio( $2, $4 ); }
			| QNAP_QUANTUM class_reference '=' factor ';'	{ qnap2_set_station_quantum( $2, $4 ); }
			| QNAP_RATE '=' factor ';'			{ qnap2_set_station_rate( $3 ); }
			| QNAP_SCHED '=' identifier ';'			{ qnap2_set_station_sched( $3 ); free( $3 ); }
			| QNAP_SERVICE class_reference '=' service ';'	{ qnap2_set_station_service( $2, $4 ); }
			| QNAP_TRANSIT class_reference '=' transit ';'	{ qnap2_set_station_transit( $2, $4 ); }
			| QNAP_TYPE '=' station_type ';'
			;

class_reference		:						{ $$ = NULL; }
			| '(' QNAP_ALL QNAP_CLASS ')'			{ $$ = NULL; }
			| '(' class_list ')'				{ $$ = $2; }
			;

class_list		: identifier					{ $$ = qnap2_append_identifier( NULL, qnap2_get_class_name( $1 ) ); free( $1 ); }
			| class_list ',' identifier			{ $$ = qnap2_append_identifier( $1, qnap2_get_class_name( $3 ) ); free( $3 ); }
			;


service			: QNAP_CST '(' factor ')'			{ $$ = qnap2_get_service_distribution( $1, $3, NULL ); }
			| QNAP_COX '(' factor ')'			{ $$ = qnap2_get_service_distribution( $1, $3, NULL ); }
			| QNAP_EXP '(' factor ')'			{ $$ = qnap2_get_service_distribution( $1, $3, NULL ); }
			| QNAP_ERLANG '(' factor ',' factor ')'		{ $$ = qnap2_get_service_distribution( $1, $3, $5 ); }		/* $4 == c^2 */
			| QNAP_HEXP '(' factor ',' factor ')'		{ $$ = qnap2_get_service_distribution( $1, $3, $5 ); }		/* $4 == k == 1/c^2 */
			;

transit			: identifier					{ $$ = qnap2_append_node( NULL, qnap2_get_transit_pair( $1, NULL ) ); }
			| transit_list 					{ $$ = $1; }
			;

transit_list		: transit_pair					{ $$ = qnap2_append_node( NULL, $1 ); }
			| transit_list ',' transit_pair			{ $$ = qnap2_append_node( $1, $3 ); }
			;

transit_pair		: identifier ',' factor				{ $$ = qnap2_get_transit_pair( qnap2_get_station_name( $1 ), $3 ); }
			;


station_type		: simple_station_type				{ qnap2_set_station_type( $1, 1 ); }
			| QNAP_SOURCE					{ qnap2_set_station_type( $1, 1 ); }
			| QNAP_MULTIPLE '(' QNAP_INTEGER ')'		{ qnap2_set_station_type( $1, $3 ); }
			| compound_station_type				{ qnap2_set_station_type( $1, 1 ); }
			| compound_station_type '(' QNAP_INTEGER ')'	{ qnap2_set_station_type( $1, $3 ); }
			;

simple_station_type	: QNAP_INFINITE					{ $$ = $1; }
			| QNAP_SINGLE					{ $$ = $1; }
			;

compound_station_type	: QNAP_SERVER ',' simple_station_type 		{ $$ = $1; }
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

exec 			: QNAP_EXEC statement ';'			{ $$ = $2; }
			;

statement		: open_statement				{ $$ = $1; }
			| closed_statement				{ $$ = $1; }

open_statement		: QNAP_IF expression QNAP_THEN statement		{ $$ = qnap2_if_statement( $2, $4, NULL ); }
			| QNAP_IF expression QNAP_THEN closed_statement 	/* -- */
			  QNAP_ELSE open_statement				{ $$ = qnap2_if_statement( $2, $4, $6 ); }
			| QNAP_WHILE expression QNAP_THEN open_statement	{ $$ = qnap2_while_statement( $2, $4 ); }
			| QNAP_FOR identifier QNAP_ASSIGNMENT loop_list		/* -- */
			  QNAP_DO open_statement				{ $$ = qnap2_for_statement( $2, $4, $6 ); }
			;

closed_statement	: simple_statement					{ $$ = $1; }
			| QNAP_IF expression QNAP_THEN closed_statement   	/* -- */
			  QNAP_ELSE closed_statement				{ $$ = qnap2_if_statement( $2, $4, $6 ); }
			| QNAP_WHILE expression QNAP_THEN closed_statement	{ $$ = qnap2_while_statement( $2, $4 ); }
			| QNAP_FOR identifier QNAP_ASSIGNMENT loop_list		/* -- */
			  QNAP_DO closed_statement				{ $$ = qnap2_for_statement( $2, $4, $6 ); }
			;

simple_statement	: QNAP_BEGIN compound_statement QNAP_END	{ $$ = $2; }
			| assignment_statement				{ $$ = $1; }
			| procedure_call				{ $$ = $1; }
			;

compound_statement	: statement					{ $$ = qnap2_append_node( NULL, $1 ); }
			| compound_statement ';' statement		{ $$ = qnap2_append_node( $1, $3 ); }
			;

assignment_statement	: identifier '(' identifier ')' QNAP_ASSIGNMENT expression 	{ $$ = qnap2_array_assignment( $1, $3, $6 ); }
			| identifier '.' identifier QNAP_ASSIGNMENT expression		{ $$ = qnap2_object_assignment( $1, $3, $5 ); }
			| identifier QNAP_ASSIGNMENT expression		{ $$ = qnap2_assignment( $1, $3 ); }
			;

procedure_call		: identifier					{ $$ = qnap2_get_function( $1, NULL ); }
			| identifier '(' expression_list ')'		{ $$ = qnap2_get_function( $1, $3 ); }
			;


expression		: expression '+' term				{ $$ = qnap2_add( $1, $3 ); }
			| expression '-' term				{ $$ = qnap2_subtract( $1, $3 ); }
			| term						{ $$ = $1; }
			;

term			: term '*' power				{ $$ = qnap2_multiply( $1, $3 ); }
			| term '/' power				{ $$ = qnap2_divide( $1, $3 ); }
			| term '%' power				{ $$ = qnap2_modulus( $1, $3 ); }
			| power						{ $$ = $1; }
			;

power			: prefix_statement QNAP_POWER power		{ $$ = qnap2_power( $1, $3 ); }
			| prefix_statement				{ $$ = $1; }
			;

prefix_statement	: '+' factor					{ $$ = $2; }
			| '-' factor					{ $$ = qnap2_negate( $2 ); }
			| postfix_statement				{ $$ = $1; }
			;

postfix_statement	: identifier '(' expression_list ')'		{ $$ = qnap2_get_function( $1, $3 ); }
			| factor					{ $$ = $1; }
			;

factor			: identifier					{ $$ = qnap2_get_variable( $1 ); }
			| LONG 						{ $$ = qnap2_get_integer( $1 ); }
			| DOUBLE					{ $$ = qnap2_get_real( $1 ); }
			| STRING					{ $$ = qnap2_get_string( $1 ); }
			| '(' expression ')'				{ $$ = $2; }
			;

expression_list		: expression					{ $$ = qnap2_append_node( NULL, $1 ); }
			| expression_list ',' expression		{ $$ = qnap2_append_node( $1, $3 ); }
			;

loop_list		: identifier { $$ = $1; }
/* QNAP STEP UNTIL; id, id,..; */
			;

/* ------------------------------------------------------------------------ */

/*
step_until		: '(' LONG QNAP_STEP LONG QNAP_UNTIL identifier ')'
			;
*/

identifier_list		: identifier					{ $$ = qnap2_append_identifier( NULL, $1 ); free( $1 ); }
			| identifier_list ',' identifier		{ $$ = qnap2_append_identifier( $1, $3 ); free( $3 ); }
			;

identifier		: QNAP_IDENTIFIER				{ $$ = $1; }
			| QNAP_CLASS					{ $$ = strdup( "class" ); }
			| QNAP_COX					{ $$ = strdup( "cox" ); }
			| QNAP_CST					{ $$ = strdup( "cst" ); }  
			| QNAP_ERLANG					{ $$ = strdup( "erlang" ); }	
			| QNAP_EXP					{ $$ = strdup( "exp" ); }   
			| QNAP_HEXP					{ $$ = strdup( "hexp" ); }
			| QNAP_INIT					{ $$ = strdup( "init" ); }
			| QNAP_INTEGER					{ $$ = strdup( "integer" ); }
			| QNAP_NAME					{ $$ = strdup( "name" ); }
			| QNAP_PRIO					{ $$ = strdup( "prio" ); }
			| QNAP_QUANTUM					{ $$ = strdup( "quantum" ); }
			| QNAP_QUEUE					{ $$ = strdup( "queue" ); }
			| QNAP_RATE					{ $$ = strdup( "rate" ); }
			| QNAP_REAL					{ $$ = strdup( "real" ); }
			| QNAP_SCHED					{ $$ = strdup( "sched" ); }
			| QNAP_SERVICE					{ $$ = strdup( "service" ); }
			| QNAP_TRANSIT					{ $$ = strdup( "transit" ); }
			| QNAP_TYPE					{ $$ = strdup( "type" ); }
                        | QNAP_INFINITE	                                { $$ = strdup( "infinite" ); }
                        | QNAP_MULTIPLE	                                { $$ = strdup( "multiple" ); }  
                        | QNAP_SERVER	                                { $$ = strdup( "server" ); }   
                        | QNAP_SINGLE	                                { $$ = strdup( "single" ); }
                        | QNAP_SOURCE	                                { $$ = strdup( "source" ); }
			;
%%
