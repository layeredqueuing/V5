/* -*- c++ -*-
 * $Id: processor.cc 13200 2018-03-05 22:48:55Z greg $
 *
 * Everything you wanted to know about a task, but were afraid to ask.
 *
 * Copyright the Real-Time and Distributed Systems Group,
 * Department of Systems and Computer Engineering,
 * Carleton University, Ottawa, Ontario, Canada. K1S 5B6
 *
 * November, 1994
 *
 * ------------------------------------------------------------------------
 */

#include "lqn2ps.h"
#include <cmath>
#include <algorithm>
#include <limits.h>
#if HAVE_VALUES_H
#include <values.h>
#endif
#if HAVE_FLOAT_H
#include <float.h>
#endif
#include <lqio/input.h>
#include <lqio/labels.h>
#include <lqio/error.h>
#include <lqio/dom_processor.h>
#include <lqio/dom_document.h>
#include "errmsg.h"
#include "processor.h"
#include "entry.h"
#include "task.h"
#include "call.h"
#include "label.h"
#include "cltn.h"
#include "model.h"

#if !defined(MAXDOUBLE)
#define MAXDOUBLE FLT_MAX
#endif

set<Processor *,ltProcessor> processor;

/* -------------------------------------------------------------------- */
/* Funky Formatting functions for inline with <<.			*/
/* -------------------------------------------------------------------- */

class SRVNProcessorManip {
public:
    SRVNProcessorManip( ostream& (*ff)(ostream&, const Processor & ), const Processor & theProcessor ) 
	: f(ff), anProcessor(theProcessor) {}

private:
    ostream& (*f)( ostream&, const Processor& );
    const Processor & anProcessor;

    friend ostream& operator<<(ostream & os, const SRVNProcessorManip& m ) 
	{ return m.f(os,m.anProcessor); }
};

static ostream& proc_scheduling_of_str( ostream&, const Processor & aProcessor );

static inline SRVNProcessorManip proc_scheduling_of( const Processor & aProcessor ) { return SRVNProcessorManip( &proc_scheduling_of_str, aProcessor ); }

/* ---------------------- Overloaded Operators ------------------------ */

ostream&
operator<<( ostream& output, const Processor& self )
{
    switch( Flags::print[OUTPUT_FORMAT].value.i ) {
#if defined(TXT_OUTPUT)
    case FORMAT_TXT:
#endif
    case FORMAT_SRVN:
#if defined(QNAP_OUTPUT)
    case FORMAT_QNAP:
#endif
    case FORMAT_XML:
	break;
    default:
	self.draw( output );
	break;
    }

    return output;
}

/* ------------------------ Constructors etc. ------------------------- */

Processor::Processor( const LQIO::DOM::Processor* aDomObject ) 
    : Entity( aDomObject, ::processor.size()+1 ),
      myGroupSelected(false)
{ 
    if ( Flags::print[PROCESSORS].value.i == PROCESSOR_NONE ) {
	iAmSelected = false;
    }

    const double r = Flags::graphical_output_style == TIMEBENCH_STYLE ? Flags::icon_height : Flags::entry_height;
    myNode = Node::newNode( r, r );
    myLabel = Label::newLabel();
}



/*
 * Destructor...
 */

Processor::~Processor()
{
    taskList.clear();
}


/*
 * Make a copy of the processor
 */

Processor *
Processor::clone( unsigned int replica ) const
{
    ostringstream aName;
    aName << name() << "_" << replica;
    set<Processor *,ltProcessor>::const_iterator nextProcessor = find_if( ::processor.begin(), ::processor.end(), eqProcStr( aName.str().c_str() ) );
    if ( nextProcessor != ::processor.end() ) {
	string msg = "Processor::expandProcessor(): cannot add symbol ";
	msg += aName.str();
	throw runtime_error( msg );
    }
    LQIO::DOM::Processor * dom = new LQIO::DOM::Processor( *dynamic_cast<const LQIO::DOM::Processor*>(getDOM()) );
    dom->setName( aName.str() );
    const_cast<LQIO::DOM::Document *>(getDOM()->getDocument())->addProcessorEntity( dom );
    
    return new Processor( dom );
}

/* ------------------------ Instance Methods -------------------------- */

/*
 * Return the processor for this processor???
 */

const Processor *
Processor::processor() const
{
    throw should_not_implement( "Processor::processor()", __FILE__, __LINE__ );
    return 0;
}



/*
 * Set the processor for this processor???
 */

Entity&
Processor::processor( const Processor * ) 
{
    throw should_not_implement( "Processor::processor(Processor *)", __FILE__, __LINE__ );
    return *this;
}

bool
Processor::hasRate() const
{ 
    const LQIO::DOM::ExternalVariable * rate = dynamic_cast<const LQIO::DOM::Processor *>(getDOM())->getRate();
    double v;
    return !rate->wasSet() || !rate->getValue(v) || v != 1.0;
}

LQIO::DOM::ExternalVariable& 
Processor::rate() const
{
    return *dynamic_cast<const LQIO::DOM::Processor *>(getDOM())->getRate();
}

bool
Processor::hasQuantum() const
{ 
    const LQIO::DOM::ExternalVariable * quantum = dynamic_cast<const LQIO::DOM::Processor *>(getDOM())->getQuantum();
    double v;
    return quantum && (!quantum->wasSet() || !quantum->getValue(v) || v != 1.0);
}

LQIO::DOM::ExternalVariable& 
Processor::quantum() const
{
    return *dynamic_cast<const LQIO::DOM::Processor *>(getDOM())->getQuantum();
}

/* -------------------------- Result Queries -------------------------- */

double 
Processor::utilization() const
{
    return dynamic_cast<const LQIO::DOM::Processor *>(getDOM())->getResultUtilization();
}


/*
 * Find the largest spread in levels for all the tasks that call this
 * processor.  Reference tasks will sort first.
 */

unsigned
Processor::taskDepth() const
{ 
    unsigned minLevel = UINT_MAX;

    for ( set<Task *,ltTask>::const_iterator nextTask = tasks().begin(); nextTask != tasks().end(); ++nextTask ) {
	const Task& aTask = **nextTask;
	minLevel = min( minLevel, aTask.level() );
    }
    return minLevel;
}



/*
 * Used for sorting.
 */

double
Processor::meanLevel() const
{
    unsigned minLevel = UINT_MAX;
    unsigned maxLevel = 0;
    for ( set<Task *,ltTask>::const_iterator nextTask = tasks().begin(); nextTask != tasks().end(); ++nextTask ) {
	const Task& aTask = **nextTask;
	minLevel = min( minLevel, aTask.level() );
	maxLevel = max( maxLevel, aTask.level() );
    }

    return (static_cast<double>(maxLevel) + static_cast<double>(minLevel)) / 2.0;
}


/*
 * Return true if this processor can schedule tasks with priority.
 */

bool
Processor::hasPriorities() const
{
    return scheduling() == SCHEDULE_HOL 	       
	|| scheduling() == SCHEDULE_PPR
	|| scheduling() == SCHEDULE_PS_HOL
	|| scheduling() == SCHEDULE_PS_PPR;
}



/*
 * Return true if we want to print this processor.
 */

bool
Processor::isInteresting() const
{
    return Flags::print[PROCESSORS].value.i == PROCESSOR_ALL
	|| (Flags::print[PROCESSORS].value.i == PROCESSOR_DEFAULT 
	    && !isInfinite() 
	    && clientsCanQueue() )
	|| (Flags::print[PROCESSORS].value.i == PROCESSOR_NONINFINITE
	    && !isInfinite() )
#if defined(TXT_OUTPUT)
	|| Flags::print[OUTPUT_FORMAT].value.i == FORMAT_TXT
#endif
	|| input_output();
}



/*
 * Return true if this entity is selected.
 * See subclasses for further tests.
 */

bool
Processor::isSelectedIndirectly() const
{
#if 0
    if ( Entity::isSelectedIndirectly() ) {
	return true;
    } else {
	for ( set<Task *,ltTask>::const_iterator nextTask = tasks().begin(); nextTask != tasks().end(); ++nextTask ) {
	    const Task& aTask = **nextTask;
	    if ( aTask.isSelected() ) {
		return true;
	    } 
	}
    }
    return false;
#else
    return Entity::isSelectedIndirectly();
#endif
}



bool
Processor::clientsCanQueue() const
{
    if ( isInfinite() ) return false;
    else {
	const LQIO::DOM::ExternalVariable * m = dynamic_cast<const LQIO::DOM::Entity *>(getDOM())->getCopies();
	double value;
	return !m->wasSet() || !m->getValue(value) || nClients() > value;
    }
}


/*
 * Return all clients to this processor.
 */

unsigned
Processor::referenceTasks( Cltn<const Entity *>& clientCltn, Element * dst ) const
{
    for ( set<Task *,ltTask>::const_iterator nextTask = tasks().begin(); nextTask != tasks().end(); ++nextTask ) {
	const Task& aTask = **nextTask;
	aTask.referenceTasks( clientCltn, dst );
    }
    return clientCltn.size();
}


/*
 * Return all clients to this processor.
 */

unsigned
Processor::clients( Cltn<const Entity *>& clientCltn, const callFunc aFunc ) const
{
    for ( set<Task *,ltTask>::const_iterator nextTask = tasks().begin(); nextTask != tasks().end(); ++nextTask ) {
	Task * aTask = *nextTask;
	clientCltn += static_cast<const Entity *>(aTask);
    }
    return clientCltn.size();
}


/*
 * Return the number of instances of all tasks calling this processor 
 */

unsigned
Processor::nClients() const
{
    unsigned sum = 0;
    for ( set<Task *,ltTask>::const_iterator nextTask = tasks().begin(); nextTask != tasks().end(); ++nextTask ) {
	const Task& aTask = **nextTask;
	if ( aTask.isInfinite() ) {
	    return UINT_MAX;
	}
	if ( aTask.copies().wasSet() ) {
	    sum += LQIO::DOM::to_double( aTask.copies() ) * aTask.countThreads();
	} else {
	    sum += 1;
	}
    }
    return sum;
}



double
Processor::getIndex() const
{
    Cltn<const Entity *> myClients;
    clients( myClients );

    Sequence<const Entity *> nextClient( myClients );
    const Entity * aClient;

    double anIndex = MAXDOUBLE;
    while ( aClient = nextClient() ) {
	anIndex = min( anIndex, aClient->index() );
    }

    return anIndex;
}


double
Processor::serviceTimeForQueueingNetwork( const unsigned k, chainTestFunc aFunc ) const
{
    double time = 0.0;
    double sum  = 0.0;
    for ( set<Task *,ltTask>::const_iterator nextTask = tasks().begin(); nextTask != tasks().end(); ++nextTask ) {
	const Task& aTask = **nextTask;
	Sequence<Entry *> nextEntry(aTask.entries());
	Entry * anEntry;

	while ( anEntry = nextEntry() ) {
	    if ( (anEntry->*aFunc)( k ) ) {
		const double tput = anEntry->throughput() ? anEntry->throughput() : 1.0;
		time += tput * anEntry->serviceTime();
		sum  += tput;
	    }
	}
    }

    /* Take mean time. */

    if ( sum ) {
	time = time / sum;
    }

    return time;
}



/*
 * move the processor and it's arcs.
 */

Processor&
Processor::moveBy( const double dx, const double dy )
{
    myNode->moveBy( dx, dy );
    myLabel->moveBy( dx, dy );

    moveDst();

    return *this;
}




/*
 * move the processor and it's arcs.
 */

Processor&
Processor::moveTo( const double x, const double y )
{
    myNode->moveTo( x, y );
    myLabel->moveTo( center() );

    sort();		/* Reorder arcs */
    moveDst();

    return *this;
}




/*
 * Move all arcs I sink.
 */

Processor&
Processor::moveDst()
{
    Sequence<GenericCall *> nextRefr( callerList() );
    GenericCall * dstCall;
    Point aPoint = center();

    /* Draw other incomming arcs. */

    while ( dstCall = nextRefr() ) {
	const_cast<Entity *>(dstCall->srcTask())->moveBy(0.0,0.0);
	dstCall->moveDst( aPoint );
    }

    return *this;
}



/*
 * Colour the node.
 */

Graphic::colour_type
Processor::colour() const
{
    if ( isSurrogate() ) {
	return Graphic::GREY_10;
    }
    switch ( Flags::print[COLOUR].value.i ) {
    case COLOUR_SERVER_TYPE:
	return Graphic::BLUE;
    case COLOUR_RESULTS:
	if ( Flags::have_results ) {
	    const double u = isInfinite() ? 0.0 : utilization() / (LQIO::DOM::to_double(copies()) * LQIO::DOM::to_double(rate()));
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
    }
    return Entity::colour();
}


/*
 * Label the node.
 */

Processor&
Processor::label()
{
    if ( Flags::print[INPUT_PARAMETERS].value.b && queueing_output() ) {
	for ( set<Task *,ltTask>::const_iterator nextTask = tasks().begin(); nextTask != tasks().end(); ++nextTask ) {
	    const Task& aTask = **nextTask;
	    Sequence<Entry *> nextEntry(aTask.entries());
	    Entry * anEntry;

	    while ( anEntry = nextEntry() ) {
		*myLabel << anEntry->name() << " (" << print_number_slices( *anEntry ) << ")";
		myLabel->newLine();
	    }
	    myLabel->newLine();
	    Entity::label();
	    while ( anEntry = nextEntry() ) {
		myLabel->newLine() << anEntry->name() << " [" << print_slice_time( *anEntry ) << "]";
	    }
	}
    } else {
	string s = name();
	if ( scheduling() != SCHEDULE_FIFO && !isInfinite() ) {
	    s += "*";			/* Not FIFO, so flag as such */
	}
	myLabel->initialize( s );
	if ( Flags::print[INPUT_PARAMETERS].value.b ) {
	    bool newline = false;
	    if ( isMultiServer() ) {
		if ( !processor_output() ) {
		    myLabel->newLine();
		    newline = true;
		}
		*myLabel << "{" << copies() << "}";
	    } else if ( isInfinite() ) {
		if ( !processor_output() ) {
		    myLabel->newLine();
		    newline = true;
		}
		*myLabel << "{" << _infty() << "}";
	    }
	    if ( isReplicated() ) {
		if ( !newline && !processor_output() ) {
		    myLabel->newLine();
		}
		*myLabel << " <" << replicas() << ">";
	    }
	}
    }
    if ( Flags::have_results && Flags::print[PROCESS_UTIL].value.b ) {
	myLabel->newLine() << begin_math( &Label::rho ) << "=" << utilization() << end_math();
	if ( hasBogusUtilization() && Flags::print[COLOUR].value.i != COLOUR_OFF ) {
	    myLabel->colour(Graphic::RED);
	}
    }
    return *this;
}



/*
 * Add a task to the list of tasks for this processor.
 */

Processor&
Processor::addTask( Task * aTask )
{
    taskList.insert(aTask);
    return *this;
}



Processor&
Processor::removeTask( Task * aTask )
{
    taskList.erase(aTask);
    return *this;
}


/*
 * Add a share to the list of shares for this processor.
 */

Processor&
Processor::addShare( Share * aShare )
{
    shareList.insert(aShare);
    return *this;
}



Processor&
Processor::removeShare( Share * aShare )
{
    shareList.erase(aShare);
    return *this;
}



#if defined(REP2FLAT)
Processor *
Processor::find_replica( const string& processor_name, const unsigned replica )
{
    ostringstream aName;
    aName << processor_name << "_" << replica;
    set<Processor *,ltProcessor>::const_iterator nextProcessor = find_if( ::processor.begin(), ::processor.end(), eqProcStr( aName.str() ) );
    if ( nextProcessor == ::processor.end() ) {
	string msg = "Processor::find_replica: cannot find symbol ";
	msg += aName.str();
	throw runtime_error( msg );
    }
    return *nextProcessor;
}


Processor *
Processor::expandProcessor( const int replica ) const
{
    Processor *aProcessor = clone( replica );
    ::processor.insert( aProcessor );
    return aProcessor;
}
#endif

/* ------------------------------------------------------------------------ */
/*                                  Output                                  */
/* ------------------------------------------------------------------------ */


ostream&
Processor::draw( ostream& output ) const
{
    ostringstream aComment;
    aComment << "Processor "
	     << name() 
	     << proc_scheduling_of( *this );
    myNode->comment( output, aComment.str() );

    Point aPoint = center();
    double r = fabs( height() / 2.0 );
    myNode->penColour( colour() == Graphic::GREY_10 ? Graphic::BLACK : colour() ).fillColour( colour() );
    /* draw bottom first because we're going to overwrite */
    if ( isMultiServer() || isInfinite() || isReplicated() ) {
	int aDepth = myNode->depth();
	myNode->depth( myNode->depth() + 1 );
	aPoint.moveBy( 2.0 * Model::scaling(), -2.0 * Model::scaling() * myNode->direction() );
	myNode->circle( output, aPoint, r );
	myNode->depth( aDepth );
    }
    myNode->circle( output, center(), r );

    myLabel->backgroundColour( colour() ).comment( output, aComment.str() );
    output << *myLabel;
    return output;
}

/*----------------------------------------------------------------------*/
/*		 	   Called from yyparse.  			*/
/*----------------------------------------------------------------------*/

/*
 * Find the processor and return it.  If processor_name is nil, then
 * create a new processor with task_name.
 */

Processor *
Processor::find( const string& name )
{
    set<Processor *,ltProcessor>::const_iterator nextProcessor = find_if( ::processor.begin(), ::processor.end(), eqProcStr( name ) );
    if ( nextProcessor == ::processor.end() ) {
	return 0;
    } else {
	return *nextProcessor;
    }
}


/*
 * Add a processor to the model.   
 */

Processor * 
Processor::create( const LQIO::DOM::Processor* domProcessor )
{
    if ( Processor::find( domProcessor->getName() ) ) {
	LQIO::input_error2( LQIO::ERR_DUPLICATE_SYMBOL, "Processor", domProcessor->getName().c_str() );
	return 0;
    }

    Processor * aProcessor = new Processor( domProcessor );
    ::processor.insert( aProcessor );
    return aProcessor;
}


/*
 * print out the processor scheduling type (and quantum).
 */

static ostream&
proc_scheduling_of_str( ostream& output, const Processor & processor )
{
    output << ' ' << scheduling_type_flag[static_cast<unsigned int>(processor.scheduling())];
    if ( processor.hasQuantum() ) {
	output << ' ' << processor.quantum();
    }
    return output;
}

/*
 * Compare function for processor layering.
 */

int
Processor::compare( const void * n1, const void *n2 )
{
    if ( Flags::sort == NO_SORT ) {
	return 0;
    }
    const Processor * p1 = *static_cast<Processor **>(const_cast<void *>(n1));
    const Processor * p2 = *static_cast<Processor **>(const_cast<void *>(n2));
    if ( p1->meanLevel() - p2->meanLevel() != 0.0 ) {
	return static_cast<int>(copysign( 1.0, p1->meanLevel() - p2->meanLevel()) );
    } else if ( p1->taskDepth() - p2->taskDepth() != 0 ) {
	return p1->taskDepth() - p2->taskDepth();
    } else switch ( Flags::sort ) {
    case REVERSE_SORT: return p2->name() < p1->name();
    case FORWARD_SORT: return p1->name() > p2->name();
    default: return 0;
    }
}



