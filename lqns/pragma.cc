/*  -*- c++ -*-
 * $Id: pragma.cc 11963 2014-04-10 14:36:42Z greg $ *
 * Pragma processing and definitions.
 *
 * Copyright the Real-Time and Distributed Systems Group,
 * Department of Systems and Computer Engineering,
 * Carleton University, Ottawa, Ontario, Canada. K1S 5B6
 *
 * November, 1994
 * April 2011
 * ------------------------------------------------------------------------
 */

#include "dim.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <algorithm>
#include <lqio/error.h>
#if !defined(HAVE_GETSUBOPT)
#include <lqio/getsbopt.h>
#endif
#include "help.h"
#include "pragma.h"
#include "lqns.h"

Pragma pragma;

std::map<const char *, Pragma::pragma_info, lt_str> Pragma::__pragmas;
std::map<const char *, Pragma::param_info, lt_str>  Pragma::__cycles_args;
std::map<const char *, Pragma::param_info, lt_str>  Pragma::__stop_on_message_loss_args;
std::map<const char *, Pragma::param_info, lt_str>  Pragma::__interlock_args;
std::map<const char *, Pragma::param_info, lt_str>  Pragma::__layering_args;
std::map<const char *, Pragma::param_info, lt_str>  Pragma::__multiserver_args;
std::map<const char *, Pragma::param_info, lt_str>  Pragma::__mva_args;
std::map<const char *, Pragma::param_info, lt_str>  Pragma::__overtaking_args;
std::map<const char *, Pragma::param_info, lt_str>  Pragma::__processor_args;
std::map<const char *, Pragma::param_info, lt_str>  Pragma::__threads_args;
std::map<const char *, Pragma::param_info, lt_str>  Pragma::__variance_args;
std::map<const char *, Pragma::param_info, lt_str>  Pragma::__warning_args;
std::map<const char *, Pragma::param_info, lt_str>  Pragma::__xml_schema_args;
#if HAVE_LIBGSL && HAVE_LIBGSLCBLAS
std::map<const char *, Pragma::param_info, lt_str>  Pragma::__quorum_distribution_args;
std::map<const char *, Pragma::param_info, lt_str>  Pragma::__quorum_delayed_calls_args;
std::map<const char *, Pragma::param_info, lt_str>  Pragma::__idle_time_args;
#endif

/*
 * Set default values in the constructor.  Defaults are used below.
 */

Pragma::Pragma() :
    _cycles(DISALLOW_CYCLES),
    _stopOnMessageLoss(true),
    _interlock(THROUGHPUT_INTERLOCK),
    _layering(BATCHED_LAYERS),
    _multiserver(DEFAULT_MULTISERVER),
    _mva(LINEARIZER_MVA),
#if HAVE_LIBGSL && HAVE_LIBGSLCBLAS
    _quorumDistribution(DEFAULT_QUORUM_DISTRIBUTION),
    _quorumDelayedCalls(DEFAULT_QUORUM_DELAYED_CALLS),
    _idletime(DEFAULT_IDLETIME),
#endif
    _overtaking(MARKOV_OVERTAKING),
    _processor(DEFAULT_PROCESSOR),
#if RESCHEDULE
    _reschedule(NO_ASYNC_RESCHEDULE),
#endif
    _tau(8),
    _threads(MAK_LUNDSTROM_THREADS),
    _variance(DEFAULT_VARIANCE),
    /* Special variables */
    _phase2_correction(COMPLEX_PHASE2),
    _entry_variance(true),
    _init_variance_only(false),
    _exponential_paths(0),
    _severity_level(LQIO::NO_ERROR)
{
}


bool
Pragma::operator==( const Pragma& p ) const
{
    /* General pragmas */
    std::map<const char *, Pragma::pragma_info, lt_str>::const_iterator next_pragma;
    for ( next_pragma = Pragma::__pragmas.begin(); next_pragma != Pragma::__pragmas.end(); ++next_pragma  ) {
        const eq_pragma_fptr equals = next_pragma->second._eq;
        if ( equals && !(this->*(equals))( p ) ) return false;
    }

    /* Special cases */

    return eqPhase2Correction( p )
        && eqExponentialPaths( p )
        && eqEntryVariance( p )
        && eqInitVarianceOnly( p );
}


/*
 * we use maps.  They have to be initialized dynamically.  This is a singleton.
 */

void
Pragma::initialize()
{
    if ( __pragmas.size() > 0 ) return;

    __pragmas["cycles"] =               pragma_info( &Pragma::setCyclesTo, &Pragma::getCyclesStr, &Pragma::eqCycles, &Help::pragmaCycles, &__cycles_args );
    __cycles_args["allow"] =            param_info( ALLOW_CYCLES,    &Help::pragmaCyclesAllow );
    __cycles_args["disallow"] =         param_info( DISALLOW_CYCLES, &Help::pragmaCyclesDisallow );

    __pragmas["stop-on-message-loss"] = pragma_info( &Pragma::setStopOnMessageLossTo, &Pragma::getStopOnMessageLossStr, &Pragma::eqStopOnMessageLoss, &Help::pragmaStopOnMessageLoss, &__stop_on_message_loss_args );
    __stop_on_message_loss_args["false"] = param_info( false, &Help::pragmaStopOnMessageLossFalse );
    __stop_on_message_loss_args["true"] =  param_info( true,  &Help::pragmaStopOnMessageLossTrue );

    __pragmas["interlocking"] =         pragma_info( &Pragma::setInterlockTo, &Pragma::getInterlockStr, &Pragma::eqInterlock, &Help::pragmaInterlock, &__interlock_args );
    __interlock_args["throughput"] =    param_info( THROUGHPUT_INTERLOCK, &Help::pragmaInterlockThroughput );
    __interlock_args["none"] =          param_info( NO_INTERLOCK,         &Help::pragmaInterlockNone );

    __pragmas["layering"] =             pragma_info( &Pragma::setLayeringTo, &Pragma::getLayeringStr, &Pragma::eqLayering, &Help::pragmaLayering, &__layering_args );
    __layering_args["batched"] =        param_info( BATCHED_LAYERS,              &Help::pragmaLayeringBatched );
    __layering_args["batched-back"] =   param_info( BACKPROPOGATE_LAYERS,        &Help::pragmaLayeringBatchedBack );
    __layering_args["mol"] =            param_info( METHOD_OF_LAYERS,            &Help::pragmaLayeringMOL );
    __layering_args["mol-back"] =       param_info( BACKPROPOGATE_METHOD_OF_LAYERS, &Help::pragmaLayeringMOLBack );
    __layering_args["squashed"] =       param_info( SQUASHED_LAYERS,             &Help::pragmaLayeringSquashed );
    __layering_args["srvn"] =           param_info( SRVN_LAYERS,                 &Help::pragmaLayeringSRVN );
    __layering_args["hwsw"] =           param_info( HWSW_LAYERS,                 &Help::pragmaLayeringHwSw );

    __pragmas["multiserver"] =          pragma_info( &Pragma::setMultiserverTo, &Pragma::getMultiserverStr, &Pragma::eqMultiserver, &Help::pragmaMultiserver, &__multiserver_args );
    __multiserver_args["default"] =     param_info( DEFAULT_MULTISERVER,        &Help::pragmaMultiServerDefault );
    __multiserver_args["conway"] =      param_info( CONWAY_MULTISERVER,         &Help::pragmaMultiServerConway );
    __multiserver_args["reiser"]  =     param_info( REISER_MULTISERVER,         &Help::pragmaMultiServerReiser );
    __multiserver_args["reiser-ps"] =   param_info( REISER_PS_MULTISERVER,      &Help::pragmaMultiServerReiserPS );
    __multiserver_args["rolia"] =       param_info( ROLIA_MULTISERVER,          &Help::pragmaMultiServerRolia );
    __multiserver_args["rolia-ps"] =    param_info( ROLIA_PS_MULTISERVER,       &Help::pragmaMultiServerRoliaPS );
    __multiserver_args["bruell"] =      param_info( BRUELL_MULTISERVER,         &Help::pragmaMultiServerBruell );
    __multiserver_args["schmidt"] =     param_info( SCHMIDT_MULTISERVER,        &Help::pragmaMultiServerSchmidt );
    __multiserver_args["suri"] =        param_info( SURI_MULTISERVER,           &Help::pragmaMultiServerSuri );

    __pragmas["mva"] =                  pragma_info( &Pragma::setMVATo, &Pragma::getMVAStr, &Pragma::eqMVA, &Help::pragmaMVA, &__mva_args );
    __mva_args["linearizer"] =          param_info( LINEARIZER_MVA,              &Help::pragmaMVALinearizer );
    __mva_args["exact"] =               param_info( EXACT_MVA,                   &Help::pragmaMVAExact );
    __mva_args["schweitzer"] =          param_info( SCHWEITZER_MVA,              &Help::pragmaMVASchweitzer );
    __mva_args["fast"] =                param_info( FAST_MVA,                    &Help::pragmaMVAFast );
    __mva_args["one-step"] =            param_info( ONESTEP_MVA,                 &Help::pragmaMVAOneStep );
    __mva_args["one-step-linearizer"] = param_info( ONESTEP_LINEARIZER,          &Help::pragmaMVAOneStepLinearizer );

#if HAVE_LIBGSL && HAVE_LIBGSLCBLAS
    __pragmas["quorum-distribution"] =  pragma_info( &Pragma::setQuorumDistributionTo, &Pragma::getQuorumDistributionStr, &Pragma::eqQuorumDistribution, &Help::pragmaQuorumDistribution, __quorum_distribution_args );
    __pragmas["quorum-delayed-calls"] = pragma_info( &Pragma::setQuorumDelayedCallsTo, &Pragma::getQuorumDelayedCallsStr, &Pragma::eqQuorumDelayedCalls, &Help::pragmaQuorumDelayedCalls, __quorum_delayed_calls_args );
    __pragmas["idletime"] =             pragma_info( &Pragma::setIdleTimeTo, &Pragma::getIdleTimeStr, &Pragma::eqIdleTime, &Help::pragmaIdleTime, __idle_time_args );
#endif

    __pragmas["overtaking"] =           pragma_info( &Pragma::setOvertakingTo, &Pragma::getOvertakingStr, &Pragma::eqOvertaking, &Help::pragmaOvertaking, &__overtaking_args );
    __overtaking_args["markov"] =       param_info( SIMPLE_OVERTAKING,         &Help::pragmaOvertakingMarkov );
    __overtaking_args["rolia"] =        param_info( MARKOV_OVERTAKING,         &Help::pragmaOvertakingRolia );
    __overtaking_args["simple"] =       param_info( ROLIA_OVERTAKING,          &Help::pragmaOvertakingSimple );
    __overtaking_args["special"] =      param_info( SPECIAL_OVERTAKING,        &Help::pragmaOvertakingSpecial );
    __overtaking_args["none"] =         param_info( NO_OVERTAKING,             &Help::pragmaOvertakingNone );

    __pragmas["processor"] =            pragma_info( &Pragma::setProcessorTo, &Pragma::getProcessorStr, &Pragma::eqProcessor, &Help::pragmaProcessor, &__processor_args );
    __processor_args["default"] =       param_info( DEFAULT_PROCESSOR,        &Help::pragmaProcessorDefault );
    __processor_args["fcfs"] =          param_info( PROCESSOR_FCFS,           &Help::pragmaProcessorFCFS );
    __processor_args["hol"] =           param_info( PROCESSOR_HOL,            &Help::pragmaProcessorHOL );
    __processor_args["ppr"] =           param_info( PROCESSOR_PR,             &Help::pragmaProcessorPPR );
    __processor_args["ps"] =            param_info( PROCESSOR_PS,             &Help::pragmaProcessorPS );

#if RESCHEDULE
    __pragmas["reschedule-on-async-send"] = pragma_info( &Pragma::setRescheduleTo, &Pragma::getRescheduleStr, &Pragma::eqReschedule, &Help::pragmaReschedule );
#else
    __pragmas["reschedule-on-async-send"] = pragma_info();
#endif
    __pragmas["tau"] =                  pragma_info( &Pragma::setTauTo, &Pragma::getTauStr, &Pragma::eqTau, &Help::pragmaTau );

    __pragmas["threads"] =              pragma_info( &Pragma::setThreadsTo, &Pragma::getThreadsStr, &Pragma::eqThreads, &Help::pragmaThreads, &__threads_args );
    __threads_args["hyper"] =           param_info( MAK_LUNDSTROM_THREADS,  &Help::pragmaThreadsHyper );
    __threads_args["mak"] =             param_info( HYPER_THREADS,          &Help::pragmaThreadsMak );
    __threads_args["none"] =            param_info( EXPONENTIAL_THREADS,    &Help::pragmaThreadsNone );
    __threads_args["exponential"] =     param_info( NO_THREADS,             &Help::pragmaThreadsExponential );

    __pragmas["variance"] =             pragma_info( &Pragma::setVarianceTo, &Pragma::getVarianceStr, &Pragma::eqVariance, &Help::pragmaVariance, &__variance_args );
    __variance_args["default"] =        param_info( DEFAULT_VARIANCE,        &Help::pragmaVarianceDefault );
    __variance_args["none"] =           param_info( NO_VARIANCE,             &Help::pragmaVarianceNone );
    __variance_args["stochastic"] =     param_info( STOCHASTIC_VARIANCE,     &Help::pragmaVarianceStochastic );
    __variance_args["mol"] =            param_info( MOL_VARIANCE,            &Help::pragmaVarianceMol );
    __variance_args["no-entry"] =       param_info( ENTRY_VARIANCE,          &Help::pragmaVarianceNoEntry );
    __variance_args["init-only"] =      param_info( INIT_VARIANCE_ONLY,      &Help::pragmaVarianceInitOnly);

    __pragmas["severity-level"] =       pragma_info( &Pragma::setSeverityLevelTo, &Pragma::getSeverityLevelStr, &Pragma::eqSeverityLevel, &Help::pragmaSeverityLevel, &__warning_args );
    __warning_args["all"] =             param_info( LQIO::NO_ERROR, 	 &Help::pragmaSeverityLevelWarnings );
    __warning_args["warning"] =         param_info( LQIO::WARNING_ONLY,  &Help::pragmaSeverityLevelWarnings );
    __warning_args["advisory"] =        param_info( LQIO::ADVISORY_ONLY, &Help::pragmaSeverityLevelRunTime);
    __warning_args["run-time"] =        param_info( LQIO::RUNTIME_ERROR, &Help::pragmaSeverityLevelRunTime);
}				        

/* ------------------------------------------------------------------------ */

const char *
Pragma::getCyclesStr() const
{
    std::map<const char *, Pragma::param_info, lt_str>::const_iterator p = find_if( __cycles_args.begin(), __cycles_args.end(), eq_pragma_value( static_cast<int>(_cycles) ) );
    if ( p != __cycles_args.end() ) return p->first;
    return 0;
}


bool
Pragma::setCyclesTo( const string& aStr )
{
    std::map<const char *, Pragma::param_info, lt_str>::const_iterator p = __cycles_args.find( aStr.c_str() );
    if ( p == __cycles_args.end() ) return false;

    setCycles( static_cast<PRAGMA_CYCLES>(p->second._i) );
    return true;
}

/* ------------------------------------------------------------------------ */

const char *
Pragma::getStopOnMessageLossStr() const
{
    std::map<const char *, Pragma::param_info, lt_str>::const_iterator p = find_if( __stop_on_message_loss_args.begin(), __stop_on_message_loss_args.end(), eq_pragma_value( static_cast<int>(_stopOnMessageLoss) ) );
    if ( p != __stop_on_message_loss_args.end() ) return p->first;
    return 0;
}


bool
Pragma::setStopOnMessageLossTo( const string& aStr )
{
    std::map<const char *, Pragma::param_info, lt_str>::const_iterator p = __stop_on_message_loss_args.find( aStr.c_str() );
    if ( p == __stop_on_message_loss_args.end() ) return false;

    setStopOnMessageLoss( static_cast<bool>(p->second._i) );
    return true;
}

/* ------------------------------------------------------------------------ */

const char *
Pragma::getInterlockStr() const
{
    std::map<const char *, Pragma::param_info, lt_str>::const_iterator p = find_if( __interlock_args.begin(), __interlock_args.end(), eq_pragma_value( static_cast<int>(_interlock) ) );
    if ( p != __interlock_args.end() ) return p->first;
    return 0;
}


bool
Pragma::setInterlockTo( const string& aStr )
{
    std::map<const char *, Pragma::param_info, lt_str>::const_iterator p = __interlock_args.find( aStr.c_str() );
    if ( p == __interlock_args.end() ) return false;

    setInterlock( static_cast<PRAGMA_INTERLOCK>(p->second._i) );
    return true;
}

/* ------------------------------------------------------------------------ */

const char *
Pragma::getLayeringStr() const
{
    std::map<const char *, Pragma::param_info, lt_str>::const_iterator p = find_if( __layering_args.begin(), __layering_args.end(), eq_pragma_value( static_cast<int>(_layering) ) );
    if ( p != __layering_args.end() ) return p->first;
    return 0;
}


bool
Pragma::setLayeringTo( const string& aStr )
{
    std::map<const char *, Pragma::param_info, lt_str>::const_iterator p = __layering_args.find( aStr.c_str() );
    if ( p == __layering_args.end() ) return false;

    setLayering( static_cast<PRAGMA_LAYERING>(p->second._i) );
    return true;
}

/* ------------------------------------------------------------------------ */

const char *
Pragma::getMultiserverStr() const
{
    std::map<const char *, Pragma::param_info, lt_str>::const_iterator p = find_if( __multiserver_args.begin(), __multiserver_args.end(), eq_pragma_value( static_cast<int>(_multiserver) ) );
    if ( p != __multiserver_args.end() ) return p->first;
    return 0;
}


bool
Pragma::setMultiserverTo( const string& aStr )
{
    std::map<const char *, Pragma::param_info, lt_str>::const_iterator p = __multiserver_args.find( aStr.c_str() );
    if ( p == __multiserver_args.end() ) return false;

    setMultiserver( static_cast<PRAGMA_MULTISERVER>(p->second._i) );
    return true;
}

/* ------------------------------------------------------------------------ */

const char *
Pragma::getMVAStr() const
{
    std::map<const char *, Pragma::param_info, lt_str>::const_iterator p = find_if( __mva_args.begin(), __mva_args.end(), eq_pragma_value( static_cast<int>(_mva) ) );
    if ( p != __mva_args.end() ) return p->first;
    return 0;
}


bool
Pragma::setMVATo( const string& aStr )
{
    std::map<const char *, Pragma::param_info, lt_str>::const_iterator p = __mva_args.find( aStr.c_str() );
    if ( p == __mva_args.end() ) return false;

    setMVA( static_cast<PRAGMA_MVA>(p->second._i) );
    return true;
}

/* ------------------------------------------------------------------------ */

const char *
Pragma::getOvertakingStr() const
{
    std::map<const char *, Pragma::param_info, lt_str>::const_iterator p = find_if( __overtaking_args.begin(), __overtaking_args.end(), eq_pragma_value( static_cast<int>(_overtaking) ) );
    if ( p != __overtaking_args.end() ) return p->first;
    return 0;
}


bool
Pragma::setOvertakingTo( const string& aStr )
{
    std::map<const char *, Pragma::param_info, lt_str>::const_iterator p = __overtaking_args.find( aStr.c_str() );
    if ( p == __overtaking_args.end() ) return false;

    if ( static_cast<PRAGMA_OVERTAKING>(p->second._i) == SIMPLE_OVERTAKING ) {
	_phase2_correction = SIMPLE_PHASE2;
	phase2_correction = SIMPLE_PHASE2;		/* Global var. */
    } else {
	setOvertaking( static_cast<PRAGMA_OVERTAKING>(p->second._i) );
    }
    return true;
}

/* ------------------------------------------------------------------------ */

const char *
Pragma::getProcessorStr() const
{
    std::map<const char *, Pragma::param_info, lt_str>::const_iterator p = find_if( __processor_args.begin(), __processor_args.end(), eq_pragma_value( static_cast<int>(_processor) ) );
    if ( p != __processor_args.end() ) return p->first;
    return 0;
}


bool
Pragma::setProcessorTo( const string& aStr )
{
    std::map<const char *, Pragma::param_info, lt_str>::const_iterator p = __processor_args.find( aStr.c_str() );
    if ( p == __processor_args.end() ) return false;

    setProcessor( static_cast<PRAGMA_PROCESSOR>(p->second._i) );
    return true;
}

const scheduling_type Pragma::processor_scheduling[] = { SCHEDULE_CUSTOMER, SCHEDULE_FIFO, SCHEDULE_HOL, SCHEDULE_PPR, SCHEDULE_PS, SCHEDULE_PS_HOL, SCHEDULE_PS_PPR };

scheduling_type
Pragma::getProcessorScheduling() const
{
    return processor_scheduling[static_cast<unsigned>(_processor)];
}



#if RESCHEDULE
const char *
Pragma::getRescheduleStr() const
{
    return 0;
}


bool
Pragma::setRescheduleTo( const string& aStr )
{
    if ( is_true( aStr ) ) {
	setReschedule( ASYNC_RESCHEDULE );
    } else {
	setReschedule( NO_ASYNC_RESCHEDULE );
    }
    return true;
}
#endif

/* ------------------------------------------------------------------------ */

const char *
Pragma::getTauStr() const
{
    return _tau_str.c_str();
}


bool
Pragma::setTauTo( const string& aStr )
{
    unsigned int i;
    if ( sscanf( aStr.c_str(), "%d", &i ) != 1 || i > 20 ) {
	return false;
    } else {
	_tau_str = aStr;
	setTau( i );
	return true;
    }
}

/* ------------------------------------------------------------------------ */

const char *
Pragma::getThreadsStr() const
{
    std::map<const char *, Pragma::param_info, lt_str>::const_iterator p = find_if( __threads_args.begin(), __threads_args.end(), eq_pragma_value( static_cast<int>(_threads) ) );
    if ( p != __threads_args.end() ) return p->first;
    return 0;
}


bool
Pragma::setThreadsTo( const string& aStr )
{
    std::map<const char *, Pragma::param_info, lt_str>::const_iterator p = __threads_args.find( aStr.c_str() );
    if ( p == __threads_args.end() ) return false;
    if ( static_cast<PRAGMA_THREADS>(p->second._i) == EXPONENTIAL_THREADS ) {
	_exponential_paths = 1;
    } else {
	setThreads( static_cast<PRAGMA_THREADS>(p->second._i) );
    }
    return true;
}

/* ------------------------------------------------------------------------ */

const char *
Pragma::getVarianceStr() const
{
    std::map<const char *, Pragma::param_info, lt_str>::const_iterator p = find_if( __variance_args.begin(), __variance_args.end(), eq_pragma_value( static_cast<int>(_variance) ) );
    if ( p != __variance_args.end() ) return p->first;
    return 0;
}


bool
Pragma::setVarianceTo( const string& aStr )
{
    std::map<const char *, Pragma::param_info, lt_str>::const_iterator p = __variance_args.find( aStr.c_str() );
    if ( p == __variance_args.end() ) return false;
    switch ( static_cast<PRAGMA_VARIANCE>(p->second._i) ) {
    case ENTRY_VARIANCE:
	_entry_variance = true;
	break;

    case INIT_VARIANCE_ONLY:
	_init_variance_only = true;
	break;

    default:
	setVariance( static_cast<PRAGMA_VARIANCE>(p->second._i) );
	break;
    }
    return true;
}

/* ------------------------------------------------------------------------ */

const char *
Pragma::getSeverityLevelStr() const
{
    std::map<const char *, Pragma::param_info, lt_str>::const_iterator p = find_if( __warning_args.begin(), __warning_args.end(), eq_pragma_value( static_cast<int>(_stopOnMessageLoss) ) );
    if ( p != __warning_args.end() ) return p->first;
    return 0;
}


bool
Pragma::setSeverityLevelTo( const string& aStr )
{
    std::map<const char *, Pragma::param_info, lt_str>::const_iterator p = __warning_args.find( aStr.c_str() );
    if ( p == __warning_args.end() ) return false;

    setSeverityLevel( static_cast<LQIO::severity_t>(p->second._i) );
    return true;
}

Pragma&
Pragma::setSeverityLevel( const LQIO::severity_t severity_level )
{
    io_vars.severity_level = severity_level;
    _severity_level = severity_level;
    return *this;
}

/*
 * Convert s to scheduling type.
 */

bool
Pragma::is_true( const string& s )
{
    return strcasecmp( s.c_str(), "true" ) == 0
	|| strcasecmp( s.c_str(), "yes" ) == 0;
}

bool
Pragma::set( const string& param, const string& value )
{
    initialize();

    std::map<const char *, Pragma::pragma_info, lt_str>::const_iterator p = Pragma::__pragmas.find( param.c_str() );

    if ( p == Pragma::__pragmas.end() ) return false;
    Pragma::set_pragma_fptr set = p->second._set;
    if ( !set ) return true;		/* ignored pragma */
    return (this->*set)( value );
}


/*
 * Update the DOM to current state. 
 */

void
Pragma::updateDOM( LQIO::DOM::Document* document ) const
{
    Pragma pragma_default;

    initialize();

    // Reset DOM __pragmas 
    document->clearPragmaList();

    /* General cases */
    std::map<const char *, Pragma::pragma_info, lt_str>::const_iterator param;
    for ( param = Pragma::__pragmas.begin(); param != Pragma::__pragmas.end(); ++param  ) {
	const pragma_info& value = param->second;
	if ( !value._eq || (this->*(value._eq))( pragma_default ) ) continue;		// No change 

	const char * p = (this->*(value._get))();
	if ( p ) {
	    document->addPragma( param->first, p );
	}
    }

    /* Special cases */
    if ( !eqPhase2Correction( pragma_default ) ) {
	document->addPragma( "overtaking", "special" );
    }
    if ( !eqExponentialPaths( pragma_default ) ) {
	document->addPragma( "threads", "exponential" );
    }
    if ( !eqEntryVariance( pragma_default ) ) {
	document->addPragma( "variance", "no-entry" );
    }
    if ( !eqInitVarianceOnly( pragma_default ) ) {
	document->addPragma( "variance", "init-only" );
    }

}


/*
 * Print out available pragmas.
 */

ostream&
Pragma::usage( ostream& output )
{
    Pragma::initialize();
    output << "Valid pragmas: " << endl;
    ios_base::fmtflags flags = output.setf( ios::left, ios::adjustfield );

    for ( std::map<const char *, Pragma::pragma_info>::const_iterator p = Pragma::__pragmas.begin(); p != Pragma::__pragmas.end(); ++p  ) {
	output << "\t" << setw(20) << p->first;

	const std::map<const char *, param_info, lt_str>* args = p->second._value;
	if ( args && args->size() > 1 ) {
	    output << " = {";

	    std::map<const char *, param_info, lt_str>::const_iterator q;
	    for ( q = args->begin(); q != args->end(); ++q ) {
		if ( q != args->begin() ) output << ",";
		output << q->first;
	    }
	    output << "}" << endl;
	} else {
	    output << " = <arg>" << endl;
	}
    }
    output.setf( flags );
    return output;
}
