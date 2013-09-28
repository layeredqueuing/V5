/* open.cc	-- Greg Franks Tue Feb 18 2003
 *
 * $Id$
 */

#include "lqn2ps.h"
#include <string>
#include <string.h>
#include <cmath>
#include <lqio/error.h>
#include "open.h"
#include "cltn.h"
#include "call.h"
#include "label.h"
#include "task.h"
#include "entry.h"

Cltn<OpenArrivalSource *> opensource;


/* ------------- Open Arrival Pseudo Tasks (for drawing)  ------------- */

OpenArrivalSource::OpenArrivalSource( const Entry * source )
    : Entity( 0, 0 ), myEntry(source)
{
    myPaths = myEntry->paths();
    assert ( myEntry->isCalled() == OPEN_ARRIVAL_REQUEST );
    OpenArrival * aCall = new OpenArrival(this,myEntry);
    myCalls << aCall;
    const_cast<Entry *>(myEntry)->addDstCall( aCall );
    opensource << this;
}


/*
 * Free resources allocated here.
 */

OpenArrivalSource::~OpenArrivalSource()
{
    myCalls.deleteContents();
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
	return myEntry->owner()->isSelected();
    }
}



OpenArrivalSource& 
OpenArrivalSource::processor( const Processor * )
{
    throw should_not_implement( "OpenArrivalSource::processor()", __FILE__, __LINE__ );
    return *this;
}



/*
 * Return all servers to this entry.
 */

unsigned
OpenArrivalSource::servers( Cltn<const Entity *> &serversCltn ) const
{
    serversCltn += myEntry->owner();
    return serversCltn.size();
}


/*
 * 
 */

bool
OpenArrivalSource::isInOpenModel( const Cltn<Entity *>& servers ) const
{
    return servers.find( const_cast<Task *>(myEntry->owner()) );
}


unsigned 
OpenArrivalSource::setChain( unsigned k, const callFunc aFunc ) const
{
    Sequence<OpenArrival *> nextCall(myCalls);
    OpenArrival * aCall;

    while ( aCall = nextCall() ) {
	aCall->setChain( k );
    }

    return k;
}



/*
 * Link the call to the destination task.
 */

OpenArrivalSource&
OpenArrivalSource::aggregate()
{
    switch ( Flags::print[AGGREGATION].value.i ) {
    case AGGREGATE_ENTRIES:
	Sequence<OpenArrival *> nextCall(myCalls);
	OpenArrival * aCall;

	while ( aCall = nextCall() ) {
	    Task * dstTask = const_cast<Task *>(aCall->dstTask());
	    dstTask->addDstCall( aCall );
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
    Sequence<OpenArrival *> nextCall(myCalls);
    OpenArrival * aCall;

    while ( aCall = nextCall() ) {
	aCall->moveSrc( aPoint );
    }
    return *this;
}


/*
 * Move the entity and it's entries.
 */

OpenArrivalSource&
OpenArrivalSource::scaleBy( const double sx, const double sy )
{
    Entity::scaleBy( sx, sy );

    Sequence<OpenArrival *> nextCall(myCalls);
    OpenArrival * aCall;

    while ( aCall = nextCall() ) {
	aCall->scaleBy( sx, sy );
    }

    return *this;
}



/*
 * Move the entity and it's entries.
 */

OpenArrivalSource&
OpenArrivalSource::depth( const unsigned depth )
{
    Entity::depth( depth );

    Sequence<OpenArrival *> nextCall(myCalls);
    OpenArrival * aCall;

    while ( aCall = nextCall() ) {
	aCall->depth( depth );
    }

    return *this;
}



/*
 * Move the entity and it's entries.
 */

OpenArrivalSource&
OpenArrivalSource::translateY( const double dy )
{
    Entity::translateY( dy );

    Sequence<OpenArrival *> nextCall(myCalls);
    OpenArrival * aCall;

    while ( aCall = nextCall() ) {
	aCall->translateY( dy );
    }

    return *this;
}



OpenArrivalSource&
OpenArrivalSource::label()
{
    Sequence<OpenArrival *> nextCall(myCalls);
    OpenArrival * aCall;

    while ( aCall = nextCall() ) {
	if ( queueing_output() ) {
	    bool print_goop = false;
	    if ( Flags::print[INPUT_PARAMETERS].value.b ) {
		*myLabel << aCall->dstName() << " (" << aCall->openArrivalRate() << ")";
		myLabel->newLine();
	    }
	    if ( Flags::have_results && Flags::print[WAITING].value.b ) {
		*myLabel << aCall->dstName() << "=" << aCall->openWait();
		myLabel->newLine();
	    }
	    if ( print_goop ) {
		myLabel->newLine();
	    }
	} else {
	    aCall->label();
	}
    }
    return *this;
}


Graphic::colour_type 
OpenArrivalSource::colour() const
{
    return myEntry->colour();
}


const string& 
OpenArrivalSource::name() const
{ 
  return myEntry->name(); 
}


ostream&
OpenArrivalSource::draw( ostream& output ) const
{
    string aComment;
    aComment += "========== Open Arrival Source ";
    aComment += name();
    aComment += " ==========";
    myNode->comment( output, aComment );

    myNode->penColour( colour() == Graphic::GREY_10 ? Graphic::BLACK : colour() ).fillColour( colour() );
    myNode->circle( output, center(), radius() );

    Sequence<OpenArrival *> nextCall(myCalls);
    OpenArrival * aCall;

    while ( aCall = nextCall() ) {
	output << *aCall;
    }
    return output;
}

ostream&
OpenArrivalSource::print( ostream& output ) const
{
    throw should_not_implement( "OpenArrivalSource::print", __FILE__, __LINE__ );
    return output;
}


/*
 * Draw the queueing model object.
 */

ostream&
OpenArrivalSource::drawClient( ostream& output, const bool is_in_open_model, const bool is_in_closed_model ) const
{
    myNode->penColour( colour() == Graphic::GREY_10 ? Graphic::BLACK : colour() ).fillColour( colour() );
    myNode->open_source( output, bottomCenter(), Entity::radius() );
    myLabel->moveTo( bottomCenter() ).justification( LEFT_JUSTIFY );
    myLabel->moveBy( Entity::radius() * 1, radius() * myNode->direction() );
    output << *myLabel;
    return output;
}


#if defined(QNAP_OUTPUT)
/*
 * QNAP queueing network.
 */

ostream&
OpenArrivalSource::printQNAPClient( ostream& output, const bool is_in_open_model, const bool is_in_closed_model, const bool multi_class ) const
{
    output << indent(0) << "name = src" << name() << ";" << endl
	   << indent(0) << "type = source;" << endl
	   << indent(0) << "service = exp(" << 1.0 / myCalls[1]->sumOfSendNoReply() << ");" << endl;
    printQNAPRequests( output, multi_class );
    return output;
}


/*
 * QNAP queueing network.
 */

ostream& 
OpenArrivalSource::printQNAPRequests( ostream& output, const bool multi_class ) const
{
    Sequence<OpenArrival *> nextCall( myCalls );
    OpenArrival * aCall;
    
    bool first = true;
    while ( aCall = nextCall() ) {
	if ( !aCall->isSelected() ) {
	    continue;
	} else if ( first ) {
	    output << indent(0) << "transit" << " = ";		/* transitions must be class independant for a source */
	    first = false;
	} else {
	    output << ", ";
	}
	output << aCall->dstTask()->name() << "," << 1;		/* Always 1. */
    }
    if ( !first ) {
	output << ";" << endl;
    }
    return output;
}
#endif


#if defined(PMIF_OUTPUT)
/*
 * PMIF queueing network.
 */

ostream&
OpenArrivalSource::printPMIFServer( ostream& output ) const
{
    output << indent(0) << "<SourceNode Name=\"" << name() << "-src\"/>" << endl;
    output << indent(0) << "<SinkNode Name=\"" << name() << "-sink\"/>" << endl;
    return output;
}


ostream&
OpenArrivalSource::printPMIFClient( ostream& output ) const
{
    output << indent(+1) << "<OpenWorkLoad WorkLoadName=\"" << open_chain( *this, true, 1 )
	   << "\" ArrivalRate=\"" << myCalls[1]->sendNoReply()
	   << "\" TimeUnits=\"" << "sec" 
	   << "\" ArrivesAt=\"" << name() << "-src"
	   << "\" DepartsAt=\"" << name() << "-sink\">" << endl;
    printPMIFRequests( output );
    output << indent(-1) << "</OpenWorkLoad>" << endl;
    return output;
}


/*
 * PMIF queueing network.
 */

ostream& 
OpenArrivalSource::printPMIFArcs( ostream& output ) const
{
    Sequence<OpenArrival *> nextCall( myCalls );
    OpenArrival * aCall;

    while ( aCall = nextCall() ) {
	if ( !aCall->isSelected() ) continue;
	output << indent(0) << "<Arc FromNode=\"" << aCall->srcName()
	       << "-src\" ToNode=\"" << aCall->dstTask()->name() << "\"/>" << endl;
	output << indent(0) << "<Arc FromNode=\"" << aCall->dstTask()->name()
	       << "\" ToNode=\"" << aCall->srcName() << "-sink\"/>" << endl;
    }
    return output;
}



ostream& 
OpenArrivalSource::printPMIFRequests( ostream& output ) const
{
    Sequence<OpenArrival *> nextCall( myCalls );
    OpenArrival * aCall;

    while ( aCall = nextCall() ) {
	if ( !aCall->isSelected() ) continue;
	output << indent(0) << "<Transit To=\"" << aCall->dstTask()->name() 
	       << "\" Probability=\"" << 1.0
	       << "\"/>" << endl;
    }
    return output;
}
#endif
