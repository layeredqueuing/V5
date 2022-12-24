/* help.cc	-- Greg Franks Wed Oct 12 2005
 *
 * $Id: option.cc 16194 2022-12-23 03:22:28Z greg $
 */

#include "lqns.h"
#include <fstream>
#include <sstream>
#include <cctype>
#include <cstdlib>
#include <errno.h>
#include <lqio/error.h>
#include <lqio/srvn_spex.h>
#include <lqio/dom_document.h>
#include <lqio/dom_pragma.h>
#include "generate.h"
#include "help.h"
#include "flags.h"
#include "model.h"
#include "option.h"
#include "pragma.h"

const std::map<const std::string, const Options::Debug> Options::Debug::__table
{
    { "all",        Debug( &Debug::all,         &Help::debugAll ) },
//    { "activities", Debug( &Debug::activities,  &Help::debugActivities ) },
//    { "calls",      Debug( &Debug::calls,       &Help::debugCalls ) },
    { "forks",      Debug( &Debug::forks,       &Help::debugForks ) },
    { "interlock",  Debug( &Debug::interlock,   &Help::debugInterlock ) },
//    { "joins",      Debug( &Debug::joins,       &Help::debugJoins ) },
    { "submodels",  Debug( &Debug::submodels,   &Help::debugSubmodels ) },
#if DEBUG_MVA
    { "mva",	    Debug( &Debug::mva,		&Help::debugMVA ) },
#endif
    { "overtaking", Debug( &Debug::overtaking,  &Help::debugOvertaking ) },
    { "variance",   Debug( &Debug::variance,    &Help::debugVariance ) },
#if HAVE_LIBGSL
    { "quorum",     Debug( &Debug::quorum,      &Help::debugQuorum ) },
#endif
    { "xml",        Debug( &Debug::xml,         &Help::debugXML ) },
    { "lqx",        Debug( &Debug::lqx,         &Help::debugLQX ) },
};

std::vector<char *> Options::Debug::__options;
unsigned long Options::Debug::__submodels = 0x00L;
std::vector<bool> Options::Debug::__bits(Options::Debug::QUORUM+1);

void
Options::Debug::initialize()
{
    __submodels = 0x00;
    if ( __options.empty() ) {
	/* Populate for getsubopt */
	for ( std::map<const std::string, const Options::Debug>::const_iterator next_opt = __table.begin(); next_opt != __table.end(); ++next_opt ) {
	    __options.push_back( const_cast<char *>(next_opt->first.c_str()) );
	}
	__options.push_back( nullptr );
    }
}

void
Options::Debug::all( const std::string& )
{
    __submodels = ~0;
//    _bits[CALLS]      = true;
    __bits[FORKS]      = true;
    __bits[INTERLOCK]  = true;
//    _bits[JOINS]      = true;
#if HAVE_LIBGSL
    __bits[QUORUM]     = true;
#endif
}

void
Options::Debug::mva( const std::string& )
{
#if DEBUG_MVA
    MVA::debug_D = false;			/* Linearizer */
    MVA::debug_L = true;			/* Queue Length */
    MVA::debug_N = false;			/* Customers */
    MVA::debug_P = false;			/* Marginal Probability */
    MVA::debug_U = false;			/* Utilization */
    MVA::debug_W = true;			/* Waiting Time */
    MVA::debug_X = false;			/* Throughput */
#endif
}

void
Options::Debug::overtaking( const std::string& )
{
    flags.print_overtaking = true;
}

void
Options::Debug::xml( const std::string& )
{
    LQIO::DOM::Document::__debugXML = true;
}

void
Options::Debug::lqx( const std::string& )
{
    LQIO::DOM::Document::lqx_parser_trace(stderr);
}

void
Options::Debug::submodels( const std::string& arg )
{
    if ( arg.empty() ) {
	__submodels = ~0;	/* Do all submodels */
    } else {
	char * endptr = nullptr;
	__submodels = static_cast<unsigned long>(strtol( arg.c_str(), &endptr, 10 ) );
	if ( endptr != nullptr && *endptr != '\0' ) {
	    throw std::invalid_argument( std::string( "submodels=<n> where n=\"" ) + arg + "\". Choose an unsigned integer bitset." ) ;
	}
    }
}


bool
Options::Debug::submodels( unsigned long submodel )
{
    return (submodel == 0 && __submodels != 0) || (((1 << (submodel - 1)) & __submodels) != 0);
}


void
Options::Debug::exec( const int ix, const std::string& arg )
{
    (*__table.at(std::string(__options.at(ix))).func())( arg );
}

std::map<const std::string, const Options::Trace> Options::Trace::__table =
{
    { "activities",    Trace( &Trace::activities,     false, &Help::traceActivities ) },
    { "convergence",   Trace( &Trace::convergence,    true,  &Help::traceConvergence ) },
    { "delta-wait",    Trace( &Trace::delta_wait,     false, &Help::traceDeltaWait ) },
    { "forks",	       Trace( &Trace::forks,          false, &Help::traceForks ) },
    { "idle-time",     Trace( &Trace::idle_time,      false, &Help::traceIdleTime ) },
    { "interlock",     Trace( &Trace::interlock,      false, &Help::traceInterlock ) },
    { "intermediate",  Trace( &Trace::intermediate,   false, &Help::traceIntermediate ) },
    { "joins",	       Trace( &Trace::joins,          false, &Help::traceJoins ) },
    { "mva",	       Trace( &Trace::mva,            true,  &Help::traceMVA ) },
    { "overtaking",    Trace( &Trace::overtaking,     false, &Help::traceOvertaking ) },
    { "quorum",        Trace( &Trace::quorum,         false, &Help::traceQuorum ) },
    { "replication",   Trace( &Trace::replication,    false, &Help::traceReplication ) },
    { "throughput",    Trace( &Trace::throughput,     false, &Help::traceThroughput ) },
    { "variance",      Trace( &Trace::variance,       false, &Help::traceVariance ) },
    { "virtual-entry", Trace( &Trace::virtual_entry,  false, &Help::traceVirtualEntry ) },
    { "wait",	       Trace( &Trace::wait,           false, &Help::traceWait ) }
//  { "processor",     Trace( &Trace::processor,      true,  &Help::traceProcessor ) },
//  { "task",          Trace( &Trace::task,	      true,  &Help::traceTask ) },
};

std::vector<char *> Options::Trace::__options;
unsigned long Options::Trace::__delta_wait = 0x00L;
unsigned long Options::Trace::__mva = 0x00L;
bool Options::Trace::__verbose = false;

void
Options::Trace::initialize()
{
    __verbose = false;
    __mva = 0x0L;

    if ( !__options.empty() ) return;
    /* Populate for getsubopt */
    for ( std::map<const std::string, const Options::Trace>::const_iterator next_opt = __table.begin(); next_opt != __table.end(); ++next_opt ) {
	__options.push_back( const_cast<char *>(next_opt->first.data()) );
    }
    __options.push_back( nullptr );
}


void
Options::Trace::activities( const std::string& arg )
{
    flags.trace_activities = true;
}

void
Options::Trace::convergence( const std::string& arg )
{
    flags.trace_convergence = true;
}

void
Options::Trace::delta_wait( const std::string& arg )
{
    if ( arg.empty() ) {
	__delta_wait = ~0;	/* Do all submodels */
    } else {
	char * endptr = nullptr;
	__delta_wait = static_cast<unsigned long>(strtol( arg.c_str(), &endptr, 10 ) );
	if ( endptr != nullptr && *endptr != '\0' ) {
	    throw std::invalid_argument( std::string( "delta-wait=<n> where n=\"" ) + arg + "\". Choose an unsigned integer bitset." );
	}
    }
}

bool
Options::Trace::delta_wait( unsigned long submodel )
{
    return (submodel == 0 && __delta_wait != 0) || (((1 << (submodel - 1)) & __delta_wait) != 0);
}


void
Options::Trace::forks( const std::string& arg )
{
    flags.trace_forks = true;
}

void
Options::Trace::idle_time( const std::string& arg )
{
    flags.trace_idle_time = true;
}

void
Options::Trace::interlock( const std::string& arg )
{
    flags.trace_interlock = true;
}

void
Options::Trace::intermediate( const std::string& arg )
{
    flags.trace_intermediate = true;
}

void
Options::Trace::joins( const std::string& arg )
{
    flags.trace_joins = true;
}


void
Options::Trace::mva( const std::string& arg )
{
    if ( arg.empty() ) {
	__mva = ~0;	/* Do all submodels */
    } else {
	char * endptr = nullptr;
	__mva = static_cast<unsigned long>(strtol( arg.c_str(), &endptr, 10 ) );
	if ( endptr != nullptr && *endptr != '\0' ) {
	    throw std::invalid_argument( std::string( "mva=<n> where n=\"" ) + arg + "\". Choose an unsigned integer bitset." );
	}
    }
}

bool
Options::Trace::mva( unsigned long submodel )
{
    return (submodel == 0 && __mva != 0) || (((1 << (submodel - 1)) & __mva) != 0);
}


void
Options::Trace::quorum( const std::string& arg )
{
    flags.trace_quorum = true;
}

void
Options::Trace::overtaking( const std::string& arg )
{
    flags.trace_overtaking = true;
    flags.print_overtaking = true;
}

void
Options::Trace::replication( const std::string& arg )
{
    flags.trace_replication = true;
}

void
Options::Trace::throughput( const std::string& arg )
{
    flags.trace_throughput = true;
}

void
Options::Trace::variance( const std::string& arg )
{
    flags.trace_variance = true;
}

void
Options::Trace::verbose( const std::string& arg )
{
    __verbose = LQIO::DOM::Pragma::isTrue( arg );
    LQIO::Spex::__verbose = __verbose;
}

void
Options::Trace::virtual_entry( const std::string& arg )
{
    flags.trace_virtual_entry = true;
    flags.trace_wait = true;
}

void
Options::Trace::wait( const std::string& arg )
{
    flags.trace_wait = true;
}

void
Options::Trace::exec( const int ix, const std::string& arg )
{
    (*__table.at(std::string(__options.at(ix))).func())( arg );
}

std::map<const std::string, const Options::Special> Options::Special::__table =
{
    { "full-reinitialize",                      Special( &Special::full_reinitialize,           false, &Help::specialFullReinitialize ) },
    { "generate",                               Special( &Special::generate_queueing_model,     true,  &Help::specialGenerateQueueingModel ) },
#if HAVE_LIBGSL
    { "ignore-overhanging-threads",             Special( &Special::ignore_overhanging_threads,  false, &Help::specialIgnoreOverhangingThreads ) },
#endif
    { "man",                                    Special( &Special::make_man,                    true,  &Help::specialMakeMan ) },
    { "min-steps",                              Special( &Special::min_steps,                   true,  &Help::specialMinSteps ) },
    { "print-interval",                         Special( &Special::print_interval,              true,  &Help::specialPrintInterval ) },
    { "overtaking",                             Special( &Special::overtaking,                  false, &Help::specialOvertaking ) },
    { "single-step",                            Special( &Special::single_step,                 false, &Help::specialSingleStep ) },
    { "tex",                                    Special( &Special::make_tex,                    true,  &Help::specialMakeTex ) },
};

std::vector<char *> Options::Special::__options;

void
Options::Special::initialize()
{
    if ( !__options.empty() ) return;
    for ( std::map<const std::string, const Options::Special>::const_iterator next_opt = __table.begin(); next_opt != __table.end(); ++next_opt ) {
	__options.push_back( const_cast<char *>(next_opt->first.c_str()) );
    }
    __options.push_back( nullptr );
}

void
Options::Special::exec( const int ix, const std::string& arg )
{
    (*__table.at(std::string(__options.at(ix))).func())( arg );
}


void
Options::Special::full_reinitialize( const std::string& )
{
    flags.full_reinitialize = true;
}


void
Options::Special::generate_queueing_model( const std::string& arg )
{
    flags.generate = true;
    if ( arg.empty() ) {
	Generate::__directory_name = "debug";
    } else {
	Generate::__directory_name = arg;
    }
}

#if HAVE_LIBGSL
void
Options::Special::ignore_overhanging_threads( const std::string& )
{
    flags.ignore_overhanging_threads = true;
}
#endif

void
Options::Special::make_man( const std::string& arg )
{
    HelpTroff man;
    if ( !arg.empty() ) {
	std::ofstream output;
	output.open( arg, std::ios::out );	/* NO \r's in output for windoze */
	if ( !output ) {
	    throw std::runtime_error( std::string( "Cannot open output file " ) + arg + " - " + strerror( errno ) );
	} else {
	    output << man;
	}
	output.close();
    } else {
	std::cout << man;
    }
    exit( 0 );
}

void
Options::Special::make_tex( const std::string& arg )
{
    HelpLaTeX man;
    if ( !arg.empty() ) {
	std::ofstream output;
	output.open( arg, std::ios::out );	/* NO \r's in output for windoze */
	if ( !output ) {
	    throw std::runtime_error( std::string( "Cannot open output file " ) + arg + " - " + strerror( errno ) );
	} else {
	    output << man;
	}
	output.close();
    } else {
	std::cout << man;
    }
    exit( 0 );
}

void
Options::Special::min_steps( const std::string& arg )
{
    char * endptr = nullptr;
    if ( arg.empty() || (flags.min_steps = (unsigned)strtol( arg.c_str(), &endptr, 10 )) == 0 || *endptr != '\0' ) {
	throw std::runtime_error( std::string( "min-steps=" ) + arg + ", choose an integer greater than 0." );
    }
}

void
Options::Special::overtaking( const std::string& )
{
    flags.print_overtaking = true;
}

void
Options::Special::print_interval( const std::string& arg )
{
    char * endptr = nullptr;
    if ( !arg.empty() && (Model::__print_interval = (unsigned)strtol( arg.c_str(), &endptr, 10 )) == 0 || *endptr != '\0' ) {
	throw std::invalid_argument( std::string( "print-interval=<n> where n=\"" ) + arg + "\".  Choose an integer greater than 0." );
    }
    flags.trace_intermediate = true;
}

void
Options::Special::single_step( const std::string& arg )
{
    char * endptr = nullptr;
    if ( arg.empty() ) {
	flags.single_step = 1;
    } else if ( (flags.single_step = (unsigned)strtol( arg.c_str(), &endptr, 10 )) == 0 || *endptr != '\0' ) {
	throw std::invalid_argument( std::string( "step=<n> where n=\"" ) + arg + "\".  Choose a non-negative integer." );
    }
}
