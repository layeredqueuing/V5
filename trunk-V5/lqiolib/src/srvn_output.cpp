/*
 *  $Id$
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
#include "srvn_output.h"
#include "srvn_spex.h"
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

    bool SRVN::ObjectOutput::__task_has_think_time = false;
    bool SRVN::ObjectOutput::__task_has_group = false;


    unsigned int SRVN::ObjectOutput::__maxPhase = 0;
    ConfidenceIntervals * SRVN::ObjectOutput::__conf95 = 0;
    ConfidenceIntervals * SRVN::ObjectOutput::__conf99 = 0;
    bool SRVN::ObjectOutput::__parseable = false;
    bool SRVN::ObjectOutput::__rtf = false;
    bool SRVN::ObjectOutput::__coloured = false;
    unsigned int SRVN::ObjectInput::__maxEntLen = 1;
    unsigned int SRVN::ObjectInput::__maxInpLen = 1;
    

    SRVN::Output::Output( const DOM::Document& document, const map<unsigned, DOM::Entity *>& entities,
			  bool print_confidence_intervals, bool print_variances, bool print_histograms )
        : _document(document), _entities(entities), 
	  _print_variances(print_variances), _print_histograms(print_histograms)
    {
        /* Set various globals for pretting printing */
        assert( ObjectOutput::__maxPhase == 0 );
        ObjectOutput::__maxPhase = document.getMaximumPhase();
        ObjectOutput::__task_has_think_time = document.taskHasThinkTime();
	ObjectOutput::__task_has_group = document.getNumberOfGroups() > 0;
        ObjectOutput::__parseable = false;
	ObjectOutput::__rtf = false;
	ObjectOutput::__coloured = false;
        const unsigned number_of_blocks = document.getResultNumberOfBlocks();
        if ( number_of_blocks >= 2 && print_confidence_intervals ) {
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

    SRVN::TimeManip SRVN::ObjectOutput::print_time( const clock_t t )  { return SRVN::TimeManip( &SRVN::ObjectOutput::printTime, t ); }
    SRVN::StringManip SRVN::Output::task_header( const char * s )      { return SRVN::StringManip( &SRVN::Output::taskHeader, s ); }
    SRVN::StringManip SRVN::Output::entry_header( const char * s )     { return SRVN::StringManip( &SRVN::Output::entryHeader, s ); }
    SRVN::StringManip SRVN::Output::activity_header( const char * s )  { return SRVN::StringManip( &SRVN::Output::activityHeader, s ); }
    SRVN::StringManip SRVN::Output::call_header( const char * s )      { return SRVN::StringManip( &SRVN::Output::callHeader, s ); }
    SRVN::UnsignedManip SRVN::Output::phase_header( const unsigned n ) { return SRVN::UnsignedManip( &SRVN::Output::phaseHeader, n ); }
    SRVN::StringManip SRVN::Output::hold_header( const char * s )      { return SRVN::StringManip( &SRVN::Output::holdHeader, s ); }
    SRVN::StringManip SRVN::Output::rwlock_header( const char * s )    { return SRVN::StringManip( &SRVN::Output::rwlockHeader, s ); }

    ostream&
    SRVN::ObjectOutput::printTime( ostream& output, const clock_t time )
    {
#if defined(HAVE_SYS_TIME_H)
#if defined(CLK_TCK)
	const double dtime = static_cast<double>(time) / static_cast<double>(CLK_TCK);
#else
	const double dtime = static_cast<double>(time) / static_cast<double>(sysconf(_SC_CLK_TCK));
#endif
	const double csecs = fmod( dtime * 100.0, 100.0 );
#else
	const double dtime = time;
	const double csecs = 0.0;
#endif
        const double secs  = fmod( floor( dtime ), 60.0 );
        const double mins  = fmod( floor( dtime / 60.0 ), 60.0 );
        const double hrs   = floor( dtime / 3600.0 );

        const std::_Ios_Fmtflags flags = output.setf( ios::dec|ios::fixed, ios::basefield|ios::fixed );
        const int precision = output.precision(0);
        output.setf( ios::right, ios::adjustfield );

        output << setw(2) << hrs;
        char fill = output.fill('0');
        output << ':' << setw(2) << mins;
        output << ':' << setw(2) << secs;
        output << '.' << setw(2) << csecs;

        output.flags(flags);
        output.precision(precision);
        output.fill(fill);
        return output;
    }


    /*
     * Print out a generic header for Entry name on fptr.
     */

    ostream&
    SRVN::Output::taskHeader( ostream& output, const char * s )
    {
        output << newline << newline << textbf << s << newline << newline << textrm
               << setw(ObjectOutput::__maxStrLen) << "Task Name" << setw(ObjectOutput::__maxStrLen) << "Entry Name";
        return output;
    }

    /* static */ ostream&
    SRVN::Output::entryHeader( ostream& output, const char * s )
    {
        output << task_header( s ) << phase_header( ObjectOutput::__maxPhase );
        return output;
    }

    ostream&
    SRVN::Output::activityHeader( ostream& output, const char * s )
    {
        output << newline << newline << s << newline << newline
               << setw(ObjectOutput::__maxStrLen) << "Task Name"
               << setw(ObjectOutput::__maxStrLen) << "Source Activity"
               << setw(ObjectOutput::__maxStrLen) << "Target Activity";
        return output;
    }

    ostream&
    SRVN::Output::callHeader( ostream& output, const char * s )
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
        std::_Ios_Fmtflags oldFlags = output.setf( ios::left, ios::adjustfield );
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
    SRVN::Output::holdHeader( ostream& output, const char * s )
    {
        std::_Ios_Fmtflags oldFlags = output.setf( ios::left, ios::adjustfield );
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
    SRVN::Output::rwlockHeader( ostream& output, const char * s )
    {
        std::_Ios_Fmtflags oldFlags = output.setf( ios::left, ios::adjustfield );
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
        std::_Ios_Fmtflags oldFlags = output.setf( ios::left, ios::adjustfield );

        output << newline << textbf << processor_info_str << newline << newline 
               << textrm << setw(ObjectOutput::__maxStrLen) << "Processor Name" << "Type    Copies  Scheduling";
	if ( getDOM().processorHasRate() ) {
	    output << " Rate";
	}
	output << newline;
        for_each( _entities.begin(), _entities.end(), ProcessorOutput( output, &ProcessorOutput::printParameters ) );

	const std::map<std::string,DOM::Group*>& groups = const_cast<DOM::Document&>(getDOM()).getGroups();
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

        if ( getDOM().entryHasThinkTime() ) {
            output << entry_header( think_time_str ) << newline;
            for_each( _entities.begin(), _entities.end(), EntryOutput( output, &EntryOutput::printEntryThinkTime, &EntryOutput::printActivityThinkTime ) );
        }

	if ( getDOM().hasMaxServiceTime() ) {
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

        if ( getDOM().entryHasDeterministicPhase() ) {
	    output << entry_header( phase_type_str ) << newline;
            for_each( _entities.begin(), _entities.end(), EntryOutput( output, &EntryOutput::printEntryPhaseType, &EntryOutput::printActivityPhaseType ) );
        } else {
            output << newline << newline << textbf << phase_type_str << newline << textrm;
            output << "All phases are stochastic." << newline;
        }

        if ( getDOM().entryHasNonExponentialPhase() ) {
            output << entry_header( cv_square_str ) << newline << textrm;
            for_each( _entities.begin(), _entities.end(), EntryOutput( output, &EntryOutput::printEntryCoefficientOfVariation, &EntryOutput::printActivityCoefficientOfVariation ) );
        } else {
            output << newline << newline << textbf << cv_square_str << newline << textrm;
            output << "All executable segments are exponential." << newline;
        }

        output << newline << newline << textbf << open_arrival_rate_str << newline << textrm;
        if ( getDOM().entryHasOpenArrivals() ) {
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
        std::_Ios_Fmtflags oldFlags = output.setf( ios::left, ios::adjustfield );

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

	if ( getDOM().hasMaxServiceTime() ) {
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

        if ( getDOM().hasReaderWait() ) {
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


    /*
     * Manipulators for inline output expressions ie
     * cout << entity_name( "foo", flag ) << newline;
     *
     * The SRVN::Output functions passed as an argument to the constructor do the work.  Different manipulators exist for typing.
     */

    SRVN::EntityNameManip SRVN::ObjectOutput::entity_name( const DOM::Entity& e, bool& b ) { return SRVN::EntityNameManip( &SRVN::ObjectOutput::printEntityName, e, b ); }
    SRVN::EntryNameManip SRVN::ObjectOutput::entry_name( const DOM::Entry& e ) { return SRVN::EntryNameManip( &SRVN::ObjectOutput::printEntryName, e ); }
    SRVN::UnsignedManip SRVN::ObjectOutput::activity_separator( const unsigned n ) { return SRVN::UnsignedManip( &SRVN::ObjectOutput::activitySeparator, n ); }
    SRVN::ConfidenceManip SRVN::ObjectOutput::conf_level( const unsigned int n, const ConfidenceIntervals::confidence_level_t l ) { return SRVN::ConfidenceManip( &SRVN::ObjectOutput::confLevel,n,l); }
    SRVN::DoubleManip SRVN::ObjectOutput::colour( const double u ) { return SRVN::DoubleManip( &SRVN::ObjectOutput::textcolour, u ); }

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
	    std::_Ios_Fmtflags oldFlags = output.setf( ios::right, ios::adjustfield );
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
        std::_Ios_Fmtflags flags = output.setf( ios::right, ios::adjustfield );
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
		output << "\\cf2 ";		/* Red 255,0,0 */
		__coloured = true;
	    } else if ( utilization >= 0.6 ) {
		output << "\\cf3 ";		/* Orange 255,164,0 */
		__coloured = true;
	    } else if ( utilization >= 0.5 ) {
		output << "\\cf4 ";		/* Green 0,255,0 */
		__coloured = true;
	    } else if ( utilization >= 0.4 ) {
		output << "\\cf5 ";		/* Blue 0,255,0 */
		__coloured = true;
	    } else if ( __coloured ) {
		output << "\\cf0 ";		/* Black 0,0,0 */
		__coloured = false;
	    }
	}
	return output;
    }

    ostream&
    SRVN::ObjectOutput::entryInfo( ostream& output, const DOM::Entry & entry, const entryFunc func )
    {
        const DOM::ExternalVariable * value = (entry.*func)();
        if ( value  != 0 ) {
            output << setw(__maxDblLen) << to_double( *value ) << ' ';
        } else {
            output << setw(__maxDblLen) << 0.0;
        }
        return output;
    }

    /* static */ ostream&
    SRVN::ObjectOutput::phaseInfo( ostream& output, const DOM::Entry & entry, const phaseFunc func )
    {
        const std::map<unsigned, DOM::Phase*>& phases = entry.getPhaseList();
        std::map<unsigned, DOM::Phase*>::const_iterator p;
        for (p = phases.begin(); p != phases.end(); ++p) {
            const DOM::Phase* phase = p->second;
            const DOM::ExternalVariable * value;
            if ( phase && (value = (phase->*func)()) != 0 ) {
                output << setw(__maxDblLen-1) << to_double( *value ) << ' ';
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
	if ( entry.getStartActivity() ) {
	    if ( !entry_func ) return output;
	    for ( unsigned int p = 1; p <= 2; ++p ) {
		output << setw(__maxDblLen-1) << (entry.*entry_func)(p) << ' ';
	    }
	    np = 2;
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
	if ( entry.getStartActivity() ) {
	    if ( !entry_func ) return output;
	    for ( unsigned int p = 1; p <= 2; ++p ) {
		output << setw(__maxDblLen-1) << (*conf)((entry.*entry_func)(p)) << ' ';
	    }
	    np = 2;
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

    ostream&
    SRVN::ObjectInput::printNumberOfCalls( ostream& output, const DOM::Call& call ) 
    {
	const DOM::ExternalVariable* var = call.getCallMean();
	double val = 0;
	output << " " << setw(ObjectInput::__maxInpLen);
	if ( !var ) {
	    output << "0";
	} else if ( var->wasSet() && var->getValue( val ) ) {
	    output << val;
	} else {
	    output << *var;
	}
	return output;
    }


    ostream&
    SRVN::ObjectInput::printCallType( ostream& output, const DOM::Call& call ) 
    {
	switch ( call.getCallType() ) {
	case DOM::Call::RENDEZVOUS: output << "y"; break;
	case DOM::Call::SEND_NO_REPLY: output << "z"; break;
	default: abort();
	}
	return output;
    }

    /* ---------------------------------------------------------------- */
    /* Parseable Output							*/
    /* ---------------------------------------------------------------- */

    SRVN::Parseable::Parseable( const DOM::Document& document, const map<unsigned, DOM::Entity *>& entities, bool print_confidence_intervals )
	: SRVN::Output( document, entities, print_confidence_intervals )
    {
	ObjectOutput::__parseable = true;		/* Set global for formatting. */
	ObjectOutput::__rtf = false;			/* Set global for formatting. */
    }

    SRVN::Parseable::~Parseable()
    {
	ObjectOutput::__parseable = false;		/* Set global for formatting. */
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
	output << "# " << DOM::Document::io_vars->lq_toolname << " " << LQIO::DOM::Document::io_vars->lq_version << endl;
	if ( DOM::Document::io_vars->lq_command_line ) {
	    output << "# " << DOM::Document::io_vars->lq_command_line << ' ' << LQIO::input_file_name << endl;
	}
	const DOM::Document& document(getDOM());
	output << "V " << (document.getResultValid() ? "y" : "n") << endl
	       << "C " << document.getResultConvergenceValue() << endl
	       << "I " << document.getResultIterations() << endl
	       << "PP " << document.getNumberOfProcessors() << endl
	       << "NP " << document.getMaximumPhase() << endl;
	output << "#!User: " << SRVN::ObjectOutput::print_time( document.getResultUserTime() ) << endl
	       << "#!Sys:  " << SRVN::ObjectOutput::print_time( document.getResultSysTime() ) << endl
	       << "#!Real: " << SRVN::ObjectOutput::print_time( document.getResultElapsedTime() ) << endl;
	if ( document._mvaStatistics.submodels > 0 ) {
	    output << "#!Solver: "
		   << document._mvaStatistics.submodels << ' '
		   << document._mvaStatistics.core << ' '
		   << document._mvaStatistics.step << ' '
		   << document._mvaStatistics.step_squared << ' '
		   << document._mvaStatistics.wait << ' '
		   << document._mvaStatistics.wait_squared << ' '
		   << document._mvaStatistics.faults << endl;
	}
	output << endl;
	return output;
    }


    ostream&
    SRVN::Parseable::printResults( ostream& output ) const
    {
	std::_Ios_Fmtflags oldFlags = output.setf( ios::left, ios::adjustfield );

	if ( getDOM().entryHasThroughputBound() ) {
	    output << "B " << getDOM().getNumberOfEntries() << endl;
	    for_each( _entities.begin(), _entities.end(), EntryOutput( output, &EntryOutput::printEntryThroughputBounds ) );
	    output << "-1" << endl << endl;
	}

	/* Waiting times */

	if ( getDOM().hasRendezvous() ) {
	    output << "W 0" << endl;
	    for_each( _entities.begin(), _entities.end(), CallOutput( output, &DOM::Call::hasRendezvous, &CallOutput::printCallWaiting, &CallOutput::printCallWaitingConfidence ) );
	    output << "-1" << endl << endl;

	    if ( getDOM().entryHasWaitingTimeVariance() ) {
		output << "VARW 0" << endl;
		for_each( _entities.begin(), _entities.end(), CallOutput( output, &DOM::Call::hasRendezvous, &CallOutput::printCallVarianceWaiting, &CallOutput::printCallVarianceWaitingConfidence ) );
		output << "-1" << endl << endl;
	    }
	}
	if ( getDOM().hasSendNoReply() ) {
	    output << "Z 0" << endl;
	    for_each( _entities.begin(), _entities.end(), CallOutput( output, &DOM::Call::hasSendNoReply, &CallOutput::printCallWaiting, &CallOutput::printCallWaitingConfidence ) );
	    output << "-1" << endl << endl;
	    if ( getDOM().entryHasWaitingTimeVariance() ) {
		output << "VARZ 0" << endl;
		for_each( _entities.begin(), _entities.end(), CallOutput( output, &DOM::Call::hasSendNoReply, &CallOutput::printCallVarianceWaiting, &CallOutput::printCallVarianceWaitingConfidence ) );
		output << "-1" << endl << endl;
	    }
	}

	/* Drop probabilities. */

	if ( getDOM().entryHasDropProbability() ) {
	    output << "DP " << 0 << endl;
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

	if ( getDOM().hasMaxServiceTime() ) {
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

	if ( getDOM().hasReaderWait() ) {
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

    /* ---------------------------------------------------------------- */
    /* RTF Output							*/
    /* ---------------------------------------------------------------- */

    SRVN::RTF::RTF( const DOM::Document& document, const map<unsigned, DOM::Entity *>& entities, bool print_confidence_intervals )
	: SRVN::Output( document, entities, print_confidence_intervals )
    {
	ObjectOutput::__parseable = false;		/* Set global for formatting. */
	ObjectOutput::__rtf = true;			/* Set global for formatting. */
    }

    SRVN::RTF::~RTF()
    {
	ObjectOutput::__rtf = false;			/* Set global for formatting. */
    }

    ostream&
    SRVN::RTF::printPreamble( ostream& output ) const
    {
	output << "{\\rtf1\\ansi\\ansicpg1252\\cocoartf1138\\cocoasubrtf230" << endl		// Boilerplate.
	       << "{\\fonttbl\\f0\\fmodern\\fcharset0 CourierNewPSMT;\\f1\\fmodern\\fcharset0 CourierNewPS-BoldMT;\\f2\\fmodern\\fcharset0 CourierNewPS-ItalicMT;}" << endl	// Fonts (f0, f1... )
	       << "{\\colortbl;\\red255\\green255\\blue255;\\red255\\green0\\blue0;\\red255\\green164\\blue0;\\red0\\green255\\blue0;\\red0\\green0\\blue255;}" << endl		// Colour table. (black, white, red).
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
    /* Input Output							*/
    /* ---------------------------------------------------------------- */

    SRVN::Input::Input( const DOM::Document& document, const map<unsigned, DOM::Entity *>& entities, bool annotate  )
	: _document(document), _entities(entities), _annotate(annotate)
    {
	/* Initialize lengths */
	const std::map<std::string,LQIO::DOM::Entry*>& entries = document.getEntries();
	ObjectInput::__maxEntLen = 1;		/* default */
	ObjectInput::__maxInpLen = 1;		/* default */

	for ( std::map<std::string,LQIO::DOM::Entry*>::const_iterator e = entries.begin(); e != entries.end(); ++e ) {
	    unsigned long size = e->first.size();
	    if ( size > ObjectInput::__maxEntLen ) ObjectInput::__maxEntLen = size;

	    const LQIO::DOM::Entry * entry = e->second;
	    const std::map<unsigned, DOM::Phase*>& phases = entry->getPhaseList();
	    for (std::map<unsigned, DOM::Phase*>::const_iterator p = phases.begin(); p != phases.end(); ++p) {
		ostringstream s;
		const DOM::Phase* phase = p->second;
		if ( !phase ) continue;
		const DOM::ExternalVariable * var = phase->getServiceTime();
		double val;
		if ( !var ) continue;
		else if ( var->wasSet() && var->getValue( val ) ) {
		    s << val;
		} else {
		    s << *var;
		}
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
	if ( _document.getSymbolExternalVariableCount() != 0 ) {
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
	return output;

	const unsigned int n_R = LQIO::DOM::Spex::__result_variables.size();
	if ( n_R > 0 ) {
	    output << "R " << n_R << endl;
	    output << "-1" << endl << endl;
	}
    }

/*
 * Print out the "general information" for srvn input output.
 */

    ostream&
    SRVN::Input::printGeneral( ostream& output ) const
    {	
	output << "# SRVN Model Description File, for file: " << LQIO::input_file_name << endl
	       << "# Generated by: " << LQIO::DOM::Document::io_vars->lq_toolname << ", version " << LQIO::DOM::Document::io_vars->lq_version << endl;
#if defined(HAVE_GETEUID)
	struct passwd * passwd = getpwuid(geteuid());
	output << "# For: " << passwd->pw_name << endl;
#elif defined(WINNT)
	output << "#For : " << getenv("USERNAME") << endl;
#endif
#if defined(HAVE_CTIME)
	time_t tloc;
	time( &tloc );
	output << "# " << ctime( &tloc );
#endif
	output << "# Invoked as: " << LQIO::DOM::Document::io_vars->lq_command_line << ' ' << LQIO::input_file_name << endl;
	
	const map<string,string>& pragmas = _document.getPragmaList();
	if ( pragmas.size() ) {
	    for ( map<string,string>::const_iterator nextPragma = pragmas.begin(); nextPragma != pragmas.end(); ++nextPragma ) {
		output << "#pragma " << nextPragma->first << "=" << nextPragma->second << endl;
	    }
	    output << endl;
	}

	/* Print general information */

	output << "G \"" << _document.getModelComment() << "\" " ;
	if ( _annotate ) {
	    output << "\t\t\t# Model comment " << endl
		   << _document.getModelConvergenceValue() << "\t\t\t# Convergence test value." << endl
		   << _document.getModelIterationLimit() << "\t\t\t# Maximum number of iterations." << endl
		   << _document.getModelPrintInterval() << "\t\t\t# Print intermediate results (see manual pages)" << endl
		   << _document.getModelUnderrelaxationCoefficient() << "\t\t\t# Model under-relaxation ( 0.0 < x <= 1.0)" << endl
		   << -1 << endl << endl;
	} else {
	    output << _document.getModelConvergenceValue() << " "
		   << _document.getModelIterationLimit() << " "
		   << _document.getModelPrintInterval() << " "
		   << _document.getModelUnderrelaxationCoefficient() << " "
		   << -1 << endl << endl;
	}
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

    /* -------------------------------------------------------------------- */
    /* Document                                                             */
    /* -------------------------------------------------------------------- */

    void
    SRVN::DocumentOutput::operator()( const DOM::Document& document ) const
    {	
        _output << "Generated by: " << LQIO::DOM::Document::io_vars->lq_toolname << ", version " << LQIO::DOM::Document::io_vars->lq_version << newline
		<< textit 
		<< "Copyright the Real-Time and Distributed Systems Group," << newline
		<< "Department of Systems and Computer Engineering" << newline
		<< "Carleton University, Ottawa, Ontario, Canada. K1S 5B6" << newline
		<< textrm << newline;

        if ( LQIO::DOM::Document::io_vars->lq_command_line ) {
            _output << "Invoked as: " << LQIO::DOM::Document::io_vars->lq_command_line << ' ' << LQIO::input_file_name << newline;
        }
        _output << "Input:  " << LQIO::input_file_name << newline;
#if     defined(HAVE_CTIME)
        time_t clock = time( (time_t *)0 );
        _output << ctime( &clock ) << newline;
#endif

        if ( document.getModelComment().length() > 0 ) {
            _output << "Comment: " << document.getModelComment() << newline;
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

        _output << "Solver: " << document.getResultPlatformInformation() << newline
		<< "    User:       " << print_time( document.getResultUserTime() ) << newline
		<< "    System:     " << print_time( document.getResultSysTime() ) << newline
		<< "    Elapsed:    " << print_time( document.getResultElapsedTime() ) << newline
		<< newline;

	if ( document._mvaStatistics.submodels > 0 ) {
	    _output << "    Submodels:  " << document._mvaStatistics.submodels << newline
		    << "    MVA Core(): " << setw(ObjectOutput::__maxDblLen) << document._mvaStatistics.core << newline
		    << "    MVA Step(): " << setw(ObjectOutput::__maxDblLen) << document._mvaStatistics.step << newline
//				       document._mvaStatistics.step_squared <<
		    << "    MVA Wait(): " << setw(ObjectOutput::__maxDblLen) << document._mvaStatistics.wait << newline;
//				       document._mvaStatistics.wait_squared <<
	    if ( document._mvaStatistics.faults ) {
		_output << "*** Faults ***" << document._mvaStatistics.faults << newline;
	    }
	}
    }

    /* -------------------------------------------------------------------- */
    /* Entities                                                             */
    /* -------------------------------------------------------------------- */

    void
    SRVN::EntityOutput::printParameters( const DOM::Entity& entity ) const
    {
        bool print_task_name = true;
        _output << entity_name( entity, print_task_name );
        ostringstream myType;
        if ( entity.getSchedulingType() == SCHEDULE_DELAY ) {
            myType << "Inf";
	} else if ( entity.getSchedulingType() == SCHEDULE_CUSTOMER ) {
            myType << "Ref("  << to_double( *entity.getCopies() ) << ")";
	} else if ( entity.getSchedulingType() == SCHEDULE_SEMAPHORE ) {
            myType << "Sema("  << to_double( *entity.getCopies() ) << ")";
	} else if ( entity.getSchedulingType() == SCHEDULE_RWLOCK ) {
            myType << "rw("  << to_double( *entity.getCopies() ) << ")";
        } else if ( entity.isMultiserver() ) {
            myType << "Mult(" << to_double( *entity.getCopies() ) << ")";
        } else {
            myType << "Uni";
        }
        _output << setw(9) << myType.str() << " " << setw(5) << entity.getReplicas() << " ";
    }

    SRVN::EntityManip SRVN::EntityInput::copies_of( const DOM::Entity & e )     { return SRVN::EntityManip( &SRVN::EntityInput::printCopies, e ); }
    SRVN::EntityManip SRVN::EntityInput::replicas_of( const DOM::Entity & e )   { return SRVN::EntityManip( &SRVN::EntityInput::printReplicas, e ); }

    ostream&
    SRVN::EntityInput::print( ostream& output, const DOM::Entity * entity ) 
    {
	if ( dynamic_cast<const DOM::Task *>(entity) ) {
	    LQIO::SRVN::TaskInput( output, 0 ).print( *dynamic_cast<const LQIO::DOM::Task *>(entity) );
	} else {
	    LQIO::SRVN::ProcessorInput( output, 0 ).print( *dynamic_cast<const LQIO::DOM::Processor *>(entity) );
	}
	return output;
    }

    ostream&
    SRVN::EntityInput::printCopies( ostream& output, const DOM::Entity & entity ) 
    { 
	const LQIO::DOM::ExternalVariable * m = entity.getCopies();
	double v;
	if ( m != 0 ) {
	    if ( m->wasSet() && m->getValue(v) ) {
		if ( v > 1 ) {
		    output << " m " << v;
		}
	    } else {
		output << " m " << *m;
	    }
	}
	return output;
    }

    ostream&
    SRVN::EntityInput::printReplicas( ostream& output, const DOM::Entity & entity )
    { 
	unsigned v = entity.getReplicas();
	if ( v > 1 ) {
	    output << " r " << v;
	}
	return output;
    }


    /* -------------------------------------------------------------------- */
    /* Processors                                                           */
    /* -------------------------------------------------------------------- */

    SRVN::ResultsManip SRVN::ProcessorOutput::processor_queueing_time( const DOM::Entry& e )  { return SRVN::ResultsManip( &SRVN::ObjectOutput::phaseResults, e, &DOM::Phase::getResultProcessorWaiting, &DOM::Phase::getResultZero ); }
    SRVN::ResultsConfidenceManip SRVN::ProcessorOutput::processor_queueing_time_confidence( const DOM::Entry& e, const ConfidenceIntervals * c )  { return SRVN::ResultsConfidenceManip( &SRVN::ObjectOutput::phaseResultsConfidence, e, &DOM::Phase::getResultProcessorWaitingVariance, &DOM::Phase::getResultZero, c, true ); }

    void
    SRVN::ProcessorOutput::operator()( const pair<unsigned, DOM::Entity *>& ep) const
    {
        const DOM::Processor * processor = dynamic_cast<const DOM::Processor *>(ep.second);
        if ( !processor ) return;

        std::_Ios_Fmtflags oldFlags = _output.setf( ios::left, ios::adjustfield );
        (this->*_func)( *processor );
        _output.flags(oldFlags);
    }


    void
    SRVN::ProcessorOutput::printParameters( const DOM::Processor& processor ) const
    {
        const std::_Ios_Fmtflags oldFlags = _output.setf( ios::left, ios::adjustfield );
        SRVN::EntityOutput::printParameters( processor );
        _output << scheduling_type_str[processor.getSchedulingType()];
	if ( processor.hasRate() && processor.getRateValue() != 1 ) {
	    _output << " " << processor.getRateValue();
	}
	_output << newline;
        _output.flags(oldFlags);
    }

    void
    SRVN::ProcessorOutput::printUtilizationAndWaiting( const DOM::Processor& processor ) const
    {
        const std::vector<DOM::Task*>& tasks = processor.getTaskList();
	const std::vector<DOM::Group*>& groups = processor.getGroupList();
	const double proc_util = !processor.isInfinite() ? processor.getResultUtilization() / (static_cast<double>(processor.getCopiesValue()) * processor.getRateValue()): 0;

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
	    for ( std::vector<DOM::Group*>::const_iterator nextGroup = groups.begin(); nextGroup != groups.end(); ++nextGroup ) {
		const DOM::Group& group = *(*nextGroup);
		printUtilizationAndWaiting( processor, group.getTaskList() );
		if ( !__parseable ) {
		    _output << textit << setw(__maxStrLen*2+8) << "Group Total:"
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
    SRVN::ProcessorOutput::printUtilizationAndWaiting( const DOM::Processor& processor, const std::vector<DOM::Task*>& tasks ) const
    {
	const bool is_infinite = processor.isInfinite();
        for ( std::vector<DOM::Task*>::const_iterator nextTask = tasks.begin(); nextTask != tasks.end(); ++nextTask ) {
            const DOM::Task * task = *nextTask;
            bool print_task_name = true;
            unsigned int item_count = 0;

            const std::vector<DOM::Entry *> & entries = task->getEntryList();
            for ( std::vector<DOM::Entry *>::const_iterator nextEntry = entries.begin(); nextEntry != entries.end(); ++nextEntry, ++item_count ) {
                const DOM::Entry * entry = *nextEntry;
		const double entry_util = (is_infinite) ? entry->getResultProcessorUtilization() / (static_cast<double>(processor.getCopiesValue()) * processor.getRateValue()) : 0;
		_output << colour( entry_util );
		if ( __parseable ) {
		    if ( print_task_name ) {
			_output << setw( __maxStrLen-1 ) << task->getName() << " "
				<< setw(2) << entries.size() << " "
				<< setw(1) << task->getPriority() << " "
				<< setw(2) << to_double( *task->getCopies() ) << " ";
			print_task_name = false;
		    } else {
			_output << setw(__maxStrLen+8) << " ";
		    }
                } else if ( print_task_name ) {
                    _output << entity_name( *task, print_task_name )
                            << setw(3) << task->getPriority() << " "
                            << setw(3) << to_double( *task->getCopies() ) << " ";
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
		const double task_util = !processor.isInfinite() ? task->getResultProcessorUtilization() / (static_cast<double>(processor.getCopiesValue()) * processor.getRateValue()): 0;
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

        std::_Ios_Fmtflags oldFlags = _output.setf( ios::left, ios::adjustfield );
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
	_output << endl;
    }

    ostream&
    SRVN::ProcessorInput::printRate( ostream& output, const DOM::Processor& processor )
    {
	if ( !dynamic_cast<const DOM::Processor *>(& processor) ) return output;

	const LQIO::DOM::ExternalVariable * r = dynamic_cast<const DOM::Processor&>(processor).getRate();
	double v;
	if ( r != 0 ) {
	    if ( r->wasSet() && r->getValue(v) ) {
		if ( v > 1.0 ) {
		    output << " R " << v;
		}
	    } else {
		output << " R " << *r;
	    }
	}
	return output;
    }


    ostream&
    SRVN::ProcessorInput::printScheduling( ostream& output, const DOM::Processor & processor )
    { 
	output << ' ';
	switch ( processor.getSchedulingType() ) {
	case SCHEDULE_DELAY:    output << 'i'; break;
	case SCHEDULE_FIFO:	output << 'f'; break;
	case SCHEDULE_HOL:	output << 'h'; break;
	case SCHEDULE_PPR:	output << 'p'; break;
	case SCHEDULE_RAND:     output << 'r'; break;
	case SCHEDULE_CFS:	output << "c " << processor.getQuantumValue(); break;
	case SCHEDULE_PS:       output << "s " << processor.getQuantumValue(); break;
	case SCHEDULE_PS_HOL:   output << "H " << processor.getQuantumValue(); break;
	case SCHEDULE_PS_PPR:   output << "P " << processor.getQuantumValue(); break;
	default:		output << '?'; break;
	} 
	return output;
    }


    /* -------------------------------------------------------------------- */
    /* Groups                                                               */
    /* -------------------------------------------------------------------- */

    void
    SRVN::GroupOutput::operator()( const std::pair<const std::string,DOM::Group*>& group ) const
    {
        std::_Ios_Fmtflags oldFlags = _output.setf( ios::left, ios::adjustfield );
        (this->*_func)( *(group.second) );
        _output.flags(oldFlags);
    }

    void
    SRVN::GroupOutput::printParameters( const DOM::Group& group ) const
    {
        const std::_Ios_Fmtflags oldFlags = _output.setf( ios::left, ios::adjustfield );
	_output << setw(__maxStrLen-1) << group.getName()
		<< " " << setw(6) << *group.getGroupShare();
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
        std::_Ios_Fmtflags oldFlags = _output.setf( ios::left, ios::adjustfield );
        (this->*_func)( *(group.second) );
        _output.flags(oldFlags);
    }

    void
    SRVN::GroupInput::print( const DOM::Group& group ) const
    {
    }

    /* -------------------------------------------------------------------- */
    /* Tasks                                                                */
    /* -------------------------------------------------------------------- */

    void
    SRVN::TaskOutput::operator()( const pair<unsigned, DOM::Entity *>& ep) const
    {
        const DOM::Task * task = dynamic_cast<const DOM::Task *>(ep.second);
        if ( !task ) return;

        std::_Ios_Fmtflags oldFlags = _output.setf( ios::left, ios::adjustfield );
        (this->*_func)( *task );
        _output.flags(oldFlags);
    }


    void
    SRVN::TaskOutput::printParameters( const DOM::Task& t ) const
    {
        const std::_Ios_Fmtflags oldFlags = _output.setf( ios::left, ios::adjustfield );
        SRVN::EntityOutput::printParameters( t );
        const DOM::Processor * processor = t.getProcessor();
        _output << setw(__maxStrLen-1) << ( processor ? processor->getName() : "--");
	if ( __task_has_group ) {
	    const DOM::Group * group = t.getGroup();
	    _output << ' ' << setw(__maxStrLen-1) << ( group ? group->getName() : "--");
	}
        _output << ' ' << setw(3) << t.getPriority() << ' ';
        if ( __task_has_think_time ) {
            _output << setw(__maxDblLen-1);
            if ( t.getSchedulingType() == SCHEDULE_CUSTOMER ) {
                _output << *t.getThinkTime();
            } else {
                _output << " ";
            }
            _output << ' ';
        }

        const std::vector<DOM::Entry *> & entries = t.getEntryList();
        std::vector<DOM::Entry *>::const_iterator nextEntry = entries.begin();
        string s;
        for ( ;; ) {
            s += (*nextEntry)->getName();
            ++nextEntry;
            if ( nextEntry != entries.end() ) {
                s += ", ";
            } else {
                break;
            }
        }
        _output << setw(__maxStrLen) << s;

        const std::map<std::string,DOM::Activity*>& activities = t.getActivities();
        if ( activities.size() > 0 ) {
            std::map<std::string,DOM::Activity*>::const_iterator nextActivity  = activities.begin();
            s = " : ";
            for ( ;; ) {
                const DOM::Activity * activity = nextActivity->second;
                s += activity->getName();
                ++nextActivity;
                if ( nextActivity != activities.end() ) {
                    s += ", ";
                } else {
                    break;
                }
            }
            _output << setw(__maxStrLen) << s;
        }
        _output << newline;
        _output.flags(oldFlags);
    }

    /* ---------- Results ---------- */

    SRVN::ResultsManip SRVN::TaskOutput::entry_utilization( const DOM::Entry& e )    { return SRVN::ResultsManip( &SRVN::ObjectOutput::phaseResults, e, &DOM::Phase::getResultUtilization, &DOM::Entry::getResultPhasePUtilization, true ); }
    SRVN::TaskResultsManip SRVN::TaskOutput::task_utilization( const DOM::Task& t )  { return SRVN::TaskResultsManip( &SRVN::ObjectOutput::taskPhaseResults, t, &DOM::Task::getResultPhasePUtilization, true ); }
    SRVN::ResultsConfidenceManip SRVN::TaskOutput::entry_utilization_confidence( const DOM::Entry& e, const ConfidenceIntervals * c )   { return SRVN::ResultsConfidenceManip( &SRVN::ObjectOutput::phaseResultsConfidence, e, &DOM::Phase::getResultUtilizationVariance, &DOM::Entry::getResultPhasePUtilizationVariance, c, true ); }
    SRVN::TaskResultsConfidenceManip SRVN::TaskOutput::task_utilization_confidence( const DOM::Task& t, const ConfidenceIntervals * c ) { return SRVN::TaskResultsConfidenceManip( &SRVN::ObjectOutput::taskPhaseResultsConfidence, t, &DOM::Task::getResultPhasePUtilizationVariance, c, true ); }

    void
    SRVN::TaskOutput::printThroughputAndUtilization( const DOM::Task& task ) const
    {
        const std::vector<DOM::Entry *> & entries = task.getEntryList();
        std::vector<DOM::Entry *>::const_iterator nextEntry;
        bool print_task_name = true;
        unsigned item_count = 0;
        for ( nextEntry = entries.begin(); nextEntry != entries.end(); ++nextEntry, ++item_count ) {
            const DOM::Entry * entry = *nextEntry;
	    const DOM::ExternalVariable * var = task.getCopies();
	    double m;
	    var->getValue( m );
	    const double entry_util = (!task.isInfinite() && task.getSchedulingType() != SCHEDULE_CUSTOMER) ? entry->getResultUtilization() / m : 0;
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
	    const double task_util = (!task.isInfinite() && task.getSchedulingType() != SCHEDULE_CUSTOMER) ? task.getResultUtilization() / static_cast<double>(task.getCopiesValue()) : 0;
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
	const LQIO::DOM::SemaphoreTask * semaphore = dynamic_cast<const LQIO::DOM::SemaphoreTask *>(&task);
	if ( !semaphore ) return;

        const std::vector<DOM::Entry *>& entries = task.getEntryList();
	LQIO::DOM::Entry * wait_entry;
	LQIO::DOM::Entry * signal_entry;
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
	const LQIO::DOM::RWLockTask * rwlock = dynamic_cast<const LQIO::DOM::RWLockTask *>(&task);
	if ( !rwlock ) return;

        const std::vector<DOM::Entry *>& entries = task.getEntryList();
	LQIO::DOM::Entry * r_lock_entry;
	LQIO::DOM::Entry * r_unlock_entry;
	LQIO::DOM::Entry * w_lock_entry;
	LQIO::DOM::Entry * w_unlock_entry;
	
	for (int i=0;i<4;i++){
	    switch (entries[i]->getRWLockFlag()) {
	    case RWLOCK_R_UNLOCK: r_unlock_entry=entries[i]; break;
	    case RWLOCK_R_LOCK:	  r_lock_entry=entries[i]; break;
	    case RWLOCK_W_UNLOCK: w_unlock_entry=entries[i]; break;
	    case RWLOCK_W_LOCK:	  w_lock_entry=entries[i]; break;
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

        std::_Ios_Fmtflags oldFlags = _output.setf( ios::left, ios::adjustfield );
        (this->*_func)( *task );
        _output.flags(oldFlags);
    }


    void
    SRVN::TaskInput::print( const DOM::Task& t ) const
    {
	_output << "  t " << t.getName()
		<< " " << scheduling_of( t )
		<< entries_of( t )
		<< queue_length_of( t )
		<< " " << t.getProcessor()->getName()
		<< priority_of( t )
		<< think_time_of( t )
		<< copies_of( t )
		<< replicas_of( t )
		<< group_of( t );
	_output << endl;

	for ( std::map<const std::string, unsigned int>::const_iterator fi = t.getFanIns().begin(); fi != t.getFanIns().end(); ++fi ) {
	    if ( fi->second > 1 ) {	/* task name, fan in */
		_output << "  i " << fi->first << " " << t.getName() << " " << fi->second << endl;
	    }
	}
	for ( std::map<const std::string, unsigned int>::const_iterator fo = t.getFanOuts().begin(); fo != t.getFanOuts().end(); ++fo ) {
	    if ( fo->second > 1 ) {
		_output << "  o " << t.getName() << " " << fo->first << " " << fo->second << endl;
	    }
	}
    }

    ostream&
    SRVN::TaskInput::printScheduling( ostream& output, const DOM::Task & task )
    { 
	switch ( task.getSchedulingType() ) {
	case SCHEDULE_BURST:     output << 'b'; break;
	case SCHEDULE_CUSTOMER:  output << 'r'; break;
	case SCHEDULE_DELAY:     output << 'n'; break;
	case SCHEDULE_FIFO:	 output << 'n'; break;
	case SCHEDULE_HOL:	 output << 'h'; break;
	case SCHEDULE_POLL:      output << 'P'; break;
	case SCHEDULE_PPR:	 output << 'p'; break;
	case SCHEDULE_RWLOCK:    output << 'W'; break;
	case SCHEDULE_SEMAPHORE: 
	case SCHEDULE_SEMAPHORE_R: 
	    if ( dynamic_cast<const DOM::SemaphoreTask&>(task).getInitialState() == LQIO::DOM::SemaphoreTask::INITIALLY_EMPTY ) {
		output << 'Z'; break;
	    } else {
		output << 'S'; break;
	    }
	    break;
	case SCHEDULE_UNIFORM:   output << 'u'; break;
	default:	   	 output << '?'; break;
	}
	return output;
    }

    ostream& 
    SRVN::TaskInput::printEntryList( ostream& output,  const DOM::Task& task ) 
    { 
	const std::vector<DOM::Entry *> & entries = task.getEntryList();
	for ( std::vector<DOM::Entry *>::const_iterator nextEntry = entries.begin(); nextEntry != entries.end(); ++nextEntry ) {
	    const DOM::Entry * entry = *nextEntry;
	    output << " " << entry->getName();
	}
	output << " -1";
	return output; 
    }

    ostream&
    SRVN::TaskInput::printCopies( ostream& output, const DOM::Task & task ) 
    { 
	if ( task.getSchedulingType() == SCHEDULE_DELAY ) { 
	    output << " i ";
	} else {
	    const LQIO::DOM::ExternalVariable * m = task.getCopies();
	    double v;
	    if ( m != 0 ) {
		if ( m->wasSet() && m->getValue(v) ) {
		    if ( v > 1.0 ) {
			output << " m " << v;
		    }	
		} else {
		    output << " m " << *m;
		}
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
	if ( task.getPriority() || task.getProcessor()->hasPriorityScheduling() ) {
	    output << " " << task.getPriority();
	}
	return output; 
    }

    ostream& 
    SRVN::TaskInput::printQueueLength( ostream& output,  const DOM::Task& task ) 
    { 
	if ( task.hasQueueLength() ) {
	    output << " q " << *task.getQueueLength();
	}
	return output; 
    }

    ostream& 
    SRVN::TaskInput::printThinkTime( ostream& output,  const DOM::Task & task ) 
    { 
	if ( task.getSchedulingType() == SCHEDULE_CUSTOMER && task.hasThinkTime() ) {
	    output << " z " << *task.getThinkTime();
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
	}

	_output << "-1" << endl;
    }

    /* -------------------------------------------------------------------- */
    /* Entries                                                              */
    /* -------------------------------------------------------------------- */

    SRVN::PhaseManip SRVN::EntryOutput::coefficient_of_variation( const DOM::Entry& e ) { return SRVN::PhaseManip( &SRVN::ObjectOutput::phaseInfo, e, &DOM::Phase::getCoeffOfVariationSquared ); }
    SRVN::ResultsManip SRVN::EntryOutput::max_service_time( const DOM::Entry& e )       { return SRVN::ResultsManip( &SRVN::ObjectOutput::phaseResults, e, &DOM::Phase::getMaxServiceTime ); }
    SRVN::EntryManip SRVN::EntryOutput::open_arrivals( const DOM::Entry& e )            { return SRVN::EntryManip( &SRVN::ObjectOutput::entryInfo, e, &DOM::Entry::getOpenArrivalRate ); }
    SRVN::PhaseTypeManip SRVN::EntryOutput::phase_type( const DOM::Entry& e )           { return SRVN::PhaseTypeManip( &SRVN::ObjectOutput::phaseTypeInfo, e, &DOM::Phase::getPhaseTypeFlag ); }
    SRVN::PhaseManip SRVN::EntryOutput::service_demand( const DOM::Entry& e )           { return SRVN::PhaseManip( &SRVN::ObjectOutput::phaseInfo, e, &DOM::Phase::getServiceTime ); }
    SRVN::PhaseManip SRVN::EntryOutput::think_time( const DOM::Entry& e )               { return SRVN::PhaseManip( &SRVN::ObjectOutput::phaseInfo, e, &DOM::Phase::getThinkTime ); }

    void
    SRVN::EntryOutput::operator()( const pair<unsigned, DOM::Entity *>& ep) const
    {
        const DOM::Task * task = dynamic_cast<const DOM::Task *>(ep.second);
        if ( !task ) return;

        std::_Ios_Fmtflags oldFlags = _output.setf( ios::left, ios::adjustfield );
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
        if ( forwarding.size() > 0 ) {
            std::vector<DOM::Call*>::const_iterator nextCall;
            for (  nextCall = forwarding.begin(); nextCall != forwarding.end(); ++nextCall ) {
                const DOM::Call * call = *nextCall;
                const DOM::Entry * dest = call->getDestinationEntry();
                _output << entity_name( entity, print_task_name )
                        << entry_name( entry )
                        << entry_name( *dest )
                        << setw(__maxDblLen) << *call->getCallMean()
                        << newline;
            }
        }
    }

    void
    SRVN::EntryOutput::printOpenArrivals( const DOM::Entry & entry, const DOM::Entity & entity, bool& print_task_name  ) const
    {
        _output << entity_name( entity, print_task_name ) << entry_name( entry ) << open_arrivals( entry ) << newline;
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
	const LQIO::DOM::ExternalVariable * var = (activity.*func)();
        _output << setw(__maxStrLen) << " " << setw(__maxStrLen) << activity.getName() << setw(__maxDblLen);
	if ( var ) {
	    _output << *var ;
	} else {
	    _output << 0.0;
	}
	_output << newline;
    }

    /* ---- Entry Results ---- */

    SRVN::ResultsManip SRVN::EntryOutput::service_time( const DOM::Entry& e )           { return SRVN::ResultsManip( &SRVN::ObjectOutput::phaseResults, e, &DOM::Phase::getResultServiceTime, &DOM::Entry::getResultPhasePServiceTime, false ); }
    SRVN::ResultsManip SRVN::EntryOutput::service_time_exceeded( const DOM::Entry& e )  { return SRVN::ResultsManip( &SRVN::ObjectOutput::phaseResults, e, &DOM::Phase::getResultMaxServiceTimeExceeded, 0 ); }
    SRVN::ResultsManip SRVN::EntryOutput::variance_service_time( const DOM::Entry& e )  { return SRVN::ResultsManip( &SRVN::ObjectOutput::phaseResults, e, &DOM::Phase::getResultVarianceServiceTime, &DOM::Entry::getResultPhasePVarianceServiceTime, true ); }

    SRVN::ResultsConfidenceManip SRVN::EntryOutput::service_time_confidence( const DOM::Entry& e, const ConfidenceIntervals * c )           { return SRVN::ResultsConfidenceManip( &SRVN::ObjectOutput::phaseResultsConfidence, e, &DOM::Phase::getResultServiceTimeVariance, &DOM::Entry::getResultPhasePServiceTimeVariance, c); }
    SRVN::ResultsConfidenceManip SRVN::EntryOutput::service_time_exceeded_confidence( const DOM::Entry& e, const ConfidenceIntervals * c )  { return SRVN::ResultsConfidenceManip( &SRVN::ObjectOutput::phaseResultsConfidence, e, &DOM::Phase::getResultMaxServiceTimeExceededVariance, 0, c); }
    SRVN::ResultsConfidenceManip SRVN::EntryOutput::variance_service_time_confidence( const DOM::Entry& e, const ConfidenceIntervals * c )  { return SRVN::ResultsConfidenceManip( &SRVN::ObjectOutput::phaseResultsConfidence, e, &DOM::Phase::getResultVarianceServiceTimeVariance, &DOM::Entry::getResultPhasePVarianceServiceTimeVariance, c); }

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
                << setw(__maxDblLen-1) << entry.getResultOpenWaitTime() << newline;
        if ( __conf95 ) {
            _output << conf_level( __maxStrLen * 2, ConfidenceIntervals::CONF_95 )
                    << setw(__maxDblLen) << " "		/* Input parameter, so ignore it */
                    << setw(__maxDblLen-1) << (*__conf95)(entry.getResultOpenWaitTimeVariance()) << newline;
        }
        if ( __conf99 ) {
            _output << conf_level( __maxStrLen * 2, ConfidenceIntervals::CONF_99 )
                    << setw(__maxDblLen) << " "
                    << setw(__maxDblLen-1) << (*__conf99)(entry.getResultOpenWaitTimeVariance()) << newline;
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
	if ( entry.hasOpenArrivalRate() ) {
	    _output << "  a " << entry.getName() << " " << entry.getOpenArrivalRateValue() << endl;
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
	    const std::map<unsigned, DOM::Phase*>& p = entry.getPhaseList();

	    _output << "  s " << setw( ObjectInput::__maxEntLen ) << entry.getName();
	    for_each( p.begin(), p.end(), PhaseInput( _output, &PhaseInput::printServiceTime ) );
	    _output << " -1" << endl;

	    if ( entry.hasNonExponentialPhases() ) {
		_output << "  c " << setw( ObjectInput::__maxEntLen ) << entry.getName();
		for_each( p.begin(), p.end(), PhaseInput( _output, &PhaseInput::printCoefficientOfVariation ) );
		_output << " -1" << endl;
	    }
	    if ( entry.hasThinkTime() ) {
		_output << "  Z " << setw( ObjectInput::__maxEntLen ) << entry.getName();
		for_each( p.begin(), p.end(), PhaseInput( _output, &PhaseInput::printThinkTime ) );
		_output << " -1" << endl;
	    }
	    if ( entry.hasDeterministicPhases() ) {
		_output << "  f " << setw( ObjectInput::__maxEntLen ) << entry.getName();
		for_each( p.begin(), p.end(), PhaseInput( _output, &PhaseInput::printPhaseFlag ) );
		_output << " -1" << endl;
	    }
	    if ( entry.hasMaxServiceTimeExceeded() ) {
		_output << "  M " << setw( ObjectInput::__maxEntLen ) << entry.getName();
		for_each( p.begin(), p.end(), PhaseInput( _output, &PhaseInput::printMaxServiceTimeExceeded ) );
		_output << " -1" << endl;
	    }
	    if ( entry.hasHistogram() ) {
		/* Histograms are stored by phase for regular entries.  Activity entries don't have phases...  Punt... */
		for ( std::map<unsigned, DOM::Phase*>::const_iterator np = p.begin(); np != p.end();  ++np ) {
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
	    _output << "  F " << setw( ObjectInput::__maxEntLen ) << entry.getName() 
		    << " " << setw( ObjectInput::__maxEntLen ) << fwd->getDestinationEntry()->getName() << number_of_calls( *fwd ) << " -1" << endl;
	}
    }

    void SRVN::EntryInput::printCalls( const DOM::Entry& entry ) const 
    { 
	/* Gather up all the call info over all phases and store in new map<to_entry,call*[3]>. */

	std::map<const DOM::Entry *, ForPhase> callsByPhase;
	const std::map<unsigned, DOM::Phase*>& phases = entry.getPhaseList();
	for (std::map<unsigned, DOM::Phase*>::const_iterator p = phases.begin(); p != phases.end(); ++p) {
	    const DOM::Phase* phase = p->second;
	    const unsigned index = p->first;
	    const std::vector<DOM::Call *>& calls = phase->getCalls();
	    for ( std::vector<DOM::Call*>::const_iterator nextCall = calls.begin(); nextCall != calls.end(); ++nextCall ) {
		const DOM::Call * call = *nextCall;
		const DOM::Entry * dest = call->getDestinationEntry();
		ForPhase& y = callsByPhase[dest];
		y.setType( call->getCallType() ); 
		y[index] = call;
	    }
	}

	/* Now iterate over the collection of calls */

	for ( std::map<const DOM::Entry *, ForPhase>::const_iterator next_y = callsByPhase.begin(); next_y != callsByPhase.end(); ++next_y ) {
	    const ForPhase& callInfo = next_y->second;
	    _output << "  ";
	    switch ( callInfo.getType() ) {
	    case DOM::Call::RENDEZVOUS: _output << "y"; break;
	    case DOM::Call::SEND_NO_REPLY: _output << "z"; break;
	    default: abort();
	    }
	    _output << " " << setw( ObjectInput::__maxEntLen ) << entry.getName() 
		    << " " << setw( ObjectInput::__maxEntLen ) << (next_y->first)->getName();
	    for (std::map<unsigned, DOM::Phase*>::const_iterator p = phases.begin(); p != phases.end(); ++p) {
		const DOM::Call * call = callInfo[p->first];
		if ( call ) {
		    _output << number_of_calls( *call );
		} else {
		    _output << " 0";
		}
	    }
	    _output << " -1" << endl;
        }
    }

    void
    SRVN::PhaseInput::operator()( const std::pair<unsigned,DOM::Phase *>& p ) const
    {
        std::_Ios_Fmtflags oldFlags = _output.setf( ios::left, ios::adjustfield );
	(this->*_func)( *(p.second) );
        _output.flags(oldFlags);
    }

    void SRVN::PhaseInput::printCoefficientOfVariation( const DOM::Phase& p ) const { _output << " " << setw(ObjectInput::__maxInpLen) << p.getCoeffOfVariationSquaredValue(); }
    void SRVN::PhaseInput::printMaxServiceTimeExceeded( const DOM::Phase& p ) const { _output << " " << setw(ObjectInput::__maxInpLen) << p.getMaxServiceTime(); }
    void SRVN::PhaseInput::printPhaseFlag( const DOM::Phase& p ) const    	    { _output << " " << setw(ObjectInput::__maxInpLen) << (p.hasDeterministicCalls() ? "1" : "0"); }

    void SRVN::PhaseInput::printServiceTime( const DOM::Phase& p ) const
    { 
	const DOM::ExternalVariable* var = p.getServiceTime();
	double val = 0;
	_output << " " << setw(ObjectInput::__maxInpLen);
	if ( !var ) {
	    _output << "0";
	} else if ( var->wasSet() && var->getValue( val ) ) {
	    _output << val;
	} else {
	    _output << *var;
	}
    }

    void SRVN::PhaseInput::printThinkTime( const DOM::Phase& p ) const
    { 
	const DOM::ExternalVariable* var = p.getThinkTime();
	double val = 0;
	_output << " " << setw(ObjectInput::__maxInpLen);
	if ( !var ) {
	    _output << "0";
	} else if ( var->wasSet() && var->getValue( val ) ) {
	    _output << val;
	} else {
	    _output << *var;
	}
    }

    void
    SRVN::ActivityInput::operator()( const std::pair<std::string,DOM::Activity *>& a ) const
    {
        std::_Ios_Fmtflags oldFlags = _output.setf( ios::left, ios::adjustfield );
	(this->*_func)( *(a.second) );
        _output.flags(oldFlags);
    }

    void
    SRVN::ActivityInput::print( const DOM::Activity& activity ) const
    {
	_output << "  s " << activity.getName();
	printServiceTime( activity );
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
	printCalls( activity );
    }

    void
    SRVN::ActivityInput::printCalls( const DOM::Activity& activity ) const
    {
	const std::vector<DOM::Call *>& calls = activity.getCalls();
	for ( std::vector<DOM::Call*>::const_iterator nextCall = calls.begin(); nextCall != calls.end(); ++nextCall ) {
	    const DOM::Call * call = *nextCall;
	    _output << "  " << call_type( *call ) << " " << activity.getName() << " " << call->getDestinationEntry()->getName() << number_of_calls( *call ) << endl;
	}
    }

    void
    SRVN::ActivityListInput::operator()( const DOM::ActivityList * precedence ) const
    {
        std::_Ios_Fmtflags oldFlags = _output.setf( ios::left, ios::adjustfield );
	(this->*_func)( *precedence );
        _output.flags(oldFlags);
    }

    void
    SRVN::ActivityListInput::print( const DOM::ActivityList& precedence ) const
    {
	switch ( precedence.getListType() ) {
	case DOM::ActivityList::JOIN_ACTIVITY_LIST:
	case DOM::ActivityList::AND_JOIN_ACTIVITY_LIST:
	case DOM::ActivityList::OR_JOIN_ACTIVITY_LIST:
	    _output << " ";
	    printPreList( precedence );
	    if ( precedence.getNext() ) {
		_output << " -> ";
		printPostList( *precedence.getNext() );
		const_cast<SRVN::ActivityListInput *>(this)->_count += 2;
	    } else {
		const_cast<SRVN::ActivityListInput *>(this)->_count += 1;
	    }
	    if ( _count < _size ) {
		_output << ";";
	    }
	    _output << endl;
	    break;
	default: break;
	}
    }

    void
    SRVN::ActivityListInput::printPreList( const DOM::ActivityList& precedence ) const		/* joins */
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
		_output << "[";
		for ( std::vector<DOM::Entry *>::const_iterator next_entry = replies.begin(); next_entry != replies.end(); ++next_entry ) {
		    if ( next_entry != replies.begin() ) {
			_output << ",";
		    }
		    _output << (*next_entry)->getName();
		}
		_output << "]";
	    }
	}
    }

    void
    SRVN::ActivityListInput::printPostList( const DOM::ActivityList& precedence ) const		/* forks */
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
     */

    void
    SRVN::CallOutput::operator()( const pair<unsigned, DOM::Entity *>& ep) const
    {
        const DOM::Task * task = dynamic_cast<const DOM::Task *>(ep.second);
        if ( !task ) return;

        std::_Ios_Fmtflags oldFlags = _output.setf( ios::left, ios::adjustfield );
        const std::vector<DOM::Entry *>& entries = task->getEntryList();
        std::vector<DOM::Entry *>::const_iterator nextEntry;
        bool print_task_name = true;
        for ( nextEntry = entries.begin(); nextEntry != entries.end(); ++nextEntry ) {
            const DOM::Entry * entry = *nextEntry;

            /* Gather up all the call info over all phases and store in new map<to_entry,call*[3]>. */
            std::map<const DOM::Entry *, ForPhase> callsByPhase;
            const std::map<unsigned, DOM::Phase*>& phases = entry->getPhaseList();
            for (std::map<unsigned, DOM::Phase*>::const_iterator p = phases.begin(); p != phases.end(); ++p) {
                const DOM::Phase* phase = p->second;
                const unsigned index = p->first;
                const std::vector<DOM::Call *>& calls = phase->getCalls();
                for ( std::vector<DOM::Call*>::const_iterator nextCall = calls.begin(); nextCall != calls.end(); ++nextCall ) {
                    const DOM::Call * call = *nextCall;
                    if ( call && (call->*_testFunc)() ) {
                        const DOM::Entry * dest = call->getDestinationEntry();
                        ForPhase& y = callsByPhase[dest];
                        y[index] = call;
                    }
                }
            }

            /* Now iterate over the collection of calls */
            for ( std::map<const DOM::Entry *, ForPhase>::const_iterator next_y = callsByPhase.begin(); next_y != callsByPhase.end(); ++next_y ) {
                const ForPhase& callInfo = next_y->second;
		const_cast<ForPhase *>(&callInfo)->setMaxPhase( __parseable ? __maxPhase : entry->getMaximumPhase() );
                _output << entity_name( *(ep.second), print_task_name )
                        << entry_name( *entry ) 
                        << entry_name( *(next_y->first) )
                        << print_calls( *this, callInfo, _meanFunc) << newline;
                if ( _confFunc && __conf95 ) {
                    _output << conf_level( __maxStrLen*3, ConfidenceIntervals::CONF_95 )
                            << print_calls( *this, callInfo, _confFunc, __conf95 ) << newline;
                }
                if ( _confFunc && __conf99 ) {
                    _output << conf_level( __maxStrLen*3, ConfidenceIntervals::CONF_99 )
                            << print_calls( *this, callInfo, _confFunc, __conf99 ) << newline;
                }
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
                    const DOM::Entry * dest = call->getDestinationEntry();
                    if ( call && (call->*_testFunc)() ) {
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
                    }
                }
            }
	    if ( __parseable && count > 1 ) {
		_output << setw(__maxStrLen) << " " << activityEOF << newline;
	    }
        }
	if ( __parseable && print_task_name == false ) {
	    _output << setw(__maxStrLen) << " " << activityEOF << newline;
	}
        _output.flags(oldFlags);
    }


    void
    SRVN::CallOutput::printCallRate( const DOM::Call * call, const ConfidenceIntervals* ) const
    {
        const DOM::ExternalVariable * value = 0;
        if ( call && (value = call->getCallMean()) != 0 ) {
            _output << to_double( *value );
        } else {
            _output << 0.0;
        }
    }

    void
    SRVN::CallOutput::printCallWaiting( const DOM::Call * call, const ConfidenceIntervals* ) const
    {
        double value = 0;
        if ( call && (value = call->getResultWaitingTime()) != 0 ) {
            _output << value;
        } else {
            _output << 0.0;
        }
    }

    void
    SRVN::CallOutput::printCallWaitingConfidence( const DOM::Call * call, const ConfidenceIntervals* conf ) const
    {
        double value = 0;
        if ( call && conf && (value = call->getResultWaitingTimeVariance()) != 0 ) {
            _output << (*conf)(value); // __number_of_blocks
        } else {
            _output << 0.0;
        }
    }

    void
    SRVN::CallOutput::printCallVarianceWaiting( const DOM::Call * call, const ConfidenceIntervals* ) const
    {
        double value = 0;
        _output << setw(__maxDblLen);
        if ( call && (value = call->getResultVarianceWaitingTime()) != 0 ) {
            _output << value;
        } else {
            _output << 0.0;
        }
    }

    void
    SRVN::CallOutput::printCallVarianceWaitingConfidence( const DOM::Call * call, const ConfidenceIntervals* conf ) const
    {
        double value = 0;
        _output << setw(__maxDblLen);
        if ( call && conf && (value = call->getResultVarianceWaitingTimeVariance()) != 0 ) {
            _output << (*conf)(value);
        } else {
            _output << 0.0;
        }
    }

    void
    SRVN::CallOutput::printDropProbability( const DOM::Call * call, const ConfidenceIntervals* ) const
    {
        double value = 0.0;
        if ( call && (value = call->getResultDropProbability()) != 0 ) {
            _output << value;
        } else {
            _output << 0.0;
        }
    }

    void
    SRVN::CallOutput::printDropProbabilityConfidence( const DOM::Call * call, const ConfidenceIntervals* conf ) const
    {
        double value = 0;
        _output << setw(__maxDblLen);
        if ( call && conf && (value = call->getResultDropProbabilityVariance()) != 0 ) {
            _output << (*conf)(value);
        } else {
            _output << 0.0;
        }
    }

    ostream&
    SRVN::CallOutput::printCalls( ostream& output, const CallOutput& info, const ForPhase& phases, const callConfFPtr func, const ConfidenceIntervals* conf )
    {
        for ( unsigned p = 1; p <= phases.getMaxPhase(); ++p ) {
            output << setw(__maxDblLen-1);
            (info.*func)( phases[p], conf );
            output << " ";
        }
	output << activityEOF;
        return output;
    }

    SRVN::ForPhase::ForPhase()
	: _maxPhase(DOM::Phase::MAX_PHASE), _type(DOM::Call::NULL_CALL)
    {
        for ( unsigned p = 0; p < DOM::Phase::MAX_PHASE; ++p ) {
            ia[p] = 0;
        }
    }

    SRVN::CallOutput::CallResultsManip SRVN::CallOutput::print_calls( const SRVN::CallOutput& c, const SRVN::ForPhase& p, const callConfFPtr x, const ConfidenceIntervals* l )
    {
        return CallResultsManip( &SRVN::CallOutput::printCalls, c, p, x, l );
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

	    for ( unsigned int p = 1; p <= DOM::Phase::MAX_PHASE; ++p ) {		/* BUG_668 */
		if ( entry->hasHistogramForPhase( p ) ) {
		    _output << "Service time histogram for entry " << entry->getName() << ", phase " << p << newline;
		    (this->*_func)( *entry->getHistogramForPhase( p ) );
		}
	    }

            const std::map<unsigned, DOM::Phase*>& phases = entry->getPhaseList();
            for (std::map<unsigned, DOM::Phase*>::const_iterator p = phases.begin(); p != phases.end(); ++p) {
                const DOM::Phase* phase = p->second;

		if ( phase->hasHistogram() ) {
		    _output << "Service time histogram for entry " << entry->getName() << ", phase " << p->first << newline;
		    (this->*_func)( *phase->getHistogram() );
		}

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
