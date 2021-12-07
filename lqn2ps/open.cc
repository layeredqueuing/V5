/* open.cc	-- Greg Franks Tue Feb 18 2003
 *
 * $Id: open.cc 15170 2021-12-07 23:33:05Z greg $
 */

#include "lqn2ps.h"
#include <algorithm>
#include <string>
#include <string.h>
#include <cmath>
#include <lqio/error.h>
#include "open.h"
#include "call.h"
#include "label.h"
#include "task.h"
#include "entry.h"

std::vector<OpenArrivalSource *> OpenArrivalSource::__source;

/* ------------- Open Arrival Pseudo Tasks (for drawing)  ------------- */

OpenArrivalSource::OpenArrivalSource( Entry * source )
    : Task( 0, 0, 0, std::vector<Entry *>() )
{
    _entries.push_back(source);		/* Owner of entry is orginal task, not this task */
    myPaths = source->paths();
    assert ( source->requestType() == request_type::OPEN_ARRIVAL );
    OpenArrival * aCall = new OpenArrival(this,source);
    _calls.push_back( aCall );
    const_cast<Entry *>(source)->addDstCall( aCall );
    __source.push_back( this );

    myNode = Node::newNode( Flags::icon_width, Flags::graphical_output_style == TIMEBENCH_STYLE ? Flags::icon_height : Flags::entry_height );
    myLabel = Label::newLabel();
}


/*
 * Free resources allocated here.
 */

OpenArrivalSource::~OpenArrivalSource()
{
    std::for_each( _calls.begin(), _calls.end(), Delete<OpenArrival *> );
}

/* ------------------------ Instance Methods -------------------------- */

/*
 * Return true if this entity is selected.
 * See subclasses for further tests.
 */

bool
OpenArrivalSource::isSelectedIndirectly() const
{
    if ( Entity::isSelectedIndirectly() ) {
	return true;
    } else {
	return myEntry().owner()->isSelected();
    }
}



/*
 * Return all servers to this entry.
 */

unsigned
OpenArrivalSource::servers( std::vector<Entity *> &servers ) const
{
    std::vector<Entity *>::iterator pos = find_if( servers.begin(), servers.end(), EQ<Element>(myEntry().owner()) );
    if ( pos == servers.end() ) {
	servers.push_back( const_cast<Task *>(myEntry().owner()) );
    }
    return servers.size();
}


/*
 * 
 */

bool
OpenArrivalSource::isInOpenModel( const std::vector<Entity *>& servers ) const
{
    return std::any_of( servers.begin(), servers.begin(), EQ<Element>(myEntry().owner()) );
}


unsigned 
OpenArrivalSource::setChain( unsigned k, const callPredicate aFunc )
{
    for_each( calls().begin(), calls().end(), Exec1<GenericCall,unsigned int>( &GenericCall::setChain, k ) );
    return k;
}



/*
 * Link the call to the destination task.
 */

OpenArrivalSource&
OpenArrivalSource::aggregate()
{
    switch ( Flags::print[AGGREGATION].opts.value.a ) {
    case Aggregate::ENTRIES:
	for ( std::vector<OpenArrival *>::const_iterator call = calls().begin(); call != calls().end(); ++call ) {
	    Task * dstTask = const_cast<Task *>((*call)->dstTask());
	    dstTask->addDstCall( (*call) );
	}
	break;
    }  	
    return *this;
}

double
OpenArrivalSource::radius() const
{
    return fabs( height() / 9 );
}



OpenArrivalSource&
OpenArrivalSource::moveTo( const double x, const double y )
{
    myNode->moveTo( x, y );
    moveSrc( bottomCenter() );
    return *this;
}



OpenArrivalSource&
OpenArrivalSource::moveSrc( const Point& aPoint )
{
    for_each( calls().begin(), calls().end(), Exec1<GenericCall,const Point &>( &GenericCall::moveSrc, aPoint ) );
    return *this;
}


/*
 * Move the entity and it's entries.
 */

OpenArrivalSource&
OpenArrivalSource::scaleBy( const double sx, const double sy )
{
    Element::scaleBy( sx, sy );
    for_each( calls().begin(), calls().end(), ExecXY<GenericCall>( &GenericCall::scaleBy, sx, sy ) );
    return *this;
}



/*
 * Move the entity and it's entries.
 */

OpenArrivalSource&
OpenArrivalSource::depth( const unsigned depth )
{
    Element::depth( depth );
    for_each( calls().begin(), calls().end(), Exec1<GenericCall,unsigned int>( &GenericCall::depth, depth ) );
    return *this;
}



/*
 * Move the entity and it's entries.
 */

OpenArrivalSource&
OpenArrivalSource::translateY( const double dy )
{
    Element::translateY( dy );
    for_each( calls().begin(), calls().end(), Exec1<GenericCall,double>( &GenericCall::translateY, dy ) );
    return *this;
}



OpenArrivalSource&
OpenArrivalSource::label()
{
    for ( std::vector<OpenArrival *>::const_iterator call = calls().begin(); call != calls().end(); ++call ) {
	if ( queueing_output() ) {
	    bool print_goop = false;
	    if ( Flags::print[INPUT_PARAMETERS].opts.value.b ) {
		*myLabel << (*call)->dstName() << " (" << (*call)->openArrivalRate() << ")";
		myLabel->newLine();
	    }
	    if ( Flags::have_results && Flags::print[WAITING].opts.value.b ) {
		*myLabel << (*call)->dstName() << "=" << opt_pct((*call)->openWait());
		myLabel->newLine();
	    }
	    if ( print_goop ) {
		myLabel->newLine();
	    }
	} else {
	    (*call)->label();
	}
    }
    return *this;
}


Graphic::colour_type 
OpenArrivalSource::colour() const
{
    return myEntry().colour();
}


const std::string& 
OpenArrivalSource::name() const
{ 
    return myEntry().name(); 
}


const OpenArrivalSource&
OpenArrivalSource::draw( std::ostream& output ) const
{
    std::string aComment;
    aComment += "========== Open Arrival Source ";
    aComment += name();
    aComment += " ==========";
    myNode->comment( output, aComment );

    myNode->penColour( colour() == Graphic::GREY_10 ? Graphic::BLACK : colour() ).fillColour( colour() );
    myNode->circle( output, center(), radius() );

    for ( std::vector<OpenArrival *>::const_iterator call = calls().begin(); call != calls().end(); ++call ) {
	output << **call;
    }
    return *this;
}

std::ostream&
OpenArrivalSource::print( std::ostream& output ) const
{
    throw should_not_implement( "OpenArrivalSource::print", __FILE__, __LINE__ );
    return output;
}


/*
 * Draw the queueing model object.
 */

std::ostream&
OpenArrivalSource::drawClient( std::ostream& output, const bool is_in_open_model, const bool is_in_closed_model ) const
{
    myNode->penColour( colour() == Graphic::GREY_10 ? Graphic::BLACK : colour() ).fillColour( colour() );
    myNode->open_source( output, bottomCenter(), Entity::radius() );
    myLabel->moveTo( bottomCenter() ).justification( Justification::LEFT );
    myLabel->moveBy( Entity::radius() * 1, radius() * myNode->direction() );
    output << *myLabel;
    return output;
}
