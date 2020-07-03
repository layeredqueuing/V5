/*
 *  $Id: srvn_output.cpp 13550 2020-05-22 11:48:05Z greg $
 *
 * Copyright the Real-Time and Distributed Systems Group,
 * Department of Systems and Computer Engineering,
 * Carleton University, Ottawa, Ontario, Canada. K1S 5B6
 *
 */

#if defined(HAVE_CONFIG_H)
#include <config.h>
#endif
#include <iostream>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <cmath>
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#if HAVE_PWD_H
#include <pwd.h>
#endif
#include <lqx/SyntaxTree.h>
#include "srvn_output.h"
#include "common_io.h"
#include "srvn_spex.h"
#include "glblerr.h"
#include "dom_extvar.h"
#include "dom_document.h"
#include "dom_processor.h"
#include "dom_histogram.h"
#include "dom_task.h"
#include "dom_entry.h"
#include "dom_phase.h"
#include "dom_call.h"
#include "dom_activity.h"
#include "dom_actlist.h"
#include "input.h"
#include "labels.h"

namespace LQIO {
    using namespace std;

    unsigned int SRVN::ObjectOutput::__maxPhase = 0;
    ConfidenceIntervals * SRVN::ObjectOutput::__conf95 = 0;
    ConfidenceIntervals * SRVN::ObjectOutput::__conf99 = 0;
    bool SRVN::ObjectOutput::__parseable = false;
    bool SRVN::ObjectOutput::__rtf = false;
    bool SRVN::ObjectOutput::__coloured = false;
    unsigned int SRVN::ObjectInput::__maxEntLen = 1;
    unsigned int SRVN::ObjectInput::__maxInpLen = 1;

    static inline void throw_bad_parameter() { throw std::domain_error( "invalid parameter" ); }

    SRVN::Output::Output( const DOM::Document& document, const map<unsigned, DOM::Entity *>& entities,
                          bool print_confidence_intervals, bool print_variances, bool print_histograms )
        : _document(document), _entities(entities),
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
    ostream&
    SRVN::Output::newline( ostream& output )
    {
        return ObjectOutput::newline( output );
    }

    ostream&
    SRVN::Output::textrm( ostream& output )
    {
        return ObjectOutput::textrm( output );
    }

    ostream&
    SRVN::Output::textbf( ostream& output )
    {
        return ObjectOutput::textbf( output );
    }

    ostream&
    SRVN::Output::textit( ostream& output )
    {
        return ObjectOutput::textit( output );
    }

    /*
     * Print out a generic header for Entry name on fptr.
     */

    ostream&
    SRVN::Output::taskHeader( ostream& output, const std::string& s )
    {
        output << newline << newline << textbf << s << newline << newline << textrm
               << setw(ObjectOutput::__maxStrLen) << "Task Name" << setw(ObjectOutput::__maxStrLen) << "Entry Name";
        return output;
    }

    /* static */ ostream&
    SRVN::Output::entryHeader( ostream& output, const std::string& s )
    {
        output << task_header( s ) << phase_header( ObjectOutput::__maxPhase );
        return output;
    }

    ostream&
    SRVN::Output::activityHeader( ostream& output, const std::string& s )
    {
        output << newline << newline << s << newline << newline
               << setw(ObjectOutput::__maxStrLen) << "Task Name"
               << setw(ObjectOutput::__maxStrLen) << "Source Activity"
               << setw(ObjectOutput::__maxStrLen) << "Target Activity";
        return output;
    }

    ostream&
    SRVN::Output::callHeader( ostream& output, const std::string& s )
    {
        output << newline << newline << textbf << s << newline << newline << textrm
               << setw(ObjectOutput::__maxStrLen) << "Task Name"
               << setw(ObjectOutput::__maxStrLen) << "Source Entry"
               << setw(ObjectOutput::__maxStrLen) << "Target Entry";
        return output;
    }

    ostream&
    SRVN::Output::phaseHeader( ostream& output, const unsigned int n_phases )
    {
        ios_base::fmtflags oldFlags = output.setf( ios::left, ios::adjustfield );
        for ( unsigned p = 1; p <= n_phases; p++ ) {
            ostringstream aString;
            aString << "Phase " << p;
            for ( unsigned int i = aString.str().length(); i < ObjectOutput::__maxDblLen; ++i ) {
                aString << ' ';
            }
            output << aString.str();
        }
        output.flags(oldFlags);
        return output;
    }

    ostream&
    SRVN::Output::holdHeader( ostream& output, const std::string& s )
    {
        ios_base::fmtflags oldFlags = output.setf( ios::left, ios::adjustfield );
        output << newline << newline << textbf << s << newline << newline << textrm
               << setw(ObjectOutput::__maxStrLen) << "Task Name"
               << setw(ObjectOutput::__maxStrLen) << "Wait Entry"
               << setw(ObjectOutput::__maxStrLen) << "Signal Entry"
               << setw(ObjectOutput::__maxDblLen) << "Hold Time"
               << setw(ObjectOutput::__maxDblLen) << "Variance"
               << setw(ObjectOutput::__maxDblLen) << "Utilization";
        output.flags(oldFlags);
        return output;
    }

    ostream&
    SRVN::Output::rwlockHeader( ostream& output, const std::string& s )
    {
        ios_base::fmtflags oldFlags = output.setf( ios::left, ios::adjustfield );
        output << newline << newline << s << newline << newline
               << setw(ObjectOutput::__maxStrLen) << "Task Name"
               << setw(ObjectOutput::__maxStrLen) << "Lock Entry"
               << setw(ObjectOutput::__maxStrLen) << "Unlock Entry"
               << setw(ObjectOutput::__maxDblLen) << "Blocked Time"
               << setw(ObjectOutput::__maxDblLen) << "Variance"
               << setw(ObjectOutput::__maxDblLen) << "Hold Time"
               << setw(ObjectOutput::__maxDblLen) << "Variance"
               << setw(ObjectOutput::__maxDblLen) << "Utilization";
        output.flags(oldFlags);
        return output;
    }

    /* ---------------------------------------------------------------- */
    /* MAIN code for printing                                           */
    /* ---------------------------------------------------------------- */

    ostream&
    SRVN::Output::print( ostream& output ) const
    {
        printPreamble( output );
        printInput( output );
        printResults( output );
        printPostamble( output );
        return output;
    }


    ostream&
    SRVN::Output::printPreamble( ostream& output ) const
    {
        DocumentOutput print( output );
        print( getDOM() );
        return output;
    }

    /* Echo of input. */
    ostream&
    SRVN::Output::printInput( ostream& output ) const
    {
        ios_base::fmtflags oldFlags = output.setf( ios::left, ios::adjustfield );

        output << newline << textbf << processor_info_str << newline << newline
               << textrm << setw(ObjectOutput::__maxStrLen) << "Processor Name" << "Type    Copies  Scheduling";
        if ( getDOM().processorHasRate() ) {
            output << " Rate";
        }
        output << newline;
        for_each( _entities.begin(), _entities.end(), ProcessorOutput( output, &ProcessorOutput::printParameters ) );

        const std::map<std::string,DOM::Group*>& groups = getDOM().getGroups();
        if ( groups.size() > 0 ) {
            output << newline << textbf << group_info_str << newline << newline
                   << setw(ObjectOutput::__maxStrLen) << "Group Name" << "Share       Processor Name" << newline << textrm;
            for_each( groups.begin(), groups.end(), GroupOutput( output, &GroupOutput::printParameters ) );
        }

        output << newline << newline << textbf << task_info_str << newline << newline
               << textrm << setw(ObjectOutput::__maxStrLen) << "Task Name" << "Type    Copies  " << setw(ObjectOutput::__maxStrLen) << "Processor Name";
        if ( getDOM().getNumberOfGroups() > 0 ) {
            output << setw(ObjectOutput::__maxStrLen) << "Group Name";
        }
        output << "Pri ";
        if ( getDOM().taskHasThinkTime() ) {
            output << setw(ObjectOutput::__maxDblLen) << "Think Time";
        }
        output << "Entry List" << newline;
        for_each( _entities.begin(), _entities.end(), TaskOutput( output, &TaskOutput::printParameters ) );

        output << entry_header( service_demand_str ) << newline;
        for_each( _entities.begin(), _entities.end(), EntryOutput( output, &EntryOutput::printEntryDemand, &EntryOutput::printActivityDemand  ) );

        if ( getDOM().hasThinkTime() ) {
            output << entry_header( think_time_str ) << newline;
            for_each( _entities.begin(), _entities.end(), EntryOutput( output, &EntryOutput::printEntryThinkTime, &EntryOutput::printActivityThinkTime ) );
        }

        if ( getDOM().hasMaxServiceTimeExceeded() ) {
            output << entry_header( max_service_time_str ) << newline;
            for_each( _entities.begin(), _entities.end(), EntryOutput( output, &EntryOutput::printEntryMaxServiceTime ) );
        }


        /* print mean number of Rendezvous */

        if ( getDOM().hasRendezvous() ) {
            output << call_header( rendezvous_rate_str ) << phase_header( ObjectOutput::__maxPhase ) << newline;
            for_each( _entities.begin(), _entities.end(), CallOutput( output, &DOM::Call::hasRendezvous, &CallOutput::printCallRate ) );
        }

        if ( getDOM().hasForwarding() > 0 ) {
            output << call_header( forwarding_probability_str ) << setw(ObjectOutput::__maxDblLen) << "Prob" << newline;
            for_each( _entities.begin(), _entities.end(), EntryOutput( output, &EntryOutput::printForwarding ) );
        }

        if ( getDOM().hasSendNoReply() ) {
            output << call_header( send_no_reply_rate_str ) << phase_header( ObjectOutput::__maxPhase ) << newline;
            for_each( _entities.begin(), _entities.end(), CallOutput( output, &DOM::Call::hasSendNoReply, &CallOutput::printCallRate ) );
        }

        if ( getDOM().hasDeterministicPhase() ) {
            output << entry_header( phase_type_str ) << newline;
            for_each( _entities.begin(), _entities.end(), EntryOutput( output, &EntryOutput::printEntryPhaseType, &EntryOutput::printActivityPhaseType ) );
        } else {
            output << newline << newline << textbf << phase_type_str << newline << textrm;
            output << "All phases are stochastic." << newline;
        }

        if ( getDOM().hasNonExponentialPhase() ) {
            output << entry_header( cv_square_str ) << newline << textrm;
            for_each( _entities.begin(), _entities.end(), EntryOutput( output, &EntryOutput::printEntryCoefficientOfVariation, &EntryOutput::printActivityCoefficientOfVariation ) );
        } else {
            output << newline << newline << textbf << cv_square_str << newline << textrm;
            output << "All executable segments are exponential." << newline;
        }

        output << newline << newline << textbf << open_arrival_rate_str << newline << textrm;
        if ( getDOM().hasOpenArrivals() ) {
            output << newline << setw(ObjectOutput::__maxStrLen) << "Entry Name" << setw(ObjectOutput::__maxDblLen) << "Arrival Rate" << newline;
            for_each( _entities.begin(), _entities.end(), EntryOutput( output, &EntryOutput::printOpenArrivals ) );
        } else {
            output << "All open arrival rates are 0." << newline;
        }
        output.flags(oldFlags);
        return output;
    }

    ostream&
    SRVN::Output::printResults( ostream& output ) const
    {
        ios_base::fmtflags oldFlags = output.setf( ios::left, ios::adjustfield );

        if ( getDOM().entryHasThroughputBound() ) {
            output << textbf << task_header( throughput_bounds_str ) << "Throughput  " << newline << textrm;
            for_each( _entities.begin(), _entities.end(), EntryOutput( output, &EntryOutput::printEntryThroughputBounds ) );
        }

        /* Waiting times */

        if ( getDOM().hasRendezvous() ) {
            output << call_header( waiting_time_str ) << phase_header( ObjectOutput::__maxPhase ) << newline;
            for_each( _entities.begin(), _entities.end(), CallOutput( output, &DOM::Call::hasRendezvous, &CallOutput::printCallWaiting, &CallOutput::printCallWaitingConfidence ) );
            if ( getDOM().entryHasWaitingTimeVariance() && _print_variances ) {
                output << call_header( waiting_time_variance_str ) << phase_header( ObjectOutput::__maxPhase ) << newline;
                for_each( _entities.begin(), _entities.end(), CallOutput( output, &DOM::Call::hasRendezvous, &CallOutput::printCallVarianceWaiting, &CallOutput::printCallVarianceWaitingConfidence ) );
            }
        }
	if ( getDOM().hasForwarding() ) {
            for_each( _entities.begin(), _entities.end(), EntryOutput( output, &EntryOutput::printForwardingWaiting ) );
	}
        if ( getDOM().hasSendNoReply() ) {
            output << call_header( snr_waiting_time_str ) << phase_header( ObjectOutput::__maxPhase ) << newline;
            for_each( _entities.begin(), _entities.end(), CallOutput( output, &DOM::Call::hasSendNoReply, &CallOutput::printCallWaiting, &CallOutput::printCallWaitingConfidence ) );
            if ( getDOM().entryHasWaitingTimeVariance() && _print_variances ) {
                output << call_header( snr_waiting_time_variance_str ) << phase_header( ObjectOutput::__maxPhase ) << newline;
                for_each( _entities.begin(), _entities.end(), CallOutput( output, &DOM::Call::hasSendNoReply, &CallOutput::printCallVarianceWaiting, &CallOutput::printCallVarianceWaitingConfidence ) );
            }
        }

        /* Drop probabilities. */

        if ( getDOM().entryHasDropProbability() ) {
            output << call_header( loss_probability_str ) << phase_header( ObjectOutput::__maxPhase ) << newline;
            for_each( _entities.begin(), _entities.end(), CallOutput( output, &DOM::Call::hasResultDropProbability, &CallOutput::printDropProbability, &CallOutput::printDropProbabilityConfidence ) );
        }

        /* Join delays */

        if ( getDOM().taskHasAndJoin() ) {
            output << activity_header( join_delay_str ) << "Mean        Variance" << newline;
            for_each( _entities.begin(), _entities.end(), TaskOutput( output, &TaskOutput::printJoinDelay ) );
        }

        /* Service time */
        output << entry_header( service_time_str ) << newline;
        for_each( _entities.begin(), _entities.end(), EntryOutput( output, &EntryOutput::printEntryServiceTime, &EntryOutput::printActivityServiceTime  ) );

        if ( getDOM().entryHasServiceTimeVariance() && _print_variances ) {
            output << entry_header( variance_str ) << "coeff of var **2" << newline;
            for_each( _entities.begin(), _entities.end(), EntryOutput( output, &EntryOutput::printEntryVarianceServiceTime, &EntryOutput::printActivityVarianceServiceTime  ) );
        }

        /* Histograms */

        if ( getDOM().hasMaxServiceTimeExceeded() ) {
            output << entry_header( service_time_exceeded_str ) << newline;
            for_each( _entities.begin(), _entities.end(), EntryOutput( output, &EntryOutput::printEntryMaxServiceTimeExceeded, &EntryOutput::printActivityMaxServiceTimeExceeded, &EntryOutput::testActivityMaxServiceTimeExceeded ) );
        }

        if ( getDOM().hasHistogram() && _print_histograms ) {
            output << newline << newline << histogram_str << newline << newline;
            for_each( _entities.begin(), _entities.end(), HistogramOutput( output, &HistogramOutput::printHistogram ) );
        }

        /* Semaphore holding times */

        if ( getDOM().hasSemaphoreWait() ) {
            output << hold_header( semaphore_hold_time_str ) << newline;
            for_each( _entities.begin(), _entities.end(), TaskOutput( output, &TaskOutput::printHoldTime ) );
        }

        /* RWLock holding times */

        if ( getDOM().hasRWLockWait() ) {
            output << rwlock_header( rwlock_hold_time_str ) << newline;
            for_each( _entities.begin(), _entities.end(), TaskOutput( output, &TaskOutput::printRWLOCKHoldTime ) );
        }


        /* Task Throughput and Utilization */

        output << task_header( throughput_str ) << setw(ObjectOutput::__maxDblLen) << "Throughput" << phase_header( ObjectOutput::__maxPhase ) << setw(ObjectOutput::__maxDblLen) << "Total" << newline;
        for_each( _entities.begin(), _entities.end(), TaskOutput( output, &TaskOutput::printThroughputAndUtilization ) );

        /* Open arrival wait times */

        if ( getDOM().entryHasOpenWait() ) {
            output << task_header( open_wait_str ) << setw(ObjectOutput::__maxDblLen) << "Lambda" << setw(ObjectOutput::__maxDblLen) << "Waiting time" << newline;
            for_each( _entities.begin(), _entities.end(), EntryOutput( output, &EntryOutput::printOpenQueueWait ) );
        }

        /* Processor utilization and waiting */

        for_each( _entities.begin(), _entities.end(), ProcessorOutput( output, &ProcessorOutput::printUtilizationAndWaiting ) );

        output.flags(oldFlags);
        return output;
    }


    ostream&
    SRVN::ObjectOutput::printEntityName( ostream& output, const DOM::Entity& entity, bool& print )
    {
        if ( print ) {
            output << setw(__maxStrLen-1) << entity.getName() << (__parseable ? ":" : " ");
            print = false;
        } else {
            output << setw(__maxStrLen) << " ";
        }
        return output;
    }

    ostream&
    SRVN::ObjectOutput::printEntryName( ostream& output, const DOM::Entry& entry )
    {
        output << setw(__maxStrLen-1) << entry.getName() << " ";
        return output;
    }

    ostream&
    SRVN::ObjectOutput::activitySeparator( ostream& output, const unsigned offset )
    {
        if ( __parseable ) {
            output << "-1";
            ios_base::fmtflags oldFlags = output.setf( ios::right, ios::adjustfield );
            output << newline << setw(offset) << ":";
            output.flags(oldFlags);
        } else {
            if ( offset > 0 ) {
                output << setw(offset) << " ";
            }
            output << setw(__maxStrLen) << "Activity Name";
        }
        return output;
    }

    ostream&
    SRVN::ObjectOutput::confLevel( ostream& output, const unsigned int fill, const ConfidenceIntervals::confidence_level_t level )
    {
        ios_base::fmtflags flags = output.setf( ios::right, ios::adjustfield );
        output << setw( fill-4 ) << ( __parseable ? " " : "+/-" );
        if ( level == ConfidenceIntervals::CONF_95 ) {
            output << (__parseable ? "%95 " : "95% " );
        } else {
            output << (__parseable ? "%99 " : "99% " );
        }
        output.flags( flags );
        return output;
    }

    ostream&
    SRVN::ObjectOutput::textcolour( ostream& output, const double utilization )
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

    ostream&
    SRVN::ObjectOutput::entryInfo( ostream& output, const DOM::Entry & entry, const entryFunc func )
    {
	output << setw(__maxDblLen) << Input::print_double_parameter( (entry.*func)(), 0. ) << ' ';	
        return output;
    }

    /* static */ ostream&
    SRVN::ObjectOutput::phaseInfo( ostream& output, const DOM::Entry & entry, const phaseFunc func )
    {
        const std::map<unsigned, DOM::Phase*>& phases = entry.getPhaseList();
        assert( phases.size() <= DOM::Phase::MAX_PHASE );
        std::map<unsigned, DOM::Phase*>::const_iterator p;
        for (p = phases.begin(); p != phases.end(); ++p) {
            const DOM::Phase* phase = p->second;
            if ( phase ) {
                output << setw(__maxDblLen-1) << Input::print_double_parameter( (phase->*func)(), 0. ) << ' ';
            } else {
                output << setw(__maxDblLen) << 0.0;
            }
        }
        return output;
    }

    /* static */ ostream&
    SRVN::ObjectOutput::phaseTypeInfo( ostream& output, const DOM::Entry & entry, const phaseTypeFunc func )
    {
        const std::map<unsigned, DOM::Phase*>& phases = entry.getPhaseList();
        assert( phases.size() <= DOM::Phase::MAX_PHASE );
        std::map<unsigned, DOM::Phase*>::const_iterator p;
        for (p = phases.begin(); p != phases.end(); ++p) {
            const DOM::Phase* phase = p->second;
            switch ( phase->getPhaseTypeFlag() ) {
            case PHASE_DETERMINISTIC: output << setw(__maxDblLen) << "determin"; break;
            case PHASE_STOCHASTIC:    output << setw(__maxDblLen) << "stochastic"; break;
            }
        }
        return output;
    }

    /* static */ ostream&
    SRVN::ObjectOutput::phaseResults( ostream& output, const DOM::Entry & entry, const doublePhaseFunc phase_func, const doubleEntryPhaseFunc entry_func, const bool pad )
    {
        unsigned int np = 0;
        const std::map<unsigned, DOM::Phase*>& phases = entry.getPhaseList();
        assert( phases.size() <= DOM::Phase::MAX_PHASE );
        if ( entry.getStartActivity() ) {
            if ( !entry_func ) return output;
            for ( unsigned int p = 1; p <= __maxPhase; ++p ) {
                output << setw(__maxDblLen-1) << (entry.*entry_func)(p) << ' ';
            }
            np = __maxPhase;
        } else {
            for ( std::map<unsigned, DOM::Phase*>::const_iterator p = phases.begin(); p != phases.end(); ++p) {
                const DOM::Phase* phase = p->second;
                output << setw(__maxDblLen-1) << (phase->*phase_func)() << ' ';
            }
            np = phases.size();
        }
        if ( pad || __parseable ) {
            for ( unsigned p = np; p < __maxPhase; ++p ) {
                output << setw(__maxDblLen) << (__parseable ? "0 " : " ");
            }
        }
        output << activityEOF;
        return output;
    }


    /* static */ ostream&
    SRVN::ObjectOutput::phaseResultsConfidence( ostream& output, const DOM::Entry & entry, const doublePhaseFunc phase_func, const doubleEntryPhaseFunc entry_func, const ConfidenceIntervals * conf, const bool pad )
    {
        unsigned int np = 0;
        const std::map<unsigned, DOM::Phase*>& phases = entry.getPhaseList();
        assert( phases.size() <= DOM::Phase::MAX_PHASE );
        if ( entry.getStartActivity() ) {
            if ( !entry_func ) return output;
            for ( unsigned int p = 1; p <= __maxPhase; ++p ) {
                output << setw(__maxDblLen-1) << (*conf)((entry.*entry_func)(p)) << ' ';
            }
            np = __maxPhase;
        } else {
            for ( std::map<unsigned, DOM::Phase*>::const_iterator p = phases.begin(); p != phases.end(); ++p) {
                const DOM::Phase* phase = p->second;
                output << setw(__maxDblLen-1) << (*conf)((phase->*phase_func)()) << ' ';
            }
            np = phases.size();
        }
        if ( pad || __parseable ) {
            for ( unsigned p = np; p < __maxPhase; ++p ) {
                output << setw(__maxDblLen) << (__parseable ? "0" : " ");
            }
        }
        output << activityEOF;
        return output;
    }

    /* static */ ostream&
    SRVN::ObjectOutput::taskPhaseResults( ostream& output, const DOM::Task & task, const doubleTaskFunc func, const bool pad )
    {
        for ( unsigned p = 1; p <= task.getResultPhaseCount(); ++p ) {
            output << setw(__maxDblLen-1) << (task.*func)(p) << ' ';
        }
        if ( pad || __parseable ) {
            for ( unsigned p = task.getResultPhaseCount(); p < __maxPhase; ++p ) {
                output << setw(__maxDblLen) << (__parseable ? "0" : " ");
            }
        }
        output << activityEOF;
        return output;
    }


    /* static */ ostream&
    SRVN::ObjectOutput::task3PhaseResults( ostream& output, const DOM::Task & task, const doubleTaskFunc func, const bool pad )
    {
        for ( unsigned p = 1; p <= DOM::Phase::MAX_PHASE; ++p ) {
            output << setw(__maxDblLen-1) << (task.*func)(p) << ' ';
        }
        output << activityEOF;
        return output;
    }


    /* static */ ostream&
    SRVN::ObjectOutput::taskPhaseResultsConfidence( ostream& output, const DOM::Task & task, const doubleTaskFunc func, const ConfidenceIntervals * conf, const bool pad  )
    {
        for ( unsigned p = 1; p <= task.getResultPhaseCount(); ++p ) {
            output << setw(__maxDblLen-1) << (*conf)((task.*func)(p)) << ' ';
        }
        if ( pad || __parseable ) {
            for ( unsigned p = task.getResultPhaseCount(); p < __maxPhase; ++p ) {
                output << setw(__maxDblLen) << (__parseable ? "0" : " ");
            }
        }
        output << activityEOF;
        return output;
    }

    /* static */ ostream&
    SRVN::ObjectOutput::task3PhaseResultsConfidence( ostream& output, const DOM::Task & task, const doubleTaskFunc func, const ConfidenceIntervals * conf, const bool pad  )
    {
        for ( unsigned p = 1; p <= DOM::Phase::MAX_PHASE; ++p ) {
            output << setw(__maxDblLen-1) << (*conf)((task.*func)(p)) << ' ';
        }
        output << activityEOF;
        return output;
    }

    void
    SRVN::ObjectInput::printReplyList( const std::vector<DOM::Entry*>& replies ) const
    {
        _output << "[";
        for ( std::vector<DOM::Entry *>::const_iterator next_entry = replies.begin(); next_entry != replies.end(); ++next_entry ) {
            if ( next_entry != replies.begin() ) {
                _output << ",";
            }
            _output << (*next_entry)->getName();
        }
        _output << "]";
    }

    ostream&
    SRVN::ObjectInput::printNumberOfCalls( ostream& output, const DOM::Call* call )
    {
	output << " " << setw(ObjectInput::__maxInpLen);
	if ( call != nullptr ) {
	    output << Input::print_double_parameter( call->getCallMean(), 0. );
	} else {
	    output << 0;
	}
	return output;
    }


    ostream&
    SRVN::ObjectInput::printCallType( ostream& output, const DOM::Call* call )
    {
        switch ( call->getCallType() ) {
        case DOM::Call::RENDEZVOUS: output << "y"; break;
        case DOM::Call::SEND_NO_REPLY: output << "z"; break;
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
	std::pair<Spex::obs_var_tab_t::const_iterator, Spex::obs_var_tab_t::const_iterator> range = Spex::get_observations().equal_range( &object );
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

    SRVN::Parseable::Parseable( const DOM::Document& document, const map<unsigned, DOM::Entity *>& entities, bool print_confidence_intervals )
        : SRVN::Output( document, entities, print_confidence_intervals )
    {
        ObjectOutput::__parseable = true;               /* Set global for formatting. */
        ObjectOutput::__rtf = false;                    /* Set global for formatting. */
    }

    SRVN::Parseable::~Parseable()
    {
        ObjectOutput::__parseable = false;              /* Set global for formatting. */
    }

    ostream&
    SRVN::Parseable::print( ostream& output ) const
    {
        printPreamble( output );
        printResults( output );
        return output;
    }


    ostream&
    SRVN::Parseable::printPreamble( ostream& output ) const
    {
        output << "# " << DOM::Document::io_vars->lq_toolname << " " << DOM::Document::io_vars->lq_version << endl;
        if ( DOM::Document::io_vars->lq_command_line ) {
            output << "# " << DOM::Document::io_vars->lq_command_line << ' ' << DOM::Document::__input_file_name << endl;
        }
	output << "# " << DOM::Common_IO::svn_id() << std::endl;
        const DOM::Document& document(getDOM());
        output << "V " << (document.getResultValid() ? "y" : "n") << endl
               << "C " << document.getResultConvergenceValue() << endl
               << "I " << document.getResultIterations() << endl
               << "PP " << document.getNumberOfProcessors() << endl
               << "NP " << document.getMaximumPhase() << endl << endl;

	if ( document.getSymbolExternalVariableCount() > 0 ) {
	    output << "# ";		/* Treat it as a comment. */
	    document.printExternalVariables( output );
	    output << endl << endl;
	}

	const char * comment = document.getModelCommentString();
	if ( comment != NULL && comment[0] != '\0' ) {
	    output << "#!Comment: " << print_comment( comment ) << endl;
	}
	if ( document.getResultUserTime() > 0.0 ) {
	    output << "#!User: " << DOM::CPUTime::print( document.getResultUserTime() ) << endl;
	}
	if ( document.getResultSysTime() > 0.0 ) {
	    output << "#!Sys:  " << DOM::CPUTime::print( document.getResultSysTime() ) << endl;
	}
	output << "#!Real: " << DOM::CPUTime::print( document.getResultElapsedTime() ) << endl;
	if ( document.getResultMaxRSS() > 0 ) {
	    output << "#!MaxRSS: " << document.getResultMaxRSS() << endl;
	}
	const LQIO::DOM::MVAStatistics& mva_info = document.getResultMVAStatistics();
	if ( mva_info.getNumberOfSubmodels() > 0 ) {
            output << "#!Solver: "
                   << mva_info.getNumberOfSubmodels() << ' '
                   << mva_info.getNumberOfCore() << ' '
                   << mva_info.getNumberOfStep() << ' '
                   << mva_info.getNumberOfStepSquared() << ' '
                   << mva_info.getNumberOfWait() << ' '
                   << mva_info.getNumberOfWaitSquared() << ' '
                   << mva_info.getNumberOfFaults() << endl;
        }
        output << endl;
        return output;
    }


    ostream&
    SRVN::Parseable::printResults( ostream& output ) const
    {
        ios_base::fmtflags oldFlags = output.setf( ios::left, ios::adjustfield );

        if ( getDOM().entryHasThroughputBound() ) {
            output << "B " << getDOM().getNumberOfEntries() << endl;
            for_each( _entities.begin(), _entities.end(), EntryOutput( output, &EntryOutput::printEntryThroughputBounds ) );
            output << "-1" << endl << endl;
        }

        /* Waiting times */

        if ( getDOM().hasRendezvous() ) {
	    unsigned int count = for_each( _entities.begin(), _entities.end(), CallOutput( output, &DOM::Call::hasRendezvous ) ).getCount();
            output << "W " << count << endl;
            for_each( _entities.begin(), _entities.end(), CallOutput( output, &DOM::Call::hasRendezvous, &CallOutput::printCallWaiting, &CallOutput::printCallWaitingConfidence ) );
            output << "-1" << endl << endl;

            if ( getDOM().entryHasWaitingTimeVariance() ) {
                output << "VARW " << count << endl;
                for_each( _entities.begin(), _entities.end(), CallOutput( output, &DOM::Call::hasRendezvous, &CallOutput::printCallVarianceWaiting, &CallOutput::printCallVarianceWaitingConfidence ) );
                output << "-1" << endl << endl;
            }
        }
        if ( getDOM().hasSendNoReply() ) {
	    unsigned int count = for_each( _entities.begin(), _entities.end(), CallOutput( output, &DOM::Call::hasSendNoReply ) ).getCount();
            output << "Z " << count << endl;
            for_each( _entities.begin(), _entities.end(), CallOutput( output, &DOM::Call::hasSendNoReply, &CallOutput::printCallWaiting, &CallOutput::printCallWaitingConfidence ) );
            output << "-1" << endl << endl;
            if ( getDOM().entryHasWaitingTimeVariance() ) {
                output << "VARZ " << count << endl;
                for_each( _entities.begin(), _entities.end(), CallOutput( output, &DOM::Call::hasSendNoReply, &CallOutput::printCallVarianceWaiting, &CallOutput::printCallVarianceWaitingConfidence ) );
                output << "-1" << endl << endl;
            }
        }

        /* Drop probabilities. */

        if ( getDOM().entryHasDropProbability() ) {
	    unsigned int count = for_each( _entities.begin(), _entities.end(), CallOutput( output, &DOM::Call::hasResultDropProbability ) ).getCount();
            output << "DP " << count << endl;
            for_each( _entities.begin(), _entities.end(), CallOutput( output, &DOM::Call::hasResultDropProbability, &CallOutput::printDropProbability, &CallOutput::printDropProbabilityConfidence ) );
            output << "-1" << endl << endl;
        }

        /* Join delays */

        if ( getDOM().taskHasAndJoin() ) {
            output << "J " << 0 << endl;
            for_each( _entities.begin(), _entities.end(), TaskOutput( output, &TaskOutput::printJoinDelay ) );
            output << "-1" << endl << endl;
        }

        /* Service time */
        output << "X " << getDOM().getNumberOfEntries() << endl;
        for_each( _entities.begin(), _entities.end(), EntryOutput( output, &EntryOutput::printEntryServiceTime, &EntryOutput::printActivityServiceTime) );
        output << "-1" << endl << endl;

        if ( getDOM().entryHasServiceTimeVariance() ) {
            output << "VAR " << getDOM().getNumberOfEntries() << endl;
            for_each( _entities.begin(), _entities.end(), EntryOutput( output, &EntryOutput::printEntryVarianceServiceTime, &EntryOutput::printActivityVarianceServiceTime  ) );
            output << "-1" << endl << endl;
        }

        if ( getDOM().hasMaxServiceTimeExceeded() ) {
            output << "E " << 0 << endl;
            for_each( _entities.begin(), _entities.end(), EntryOutput( output, &EntryOutput::printEntryMaxServiceTimeExceeded, &EntryOutput::printActivityMaxServiceTimeExceeded, &EntryOutput::testActivityMaxServiceTimeExceeded ) );
            output << "-1" << endl << endl;
        }

        /* Semaphore holding times */

        if ( getDOM().hasSemaphoreWait() ) {
            output << "H " << 0 << endl;
            for_each( _entities.begin(), _entities.end(), TaskOutput( output, &TaskOutput::printHoldTime ) );
            output << "-1" << endl << endl;
        }

        /* Rwlock holding times */

        if ( getDOM().hasRWLockWait() ) {
            output << "L " << 0 << endl;
            for_each( _entities.begin(), _entities.end(), TaskOutput( output, &TaskOutput::printRWLOCKHoldTime ) );
            output << "-1" << endl << endl;
        }

        /* Task Throughput and Utilization */

        output << "FQ " << getDOM().getNumberOfTasks() << endl;
        for_each( _entities.begin(), _entities.end(), TaskOutput( output, &TaskOutput::printThroughputAndUtilization ) );
        output << "-1"  << endl << endl;

        if ( getDOM().entryHasOpenWait() ) {
            output << "R " << 0 << endl;
            for_each( _entities.begin(), _entities.end(), EntryOutput( output, &EntryOutput::printOpenQueueWait ) );
            output << "-1"  << endl << endl;
        }

        /* Processor utilization and waiting */

        for_each( _entities.begin(), _entities.end(), ProcessorOutput( output, &ProcessorOutput::printUtilizationAndWaiting ) );
        output << "-1"  << endl << endl;

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

    SRVN::RTF::RTF( const DOM::Document& document, const map<unsigned, DOM::Entity *>& entities, bool print_confidence_intervals )
        : SRVN::Output( document, entities, print_confidence_intervals )
    {
        ObjectOutput::__parseable = false;              /* Set global for formatting. */
        ObjectOutput::__rtf = true;                     /* Set global for formatting. */
    }

    SRVN::RTF::~RTF()
    {
        ObjectOutput::__rtf = false;                    /* Set global for formatting. */
    }

    ostream&
    SRVN::RTF::printPreamble( ostream& output ) const
    {
        output << "{\\rtf1\\ansi\\ansicpg1252\\cocoartf1138\\cocoasubrtf230" << endl            // Boilerplate.
               << "{\\fonttbl\\f0\\fmodern\\fcharset0 CourierNewPSMT;\\f1\\fmodern\\fcharset0 CourierNewPS-BoldMT;\\f2\\fmodern\\fcharset0 CourierNewPS-ItalicMT;}" << endl     // Fonts (f0, f1... )
               << "{\\colortbl;\\red255\\green255\\blue255;\\red255\\green0\\blue0;\\red255\\green164\\blue0;\\red0\\green255\\blue0;\\red0\\green0\\blue255;}" << endl         // Colour table. (black, white, red).
               << "\\vieww15500\\viewh10160\\viewkind0" << endl
               << "\\pard\\tx560\\tx1120\\tx1680\\tx2240\\tx2800\\tx3360\\tx3920\\tx4480\\tx5040\\tx5600\\tx6160\\tx6720\\pardirnatural" << endl
               << "\\f0\\fs24 \\cf0 ";

        DocumentOutput print( output );
        print( getDOM() );
        return output;
    }

    ostream&
    SRVN::RTF::printPostamble( ostream& output ) const
    {
        output << "}" << endl;
        return output;
    }

    /* ---------------------------------------------------------------- */
    /* Input Output                                                     */
    /* ---------------------------------------------------------------- */

    SRVN::Input::Input( const DOM::Document& document, const map<unsigned, DOM::Entity *>& entities, bool annotate )
        : _document(document), _entities(entities), _annotate(annotate)
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
                ostringstream s;
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
    ostream&
    SRVN::Input::print( ostream& output ) const
    {
        printHeader( output );

        if ( Spex::numberOfInputVariables() > 0 && !_document.instantiated() ) {
            output << endl;
	    if ( _annotate ) {
 		output << "# SPEX Variable definition and initialization." << endl
		       << "# SYNTAX-FORM-A: $var = spex_expr" << endl
		       << "#   spex_expr may contain operators, variables and constants." << endl
		       << "# SYNTAX-FORM-B: $var = [constant_1, constant_2, ...]" << endl
		       << "#   The solver will iterate over constant_1, constant_2, ..." << endl
		       << "#   Arrays nest, so multiple arrays will generate a factorial experiment." << endl
		       << "# SYNTAX-FORM-C: $var = [constant_1 : constant_2, constant_3]" << endl
		       << "#   The solver will iterate from constant_1 to constant_2 with a step size of constant_3" << endl
		       << "# SYNTAX-FORM-D: $index, $var = spex_expr" << endl
		       << "#   $index is a variable defined using SYNTAX-FORM-B or C.  $index may or may not appear in spex_expr." << endl
		       << "#   This form is used to allow one array to change multiple variables." << endl;
	    }
	    const std::map<std::string,LQX::SyntaxTreeNode *>& input_variables = Spex::get_input_variables();
	    LQX::SyntaxTreeNode::setVariablePrefix( "$" );
	    for_each( input_variables.begin(), input_variables.end(), Spex::PrintInputVariable( output ) );
        }

        printGeneral( output );

        const unsigned int n_proc = count_if( _entities.begin(), _entities.end(), is_processor );
        output << endl << "P " << n_proc << endl;
        if ( _annotate ) {
            output << "# SYNTAX: p ProcessorName SchedDiscipline [multiplicity]" << endl
                   << "#   ProcessorName is any string, globally unique among processors." << endl
                   << "#   SchedDiscipline = f {fifo}" << endl
                   << "#                   | r {random}" << endl
                   << "#                   | p {premptive}" << endl
                   << "#                   | h {hol (or non-pre-emptive priority)}" << endl
                   << "#                   | s quantum {processor-sharing (or round-robin)} " << endl
                   << "#   multiplicity = m value {multiprocessor}" << endl
                   << "#                | i {infinite or delay server}" << endl;
        }
        for_each( _entities.begin(), _entities.end(), ProcessorInput( output, &ProcessorInput::print ) );
        output << -1 << endl;


        const std::map<std::string,DOM::Group*>& groups = _document.getGroups();
        if ( groups.size() > 0 ) {
            output << endl << "U " << groups.size() << endl;
            if ( _annotate ) {
                output << "# SYNTAX: g GroupName share cap ProcessorName" << endl;
            }
            for_each( groups.begin(), groups.end(), GroupInput( output, &GroupInput::print ) );
            output << -1 << endl;
        }

        const unsigned int n_task = count_if( _entities.begin(), _entities.end(), is_task );
        output << endl << "T " << n_task << endl;
        if ( _annotate ) {
            output << "# SYNTAX: t TaskName TaskType EntryList -1 ProcessorName [priority] [multiplicity]" << endl
                   << "#   TaskName is any string, globally unique among tasks." << endl
                   << "#   TaskType = r {reference or user task}" << endl
                   << "#            | n {other} " << endl
                   << "#   multiplicity = m value {multithreaded}" << endl
                   << "#                | i {infinite}" << endl;
        }
        for_each( _entities.begin(), _entities.end(), TaskInput( output, &TaskInput::print ) );
        output << -1 << endl;

        output << endl << "E " << _document.getNumberOfEntries() << endl;
        if ( _annotate ) {
            output << "# SYNTAX-FORM-A: Token EntryName Value1 [Value2] [Value3] -1" << endl
                   << "#   EntryName is a string, globally unique over all entries " << endl
                   << "#   Values are for phase 1, 2 and 3 {phase 1 is before the reply} " << endl
                   << "#   Token indicate the significance of the Value: " << endl
                   << "#       s - HostServiceDemand for EntryName " << endl
                   << "#       c - HostServiceCoefficientofVariation" << endl
                   << "#       f - PhaseTypeFlag" << endl
                   << "# SYNTAX-FORM-B: Token FromEntry ToEntry Value1 [Value2] [Value3] -1" << endl
                   << "#   Token indicate the Value Definitions: " << endl
                   << "#       y - SynchronousCalls {no. of rendezvous} " << endl
                   << "#       F - ProbForwarding {forward to ToEntry rather than replying} " << endl
                   << "#       z - AsynchronousCalls {no. of send-no-reply messages} " << endl;
        }
        for_each( _entities.begin(), _entities.end(), TaskInput( output, &TaskInput::printEntryInput ) );
        output << -1 << endl;

        for_each( _entities.begin(), _entities.end(), TaskInput( output, &TaskInput::printActivityInput ) );

        const unsigned int n_R = Spex::numberOfResultVariables();
        if ( n_R > 0 && !_document.instantiated() ) {
            output << endl << "R " << n_R << endl;
	    if ( _annotate ) {
		output << "# SYNTAX: spex-expr" << endl
		       << "#   Any variable defined earlier can be used." << endl;
	    }
	    const std::vector<Spex::var_name_and_expr>& results = Spex::get_result_variables();
	    LQX::SyntaxTreeNode::setVariablePrefix( "$" );
	    for_each( results.begin(), results.end(), Spex::PrintResultVariable( output, 2 ) );
            output << "-1" << endl;
        }
        return output;
    }

    /*
     * Print out the "general information" for srvn input output.
     */

    ostream&
    SRVN::Input::printHeader( ostream& output ) const
    {
        output << "# SRVN Model Description File, for file: " << DOM::Document::__input_file_name << endl
               << "# Generated by: " << DOM::Document::io_vars->lq_toolname << ", version " << DOM::Document::io_vars->lq_version << endl;
	const DOM::GetLogin login;
	output << "# For: " << login << endl;
#if HAVE_CTIME
        time_t tloc;
        time( &tloc );
        output << "# " << ctime( &tloc );
#endif
        output << "# Invoked as: " << DOM::Document::io_vars->lq_command_line << ' ' << DOM::Document::__input_file_name << endl
	       << "# " << DOM::Common_IO::svn_id() << std::endl
	       << "# " << std::setfill( '-' ) << std::setw( 72 ) << '-' << std::setfill( ' ' ) << std::endl;

        const map<string,string>& pragmas = _document.getPragmaList();
        if ( pragmas.size() ) {
            output << endl;
            for ( map<string,string>::const_iterator nextPragma = pragmas.begin(); nextPragma != pragmas.end(); ++nextPragma ) {
                output << "#pragma " << nextPragma->first << "=" << nextPragma->second << endl;
            }
        }
        return output;
    }

    /*
     * Print general information. (Print default values if set by spex)
     */

    ostream&
    SRVN::Input::printGeneral( ostream& output ) const
    {
        output << endl << "G \"" << *_document.getModelComment() << "\" " ;
        if ( _annotate ) {
            output << "\t\t\t# Model comment " << endl
                   << *_document.getModelConvergence() << "\t\t\t# Convergence test value." << endl
                   << *_document.getModelIterationLimit() << "\t\t\t# Maximum number of iterations." << endl
                   << *_document.getModelPrintInterval() << "\t\t\t# Print intermediate results (see manual pages)" << endl
                   << *_document.getModelUnderrelaxationCoefficient() << "\t\t\t# Model under-relaxation ( 0.0 < x <= 1.0)" << endl
                   << -1;
        } else {
            output << *_document.getModelConvergence() << " "
                   << *_document.getModelIterationLimit() << " "
                   << *_document.getModelPrintInterval() << " "
                   << *_document.getModelUnderrelaxationCoefficient() << " "
                   << -1;
        }
        if ( !_document.instantiated() ) {
	    for ( std::vector<Spex::ObservationInfo>::const_iterator obs = Spex::get_document_variables().begin(); obs != Spex::get_document_variables().end(); ++obs ) {
		output << " ";
		obs->print( output );
	    }
        }
        output << endl;
        return output;
    }

    bool
    SRVN::Input::is_processor( const pair<unsigned,DOM::Entity *>& ep )
    {
        return dynamic_cast<const DOM::Processor *>(ep.second) != 0;
    }

    bool
    SRVN::Input::is_task( const pair<unsigned, DOM::Entity *>& ep )
    {
        return dynamic_cast<const DOM::Task *>(ep.second) != 0;
    }

    /*
     * Print out the variable, var.  If it's a variable, print out the name.  If the variable has been set, print the value, but only 
     * if the value is valid.
     */
    
    /* static */ ostream&
    SRVN::Input::doubleAndGreaterThan( ostream& output, const std::string& str, const DOM::ExternalVariable * var, double floor_value )
    {
        if ( !var ) {
	    return output;
	} else if ( !var->wasSet() ) {
	    output << str << print_double_parameter( var, floor_value );
	} else {
	    double value;
	    if ( !var->getValue(value) ) throw std::domain_error( "not a number" );
	    if ( std::isinf(value) ) throw std::domain_error( "infinity" );
	    if ( value < floor_value ) {
		std::stringstream ss;
		ss << value << " < " << floor_value;
		throw std::domain_error( ss.str() );
	    }
	    if ( value > floor_value ) {
		output << str << *var;		/* Ignore if it's the default */
	    }
        }
	return output;
    }

    /*
     * Print out the variable, var.  If it's a variable, print out the name.  If the variable has been set, print the value, but only 
     * if the value is valid.
     */
    
    /* static */ ostream&
    SRVN::Input::integerAndGreaterThan( ostream& output, const std::string& str, const DOM::ExternalVariable * var, double floor_value )
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

    /* static */ ostream&
    SRVN::Input::printDoubleExtvarParameter( ostream& output, const DOM::ExternalVariable * var, double floor_value )
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
	    std::map<const DOM::ExternalVariable *, const LQX::SyntaxTreeNode *>::const_iterator vp = Spex::__inline_expression.find( var );
	    if ( vp != Spex::__inline_expression.end() ) {
		output << "{ ";
		vp->second->print( output );
		output << " }";
	    } else {
		output << *var;
	    }
	}
	return output;
    }

    /* static */ ostream&
    SRVN::Input::printIntegerExtvarParameter( ostream& output, const DOM::ExternalVariable * var, double floor_value )
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
	    std::map<const DOM::ExternalVariable *, const LQX::SyntaxTreeNode *>::const_iterator vp = Spex::__inline_expression.find( var );
	    if ( vp != Spex::__inline_expression.end() ) {
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
        _output << "Generated by: " << DOM::Document::io_vars->lq_toolname << ", version " << DOM::Document::io_vars->lq_version << newline
                << textit
                << "Copyright the Real-Time and Distributed Systems Group," << newline
                << "Department of Systems and Computer Engineering" << newline
                << "Carleton University, Ottawa, Ontario, Canada. K1S 5B6" << newline
                << textrm << newline;

        if ( DOM::Document::io_vars->lq_command_line ) {
            _output << "Invoked as: " << DOM::Document::io_vars->lq_command_line << ' ' << DOM::Document::__input_file_name << newline;
        }
        _output << "Input:  " << DOM::Document::__input_file_name << newline;
#if     defined(HAVE_CTIME)
        time_t clock = time( (time_t *)0 );
        _output << ctime( &clock ) << newline;
#endif

        if ( document.getModelComment()->wasSet() ) {
            _output << "Comment: " << document.getModelCommentString() << newline;
        }
        if ( document.getSymbolExternalVariableCount() > 0 ) {
            _output << "Variables: ";
            document.printExternalVariables( _output ) << newline;
        }
        _output << newline
                << "Convergence test value: " << document.getResultConvergenceValue() << newline
                << "Number of iterations:   " << document.getResultIterations() << newline;
        if ( document.getSimulationSeedValue() != 0 ) {
            _output << "Seed Value:             " << document.getSimulationSeedValue() << newline;
        }
        if ( document.getExtraComment().length() > 0 ) {
            _output << "Other:                  " << document.getExtraComment() << newline;
        }
        _output << newline;

        const map<string,string>& pragmas = document.getPragmaList();
        if ( pragmas.size() ) {
            _output << "Pragma" << (pragmas.size() > 1 ? "s:" : ":") << newline;
            for ( map<string,string>::const_iterator nextPragma = pragmas.begin(); nextPragma != pragmas.end(); ++nextPragma ) {
                _output << "    " << nextPragma->first << "=" << nextPragma->second << newline;
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

	const LQIO::DOM::MVAStatistics& mva_info = document.getResultMVAStatistics();
	if ( mva_info.getNumberOfSubmodels() > 0 ) {
            _output << "    Submodels:  " << mva_info.getNumberOfSubmodels() << newline
                    << "    MVA Core(): " << setw(ObjectOutput::__maxDblLen) << mva_info.getNumberOfCore() << newline
                    << "    MVA Step(): " << setw(ObjectOutput::__maxDblLen) << mva_info.getNumberOfStep() << newline
//                                     mva_info.getNumberOfStepSquared() <<
                    << "    MVA Wait(): " << setw(ObjectOutput::__maxDblLen) << mva_info.getNumberOfWait() << newline;
//                                     mva_info.getNumberOfWaitSquared() <<
            if ( mva_info.getNumberOfFaults() ) {
                _output << "*** Faults ***" << mva_info.getNumberOfFaults() << newline;
            }
        }
    }

    /* -------------------------------------------------------------------- */
    /* Entities                                                             */
    /* -------------------------------------------------------------------- */

    void
    SRVN::EntityOutput::printCommonParameters( const DOM::Entity& entity ) const
    {
        bool print_task_name = true;
        _output << entity_name( entity, print_task_name );
        ostringstream myType;
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
        _output << setw(9) << myType.str() << " " << setw(5) << entity.getReplicasValue() << " ";
    }

    ostream&
    SRVN::EntityInput::print( ostream& output, const DOM::Entity * entity )
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
    SRVN::ProcessorOutput::operator()( const pair<unsigned, DOM::Entity *>& ep) const
    {
        const DOM::Processor * processor = dynamic_cast<const DOM::Processor *>(ep.second);
        if ( !processor ) return;

        ios_base::fmtflags oldFlags = _output.setf( ios::left, ios::adjustfield );
        (this->*_func)( *processor );
        _output.flags(oldFlags);
    }


    void
    SRVN::ProcessorOutput::printParameters( const DOM::Processor& processor ) const
    {
        const ios_base::fmtflags oldFlags = _output.setf( ios::left, ios::adjustfield );
        SRVN::EntityOutput::printCommonParameters( processor );
	if ( processor.isInfinite() ) {
	    _output << scheduling_label[SCHEDULE_DELAY].str;
	} else {
	    _output << scheduling_label[processor.getSchedulingType()].str;
	    if ( processor.hasRate() ) {
		_output << " " << Input::print_double_parameter( processor.getRate(), 0. );
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
        const double rate = processor.getRateValue();
        const double proc_util = !processor.isInfinite() ? processor.getResultUtilization() / (static_cast<double>(processor.getCopiesValue()) * rate): 0;

        if ( __parseable ) {
            _output << "P " << processor.getName() << " " << tasks.size() << newline;
        } else {
            _output << newline << newline << textbf << utilization_str << ' ' << textrm << colour( proc_util )
                    << processor.getName() << colour( 0 )
                    << newline << newline
                    << setw(__maxStrLen) << "Task Name"
                    << "Pri n   "
                    << setw(__maxStrLen) << "Entry Name"
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
		_output << textit << setw(__maxStrLen*2+8) << total
			<< setw(__maxDblLen-1) << group.getResultUtilization() << ' ' << newline;
		if ( __conf95 ) {
		    _output << conf_level(__maxStrLen*2+8,ConfidenceIntervals::CONF_95)
			    << setw(__maxDblLen-1) << (*__conf95)(group.getResultUtilizationVariance()) << ' ' << newline;
		}
		if ( __conf99 ) {
		    _output << conf_level(__maxStrLen*2+8,ConfidenceIntervals::CONF_99)
			    << setw(__maxDblLen-1) << (*__conf99)(group.getResultUtilizationVariance()) << ' ' << newline;
		}
		_output << textrm;
	    }
        } else {
            printUtilizationAndWaiting( processor, tasks );
        }

        /* Total for processor */

        if ( tasks.size() > 1 ) {
            if ( __parseable ) { _output << setw( __maxStrLen ) << " " << activityEOF << newline; }
            _output << textit << colour( proc_util )<< setw(__maxStrLen*2+8) << ( __parseable ? " " : "Processor Total:" )
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
	double copies = 1.0;
	if ( !is_infinite ) {
	    copies = static_cast<double>(processor.getCopiesValue());
	}
        for ( std::set<DOM::Task*>::const_iterator nextTask = tasks.begin(); nextTask != tasks.end(); ++nextTask ) {
            const DOM::Task * task = *nextTask;
            bool print_task_name = true;
            unsigned int item_count = 0;

            const std::vector<DOM::Entry *> & entries = task->getEntryList();
            for ( std::vector<DOM::Entry *>::const_iterator nextEntry = entries.begin(); nextEntry != entries.end(); ++nextEntry, ++item_count ) {
                const DOM::Entry * entry = *nextEntry;
                const double entry_util = is_infinite ? 0 : entry->getResultProcessorUtilization() / (copies * processor.getRateValue());
		const unsigned int multiplicity = task->isInfinite() ? 1 : task->getCopiesValue();
                _output << colour( entry_util );
                if ( __parseable ) {
                    if ( print_task_name ) {
                        _output << setw( __maxStrLen-1 ) << task->getName() << " "
                                << setw(2) << entries.size() << " "
                                << setw(1) << task->getPriorityValue() << " "
                                << setw(2) << multiplicity << " ";
                        print_task_name = false;
                    } else {
                        _output << setw(__maxStrLen+8) << " ";
                    }
                } else if ( print_task_name ) {
                    _output << entity_name( *task, print_task_name )
                            << setw(3) << task->getPriorityValue() << " "
                            << setw(3) << multiplicity << " ";
                } else {
                    _output << setw(__maxStrLen+8) << " ";
                }
                _output << entry_name( *entry )
                        << setw(__maxDblLen-1) << entry->getResultProcessorUtilization() << " "
                        << processor_queueing_time( *entry ) << newline;
                if ( __conf95 ) {
                    _output << conf_level(__maxStrLen*2+8,ConfidenceIntervals::CONF_95)
                            << setw(__maxDblLen-1) << (*__conf95)(entry->getResultProcessorUtilizationVariance()) << " "
                            << setw(__maxDblLen-1) << processor_queueing_time_confidence( *entry, __conf95 ) << newline;
                }
                if ( __conf99 ) {
                    _output << conf_level(__maxStrLen*2+8,ConfidenceIntervals::CONF_99)
                            << setw(__maxDblLen-1) << (*__conf99)(entry->getResultProcessorUtilizationVariance()) << " "
                            << setw(__maxDblLen-1) << processor_queueing_time_confidence( *entry, __conf99 ) << newline;
                }
                _output << colour( 0 );
            }

            const std::map<std::string,DOM::Activity*>& activities = task->getActivities();
            if ( activities.size() > 0 ) {
                _output << activity_separator(__maxStrLen+8) << newline;
                std::map<std::string,DOM::Activity*>::const_iterator nextActivity;
                for ( nextActivity = activities.begin(); nextActivity != activities.end(); ++nextActivity, ++item_count ) {
                    const DOM::Activity * activity = nextActivity->second;
                    _output << setw(__maxStrLen+8) << " "
                            << setw(__maxStrLen-1) << activity->getName() << ' '
                            << setw(__maxDblLen-1) << activity->getResultProcessorUtilization() << ' '
                            << setw(__maxDblLen-1) << activity->getResultProcessorWaiting() << ' '
                            << activityEOF << newline;
                    if ( __conf95 ) {
                        _output << conf_level(__maxStrLen*2+8,ConfidenceIntervals::CONF_95)
                                << setw(__maxDblLen-1) << (*__conf95)(activity->getResultProcessorUtilizationVariance()) << ' '
                                << setw(__maxDblLen-1) << (*__conf95)(activity->getResultProcessorWaitingVariance()) << ' '
                                << activityEOF << newline;
                    }
                    if ( __conf99 ) {
                        _output << conf_level(__maxStrLen*2+8,ConfidenceIntervals::CONF_99)
                                << setw(__maxDblLen-1) << (*__conf99)(activity->getResultProcessorUtilizationVariance()) << ' '
                                << setw(__maxDblLen-1) << (*__conf99)(activity->getResultProcessorWaitingVariance()) << ' '
                                << activityEOF << newline;
                    }
                }
            }
            if ( __parseable ) {
                _output << setw(__maxStrLen+8) << " " << activityEOF << newline;
            }

            /* Total for task */

            if ( item_count > 1 ) {
                const double task_util = !processor.isInfinite() ? task->getResultProcessorUtilization() / (copies * processor.getRateValue()): 0;
                _output << textit << colour( task_util );
                _output << setw(__maxStrLen*2+8) << ( __parseable ? " " : "Task Total:" )
                        << setw(__maxDblLen-1) << task->getResultProcessorUtilization() << ' ' << newline;
                if ( __conf95 ) {
                    _output << conf_level(__maxStrLen*2+8,ConfidenceIntervals::CONF_95)
                            << setw(__maxDblLen-1) << (*__conf95)(task->getResultProcessorUtilizationVariance()) << ' ' << newline;
                }
                if ( __conf99 ) {
                    _output << conf_level(__maxStrLen*2+8,ConfidenceIntervals::CONF_99)
                            << setw(__maxDblLen-1) << (*__conf99)(task->getResultProcessorUtilizationVariance()) << ' ' << newline;
                }
                _output << colour( 0 ) << textrm;
            }
        }
    }

    void
    SRVN::ProcessorInput::operator()( const pair<unsigned, DOM::Entity *>& ep) const
    {
        const DOM::Processor * processor = dynamic_cast<const DOM::Processor *>(ep.second);
        if ( !processor ) return;

        ios_base::fmtflags oldFlags = _output.setf( ios::left, ios::adjustfield );
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
        _output << endl;
    }

    ostream&
    SRVN::ProcessorInput::printCopies( ostream& output, const DOM::Processor & processor )
    {
	if ( !processor.isInfinite() ) {
	    try {
		output << Input::is_integer_and_gt( " m ", processor.getCopies(), 1.0 );
	    }
	    catch ( const std::domain_error& e ) {
		solution_error( LQIO::ERR_INVALID_PARAMETER, "multiplicity", "processor", processor.getName().c_str(), e.what() );
		throw_bad_parameter();
	    }
	}
        return output;
    }

    ostream&
    SRVN::ProcessorInput::printReplicas( ostream& output, const DOM::Processor & processor )
    {
	try {
	    output << Input::is_integer_and_gt( " r ", processor.getReplicas(), 1.0 );
	}
	catch ( const std::domain_error& e ) {
	    solution_error( LQIO::ERR_INVALID_PARAMETER, "replicas", "processor", processor.getName().c_str(), e.what() );
	}
        return output;
    }

    ostream&
    SRVN::ProcessorInput::printRate( ostream& output, const DOM::Processor& processor )
    {
	try {
	    output << Input::is_double_and_gt( " R ", dynamic_cast<const DOM::Processor&>(processor).getRate(), 1.0 );
	}
	catch ( const std::domain_error& e ) {
	    solution_error( LQIO::ERR_INVALID_PARAMETER, "rate", "processor", processor.getName().c_str(), e.what() );
	    throw_bad_parameter();
	}
        return output;
    }


    ostream&
    SRVN::ProcessorInput::printScheduling( ostream& output, const DOM::Processor & processor )
    {
	output << " ";
	if ( processor.isInfinite() ) {
	    output << scheduling_label[SCHEDULE_DELAY].flag;
	} else {
	    output << scheduling_label[static_cast<unsigned int>(processor.getSchedulingType())].flag;
	    if ( processor.hasQuantumScheduling() ) {
		output << ' ' << Input::print_double_parameter( processor.getQuantum(), 0. );
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
        ios_base::fmtflags oldFlags = _output.setf( ios::left, ios::adjustfield );
        (this->*_func)( *(group.second) );
        _output.flags(oldFlags);
    }

    void
    SRVN::GroupOutput::printParameters( const DOM::Group& group ) const
    {
        const ios_base::fmtflags oldFlags = _output.setf( ios::left, ios::adjustfield );
        _output << setw(__maxStrLen-1) << group.getName()
                << " " << setw(6) << Input::print_double_parameter( group.getGroupShare(), 0. );
        if ( group.getCap() ) {
            _output << " cap  ";
        } else {
            _output << "      ";
        }
        _output << setw(__maxStrLen) << group.getProcessor()->getName()
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
        ios_base::fmtflags oldFlags = _output.setf( ios::left, ios::adjustfield );
        (this->*_func)( *(group.second) );
        _output.flags(oldFlags);
    }

    void
    SRVN::GroupInput::print( const DOM::Group& group ) const
    {
        _output << "  g " << group.getName()
                << " " << Input::print_double_parameter( group.getGroupShare(), 0. );
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
    SRVN::TaskOutput::operator()( const pair<unsigned, DOM::Entity *>& ep) const
    {
        const DOM::Task * task = dynamic_cast<const DOM::Task *>(ep.second);
        if ( !task ) return;

        ios_base::fmtflags oldFlags = _output.setf( ios::left, ios::adjustfield );
        (this->*_func)( *task );
        _output.flags(oldFlags);
    }


    void
    SRVN::TaskOutput::printParameters( const DOM::Task& task ) const
    {
        const ios_base::fmtflags oldFlags = _output.setf( ios::left, ios::adjustfield );
        SRVN::EntityOutput::printCommonParameters( task );
        const DOM::Processor * processor = task.getProcessor();
        _output << setw(__maxStrLen-1) << ( processor ? processor->getName() : "--");
        if ( task.getDocument()->getNumberOfGroups() > 0 ) {
            const DOM::Group * group = task.getGroup();
            _output << ' ' << setw(__maxStrLen-1) << ( group ? group->getName() : "--");
        }
        _output << ' ' << setw(3) << Input::print_double_parameter( task.getPriority(), 0. );
        if ( task.getDocument()->taskHasThinkTime() ) {
            _output << setw(__maxDblLen-1);
            if ( task.getSchedulingType() == SCHEDULE_CUSTOMER ) {
                _output << Input::print_double_parameter( task.getThinkTime(), 0. );
            } else {
                _output << " ";
            }
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
                    << setw(__maxDblLen-1) << entry->getResultThroughput() << ' '
                    << entry_utilization( *entry )
                    << entry->getResultUtilization() << newline;
            if ( __conf95 ) {
                _output << conf_level( __maxStrLen*2, ConfidenceIntervals::CONF_95 )
                        << setw(__maxDblLen-1) << (*__conf95)(entry->getResultThroughputVariance()) << ' '
                        << entry_utilization_confidence( *entry, __conf95 )
                        << (*__conf95)(entry->getResultUtilizationVariance()) << newline;
            }
            if ( __conf99 ) {
                _output << conf_level( __maxStrLen*2, ConfidenceIntervals::CONF_99 )
                        << setw(__maxDblLen-1) << (*__conf99)(entry->getResultThroughputVariance()) << ' '
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
                _output << setw(__maxStrLen ) << " "
                        << setw(__maxStrLen-1) << activity->getName() << ' '
                        << setw(__maxDblLen-1) << activity->getResultThroughput() << ' '
                        << setw(__maxDblLen) << activity->getResultUtilization() << activityEOF << newline;
                if ( __conf95 ) {
                    _output << conf_level( __maxStrLen*2, ConfidenceIntervals::CONF_95 )
                            << setw(__maxDblLen-1) << (*__conf95)(activity->getResultThroughputVariance()) << ' '
                            << setw(__maxDblLen-1) << (*__conf95)(activity->getResultUtilizationVariance()) << ' ' << activityEOF << newline;
                }
                if ( __conf99 ) {
                    _output << conf_level( __maxStrLen*2, ConfidenceIntervals::CONF_99 )
                            << setw(__maxDblLen-1) << (*__conf99)(activity->getResultThroughputVariance()) << ' '
                            << setw(__maxDblLen-1) << (*__conf99)(activity->getResultUtilizationVariance()) << ' ' << activityEOF << newline;
                }
            }
        }

        if ( item_count > 0 && __parseable ) {
            _output << setw(__maxStrLen) << " " << activityEOF << newline;
        }

        /* Totals */
        if ( item_count > 1 && task.getResultPhaseCount() > 0 ) {
            const double task_util = task.getResultUtilization() / copies;
            _output << textit << colour( task_util ) <<  setw( __maxStrLen*2 ) << (__parseable ? " " : "Total:" )
                    << setw(__maxDblLen-1) << task.getResultThroughput() << ' '
                    << task_utilization( task )
                    << task.getResultUtilization() << newline;
            if ( __conf95 ) {
                _output << conf_level( __maxStrLen*2, ConfidenceIntervals::CONF_95 )
                        << setw(__maxDblLen-1) << (*__conf95)(task.getResultThroughputVariance()) << ' '
                        << task_utilization_confidence( task, __conf95 )
                        << (*__conf95)(task.getResultUtilizationVariance()) << newline;
            }
            if ( __conf99 ) {
                _output << conf_level( __maxStrLen*2, ConfidenceIntervals::CONF_99 )
                        << setw(__maxDblLen-1) << (*__conf99)(task.getResultThroughputVariance()) << ' '
                        << task_utilization_confidence( task, __conf99 )
                        << (*__conf99)(task.getResultUtilizationVariance()) << newline;
            }
            _output << colour( 0 ) << textrm;
        }
    }


    void
    SRVN::TaskOutput::printJoinDelay( const DOM::Task& task ) const
    {
        const std::map<std::string,DOM::Activity*>& activities = task.getActivities();
        if ( activities.size() > 0 ) {
            bool print_task_name = true;
            std::map<std::string,DOM::Activity*>::const_iterator nextActivity;
            for ( nextActivity = activities.begin(); nextActivity != activities.end(); ++nextActivity ) {
                const DOM::Activity * activity = nextActivity->second;
                const DOM::AndJoinActivityList * activityList = dynamic_cast<DOM::AndJoinActivityList *>(activity->getOutputToList());
                if ( !activityList ) continue;
                const DOM::Activity * first;
                const DOM::Activity * last;
                activityList->activitiesForName( &first, &last );
                if ( first != activity ) continue;
                _output << entity_name( task, print_task_name )
                        << setw(__maxStrLen-1) << first->getName() << " "
                        << setw(__maxStrLen-1) << last->getName() << " "
                        << setw(__maxDblLen-1) << activityList->getResultJoinDelay() << ' '
                        << setw(__maxDblLen-1) << activityList->getResultVarianceJoinDelay()
                        << newline;
                if ( __conf95 ) {
                    _output << conf_level( __maxStrLen*3, ConfidenceIntervals::CONF_95 )
                            << setw(__maxDblLen-1) << (*__conf95)(activityList->getResultJoinDelayVariance()) << ' '
                            << setw(__maxDblLen-1) << (*__conf95)(activityList->getResultVarianceJoinDelayVariance())
                            << newline;
                }
                if ( __conf99 ) {
                    _output << conf_level( __maxStrLen*3, ConfidenceIntervals::CONF_99 )
                            << setw(__maxDblLen-1) << (*__conf99)(activityList->getResultJoinDelayVariance()) << ' '
                            << setw(__maxDblLen-1) << (*__conf99)(activityList->getResultVarianceJoinDelayVariance())
                            << newline;
                }
            }
            if ( !print_task_name && __parseable ) {
                _output << activityEOF << newline;
            }
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
        if ( entries[0]->getSemaphoreFlag() == SEMAPHORE_SIGNAL ) {
            wait_entry = entries[1];
            signal_entry = entries[0];
        } else {
            wait_entry = entries[0];
            signal_entry = entries[1];
        }
        bool print_task_name = true;
        _output << entity_name(task,print_task_name)
                << setw(__maxStrLen-1) << wait_entry->getName() << " "
                << setw(__maxStrLen-1) << signal_entry->getName() << " "
                << setw(__maxDblLen-1) << semaphore->getResultHoldingTime()  << " "
                << setw(__maxDblLen-1) << semaphore->getResultVarianceHoldingTime()  << " "
                << setw(__maxDblLen-1) << semaphore->getResultHoldingUtilization() << newline;
        if ( __conf95 ) {
            _output << conf_level( __maxStrLen*3, ConfidenceIntervals::CONF_95 )
                    << setw(__maxDblLen-1) << (*__conf95)( semaphore->getResultHoldingTimeVariance() ) << " "
                    << setw(__maxDblLen-1) << (*__conf95)( semaphore->getResultVarianceHoldingTimeVariance() ) << " "
                    << setw(__maxDblLen-1) << (*__conf95)( semaphore->getResultHoldingUtilizationVariance() ) << newline;

        }
        if ( __conf99 ) {
            _output << conf_level( __maxStrLen*3, ConfidenceIntervals::CONF_99 )
                    << setw(__maxDblLen-1) << (*__conf99)( semaphore->getResultHoldingTimeVariance() ) << " "
                    << setw(__maxDblLen-1) << (*__conf99)( semaphore->getResultVarianceHoldingTimeVariance() ) << " "
                    << setw(__maxDblLen-1) << (*__conf99)( semaphore->getResultHoldingUtilizationVariance() ) << newline;

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
        DOM::Entry * r_lock_entry = NULL;
        DOM::Entry * r_unlock_entry = NULL;
        DOM::Entry * w_lock_entry = NULL;
        DOM::Entry * w_unlock_entry = NULL;

        for (int i=0;i<4;i++){
            switch (entries[i]->getRWLockFlag()) {
            case RWLOCK_R_UNLOCK: r_unlock_entry=entries[i]; break;
            case RWLOCK_R_LOCK:   r_lock_entry=entries[i]; break;
            case RWLOCK_W_UNLOCK: w_unlock_entry=entries[i]; break;
            case RWLOCK_W_LOCK:   w_lock_entry=entries[i]; break;
            default: abort();
            }
        }

        bool print_task_name = true;
        _output << entity_name(task,print_task_name)
                << setw(__maxStrLen-1) << r_lock_entry->getName() << " "
                << setw(__maxStrLen-1) << r_unlock_entry->getName() << " "
                << setw(__maxDblLen-1) << rwlock->getResultReaderBlockedTime()  << " "
                << setw(__maxDblLen-1) << rwlock->getResultVarianceReaderBlockedTime()  << " "
                << setw(__maxDblLen-1) << rwlock->getResultReaderHoldingTime()  << " "
                << setw(__maxDblLen-1) << rwlock->getResultVarianceReaderHoldingTime()  << " "
                << setw(__maxDblLen-1) << rwlock->getResultReaderHoldingUtilization() << newline;

        if ( __conf95 ) {
            _output << conf_level( __maxStrLen*3, ConfidenceIntervals::CONF_95 )
                    << setw(__maxDblLen-1) << (*__conf95)( rwlock->getResultReaderBlockedTimeVariance() ) << " "
                    << setw(__maxDblLen-1) << (*__conf95)( rwlock->getResultVarianceReaderBlockedTimeVariance() ) << " "
                    << setw(__maxDblLen-1) << (*__conf95)( rwlock->getResultReaderHoldingTimeVariance() ) << " "
                    << setw(__maxDblLen-1) << (*__conf95)( rwlock->getResultVarianceReaderHoldingTimeVariance() ) << " "
                    << setw(__maxDblLen-1) << (*__conf95)( rwlock->getResultReaderHoldingUtilizationVariance() ) << newline;
        }
        if ( __conf99 ) {
            _output << conf_level( __maxStrLen*3, ConfidenceIntervals::CONF_99 )
                    << setw(__maxDblLen-1) << (*__conf99)( rwlock->getResultReaderBlockedTimeVariance() ) << " "
                    << setw(__maxDblLen-1) << (*__conf99)( rwlock->getResultVarianceReaderBlockedTimeVariance() ) << " "
                    << setw(__maxDblLen-1) << (*__conf99)( rwlock->getResultReaderHoldingTimeVariance() ) << " "
                    << setw(__maxDblLen-1) << (*__conf99)( rwlock->getResultVarianceReaderHoldingTimeVariance() ) << " "
                    << setw(__maxDblLen-1) << (*__conf99)( rwlock->getResultReaderHoldingUtilizationVariance() ) << newline;
        }
        _output << entity_name(task,print_task_name)
                << setw(__maxStrLen-1) << w_lock_entry->getName() << " "
                << setw(__maxStrLen-1) << w_unlock_entry->getName() << " "
                << setw(__maxDblLen-1) << rwlock->getResultWriterBlockedTime()  << " "
                << setw(__maxDblLen-1) << rwlock->getResultVarianceWriterBlockedTime()  << " "
                << setw(__maxDblLen-1) << rwlock->getResultWriterHoldingTime()  << " "
                << setw(__maxDblLen-1) << rwlock->getResultVarianceWriterHoldingTime()  << " "
                << setw(__maxDblLen-1) << rwlock->getResultWriterHoldingUtilization() << newline;

        if ( __conf95 ) {
            _output << conf_level( __maxStrLen*3, ConfidenceIntervals::CONF_95 )
                    << setw(__maxDblLen-1) << (*__conf95)( rwlock->getResultWriterBlockedTimeVariance() ) << " "
                    << setw(__maxDblLen-1) << (*__conf95)( rwlock->getResultVarianceWriterBlockedTimeVariance() ) << " "
                    << setw(__maxDblLen-1) << (*__conf95)( rwlock->getResultWriterHoldingTimeVariance() ) << " "
                    << setw(__maxDblLen-1) << (*__conf95)( rwlock->getResultVarianceWriterHoldingTimeVariance() ) << " "
                    << setw(__maxDblLen-1) << (*__conf95)( rwlock->getResultWriterHoldingUtilizationVariance() ) << newline;
        }
        if ( __conf99 ) {
            _output << conf_level( __maxStrLen*3, ConfidenceIntervals::CONF_99 )
                    << setw(__maxDblLen-1) << (*__conf99)( rwlock->getResultWriterBlockedTimeVariance() ) << " "
                    << setw(__maxDblLen-1) << (*__conf99)( rwlock->getResultVarianceWriterBlockedTimeVariance() ) << " "
                    << setw(__maxDblLen-1) << (*__conf99)( rwlock->getResultWriterHoldingTimeVariance() ) << " "
                    << setw(__maxDblLen-1) << (*__conf99)( rwlock->getResultVarianceWriterHoldingTimeVariance() ) << " "
                    << setw(__maxDblLen-1) << (*__conf99)( rwlock->getResultWriterHoldingUtilizationVariance() ) << newline;
        }
    }

    /*
     * SRVN style for a task
     */

    void
    SRVN::TaskInput::operator()( const pair<unsigned, DOM::Entity *>& ep) const
    {
        const DOM::Task * task = dynamic_cast<const DOM::Task *>(ep.second);
        if ( !task ) return;

        ios_base::fmtflags oldFlags = _output.setf( ios::left, ios::adjustfield );
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
        _output << endl;

        for ( std::map<const std::string, LQIO::DOM::ExternalVariable *>::const_iterator fi = task.getFanIns().begin(); fi != task.getFanIns().end(); ++fi ) {
            if ( !DOM::Common_IO::is_default_value( fi->second, 1 ) ) {     /* task name, fan in */
                _output << "  I " << fi->first << " " << task.getName() << " " << Input::print_integer_parameter(fi->second,0) << endl;
            }
        }
        for ( std::map<const std::string, LQIO::DOM::ExternalVariable *>::const_iterator fo = task.getFanOuts().begin(); fo != task.getFanOuts().end(); ++fo ) {
            if ( !DOM::Common_IO::is_default_value( fo->second, 1 ) ) {
                _output << "  O " << task.getName() << " " << fo->first << " " << Input::print_integer_parameter(fo->second,0) << endl;
            }
        }

    }

    ostream&
    SRVN::TaskInput::printScheduling( ostream& output, const DOM::Task & task )
    {
	output << " ";
        if ( task.isInfinite() ) {
            output << scheduling_label[SCHEDULE_DELAY].flag;
        } else {
	    output << scheduling_label[task.getSchedulingType()].flag;
	}
        return output;
    }

    ostream&
    SRVN::TaskInput::printEntryList( ostream& output,  const DOM::Task& task )
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

    ostream&
    SRVN::TaskInput::printCopies( ostream& output, const DOM::Task & task )
    {
        if ( !task.isInfinite() ) {
	    try {
		output << Input::is_integer_and_gt( " m ", task.getCopies(), 1.0 );
	    }
	    catch ( const std::domain_error& e ) {
		solution_error( LQIO::ERR_INVALID_PARAMETER, "multiplicity", "task", task.getName().c_str(), e.what() );
		throw_bad_parameter();
	    }
        }
        return output;
    }

    ostream&
    SRVN::TaskInput::printGroup( ostream& output,  const DOM::Task& task )
    {
        if ( task.getGroup() ) {
            output << " g "  << task.getGroup()->getName();
        }
        return output;
    }

    ostream&
    SRVN::TaskInput::printPriority( ostream& output,  const DOM::Task& task )
    {
        if ( task.hasPriority() || task.getProcessor()->hasPriorityScheduling() ) {
	    try {
		output << " " << Input::print_integer_parameter( task.getPriority(), 0 );
	    }
	    catch ( const std::domain_error& e ) {
		solution_error( LQIO::ERR_INVALID_PARAMETER, "priority", "task", task.getName().c_str(), e.what() );
		throw_bad_parameter();
	    }
        }
        return output;
    }

    ostream&
    SRVN::TaskInput::printQueueLength( ostream& output,  const DOM::Task& task )
    {
        if ( task.hasQueueLength() ) {
	    try {
		output << " q " << Input::print_integer_parameter( task.getQueueLength(), 0 );
	    }
	    catch ( const std::domain_error& e ) {
		solution_error( LQIO::ERR_INVALID_PARAMETER, "queue length", "task", task.getName().c_str(), e.what() );
		throw_bad_parameter();
	    }
        }
        return output;
    }

    ostream&
    SRVN::TaskInput::printReplicas( ostream& output, const DOM::Task & task )
    {
	try {
	    output << Input::is_integer_and_gt( " r ", task.getReplicas(), 1.0 );
	}
	catch ( const std::domain_error& e ) {
	    solution_error( LQIO::ERR_INVALID_PARAMETER, "replicas", "task", task.getName().c_str(), e.what() );
	    throw_bad_parameter();
	}
        return output;
    }

    ostream&
    SRVN::TaskInput::printThinkTime( ostream& output,  const DOM::Task & task )
    {
        if ( task.getSchedulingType() == SCHEDULE_CUSTOMER && task.hasThinkTime() ) {
            output << " z " << Input::print_integer_parameter( task.getThinkTime(), 0 );
        }
        return output;
    }

    void
    SRVN::TaskInput::printEntryInput( const DOM::Task& task ) const
    {
        _output << "# ---------- " << task.getName() << " ----------" << endl;
        const std::vector<DOM::Entry *> & entries = task.getEntryList();
        for_each( entries.begin(), entries.end(), EntryInput( _output, &EntryInput::print ) );
    }

    void
    SRVN::TaskInput::printActivityInput( const DOM::Task& task ) const
    {
        const std::map<std::string,DOM::Activity*>& activities = task.getActivities();
        if ( activities.size() == 0 ) return;

        _output << endl << "A " << task.getName() << endl;
        for_each( activities.begin(), activities.end(), ActivityInput( _output, &ActivityInput::print ) );

        const std::set<DOM::ActivityList*>& precedences = task.getActivityLists();
        if ( precedences.size() ) {
            _output << " :" << endl;
            for_each( precedences.begin(), precedences.end(), ActivityListInput( _output, &ActivityListInput::print, precedences.size() ) );
        } else if ( activities.size() ) {
            const std::map<std::string,DOM::Activity*>::const_iterator i = activities.begin();
            const DOM::Activity * activity = i->second;
            if ( activity->getReplyList().size() > 0 ) {
                _output << " :" << endl << "  " << activity->getName();
                printReplyList( activity->getReplyList() );
                _output << endl;
            }
        }

        _output << "-1" << endl;
    }

    /* -------------------------------------------------------------------- */
    /* Entries                                                              */
    /* -------------------------------------------------------------------- */

    void
    SRVN::EntryOutput::operator()( const pair<unsigned, DOM::Entity *>& ep) const
    {
        const DOM::Task * task = dynamic_cast<const DOM::Task *>(ep.second);
        if ( !task ) return;

        ios_base::fmtflags oldFlags = _output.setf( ios::left, ios::adjustfield );
        const std::vector<DOM::Entry *>& entries = task->getEntryList();
        std::vector<DOM::Entry *>::const_iterator nextEntry;
        bool print_task_name = true;
        for ( nextEntry = entries.begin(); nextEntry != entries.end(); ++nextEntry ) {
            (this->*_entryFunc)( **nextEntry, *(ep.second), print_task_name );
        }

        const std::map<std::string,DOM::Activity*>& activities = task->getActivities();
        if ( activities.size() > 0 && _activityFunc != NULL ) {
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
                _output << entity_name( *(ep.second), print_task_name ) << activity_separator(0) << newline;
                for ( nextActivity = activities.begin(); nextActivity != activities.end(); ++nextActivity ) {
                    (this->*_activityFunc)( *nextActivity->second );
                }
            }
        }
        if ( _activityFunc != NULL && print_task_name == false && __parseable ) {
            _output << setw(__maxStrLen) << " " << activityEOF << newline;
        }
        _output.flags(oldFlags);
    }

    /* ---------- Entry parameters ---------- */

    void
    SRVN::EntryOutput::printEntryCoefficientOfVariation( const DOM::Entry & entry, const DOM::Entity & entity, bool& print_task_name  ) const
    {
        if ( entry.getStartActivity() == NULL ) {
	    _output << entity_name( entity, print_task_name ) << entry_name( entry ) << coefficient_of_variation( entry ) << newline;
        }
    }

    void
    SRVN::EntryOutput::printEntryDemand( const DOM::Entry & entry, const DOM::Entity & entity, bool& print_task_name ) const
    {
        if ( entry.getStartActivity() == NULL ) {
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
        if ( entry.getStartActivity() == NULL ) {
            _output << entity_name( entity, print_task_name ) << entry_name( entry ) << phase_type( entry ) << newline;
        }
    }

    void
    SRVN::EntryOutput::printEntryThinkTime( const DOM::Entry & entry, const DOM::Entity & entity, bool& print_task_name  ) const
    {
        if ( entry.getStartActivity() == NULL ) {
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
		    << setw(__maxDblLen) << Input::print_double_parameter( call->getCallMean(), 0. )
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
	    solution_error( LQIO::ERR_INVALID_PARAMETER, "open arrivals", "entry", entry.getName().c_str(), e.what() );
	    throw_bad_parameter();
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
        _output << setw(__maxStrLen) << " " << setw(__maxStrLen-1) << activity.getName() << " ";
        switch (activity.getPhaseTypeFlag()) {
        case PHASE_DETERMINISTIC: _output << setw(__maxDblLen) << "determin";
        case PHASE_STOCHASTIC:    _output << setw(__maxDblLen) << "stochastic";
        }
        _output << newline;
    }

    void
    SRVN::EntryOutput::printActivity( const DOM::Activity& activity, const activityFunc func ) const
    {
	_output << setw(__maxStrLen) << " " << setw(__maxStrLen) << activity.getName() << setw(__maxDblLen) << Input::print_double_parameter( (activity.*func)(), 0. ) << newline;
    }

    /* ---- Entry Results ---- */

    void
    SRVN::EntryOutput::printEntryThroughputBounds( const DOM::Entry &entry, const DOM::Entity &entity, bool& print ) const
    {
        _output << entity_name(entity,print) << entry_name( entry ) << setw(__maxDblLen) << entry.getResultThroughputBound() << newline;
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
            _output << setw(__maxDblLen) << value;
        }
        _output << newline;

        value = entry.getResultSquaredCoeffVariationVariance();
        if ( __conf95 ) {
            _output << conf_level( __maxStrLen*2, ConfidenceIntervals::CONF_95 ) << variance_service_time_confidence(entry,__conf95);
            if ( !__parseable && value > 0.0 ) {
                _output << setw(__maxDblLen) << (*__conf95)(value);
            }
            _output << newline;
        }
        if ( __conf99 ) {
            _output << conf_level( __maxStrLen*2, ConfidenceIntervals::CONF_99 ) << variance_service_time_confidence(entry,__conf99);
            if ( !__parseable && value > 0.0 ) {
                _output << setw(__maxDblLen) << (*__conf99)(value);
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
                << setw(__maxDblLen-1) << entry.getResultThroughput() << " "
                << setw(__maxDblLen-1) << entry.getResultWaitingTime() << newline;
        if ( __conf95 ) {
            _output << conf_level( __maxStrLen * 2, ConfidenceIntervals::CONF_95 )
                    << setw(__maxDblLen) << " "         /* Input parameter, so ignore it */
                    << setw(__maxDblLen-1) << (*__conf95)(entry.getResultWaitingTimeVariance()) << newline;
        }
        if ( __conf99 ) {
            _output << conf_level( __maxStrLen * 2, ConfidenceIntervals::CONF_99 )
                    << setw(__maxDblLen) << " "
                    << setw(__maxDblLen-1) << (*__conf99)(entry.getResultWaitingTimeVariance()) << newline;
        }
    }

    void
    SRVN::EntryOutput::printForwardingWaiting( const DOM::Entry &entry, const DOM::Entity &entity, bool& print ) const
    {
	const std::vector<DOM::Call *>& forwarding = entry.getForwarding();
	for ( std::vector<DOM::Call *>::const_iterator call = forwarding.begin(); call != forwarding.end(); ++call ) {
	    _output << entity_name( entity, print )
		    << entry_name( entry )
		    << entry_name( *(*call)->getDestinationEntry() )
		    << setw(__maxDblLen) << (*call)->getResultWaitingTime()
		    << newline;
	    if ( __conf95 ) {
		_output << conf_level( __maxStrLen * 3, ConfidenceIntervals::CONF_95 )
			<< setw(__maxDblLen) << (*__conf95)((*call)->getResultWaitingTimeVariance()) << newline;
	    }
	    if ( __conf99 ) {
		_output << conf_level( __maxStrLen * 3, ConfidenceIntervals::CONF_99 )
			<< setw(__maxDblLen) << (*__conf99)((*call)->getResultWaitingTimeVariance()) << newline;
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
        _output << setw(__maxStrLen) << " " << setw(__maxStrLen-1) << activity.getName() << " " << setw(__maxDblLen) << (activity.*mean)() << activityEOF << newline;
        if ( __conf95 && variance ) {
            _output << conf_level( __maxStrLen*2, ConfidenceIntervals::CONF_95 )
                    << setw(__maxDblLen) << (*__conf95)((activity.*variance)()) << activityEOF << newline;
        }
        if ( __conf99 && variance ) {
            _output << conf_level( __maxStrLen*2, ConfidenceIntervals::CONF_99 )
                    << setw(__maxDblLen) << (*__conf99)((activity.*variance)()) << activityEOF << newline;
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
		_output << "  a " << entry.getName() << " " << Input::print_double_parameter( entry.getOpenArrivalRate(), 0. ) << endl;
	    }
	    catch ( const std::domain_error& e ) {
		solution_error( LQIO::ERR_INVALID_PARAMETER, "open arrivals", "entry", entry.getName().c_str(), e.what() );
	    }
        }

        switch ( entry.getSemaphoreFlag() ) {
        case SEMAPHORE_SIGNAL: _output << "  P " << entry.getName() << endl; break;
        case SEMAPHORE_WAIT:   _output << "  V " << entry.getName() << endl; break;
        default: break;
        }
        switch ( entry.getRWLockFlag() ) {
        case RWLOCK_R_UNLOCK: _output << "  U " << entry.getName() << endl; break;
        case RWLOCK_R_LOCK:   _output << "  R " << entry.getName() << endl; break;
        case RWLOCK_W_UNLOCK: _output << "  X " << entry.getName() << endl; break;
        case RWLOCK_W_LOCK:   _output << "  W " << entry.getName() << endl; break;
        default: break;
        }

        if ( entry.getStartActivity() ) {
            _output << "  A " << entry.getName() << " " << entry.getStartActivity()->getName() << endl;
            if ( entry.hasHistogram() ) {
                /* BUG_668 */
                for ( unsigned p = 1; p <= DOM::Phase::MAX_PHASE; ++p ) {
                    if ( entry.hasHistogramForPhase( p ) ) {
                        const DOM::Histogram *h = entry.getHistogramForPhase( p );
                        _output << "  H " << entry.getName() << " " << p << " " << h->getMin() << " : " <<  h->getMax() << " " << h->getBins() << endl;
                    }
                }
            }
            printForwarding( entry );

        } else {
            const std::map<unsigned, DOM::Phase*>& phases = entry.getPhaseList();
            assert( phases.size() <= DOM::Phase::MAX_PHASE );

            _output << "  s " << setw( ObjectInput::__maxEntLen ) << entry.getName();
            for_each( phases.begin(), phases.end(), PhaseInput( _output, &PhaseInput::printServiceTime ) );
            _output << " -1";
            if ( !entry.getDocument()->instantiated() ) {
                printObservationVariables( _output, entry );
            }
	    _output << endl;

            if ( entry.hasNonExponentialPhases() ) {
                _output << "  c " << setw( ObjectInput::__maxEntLen ) << entry.getName();
                for_each( phases.begin(), phases.end(), PhaseInput( _output, &PhaseInput::printCoefficientOfVariation ) );
                _output << " -1" << endl;
            }
            if ( entry.hasThinkTime() ) {
                _output << "  Z " << setw( ObjectInput::__maxEntLen ) << entry.getName();
                for_each( phases.begin(), phases.end(), PhaseInput( _output, &PhaseInput::printThinkTime ) );
                _output << " -1" << endl;
            }
            if ( entry.hasDeterministicPhases() ) {
                _output << "  f " << setw( ObjectInput::__maxEntLen ) << entry.getName();
                for_each( phases.begin(), phases.end(), PhaseInput( _output, &PhaseInput::printPhaseFlag ) );
                _output << " -1" << endl;
            }
            if ( entry.hasMaxServiceTimeExceeded() ) {
                _output << "  M " << setw( ObjectInput::__maxEntLen ) << entry.getName();
                for_each( phases.begin(), phases.end(), PhaseInput( _output, &PhaseInput::printMaxServiceTimeExceeded ) );
                _output << " -1" << endl;
            }
            if ( entry.hasHistogram() ) {
                /* Histograms are stored by phase for regular entries.  Activity entries don't have phases...  Punt... */
                for ( std::map<unsigned, DOM::Phase*>::const_iterator np = phases.begin(); np != phases.end();  ++np ) {
                    const DOM::Phase * p = np->second;
                    if ( p->hasHistogram() ) {
                        const DOM::Histogram *h = p->getHistogram();
                        _output << "  H " << entry.getName() << " " << np->first << " " << h->getMin() << " : " <<  h->getMax() << " " << h->getBins() << endl;
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
	    const LQIO::DOM::Entry * dst = fwd->getDestinationEntry();
	    try {
		_output << "  F " << setw( ObjectInput::__maxEntLen ) << entry.getName()
			<< " " << setw( ObjectInput::__maxEntLen ) << dst->getName() << number_of_calls( fwd ) << " -1" << endl;
	    }
	    catch ( const std::domain_error& e ) {
		LQIO::solution_error( LQIO::ERR_INVALID_FWDING_PARAMETER, entry.getName().c_str(), dst->getName().c_str(), e.what() );
		throw_bad_parameter();
	    }
        }
    }

    void SRVN::EntryInput::printCalls( const DOM::Entry& entry ) const
    {
        /* Gather up all the call info over all phases and store in new map<to_entry,call*[3]>. */

        std::map<const DOM::Entry *, DOM::ForPhase> callsByPhase;
        const std::map<unsigned, DOM::Phase*>& phases = entry.getPhaseList();
        assert( phases.size() <= DOM::Phase::MAX_PHASE );
	for_each( phases.begin(), phases.end(), DOM::CollectCalls( callsByPhase ) );		/* Don't care about type of call here */

        /* Now iterate over the collection of calls */

        for ( std::map<const DOM::Entry *, DOM::ForPhase>::const_iterator next_y = callsByPhase.begin(); next_y != callsByPhase.end(); ++next_y ) {
	    const DOM::Entry * dst = next_y->first;
            const DOM::ForPhase& calls_by_phase = next_y->second;
            _output << "  ";
            switch ( calls_by_phase.getType() ) {
            case DOM::Call::RENDEZVOUS: _output << "y"; break;
            case DOM::Call::SEND_NO_REPLY: _output << "z"; break;
            default: abort();
            }
            _output << " " << setw( ObjectInput::__maxEntLen ) << entry.getName()
                    << " " << setw( ObjectInput::__maxEntLen ) << dst->getName();
	    unsigned int n = 0;
            for (std::map<unsigned, DOM::Phase*>::const_iterator p = phases.begin(); p != phases.end(); ++p, ++n ) {
		static const char * const phase[] = { "1", "2", "3" };
		try {
		    _output << number_of_calls( calls_by_phase[p->first] );
		}
		catch ( const std::domain_error& e ) {
		    LQIO::solution_error( LQIO::ERR_INVALID_CALL_PARAMETER,
					  "entry", entry.getName().c_str(),
					  "phase", phase[n],
					  dst->getName().c_str(), e.what() );
		    throw_bad_parameter();
		}
	    }
            _output << " -1";
            if ( !entry.getDocument()->instantiated() ) {
                for (std::map<unsigned, DOM::Phase*>::const_iterator p = phases.begin(); p != phases.end(); ++p) {
                    const DOM::Call * call = calls_by_phase[p->first];
		    if ( call ) {
			printObservationVariables( _output, *call );
		    }
                }
            }
            _output << endl;
        }
    }

    void
    SRVN::PhaseInput::operator()( const std::pair<unsigned,DOM::Phase *>& p ) const
    {
        ios_base::fmtflags oldFlags = _output.setf( ios::left, ios::adjustfield );
        (this->*_func)( *(p.second) );
        _output.flags(oldFlags);
    }

    void SRVN::PhaseInput::printCoefficientOfVariation( const DOM::Phase& p ) const
    {
	try {
	    _output << " " << setw(ObjectInput::__maxInpLen) << Input::print_double_parameter( p.getCoeffOfVariationSquared(), 0. );
	}
	catch ( const std::domain_error& e ) {
	    solution_error( LQIO::ERR_INVALID_PARAMETER, "CV sq", p.getTypeName(), p.getName().c_str(), e.what() );
	    throw_bad_parameter();
	}
	    
    }

    void SRVN::PhaseInput::printMaxServiceTimeExceeded( const DOM::Phase& p ) const
    {
	try {
	    _output << " " << setw(ObjectInput::__maxInpLen) << p.getMaxServiceTime();
	}
	catch ( const std::domain_error& e ) {
	    solution_error( LQIO::ERR_INVALID_PARAMETER, "Exceeded", p.getTypeName(), p.getName().c_str(), e.what() );
	    throw_bad_parameter();
	}
    }
    
    void SRVN::PhaseInput::printPhaseFlag( const DOM::Phase& p ) const
    {
	_output << " " << setw(ObjectInput::__maxInpLen) << (p.hasDeterministicCalls() ? "1" : "0");
    }
    
    void SRVN::PhaseInput::printServiceTime( const DOM::Phase& p ) const
    {
	try {
	    _output << " " << setw(ObjectInput::__maxInpLen) << Input::print_double_parameter( p.getServiceTime(), 0. );
	}
	catch ( const std::domain_error& e ) {
	    solution_error( LQIO::ERR_INVALID_PARAMETER, "service time", p.getTypeName(), p.getName().c_str(), e.what() );
	    throw_bad_parameter();
	}
    }
    
    void SRVN::PhaseInput::printThinkTime( const DOM::Phase& p ) const
    {
	try {
	    _output << " " << setw(ObjectInput::__maxInpLen) << Input::print_double_parameter( p.getThinkTime(), 0. );
	}
	catch ( const std::domain_error& e ) {
	    solution_error( LQIO::ERR_INVALID_PARAMETER, "CV sq", p.getTypeName(), p.getName().c_str(), e.what() );
	    throw_bad_parameter();
	}
    }

    void
    SRVN::ActivityInput::operator()( const std::pair<std::string,DOM::Activity *>& a ) const
    {
        ios_base::fmtflags oldFlags = _output.setf( ios::left, ios::adjustfield );
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
        _output << endl;
        if ( activity.isNonExponential() ) {
            _output << "  c " << activity.getName();
            printCoefficientOfVariation( activity );
            _output << endl;
        }
        if ( activity.hasThinkTime() ) {
            _output << "  Z " << activity.getName();
            printThinkTime( activity );
            _output << endl;
        }
        if ( activity.hasDeterministicCalls() ) {
            _output << "  f " << activity.getName();
            printPhaseFlag( activity );
            _output << endl;
        }
        if ( activity.hasMaxServiceTimeExceeded() ) {
            _output << "  M " << activity.getName();
            printMaxServiceTimeExceeded( activity );
            _output << endl;
        }
        if ( activity.hasHistogram() ) {
            const DOM::Histogram *h = activity.getHistogram();
            _output << "  H " << activity.getName() << h->getMin() << " : " <<  h->getMax() << " " << h->getBins() << endl;
        }
        const std::vector<DOM::Call *>& calls = activity.getCalls();
	for_each( calls.begin(), calls.end(), ActivityCallInput( _output, &ActivityCallInput::print ) );
    }


    void
    SRVN::ActivityListInput::operator()( const DOM::ActivityList * precedence ) const
    {
        ios_base::fmtflags oldFlags = _output.setf( ios::left, ios::adjustfield );
        (this->*_func)( *precedence );
        _output.flags(oldFlags);
    }

    void
    SRVN::ActivityListInput::print( const DOM::ActivityList& precedence ) const
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
        if ( _count < _size ) {
            _output << ";";
        }
        _output << endl;
    }

    void
    SRVN::ActivityListInput::printPreList( const DOM::ActivityList& precedence ) const          /* joins */
    {
        const std::vector<const DOM::Activity*>& list = precedence.getList();
        for ( std::vector<const DOM::Activity*>::const_iterator next_activity = list.begin(); next_activity != list.end(); ++next_activity ) {
            const DOM::Activity * activity = *next_activity;
            switch ( precedence.getListType() ) {
            case DOM::ActivityList::AND_JOIN_ACTIVITY_LIST:
                if ( next_activity != list.begin() ) {
                    _output << " &";
                }
                break;

            case DOM::ActivityList::OR_JOIN_ACTIVITY_LIST:
                if ( next_activity != list.begin() ) {
                    _output << " +";
                }
                break;

            case DOM::ActivityList::JOIN_ACTIVITY_LIST:
                break;

            default:
                abort();
            }

            _output << " " << activity->getName();
            const std::vector<DOM::Entry*>& replies = activity->getReplyList();
            if ( replies.size() ) {
                printReplyList( replies );
            }
        }
    }

    void
    SRVN::ActivityListInput::printPostList( const DOM::ActivityList& precedence ) const         /* forks */
    {
        const DOM::Activity * end_activity = 0;
        bool first = true;

        const std::vector<const DOM::Activity*>& list = precedence.getList();
        for ( std::vector<const DOM::Activity*>::const_iterator next_activity = list.begin(); next_activity != list.end(); ++next_activity ) {
            const DOM::Activity * activity = *next_activity;
            switch ( precedence.getListType() ) {
            case DOM::ActivityList::AND_FORK_ACTIVITY_LIST:
                if ( !first ) {
                    _output << " & ";
                }
                break;

            case DOM::ActivityList::OR_FORK_ACTIVITY_LIST:
                if ( !first ) {
                    _output << " + ";
                }
                _output << "(" << precedence.getParameterValue( activity ) << ") ";
                break;

            case DOM::ActivityList::FORK_ACTIVITY_LIST:
                break;

            case DOM::ActivityList::REPEAT_ACTIVITY_LIST:
                if ( precedence.getParameter( activity ) == NULL ) {
                    end_activity = activity;
                    continue;
                }
                if ( !first ) {
                    _output << " , ";
                }
                _output << precedence.getParameterValue( activity ) << " * ";
                break;

            default:
                abort();
            }
            _output << activity->getName();
            first = false;
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
    SRVN::CallOutput::operator()( const pair<unsigned, DOM::Entity *>& ep) const
    {
        const DOM::Task * task = dynamic_cast<const DOM::Task *>(ep.second);
        if ( !task ) return;

        ios_base::fmtflags oldFlags = _output.setf( ios::left, ios::adjustfield );
        const std::vector<DOM::Entry *>& entries = task->getEntryList();
        std::vector<DOM::Entry *>::const_iterator nextEntry;
        bool print_task_name = true;
        for ( nextEntry = entries.begin(); nextEntry != entries.end(); ++nextEntry ) {
            const DOM::Entry * entry = *nextEntry;

            /* Gather up all the call info over all phases and store in new map<to_entry,call*[3]>. */
            std::map<const DOM::Entry *, DOM::ForPhase> callsByPhase;
            const std::map<unsigned, DOM::Phase*>& phases = entry->getPhaseList();
            assert( phases.size() <= DOM::Phase::MAX_PHASE );
	    for_each( phases.begin(), phases.end(), DOM::CollectCalls( callsByPhase, _testFunc ) );

            /* Now iterate over the collection of calls */
	    if ( _meanFunc ) {
		for ( std::map<const DOM::Entry *, DOM::ForPhase>::iterator next_y = callsByPhase.begin(); next_y != callsByPhase.end(); ++next_y ) {
		    DOM::ForPhase& calls_by_phase = next_y->second;
		    calls_by_phase.setMaxPhase( __parseable ? __maxPhase : entry->getMaximumPhase() );
		    _output << entity_name( *(ep.second), print_task_name )
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
		    if ( call == NULL || !(call->*_testFunc)() ) {
			continue;
		    } else if ( _meanFunc ) {
			const DOM::Entry * dest = call->getDestinationEntry();
                        if ( count == 0 ) {
                            _output << entity_name( *(ep.second), print_task_name ) << activity_separator(__maxStrLen) << newline;
                            count += 1;
                        }
                        _output << setw(__maxStrLen) << " "  << setw(__maxStrLen-1) << activity->getName() << " " << entry_name( *dest ) << setw(__maxDblLen-1);
                        (this->*_meanFunc)( call, 0 );
                        _output << " " << activityEOF << newline;
                        if ( _confFunc && __conf95 ) {
                            _output << conf_level( __maxStrLen*3, ConfidenceIntervals::CONF_95 ) << setw(__maxDblLen-1);
                            (this->*_confFunc)( call, __conf95 );
                            _output << " " << activityEOF << newline;
                        }
                        if ( _confFunc && __conf99 ) {
                            _output << conf_level( __maxStrLen*3, ConfidenceIntervals::CONF_99 ) << setw(__maxDblLen-1);
                            (this->*_confFunc)( call, __conf99 );
                            _output << " " << activityEOF << newline;
                        }
                    } else {
			_count += 1;
		    }
                }
            }
            if ( __parseable && _meanFunc && count > 1 ) {
                _output << setw(__maxStrLen) << " " << activityEOF << newline;
            }
        }
        if ( __parseable && _meanFunc && print_task_name == false ) {
            _output << setw(__maxStrLen) << " " << activityEOF << newline;
        }
        _output.flags(oldFlags);
    }


    void
    SRVN::CallOutput::printCallRate( const DOM::Call * call, const ConfidenceIntervals* ) const
    {
        if ( call ) {
	    _output << Input::print_double_parameter( call->getCallMean(), 0 );
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
	_output << value;
    }

    void
    SRVN::CallOutput::printCallWaitingConfidence( const DOM::Call * call, const ConfidenceIntervals* conf ) const
    {
        double value = 0;
        if ( call && conf ) {
	    value = (*conf)(call->getResultWaitingTimeVariance());
        }
	_output << value;
    }

    void
    SRVN::CallOutput::printCallVarianceWaiting( const DOM::Call * call, const ConfidenceIntervals* ) const
    {
        double value = 0;
        if ( call ) {
	    value = call->getResultVarianceWaitingTime();
        }
        _output << setw(__maxDblLen) << value;
    }

    void
    SRVN::CallOutput::printCallVarianceWaitingConfidence( const DOM::Call * call, const ConfidenceIntervals* conf ) const
    {
        double value = 0;
        if ( call && conf ) {
	    value = call->getResultVarianceWaitingTimeVariance();
        }
        _output << setw(__maxDblLen) << (*conf)(value);
    }

    void
    SRVN::CallOutput::printDropProbability( const DOM::Call * call, const ConfidenceIntervals* ) const
    {
        double value = 0.0;
        if ( call ) {
	    value = call->getResultDropProbability();
        }
	_output << value;
    }

    void
    SRVN::CallOutput::printDropProbabilityConfidence( const DOM::Call * call, const ConfidenceIntervals* conf ) const
    {
        double value = 0;
        if ( call && conf ) {
	    value = (*conf)(call->getResultDropProbabilityVariance());
        }
        _output << setw(__maxDblLen) << value;
    }

    ostream&
    SRVN::CallOutput::printCalls( ostream& output, const CallOutput& info, const DOM::ForPhase& phases, const callConfFPtr func, const ConfidenceIntervals* conf )
    {
        for ( unsigned p = 1; p <= phases.getMaxPhase(); ++p ) {
            output << setw(__maxDblLen-1);
            (info.*func)( phases[p], conf );
            output << " ";
        }
        output << activityEOF;
        return output;
    }

    void
    SRVN::ActivityCallInput::print( const DOM::Call* call ) const
    {
	const LQIO::DOM::Activity * src = dynamic_cast<const LQIO::DOM::Activity *>(call->getSourceObject());
	assert(src != nullptr );
	const LQIO::DOM::Entry * dst = call->getDestinationEntry();
	try {
	    _output << "  " << call_type( call ) << " " << src->getName() << " " << dst->getName() << number_of_calls( call );
	}
	catch ( const std::domain_error& e ) {
	    const LQIO::DOM::Task * task = src->getTask();
	    LQIO::solution_error( LQIO::ERR_INVALID_CALL_PARAMETER,
				  "task", task->getName().c_str(),
				  src->getTypeName(), src->getName().c_str(),
				  dst->getName().c_str(), e.what() );
	    throw_bad_parameter();
	}
	if ( !call->getDocument()->instantiated() ) {
	    printObservationVariables( _output, *call );
	}
	_output << endl;
    }

    /* -------------------------------------------------------------------- */
    /* Histograms                                                           */
    /* -------------------------------------------------------------------- */

    void
    SRVN::HistogramOutput::operator()( const pair<unsigned, DOM::Entity *>& ep ) const
    {
        if ( __parseable ) return;

        const DOM::Task * task = dynamic_cast<const DOM::Task *>(ep.second);
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
        std::set<DOM::ActivityList*>::const_iterator nextActivityList;
        for ( nextActivityList = activity_lists.begin(); nextActivityList != activity_lists.end(); ++nextActivityList ) {
            const DOM::ActivityList * activityList = *nextActivityList;
            if ( activityList->hasHistogram() ) {
                const DOM::Activity * first;
                const DOM::Activity * last;
                activityList->activitiesForName( &first, &last );
                _output << "Histogram for task " << task->getName() << ", join activity list " << first->getName() << " to " << last->getName() << newline << newline;
                (this->*_func)( *activityList->getHistogram() );
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

        if ( histogram.getHistogramType() == DOM::Histogram::CONTINUOUS ) {
            _output << setw(4) << " " << setw( 17 ) << "<=  bin  <";
        } else {
            _output << "  bin  ";
        }
        _output  << " " << setw(9) << "mean";
        if ( __conf95 ) {
            _output << " " << setw(9) << "+/- 95%";
        }
        _output << newline;
        for ( unsigned int i = ( hist_min == 0 ? 1 : 0); i <= limit; i++ ) {
            const double mean = histogram.getBinMean( i );
            if ( i == limit && mean == 0 ) break;

            const double x1 = ( i == 0 ) ? 0 : hist_min + (i-1) * bin_size;
            const streamsize old_precision = _output.precision( 6 );
            if ( histogram.getHistogramType() == DOM::Histogram::CONTINUOUS ) {
                const double x2 = ( i == limit ) ? __DBL_MAX__ : hist_min + i * bin_size;
                _output << setw( 4 ) << " ";
                if ( i == 0 ) {
                    _output << setw( 17 ) << "underflow";
                } else if ( i == limit ) {
                    _output << setw( 17 ) << "overflow";
                } else {
                    _output << setw( 8 ) << x1 << " " << setw( 8 ) << x2;
                }
            } else {
                _output << "  " << setw(3) << x1 << "  ";
            }
            _output.setf( ios::fixed, ios::floatfield );
            _output << " " << setw( 9 ) << mean;
            if ( __conf95 ) {
                _output << " " << setw( 9 ) << (*__conf95)(histogram.getBinVariance( i ));
            }
            _output.unsetf( ios::floatfield );
            _output.precision( old_precision );
            _output << "|";
            const unsigned int count = static_cast<unsigned int>( mean * plot_width / max_value + 0.5 );
            if ( count > 0 ) {
                _output << setw( count ) << setfill( '*' ) << '*' << setfill( ' ' );
            }
            _output << newline;
        }
        _output << newline;
    }
}
