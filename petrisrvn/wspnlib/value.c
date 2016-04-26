/*
 *  $Id: value.c 11063 2012-07-06 16:15:58Z greg $
 *
 *  This file provides different values for modelling.
 *
 */

#include <../config.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "global.h"
#include "wspn.h"

static double tran_rate( struct trans_object *trans );

/*
 * return the mean number of tokens at place obj_name.
 */

double
value_pmmean( const char *obj_name )
{
	struct place_object *place = find_place( obj_name );

	if ( place ) {
		return place->distr->prob;
	} else {
		return -1.0;
	}
}

/*
 * Return the variance of `m' tokens at place obj_name.
 */

double
value_pmvariance( const char *obj_name )
{
	struct place_object *place = find_place( obj_name );
	struct probability *p;
	double var = 0.0;
	double mean;

	if ( place == NULL || place->distr->next == NULL ) return 0.0;

	mean = place->distr->prob;
/*	for ( p = place->distr->next->next; p; p = p->next ) { */
	for ( p = place->distr->next; p; p = p->next ) { 
		var += p->prob * (p->val - mean) * (p->val - mean);
	}
	return var;
}


/*
 * Return the probability that `m' tokens are found at place obj_name.
 */

double
value_prob( const char *obj_name, int m )
{
	struct place_object *place = find_place( obj_name );
	struct probability *p;

	if ( place == NULL ) return -1.0;

	for ( p = place->distr->next->next; p && m != p->val; p = p->next ) ;

	if ( p ) {
		return p->prob;
	} else {
		return 0.0;
	}
}


/*
 * Return the firing rate of the timed transition obj_name.
 */

double
value_trate( const char *obj_name )
{
	struct trans_object * trans	= find_trans( obj_name );

	if ( trans ) {
		return tran_rate( trans );
	} else {
		return 0.0;
	}
}


						
/*
 * Return the rate of the immediate transition name obj_name.
 */

double
value_itrate( const char *obj_name )
{
	struct trans_object * trans	= find_itrans( obj_name );

	if ( trans ) {
		return tran_rate( trans );
	} else {
		return 0.0;
	}
}


/*
 * Return the firing rate...
 */

static double
tran_rate( struct trans_object *trans )
{
	struct rpar_object * rpar;
	unsigned j;

	if ( trans->rpar != NULL && trans->mark_dep != NULL ) {
		
		return 0.0;
		
	} else if (trans->rpar != NULL ) {
		
		return trans->rpar->value;
		
	} else if ( trans->fire_rate.ff < 0 ) {
		
		j = -trans->fire_rate.ff;
		for ( rpar = netobj->rpars; rpar && j > 1; rpar = rpar->next, --j ) ;
		if ( rpar ) {
			return (double)rpar->value;
		} else {
			return 0.0;
		}
		
	} else {
		
		return (double)trans->fire_rate.ff;
		
	}
}



/*
 */

int 
value_mpar( const char *obj_name ) 
{
	struct mpar_object *mpar;

	for ( mpar = netobj->mpars; mpar != NULL && strcmp(obj_name, mpar->tag) != 0; mpar = mpar->next );
	if ( mpar != NULL ) {
		return mpar->value;
	} else {
		(void) fprintf( stderr, "No marking parameter named %s found in net\n", obj_name );
		return 0;
	}
}

/*
 * Return the initial number of tokens at place obj_name
 */

int 
value_pmar( const char * obj_name ) 
{
	struct place_object *place = find_place( obj_name );

	if (place == NULL) {
		return 0;
	} else if (place->mpar != NULL) {
		return place->mpar->value;
	} else {
		return place->tokens;
	}
}

/*
 * REturn the value of the rate parameter obj_name
 */

double
value_rpar( const char * obj_name )
{
	struct rpar_object *rpar = find_rpar( obj_name );

	if ( rpar ) {
		return rpar->value;
	} else {
		return 0;
	}
}

/*
 * Search and return the value of the result obj_name.
 */

double
value_res(obj_name)
char obj_name[];
{
	struct res_object *result = find_result( obj_name );

	if ( result ) {
		return result->value;
	} else {
		return 0.0;
	}
}


/*
 * Return the throughput of the timed transition obj_name;
 */

double
value_tput( const char * obj_name ) 
{
	struct trans_object * trans = find_trans( obj_name );

	if ( trans ) {
		return trans->f_time;
	} else {
		return 0.0;
	}
}



/*
 * Return the throughput of the immediate transition obj_name
 */

double 
value_itput( const char * obj_name )
{
	struct trans_object * trans = find_itrans( obj_name );

	if ( trans ) {
		return trans->f_time;
	} else {
		return 0.0;
	}
}

/*
 * Find the place in the net.
 */

struct place_object *
find_place( const char * obj_name )
{
	struct place_object * place;
	
	for ( place = netobj->places; place != NULL && strcmp(obj_name, place->tag) != 0; place = place->next );

	if ( !place && wspn_report_errors ) {
		(void) fprintf( stderr, "No place named %s in net\n", obj_name );
	}
	return place;
}


	

/*
 * Find the exponentially timed transition in the net.
 */

struct trans_object *
find_trans( const char * obj_name )
{
	struct trans_object * trans;

	for ( trans = netobj->trans; trans != NULL && strcmp(obj_name, trans->tag) != 0; trans = trans->next );

	if ( !trans && wspn_report_errors ) {
		(void) fprintf( stderr, "No transition named %s found in net\n", obj_name );
	}
	
	return trans;;
}


/*
 * Find the immediate transition in the net.
 */

struct trans_object *
find_itrans( const char * obj_name ) 
{
	struct group_object * cur_group;
	struct trans_object * trans;

	for ( cur_group = netobj->groups; cur_group; cur_group = cur_group->next ) {
		for ( trans = cur_group->trans; trans != NULL && strcmp(obj_name, trans->tag) != 0; trans = trans->next );
		if ( trans ) return trans;
	}

	if ( wspn_report_errors ) {
		(void) fprintf( stderr, "No transition named %s found in net\n", obj_name );
	}
	return NULL;
}

/*
 * Find the named result.
 */

struct res_object *
find_result( const char * obj_name )
{
	struct res_object * result;
	
	for ( result = netobj->results; result != NULL && strcmp(obj_name, result->tag) != 0; result = result->next );

	if ( !result && wspn_report_errors ) {
		(void) fprintf( stderr, "No result named %s found in net\n", obj_name );
	}
	return result;
}


/*
 * Find the named rate parameter object.
 */

struct rpar_object *
find_rpar( const char * obj_name ) 
{
	struct rpar_object * rpar;
	
	for ( rpar = netobj->rpars; rpar != NULL && strcmp(obj_name, rpar->tag) != 0; rpar = rpar->next );

	if ( !rpar && wspn_report_errors ) {
		(void) fprintf( stderr, "No rate parameter named %s found in net\n", obj_name );
	}
	return rpar;
}


/*
 * Find and return the number of tangible and vanishing markings.
 */

int
solution_stats( tangible, vanishing, precision )
int * tangible;
int * vanishing;
double * precision;
{
	char filename[BUFSIZ];
	FILE * fptr;
	int rc;
	int topvan, maxmark;
	
	(void)sprintf( filename, "nets/%s.rgr_aux", edit_file );
	fptr = fopen( filename, "r");
	if ( fptr == NULL ) {
		(void) fprintf( stderr, "Cannot open " );
		perror( filename );
		rc = 0;
	} else if ( fscanf(fptr,"toptan= %d\ntopvan= %d\nmaxmark= %d\n", tangible, &topvan, &maxmark ) != 3 ) {
		(void) fprintf( stderr, "Error reading marking data from %s\n", filename );
		rc = 0;
	} else {
		*vanishing = maxmark - topvan;
		rc = 1;
	}
	if ( fptr ) {
	    (void) fclose( fptr );
	}
	if ( !rc ) return rc;

	(void)sprintf( filename, "nets/%s.mpd", edit_file );
	fptr = fopen( filename, "r" );
	if ( !fptr ) {
		(void) fprintf( stderr, "Cannot open " );
		perror( filename );
		rc = 0;
	} else if ( fread( (char *)precision, sizeof(double), 1, fptr ) != 1 ) {
		(void) fprintf( stderr, "Error reading precision from %s\n", filename );
		rc = 0;
	}
	*precision = -*precision;
	(void) fclose( fptr );
	return rc;
}
