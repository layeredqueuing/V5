/************************************************************************/
/* Copyright the Real-Time and Distributed Systems Group,		*/
/* Department of Systems and Computer Engineering,			*/
/* Carleton University, Ottawa, Ontario, Canada. K1S 5B6		*/
/* 									*/
/* September 1991.							*/
/* August 2003.								*/
/************************************************************************/

/*
 * $Id$
 *
 * Make various model objects.
 */

#include "petrisrvn.h"
#include <cstdlib>
#include <cstdarg>
#include <cstring>
#if HAVE_MALLOC_H
#include <malloc.h>
#endif
#include <cassert>
#include <ctype.h>
#include <wspnlib/global.h>
#include <wspnlib/wspn.h>
#include <lqio/glblerr.h>
#include <lqio/input.h>
#include "errmsg.h"
#include "makeobj.h"

#define	MAX_NAME_SIZE	20		/* greatspn only allows 32 chars, hash at 20.	*/
#define	HASH_TABLE_SIZE	1001

static char * add_to_hash_table( const char * key, char * aStr );
static int hash ( const char * aStr  );
static int strhcmp( const char * str1, const char * str2 );

static char * hash_table[HASH_TABLE_SIZE];

/*----------------------------------------------------------------------*/
/*				PLACES					*/
/*----------------------------------------------------------------------*/

/*
 * Create a place.
 */

struct place_object *
create_place( double curr_x, double curr_y, LAYER layer, int tokens, const char * format, ... )
{
    va_list args;
    char name[BUFSIZ];
    static struct place_object *cur_place;

    assert( curr_x > 0 && curr_y > 0 );

    va_start( args, format );
    (void) vsprintf( name, format, args );
    va_end( args );

    if ( !netobj->places ) {
	netobj->places = (struct place_object *) malloc(sizeof (struct place_object ));
	cur_place = netobj->places;
    } else {
	cur_place->next = (struct place_object *) malloc(sizeof (struct place_object));
	cur_place = cur_place->next;
    }
    cur_place->tag      = strdup32x( name );
    cur_place->m0       = tokens;
    cur_place->tokens   = tokens;
    cur_place->color    = NULL;
    cur_place->lisp     = NULL;
    cur_place->distr    = NULL;
    cur_place->mpar     = NULL;
    cur_place->cmark    = NULL;
    cur_place->layer    = WHOLE_NET_LAYER | layer;

    /* Coordinates */

    move_place_tag( move_place( cur_place, curr_x, curr_y ), 0.25, 0.0 );

    cur_place->next  = NULL;

    return cur_place;
}


struct place_object *
move_place_tag( struct place_object *cur_place, double x_offset, double y_offset )
{
    cur_place->tagpos.x = IN_TO_PIX(x_offset);
    cur_place->tagpos.y = IN_TO_PIX(y_offset);
    return cur_place;
}




struct place_object *
move_place( struct place_object *cur_place, double x_offset, double y_offset )
{
    cur_place->center.x = IN_TO_PIX(x_offset);
    cur_place->center.y = IN_TO_PIX(y_offset);
    return cur_place;
}


struct place_object *
rename_place( struct place_object *cur_place, const char * format, ... )
{
    va_list args;
    char name[BUFSIZ];
    va_start( args, format );
    (void) vsprintf( name, format, args );
    va_end( args );

    cur_place->tag = strdup32x( name );
    return cur_place;
}


/*
 * Create a rate parameter and link to list.
 */

short
create_rpar( double curr_x, double curr_y, LAYER layer, double rate, const char * format, ... )
{
    va_list args;
    char name[BUFSIZ];
    static struct rpar_object *cur_rpar;

    va_start( args, format );
    (void) vsprintf( name, format, args );
    va_end( args );

    if ( netobj->rpars == NULL) {
	netobj->rpars = (struct rpar_object *) malloc(sizeof (struct rpar_object ));
	cur_rpar = netobj->rpars;
    } else {
	cur_rpar->next = (struct rpar_object *) malloc(sizeof (struct rpar_object ));
	cur_rpar = cur_rpar->next;
    }

    cur_rpar->tag      = strdup32x( name );
    cur_rpar->value    = rate;
    cur_rpar->center.x = IN_TO_PIX(curr_x);
    cur_rpar->center.y = IN_TO_PIX(curr_y);
    cur_rpar->layer    = WHOLE_NET_LAYER | layer;
    cur_rpar->next     = NULL;

    return no_rpar( cur_rpar->tag );
}


/*
 * Search the rate parameter list for the given name.
 * BEWARE -- we don't hash format...
 */

unsigned
no_rpar( const char * format, ... )
{
    va_list args;
    char name[BUFSIZ];

    struct rpar_object *cur_rpar;
    unsigned i;

    va_start( args, format );
    (void) vsprintf( name, format, args );
    va_end( args );

    for ( cur_rpar = netobj->rpars, i = 1; cur_rpar != NULL; cur_rpar = cur_rpar->next, ++i) {
	if (strcmp(cur_rpar->tag, name ) == 0) {
	    return i;
	}
    }
    return 0;
}


/*
 * Return the index of the place.  The place name is created.
 */

struct place_object *
no_place( const char * format, ... )
{
    va_list args;
    char name[BUFSIZ];

    struct place_object * cur_place;

    va_start( args, format );
    (void) vsprintf( name, format, args );
    va_end( args );

    for ( cur_place = netobj->places; cur_place != NULL; cur_place = cur_place->next ) {
	if (strhcmp(cur_place->tag, name) == 0)
	    return cur_place;
    }
    return 0;
}



/*
 * Change marking.  Return old marking.
 */

unsigned
inc_marking( const char * format, ... )
{
    va_list args;
    char name[BUFSIZ];

    int tokens;

    va_start( args, format );
    (void) vsprintf( name, format, args );
    va_end( args );

    tokens = value_pmar( name );
    change_pmar( name, tokens + 1 );
    return tokens;
}

/*----------------------------------------------------------------------*/
/*				TRANSITIONS				*/
/*----------------------------------------------------------------------*/

/*
 * Create a transition.  NOTE!  Timed transitions must preceed immediate transitions!
 */

struct trans_object *
create_trans( double x_pos, double y_pos, LAYER layer, double rate, short enable, short kind, const char * format, ... )
{
    va_list args;
    char name[BUFSIZ];

    static struct trans_object * last_trans;
    struct trans_object * cur_trans = (struct trans_object *) malloc(sizeof (struct trans_object ));

    va_start( args, format );
    (void) vsprintf( name, format, args );
    va_end( args );

    cur_trans->next  = NULL;

    if ( netobj->trans == NULL) {
	netobj->trans = cur_trans;
	last_trans    = cur_trans;

    } else switch ( kind ) {
    case DETERMINISTIC:
    case EXPONENTIAL:
	cur_trans->next  = netobj->trans;
	netobj->trans    = cur_trans;
	break;
		
    default:
	last_trans->next = cur_trans;
	last_trans = cur_trans;
	break;
    }

    cur_trans->tag	        = strdup32x( name );
    cur_trans->fire_rate.ff 	= rate;
    cur_trans->kind     	= kind;
    cur_trans->enabl		= enable;		/* 0 == infinite server */

    cur_trans->color		= NULL;
    cur_trans->lisp		= NULL;
    cur_trans->Lbound    	= 0;
    cur_trans->Ebound    	= 0;
    cur_trans->Rbound    	= 0;
    cur_trans->orient       	= 0;
    cur_trans->rpar		= NULL;
    cur_trans->mark_dep 	= NULL;
    cur_trans->layer    	= WHOLE_NET_LAYER | layer;

    /* Coordinates */

    move_trans_tag( move_trans( cur_trans, x_pos, y_pos ), 0.125, -0.125 );

    return cur_trans;
}


/*
 * Change orientation of trans.  Return the first argument.
 */

struct trans_object *
orient_trans( struct trans_object * trans, short orientation )
{
    trans->orient = orientation;
    return trans;
}


struct trans_object *
move_trans( struct trans_object * cur_trans, double x_offset, double y_offset )
{
    cur_trans->center.x = IN_TO_PIX(x_offset);
    cur_trans->center.y = IN_TO_PIX(y_offset);
    return cur_trans;
}

struct trans_object *
move_trans_tag( struct trans_object * cur_trans, double x_offset, double y_offset )
{
    cur_trans->tagpos.x  = IN_TO_PIX(x_offset);
    cur_trans->tagpos.y  = IN_TO_PIX(y_offset);
    cur_trans->ratepos.x = IN_TO_PIX(x_offset);
    cur_trans->ratepos.y = IN_TO_PIX(-y_offset);
    return cur_trans;
}



/*
 * Return the index of the transition.  The transition name is created.
 */

struct trans_object *
no_trans( const char * format, ... )
{
    va_list args;
    char name[BUFSIZ];

    struct trans_object * cur_trans;

    va_start( args, format );
    (void) vsprintf( name, format, args );
    va_end( args );

    for ( cur_trans = netobj->trans; cur_trans != NULL; cur_trans = cur_trans->next ) {
	if (strhcmp(cur_trans->tag, name) == 0)
	    return cur_trans;
    }
    return 0;
}


/*
 * Create an arc.
 *   layer -- drawing layer.
 *   type  -- TO_PLACE | TO_TRANS
 *   transition
 *   place.
 */

void 
create_arc (LAYER layer, int type, struct trans_object *transition, struct place_object *place)
{
    create_arc_mult( layer, type, transition, place, 1 );
}


void 
create_arc_mult(LAYER layer, int type, struct trans_object *transition, struct place_object *place, short mult )
{
    static struct arc_object *cur_arc;

    assert( transition != 0 );
    assert( place != 0);
	
    if ( netobj->arcs == NULL ) {
	netobj->arcs = (struct arc_object *) malloc( sizeof( struct arc_object ) );
	cur_arc = netobj->arcs;
    } else {
	cur_arc->next = (struct arc_object *) malloc( sizeof( struct arc_object ) );
	cur_arc = cur_arc->next;
    }

    cur_arc->type  = type;
    cur_arc->mult  = mult;
    cur_arc->layer = WHOLE_NET_LAYER | layer;
    cur_arc->place = place;
    cur_arc->trans = transition;
    cur_arc->color = NULL;
    cur_arc->lisp  = NULL;
    cur_arc->next  = NULL;

    cur_arc->point = (struct coordinate *)malloc(COORD_SIZE);
    cur_arc->point->next = (struct coordinate *)malloc(COORD_SIZE);

    switch ( type ) {
    case TO_PLACE:
	cur_arc->point->x = transition->center.x;
	cur_arc->point->y = transition->center.y;
	cur_arc->point->next->x = place->center.x;
	cur_arc->point->next->y = place->center.y;
	break;

    case TO_TRANS:
    case INHIBITOR:
	cur_arc->point->x = place->center.x;
	cur_arc->point->y = place->center.y;
	cur_arc->point->next->x = transition->center.x;
	cur_arc->point->next->y = transition->center.y;
	break;

    default:
	break;
    }
    cur_arc->point->next->next = NULL;
}

/*----------------------------------------------------------------------*/
/*			       RESULTS					*/
/*----------------------------------------------------------------------*/


/*
 * Create a result object in the net.
 */

struct res_object * 
create_res ( double curr_x, double curr_y, const char *format_name, const char *format_result, ... )
{
    va_list args;
    char result_str[BUFSIZ];
    char name_str[BUFSIZ];
    static struct res_object *cur_res;

    va_start( args, format_result );
    (void) vsprintf( name_str, format_name, args );
    va_end( args );
    va_start( args, format_result );
    (void) vsprintf( result_str, format_result, args );
    va_end( args );

    if ( netobj->results == NULL) {
	netobj->results = (struct res_object *) malloc(sizeof (struct res_object));
	cur_res = netobj->results;
    } else {
	cur_res->next = (struct res_object *) malloc(sizeof (struct res_object));
	cur_res = cur_res->next;
    }

    cur_res->tag = strdup32x( name_str );
    cur_res->text = (struct com_object *)malloc( sizeof( struct com_object ) );
    cur_res->text->line = strdup( result_str );
    cur_res->text->next = NULL;
    cur_res->center.x = IN_TO_PIX(curr_x);
    cur_res->center.y = IN_TO_PIX(curr_y);
    cur_res->next = NULL;
    cur_res->value = -1.0;

    return cur_res;
}



/*
 * Move all immediate transitions to group "G1".
 */

typedef struct groupy {
    struct group_object * group;
    struct trans_object ** trans;
} groupies_t;

static groupies_t * group= 0;

void
groupize (void)
{
    struct trans_object * cur_trans;
    struct trans_object ** last_trans;
    short max_group = 0;
    int i;
	
    /* Count up transition types... */
	
    for ( cur_trans = netobj->trans, last_trans = &netobj->trans; cur_trans; cur_trans = cur_trans->next ) {
	switch ( cur_trans->kind ) {
	case DETERMINISTIC:
	case EXPONENTIAL:
	    break;

	default:
	    i = cur_trans->kind - IMMEDIATE;
	    if ( i > max_group ) {
		max_group = i;
	    }
	    break;
	}
    }

    group = (groupies_t *)malloc( sizeof( struct groupy ) * (max_group + 1));
    for ( i = 0; i <= max_group; ++i ) {
	char * buf = (char *)malloc( 32 );
	struct group_object * cur_group = (struct group_object *)malloc( sizeof ( struct group_object ));

	group[i].group      = cur_group;
	group[i].trans	    = &cur_group->trans;
	sprintf( buf, "G%d", i+1 );

	cur_group->tag      = buf;
	cur_group->pri	    = i+1;
	cur_group->center.x = 0;
	cur_group->center.y = 0;
	cur_group->trans    = NULL;
	cur_group->movelink = NULL;
	cur_group->next     = NULL;
    }
    netobj->groups 	 = group[0].group;

    for ( cur_trans = netobj->trans, last_trans = &netobj->trans; cur_trans; cur_trans = cur_trans->next ) {
	switch ( cur_trans->kind ) {
	case DETERMINISTIC:
	case EXPONENTIAL:
	    last_trans = &cur_trans->next;
	    break;

	default:
	    i = cur_trans->kind - IMMEDIATE;
			
	    *group[i].trans = cur_trans;
	    *last_trans     = cur_trans->next;
	    group[i].trans  = &cur_trans->next;
	    break;

	}

    }


    *group[0].trans = NULL;
    for ( i = 1; i <= max_group; ++i ) {
	*group[i].trans = NULL;
	group[i-1].group->next = group[i].group;
    }

    *last_trans  = NULL;
}


void
free_group_store()
{
    if ( group ) {
	free( group );
    }
    group = 0;
}


/*
 * Shift all rpars.  Rate parameters must be created before
 * transitions, so we have to shift them once we know where they go.
 */

void
shift_rpars( double offset_x, double offset_y )
{
    struct rpar_object *cur_rpar;
    struct res_object *cur_res;
	
    for ( cur_rpar = netobj->rpars; cur_rpar; cur_rpar = cur_rpar->next ) {
	cur_rpar->center.x += IN_TO_PIX( offset_x );
    }

    for ( cur_res = netobj->results; cur_res; cur_res = cur_res->next ) {
	cur_res->center.x += IN_TO_PIX( offset_x );
    }
}

/*----------------------------------------------------------------------*/
/*             Name munging due to limits in GreatSPN.                  */
/* Names that are larger than MAX_NAME_SIZE are hashed.  A new name     */
/* is then generated for the GreatSPN solver.  GreatSPN limits names    */
/* to 32 characters in length.                                          */
/*----------------------------------------------------------------------*/

/*
 * Check length of string before reallocating.  Remove underscores.
 * GreatSPN is sooo picky...
 */

char *
strdup32x( const char * aStr )
{
    if ( strchr( aStr, '_' ) != 0 ) {
	char * buf = (char *)malloc( strlen( aStr ) + 1 );
	char * p = buf;		/* destination of fixed string 	*/
	const char * q = aStr;	/* Source from which we strip	*/
	for ( ; *q; ++q ) {
	    if ( isalnum( *q ) ) *p++ = *q;
	}
	*p++ = '\0';		/* Null terminate the sucker.	*/
	p = add_to_hash_table( aStr, buf );
	free( buf );
	return p;
    } else if ( strlen( aStr ) > MAX_NAME_SIZE ) {
	return add_to_hash_table( aStr, 0 );
    } else {
	return strdup( aStr );
    }
}


/*
 * Add aStr to the hash table and return a magical string.
 */
 
static char *
add_to_hash_table( const char * key, char * aStr )
{
    int i = hash( key );
    char buf[33];
    unsigned count;
	
    if ( i == -1 ) {
	LQIO::solution_error( FTL_TAG_TABLE_FULL );
	exit( EXCEPTION_EXIT );
    }

    hash_table[i] = strdup( key );

    if ( aStr ) {
	strncpy( buf, aStr, MAX_NAME_SIZE );
    } else {
	strncpy( buf, key, MAX_NAME_SIZE );
    }
    buf[MAX_NAME_SIZE] = 0;
    count = strlen( buf );
    if ( count > MAX_NAME_SIZE ) count = MAX_NAME_SIZE;
    sprintf( buf+count, "%d", i );
    return strdup( buf );
}


/*
 * Hash a string.
 */

static int
hash ( const char * aStr  )
{
    const char * p;
    unsigned long i;
    unsigned long index;
	
    for ( p = aStr, i = 0; *p; ++p ) {
	i += (unsigned)*p;
    }

    i %= HASH_TABLE_SIZE;
    index = i;

    do {
	if ( !hash_table[i] || strcmp( hash_table[i], aStr ) == 0 ) {
	    return i;
	} else {
	    i = (i + 1) % HASH_TABLE_SIZE;
	}
    } while ( i != index );

    return -1;
}



/*
 * compare str1 to str2.  Hash str2 if necessary.
 */

static int
strhcmp( const char * str1, const char * str2 )
{
    if ( strlen( str2 ) > MAX_NAME_SIZE || strchr( str2, '_' ) ) {
	char buf[BUFSIZ];
	strcpy( buf, str2 );
	hash_name( buf );
	return strcmp( str1, buf );
    } else {
	return strcmp( str1, str2 );
    }
}



/*
 * Translate the name into something found in the hash table.
 */

void
hash_name( char * aStr )
{
    int i = hash( aStr );
    bool dohash = strlen( aStr ) > MAX_NAME_SIZE;
    char * p = aStr;		/* destination of fixed string 	*/
    char * q = aStr;		/* Source from which we strip	*/
    for ( ; *q; ++q ) {
	if ( isalnum( *q ) ) {
	    *p++ = *q;
	} else {
	    dohash = true;
	}
    }
    *p++ = '\0';		/* Null terminate the sucker.	*/
    if ( dohash  ) {
	int count;
	aStr[MAX_NAME_SIZE] = 0;
	count = strlen( aStr );
	sprintf( aStr+count, "%d", i );
    }
}


/*
 * Reset all entries in hash table.
 */

void
clear_hash_table()
{
    int i;
    for ( i = 0; i < HASH_TABLE_SIZE; ++i ) {
	if ( hash_table[i] ) {
	    free( hash_table[i] );
	    hash_table[i] = 0;
	}
    }
}
