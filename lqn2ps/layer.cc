/* layer.cc	-- Greg Franks Tue Jan 28 2003
 *
 * $Id$
 *
 * A layer consists of a set of tasks with the same nesting depth from
 * reference tasks.  Reference tasks are in layer 1, the immediate
 * servers to reference tasks are in layer 2, etc.  There are variants
 * of this, bien sur.
 */

#include "lqn2ps.h"
#include <cstdlib>
#include <algorithm>
#include <lqio/error.h>
#include <lqio/srvn_output.h>
#include <lqio/dom_document.h>
#if defined(HAVE_VALUES_H)
#include <values.h>
#endif
#if defined(HAVE_FLOAT_H)
#include <float.h>
#endif
#include "layer.h"
#include "cltn.h"
#include "entity.h"
#include "task.h"
#include "processor.h"
#include "entry.h"
#include "activity.h"
#include "arc.h"
#include "label.h"
#include "open.h"


#if !defined(MAXDOUBLE)
#define MAXDOUBLE FLT_MAX
#endif

/*----------------------------------------------------------------------*/
/*                         Helper Functions                             */
/*----------------------------------------------------------------------*/

/*
 * Print all results.
 */

ostream&
operator<<( ostream& output, const Layer& self )
{
    return self.print( output );
}

Layer::Layer()  
    : myOrigin(0,0), myExtent(0,0), myNumber(0), myLabel(0), myChains(0)
{
    myLabel = Label::newLabel();
}


Layer::~Layer()  
{
    if ( myLabel ) {
	delete myLabel;
    }
}

Layer&
Layer::operator<<( Entity * elem )
{
    myEntities << elem;
    return *this;
}



Layer&
Layer::operator+=( Entity * elem )
{
    myEntities += elem;
    return *this;
}


Layer&
Layer::operator-=( Entity * elem )
{
    myEntities -= elem;
    return *this;
}

Layer const&
Layer::rename() const
{
    Sequence<Entity *> nextEntity( entities() );
    Entity * anEntity;
    while ( anEntity = nextEntity() ) {
	anEntity->rename();
    }
    return *this;
}


Layer& 
Layer::number( const unsigned n ) 
{ 
    myNumber = n; 

    return *this; 
}



Layer const&
Layer::check() const
{
    Sequence<Entity *> nextEntity( entities() );
    Entity * anEntity;
    while ( anEntity = nextEntity() ) {
	anEntity->check();
    }
    return *this;
}



/*
 * delete unused items from layers.
 */

Layer&
Layer::prune() 
{
    for ( unsigned x = myEntities.size(); x > 0; --x ) {
	Entity * anEntity = myEntities[x];
	Task * aTask = dynamic_cast<ReferenceTask *>(anEntity);
	Processor * aProc = dynamic_cast<Processor *>(anEntity);

	if ( aTask && !aTask->hasCalls( &Call::hasAnyCall ) ) {

	    myEntities -= anEntity;

	} else if ( aProc ) {

	    /* 
	     * If a processor is not refereneced, or if the task was
	     * removed because it's a reference task that makes no
	     * calls, then delete the processor. 
	     */

	    if ( aProc->nTasks() == 0 ) {
		myEntities -= anEntity;
	    } else {
		for ( set<Task *,ltTask>::const_iterator nextTask = aProc->tasks().begin(); nextTask != aProc->tasks().end(); ++nextTask ) {
		    const Task& aTask = **nextTask;
		    if ( !aTask.isReferenceTask() || aTask.hasCalls( &Call::hasAnyCall ) ) goto found;
		}
		myEntities -= anEntity;
	    }
	found:;
	}
    }
    return *this;
}



const Layer&
Layer::aggregate() const
{
    Sequence<Entity *> nextEntity( entities() );
    Entity * anEntity;
    while ( anEntity = nextEntity() ) {
	anEntity->aggregate();
    }
    return *this;
}



const Layer&
Layer::sort( compare_func_ptr compare ) const
{
    myEntities.sort( compare );
    return *this;
}



Layer const&
Layer::format( const double y ) const
{
    myOrigin.moveTo( 0., y );
    myExtent.moveTo( 0., 0. );
    Sequence<Entity *> nextEntity( entities() );
    Entity * anEntity;
    for ( double x = 0; anEntity = nextEntity(); ) {
	if ( !anEntity->isSelectedIndirectly() ) continue;
	Task * aTask = dynamic_cast<Task *>(anEntity);
	if ( aTask && Flags::print[AGGREGATION].value.i != AGGREGATE_ENTRIES ) {
	    aTask->format();
	}
	x = moveTo( x, y, anEntity );
    }
    return *this;
}


Layer const&
Layer::reformat() const
{
    myOrigin.moveTo( 0.0, MAXDOUBLE );
    myExtent.moveTo( 0.0, 0.0 );
    Sequence<Entity *> nextEntity( entities() );
    Entity * anEntity;
    for ( double x = 0; anEntity = nextEntity(); ) {
	if ( !anEntity->isSelectedIndirectly() ) continue;
	Task * aTask = dynamic_cast<Task *>(anEntity);
	if ( aTask && Flags::print[AGGREGATION].value.i != AGGREGATE_ENTRIES ) {
	    aTask->reformat();
	}
	x = moveTo( x, anEntity->bottom(), anEntity );
	myOrigin.y( min( myOrigin.y(), anEntity->bottom() ) );
    }
    return *this;
}


double
Layer::moveTo( double x, const double y, Entity * anEntity ) const
{
    anEntity->moveTo( x, y );
    x += anEntity->width();
    myExtent.moveTo( x, max( myExtent.y(), anEntity->height() ) );
    x += Flags::print[X_SPACING].value.f;
    return x;
}



Layer const&
Layer::moveBy( const double dx, const double dy )  const
{
    myOrigin.moveBy( dx, dy );

    Sequence<Entity *> nextEntity( entities() );
    Entity * anEntity;
    while ( anEntity = nextEntity() ) {
	anEntity->moveBy( dx, dy );
    }
    if ( myLabel ) {
	myLabel->moveBy( dx, dy );
    }
    return *this;
}



/* 
 * Move the label to...
 */

Layer const&
Layer::moveLabelTo( const double xx, const double yy ) const
{
    if ( Flags::print_layer_number ) {
	myLabel->justification( LEFT_JUSTIFY ).moveTo( xx, y() + yy );
    } else if ( submodel_output() ) {
	myLabel->moveTo( myOrigin.x() + myExtent.x() / 2.0, myOrigin.y() - Flags::print[FONT_SIZE].value.i * 1.2 );
    }
    return *this;
}



Layer const&
Layer::scaleBy( const double sx, const double sy ) const
{
    Sequence<Entity *> nextEntity( entities() );
    Entity * anEntity;
    while  ( anEntity = nextEntity() ) {
	anEntity->scaleBy( sx, sy );
    }
    myOrigin.scaleBy( sx, sy );
    myExtent.scaleBy( sx, sy );

    if ( myLabel ) {
	myLabel->scaleBy( sx, sy );
    }
    return *this;
}



Layer const&
Layer::translateY( const double dy )  const
{
    Sequence<Entity *> nextEntity( entities() );
    Entity * anEntity;
    while ( anEntity = nextEntity() ) {
	anEntity->translateY( dy );
    }
    myOrigin.y( dy - myOrigin.y() );

    if ( myLabel ) {
	myLabel->translateY( dy );
    }
    return *this;
}



Layer const&
Layer::depth( const unsigned layer ) const
{
    Sequence<Entity *> nextEntity( entities() );
    Entity * anEntity;
    while ( anEntity = nextEntity() ) {
	anEntity->depth( layer );
    }
    return *this;
}


Layer const&
Layer::fill( const double maxWidthPts ) const
{
    double width = 0.0;
    Sequence<Entity *> nextEntity( entities() );
    Entity * anEntity;
    while ( anEntity = nextEntity() ) {
	width += anEntity->width();
    }
    
    const double fill = max( 0.0, (maxWidthPts - width) / static_cast<double>(entities().size() + 1) );
    if ( fill < Flags::print[X_SPACING].value.f ) return *this;		/* Don't bother... */

    myOrigin.x( fill );
    
    for ( double x = fill; anEntity = nextEntity(); x += fill ) {
	const double y = anEntity->bottom();
	anEntity->moveTo( x, y );
	x += anEntity->width();
	myExtent.x( x - fill );
    }

    return *this;
}



Layer const&
Layer::justify( const double width ) const
{
    return justify( width, Flags::node_justification );
}


Layer const&
Layer::justify( const double maxWidthPts, const justification_type justification ) const
{
    switch ( justification ) {
    case ALIGN_JUSTIFY:
    case DEFAULT_JUSTIFY:
    case CENTER_JUSTIFY:
	moveBy( (maxWidthPts - width())/2, 0.0 );
	break;
    case RIGHT_JUSTIFY:
	moveBy( (maxWidthPts - width()), 0.0 );
	break;
    case LEFT_JUSTIFY:
	moveBy( 0.0, 0.0 );		/* Force recomputation of slopes */
	break;
    default:
	abort();	
    }
    return *this;
}



Layer const&
Layer::align( const double height )  const
{
    Sequence<Entity *> nextEntity( entities() );
    Entity * anEntity;
    while ( anEntity = nextEntity() ) {
	double delta = height - anEntity->height();
	if ( delta > 0. ) {
	    anEntity->moveBy( 0, delta );
	}
    }
    return *this;
}



/*
 * Align tasks between layers.
 */

const Layer&
Layer::alignEntities() const
{
    /* Move objects right starting from the right side */
    for ( unsigned int i = size(); i > 0; --i ) {
	shift( i, myEntities[i]->align() );
    }
    /* Move objects left starting from the left side */
    for ( unsigned int i = 1; i <= size(); ++i ) {
	shift( i, myEntities[i]->align() );
    }

    return *this;
}



/*
 * Move object `i' by `amount' if possible.  The bounding box is recomputed if necessary.
 */

Layer const&
Layer::shift( unsigned i, double amount ) const
{
    if ( amount < 0.0 ) {
	/* move left if I can */
	if ( i == 1 ) {
	    myEntities[i]->moveBy( amount, 0 );
	    myOrigin.moveBy( amount, 0 );
	} else {
	    myEntities[i]->moveBy( min( max( (myEntities[i-1]->right() + Flags::print[X_SPACING].value.f) - myEntities[i]->left(), amount ), 0 ), 0 );
	}
	myOrigin.x( myEntities[1]->left() );
	myExtent.x( myEntities[size()]->right() - myEntities[1]->left() );
	if ( i < size() && myEntities[i]->forwardsTo( dynamic_cast<Task *>(myEntities[i+1]) ) ) {
	    shift( i+1, amount );
	} 
    } else if ( amount > 0.0 ) { 
	/* move right if I can */
	if ( i == size() ) {
	    myEntities[i]->moveBy( amount, 0 );
	} else {
	    myEntities[i]->moveBy( max( min( myEntities[i+1]->left() - (myEntities[i]->right() + Flags::print[X_SPACING].value.f), amount ), 0 ), 0 );
	}
	myOrigin.x( myEntities[1]->left() );
	myExtent.x( myEntities[size()]->right() - myEntities[1]->left() );
	if ( i > 1 && myEntities[i-1]->forwardsTo( dynamic_cast<Task *>(myEntities[i]) ) ) {
	    shift( i-1, amount );
	}
    }
    return *this;
}



/*
 * Label all objects in this layer.
 */

Layer const&
Layer::label() const
{
    Sequence<Entity *> nextEntity( entities() );
    Entity * anEntity;
    while ( anEntity = nextEntity() ) {
	anEntity->label();
    }
    if ( Flags::print_layer_number ) {
	myLabel->initialize( "Layer " ) << number();
    }
    return *this;
}


/*
 * Select all servers in this submodel for printing.
 */

Layer const&
Layer::selectSubmodel() const
{
    Sequence<Entity *> nextServer( entities() );
    Entity * aServer;
    while ( aServer = nextServer() ) {
	if ( !aServer->isProcessor() || Flags::print[PROCESSORS].value.i != PROCESSOR_NONE ) {
	    aServer->isSelected( true );		/* Enable arc drawing to this entity */
	}
    }
    return *this;
}


/*
 * Select all servers in this submodel for printing.
 */

Layer const&
Layer::deselectSubmodel() const
{
    Sequence<Entity *> nextServer( entities() );
    Entity * aServer;
    while ( aServer = nextServer() ) {
	aServer->isSelected( false );		/* Enable arc drawing to this entity */
    }
    return *this;
}


const Layer&
Layer::generateSubmodel() const
{
    if ( myClients.size() > 0 ) return *this;

    myChains = 0;

    /* Find clients */

    Sequence<Entity *> nextServer( entities() );
    Entity * aServer;
    while ( aServer = nextServer() ) {
	if ( aServer->isSelected() ) {
	    aServer->clients( myClients );		/* Now find out who calls it */
	}
    }

    /* Set the chains in the model */

    Sequence<const Entity *> nextClient(myClients);
    const Entity * aClient;

    while ( aClient = nextClient() ) {
	if ( aClient->isInClosedModel( entities() ) ) {
	    myChains += 1;
	    myChains = const_cast<Entity *>(aClient)->setChain( myChains, &GenericCall::hasRendezvous );
	}
	if ( aClient->isInOpenModel( entities() ) ) {
	    myChains += 1;
	    myChains = const_cast<Entity *>(aClient)->setChain( myChains, &GenericCall::hasSendNoReply );
	}
    }

    return *this;
}


const Layer&
Layer::generateClientSubmodel() const
{
    myChains = 0;

    /* Find clients */

    Sequence<Entity *> nextServer( entities() );
    Entity * aServer;
    while ( aServer = nextServer() ) {
	if ( aServer->isSelected() ) {
	    aServer->referenceTasks( myClients, aServer );		/* Now find out who calls it */
	}
    }

    /* Set the chains in the model */

    Sequence<const Entity *> nextClient( myClients );
    const Entity * aClient;

    while ( aClient = nextClient() ) {
	if ( aClient->isInClosedModel( entities() ) ) {
	    myChains += 1;
	    myChains = const_cast<Entity *>(aClient)->setChain( myChains, &GenericCall::hasRendezvous );
	}
	if ( aClient->isInOpenModel( entities() ) ) {
	    myChains += 1;
	    myChains = const_cast<Entity *>(aClient)->setChain( myChains, &GenericCall::hasSendNoReply );
	}
    }

    return *this;
}


/*+ BUG_626 */
const Layer&
Layer::transmorgrify( LQIO::DOM::Document * document, Processor *& surrogate_processor, Task *& surrogate_task ) const
{
    Sequence<Entity *> nextServer( entities() );
    Entity * aServer;
    while ( aServer = nextServer() ) {
	if ( !aServer->isSelected() || !aServer->isTask() ) continue;

	findOrAddSurrogateProcessor( document, surrogate_processor, const_cast<Task *>(dynamic_cast<const Task *>(aServer)), number()+1 );

	/* ---------- Servers ---------- */

	Sequence<Entry *> nextEntry( dynamic_cast<Task *>(aServer)->entries() );
	const Entry * anEntry;
	while ( anEntry = nextEntry() ) {

	    /* These are graphical object calls */

	    Cltn<Call *>& calls = const_cast<Cltn<Call *>& >(anEntry->callList());
	    calls.clearContents();

	    /* Forwarded calls? */

	    const LQIO::DOM::Entry * dom_entry = dynamic_cast<const LQIO::DOM::Entry *>(anEntry->getDOM());
	    const std::map<unsigned, LQIO::DOM::Phase*>& phases = dom_entry->getPhaseList();
	    for ( std::map<unsigned, LQIO::DOM::Phase*>::const_iterator p = phases.begin(); p != phases.end(); ++p ) {
		resetServerPhaseParameters( document, p->second );
	    }
	}

	Sequence<Activity *> nextActivity( dynamic_cast<Task *>(aServer)->activities() );
	const Activity * anActivity;
	while ( anActivity = nextActivity() ) {
	    Cltn<Call *>& calls = const_cast<Cltn<Call *>& >( anActivity->callList() );
	    calls.clearContents();

	    resetServerPhaseParameters( document, const_cast<LQIO::DOM::Phase *>(anActivity->getDOM()) );
	}
    }

    /* ---------- Clients ---------- */

    Sequence<const Entity *> nextClient(myClients);
    const Entity * aClient;
    while ( aClient = nextClient() ) {
	const Task * aTask = dynamic_cast<const Task *>(aClient);
	if ( !aTask ) continue;

	LQIO::DOM::Task * dom_task = const_cast<LQIO::DOM::Task *>(dynamic_cast<const LQIO::DOM::Task *>(aTask->getDOM()));
	dom_task->setSchedulingType( SCHEDULE_CUSTOMER );   // Clients become reference tasks.

	/* Create a fake processor if necessary */

	if ( !aTask->processor()->isSelected() ) {
	    findOrAddSurrogateProcessor( document, surrogate_processor, const_cast<Task *>(dynamic_cast<const Task *>(aTask)), number() );
	}

	/* for all clients, reroute all non-selected calls to surrogate */

	Sequence<Entry *> nextEntry( aTask->entries() );
	const Entry * anEntry;
	while ( anEntry = nextEntry() ) {
	    Sequence<Call *> nextCall( anEntry->callList() );
	    Call * aCall;
	    while ( aCall = nextCall() ) {
		if ( aCall->isSelected() || aCall->dstTask()->isSelectedIndirectly() ) continue;
		findOrAddSurrogateTask( document, surrogate_processor, surrogate_task, const_cast<Entry *>( aCall->dstEntry()), number() );
	    }
	}
	Sequence<Activity *> nextActivity( aTask->activities() );
	const Activity * anActivity;
	while ( anActivity = nextActivity() ) {
	    Sequence<Call *> nextCall( anActivity->callList() );
	    Call * aCall;
	    while ( aCall = nextCall() ) {
		if ( aCall->isSelected() || aCall->dstTask()->isSelectedIndirectly() ) continue;
		findOrAddSurrogateTask( document, surrogate_processor, surrogate_task, const_cast<Entry *>( aCall->dstEntry()), number() );
	    }
	}

	/* set think times. */

	if ( document->hasResults() ) {
	    const double util = dom_task->getResultUtilization();
	    const double tput = dom_task->getResultThroughput();
	    const double n    = static_cast<double>(dom_task->getCopiesValue());
	    if ( util >= n || tput == 0. ) {
		dom_task->setThinkTimeValue( 0. );
	    } else {
		dom_task->setThinkTimeValue( (n - util) / tput );
	    }
	}
    }
    return *this;
}


/*
 * If a surrogate processor does not exist, make one.
 * Then unlink the task from the old processor and link it to the new one.
 */

Processor *
Layer::findOrAddSurrogateProcessor( LQIO::DOM::Document * document, Processor *& processor, Task * task, const unsigned level ) const
{
    LQIO::DOM::Processor * dom_processor = 0;
    if ( !processor ) {
	/* Need to create a new processor */
	dom_processor = new LQIO::DOM::Processor( document, "Surrogate", SCHEDULE_DELAY, new LQIO::DOM::ConstantExternalVariable(1), 1, 0 );
	document->addProcessorEntity( dom_processor );
	processor = new Processor( dom_processor );		/* This is a model object */
	processor->isSelected( true ).isSurrogate( true ).setLevel( level );
	const_cast<Layer *>(this)->myEntities << processor;
	::processor.insert( processor );
    } else {
	dom_processor = const_cast<LQIO::DOM::Processor *>(dynamic_cast<const LQIO::DOM::Processor *>(processor->getDOM()));
    }
    if ( task ) {
	LQIO::DOM::Task * dom_task = const_cast<LQIO::DOM::Task *>(dynamic_cast<const LQIO::DOM::Task *>(task->getDOM()));
	std::vector<LQIO::DOM::Task*>& old_list = const_cast<std::vector<LQIO::DOM::Task*>&>(dom_task->getProcessor()->getTaskList());
	std::vector<LQIO::DOM::Task*>::iterator pos = find( old_list.begin(), old_list.end(), dom_task );
	old_list.erase( pos );		// Remove task from original processor 
	dom_processor->addTask( dom_task );
	dom_task->setProcessor( dom_processor );
	Processor * old_processor = const_cast<Processor *>(task->processor());
	old_processor->removeTask( task );
	processor->addTask( task );

	/* Remap processor call */
//	Point aPoint = processor->center();
	Cltn<EntityCall *>& task_calls = const_cast<Cltn<EntityCall *>& >(task->callList());
	Sequence<EntityCall *> nextCall( task_calls );
	EntityCall * aCall;
	while ( aCall = nextCall() ) {
	    if ( aCall->isProcessorCall() ) {
		dynamic_cast<ProcessorCall *>(aCall)->setDstProcessor( processor );
//		aCall->moveDst( aPoint );
		old_processor->removeDstCall( aCall );
		processor->addDstCall( aCall );
	    }
	}
    }

    return processor;
}

Task *
Layer::findOrAddSurrogateTask( LQIO::DOM::Document* document, Processor*& processor, Task*& task, Entry * entry, const unsigned level ) const
{
    LQIO::DOM::Task * dom_task;
    /* Make sure task exists */
    std::vector<LQIO::DOM::Entry *> dom_entries;
    Cltn<Entry *> entries;
    if ( !task ) {
	findOrAddSurrogateProcessor( document, processor, 0, level+1 );
	LQIO::DOM::Processor * dom_processor = const_cast<LQIO::DOM::Processor *>(dynamic_cast<const LQIO::DOM::Processor *>(processor->getDOM()));
	dom_task = new LQIO::DOM::Task( document, "Surrogate", SCHEDULE_DELAY, dom_entries, 0, 
					dom_processor, 0, 
					new LQIO::DOM::ConstantExternalVariable(1), 1, 0, 0 );
	document->addTaskEntity( dom_task );
	dom_processor->addTask( dom_task );
	task = new ServerTask( dom_task, processor, 0, entries );
	task->isSelected( true ).isSurrogate( true ).setLevel( level );
	const_cast<Layer *>(this)->myEntities << task;
	::task.insert( task );
    }

    /* Now find or create an entry for it. */
    findOrAddSurrogateEntry( document, task, entry );

    return task;
}

Entry *
Layer::findOrAddSurrogateEntry( LQIO::DOM::Document* document, Task* task, Entry * entry ) const
{
    Task * src_task = const_cast<Task *>( entry->owner() );
    if ( src_task == task ) return entry;		/* already moved. */

    src_task->removeEntry( entry );
    task->addEntry( entry );
    entry->owner( task );

    LQIO::DOM::Entry * dom_entry = const_cast<LQIO::DOM::Entry *>(dynamic_cast<const LQIO::DOM::Entry *>(entry->getDOM()));
    LQIO::DOM::Task * old_task = const_cast<LQIO::DOM::Task *>(dom_entry->getTask());

    LQIO::DOM::Task * dom_task = const_cast<LQIO::DOM::Task *>(dynamic_cast<const LQIO::DOM::Task *>(task->getDOM()));
    assert( dom_task != 0 );

    std::vector<LQIO::DOM::Entry*>& old_entries = const_cast<std::vector<LQIO::DOM::Entry*>&>(old_task->getEntryList());
    std::vector<LQIO::DOM::Entry*>::iterator pos = find( old_entries.begin(), old_entries.end(), dom_entry );
    old_entries.erase( pos );
    dom_entry->setTask( dom_task );
    std::vector<LQIO::DOM::Entry*>& dom_entries = const_cast<std::vector<LQIO::DOM::Entry*>&>(dom_task->getEntryList());
    dom_entries.push_back( dom_entry );

    /* This is a server, so reset it. */

    dom_entry->setEntryType( LQIO::DOM::Entry::ENTRY_STANDARD );	/* Force type to standard */
    const std::map<unsigned, LQIO::DOM::Phase*>& phases = dom_entry->getPhaseList();
    for ( std::map<unsigned, LQIO::DOM::Phase*>::const_iterator p = phases.begin(); p != phases.end(); ++p ) {
	resetServerPhaseParameters( document, p->second );
    }
    
    return entry;
}


const Layer& 
Layer::resetServerPhaseParameters( LQIO::DOM::Document* document, LQIO::DOM::Phase * phase ) const
{
    /* Delete all calls to all other servers from the DOM. */
    
    std::vector<LQIO::DOM::Call*>& dom_calls = const_cast<std::vector<LQIO::DOM::Call*>& >(phase->getCalls());
    dom_calls.clear();		// Should likely delete... 

    /* Set service time to residence time from solution */

    if ( document->hasResults() ) {
	const double mean = phase->getResultServiceTime();
	phase->setServiceTimeValue( mean );
	if ( Flags::output_coefficient_of_variation ) {
	    const double var  = phase->getResultVarianceServiceTime();
	    phase->setCoeffOfVariationSquaredValue( var / ( mean * mean ) );
	} else {
	    phase->setCoeffOfVariationSquared( 0 );		// Nuke value.
	}
    }
    return *this;
}

/*- BUG_626 */


double 
Layer::labelWidth() const
{
    if ( myLabel ) {
	return myLabel->width();
    } else {
	return 0.0;
    }
}

/*
 * Print a layer.
 */

ostream&
Layer::print( ostream& output ) const
{
    Sequence<Entity *> nextEntity( entities() );
    Entity * anEntity;
    while ( anEntity = nextEntity() ) {
	if ( anEntity->isSelectedIndirectly() ) {
	    output << *anEntity;
	}
    }

    if ( myLabel ) {
	output << *myLabel;
    }

    return output;
}



/*
 * Print a layer.
 */

ostream&
Layer::printSubmodel( ostream& output ) const
{
    output << "---------- Submodel: " << number()-1 << " ----------" << endl;

    if ( myClients.size() ) {
	output << "Clients: " << endl;
	Sequence<const Entity *> nextClient(myClients);
	const Entity * aClient;
	while ( aClient = nextClient() ) {
	    output << *aClient;
	}
    }

    output << "Servers: " << endl;
    Sequence<Entity *> nextServer( entities() );
    Entity * aServer;
    while ( aServer = nextServer() ) {
	if ( aServer->isSelected() ) {
	    output << *aServer;
	}
    }

    return output;
}



/*
 * Print a layer.
 */

ostream&
Layer::printSummary( ostream& output ) const
{
    unsigned int n_processors = 0;
    unsigned int n_tasks = 0;
    unsigned int n_customers = 0;

    Sequence<Entity *> nextEntity( entities() );
    Entity * anEntity;
    while ( anEntity = nextEntity() ) {
	if ( anEntity->isProcessor() ) {
	    n_processors += 1;
	} else if ( anEntity->isReferenceTask() ) {
	    n_customers += 1;
	} else {
	    n_tasks += 1;
	}
    }
    if ( n_customers )  output << n_customers  << " " << plural( "Customer", n_customers  )  << (n_tasks + n_processors ? ", " : "");
    if ( n_tasks )      output << n_tasks      << " " << plural( "Task", n_tasks )           << (n_processors ? ", " : "");
    if ( n_processors ) output << n_processors << " " << plural( "Processor", n_processors );
    output << ".";

    return output;
}



/*
 * Draw the submodel as a queueing network.
 */

ostream&
Layer::drawQueueingNetwork( ostream& output ) const
{
    Sequence<const Entity *> nextClient( myClients );
    const Entity *aClient;

    double max_x = x() + width();
    while ( aClient = nextClient() ) {
	bool is_in_open_model = false;
	bool is_in_closed_model = false;
	if ( aClient->isInClosedModel( entities() ) ) {
	    is_in_closed_model = true;
	}
	if ( aClient->isInOpenModel( entities() ) ) {
	    is_in_open_model = true;
	}
	aClient->drawClient( output, is_in_open_model, is_in_closed_model );
	max_x = max( max_x, aClient->right() );
    }

    /* Now draw connections */

    Vector<bool> chain( nChains(), false );	/* for drawing */
    Cltn<Arc *> clientArc( nChains() );

    Sequence<Entity *> nextServer( entities() );
    Entity * aServer;
    while ( aServer = nextServer() ) {
	if ( aServer->isSelected() ) {
	    aServer->drawServer( output );
	    aServer->drawQueueingNetwork( output, max_x, y(), chain, clientArc );	/* Draw it. */
	}
    }
    for ( unsigned k = 1; k <= nChains(); ++k ) {
	if ( chain[k] ) {
	    delete clientArc[k];
	}
    }
    return output;
}



#if defined(QNAP_OUTPUT)
/*
 * Print the submodel as a queueing network for QNAP.
 */

ostream&
Layer::printQNAP( ostream& output ) const
{
    const bool multi_class = nChains() > 1;
    Sequence<const Entity *> nextClient( myClients );
    Sequence<Entity *> nextServer( entities() );
    const Entity * aServer;
    const Entity * aClient;

    /* Stick all useful stations into a single collection */

    Cltn<const Entity *> stations;
    while ( aServer = nextServer() ) {
	if ( aServer->isSelected() ) {
	    stations += aServer;
	}
    }
    while ( aClient = nextClient() ) {
	if ( aClient->isSelectedIndirectly() ) {
	    stations += aClient;
	}
    }

    Sequence<const Entity *> nextStation( stations );
    const Entity * aStation;

    set_indent(0);
    output << indent(+1) << "/declare/" << endl;

    /* Class info */

    if ( multi_class ) {
	output << indent(0) << "class ";
	for ( unsigned k = 1; k <= nChains(); ++k ) {
	    if ( k > 1 ) {
		output << ", ";
	    }
	    output << "k" << k;
	}
	output << ";" << endl;
    }

    /* Dimensions for replicated stations */

    bool first = true;
    while ( aStation = nextStation() ) {
	if ( aStation->isReplicated() ) {
	    if ( first ) {
		output << indent(0) << "integer i, ";
		first = false;
	    } else {
		output << ", ";
	    }
	    output << qnap_replicas( *aStation ) << " = " << aStation->replicas();
	}
    }
    if ( !first ) {
	output << ";" << endl;
    }

    first = true;
    while ( aClient = nextClient() ) {
	if ( !dynamic_cast<const Task *>(aClient) ) continue;
	Sequence<EntityCall *> nextCall( dynamic_cast<const Task *>(aClient)->callList() );
	EntityCall * aCall;
	while ( aCall = nextCall() ) {
	    if ( aCall->fanOut() > 1 ) {
		if ( first ) {
		    output << indent(0) << "real ";
		    first = false;
		} else {
		    output << ", ";
		}
		output << qnap_visits( *aCall ) << "(" << aCall->fanOut() << ")"; 
	    }
	}
    }
    if ( !first ) {
	output << ";" << endl;
    }

    /* The stations themselves */

    first = true;
    output << indent(0) << "queue ";
    while ( aStation = nextStation() ) {
	if ( !first ) {
	    output << ", ";
	} else {
	    first = false;
	}
	if ( dynamic_cast<const OpenArrivalSource *>(aStation)  ) {
	    output << "src";
	}
	output << aStation->name();
	if ( aStation->isReplicated() ) {
	    output << "(" << qnap_replicas( *aStation ) << ")";
	}
    }
    output << indent(-1) << ";" << endl;

    output << "&" << endl << "& ---------- Clients ----------" << endl << "&" << endl;

    while ( aClient = nextClient() ) {
	const bool is_in_closed_model = aClient->isInClosedModel( entities() );
	const bool is_in_open_model = aClient->isInOpenModel( entities() );

	output << indent(+1) << "/station/" << endl;
	aClient->printQNAPClient( output, is_in_open_model, is_in_closed_model, multi_class );
	output << indent(-1) << endl;
    }

    output << "&" << endl << "& ---------- Servers ----------" << endl << "&" << endl;
    while ( aServer = nextServer() ) {
	if ( !aServer->isSelected() ) continue;
	output << indent(+1) << "/station/" << endl;
	aServer->printQNAPServer( output, multi_class );
	output << indent(-1) << endl;
    }
    
    output << "&" << endl << "&" << endl << "&" << endl;

    if ( multi_class ) {
	output << indent(+1) << "/control/" << endl;
	output << indent(0) << "class = all queue;" << endl;
	output << indent(-1) << endl;
    }
    output << indent(+1) << "/exec/" << endl
	   << indent(+1) << "begin" << endl;

#if 0
    while ( aClient = nextClient() ) {
	if ( !dynamic_cast<const Task *>(aClient) ) continue;
	Sequence<EntityCall *> nextCall( dynamic_cast<const Task *>(aClient)->callList() );
	EntityCall * aCall;
	while ( aCall = nextCall() ) {
	    if ( aCall->fanOut() > 1 ) {
		output << indent(0) << "for i := (1, step 1 until " << aCall->fanOut() << ") do" << endl;
		output << indent(0) << "   " << qnap_visits( *aCall ) << "(i) := " << aCall->rendezvous() << ";" << endl;
	    }
	}
    }
    /* Set service times */
    while ( aClient = nextClient() ) {
	output << indent(0) << aClient->name() << ".service:=" << 0.0 << ";" << endl;
    }
    while ( aServer = nextServer() ) {
	output << indent(0) << aServer->name() << ".service:=" << aServer->serviceTime(1) << ";" << endl;
    }
#endif

    output << indent(0) << "solve;" << endl;
    output << indent(-1) << "end;" << endl;
    output << indent(-1) << "/end/" << endl;
    return output;
}
#endif



#if defined(PMIF_OUTPUT)
/*
 * Print the submodel as a queueing network for QNAP.
 */

ostream&
Layer::printPMIF( ostream& output ) const
{
    Sequence<const Entity *> nextClient( myClients );
    Sequence<Entity *> nextServer( entities() );
    const Entity * aServer;
    const Entity * aClient;

    output << indent(+1) << "<Node>" << endl;
    while ( aServer = nextServer() ) {
	aServer->printPMIFServer( output );
    }
    while ( aClient = nextClient() ) {
	aClient->printPMIFServer( output );
    }
    output << indent(-1) << "</Node>" << endl;

    /* All arcs (?) */

    while ( aClient = nextClient() ) {
	aClient->printPMIFArcs( output );
    }

    /* Workloads  a.k.a. classes */

    output << indent(+1) << "<Workload>" << endl;
    while ( aClient = nextClient() ) {
	aClient->printPMIFClient( output );
    }
    output << indent(-1) << "</Workload>" << endl;

    /* ServiceRequests */

    output << indent(+1) << "<ServiceRequest>" << endl;
    while ( aServer = nextServer() ) {
	aServer->printPMIFReplies( output );
    }
    output << indent(-1) << "</ServiceRequest>" << endl;

    return output;
}
#endif
