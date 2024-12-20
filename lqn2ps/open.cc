/* open.cc	-- Greg Franks Tue Feb 18 2003
 *
 * $Id: open.cc 17368 2024-10-15 21:03:38Z greg $
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
    _paths = source->paths();
    assert ( source->requestType() == request_type::OPEN_ARRIVAL );
    OpenArrival * call = new OpenArrival(this,source);
    addSrcCall( call );
    const_cast<Entry *>(source)->addDstCall( call );
    __source.push_back( this );

    _node = Node::newNode( Flags::icon_width, Flags::graphical_output_style == Output_Style::TIMEBENCH ? Flags::icon_height : Flags::entry_height );
    _label = Label::newLabel();
}


/*
 * Free resources allocated here.
 */

OpenArrivalSource::~OpenArrivalSource()
{
    std::for_each( _calls.begin(), _calls.end(), []( OpenArrival * openArrival ){ delete openArrival; } );
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
	return entry().owner()->isSelected();
    }
}



/*
 * Return all servers to this entry.
 */

unsigned
OpenArrivalSource::servers( std::vector<Entity *> &servers ) const
{
    if ( std::find_if( servers.begin(), servers.end(), [=]( Element * server ){ return server == entry().owner(); } ) == servers.end() ) {
	servers.push_back( const_cast<Task *>(entry().owner()) );
    }
    return servers.size();
}


void
OpenArrivalSource::addSrcCall( OpenArrival * call )
{
    _calls.push_back( call );
}


void
OpenArrivalSource::removeSrcCall( OpenArrival * call )
{
    std::vector<OpenArrival *>::iterator pos = std::find( _calls.begin(), _calls.end(), call ) ;
    if ( pos != _calls.end() ) {
	_calls.erase( pos );
    }
}


/*
 * 
 */

bool
OpenArrivalSource::isInOpenModel( const std::vector<Entity *>& servers ) const
{
    return std::any_of( servers.begin(), servers.end(), [=]( const Element * server ){ return server == entry().owner(); } );
}


unsigned 
OpenArrivalSource::setChain( unsigned k, const callPredicate aFunc ) const
{
    std::for_each( calls().begin(), calls().end(), [=]( GenericCall * call ){ call->setChain( k ); } );
    return k;
}



/*
 * Link the call to the destination task.
 */

OpenArrivalSource&
OpenArrivalSource::aggregate()
{
    switch ( Flags::aggregation() ) {
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
    _node->moveTo( x, y );
    moveSrc( bottomCenter() );
    return *this;
}



OpenArrivalSource&
OpenArrivalSource::moveSrc( const Point& aPoint )
{
    std::for_each( calls().begin(), calls().end(), [=]( OpenArrival* call ){ call->moveSrc( aPoint ); } );
    return *this;
}


/*
 * Move the entity and it's entries.
 */

OpenArrivalSource&
OpenArrivalSource::scaleBy( const double sx, const double sy )
{
    Element::scaleBy( sx, sy );
    std::for_each( calls().begin(), calls().end(), [=]( OpenArrival* call ){ call->scaleBy( sx, sy ); } );
    return *this;
}



/*
 * Move the entity and it's entries.
 */

OpenArrivalSource&
OpenArrivalSource::depth( const unsigned depth )
{
    Element::depth( depth );
    std::for_each( calls().begin(), calls().end(), [=]( OpenArrival* call ){ call->depth( depth ); } );
    return *this;
}



/*
 * Move the entity and it's entries.
 */

OpenArrivalSource&
OpenArrivalSource::translateY( const double dy )
{
    Element::translateY( dy );
    std::for_each( calls().begin(), calls().end(), [=]( GenericCall * call ){ call->translateY( dy ); } );
    return *this;
}



OpenArrivalSource&
OpenArrivalSource::label()
{
    for ( std::vector<OpenArrival *>::const_iterator call = calls().begin(); call != calls().end(); ++call ) {
	if ( queueing_output() ) {
	    bool print_goop = false;
	    if ( Flags::print_input_parameters() ) {
		*_label << (*call)->dstName() << " (" << (*call)->openArrivalRate() << ")";
		_label->newLine();
	    }
	    if ( Flags::have_results && Flags::print[WAITING].opts.value.b ) {
		*_label << (*call)->dstName() << "=" << opt_pct((*call)->openWait());
		_label->newLine();
	    }
	    if ( print_goop ) {
		_label->newLine();
	    }
	} else {
	    (*call)->label();
	}
    }
    return *this;
}


Graphic::Colour 
OpenArrivalSource::colour() const
{
    return entry().colour();
}


const std::string& 
OpenArrivalSource::name() const
{ 
    return entry().name(); 
}


const OpenArrivalSource&
OpenArrivalSource::draw( std::ostream& output ) const
{
    std::string aComment;
    aComment += "========== Open Arrival Source ";
    aComment += name();
    aComment += " ==========";
    _node->comment( output, aComment );

    _node->penColour( colour() == Graphic::Colour::GREY_10 ? Graphic::Colour::BLACK : colour() ).fillColour( colour() );
    _node->circle( output, center(), radius() );

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
    _node->penColour( colour() == Graphic::Colour::GREY_10 ? Graphic::Colour::BLACK : colour() ).fillColour( colour() );
    _node->open_source( output, bottomCenter(), Entity::radius() );
    _label->moveTo( bottomCenter() ).justification( Justification::LEFT );
    _label->moveBy( Entity::radius() * 1, radius() * _node->direction() );
    output << *_label;
    return output;
}
