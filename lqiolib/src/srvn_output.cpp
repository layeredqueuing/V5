/*
 *  $Id: srvn_output.cpp 17603 2025-11-26 22:09:43Z greg $
 *
 * Copyright the Real-Time and Distributed Systems Group,
 * Department of Systems and Computer Engineering,
 * Carleton University, Ottawa, Ontario, Canada. K1S 5B6
 *
 */

#if HAVE_CONFIG_H
#include <config.h>
#endif
#include <algorithm>
#include <cmath>
#include <functional>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <lqx/SyntaxTree.h>
#include "common_io.h"
#include "dom_activity.h"
#include "dom_actlist.h"
#include "dom_call.h"
#include "dom_document.h"
#include "dom_entry.h"
#include "dom_group.h"
#include "dom_histogram.h"
#include "dom_phase.h"
#include "dom_processor.h"
#include "dom_task.h"
#include "glblerr.h"
#include "input.h"
#include "labels.h"
#include "srvn_output.h"
#include "srvn_spex.h"

namespace LQIO {
    unsigned int SRVN::ObjectOutput::__maxPhase = 0;
    ConfidenceIntervals * SRVN::ObjectOutput::__conf95 = 0;
    ConfidenceIntervals * SRVN::ObjectOutput::__conf99 = 0;
    bool SRVN::ObjectOutput::__parseable = false;
    bool SRVN::ObjectOutput::__rtf = false;
    bool SRVN::ObjectOutput::__coloured = false;
    unsigned int SRVN::ObjectInput::__maxEntLen = 1;
    unsigned int SRVN::ObjectInput::__maxInpLen = 1;

    static inline void throw_bad_parameter() { throw std::domain_error( "invalid parameter" ); }

    bool
    SRVN::for_each_entry_any_of::operator()( const DOM::Entity * entity ) const
    {
	const DOM::Task * task = dynamic_cast<const DOM::Task *>(entity);
	if ( task == nullptr ) return false;

	const std::vector<DOM::Entry *> & entries = task->getEntryList();
	return std::any_of( entries.begin(), entries.end(), std::mem_fn( _f ) );
    }

    int
    SRVN::for_each_entry_count_if::operator()( const DOM::Entity * entity ) const
    {
	const DOM::Task * task = dynamic_cast<const DOM::Task *>(entity);
	if ( task == nullptr ) return 0;

	const std::vector<DOM::Entry *> & entries = task->getEntryList();
	return std::count_if( entries.begin(), entries.end(), std::mem_fn( _f ) );
    }

    SRVN::ForPhase::ForPhase()
	: _calls(), _maxPhase(DOM::Phase::MAX_PHASE), _type(DOM::Call::Type::NULL_CALL)
    {
    }

    void
    SRVN::CollectCalls::operator()( const std::pair<unsigned, DOM::Phase*>& p )
    {
	const unsigned n = p.first;
	const std::vector<DOM::Call*>& calls = p.second->getCalls();
	for ( std::vector<DOM::Call*>::const_iterator c = calls.begin(); c != calls.end(); ++c ) {
	    const DOM::Call* call = *c;
	    if ( !_test || (call->*_test)() ) {
		ForPhase& y = _calls[call->getDestinationEntry()];
		y.setType( call->getCallType() );
		y[n] = call;
	    }
	}
    }

    SRVN::Output::Output( const DOM::Document& document,
                          bool print_confidence_intervals, bool print_variances, bool print_histograms )
        : _document(document), _entities(document.getEntities()),
          _print_variances(print_variances), _print_histograms(print_histograms)
    {
        /* Set various globals for pretting printing */
        assert( ObjectOutput::__maxPhase == 0 );
        ObjectOutput::__maxPhase = document.getMaximumPhase();
        ObjectOutput::__parseable = false;
        ObjectOutput::__rtf = false;
        ObjectOutput::__coloured = false;
	const unsigned number_of_blocks = document.getResultNumberOfBlocks();
        if ( document.hasConfidenceIntervals() && number_of_blocks >= 2 && print_confidence_intervals ) {
            ObjectOutput::__conf95 = new ConfidenceIntervals( ConfidenceIntervals::CONF_95, number_of_blocks );
            ObjectOutput::__conf99 = new ConfidenceIntervals( ConfidenceIntervals::CONF_99, number_of_blocks );
        } else {
            ObjectOutput::__conf95 = 0;
            ObjectOutput::__conf99 = 0;
        }
    }

    SRVN::Output::~Output()
    {
        ObjectOutput::__maxPhase = 0;
        if ( ObjectOutput::__conf95 ) delete ObjectOutput::__conf95;
        if ( ObjectOutput::__conf99 ) delete ObjectOutput::__conf99;
        ObjectOutput::__conf95 = 0;
        ObjectOutput::__conf99 = 0;
    }

    /* Proxies */
    std::ostream&
    SRVN::Output::newline( std::ostream& output )
    {
        return ObjectOutput::newline( output );
    }

    std::ostream&
    SRVN::Output::textrm( std::ostream& output )
    {
        return ObjectOutput::textrm( output );
    }

    std::ostream&
    SRVN::Output::textbf( std::ostream& output )
    {
        return ObjectOutput::textbf( output );
    }

    std::ostream&
    SRVN::Output::textit( std::ostream& output )
    {
        return ObjectOutput::textit( output );
    }

    /*
     * Print out a generic header for Entry name on fptr.
     */

    std::ostream&
    SRVN::Output::taskHeader( std::ostream& output, const std::string& s )
    {
        output << newline << newline << textbf << s << newline << newline << textrm
               << std::setw(ObjectOutput::__maxStrLen) << "Task Name" << std::setw(ObjectOutput::__maxStrLen) << "Entry Name";
        return output;
    }

    /* static */ std::ostream&
    SRVN::Output::entryHeader( std::ostream& output, const std::string& s )
    {
        output << task_header( s ) << phase_header( ObjectOutput::__maxPhase );
        return output;
    }

    std::ostream&
    SRVN::Output::activityHeader( std::ostream& output, const std::string& s )
    {
        output << newline << newline << s << newline << newline
               << std::setw(ObjectOutput::__maxStrLen) << "Task Name"
               << std::setw(ObjectOutput::__maxStrLen) << "Source Activity"
               << std::setw(ObjectOutput::__maxStrLen) << "Target Activity";
        return output;
    }

    std::ostream&
    SRVN::Output::callHeader( std::ostream& output, const std::string& s )
    {
        output << newline << newline << textbf << s << newline << newline << textrm
               << std::setw(ObjectOutput::__maxStrLen) << "Task Name"
               << std::setw(ObjectOutput::__maxStrLen) << "Source Entry"
               << std::setw(ObjectOutput::__maxStrLen) << "Target Entry";
        return output;
    }

    std::ostream&
    SRVN::Output::phaseHeader( std::ostream& output, const unsigned int n_phases )
    {
        std::ios_base::fmtflags oldFlags = output.setf( std::ios::left, std::ios::adjustfield );
        for ( unsigned p = 1; p <= n_phases; p++ ) {
	    std::ostringstream aString;
            aString << "Phase " << p;
            output << std::setw(ObjectOutput::__maxDblLen) << aString.str();
        }
        output.flags(oldFlags);
        return output;
    }

    std::ostream&
    SRVN::Output::holdHeader( std::ostream& output, const std::string& s )
    {
        std::ios_base::fmtflags oldFlags = output.setf( std::ios::left, std::ios::adjustfield );
        output << newline << newline << textbf << s << newline << newline << textrm
               << std::setw(ObjectOutput::__maxStrLen) << "Task Name"
               << std::setw(ObjectOutput::__maxStrLen) << "Wait Entry"
               << std::setw(ObjectOutput::__maxStrLen) << "Signal Entry"
               << std::setw(ObjectOutput::__maxDblLen) << "Hold Time"
               << std::setw(ObjectOutput::__maxDblLen) << "Variance"
               << std::setw(ObjectOutput::__maxDblLen) << "Utilization";
        output.flags(oldFlags);
        return output;
    }

    std::ostream&
    SRVN::Output::rwlockHeader( std::ostream& output, const std::string& s )
    {
        std::ios_base::fmtflags oldFlags = output.setf( std::ios::left, std::ios::adjustfield );
        output << newline << newline << s << newline << newline
               << std::setw(ObjectOutput::__maxStrLen) << "Task Name"
               << std::setw(ObjectOutput::__maxStrLen) << "Lock Entry"
               << std::setw(ObjectOutput::__maxStrLen) << "Unlock Entry"
               << std::setw(ObjectOutput::__maxDblLen) << "Blocked Time"
               << std::setw(ObjectOutput::__maxDblLen) << "Variance"
               << std::setw(ObjectOutput::__maxDblLen) << "Hold Time"
               << std::setw(ObjectOutput::__maxDblLen) << "Variance"
               << std::setw(ObjectOutput::__maxDblLen) << "Utilization";
        output.flags(oldFlags);
        return output;
    }

    /* ---------------------------------------------------------------- */
    /* MAIN code for printing                                           */
    /* ---------------------------------------------------------------- */

    std::ostream&
    SRVN::Output::print( std::ostream& output ) const
    {
        printPreamble( output );
        printInput( output );
        printResults( output );
        printPostamble( output );
        return output;
    }


    std::ostream&
    SRVN::Output::printPreamble( std::ostream& output ) const
    {
        DocumentOutput print( output );
        print( getDOM() );
        return output;
    }

    /* Echo of input. */
    std::ostream&
    SRVN::Output::printInput( std::ostream& output ) const
    {
        std::ios_base::fmtflags oldFlags = output.setf( std::ios::left, std::ios::adjustfield );

        output << newline << textbf << processor_info_str << newline << newline
               << textrm << std::setw(ObjectOutput::__maxStrLen) << "Processor Name" << "Type    Copies  Scheduling";
        if ( getDOM().processorHasRate() ) {
            output << " Rate";
        }
        output << newline;
        std::for_each( _entities.begin(), _entities.end(), ProcessorOutput( output, &ProcessorOutput::printParameters ) );

        const std::map<std::string,DOM::Group*>& groups = getDOM().getGroups();
        if ( groups.size() > 0 ) {
            output << newline << textbf << group_info_str << newline << newline
                   << std::setw(ObjectOutput::__maxStrLen) << "Group Name" << "Share       Processor Name" << newline << textrm;
            std::for_each( groups.begin(), groups.end(), GroupOutput( output, &GroupOutput::printParameters ) );
        }

        output << newline << newline << textbf << task_info_str << newline << newline
               << textrm << std::setw(ObjectOutput::__maxStrLen) << "Task Name" << "Type    Copies  " << std::setw(ObjectOutput::__maxStrLen) << "Processor Name";
        if ( getDOM().getNumberOfGroups() > 0 ) {
            output << std::setw(ObjectOutput::__maxStrLen) << "Group Name";
        }
        output << "Pri ";
        if ( getDOM().taskHasThinkTime() ) {
            output << std::setw(ObjectOutput::__maxDblLen) << "Think Time";
        }
        output << "Entry List" << newline;
        std::for_each( _entities.begin(), _entities.end(), TaskOutput( output, &TaskOutput::printParameters ) );

        output << entry_header( service_demand_str ) << newline;
        std::for_each( _entities.begin(), _entities.end(), EntryOutput( output, &EntryOutput::printEntryDemand, &EntryOutput::printActivityDemand  ) );

        if ( getDOM().hasThinkTime() ) {
            output << entry_header( think_time_str ) << newline;
            std::for_each( _entities.begin(), _entities.end(), EntryOutput( output, &EntryOutput::printEntryThinkTime, &EntryOutput::printActivityThinkTime ) );
        }

        if ( getDOM().hasMaxServiceTimeExceeded() ) {
            output << entry_header( max_service_time_str ) << newline;
            std::for_each( _entities.begin(), _entities.end(), EntryOutput( output, &EntryOutput::printEntryMaxServiceTime ) );
        }

        /* print mean number of Rendezvous */

        if ( getDOM().hasRendezvous() || getDOM().hasForwarding() ) {
            output << call_header( rendezvous_rate_str ) << phase_header( ObjectOutput::__maxPhase ) << newline;
            std::for_each( _entities.begin(), _entities.end(), CallOutput( output, &DOM::Call::hasRendezvous, &CallOutput::printCallRate ) );
	    if ( getDOM().hasForwarding() ) {
		output << call_header( forwarding_probability_str ) << std::setw(ObjectOutput::__maxDblLen) << "Prob" << newline;
		std::for_each( _entities.begin(), _entities.end(), EntryOutput( output, &EntryOutput::printForwarding ) );
	    }
	}

        if ( getDOM().hasSendNoReply() ) {
            output << call_header( send_no_reply_rate_str ) << phase_header( ObjectOutput::__maxPhase ) << newline;
            std::for_each( _entities.begin(), _entities.end(), CallOutput( output, &DOM::Call::hasSendNoReply, &CallOutput::printCallRate ) );
        }

        if ( getDOM().hasDeterministicPhase() ) {
            output << entry_header( phase_type_str ) << newline;
            std::for_each( _entities.begin(), _entities.end(), EntryOutput( output, &EntryOutput::printEntryPhaseType, &EntryOutput::printActivityPhaseType ) );
        } else {
            output << newline << newline << textbf << phase_type_str << newline << textrm;
            output << "All phases are stochastic." << newline;
        }

        if ( getDOM().hasNonExponentialPhase() ) {
            output << entry_header( cv_square_str ) << newline << textrm;
            std::for_each( _entities.begin(), _entities.end(), EntryOutput( output, &EntryOutput::printEntryCoefficientOfVariation, &EntryOutput::printActivityCoefficientOfVariation ) );
        } else {
            output << newline << newline << textbf << cv_square_str << newline << textrm;
            output << "All executable segments are exponential." << newline;
        }

        output << newline << newline << textbf << open_arrival_rate_str << newline << textrm;
        if ( getDOM().hasOpenArrivals() ) {
            output << newline << std::setw(ObjectOutput::__maxStrLen) << "Entry Name" << std::setw(ObjectOutput::__maxDblLen) << "Arrival Rate" << newline;
            std::for_each( _entities.begin(), _entities.end(), EntryOutput( output, &EntryOutput::printOpenArrivals ) );
        } else {
            output << "All open arrival rates are 0." << newline;
        }
        output.flags(oldFlags);
        return output;
    }

    std::ostream&
    SRVN::Output::printResults( std::ostream& output ) const
    {
        std::ios_base::fmtflags oldFlags = output.setf( std::ios::left, std::ios::adjustfield );

        if ( getDOM().entryHasThroughputBound() ) {
            output << textbf << task_header( throughput_bounds_str ) << "Throughput  " << newline << textrm;
            std::for_each( _entities.begin(), _entities.end(), EntryOutput( output, &EntryOutput::printEntryThroughputBounds ) );
        }

        /* Waiting times */

        if ( getDOM().hasRendezvous() ) {
            output << call_header( waiting_time_str ) << phase_header( ObjectOutput::__maxPhase ) << newline;
            std::for_each( _entities.begin(), _entities.end(), CallOutput( output, &DOM::Call::hasRendezvous, &CallOutput::printCallWaiting, &CallOutput::printCallWaitingConfidence ) );
        }
	if ( getDOM().hasForwarding() ) {
	    output << textbf << call_header( fwd_waiting_time_str ) << newline << textrm;
	    std::for_each( _entities.begin(), _entities.end(), EntryOutput( output, &EntryOutput::printForwardingWaiting ) );
	}

        if ( getDOM().hasSendNoReply() ) {
            output << call_header( snr_waiting_time_str ) << phase_header( ObjectOutput::__maxPhase ) << newline;
            std::for_each( _entities.begin(), _entities.end(), CallOutput( output, &DOM::Call::hasSendNoReply, &CallOutput::printCallWaiting, &CallOutput::printCallWaitingConfidence ) );
        }

	/* Waiting time variances */

	if ( getDOM().entryHasWaitingTimeVariance() && _print_variances ) {
	    if ( getDOM().hasRendezvous() ) {
		output << call_header( waiting_time_variance_str ) << phase_header( ObjectOutput::__maxPhase ) << newline;
		std::for_each( _entities.begin(), _entities.end(), CallOutput( output, &DOM::Call::hasRendezvous, &CallOutput::printCallVarianceWaiting, &CallOutput::printCallVarianceWaitingConfidence ) );
	    }
	    if ( getDOM().hasForwarding() ) {
		output << textbf << call_header( fwd_waiting_time_variance_str ) << newline << textrm;
		std::for_each( _entities.begin(), _entities.end(), EntryOutput( output, &EntryOutput::printForwardingVarianceWaiting ) );
	    }

	    if ( getDOM().hasSendNoReply() ) {
		output << call_header( snr_waiting_time_variance_str ) << phase_header( ObjectOutput::__maxPhase ) << newline;
		std::for_each( _entities.begin(), _entities.end(), CallOutput( output, &DOM::Call::hasSendNoReply, &CallOutput::printCallVarianceWaiting, &CallOutput::printCallVarianceWaitingConfidence ) );
	    }
	}

	/* Drop probabilities. */

        if ( getDOM().entryHasDropProbability() ) {
            output << call_header( loss_probability_str ) << phase_header( ObjectOutput::__maxPhase ) << newline;
            std::for_each( _entities.begin(), _entities.end(), CallOutput( output, &DOM::Call::hasResultDropProbability, &CallOutput::printDropProbability, &CallOutput::printDropProbabilityConfidence ) );
        }

        /* Join delays */

        if ( getDOM().taskHasAndJoin() ) {
            output << activity_header( join_delay_str ) << "Mean        Variance" << newline;
            std::for_each( _entities.begin(), _entities.end(), TaskOutput( output, &TaskOutput::printJoinDelay ) );
        }

        /* Service time */
        output << entry_header( service_time_str ) << newline;
        std::for_each( _entities.begin(), _entities.end(), EntryOutput( output, &EntryOutput::printEntryServiceTime, &EntryOutput::printActivityServiceTime  ) );

        if ( getDOM().entryHasServiceTimeVariance() && _print_variances ) {
            output << entry_header( variance_str ) << "coeff of var **2" << newline;
            std::for_each( _entities.begin(), _entities.end(), EntryOutput( output, &EntryOutput::printEntryVarianceServiceTime, &EntryOutput::printActivityVarianceServiceTime  ) );
        }

        /* Histograms */

        if ( getDOM().hasMaxServiceTimeExceeded() ) {
            output << entry_header( service_time_exceeded_str ) << newline;
            std::for_each( _entities.begin(), _entities.end(), EntryOutput( output, &EntryOutput::printEntryMaxServiceTimeExceeded, &EntryOutput::printActivityMaxServiceTimeExceeded, &EntryOutput::testActivityMaxServiceTimeExceeded ) );
        }

        if ( getDOM().hasHistogram() && _print_histograms ) {
            output << newline << newline << histogram_str << newline << newline;
            std::for_each( _entities.begin(), _entities.end(), HistogramOutput( output, &HistogramOutput::printHistogram ) );
        }

        /* Semaphore holding times */

        if ( getDOM().hasSemaphoreWait() ) {
            output << hold_header( semaphore_hold_time_str ) << newline;
            std::for_each( _entities.begin(), _entities.end(), TaskOutput( output, &TaskOutput::printHoldTime ) );
        }

        /* RWLock holding times */

        if ( getDOM().hasRWLockWait() ) {
            output << rwlock_header( rwlock_hold_time_str ) << newline;
            std::for_each( _entities.begin(), _entities.end(), TaskOutput( output, &TaskOutput::printRWLOCKHoldTime ) );
        }


        /* Task Throughput and Utilization */

        output << task_header( throughput_str ) << std::setw(ObjectOutput::__maxDblLen) << "Throughput" << phase_header( ObjectOutput::__maxPhase ) << std::setw(ObjectOutput::__maxDblLen) << "Total" << newline;
        std::for_each( _entities.begin(), _entities.end(), TaskOutput( output, &TaskOutput::printThroughputAndUtilization ) );

        /* Open arrival wait times */

        if ( getDOM().hasOpenArrivals() ) {
            output << task_header( open_wait_str ) << std::setw(ObjectOutput::__maxDblLen) << "Lambda" << std::setw(ObjectOutput::__maxDblLen) << "Waiting time" << newline;
            std::for_each( _entities.begin(), _entities.end(), EntryOutput( output, &EntryOutput::printOpenQueueWait ) );

	    if ( std::any_of( _entities.begin(), _entities.end(), for_each_entry_any_of( &DOM::Entry::hasResultDropProbability ) ) ) {
		output << task_header( loss_probability_str ) << std::setw(ObjectOutput::__maxDblLen) << "Drop Probability" << newline;
		std::for_each( _entities.begin(), _entities.end(), EntryOutput( output, &EntryOutput::printOpenQueueDropProbability ) );
	    }
        }

        /* Processor utilization and waiting */

        std::for_each( _entities.begin(), _entities.end(), ProcessorOutput( output, &ProcessorOutput::printUtilizationAndWaiting ) );

#if defined(BUG_393)
	/* If marginals available... */

	std::for_each( _entities.begin(), _entities.end(), EntityOutput( output, &EntityOutput::printMarginalQueueProbabilities ) );
#endif
	
        output.flags(oldFlags);
        return output;
    }


    std::ostream&
    SRVN::ObjectOutput::printEntityName( std::ostream& output, const DOM::Entity& entity, bool& print )
    {
        if ( print ) {
            output << std::setw(__maxStrLen-1) << entity.getName() << (__parseable ? ":" : " ");
            print = false;
        } else {
            output << std::setw(__maxStrLen) << " ";
        }
        return output;
    }

    std::ostream&
    SRVN::ObjectOutput::printEntryName( std::ostream& output, const DOM::Entry& entry )
    {
        output << std::setw(__maxStrLen-1) << entry.getName() << " ";
        return output;
    }

    std::ostream&
    SRVN::ObjectOutput::activitySeparator( std::ostream& output, const unsigned offset )
    {
        if ( __parseable ) {
            output << "-1";
            std::ios_base::fmtflags oldFlags = output.setf( std::ios::right, std::ios::adjustfield );
            output << newline << std::setw(offset) << ":";
            output.flags(oldFlags);
        } else {
            if ( offset > 0 ) {
                output << std::setw(offset) << " ";
            }
            output << std::setw(__maxStrLen) << "Activity Name";
        }
        return output;
    }

    std::ostream&
    SRVN::ObjectOutput::confLevel( std::ostream& output, const unsigned int fill, const ConfidenceIntervals::confidence_level_t level )
    {
        std::ios_base::fmtflags flags = output.setf( std::ios::right, std::ios::adjustfield );
        output << std::setw( fill-4 ) << ( __parseable ? " " : "+/-" );
        if ( level == ConfidenceIntervals::CONF_95 ) {
            output << (__parseable ? "%95 " : "95% " );
        } else {
            output << (__parseable ? "%99 " : "99% " );
        }
        output.flags( flags );
        return output;
    }

    std::ostream&
    SRVN::ObjectOutput::textcolour( std::ostream& output, const double utilization )
    {
        if ( __rtf ) {
            if ( utilization >= 0.8 ) {
                output << "\\cf2 ";             /* Red 255,0,0 */
                __coloured = true;
            } else if ( utilization >= 0.6 ) {
                output << "\\cf3 ";             /* Orange 255,164,0 */
                __coloured = true;
            } else if ( utilization >= 0.5 ) {
                output << "\\cf4 ";             /* Green 0,255,0 */
                __coloured = true;
            } else if ( utilization >= 0.4 ) {
                output << "\\cf5 ";             /* Blue 0,255,0 */
                __coloured = true;
            } else if ( __coloured ) {
                output << "\\cf0 ";             /* Black 0,0,0 */
                __coloured = false;
            }
        }
        return output;
    }

    std::ostream&
    SRVN::ObjectOutput::entryInfo( std::ostream& output, const DOM::Entry & entry, const entryFunc func )
    {
	output << std::setw(__maxDblLen) << Input::print_double_parameter( (entry.*func)(), 0. ) << ' ';	
        return output;
    }

    /* static */ std::ostream&
    SRVN::ObjectOutput::phaseInfo( std::ostream& output, const DOM::Entry & entry, const phaseFunc func )
    {
        const std::map<unsigned, DOM::Phase*>& phases = entry.getPhaseList();
        assert( phases.size() <= DOM::Phase::MAX_PHASE );
        for ( std::map<unsigned, DOM::Phase*>::const_iterator p = phases.begin(); p != phases.end(); ++p ) {
            const DOM::Phase* phase = p->second;
            if ( phase ) {
                output << std::setw(__maxDblLen-1) << Input::print_double_parameter( (phase->*func)(), 0. ) << ' ';
            } else {
                output << std::setw(__maxDblLen) << 0.0;
            }
        }
        return output;
    }

    /* static */ std::ostream&
    SRVN::ObjectOutput::phaseTypeInfo( std::ostream& output, const DOM::Entry & entry, const phaseTypeFunc func )
    {
        const std::map<unsigned, DOM::Phase*>& phases = entry.getPhaseList();
        assert( phases.size() <= DOM::Phase::MAX_PHASE );
        for ( std::map<unsigned, DOM::Phase*>::const_iterator p = phases.begin(); p != phases.end(); ++p) {
            const DOM::Phase* phase = p->second;
            switch ( phase->getPhaseTypeFlag() ) {
            case DOM::Phase::Type::DETERMINISTIC: output << std::setw(__maxDblLen) << "determin"; break;
            case DOM::Phase::Type::STOCHASTIC:    output << std::setw(__maxDblLen) << "stochastic"; break;
            }
        }
        return output;
    }

    /* static */ std::ostream&
    SRVN::ObjectOutput::phaseResults( std::ostream& output, const DOM::Entry & entry, const doublePhaseFunc phase_func, const doubleEntryPhaseFunc entry_func, const bool pad )
    {
        unsigned int np = 0;
        const std::map<unsigned, DOM::Phase*>& phases = entry.getPhaseList();
        assert( phases.size() <= DOM::Phase::MAX_PHASE );
        if ( entry.getStartActivity() ) {
            if ( !entry_func ) return output;
            for ( unsigned int p = 1; p <= __maxPhase; ++p ) {
		if ( p > 1 ) output  << ' ';
                output << std::setw(__maxDblLen-1) << (entry.*entry_func)(p);
            }
            np = __maxPhase;
        } else {
            for ( std::map<unsigned, DOM::Phase*>::const_iterator p = phases.begin(); p != phases.end(); ++p) {
                const DOM::Phase* phase = p->second;
                output << std::setw(__maxDblLen-1) << (phase->*phase_func)() << ' ';
            }
            np = phases.size();
        }
        if ( pad || __parseable ) {
            for ( unsigned p = np; p < __maxPhase; ++p ) {
                output << std::setw(__maxDblLen) << (__parseable ? "0" : " ");
            }
        }
        output << activityEOF;
        return output;
    }


    /* static */ std::ostream&
    SRVN::ObjectOutput::phaseResultsConfidence( std::ostream& output, const DOM::Entry & entry, const doublePhaseFunc phase_func, const doubleEntryPhaseFunc entry_func, const ConfidenceIntervals * conf, const bool pad )
    {
        unsigned int np = 0;
        const std::map<unsigned, DOM::Phase*>& phases = entry.getPhaseList();
        assert( phases.size() <= DOM::Phase::MAX_PHASE );
        if ( entry.getStartActivity() ) {
            if ( !entry_func ) return output;
            for ( unsigned int p = 1; p <= __maxPhase; ++p ) {
                output << std::setw(__maxDblLen-1) << (*conf)((entry.*entry_func)(p)) << ' ';
            }
            np = __maxPhase;
        } else {
            for ( std::map<unsigned, DOM::Phase*>::const_iterator p = phases.begin(); p != phases.end(); ++p) {
                const DOM::Phase* phase = p->second;
                output << std::setw(__maxDblLen-1) << (*conf)((phase->*phase_func)()) << ' ';
            }
            np = phases.size();
        }
        if ( pad || __parseable ) {
            for ( unsigned p = np; p < __maxPhase; ++p ) {
                output << std::setw(__maxDblLen) << (__parseable ? "0" : " ");
            }
        }
        output << activityEOF;
        return output;
    }

    /* static */ std::ostream&
    SRVN::ObjectOutput::taskPhaseResults( std::ostream& output, const DOM::Task & task, const doubleTaskFunc func, const bool pad )
    {
        for ( unsigned p = 1; p <= task.getResultPhaseCount(); ++p ) {
            output << std::setw(__maxDblLen-1) << (task.*func)(p) << ' ';
        }
        if ( pad || __parseable ) {
            for ( unsigned p = task.getResultPhaseCount(); p < __maxPhase; ++p ) {
                output << std::setw(__maxDblLen) << (__parseable ? "0" : " ");
            }
        }
        output << activityEOF;
        return output;
    }


    /* static */ std::ostream&
    SRVN::ObjectOutput::task3PhaseResults( std::ostream& output, const DOM::Task & task, const doubleTaskFunc func, const bool pad )
    {
        for ( unsigned p = 1; p <= DOM::Phase::MAX_PHASE; ++p ) {
            output << std::setw(__maxDblLen-1) << (task.*func)(p) << ' ';
        }
        output << activityEOF;
        return output;
    }


    /* static */ std::ostream&
    SRVN::ObjectOutput::taskPhaseResultsConfidence( std::ostream& output, const DOM::Task & task, const doubleTaskFunc func, const ConfidenceIntervals * conf, const bool pad  )
    {
        for ( unsigned p = 1; p <= task.getResultPhaseCount(); ++p ) {
            output << std::setw(__maxDblLen-1) << (*conf)((task.*func)(p)) << ' ';
        }
        if ( pad || __parseable ) {
            for ( unsigned p = task.getResultPhaseCount(); p < __maxPhase; ++p ) {
                output << std::setw(__maxDblLen) << (__parseable ? "0" : " ");
            }
        }
        output << activityEOF;
        return output;
    }

    /* static */ std::ostream&
    SRVN::ObjectOutput::task3PhaseResultsConfidence( std::ostream& output, const DOM::Task & task, const doubleTaskFunc func, const ConfidenceIntervals * conf, const bool pad  )
    {
        for ( unsigned p = 1; p <= DOM::Phase::MAX_PHASE; ++p ) {
            output << std::setw(__maxDblLen-1) << (*conf)((task.*func)(p)) << ' ';
        }
        output << activityEOF;
        return output;
    }

    void
    SRVN::ObjectInput::printReplyList( const std::vector<DOM::Entry*>& replies ) const
    {
        _output << "[";
        for ( std::vector<DOM::Entry *>::const_iterator entry = replies.begin(); entry != replies.end(); ++entry ) {
            if ( entry != replies.begin() ) {
                _output << ",";
            }
            _output << (*entry)->getName();
        }
        _output << "]";
    }

    std::ostream&
    SRVN::ObjectInput::printNumberOfCalls( std::ostream& output, const DOM::Call* call )
    {
	output << " " << std::setw(ObjectInput::__maxInpLen);
	if ( call != nullptr ) {
	    output << Input::print_double_parameter( call->getCallMean() );
	} else {
	    output << 0;
	}
	return output;
    }


    std::ostream&
    SRVN::ObjectInput::printCallType( std::ostream& output, const DOM::Call* call )
    {
        switch ( call->getCallType() ) {
        case DOM::Call::Type::RENDEZVOUS: output << "y"; break;
        case DOM::Call::Type::SEND_NO_REPLY: output << "z"; break;
        default: abort();
        }
        return output;
    }

    /*
     * Collect all variables associated with the object, then print them out.  Since entries have their own variables,
     * and phases are associated with entries, do both at the same time.
     */

    /* static  */ std::ostream&
    SRVN::ObjectInput::printObservationVariables( std::ostream& output, const DOM::DocumentObject& object )
    {
	std::pair<Spex::obs_var_tab_t::const_iterator, Spex::obs_var_tab_t::const_iterator> range = Spex::observations().equal_range( &object );
	for ( Spex::obs_var_tab_t::const_iterator obs = range.first; obs != range.second; ++obs ) {
	    output << " ";
	    obs->second.print( output );
	}
	/* If object is an entry, call again as a phase. */
	if ( dynamic_cast<const DOM::Entry *>(&object) != 0 ) {
	    const std::map<unsigned, DOM::Phase*>& phases = dynamic_cast<const DOM::Entry *>(&object)->getPhaseList();
	    for ( std::map<unsigned, DOM::Phase*>::const_iterator phase = phases.begin(); phase != phases.end(); ++phase ) {
		printObservationVariables( output, *phase->second );
	    }
	}
	return output;
    }

    /* ---------------------------------------------------------------- */
    /* Parseable Output                                                 */
    /* ---------------------------------------------------------------- */

    SRVN::Parseable::Parseable( const DOM::Document& document, bool print_confidence_intervals )
        : SRVN::Output( document, print_confidence_intervals )
    {
        ObjectOutput::__parseable = true;               /* Set global for formatting. */
        ObjectOutput::__rtf = false;                    /* Set global for formatting. */
    }

    SRVN::Parseable::~Parseable()
    {
        ObjectOutput::__parseable = false;              /* Set global for formatting. */
    }

    std::ostream&
    SRVN::Parseable::print( std::ostream& output ) const
    {
        printPreamble( output );
        printResults( output );
        return output;
    }


    std::ostream&
    SRVN::Parseable::printPreamble( std::ostream& output ) const
    {
        output << "# " << io_vars.lq_toolname << " " << io_vars.lq_version << std::endl;
        if ( io_vars.lq_command_line.size() > 0 ) {
            output << "# " << io_vars.lq_command_line << ' ' << DOM::Document::__input_file_name << std::endl;
        }
	output << "# " << DOM::Common_IO::svn_id() << std::endl;
        const DOM::Document& document(getDOM());
        output << "V " << (document.getResultValid() ? "y" : "n") << std::endl
               << "C " << document.getResultConvergenceValue() << std::endl
               << "I " << document.getResultIterations() << std::endl
               << "PP " << document.getNumberOfProcessors() << std::endl
               << "NP " << document.getMaximumPhase() << std::endl << std::endl;

	if ( document.getSymbolExternalVariableCount() > 0 ) {
	    output << "# ";		/* Treat it as a comment. */
	    document.printExternalVariables( output );
	    output << std::endl << std::endl;
	}

        const std::map<std::string,std::string>& pragmas = _document.getPragmaList();
	if ( !pragmas.empty() ) {
	    for ( std::map<std::string,std::string>::const_iterator pragma = pragmas.begin(); pragma != pragmas.end(); ++pragma ) {
		output << "#pragma " << pragma->first;
		if ( !pragma->second.empty() ) output << "=" << pragma->second;
		output << std::endl;
	    }
	    output << std::endl;
	}

	const std::string comment( document.getModelComment() );
	if ( !comment.empty() ) {
	    output << "#!Comment: " << print_comment( comment ) << std::endl;
	}
	if ( document.getResultUserTime() > 0.0 ) {
	    output << "#!User: " << DOM::CPUTime::print( document.getResultUserTime() ) << std::endl;
	}
	if ( document.getResultSysTime() > 0.0 ) {
	    output << "#!Sys:  " << DOM::CPUTime::print( document.getResultSysTime() ) << std::endl;
	}
	output << "#!Real: " << DOM::CPUTime::print( document.getResultElapsedTime() ) << std::endl;
	if ( document.getResultMaxRSS() > 0 ) {
	    output << "#!MaxRSS: " << document.getResultMaxRSS() << std::endl;
	}
	const DOM::MVAStatistics& mva_info = document.getResultMVAStatistics();
	if ( mva_info.getNumberOfSubmodels() > 0 ) {
            output << "#!Solver: "
                   << mva_info.getNumberOfSubmodels() << ' '
                   << mva_info.getNumberOfCore() << ' '
                   << mva_info.getNumberOfStep() << ' '
                   << mva_info.getNumberOfStepSquared() << ' '
                   << mva_info.getNumberOfWait() << ' '
                   << mva_info.getNumberOfWaitSquared() << ' '
                   << mva_info.getNumberOfFaults() << std::endl;
        }

        output << std::endl;
        return output;
    }


    std::ostream&
    SRVN::Parseable::printResults( std::ostream& output ) const
    {
        std::ios_base::fmtflags oldFlags = output.setf( std::ios::left, std::ios::adjustfield );

        if ( getDOM().entryHasThroughputBound() ) {
            output << "B " << getDOM().getNumberOfEntries() << std::endl;
            std::for_each( _entities.begin(), _entities.end(), EntryOutput( output, &EntryOutput::printEntryThroughputBounds ) );
            output << "-1" << std::endl << std::endl;
        }

        /* Waiting times */

	const unsigned int count_w = std::for_each( _entities.begin(), _entities.end(), CallOutput( output, &DOM::Call::hasRendezvous ) ).getCount();
        if ( count_w > 0 ) {
            output << "W " << count_w << std::endl;
            std::for_each( _entities.begin(), _entities.end(), CallOutput( output, &DOM::Call::hasRendezvous, &CallOutput::printCallWaiting, &CallOutput::printCallWaitingConfidence ) );
            output << "-1" << std::endl << std::endl;

	    if ( getDOM().entryHasWaitingTimeVariance() && _print_variances ) {
                output << "VARW " << count_w << std::endl;
                std::for_each( _entities.begin(), _entities.end(), CallOutput( output, &DOM::Call::hasRendezvous, &CallOutput::printCallVarianceWaiting, &CallOutput::printCallVarianceWaitingConfidence ) );
                output << "-1" << std::endl << std::endl;
            }
        }

	const unsigned int count_f = std::for_each( _entities.begin(), _entities.end(), EntryOutput::CountForwarding() ).getCount();
	if ( count_f > 0 ) {
	    output << "F " << count_f << std::endl;
	    /* Ignore activities, but force end-of-list with NullActivity____ */
	    std::for_each( _entities.begin(), _entities.end(), EntryOutput( output, &EntryOutput::printForwardingWaiting, &EntryOutput::nullActivityFunc, &EntryOutput::nullActivityTest ) );
            output << "-1" << std::endl << std::endl;

	    if ( getDOM().entryHasWaitingTimeVariance() && _print_variances ) {
                output << "VARF " << count_w << std::endl;
		std::for_each( _entities.begin(), _entities.end(), EntryOutput( output, &EntryOutput::printForwardingVarianceWaiting, &EntryOutput::nullActivityFunc, &EntryOutput::nullActivityTest ) );
                output << "-1" << std::endl << std::endl;
            }
	}

	const unsigned int count_z = std::for_each( _entities.begin(), _entities.end(), CallOutput( output, &DOM::Call::hasSendNoReply ) ).getCount();
        if ( count_z > 0 ) {
            output << "Z " << count_z << std::endl;
            std::for_each( _entities.begin(), _entities.end(), CallOutput( output, &DOM::Call::hasSendNoReply, &CallOutput::printCallWaiting, &CallOutput::printCallWaitingConfidence ) );
            output << "-1" << std::endl << std::endl;

	    if ( getDOM().entryHasWaitingTimeVariance() && _print_variances ) {
                output << "VARZ " << count_z << std::endl;
                std::for_each( _entities.begin(), _entities.end(), CallOutput( output, &DOM::Call::hasSendNoReply, &CallOutput::printCallVarianceWaiting, &CallOutput::printCallVarianceWaitingConfidence ) );
                output << "-1" << std::endl << std::endl;
            }
	}

        /* Drop probabilities. */

        if ( getDOM().entryHasDropProbability() ) {
	    unsigned int count = std::for_each( _entities.begin(), _entities.end(), CallOutput( output, &DOM::Call::hasResultDropProbability ) ).getCount();
            output << "DP " << count << std::endl;
            std::for_each( _entities.begin(), _entities.end(), CallOutput( output, &DOM::Call::hasResultDropProbability, &CallOutput::printDropProbability, &CallOutput::printDropProbabilityConfidence ) );
            output << "-1" << std::endl << std::endl;
        }

        /* Join delays */

        if ( getDOM().taskHasAndJoin() ) {
            output << "J " << 0 << std::endl;
            std::for_each( _entities.begin(), _entities.end(), TaskOutput( output, &TaskOutput::printJoinDelay ) );
            output << "-1" << std::endl << std::endl;
        }

        /* Service time */
        output << "X " << getDOM().getNumberOfEntries() << std::endl;
        std::for_each( _entities.begin(), _entities.end(), EntryOutput( output, &EntryOutput::printEntryServiceTime, &EntryOutput::printActivityServiceTime) );
        output << "-1" << std::endl << std::endl;

        if ( getDOM().entryHasServiceTimeVariance() ) {
            output << "VAR " << getDOM().getNumberOfEntries() << std::endl;
            std::for_each( _entities.begin(), _entities.end(), EntryOutput( output, &EntryOutput::printEntryVarianceServiceTime, &EntryOutput::printActivityVarianceServiceTime  ) );
            output << "-1" << std::endl << std::endl;
        }

        if ( getDOM().hasMaxServiceTimeExceeded() ) {
            output << "E " << 0 << std::endl;
            std::for_each( _entities.begin(), _entities.end(), EntryOutput( output, &EntryOutput::printEntryMaxServiceTimeExceeded, &EntryOutput::printActivityMaxServiceTimeExceeded, &EntryOutput::testActivityMaxServiceTimeExceeded ) );
            output << "-1" << std::endl << std::endl;
        }

        /* Semaphore holding times */

        if ( getDOM().hasSemaphoreWait() ) {
            output << "H " << 0 << std::endl;
            std::for_each( _entities.begin(), _entities.end(), TaskOutput( output, &TaskOutput::printHoldTime ) );
            output << "-1" << std::endl << std::endl;
        }

        /* Rwlock holding times */

        if ( getDOM().hasRWLockWait() ) {
            output << "L " << 0 << std::endl;
            std::for_each( _entities.begin(), _entities.end(), TaskOutput( output, &TaskOutput::printRWLOCKHoldTime ) );
            output << "-1" << std::endl << std::endl;
        }

        /* Task Throughput and Utilization */

        output << "FQ " << getDOM().getNumberOfTasks() << std::endl;
        std::for_each( _entities.begin(), _entities.end(), TaskOutput( output, &TaskOutput::printThroughputAndUtilization ) );
        output << "-1"  << std::endl << std::endl;

	/* Open Arrivals */

        if ( getDOM().entryHasOpenWait() ) {
	    unsigned int count = std::count_if( _entities.begin(), _entities.end(), for_each_entry_count_if( &DOM::Entry::hasOpenArrivalRate ) );
            output << "R " << count << std::endl;
            std::for_each( _entities.begin(), _entities.end(), EntryOutput( output, &EntryOutput::printOpenQueueWait ) );
            output << "-1"  << std::endl << std::endl;
        }

        /* Processor utilization and waiting */

        std::for_each( _entities.begin(), _entities.end(), ProcessorOutput( output, &ProcessorOutput::printUtilizationAndWaiting ) );
        output << "-1"  << std::endl << std::endl;

        output.flags(oldFlags);
        return output;
    }

    /*
     * Strip newlines and other funny characters
     */

    /* static */ std::ostream&
    SRVN::Parseable::printComment( std::ostream& output, const std::string& s )
    {
	for ( std::string::const_iterator p = s.begin(); p != s.end() ; ++p ) {	/* Handle comments */
	    if ( *p == '\n' ) {
		output << "\\n";
	    } else {
		output << *p;
	    }
	}
	return output;
    }

    /* ---------------------------------------------------------------- */
    /* RTF Output                                                       */
    /* ---------------------------------------------------------------- */

    SRVN::RTF::RTF( const DOM::Document& document, bool print_confidence_intervals )
        : SRVN::Output( document, print_confidence_intervals )
    {
        ObjectOutput::__parseable = false;              /* Set global for formatting. */
        ObjectOutput::__rtf = true;                     /* Set global for formatting. */
    }

    SRVN::RTF::~RTF()
    {
        ObjectOutput::__rtf = false;                    /* Set global for formatting. */
    }

    std::ostream&
    SRVN::RTF::printPreamble( std::ostream& output ) const
    {
        output << "{\\rtf1\\ansi\\ansicpg1252\\cocoartf1138\\cocoasubrtf230" << std::endl            // Boilerplate.
               << "{\\fonttbl\\f0\\fmodern\\fcharset0 CourierNewPSMT;\\f1\\fmodern\\fcharset0 CourierNewPS-BoldMT;\\f2\\fmodern\\fcharset0 CourierNewPS-ItalicMT;}" << std::endl     // Fonts (f0, f1... )
               << "{\\colortbl;\\red255\\green255\\blue255;\\red255\\green0\\blue0;\\red255\\green164\\blue0;\\red0\\green255\\blue0;\\red0\\green0\\blue255;}" << std::endl         // Colour table. (black, white, red).
               << "\\vieww15500\\viewh10160\\viewkind0" << std::endl
               << "\\pard\\tx560\\tx1120\\tx1680\\tx2240\\tx2800\\tx3360\\tx3920\\tx4480\\tx5040\\tx5600\\tx6160\\tx6720\\pardirnatural" << std::endl
               << "\\f0\\fs24 \\cf0 ";

        DocumentOutput print( output );
        print( getDOM() );
        return output;
    }

    std::ostream&
    SRVN::RTF::printPostamble( std::ostream& output ) const
    {
        output << "}" << std::endl;
        return output;
    }

    /* ---------------------------------------------------------------- */
    /* Input Output                                                     */
    /* ---------------------------------------------------------------- */

    SRVN::Input::Input( const DOM::Document& document, bool annotate )
        : _document(document), _entities(document.getEntities()), _annotate(annotate)
    {
        /* Initialize lengths */
        const std::map<std::string,DOM::Entry*>& entries = document.getEntries();
        ObjectInput::__maxEntLen = 1;           /* default */
        ObjectInput::__maxInpLen = 1;           /* default */

        for ( std::map<std::string,DOM::Entry*>::const_iterator e = entries.begin(); e != entries.end(); ++e ) {
            unsigned long size = e->first.size();
            if ( size > ObjectInput::__maxEntLen ) ObjectInput::__maxEntLen = size;

            const DOM::Entry * entry = e->second;
            const std::map<unsigned, DOM::Phase*>& phases = entry->getPhaseList();
            assert( phases.size() <= DOM::Phase::MAX_PHASE );
            for (std::map<unsigned, DOM::Phase*>::const_iterator p = phases.begin(); p != phases.end(); ++p) {
                std::ostringstream s;
                const DOM::Phase* phase = p->second;
                if ( !phase ) continue;
                const DOM::ExternalVariable * var = phase->getServiceTime();
                if ( !var ) continue;
                s << *var;
                size = s.str().size();
                if ( size > ObjectInput::__maxInpLen ) ObjectInput::__maxInpLen = size;
            }
        }
    }

    SRVN::Input::~Input()
    {
    }

    /* Echo of input. */
    std::ostream&
    SRVN::Input::print( std::ostream& output ) const
    {
        printHeader( output );

        if ( !Spex::input_variables().empty() && !_document.instantiated() ) {
            output << std::endl;
	    if ( _annotate ) {
 		output << "# SPEX Variable definition and initialization." << std::endl
		       << "# SYNTAX-FORM-A: $var = spex_expr" << std::endl
		       << "#   spex_expr may contain operators, variables and constants." << std::endl
		       << "# SYNTAX-FORM-B: $var = [constant_1, constant_2, ...]" << std::endl
		       << "#   The solver will iterate over constant_1, constant_2, ..." << std::endl
		       << "#   Arrays nest, so multiple arrays will generate a factorial experiment." << std::endl
		       << "# SYNTAX-FORM-C: $var = [constant_1 : constant_2, constant_3]" << std::endl
		       << "#   The solver will iterate from constant_1 to constant_2 with a step size of constant_3" << std::endl
		       << "# SYNTAX-FORM-D: $index, $var = spex_expr" << std::endl
		       << "#   $index is a variable defined using SYNTAX-FORM-B or C.  $index may or may not appear in spex_expr." << std::endl
		       << "#   This form is used to allow one array to change multiple variables." << std::endl;
	    }
	    const std::map<std::string,LQX::SyntaxTreeNode *>& input_variables = Spex::input_variables();
	    LQX::SyntaxTreeNode::setVariablePrefix( "$" );
	    std::for_each( input_variables.begin(), input_variables.end(), Spex::PrintInputVariable( output ) );
        }

        printGeneral( output );

        const unsigned int n_proc = std::count_if( _entities.begin(), _entities.end(), is_processor );
        output << std::endl << "P " << n_proc << std::endl;
        if ( _annotate ) {
            output << "# SYNTAX: p ProcessorName SchedDiscipline [flags]" << std::endl
                   << "#   ProcessorName is any string, globally unique among processors." << std::endl
                   << "#   SchedDiscipline = f {fifo}" << std::endl
                   << "#                   | r {random}" << std::endl
                   << "#                   | p {premptive}" << std::endl
                   << "#                   | h {hol (or non-pre-emptive priority)}" << std::endl
                   << "#                   | s <real> {processor-sharing (or round-robin) with quantum} " << std::endl
                   << "#   flags = m <int> {multiprocessor}" << std::endl
                   << "#         | i {infinite or delay server}" << std::endl
		   << "#         | R <real> {rate multiplier}" << std::endl;
        }
        std::for_each( _entities.begin(), _entities.end(), ProcessorInput( output, &ProcessorInput::print ) );
        output << -1 << std::endl;


        const std::map<std::string,DOM::Group*>& groups = _document.getGroups();
        if ( groups.size() > 0 ) {
            output << std::endl << "U " << groups.size() << std::endl;
            if ( _annotate ) {
                output << "# SYNTAX: g GroupName share cap ProcessorName" << std::endl;
            }
            std::for_each( groups.begin(), groups.end(), GroupInput( output, &GroupInput::print ) );
            output << -1 << std::endl;
        }

        const unsigned int n_task = std::count_if( _entities.begin(), _entities.end(), is_task );
        output << std::endl << "T " << n_task << std::endl;
        if ( _annotate ) {
            output << "# SYNTAX: t TaskName TaskType EntryList -1 ProcessorName [flags]" << std::endl
                   << "#   TaskName is any string, globally unique among tasks." << std::endl
                   << "#   TaskType = r {reference or user task}" << std::endl
                   << "#            | n {other} " << std::endl
                   << "#   flags = m <int> {multithreaded}" << std::endl
                   << "#         | i {infinite or delay server}" << std::endl
		   << "#         | z <real> {think time}" << std::endl
		   << "#         | <int> {task priority}" << std::endl;
        }
        std::for_each( _entities.begin(), _entities.end(), TaskInput( output, &TaskInput::print ) );
        output << -1 << std::endl;

        output << std::endl << "E " << _document.getNumberOfEntries() << std::endl;
        if ( _annotate ) {
            output << "# SYNTAX-FORM-A: Token EntryName Value1 [Value2] [Value3] -1" << std::endl
                   << "#   EntryName is a string, globally unique over all entries " << std::endl
                   << "#   Values are for phase 1, 2 and 3 {phase 1 is before the reply} " << std::endl
                   << "#   Token indicate the significance of the Value: " << std::endl
                   << "#       s - HostServiceDemand for EntryName " << std::endl
                   << "#       c - HostServiceCoefficientofVariation" << std::endl
                   << "#       f - PhaseTypeFlag" << std::endl
                   << "# SYNTAX-FORM-B: Token FromEntry ToEntry Value1 [Value2] [Value3] -1" << std::endl
                   << "#   Token indicate the Value Definitions: " << std::endl
                   << "#       y - SynchronousCalls {no. of rendezvous} " << std::endl
                   << "#       F - ProbForwarding {forward to ToEntry rather than replying} " << std::endl
                   << "#       z - AsynchronousCalls {no. of send-no-reply messages} " << std::endl;
        }
        std::for_each( _entities.begin(), _entities.end(), TaskInput( output, &TaskInput::printEntryInput ) );
        output << -1 << std::endl;

        std::for_each( _entities.begin(), _entities.end(), TaskInput( output, &TaskInput::printActivityInput ) );

	printResultConvergenceVariables( output, LQIO::Spex::result_variables(), "R" );
	printResultConvergenceVariables( output, LQIO::Spex::convergence_variables(), "C" );

        return output;
    }

    /*
     * Print out the "general information" for srvn input output.
     */

    std::ostream&
    SRVN::Input::printHeader( std::ostream& output ) const
    {
        output << "# SRVN Model Description File, for file: " << DOM::Document::__input_file_name << std::endl
               << "# Generated by: " << io_vars.lq_toolname << ", version " << io_vars.lq_version << std::endl;
	const DOM::GetLogin login;
	output << "# For: " << login << std::endl;
#if HAVE_CTIME
        time_t tloc;
        time( &tloc );
        output << "# " << ctime( &tloc );
#endif
        output << "# Invoked as: " << io_vars.lq_command_line << ' ' << DOM::Document::__input_file_name << std::endl
	       << "# " << DOM::Common_IO::svn_id() << std::endl
	       << "# " << std::setfill( '-' ) << std::setw( 72 ) << '-' << std::setfill( ' ' ) << std::endl;
        if ( !_document.getResultComment().empty() ) {
            output << "# " << _document.getResultComment() << std::endl;
        }

        const std::map<std::string,std::string>& pragmas = _document.getPragmaList();
        if ( !pragmas.empty() ) {
            output << std::endl;
            for ( std::map<std::string,std::string>::const_iterator pragma = pragmas.begin(); pragma != pragmas.end(); ++pragma ) {
                output << "#pragma " << pragma->first;
		if ( !pragma->second.empty() ) output << "=" << pragma->second;
		output << std::endl;
            }
        }
        return output;
    }

    /*
     * Print general information. (Print default values if set by spex)
     */

    std::ostream&
    SRVN::Input::printGeneral( std::ostream& output ) const
    {
        output << std::endl << "G \"" << _document.getModelComment() << "\" " ;
        if ( _annotate ) {
            output << "\t\t\t# Model comment " << std::endl
                   << *_document.getModelConvergence() << "\t\t\t# Convergence test value." << std::endl
                   << *_document.getModelIterationLimit() << "\t\t\t# Maximum number of iterations." << std::endl
                   << *_document.getModelPrintInterval() << "\t\t\t# Print intermediate results (see manual pages)" << std::endl
                   << *_document.getModelUnderrelaxationCoefficient() << "\t\t\t# Model under-relaxation ( 0.0 < x <= 1.0)" << std::endl
                   << -1;
        } else {
            output << *_document.getModelConvergence() << " "
                   << *_document.getModelIterationLimit() << " "
                   << *_document.getModelPrintInterval() << " "
                   << *_document.getModelUnderrelaxationCoefficient() << " "
                   << -1;
        }
        if ( !_document.instantiated() ) {
	    for ( std::vector<Spex::ObservationInfo>::const_iterator obs = Spex::document_variables().begin(); obs != Spex::document_variables().end(); ++obs ) {
		output << " ";
		obs->print( output );
	    }
        }
        output << std::endl;
        return output;
    }

    /*
     * Common code for R and C sections
     */
	
    std::ostream& SRVN::Input::printResultConvergenceVariables( std::ostream& output, const std::vector<std::pair<const std::string,LQX::SyntaxTreeNode *>>& variables, const std::string& keyword ) const
    {
        const unsigned int n = variables.size();
        if ( n > 0 && !_document.instantiated() ) {
            output << std::endl << keyword << " " << n << std::endl;
	    if ( _annotate ) {
		output << "# SYNTAX: spex-expr" << std::endl
		       << "#   Any variable defined earlier can be used." << std::endl;
	    }
	    LQX::SyntaxTreeNode::setVariablePrefix( "$" );
	    std::for_each( variables.begin(), variables.end(), Spex::PrintVarNameAndExpr( output, 2 ) );
            output << "-1" << std::endl;
        }
	return output;
    }

    bool
    SRVN::Input::is_processor( const DOM::Entity * entity )
    {
        return dynamic_cast<const DOM::Processor *>(entity) != nullptr;
    }

    bool
    SRVN::Input::is_task( const DOM::Entity * entity )
    {
        return dynamic_cast<const DOM::Task *>(entity) != nullptr;
    }

    /*
     * Print out the variable, var.  If it's a variable, print out the name.  If the variable has been set, print the value, but only
     * if the value is valid.
     */
    
    /* static */ std::ostream&
    SRVN::Input::doubleAndNotEqual( std::ostream& output, const std::string& str, const DOM::ExternalVariable * var, double default_value )
    {
        if ( !var ) {
	    return output;
	} else if ( !var->wasSet() ) {
	    output << str << print_double_parameter( var );
	} else {
	    double value;
	    if ( !var->getValue(value) ) throw std::domain_error( "not a number" );
	    if ( std::isinf(value) ) throw std::domain_error( "infinity" );
	    if ( value > 0.0 && value != default_value ) {
		output << str << *var;		/* Ignore if it's the default */
	    }
        }
	return output;
    }

    /*
     * Print out the variable, var.  If it's a variable, print out the name.  If the variable has been set, print the value, but only
     * if the value is valid.
     */
    
    /* static */ std::ostream&
    SRVN::Input::integerAndGreaterThan( std::ostream& output, const std::string& str, const DOM::ExternalVariable * var, double floor_value )
    {
        if ( !var ) {
	    return output;
	} else if ( !var->wasSet() ) {
	    output << str << print_integer_parameter( var, floor_value );
	} else {
	    double value;
	    if ( !var->getValue(value) ) throw std::domain_error( "not a number" );
	    if ( std::isinf(value) ) throw std::domain_error( "infinity" );
	    if ( value != rint(value) ) throw std::domain_error( "invalid integer" );
	    if ( value < floor_value ) {
		std::stringstream ss;
		ss << value << " < " << floor_value;
		throw std::domain_error( ss.str() );
	    }
	    if ( value > floor_value ) {
		output << str << *var;
	    }
        }
	return output;
    }

    /* static */ std::ostream&
    SRVN::Input::printDoubleExtvarParameter( std::ostream& output, const DOM::ExternalVariable * var, double floor_value )
    {
	double value = 0;
	if ( !var || (var->wasSet() && var->getValue(value)) ) {
	    if ( std::isinf(value) ) throw std::domain_error( "infinity" );
	    if ( value < floor_value ) {
		std::stringstream ss;
		ss << value << " < " << floor_value;
		throw std::domain_error( ss.str() );
	    }
	    output << value;
	} else {
	    std::map<const DOM::ExternalVariable *, const LQX::SyntaxTreeNode *>::const_iterator vp = Spex::inline_expressions().find( var );
	    if ( vp != Spex::inline_expressions().end() ) {
		output << "{ ";
		vp->second->print( output );
		output << " }";
	    } else {
		output << *var;
	    }
	}
	return output;
    }

    /* static */ std::ostream&
    SRVN::Input::printIntegerExtvarParameter( std::ostream& output, const DOM::ExternalVariable * var, double floor_value )
    {
	double value = 0;
	if ( !var || (var->wasSet() && var->getValue(value)) ) {
	    if ( std::isinf(value) ) throw std::domain_error( "infinity" );
	    if ( value != rint(value) ) throw std::domain_error( "invalid integer" );
	    if ( value < floor_value ) {
		std::stringstream ss;
		ss << value << " < " << floor_value;
		throw std::domain_error( ss.str() );
	    }
	    output << value;
	} else {
	    std::map<const DOM::ExternalVariable *, const LQX::SyntaxTreeNode *>::const_iterator vp = Spex::inline_expressions().find( var );
	    if ( vp != Spex::inline_expressions().end() ) {
		output << "{ ";
		vp->second->print( output );
		output << " }";
	    } else {
		output << *var;
	    }
	}
	return output;
    }

    /* -------------------------------------------------------------------- */
    /* Document                                                             */
    /* -------------------------------------------------------------------- */

    void
    SRVN::DocumentOutput::operator()( const DOM::Document& document ) const
    {
        _output << "Generated by: " << io_vars.lq_toolname << ", version " << io_vars.lq_version << newline
                << textit
                << "Copyright the Real-Time and Distributed Systems Group," << newline
                << "Department of Systems and Computer Engineering" << newline
                << "Carleton University, Ottawa, Ontario, Canada. K1S 5B6" << newline
                << textrm << newline;

        if ( io_vars.lq_command_line.size() > 0 ) {
            _output << "Invoked as: " << io_vars.lq_command_line << ' ' << DOM::Document::__input_file_name << newline;
        }
        _output << "Input:  " << DOM::Document::__input_file_name << newline;
#if HAVE_CTIME
        time_t clock = time( (time_t *)0 );
        _output << ctime( &clock ) << newline;
#endif

	const std::string& comment = document.getModelComment();
        if ( !comment.empty() ) {
            _output << "Comment: " << comment << newline;
        }
        if ( !document.getResultComment().empty() ) {
            _output << "Other:   " << document.getResultComment() << newline;
        }
        if ( document.getSymbolExternalVariableCount() > 0 ) {
            _output << "Variables: ";
            document.printExternalVariables( _output ) << newline;
        }
        _output << newline
                << "Convergence test value: " << document.getResultConvergenceValue() << newline
                << "Number of iterations:   " << document.getResultIterations() << newline;
        _output << newline;

        const std::map<std::string,std::string>& pragmas = document.getPragmaList();
        if ( pragmas.size() ) {
            _output << "Pragma" << (pragmas.size() > 1 ? "s:" : ":") << newline;
            for ( std::map<std::string,std::string>::const_iterator pragma = pragmas.begin(); pragma != pragmas.end(); ++pragma ) {
                _output << "    " << pragma->first;
		if ( !pragma->second.empty() ) _output << "=" << pragma->second;
		_output << newline;
            }
            _output << newline;
        }

        _output << "Solver: " << document.getResultPlatformInformation() << newline;
	if ( document.getResultUserTime() > 0.0 ) {
	    _output << "    User:       " << DOM::CPUTime::print( document.getResultUserTime() ) << newline;
	}
	if ( document.getResultSysTime() > 0.0 ) {
	    _output << "    System:     " << DOM::CPUTime::print( document.getResultSysTime() ) << newline;
	}
	_output << "    Elapsed:    " << DOM::CPUTime::print( document.getResultElapsedTime() ) << newline
                << newline;
	if ( document.getResultMaxRSS() > 0 ) {
	    _output << "    MaxRSS:     " << document.getResultMaxRSS() << newline;
	}
	_output << newline;

	const DOM::MVAStatistics& mva_info = document.getResultMVAStatistics();
	if ( mva_info.getNumberOfSubmodels() > 0 ) {
            _output << "    Submodels:  " << mva_info.getNumberOfSubmodels() << newline
                    << "    MVA Core(): " << std::setw(ObjectOutput::__maxDblLen) << mva_info.getNumberOfCore() << newline
                    << "    MVA Step(): " << std::setw(ObjectOutput::__maxDblLen) << mva_info.getNumberOfStep() << newline
//                                     mva_info.getNumberOfStepSquared() <<
                    << "    MVA Wait(): " << std::setw(ObjectOutput::__maxDblLen) << mva_info.getNumberOfWait() << newline;
//                                     mva_info.getNumberOfWaitSquared() <<
            if ( mva_info.getNumberOfFaults() ) {
                _output << "    *** Faults *** " << mva_info.getNumberOfFaults() << newline;
            }
        }
    }

    /* -------------------------------------------------------------------- */
    /* Entities                                                             */
    /* -------------------------------------------------------------------- */

    void
    SRVN::EntityOutput::operator()( const DOM::Entity * entity ) const
    {
        std::ios_base::fmtflags oldFlags = _output.setf( std::ios::left, std::ios::adjustfield );
        (this->*_func)( *entity );
        _output.flags(oldFlags);
    }
    
    void
    SRVN::EntityOutput::printCommonParameters( const DOM::Entity& entity ) const
    {
        bool print_task_name = true;
        _output << entity_name( entity, print_task_name );
        std::ostringstream myType;
        if ( entity.isInfinite() ) {
            myType << "Inf";
        } else if ( entity.getSchedulingType() == SCHEDULE_CUSTOMER ) {
            myType << "Ref("  << entity.getCopiesValue() << ")";
        } else if ( entity.getSchedulingType() == SCHEDULE_SEMAPHORE ) {
            myType << "Sema("  << entity.getCopiesValue() << ")";
        } else if ( entity.getSchedulingType() == SCHEDULE_RWLOCK ) {
            myType << "rw("  << entity.getCopiesValue() << ")";
        } else if ( entity.isMultiserver() ) {
            myType << "Mult(" << entity.getCopiesValue() << ")";
        } else {
            myType << "Uni";
        }
        _output << std::setw(7) << myType.str() << " " << std::setw(6) << std::right << entity.getReplicasValue() << "  " << std::left;
    }

#if defined(BUG_393)
    void
    SRVN::EntityOutput::printMarginalQueueProbabilities( const DOM::Entity& entity ) const
    {
	const std::vector<double>& marginals = entity.getResultMarginalQueueProbabilities();
	if ( marginals.empty() || __parseable ) return;

	_output << newline << newline << "Marginal Queue Probabilities for processor: " << entity.getName() << newline;
	for ( unsigned int i = 0; i < marginals.size(); ++i ) {
	    if ( i != 0 ) _output << ", ";
	    _output << "P_" << i << "=" << marginals.at(i);
	}
	_output << newline;
    }
#endif

    std::ostream&
    SRVN::EntityInput::print( std::ostream& output, const DOM::Entity * entity )
    {
        if ( dynamic_cast<const DOM::Task *>(entity) ) {
            LQIO::SRVN::TaskInput( output, 0 ).print( *dynamic_cast<const DOM::Task *>(entity) );
        } else {
            LQIO::SRVN::ProcessorInput( output, 0 ).print( *dynamic_cast<const DOM::Processor *>(entity) );
        }
        return output;
    }

    /* -------------------------------------------------------------------- */
    /* Processors                                                           */
    /* -------------------------------------------------------------------- */

    void
    SRVN::ProcessorOutput::operator()( const DOM::Entity * entity ) const
    {
	const DOM::Processor * processor = dynamic_cast<const DOM::Processor *>(entity);
        if ( !processor ) return;

        std::ios_base::fmtflags oldFlags = _output.setf( std::ios::left, std::ios::adjustfield );
        (this->*_func)( *processor );
        _output.flags(oldFlags);
    }


    void
    SRVN::ProcessorOutput::printParameters( const DOM::Processor& processor ) const
    {
        const std::ios_base::fmtflags oldFlags = _output.setf( std::ios::left, std::ios::adjustfield );
        SRVN::EntityOutput::printCommonParameters( processor );
	if ( processor.isInfinite() ) {
	    _output << scheduling_label.at(SCHEDULE_DELAY).str;
	} else {
	    _output << scheduling_label.at(processor.getSchedulingType()).str;
	    if ( processor.hasRate() ) {
		_output << " " << Input::print_double_parameter( processor.getRate() );
	    }
	}
        _output << newline;
        _output.flags(oldFlags);
    }

    void
    SRVN::ProcessorOutput::printUtilizationAndWaiting( const DOM::Processor& processor ) const
    {
        const std::set<DOM::Task*>& tasks = processor.getTaskList();
        const std::set<DOM::Group*>& groups = processor.getGroupList();
        const double rate = processor.hasRate() ? processor.getRateValue() : 1.0;
        const double proc_util = !processor.isInfinite() ? processor.getResultUtilization() / (static_cast<double>(processor.getCopiesValue()) * rate): 0;

        if ( __parseable ) {
            _output << "P " << processor.getName() << " " << tasks.size() << newline;
        } else {
            _output << newline << newline << textbf << utilization_str << ' ' << textrm << colour( proc_util )
                    << processor.getName() << colour( 0 )
                    << newline << newline
                    << std::setw(__maxStrLen) << "Task Name"
                    << "Pri n   "
                    << std::setw(__maxStrLen) << "Entry Name"
                    << "Utilization "
                    << Output::phase_header( __maxPhase ) << newline;
        }

        /* groups go here... */
        if ( groups.size() > 0 ) {
            for ( std::set<DOM::Group*>::const_iterator nextGroup = groups.begin(); nextGroup != groups.end(); ++nextGroup ) {
                const DOM::Group& group = *(*nextGroup);
                printUtilizationAndWaiting( processor, group.getTaskList() );
		std::string total = "Group ";
                if ( __parseable ) {
		    total = "G ";
		    total += group.getName();
		} else {
		    total = "Group ";
		    total += group.getName();
		    total += " Total:";
		}
		_output << textit << std::setw(__maxStrLen*2+8) << total
			<< std::setw(__maxDblLen-1) << group.getResultUtilization() << ' ' << newline;
		if ( __conf95 ) {
		    _output << conf_level(__maxStrLen*2+8,ConfidenceIntervals::CONF_95)
			    << std::setw(__maxDblLen-1) << (*__conf95)(group.getResultUtilizationVariance()) << ' ' << newline;
		}
		if ( __conf99 ) {
		    _output << conf_level(__maxStrLen*2+8,ConfidenceIntervals::CONF_99)
			    << std::setw(__maxDblLen-1) << (*__conf99)(group.getResultUtilizationVariance()) << ' ' << newline;
		}
		_output << textrm;
	    }
        } else {
            printUtilizationAndWaiting( processor, tasks );
        }

        /* Total for processor */

        if ( tasks.size() > 1 ) {
            if ( __parseable ) { _output << std::setw( __maxStrLen ) << " " << activityEOF << newline; }
            _output << textit << colour( proc_util )<< std::setw(__maxStrLen*2+8) << ( __parseable ? " " : "Processor Total:" )
                    << processor.getResultUtilization()  << newline;
            if ( __conf95 ) {
                _output << conf_level(__maxStrLen*2+8,ConfidenceIntervals::CONF_95)
                        << (*__conf95)(processor.getResultUtilizationVariance()) << newline;
            }
            if ( __conf99 ) {
                _output << conf_level(__maxStrLen*2+8,ConfidenceIntervals::CONF_99)
                        << (*__conf99)(processor.getResultUtilizationVariance()) << newline;
            }
            _output << textrm << colour( 0 );
        }
        if ( __parseable ) { _output << activityEOF << newline << newline; }
    }



    void
    SRVN::ProcessorOutput::printUtilizationAndWaiting( const DOM::Processor& processor, const std::set<DOM::Task*>& tasks ) const
    {
        const bool is_infinite = processor.isInfinite();
	const double copies = is_infinite ? 1.0 : static_cast<double>(processor.getCopiesValue());
	const double rate   = processor.hasRate() ? processor.getRateValue() : 1.0;

        for ( std::set<DOM::Task*>::const_iterator nextTask = tasks.begin(); nextTask != tasks.end(); ++nextTask ) {
            const DOM::Task * task = *nextTask;
            bool print_task_name = true;
            unsigned int item_count = 0;
	    const unsigned int multiplicity = task->isInfinite() ? 1 : task->getCopiesValue();

            const std::vector<DOM::Entry *> & entries = task->getEntryList();
            for ( std::vector<DOM::Entry *>::const_iterator nextEntry = entries.begin(); nextEntry != entries.end(); ++nextEntry, ++item_count ) {
                const DOM::Entry * entry = *nextEntry;
                const double entry_util = is_infinite ? 0 : entry->getResultProcessorUtilization() / (copies * rate);
                _output << colour( entry_util );
                if ( __parseable ) {
                    if ( print_task_name ) {
                        _output << std::setw( __maxStrLen-1 ) << task->getName() << " "
                                << std::setw(2) << entries.size() << " "
                                << std::setw(1) << task->getPriorityValue() << " "
                                << std::setw(2) << multiplicity << " ";
                        print_task_name = false;
                    } else {
                        _output << std::setw(__maxStrLen+8) << " ";
                    }
                } else if ( print_task_name ) {
                    _output << entity_name( *task, print_task_name )
                            << std::setw(3) << task->getPriorityValue() << " "
                            << std::setw(3) << multiplicity << " ";
                } else {
                    _output << std::setw(__maxStrLen+8) << " ";
                }
                _output << entry_name( *entry )
                        << std::setw(__maxDblLen-1) << entry->getResultProcessorUtilization() << " "
                        << processor_queueing_time( *entry ) << newline;
                if ( __conf95 ) {
                    _output << conf_level(__maxStrLen*2+8,ConfidenceIntervals::CONF_95)
                            << std::setw(__maxDblLen-1) << (*__conf95)(entry->getResultProcessorUtilizationVariance()) << " "
                            << std::setw(__maxDblLen-1) << processor_queueing_time_confidence( *entry, __conf95 ) << newline;
                }
                if ( __conf99 ) {
                    _output << conf_level(__maxStrLen*2+8,ConfidenceIntervals::CONF_99)
                            << std::setw(__maxDblLen-1) << (*__conf99)(entry->getResultProcessorUtilizationVariance()) << " "
                            << std::setw(__maxDblLen-1) << processor_queueing_time_confidence( *entry, __conf99 ) << newline;
                }
                _output << colour( 0 );
            }

            const std::map<std::string,DOM::Activity*>& activities = task->getActivities();
            if ( activities.size() > 0 ) {
                _output << activity_separator(__maxStrLen+8) << newline;
                std::map<std::string,DOM::Activity*>::const_iterator nextActivity;
                for ( nextActivity = activities.begin(); nextActivity != activities.end(); ++nextActivity, ++item_count ) {
                    const DOM::Activity * activity = nextActivity->second;
                    _output << std::setw(__maxStrLen+8) << " "
                            << std::setw(__maxStrLen-1) << activity->getName() << ' '
                            << std::setw(__maxDblLen-1) << activity->getResultProcessorUtilization() << ' '
                            << std::setw(__maxDblLen-1) << activity->getResultProcessorWaiting() << ' '
                            << activityEOF << newline;
                    if ( __conf95 ) {
                        _output << conf_level(__maxStrLen*2+8,ConfidenceIntervals::CONF_95)
                                << std::setw(__maxDblLen-1) << (*__conf95)(activity->getResultProcessorUtilizationVariance()) << ' '
                                << std::setw(__maxDblLen-1) << (*__conf95)(activity->getResultProcessorWaitingVariance()) << ' '
                                << activityEOF << newline;
                    }
                    if ( __conf99 ) {
                        _output << conf_level(__maxStrLen*2+8,ConfidenceIntervals::CONF_99)
                                << std::setw(__maxDblLen-1) << (*__conf99)(activity->getResultProcessorUtilizationVariance()) << ' '
                                << std::setw(__maxDblLen-1) << (*__conf99)(activity->getResultProcessorWaitingVariance()) << ' '
                                << activityEOF << newline;
                    }
                }
            }
            if ( __parseable ) {
                _output << std::setw(__maxStrLen+8) << " " << activityEOF << newline;
            }

            /* Total for task */

            if ( item_count > 1 ) {
                const double task_util = !processor.isInfinite() ? task->getResultProcessorUtilization() / (copies * rate): 0;
                _output << textit << colour( task_util );
                _output << std::setw(__maxStrLen*2+8) << ( __parseable ? " " : "Task Total:" )
                        << std::setw(__maxDblLen-1) << task->getResultProcessorUtilization() << ' ' << newline;
                if ( __conf95 ) {
                    _output << conf_level(__maxStrLen*2+8,ConfidenceIntervals::CONF_95)
                            << std::setw(__maxDblLen-1) << (*__conf95)(task->getResultProcessorUtilizationVariance()) << ' ' << newline;
                }
                if ( __conf99 ) {
                    _output << conf_level(__maxStrLen*2+8,ConfidenceIntervals::CONF_99)
                            << std::setw(__maxDblLen-1) << (*__conf99)(task->getResultProcessorUtilizationVariance()) << ' ' << newline;
                }
                _output << colour( 0 ) << textrm;
            }
        }
    }

    void
    SRVN::ProcessorInput::operator()( const DOM::Entity * entity) const
    {
        const DOM::Processor * processor = dynamic_cast<const DOM::Processor *>(entity);
        if ( !processor ) return;

        std::ios_base::fmtflags oldFlags = _output.setf( std::ios::left, std::ios::adjustfield );
        (this->*_func)( *processor );
        _output.flags(oldFlags);
    }


    void
    SRVN::ProcessorInput::print( const DOM::Processor& processor ) const
    {
        if ( processor.getTaskList().size() == 0 ) return;
        _output << "  p " << processor.getName()
                << scheduling_of( processor )
                << copies_of( processor )
                << replicas_of( processor )
                << rate_of( processor );
        if ( !processor.getDocument()->instantiated() ) {
            printObservationVariables( _output, processor );
        }
        _output << std::endl;
    }

    std::ostream&
    SRVN::ProcessorInput::printCopies( std::ostream& output, const DOM::Processor & processor )
    {
	if ( !processor.isInfinite() ) {
	    try {
		output << Input::is_integer_and_gt( " m ", processor.getCopies(), 1.0 );
	    }
	    catch ( const std::domain_error& e ) {
		processor.throw_invalid_parameter( "multiplicity", e.what() );
	    }
	}
        return output;
    }

    std::ostream&
    SRVN::ProcessorInput::printReplicas( std::ostream& output, const DOM::Processor & processor )
    {
	try {
	    output << Input::is_integer_and_gt( " r ", processor.getReplicas(), 1.0 );
	}
	catch ( const std::domain_error& e ) {
	    processor.throw_invalid_parameter( "replicas", e.what() );
	}
        return output;
    }

    std::ostream&
    SRVN::ProcessorInput::printRate( std::ostream& output, const DOM::Processor& processor )
    {
	try {
	    output << Input::is_double_and_ne( " R ", dynamic_cast<const DOM::Processor&>(processor).getRate(), 1.0 );
	}
	catch ( const std::domain_error& e ) {
	    processor.throw_invalid_parameter( "rate", e.what() );
	}
        return output;
    }


    std::ostream&
    SRVN::ProcessorInput::printScheduling( std::ostream& output, const DOM::Processor & processor )
    {
	output << " ";
	if ( processor.isInfinite() ) {
	    output << scheduling_label.at(SCHEDULE_DELAY).flag;
	} else {
	    output << scheduling_label.at(processor.getSchedulingType()).flag;
	    if ( processor.hasQuantumScheduling() ) {
		output << ' ' << Input::print_double_parameter( processor.getQuantum() );
	    }
	}
        return output;
    }


    /* -------------------------------------------------------------------- */
    /* Groups                                                               */
    /* -------------------------------------------------------------------- */

    void
    SRVN::GroupOutput::operator()( const std::pair<const std::string,DOM::Group*>& group ) const
    {
        std::ios_base::fmtflags oldFlags = _output.setf( std::ios::left, std::ios::adjustfield );
        (this->*_func)( *(group.second) );
        _output.flags(oldFlags);
    }

    void
    SRVN::GroupOutput::printParameters( const DOM::Group& group ) const
    {
        const std::ios_base::fmtflags oldFlags = _output.setf( std::ios::left, std::ios::adjustfield );
        _output << std::setw(__maxStrLen-1) << group.getName()
                << " " << std::setw(6) << Input::print_double_parameter( group.getGroupShare(), 0. );
        if ( group.getCap() ) {
            _output << " cap  ";
        } else {
            _output << "      ";
        }
        _output << std::setw(__maxStrLen) << group.getProcessor()->getName()
                << newline;

        _output.flags(oldFlags);
    }


//    void
//    SRVN::GroupOutput::printUtilization( const DOM::Group& group ) const
//    {
//    }

    void
    SRVN::GroupInput::operator()( const std::pair<const std::string,DOM::Group*>& group ) const
    {
        std::ios_base::fmtflags oldFlags = _output.setf( std::ios::left, std::ios::adjustfield );
        (this->*_func)( *(group.second) );
        _output.flags(oldFlags);
    }

    void
    SRVN::GroupInput::print( const DOM::Group& group ) const
    {
        _output << "  g " << group.getName()
                << " " << Input::print_double_parameter( group.getGroupShare() );
        if ( group.getCap() ) {
            _output << " c";
        }
        const DOM::Processor * proc = group.getProcessor();
        _output << " " << proc->getName() << std::endl;
    }

    /* -------------------------------------------------------------------- */
    /* Tasks                                                                */
    /* -------------------------------------------------------------------- */

    void
    SRVN::TaskOutput::operator()( const DOM::Entity * entity) const
    {
        const DOM::Task * task = dynamic_cast<const DOM::Task *>(entity);
        if ( !task ) return;

        std::ios_base::fmtflags oldFlags = _output.setf( std::ios::left, std::ios::adjustfield );
        (this->*_func)( *task );
        _output.flags(oldFlags);
    }


    void
    SRVN::TaskOutput::printParameters( const DOM::Task& task ) const
    {
        const std::ios_base::fmtflags oldFlags = _output.setf( std::ios::left, std::ios::adjustfield );
        SRVN::EntityOutput::printCommonParameters( task );
        const DOM::Processor * processor = task.getProcessor();
        _output << std::setw(__maxStrLen-1) << ( processor ? processor->getName() : "--");
        if ( task.getDocument()->getNumberOfGroups() > 0 ) {
            const DOM::Group * group = task.getGroup();
            _output << ' ' << std::setw(__maxStrLen-1) << ( group ? group->getName() : "--");
        }
        _output << ' ' << std::setw(3) << std::right << Input::print_double_parameter( task.getPriority(), 0. );
        if ( task.getDocument()->taskHasThinkTime() ) {
            _output << ' ' << std::setw(__maxDblLen-2);
            if ( task.getSchedulingType() == SCHEDULE_CUSTOMER ) {
                _output << Input::print_double_parameter( task.getThinkTime() );
            } else {
                _output << ' ';
            }
	    _output << ' ';
        }

        const std::vector<DOM::Entry *> & entries = task.getEntryList();
        for ( std::vector<DOM::Entry *>::const_iterator nextEntry = entries.begin(); nextEntry != entries.end(); ++nextEntry ) {
	    if ( nextEntry != entries.begin() ) _output << ",";
	    _output << " " << (*nextEntry)->getName();
        }

        const std::map<std::string,DOM::Activity*>& activities = task.getActivities();
        if ( activities.size() > 0 ) {
            _output << " :";
            for ( std::map<std::string,DOM::Activity*>::const_iterator nextActivity  = activities.begin(); nextActivity != activities.end(); ++nextActivity ) {
                const DOM::Activity * activity = nextActivity->second;
		if ( nextActivity != activities.begin() ) _output << ",";
		_output << " " << activity->getName();
            }
        }
        _output << newline;
        _output.flags(oldFlags);
    }

    /* ---------- Results ---------- */

    void
    SRVN::TaskOutput::printThroughputAndUtilization( const DOM::Task& task ) const
    {
        const std::vector<DOM::Entry *> & entries = task.getEntryList();
        std::vector<DOM::Entry *>::const_iterator nextEntry;
        bool print_task_name = true;
        unsigned item_count = 0;
	const bool is_infinite = task.isInfinite() || task.getSchedulingType() == SCHEDULE_CUSTOMER || task.getSchedulingType() == SCHEDULE_DELAY;
	double copies = 1.0;
	if ( !is_infinite ) {
	    copies = static_cast<double>(task.getCopiesValue());
	}
        for ( nextEntry = entries.begin(); nextEntry != entries.end(); ++nextEntry, ++item_count ) {
            const DOM::Entry * entry = *nextEntry;
	    const double entry_util = entry->getResultUtilization() / copies;
            _output << colour( entry_util ) << entity_name( task, print_task_name )
                    << entry_name( *entry )
                    << std::setw(__maxDblLen-1) << entry->getResultThroughput() << ' '
                    << entry_utilization( *entry )
                    << entry->getResultUtilization() << newline;
            if ( __conf95 ) {
                _output << conf_level( __maxStrLen*2, ConfidenceIntervals::CONF_95 )
                        << std::setw(__maxDblLen-1) << (*__conf95)(entry->getResultThroughputVariance()) << ' '
                        << entry_utilization_confidence( *entry, __conf95 )
                        << (*__conf95)(entry->getResultUtilizationVariance()) << newline;
            }
            if ( __conf99 ) {
                _output << conf_level( __maxStrLen*2, ConfidenceIntervals::CONF_99 )
                        << std::setw(__maxDblLen-1) << (*__conf99)(entry->getResultThroughputVariance()) << ' '
                        << entry_utilization_confidence( *entry, __conf99 )
                        << (*__conf99)(entry->getResultUtilizationVariance()) << newline;
            }
            _output << colour( 0 );
        }

        const std::map<std::string,DOM::Activity*>& activities = task.getActivities();
        if ( activities.size() > 0 ) {
            _output << entity_name( task, print_task_name ) << activity_separator(0) << newline;
            std::map<std::string,DOM::Activity*>::const_iterator nextActivity;
            for ( nextActivity = activities.begin(); nextActivity != activities.end(); ++nextActivity ) {
                const DOM::Activity * activity = nextActivity->second;
                _output << std::setw(__maxStrLen ) << " "
                        << std::setw(__maxStrLen-1) << activity->getName() << ' '
                        << std::setw(__maxDblLen-1) << activity->getResultThroughput() << ' '
                        << std::setw(__maxDblLen) << activity->getResultUtilization() << activityEOF << newline;
                if ( __conf95 ) {
                    _output << conf_level( __maxStrLen*2, ConfidenceIntervals::CONF_95 )
                            << std::setw(__maxDblLen-1) << (*__conf95)(activity->getResultThroughputVariance()) << ' '
                            << std::setw(__maxDblLen-1) << (*__conf95)(activity->getResultUtilizationVariance()) << ' ' << activityEOF << newline;
                }
                if ( __conf99 ) {
                    _output << conf_level( __maxStrLen*2, ConfidenceIntervals::CONF_99 )
                            << std::setw(__maxDblLen-1) << (*__conf99)(activity->getResultThroughputVariance()) << ' '
                            << std::setw(__maxDblLen-1) << (*__conf99)(activity->getResultUtilizationVariance()) << ' ' << activityEOF << newline;
                }
            }
        }

        if ( item_count > 0 && __parseable ) {
            _output << std::setw(__maxStrLen) << " " << activityEOF << newline;
        }

        /* Totals */
        if ( item_count > 1 && task.getResultPhaseCount() > 0 ) {
            const double task_util = task.getResultUtilization() / copies;
            _output << textit << colour( task_util ) << std::setw( __maxStrLen*2 ) << (__parseable ? " " : "Total:" )
                    << std::setw(__maxDblLen-1) << task.getResultThroughput() << ' '
                    << task_utilization( task )
                    << task.getResultUtilization() << newline;
            if ( __conf95 ) {
                _output << conf_level( __maxStrLen*2, ConfidenceIntervals::CONF_95 )
                        << std::setw(__maxDblLen-1) << (*__conf95)(task.getResultThroughputVariance()) << ' '
                        << task_utilization_confidence( task, __conf95 )
                        << (*__conf95)(task.getResultUtilizationVariance()) << newline;
            }
            if ( __conf99 ) {
                _output << conf_level( __maxStrLen*2, ConfidenceIntervals::CONF_99 )
                        << std::setw(__maxDblLen-1) << (*__conf99)(task.getResultThroughputVariance()) << ' '
                        << task_utilization_confidence( task, __conf99 )
                        << (*__conf99)(task.getResultUtilizationVariance()) << newline;
            }
            _output << colour( 0 ) << textrm;
        }
    }


    void
    SRVN::TaskOutput::printJoinDelay( const DOM::Task& task ) const
    {
        const std::set<DOM::ActivityList*>& activity_lists = task.getActivityLists();
        std::set<DOM::ActivityList*>::const_iterator next_list;
	bool print_task_name = true;
        for ( std::set<DOM::ActivityList*>::const_iterator  list = activity_lists.begin(); list != activity_lists.end(); ++list ) {
	    if ( !dynamic_cast<DOM::AndJoinActivityList *>( *list ) ) continue;
	    const DOM::Activity * first;
	    const DOM::Activity * last;
	    (*list)->activitiesForName( first, last );
	    _output << entity_name( task, print_task_name )
		    << std::setw(__maxStrLen-1) << first->getName() << " "
		    << std::setw(__maxStrLen-1) << last->getName() << " "
		    << std::setw(__maxDblLen-1) << (*list)->getResultJoinDelay() << ' '
		    << std::setw(__maxDblLen-1) << (*list)->getResultVarianceJoinDelay()
		    << newline;
	    if ( __conf95 ) {
		_output << conf_level( __maxStrLen*3, ConfidenceIntervals::CONF_95 )
			<< std::setw(__maxDblLen-1) << (*__conf95)((*list)->getResultJoinDelayVariance()) << ' '
			<< std::setw(__maxDblLen-1) << (*__conf95)((*list)->getResultVarianceJoinDelayVariance())
			<< newline;
	    }
	    if ( __conf99 ) {
		_output << conf_level( __maxStrLen*3, ConfidenceIntervals::CONF_99 )
			<< std::setw(__maxDblLen-1) << (*__conf99)((*list)->getResultJoinDelayVariance()) << ' '
			<< std::setw(__maxDblLen-1) << (*__conf99)((*list)->getResultVarianceJoinDelayVariance())
			<< newline;
	    }
	    print_task_name = false;
	}
	if ( !print_task_name && __parseable ) {
	    _output << activityEOF << newline;
	}
    }

/*
  Semaphore holding times:

  Task Name               Source EntryTarget EntryPhase 1     Phase 2     Phase 3
  semaphore               wait        signal      0.70367     1.8099      0
*/

    void SRVN::TaskOutput::printHoldTime( const DOM::Task& task ) const
    {
        const DOM::SemaphoreTask * semaphore = dynamic_cast<const DOM::SemaphoreTask *>(&task);
        if ( !semaphore ) return;

        const std::vector<DOM::Entry *>& entries = task.getEntryList();
        DOM::Entry * wait_entry;
        DOM::Entry * signal_entry;
        if ( entries[0]->getSemaphoreFlag() == DOM::Entry::Semaphore::SIGNAL ) {
            wait_entry = entries[1];
            signal_entry = entries[0];
        } else {
            wait_entry = entries[0];
            signal_entry = entries[1];
        }
        bool print_task_name = true;
        _output << entity_name(task,print_task_name)
                << std::setw(__maxStrLen-1) << wait_entry->getName() << " "
                << std::setw(__maxStrLen-1) << signal_entry->getName() << " "
                << std::setw(__maxDblLen-1) << semaphore->getResultHoldingTime()  << " "
                << std::setw(__maxDblLen-1) << semaphore->getResultVarianceHoldingTime()  << " "
                << std::setw(__maxDblLen-1) << semaphore->getResultHoldingUtilization() << newline;
        if ( __conf95 ) {
            _output << conf_level( __maxStrLen*3, ConfidenceIntervals::CONF_95 )
                    << std::setw(__maxDblLen-1) << (*__conf95)( semaphore->getResultHoldingTimeVariance() ) << " "
                    << std::setw(__maxDblLen-1) << (*__conf95)( semaphore->getResultVarianceHoldingTimeVariance() ) << " "
                    << std::setw(__maxDblLen-1) << (*__conf95)( semaphore->getResultHoldingUtilizationVariance() ) << newline;

        }
        if ( __conf99 ) {
            _output << conf_level( __maxStrLen*3, ConfidenceIntervals::CONF_99 )
                    << std::setw(__maxDblLen-1) << (*__conf99)( semaphore->getResultHoldingTimeVariance() ) << " "
                    << std::setw(__maxDblLen-1) << (*__conf99)( semaphore->getResultVarianceHoldingTimeVariance() ) << " "
                    << std::setw(__maxDblLen-1) << (*__conf99)( semaphore->getResultHoldingUtilizationVariance() ) << newline;

        }
    }

    /*
      RWlock holding times:

      Task Name            Source   EntryTarget  Blocked Time    Variance  Hold Time     Variance  Utilization
      rwlock               r-lock    r-unlock      0.70367        1.8099       0
      rwlock               w-lock    w-unlock      0.70367        1.8099       0
    */

    void SRVN::TaskOutput::printRWLOCKHoldTime( const DOM::Task& task ) const
    {
        const DOM::RWLockTask * rwlock = dynamic_cast<const DOM::RWLockTask *>(&task);
        if ( !rwlock ) return;

        const std::vector<DOM::Entry *>& entries = task.getEntryList();
        DOM::Entry * r_lock_entry = nullptr;
        DOM::Entry * r_unlock_entry = nullptr;
        DOM::Entry * w_lock_entry = nullptr;
        DOM::Entry * w_unlock_entry = nullptr;

        for (int i=0;i<4;i++){
            switch (entries[i]->getRWLockFlag()) {
            case DOM::Entry::RWLock::READ_UNLOCK:  r_unlock_entry=entries[i]; break;
            case DOM::Entry::RWLock::READ_LOCK:    r_lock_entry=entries[i]; break;
            case DOM::Entry::RWLock::WRITE_UNLOCK: w_unlock_entry=entries[i]; break;
            case DOM::Entry::RWLock::WRITE_LOCK:   w_lock_entry=entries[i]; break;
            default: abort();
            }
        }

        bool print_task_name = true;
        _output << entity_name(task,print_task_name)
                << std::setw(__maxStrLen-1) << r_lock_entry->getName() << " "
                << std::setw(__maxStrLen-1) << r_unlock_entry->getName() << " "
                << std::setw(__maxDblLen-1) << rwlock->getResultReaderBlockedTime()  << " "
                << std::setw(__maxDblLen-1) << rwlock->getResultVarianceReaderBlockedTime()  << " "
                << std::setw(__maxDblLen-1) << rwlock->getResultReaderHoldingTime()  << " "
                << std::setw(__maxDblLen-1) << rwlock->getResultVarianceReaderHoldingTime()  << " "
                << std::setw(__maxDblLen-1) << rwlock->getResultReaderHoldingUtilization() << newline;

        if ( __conf95 ) {
            _output << conf_level( __maxStrLen*3, ConfidenceIntervals::CONF_95 )
                    << std::setw(__maxDblLen-1) << (*__conf95)( rwlock->getResultReaderBlockedTimeVariance() ) << " "
                    << std::setw(__maxDblLen-1) << (*__conf95)( rwlock->getResultVarianceReaderBlockedTimeVariance() ) << " "
                    << std::setw(__maxDblLen-1) << (*__conf95)( rwlock->getResultReaderHoldingTimeVariance() ) << " "
                    << std::setw(__maxDblLen-1) << (*__conf95)( rwlock->getResultVarianceReaderHoldingTimeVariance() ) << " "
                    << std::setw(__maxDblLen-1) << (*__conf95)( rwlock->getResultReaderHoldingUtilizationVariance() ) << newline;
        }
        if ( __conf99 ) {
            _output << conf_level( __maxStrLen*3, ConfidenceIntervals::CONF_99 )
                    << std::setw(__maxDblLen-1) << (*__conf99)( rwlock->getResultReaderBlockedTimeVariance() ) << " "
                    << std::setw(__maxDblLen-1) << (*__conf99)( rwlock->getResultVarianceReaderBlockedTimeVariance() ) << " "
                    << std::setw(__maxDblLen-1) << (*__conf99)( rwlock->getResultReaderHoldingTimeVariance() ) << " "
                    << std::setw(__maxDblLen-1) << (*__conf99)( rwlock->getResultVarianceReaderHoldingTimeVariance() ) << " "
                    << std::setw(__maxDblLen-1) << (*__conf99)( rwlock->getResultReaderHoldingUtilizationVariance() ) << newline;
        }
        _output << entity_name(task,print_task_name)
                << std::setw(__maxStrLen-1) << w_lock_entry->getName() << " "
                << std::setw(__maxStrLen-1) << w_unlock_entry->getName() << " "
                << std::setw(__maxDblLen-1) << rwlock->getResultWriterBlockedTime()  << " "
                << std::setw(__maxDblLen-1) << rwlock->getResultVarianceWriterBlockedTime()  << " "
                << std::setw(__maxDblLen-1) << rwlock->getResultWriterHoldingTime()  << " "
                << std::setw(__maxDblLen-1) << rwlock->getResultVarianceWriterHoldingTime()  << " "
                << std::setw(__maxDblLen-1) << rwlock->getResultWriterHoldingUtilization() << newline;

        if ( __conf95 ) {
            _output << conf_level( __maxStrLen*3, ConfidenceIntervals::CONF_95 )
                    << std::setw(__maxDblLen-1) << (*__conf95)( rwlock->getResultWriterBlockedTimeVariance() ) << " "
                    << std::setw(__maxDblLen-1) << (*__conf95)( rwlock->getResultVarianceWriterBlockedTimeVariance() ) << " "
                    << std::setw(__maxDblLen-1) << (*__conf95)( rwlock->getResultWriterHoldingTimeVariance() ) << " "
                    << std::setw(__maxDblLen-1) << (*__conf95)( rwlock->getResultVarianceWriterHoldingTimeVariance() ) << " "
                    << std::setw(__maxDblLen-1) << (*__conf95)( rwlock->getResultWriterHoldingUtilizationVariance() ) << newline;
        }
        if ( __conf99 ) {
            _output << conf_level( __maxStrLen*3, ConfidenceIntervals::CONF_99 )
                    << std::setw(__maxDblLen-1) << (*__conf99)( rwlock->getResultWriterBlockedTimeVariance() ) << " "
                    << std::setw(__maxDblLen-1) << (*__conf99)( rwlock->getResultVarianceWriterBlockedTimeVariance() ) << " "
                    << std::setw(__maxDblLen-1) << (*__conf99)( rwlock->getResultWriterHoldingTimeVariance() ) << " "
                    << std::setw(__maxDblLen-1) << (*__conf99)( rwlock->getResultVarianceWriterHoldingTimeVariance() ) << " "
                    << std::setw(__maxDblLen-1) << (*__conf99)( rwlock->getResultWriterHoldingUtilizationVariance() ) << newline;
        }
    }

    /*
     * SRVN style for a task
     */

    void
    SRVN::TaskInput::operator()( const DOM::Entity * entity) const
    {
        const DOM::Task * task = dynamic_cast<const DOM::Task *>(entity);
        if ( !task ) return;

        std::ios_base::fmtflags oldFlags = _output.setf( std::ios::left, std::ios::adjustfield );
        (this->*_func)( *task );
        _output.flags(oldFlags);
    }


    void
    SRVN::TaskInput::print( const DOM::Task& task ) const
    {
        _output << "  t " << task.getName()
                << scheduling_of( task )
                << entries_of( task )
                << queue_length_of( task )
                << " " << task.getProcessor()->getName()
                << priority_of( task )
                << think_time_of( task )
                << copies_of( task )
                << replicas_of( task )
                << group_of( task );
        if ( !task.getDocument()->instantiated() ) {
            printObservationVariables( _output, task );
        }
        _output << std::endl;

        for ( std::map<const std::string, const DOM::ExternalVariable *>::const_iterator fi = task.getFanIns().begin(); fi != task.getFanIns().end(); ++fi ) {
            if ( !DOM::Common_IO::is_default_value( fi->second, 1 ) ) {     /* task name, fan in */
                _output << "  I " << fi->first << " " << task.getName() << " " << Input::print_integer_parameter(fi->second,0) << std::endl;
            }
        }
        for ( std::map<const std::string, const DOM::ExternalVariable *>::const_iterator fo = task.getFanOuts().begin(); fo != task.getFanOuts().end(); ++fo ) {
            if ( !DOM::Common_IO::is_default_value( fo->second, 1 ) ) {
                _output << "  O " << task.getName() << " " << fo->first << " " << Input::print_integer_parameter(fo->second,0) << std::endl;
            }
        }

    }

    std::ostream&
    SRVN::TaskInput::printScheduling( std::ostream& output, const DOM::Task & task )
    {
	output << " ";
        if ( task.isInfinite() ) {
            output << scheduling_label.at(SCHEDULE_DELAY).flag;
        } else {
	    output << scheduling_label.at(task.getSchedulingType()).flag;
	}
        return output;
    }

    std::ostream&
    SRVN::TaskInput::printEntryList( std::ostream& output,  const DOM::Task& task )
    {
        const std::vector<DOM::Entry *> & entries = task.getEntryList();
        for ( std::vector<DOM::Entry *>::const_iterator nextEntry = entries.begin(); nextEntry != entries.end(); ++nextEntry ) {
            const DOM::Entry * entry = *nextEntry;
	    if ( entry->isDefined() ) {
		output << " " << entry->getName();
	    }
        }
        output << " -1";
        return output;
    }

    std::ostream&
    SRVN::TaskInput::printCopies( std::ostream& output, const DOM::Task & task )
    {
        if ( !task.isInfinite() ) {
	    try {
		output << Input::is_integer_and_gt( " m ", task.getCopies(), 1.0 );
	    }
	    catch ( const std::domain_error& e ) {
		task.throw_invalid_parameter( "multiplicity", e.what() );
	    }
        }
        return output;
    }

    std::ostream&
    SRVN::TaskInput::printGroup( std::ostream& output,  const DOM::Task& task )
    {
        if ( task.getGroup() ) {
            output << " g "  << task.getGroup()->getName();
        }
        return output;
    }

    std::ostream&
    SRVN::TaskInput::printPriority( std::ostream& output,  const DOM::Task& task )
    {
        if ( task.hasPriority() || task.getProcessor()->hasPriorityScheduling() ) {
	    try {
		output << " " << Input::print_integer_parameter( task.getPriority(), 0 );
	    }
	    catch ( const std::domain_error& e ) {
		task.throw_invalid_parameter( "priority", e.what() );
	    }
        }
        return output;
    }

    std::ostream&
    SRVN::TaskInput::printQueueLength( std::ostream& output,  const DOM::Task& task )
    {
        if ( task.hasQueueLength() ) {
	    try {
		output << " q " << Input::print_integer_parameter( task.getQueueLength(), 0 );
	    }
	    catch ( const std::domain_error& e ) {
		task.throw_invalid_parameter( "queue length", e.what() );
	    }
        }
        return output;
    }

    std::ostream&
    SRVN::TaskInput::printReplicas( std::ostream& output, const DOM::Task & task )
    {
	try {
	    output << Input::is_integer_and_gt( " r ", task.getReplicas(), 1.0 );
	}
	catch ( const std::domain_error& e ) {
	    task.throw_invalid_parameter( "replicas", e.what() );
	}
        return output;
    }

    std::ostream&
    SRVN::TaskInput::printThinkTime( std::ostream& output,  const DOM::Task & task )
    {
        if ( task.getSchedulingType() == SCHEDULE_CUSTOMER && task.hasThinkTime() ) {
            output << " z " << Input::print_double_parameter( task.getThinkTime() );
        }
        return output;
    }

    void
    SRVN::TaskInput::printEntryInput( const DOM::Task& task ) const
    {
        _output << "# ---------- " << task.getName() << " ----------" << std::endl;
        const std::vector<DOM::Entry *> & entries = task.getEntryList();
        std::for_each( entries.begin(), entries.end(), EntryInput( _output, &EntryInput::print ) );
    }

    void
    SRVN::TaskInput::printActivityInput( const DOM::Task& task ) const
    {
        const std::map<std::string,DOM::Activity*>& activities = task.getActivities();
        if ( activities.size() == 0 ) return;

        _output << std::endl << "A " << task.getName() << std::endl;
        std::for_each( activities.begin(), activities.end(), ActivityInput( _output, &ActivityInput::print ) );

	/* Find all activties that reply */
	std::set<const DOM::Activity *> deferredReplyActivities;
	std::for_each( activities.begin(), activities.end(), [&]( const auto& activity ){ if ( !activity.second->getReplyList().empty() ) deferredReplyActivities.insert( activity.second ); } );

	/* Print out all defined precedence lists. */
        const std::set<DOM::ActivityList*>& precedences = task.getActivityLists();
	if ( !precedences.empty() || !deferredReplyActivities.empty() ) _output << " :" << std::endl;
	std::for_each( precedences.begin(), precedences.end(), ActivityListInput( _output, &ActivityListInput::print, precedences.size(), deferredReplyActivities ) );

	/* BUG 377 Any activities that reply and were not in a pre-precedence list are output here */
	for ( std::set<const DOM::Activity *>::const_iterator activity = deferredReplyActivities.begin(); activity != deferredReplyActivities.end(); ++activity ) {
	    _output << "  " << (*activity)->getName();
	    printReplyList( (*activity)->getReplyList() );
	    if ( std::next( activity ) != deferredReplyActivities.end() ) _output << ";";
	    _output << std::endl;
	}

        _output << "-1" << std::endl;
    }

    /* -------------------------------------------------------------------- */
    /* Entries                                                              */
    /* -------------------------------------------------------------------- */

    void
    SRVN::EntryOutput::operator()( const DOM::Entity *entity) const
    {
        const DOM::Task * task = dynamic_cast<const DOM::Task *>(entity);
        if ( !task ) return;

        std::ios_base::fmtflags oldFlags = _output.setf( std::ios::left, std::ios::adjustfield );
        const std::vector<DOM::Entry *>& entries = task->getEntryList();
        bool print_task_name = true;
	std::for_each( entries.begin(), entries.end(), [&]( DOM::Entry * entry ){ (this->*_entryFunc)( *entry, *(entity), print_task_name ); } );

        const std::map<std::string,DOM::Activity*>& activities = task->getActivities();
        if ( activities.size() > 0 && _activityFunc != nullptr ) {
            std::map<std::string,DOM::Activity*>::const_iterator nextActivity;
            bool found;
            if ( _testFunc ) {
                found = false;
                for ( nextActivity = activities.begin(); !found && nextActivity != activities.end(); ++nextActivity ) {
                    if ( (this->*_testFunc)( *nextActivity->second ) ) found = true;
                }
            } else {
                found = true;
            }
            if ( found ) {
                _output << entity_name( *(entity), print_task_name ) << activity_separator(0) << newline;
		std::for_each( activities.begin(), activities.end(), [&]( const std::pair<std::string,DOM::Activity*>& activity ){ (this->*_activityFunc)( *activity.second ); } );
            }
        }
        if ( _activityFunc != nullptr && print_task_name == false && __parseable ) {
            _output << std::setw(__maxStrLen) << " " << activityEOF << newline;
        }
        _output.flags(oldFlags);
    }

    void SRVN::EntryOutput::CountForwarding::operator()( const DOM::Entity * entity )
    {
        const DOM::Task * task = dynamic_cast<const DOM::Task *>(entity);
        if ( !task ) return;

        const std::vector<DOM::Entry *>& entries = task->getEntryList();
	std::for_each( entries.begin(), entries.end(), [&]( const DOM::Entry * entry ){ _count += entry->getForwarding().size(); } );
    }

    /* ---------- Entry parameters ---------- */

    void
    SRVN::EntryOutput::printEntryCoefficientOfVariation( const DOM::Entry & entry, const DOM::Entity & entity, bool& print_task_name  ) const
    {
        if ( entry.getStartActivity() == nullptr ) {
	    _output << entity_name( entity, print_task_name ) << entry_name( entry ) << coefficient_of_variation( entry ) << newline;
        }
    }

    void
    SRVN::EntryOutput::printEntryDemand( const DOM::Entry & entry, const DOM::Entity & entity, bool& print_task_name ) const
    {
        if ( entry.getStartActivity() == nullptr ) {
	    _output << entity_name( entity, print_task_name ) << entry_name( entry ) << service_demand( entry ) << newline;
        }
    }

    void
    SRVN::EntryOutput::printEntryMaxServiceTime( const DOM::Entry & entry, const DOM::Entity & entity, bool& print_task_name  ) const
    {
        if ( entry.hasMaxServiceTimeExceeded() ) {
            _output << entity_name( entity, print_task_name ) << entry_name( entry ) << max_service_time( entry ) << newline;
        }
    }

    void
    SRVN::EntryOutput::printEntryPhaseType( const DOM::Entry & entry, const DOM::Entity & entity, bool& print_task_name  ) const
    {
        if ( entry.getStartActivity() == nullptr ) {
            _output << entity_name( entity, print_task_name ) << entry_name( entry ) << phase_type( entry ) << newline;
        }
    }

    void
    SRVN::EntryOutput::printEntryThinkTime( const DOM::Entry & entry, const DOM::Entity & entity, bool& print_task_name  ) const
    {
        if ( entry.getStartActivity() == nullptr ) {
	    _output << entity_name( entity, print_task_name ) << entry_name( entry ) << think_time( entry ) << newline;
        }
    }

    void
    SRVN::EntryOutput::printForwarding( const DOM::Entry & entry, const DOM::Entity & entity, bool& print_task_name ) const
    {
        const std::vector<DOM::Call *>& forwarding = entry.getForwarding();
	std::vector<DOM::Call*>::const_iterator nextCall;
	for ( nextCall = forwarding.begin(); nextCall != forwarding.end(); ++nextCall ) {
	    const DOM::Call * call = *nextCall;
	    const DOM::Entry * dest = call->getDestinationEntry();
	    _output << entity_name( entity, print_task_name )
		    << entry_name( entry )
		    << entry_name( *dest )
		    << std::setw(__maxDblLen) << Input::print_double_parameter( call->getCallMean(), 0. )
		    << newline;
	}
    }

    void
    SRVN::EntryOutput::printOpenArrivals( const DOM::Entry & entry, const DOM::Entity & entity, bool& print_task_name  ) const
    {
	if ( !entry.hasOpenArrivalRate() ) return;

	try {
	    _output << entity_name( entity, print_task_name ) << entry_name( entry ) << open_arrivals( entry ) << newline;
	}
	catch ( const std::domain_error& e ) {
	    entry.throw_invalid_parameter( "open arrivals", e.what() );
	}
    }

    /* ---- Activities ---- */

    void SRVN::EntryOutput::printActivityCoefficientOfVariation( const DOM::Activity &activity ) const { printActivity( activity, &DOM::Activity::getCoeffOfVariationSquared ); }
    void SRVN::EntryOutput::printActivityDemand( const DOM::Activity &activity ) const { printActivity( activity, &DOM::Activity::getServiceTime ); }
    void SRVN::EntryOutput::printActivityMaxServiceTime( const DOM::Activity &activity ) const {};
    void SRVN::EntryOutput::printActivityThinkTime( const DOM::Activity &activity ) const{ printActivity( activity, &DOM::Activity::getThinkTime ); }

    void
    SRVN::EntryOutput::printActivityPhaseType( const DOM::Activity& activity ) const
    {
        _output << std::setw(__maxStrLen) << " " << std::setw(__maxStrLen-1) << activity.getName() << " ";
        switch (activity.getPhaseTypeFlag()) {
        case DOM::Phase::Type::DETERMINISTIC: _output << std::setw(__maxDblLen) << "determin"; break;
        case DOM::Phase::Type::STOCHASTIC:    _output << std::setw(__maxDblLen) << "stochastic"; break;
        }
        _output << newline;
    }

    void
    SRVN::EntryOutput::printActivity( const DOM::Activity& activity, const activityFunc func ) const
    {
	_output << std::setw(__maxStrLen) << " " << std::setw(__maxStrLen) << activity.getName() << std::setw(__maxDblLen) << Input::print_double_parameter( (activity.*func)(), 0. ) << newline;
    }

    /* ---- Entry Results ---- */

    void
    SRVN::EntryOutput::printEntryThroughputBounds( const DOM::Entry &entry, const DOM::Entity &entity, bool& print ) const
    {
        _output << entity_name(entity,print) << entry_name( entry ) << std::setw(__maxDblLen) << entry.getResultThroughputBound() << newline;
    }

    void
    SRVN::EntryOutput::printEntryServiceTime( const DOM::Entry &entry, const DOM::Entity &entity, bool& print ) const
    {
        _output << entity_name(entity,print) << entry_name( entry ) << service_time(entry) << newline;
        if ( __conf95 ) {
            _output << conf_level( __maxStrLen*2, ConfidenceIntervals::CONF_95 ) << service_time_confidence(entry,__conf95) << newline;
            _output << conf_level( __maxStrLen*2, ConfidenceIntervals::CONF_99 ) << service_time_confidence(entry,__conf99) << newline;
        }
    }

    void
    SRVN::EntryOutput::printEntryMaxServiceTimeExceeded( const DOM::Entry &entry, const DOM::Entity &entity, bool& print ) const
    {
        if ( entry.hasMaxServiceTimeExceeded() ) {
            _output << entity_name(entity,print) << entry_name( entry ) << service_time_exceeded(entry) << newline;
            if ( __conf95 ) {
                _output << conf_level( __maxStrLen*2, ConfidenceIntervals::CONF_95 ) << service_time_exceeded_confidence(entry,__conf95) << newline;
                _output << conf_level( __maxStrLen*2, ConfidenceIntervals::CONF_99 ) << service_time_exceeded_confidence(entry,__conf99) << newline;
            }
        }
    }

    void
    SRVN::EntryOutput::printEntryVarianceServiceTime( const DOM::Entry &entry, const DOM::Entity &entity, bool& print ) const
    {
        _output << entity_name(entity,print) << entry_name( entry ) << variance_service_time(entry);

        double value = 0.0;
        if ( !__parseable ) {
            value = entry.getResultSquaredCoeffVariation();
            if ( !value ) {
                /* Recompute based on variance */
                const std::map<unsigned, DOM::Phase*>& phases = entry.getPhaseList();
                assert( phases.size() <= DOM::Phase::MAX_PHASE );
                double sum_of_t = 0;
                double sum_of_v = 0;
                for ( std::map<unsigned, DOM::Phase*>::const_iterator p = phases.begin(); p != phases.end(); ++p) {
                    const DOM::Phase* phase = p->second;
                    if ( !phase ) continue;
                    sum_of_t += phase->getResultServiceTime();
                    sum_of_v += phase->getResultVarianceServiceTime();
                }
                if ( sum_of_t > 0 ) {
                    value = sum_of_v / ( sum_of_t * sum_of_t );
                }
            }
            _output << std::setw(__maxDblLen) << value;
        }
        _output << newline;

        value = entry.getResultSquaredCoeffVariationVariance();
        if ( __conf95 ) {
            _output << conf_level( __maxStrLen*2, ConfidenceIntervals::CONF_95 ) << variance_service_time_confidence(entry,__conf95);
            if ( !__parseable && value > 0.0 ) {
                _output << std::setw(__maxDblLen) << (*__conf95)(value);
            }
            _output << newline;
        }
        if ( __conf99 ) {
            _output << conf_level( __maxStrLen*2, ConfidenceIntervals::CONF_99 ) << variance_service_time_confidence(entry,__conf99);
            if ( !__parseable && value > 0.0 ) {
                _output << std::setw(__maxDblLen) << (*__conf99)(value);
            }
            _output << newline;
        }
    }

    void
    SRVN::EntryOutput::printOpenQueueWait( const DOM::Entry &entry, const DOM::Entity &entity, bool& print ) const
    {
        if ( !entry.hasOpenArrivalRate() ) return;

        _output << entity_name( entity, print )
                << entry_name( entry )
                << std::setw(__maxDblLen-1) << entry.getResultThroughput() << " "
                << std::setw(__maxDblLen-1) << entry.getResultWaitingTime() << newline;
        if ( __conf95 ) {
            _output << conf_level( __maxStrLen * 2, ConfidenceIntervals::CONF_95 )
                    << std::setw(__maxDblLen) << " "         /* Input parameter, so ignore it */
                    << std::setw(__maxDblLen-1) << (*__conf95)(entry.getResultWaitingTimeVariance()) << newline;
        }
        if ( __conf99 ) {
            _output << conf_level( __maxStrLen * 2, ConfidenceIntervals::CONF_99 )
                    << std::setw(__maxDblLen) << " "
                    << std::setw(__maxDblLen-1) << (*__conf99)(entry.getResultWaitingTimeVariance()) << newline;
        }
    }

    void
    SRVN::EntryOutput::printOpenQueueDropProbability( const DOM::Entry &entry, const DOM::Entity &entity, bool& print ) const
    {
        if ( !entry.hasOpenArrivalRate() || !entry.hasResultDropProbability() ) return;

        _output << entity_name( entity, print )
                << entry_name( entry )
                << std::setw(__maxDblLen-1) << entry.getResultDropProbability() << newline;
        if ( __conf95 ) {
            _output << conf_level( __maxStrLen * 2, ConfidenceIntervals::CONF_95 )
                    << std::setw(__maxDblLen-1) << (*__conf95)(entry.getResultDropProbabilityVariance()) << newline;
	}
        if ( __conf99 ) {
            _output << conf_level( __maxStrLen * 2, ConfidenceIntervals::CONF_99 )
                    << std::setw(__maxDblLen-1) << (*__conf99)(entry.getResultDropProbabilityVariance()) << newline;
	}
    }

    void
    SRVN::EntryOutput::printForwardingWaiting( const DOM::Entry &entry, const DOM::Entity &entity, bool& print ) const
    {
	commonPrintForwarding( entry, entity, print, &DOM::Call::getResultWaitingTime, &DOM::Call::getResultWaitingTimeVariance );
    }

    void
    SRVN::EntryOutput::printForwardingVarianceWaiting( const DOM::Entry &entry, const DOM::Entity &entity, bool& print ) const
    {
	commonPrintForwarding( entry, entity, print, &DOM::Call::getResultVarianceWaitingTime, &DOM::Call::getResultVarianceWaitingTimeVariance );
    }

    void
    SRVN::EntryOutput::commonPrintForwarding( const DOM::Entry &entry, const DOM::Entity &entity, bool& print, doubleCallFunc get_result, doubleCallFunc get_variance ) const
    {
	const std::vector<DOM::Call *>& forwarding = entry.getForwarding();
	for ( std::vector<DOM::Call *>::const_iterator call = forwarding.begin(); call != forwarding.end(); ++call ) {
	    _output << entity_name( entity, print )
		    << entry_name( entry )
		    << entry_name( *(*call)->getDestinationEntry() )
		    << std::setw(__maxDblLen-1) << ((*call)->*get_result)() << " ";
	    _output << newline;
	    if ( __conf95 ) {
		_output << conf_level( __maxStrLen * 3, ConfidenceIntervals::CONF_95 )
			<< std::setw(__maxDblLen-1) << (*__conf95)(((*call)->*get_variance)()) << " ";
		_output << newline;
	    }
	    if ( __conf99 ) {
		_output << conf_level( __maxStrLen * 3, ConfidenceIntervals::CONF_99 )
			<< std::setw(__maxDblLen-1) << (*__conf99)(((*call)->*get_variance)()) << " ";
		_output << newline;
	    }
	}
    }

    /* ---------- Activity Results ---------- */

    void SRVN::EntryOutput::printActivityServiceTime( const DOM::Activity &activity ) const { activityResults( activity, &DOM::Activity::getResultServiceTime, &DOM::Activity::getResultServiceTimeVariance ); }
    void SRVN::EntryOutput::printActivityMaxServiceTimeExceeded( const DOM::Activity &activity ) const { activityResults( activity, &DOM::Activity::getResultMaxServiceTimeExceeded, 0 ); }
    bool SRVN::EntryOutput::testActivityMaxServiceTimeExceeded( const DOM::Activity &activity ) const { return testForActivityResults( activity, &DOM::Activity::hasMaxServiceTimeExceeded ); }
    void SRVN::EntryOutput::printActivityVarianceServiceTime( const DOM::Activity &activity ) const { activityResults( activity, &DOM::Activity::getResultVarianceServiceTime, &DOM::Activity::getResultVarianceServiceTimeVariance ); }

    void
    SRVN::EntryOutput::activityResults( const DOM::Activity& activity, const doubleActivityFunc mean,  const doubleActivityFunc variance ) const
    {
        _output << std::setw(__maxStrLen) << " " << std::setw(__maxStrLen-1) << activity.getName() << " " << std::setw(__maxDblLen) << (activity.*mean)() << activityEOF << newline;
        if ( __conf95 && variance ) {
            _output << conf_level( __maxStrLen*2, ConfidenceIntervals::CONF_95 )
                    << std::setw(__maxDblLen) << (*__conf95)((activity.*variance)()) << activityEOF << newline;
        }
        if ( __conf99 && variance ) {
            _output << conf_level( __maxStrLen*2, ConfidenceIntervals::CONF_99 )
                    << std::setw(__maxDblLen) << (*__conf99)((activity.*variance)()) << activityEOF << newline;
        }
    }

    bool
    SRVN::EntryOutput::testForActivityResults( const DOM::Activity& activity, const boolActivityFunc tf ) const
    {
        return !tf || (activity.*tf)();
    }

    void
    SRVN::EntryInput::operator()( const DOM::Entry * entry ) const
    {
        (this->*_entryFunc)( *entry );
    }

    void
    SRVN::EntryInput::print( const DOM::Entry& entry ) const
    {
	if ( !entry.isDefined() ) return;

        if ( entry.hasOpenArrivalRate() ) {
	    try {
		_output << "  a " << entry.getName() << " " << Input::print_double_parameter( entry.getOpenArrivalRate(), 0. ) << std::endl;
	    }
	    catch ( const std::domain_error& e ) {
		entry.runtime_error( LQIO::ERR_INVALID_PARAMETER, "open arrivals", e.what() );
	    }
        }

        switch ( entry.getSemaphoreFlag() ) {
        case DOM::Entry::Semaphore::SIGNAL: _output << "  P " << entry.getName() << std::endl; break;
        case DOM::Entry::Semaphore::WAIT:   _output << "  V " << entry.getName() << std::endl; break;
        default: break;
        }
        switch ( entry.getRWLockFlag() ) {
        case DOM::Entry::RWLock::READ_UNLOCK:  _output << "  U " << entry.getName() << std::endl; break;
        case DOM::Entry::RWLock::READ_LOCK:    _output << "  R " << entry.getName() << std::endl; break;
        case DOM::Entry::RWLock::WRITE_UNLOCK: _output << "  X " << entry.getName() << std::endl; break;
        case DOM::Entry::RWLock::WRITE_LOCK:   _output << "  W " << entry.getName() << std::endl; break;
        default: break;
        }

        if ( entry.getStartActivity() ) {
            _output << "  A " << entry.getName() << " " << entry.getStartActivity()->getName() << std::endl;
            if ( entry.hasHistogram() ) {
                /* BUG_668 */
                for ( unsigned p = 1; p <= DOM::Phase::MAX_PHASE; ++p ) {
                    if ( entry.hasHistogramForPhase( p ) ) {
                        const DOM::Histogram *h = entry.getHistogramForPhase( p );
                        _output << "  H " << entry.getName() << " " << p << " " << h->getMin() << " : " <<  h->getMax() << " " << h->getBins() << std::endl;
                    }
                }
            }
            printForwarding( entry );

        } else {
            const std::map<unsigned, DOM::Phase*>& phases = entry.getPhaseList();
            assert( phases.size() <= DOM::Phase::MAX_PHASE );

            _output << "  s " << std::setw( ObjectInput::__maxEntLen ) << entry.getName();
            std::for_each( phases.begin(), phases.end(), PhaseInput( _output, &PhaseInput::printServiceTime ) );
            _output << " -1";
            if ( !entry.getDocument()->instantiated() ) {
                printObservationVariables( _output, entry );
            }
	    _output << std::endl;

            if ( entry.hasNonExponentialPhases() ) {
                _output << "  c " << std::setw( ObjectInput::__maxEntLen ) << entry.getName();
                std::for_each( phases.begin(), phases.end(), PhaseInput( _output, &PhaseInput::printCoefficientOfVariation ) );
                _output << " -1" << std::endl;
            }
            if ( entry.hasThinkTime() ) {
                _output << "  Z " << std::setw( ObjectInput::__maxEntLen ) << entry.getName();
                std::for_each( phases.begin(), phases.end(), PhaseInput( _output, &PhaseInput::printThinkTime ) );
                _output << " -1" << std::endl;
            }
            if ( entry.hasDeterministicPhases() ) {
                _output << "  f " << std::setw( ObjectInput::__maxEntLen ) << entry.getName();
                std::for_each( phases.begin(), phases.end(), PhaseInput( _output, &PhaseInput::printPhaseFlag ) );
                _output << " -1" << std::endl;
            }
            if ( entry.hasMaxServiceTimeExceeded() ) {
                _output << "  M " << std::setw( ObjectInput::__maxEntLen ) << entry.getName();
                std::for_each( phases.begin(), phases.end(), PhaseInput( _output, &PhaseInput::printMaxServiceTimeExceeded ) );
                _output << " -1" << std::endl;
            }
            if ( entry.hasHistogram() ) {
                /* Histograms are stored by phase for regular entries.  Activity entries don't have phases...  Punt... */
                for ( std::map<unsigned, DOM::Phase*>::const_iterator np = phases.begin(); np != phases.end();  ++np ) {
                    const DOM::Phase * p = np->second;
                    if ( p->hasHistogram() ) {
                        const DOM::Histogram *h = p->getHistogram();
                        _output << "  H " << entry.getName() << " " << np->first << " " << h->getMin() << " : " <<  h->getMax() << " " << h->getBins() << std::endl;
                    }
                }
            }
            printForwarding( entry );
            printCalls( entry );
        }
    }


    void SRVN::EntryInput::printForwarding( const DOM::Entry& entry ) const
    {
        /* Forwarding */

        const std::vector<DOM::Call*>& fwds = entry.getForwarding();
        for ( std::vector<DOM::Call*>::const_iterator nextFwd = fwds.begin(); nextFwd != fwds.end(); ++nextFwd ) {
            const DOM::Call * fwd = *nextFwd;
	    const DOM::Entry * dst = fwd->getDestinationEntry();
	    try {
		_output << "  F " << std::setw( ObjectInput::__maxEntLen ) << entry.getName()
			<< " " << std::setw( ObjectInput::__maxEntLen ) << dst->getName() << number_of_calls( fwd ) << " -1" << std::endl;
	    }
	    catch ( const std::domain_error& e ) {
		fwd->throw_invalid_parameter( "probability", e.what() );
	    }
        }
    }

    void SRVN::EntryInput::printCalls( const DOM::Entry& entry ) const
    {
        /* Gather up all the call info over all phases and store in new map<to_entry,call*[3]>. */

        std::map<const DOM::Entry *, ForPhase> callsByPhase;
        const std::map<unsigned, DOM::Phase*>& phases = entry.getPhaseList();
        assert( phases.size() <= DOM::Phase::MAX_PHASE );
	std::for_each( phases.begin(), phases.end(), CollectCalls( callsByPhase ) );		/* Don't care about type of call here */

        /* Now iterate over the collection of calls */

        for ( std::map<const DOM::Entry *, ForPhase>::const_iterator next_y = callsByPhase.begin(); next_y != callsByPhase.end(); ++next_y ) {
	    const DOM::Entry * dst = next_y->first;
            const ForPhase& calls_by_phase = next_y->second;
            _output << "  ";
            switch ( calls_by_phase.getType() ) {
            case DOM::Call::Type::RENDEZVOUS: _output << "y"; break;
            case DOM::Call::Type::SEND_NO_REPLY: _output << "z"; break;
            default: abort();
            }
            _output << " " << std::setw( ObjectInput::__maxEntLen ) << entry.getName()
                    << " " << std::setw( ObjectInput::__maxEntLen ) << dst->getName();
	    unsigned int n = 1;
            for (std::map<unsigned, DOM::Phase*>::const_iterator p = phases.begin(); p != phases.end(); ++p, ++n ) {
		while ( n < p->first ) {		/* Pad */
		    _output << number_of_calls( nullptr );
		    ++n;
		}
		try {
		    _output << number_of_calls( calls_by_phase[p->first] );
		}
		catch ( const std::domain_error& e ) {
		    throw_bad_parameter();
		}
	    }
            _output << " -1";
            if ( !entry.getDocument()->instantiated() ) {
                for (std::map<unsigned, DOM::Phase*>::const_iterator p = phases.begin(); p != phases.end(); ++p ) {
                    const DOM::Call * call = calls_by_phase[p->first];
		    if ( call ) {
			printObservationVariables( _output, *call );
		    }
                }
            }
            _output << std::endl;
        }
    }

    void
    SRVN::PhaseInput::operator()( const std::pair<unsigned,DOM::Phase *>& p ) const
    {
        std::ios_base::fmtflags oldFlags = _output.setf( std::ios::left, std::ios::adjustfield );

	/* Pad with default values if phase is missing from list */

	for ( ; _p < p.first; ++_p ) {
	    (this->*_func)( DOM::Phase() );
	}
        (this->*_func)( *(p.second) );
        _output.flags(oldFlags);
	++_p;
    }

    void SRVN::PhaseInput::printCoefficientOfVariation( const DOM::Phase& p ) const
    {
	try {
	    _output << " " << std::setw(ObjectInput::__maxInpLen) << Input::print_double_parameter( p.getCoeffOfVariationSquared(), 0. );
	}
	catch ( const std::domain_error& e ) {
	    p.throw_invalid_parameter( "CV sq", e.what() );
	}

    }

    void SRVN::PhaseInput::printMaxServiceTimeExceeded( const DOM::Phase& p ) const
    {
	try {
	    _output << " " << std::setw(ObjectInput::__maxInpLen) << p.getMaxServiceTime();
	}
	catch ( const std::domain_error& e ) {
	    p.throw_invalid_parameter( "Exceeded", e.what() );
	}
    }

    void SRVN::PhaseInput::printPhaseFlag( const DOM::Phase& p ) const
    {
	_output << " " << std::setw(ObjectInput::__maxInpLen) << (p.hasDeterministicCalls() ? "1" : "0");
    }

    void SRVN::PhaseInput::printServiceTime( const DOM::Phase& p ) const
    {
	try {
	    _output << " " << std::setw(ObjectInput::__maxInpLen) << Input::print_double_parameter( p.getServiceTime(), 0. );
	}
	catch ( const std::domain_error& e ) {
	    p.throw_invalid_parameter( "service time", e.what() );
	}
    }

    void SRVN::PhaseInput::printThinkTime( const DOM::Phase& p ) const
    {
	try {
	    _output << " " << std::setw(ObjectInput::__maxInpLen) << Input::print_double_parameter( p.getThinkTime(), 0. );
	}
	catch ( const std::domain_error& e ) {
	    p.throw_invalid_parameter( "CV sq", e.what() );
	}
    }

    void
    SRVN::ActivityInput::operator()( const std::pair<std::string,DOM::Activity *>& a ) const
    {
        std::ios_base::fmtflags oldFlags = _output.setf( std::ios::left, std::ios::adjustfield );
        (this->*_func)( *(a.second) );
        _output.flags(oldFlags);
    }

    void
    SRVN::ActivityInput::print( const DOM::Activity& activity ) const
    {
        _output << "  s " << activity.getName();
        printServiceTime( activity );
	if ( !activity.getDocument()->instantiated() ) {
	    printObservationVariables( _output, activity );
	}
        _output << std::endl;
        if ( activity.hasCoeffOfVariationSquared() ) {
            _output << "  c " << activity.getName();
            printCoefficientOfVariation( activity );
            _output << std::endl;
        }
        if ( activity.hasThinkTime() ) {
            _output << "  Z " << activity.getName();
            printThinkTime( activity );
            _output << std::endl;
        }
        if ( activity.hasDeterministicCalls() ) {
            _output << "  f " << activity.getName();
            printPhaseFlag( activity );
            _output << std::endl;
        }
        if ( activity.hasMaxServiceTimeExceeded() ) {
            _output << "  M " << activity.getName();
            printMaxServiceTimeExceeded( activity );
            _output << std::endl;
        }
        if ( activity.hasHistogram() ) {
            const DOM::Histogram *h = activity.getHistogram();
            _output << "  H " << activity.getName() << h->getMin() << " : " <<  h->getMax() << " " << h->getBins() << std::endl;
        }
        const std::vector<DOM::Call *>& calls = activity.getCalls();
	std::for_each( calls.begin(), calls.end(), ActivityCallInput( _output, &ActivityCallInput::print ) );
    }


    void
    SRVN::ActivityListInput::operator()( const DOM::ActivityList * precedence )
    {
        std::ios_base::fmtflags oldFlags = _output.setf( std::ios::left, std::ios::adjustfield );
        (this->*_func)( *precedence );
        _output.flags(oldFlags);
    }

    void
    SRVN::ActivityListInput::print( const DOM::ActivityList& precedence )
    {
        if ( precedence.isForkList() ) return;

        _output << " ";
        printPreList( precedence );
        if ( precedence.getNext() ) {
            _output << " -> ";
            printPostList( *precedence.getNext() );
            _count += 2;
        } else {
            _count += 1;
        }
        if ( _count < _size || !_pending_reply_activities.empty() ) {
            _output << ";";
        }
        _output << std::endl;
    }

    void
    SRVN::ActivityListInput::printPreList( const DOM::ActivityList& precedence )          /* joins */
    {
        const std::vector<const DOM::Activity*>& list = precedence.getList();
        for ( std::vector<const DOM::Activity*>::const_iterator next_activity = list.begin(); next_activity != list.end(); ++next_activity ) {
            const DOM::Activity * activity = *next_activity;
            switch ( precedence.getListType() ) {
            case DOM::ActivityList::Type::AND_JOIN:
                if ( next_activity != list.begin() ) {
                    _output << " &";
                }
                break;

            case DOM::ActivityList::Type::OR_JOIN:
                if ( next_activity != list.begin() ) {
                    _output << " +";
                }
                break;

            case DOM::ActivityList::Type::JOIN:
                break;

            default:
                abort();
            }

            _output << " " << activity->getName();
            const std::vector<DOM::Entry*>& replies = activity->getReplyList();
            if ( !replies.empty() ) {
                printReplyList( replies );
		_pending_reply_activities.erase( activity );	// No longer pending.
            }
        }
    }

    void
    SRVN::ActivityListInput::printPostList( const DOM::ActivityList& precedence )         /* forks */
    {
        const DOM::Activity * end_activity = nullptr;

        const std::vector<const DOM::Activity*>& list = precedence.getList();
        for ( std::vector<const DOM::Activity*>::const_iterator next = list.begin(); next != list.end(); ++next ) {
            const DOM::Activity * activity = *next;
	    if ( !activity->getReplyList().empty() ) {
		_pending_reply_activities.insert( activity );
	    }
            switch ( precedence.getListType() ) {
            case DOM::ActivityList::Type::AND_FORK:
                if ( next != list.begin() ) {
                    _output << " & ";
                }
                break;

            case DOM::ActivityList::Type::OR_FORK:
                if ( next != list.begin() ) {
                    _output << " + ";
                }
                _output << "(" << precedence.getParameterValue( activity ) << ") ";
                break;

            case DOM::ActivityList::Type::FORK:
                break;

            case DOM::ActivityList::Type::REPEAT:
                if ( precedence.getParameter( activity ) == nullptr ) {
                    end_activity = activity;
                    continue;
                }
                if ( next != list.begin() ) {
                    _output << " , ";
                }
                _output << precedence.getParameterValue( activity ) << " * ";
                break;

            default:
                abort();
            }
            _output << activity->getName();
        }
        if ( end_activity ) {
            _output << ", " << end_activity->getName();
        }
    }

    /* -------------------------------------------------------------------- */
    /* Calls                                                                */
    /* -------------------------------------------------------------------- */

    /*
     * This one is tortuous because a given phase may call lots of different entries.
     * Note that if _meanFunc is NOT set, this function simply counts the number
     * of calls.
     */

    void
    SRVN::CallOutput::operator()( const DOM::Entity * entity) const
    {
        const DOM::Task * task = dynamic_cast<const DOM::Task *>(entity);
        if ( !task ) return;

        std::ios_base::fmtflags oldFlags = _output.setf( std::ios::left, std::ios::adjustfield );
        const std::vector<DOM::Entry *>& entries = task->getEntryList();
        std::vector<DOM::Entry *>::const_iterator nextEntry;
        bool print_task_name = true;
        for ( nextEntry = entries.begin(); nextEntry != entries.end(); ++nextEntry ) {
            const DOM::Entry * entry = *nextEntry;

            /* Gather up all the call info over all phases and store in new map<to_entry,call*[3]>. */
            std::map<const DOM::Entry *, ForPhase> callsByPhase;
            const std::map<unsigned, DOM::Phase*>& phases = entry->getPhaseList();
            assert( phases.size() <= DOM::Phase::MAX_PHASE );
	    std::for_each( phases.begin(), phases.end(), CollectCalls( callsByPhase, _testFunc ) );

            /* Now iterate over the collection of calls */
	    if ( _meanFunc ) {
		for ( std::map<const DOM::Entry *, ForPhase>::iterator next_y = callsByPhase.begin(); next_y != callsByPhase.end(); ++next_y ) {
		    ForPhase& calls_by_phase = next_y->second;
		    calls_by_phase.setMaxPhase( __parseable ? __maxPhase : entry->getMaximumPhase() );
		    _output << entity_name( *(entity), print_task_name )
			    << entry_name( *entry )
			    << entry_name( *(next_y->first) )
			    << print_calls( *this, calls_by_phase, _meanFunc) << newline;
		    if ( _confFunc && __conf95 ) {
			_output << conf_level( __maxStrLen*3, ConfidenceIntervals::CONF_95 )
				<< print_calls( *this, calls_by_phase, _confFunc, __conf95 ) << newline;
		    }
		    if ( _confFunc && __conf99 ) {
			_output << conf_level( __maxStrLen*3, ConfidenceIntervals::CONF_99 )
				<< print_calls( *this, calls_by_phase, _confFunc, __conf99 ) << newline;
		    }
		}
            } else {
		_count += callsByPhase.size();
	    }
        }
        /* Now repeat for activities */
        const std::map<std::string,DOM::Activity*>& activities = task->getActivities();
        if ( activities.size() > 0 ) {
            unsigned count = 0;
            std::map<std::string,DOM::Activity*>::const_iterator nextActivity;
            for ( nextActivity = activities.begin(); nextActivity != activities.end(); ++nextActivity ) {
                const DOM::Activity * activity = nextActivity->second;
                const std::vector<DOM::Call *>& calls = activity->getCalls();
                for ( std::vector<DOM::Call*>::const_iterator nextCall = calls.begin(); nextCall != calls.end(); ++nextCall ) {
                    const DOM::Call * call = *nextCall;
		    if ( call == nullptr || !(call->*_testFunc)() ) {
			continue;
		    } else if ( _meanFunc ) {
			const DOM::Entry * dest = call->getDestinationEntry();
                        if ( count == 0 ) {
                            _output << entity_name( *(entity), print_task_name ) << activity_separator(0) << newline;
                            count += 1;
                        }
                        _output << std::setw(__maxStrLen) << " "  << std::setw(__maxStrLen-1) << activity->getName() << " " << entry_name( *dest ) << std::setw(__maxDblLen-1);
                        (this->*_meanFunc)( call, 0 );
                        _output << " " << activityEOF << newline;
                        if ( _confFunc && __conf95 ) {
                            _output << conf_level( __maxStrLen*3, ConfidenceIntervals::CONF_95 ) << std::setw(__maxDblLen-1);
                            (this->*_confFunc)( call, __conf95 );
                            _output << " " << activityEOF << newline;
                        }
                        if ( _confFunc && __conf99 ) {
                            _output << conf_level( __maxStrLen*3, ConfidenceIntervals::CONF_99 ) << std::setw(__maxDblLen-1);
                            (this->*_confFunc)( call, __conf99 );
                            _output << " " << activityEOF << newline;
                        }
                    } else {
			_count += 1;
		    }
                }
            }
            if ( __parseable && _meanFunc && count > 1 ) {
                _output << std::setw(__maxStrLen) << " " << activityEOF << newline;
            }
        }
        if ( __parseable && _meanFunc && print_task_name == false ) {
            _output << std::setw(__maxStrLen) << " " << activityEOF << newline;
        }
        _output.flags(oldFlags);
    }


    void
    SRVN::CallOutput::printCallRate( const DOM::Call * call, const ConfidenceIntervals* ) const
    {
        if ( call ) {
	    _output << Input::print_double_parameter( call->getCallMean() );
        } else {
            _output << 0.0;
        }
    }

    void
    SRVN::CallOutput::printCallWaiting( const DOM::Call * call, const ConfidenceIntervals* ) const
    {
        double value = 0;
        if ( call ) {
	    value = call->getResultWaitingTime();
        }
	_output << std::setw(__maxDblLen) << value;
    }

    void
    SRVN::CallOutput::printCallWaitingConfidence( const DOM::Call * call, const ConfidenceIntervals* conf ) const
    {
        double value = 0;
        if ( call && conf ) {
	    value = (*conf)(call->getResultWaitingTimeVariance());
        }
	_output << std::setw(__maxDblLen) << value;
    }

    void
    SRVN::CallOutput::printCallVarianceWaiting( const DOM::Call * call, const ConfidenceIntervals* ) const
    {
        double value = 0;
        if ( call ) {
	    value = call->getResultVarianceWaitingTime();
        }
        _output << std::setw(__maxDblLen) << value;
    }

    void
    SRVN::CallOutput::printCallVarianceWaitingConfidence( const DOM::Call * call, const ConfidenceIntervals* conf ) const
    {
        double value = 0;
        if ( call && conf ) {
	    value = call->getResultVarianceWaitingTimeVariance();
        }
        _output << std::setw(__maxDblLen) << (*conf)(value);
    }

    void
    SRVN::CallOutput::printDropProbability( const DOM::Call * call, const ConfidenceIntervals* ) const
    {
        double value = 0.0;
        if ( call ) {
	    value = call->getResultDropProbability();
        }
	_output << std::setw(__maxDblLen) << value;
    }

    void
    SRVN::CallOutput::printDropProbabilityConfidence( const DOM::Call * call, const ConfidenceIntervals* conf ) const
    {
        double value = 0;
        if ( call && conf ) {
	    value = (*conf)(call->getResultDropProbabilityVariance());
        }
        _output << std::setw(__maxDblLen) << value;
    }

    std::ostream&
    SRVN::CallOutput::printCalls( std::ostream& output, const CallOutput& info, const ForPhase& phases, const callConfFPtr func, const ConfidenceIntervals* conf )
    {
        for ( unsigned p = 1; p <= phases.getMaxPhase(); ++p ) {
            output << std::setw(__maxDblLen-1);
            (info.*func)( phases[p], conf );
	    output << " ";
        }
        output << activityEOF;
        return output;
    }

    void
    SRVN::ActivityCallInput::print( const DOM::Call* call ) const
    {
	const DOM::Activity * src = dynamic_cast<const DOM::Activity *>(call->getSourceObject());
	assert(src != nullptr );
	const DOM::Entry * dst = call->getDestinationEntry();
	try {
	    _output << "  " << call_type( call ) << " " << src->getName() << " " << dst->getName() << number_of_calls( call );
	}
	catch ( const std::domain_error& e ) {
	    call->throw_invalid_parameter( "rate", e.what() );
	}
	if ( !call->getDocument()->instantiated() ) {
	    printObservationVariables( _output, *call );
	}
	_output << std::endl;
    }

    /* -------------------------------------------------------------------- */
    /* Histograms                                                           */
    /* -------------------------------------------------------------------- */

    void
    SRVN::HistogramOutput::operator()( const DOM::Entity * entity ) const
    {
        if ( __parseable ) return;

        const DOM::Task * task = dynamic_cast<const DOM::Task *>(entity);
        if ( !task ) return;

        const std::vector<DOM::Entry *>& entries = task->getEntryList();
        std::vector<DOM::Entry *>::const_iterator nextEntry;
        for ( nextEntry = entries.begin(); nextEntry != entries.end(); ++nextEntry ) {
            const DOM::Entry * entry = *nextEntry;

            for ( unsigned int p = 1; p <= DOM::Phase::MAX_PHASE; ++p ) {               /* BUG_668 */
                if ( entry->hasHistogramForPhase( p ) ) {
                    _output << "Service time histogram for entry " << entry->getName() << ", phase " << p << newline;
                    (this->*_func)( *entry->getHistogramForPhase( p ) );
                }
            }

            const std::map<unsigned, DOM::Phase*>& phases = entry->getPhaseList();
            assert( phases.size() <= DOM::Phase::MAX_PHASE );
            for (std::map<unsigned, DOM::Phase*>::const_iterator p = phases.begin(); p != phases.end(); ++p) {
                const DOM::Phase* phase = p->second;
                const std::vector<DOM::Call*>& calls = phase->getCalls();
                for ( std::vector<DOM::Call*>::const_iterator c = calls.begin(); c != calls.end(); ++c ) {
                    const DOM::Call * call = *c;
                    if ( call->hasHistogram() ) {
                        _output << "Queue Length histogram for entry " << entry->getName() << ", phase " << p->first << ", call to " << call->getDestinationEntry()->getName() << newline;
                        (this->*_func)( *call->getHistogram() );
                    }
                }
            }
        }

        const std::map<std::string,DOM::Activity*>& activities = task->getActivities();
        std::map<std::string,DOM::Activity*>::const_iterator nextActivity;
        for ( nextActivity = activities.begin(); nextActivity != activities.end(); ++nextActivity ) {
            const DOM::Activity * activity = nextActivity->second;
            if ( activity->hasHistogram() ) {
                _output << "Service time histogram for task " << task->getName() << ", activity " << activity->getName() << newline << newline;
                (this->*_func)( *activity->getHistogram() );
            }

            const std::vector<DOM::Call*>& calls = activity->getCalls();
            for ( std::vector<DOM::Call*>::const_iterator c = calls.begin(); c != calls.end(); ++c ) {
                const DOM::Call * call = *c;
                if ( call->hasHistogram() ) {
                    _output << "Queue length histogram for task " << task->getName() << ", activity " << activity->getName() << ", call to " << call->getDestinationEntry()->getName() << newline;
                    (this->*_func)( *call->getHistogram() );
                }
            }
        }

        const std::set<DOM::ActivityList*>& activity_lists = task->getActivityLists();
        for ( std::set<DOM::ActivityList*>::const_iterator  list = activity_lists.begin(); list != activity_lists.end(); ++list ) {
            if ( (*list)->hasHistogram() ) {
                const DOM::Activity * first;
                const DOM::Activity * last;
                (*list)->activitiesForName( first, last );
                _output << "Histogram for task " << task->getName() << ", join activity list " << first->getName() << " to " << last->getName() << newline << newline;
                (this->*_func)( *(*list)->getHistogram() );
            }
        }

        if ( task->hasHistogram() ) {
            _output << "Histogram for task " << task->getName() << newline << newline;
            (this->*_func)( *task->getHistogram() );
        }
    }



    void
    SRVN::HistogramOutput::printHistogram( const DOM::Histogram& histogram ) const
    {
        static const double plot_width = 20;
        const unsigned hist_bins = histogram.getBins();
        if ( hist_bins == 0 ) return;

        const double hist_min = histogram.getMin();
        const double hist_max = histogram.getMax();
        const double bin_size = hist_bins > 0 ? (hist_max - hist_min)/static_cast<double>(hist_bins) : 0;

        double max_value = 0;

        /* Find biggest bin. */
        const unsigned limit = histogram.getOverflowIndex();
        for ( unsigned int i = 0; i <= limit; i++ ) {
            const double value = histogram.getBinMean( i );
            if ( value > max_value ) {
                max_value = value;
            }
        }

        if ( histogram.getHistogramType() == DOM::Histogram::Type::CONTINUOUS ) {
            _output << std::setw(4) << " " << std::setw( 17 ) << "<=  bin  <";
        } else {
            _output << "  bin  ";
        }
        _output  << " " << std::setw(9) << "mean";
        if ( __conf95 ) {
            _output << " " << std::setw(9) << "+/- 95%";
        }
        _output << newline;
        for ( unsigned int i = ( hist_min == 0 ? 1 : 0); i <= limit; i++ ) {
            const double mean = histogram.getBinMean( i );
            if ( i == limit && mean == 0 ) break;

            const double x1 = ( i == 0 ) ? 0 : hist_min + (i-1) * bin_size;
            const std::streamsize old_precision = _output.precision( 6 );
            if ( histogram.getHistogramType() == DOM::Histogram::Type::CONTINUOUS ) {
                const double x2 = ( i == limit ) ? std::numeric_limits<double>::max() : hist_min + i * bin_size;
                _output << std::setw( 4 ) << " ";
                if ( i == 0 ) {
                    _output << std::setw( 17 ) << "underflow";
                } else if ( i == limit ) {
                    _output << std::setw( 17 ) << "overflow";
                } else {
                    _output << std::setw( 8 ) << x1 << " " << std::setw( 8 ) << x2;
                }
            } else {
                _output << "  " << std::setw(3) << x1 << "  ";
            }
            _output.setf( std::ios::fixed, std::ios::floatfield );
            _output << " " << std::setw( 9 ) << mean;
            if ( __conf95 ) {
                _output << " " << std::setw( 9 ) << (*__conf95)(histogram.getBinVariance( i ));
            }
            _output.unsetf( std::ios::floatfield );
            _output.precision( old_precision );
            _output << "|";
            const unsigned int count = static_cast<unsigned int>( mean * plot_width / max_value + 0.5 );
            if ( count > 0 ) {
                _output << std::setw( count ) << std::setfill( '*' ) << '*' << std::setfill( ' ' );
            }
            _output << newline;
        }
        _output << newline;
    }
}
