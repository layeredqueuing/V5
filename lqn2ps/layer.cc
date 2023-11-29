/* layer.cc	-- Greg Franks Tue Jan 28 2003
 *
 * $Id: layer.cc 16869 2023-11-28 21:04:29Z greg $
 *
 * A layer consists of a set of tasks with the same nesting depth from
 * reference tasks.  Reference tasks are in layer 1, the immediate
 * servers to reference tasks are in layer 2, etc.  There are variants
 * of this, bien sur.
 */

#include "lqn2ps.h"
#include <cstdlib>
#include <algorithm>
#include <functional>
#include <lqio/dom_activity.h>
#include <lqio/dom_document.h>
#include <lqio/error.h>
#include <lqio/srvn_output.h>
#include "activity.h"
#include "arc.h"
#include "entity.h"
#include "entry.h"
#include "errmsg.h"
#include "label.h"
#include "layer.h"
#include "open.h"
#include "processor.h"
#include "task.h"

Layer::Layer()
    : _entities(), _origin(0,0), _extent(0,0), _number(0), _label(nullptr), _clients(), _chains(0), _bcmp_model()
{
    _label = Label::newLabel();
}

Layer::Layer( const Layer& src )
    : _entities(src._entities), _origin(src._origin), _extent(src._extent), _number(src._number), _label(nullptr), _clients(), _chains(), _bcmp_model()
{
    _label = Label::newLabel();
}

Layer::~Layer()
{
    delete _label;
}

Layer&
Layer::operator=( const Layer& src )
{
    _entities = src._entities;
    _origin = src._origin;
    _extent = src._extent;
    _number = src._number;
    delete _label;
    _label = Label::newLabel();
    return *this;
}

double
Layer::labelWidth() const
{
    return _label->width();
}


Layer&
Layer::append( Entity * entity )
{
    std::vector<Entity *>::iterator pos = find_if( _entities.begin(), _entities.end(), EQ<Element>(entity) );
    if ( pos == _entities.end() ) {
	_entities.push_back( entity );
    }
    return *this;
}


Layer&
Layer::erase( std::vector<Entity *>::iterator pos )
{
    if ( pos != _entities.end() ) {
	_entities.erase( pos );
    }
    return *this;
}


Layer&
Layer::remove( Entity * entity )
{
    std::vector<Entity *>::iterator pos = std::find( _entities.begin(), _entities.end(), entity );
    if ( pos != _entities.end() ) {
	_entities.erase( pos );
    }
    return *this;
}

Layer&
Layer::number( const unsigned n )
{
    _number = n;
    return *this;
}



bool
Layer::check() const
{
    return std::for_each( entities().begin(), entities().end(), AndPredicate<Entity>( &Entity::check ) ).result();
}



unsigned
Layer::count( const taskPredicate predicate ) const
{
    return count_if( entities().begin(), entities().end(), Predicate1<Entity,taskPredicate>( &Entity::test, predicate ) );
}



unsigned
Layer::count( const callPredicate aFunc ) const
{
    return std::accumulate( entities().begin(), entities().end(), 0, Entity::count_callers( aFunc ) );
}



/*
 * delete unused items from layers.
 */

Layer&
Layer::prune()
{
    for ( std::vector<Entity *>::iterator entity = _entities.begin(); entity != _entities.end(); ++entity ) {
	Task * aTask = dynamic_cast<ReferenceTask *>((*entity));
	Processor * aProc = dynamic_cast<Processor *>((*entity));

	if ( aTask && !aTask->hasCalls( &Call::hasAnyCall ) ) {

	    _entities.erase(entity);

	} else if ( aProc ) {

	    /*
	     * If a processor is not refereneced, or if the task was removed
	     * because it's a reference task that makes no calls, then delete
	     * the processor.
	     */

	    if ( aProc->nTasks() == 0 ) {
		_entities.erase(entity);
	    } else {
		for ( std::set<Task *>::const_iterator task = aProc->tasks().begin(); task != aProc->tasks().end(); ++task ) {
		    if ( !(*task)->isReferenceTask() || (*task)->hasCalls( &Call::hasAnyCall ) ) goto found;
		}
		_entities.erase(entity);
	    }
	found:;
	}
    }
    return *this;
}


Layer&
Layer::sort( compare_func_ptr compare )
{
    std::sort( _entities.begin(), _entities.end(), compare );
    return *this;
}



Layer&
Layer::format( const double y )
{
    if ( Flags::debug ) std::cerr << "Layer::format layer " << number() << std::endl;
    _origin.moveTo( 0.0, y );
    Position bounds = std::for_each( entities().begin(), entities().end(), Position( &Task::format, y ) );
    _extent.moveTo( bounds.x(), bounds.height() );
    return *this;
}


Layer&
Layer::reformat()
{
    if ( Flags::debug ) std::cerr << "Layer::reformat layer " << number() << std::endl;
    Position bounds = std::for_each( entities().begin(), entities().end(), Position( &Task::reformat ) );
    _origin.moveTo( 0.0, bounds.y() );
    _extent.moveTo( bounds.x(), bounds.height() );
    return *this;
}


Layer&
Layer::moveBy( const double dx, const double dy )
{
    _origin.moveBy( dx, dy );
    std::for_each( entities().begin(), entities().end(), ExecXY<Element>( &Element::moveBy, dx, dy ) );
    _label->moveBy( dx, dy );
    return *this;
}



/*
 * Move the label to...
 */

Layer&
Layer::moveLabelTo( const double xx, const double yy )
{
    if ( Flags::print_layer_number ) {
	_label->justification( Justification::LEFT ).moveTo( xx, y() + yy );
    } else if ( submodel_output() ) {
	_label->moveTo( _origin.x() + _extent.x() / 2.0, _origin.y() - Flags::font_size() * 1.2 );
    }
    return *this;
}



Layer&
Layer::scaleBy( const double sx, const double sy )
{
    std::for_each( entities().begin(), entities().end(), ExecXY<Element>( &Element::scaleBy, sx, sy ) );
    _origin.scaleBy( sx, sy );
    _extent.scaleBy( sx, sy );
    _label->scaleBy( sx, sy );
    return *this;
}



Layer&
Layer::translateY( const double dy )
{
    std::for_each( entities().begin(), entities().end(), Exec1<Element,double>( &Element::translateY, dy ) );
    _origin.y( dy - _origin.y() );
    _label->translateY( dy );
    return *this;
}



Layer&
Layer::depth( const unsigned depth )
{
    std::for_each( entities().begin(), entities().end(), Exec1<Element,unsigned int>( &Element::depth, depth ) );
    return *this;
}


Layer&
Layer::fill( const double maxWidthPts )
{
    const double width = std::accumulate( entities().begin(), entities().end(), 0.0, sum( &Element::width ) );
    const double fill = std::max( 0.0, (maxWidthPts - width) / static_cast<double>(entities().size() + 1) );
    if ( fill < Flags::x_spacing() ) return *this;		/* Don't bother... */

    _origin.x( fill );

    double x = fill;
    for ( std::vector<Entity *>::const_iterator entity = entities().begin(); entity != entities().end(); ++entity ) {
	const double y = (*entity)->bottom();
	(*entity)->moveTo( x, y );
	x += (*entity)->width();
	_extent.x( x - fill );
	x += fill;
    }

    return *this;
}



Layer&
Layer::justify( const double width )
{
    return justify( width, Flags::node_justification );
}


Layer&
Layer::justify( const double maxWidthPts, const Justification justification )
{
    switch ( justification ) {
    case Justification::ALIGN:
    case Justification::DEFAULT:
    case Justification::CENTER:
	moveBy( (maxWidthPts - width())/2, 0.0 );
	break;
    case Justification::RIGHT:
	moveBy( (maxWidthPts - width()), 0.0 );
	break;
    case Justification::LEFT:
	moveBy( 0.0, 0.0 );		/* Force recomputation of slopes */
	break;
    default:
	abort();
    }
    return *this;
}



Layer&
Layer::align()
{
    for ( std::vector<Entity *>::const_iterator entity = entities().begin(); entity != entities().end(); ++entity ) {
	const double delta = height() - (*entity)->height();
	if ( delta > 0. ) {
	    (*entity)->moveBy( 0, delta );
	}
    }
    return *this;
}



/*
 * Align tasks between layers.
 */

Layer&
Layer::alignEntities()
{
    std::sort( _entities.begin(), _entities.end(), &Entity::compareCoord );

    if ( Flags::debug ) std::cerr << "Layer::alignEntities layer " << number() << std::endl;
    /* Move objects right starting from the right side */
    for ( unsigned int i = size(); i > 0; ) {
	i -= 1;
	shift( i, _entities[i]->align() );
    }
    /* Move objects left starting from the left side */
    for ( unsigned int i = 0; i < size(); ++i ) {
	shift( i, _entities[i]->align() );
    }

    return *this;
}



/*
 * Move object `i' by `amount' if possible.  The bounding box is recomputed if necessary.
 */

Layer&
Layer::shift( unsigned i, double amount )
{
    if ( amount < 0.0 ) {
	/* move left if I can */
	if ( Flags::debug ) std::cerr << "Layer::shift layer " << number() << " shift " << _entities.at(i)->name() << " left from ("
				      << _entities.at(i)->left() << "," << _entities.at(i)->bottom() << ")";
	if ( i == 0 ) {
	    _entities[i]->moveBy( amount, 0 );
	    _origin.moveBy( amount, 0 );
	} else {
	    _entities[i]->moveBy( std::min( std::max( (_entities[i-1]->right() + Flags::x_spacing()) - _entities[i]->left(), amount ), 0.0 ), 0 );
	}
	if ( Flags::debug ) std::cerr << " to (" << _entities.at(i)->left() << "," << _entities.at(i)->bottom() << ")" << std::endl;
	_origin.x( _entities.front()->left() );
	_extent.x( _entities.back()->right() - _entities.front()->left() );
	if ( i + 1 < size() && _entities[i]->forwardsTo( dynamic_cast<Task *>(_entities[i+1]) ) ) {
	    shift( i+1, amount );
	}
    } else if ( amount > 0.0 ) {
	/* move right if I can */
	if ( Flags::debug ) std::cerr << "Layer::shift layer " << number() << " shift " << _entities.at(i)->name() << " right from ("
				      << _entities.at(i)->left() << "," << _entities.at(i)->bottom() << ")";
	if ( i + 1 == size() ) {
	    _entities[i]->moveBy( amount, 0 );
	} else {
	    _entities[i]->moveBy( std::max( std::min( _entities[i+1]->left() - (_entities[i]->right() + Flags::x_spacing()), amount ), 0.0 ), 0 );
	}
	if ( Flags::debug ) std::cerr << " to (" << _entities.at(i)->left() << "," << _entities.at(i)->bottom() << ")" << std::endl;
	_origin.x( _entities.front()->left() );
	_extent.x( _entities.back()->right() - _entities.front()->left() );
	if ( i > 0 && _entities[i-1]->forwardsTo( dynamic_cast<Task *>(_entities[i]) ) ) {
	    shift( i-1, amount );
	}
    }
    return *this;
}



/*
 * Label all objects in this layer.  If it's a BCMP model, then only
 * one layer needs to (and can) be labelled.
 */

Layer&
Layer::label()
{
    if ( !Flags::bcmp_model ) {
	std::for_each( entities().begin(), entities().end(), std::mem_fn( &Element::label ) );
    } else if ( !_bcmp_model.empty() ) {
	std::for_each( clients().begin(), clients().end(), Entity::label_BCMP_client( _bcmp_model ) );
	std::for_each( entities().begin(), entities().end(), Entity::label_BCMP_server( _bcmp_model ) );
    }
    if ( Flags::print_layer_number ) {
	*_label << "Layer " << number();
    }
    return *this;
}


void Layer::Position::operator()( Entity * entity )
{
    if ( !entity->isSelectedIndirectly() ) return;
    if ( _x != 0.0 ) _x += Flags::x_spacing();
    Task * aTask = dynamic_cast<Task *>(entity);
    if ( aTask && Flags::aggregation() != Aggregate::ENTRIES ) {
	(aTask->*_f)();
    }
    if ( Flags::debug ) std::cerr << "  Layer::Position move " << entity->name() << " to (" << _x << "," << entity->bottom() << ")" << std::endl;
    entity->moveTo( _x, (_f == &Task::reformat ? entity->bottom() : _y) );
    _x += entity->width();
    _y = std::min( _y, entity->bottom() );
    _h = std::max( _h, entity->height() );
}

/*
 * Select all servers in this submodel for printing.
 */

Layer&
Layer::selectSubmodel()
{
    for ( std::vector<Entity *>::const_iterator entity = entities().begin(); entity != entities().end(); ++entity ) {
	if ( !(*entity)->isProcessor() || Flags::processors() != Processors::NONE ) {
	    (*entity)->setSelected( true );		/* Enable arc drawing to this entity */
	}
    }
    return *this;
}


/*
 * Select all servers in this submodel for printing.
 */

Layer&
Layer::deselectSubmodel()
{
    std::for_each( entities().begin(), entities().end(), Exec1<Entity,bool>( &Entity::setSelected, false ) );
    return *this;
}


Layer&
Layer::generateSubmodel()
{
    if ( clients().size() > 0 ) return *this;

    _chains = 0;

    /* Find clients */

    for ( std::vector<Entity *>::const_iterator entity = entities().begin(); entity != entities().end(); ++entity ) {
	if ( (*entity)->isSelected() ) {
	    (*entity)->clients( _clients );		/* Now find out who calls it */
	}
    }

    /* Set the chains in the model */

    for ( std::vector<Task *>::const_iterator client = clients().begin(); client != clients().end(); ++client ) {
	if ( (*client)->isInClosedModel( entities() ) ) {
	    _chains += 1;
	    _chains = const_cast<Task *>(*client)->setChain( _chains, &GenericCall::hasRendezvous );
	}
	if ( (*client)->isInOpenModel( entities() ) ) {
	    _chains += 1;
	    _chains = const_cast<Task *>(*client)->setChain( _chains, &GenericCall::hasSendNoReply );
	}
    }

    return *this;
}


/*
 * !!!
 * Move all service time for entries up into the task.  However, the
 * service time is by chain (i.e, the clients of the submodel).
 */

Layer&
Layer::aggregate()
{
    _clients.clear();
    for ( std::vector<Entity *>::const_iterator entity = entities().begin(); entity != entities().end(); ++entity ) {
	Task * task = dynamic_cast<Task *>(*entity);
	if ( !task ) continue;
	const std::vector<Entry *>& entries = task->entries();
	for ( std::vector<Entry *>::const_iterator entry = entries.begin(); entry != entries.end(); ++entry ) {
	    const std::vector<GenericCall *>& calls = (*entry)->callers();
	    for ( std::vector<GenericCall *>::const_iterator i = calls.begin(); i != calls.end(); ++i ) {
		Call * call = dynamic_cast<Call *>(*i);
		if ( !call ) continue;
		Task * client = const_cast<Task *>(call->srcTask());
		Task * server = const_cast<Task *>(call->dstTask());
		if ( std::none_of( _clients.begin(), _clients.end(), EQ<Element>(client) ) ) {
		    _clients.push_back( client );				/* add the client to my clients */
		}
		callPredicate predicate = nullptr;
		if ( call->hasForwarding() ) {
		    predicate = &GenericCall::hasForwarding;
		} else if ( call->hasSendNoReply() ) {
		    predicate = &GenericCall::hasSendNoReply;
		}
		EntityCall * new_call = client->findOrAddCall( server, predicate );	/* create a call... */
		TaskCall * task_call = dynamic_cast<TaskCall * >(new_call);
		if ( task_call == nullptr ) continue;

		/* Set rate on call? By phase? */
		if ( call->hasForwarding() ) {
		    task_call->taskForward( LQIO::DOM::ConstantExternalVariable( call->forward() ) );
		} else if ( call->hasSendNoReply() ) {
		    task_call->sendNoReply( LQIO::DOM::ConstantExternalVariable( 1.0 ) );	/* Set value to force type. */
		} else {
		    task_call->rendezvous( LQIO::DOM::ConstantExternalVariable( 1.0 ) );	/* Set value to force type. */
		}
	    }
	}
    }
    return *this;
}

/*+ BUG_626 */

/*
 * Convert all clients to reference tasks.
 */

Layer&
Layer::transmorgrifyClients( LQIO::DOM::Document * document )		/* BUG_440 */
{
    for ( std::vector<Task *>::const_iterator client = clients().begin(); client != clients().end(); ++client ) {
	dynamic_cast<LQIO::DOM::Task *>(const_cast<LQIO::DOM::DocumentObject *>((*client)->getDOM()))->setSchedulingType( SCHEDULE_CUSTOMER );   // Clients become reference tasks.
    }

    for ( std::vector<Task *>::const_iterator client = clients().begin(); client != clients().end(); ++client ) {
	Task * task = *client;

	/*  If the processor is not in the submodel, then create a fake processor */
	
	if ( !task->processor()->isSelected() ) {
	    addSurrogateProcessor( document, task, number()+1 );
	}
	
	/* Update entry parameters */
	
	const std::vector<Entry *>& entries = task->entries();
	for ( std::vector<Entry *>::const_iterator entry = entries.begin(); entry != entries.end(); ++entry ) {

	    /* Unlink calls from any other tasks */

	    std::vector<GenericCall *> callers = (*entry)->callers();	// Make a copy
	    for ( std::vector<GenericCall *>::const_iterator call = callers.begin(); call != callers.end(); ++call ) {
		const_cast<GenericCall *>(*call)->unlink();
		delete *call;
	    }
	    (*entry)->isCalledBy( request_type::NOT_CALLED );

	    /* Compute service time for each phase */
	    
	    resetClientPhaseParameters( *entry );

	    /* Set visit probabilities if there is more than one entry. */
	    
	    if ( entries.size() > 1 ) {
		dynamic_cast<LQIO::DOM::Entry *>(const_cast<LQIO::DOM::DocumentObject *>((*entry)->getDOM()))->setVisitProbabilityValue( (*entry)->visitProbability() );
	    }
	}

	/* set think times. */
	    
	if ( document->hasResults() ) {
	    dynamic_cast<LQIO::DOM::Task *>(const_cast<LQIO::DOM::DocumentObject *>(task->getDOM()))->setThinkTimeValue( task->idleTime() );
	}

    }
    return *this;
}



/*
 * Get results from all calls except to those tasks which are not in the entity set.
 * See also Phase::serviceTimeForSRVNInput.
 */

void
Layer::resetClientPhaseParameters( Entry * entry )
{
    const unsigned int max_phase = entry->maxPhase();
    LQIO::DOM::Entry * dom_entry = dynamic_cast<LQIO::DOM::Entry *>(const_cast<LQIO::DOM::DocumentObject *>(entry->getDOM()));
    for ( unsigned int p = 1; p <= max_phase; ++p ) {
	double residence_time = entry->serviceTimeForSRVNInput(p);
	if ( residence_time > 0. ) {
	    dom_entry->getPhase( p )->setServiceTimeValue( residence_time );
	}
    }
}


/*
 * Transform a submodel into a full blown LQN that can be solved on its own.
 * Add surrogate processors to any servers which are tasks.  The service time
 * for servers which are tasks is the residence time for the entry found from
 * the results.  The service time for clients is calculated from the utilization
 * and throughput (like lqns).
*/

Layer&
Layer::transmorgrifyServers( LQIO::DOM::Document * document )
{
    /* ---------- Servers ---------- */

    std::vector<Entity *> entities = _entities;		/* Copy as _entities will change */
    for ( std::vector<Entity *>::const_iterator entity = entities.begin(); entity != entities.end(); ++entity ) {
	Task * task = dynamic_cast<Task *>(*entity);
	if ( !(*entity)->isSelected() || task == nullptr ) continue;

	const_cast<Processor *>(task->processor())->setSelected( true );
//	if ( !task->processor()->isSelected() ) {
//	    std::cerr << "Add surrogate processor for server task " << task->name() << std::endl;
//	}
//	findOrAddSurrogateProcessor( document, surrogate_processor, task, number()+1 );

//	I need to unlink any calls to any other reference tasks. */
	const std::vector<Entry *>& entries = task->entries();
	for ( std::vector<Entry *>::const_iterator entry = entries.begin(); entry != entries.end(); ++entry ) {

	    /* These are graphical object calls */

	    std::vector<Call *>& calls = const_cast<std::vector<Call *>& >((*entry)->calls());
	    calls.clear();

	    /* Forwarded calls? */

	    const std::map<unsigned, LQIO::DOM::Phase*>& phases = dynamic_cast<const LQIO::DOM::Entry *>((*entry)->getDOM())->getPhaseList();
	    std::for_each( phases.begin(), phases.end(), ResetServerPhaseParameters( document->hasResults() ) );
	}

	const std::map<std::string,LQIO::DOM::Activity*>& activities = dynamic_cast<const LQIO::DOM::Task *>(task->getDOM())->getActivities();
	std::for_each( activities.begin(), activities.end(), ResetServerPhaseParameters( document->hasResults() ) );
    }

    return *this;
}



/*
 * Make a surrogate processor for the task.  Then unlink the task from
 * the old processor and link it to the new one.
 */

Layer&
Layer::addSurrogateProcessor( LQIO::DOM::Document * document, Task * task, size_t level )
{
    LQIO::DOM::Processor * dom_processor = new LQIO::DOM::Processor( document, task->name(), SCHEDULE_DELAY );
    document->addProcessorEntity( dom_processor );
    Processor * new_processor = new Processor( dom_processor );		/* This is a model object */
    new_processor->setSelected( true ).setSurrogate( true ).setLevel( level );
    _entities.push_back( new_processor );
    Processor::__processors.insert( new_processor );

    Processor * old_processor = const_cast<Processor *>(task->processor());
    LQIO::DOM::Task * dom_task = const_cast<LQIO::DOM::Task *>(dynamic_cast<const LQIO::DOM::Task *>(task->getDOM()));
    std::set<LQIO::DOM::Task*>& old_list = const_cast<std::set<LQIO::DOM::Task*>&>(dom_task->getProcessor()->getTaskList());
    old_list.erase( find( old_list.begin(), old_list.end(), dom_task ) );		// Remove task from original processor
    dom_processor->addTask( dom_task );
    dom_task->setProcessor( dom_processor );
    old_processor->removeTask( task );
    new_processor->addTask( task );
    task->removeProcessor( old_processor );
    task->addProcessor( new_processor );

    /* Remap processor call */
//	Point aPoint = processor->center();
    std::vector<EntityCall *>& task_calls = const_cast<std::vector<EntityCall *>& >(task->calls());
    for ( std::vector<EntityCall *>::const_iterator call = task_calls.begin(); call != task_calls.end(); ++call ) {
	if ( !(*call)->isProcessorCall() ) continue;
	(*call)->setDstEntity( new_processor );
//		(*call)->moveDst( aPoint );
	old_processor->removeDstCall( (*call) );
	new_processor->addDstCall( (*call) );
    }
    return *this;
}


void
Layer::ResetServerPhaseParameters::reset( LQIO::DOM::Activity * activity ) const { reset( dynamic_cast<LQIO::DOM::Phase*>(activity) ); }

void
Layer::ResetServerPhaseParameters::reset( LQIO::DOM::Phase * phase ) const
{
    std::vector<LQIO::DOM::Call*>& dom_calls = const_cast<std::vector<LQIO::DOM::Call*>& >(phase->getCalls());
    dom_calls.clear();		// Should likely delete...

    if ( _hasResults ) {
	const double mean = phase->getResultServiceTime();
	phase->setServiceTimeValue( mean );
	if ( Flags::output_coefficient_of_variation ) {
	    phase->setCoeffOfVariationSquaredValue( phase->getResultVarianceServiceTime() / square( mean ) );
	}
    }
}

/* -------------------------------------------------------------------- */
/*			       Printing   				*/
/* -------------------------------------------------------------------- */

/*
 * Print a layer.
 */

std::ostream&
Layer::print( std::ostream& output ) const
{
    for ( std::vector<Entity *>::const_iterator entity = entities().begin(); entity != entities().end(); ++entity ) {
	if ( (*entity)->isSelectedIndirectly() ) {
	    output << *(*entity);
	}
    }

    output << *_label;

    return output;
}



/*
 * Print a layer.  Ignore Flags::print[OUPTUT_FORMAT]
 */

std::ostream&
Layer::printSubmodel( std::ostream& output ) const
{
    const size_t n_clients = clients().size();
    output << "---------- Submodel: " << number() << " ----------" << std::endl;

    if ( n_clients > 0 ) {
	output << "Clients: " << std::endl;
	for ( std::vector<Task *>::const_iterator client = _clients.begin(); client != _clients.end(); ++client ) {
	    (*client)->print( output );
	}
    }

    output << "Servers: " << std::endl;
    for ( std::vector<Entity *>::const_iterator entity = entities().begin(); entity != entities().end(); ++entity ) {
	if ( (*entity)->isSelected() ) {
	    (*entity)->print( output );
	}
    }

    return output;
}

std::ostream&
Layer::printSubmodelSummary( std::ostream& output ) const
{
    unsigned int n_clients  = clients().size();
    unsigned int n_servers  = entities().size();
    unsigned int n_multi    = count_if( entities().begin(), entities().end(), std::mem_fn( &Entity::isMultiServer ) );
    unsigned int n_infinite = count_if( entities().begin(), entities().end(), std::mem_fn( &Entity::isInfinite ) );
    unsigned int n_fixed    = n_servers - (n_multi + n_infinite);
    if ( n_clients ) output << n_clients << " " << plural( "Client", n_clients ) << (n_servers ? "; " : "");
    if ( n_servers ) output << n_servers << " " << plural( "Server", n_servers ) << " ("
			    << n_fixed << " Fixed, "
			    << n_multi << " Multi, "
			    << n_infinite << " Infinite)";
    output << "." << std::endl;
    return output;
}


/*
 * Print a layer.
 */

std::ostream&
Layer::printSummary( std::ostream& output ) const
{
    unsigned int n_processors = 0;
    unsigned int n_tasks = 0;
    unsigned int n_customers = 0;

    for ( std::vector<Entity *>::const_iterator entity = entities().begin(); entity != entities().end(); ++entity ) {
	if ( (*entity)->isProcessor() ) {
	    n_processors += 1;
	} else if ( (*entity)->isReferenceTask() ) {
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

/* -------------------------------------------------------------------- */
/*			   Queueing Models				*/
/* -------------------------------------------------------------------- */


/*- BUG_626 */

/*
 * Translate a "pruned" lqn model into a BCMP model.
 */

bool
Layer::createBCMPModel()
{
    if ( Flags::bcmp_model ) {

	/* A strictly BCMP model should have no tasks after the pruning operation was done. */
	
	std::vector<const Entity *> tasks = std::accumulate( entities().begin(), entities().end(), std::vector<const Entity *>(), Select<const Entity>( &Entity::isTask ) );
	if ( !tasks.empty() ) {
	    std::vector<std::string> names = std::accumulate( tasks.begin(), tasks.end(), std::vector<std::string>(), Collect<std::string,const Entity>( &Entity::name ) );
	    LQIO::runtime_error( ERR_BCMP_CONVERSION_FAILURE, std::accumulate( std::next(names.begin()), names.end(), names.front(), &fold ).c_str() );
	    return false;
	}
    }

    /* 
     * Create all of the chains for the model.  Clients in the lqns model are chains in the BCMP model.  Add a "terminals" stations.
     */

    std::pair<BCMP::Model::Station::map_t::iterator,bool> result = _bcmp_model.insertStation( ReferenceTask::__BCMP_station_name, BCMP::Model::Station(BCMP::Model::Station::Type::DELAY) );	// QNAP2 limit is 8.
    BCMP::Model::Station& terminals = result.first->second;
    terminals.setReference(true);
    std::for_each( clients().begin(), clients().end(), Task::create_chain( _bcmp_model, terminals, entities() ) );

    /*
     * Create all of the stations except for the terminals (which are the clients in the lqn schema.
     */

    std::for_each( entities().begin(), entities().end(), Entity::create_station( _bcmp_model ) );

    /*
     * Find the demand for all servers (reference tasks will never be here) for each client (chain) in the BCMP model.  For server
     * tasks, this will be the residence time for the entry associated with the client.  For processors, it is simply the demand for
     * the task.
     */

    std::for_each( clients().begin(), clients().end(), Task::create_demand( _bcmp_model, entities() ) );

    return true;
}



/*
 * Draw the submodel as a queueing network.
 */

std::ostream&
Layer::drawQueueingNetwork( std::ostream& output ) const
{
    double max_x = x() + width();
    for ( std::vector<Task *>::const_iterator client = clients().begin(); client != clients().end(); ++client ) {
	(*client)->drawClient( output, (*client)->isInOpenModel( entities() ), (*client)->isInClosedModel( entities() ) );
	max_x = std::max( max_x, (*client)->right() );
    }

    /* Now draw connections */

    std::vector<bool> chain( nChains()+1, false );	/* for drawing */
    std::vector<Arc *> clientArc( nChains()+1 );

    for ( std::vector<Entity *>::const_iterator server = entities().begin(); server != entities().end(); ++server ) {
	if ( (*server)->isSelected() ) {
	    (*server)->drawQueueingNetwork( output, max_x, y(), chain, clientArc );	/* Draw it. */
	}
    }
    for ( unsigned k = 1; k <= nChains(); ++k ) {
	if ( chain[k] ) {
	    delete clientArc[k];
	}
    }
    return output;
}
