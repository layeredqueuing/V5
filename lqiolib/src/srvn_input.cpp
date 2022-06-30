/*
 *  $Id: srvn_input.cpp 15734 2022-06-30 02:19:44Z greg $
 *
 *  Created by Martin Mroz on 24/02/09.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>
#include <cstdlib>
#include <cstdarg>
#include <cassert>
#include <string>
#include <cstring>
#include <cmath>
#include <limits>
#include <errno.h>
#if HAVE_SYS_ERRNO_H
#include <sys/errno.h>
#endif
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <sys/stat.h>
#if HAVE_SYS_MMAN_H
#include <sys/types.h>
#include <sys/mman.h>
#endif
#include "common_io.h"
#include "dom_activity.h"
#include "dom_actlist.h"
#include "dom_document.h"
#include "dom_entry.h"
#include "dom_group.h"
#include "dom_histogram.h"
#include "dom_phase.h"
#include "dom_processor.h"
#include "dom_task.h"
#include "error.h"
#include "filename.h"
#include "glblerr.h"
#include "input.h"
#include "srvn_input.h"
#include "srvn_results.h"

extern "C" {
struct yy_buffer_state;
    extern int LQIO_parse();

#include "srvn_gram.h"
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

/*
 * Return true if the var 'n' is set to infinity.
 */

static inline bool is_infinity( const LQIO::DOM::ExternalVariable * n )
{
    double value = 0.0;
    return n != nullptr && (n->wasSet() && n->getValue(value) && std::isinf(value));
}
    
static void set_phase_name( const LQIO::DOM::Entry * entry, LQIO::DOM::Phase * phase, const unsigned i )
{
    if ( phase->getName().size() > 0 ) return;
    std::string name = entry->getName();
    name += "_ph";
    name += "0123"[i];
    phase->setName( name );
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
  
/* 
 * Build a Processor entity to map into the document 
 */
  
void *
srvn_add_processor( const char *processor_name, scheduling_type scheduling_flag, void * cpu_quantum )
{
    LQIO::DOM::Processor* processor = LQIO::DOM::__document->getProcessorByName(processor_name);

    if ( processor != nullptr ) {
	processor->input_error( LQIO::ERR_DUPLICATE_SYMBOL );
	return nullptr;
    } 

    processor = new LQIO::DOM::Processor( LQIO::DOM::__document, processor_name, scheduling_flag );
//					  new LQIO::DOM::ConstantExternalVariable(1) );	/* Set the default to 1 */

    if ( !LQIO::DOM::Common_IO::is_default_value( static_cast<LQIO::DOM::ExternalVariable *>(cpu_quantum), 0. ) ) {
	if ( scheduling_flag == SCHEDULE_FIFO
	      || scheduling_flag == SCHEDULE_HOL
	      || scheduling_flag == SCHEDULE_PPR
	      || scheduling_flag == SCHEDULE_RAND ) {
	    input_error2( LQIO::WRN_QUANTUM_SCHEDULING, processor_name, scheduling_label[scheduling_flag].str );
	} else {
	    processor->setQuantum( static_cast<LQIO::DOM::ExternalVariable *>(cpu_quantum) );
	}
    } else if ( scheduling_flag == SCHEDULE_CFS ) {
	input_error2( LQIO::ERR_NO_QUANTUM_SCHEDULING, processor_name, scheduling_label[scheduling_flag].str );
    }

    /* Map into the document */
    LQIO::DOM::__document->addProcessorEntity(processor);
    return processor;
}

void *
srvn_add_group( const char *group_name, void * group_share, const char *processor_name, int cap )
{
    /* Locate the appropriate resources, and spit out a warning if they cannot be found... */
    LQIO::DOM::Processor* processor = LQIO::DOM::__document->getProcessorByName(processor_name);
    if (processor == nullptr) {
	return nullptr;
    }
    
    /* Check if this group exists yet */
    LQIO::DOM::Group* group = LQIO::DOM::__document->getGroupByName(group_name);
    if ( group != nullptr) {
	group->input_error( LQIO::ERR_DUPLICATE_SYMBOL );
	return nullptr;
    }

    if ( processor->getSchedulingType() != SCHEDULE_CFS ) {
	group->input_error( LQIO::WRN_NON_CFS_PROCESSOR, processor_name );
	return nullptr;		/* Ignore group */
    }
    
    /* Store the group inside of the Document */
    group = new LQIO::DOM::Group(LQIO::DOM::__document, group_name, processor, 
				 static_cast<LQIO::DOM::ExternalVariable *>( group_share ), static_cast<bool>(cap) );
    LQIO::DOM::__document->addGroup(group);
    processor->addGroup(group);
    return group;
}

void *
srvn_add_task (const char * task_name, const scheduling_type scheduling, const void * entries, const char * processor_name )
{
    /* Obtain the processor we will add to */
    LQIO::DOM::Processor* processor = LQIO::DOM::__document->getProcessorByName(processor_name);
    if ( processor == nullptr ) {
	LQIO::input_error2( LQIO::ERR_NOT_DEFINED, processor_name );
	return nullptr;
    }

    LQIO::DOM::Task* task = LQIO::DOM::__document->getTaskByName(task_name);
    if ( task != nullptr ) {
	task->input_error( LQIO::ERR_DUPLICATE_SYMBOL );
	return nullptr;

    } else if ( entries == nullptr ) {
	task->input_error( LQIO::ERR_TASK_HAS_NO_ENTRIES );
	return nullptr;
    } else if ( scheduling == SCHEDULE_SEMAPHORE ) {
	task = new LQIO::DOM::SemaphoreTask( LQIO::DOM::__document, task_name, *static_cast<const std::vector<LQIO::DOM::Entry *>*>(entries), processor );
    } else if ( scheduling == SCHEDULE_RWLOCK ) {
	task = new LQIO::DOM::RWLockTask( LQIO::DOM::__document, task_name, *static_cast<const std::vector<LQIO::DOM::Entry *>*>(entries), processor );
    } else {
	task = new LQIO::DOM::Task( LQIO::DOM::__document, task_name, scheduling, *static_cast<const std::vector<LQIO::DOM::Entry *>*>(entries), processor );
    }

    /* Task Constructor copies the vector, so... */

    delete static_cast<const std::vector<LQIO::DOM::Entry *>*>(entries);
    
    /* Link in the entity information */

    LQIO::DOM::__document->addTaskEntity(task);
    processor->addTask(task);
    return task;
}
  
void
srvn_store_fanin( const char * src_name, const char * dst_name, void * value )
{
    LQIO::DOM::Task* dst_task = static_cast<LQIO::DOM::Task*>(srvn_get_task( dst_name ));
    if ( dst_task == nullptr ) {
	return;
    } else if ( src_name == dst_name ) {
        LQIO::input_error2( LQIO::ERR_SRC_EQUALS_DST, src_name, dst_name );
	return;
    }
    dst_task->setFanIn( src_name, static_cast<LQIO::DOM::ExternalVariable *>(value) );
}

void
srvn_store_fanout( const char * src_name, const char * dst_name, void * value )
{
    LQIO::DOM::Task* src_task = static_cast<LQIO::DOM::Task*>(srvn_get_task( src_name ));
    if ( src_task == nullptr ) {
	return;
    } else if ( src_name == dst_name ) {
        LQIO::input_error2( LQIO::ERR_SRC_EQUALS_DST, src_name, dst_name );
	return;
    }
    src_task->setFanOut( dst_name, static_cast<LQIO::DOM::ExternalVariable *>(value) );
}

void * 
srvn_add_entry (const char * entry_name, const void * entry_list)
{
    /* This method adds an entry for the current name, with given next entry */

    std::vector<LQIO::DOM::Entry *> * entries = const_cast<std::vector<LQIO::DOM::Entry *>*>(static_cast<const std::vector<LQIO::DOM::Entry *> *>(entry_list));
    LQIO::DOM::Entry* entry = LQIO::DOM::__document->getEntryByName(entry_name);
    if ( entry != nullptr  ) {
	entry->input_error( LQIO::ERR_DUPLICATE_SYMBOL );
    } else {
	if ( !entries ) {
	    entries = new std::vector<LQIO::DOM::Entry *>;
	}
	entry = new LQIO::DOM::Entry(LQIO::DOM::__document, entry_name );
	entries->push_back(entry);
	LQIO::DOM::__document->addEntry(entry);
    }
    return static_cast<void *>(entries);
}
  
void 
srvn_set_model_parameters (const char *model_comment, void * conv_val, void * it_limit, void * print_int, void * underrelax_coeff )
{
    LQIO::DOM::__document->setModelParameters(model_comment, 
					      static_cast<LQIO::DOM::ExternalVariable*>(conv_val), 
					      static_cast<LQIO::DOM::ExternalVariable*>(it_limit), 
					      static_cast<LQIO::DOM::ExternalVariable*>(print_int), 
					      static_cast<LQIO::DOM::ExternalVariable*>(underrelax_coeff), 0);
}

void *
srvn_get_task( const char * task_name )
{
    LQIO::DOM::Task* task = LQIO::DOM::__document->getTaskByName(task_name);
    if ( task == nullptr ) {
	LQIO::input_error2( LQIO::ERR_NOT_DEFINED, task_name );
	/* Define it to suppress additional errors */
	task = new LQIO::DOM::Task(LQIO::DOM::__document, task_name, SCHEDULE_DELAY, std::vector<LQIO::DOM::Entry *>(), nullptr );
	LQIO::DOM::__document->addTaskEntity(task);
    }
    return static_cast<void *>(task);
}


void *
srvn_get_entry( const char * entry_name )
{
    LQIO::DOM::Entry * entry = LQIO::DOM::__document->getEntryByName(entry_name);
    if ( entry == nullptr ) {
	LQIO::input_error2( LQIO::ERR_NOT_DEFINED, entry_name );
	/* Define it to suppress additional errors */
	entry = new LQIO::DOM::Entry(LQIO::DOM::__document, entry_name );
	LQIO::DOM::__document->addEntry(entry);
    }
    return static_cast<void *>(entry);
}

void 
srvn_set_phase_type_flag (void * entry_v, unsigned n_phases, ...)
{
    LQIO::DOM::Entry* entry = static_cast<LQIO::DOM::Entry*>(entry_v);
    if ( !entry ) return; 

    LQIO::DOM::Document::db_check_set_entry(entry, LQIO::DOM::Entry::Type::STANDARD);
    
    /* Push all the times */
    va_list ap;
    va_start(ap, n_phases);
    for (unsigned int i = 1; i <= n_phases; i++) {
	LQIO::DOM::Phase* phase = entry->getPhase(i);
	if ( va_arg(ap, int) == 0 ) {
	    phase->setPhaseTypeFlag(LQIO::DOM::Phase::Type::STOCHASTIC);
	} else {
	    phase->setPhaseTypeFlag(LQIO::DOM::Phase::Type::DETERMINISTIC);
	}
    }
    
    /* Close the argument list */
    va_end(ap);
}

void 
srvn_set_semaphore_flag ( void * entry_v, int set )
{
    LQIO::DOM::Entry* entry = static_cast<LQIO::DOM::Entry*>(entry_v);
    if ( !entry ) return;
    switch ( set ) {
    case 'P': entry->setSemaphoreFlag(LQIO::DOM::Entry::Semaphore::SIGNAL); break;
    case 'V': entry->setSemaphoreFlag(LQIO::DOM::Entry::Semaphore::WAIT); break;
    default: abort();
    }
}

void 
srvn_set_rwlock_flag ( void * entry_v, int set )
{
    LQIO::DOM::Entry* entry = static_cast<LQIO::DOM::Entry*>(entry_v);
    if ( !entry ) return;
    switch ( set ) {
    case 'R': entry->setRWLockFlag(LQIO::DOM::Entry::RWLock::READ_LOCK); break;
    case 'U': entry->setRWLockFlag(LQIO::DOM::Entry::RWLock::READ_UNLOCK); break;
    case 'W': entry->setRWLockFlag(LQIO::DOM::Entry::RWLock::WRITE_LOCK); break;
    case 'X': entry->setRWLockFlag(LQIO::DOM::Entry::RWLock::WRITE_UNLOCK); break;
    default: abort();
    }
}

void 
srvn_store_coeff_of_variation (void * entry_v, unsigned n_phases, ...)
{
    /* Obtain the entry that we will be adding the phase think times to */
    LQIO::DOM::Entry* entry = static_cast<LQIO::DOM::Entry*>(entry_v);
    if ( !entry ) return;

    LQIO::DOM::Document::db_check_set_entry(entry, LQIO::DOM::Entry::Type::STANDARD);

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

    LQIO::DOM::Document::db_check_set_entry(entry);
    
    /* Store the open arrival rate */
    entry->setEntryPriority(new LQIO::DOM::ConstantExternalVariable(arg));
}

void 
srvn_store_open_arrival_rate (void  * entry_v, void * arg )
{
    /* Obtain the entry that we will be adding the phase times to */
    LQIO::DOM::Entry* entry = static_cast<LQIO::DOM::Entry*>(entry_v);
    if ( !entry ) return;
    LQIO::DOM::Document::db_check_set_entry(entry);
    
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
    LQIO::DOM::Document::db_check_set_entry(entry, LQIO::DOM::Entry::Type::STANDARD);
    
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

    LQIO::DOM::Document::db_check_set_entry(entry, LQIO::DOM::Entry::Type::STANDARD);
    
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
    if ( from_entry == nullptr || to_entry == nullptr) {
	return;
    } else if ( from_entry == to_entry ) {
        LQIO::input_error2( LQIO::ERR_SRC_EQUALS_DST, from_entry->getName().c_str(), to_entry->getName().c_str() );
	return;
    }

    LQIO::DOM::Document::db_check_set_entry(from_entry);
    LQIO::DOM::Document::db_check_set_entry(to_entry);
    
    /* Build a variable for the storage of the P(fwd) and set it on the originator */
    LQIO::DOM::Call * call = from_entry->getForwardingToTarget(to_entry);
    if ( call == nullptr ) {
	LQIO::DOM::Call* call = new LQIO::DOM::Call(LQIO::DOM::__document, from_entry, to_entry, static_cast<LQIO::DOM::ExternalVariable *>(prob) );
	from_entry->addForwardingCall(call);
	std::string name = from_entry->getName();
	name += '_';
	name += to_entry->getName();
	call->setName(name);
    } else if (call->getCallType() != LQIO::DOM::Call::Type::NULL_CALL) {
	LQIO::input_error2( LQIO::WRN_MULTIPLE_SPECIFICATION );
    }
}

void 
srvn_store_rnv_data (void * from_entry_v, void * to_entry_v, unsigned n_phases, ...)
{
    /* Obtain the entry that we will be adding the phase times to */
    LQIO::DOM::Entry* from_entry = static_cast<LQIO::DOM::Entry *>(from_entry_v);
    LQIO::DOM::Entry* to_entry = static_cast<LQIO::DOM::Entry *>(to_entry_v);

    if ( from_entry == nullptr || to_entry == nullptr) {
	return;
    } else if ( from_entry == to_entry ) {
        LQIO::input_error2( LQIO::ERR_SRC_EQUALS_DST, from_entry->getName().c_str(), to_entry->getName().c_str() );
	return;
    }
    /* Make sure that this is a standard entry */
    LQIO::DOM::Document::db_check_set_entry(from_entry, LQIO::DOM::Entry::Type::STANDARD);
    LQIO::DOM::Document::db_check_set_entry(to_entry);
    
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
	if (call == nullptr) {
	    call = new LQIO::DOM::Call(LQIO::DOM::__document, LQIO::DOM::Call::Type::RENDEZVOUS, phase, to_entry, ev);
	    phase->addCall( call );
	    std::string name = phase->getName();
	    name += '_';
	    name += to_entry->getName();
	    call->setName(name);
	} else {
	    if (call->getCallType() != LQIO::DOM::Call::Type::NULL_CALL) {
		LQIO::input_error2( LQIO::WRN_MULTIPLE_SPECIFICATION );
	    }
          
	    /* Set the new call type and the new mean */
	    call->setCallType(LQIO::DOM::Call::Type::RENDEZVOUS);
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

    if ( from_entry == nullptr || to_entry == nullptr) {
	return;
    } else if ( from_entry == to_entry ) {
	LQIO::input_error2( LQIO::ERR_SRC_EQUALS_DST, from_entry->getName().c_str(), to_entry->getName().c_str() );
	return;
    }
    /* Make sure that this is a standard entry */
    LQIO::DOM::Document::db_check_set_entry(from_entry, LQIO::DOM::Entry::Type::STANDARD);
    LQIO::DOM::Document::db_check_set_entry(to_entry);
    
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
	if (call == nullptr) {
	    call = new LQIO::DOM::Call(LQIO::DOM::__document,LQIO::DOM::Call::Type::SEND_NO_REPLY, phase, to_entry, ev);
	    phase->addCall( call );
	    std::string name = phase->getName();
	    name += '_';
	    name += to_entry->getName();
	    call->setName(name);
	} else {
	    if (call->getCallType() != LQIO::DOM::Call::Type::NULL_CALL) {
		LQIO::input_error2( LQIO::WRN_MULTIPLE_SPECIFICATION );
	    }
          
	    /* Set the new call type and the new mean */
	    call->setCallType(LQIO::DOM::Call::Type::SEND_NO_REPLY);
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
    LQIO::DOM::Document::db_check_set_entry(entry, LQIO::DOM::Entry::Type::ACTIVITY);
    LQIO::DOM::Task* task = const_cast<LQIO::DOM::Task*>(entry->getTask());
    LQIO::DOM::Activity* activity = task->getActivity(startActivityName, true);
    entry->setStartActivity(activity);
}


void
srvn_set_proc_multiplicity( void * proc_v, void * copies)
{
    LQIO::DOM::Processor * proc = static_cast<LQIO::DOM::Processor *>(proc_v);
    if ( proc == nullptr || copies == nullptr ) return;
    proc->setCopies( static_cast<LQIO::DOM::ExternalVariable *>(copies) );
}


void srvn_set_proc_rate( void * proc_v, void * rate )
{
    LQIO::DOM::Processor * proc = static_cast<LQIO::DOM::Processor *>(proc_v);
    if ( proc == nullptr || rate == nullptr ) return;

    proc->setRate( static_cast<LQIO::DOM::ExternalVariable *>(rate) );
}


void srvn_set_proc_replicas( void * proc_v, void * replicas )
{
    LQIO::DOM::Processor * proc = static_cast<LQIO::DOM::Processor *>(proc_v);
    if ( proc == nullptr || replicas == nullptr ) return;
    proc->setReplicas( static_cast<LQIO::DOM::ExternalVariable *>(replicas) );
}

void
srvn_set_task_group( void * task_v, const char * group_name )
{
    LQIO::DOM::Task * task = static_cast<LQIO::DOM::Task *>(task_v);
    if ( task == nullptr || group_name == nullptr ) return;

    /* Ditto for the group, if specified */
    LQIO::DOM::Group * group = LQIO::DOM::__document->getGroupByName(group_name);
    if ( group == nullptr ) {
	LQIO::input_error2( LQIO::ERR_NOT_DEFINED, group_name );
    } else {
	group->addTask(task);
	task->setGroup(group);
    }
}


void
srvn_set_task_multiplicity( void * task_v, void * copies )
{
    LQIO::DOM::Task * task = static_cast<LQIO::DOM::Task *>(task_v);
    if ( task == nullptr || copies == nullptr ) return;
    if ( schedule_customer( task->getSchedulingType() ) && is_infinity( static_cast<LQIO::DOM::ExternalVariable *>(copies) ) ) {
	LQIO::input_error2( LQIO::ERR_REFERENCE_TASK_IS_INFINITE, task->getName().c_str() );
    } else {
	task->setCopies( static_cast<LQIO::DOM::ExternalVariable *>(copies) );
    }
}

void
srvn_set_task_priority( void * task_v, void * priority )
{
    LQIO::DOM::Task * task = static_cast<LQIO::DOM::Task *>(task_v);
    if ( task == nullptr || priority == nullptr ) return;
    task->setPriority( static_cast<LQIO::DOM::ExternalVariable *>(priority) );
}


void
srvn_set_task_queue_length( void * task_v, void * length )
{
    LQIO::DOM::Task * task = static_cast<LQIO::DOM::Task *>(task_v);
    if ( task == nullptr || length == nullptr ) return;
    task->setQueueLength( static_cast<LQIO::DOM::ExternalVariable *>(length) );
}


void srvn_set_task_replicas( void * task_v, void * replicas )
{
    LQIO::DOM::Task * task = static_cast<LQIO::DOM::Task *>(task_v);
    if ( task == nullptr || replicas == nullptr ) return;
    task->setReplicas( static_cast<LQIO::DOM::ExternalVariable *>(replicas) );
}
    

void
srvn_set_task_think_time( void * task_v, void * time )
{
    LQIO::DOM::Task * task = static_cast<LQIO::DOM::Task *>(task_v);
    if ( task == nullptr || time == nullptr || LQIO::DOM::Common_IO::is_default_value( static_cast<LQIO::DOM::ExternalVariable *>(time), 0.0 ) ) return;
    if ( schedule_customer( task->getSchedulingType() ) ) {
	task->setThinkTime( static_cast<LQIO::DOM::ExternalVariable *>(time) );
    } else {
	LQIO::input_error2( LQIO::ERR_NON_REF_THINK_TIME, task->getName().c_str() );
    }
}


void 
srvn_set_task_tokens( void * task_v, int tokens )
{
    LQIO::DOM::Task * task = static_cast<LQIO::DOM::Task *>(task_v);
    if ( !task ) return;
    if ( tokens == 0 ) {
	dynamic_cast<LQIO::DOM::SemaphoreTask *>(task)->setInitialState(LQIO::DOM::SemaphoreTask::InitialState::EMPTY);
    }
}

/*
 * find an activity.  
 */

void *
srvn_find_activity( void * task, const char * name )
{
    if ( !task ) return nullptr;
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
    if ( !task ) return nullptr;
    return static_cast<LQIO::DOM::Task *>(task)->getActivity(name, true);
}

void
srvn_set_activity_call_name( const void * task, const void * activity, const void * entry, void * call ) 
{
    if ( !task || !activity || !entry || !call ) return;
    std::string name = static_cast<const LQIO::DOM::Task *>(task)->getName();
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
srvn_store_activity_rnv_data( void * activity, void * dst_entry_v, void * calls ) 
{
    LQIO::DOM::Entry* dst_entry = static_cast<LQIO::DOM::Entry *>(dst_entry_v);
    if ( !activity || !dst_entry ) return nullptr;
    LQIO::DOM::Document::db_check_set_entry(dst_entry);
    
    LQIO::DOM::Call* call = new LQIO::DOM::Call(LQIO::DOM::__document, LQIO::DOM::Call::Type::RENDEZVOUS, static_cast<LQIO::DOM::Activity *>(activity), dst_entry,
						static_cast<LQIO::DOM::ExternalVariable *>(calls));
    static_cast<LQIO::DOM::Activity *>(activity)->addCall(call);
    return call;
}
 
void 
srvn_store_activity_service_time ( void * activity, void * service_time ) 
{
    if ( !activity ) return;
    static_cast<LQIO::DOM::Activity *>(activity)->setServiceTime(static_cast<LQIO::DOM::ExternalVariable *>(service_time));
    static_cast<LQIO::DOM::Activity *>(activity)->setIsSpecified(true);
}
 
void *
srvn_store_activity_snr_data ( void * activity, void * dst_entry_v, void * calls ) 
{
    LQIO::DOM::Entry* dst_entry = static_cast<LQIO::DOM::Entry *>(dst_entry_v);
    if ( !activity || !dst_entry ) return nullptr;
    LQIO::DOM::Document::db_check_set_entry(dst_entry);
    
    LQIO::DOM::Call* call = new LQIO::DOM::Call(LQIO::DOM::__document, LQIO::DOM::Call::Type::SEND_NO_REPLY, static_cast<LQIO::DOM::Activity *>(activity), dst_entry,
						static_cast<LQIO::DOM::ExternalVariable *>(calls));
    static_cast<LQIO::DOM::Activity *>(activity)->addCall(call);
    return call;
}
 
void 
srvn_store_activity_think_time ( void * activity, void * think_time )
{
    if ( !activity ) return;
    static_cast<LQIO::DOM::Activity *>(activity)->setThinkTime(static_cast<LQIO::DOM::ExternalVariable *>(think_time));
}

void 
srvn_set_activity_histogram ( void * activity, const double min, const double max, const int n_bins )
{
    if ( !activity ) return;
    static_cast<LQIO::DOM::Activity *>(activity)->setHistogram(new LQIO::DOM::Histogram( LQIO::DOM::__document, LQIO::DOM::Histogram::Type::CONTINUOUS, n_bins, min, max ));
}

void 
srvn_set_activity_phase_type_flag ( void * activity, const int flag ) 
{
    if ( !activity ) return;
    if ( flag == 0 ) {
	static_cast<LQIO::DOM::Activity *>(activity)->setPhaseTypeFlag(LQIO::DOM::Phase::Type::STOCHASTIC);
    } else {
	static_cast<LQIO::DOM::Activity *>(activity)->setPhaseTypeFlag(LQIO::DOM::Phase::Type::DETERMINISTIC);
    }
}

void 
srvn_set_histogram ( void * entry_v, const unsigned phase, const double min, const double max, const int n_bins )
{
    /* Grab the entry */
    LQIO::DOM::Entry* entry = static_cast<LQIO::DOM::Entry *>(entry_v);
    LQIO::DOM::Document::db_check_set_entry(entry);
    if ( !entry ) return;
    
    /* Grab the phase and store the histogram.  DO NOT Create a phase. */
    if ( entry->hasPhase(phase) ) {
	entry->getPhase(phase)->setHistogram(new LQIO::DOM::Histogram(LQIO::DOM::__document, LQIO::DOM::Histogram::Type::CONTINUOUS, n_bins, min, max ));
    } else {
	entry->setHistogramForPhase(phase, new LQIO::DOM::Histogram(LQIO::DOM::__document, LQIO::DOM::Histogram::Type::CONTINUOUS, n_bins, min, max ));
    }
}

void * 
srvn_act_add_reply ( const void * task, const void * entry, void * entry_list )
{
    const LQIO::DOM::Task* domTask = static_cast<const LQIO::DOM::Task*>(task);
    const LQIO::DOM::Entry* domEntry = static_cast<const LQIO::DOM::Entry*>(entry);
    if (task == nullptr || entry == nullptr) return entry_list;
    /* This one is kinda neat since lots happens... */
    /* This method provides an initially NULL entry list and wants it back... */
    std::vector<LQIO::DOM::Entry*>* localEntryList = static_cast<std::vector<LQIO::DOM::Entry*>*>(entry_list);
    if (localEntryList == nullptr) { localEntryList = new std::vector<LQIO::DOM::Entry*>(); }
    
    /* Now we need to find an entry for the given name */
    if ( domEntry->getTask() != task ) {
	domEntry->input_error( LQIO::ERR_WRONG_TASK_FOR_ENTRY, domTask->getName().c_str() );
    } else {
	localEntryList->push_back(const_cast<LQIO::DOM::Entry *>(domEntry));
    }
    
    return localEntryList;
}

void * 
srvn_act_and_fork_list ( const void * aTask, void * activity, void * pActivityList )
{
    if ( !aTask || !activity ) return nullptr;
    return LQIO::DOM::act_and_fork_list( static_cast<const LQIO::DOM::Task *>(aTask), static_cast<LQIO::DOM::Activity *>(activity), 
					 static_cast<LQIO::DOM::ActivityList *>(pActivityList), 0 );
}

void * 
srvn_act_and_join_list ( const void * aTask, void * activity, void * pActivityList, void * quorum_count )
{
    if ( !aTask || !activity ) return nullptr;
    return LQIO::DOM::act_and_join_list( static_cast<const LQIO::DOM::Task *>(aTask), static_cast<LQIO::DOM::Activity *>(activity), 
					 static_cast<LQIO::DOM::ActivityList *>(pActivityList), 
					 static_cast<LQIO::DOM::ExternalVariable *>(quorum_count), 0 );
}

void * 
srvn_act_fork_item ( const void * aTask, void * activity )
{
    if ( !aTask || !activity ) return nullptr;
    return LQIO::DOM::act_fork_item( static_cast<const LQIO::DOM::Task *>(aTask), static_cast<LQIO::DOM::Activity *>(activity), 0 );
}

void * 
srvn_act_join_item ( const void * aTask, void * activity )
{
    if ( !aTask || !activity ) return nullptr;
    return LQIO::DOM::act_join_item( static_cast<const LQIO::DOM::Task *>(aTask), static_cast<LQIO::DOM::Activity *>(activity), 0 );
}

void * 
srvn_act_or_fork_list ( const void * aTask, void * probability, void * activity, void * pActivityList)
{
    if ( !aTask || !activity ) return nullptr;
    return LQIO::DOM::act_or_fork_list( static_cast<const LQIO::DOM::Task *>(aTask), static_cast<LQIO::DOM::Activity *>(activity), 
					static_cast<LQIO::DOM::ActivityList *>(pActivityList), 
					static_cast<LQIO::DOM::ExternalVariable *>(probability), 0 );
}

void * 
srvn_act_or_join_list ( const void * aTask, void * activity, void * pActivityList )
{
    if ( !aTask || !activity ) return nullptr;
    return LQIO::DOM::act_or_join_list( static_cast<const LQIO::DOM::Task *>(aTask), static_cast<LQIO::DOM::Activity *>(activity), 
					      static_cast<LQIO::DOM::ActivityList *>(pActivityList), 0 );
}

void * 
srvn_act_loop_list ( const void * aTask, void * count, void * activity, void * pActivityList )
{
    if ( !aTask || !activity ) return nullptr;
    return LQIO::DOM::act_loop_list( static_cast<const LQIO::DOM::Task *>(aTask), static_cast<LQIO::DOM::Activity *>(activity), 
				     static_cast<LQIO::DOM::ActivityList *>(pActivityList), 
				     static_cast<LQIO::DOM::ExternalVariable *>(count), 0 );
}

void 
srvn_act_add_reply_list ( const void * aTask, void * activity, void * entry_list )
{
    /* Obtain references to the task and the activity from the parameter data */
    if ( !activity ) return;
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
    if (src != nullptr) {
	srcList->setNext(dstList);
    } if (dst != nullptr) {
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
    if ( tolower(*p++) != 'p' || tolower(*p++) != 'r' || tolower(*p++) != 'a' || tolower(*p++) != 'g' || tolower(*p++) != 'm' || tolower(*p++) != 'a' ) return;

    do {
	while ( isspace( *p ) ) ++p;		/* Skip leading whitespace. */
	std::string param;
	std::string value;
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
	LQIO::DOM::__document->addPragma(param,value);
    } while ( *p++ == ',' );
}

void * 
srvn_find_task ( const char * taskName )
{
    /* Return the task with the given name */
    LQIO::DOM::Task* task = LQIO::DOM::__document->getTaskByName(taskName);
    if ( !task ) {
	LQIO::input_error2( LQIO::ERR_NOT_DEFINED, taskName );
    }
    return static_cast<void *>(task);
}

/*
 * Convert an integer into a variable.  Suppress zeros and negative numbers so that 
 * phases and what have you are not created unnecessarily.
 */

void * srvn_int_constant( const int i )
{
    if ( i > 0 ) {
	return new LQIO::DOM::ConstantExternalVariable( static_cast<double>(i) );
    } else {
	return nullptr;
    }
}

/*
 * Convert a real into a variable.  Suppress zeros and negative numbers so that 
 * phases and what have you are not created unnecessarily.
 */

void * srvn_real_constant( const double d )
{
    if ( d > 0. ) {
	return new LQIO::DOM::ConstantExternalVariable( d );
    } else { 
	return nullptr;
    }
}

void * srvn_variable( const char * s )
{
    return LQIO::DOM::__document->db_build_parameter_variable(s,nullptr);
}


double
srvn_get_infinity()
{
    return std::numeric_limits<double>::infinity();
}

namespace LQIO {
    namespace DOM {

	static ActivityList * 
	act_and_fork_list ( const Task * domTask, Activity * activity,
			    ActivityList * activityList, const void * element )
	{
	    /* Make sure we have a task */
	    if (domTask == nullptr || activity == nullptr) { return activityList; }

	    /* Configure the activity */
	    if (activity->isStartActivity()) {
		activity->input_error( ERR_IS_START_ACTIVITY );
	    } else {
		if (!activityList) {
		    activityList = new ActivityList(LQIO::DOM::__document,domTask,ActivityList::Type::AND_FORK);
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
	    if (domTask == nullptr || activity == nullptr) { return activityList; }

	    /* Create the AND join list */
	    if (activityList == nullptr) {
		activityList = new AndJoinActivityList(LQIO::DOM::__document,domTask,quorum_count);
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
	    if (domTask == nullptr || activity == nullptr ) { return nullptr; }
	    ActivityList* activityList = nullptr;

	    /* Configure the activity */
	    activityList = new ActivityList(LQIO::DOM::__document,domTask,ActivityList::Type::FORK);
	    activity->inputFrom(activityList);
	    activityList->add(activity);

	    /* Return the list itself */
	    return activityList;
	}

	static ActivityList * 
	act_join_item ( const Task * domTask, Activity * activity, const void * element  )
	{
	    /* Make sure we have a task */
	    if (domTask == nullptr || activity == nullptr)  { return nullptr; }
	    ActivityList* activityList = nullptr;

	    /* Configure the activity */
	    activityList = new ActivityList(LQIO::DOM::__document,domTask,ActivityList::Type::JOIN);
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
	    if (domTask == nullptr || !activity) { return activityList; }

	    /* Configure the activity */
	    double value = 0.;
	    if ( probability->wasSet() && probability->getValue(value) && (value < 0. || 1. < value) ) {
		input_error2( ERR_INVALID_PROBABILITY, probability );
	    } else {
		if (activity->isStartActivity()) {
		    activity->input_error( ERR_IS_START_ACTIVITY );
		} else {
		    if (activityList == nullptr) {
			activityList = new ActivityList(LQIO::DOM::__document,domTask,ActivityList::Type::OR_FORK);
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
	    if (domTask == nullptr || activity == nullptr) { return activityList; }

	    /* Configure the activity */
	    if (activityList == nullptr) {
		activityList = new ActivityList(LQIO::DOM::__document,domTask,ActivityList::Type::OR_JOIN);
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
	    if (domTask == nullptr || activity == nullptr) { return activityList; }

	    /* Configure the activity */
	    if (activityList == nullptr) {
		activityList = new ActivityList(LQIO::DOM::__document,domTask,ActivityList::Type::REPEAT);
	    }
	    activityList->add(activity, count);
	    activity->inputFrom(activityList);

	    /* Return the list itself */
	    return activityList;
	}
    }
}


bool LQIO::SRVN::load(LQIO::DOM::Document& document, const std::string& input_file_name, bool load_results )
{
    unsigned errorCode = 0;
    if ( !Filename::isFileName( input_file_name ) ) {
	LQIO_in = stdin;
    } else if (!( LQIO_in = fopen( input_file_name.c_str(), "r" ) ) ) {
	std::cerr << LQIO::io_vars.lq_toolname << ": Cannot open input file " << input_file_name << " - " << strerror( errno ) << std::endl;
	return false;
    } 
    int LQIO_in_fd = fileno( LQIO_in );

    struct stat statbuf;
    if ( isatty( LQIO_in_fd ) ) {
	std::cerr << LQIO::io_vars.lq_toolname << ": Input from terminal is not allowed." << std::endl;
	return false;
    } else if ( fstat( LQIO_in_fd, &statbuf ) != 0 ) {
	std::cerr << LQIO::io_vars.lq_toolname << ": Cannot stat " << input_file_name << " - " << strerror( errno ) << std::endl;
	return false;
#if defined(S_ISSOCK)
    } else if ( !S_ISREG(statbuf.st_mode) && !S_ISFIFO(statbuf.st_mode) && !S_ISSOCK(statbuf.st_mode) ) {
#else
    } else if ( !S_ISREG(statbuf.st_mode) && !S_ISFIFO(statbuf.st_mode) ) {
#endif
	std::cerr << LQIO::io_vars.lq_toolname << ": Input from " << input_file_name << " is not allowed." << std::endl;
	return false;
    } 

    LQIO_lineno = 1;

#if HAVE_MMAP
    char * buffer = static_cast<char *>(mmap( 0, statbuf.st_size, PROT_READ, MAP_PRIVATE|MAP_FILE, LQIO_in_fd, 0 ));
    if ( buffer != MAP_FAILED ) {
	yy_buffer_state * yybuf = LQIO__scan_string( buffer );
	try {
	    srvn_start_token = SRVN_INPUT;
	    errorCode = LQIO_parse();
	}
	catch ( const std::domain_error& e ) {
	    std::cerr << LQIO::io_vars.lq_toolname << ": " << e.what() << "." << std::endl;
	    errorCode = 1;
	}
	catch ( const std::runtime_error& e ) {
	    std::cerr << LQIO::io_vars.lq_toolname << ": " << e.what() << "." << std::endl;
	    errorCode = 1;
	}
	LQIO__delete_buffer( yybuf );
	munmap( buffer, statbuf.st_size );
    } else {
#endif
	/* Try the old way (for pipes) */
	try {
	    srvn_start_token = SRVN_INPUT;
	    errorCode = LQIO_parse();
	}
	catch ( const std::runtime_error& e ) {
	    std::cerr << LQIO::io_vars.lq_toolname << ": " << e.what() << "." << std::endl;
	    errorCode = 1;
	}
#if HAVE_MMAP
    }
#endif
    if ( LQIO::io_vars.anError() ) {
	errorCode = 1;
    } else if ( load_results && Filename::isFileName( input_file_name ) ) {
	LQIO::Filename parse_name( input_file_name, "p" );
	try {
	    if ( parse_name.mtimeCmp( input_file_name ) < 0 ) {
		std::cerr << LQIO::io_vars.lq_toolname << ": input file " << input_file_name << " is more recent than " << parse_name() 
			  << " -- results ignored. " << std::endl;
	    } else {
		LQIO::SRVN::loadResults( parse_name() );
	    }
	} 
	catch ( const std::invalid_argument& err ) {
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
    va_list args;
    va_start( args, fmt );
    LQIO::verrprintf( stderr, LQIO::error_severity::ERROR, LQIO::DOM::Document::__input_file_name.c_str(), LQIO_lineno, 0, fmt, args );
    va_end( args );
}


   
/*
 * Print out and error message (and the line number on which it
 * occurred.
 */

void
srvnwarning( const char * fmt, ... )
{
    va_list args;
    va_start( args, fmt );
    LQIO::verrprintf( stderr, LQIO::error_severity::WARNING, LQIO::DOM::Document::__input_file_name.c_str(), LQIO_lineno, 0, fmt, args );
    va_end( args );
}


   
