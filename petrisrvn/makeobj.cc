/************************************************************************/
/* Copyright the Real-Time and Distributed Systems Group,		*/
/* Department of Systems and Computer Engineering,			*/
/* Carleton University, Ottawa, Ontario, Canada. K1S 5B6		*/
/* 									*/
/* September 1991.							*/
/* August 2003.								*/
/************************************************************************/

/*
 * $Id: makeobj.cc 17069 2024-02-27 23:16:21Z greg $
 *
 * Make various model objects.
 */

#include "petrisrvn.h"
#include <map>
#include <cstdlib>
#include <cstdarg>
#include <cstring>
#include <string>
#include <sstream>
#include <iomanip>
#include <stdexcept>
#if HAVE_MALLOC_H
#include <malloc.h>
#endif
#include <cassert>
#include <ctype.h>
#include <wspnlib/global.h>
#include <wspnlib/wspn.h>
#include <lqio/glblerr.h>
#include "errmsg.h"
#include "makeobj.h"

std::map<std::string,std::string> netobj_name_table;

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
    (void) vsnprintf( name, BUFSIZ, format, args );
    va_end( args );

    if ( !netobj->places ) {
	netobj->places = (struct place_object *) malloc(sizeof (struct place_object ));
	cur_place = netobj->places;
    } else {
	cur_place->next = (struct place_object *) malloc(sizeof (struct place_object));
	cur_place = cur_place->next;
    }
    cur_place->tag      = strdup( insert_netobj_name( name ).c_str() );
    cur_place->m0       = tokens;
    cur_place->tokens   = tokens;
    cur_place->color    = nullptr;
    cur_place->lisp     = nullptr;
    cur_place->distr    = nullptr;
    cur_place->mpar     = nullptr;
    cur_place->cmark    = nullptr;
    cur_place->layer    = WHOLE_NET_LAYER | layer;

    /* Coordinates */

    move_place_tag( move_place( cur_place, curr_x, curr_y ), 0.25, 0.0 );

    cur_place->next  = nullptr;

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
    (void) vsnprintf( name, BUFSIZ, format, args );
    va_end( args );

    if ( netobj->rpars == nullptr) {
	netobj->rpars = (struct rpar_object *) malloc(sizeof (struct rpar_object ));
	cur_rpar = netobj->rpars;
    } else {
	cur_rpar->next = (struct rpar_object *) malloc(sizeof (struct rpar_object ));
	cur_rpar = cur_rpar->next;
    }

    cur_rpar->tag      = strdup( insert_netobj_name( name ).c_str() );
    cur_rpar->value    = rate;
    cur_rpar->center.x = IN_TO_PIX(curr_x);
    cur_rpar->center.y = IN_TO_PIX(curr_y);
    cur_rpar->layer    = WHOLE_NET_LAYER | layer;
    cur_rpar->next     = nullptr;

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
    (void) vsnprintf( name, BUFSIZ, format, args );
    va_end( args );

    for ( cur_rpar = netobj->rpars, i = 1; cur_rpar != nullptr; cur_rpar = cur_rpar->next, ++i) {
	if (strcmp(cur_rpar->tag, name ) == 0) {
	    return i;
	}
    }
    return 0;
}


/*
 * Return the place object.  The tag name of the place is found in the netobj_name_table.
 */

struct place_object *
no_place( const char * format, ... )
{
    va_list args;
    char name[BUFSIZ];

    struct place_object * cur_place;

    va_start( args, format );
    (void) vsnprintf( name, BUFSIZ, format, args );
    va_end( args );

    std::map<std::string,std::string>::const_iterator i = netobj_name_table.find( name );
    if ( i != netobj_name_table.end() ) {
	for ( cur_place = netobj->places; cur_place != nullptr; cur_place = cur_place->next ) {
	    if ( i->second == cur_place->tag ) {
		return cur_place;
	    }
	}
    }
    throw std::runtime_error( std::string( "No place named: " ) + name );
    return nullptr;
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
    (void) vsnprintf( name, BUFSIZ, format, args );
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
    (void) vsnprintf( name, BUFSIZ, format, args );
    va_end( args );

    cur_trans->next  = nullptr;

    if ( netobj->trans == nullptr) {
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

    cur_trans->tag	        = strdup( insert_netobj_name( name ).c_str() );	/* Need a copy since it is freed in wspnlib */
    cur_trans->fire_rate.ff 	= rate;
    cur_trans->kind     	= kind;
    cur_trans->enabl		= enable;		/* 0 == infinite server */

    cur_trans->color		= nullptr;
    cur_trans->lisp		= nullptr;
    cur_trans->Lbound    	= 0;
    cur_trans->Ebound    	= 0;
    cur_trans->Rbound    	= 0;
    cur_trans->orient       	= 0;
    cur_trans->rpar		= nullptr;
    cur_trans->mark_dep 	= nullptr;
    cur_trans->layer    	= WHOLE_NET_LAYER | layer;

    /* Coordinates */

    move_trans_tag( move_trans( cur_trans, x_pos, y_pos ), 0.125, -0.125 );

    return cur_trans;
}


/*
 * Change orientation of trans.  Return the first argument.
 * The default (horizontal) is zero.  one is vertical.
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
    (void) vsnprintf( name, BUFSIZ, format, args );
    va_end( args );

    std::map<std::string,std::string>::const_iterator i = netobj_name_table.find( name );
    if ( i != netobj_name_table.end() ) {
	for ( cur_trans = netobj->trans; cur_trans != nullptr; cur_trans = cur_trans->next ) {
	    if ( i->second == cur_trans->tag ) {
		return cur_trans;
	    }
	}
    }
    std::string msg = "No transition named: ";
    msg += name;
    throw std::runtime_error( msg.c_str() );
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
create_arc (LAYER layer, int type, const struct trans_object *transition, const struct place_object *place)
{
    create_arc_mult( layer, type, transition, place, 1 );
}


void
create_arc_mult(LAYER layer, int type, const struct trans_object *transition, const struct place_object *place, short mult )
{
    static struct arc_object *cur_arc;

    assert( transition != nullptr );
    assert( place != nullptr );

    if ( netobj->arcs == nullptr ) {
	netobj->arcs = (struct arc_object *) malloc( sizeof( struct arc_object ) );
	cur_arc = netobj->arcs;
    } else {
	cur_arc->next = (struct arc_object *) malloc( sizeof( struct arc_object ) );
	cur_arc = cur_arc->next;
    }

    cur_arc->type  = type;
    cur_arc->mult  = mult;
    cur_arc->layer = WHOLE_NET_LAYER | layer;
    cur_arc->place = const_cast<struct place_object *>(place);
    cur_arc->trans = const_cast<struct trans_object *>(transition);
    cur_arc->color = nullptr;
    cur_arc->lisp  = nullptr;
    cur_arc->next  = nullptr;

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
    cur_arc->point->next->next = nullptr;
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
    (void) vsnprintf( name_str, BUFSIZ, format_name, args );
    va_end( args );
    va_start( args, format_result );
    (void) vsnprintf( result_str, BUFSIZ, format_result, args );
    va_end( args );

    if ( netobj->results == nullptr) {
	netobj->results = (struct res_object *) malloc(sizeof (struct res_object));
	cur_res = netobj->results;
    } else {
	cur_res->next = (struct res_object *) malloc(sizeof (struct res_object));
	cur_res = cur_res->next;
    }

    cur_res->tag = strdup( insert_netobj_name( name_str ).c_str() );
    cur_res->text = (struct com_object *)malloc( sizeof( struct com_object ) );
    cur_res->text->line = strdup( result_str );
    cur_res->text->next = nullptr;
    cur_res->center.x = IN_TO_PIX(curr_x);
    cur_res->center.y = IN_TO_PIX(curr_y);
    cur_res->next = nullptr;
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
	snprintf( buf, 32, "G%d", i+1 );

	cur_group->tag      = buf;
	cur_group->pri	    = i+1;
	cur_group->center.x = 0;
	cur_group->center.y = 0;
	cur_group->trans    = nullptr;
	cur_group->movelink = nullptr;
	cur_group->next     = nullptr;
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


    *group[0].trans = nullptr;
    for ( i = 1; i <= max_group; ++i ) {
	*group[i].trans = nullptr;
	group[i-1].group->next = group[i].group;
    }

    *last_trans  = nullptr;
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

const std::string&
insert_netobj_name( const std::string& name )
{
    std::map<std::string,std::string>::const_iterator i = netobj_name_table.find( name );
    if ( i == netobj_name_table.end() ) {
	bool hash = false;
	std::ostringstream buf;
	std::string::const_iterator src;
	for ( src = name.begin(); src != name.end() && buf.str().size() < 6; ++src ) {
	    if ( isalnum(*src) ) {
		buf << *src;
	    } else {
		hash = true;
	    }
	}
	if ( hash || src != name.end() ) {
	    buf << std::setw( 4 ) << std::setfill( '0' ) << netobj_name_table.size();
	}
	std::pair<std::map<std::string,std::string>::const_iterator,bool> result = netobj_name_table.insert( std::pair<std::string,std::string>( name, buf.str() ) );
	i = result.first;
    }
    return i->second;		/* Old GreatSPN code... */
}


char *
find_netobj_name( const std::string& name )
{
    std::map<std::string,std::string>::const_iterator i = netobj_name_table.find( name );
    if ( i != netobj_name_table.end() ) {
	return const_cast<char *>(i->second.c_str());
    } else {
	throw std::runtime_error( std::string( "No object named: " ) + name );
    }
    return nullptr;
}
