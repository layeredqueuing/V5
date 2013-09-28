/* -*- c++ -*-
 * $Id$
 *
 * Everything you wanted to know about a task or processor, but were
 * afraid to ask.
 *
 * Copyright the Real-Time and Distributed Systems Group,
 * Department of Systems and Computer Engineering,
 * Carleton University, Ottawa, Ontario, Canada. K1S 5B6
 *
 * January 2002
 */

#include "lqn2ps.h"
#include <cstring>
#include <cassert>
#include <cmath>
#include <sstream>
#include <cstdlib>
#include <limits.h>
#if HAVE_IEEEFP_H
#include <ieeefp.h>
#endif
#include <lqio/error.h>
#include <lqio/dom_entity.h>
#include <lqio/dom_task.h>
#include <lqio/srvn_output.h>
#include "model.h"
#include "entity.h"
#include "entry.h"
#include "task.h"
#include "processor.h"
#include "errmsg.h"
#include "label.h"
#include "arc.h"

//#define	DEBUG


/* ---------------------- Overloaded Operators ------------------------ */

/*
 * Printing function.
 */

ostream&
operator<<( ostream& output, const Entity& self ) 
{
    switch( Flags::print[OUTPUT_FORMAT].value.i ) {
#if defined(TXT_OUTPUT)
    case FORMAT_TXT:
	self.print( output );
	break;
#endif
    case FORMAT_SRVN:
#if defined(QNAP_OUTPUT)
    case FORMAT_QNAP:
#endif
	break;
    default:
	self.draw( output );
	break;
    }

    return output;
}


/*
 * Comparison function.
 */

int
operator==( const Entity& a, const Entity& b )
{
    return a.name() == b.name();
}

/* ----------------------- Abstract Superclass. ----------------------- */

/*
 * Set me up.
 */

Entity::Entity( const LQIO::DOM::Entity* domEntity, const size_t id )
    : Element( domEntity, id ),
      myLevel(0),
      mySubmodel(0),
      myIndex(UINT_MAX),
      iAmSelected(true),
      iAmASurrogate(false)
{
#if HAVE_REGEX_T
    if ( Flags::print[INCLUDE_ONLY].value.r ) {
	iAmSelected = regexec( Flags::print[INCLUDE_ONLY].value.r, const_cast<char *>(name().c_str()), 0, 0, 0 ) != REG_NOMATCH;
    } else 
#endif
      if ( submodel_output()
	 || queueing_output()
	 || Flags::print[CHAIN].value.i != 0 ) {
	iAmSelected = false;
    }
}

/*
 * Delete all entries associated with this task.
 */

Entity::~Entity()
{	
    delete myNode;
    delete myLabel;
}



/*
 * Compare function for normal layering.
 */

int
Entity::compare( const void * n1, const void *n2 )
{
    const Entity *e1 = *static_cast<Entity **>(const_cast<void *>(n1));
    const Entity *e2 = *static_cast<Entity **>(const_cast<void *>(n2));

    if ( e1->forwardsTo( dynamic_cast<const Task *>(e2) ) ) {
	return -1;
    } else if ( e2->forwardsTo( dynamic_cast<const Task *>(e1) ) ) {
	return 1;
    } else if ( e1->hasForwardingLevel() && !e2->hasForwardingLevel() ) {
	return -1;
    } else if ( e2->hasForwardingLevel() && !e1->hasForwardingLevel() ) {
	return 1;
    } else {
	return Element::compare( e1, e2 );
    }
}


int
Entity::compareLevel( const void * n1, const void *n2 )
{
    const Entity * e1 = *static_cast<Entity **>(const_cast<void *>(n1));
    const Entity * e2 = *static_cast<Entity **>(const_cast<void *>(n2));
    int diff = static_cast<int>(e1->level()) - static_cast<int>(e2->level());
    if ( diff != 0 ) {
	return diff;
    }
    return static_cast<int>( copysign( 1.0, e1->topCenter().x() - e2->topCenter().x() ) );
}



int
Entity::compareCoord( const void * n1, const void *n2 )
{
    const Entity * e1 = *static_cast<Entity **>(const_cast<void *>(n1));
    const Entity * e2 = *static_cast<Entity **>(const_cast<void *>(n2));

    return static_cast<int>(copysign( 1.0, e1->index() - e2->index() ) );
}

/* ------------------------ Instance Methods -------------------------- */

Entity& 
Entity::setCopies( const unsigned anInt )
{ 
    LQIO::DOM::Entity * dom = const_cast<LQIO::DOM::Entity *>(dynamic_cast<const LQIO::DOM::Entity *>(getDOM()));
    dom->setCopiesValue( anInt );
    return *this; 
}


const LQIO::DOM::ExternalVariable&
Entity::copies() const
{
    return *dynamic_cast<const LQIO::DOM::Entity *>(getDOM())->getCopies();
}

unsigned
Entity::fanIn( const Entity * aClient ) const
{
    if ( dynamic_cast<const LQIO::DOM::Task *>(getDOM()) ) {
	return dynamic_cast<const LQIO::DOM::Task *>(getDOM())->getFanIn( aClient->name() );
    } else {
	return aClient->replicas() / replicas();
    }
}

unsigned
Entity::fanOut( const Entity * aServer ) const
{
    if ( dynamic_cast<const LQIO::DOM::Task *>(getDOM()) ) {
	return dynamic_cast<const LQIO::DOM::Task *>(getDOM())->getFanOut( aServer->name() );
    } else {
	return replicas() / aServer->replicas();
    }
}



/*
 * Return true if this is a multiserver.  Variables return true.
 */

bool 
Entity::isInfinite() const
{ 
    return scheduling() == SCHEDULE_DELAY;
}


/*
 * Return true if this is a multiserver.  Variables return true.
 */

bool 
Entity::isMultiServer() const
{ 
    const LQIO::DOM::ExternalVariable * m = dynamic_cast<const LQIO::DOM::Entity *>(getDOM())->getCopies();
    double v;
    return !m->wasSet() || !m->getValue(v) || v > 1.0;
}


const unsigned 
Entity::replicas() const
{
    return dynamic_cast<const LQIO::DOM::Entity *>(getDOM())->getReplicas();
}


const scheduling_type 
Entity::scheduling() const 
{
    return dynamic_cast<const LQIO::DOM::Entity *>(getDOM())->getSchedulingType();
}

//    bool isReplicated() const       { return replicas() > 1; }

/*
 * Return true if this entity is selected.
 * See subclasses for further tests.
 */

bool
Entity::isSelectedIndirectly() const
{
    if ( Flags::print[CHAIN].value.i != 0 && !queueing_output() ) {
	return hasPath( Flags::print[CHAIN].value.i );
    } else { 
	return isSelected();
    }
}



/* 
 * Return true if the utilization of this task is too high.
 */

bool
Entity::hasBogusUtilization() const
{
    return !isInfinite() && utilization() / LQIO::DOM::to_double( copies() )  > 1.05;
}

/*
 * Return the amount I should move by IFF I can be aligned.
 */

double
Entity::align() const
{
    if ( isForwardingTarget() ) return 0.0;		/* Don't align me if I call somebody via forwarding. */

    /* A server aligns with parent */

    Cltn<const Entity *> myClients;
    clients( myClients, &Call::hasAncestorLevel );
    if ( myClients.size() == 1 ) {
	const Entity * myParent = myClients[1];
	return myParent->center().x() - center().x();
    }
    return 0.0;
}



/*
 * Sort entries and activities based on when they were visited.
 */

Entity const &
Entity::sort() const
{
    myCallers.sort( Call::compareDst );

    return *this;
}


/*
 * Set the service time for chain k to s.  If chain k is not found, add it.
 */

Entity& 
Entity::serviceTime( const unsigned k, const double s )
{
    if ( myPaths.size() > myServiceTime.size() ) {
	myServiceTime.grow( myPaths.size() - myServiceTime.size() );
    }

    unsigned i = myPaths.find( k );
    if ( i > 0 ) {
	myServiceTime[i] = s;
    }

    return *this;
}


/*
 * Get the service time for chain k.
 */

double
Entity::serviceTime( const unsigned k ) const
{
    unsigned i = myServerChains.find( k );
    if ( i != 0 ) {
	return myServiceTime[i];
    } else {
	return 0.0;
    }
}


#if defined(REP2FLAT)
Entity& 
Entity::removeReplication() 
{ 
    LQIO::DOM::Entity * dom = const_cast<LQIO::DOM::Entity *>(dynamic_cast<const LQIO::DOM::Entity *>(getDOM()));
    dom->setReplicas(1);
    return *this; 
}
#endif


/*
 * Count the number of calls that match the criteria passed
 */

unsigned
Entity::countCallers() const
{
    unsigned count = 0;

    Sequence<GenericCall *> nextCall( callerList() );
    const GenericCall * aCall;

    while ( aCall = nextCall() ) {
	if ( aCall->isSelected() ) {
	    count += 1;
	}
    }
    return count;
}



/*
 * Return the size of the radius for drawing queueing networks.
 */

double 
Entity::radius() const
{
    return Flags::icon_height * Model::scaling() / 5.0;
}



Graphic::colour_type
Entity::colour() const
{
    if ( isSurrogate() ) {
	return Graphic::GREY_10;
    }
    switch ( Flags::print[COLOUR].value.i ) {
    case COLOUR_RESULTS:
	if ( Flags::have_results ) {
	    const double u = isInfinite() ? 0.0 : utilization() / LQIO::DOM::to_double(copies());
	    if ( u < 0.4 || !Flags::use_colour && u < 0.8 ) {
		return Graphic::DEFAULT_COLOUR;
	    } else if ( Flags::use_colour ) {
		if ( u < 0.5 ) {
		    return Graphic::BLUE;
		} else if ( u < 0.6 ) {
		    return Graphic::GREEN;
		} else if ( u < 0.8 ) {
		    return Graphic::ORANGE;
		} else { 
		    return Graphic::RED;
		}
	    } else {
		return Graphic::GREY_10;
	    }
	}
	break;

    case COLOUR_CLIENTS:
	return (Graphic::colour_type)(myPaths.min() % 7 + 3);

    case COLOUR_LAYERS:
	return (Graphic::colour_type)(level() % 7 + 3);
    }
    return Graphic::DEFAULT_COLOUR;			// No colour.
}



/*
 * Label the node.
 */

Entity&
Entity::label()
{
    myLabel->initialize( name() );
    if ( Flags::print[INPUT_PARAMETERS].value.b ) {
	if ( isMultiServer() ) {
	    *myLabel << " {" << copies() << "}";
	} else if ( isInfinite() ) {
	    *myLabel << " {" << _infty() << "}";
	}
	if ( isReplicated() ) {
	    *myLabel << " <" << replicas() << ">";
	}
    }
    return *this;
}



ostream&
Entity::print( ostream& output ) const
{
    LQIO::SRVN::EntityInput::print( output, dynamic_cast<const LQIO::DOM::Entity *>(getDOM()) );
    return output;
}


/*
 * Draw the queue for the queueing object.
 */

ostream&
Entity::drawServer( ostream& output ) const
{
    string aComment;
    aComment += "========== ";
    aComment += name();
    aComment += " ==========";
    myNode->comment( output, aComment );
    myNode->penColour( colour() == Graphic::GREY_10 ? Graphic::BLACK : colour() ).fillColour( colour() );

    /* Draw the queue. */

    if ( !isInfinite() ) {
	myNode->draw_queue( output, bottomCenter(), radius() );
    }

    /* Draw the server. */

    if ( isMultiServer() || isInfinite() ) {
	myNode->multi_server( output, bottomCenter(), radius() );
    } else {
	Point aPoint = bottomCenter().moveBy( 0, radius() * myNode->direction() );
	myNode->circle( output, aPoint, radius() );
    }

    /* Draw the label */

    myLabel->moveTo( bottomCenter() );
    myLabel->justification( LEFT_JUSTIFY ).moveBy( radius() * 2, radius() * 2 * myNode->direction() );
    output << *myLabel;

    return output;
}



Graphic::colour_type 
Entity::chainColour( unsigned int k ) const
{
    static Graphic::colour_type chain_colours[] = { Graphic::BLACK, Graphic::RED, Graphic::BLUE, Graphic::GREEN, Graphic::ORANGE };

    if ( Flags::print[COLOUR].value.i == COLOUR_CHAINS ) { 
	return chain_colours[k%5];
    } else if ( colour() == Graphic::GREY_10 ) {
	return Graphic::BLACK;
    } else {
	return colour();
    }
}


/*
 * Draw the queueing network
 */

ostream&
Entity::drawQueueingNetwork( ostream& output, const double max_x, const double max_y, Vector<bool> &chain, Cltn<Arc *>& lastArc ) const
{
    Cltn<const Entity *> myClients;
    if ( Flags::print[LAYERING].value.i == LAYERING_CLIENT ) {
	referenceTasks( myClients, const_cast<Entity *>(this) );
    } else { 
	clients( myClients );
    }
    Sequence<const Entity *> nextClient(myClients);
    const Entity * aClient;

    drawServer( output );

    /* find all clients with chains calling me... */

    for ( unsigned i = 1; i <= myServerChains.size(); ++i ) {
	unsigned k = myServerChains[i];

	/* Draw connections to this server */

	while ( aClient = nextClient() ) {
	    if ( !aClient->hasClientChain( k ) ) continue;

	    stringstream aComment;
	    aComment << "---------- Chain " << k << ": " << name() << " -> " <<  aClient->name() << " ----------";
	    myNode->comment( output, aComment.str() );
	    drawServerToClient( output, max_x, max_y, aClient, chain, k, chain.size() );
	    
	    aComment.seekp(17, ios::beg);		// rewind.
	    aComment << k << ": " << aClient->name() << " -> " <<  name() << " ----------";
	    myNode->comment( output, aComment.str() );
	    drawClientToServer( output, aClient, chain, k, chain.size(), lastArc );
	}
    }

    return output;
}



/*
 * From Server to Client
 */

ostream&
Entity::drawServerToClient( ostream& output, const double max_x, const double min_y, const Entity * aClient, 
			    Vector<bool> &chain, const unsigned k, const unsigned max_k ) const
{
    if ( !hasServerChain( k ) ) return output;

    Arc * outArc = Arc::newArc( 6 );
    outArc->scaleBy( Model::scaling(), Model::scaling() ).penColour( chainColour( k ) ).depth( myNode->depth() );
    const double direction = static_cast<double>(myNode->direction());
    const double spacing = Flags::print[Y_SPACING].value.f * Model::scaling();

    if ( aClient->hasClientClosedChain(k) ) {
	const double offset = radius() / 2.5;
	double x = bottomCenter().x() - radius() + myServerChains.find(k) * 2.0 * radius() / ( 1.0 + myServerChains.size() );
	double y = min_y - (radius() + offset * (max_k - k)) * direction;
	(*outArc)[2].moveTo( x, y );

	if ( isMultiServer() || isInfinite() ) {
	    outArc->moveSrc( bottomCenter() );
	} else {
	    Point aPoint = bottomCenter().moveBy( 0, radius() * direction );
	    outArc->moveSrc( x, aPoint.y() );
	    (*outArc)[1] = outArc->srcIntersectsCircle( aPoint, radius() );
	}

	Label * aLabel = 0;
	if ( !chain[k] ) {
	    outArc->arrowhead( Graphic::CLOSED_ARROW );
	    x = max_x + offset * (max_k - k);
	    (*outArc)[3].moveTo( x, y );

	    if ( Flags::flatten_submodel || aClient->isReferenceTask() ) {
		y = aClient->top() + (radius() + (offset * (max_k - k))) * direction;
	    } else {
		y = aClient->top() + (spacing / 4.0 - (offset * (k-1))) * direction;
	    }
	    (*outArc)[4].moveTo( x, y );
	    if ( aClient->myClientOpenChains.size() ) {
		x = aClient->topCenter().moveBy( aClient->radius() * -3.0, 0 ).x();
	    } else {
		x = aClient->topCenter().x();
	    }
	    (*outArc)[5].moveTo( x - radius() + aClient->myClientClosedChains.find(k) * 2.0 * radius() / ( 1.0 + aClient->myClientClosedChains.size() ), y );
		
	    y = aClient->bottomCenter().y() + (2.0 * radius()) * direction;
	    outArc->moveDst( x, y );

	    aLabel = Label::newLabel();
	    aLabel->moveTo( (*outArc)[5] ).moveBy( 2.0 * offset, 0 ).backgroundColour( Graphic::DEFAULT_COLOUR );
	    (*aLabel) << k;
	} else {
	    outArc->arrowhead( Graphic::NO_ARROW );
	    Node * aNode = Node::newNode( 0, 0 );

	    aNode->penColour( chainColour( k ) ).fillColour( chainColour( k ) ).fillstyle( Graphic::FILL_SOLID );
	    aNode->circle( output, (*outArc)[2], Model::scaling() );

	    delete aNode;
	    (*outArc)[3] = (*outArc)[4] = (*outArc)[5] = (*outArc)[6] = (*outArc)[2];
	}
	output << *outArc;
	if ( aLabel ) {
	    output << *aLabel;
	    delete aLabel;
	}
    } 
    if ( aClient->hasClientOpenChain(k) ) {
	double x = bottomCenter().x() - radius() + myServerChains.find(k) * 2.0 * radius() / ( 1.0 + myServerChains.size() );
	double y = min_y - radius() * direction;
	(*outArc)[2].moveTo( x, y );

	/* Only if we're a multiserver.. */
	if ( isMultiServer() || isInfinite() ) {
	    outArc->moveSrc( bottomCenter() );
	} else {
	    Point aPoint = bottomCenter().moveBy( 0, radius() * direction );
	    outArc->moveSrc( x, aPoint.y() );
	    (*outArc)[1] = outArc->srcIntersectsCircle( aPoint, radius() );
	}

	/* From server... */
	x = bottomCenter().x();
	y = min_y - (radius() * direction * 3.5);
	(*outArc)[3].moveTo( x, y );
	(*outArc)[4] = (*outArc)[5] = (*outArc)[6] = (*outArc)[3];
	output << *outArc;		/* Warning -- print can delete points! */
	Point aPoint( x, min_y - (radius() * direction * 3.0) );
	myNode->open_sink( output, aPoint, radius() );
    }
    delete outArc;
    return output;
}


/*
 * From Client to Server
 */

ostream&
Entity::drawClientToServer( ostream& output, const Entity * aClient, Vector<bool> &chain, const unsigned k, 
			    const unsigned max_k, Cltn<Arc *>& lastArc ) const
{
    const unsigned N_POINTS = 5;
    Arc * inArc  = Arc::newArc( N_POINTS );

    inArc->scaleBy( Model::scaling(), Model::scaling() ).depth( myNode->depth() ).penColour( chainColour( k ) );

    const double direction = static_cast<double>(myNode->direction());
    const double offset = radius() / 2.0;

    double x = aClient->bottomCenter().x();
    double y = aClient->bottomCenter().y();

    if ( aClient->myClientOpenChains.size() && aClient->myClientClosedChains.size() ) {
	if ( aClient->myClientOpenChains.find( k ) ) {
	    x += aClient->radius() * 1.5;
	} else {
	    x -= aClient->radius() * 3.0;
	}
    }
    (*inArc)[1].moveTo( x, y );

    /* Adjust for chains */

    if ( aClient->myClientOpenChains.find( k ) ) {
	x = x - radius() + aClient->myClientOpenChains.find(k) * 2.0 * radius() / ( 1.0 + aClient->myClientOpenChains.size() );
    } else {
	x = x - radius() + aClient->myClientClosedChains.find(k) * 2.0 * radius() / ( 1.0 + aClient->myClientClosedChains.size() );
    }
    y -= static_cast<double>(k+1) * offset * direction;
    (*inArc)[2].moveTo( x, y );

    x = bottomCenter().x() - radius() + myServerChains.find(k) * 2.0 * radius() / ( 1.0 + myServerChains.size() );
    (*inArc)[3].moveTo( x, y );

    y = bottomCenter().y() + 4.0 * radius() * myNode->direction();
    (*inArc)[4].moveTo( x, y );

    Label * aLabel = Label::newLabel();
    aLabel->moveTo( (*inArc)[3].x(), ((*inArc)[3].y() + (*inArc)[4].y()) / 2.0 ).backgroundColour( Graphic::DEFAULT_COLOUR );
    (*aLabel) << k;

    if ( isInfinite() ) {
	x = bottomCenter().x();
	y = bottomCenter().y() + 2.0 * radius() * myNode->direction();
	(*inArc)[5].moveTo( x, y );
    } else {
	(*inArc)[5] = (*inArc)[4];
    }

    /* Frobnicate -- chain already exists, so join this chain to that chain. */
    if ( chain[k] ) {
	Node * aNode = Node::newNode( 0, 0 );
	aNode->penColour( chainColour( k ) ).fillColour( chainColour( k ) ).fillstyle( Graphic::FILL_SOLID );

	if ( (*inArc)[3].x() <= (*lastArc[k])[3].x() ) {
	    (*inArc)[1] = (*inArc)[2];
	    (*inArc)[2].x((*lastArc[k])[3].x());
	    (*lastArc[k])[3] = (*inArc)[3];
	} else if ( (*lastArc[k])[3].x() < (*inArc)[3].x() && (*inArc)[3].x() < (*lastArc[k])[2].x() ) {
	    (*inArc)[2].x((*inArc)[3].x());
	    (*inArc)[1] = (*inArc)[2];
	} else if ( (*inArc)[2].x() < (*lastArc[k])[3].x() ) {
	    (*inArc)[2] = (*lastArc[k])[3];
	    (*inArc)[1] = (*inArc)[2];
	} else {
	    (*inArc)[1] = (*inArc)[2];
	}
	if ( (*lastArc[k])[3].x() < (*inArc)[3].x() ) {
	    (*lastArc[k])[3] = (*inArc)[3];
	}
	aNode->circle( output, (*inArc)[1], Model::scaling() );
	delete aNode;

    } else {
	Arc * anArc = Arc::newArc( N_POINTS );
	*anArc = *inArc;
	lastArc[k] = anArc;
    }

    output << *inArc;
    if ( aLabel ) {
	output << *aLabel;
	delete aLabel;
    }

    delete inArc;
    chain[k] = true;

    return output;
}




#if defined(QNAP_OUTPUT)
/*
 * QNAP queueing network.
 */

ostream&
Entity::printQNAPServer( ostream& output, const bool multi_class ) const
{
    output << indent(0) << "name = " << qnap_name( *this ) << ";" << endl;
    if ( isMultiServer() ) {
	output << indent(0) << "type = multiple(" << copies() << ");" << endl;
    } else if ( isInfinite() ) {
	output << indent(0) << "type = infinite;" << endl;
    }
    switch ( scheduling() ) {
    case SCHEDULE_FIFO: output << indent(0) << "sched = fifo;" << endl; break;
    case SCHEDULE_PS:   output << indent(0) << "sched = ps;" << endl; break;
    case SCHEDULE_PPR:  output << indent(0) << "sched = prior, preempt;" << endl; break;
    }
    for ( unsigned i = 1; i <= myServerChains.size(); ++i ) {
	output << indent(0) << "service" << server_chain( *this, multi_class, i ) 
	       << " = exp(" << serviceTimeForQueueingNetwork(myServerChains[i],&Element::hasServerChain) 
	       << ");" << endl;
    }
    printQNAPReplies( output, multi_class );
    return output;
}


/*
 * Generate the "transits" that correspond to the replies.  Open classes go "out."
 */

ostream& 
Entity::printQNAPReplies( ostream& output, const bool multi_class ) const
{
    Sequence<GenericCall *> nextCall( myCallers );
    GenericCall * aCall;

    while ( aCall = nextCall() ) {
	if ( !aCall->isSelected() ) continue;
	output << indent(0) << "transit";
	if ( aCall->hasSendNoReply() ) {
	    output << open_chain( *aCall->srcTask(), multi_class, 1 ) << " = out;" << endl;
	} else {
	    output << closed_chain( *aCall->srcTask(), multi_class, 1 ) << " = " << aCall->srcName() << ",1;" << endl;
	}
    } 

    return output;
}
#endif


#if defined(PMIF_OUTPUT)
/*
 * PMIF queueing network.
 */

ostream&
Entity::printPMIFServer( ostream& output ) const
{
    output << indent(0) << "<Server Name=\"" << name() 
	   << "\" Quantity=\"" << copies();
    output << "\" SchedulingPolicy=\"";
    if ( isInfinite() ) {
	output << "IS";
    } else switch ( scheduling() ) {
    case SCHEDULE_DELAY: output << "IS"; break;
    case SCHEDULE_PS:    output << "PS"; break;
    default:	         output << "FCFS"; break;
    }
    output << "\"/>" << endl;
    return output;
}


ostream&
Entity::printPMIFReplies( ostream& output ) const
{
    Sequence<GenericCall *> nextCall( myCallers );
    GenericCall * aCall;

    for ( unsigned k = 1; k <= myServerChains.size(); ++k ) {
	output << indent(+1) << "<DemandServiceRequest WorkloadName=\"" << server_chain( *this, true, 1 )
	       << "\" ServerID=\"" << name()
	       << "\" NumberOfVisits=\"" << 1
	       << "\" ServiceDemand=\"" << serviceTimeForQueueingNetwork(myServerChains[k],&Element::hasServerChain) 
	       << "\">" << endl;
	while ( aCall = nextCall() ) {
	    if ( !aCall->isSelected() || !aCall->srcTask()->hasClientChain(myServerChains[k]) ) {
		continue;
	    } else if ( aCall->hasSendNoReply() ) {
		output << indent(0) << "<Transit To=\"" << aCall->srcName() << "-sink"
		       << "\" Probability=\"" << 1.0
		       << "\"/>" << endl;
	    } else if ( aCall->hasRendezvous() ) {
		output << indent(0) << "<Transit To=\"" << aCall->srcName() 
		       << "\" Probability=\"" << 1.0
		       << "\"/>" << endl;
	    }
	}
	output << indent(-1) << "</DemandServiceRequest>" << endl; 
   }
    return output;
}
#endif


/*
 * Print entry service time parameters.
 */

ostream&
Entity::printName( ostream& output, const int count ) const
{
    if ( count == 0 ) {
	output << setw( maxStrLen-1 ) << name() << " ";
    } else {
	output << setw( maxStrLen ) << " ";
    }
    return output;
}

/* -------------------- SRVN Input File Generation  ------------------- */

static ostream&
copies_of_str( ostream& output, const Entity& anEntity )
{
    if ( anEntity.isMultiServer() ) {
	output << " m " << instantiate( anEntity.copies() );
    }
    return output;
}



static ostream&
replicas_of_str( ostream& output, const Entity& anEntity )
{
    if ( anEntity.replicas() > 1 ) {
	output << " r " << anEntity.replicas();
    }
    return output;
}

#if defined(QNAP_OUTPUT)
static ostream&
qnap_name_str( ostream& output, const Entity& anEntity )
{
    output << anEntity.name();
    if ( anEntity.replicas() > 1 ) {
	output << "( 1 step 1 until " << qnap_replicas( anEntity ) << ")";
    }
    return output;
}


static ostream&
qnap_replicas_str( ostream& output, const Entity& anEntity )
{
    output << "n_" << anEntity.name();
    return output;
}
#endif

SRVNEntityManip
copies_of( const Entity & anEntity )
{
    return SRVNEntityManip( &copies_of_str, anEntity );
}


SRVNEntityManip
replicas_of( const Entity & anEntity )
{
    return SRVNEntityManip( &replicas_of_str, anEntity );
}


#if defined(QNAP_OUTPUT)
SRVNEntityManip 
qnap_name( const Entity & anEntity )
{
    return SRVNEntityManip( &qnap_name_str, anEntity );
}

SRVNEntityManip 
qnap_replicas( const Entity & anEntity )
{
    return SRVNEntityManip( &qnap_replicas_str, anEntity );
}
#endif
