/*
 *  $Id: srvn_results.cpp 14381 2021-01-19 18:52:02Z greg $
 *
 *  Created by Martin Mroz on 24/02/09.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#if defined(HAVE_CONFIG_H)
#include <config.h>
#endif

#include <sys/stat.h>
#if HAVE_SYS_MMAN_H
#include <sys/types.h>
#include <sys/mman.h>
#endif
#include <cstdarg>
#include <cstring>
#include <errno.h>
#include <algorithm>
#include "dom_document.h"
#include "dom_processor.h"
#include "dom_task.h"
#include "dom_entry.h"
#include "dom_activity.h"
#include "filename.h"
#include "confidence_intervals.h"
#include "srvn_results.h"
#include "srvn_input.h"
#include "glblerr.h"
#include "error.h"

static const char * results_file_name;	/* Filename for the parser's input file */
static unsigned int n_phases = 0;

static void results_error2( unsigned err, ... );

struct yy_buffer_state;

extern "C" {
    extern FILE * resultin;		/* from srvn_gram.y, implicitly */
    int resultparse();
    extern yy_buffer_state * result_scan_string( const char * );
    extern void result_delete_buffer( yy_buffer_state * );
}

/*----------------------------------------------------------------------*/
/*		 	   Called from xxparse.  			*/
/*----------------------------------------------------------------------*/

void set_general(int v, double c, int i, int pr, int ph)
{
    LQIO::DOM::__document->setResultValid( v != 0 )
	.setResultConvergenceValue( c )
	.setResultIterations( i );
    if ( ph < 0 || static_cast<int>(LQIO::DOM::Phase::MAX_PHASE) < ph ) throw std::runtime_error( "set_general" );
    n_phases = ph;
}

void set_variable( const char * variable_name, double value )
{
    LQIO::DOM::SymbolExternalVariable* variable = LQIO::DOM::__document->getSymbolExternalVariable( variable_name );    
    LQX::Program * program = LQIO::DOM::__document->getLQXProgram();
    if ( variable && program ) {
	LQIO::DOM::__document->registerExternalSymbolsWithProgram( program );
	variable->set( value );
    }
}

void add_comment( const char * comment )
{
    LQIO::DOM::__document->setModelComment( new LQIO::DOM::ConstantExternalVariable( comment ) );
}

void add_elapsed_time( double time )
{
    LQIO::DOM::__document->setResultElapsedTime( time );
}
 
void add_user_time( double time )
{
    LQIO::DOM::__document->setResultUserTime( time );
}

void add_system_time( double time )
{
    LQIO::DOM::__document->setResultSysTime( time );
}

void add_max_rss( long max_rss )
{
    LQIO::DOM::__document->setResultMaxRSS( max_rss );
}
    
void add_mva_solver_info( const unsigned int submodels, const unsigned long core, const double step, const double step_squared, const double wait, const double wait_squared, const unsigned int faults )
{
    LQIO::DOM::__document->setMVAStatistics( submodels, core, step, step_squared, wait, wait_squared, faults );
}

/*
 * No-operation.
 */

void
add_output_pragma( const char *str, int len )
{
}

/* -------------------------------------- Processors ----------------------------- */

/*
 * Add the processor data to the processor list.  Total processor utilzation for the case it's not found in the results file. 
 */

void
add_proc (const char *processor_name)
{
    LQIO::DOM::Processor * processor = LQIO::DOM::__document->getProcessorByName( processor_name );
    if ( !processor ) {
	results_error2( LQIO::ERR_NOT_DEFINED, processor_name );
    } else {
	processor->computeResultUtilization();		// Derive from task utilizations */
    }
}


/*
 * Store total processor utilization (not present if there is only one task).
 */

void
add_total_proc( const char * processor_name, double value )
{
    LQIO::DOM::Processor * processor = LQIO::DOM::__document->getProcessorByName( processor_name );
    if ( processor ) {
	processor->setResultUtilization( value );
    }
}

/*
 * Store confidence level info for the processor.
 */

void
add_total_proc_confidence ( const char * processor_name, int conf_level, double value )
{
    if ( conf_level != 95) return;

    LQIO::DOM::Processor * processor = LQIO::DOM::__document->getProcessorByName( processor_name );
    if ( processor ) {
	processor->setResultUtilizationVariance( LQIO::ConfidenceIntervals::invert( value, LQIO::DOM::__document->getResultNumberOfBlocks(), 
										    LQIO::ConfidenceIntervals::CONF_95 ) );
    }
}

/* ------------------------------- Group ------------------------------ */

/*
 * Add the utilziation for the group group_name.
 */

void
add_group_util( const char * group_name, double value )
{
    LQIO::DOM::Group * group = LQIO::DOM::__document->getGroupByName( group_name );
    if ( !group ) {
	results_error2( LQIO::ERR_NOT_DEFINED, group_name );
    } else {
	group->setResultUtilization( value );
    }
}


/*
 * Store confidence interval data (as variance) for the group.
 */

void add_group_util_conf( const char * group_name, int conf_level, double value )
{
    if ( conf_level != 95) return;

    LQIO::DOM::Group * group = LQIO::DOM::__document->getGroupByName( group_name );
    if ( !group ) {
	results_error2( LQIO::ERR_NOT_DEFINED, group_name );
    } else {
	group->setResultUtilizationVariance( LQIO::ConfidenceIntervals::invert( value, LQIO::DOM::__document->getResultNumberOfBlocks(), 
										    LQIO::ConfidenceIntervals::CONF_95 ) );
    }
}

/* ------------------------------- Task ------------------------------- */

/*
 * Add the throughput and utilization per task to the list.
 */

void
add_thpt_ut (const char *task_name)
{
    LQIO::DOM::Task * task = LQIO::DOM::__document->getTaskByName( task_name );
    if ( !task ) {
	results_error2( LQIO::ERR_NOT_DEFINED, task_name );
    } else {
	task->computeResultThroughput();
	task->computeResultUtilization();
    }
}


/*
 * Add the throughput and utilization per task to the list.
 */

/*ARGSUSED*/
void
total_thpt_ut ( const char * task_name, double tput, double * utilization, double tot_util )
{
    LQIO::DOM::Task * task = LQIO::DOM::__document->getTaskByName( task_name );
    if ( task ) {
	task->setResultThroughput( tput )
	    .setResultUtilization( tot_util )
	    .setResultPhaseUtilizations( n_phases, utilization );
    }
}



/*
 * Add the throughput and utilization per task to the list.
 */

void
total_thpt_ut_confidence ( const char * task_name, int conf_level, double tput, double * utilization, double tot_util )
{
    if ( conf_level != 95) return;
    LQIO::DOM::Task * task = LQIO::DOM::__document->getTaskByName( task_name );
    if ( task ) {
	task->setResultThroughputVariance( LQIO::ConfidenceIntervals::invert( tput, LQIO::DOM::__document->getResultNumberOfBlocks(), 
									      LQIO::ConfidenceIntervals::CONF_95  ) )
	    .setResultUtilizationVariance( LQIO::ConfidenceIntervals::invert( tot_util, LQIO::DOM::__document->getResultNumberOfBlocks(), 
									      LQIO::ConfidenceIntervals::CONF_95  ) );
	/* Do this the hard way */
	double variance[LQIO::DOM::Phase::MAX_PHASE];
	for ( unsigned p = 0; p < n_phases; ++p ) {
	    variance[p] = LQIO::ConfidenceIntervals::invert( utilization[p], LQIO::DOM::__document->getResultNumberOfBlocks(), 
							     LQIO::ConfidenceIntervals::CONF_95  );
	}
	task->setResultPhaseUtilizationVariances( n_phases, variance );
    }
}


/*
 * Called when all of the data for a task has been collected.  If there is no
 * total field, this will total it up.  If copies is set, it overrides the value
 * (possibily a variable) unless the value is infinity (see bug 246).
 */

void
add_proc_task (const char *task_name, unsigned int copies )
{
    LQIO::DOM::Task * task = LQIO::DOM::__document->getTaskByName( task_name );
    if ( task ) {
	if ( copies > 0 && !task->isInfinite() ) {
	    task->setCopies( new LQIO::DOM::ConstantExternalVariable( copies ) );		/* Override */
	}
	task->computeResultProcessorUtilization();
    }
}

void
add_task_proc (const char *task_name, double util )
{
    LQIO::DOM::Task * task = LQIO::DOM::__document->getTaskByName( task_name );
    if ( task ) {
	task->setResultProcessorUtilization( util );
    }
}


void
add_task_proc_confidence (const char *task_name, int conf_level, double util )
{
    if ( conf_level != 95) return;
    LQIO::DOM::Task * task = LQIO::DOM::__document->getTaskByName( task_name );
    if ( task ) {
	task->setResultProcessorUtilizationVariance( LQIO::ConfidenceIntervals::invert( util, LQIO::DOM::__document->getResultNumberOfBlocks(), 
											LQIO::ConfidenceIntervals::CONF_95  ) );
    }
}

/*+ BUG_164 */
void
add_holding_time( const char * task_name, const char * acquire, const char * release, double time, double variance, double utilization )
{
    LQIO::DOM::SemaphoreTask * task = dynamic_cast<LQIO::DOM::SemaphoreTask *>(LQIO::DOM::__document->getTaskByName( task_name ));
    if ( task ) {
	task->setResultHoldingTime( time );
	task->setResultVarianceHoldingTime( variance );
	task->setResultHoldingUtilization( utilization );
    }
}

void
add_holding_time_confidence( const char * task_name, const char * acquire, const char * release, int level, double time, double variance, double utilization )
{
    if ( level != 95) return;
    LQIO::DOM::SemaphoreTask * task = dynamic_cast<LQIO::DOM::SemaphoreTask *>(LQIO::DOM::__document->getTaskByName( task_name ));
    if ( task ) {
	task->setResultHoldingTimeVariance( LQIO::ConfidenceIntervals::invert( time, LQIO::DOM::__document->getResultNumberOfBlocks(), 
									       LQIO::ConfidenceIntervals::CONF_95 ) );
	task->setResultVarianceHoldingTimeVariance( LQIO::ConfidenceIntervals::invert( variance, LQIO::DOM::__document->getResultNumberOfBlocks(), 
										       LQIO::ConfidenceIntervals::CONF_95 ) );
	task->setResultHoldingUtilizationVariance( LQIO::ConfidenceIntervals::invert( utilization, LQIO::DOM::__document->getResultNumberOfBlocks(), 
										      LQIO::ConfidenceIntervals::CONF_95 ) );
    }
}
/*- BUG_164 */

/*+ RWLOCK */

void
add_reader_holding_time( const char * task_name, const char * lock, const char * unlock, 
			 double blocked_time, double blocked_variance, double hold_time, double hold_variance,double utilization )
{
    LQIO::DOM::RWLockTask * task = dynamic_cast<LQIO::DOM::RWLockTask *>(LQIO::DOM::__document->getTaskByName( task_name ));
    if ( task ) {
	
	task->setResultReaderBlockedTime( blocked_time );
	task->setResultVarianceReaderBlockedTime(blocked_variance ) ;
	task->setResultReaderHoldingTime(hold_time ) ;
	task->setResultVarianceReaderHoldingTime(hold_variance) ;
	task->setResultReaderHoldingUtilization( utilization );
    }
}


void
add_writer_holding_time( const char * task_name, const char * lock, const char * unlock, 
			 double blocked_time, double blocked_variance, double hold_time, double hold_variance,double utilization )
{
    LQIO::DOM::RWLockTask * task = dynamic_cast<LQIO::DOM::RWLockTask *>(LQIO::DOM::__document->getTaskByName( task_name ));
    if ( task ) {
	
	task->setResultWriterBlockedTime( blocked_time );
	task->setResultVarianceWriterBlockedTime(blocked_variance ) ;
	task->setResultWriterHoldingTime(hold_time ) ;
	task->setResultVarianceWriterHoldingTime(hold_variance) ;
	task->setResultWriterHoldingUtilization( utilization );
    }
}

void
add_reader_holding_time_confidence( const char * task_name, const char * lock, const char * unlock, int level, 
				    double blocked_time, double blocked_variance, double hold_time, double hold_variance, double utilization )
{
    if ( level != 95) return;
    LQIO::DOM::RWLockTask * task = dynamic_cast<LQIO::DOM::RWLockTask *>(LQIO::DOM::__document->getTaskByName( task_name ));

    if ( task ) {

	task->setResultReaderBlockedTimeVariance( LQIO::ConfidenceIntervals::invert( blocked_time, LQIO::DOM::__document->getResultNumberOfBlocks(), 
										     LQIO::ConfidenceIntervals::CONF_95 ) );
	task->setResultVarianceReaderBlockedTimeVariance( LQIO::ConfidenceIntervals::invert( blocked_variance, LQIO::DOM::__document->getResultNumberOfBlocks(), 
											     LQIO::ConfidenceIntervals::CONF_95 ) );
	task->setResultReaderHoldingTimeVariance( LQIO::ConfidenceIntervals::invert( hold_time, LQIO::DOM::__document->getResultNumberOfBlocks(), 
										     LQIO::ConfidenceIntervals::CONF_95 ) );
	task->setResultVarianceReaderHoldingTimeVariance( LQIO::ConfidenceIntervals::invert( hold_variance, LQIO::DOM::__document->getResultNumberOfBlocks(), 
											     LQIO::ConfidenceIntervals::CONF_95 ) );
	task->setResultReaderHoldingUtilizationVariance( LQIO::ConfidenceIntervals::invert( utilization, LQIO::DOM::__document->getResultNumberOfBlocks(), 
											    LQIO::ConfidenceIntervals::CONF_95 ) );
    }
}

void
add_writer_holding_time_confidence( const char * task_name, const char * lock, const char * unlock, int level, 
				    double blocked_time, double blocked_variance, double hold_time, double hold_variance, double utilization )
{
    if ( level != 95) return;
    LQIO::DOM::RWLockTask * task = dynamic_cast<LQIO::DOM::RWLockTask *>(LQIO::DOM::__document->getTaskByName( task_name ));

    if ( task ) {

	task->setResultWriterBlockedTimeVariance( LQIO::ConfidenceIntervals::invert( blocked_time, LQIO::DOM::__document->getResultNumberOfBlocks(), 
										     LQIO::ConfidenceIntervals::CONF_95 ) );
	task->setResultVarianceWriterBlockedTimeVariance( LQIO::ConfidenceIntervals::invert( blocked_variance, LQIO::DOM::__document->getResultNumberOfBlocks(), 
											     LQIO::ConfidenceIntervals::CONF_95 ) );
	task->setResultWriterHoldingTimeVariance( LQIO::ConfidenceIntervals::invert( hold_time, LQIO::DOM::__document->getResultNumberOfBlocks(), 
										     LQIO::ConfidenceIntervals::CONF_95 ) );
	task->setResultVarianceWriterHoldingTimeVariance( LQIO::ConfidenceIntervals::invert( hold_variance, LQIO::DOM::__document->getResultNumberOfBlocks(), 
											     LQIO::ConfidenceIntervals::CONF_95 ) );
	task->setResultWriterHoldingUtilizationVariance( LQIO::ConfidenceIntervals::invert( utilization, LQIO::DOM::__document->getResultNumberOfBlocks(), 
											    LQIO::ConfidenceIntervals::CONF_95 ) );
    }
}

/*- RWLOCK */

/* ------------------------------- Entry ------------------------------ */

/*
 * Add a bound datum to the list.
 */

void
add_bound (const char *entry_name, double lower, double upper )
{
    LQIO::DOM::Entry * entry = LQIO::DOM::__document->getEntryByName( entry_name );

    if ( entry ) {
	entry->setResultThroughputBound( upper );
    }
}


/*
 * Execution time.
 */

void
add_service (const char *entry_name, double *time)
{
    LQIO::DOM::Entry * entry = LQIO::DOM::__document->getEntryByName( entry_name );
    if ( entry ) {
	for ( unsigned p = 1; p <= LQIO::DOM::Phase::MAX_PHASE; ++p ) {
	    entry->setResultPhasePServiceTime( p, time[p-1] );
	}
    }
}


void
add_service_confidence( const char *entry_name, int conf_level, double * time )
{
    if ( conf_level != 95) return;

    LQIO::DOM::Entry * entry = LQIO::DOM::__document->getEntryByName( entry_name );
    if ( entry ) {
	for ( unsigned p = 1; p <= LQIO::DOM::Phase::MAX_PHASE; ++p ) {
	    entry->setResultPhasePServiceTimeVariance( p, LQIO::ConfidenceIntervals::invert( time[p-1], LQIO::DOM::__document->getResultNumberOfBlocks(), 
											     LQIO::ConfidenceIntervals::CONF_95 ) );
	}
    }
}


/*
 * Variance
 */

void
add_variance (const char *entry_name, double *time)
{
    LQIO::DOM::Entry * entry = LQIO::DOM::__document->getEntryByName( entry_name );
    if ( entry ) {
	for ( unsigned p = 1; p <= LQIO::DOM::Phase::MAX_PHASE; ++p ) {
	    entry->setResultPhasePVarianceServiceTime( p, time[p-1] );
	}
    }
}


void
add_variance_confidence( const char * entry_name, int conf_level, double * time )
{
    if ( conf_level != 95) return;

    LQIO::DOM::Entry * entry = LQIO::DOM::__document->getEntryByName( entry_name );
    if ( entry ) {
	for ( unsigned p = 1; p <= LQIO::DOM::Phase::MAX_PHASE; ++p ) {
	    entry->setResultPhasePVarianceServiceTimeVariance( p, LQIO::ConfidenceIntervals::invert( time[p-1], LQIO::DOM::__document->getResultNumberOfBlocks(), 
												     LQIO::ConfidenceIntervals::CONF_95 ) );
	}
    }
}


/*
 * Service Time exceeded.
 */

void
add_service_exceeded (const char *entry_name, double *time)
{
//    LQIO::DOM::Entry * entry = LQIO::DOM::__document->getEntryByName( entry_name );
}

void
add_service_exceeded_confidence( const char * entry_name, int conf_level, double * time )
{
//    LQIO::DOM::Entry * entry = LQIO::DOM::__document->getEntryByName( entry_name );
}



/*
 * Store the statistics collected for a histogram.
 */

void
add_histogram_statistics( const char * entry_name, const unsigned phase, const double mean, const double stddev, const double skew, const double kurtosis )
{
//    LQIO::DOM::Entry * entry = LQIO::DOM::__document->getEntryByName( entry_name );
}

void 
add_histogram_bin( const char * entry_name, const unsigned phase, const double begin, const double end, const double prob, const double conf95, const double conf99 )
{
//    LQIO::DOM::Entry * entry = LQIO::DOM::__document->getEntryByName( entry_name );
}



/*
 * Entry related throughput, utilization.
 */

void
add_entry_thpt_ut (const char *entry_name, double tput, double *util, double total_util )
{
    LQIO::DOM::Entry * entry = LQIO::DOM::__document->getEntryByName( entry_name );
    if ( entry ) {
	entry->setResultThroughput( tput )
	    .setResultUtilization( total_util );
    
	for ( unsigned p = 1; p <= LQIO::DOM::Phase::MAX_PHASE; ++p ) {
	    entry->setResultPhasePUtilization( p, util[p-1] );
	}
    }
}



void
add_entry_thpt_ut_confidence (const char * entry_name, int conf_level, double tput, double *util, double total_util )
{
    if ( conf_level != 95 ) return;

    LQIO::DOM::Entry * entry = LQIO::DOM::__document->getEntryByName( entry_name );
    if ( entry ) {
	entry->setResultThroughputVariance( LQIO::ConfidenceIntervals::invert( tput, LQIO::DOM::__document->getResultNumberOfBlocks(), 
									       LQIO::ConfidenceIntervals::CONF_95 ) )
	    .setResultUtilizationVariance( LQIO::ConfidenceIntervals::invert( total_util, LQIO::DOM::__document->getResultNumberOfBlocks(), 
									      LQIO::ConfidenceIntervals::CONF_95 ) );
    
	for ( unsigned p = 1; p <= LQIO::DOM::Phase::MAX_PHASE; ++p ) {
	    entry->setResultPhasePUtilizationVariance( p, LQIO::ConfidenceIntervals::invert( util[p-1], LQIO::DOM::__document->getResultNumberOfBlocks(), 
											     LQIO::ConfidenceIntervals::CONF_95 ) );
	}
    }
}


/*
 *  Open arrival data.
 */

void
add_open_arriv (const char *task, const char *entry_name, double arrival, double waiting)
{
    LQIO::DOM::Entry * entry = LQIO::DOM::__document->getEntryByName( entry_name );

    if ( entry ) {
	entry->setResultWaitingTime( waiting );
    }
}



void
add_open_arriv_confidence( const char *task, const char *entry_name, int conf_level, double waiting )
{
    if ( conf_level != 95) return;

    LQIO::DOM::Entry * entry = LQIO::DOM::__document->getEntryByName( entry_name );

    if ( entry ) {
	entry->setResultWaitingTimeVariance( LQIO::ConfidenceIntervals::invert( waiting, LQIO::DOM::__document->getResultNumberOfBlocks(), 
										 LQIO::ConfidenceIntervals::CONF_95 ) );
    }
}


/*
 * Add the processor entry information to a temporary list.
 */

void
add_entry_proc (const char *entry_name, double utilization, double *waiting)
{
    LQIO::DOM::Entry * entry = LQIO::DOM::__document->getEntryByName( entry_name );

    if ( entry ) {
	entry->setResultProcessorUtilization( utilization );
	for ( unsigned p = 1; p <= LQIO::DOM::Phase::MAX_PHASE; ++p ) {
	    entry->setResultPhasePProcessorWaiting( p, waiting[p-1] );
	}
    }
}


/*
 * Store confidence level info for waiting time for processor for
 * the particular task.
 */

void
add_entry_proc_confidence( const char *entry_name, int conf_level, double utilization, double *waiting )
{
    if ( conf_level != 95) return;

    LQIO::DOM::Entry * entry = LQIO::DOM::__document->getEntryByName( entry_name );

    if ( entry ) {
	entry->setResultProcessorUtilizationVariance( LQIO::ConfidenceIntervals::invert( utilization, LQIO::DOM::__document->getResultNumberOfBlocks(), 
											 LQIO::ConfidenceIntervals::CONF_95 ) );
	for ( unsigned p = 1; p <= LQIO::DOM::Phase::MAX_PHASE; ++p ) {
	    entry->setResultPhasePProcessorWaitingVariance( p, LQIO::ConfidenceIntervals::invert( waiting[p-1], LQIO::DOM::__document->getResultNumberOfBlocks(), 
												  LQIO::ConfidenceIntervals::CONF_95 ) );
	}
    }
}

/* ------------------------------- Call ------------------------------- */

/*
 * Add a waiting time datum to the list of waiting times.
 */

void
add_waiting (const char *from, const char *to, double *delay)
{
    LQIO::DOM::Entry * from_entry = LQIO::DOM::__document->getEntryByName( from );
    LQIO::DOM::Entry * to_entry   = LQIO::DOM::__document->getEntryByName( to );

    if ( from_entry && to_entry ) {
	for ( unsigned p = 1; p <= from_entry->getMaximumPhase(); ++p ) {
	    LQIO::DOM::Call* call = from_entry->getCallToTarget(to_entry, p);
	    if ( call ) {
		call->setResultWaitingTime( delay[p-1] );
	    }
	}
    }
}


void
add_waiting_confidence(const char * from, const char * to, int conf_level, double *delay )
{
    if ( conf_level != 95) return;

    LQIO::DOM::Entry * from_entry = LQIO::DOM::__document->getEntryByName( from );
    LQIO::DOM::Entry * to_entry   = LQIO::DOM::__document->getEntryByName( to );

    if ( from_entry && to_entry ) {
	for ( unsigned p = 1; p <= from_entry->getMaximumPhase(); ++p ) {
	    LQIO::DOM::Call* call = from_entry->getCallToTarget(to_entry, p);
	    if (call) {
		call->setResultWaitingTimeVariance( LQIO::ConfidenceIntervals::invert( delay[p-1], LQIO::DOM::__document->getResultNumberOfBlocks(), 
										       LQIO::ConfidenceIntervals::CONF_95 )  );
	    }
	}
    }
}


void add_fwd_waiting(const char *to, const char *from, double delay)
{
    LQIO::DOM::Entry * from_entry = LQIO::DOM::__document->getEntryByName( from );
    LQIO::DOM::Entry * to_entry   = LQIO::DOM::__document->getEntryByName( to );

    if ( from_entry && to_entry ) {
	LQIO::DOM::Call* call = from_entry->getForwardingToTarget(to_entry);
	if ( call ) {
	    call->setResultWaitingTime( delay );
	}
    }
}


void add_fwd_waiting_confidence(const char *to, const char *from, int conf_level, double delay)
{
    LQIO::DOM::Entry * from_entry = LQIO::DOM::__document->getEntryByName( from );
    LQIO::DOM::Entry * to_entry   = LQIO::DOM::__document->getEntryByName( to );

    if ( from_entry && to_entry ) {
	LQIO::DOM::Call* call = from_entry->getForwardingToTarget(to_entry);
	if ( call ) {
		call->setResultWaitingTimeVariance( LQIO::ConfidenceIntervals::invert( delay, LQIO::DOM::__document->getResultNumberOfBlocks(), 
										       LQIO::ConfidenceIntervals::CONF_95 )  );
	}
    }
}


/*
 * Just store it like above...  the .in file will generate the proper arc type.
 */

void
add_snr_waiting(const char *from, const char *to, double *delay) 
{
    add_waiting( from, to, delay );
}



void
add_snr_waiting_confidence(const char * from, const char * to, int conf_level, double *delay )
{
    add_waiting_confidence( from, to, conf_level, delay );
}


/*
 * Add a waiting time datum to the list of waiting times.
 */

void
add_wait_variance (const char *from, const char *to, double *delay)
{
    LQIO::DOM::Entry * from_entry = LQIO::DOM::__document->getEntryByName( from );
    LQIO::DOM::Entry * to_entry   = LQIO::DOM::__document->getEntryByName( to );

    if ( from_entry && to_entry ) {
	for ( unsigned p = 1; p <= from_entry->getMaximumPhase(); ++p ) {
	    LQIO::DOM::Call* call = from_entry->getCallToTarget(to_entry, p);
	    if (call) {
		call->setResultVarianceWaitingTime( delay[p-1] );
	    }
	}
    }
}


/*
 * Store confidence interval data.
 */


void
add_wait_variance_confidence(const char *from, const char *to, int conf_level, double *delay )
{
    if ( conf_level != 95) return;

    LQIO::DOM::Entry * from_entry = LQIO::DOM::__document->getEntryByName( from );
    LQIO::DOM::Entry * to_entry   = LQIO::DOM::__document->getEntryByName( to );

    if ( from_entry && to_entry ) {
	for ( unsigned p = 1; p <= from_entry->getMaximumPhase(); ++p ) {
	    LQIO::DOM::Call* call = from_entry->getCallToTarget(to_entry, p);
	    if (call) {
		call->setResultVarianceWaitingTimeVariance( LQIO::ConfidenceIntervals::invert( delay[p-1], LQIO::DOM::__document->getResultNumberOfBlocks(), 
											       LQIO::ConfidenceIntervals::CONF_95 )  );
	    }
	}
    }
}


void add_fwd_wait_variance(const char *to, const char * from, double delay)
{
    LQIO::DOM::Entry * from_entry = LQIO::DOM::__document->getEntryByName( from );
    LQIO::DOM::Entry * to_entry   = LQIO::DOM::__document->getEntryByName( to );

    if ( from_entry && to_entry ) {
	for ( unsigned p = 1; p <= from_entry->getMaximumPhase(); ++p ) {
	    LQIO::DOM::Call* call = from_entry->getForwardingToTarget(to_entry);
	    if (call) {
		call->setResultVarianceWaitingTime( delay );
	    }
	}
    }
}

void add_fwd_wait_variance_confidence(const char *to, const char * from, int conf_level, double delay)
{
    LQIO::DOM::Entry * from_entry = LQIO::DOM::__document->getEntryByName( from );
    LQIO::DOM::Entry * to_entry   = LQIO::DOM::__document->getEntryByName( to );

    if ( from_entry && to_entry ) {
	for ( unsigned p = 1; p <= from_entry->getMaximumPhase(); ++p ) {
	    LQIO::DOM::Call* call = from_entry->getForwardingToTarget(to_entry);
	    if (call) {
		call->setResultVarianceWaitingTimeVariance( LQIO::ConfidenceIntervals::invert( delay, LQIO::DOM::__document->getResultNumberOfBlocks(), 
											       LQIO::ConfidenceIntervals::CONF_95 )  );
	    }
	}
    }
}


/*
 * Add a waiting time datum to the list of waiting times.
 */

void
add_snr_wait_variance (const char *from, const char *to, double *delay)
{
    add_wait_variance (from, to, delay);
}

void 
add_snr_wait_variance_confidence (const char *from, const char *to, int conf_level, double *delay)
{
    add_wait_variance_confidence (from, to, conf_level, delay);
}


/*
 * Add a drop probability.
 */

void
add_drop_probability (const char *from, const char *to, double *prob)
{
    LQIO::DOM::Entry * from_entry = LQIO::DOM::__document->getEntryByName( from );
    LQIO::DOM::Entry * to_entry   = LQIO::DOM::__document->getEntryByName( to );

    if ( from_entry && to_entry ) {
	for ( unsigned p = 1; p <= from_entry->getMaximumPhase(); ++p ) {
	    LQIO::DOM::Call* call = from_entry->getCallToTarget(to_entry, p);
	    if (call) {
		call->setResultDropProbability( prob[p-1] );
	    }
	}
    }
}


/*
 * Store confidence interval data.
 *	**CAUTION**
 * this function is called before add_waiting, so the values
 * have to be stored in global variables.
 */

void
add_drop_probability_confidence(const char *from, const char *to, int conf_level, double *prob )
{
    if ( conf_level != 95) return;

    LQIO::DOM::Entry * from_entry = LQIO::DOM::__document->getEntryByName( from );
    LQIO::DOM::Entry * to_entry   = LQIO::DOM::__document->getEntryByName( to );

    if ( from_entry && to_entry ) {
	for ( unsigned p = 1; p <= from_entry->getMaximumPhase(); ++p ) {
	    LQIO::DOM::Call* call = from_entry->getCallToTarget(to_entry, p);
	    if (call) {
		call->setResultDropProbabilityVariance( LQIO::ConfidenceIntervals::invert( prob[p-1], LQIO::DOM::__document->getResultNumberOfBlocks(), 
											   LQIO::ConfidenceIntervals::CONF_95 )  );
	    }
	}
    }
}


/* NOP */
void
add_overtaking ( const char * e1, const char * e2, const char * e3, const char * e4, int p, double *ot )
{
}
/* ---------------------------- Activities ---------------------------- */

static LQIO::DOM::Activity *
srvn_find_activity( const char * task_name, const char * activity_name )
{
    const LQIO::DOM::Task * task = LQIO::DOM::__document->getTaskByName( task_name );
    if ( !task ) {
	results_error2( LQIO::ERR_NOT_DEFINED, task_name );
	return nullptr;
    } else {
	LQIO::DOM::Activity* activity = task->getActivity(activity_name);
	if ( !activity ) {
	    results_error2( LQIO::ERR_NOT_DEFINED, activity_name );
	    return nullptr;
	}
	return activity;
    }
}


void
add_act_service (const char * task_name, const char *activity_name, double *time)
{
    LQIO::DOM::Activity * activity = srvn_find_activity( task_name, activity_name );
    if ( activity ) {
	activity->setResultServiceTime( time[0] );
    }
}

void
add_act_service_confidence (const char * task_name, const char *activity_name, int conf_level, double *time)
{
    if ( conf_level != 95) return;

    LQIO::DOM::Activity * activity = srvn_find_activity( task_name, activity_name );
    if ( activity ) {
	activity->setResultServiceTimeVariance( LQIO::ConfidenceIntervals::invert( time[0], LQIO::DOM::__document->getResultNumberOfBlocks(), 
										   LQIO::ConfidenceIntervals::CONF_95 ) );
    }
}

void
add_act_variance (const char * task_name, const char *activity_name, double *time)
{
    LQIO::DOM::Activity * activity = srvn_find_activity( task_name, activity_name );
    if ( activity ) {
	activity->setResultVarianceServiceTime( time[0] );
    }
}

void
add_act_variance_confidence (const char * task_name, const char *activity_name, int conf_level, double *time)
{
    if ( conf_level != 95) return;

    LQIO::DOM::Activity * activity = srvn_find_activity( task_name, activity_name );
    if ( activity ) {
	activity->setResultVarianceServiceTimeVariance( LQIO::ConfidenceIntervals::invert( time[0], LQIO::DOM::__document->getResultNumberOfBlocks(), 
											   LQIO::ConfidenceIntervals::CONF_95 ) );
    }
}

void
add_act_thpt_ut ( const char * task_name, const char * activity_name, double throughput, double *utilization)
{
    LQIO::DOM::Activity * activity = srvn_find_activity( task_name, activity_name );
    if ( activity ) {
	activity->setResultThroughput( throughput )
	    .setResultUtilization( utilization[0] );
    }
}

void
add_act_thpt_ut_confidence ( const char * task_name, const char * activity_name, int conf_level, double throughput, double *utilization)
{
    if ( conf_level != 95 ) return;

    LQIO::DOM::Activity * activity = srvn_find_activity( task_name, activity_name );
    if ( activity ) {
	activity->setResultThroughputVariance( LQIO::ConfidenceIntervals::invert( throughput, LQIO::DOM::__document->getResultNumberOfBlocks(), 
										  LQIO::ConfidenceIntervals::CONF_95 ) )
	    .setResultUtilizationVariance( LQIO::ConfidenceIntervals::invert( utilization[0], LQIO::DOM::__document->getResultNumberOfBlocks(), 
									      LQIO::ConfidenceIntervals::CONF_95 ) );
    }
}

/*
 * Add the processor entry information to a temporary list.
 * Done by the entry.
 */

void
add_act_proc ( const char * task_name, const char *activity_name, double utilization, double *waiting)
{
    LQIO::DOM::Activity * activity = srvn_find_activity( task_name, activity_name );
    if ( activity ) {
	activity->setResultUtilization( utilization )
	    .setResultProcessorWaiting( waiting[0] );
    }
}



/*
 * Store confidence level info for waiting time for processor for
 * the particular task.
 */

void
add_act_proc_confidence ( const char * task_name, const char *activity_name, int conf_level, double utilization, double *waiting)
{
    if ( conf_level != 95) return;

    LQIO::DOM::Activity * activity = srvn_find_activity( task_name, activity_name );
    if ( activity ) {
	activity->setResultUtilizationVariance( LQIO::ConfidenceIntervals::invert( utilization, LQIO::DOM::__document->getResultNumberOfBlocks(), 
										   LQIO::ConfidenceIntervals::CONF_95 ) )
	    .setResultProcessorWaitingVariance( LQIO::ConfidenceIntervals::invert( waiting[0], LQIO::DOM::__document->getResultNumberOfBlocks(), 
										   LQIO::ConfidenceIntervals::CONF_95 ) );
    }
}

void
add_act_service_exceeded (const char * task_name, const char *activity_name, double *time)
{
    LQIO::DOM::Activity * activity = srvn_find_activity( task_name, activity_name );
    if ( activity ) {
    }
}

void
add_act_service_exceeded_confidence (const char * task_name, const char *activity_name, int conf_level, double *time)
{
    LQIO::DOM::Activity * activity = srvn_find_activity( task_name, activity_name );
    if ( activity ) {
    }
}



/*
 * Add histogram data
 */

void
add_act_histogram_statistics( const char * task_name, const char * activity_name, const double mean, const double stddev, const double skew, const double kurtosis )
{
    LQIO::DOM::Activity * activity = srvn_find_activity( task_name, activity_name );
    if ( activity ) {
    }
}


void
add_act_histogram_bin( const char * task_name, const char * activity_name, const double begin, const double end, const double prob, const double conf95, const double conf99 )
{
    LQIO::DOM::Activity * activity = srvn_find_activity( task_name, activity_name );
    if ( activity ) {
    }
}

/* -------------------------- Activity Call --------------------------- */

/* These function are used to place data into the data structures. */

void 
add_act_waiting (const char * task_name, const char *from, const char *to, double *delay)
{
    LQIO::DOM::Activity * from_activity = srvn_find_activity( task_name, from );
    LQIO::DOM::Entry * to_entry = LQIO::DOM::__document->getEntryByName( to );
    if ( from_activity && to_entry ) {
	LQIO::DOM::Call * call = from_activity->getCallToTarget( to_entry );
	if ( call ) {
	    call->setResultWaitingTime( delay[0] );
	}
    }
}

/* These function are used to place data into the data structures. */

void 
add_act_waiting_confidence (const char * task_name, const char *from, const char *to, int conf_level, double *delay)
{
    if ( conf_level != 95) return;

    LQIO::DOM::Activity * from_activity = srvn_find_activity( task_name, from );
    LQIO::DOM::Entry * to_entry = LQIO::DOM::__document->getEntryByName( to );
    if ( from_activity && to_entry ) {
	LQIO::DOM::Call * call = from_activity->getCallToTarget( to_entry );
	if ( call ) {
	    call->setResultWaitingTimeVariance( LQIO::ConfidenceIntervals::invert(delay[0], LQIO::DOM::__document->getResultNumberOfBlocks(), 
										  LQIO::ConfidenceIntervals::CONF_95 )  );
	}
    }
}


void 
add_act_snr_waiting (const char * task_name, const char *from, const char *to, double *delay)
{
    add_act_waiting(task_name, from, to, delay);
}


void 
add_act_snr_waiting_confidence (const char * task_name, const char *from, const char *to, int conf_level, double *delay)
{
    add_act_waiting_confidence(task_name, from, to, conf_level, delay);
}


void 
add_act_wait_variance (const char * task_name, const char *from, const char * to, double *delay)
{
    LQIO::DOM::Activity * from_activity = srvn_find_activity( task_name, from );
    LQIO::DOM::Entry * to_entry = LQIO::DOM::__document->getEntryByName( to );
    if ( from_activity && to_entry ) {
	LQIO::DOM::Call * call = from_activity->getCallToTarget( to_entry );
	if ( call ) {
	    call->setResultVarianceWaitingTime( delay[0] );
	}
    }
}


void 
add_act_wait_variance_confidence (const char * task_name, const char *from, const char * to, int conf_level, double *delay)
{
    if ( conf_level != 95) return;

    LQIO::DOM::Activity * from_activity = srvn_find_activity( task_name, from );
    LQIO::DOM::Entry * to_entry = LQIO::DOM::__document->getEntryByName( to );
    if ( from_activity && to_entry ) {
	LQIO::DOM::Call * call = from_activity->getCallToTarget( to_entry );
	if ( call ) {
	    call->setResultVarianceWaitingTimeVariance( LQIO::ConfidenceIntervals::invert(delay[0], LQIO::DOM::__document->getResultNumberOfBlocks(), 
											  LQIO::ConfidenceIntervals::CONF_95 )  );
	}
    }
}


void 
add_act_snr_wait_variance (const char * task_name, const char *from, const char *to, double *delay)
{
    add_act_wait_variance( task_name, from, to, delay);
}


void 
add_act_snr_wait_variance_confidence (const char * task_name, const char *from, const char *to, int conf_level, double *delay)
{
    add_act_wait_variance_confidence(task_name, from, to, conf_level, delay);
}


void
add_act_drop_probability( const char * task_name, const char *from, const char *to, double *prob )
{
    LQIO::DOM::Activity * from_activity = srvn_find_activity( task_name, from );
    LQIO::DOM::Entry * to_entry = LQIO::DOM::__document->getEntryByName( to );
    if ( from_activity && to_entry ) {
	LQIO::DOM::Call * call = from_activity->getCallToTarget( to_entry );
	if ( call ) {
	    call->setResultDropProbability( prob[0] );
	}
    }
}


void
add_act_drop_probability_confidence( const char * task_name, const char *from, const char *to, int conf_level, double *prob )
{
    if ( conf_level != 95) return;

    LQIO::DOM::Activity * from_activity = srvn_find_activity( task_name, from );
    LQIO::DOM::Entry * to_entry = LQIO::DOM::__document->getEntryByName( to );
    if ( from_activity && to_entry ) {
	LQIO::DOM::Call * call = from_activity->getCallToTarget( to_entry );
	if ( call ) {
	    call->setResultDropProbabilityVariance( LQIO::ConfidenceIntervals::invert(prob[0], LQIO::DOM::__document->getResultNumberOfBlocks(), 
										      LQIO::ConfidenceIntervals::CONF_95 )  );
	}
    }
}

/*
 * Add a waiting time datum to the list of waiting times.
 */

void
add_join (const char * task_name, const char *first, const char *last, double mean, double variance)
{
    LQIO::DOM::Activity * lastActivity = srvn_find_activity( task_name, last );
    LQIO::DOM::Activity * firstActivity = srvn_find_activity( task_name, first );

    if ( firstActivity && lastActivity ) {
	LQIO::DOM::AndJoinActivityList * join_list = dynamic_cast<LQIO::DOM::AndJoinActivityList *>(firstActivity->getOutputToList());
	if ( join_list == lastActivity->getOutputToList() ) {
	    join_list->setResultJoinDelay( mean )
		.setResultVarianceJoinDelay( variance );
	}
    }
}


/*
 * Store confidence interval data.
 */


void
add_join_confidence(const char * task_name, const char *first, const char *last, int conf_level, double mean, double variance )
{
    if ( conf_level != 95) return;

    LQIO::DOM::Activity * lastActivity = srvn_find_activity( task_name, last );
    LQIO::DOM::Activity * firstActivity = srvn_find_activity( task_name, first );

    if ( firstActivity && lastActivity ) {
	LQIO::DOM::AndJoinActivityList * join_list = dynamic_cast<LQIO::DOM::AndJoinActivityList *>(firstActivity->getOutputToList());
	if ( join_list == lastActivity->getOutputToList() ) {
	    join_list->setResultJoinDelayVariance( LQIO::ConfidenceIntervals::invert(mean, LQIO::DOM::__document->getResultNumberOfBlocks(), 
										     LQIO::ConfidenceIntervals::CONF_95 ) )
		.setResultVarianceJoinDelayVariance( LQIO::ConfidenceIntervals::invert(variance, LQIO::DOM::__document->getResultNumberOfBlocks(), 
										       LQIO::ConfidenceIntervals::CONF_95 ) );
	}
    }
}

/*
 * Generate the parseable file name from the input file name and load the results.
 * This will likey be deprecated somewhat with XML input.
 */

namespace LQIO {
    namespace SRVN {
	bool
	loadResults( const std::string& filename )
	{
	    srvn_max_phases = LQIO::DOM::Phase::MAX_PHASE;	/* for phase lists in resultparse() */
	    
	    if ( ! LQIO::DOM::__document ) {
		return false;
	    }
	    if ( filename == "-" ) {
		resultin = stdin;
		resultparse();
	    } else if ( ( resultin = fopen( filename.c_str(), "r" ) ) != nullptr ) {
		results_file_name = filename.c_str();
		resultlineno = 1;
#if HAVE_MMAP
		struct stat statbuf;
		int resultin_fd = fileno( resultin );
		if ( fstat( resultin_fd, &statbuf ) != 0 ) {
		    std::cerr << LQIO::io_vars.lq_toolname << ": Cannot stat " << filename << " - " << strerror( errno ) << std::endl;
		    return false;
		}

		char * buffer = static_cast<char *>(mmap( 0, statbuf.st_size, PROT_READ, MAP_PRIVATE|MAP_FILE, resultin_fd, 0 ));
		if ( buffer != MAP_FAILED ) {
		    yy_buffer_state * yybuf = result_scan_string( buffer );
		    resultparse();
		    result_delete_buffer( yybuf );
		    munmap( buffer, statbuf.st_size );
		} else {
		    /* Try the old way (for pipes) */
#endif
		    resultparse();
#if HAVE_MMAP
		}
#endif
		if ( resultin ) {
		    fclose( resultin );
		}
	    } else {
                std::cerr << LQIO::io_vars.lq_toolname << ": Cannot open result file " << filename << " - " << strerror( errno ) << std::endl;
		return false;
	    }
	    return !LQIO::io_vars.anError();
	}
    }
}


/*
 * Print out and error message (and the line number on which it
 * occurred.
 */

void
results_error( const char * fmt, ... )
{
    va_list args;
    va_start( args, fmt );
    (void) LQIO::verrprintf( stderr, LQIO::ADVISORY_ONLY, results_file_name, resultlineno, 0, fmt, args );
    va_end( args );
}

static void
results_error2( unsigned err, ... )
{
    va_list args;
    va_start( args, err );
    LQIO::verrprintf( stderr, LQIO::io_vars.error_messages[err].severity, results_file_name, resultlineno, 0,
		      LQIO::io_vars.error_messages[err].message, args );
    va_end( args );
}
