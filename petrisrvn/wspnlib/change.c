/*
 *  $Id: change.c 10401 2011-07-06 22:53:31Z greg $
 *
 *  This file changes the net definitions.
 *
 *  $HeadURL$
 */

#include <stdio.h>
#include <math.h>
#include "global.h"
#include "wspn.h"

int wspn_report_errors	= 1;
int wspn_accept_input	= 1;

static int change_rate( const char * obj_name, struct trans_object * trans, float fvalue );

/*
 * Change timed transitions firing rate.  Returns 1 on success and 0 on failure.
 */

int
change_trate( const char  *obj_name, float fvalue )
{
	struct trans_object *trans	= find_trans( obj_name );

	if ( trans ) {
		return change_rate( obj_name, trans, fvalue );
	} else {
		return 0;
	}
}

/*
 * Change immediate transitions switch rate.
 */

int
change_itrate( const char *obj_name, float fvalue )
{
	struct trans_object *trans	= find_itrans( obj_name );

	if ( trans ) {
		return change_rate( obj_name, trans, fvalue );
	} else {
		return 0;
	}
}


/*
 * Change rate of transition.
 */

static int
change_rate( const char * obj_name, struct trans_object * trans, float fvalue )
{
	
	if ( trans->rpar == NULL && trans->mark_dep == NULL ) {
		trans->fire_rate.ff = fvalue;
		return 1;
	} else if (trans->mark_dep == NULL) {
		trans->rpar->value = fvalue;
		return 1;
	} else if ( wspn_accept_input ) {
		printf("\n---Marking dependence!---");
		printf("\n---Enter marking dependence expression.---");
		scanf("%s", trans->mark_dep->line);
		return 1;
	} else if ( wspn_report_errors ) {
		(void) fprintf( stderr, "Marking dependence for transition named %s\n", obj_name );
	}
	return 0;
}

/*
 * Change the number of tokens at place obj_name.
 */

int
change_pmar( const char *obj_name, int ivalue )
{
	struct place_object *place 	= find_place( obj_name );

	if ( place == NULL ){
		return 0;
	} else if (place->mpar != NULL) {
		place->mpar->value = ivalue;
	} else {
		place->tokens = place->m0 = ivalue;
	}
	return 1;
}


/*
 * Change the results...
 */
 
int
change_res(const char  * obj_name, float           fvalue )
{
	struct res_object *result = find_result( obj_name );

	if ( result ) {
		result->value = fvalue;
		return 1;
	} else {
		return 0;
	}
}


/*
 * change the rate parameter name obj_name to fvalue.
 */

int
change_rpar( const char * obj_name, float fvalue )
{
	struct rpar_object *rparam = find_rpar( obj_name );

	if ( rparam  ) {
		rparam->value = fvalue;
		return 1;
	} else {
		return 0;
	}
}
