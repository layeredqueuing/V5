/* actlayer.cc	-- Greg Franks Mon Apr  7 2003
 *
 * Layering logic for activities.
 *
 * $Id: actlayer.cc 11963 2014-04-10 14:36:42Z greg $
 */



#include "actlayer.h"
#include <cstdlib>
#include "activity.h"
#include "actlist.h"
#include <cstdlib>
#include <limits.h>
#if HAVE_VALUES_H
#include <values.h>
#endif
#if HAVE_FLOAT_H
#include <float.h>
#endif

#if !defined(MAXDOUBLE)
#define MAXDOUBLE FLT_MAX
#endif

/*----------------------------------------------------------------------*/
/*                         Helper Functions                             */
/*----------------------------------------------------------------------*/

/*
 * Print all results.
 */

ostream&
operator<<( ostream& output, const ActivityLayer& self )
{
    return self.print( output );
}

ActivityLayer& 
ActivityLayer::operator<<( Activity * elem )
{
    myActivities << elem;
    return *this;
}


ActivityLayer&
ActivityLayer::operator+=( Activity * elem )
{
    myActivities += elem;
    return *this;
}



ActivityLayer&
ActivityLayer::clearContents()
{
    myActivities.chop( myActivities.size() );
    return *this;
}


ActivityLayer const&
ActivityLayer::sort( compare_func_ptr compare ) const
{
    myActivities.sort( compare );
    Sequence<Activity *> nextActivity( activities() );

    /* Resort incoming lists */

    Activity * anActivity;
    while ( anActivity = nextActivity() ) {
	if ( anActivity->inputFromList ) {
	    anActivity->inputFromList->sort( &Activity::compareCoord );
	}
	if ( anActivity->outputToList ) {
	    anActivity->outputToList->sort( compare );
	}
    }

    return *this;
}



ActivityLayer const& 
ActivityLayer::format( const double y ) const
{
    origin.moveTo( 0, y );
    extent.moveTo( 0, 0 );

    Sequence<Activity *> nextActivity( activities() );
    Activity * anActivity;
    for ( double x = 0; anActivity = nextActivity(); x += Flags::act_x_spacing ) {
	anActivity->moveTo( x, y );
	x += anActivity->width();
	extent.moveTo( x, max( extent.y(), anActivity->height() ) );
    }
    return *this;
}


ActivityLayer const& 
ActivityLayer::reformat( const double ) const
{
    extent.moveTo( 0, 0 );

    Sequence<Activity *> nextActivity( activities() );
    Activity * anActivity;
    for ( double x = 0; anActivity = nextActivity(); x += Flags::act_x_spacing ) {
	anActivity->moveTo( x, anActivity->bottom() );
	x += anActivity->width();
	extent.moveTo( x, max( extent.y(), anActivity->height() ) );
    }
    return *this;
}


ActivityLayer const&
ActivityLayer::label() const
{
    Sequence<Activity *> nextActivity( activities() );
    Activity * anActivity;
    while ( anActivity = nextActivity() ) {
	anActivity->label();
    }
    return *this;
}


ActivityLayer const&
ActivityLayer::moveTo( const double x, const double y )  const
{
    const double dx = x - origin.x();
    const double dy = y - origin.y();
    origin.moveTo( x, y );

    Sequence<Activity *> nextActivity( activities() );
    Activity * anActivity;
    while ( anActivity = nextActivity() ) {
	anActivity->moveBy( dx, dy );
    }
    return *this;
}



ActivityLayer const&
ActivityLayer::moveBy( const double dx, const double dy )  const
{
    origin.moveBy( dx, dy );

    Sequence<Activity *> nextActivity( activities() );
    Activity * anActivity;
    while ( anActivity = nextActivity() ) {
	anActivity->moveBy( dx, dy );
    }
    return *this;
}



ActivityLayer const&
ActivityLayer::scaleBy( const double sx, const double sy ) const
{
    Sequence<Activity *> nextActivity( activities() );
    Activity * anActivity;
    while  ( anActivity = nextActivity() ) {
	anActivity->scaleBy( sx, sy );
    }
    extent.scaleBy( sx, sy );
    origin.scaleBy( sx, sy );
    return *this;
}



ActivityLayer const&
ActivityLayer::translateY( const double dy )  const
{
    Sequence<Activity *> nextActivity( activities() );
    Activity * anActivity;
    while ( anActivity = nextActivity() ) {
	anActivity->translateY( dy );
    }
    origin.y( dy - origin.y() );
    return *this;
}



ActivityLayer const&
ActivityLayer::depth( const unsigned layer ) const
{
    Sequence<Activity *> nextActivity( activities() );
    Activity * anActivity;
    while ( anActivity = nextActivity() ) {
	anActivity->depth( layer );
    }
    return *this;
}



ActivityLayer const&
ActivityLayer::justify( const double maxWidthPts, const justification_type justification ) const
{
    switch ( justification ) {
    case ALIGN_JUSTIFY:
    case DEFAULT_JUSTIFY:
    case CENTER_JUSTIFY:
	moveBy( (maxWidthPts - width())/2, 0.0 );
	break;
    case RIGHT_JUSTIFY:
	moveBy( (maxWidthPts - width()), 0.0 );
	break;
    case LEFT_JUSTIFY:
	moveBy( 0, 0.0 );		/* Force recomputation of slopes */
	break;
    default:
	abort();	
    }
    return *this;
}


/*
 * Align activities between layers.
 */

ActivityLayer const&
ActivityLayer::alignActivities() const
{
    myActivities.sort( &Activity::compareCoord );
    const unsigned n = size();

    /* Move objects right starting from the right side */
    for ( unsigned int i = n; i > 0; --i ) {
	shift( i, myActivities[i]->align() );
    }

    /* Move objects left starting from the left side */
    for ( unsigned int i = 1; i <= n; ++i ) {
	shift( i, myActivities[i]->align() );
    }

    return *this;
}


ActivityLayer const&
ActivityLayer::shift( unsigned i, double amount ) const
{
    unsigned j;

    if ( amount < 0.0 ) {
	/* move left if I can */
	while ( i > 1 && myActivities[i-1]->inputFromList == myActivities[i]->inputFromList ) --i;	/* Search for left */
	if ( i > 1 ) {
	    amount = min( max( (myActivities[i-1]->right() + Flags::act_x_spacing) - myActivities[i]->left(), amount ), 0.0 );
	}
	for ( j = i; j <= size() && myActivities[j]->inputFromList == myActivities[i]->inputFromList; ++j ) {
	    myActivities[j]->moveBy( amount, 0 );
	}
	if ( j > size() ) {
	    if ( i == 1 ) {
		origin.moveBy( amount, 0 );
	    } else {
		extent.moveBy( amount, 0 );
	    }
	} else if ( i == 1 ) {
	    origin.moveBy( amount, 0 );
	    extent.moveBy( -amount, 0 );
	}
    } else if ( amount > 0.0 ) { 
	/* move right if I can */
	while ( i < size() && myActivities[i+1]->inputFromList == myActivities[i]->inputFromList ) ++i;	/* Search for right */
	if ( i < size() ) {
	    amount = max( min( myActivities[i+1]->left() - (myActivities[i]->right() + Flags::act_x_spacing), amount ), 0 );
	} 
	for ( j = i; j >= 1 && myActivities[j]->inputFromList == myActivities[i]->inputFromList; --j ) {
	    myActivities[j]->moveBy( amount, 0 );
	}
	if ( i == size() ) {
	    if ( j == 0 ) {
		origin.moveBy( amount, 0 );
	    } else {
		extent.moveBy( amount, 0 );
	    }
	} else if ( j == 0 ) {
	    origin.moveBy( amount, 0 );
	    extent.moveBy( -amount, 0 );
	}
    }
    return *this;
}



ActivityLayer const&
ActivityLayer::crop() const
{
    Sequence<Activity *> nextActivity(myActivities);
    Activity * anActivity;

    double left  = MAXDOUBLE;
    double right = 0.;
    while ( anActivity = nextActivity() ) {
	left  = min( left, anActivity->left() );
	right = max( right, anActivity->right() );
    }	
    origin.x( left );
    extent.x( right - left );
    return *this;
}


ostream&
ActivityLayer::print( ostream& output ) const
{
    return output;
}
