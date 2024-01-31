/* -*- c++ -*-
 * $Id: processor.cc 16969 2024-01-28 22:57:43Z greg $
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
#include <algorithm>
#include <cmath>
#include <limits>
#include <lqx/SyntaxTree.h>
#include <lqio/dom_document.h>
#include <lqio/dom_processor.h>
#include <lqio/dom_task.h>
#include <lqio/error.h>
#include <lqio/labels.h>
#include "call.h"
#include "entry.h"
#include "errmsg.h"
#include "label.h"
#include "model.h"
#include "pragma.h"
#include "processor.h"
#include "task.h"

std::set<Processor *,LT<Processor> > Processor::__processors;
std::map<std::string,unsigned> Processor::__key_table;		/* For squishName 	*/
std::map<std::string,std::string> Processor::__symbol_table;	/* For rename		*/

/* -------------------------------------------------------------------- */
/* Funky Formatting functions for inline with <<.			*/
/* -------------------------------------------------------------------- */

class SRVNProcessorManip {
public:
    SRVNProcessorManip( std::ostream& (*ff)(std::ostream&, const Processor & ), const Processor & theProcessor ) 
	: f(ff), anProcessor(theProcessor) {}

private:
    std::ostream& (*f)( std::ostream&, const Processor& );
    const Processor & anProcessor;

    friend std::ostream& operator<<(std::ostream & os, const SRVNProcessorManip& m ) 
	{ return m.f(os,m.anProcessor); }
};

static std::ostream& proc_scheduling_of_str( std::ostream&, const Processor & aProcessor );
static inline SRVNProcessorManip proc_scheduling_of( const Processor & aProcessor ) { return SRVNProcessorManip( &proc_scheduling_of_str, aProcessor ); }

/* ------------------------ Constructors etc. ------------------------- */

Processor::Processor( const LQIO::DOM::Processor* dom ) 
    : Entity( dom, __processors.size()+1 ),
      _tasks(),
      _shares(),
      _groupIsSelected(false)
{ 
    if ( Flags::processors() == Processors::NONE ) {
	setSelected(false);
    }
    if ( !isMultiServer() && scheduling() != SCHEDULE_DELAY && !Pragma::defaultProcessorScheduling() ) {
	/* Change scheduling type for uni-processors (usually from FCFS to PS) */
	const_cast<LQIO::DOM::Processor *>(dom)->setSchedulingType(Pragma::processorScheduling());
    }

    const double r = Flags::graphical_output_style == Output_Style::TIMEBENCH ? Flags::icon_height : Flags::entry_height;
    _node = Node::newNode( r, r );
    _label = Label::newLabel();
}



/*
 * Destructor...
 */

Processor::~Processor()
{
}


/*
 * Make a copy of the processor
 */

Processor *
Processor::clone( const std::string& name ) const
{
    std::set<Processor *>::const_iterator nextProcessor = find_if( __processors.begin(), __processors.end(), [&]( Processor * processor ){ return processor->name() == name; } );
    if ( nextProcessor != __processors.end() ) {
	const std::string msg = "Processor::clone(): cannot add symbol " + name;
	throw std::runtime_error( msg );
    }
    LQIO::DOM::Processor * dom = new LQIO::DOM::Processor( *dynamic_cast<const LQIO::DOM::Processor*>(getDOM()) );
    dom->setName( name );
    const_cast<LQIO::DOM::Document *>(getDOM()->getDocument())->addProcessorEntity( dom );
    
    return new Processor( dom );
}

/* ------------------------ Instance Methods -------------------------- */

bool
Processor::hasRate() const
{ 
    const LQIO::DOM::ExternalVariable * rate = dynamic_cast<const LQIO::DOM::Processor *>(getDOM())->getRate();
    double v;
    return rate != nullptr && (!rate->wasSet() || !rate->getValue(v) || v != 1.0);
}

const LQIO::DOM::ExternalVariable& 
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

const LQIO::DOM::ExternalVariable& 
Processor::quantum() const
{
    return *dynamic_cast<const LQIO::DOM::Processor *>(getDOM())->getQuantum();
}

/* -------------------------- Result Queries -------------------------- */

/*
 * Derive from the tasks.
 */

double
Processor::throughput() const
{
    return std::accumulate( tasks().begin(), tasks().end(), 0., Task::sum_throughput );
}


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
    size_t minLevel = std::numeric_limits<std::size_t>::max();

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
    size_t minLevel = std::numeric_limits<std::size_t>::max();
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
	|| scheduling() == SCHEDULE_PPR;
}



/*
 * Return true if we want to print this processor.
 */

bool
Processor::isInteresting() const
{
    return Flags::processors() == Processors::ALL
	|| ((Flags::processors() == Processors::DEFAULT || Flags::processors() == Processors::QUEUEABLE)
	    && clientsCanQueue() )
	|| (Flags::processors() == Processors::NONINFINITE
	    && !isInfinite() )
#if defined(TXT_OUTPUT)
	|| Flags::output_format() == File_Format::TXT
#endif
	|| input_output();
}


/*
 * Return true if there is any chance a queue will form at this processor.
 */

bool
Processor::clientsCanQueue() const
{
    if ( isInfinite() ) {
	return false;
    } else {
	const LQIO::DOM::ExternalVariable * m = dynamic_cast<const LQIO::DOM::Entity *>(getDOM())->getCopies();
	double value;
	if ( m == nullptr || !m->wasSet() || !m->getValue(value) ) return nClients() > 1;
	return nClients() > value;
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
Processor::clients( std::vector<Task *>& clients, const callPredicate aFunc ) const
{
    for ( std::set<Task *>::const_iterator task = tasks().begin(); task != tasks().end(); ++task ) {
	if ( std::none_of( clients.begin(), clients.end(), EQ<Element>((*task)) ) ) {
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
	    return std::numeric_limits<unsigned int>::max();
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
    std::vector<Task *> clients;
    double anIndex = std::numeric_limits<double>::max();
    this->clients( clients );

    for( std::vector<Task *>::const_iterator client = clients.begin(); client != clients.end(); ++client ) {
	anIndex = std::min( anIndex, (*client)->index() );
    }

    return anIndex;
}


/*
 * move the processor and it's arcs.
 */

Processor&
Processor::moveBy( const double dx, const double dy )
{
    _node->moveBy( dx, dy );
    _label->moveBy( dx, dy );

    moveDst();

    return *this;
}




/*
 * move the processor and it's arcs.
 */

Processor&
Processor::moveTo( const double x, const double y )
{
    _node->moveTo( x, y );
    _label->moveTo( center() );

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

Graphic::Colour
Processor::colour() const
{
    if ( isSurrogate() ) {
	return Graphic::Colour::GREY_10;
    }
    switch ( Flags::colouring() ) {
    case Colouring::SERVER_TYPE:
	return Graphic::Colour::BLUE;
    }
    return Entity::colour();
}


/*
 * Label the node.
 */

Processor&
Processor::label()
{
    *_label << name();
    if ( queueing_output() ) {
	if ( Flags::print_input_parameters() ) {
	    for ( std::set<Task *>::const_iterator task = tasks().begin(); task != tasks().end(); ++task ) {
		if ( utilization() > 0. ) {
		    /* We have results... */
		    (*task)->labelQueueingNetwork( &Entry::labelQueueingNetworkProcessorResponseTime, *_label );
		} else {
		    (*task)->labelQueueingNetwork( &Entry::labelQueueingNetworkProcessorServiceTime, *_label );
		}
	    }
	}
    } else {
	if ( scheduling() != SCHEDULE_FIFO && !isInfinite() ) {
	    *_label << "*";
	}
	if ( Flags::print_input_parameters() ) {
	    bool newline = false;
	    if ( isMultiServer() ) {
		if ( !processor_output() ) {
		    _label->newLine();
		    newline = true;
		}
		*_label << "{" << copies() << "}";
	    } else if ( isInfinite() ) {
		if ( !processor_output() ) {
		    _label->newLine();
		    newline = true;
		}
		*_label << "{" << _infty() << "}";
	    }
	    if ( isReplicated() ) {
		if ( !newline && !processor_output() ) {
		    _label->newLine();
		}
		*_label << " <" << replicas() << ">";
	    }
	}
    }
    if ( Flags::have_results ) {
 	bool print_goop = false;
	if ( Flags::print[TASK_THROUGHPUT].opts.value.b ) {
	    _label->newLine();
	    if ( throughput() == 0.0 && Flags::colouring() != Colouring::NONE ) {
		_label->colour( Graphic::Colour::RED );
	    }
	    *_label << begin_math( &Label::lambda ) << "=" << opt_pct(throughput());
	    print_goop = true;
	}
	if ( Flags::print[PROCESSOR_UTILIZATION].opts.value.b ) {
	    if ( print_goop ) {
		*_label << ',';
	    } else {
		_label->newLine() << begin_math();
		print_goop = true;
	    }
	    *_label << _rho() << "=" << opt_pct(utilization());
	    if ( hasBogusUtilization() && Flags::colouring() != Colouring::NONE ) {
		_label->colour(Graphic::Colour::RED);
	    }
	}
	if ( print_goop ) {
	    *_label << end_math();
	}
    }
    return *this;
}



/*
 * demand is map<class,<visits,service>> for this station.  Processors
 * are always servers, so label for all classes.
 */

Processor&
Processor::labelBCMPModel( const BCMP::Model::Station::Class::map_t& demands, const std::string& )
{
    *_label << name();
    if ( isMultiServer() ) {
	*_label << "{" << copies() << "}";
    } else if ( isInfinite() ) {
	*_label << "{" << _infty() << "}";
    }
    for ( BCMP::Model::Station::Class::map_t::const_iterator demand = demands.begin(); demand != demands.end(); ++demand ) {
	_label->newLine();
	*_label << demand->first << "(" << *demand->second.visits() << "," << *demand->second.service_time() << ")";
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



#if REP2FLAT
Processor *
Processor::find_replica( const std::string& processor_name, const unsigned replica )
{
    std::ostringstream name;
    name << processor_name << "_" << replica;
    std::set<Processor *>::const_iterator nextProcessor = find_if( __processors.begin(), __processors.end(), [&]( Processor * processor ){ return processor->name() == name.str(); } );
    if ( nextProcessor == __processors.end() ) {
	std::string msg = "Processor::find_replica: cannot find symbol ";
	msg += name.str();
	throw std::runtime_error( msg );
    }
    return *nextProcessor;
}


Processor&
Processor::expand()
{
    unsigned int replicas = replicasValue();
    for ( unsigned int replica = 1; replica <= replicas; replica++ ) {
	std::ostringstream new_name;
	new_name << name() << "_" << replica;
	Processor * new_processor = clone( new_name.str() );
	__processors.insert( new_processor );

	/* Patch up observations */

	if ( replica == 1 ) {
	    cloneObservations( getDOM(), new_processor->getDOM() );
	}
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
	if ( !rc.second ) throw std::runtime_error( "Duplicate processor" );
	(*root)->setName( root_name );
	const_cast<LQIO::DOM::Processor *>(dynamic_cast<const LQIO::DOM::Processor *>((*root)))->clearTaskList();
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
Processor::draw( std::ostream& output ) const
{
    std::ostringstream aComment;
    aComment << "Processor "
	     << name() 
	     << proc_scheduling_of( *this );
    _node->comment( output, aComment.str() );

    Point aPoint = center();
    double r = fabs( height() / 2.0 );
    _node->penColour( colour() == Graphic::Colour::GREY_10 ? Graphic::Colour::BLACK : colour() ).fillColour( colour() );
    /* draw bottom first because we're going to overwrite */
    if ( isMultiServer() || isInfinite() || isReplicated() ) {
	int aDepth = _node->depth();
	_node->depth( _node->depth() + 1 );
	aPoint.moveBy( 2.0 * Model::scaling(), -2.0 * Model::scaling() * _node->direction() );
	_node->circle( output, aPoint, r );
	_node->depth( aDepth );
    }
    _node->circle( output, center(), r );

    _label->backgroundColour( colour() ).comment( output, aComment.str() );
    output << *_label;
    return *this;
}


/*+ BUG_323 */

/* 
 * Find the total demand by each class (client tasks in submodel), then change back to visits/service time when needed.  Demands are
 * stored in entries of tasks.
 */

void
Processor::accumulateDemand( const Entry& entry, const std::string& class_name, BCMP::Model::Station& station ) const
{
    BCMP::Model::Station::Class& k = station.classAt(class_name);
    LQX::ConstantValueExpression * visits = new LQX::ConstantValueExpression(entry.visitProbability());
    k.accumulate( BCMP::Model::Station::Class( visits, entry.serviceTime()) );
}

void
Processor::accumulateResponseTime( const Entry& entry, const std::string& class_name, BCMP::Model::Station& station ) const
{
    accumulateDemand( entry, class_name, station );
}
/*- BUG_323 */

/*----------------------------------------------------------------------*/
/*		 	   Called from yyparse.  			*/
/*----------------------------------------------------------------------*/

/*
 * Find the processor and return it.  If processor_name is nil, then
 * create a new processor with task_name.
 */

Processor *
Processor::find( const std::string& name )
{
    std::set<Processor *>::const_iterator processor = find_if( __processors.begin(), __processors.end(), [&]( Processor * processor ){ return processor->name() == name; } );
    return processor != __processors.end() ? *processor : nullptr;
}


/*
 * Add a processor to the model.   
 */

Processor * 
Processor::create( const std::pair<std::string,LQIO::DOM::Processor *>& p )
{
    const std::string& name = p.first;
    const LQIO::DOM::Processor* dom = p.second;
    if ( Processor::find( name ) ) {
	dom->input_error( LQIO::ERR_DUPLICATE_SYMBOL );
	return nullptr;
    }

    Processor * aProcessor = new Processor( dom );
    __processors.insert( aProcessor );
    return aProcessor;
}


/*
 * print out the processor scheduling type (and quantum).
 */

static std::ostream&
proc_scheduling_of_str( std::ostream& output, const Processor & processor )
{
    output << ' ' << scheduling_label.at(processor.scheduling()).str;
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
    if ( Flags::sort == Sorting::NONE ) {
	return false;
    }
    const Processor * p1 = *static_cast<Processor **>(const_cast<void *>(n1));
    const Processor * p2 = *static_cast<Processor **>(const_cast<void *>(n2));
    if ( p1->meanLevel() - p2->meanLevel() != 0.0 ) {
	return p1->meanLevel() < p2->meanLevel();
    } else if ( p1->taskDepth() - p2->taskDepth() != 0 ) {
	return p1->taskDepth() < p2->taskDepth();
    } else switch ( Flags::sort ) {
	case Sorting::REVERSE: return p2->name() > p1->name();
	case Sorting::FORWARD: return p1->name() < p2->name();
    default: return false;
    }
}



