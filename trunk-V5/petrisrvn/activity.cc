/************************************************************************/
/* Copyright the Real-Time and Distributed Systems Group,		*/
/* Department of Systems and Computer Engineering,			*/
/* Carleton University, Ottawa, Ontario, Canada. K1S 5B6		*/
/* 									*/
/* September 1991.							*/
/* August 2003.								*/
/************************************************************************/

/*
 * $Id: petrisrvn.cc 10943 2012-06-13 20:21:13Z greg $
 *
 * Generate a Petri-net from an SRVN description.
 *
 */

#include <cmath>
#include <lqio/glblerr.h>
#include <lqio/error.h>
#include <lqio/dom_activity.h>
#include <lqio/dom_actlist.h>
#include <lqio/dom_extvar.h>
#include "makeobj.h"
#include "entry.h"
#include "activity.h"
#include "actlist.h"
#include "task.h"
#include "results.h"

std::map<LQIO::DOM::ActivityList*, LQIO::DOM::ActivityList*> Activity::actConnections;
std::map<LQIO::DOM::ActivityList*, ActivityList *> Activity::domToNative;

Activity::Activity( LQIO::DOM::Activity * dom, Task * task )
    : Phase( dom, task ),
      _input(0),
      _output(0),
      _replies(),
      _is_start_activity(false),
      _is_reachable(false),
      _is_specified(dom->isSpecified())
{
    for ( unsigned int m = 0; m < MAX_MULT; ++m ) {
	_throughput[m] = 0;
    }
}

double Activity::check() 
{
    double count = Phase::check();
    return count;
}

/*
 * create a phase.
 */

double
Activity::transmorgrify( const double x_pos, const double y_pos, const unsigned m,
			 const Entry * e, const double p_pos, const short enabling,
			 struct place_object * end_place, bool can_reply )
{
    bool done 		= true;		/* flag indicating end of path. */
    const LAYER layer_mask = ENTRY_LAYER(e->entry_id())|(m == 0 ? PRIMARY_LAYER : 0);
	
    double next_pos = Phase::transmorgrify( x_pos, y_pos, m, layer_mask, p_pos, enabling );

    /* Attach reply. */
	
    if ( task()->is_server() && !task()->inservice_flag() && this->_replies.size() && can_reply ) {
	for ( std::set<Entry *>::const_iterator r = this->_replies.begin(); r != this->_replies.end(); ++r ) {
	    Entry * an_entry = *r;
	    if ( an_entry->requests() != RENDEZVOUS_REQUEST ) continue;
	    create_arc( ENTRY_LAYER(an_entry->entry_id())|(m == 0 ? PRIMARY_LAYER : 0), TO_PLACE, this->doneX[m], an_entry->DX[m] );
#if defined(BUG_622)
	    if ( an_entry->n_phases() > 1 ) {
		create_arc( MEASUREMENT_LAYER, TO_TRANS, this->doneX[m], an_entry->phase[1].XX[m] );
		create_arc( MEASUREMENT_LAYER, TO_PLACE, this->doneX[m], an_entry->phase[2].XX[m] );
	    }
#endif
	}
    }
	
    /* go down list creating more phases. */

    if ( this->_output ) {
	done = link_activity( x_pos, y_pos, e, m, next_pos, enabling, end_place, can_reply );
    }

    if ( done && (!task()->inservice_flag() || task()->is_client() ) ) {
	create_arc( layer_mask, TO_PLACE, this->doneX[m], end_place );
	/* BUG_164 ? */
#if defined(BUG_622)
	if ( !task()->gdX[m] && can_reply ) {
	    /* activities end normally, so link to done */
	    create_arc( MEASUREMENT_LAYER, TO_TRANS, this->doneX[m], e->phase[e->n_phases()].XX[m] );	/* End phase n */
	}
#endif
	if ( task()->is_sync_server() ) {
	    move_place( task()->GdX[m], this->done_xpos[m], this->done_ypos[m]-0.5 );
	    move_trans( task()->gdX[m], this->done_xpos[m], this->done_ypos[m]-1.0 );
	}
    }

    return next_pos + 1;
}



/*
 * Recursively follow the path.  We are looking for external joins.
 */

bool
Activity::find_children( my_stack_t<Activity *>& activity_stack, my_stack_t<ActivityList *>& fork_stack, const Entry * e )
{
    bool has_service_time = s() || think_time();

    _is_reachable = true;
    if ( activity_stack.find( this ) >= 0 ) {
	activity_cycle_error( LQIO::ERR_CYCLE_IN_ACTIVITY_GRAPH, task()->name(), activity_stack );
	return has_service_time;
    }

    /* tag phase */
    
    if ( _entry == 0 ) {
	_entry = e;
    }
    
    if ( _output ) {
	activity_stack.push( this );
	if ( _output->join_find_children( activity_stack, fork_stack, e ) ) {
	    has_service_time = true;
	}
	activity_stack.pop();
    }
	  
    return has_service_time;
}	



void Activity::activity_cycle_error( int err, const char *, my_stack_t<Activity *>& activity_stack )
{
    static char buf[BUFSIZ];
    size_t l = 0;

    while ( !activity_stack.empty() ) {
	Activity * ap = activity_stack.top();
	l += snprintf( &buf[l], BUFSIZ-l, ", %s", ap->name() );
    }
    LQIO::solution_error( err, name(), buf );
}

/*
 * Recursively follow the path.  We are looking for external joins.
 */

double
Activity::count_replies( my_stack_t<Activity *>& activity_stack, const Entry * e,
			 const double rate, const unsigned curr_phase, unsigned& next_phase )
{
    double sum = 0.0;
    next_phase = curr_phase;
    
    if ( activity_stack.find( this ) < 0 ) {

	/* tag phase */
    
	if ( this->_entry == 0 ) {
	    this->_entry = e;
	}
    
	/* Look for reply.  Flag as necessary */
	
	if ( replies_to( e ) ) {
	    if ( curr_phase >= 2 ) {
		solution_error( LQIO::ERR_DUPLICATE_REPLY, task()->name(), this->name(), e->name() );
	    } else if ( rate <= 0 ) {
		solution_error( LQIO::ERR_INVALID_REPLY, task()->name(), this->name(), e->name() );
	    } else if ( e->requests() == SEND_NO_REPLY_REQUEST ) {
		solution_error( LQIO::ERR_REPLY_SPECIFIED_FOR_SNR_ENTRY, task()->name(), this->name(), e->name() );
	    } else {
		next_phase = 2;
		sum = rate;
	    }
	} else if ( curr_phase > 1 ) {
	    const_cast<Entry *>(e)->set_n_phases( curr_phase );
	}

	if ( _output ) {
	    activity_stack.push( this );
	    sum += _output->join_count_replies( activity_stack, e, rate, next_phase, next_phase );
	    activity_stack.pop();
	}

    }
    return sum;
}	



Activity&
Activity::add_reply_list()
{
    /* This information is stored in the LQIO DOM itself */
    LQIO::DOM::Activity* dom = dynamic_cast<LQIO::DOM::Activity *>(get_dom());
    const std::vector<LQIO::DOM::Entry*>& reply_list = dom->getReplyList();
    if (reply_list.size() == 0) {
	return * this;
    }
	
    /* Walk over the list and do the equivalent of calling act_add_reply n times */
    std::vector<LQIO::DOM::Entry*>::const_iterator iter;
    for (iter = reply_list.begin(); iter != reply_list.end(); ++iter) {
	const LQIO::DOM::Entry* entry = *iter;
	Entry * ep = Entry::find( entry->getName() );
	
	if ( !ep ) {
	    LQIO::input_error2( LQIO::ERR_NOT_DEFINED, entry->getName().c_str() );
	} else if ( ep->task() != task() ) {
	    LQIO::input_error2( LQIO::ERR_WRONG_TASK_FOR_ENTRY, entry->getName().c_str(), task()->name() );
	} else {
	    _replies.insert( ep );
	}
    }

    return *this;
}



Activity& Activity::add_activity_lists()
{
    /* Obtain the Task and Activity information DOM records */
    LQIO::DOM::Activity* dom = dynamic_cast<LQIO::DOM::Activity*>(get_dom());
    if (dom == NULL) { return *this; }
	
    /* May as well start with the outputToList, this is done with various methods */
    LQIO::DOM::ActivityList* joinList = dom->getOutputToList();
    ActivityList * localActivityList = NULL;
    if (joinList != NULL && joinList->getProcessed() == false) {
	const std::vector<const LQIO::DOM::Activity*>& list = joinList->getList();
	std::vector<const LQIO::DOM::Activity*>::const_iterator iter;
	joinList->setProcessed(true);
	for (iter = list.begin(); iter != list.end(); ++iter) {
	    const LQIO::DOM::Activity* dom = *iter;
	    Activity * nextActivity = task()->find_activity( dom->getName().c_str() );
	    if ( !nextActivity ) {
		LQIO::input_error2( LQIO::ERR_NOT_DEFINED, dom->getName().c_str() );
		continue;
	    }
			
	    /* Add the activity to the appropriate list based on what kind of list we have */
	    switch ( joinList->getListType() ) {
	    case LQIO::DOM::ActivityList::JOIN_ACTIVITY_LIST:
		localActivityList = nextActivity->act_join_item( joinList );
		break;
	    case LQIO::DOM::ActivityList::AND_JOIN_ACTIVITY_LIST:
		localActivityList = nextActivity->act_and_join_list( localActivityList, joinList );
		break;
	    case LQIO::DOM::ActivityList::OR_JOIN_ACTIVITY_LIST:
		localActivityList = nextActivity->act_or_join_list( localActivityList, joinList );
		break;
	    default:
		abort();
	    }
	}
		
	/* Create the association for the activity list */
	domToNative[joinList] = localActivityList;
	if (joinList->getNext() != NULL) {
	    actConnections[joinList] = joinList->getNext();
	}
    }
	
    /* Now we move onto the inputList, or the fork list */
    LQIO::DOM::ActivityList* forkList = dom->getInputFromList();
    localActivityList = NULL;
    if (forkList != NULL && forkList->getProcessed() == false) {
	const std::vector<const LQIO::DOM::Activity*>& list = forkList->getList();
	std::vector<const LQIO::DOM::Activity*>::const_iterator iter;
	forkList->setProcessed(true);
	for (iter = list.begin(); iter != list.end(); ++iter) {
	    const LQIO::DOM::Activity* dom = *iter;
	    Activity * nextActivity = task()->find_activity( dom->getName().c_str() );
	    if ( !nextActivity ) {
		LQIO::input_error2( LQIO::ERR_NOT_DEFINED, dom->getName().c_str() );
		continue;
	    }
			
	    /* Add the activity to the appropriate list based on what kind of list we have */
	    switch ( forkList->getListType() ) {
	    case LQIO::DOM::ActivityList::FORK_ACTIVITY_LIST:	
		localActivityList = nextActivity->act_fork_item( forkList );
		break;
	    case LQIO::DOM::ActivityList::AND_FORK_ACTIVITY_LIST:
		localActivityList = nextActivity->act_and_fork_list( localActivityList, forkList  );
		break;
	    case LQIO::DOM::ActivityList::OR_FORK_ACTIVITY_LIST:
		localActivityList = nextActivity->act_or_fork_list( localActivityList, forkList );
		break;
	    case LQIO::DOM::ActivityList::REPEAT_ACTIVITY_LIST:
		localActivityList = nextActivity->act_loop_list( localActivityList, forkList );
		break;
	    default:
		abort();
	    }
	}
		
	/* Create the association for the activity list */
	domToNative[forkList] = localActivityList;
	if (forkList->getNext() != NULL) {
	    actConnections[forkList] = forkList->getNext();
	}
    }

    return *this;
}



/*
 * Search for a reply by this activity to entry e.
 * Return true if found.
 */

bool
Activity::replies_to( const Entry * e ) const
{
    std::set<Entry *>::const_iterator r = _replies.find( const_cast<Entry *>(e) );
    return r != _replies.end();
}



/*
 * Create links to and from activities.
 */

bool
Activity::link_activity( double x_pos, double y_pos, const Entry * e, const unsigned m,
		     double& p_pos, const short enabling,
		     struct place_object * end_place, const bool can_reply )
{
    const unsigned ne = task()->n_entries();	/* For offset macro. */
    ActivityList * join_list = this->_output;
    ActivityList * fork_list = join_list->u.join.next;
    Activity * next_act;
    struct trans_object * join_trans = this->doneX[m];
    unsigned i;
    double curr_pos = 0;
    double next_pos = 0;
    double sum;
    const LAYER layer_mask = ENTRY_LAYER(e->entry_id())|(m == 0 ? PRIMARY_LAYER : 0);
    
    /* Find activity in the join_list */
	
    for ( i = 0; i < join_list->n_acts() && join_list->list[i] != this; ++i );

    if ( i == join_list->n_acts() ) abort();
	
    switch ( join_list->type() ) {
    case ACT_AND_JOIN_LIST:

	if ( !join_list->FjP[m] ) {

	    /* Create place and link join trans */

	    join_list->FjP[m] = move_place_tag( create_place( X_OFFSET(p_pos,0.25), y_pos, layer_mask, 0, "AJ%s%d", join_list->list[0]->name(), m ), Place::PLACE_X_OFFSET, Place::PLACE_Y_OFFSET );
	}

	create_arc( layer_mask, TO_PLACE, join_trans, join_list->FjP[m] );
	    

	if ( join_list->u.join.type == JOIN_SYNCHRONIZATION ) {
#if defined(BUG_163)
	    create_arc( layer_mask, TO_PLACE, join_trans, task()->SyX[m] );	/* Release task */
#else
	    create_arc( layer_mask, TO_PLACE, join_trans, task()->TX[m] );	/* Release task */
#endif
	}

	if ( join_list->FjT[0][m] == 0 ) {

	    /* Create join transition and arc. */

	    join_trans = move_trans_tag( create_trans( X_OFFSET(p_pos+1,0.0), y_pos+0.5, layer_mask, 1.0, 1, IMMEDIATE + 1, "aj%s%d", this->name(), m ), Place::PLACE_X_OFFSET, Place::PLACE_Y_OFFSET );
	    join_list->FjT[0][m] = join_trans;
#if defined(BUG_263)
	    if ( join_list->u.join.quorumCount == 0 ) {
#endif
		create_arc_mult( layer_mask, TO_TRANS, join_trans, join_list->FjP[m], join_list->n_acts() );
#if defined(BUG_263)
	    } else {
		struct trans_object * sink_trans = move_trans_tag( create_trans( X_OFFSET(p_pos+1,0.0), y_pos-0.5, layer_mask, 1.0, 1, IMMEDIATE + 1, "qs%s%d", this->name(), m ),
								   Place::PLACE_X_OFFSET, Place::PLACE_Y_OFFSET );
		struct place_object * sink_place = move_place_tag( create_place( X_OFFSET(p_pos+0.0,0.25), y_pos-1.0, layer_mask, 0, "QS%s%d", join_list->list[0]->name(), m ),
								   Place::PLACE_X_OFFSET, Place::PLACE_Y_OFFSET );
		struct place_object * pass_place = move_place_tag( create_place( X_OFFSET(p_pos+0.0,0.25), y_pos+1.0, layer_mask, 1, "QP%s%d", join_list->list[0]->name(), m ),
								   Place::PLACE_X_OFFSET, Place::PLACE_Y_OFFSET );
		create_arc_mult( layer_mask, TO_TRANS, join_trans, join_list->FjP[m], join_list->u.join.quorumCount );
		create_arc_mult( layer_mask, TO_TRANS, sink_trans, join_list->FjP[m], join_list->n_acts() - join_list->u.join.quorumCount );
		create_arc( layer_mask, TO_TRANS, join_trans, pass_place );
		create_arc( layer_mask, TO_PLACE, join_trans, sink_place );
		create_arc( layer_mask, TO_TRANS, sink_trans, sink_place );
		create_arc( layer_mask, TO_PLACE, sink_trans, pass_place );
		/* Don't allow the task to proceed until all threads flush */
		create_arc( layer_mask, INHIBITOR, task()->gdX[m], sink_place );
	    }
#endif

	    if ( join_list->u.join.type == JOIN_SYNCHRONIZATION ) {
#if defined(BUG_163)
		create_arc( layer_mask, TO_TRANS, join_trans, task()->SyX[m] );	/* Acquire task */
#else
		create_arc( layer_mask, TO_TRANS, join_trans, task()->TX[m] );	/* Acquire task */
#endif
	    }

	    
	    if ( !join_list->u.join.FjM[m]
		 && join_list->u.join.fork[0]
		 && join_list->u.join.fork[0]->u.fork.prev->FjF[m] ) {

		/* Create a place for measuring the join delay. */

		if ( m == 0 || distinguish_join_customers ) {
		    join_list->u.join.FjM[m] = move_place_tag( create_place( X_OFFSET(p_pos,-0.25), y_pos-1.0, MEASUREMENT_LAYER, 0, "JJ%s%d", join_list->u.join.fork[0]->u.fork.prev->list[0]->name(), m ),
							       Place::PLACE_X_OFFSET, Place::PLACE_Y_OFFSET );
		} else {
		    join_list->u.join.FjM[m] = join_list->u.join.FjM[0];
		}
		create_arc( MEASUREMENT_LAYER, TO_TRANS, join_trans, join_list->u.join.FjM[m] );
		create_arc( MEASUREMENT_LAYER, TO_PLACE, join_list->u.join.fork[0]->u.fork.prev->FjF[m], join_list->u.join.FjM[m] );
	    }

	} else {

	    /* Outgoing arc already exists. */
			
	    return false;

	}
	x_pos += 0.5 * ne;
	break;
		
    case ACT_OR_JOIN_LIST:
	if ( !join_list->FjP[m] ) {
	    /* Create place and link join trans */
	    join_list->FjP[m] = move_place_tag( create_place( X_OFFSET(p_pos,0.25), y_pos, layer_mask, 0, "OJ%s%d", this->name(), m ), Place::PLACE_X_OFFSET, Place::PLACE_Y_OFFSET );
	    create_arc( layer_mask, TO_PLACE, join_trans, join_list->FjP[m] );
	    join_trans = move_trans_tag( create_trans( X_OFFSET(p_pos+1,0.0), y_pos+0.5, layer_mask, 1.0, 1, IMMEDIATE + 1, "oj%s%d", this->name(), m ), Place::PLACE_X_OFFSET, Place::PLACE_Y_OFFSET );
	    create_arc( layer_mask, TO_TRANS, join_trans, join_list->FjP[m] );
	    join_list->FjT[0][m] = join_trans;
	} else {
	    /* Outgoing arc already exists. stop after connecting. */
	    create_arc( layer_mask, TO_PLACE, join_trans, join_list->FjP[m] );
	    return false;
	}
	x_pos += 0.5 * ne;
	break;
	
    case ACT_JOIN_LIST:
	if ( !join_list->FjT[0][m] ) {
	    join_list->FjT[0][m] = join_trans;
	}
	break;

    default:
	abort();
    }

    if ( !fork_list ) return true;

    switch ( fork_list->type() ) {
    case ACT_AND_FORK_LIST:
	fork_list->u.fork.prev->FjF[m] = join_trans;

	if ( !fork_list->u.fork.join ) {

	    /* No join, so connect to the flush place. */

	    for ( i = 0; i < fork_list->n_acts(); ++i ) {
		next_act = fork_list->list[i];
		if ( fork_list->u.fork.reachable[i] || (!task()->has_main_thread() && i == 0) ) {

		    /* this is the branch that reaches a join (i.e. main branch) */

		    curr_pos = next_act->transmorgrify( x_pos, y_pos+i, m, e, p_pos, enabling, end_place, can_reply );
		    if ( end_place ) {
			move_place( end_place, X_OFFSET(curr_pos,0.), y_pos+i-1.0 );
		    }

		} else {

		    /* This is the branch that does not join. */

		    struct place_object * flush_place = move_place_tag( create_place( X_OFFSET(p_pos,-0.25), y_pos+i-1, JOIN_LAYER, 0, "FF%s%d", next_act->name(), m ), Place::PLACE_X_OFFSET, Place::PLACE_Y_OFFSET );
		    create_arc( JOIN_LAYER, TO_TRANS, task()->gdX[m], flush_place );
		    curr_pos = next_act->transmorgrify( x_pos, y_pos+i, m, e, p_pos, enabling, flush_place, can_reply );
		    move_place( flush_place, X_OFFSET(curr_pos,-0.75), y_pos+i );

		}

		if ( !task()->inservice_flag() || task()->type() != SERVER ) {
		    create_arc( layer_mask, TO_PLACE, join_trans, next_act->ZX[m] );
		}

		if ( curr_pos > next_pos ) {
		    next_pos = curr_pos;
		}
	    }
	    break;

	} else if ( !fork_list->u.fork.join->u.join.FjM[m] && fork_list->u.fork.join->FjT[0][m] ) {

	    /* We have a matching join, but no measurement place, */
	    /* so create a place for measuring the join delay. 	*/

	    if ( m == 0 || distinguish_join_customers ) {
		fork_list->u.fork.join->u.join.FjM[m] = move_place_tag( create_place( PIX_TO_IN(fork_list->u.fork.join->FjT[0][m]->center.x), y_pos, JOIN_LAYER, 0, "JJ%s%d", fork_list->u.fork.prev->list[0]->name(), m ),
									Place::PLACE_X_OFFSET, Place::PLACE_Y_OFFSET );
	    } else {
		fork_list->u.fork.join->u.join.FjM[m] = fork_list->u.fork.join->u.join.FjM[0];
	    }
	    create_arc( JOIN_LAYER, TO_TRANS, fork_list->u.fork.join->FjT[0][m], fork_list->u.fork.join->u.join.FjM[m] );
	    create_arc( JOIN_LAYER, TO_PLACE, join_trans, fork_list->u.fork.join->u.join.FjM[m] );
	}
	/* Fall through */
    case ACT_FORK_LIST:
	for ( i = 0; i < fork_list->n_acts(); ++i ) {
	    next_act = fork_list->list[i];
	    curr_pos = next_act->transmorgrify( x_pos, y_pos+i, m, e, p_pos, enabling, end_place, can_reply );
	    if ( !task()->inservice_flag() || task()->is_client() ) {
		create_arc( layer_mask, TO_PLACE, join_trans, next_act->ZX[m] );
	    }
	    if ( curr_pos > next_pos ) {
		next_pos = curr_pos;
	    }
	}
	break;

    case ACT_OR_FORK_LIST:
	/* Create place */
		
	fork_list->FjP[m] = move_place_tag( create_place( X_OFFSET(p_pos,0.25), y_pos, layer_mask, 0, "OF%s%d", this->name(), m ),
					       Place::PLACE_X_OFFSET, Place::PLACE_Y_OFFSET );
	create_arc( layer_mask, TO_PLACE, join_trans, fork_list->FjP[m] );

	/* Now create OR-branches. */
		
	sum = 0.0;
	for ( i = 0; i < fork_list->n_acts(); ++i ) {
	    next_act = fork_list->list[i];
	    sum += fork_list->u.fork.prob[i];
	    fork_list->FjT[i][m] = move_trans_tag( create_trans( X_OFFSET(p_pos+1,0.0), y_pos+i-0.5, layer_mask, fork_list->u.fork.prob[i], 1, IMMEDIATE, "oj%s%d", next_act->name(), m ),
						   Place::PLACE_X_OFFSET, Place::PLACE_Y_OFFSET );
	    join_trans = fork_list->FjT[i][m];
	    create_arc( layer_mask, TO_TRANS, join_trans, fork_list->FjP[m] );

	    curr_pos = next_act->transmorgrify( x_pos+0.5, y_pos+i, m, e, p_pos, enabling, end_place, can_reply );
	    if ( !task()->inservice_flag() || task()->is_client() ) {
		create_arc( layer_mask, TO_PLACE, join_trans, next_act->ZX[m] );
	    }
	    if ( curr_pos > next_pos ) {
		next_pos = curr_pos;
	    }
	}
	if ( fabs( 1.0 - sum ) > EPSILON ) {
	    char * aBuf = fork_list->fork_join_name();
	    LQIO::solution_error( LQIO::ERR_MISSING_OR_BRANCH, aBuf, this->name(), sum );
	    free( aBuf );
	}
	break;

    case ACT_LOOP_LIST:
	sum = 1;
	for ( i = 0; i < fork_list->n_acts(); ++i ) {
	    sum += fork_list->u.loop.count[i];
	}

	fork_list->u.loop.LoopP[m] = move_place_tag( create_place( X_OFFSET(p_pos,0.25), y_pos, layer_mask, 0, "LOOP%s%d", this->name(), m ),
						     Place::PLACE_X_OFFSET, Place::PLACE_Y_OFFSET );
	create_arc( layer_mask, TO_PLACE, join_trans, fork_list->u.loop.LoopP[m] );

	/* termination of loop */

	next_act = fork_list->u.loop.endlist;
	fork_list->u.loop.LoopT[m] = create_trans( X_OFFSET(p_pos+1,0.0), y_pos-0.5, layer_mask,
						   1.0/sum, 1, IMMEDIATE,
						   "lend%s%d", this->name(), m );
	join_trans = fork_list->u.loop.LoopT[m];
	create_arc( layer_mask, TO_TRANS, join_trans, fork_list->u.loop.LoopP[m] );

	if ( next_act ) {
	    next_pos = next_act->transmorgrify( x_pos+0.5, y_pos, m, e, p_pos, enabling, end_place, can_reply );
	    if ( !task()->inservice_flag() || task()->is_client()  ) {
		create_arc( layer_mask, TO_PLACE, join_trans, next_act->ZX[m] );
	    }
	} else {
	    create_arc( layer_mask, TO_PLACE, join_trans, end_place );
#if defined(BUG_622)
	    if ( !task()->gdX[m] && can_reply ) {
		/* activities end normally, so link to done */
		create_arc( MEASUREMENT_LAYER, TO_TRANS, join_trans, e->phase[e->n_phases()].XX[m] );	/* End phase n */
	    }
#endif
	}

	/* Sub-branch of loop */

	for ( i = 0; i < fork_list->n_acts(); ++i ) {
	    fork_list->FjT[i][m] = move_trans_tag( create_trans( X_OFFSET(p_pos+1,0.0), y_pos+0.5, layer_mask, fork_list->u.loop.count[i]/sum, 1, IMMEDIATE, "loop%s%d", fork_list->list[i]->name(), m ),
						   Place::PLACE_X_OFFSET, Place::PLACE_Y_OFFSET );
	    join_trans = fork_list->FjT[i][m];
	    create_arc( layer_mask, TO_TRANS, join_trans, fork_list->u.loop.LoopP[m] );

	    curr_pos = fork_list->list[i]->transmorgrify( x_pos+0.5, y_pos+1, m, e, p_pos,
							  enabling, fork_list->u.loop.LoopP[m], false );
	    create_arc( layer_mask, TO_PLACE, join_trans, fork_list->list[i]->ZX[m] );
	    if ( curr_pos > next_pos ) {
		next_pos = curr_pos;
	    }
	}

	break;
		
    default:
	abort();
    }

    if ( next_pos > p_pos ) {
	p_pos = next_pos;
    }

    return false;
}

void 
Activity::insert_DOM_results()
{
    Phase::insert_DOM_results();

    LQIO::DOM::Activity * dom = dynamic_cast<LQIO::DOM::Activity *>(get_dom());
    dom->setResultThroughput(_throughput[0]);
}


/*
 * Recursively follow the path taken from this activity and gobble up tokens.
 * Routing and gobbling are dependent on the sum_forks and task_utilization variables.
 */


void
Activity::follow_activity_for_tokens( const Entry * e, unsigned p, const unsigned m,
				      const bool sum_forks, const double scale,
				      util_fnptr util_func, double mean_tokens[] )
{
    mean_tokens[p] += scale * (this->*util_func)( m );
    if ( util_func == &Phase::get_utilization ) {
	_throughput[m] = get_tput( IMMEDIATE, "done%s%d", name(), m );
    }

    if ( replies_to( e ) ) {
	p = 2;
    }

    if ( _output ) {
	_output->follow_join_for_tokens( e, p, m, this, sum_forks, scale, util_func, mean_tokens );
    }
}



double Activity::residence_time() const
{
    return task_tokens[0] / _throughput[0];
}

/* ------------------------------------------------------------------------ */

/*
 * Add activity to the sequence list.  
 */

ActivityList *
Activity::act_join_item( LQIO::DOM::ActivityList * dom_activitylist )
{
    ActivityList * list = 0;

    if ( _output ) {
	LQIO::input_error2( LQIO::ERR_DUPLICATE_ACTIVITY_LVALUE, name() );
    } else {
	list = realloc_list( ACT_JOIN_LIST, 0, dom_activitylist );
	list->u.join.quorumCount = 0;
	list->list[list->_n_acts++] = this;
	_output = list;
    }

    return list;
}


/*
 * Add activity to the activity_list.  This list is for AND joining.
 */

ActivityList *
Activity::act_and_join_list ( ActivityList* input_list, LQIO::DOM::ActivityList * dom_activitylist )
{
    if ( _output ) {
	LQIO::input_error2( LQIO::ERR_DUPLICATE_ACTIVITY_LVALUE, name() );
	return input_list;
    } 

    ActivityList * list = realloc_list( ACT_AND_JOIN_LIST, input_list, dom_activitylist );
    list->list[list->_n_acts++] = this;
    _output = list;

    list->u.join.quorumCount = dynamic_cast<LQIO::DOM::AndJoinActivityList*>(dom_activitylist)->getQuorumCountValue();
    if ( list->u.join.quorumCount > 0 ) {
	Task * a_task = const_cast<Task *>(task());
	a_task->set_n_phases( 2 );
    }
          
    return list;
}



/*
 * Add activity to the activity_list.  This list is for OR joining.
 */

ActivityList *
Activity::act_or_join_list ( ActivityList * input_list, LQIO::DOM::ActivityList * dom_activitylist )
{
    ActivityList * list = input_list;

    if ( _output ) {
	LQIO::input_error2( LQIO::ERR_DUPLICATE_ACTIVITY_LVALUE, name() );
    } else {
	list = realloc_list( ACT_OR_JOIN_LIST, input_list, dom_activitylist );
	list->list[list->_n_acts++] = this;
	_output = list;
    }
    return list;
}



ActivityList *
Activity::act_fork_item( LQIO::DOM::ActivityList * dom_activitylist)
{
    ActivityList * list = 0;

    if ( _is_start_activity ) {
	LQIO::input_error2( LQIO::ERR_IS_START_ACTIVITY, name() );
    } else if ( _input ) {
	LQIO::input_error2( LQIO::ERR_DUPLICATE_ACTIVITY_RVALUE, name() );
    } else {
	list = realloc_list( ACT_FORK_LIST, 0, dom_activitylist );
	list->list[list->_n_acts++] = this;
	_input = list;
    }

    return list;
}


/*
 * Add activity to the activity_list.  This list is for AND forking.
 */

ActivityList *
Activity::act_and_fork_list ( ActivityList * input_list, LQIO::DOM::ActivityList * dom_activitylist )
{
    ActivityList * list = input_list;

    if ( _is_start_activity ) {
	LQIO::input_error2( LQIO::ERR_IS_START_ACTIVITY, name() );
    } else if ( _input ) {
	LQIO::input_error2( LQIO::ERR_DUPLICATE_ACTIVITY_RVALUE, name() );
    } else {
	list = realloc_list( ACT_AND_FORK_LIST, input_list, dom_activitylist );
	list->list[list->_n_acts++] = this;
	_input = list;
    }
    return list;
}



/*
 * Add activity to the activity_list.  This list is for OR forking.
 */

ActivityList *
Activity::act_or_fork_list ( ActivityList * input_list, LQIO::DOM::ActivityList * dom_activitylist )
{
    ActivityList * list = input_list;

    if ( _is_start_activity ) {
	LQIO::input_error2( LQIO::ERR_IS_START_ACTIVITY, name() );
    } else if ( _input ) {
	LQIO::input_error2( LQIO::ERR_DUPLICATE_ACTIVITY_RVALUE, name() );
    } else {
	list = realloc_list( ACT_OR_FORK_LIST, input_list, dom_activitylist );
	list->u.fork.prob[list->_n_acts] = dom_activitylist->getParameterValue(dynamic_cast<LQIO::DOM::Activity *>(get_dom()));
	list->list[list->_n_acts++] = this;
	_input = list;
    }
    return list;
}



/*
 * Add activity to the activity_list.  This list is for Looping.
 */

ActivityList *
Activity::act_loop_list ( ActivityList * input_list, LQIO::DOM::ActivityList * dom_activitylist )
{
    ActivityList * list = input_list;
	  
    if ( _is_start_activity ) {
	LQIO::input_error2( LQIO::ERR_IS_START_ACTIVITY, name() );
    } else if ( _input ) {
	LQIO::input_error2( LQIO::ERR_DUPLICATE_ACTIVITY_RVALUE, name() );
    } else {
	list = realloc_list( ACT_LOOP_LIST, input_list, dom_activitylist );
	LQIO::DOM::ExternalVariable * count = dom_activitylist->getParameter(dynamic_cast<LQIO::DOM::Activity *>(get_dom()));
	if ( count != NULL ) {
	    list->u.loop.count[list->_n_acts] = LQIO::DOM::to_double( *count );
	    list->list[list->_n_acts++] = this;
	} else {
	    list->u.loop.endlist = this;
	}
	_input = list;
    }
    return list;
}



/*
 * Allocate a list and storage for items in it.  The list will grow if necessary.
 */

ActivityList * 	
Activity::realloc_list ( const list_type type, const ActivityList * input_list, LQIO::DOM::ActivityList * dom_activity_list )
{
    ActivityList * list;
	
    if ( input_list ) {
	list = const_cast<ActivityList *>(input_list);
    } else if ( task()->n_act_lists() >= MAX_ACT*2 ) {
	input_error2( LQIO::ERR_TOO_MANY_X, "activity lists ", MAX_ACT*2 );
	return 0;
    } else {
	list = new ActivityList( type, dom_activity_list );
	const_cast<Task *>(task())->act_lists.push_back( list );
    }
    return list;
}



/* static */ void Activity::complete_activity_connections ()
{
    /* We stored all the necessary connections and resolved the list identifiers so finalize */
    std::map<LQIO::DOM::ActivityList*, LQIO::DOM::ActivityList*>::iterator iter;
    for (iter = Activity::actConnections.begin(); iter != Activity::actConnections.end(); ++iter) {
	ActivityList* src = Activity::domToNative[iter->first];
	ActivityList* dst = Activity::domToNative[iter->second];
	assert(src != NULL && dst != NULL);
	act_connect(src, dst);
    }
}



/*
 * Connect the src and dst lists together.
 */

/* static */ void Activity::act_connect ( ActivityList * src, ActivityList * dst )
{
    if ( src ) {
	assert( src->is_join_list() );
	src->u.join.next = dst;
    }
    if ( dst ) {
	if ( dst->is_loop_list() ) {
	    dst->u.loop.prev = src;
	} else if ( dst->is_fork_list() ) {
	    dst->u.fork.prev = src;
	} else {
	    abort();
	}
    }
}


