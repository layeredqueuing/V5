/* actlayer.cc	-- Greg Franks Mon Apr  7 2003
 *
 * Layering logic for activities.
 *
 * $Id: actlayer.cc 16967 2024-01-28 20:33:35Z greg $
 */



#include <algorithm>
#include <cstdlib>
#include <limits>
#include "actlayer.h"
#include "activity.h"
#include "actlist.h"

ActivityLayer::ActivityLayer( const ActivityLayer& src ) :
    myActivities(src.myActivities),
    origin(src.origin),
    extent(src.extent)
{
}

/*----------------------------------------------------------------------*/
/*                         Helper Functions                             */
/*----------------------------------------------------------------------*/

ActivityLayer&
ActivityLayer::operator+=( Activity * elem )
{
    myActivities.push_back( elem );
    return *this;
}



ActivityLayer&
ActivityLayer::clearContents()
{
    myActivities.clear();
    return *this;
}


ActivityLayer&
ActivityLayer::sort( compare_func_ptr compare ) 
{
    std::sort( myActivities.begin(), myActivities.end(), compare );

    /* Resort incoming lists */

    for ( std::vector<Activity *>::const_iterator activity = activities().begin(); activity != activities().end(); ++activity ) {
	if ( (*activity)->inputFrom() ) {
	    (*activity)->_inputFrom->sort( (compare_func_ptr)(&Activity::compareCoord) );
	}
	if ( (*activity)->outputTo() ) {
	    (*activity)->_outputTo->sort( compare );
	}
    }

    return *this;
}



ActivityLayer&
ActivityLayer::format( const double y )
{
    origin.moveTo( 0, y );
    extent.moveTo( 0, 0 );

    double x = 0; 
    for ( std::vector<Activity *>::const_iterator activity = activities().begin(); activity != activities().end(); ++activity ) {
	(*activity)->moveTo( x, y );
	x += (*activity)->width();
	extent.moveTo( x, std::max( extent.y(), (*activity)->height() ) );
	x += Flags::act_x_spacing;
    }
    return *this;
}


ActivityLayer&
ActivityLayer::reformat( const double )
{
    extent.moveTo( 0, 0 );

    double x = 0; 
    for ( std::vector<Activity *>::const_iterator activity = activities().begin(); activity != activities().end(); ++activity ) {
	(*activity)->moveTo( x, (*activity)->bottom() );
	x += (*activity)->width();
	extent.moveTo( x, std::max( extent.y(), (*activity)->height() ) );
	x += Flags::act_x_spacing;
    }
    return *this;
}


ActivityLayer&
ActivityLayer::label()
{
    return *this;
}


ActivityLayer&
ActivityLayer::moveTo( const double x, const double y )
{
    const double dx = x - origin.x();
    const double dy = y - origin.y();
    origin.moveTo( x, y );

    std::for_each( activities().begin(), activities().end(), [=]( Activity * activity ){ activity->moveBy( dx, dy ); } );
    return *this;
}



ActivityLayer&
ActivityLayer::moveBy( const double dx, const double dy )
{
    origin.moveBy( dx, dy );
    std::for_each( activities().begin(), activities().end(), [=]( Activity * activity ){ activity->moveBy( dx, dy ); } );
    return *this;
}



ActivityLayer&
ActivityLayer::scaleBy( const double sx, const double sy )
{
    extent.scaleBy( sx, sy );
    origin.scaleBy( sx, sy );
    std::for_each( activities().begin(), activities().end(), [=]( Activity * activity ){ activity->scaleBy( sx, sy ); } );
    return *this;
}



ActivityLayer&
ActivityLayer::translateY( const double dy )
{
    origin.y( dy - origin.y() );
    std::for_each( activities().begin(), activities().end(), [=]( Activity * activity ){ activity->translateY( dy ); } );
    return *this;
}



ActivityLayer&
ActivityLayer::depth( const unsigned depth )
{
    std::for_each( activities().begin(), activities().end(), [=]( Activity * activity ){ activity->depth( depth ); } );
    return *this;
}



ActivityLayer&
ActivityLayer::justify( const double maxWidthPts, const Justification justification )
{
    switch ( justification ) {
    case Justification::ALIGN:
    case Justification::DEFAULT:
    case Justification::CENTER:
	moveBy( (maxWidthPts - width())/2, 0.0 );
	break;
    case Justification::RIGHT:
	moveBy( (maxWidthPts - width()), 0.0 );
	break;
    case Justification::LEFT:
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

ActivityLayer&
ActivityLayer::alignActivities()
{
    std::sort( myActivities.begin(), myActivities.end(), &Activity::compareCoord );
    const unsigned n = size();

    /* Move objects right starting from the right side */
    for ( unsigned int i = n; i > 0;) {
	i -= 1;
	shift( i, myActivities[i]->align() );
    }

    /* Move objects left starting from the left side */
    for ( unsigned int i = 0; i < n; ++i ) {
	shift( i, myActivities[i]->align() );
    }

    return *this;
}


ActivityLayer&
ActivityLayer::shift( unsigned i, double amount )
{
    const unsigned int n = size();
    unsigned int j;
    if ( amount < 0.0 ) {
	/* move left if I can */
	while ( i > 0 && myActivities[i-1]->inputFrom() == myActivities[i]->inputFrom() ) --i;	/* Search for left */
	if ( i > 0 ) {
	    amount = std::min( std::max( (myActivities[i-1]->right() + Flags::act_x_spacing) - myActivities[i]->left(), amount ), 0.0 );
	}
	for ( j = i; j < n && myActivities[j]->inputFrom() == myActivities[i]->inputFrom(); ++j ) {
	    myActivities[j]->moveBy( amount, 0 );
	}
	if ( j == n ) {
	    if ( i == 0 ) {
		origin.moveBy( amount, 0 );
	    } else {
		extent.moveBy( amount, 0 );
	    }
	} else if ( i == 0 ) {
	    origin.moveBy( amount, 0 );
	    extent.moveBy( -amount, 0 );
	}
    } else if ( amount > 0.0 ) { 
	/* move right if I can */
	i += 1;
	while ( i < n && myActivities[i]->inputFrom() == myActivities[i-1]->inputFrom() ) ++i;	/* Search for right */
	if ( i < n ) {
	    amount = std::max( std::min( myActivities[i]->left() - (myActivities[i]->right() + Flags::act_x_spacing), amount ), 0.0 );
	} 
	for ( j = i; j > 0 && myActivities[j-1]->inputFrom() == myActivities[i-1]->inputFrom(); ) {
	    j -= 1;
	    myActivities[j]->moveBy( amount, 0 );
	}
	if ( i == n ) {
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



ActivityLayer&
ActivityLayer::crop()
{
    double left  = std::numeric_limits<double>::max();
    double right = 0.;
    for ( std::vector<Activity *>::const_iterator activity = activities().begin(); activity != activities().end(); ++activity ) {
	left  = std::min( left, (*activity)->left() );
	right = std::max( right, (*activity)->right() );
    }	
    origin.x( left );
    extent.x( right - left );
    return *this;
}


std::ostream&
ActivityLayer::print( std::ostream& output ) const
{
    return output;
}
