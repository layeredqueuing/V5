/* -*- c++ -*-
 * $Id: processor.cc 13523 2020-03-03 16:19:29Z greg $
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
#include "model.h"

std::set<Processor *,LT<Processor> > Processor::__processors;

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

/* ------------------------ Constructors etc. ------------------------- */

Processor::Processor( const LQIO::DOM::Processor* aDomObject ) 
    : Entity( aDomObject, __processors.size()+1 ),
      _tasks(),
      _shares(),
      _groupIsSelected(false)
{ 
    if ( Flags::print[PROCESSORS].value.i == PROCESSOR_NONE ) {
	isSelected(false);
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
    _tasks.clear();
}


/*
 * Make a copy of the processor
 */

Processor *
Processor::clone( const std::string& new_name ) const
{
    std::set<Processor *>::const_iterator nextProcessor = find_if( __processors.begin(), __processors.end(), EQStr<Processor>( new_name ) );
    if ( nextProcessor != __processors.end() ) {
	string msg = "Processor::expandProcessor(): cannot add symbol ";
	msg += new_name;
	throw runtime_error( msg );
    }
    LQIO::DOM::Processor * dom = new LQIO::DOM::Processor( *dynamic_cast<const LQIO::DOM::Processor*>(getDOM()) );
    dom->setName( new_name );
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

size_t
Processor::taskDepth() const
{ 
    size_t minLevel = UINT_MAX;

    for ( std::set<Task *>::const_iterator nextTask = tasks().begin(); nextTask != tasks().end(); ++nextTask ) {
	const Task& aTask = **nextTask;
	minLevel = std::min( minLevel, aTask.level() );
    }
    return minLevel;
}



/*
 * Used for sorting.
 */

double
Processor::meanLevel() const
{
    size_t minLevel = UINT_MAX;
    size_t maxLevel = 0;
    for ( std::set<Task *>::const_iterator nextTask = tasks().begin(); nextTask != tasks().end(); ++nextTask ) {
	const Task& aTask = **nextTask;
	minLevel = std::min( minLevel, aTask.level() );
	maxLevel = std::max( maxLevel, aTask.level() );
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


bool
Processor::clientsCanQueue() const
{
    if ( isInfinite() ) return false;
    else {
	const LQIO::DOM::ExternalVariable * m = dynamic_cast<const LQIO::DOM::Entity *>(getDOM())->getCopies();
	double value;
	return m == NULL || !m->wasSet() || !m->getValue(value) || nClients() > value;
    }
}


/*
 * Return all clients to this processor.
 */

unsigned
Processor::referenceTasks( std::vector<Entity *>& clientCltn, Element * dst ) const
{
    for ( std::set<Task *>::const_iterator nextTask = tasks().begin(); nextTask != tasks().end(); ++nextTask ) {
	const Task& aTask = **nextTask;
	aTask.referenceTasks( clientCltn, dst );
    }
    return clientCltn.size();
}


/*
 * Return all clients to this processor.
 */

unsigned
Processor::clients( std::vector<Entity *>& clients, const callPredicate aFunc ) const
{
    for ( std::set<Task *>::const_iterator task = tasks().begin(); task != tasks().end(); ++task ) {
	if ( find_if( clients.begin(), clients.end(), EQ<Element>((*task)) ) == clients.end() ) {
	    clients.push_back((*task));
	}
    }
    return clients.size();
}


/*
 * Return the number of instances of all tasks calling this processor 
 */

unsigned
Processor::nClients() const
{
    unsigned sum = 0;
    for ( std::set<Task *>::const_iterator nextTask = tasks().begin(); nextTask != tasks().end(); ++nextTask ) {
	const Task& aTask = **nextTask;
	if ( aTask.isInfinite() ) {
	    return UINT_MAX;
	}
	const LQIO::DOM::ExternalVariable * copies = dynamic_cast<const LQIO::DOM::Task *>(aTask.getDOM())->getCopies();
	double value = 1;
	if ( copies && copies->wasSet() ) {
	    copies->getValue(value);
	}
	sum += value * aTask.countThreads();
    }
    return sum;
}



double
Processor::getIndex() const
{
    std::vector<Entity *> clients;
    double anIndex = MAXDOUBLE;
    this->clients( clients );

    for( std::vector<Entity *>::const_iterator client = clients.begin(); client != clients.end(); ++client ) {
	anIndex = min( anIndex, (*client)->index() );
    }

    return anIndex;
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
    Point aPoint = center();

    /* Draw other incomming arcs. */

    for ( std::vector<GenericCall *>::const_iterator call = callers().begin(); call != callers().end(); ++call ) {
	const_cast<Task *>((*call)->srcTask())->moveBy(0.0,0.0);
	(*call)->moveDst( aPoint );
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
	for ( std::set<Task *>::const_iterator nextTask = tasks().begin(); nextTask != tasks().end(); ++nextTask ) {
	    const Task * aTask = *nextTask;
	    for ( std::vector<Entry *>::const_iterator entry = aTask->entries().begin(); entry != aTask->entries().end(); ++entry ) {
		*myLabel << (*entry)->name() << " (" << print_number_slices( *(*entry) ) << ")";
		myLabel->newLine();
	    }
	    myLabel->newLine();
	    Entity::label();
	    for ( std::vector<Entry *>::const_iterator entry = aTask->entries().begin(); entry != aTask->entries().end(); ++entry ) {
		myLabel->newLine() << (*entry)->name() << " [" << print_slice_time( *(*entry) ) << "]";
	    }
	}
    } else {
	*myLabel << name();
	if ( scheduling() != SCHEDULE_FIFO && !isInfinite() ) {
	    *myLabel << "*";
	}
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
    if ( Flags::have_results && Flags::print[PROCESSOR_UTILIZATION].value.b ) {
	myLabel->newLine() << begin_math( &Label::rho ) << "=" << opt_pct(utilization()) << end_math();
	if ( hasBogusUtilization() && Flags::print[COLOUR].value.i != COLOUR_OFF ) {
	    myLabel->colour(Graphic::RED);
	}
    }
    return *this;
}



/*
 * Rename processors
 */

Processor&
Processor::rename()
{
    std::ostringstream name;
    name << "p" << elementId();
    const_cast<LQIO::DOM::DocumentObject *>(getDOM())->setName( name.str() );
    return *this;
}



#if defined(REP2FLAT)
Processor *
Processor::find_replica( const string& processor_name, const unsigned replica )
{
    ostringstream aName;
    aName << processor_name << "_" << replica;
    std::set<Processor *>::const_iterator nextProcessor = find_if( __processors.begin(), __processors.end(), EQStr<Processor>( aName.str() ) );
    if ( nextProcessor == __processors.end() ) {
	string msg = "Processor::find_replica: cannot find symbol ";
	msg += aName.str();
	throw runtime_error( msg );
    }
    return *nextProcessor;
}


Processor&
Processor::expandProcessor()
{
    unsigned int numProcReplicas = replicasValue();
    for ( unsigned int replica = 1; replica <= numProcReplicas; replica++) {
	ostringstream new_name;
	new_name << name() << "_" << replica;
	__processors.insert( clone( new_name.str() ) );
    }
    return *this;
}


/*
 * Rename XXX_1 to XXX and reinsert the processor and its dom into their associated arrays.   XXX_2 and up will be discarded.
 */

Processor&
Processor::replicateProcessor( LQIO::DOM::DocumentObject ** root )
{
    unsigned int replica = 0;
    std::string root_name = baseReplicaName( replica );
    if ( replica == 1 ) {
	*root = const_cast<LQIO::DOM::DocumentObject *>(getDOM());
	std::pair<std::set<Processor *>::iterator,bool> rc = __processors.insert( this );
	if ( !rc.second ) throw runtime_error( "Duplicate processor" );
	(*root)->setName( root_name );
	const_cast<LQIO::DOM::Document *>((*root)->getDocument())->addProcessorEntity( dynamic_cast<LQIO::DOM::Processor *>(*root) );
    } else if ( dynamic_cast<LQIO::DOM::Processor *>(*root)->getReplicasValue() < replica ) {
	dynamic_cast<LQIO::DOM::Processor *>(*root)->setReplicasValue( replica );
	update_mean( *root, &LQIO::DOM::DocumentObject::setResultUtilization, getDOM(), &LQIO::DOM::DocumentObject::getResultUtilization, replica );
	update_variance( *root, &LQIO::DOM::DocumentObject::setResultUtilizationVariance, getDOM(), &LQIO::DOM::DocumentObject::getResultUtilizationVariance );
    }
    return *this;
}

#endif

/* ------------------------------------------------------------------------ */
/*                                  Output                                  */
/* ------------------------------------------------------------------------ */


const Processor&
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
    return *this;
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
    std::set<Processor *>::const_iterator processor = find_if( __processors.begin(), __processors.end(), EQStr<Processor>( name ) );
    return processor != __processors.end() ? *processor : 0;
}


/*
 * Add a processor to the model.   
 */

Processor * 
Processor::create( const std::pair<std::string,LQIO::DOM::Processor *>& p )
{
    const std::string& name = p.first;
    const LQIO::DOM::Processor* domProcessor = p.second;
    if ( Processor::find( name ) ) {
	LQIO::input_error2( LQIO::ERR_DUPLICATE_SYMBOL, "Processor", name.c_str() );
	return 0;
    }

    Processor * aProcessor = new Processor( domProcessor );
    __processors.insert( aProcessor );
    return aProcessor;
}


/*
 * print out the processor scheduling type (and quantum).
 */

static ostream&
proc_scheduling_of_str( ostream& output, const Processor & processor )
{
    output << ' ' << scheduling_label[static_cast<unsigned int>(processor.scheduling())].str;
    if ( processor.hasQuantum() ) {
	output << ' ' << processor.quantum();
    }
    return output;
}

/*
 * Compare function for processor layering.
 */

bool
Processor::compare( const void * n1, const void *n2 )
{
    if ( Flags::sort == NO_SORT ) {
	return false;
    }
    const Processor * p1 = *static_cast<Processor **>(const_cast<void *>(n1));
    const Processor * p2 = *static_cast<Processor **>(const_cast<void *>(n2));
    if ( p1->meanLevel() - p2->meanLevel() != 0.0 ) {
	return p1->meanLevel() < p2->meanLevel();
    } else if ( p1->taskDepth() - p2->taskDepth() != 0 ) {
	return p1->taskDepth() < p2->taskDepth();
    } else switch ( Flags::sort ) {
    case REVERSE_SORT: return p2->name() > p1->name();
    case FORWARD_SORT: return p1->name() < p2->name();
    default: return false;
    }
}



