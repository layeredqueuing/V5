/* -*- c++ -*-
 * $Id: entity.cc 14387 2021-01-21 14:09:16Z greg $
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
#include <algorithm>
#include <cstring>
#include <cassert>
#include <cmath>
#include <sstream>
#include <cstdlib>
#include <limits.h>
#if HAVE_IEEEFP_H
#include <ieeefp.h>
#endif
#include <lqx/SyntaxTree.h>
#include <lqio/error.h>
#include <lqio/dom_entity.h>
#include <lqio/dom_task.h>
#include <lqio/srvn_output.h>
#include <lqio/srvn_spex.h>
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

std::ostream&
operator<<( std::ostream& output, const Entity& self ) 
{
    if ( Flags::print[OUTPUT_FORMAT].value.i == FORMAT_TXT ) {
	self.print( output );
    } else {
	self.draw( output );
    }
    return output;
}

/* ----------------------- Abstract Superclass. ----------------------- */

/*
 * Set me up.
 */

Entity::Entity( const LQIO::DOM::Entity* domEntity, const size_t id )
    : Element( domEntity, id ),
      _callers(),
      _level(0),
      _isSelected(true),
      _isSurrogate(false)
{
    if ( Flags::print[INCLUDE_ONLY].value.r ) {
	_isSelected = std::regex_match( name(), *Flags::print[INCLUDE_ONLY].value.r );
    } else if ( submodel_output()
		|| queueing_output()
		|| Flags::print[CHAIN].value.i != 0 ) {
	_isSelected = false;
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
 * Compare function for normal layering.  Return true if e1 < e2.
 */

bool
Entity::compare( const Entity * e1, const Entity *e2 )
{
    if ( e1->forwardsTo( dynamic_cast<const Task *>(e2) ) ) {
	return true;
    } else if ( e2->forwardsTo( dynamic_cast<const Task *>(e1) ) ) {
	return false;
    } else if ( e1->hasForwardingLevel() && !e2->hasForwardingLevel() ) {
	return true;
    } else if ( e2->hasForwardingLevel() && !e1->hasForwardingLevel() ) {
	return false;
    } else {
	return Element::compare( e1, e2 );
    }
}


bool
Entity::compareLevel( const Entity * e1, const Entity *e2 )
{
    int diff = static_cast<int>(e1->level()) - static_cast<int>(e2->level());
    if ( diff < 0 ) {
	return true;
    }
    return e1->topCenter().x() - e2->topCenter().x() < 0.0;
}

/* ------------------------ Instance Methods -------------------------- */

const LQIO::DOM::ExternalVariable&
Entity::copies() const
{
    return *dynamic_cast<const LQIO::DOM::Entity *>(getDOM())->getCopies();
}

unsigned int
Entity::copiesValue() const
{
    return dynamic_cast<const LQIO::DOM::Entity *>(getDOM())->getCopiesValue();
}

const LQIO::DOM::ExternalVariable&
Entity::replicas() const
{
    return *dynamic_cast<const LQIO::DOM::Entity *>(getDOM())->getReplicas();
}

unsigned int
Entity::replicasValue() const
{
    return dynamic_cast<const LQIO::DOM::Entity *>(getDOM())->getReplicasValue();
}

const LQIO::DOM::ExternalVariable&
Entity::fanIn( const Entity * aClient ) const
{
    return *dynamic_cast<const LQIO::DOM::Task *>(getDOM())->getFanIn( aClient->name() );
}

const LQIO::DOM::ExternalVariable&
Entity::fanOut( const Entity * aServer ) const
{
    return *dynamic_cast<const LQIO::DOM::Task *>(getDOM())->getFanOut( aServer->name() );
}

unsigned
Entity::fanInValue( const Entity * aClient ) const
{
    return dynamic_cast<const LQIO::DOM::Task *>(getDOM())->getFanInValue( aClient->name() );
}

unsigned
Entity::fanOutValue( const Entity * aServer ) const
{
    return dynamic_cast<const LQIO::DOM::Task *>(getDOM())->getFanOutValue( aServer->name() );
}

void
Entity::removeDstCall( GenericCall * aCall)
{
    std::vector<GenericCall *>::iterator pos = find_if( _callers.begin(), _callers.end(), EQ<GenericCall>( aCall ) );
    if ( pos != _callers.end() ) {
	_callers.erase( pos );
    }
}


/*
 * Return true if this is a multiserver.  Variables return true.
 */

bool 
Entity::isInfinite() const
{ 
    return scheduling() == SCHEDULE_DELAY || dynamic_cast<const LQIO::DOM::Entity *>(getDOM())->isInfinite();
}


/*
 * Return true if this is a multiserver.  Variables return true.
 */

bool 
Entity::isMultiServer() const
{
    return dynamic_cast<const LQIO::DOM::Entity *>(getDOM())->hasCopies();
}


bool
Entity::isReplicated() const
{
    return dynamic_cast<const LQIO::DOM::Entity *>(getDOM())->hasReplicas();
}


const scheduling_type 
Entity::scheduling() const 
{
    return dynamic_cast<const LQIO::DOM::Entity *>(getDOM())->getSchedulingType();
}

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
    return !isInfinite() && utilization() / copiesValue() > 1.05;
}

/*
 * Return the amount I should move by IFF I can be aligned.
 */

double
Entity::align() const
{
    if ( isForwardingTarget() ) return 0.0;		/* Don't align me if I call somebody via forwarding. */

    /* A server aligns with parent */

    std::vector<Entity *> myClients;
    clients( myClients, &Call::hasAncestorLevel );
    if ( myClients.size() == 1 ) {
	const Entity * myParent = myClients[0];
	return myParent->center().x() - center().x();
    }
    return 0.0;
}



/*
 * Sort entries and activities based on when they were visited.
 */

Entity&
Entity::sort()
{
    std::sort( _callers.begin(), _callers.end(), Call::compareDst );
    return *this;
}


#if defined(REP2FLAT)
Entity& 
Entity::removeReplication() 
{ 
    LQIO::DOM::Entity * dom = const_cast<LQIO::DOM::Entity *>(dynamic_cast<const LQIO::DOM::Entity *>(getDOM()));
    dom->setReplicasValue(1);
    return *this; 
}
#endif


bool
Entity::test( const taskPredicate predicate ) const
{
    const Task * task = dynamic_cast<const Task *>(this);
    return task && task->isServerTask() && (!predicate || (task->*predicate)());
}


/*
 * Count the number of calls that match the criteria passed
 */

unsigned
Entity::countCallers() const
{
    return std::count_if( _callers.begin(), _callers.end(), Predicate<GenericCall>( &GenericCall::isSelected ) );
}


unsigned int
Entity::count_callers::operator()( unsigned int augend, const Entity * entity ) const
{
    const Task * task = dynamic_cast<const Task *>(entity);
    if ( !task && task->isServerTask() ) return augend;
    const std::vector<Entry *> entries = task->entries();
    return std::accumulate( entries.begin(), entries.end(), augend, Entry::count_callers( _predicate ) );
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
	    return colourForUtilization( isInfinite() ? 0.0 : utilization() / copiesValue() );
	}
	break;

	if ( Flags::have_results ) {
    case COLOUR_DIFFERENCES:
	    return colourForDifference( utilization() );
	}
	break;

    case COLOUR_CLIENTS:
	return (Graphic::colour_type)(*myPaths.begin() % 11 + 5);		// first element is smallest 

    case COLOUR_LAYERS:
	return (Graphic::colour_type)(level() % 11 + 5);
    }
    return Graphic::DEFAULT_COLOUR;			// No colour.
}



/*
 * Label the node.
 */

Entity&
Entity::label()
{
    *myLabel << name();
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



std::ostream&
Entity::print( std::ostream& output ) const
{
    LQIO::SRVN::EntityInput::print( output, dynamic_cast<const LQIO::DOM::Entity *>(getDOM()) );
    return output;
}


Graphic::colour_type 
Entity::chainColour( unsigned int k ) const
{
    static Graphic::colour_type chain_colours[] = { Graphic::BLACK, Graphic::MAGENTA, Graphic::VIOLET, Graphic::BLUE, Graphic::INDIGO, Graphic::CYAN, Graphic::TURQUOISE, Graphic::GREEN, Graphic::SPRINGGREEN, Graphic::YELLOW, Graphic::ORANGE, Graphic::RED };

    if ( Flags::print[COLOUR].value.i == COLOUR_CHAINS ) { 
	return chain_colours[k%12];
    } else if ( colour() == Graphic::GREY_10 || colour() == Graphic::DEFAULT_COLOUR ) {
	return Graphic::BLACK;
    } else {
	return colour();
    }
}


/*
 * Print entry service time parameters.
 */

std::ostream&
Entity::printName( std::ostream& output, const int count ) const
{
    if ( count == 0 ) {
	output << std::setw( maxStrLen-1 ) << name() << " ";
    } else {
	output << std::setw( maxStrLen ) << " ";
    }
    return output;
}

/* -------------------------------------------------------------------- */
/*			   Queuieng Network				*/
/* -------------------------------------------------------------------- */

/*
 * Draw the queueing network
 */

std::ostream&
Entity::drawQueueingNetwork( std::ostream& output, const double max_x, const double max_y, std::vector<bool> &chain, std::vector<Arc *>& lastArc ) const
{
    std::vector<Entity *> myClients;
    clients( myClients );
    drawServer( output );

    /* find all clients with chains calling me... */

    for ( std::set<unsigned>::const_iterator k = myServerChains.begin(); k != myServerChains.end(); ++k ) {

	/* Draw connections to this server */

	for ( std::vector<Entity *>::iterator client = myClients.begin(); client != myClients.end(); ++client ) {
	    if ( !(*client)->hasClientChain( *k ) ) continue;

	    std::stringstream aComment;
	    aComment << "---------- Chain " << *k << ": " << name() << " -> " <<  (*client)->name() << " ----------";
	    myNode->comment( output, aComment.str() );
	    drawServerToClient( output, max_x, max_y, (*client), chain, *k );
	    
	    aComment.seekp(17, std::ios::beg);		// rewind.
	    aComment << *k << ": " << (*client)->name() << " -> " <<  name() << " ----------";
	    myNode->comment( output, aComment.str() );
	    drawClientToServer( output, (*client), chain, *k, lastArc );
	}
    }

    return output;
}



/*
 * Draw the queue for the queueing object.
 */

std::ostream&
Entity::drawServer( std::ostream& output ) const
{
    std::string aComment;
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

    myLabel->moveTo( bottomCenter() )
	.justification( LEFT_JUSTIFY )
	.moveBy( radius() * 1.5, radius() * 3.0 * myNode->direction() );
    output << *myLabel;

    return output;
}



/*
 * From Server to Client
 */

std::ostream&
Entity::drawServerToClient( std::ostream& output, const double max_x, const double min_y, const Entity * aClient, std::vector<bool> &chain, const unsigned k ) const
{
    const unsigned int max_k = chain.size() - 1;
    if ( !hasServerChain( k ) ) return output;

    Arc * outArc = Arc::newArc( 6 );
    outArc->scaleBy( Model::scaling(), Model::scaling() ).penColour( chainColour( k ) ).depth( myNode->depth() );
    const double direction = static_cast<double>(myNode->direction());
    const double spacing = Flags::print[Y_SPACING].value.f * Model::scaling();

    if ( aClient->hasClientClosedChain(k) ) {
	const double offset = radius() / 2.5;
	double x = bottomCenter().x() - radius() + offsetOf( myServerChains, k ) * 2.0 * radius() / ( 1.0 + myServerChains.size() );
	double y = min_y - (radius() + offset * (max_k - k)) * direction;
	outArc->pointAt(1).moveTo( x, y );

	if ( isMultiServer() || isInfinite() ) {
	    outArc->moveSrc( bottomCenter() );
	} else {
	    Point aPoint = bottomCenter().moveBy( 0, radius() * direction );
	    outArc->moveSrc( x, aPoint.y() );

	    outArc->pointAt(0) = outArc->srcIntersectsCircle( aPoint, radius() );
	}

	Label * aLabel = 0;
	if ( !chain[k] ) {
	    outArc->arrowhead( Graphic::CLOSED_ARROW );
	    x = max_x + offset * (max_k - k);
	    outArc->pointAt(2).moveTo( x, y );

	    if ( Flags::flatten_submodel || aClient->isReferenceTask() ) {
		y = aClient->top() + (radius() + (offset * (max_k - k))) * direction;
	    } else {
		y = aClient->top() + (spacing / 4.0 - (offset * (k-1))) * direction;
	    }
	    outArc->pointAt(3).moveTo( x, y );
	    if ( aClient->myClientOpenChains.size() ) {
		x = aClient->topCenter().moveBy( aClient->radius() * -3.0, 0 ).x();
	    } else {
		x = aClient->topCenter().x();
	    }
	    outArc->pointAt(4).moveTo( x - radius() + offsetOf( aClient->myClientClosedChains, k ) * 2.0 * radius() / ( 1.0 + aClient->myClientClosedChains.size() ), y );
		
	    y = aClient->bottomCenter().y() + (2.0 * radius()) * direction;
	    outArc->moveDst( x, y );

	    aLabel = Label::newLabel();
	    aLabel->moveTo( outArc->pointAt(4) ).moveBy( 2.0 * offset, 0 ).backgroundColour( Graphic::DEFAULT_COLOUR );
	    (*aLabel) << k;
	} else {
	    outArc->arrowhead( Graphic::NO_ARROW );
	    Node * aNode = Node::newNode( 0, 0 );

	    aNode->penColour( chainColour( k ) ).fillColour( chainColour( k ) ).fillStyle( Graphic::FILL_SOLID );
	    aNode->circle( output, outArc->pointAt(1), Model::scaling() );

	    delete aNode;
	    outArc->resize(2);
	}
	output << *outArc;
	if ( aLabel ) {
	    output << *aLabel;
	    delete aLabel;
	}
    } 
    if ( aClient->hasClientOpenChain(k) ) {
	double x = bottomCenter().x() - radius() + offsetOf( myServerChains, k ) * 2.0 * radius() / ( 1.0 + myServerChains.size() );
	double y = min_y - radius() * direction;
	outArc->pointAt(1).moveTo( x, y );

	/* Only if we're a multiserver.. */
	if ( isMultiServer() || isInfinite() ) {
	    outArc->moveSrc( bottomCenter() );
	} else {
	    Point aPoint = bottomCenter().moveBy( 0, radius() * direction );
	    outArc->moveSrc( x, aPoint.y() );
	    outArc->pointAt(0) = outArc->srcIntersectsCircle( aPoint, radius() );
	}

	/* From server... */
	x = bottomCenter().x();
	y = min_y - (radius() * direction * 3.5);
	outArc->pointAt(2).moveTo( x, y );
	outArc->resize(3);
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

std::ostream&
Entity::drawClientToServer( std::ostream& output, const Entity * aClient, std::vector<bool> &chain, const unsigned k, std::vector<Arc *>& lastArc ) const
{
    const unsigned N_POINTS = 5;
    Arc * inArc  = Arc::newArc( N_POINTS );

    inArc->scaleBy( Model::scaling(), Model::scaling() ).depth( myNode->depth() ).penColour( chainColour( k ) );

    const double direction = static_cast<double>(myNode->direction());
    const double offset = radius() / 2.0;

    double x = aClient->bottomCenter().x();
    double y = aClient->bottomCenter().y();

    if ( aClient->myClientOpenChains.size() && aClient->myClientClosedChains.size() ) {
	if ( aClient->myClientOpenChains.find( k ) != aClient->myClientOpenChains.end() ) {
	    x += aClient->radius() * 1.5;
	} else {
	    x -= aClient->radius() * 3.0;
	}
    }
    inArc->pointAt(0).moveTo( x, y );

    /* Adjust for chains */

    if ( aClient->myClientOpenChains.find( k ) != aClient->myClientOpenChains.end() ) {
	x = x - radius() + offsetOf( aClient->myClientOpenChains, k ) * 2.0 * radius() / ( 1.0 + aClient->myClientOpenChains.size() );
    } else {
	x = x - radius() + offsetOf( aClient->myClientClosedChains, k ) * 2.0 * radius() / ( 1.0 + aClient->myClientClosedChains.size() );
    }
    y -= static_cast<double>(k+1) * offset * direction;
    inArc->pointAt(1).moveTo( x, y );

    x = bottomCenter().x() - radius() + offsetOf( myServerChains, k ) * 2.0 * radius() / ( 1.0 + myServerChains.size() );
    inArc->pointAt(2).moveTo( x, y );

    y = bottomCenter().y() + 4.0 * radius() * myNode->direction();
    inArc->pointAt(3).moveTo( x, y );

    Label * aLabel = Label::newLabel();
    aLabel->moveTo( inArc->pointAt(2).x(), (inArc->pointAt(2).y() + inArc->pointAt(3).y()) / 2.0 ).backgroundColour( Graphic::DEFAULT_COLOUR );
    (*aLabel) << k;

    if ( isInfinite() ) {
	x = bottomCenter().x();
	y = bottomCenter().y() + 2.0 * radius() * myNode->direction();
	inArc->pointAt(4).moveTo( x, y );
    } else {
	inArc->pointAt(4) = inArc->pointAt(3);
    }

    /* Frobnicate -- chain already exists, so join this chain to that chain. */
    if ( chain[k] ) {
	Node * aNode = Node::newNode( 0, 0 );
	aNode->penColour( chainColour( k ) ).fillColour( chainColour( k ) ).fillStyle( Graphic::FILL_SOLID );

	if ( inArc->pointAt(2).x() <= lastArc[k]->pointAt(2).x() ) {
	    inArc->pointAt(0) = inArc->pointAt(1);
	    inArc->pointAt(1).x(lastArc[k]->pointAt(2).x());
	    lastArc[k]->pointAt(2) = inArc->pointAt(2);
	} else if ( lastArc[k]->pointAt(2).x() < inArc->pointAt(2).x() && inArc->pointAt(2).x() < lastArc[k]->pointAt(1).x() ) {
	    inArc->pointAt(1).x(inArc->pointAt(2).x());
	    inArc->pointAt(0) = inArc->pointAt(1);
	} else if ( inArc->pointAt(1).x() < lastArc[k]->pointAt(2).x() ) {
	    inArc->pointAt(1) = lastArc[k]->pointAt(2);
	    inArc->pointAt(0) = inArc->pointAt(1);
	} else {
	    inArc->pointAt(0) = inArc->pointAt(1);
	}
	if ( lastArc[k]->pointAt(2).x() < inArc->pointAt(2).x() ) {
	    lastArc[k]->pointAt(2) = inArc->pointAt(2);
	}
	aNode->circle( output, inArc->pointAt(0), Model::scaling() );
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


/* static */ unsigned
Entity::offsetOf( const std::set<unsigned>& chains, unsigned k )
{
    unsigned int i = 1;
    for ( std::set<unsigned>::const_iterator j = chains.begin(); j != chains.end() && *j != k; ++j ) {
	i += 1;
    }
    return i;
}

    
void
Entity::label_BCMP_server::operator()( Entity * entity ) const
{
    const BCMP::Model::Station& station = const_cast<BCMP::Model&>(_model).stationAt( entity->name() );
    entity->labelBCMPModel( station.demands() );
}


void
Entity::label_BCMP_client::operator()( Entity * entity ) const
{
    const BCMP::Model::Station& station = const_cast<BCMP::Model&>(_model).stationAt( ReferenceTask::__BCMP_station_name );
    entity->labelBCMPModel( station.demands(), entity->name() );
}


void
Entity::create_class::operator()( const Entity * entity ) const
{
    BCMP::Model::Class::Type type;
    if ( entity->isInOpenModel(_servers) && entity->isInClosedModel(_servers) ) type = BCMP::Model::Class::MIXED;
    else if ( entity->isInOpenModel(_servers) ) type = BCMP::Model::Class::OPEN;
    else type = BCMP::Model::Class::CLOSED;

    /* Think time for a task is the class think time. */

    const Task * task = dynamic_cast<const Task *>(entity);
    const LQIO::DOM::ExternalVariable * copies = entity->isMultiServer() ? &entity->copies() : &Element::ONE;
    const LQIO::DOM::ExternalVariable * think_time = task->hasThinkTime() ? &dynamic_cast<const ReferenceTask *>(task)->thinkTime() : &Element::ZERO;
    _model.insertClass( entity->name(), type, copies, think_time );
}


void
Entity::create_station::operator()( const Entity * entity ) const
{
    BCMP::Model::Station::Type type;
    if ( _type == BCMP::Model::Station::CUSTOMER ) type = _type;
    else if ( entity->isInfinite() ) type = BCMP::Model::Station::DELAY;
    else if ( entity->isMultiServer() ) type = BCMP::Model::Station::MULTISERVER;
    else type = BCMP::Model::Station::LOAD_INDEPENDENT;
    _model.insertStation( entity->name(), type, entity->scheduling(), dynamic_cast<const LQIO::DOM::Entity *>(entity->getDOM())->getCopies() );
}

/* +BUG_270 */
/*
 * Add two external variables.  Otherwise propogate a copy of one or
 * the other.  If either the augend or the addend is a
 * SymbolExternalVariable, then I will have to create an expression to
 * add the two.
 */

/* static */ LQX::SyntaxTreeNode *
Entity::getVariableExpression( const LQIO::DOM::ExternalVariable * variable )
{
    double value;
    LQX::SyntaxTreeNode * expression;
    if ( variable->wasSet() && variable->getValue( value ) ) {
	expression = new LQX::ConstantValueExpression( to_double(*variable) );
    } else {
	expression = new LQX::VariableExpression( variable->getName(), true );
    }
    return expression;
}

const LQIO::DOM::ExternalVariable *
Entity::addExternalVariables( const LQIO::DOM::ExternalVariable * augend, const LQIO::DOM::ExternalVariable * addend )
{
    if ( LQIO::DOM::ExternalVariable::isDefault( augend ) ) {
	return addend;
    } else if ( LQIO::DOM::ExternalVariable::isDefault( augend ) ) {
	return augend;
    } else if ( dynamic_cast<const LQIO::DOM::ConstantExternalVariable *>(augend) && dynamic_cast<const LQIO::DOM::ConstantExternalVariable *>(addend) ) {
	return new LQIO::DOM::ConstantExternalVariable( to_double(*augend) + to_double(*addend) );
    } else {
	/* More complicated... */
	LQX::SyntaxTreeNode * arg1 = getVariableExpression( augend );
	LQX::SyntaxTreeNode * arg2 = getVariableExpression( addend );
	LQIO::DOM::ExternalVariable * sum = static_cast<LQIO::DOM::ExternalVariable *>(spex_inline_expression( spex_add( arg1, arg2 ) ));
	std::cout << "Entity::addExternalVariables(" << *augend << "," << *addend << ") --> " << *(sum) << std::endl;
	return sum;
    }
}
