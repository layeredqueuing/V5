/* -*- c++ -*-
 * $Id: entity.cc 16853 2023-11-20 18:38:30Z greg $
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
#include <functional>
#include <sstream>
#include <cstdlib>
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
    if ( Flags::output_format() == File_Format::TXT ) {
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
    if ( Flags::include_only() != nullptr ) {
	_isSelected = std::regex_match( name(), *Flags::include_only() );
    } else if ( submodel_output()
		|| queueing_output()
		|| Flags::chain() != 0 ) {
	_isSelected = false;
    }
}

/*
 * Delete all entries associated with this task.
 */

Entity::~Entity()
{	
    delete _node;
    delete _label;
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
    if ( Flags::chain() != 0 && !queueing_output() ) {
	return hasPath( Flags::chain() );
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

    std::vector<Task *> myClients;
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


#if REP2FLAT
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
    return std::count_if( _callers.begin(), _callers.end(), std::mem_fn( &GenericCall::isSelected ) );
}


unsigned int
Entity::count_callers::operator()( unsigned int augend, const Entity * entity ) const
{
    const Task * task = dynamic_cast<const Task *>(entity);
    if ( task != nullptr && task->isServerTask() ) return augend;
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



Graphic::Colour
Entity::colour() const
{
    if ( isSurrogate() ) {
	return Graphic::Colour::GREY_10;
    }
    switch ( Flags::colouring() ) {
    case Colouring::RESULTS:
	if ( Flags::have_results ) {
	    if ( !std::isfinite( utilization() ) ) {
		return Graphic::Colour::RED;
	    } else if ( isInfinite() ) {
		return Graphic::Colour::DEFAULT;
	    } else {
		return colourForUtilization( utilization() / copiesValue() );
	    }
	}
	break;

    case Colouring::DIFFERENCES:
	if ( Flags::have_results ) {
	    return colourForDifference( utilization() );
	}
	break;

    case Colouring::CLIENTS:
	return (Graphic::Colour)(*myPaths.begin() % 11 + 5);		// first element is smallest 

    case Colouring::LAYERS:
	return (Graphic::Colour)(level() % 11 + 5);
    }
    return Graphic::Colour::DEFAULT;			// No colour.
}



/*
 * Label the node.
 */

Entity&
Entity::label()
{
    *_label << name();
    if ( Flags::print_input_parameters() ) {
	if ( isMultiServer() ) {
	    *_label << " {" << copies() << "}";
	} else if ( isInfinite() ) {
	    *_label << " {" << _infty() << "}";
	}
	if ( isReplicated() ) {
	    *_label << " <" << replicas() << ">";
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


Graphic::Colour 
Entity::chainColour( unsigned int k ) const
{
    static Graphic::Colour chain_colours[] = { Graphic::Colour::BLACK, Graphic::Colour::MAGENTA, Graphic::Colour::VIOLET, Graphic::Colour::BLUE, Graphic::Colour::INDIGO, Graphic::Colour::CYAN, Graphic::Colour::TURQUOISE, Graphic::Colour::GREEN, Graphic::Colour::SPRINGGREEN, Graphic::Colour::YELLOW, Graphic::Colour::ORANGE, Graphic::Colour::RED };

    if ( Flags::colouring() == Colouring::CHAINS ) { 
	return chain_colours[k%12];
    } else if ( colour() == Graphic::Colour::GREY_10 || colour() == Graphic::Colour::DEFAULT ) {
	return Graphic::Colour::BLACK;
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
    std::vector<Task *> myClients;
    clients( myClients );
    drawServer( output );

    /* find all clients with chains calling me... */

    for ( std::set<unsigned>::const_iterator k = myServerChains.begin(); k != myServerChains.end(); ++k ) {

	/* Draw connections to this server */

	for ( std::vector<Task *>::iterator client = myClients.begin(); client != myClients.end(); ++client ) {
	    if ( !(*client)->hasClientChain( *k ) ) continue;

	    std::stringstream aComment;
	    aComment << "========== Chain " << *k << ": " << name() << " -> " <<  (*client)->name() << " ==========";
	    _node->comment( output, aComment.str() );
	    drawServerToClient( output, max_x, max_y, (*client), chain, *k );
	    
	    aComment.seekp(17, std::ios::beg);		// rewind.
	    aComment << *k << ": " << (*client)->name() << " -> " <<  name() << " ==========";
	    _node->comment( output, aComment.str() );
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
    _node->comment( output, aComment );
    _node->penColour( colour() == Graphic::Colour::GREY_10 ? Graphic::Colour::BLACK : colour() ).fillColour( colour() );

    /* Draw the queue. */

    if ( !isInfinite() ) {
	_node->draw_queue( output, bottomCenter(), radius() );
    }

    /* Draw the server. */

    if ( isMultiServer() || isInfinite() ) {
	_node->multi_server( output, bottomCenter(), radius() );
    } else {
	Point aPoint = bottomCenter().moveBy( 0, radius() * _node->direction() );
	_node->circle( output, aPoint, radius() );
    }

    /* Draw the label */

    _label->moveTo( bottomCenter() )
	.justification( Justification::LEFT )
	.moveBy( radius() * 1.5, radius() * 3.0 * _node->direction() );
    output << *_label;

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
    outArc->scaleBy( Model::scaling(), Model::scaling() ).penColour( chainColour( k ) ).depth( _node->depth() );
    const double direction = static_cast<double>(_node->direction());
    const double spacing = Flags::y_spacing() * Model::scaling();

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
	    outArc->arrowhead( Graphic::Arrowhead::CLOSED );
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
	    aLabel->moveTo( outArc->pointAt(4) ).moveBy( 2.0 * offset, 0 ).backgroundColour( Graphic::Colour::DEFAULT );
	    (*aLabel) << k;
	} else {
	    outArc->arrowhead( Graphic::Arrowhead::NONE );
	    Node * aNode = Node::newNode( 0, 0 );

	    aNode->penColour( chainColour( k ) ).fillColour( chainColour( k ) ).fillStyle( Graphic::Fill::SOLID );
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
	_node->open_sink( output, aPoint, radius() );
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

    inArc->scaleBy( Model::scaling(), Model::scaling() ).depth( _node->depth() ).penColour( chainColour( k ) );

    const double direction = static_cast<double>(_node->direction());
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

    y = bottomCenter().y() + 4.0 * radius() * _node->direction();
    inArc->pointAt(3).moveTo( x, y );

    Label * aLabel = Label::newLabel();
    aLabel->moveTo( inArc->pointAt(2).x(), (inArc->pointAt(2).y() + inArc->pointAt(3).y()) / 2.0 ).backgroundColour( Graphic::Colour::DEFAULT );
    (*aLabel) << k;

    if ( isInfinite() ) {
	x = bottomCenter().x();
	y = bottomCenter().y() + 2.0 * radius() * _node->direction();
	inArc->pointAt(4).moveTo( x, y );
    } else {
	inArc->pointAt(4) = inArc->pointAt(3);
    }

    /* Frobnicate -- chain already exists, so join this chain to that chain. */
    if ( chain[k] ) {
	Node * aNode = Node::newNode( 0, 0 );
	aNode->penColour( chainColour( k ) ).fillColour( chainColour( k ) ).fillStyle( Graphic::Fill::SOLID );

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
    entity->labelBCMPModel( station.classes() );
}


void
Entity::label_BCMP_client::operator()( Entity * entity ) const
{
    const BCMP::Model::Station& station = const_cast<BCMP::Model&>(_model).stationAt( ReferenceTask::__BCMP_station_name );
    entity->labelBCMPModel( station.classes(), entity->name() );
}


void
Entity::create_station::operator()( const Entity * entity ) const
{
    BCMP::Model::Station::Type type;
    if ( entity->isInfinite() ) type = BCMP::Model::Station::Type::DELAY;
    else if ( entity->isMultiServer() ) type = BCMP::Model::Station::Type::MULTISERVER;
    else type = BCMP::Model::Station::Type::LOAD_INDEPENDENT;
    const LQIO::DOM::ExternalVariable * copies = dynamic_cast<const LQIO::DOM::Entity *>(entity->getDOM())->getCopies();
    _model.insertStation( entity->name(), type, entity->scheduling(), getLQXVariable( copies ) );
}


/* +BUG_270 */
/*
 * Add two external variables.  Otherwise propogate a copy of one or
 * the other.  If either the augend or the addend is a
 * SymbolExternalVariable, then I will have to create an expression to
 * add the two.
 */


double
Entity::to_double( LQX::SyntaxTreeNode * var )
{
    if ( var == nullptr ) return 0.0;
    LQX::SymbolAutoRef symbol = var->invoke(nullptr);
    if ( symbol->getType() != LQX::Symbol::SYM_DOUBLE ) throw std::domain_error( "invalid double" );
    return symbol->getDoubleValue();
}


unsigned int
Entity::to_unsigned( LQX::SyntaxTreeNode * var )
{
    if ( var == nullptr ) return 0.0;
    LQX::SymbolAutoRef symbol = var->invoke(nullptr);
    if ( symbol->getType() != LQX::Symbol::SYM_DOUBLE ) throw std::domain_error( "invalid unsigned integer" );
    double value = symbol->getDoubleValue();
    if ( value != rint(value) ) throw std::domain_error( "invalid integer" );
    return static_cast<unsigned int>(value);
}


/* static */ LQX::SyntaxTreeNode *
Entity::getLQXVariable( const LQIO::DOM::ExternalVariable* variable )
{
    if ( variable == nullptr ) return nullptr;

    double value;
    LQX::SyntaxTreeNode * expression;
    if ( variable->wasSet() && variable->getValue( value ) ) {
	expression = new LQX::ConstantValueExpression( LQIO::DOM::to_double(*variable) );
    } else {
	expression = new LQX::VariableExpression( variable->getName(), true );
    }
    return expression;
}

/* static */ LQX::SyntaxTreeNode *
Entity::getLQXVariable( const LQIO::DOM::ExternalVariable* variable, double default_value )
{
    if ( variable == nullptr ) return new LQX::ConstantValueExpression( default_value );
    return getLQXVariable( variable );
}

LQX::SyntaxTreeNode *
Entity::addLQXExpressions( LQX::SyntaxTreeNode * augend, LQX::SyntaxTreeNode * addend )
{
    if ( BCMP::Model::isDefault( augend ) ) {
	return addend;
    } else if ( BCMP::Model::isDefault( addend ) ) {
	return augend;
    } else if ( dynamic_cast<LQX::ConstantValueExpression *>(augend) && dynamic_cast<LQX::ConstantValueExpression *>(addend) ) {
	return new LQX::ConstantValueExpression( to_double(augend) + to_double(addend) );
    } else {
	LQX::SyntaxTreeNode * sum =  new LQX::MathExpression( LQX::MathOperation::ADD, augend, addend );
//	std::cout << "Entity::addLQXExpressions(" << *augend << "," << *addend << ") --> " << *(sum) << std::endl;
	return sum;
    }
}

LQX::SyntaxTreeNode *
Entity::subtractLQXExpressions( LQX::SyntaxTreeNode * minuend, LQX::SyntaxTreeNode * subtrahend )
{
    if ( BCMP::Model::isDefault( subtrahend ) ) {
	return minuend;
    } else if ( BCMP::Model::isDefault( minuend ) ) {
	return new LQX::MathExpression( LQX::MathOperation::NEGATE, subtrahend, nullptr );
    } else if ( dynamic_cast<LQX::ConstantValueExpression *>(minuend) && dynamic_cast<LQX::ConstantValueExpression *>(subtrahend) ) {
	return new LQX::ConstantValueExpression( to_double(minuend) + to_double(subtrahend) );
    } else {
	LQX::SyntaxTreeNode * sum =  new LQX::MathExpression( LQX::MathOperation::SUBTRACT, minuend, subtrahend );
//	std::cout << "Entity::subtractQXExpressions(" << *minuend << "," << *addend << ") --> " << *(sum) << std::endl;
	return sum;
    }
}

LQX::SyntaxTreeNode *
Entity::multiplyLQXExpressions( LQX::SyntaxTreeNode * multiplicand, LQX::SyntaxTreeNode * multiplier )
{
    if ( BCMP::Model::isDefault( multiplicand, 1. ) ) {
	return multiplier;
    } else if ( BCMP::Model::isDefault( multiplier, 1. ) ) {
	return multiplicand;
    } else if ( dynamic_cast<LQX::ConstantValueExpression *>(multiplicand) && dynamic_cast<LQX::ConstantValueExpression *>(multiplier) ) {
	return new LQX::ConstantValueExpression( to_double(multiplicand) + to_double(multiplier) );
    } else {
	LQX::SyntaxTreeNode * product = new LQX::MathExpression( LQX::MathOperation::MULTIPLY, multiplicand, multiplier );
//	std::cout << "Entity::addLQXExpressions(" << *multiplicand << "," << *multiplier << ") --> " << *(product) << std::endl;
	return product;
    }
}

LQX::SyntaxTreeNode *
Entity::divideLQXExpressions( LQX::SyntaxTreeNode * dividend, LQX::SyntaxTreeNode * divisor )
{
    if ( BCMP::Model::isDefault( dividend, 0. ) ) {		/* zero / ? -> zero */
	return dividend;
    } else if ( BCMP::Model::isDefault( divisor, 1. ) ) {	/* division by one */
	return dividend;
    } else if ( dynamic_cast<LQX::ConstantValueExpression *>(dividend) && dynamic_cast<LQX::ConstantValueExpression *>(divisor) ) {
	return new LQX::ConstantValueExpression( to_double(dividend) / to_double(divisor) );
    } else {
	LQX::SyntaxTreeNode * quotient =  new LQX::MathExpression( LQX::MathOperation::DIVIDE, divisor, dividend );
//	std::cout << "Entity::addLQXExpressions(" << *dividend << "," << *divisor << ") --> " << *(quotient) << std::endl;
	return quotient;
    }
}
