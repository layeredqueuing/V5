/*
 *  $Id: srvn_input.cpp 12554 2016-04-08 20:28:43Z greg $
 *
 *  Created by Martin Mroz on 24/02/09.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#if defined(HAVE_CONFIG_H)
#include <config.h>
#endif

#include <iostream>
#include <cstdlib>
#include <cstdarg>
#include <cassert>
#include <string>
#include <cstring>
#include <errno.h>
#if HAVE_SYS_ERRNO_H
#include <sys/errno.h>
#endif
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <sys/stat.h>
#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#if HAVE_SYS_MMAN_H
#include <sys/mman.h>
#endif

#include "dom_document.h"
#include "srvn_input.h"
#include "srvn_results.h"
#include "dom_histogram.h"
#include "input.h"
#include "error.h"
#include "glblerr.h"
#include "filename.h"


struct yy_buffer_state;

extern "C" {
    extern FILE * LQIO_in;		/* from srvn_gram.y, implicitly */
    extern yy_buffer_state * LQIO__scan_string( const char * );
    extern void LQIO__delete_buffer( yy_buffer_state * );
}

/* Pointer to the current document and entry list map */
namespace LQIO {
    namespace DOM {
	static ActivityList * act_and_fork_list( const Task *, Activity *, ActivityList *, const void * );
	static ActivityList * act_and_join_list( const Task *, Activity *, ActivityList *, ExternalVariable *, const void * );
	static ActivityList * act_fork_item( const Task *, Activity *, const void * );
	static ActivityList * act_join_item( const Task *, Activity *, const void * );
	static ActivityList * act_or_fork_list( const Task *, Activity *, ActivityList *, ExternalVariable *, const void * );
	static ActivityList * act_or_join_list( const Task *, Activity *, ActivityList *, const void * );
	static ActivityList * act_loop_list( const Task *, Activity *, ActivityList *, ExternalVariable *, const void * );
    }
}

static inline bool schedule_customer( scheduling_type scheduling_flag )
{
    return scheduling_flag == SCHEDULE_CUSTOMER || scheduling_flag == SCHEDULE_UNIFORM || scheduling_flag == SCHEDULE_BURST;
}

static void set_phase_name( const LQIO::DOM::Entry * entry, LQIO::DOM::Phase * phase, const unsigned i )
{
    if ( phase->getName().size() > 0 ) return;
    string name = entry->getName();
    name += "_ph";
    name += "0123"[i];
    phase->setName( name );
}


  
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
  
  
void *
srvn_add_processor( const char *processor_name, scheduling_type scheduling_flag, 
		    void * cpu_quantum, void * n_cpus, int n_replicas, void * cpu_rate  )
{
    /* Build a Processor entity to map into the document */
    LQIO::DOM::Processor* processor = 0;

    if ( LQIO::DOM::currentDocument->getProcessorByName(processor_name) ) {
	LQIO::input_error2( LQIO::ERR_DUPLICATE_SYMBOL, "Processor", processor_name );
	return 0;
    } else if ( n_cpus == 0 ) {
	/* Force to SCHEDULE_DELAY */
	processor = new LQIO::DOM::Processor( LQIO::DOM::currentDocument, processor_name, SCHEDULE_DELAY, 
					      new LQIO::DOM::ConstantExternalVariable(1),
					      n_replicas, 0 );
    } else {
	processor = new LQIO::DOM::Processor( LQIO::DOM::currentDocument, processor_name, scheduling_flag, 
					      static_cast<LQIO::DOM::ExternalVariable *>(n_cpus),
					      n_replicas, 0 );
    }

    double value;
    if ( cpu_quantum != NULL && (!static_cast<const LQIO::DOM::ExternalVariable *>(cpu_quantum)->wasSet() || (static_cast<const LQIO::DOM::ExternalVariable *>(cpu_quantum)->getValue( value ) && value != 0.0)) ) {
	if ( scheduling_flag == SCHEDULE_FIFO
	      || scheduling_flag == SCHEDULE_HOL
	      || scheduling_flag == SCHEDULE_PPR
	      || scheduling_flag == SCHEDULE_RAND ) {
	    input_error2( LQIO::WRN_QUANTUM_SCHEDULING, processor_name, scheduling_type_str[scheduling_flag] );
	} else {
	    processor->setQuantum( static_cast<LQIO::DOM::ExternalVariable *>(cpu_quantum) );
	}
    } else if ( scheduling_flag == SCHEDULE_CFS ) {
	input_error2( LQIO::ERR_NO_QUANTUM_SCHEDULING, processor_name, scheduling_type_str[scheduling_flag] );
    }

    processor->setRate( static_cast<LQIO::DOM::ExternalVariable *>(cpu_rate) );
    
    /* Map into the document */
    LQIO::DOM::currentDocument->addProcessorEntity(processor);
    return processor;
}

void 
srvn_add_group( const char *group_name, void * group_share, const char *processor_name, int cap )
{
    /* Locate the appropriate resources, and spit out a warning if they cannot be found... */
    LQIO::DOM::Processor* processor = LQIO::DOM::currentDocument->getProcessorByName(processor_name);
    if (processor == NULL) {
	return;
    }
    
    /* Check if this group exists yet */
    if (LQIO::DOM::currentDocument->getGroupByName(group_name) != NULL) {
	LQIO::input_error2( LQIO::ERR_DUPLICATE_SYMBOL, "Group", group_name );
	return;
    }

    if ( processor->getSchedulingType() != SCHEDULE_CFS ) {
	LQIO::input_error2( LQIO::WRN_NON_CFS_PROCESSOR, group_name, processor_name );
	return;		/* Ignore group */
    }
    
    /* Store the group inside of the Document */
    LQIO::DOM::Group* group = new LQIO::DOM::Group(LQIO::DOM::currentDocument, group_name, processor, 
						   static_cast<LQIO::DOM::ExternalVariable *>( group_share ), static_cast<bool>(cap), 0);
    LQIO::DOM::currentDocument->addGroup(group);
    processor->addGroup(group);
}

void *
srvn_add_task (const char * task_name, const scheduling_type scheduling, const void * entries, 
	       void * queue_length, const char * processor_name, const int priority, 
	       void * think_time, void * n_copies, const int n_replicas, const char * group_name )
{
    /* Obtain the processor we will add to */
    LQIO::DOM::Processor* processor = LQIO::DOM::currentDocument->getProcessorByName(processor_name);
    if ( !processor ) {
	LQIO::input_error2( LQIO::ERR_NOT_DEFINED, processor_name );
	return 0;
    }

    /* Ditto for the group, if specified */
    LQIO::DOM::Group * group = NULL;
    if ( group_name ) {
	group = LQIO::DOM::currentDocument->getGroupByName(group_name);
	if ( !group ) {
	    LQIO::input_error2( LQIO::ERR_NOT_DEFINED, group_name );
	}
    }

    double value;
    LQIO::DOM::Task* task = 0;
    if ( LQIO::DOM::currentDocument->getTaskByName(task_name) ) {
	LQIO::input_error2( LQIO::ERR_DUPLICATE_SYMBOL, "Task", task_name );
	return 0;

    } else if ( !entries ) {
	LQIO::input_error2( LQIO::ERR_NO_ENTRIES_DEFINED_FOR_TASK, task_name );
	return 0;

    } else if ( scheduling == SCHEDULE_SEMAPHORE ) {
	task = new LQIO::DOM::SemaphoreTask( LQIO::DOM::currentDocument, task_name, 
					     *static_cast<const std::vector<LQIO::DOM::Entry *>*>(entries), processor, 
					     static_cast<LQIO::DOM::ExternalVariable *>(queue_length), priority, 
					     static_cast<LQIO::DOM::ExternalVariable *>(n_copies), 
					     n_replicas, group, (void *)0);

    } else if ( scheduling == SCHEDULE_RWLOCK ) {
	task = new LQIO::DOM::RWLockTask( LQIO::DOM::currentDocument, task_name, 
					  *static_cast<const std::vector<LQIO::DOM::Entry *>*>(entries), processor, 
					  static_cast<LQIO::DOM::ExternalVariable *>(queue_length), priority, 
					  static_cast<LQIO::DOM::ExternalVariable *>(n_copies), 
					  n_replicas, group, (void *)0);


    } else if ( n_copies == 0 					/* Always infinite if copies == 0 */
		|| scheduling == SCHEDULE_DELAY
		|| (static_cast<LQIO::DOM::ExternalVariable *>(n_copies)->wasSet() && static_cast<LQIO::DOM::ExternalVariable *>(n_copies)->getValue(value) && value == 0) ) {
	if ( schedule_customer( scheduling ) ) {
	    LQIO::input_error2( LQIO::ERR_REFERENCE_TASK_IS_INFINITE, "Task", task_name );
	}
	task = new LQIO::DOM::Task( LQIO::DOM::currentDocument, task_name, SCHEDULE_DELAY, *static_cast<const std::vector<LQIO::DOM::Entry *>*>(entries), processor, 
				    static_cast<LQIO::DOM::ExternalVariable *>(queue_length), priority, 
				    new LQIO::DOM::ConstantExternalVariable(1), 
				    n_replicas, group, (void *)0);

    } else {
	task = new LQIO::DOM::Task( LQIO::DOM::currentDocument, task_name, scheduling, *static_cast<const std::vector<LQIO::DOM::Entry *>*>(entries), processor, 
				    static_cast<LQIO::DOM::ExternalVariable *>(queue_length), priority, 
				    static_cast<LQIO::DOM::ExternalVariable *>(n_copies), 
				    n_replicas, group, (void *)0);
    }

    if ( think_time != NULL && (!static_cast<LQIO::DOM::ExternalVariable *>(think_time)->wasSet() || (static_cast<LQIO::DOM::ExternalVariable *>(think_time)->getValue( value ) && value != 0.0))) {
	if ( schedule_customer( scheduling ) ) {
	    task->setThinkTime( static_cast<LQIO::DOM::ExternalVariable *>(think_time) );
	} else {
	    LQIO::input_error2( LQIO::ERR_NON_REF_THINK_TIME, task_name );
	}
    }

    /* Link in the entity information */
    LQIO::DOM::currentDocument->addTaskEntity(task);
    processor->addTask(task);
    if ( group ) group->addTask(task);
    return task;
}
  
void
srvn_store_fanin( const char * src_name, const char * dst_name, int value )
{
    LQIO::DOM::Task* dst_task = LQIO::DOM::currentDocument->getTaskByName(dst_name);
    dst_task->setFanIn( src_name, value );
}

void
srvn_store_fanout( const char * src_name, const char * dst_name, int value )
{
    LQIO::DOM::Task* src_task = LQIO::DOM::currentDocument->getTaskByName(src_name);
    src_task->setFanOut( dst_name, value );
}

void * 
srvn_add_entry (const char * entry_name, const void * entry_list)
{
    /* This method adds an entry for the current name, with given next entry */

    std::vector<LQIO::DOM::Entry *> * entries = const_cast<std::vector<LQIO::DOM::Entry *>*>(static_cast<const std::vector<LQIO::DOM::Entry *> *>(entry_list));
    if ( LQIO::DOM::currentDocument->getEntryByName(entry_name) ) {
	LQIO::input_error2( LQIO::ERR_DUPLICATE_SYMBOL, "Entry", entry_name );
    } else {

	if ( !entries ) {
	    entries = new std::vector<LQIO::DOM::Entry *>;
	}

	LQIO::DOM::Entry* entry = new LQIO::DOM::Entry(LQIO::DOM::currentDocument, entry_name, (void *)0);
	entries->push_back(entry);
	LQIO::DOM::currentDocument->addEntry(entry);
    }
    return static_cast<void *>(entries);
}
  
void 
srvn_set_model_parameters (const char *model_comment, void * conv_val, void * it_limit, void * print_int, void * underrelax_coeff )
{
    LQIO::DOM::currentDocument->setModelParameters(model_comment, 
						   static_cast<LQIO::DOM::ExternalVariable*>(conv_val), 
						   static_cast<LQIO::DOM::ExternalVariable*>(it_limit), 
						   static_cast<LQIO::DOM::ExternalVariable*>(print_int), 
						   static_cast<LQIO::DOM::ExternalVariable*>(underrelax_coeff), 0);
}

void *
srvn_get_entry( const char * entry_name )
{
    LQIO::DOM::Entry * entry = LQIO::DOM::currentDocument->getEntryByName(entry_name);
    if ( entry == NULL ) {
	LQIO::input_error2( LQIO::ERR_NOT_DEFINED, entry_name );
	/* Define it to suppress additional errors */
	entry = new LQIO::DOM::Entry(LQIO::DOM::currentDocument, entry_name, (void *)0);
	LQIO::DOM::currentDocument->addEntry(entry);
    }
    return static_cast<void *>(entry);
}

void 
srvn_set_phase_type_flag (void * entry_v, unsigned n_phases, ...)
{
    LQIO::DOM::Entry* entry = static_cast<LQIO::DOM::Entry*>(entry_v);
    if ( !entry ) return; 

    LQIO::DOM::Document::db_check_set_entry(entry, entry->getName(), LQIO::DOM::Entry::ENTRY_STANDARD);
    
    /* Push all the times */
    va_list ap;
    va_start(ap, n_phases);
    for (unsigned int i = 1; i <= n_phases; i++) {
	LQIO::DOM::Phase* phase = entry->getPhase(i);
	const phase_type arg = (phase_type)va_arg(ap, int);
	phase->setPhaseTypeFlag(arg);
    }
    
    /* Close the argument list */
    va_end(ap);
}

void 
srvn_set_semaphore_flag ( void * entry_v, semaphore_entry_type set )
{
    LQIO::DOM::Entry* entry = static_cast<LQIO::DOM::Entry*>(entry_v);
    if ( !entry ) return;
    entry->setSemaphoreFlag(set);
}

void 
srvn_set_rwlock_flag ( void * entry_v, rwlock_entry_type set )
{
    LQIO::DOM::Entry* entry = static_cast<LQIO::DOM::Entry*>(entry_v);
    if ( !entry ) return;
    entry->setRWLockFlag(set);
}

void 
srvn_store_coeff_of_variation (void * entry_v, unsigned n_phases, ...)
{
    /* Obtain the entry that we will be adding the phase think times to */
    LQIO::DOM::Entry* entry = static_cast<LQIO::DOM::Entry*>(entry_v);
    if ( !entry ) return;

    LQIO::DOM::Document::db_check_set_entry(entry, entry->getName(), LQIO::DOM::Entry::ENTRY_STANDARD);

    /* Push all the times */
    va_list ap;
    va_start(ap, n_phases);
    for (unsigned int i = 1; i <= n_phases; i++) {
	void * arg = va_arg(ap, void *);
	if ( arg != 0 ) {
	    LQIO::DOM::Phase* phase = entry->getPhase(i);
	    phase->setCoeffOfVariationSquared(static_cast<LQIO::DOM::ExternalVariable *>(arg));
	} else if ( entry->hasPhase(i) ) {
	    LQIO::DOM::Phase* phase = entry->getPhase(i);
	    phase->setCoeffOfVariationSquaredValue( 0. );
	}
    }
    
    /* Close the argument list */
    va_end(ap);
}

void 
srvn_store_entry_priority ( void * entry_v, const int arg )
{
    /* Obtain the entry that we will be adding the phase times to */
    LQIO::DOM::Entry* entry = static_cast<LQIO::DOM::Entry*>(entry_v);
    if ( !entry ) return;

    LQIO::DOM::Document::db_check_set_entry(entry, entry->getName());
    
    /* Store the open arrival rate */
    entry->setEntryPriority(new LQIO::DOM::ConstantExternalVariable(arg));
}

void 
srvn_store_open_arrival_rate (void  * entry_v, void * arg )
{
    /* Obtain the entry that we will be adding the phase times to */
    LQIO::DOM::Entry* entry = static_cast<LQIO::DOM::Entry*>(entry_v);
    if ( !entry ) return;
    LQIO::DOM::Document::db_check_set_entry(entry, entry->getName());
    
    /* Store the open arrival rate */
    entry->setOpenArrivalRate(static_cast<LQIO::DOM::ExternalVariable *>(arg));
}

void 
srvn_store_phase_service_time (void * entry_v, unsigned n_phases, ... )
{
    /* Obtain the entry that we will be adding the phase times to */
    LQIO::DOM::Entry* entry = static_cast<LQIO::DOM::Entry*>(entry_v);
    if ( !entry ) return;

    /* Make sure that this is a standard entry */
    LQIO::DOM::Document::db_check_set_entry(entry, entry->getName(), LQIO::DOM::Entry::ENTRY_STANDARD);
    
    /* Push all the times */
    va_list ap;
    va_start(ap, n_phases);
    for (unsigned int i = 1; i <= n_phases; i++) {
	void * arg = va_arg(ap, void *);
	if ( arg == 0 ) continue;
	LQIO::DOM::Phase* phase = entry->getPhase(i);
	set_phase_name( entry, phase, i );
	phase->setServiceTime(static_cast<LQIO::DOM::ExternalVariable *>(arg));
    }
    
    /* Close the argument list */
    va_end(ap);
}

void 
srvn_store_phase_think_time ( void * entry_v, unsigned n_phases, ... )
{
    /* Obtain the entry that we will be adding the phase think times to */
    LQIO::DOM::Entry* entry = static_cast<LQIO::DOM::Entry*>(entry_v);
    if ( !entry ) return;

    LQIO::DOM::Document::db_check_set_entry(entry, entry->getName(), LQIO::DOM::Entry::ENTRY_STANDARD);
    
    /* Push all the times */
    va_list ap;
    va_start(ap, n_phases);
    for (unsigned int i = 1; i <= n_phases; i++) {
	void * arg = va_arg(ap, void *);
	if (arg == 0) continue;
	LQIO::DOM::Phase* phase = entry->getPhase(i);
	set_phase_name( entry, phase, i );
	phase->setThinkTime(static_cast<LQIO::DOM::ExternalVariable *>(arg));
    }
    
    /* Close the argument list */
    va_end(ap);
}

void 
srvn_store_queue_insertion (const char *entry_name, const char *queue_name, double rate)
{
    /* +-+ #UNKNOWN/DEAD# +-+ */
    abort();
}

void 
srvn_store_prob_forward_data ( void * from_entry_v, void * to_entry_v, void * prob  )
{
    LQIO::DOM::Entry* from_entry = static_cast<LQIO::DOM::Entry *>(from_entry_v);
    LQIO::DOM::Entry* to_entry = static_cast<LQIO::DOM::Entry *>(to_entry_v);
    if ( from_entry == NULL || to_entry == NULL) return;

    LQIO::DOM::Document::db_check_set_entry(from_entry, from_entry->getName());
    LQIO::DOM::Document::db_check_set_entry(to_entry, to_entry->getName());
    
    /* Build a variable for the storage of the P(fwd) and set it on the originator */
    LQIO::DOM::Call * call = from_entry->getForwardingToTarget(to_entry);
    if ( call == NULL ) {
	LQIO::DOM::Call* call = new LQIO::DOM::Call(LQIO::DOM::currentDocument, from_entry, to_entry, static_cast<LQIO::DOM::ExternalVariable *>(prob) );
	from_entry->addForwardingCall(call);
	string name = from_entry->getName();
	name += '_';
	name += to_entry->getName();
	call->setName(name);
    } else if (call->getCallType() != LQIO::DOM::Call::NULL_CALL) {
	LQIO::input_error2( LQIO::WRN_MULTIPLE_SPECIFICATION );
    }
}

void 
srvn_store_rnv_data (void * from_entry_v, void * to_entry_v, unsigned n_phases, ...)
{
    /* Obtain the entry that we will be adding the phase times to */
    LQIO::DOM::Entry* from_entry = static_cast<LQIO::DOM::Entry *>(from_entry_v);
    LQIO::DOM::Entry* to_entry = static_cast<LQIO::DOM::Entry *>(to_entry_v);

    if ( from_entry == NULL || to_entry == NULL) {
	return;
    } else if ( from_entry == to_entry ) {
        LQIO::input_error2( LQIO::ERR_SRC_EQUALS_DST, from_entry->getName().c_str(), to_entry->getName().c_str() );
	return;
    }
    /* Make sure that this is a standard entry */
    LQIO::DOM::Document::db_check_set_entry(from_entry, from_entry->getName(), LQIO::DOM::Entry::ENTRY_STANDARD);
    LQIO::DOM::Document::db_check_set_entry(to_entry, to_entry->getName());
    
    /* Push all the times */
    va_list ap;
    va_start(ap, n_phases);
    for (unsigned int i = 1; i <= n_phases; i++) {
	void * arg = va_arg(ap, void *);
	if (arg == 0) continue;
	LQIO::DOM::Phase* phase = from_entry->getPhase(i);
	set_phase_name( from_entry, phase, i );

	LQIO::DOM::ExternalVariable* ev = static_cast<LQIO::DOM::ExternalVariable *>(arg);
	LQIO::DOM::Call* call = from_entry->getCallToTarget(to_entry, i);
        
	/* Check the existence */
	if (call == NULL) {
	    call = new LQIO::DOM::Call(LQIO::DOM::currentDocument, LQIO::DOM::Call::RENDEZVOUS, phase, to_entry, ev);
	    phase->addCall( call );
	    string name = phase->getName();
	    name += '_';
	    name += to_entry->getName();
	    call->setName(name);
	} else {
	    if (call->getCallType() != LQIO::DOM::Call::NULL_CALL) {
		LQIO::input_error2( LQIO::WRN_MULTIPLE_SPECIFICATION );
	    }
          
	    /* Set the new call type and the new mean */
	    call->setCallType(LQIO::DOM::Call::RENDEZVOUS);
	    call->setCallMean(ev);
	}
    }
    
    /* Close the argument list */
    va_end(ap);
}

void 
srvn_store_snr_data ( void * from_entry_v, void * to_entry_v, unsigned n_phases, ...)
{
    /* Obtain the entry that we will be adding the phase times to */
    LQIO::DOM::Entry* from_entry = static_cast<LQIO::DOM::Entry *>(from_entry_v);
    LQIO::DOM::Entry* to_entry = static_cast<LQIO::DOM::Entry *>(to_entry_v);

    if ( from_entry == NULL || to_entry == NULL) {
	return;
    } else if ( from_entry == to_entry ) {
	LQIO::input_error2( LQIO::ERR_SRC_EQUALS_DST, from_entry->getName().c_str(), to_entry->getName().c_str() );
	return;
    }
    /* Make sure that this is a standard entry */
    LQIO::DOM::Document::db_check_set_entry(from_entry, from_entry->getName(), LQIO::DOM::Entry::ENTRY_STANDARD);
    LQIO::DOM::Document::db_check_set_entry(to_entry, to_entry->getName());
    
    /* Push all the times */
    va_list ap;
    va_start(ap, n_phases);
    for (unsigned int i = 1; i <= n_phases; i++) {
	void * arg = va_arg(ap, void *);
	if (arg == 0) continue;
	LQIO::DOM::Phase* phase = from_entry->getPhase(i);
	set_phase_name( from_entry, phase, i );

	LQIO::DOM::ExternalVariable* ev = static_cast<LQIO::DOM::ExternalVariable *>(arg);
	LQIO::DOM::Call* call = from_entry->getCallToTarget(to_entry, i);
        
	/* Check the existence */
	if (call == NULL) {
	    LQIO::DOM::Call* call = new LQIO::DOM::Call(LQIO::DOM::currentDocument,LQIO::DOM::Call::SEND_NO_REPLY, phase, to_entry, ev);
	    phase->addCall( call );
	    string name = phase->getName();
	    name += '_';
	    name += to_entry->getName();
	    call->setName(name);
	} else {
	    if (call->getCallType() != LQIO::DOM::Call::NULL_CALL) {
		LQIO::input_error2( LQIO::WRN_MULTIPLE_SPECIFICATION );
	    }
          
	    /* Set the new call type and the new mean */
	    call->setCallType(LQIO::DOM::Call::SEND_NO_REPLY);
	    call->setCallMean(ev);
	}
    }
    
    /* Close the argument list */
    va_end(ap);
}
  
void 
srvn_set_start_activity ( void * entry_v, const char * startActivityName )
{
    LQIO::DOM::Entry* entry = static_cast<LQIO::DOM::Entry *>(entry_v);
    if ( !entry ) return;
    LQIO::DOM::Document::db_check_set_entry(entry, entry->getName(), LQIO::DOM::Entry::ENTRY_ACTIVITY);
    LQIO::DOM::Task* task = const_cast<LQIO::DOM::Task*>(entry->getTask());
    LQIO::DOM::Activity* activity = task->getActivity(startActivityName, true);
    entry->setStartActivity(activity);
}


void 
srvn_set_tokens( void * task_v, unsigned int tokens )
{
    LQIO::DOM::Task * task = static_cast<LQIO::DOM::Task *>(task_v);
    if ( !task ) return;
    if ( dynamic_cast<LQIO::DOM::SemaphoreTask *>(task) && tokens == 0 ) {
	dynamic_cast<LQIO::DOM::SemaphoreTask *>(task)->setInitialState(LQIO::DOM::SemaphoreTask::INITIALLY_EMPTY);
    }
}

/*
 * find an activity.  
 */

void *
srvn_find_activity( void * task, const char * name )
{
    LQIO::DOM::Activity * activity = static_cast<LQIO::DOM::Task *>(task)->getActivity(name, false);
    if ( !activity ) {
	input_error2( LQIO::ERR_NOT_DEFINED, name );
    }
    return activity;
}

/*
 * find an activity.  If not found, create it.
 */

void *
srvn_get_activity( void * task, const char * name )
{
    return static_cast<LQIO::DOM::Task *>(task)->getActivity(name, true);
}

void
srvn_set_activity_call_name( const void * task, const void * activity, const void * entry, void * call ) 
{
    if ( !task || !activity || !entry || !call ) return;
    string name = static_cast<const LQIO::DOM::Task *>(task)->getName();
    name += '_';
    name += static_cast<const LQIO::DOM::Activity *>(activity)->getName();
    name += '_';
    name += static_cast<const LQIO::DOM::Entry *>(entry)->getName();
    static_cast<LQIO::DOM::Call *>(call)->setName(name);
}


void 
srvn_store_activity_coeff_of_variation( void * activity, void * cv2 )
{
    if ( !activity ) return;
    static_cast<LQIO::DOM::Activity *>(activity)->setCoeffOfVariationSquared(static_cast<LQIO::DOM::ExternalVariable *>(cv2));
}
 
void  *
srvn_store_activity_rnv_data ( void * activity, void * dst_entry_v, void * calls ) 
{
    LQIO::DOM::Entry* dst_entry = static_cast<LQIO::DOM::Entry *>(dst_entry_v);
    if ( !activity || !dst_entry ) return 0;
    LQIO::DOM::Document::db_check_set_entry(dst_entry, dst_entry->getName());
    
    LQIO::DOM::Call* call = new LQIO::DOM::Call(LQIO::DOM::currentDocument, LQIO::DOM::Call::RENDEZVOUS, static_cast<LQIO::DOM::Activity *>(activity), dst_entry,
						static_cast<LQIO::DOM::ExternalVariable *>(calls));
    static_cast<LQIO::DOM::Activity *>(activity)->addCall(call);
    return call;
}
 
void 
srvn_store_activity_service_time ( void * activity, void * service_time ) 
{
    static_cast<LQIO::DOM::Activity *>(activity)->setServiceTime(static_cast<LQIO::DOM::ExternalVariable *>(service_time));
    static_cast<LQIO::DOM::Activity *>(activity)->setIsSpecified(true);
}
 
void *
srvn_store_activity_snr_data ( void * activity, void * dst_entry_v, void * calls ) 
{
    LQIO::DOM::Entry* dst_entry = static_cast<LQIO::DOM::Entry *>(dst_entry_v);
    if ( !activity || !dst_entry ) return 0;
    LQIO::DOM::Document::db_check_set_entry(dst_entry, dst_entry->getName());
    
    LQIO::DOM::Call* call = new LQIO::DOM::Call(LQIO::DOM::currentDocument, LQIO::DOM::Call::SEND_NO_REPLY, static_cast<LQIO::DOM::Activity *>(activity), dst_entry,
						static_cast<LQIO::DOM::ExternalVariable *>(calls));
    static_cast<LQIO::DOM::Activity *>(activity)->addCall(call);
    return call;
}
 
void 
srvn_store_activity_think_time ( void * activity, void * think_time )
{
    static_cast<LQIO::DOM::Activity *>(activity)->setThinkTime(static_cast<LQIO::DOM::ExternalVariable *>(think_time));
}

void 
srvn_set_activity_histogram ( void * activity, const double min, const double max, const int n_bins )
{
    static_cast<LQIO::DOM::Activity *>(activity)->setHistogram(new LQIO::DOM::Histogram( LQIO::DOM::currentDocument, LQIO::DOM::Histogram::CONTINUOUS, n_bins, min, max, 0 ));
}

void 
srvn_set_activity_phase_type_flag ( void * activity, const int flag ) 
{
    static_cast<LQIO::DOM::Activity *>(activity)->setPhaseTypeFlag(static_cast<const phase_type>(flag));
}

void 
srvn_set_histogram ( void * entry_v, const unsigned phase, const double min, const double max, const int n_bins )
{
    /* Grab the entry */
    LQIO::DOM::Entry* entry = static_cast<LQIO::DOM::Entry *>(entry_v);
    LQIO::DOM::Document::db_check_set_entry(entry, entry->getName());
    if ( !entry ) return;
    
    /* Grab the phase and store the histogram.  DO NOT Create a phase. */
    if ( entry->hasPhase(phase) ) {
	entry->getPhase(phase)->setHistogram(new LQIO::DOM::Histogram(LQIO::DOM::currentDocument, LQIO::DOM::Histogram::CONTINUOUS, n_bins, min, max, 0 ));
    } else {
	entry->setHistogramForPhase(phase, new LQIO::DOM::Histogram(LQIO::DOM::currentDocument, LQIO::DOM::Histogram::CONTINUOUS, n_bins, min, max, 0 ));
    }
}

void * 
srvn_act_add_reply ( const void * task, const void * entry, void * entry_list )
{
    const LQIO::DOM::Task* domTask = static_cast<const LQIO::DOM::Task*>(task);
    const LQIO::DOM::Entry* domEntry = static_cast<const LQIO::DOM::Entry*>(entry);
    if (task == NULL || entry == NULL) return entry_list;
    /* This one is kinda neat since lots happens... */
    /* This method provides an initially NULL entry list and wants it back... */
    std::vector<LQIO::DOM::Entry*>* localEntryList = static_cast<std::vector<LQIO::DOM::Entry*>*>(entry_list);
    if (localEntryList == NULL) { localEntryList = new std::vector<LQIO::DOM::Entry*>(); }
    
    /* Now we need to find an entry for the given name */
    if ( domEntry->getTask() != task ) {
	LQIO::input_error2( LQIO::ERR_WRONG_TASK_FOR_ENTRY, domEntry->getName().c_str(), domTask->getName().c_str() );
    } else {
	localEntryList->push_back(const_cast<LQIO::DOM::Entry *>(domEntry));
    }
    
    return localEntryList;
}

void * 
srvn_act_and_fork_list ( const void * aTask, void * activity, void * pActivityList )
{
    return LQIO::DOM::act_and_fork_list( static_cast<const LQIO::DOM::Task *>(aTask), static_cast<LQIO::DOM::Activity *>(activity), 
					 static_cast<LQIO::DOM::ActivityList *>(pActivityList), 0 );
}

void * 
srvn_act_and_join_list ( const void * aTask, void * activity, void * pActivityList, void * quorum_count )
{
    return LQIO::DOM::act_and_join_list( static_cast<const LQIO::DOM::Task *>(aTask), static_cast<LQIO::DOM::Activity *>(activity), 
					 static_cast<LQIO::DOM::ActivityList *>(pActivityList), 
					 static_cast<LQIO::DOM::ExternalVariable *>(quorum_count), 0 );
}

void * 
srvn_act_fork_item ( const void * aTask, void * activity )
{
    return LQIO::DOM::act_fork_item( static_cast<const LQIO::DOM::Task *>(aTask), static_cast<LQIO::DOM::Activity *>(activity), 0 );
}

void * 
srvn_act_join_item ( const void * aTask, void * activity )
{
    return LQIO::DOM::act_join_item( static_cast<const LQIO::DOM::Task *>(aTask), static_cast<LQIO::DOM::Activity *>(activity), 0 );
}

void * 
srvn_act_or_fork_list ( const void * aTask, void * probability, void * activity, void * pActivityList)
{
    return LQIO::DOM::act_or_fork_list( static_cast<const LQIO::DOM::Task *>(aTask), static_cast<LQIO::DOM::Activity *>(activity), 
					static_cast<LQIO::DOM::ActivityList *>(pActivityList), 
					static_cast<LQIO::DOM::ExternalVariable *>(probability), 0 );
}

void * 
srvn_act_or_join_list ( const void * aTask, void * activity, void * pActivityList )
{
    return LQIO::DOM::act_or_join_list( static_cast<const LQIO::DOM::Task *>(aTask), static_cast<LQIO::DOM::Activity *>(activity), 
					      static_cast<LQIO::DOM::ActivityList *>(pActivityList), 0 );
}

void * 
srvn_act_loop_list ( const void * aTask, void * count, void * activity, void * pActivityList )
{
    return LQIO::DOM::act_loop_list( static_cast<const LQIO::DOM::Task *>(aTask), static_cast<LQIO::DOM::Activity *>(activity), 
				     static_cast<LQIO::DOM::ActivityList *>(pActivityList), 
				     static_cast<LQIO::DOM::ExternalVariable *>(count), 0 );
}

void 
srvn_act_add_reply_list ( const void * aTask, void * activity, void * entry_list )
{
    /* Obtain references to the task and the activity from the parameter data */
    const LQIO::DOM::Task* domTask = static_cast<const LQIO::DOM::Task *>(aTask);
    if ( !domTask || !activity ) return;
    std::vector<LQIO::DOM::Entry*>* local_list = static_cast<std::vector<LQIO::DOM::Entry*>*>(entry_list);
    if ( local_list ) {
	static_cast<LQIO::DOM::Activity *>(activity)->getReplyList() = *local_list;
	delete local_list;
    }
}

void 
srvn_act_connect ( const void *, void * src, void * dst )
{
    /* Cast the lists to their DOM types for the connection */
    LQIO::DOM::ActivityList* srcList = static_cast<LQIO::DOM::ActivityList*>(src);
    LQIO::DOM::ActivityList* dstList = static_cast<LQIO::DOM::ActivityList*>(dst);
    
    /* Make the connection */
    if (src != NULL) {
	srcList->setNext(dstList);
    } if (dst != NULL) {
	dstList->setPrevious(srcList);
    }
}

void 
srvn_add_communication_delay (const char * from_proc, const char * to_proc, void * delay)
{
    /* +-+ #DEFER# +-+ */
    abort();
}

void 
srvn_pragma (const char* pragmaText)
{
    /* Add the pragma to the list for later evaluation */
    const char * p = pragmaText;
    while ( isspace( *p ) ) ++p;
    if ( *p++ != '#' ) return;
    while ( isspace( *p ) ) ++p;
    if ( tolower(*p++) != 'p' ) return;
    if ( tolower(*p++) != 'r' ) return;
    if ( tolower(*p++) != 'a' ) return;
    if ( tolower(*p++) != 'g' ) return;
    if ( tolower(*p++) != 'm' ) return;
    if ( tolower(*p++) != 'a' ) return;

    do {
	while ( isspace( *p ) ) ++p;		/* Skip leading whitespace. */
	string param;
	string value;
	while ( *p && !isspace( *p ) && *p != '=' && *p != ',' ) {
	    param += *p++;			/* get parameter */
	}
	while ( isspace( *p ) ) ++p;
	if ( *p == '=' ) {
	    ++p;
	    while ( isspace( *p ) ) ++p;
	    while ( *p && !isspace( *p ) && *p != ',' ) {
		value += *p++;
	    }
	}
	while ( isspace( *p ) ) ++p;
	LQIO::DOM::currentDocument->addPragma(param,value);
    } while ( *p++ == ',' );
}

void * 
srvn_find_task ( const char * taskName )
{
    /* Return the task with the given name */
    LQIO::DOM::Task* task = LQIO::DOM::currentDocument->getTaskByName(taskName);
    if ( !task ) {
	LQIO::input_error2( LQIO::ERR_NOT_DEFINED, taskName );
    }
    return static_cast<void *>(task);
}

void * srvn_int_constant( const int i )
{
    if ( i > 0 ) {
	return new LQIO::DOM::ConstantExternalVariable( static_cast<double>(i) );
    } else {
	return NULL;
    }
}

void * srvn_real_constant( const double d )
{
    if ( d > 0. ) {
	return new LQIO::DOM::ConstantExternalVariable( d );
    } else { 
	return NULL;
    }
}

void * srvn_variable( const char * s )
{
    return LQIO::DOM::currentDocument->db_build_parameter_variable(s,NULL);
}


namespace LQIO {
    namespace DOM {

	static ActivityList * 
	act_and_fork_list ( const Task * domTask, Activity * activity,
			    ActivityList * activityList, const void * element )
	{
	    /* Make sure we have a task */
	    if (domTask == NULL || activity == NULL) { return activityList; }

	    /* Configure the activity */
	    if (activity->isStartActivity()) {
		input_error2( ERR_IS_START_ACTIVITY, activity->getName().c_str() );
	    } else {
		if (!activityList) {
		    activityList = new ActivityList(LQIO::DOM::currentDocument,domTask,ActivityList::AND_FORK_ACTIVITY_LIST,element);
		}
		activity->inputFrom(activityList);
		activityList->add(activity);
	    }

	    /* Return the list itself */
	    return activityList;
	}

	static ActivityList * 
	act_and_join_list ( const Task * domTask, Activity * activity, 
			    ActivityList * activityList, ExternalVariable * quorum_count, const void * element  )
	{
	    /* Make sure we have a task */
	    if (domTask == NULL || activity == NULL) { return activityList; }

	    /* Create the AND join list */
	    if (activityList == NULL) {
		activityList = new AndJoinActivityList(LQIO::DOM::currentDocument,domTask,quorum_count,element);
	    }
	    activityList->add(activity);
	    activity->outputTo(activityList);
	    if ( quorum_count ) {
		dynamic_cast<AndJoinActivityList *>(activityList)->setQuorumCount( quorum_count );
	    }

	    /* Return the activity list */
	    return activityList;
	}

	static ActivityList * 
	act_fork_item ( const Task * domTask, Activity * activity, const void * element  )
	{
	    /* Make sure we have a task */
	    if (domTask == NULL || activity == NULL ) { return NULL; }
	    ActivityList* activityList = NULL;

	    /* Configure the activity */
	    activityList = new ActivityList(LQIO::DOM::currentDocument,domTask,ActivityList::FORK_ACTIVITY_LIST,element);
	    activity->inputFrom(activityList);
	    activityList->add(activity);

	    /* Return the list itself */
	    return activityList;
	}

	static ActivityList * 
	act_join_item ( const Task * domTask, Activity * activity, const void * element  )
	{
	    /* Make sure we have a task */
	    if (domTask == NULL || activity == NULL)  { return NULL; }
	    ActivityList* activityList = NULL;

	    /* Configure the activity */
	    activityList = new ActivityList(LQIO::DOM::currentDocument,domTask,ActivityList::JOIN_ACTIVITY_LIST,element);
	    activity->outputTo(activityList);
	    activityList->add(activity);

	    /* Return the list itself */
	    return activityList;
	}

	static ActivityList * 
	act_or_fork_list( const Task * domTask, Activity * activity, ActivityList * activityList,
			  ExternalVariable * probability, const void * element )
	{
	    /* Make sure we have a task */
	    if (domTask == NULL || !activity) { return activityList; }

	    /* Configure the activity */
	    double value = 0.;
	    if ( probability->wasSet() && probability->getValue(value) && (value < 0. || 1. < value) ) {
		input_error2( ERR_INVALID_PROBABILITY, probability );
	    } else {
		if (activity->isStartActivity()) {
		    input_error2( ERR_IS_START_ACTIVITY, activity->getName().c_str() );
		} else {
		    if (activityList == NULL) {
			activityList = new ActivityList(LQIO::DOM::currentDocument,domTask,ActivityList::OR_FORK_ACTIVITY_LIST,element);
		    }
		    activityList->add(activity, probability);
		    activity->inputFrom(activityList);
		}
	    }

	    /* Return the list itself */
	    return activityList;
	}

	static ActivityList * 
	act_or_join_list( const Task * domTask, Activity * activity,
			  ActivityList * activityList, const void * element  )
	{
	    /* Make sure we have a task */
	    if (domTask == NULL || activity == NULL) { return activityList; }

	    /* Configure the activity */
	    if (activityList == NULL) {
		activityList = new ActivityList(LQIO::DOM::currentDocument,domTask,ActivityList::OR_JOIN_ACTIVITY_LIST,element);
	    }
	    activityList->add(activity);
	    activity->outputTo(activityList);

	    /* Return the list itself */
	    return activityList;
	}

	static ActivityList * 
	act_loop_list( const Task * domTask, Activity * activity,
		       ActivityList * activityList,
		       LQIO::DOM::ExternalVariable * count, const void * element )
	{
	    /* Make sure we have a task */
	    if (domTask == NULL || activity == NULL) { return activityList; }

	    /* Configure the activity */
	    if (activityList == NULL) {
		activityList = new ActivityList(LQIO::DOM::currentDocument,domTask,ActivityList::REPEAT_ACTIVITY_LIST,element);
	    }
	    activityList->add(activity, count);
	    activity->inputFrom(activityList);

	    /* Return the list itself */
	    return activityList;
	}
    }
    extern lqio_params_stats* io_vars;
}


extern "C" {
    int LQIO_parse();
}

bool LQIO::SRVN::load(LQIO::DOM::Document& document, const string& input_filename, const string& output_filename, unsigned& errorCode, bool load_results )
{
    if ( input_filename == "-" ) {
	LQIO_in = stdin;
    } else if (!( LQIO_in = fopen( input_filename.c_str(), "r" ) ) ) {
	std::cerr << LQIO::DOM::Document::io_vars->lq_toolname << ": Cannot open input file " << LQIO::input_file_name << " - " << strerror( errno ) << std::endl;
	return 0;
    } 
    int LQIO_in_fd = fileno( LQIO_in );

    struct stat statbuf;
    if ( isatty( LQIO_in_fd ) ) {
	std::cerr << LQIO::DOM::Document::io_vars->lq_toolname << ": Input from terminal is not allowed." << std::endl;
	return 0;
    } else if ( fstat( LQIO_in_fd, &statbuf ) != 0 ) {
	std::cerr << LQIO::DOM::Document::io_vars->lq_toolname << ": Cannot stat " << input_file_name << " - " << strerror( errno ) << std::endl;
	return false;
#if defined(S_ISSOCK)
    } else if ( !S_ISREG(statbuf.st_mode) && !S_ISFIFO(statbuf.st_mode) && !S_ISSOCK(statbuf.st_mode) ) {
#else
    } else if ( !S_ISREG(statbuf.st_mode) && !S_ISFIFO(statbuf.st_mode) ) {
#endif
	std::cerr << LQIO::DOM::Document::io_vars->lq_toolname << ": Input from " << input_file_name << " is not allowed." << std::endl;
	return false;
    } 

    LQIO_lineno = 1;

#if HAVE_MMAP
    char * buffer = static_cast<char *>(mmap( 0, statbuf.st_size, PROT_READ, MAP_PRIVATE|MAP_FILE, LQIO_in_fd, 0 ));
    if ( buffer != MAP_FAILED ) {
	yy_buffer_state * yybuf = LQIO__scan_string( buffer );
	try {
	    errorCode = LQIO_parse();
	}
	catch ( runtime_error& e ) {
	    std::cerr << LQIO::DOM::Document::io_vars->lq_toolname << ": " << e.what() << "." << std::endl;
	    errorCode = 1;
	}
	LQIO__delete_buffer( yybuf );
	munmap( buffer, statbuf.st_size );
    } else {
#endif
	/* Try the old way (for pipes) */
	try {
	    errorCode = LQIO_parse();
	}
	catch ( runtime_error& e ) {
	    std::cerr << LQIO::DOM::Document::io_vars->lq_toolname << ": " << e.what() << "." << std::endl;
	    errorCode = 1;
	}
#if HAVE_MMAP
    }
#endif
    if ( LQIO::DOM::Document::io_vars->anError ) {
	errorCode = 1;
    } else if ( load_results && input_filename != "-" ) {
	LQIO::Filename parse_name( input_filename.c_str(), "p" );
	try {
	    if ( parse_name.mtimeCmp( input_filename.c_str() ) < 0 ) {
		std::cerr << LQIO::DOM::Document::io_vars->lq_toolname << ": input file " << input_filename << " is more recent than " << parse_name() 
			  << " -- results ignored. " << std::endl;
	    } else {
		LQIO::SRVN::loadResults( parse_name() );
	    }
	} 
	catch ( std::invalid_argument& err ) {
	    /* Ignore */
	}
    }


    if ( LQIO_in && LQIO_in != stdin ) {
	fclose( LQIO_in );
    }

    return errorCode == 0;
}




/*
 * Print out and error message (and the line number on which it
 * occurred.
 */

void
LQIO_error( const char * fmt, ... )
{
    extern size_t srvnleng;
    extern char *srvntext;

    va_list args;
    va_start( args, fmt );
    LQIO::verrprintf( stderr, LQIO::RUNTIME_ERROR, LQIO::input_file_name, LQIO_lineno, 0, fmt, args );
    va_end( args );
}


   
/*
 * Print out and error message (and the line number on which it
 * occurred.
 */

void
srvnwarning( const char * fmt, ... )
{
    extern size_t srvnleng;
    extern char *srvntext;

    va_list args;
    va_start( args, fmt );
    LQIO::verrprintf( stderr, LQIO::WARNING_ONLY, LQIO::input_file_name, LQIO_lineno, 0, fmt, args );
    va_end( args );
}


   
