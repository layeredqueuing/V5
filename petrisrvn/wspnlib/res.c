/*
 *  $Id: res.c 10972 2012-06-19 01:12:22Z greg $
 *
 *  Routines to edit and show result definitions.
 *
 */

#include <../config.h>
#include <math.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdlib.h>
#include "global.h"
#include "wspn.h"

static void get_pascal_real ( FILE * file, double * num );

/***********************  local variables  ************************/

static struct place_object *place;
static struct res_object *n_res;
static struct probability *prob;
static struct probability *temp_pr;

static int      noplace;
static float    error, attime;


static void
clear_res()
{

	for (place = netobj->places; place != NULL; place = place->next) {
		for (prob = place->distr, place->distr = NULL; prob != NULL; prob = temp_pr) {
			temp_pr = prob->next;
			free((char *) prob);
		}
	}
	for (n_res = netobj->results; n_res != NULL; n_res = n_res->next) {
		n_res->value = -1.0;
	}
}


static void
get_pascal_real ( FILE * file, double * num )
{
	if ( fread( num, sizeof( double ), 1, file ) != 1 ) {
		if ( errno != 0 ) {
			perror( "get_pascal_real: read error" );
		}
		*num = 0.0;
	}
}

static void
get_token_distr( FILE  *file, int sim )
{
	register int    i;
	int             min_t, max_t;
	double          rr;

	for (place = netobj->places; place != NULL; place = place->next) {
		get_pascal_real(file, &rr);
		min_t = (int) (rr + 0.5);
		get_pascal_real(file, &rr);
		max_t = (int) (rr + 0.5);
		place->distr = prob = (struct probability *) emalloc(PROB_SIZE);
		prob->prob = 0.0;
		prob->val = -1;
		temp_pr = prob->next = (struct probability *) emalloc(PROB_SIZE);
		temp_pr->val = -2;
		for (i = min_t; i <= max_t; i++) {
			temp_pr = temp_pr->next = (struct probability *) emalloc(PROB_SIZE);
			get_pascal_real(file, &rr);
			prob->prob += i * rr;
			temp_pr->prob = rr;
			temp_pr->val = i;
		}
		temp_pr->next = NULL;
	}
	if (sim)
		for (place = netobj->places; place != NULL; place = place->next) {
			get_pascal_real(file, &rr);
			place->distr->next->prob = rr;
		}
	else
		for (place = netobj->places; place != NULL; place = place->next) {
			rr = error * place->distr->prob;
			place->distr->next->prob = rr;
		}
}


static void
get_throughputs()
{
	char  linebuf[BUFSIZ];
	struct trans_object * trans;
	struct group_object * group;
	
	for ( trans = netobj->trans ; trans != NULL ; trans = trans->next ) {
		if ( !fgets(linebuf, BUFSIZ, dfp) ) return;
		sscanf( linebuf, "%*s = %f", &(trans->f_time) );
	}
	for ( group = netobj->groups ; group != NULL ; group = group->next ) {
		for ( trans = group->trans ; trans != NULL ; trans = trans->next ) {
			if ( !fgets( linebuf, BUFSIZ, dfp ) ) return;
			sscanf( linebuf,"%*s = %f", &(trans->f_time) );
		}
	}
}


int
collect_res( int complain, const char * toolname )
{
	char            filename[BUFSIZ];
	char            linebuf[BUFSIZ];
	double          rr;
	struct stat     stb, stb2;

	clear_res();
	(void) sprintf(filename, "nets/%s.sta", edit_file);
	(void) sprintf(linebuf, "nets/%s.net", edit_file);

	if ( stat(filename, &stb) < 0 || stat(linebuf, &stb2) < 0 || stb2.st_mtime > stb.st_mtime ) {
		if (complain) {
			(void) fprintf( stderr, "%s: No up-to-date results for net %s\n", toolname, edit_file);
		}
		return 0;
	}
	if ((dfp = fopen(filename, "r")) == NULL) {
		if (complain) {
			(void) fprintf( stderr, "%s: cannot open -- ", toolname );
			perror( filename );
		}
		return 0;
	}
	for (n_res = netobj->results; n_res != NULL; n_res = n_res->next) {
		if ( !fgets(linebuf, BUFSIZ, dfp) ) break;
		sscanf( linebuf, "%*s = %f", &(n_res->value));
	}
	get_throughputs();
	(void) fclose(dfp);

	for (place = netobj->places, noplace = 0; place != NULL; place = place->next, noplace++);


	sprintf(filename, "nets/%s.mpd", edit_file);
	if ((dfp = fopen(filename, "r")) == NULL) {
		/* simulation results */
		sprintf(filename, "nets/%s.tpd", edit_file);
		dfp = fopen(filename, "r");
		get_token_distr(dfp, TRUE);
		get_pascal_real(dfp, &rr);
		attime = -1.0;
		(void) fclose(dfp);
	} else {
		/* analytic results */
		get_pascal_real(dfp, &rr);
		error = -rr;
		get_pascal_real(dfp, &rr);
		attime = rr;
		(void) fclose(dfp);
		if ( complain ) {
			if (attime > 0.0) {
				fprintf(stderr, "the transient solution at time %g has precision %.2g\n", attime, error);
			} else {
				fprintf(stderr, "the steady-state solution has precision %.2g\n", error);
			}
		}
		(void) sprintf(filename, "nets/%s.tpd", edit_file);
		dfp = fopen(filename, "r");
		get_token_distr(dfp, FALSE);
		(void) fclose(dfp);
	}
	return 1;
}

