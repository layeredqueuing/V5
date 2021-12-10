/* help.cc	-- Greg Franks Wed Oct 12 2005
 *
 * $Id: option.cc 15194 2021-12-10 12:01:01Z greg $
 */

#include "lqns.h"
#include <fstream>
#include <sstream>
#include <errno.h>
#include <ctype.h>
#include <cstdlib>
#include <lqio/error.h>
#include <lqio/dom_document.h>
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
    { "layers",     Debug( &Debug::layers,      &Help::debugLayers ) },
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
std::vector<bool> Options::Debug::_bits(Options::Debug::QUORUM+1);

void
Options::Debug::initialize()
{
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
//    _bits[CALLS]      = true;
    _bits[FORKS]      = true;
    _bits[INTERLOCK]  = true;
//    _bits[JOINS]      = true;
    _bits[LAYERS]     = true;
#if HAVE_LIBGSL
    _bits[QUORUM]     = true;
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
Options::Debug::exec( const int ix, const std::string& arg )
{
    (*__table.at(std::string(__options.at(ix))).func())( arg );
}

std::map<const std::string, const Options::Trace> Options::Trace::__table =
{
    { "activities",    Trace( &Trace::activities,     false, &Help::traceActivities ) },
    { "convergence",   Trace( &Trace::convergence,    true,  &Help::traceConvergence ) },
    { "delta_wait",    Trace( &Trace::delta_wait,     false, &Help::traceDeltaWait ) },
    { "forks",	       Trace( &Trace::forks,          false, &Help::traceForks ) },
    { "idle-time",     Trace( &Trace::idle_time,      false, &Help::traceIdleTime ) },
    { "interlock",     Trace( &Trace::interlock,      false, &Help::traceInterlock ) },
    { "intermediate",  Trace( &Trace::intermediate,   false, &Help::traceIntermediate ) },
    { "joins",	       Trace( &Trace::joins,          false, &Help::traceJoins ) },
    { "mva",	       Trace( &Trace::mva,            true,  &Help::traceMva ) },
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

void
Options::Trace::initialize()
{
    if ( __options.empty() ) {
	/* Populate for getsubopt */
	for ( std::map<const std::string, const Options::Trace>::const_iterator next_opt = __table.begin(); next_opt != __table.end(); ++next_opt ) {
	    __options.push_back( const_cast<char *>(next_opt->first.data()) );
	}
	__options.push_back( nullptr );
    }
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
    flags.trace_delta_wait = true;
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
    unsigned temp;
    flags.trace_mva = true;
    if ( arg.empty() ) {
	flags.trace_submodel = 0;	/* Do all submodels */
    } else if ( 0 < ( temp = (unsigned)strtol( arg.c_str(), 0, 10 ) ) && temp < 100 ) {
	flags.trace_submodel = temp;
    } else {
	std::cerr << LQIO::io_vars.lq_toolname << " -tmva=" << arg << " is invalid." << std::endl;
    }
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
    { "iteration-limit",                        Special( &Special::iteration_limit,             true,  &Help::specialIterationLimit ) },
    { "print-interval",                         Special( &Special::print_interval,              true,  &Help::specialPrintInterval ) },
    { "overtaking",                             Special( &Special::overtaking,                  false, &Help::specialOvertaking ) },
    { "convergence-value",                      Special( &Special::convergence_value,           true,  &Help::specialConvergenceValue ) },
    { "single-step",                            Special( &Special::single_step,                 false, &Help::specialSingleStep ) },
    { "underrelaxation",                        Special( &Special::underrelaxation,             true,  &Help::specialUnderrelaxation ) },
    { "generate",                               Special( &Special::generate_queueing_model,     true,  &Help::specialGenerateQueueingModel ) },
    { LQIO::DOM::Pragma::_mol_underrelaxation_, Special( &Special::mol_ms_underrelaxation,      true,  &Help::specialMolMSUnderrelaxation ) },
    { "man",                                    Special( &Special::make_man,                    true,  &Help::specialMakeMan ) },
    { "tex",                                    Special( &Special::make_tex,                    true,  &Help::specialMakeTex ) },
    { "min-steps",                              Special( &Special::min_steps,                   true,  &Help::specialMinSteps ) },
#if HAVE_LIBGSL
    { "ignore-overhanging-threads",             Special( &Special::ignore_overhanging_threads,  false, &Help::specialIgnoreOverhangingThreads ) },
#endif
    { "full-reinitialize",                      Special( &Special::full_reinitialize,           false, &Help::specialFullReinitialize ) }
};

std::vector<char *> Options::Special::__options;


void
Options::Special::initialize()
{
    if ( __options.empty() ) {
	for ( std::map<const std::string, const Options::Special>::const_iterator next_opt = __table.begin(); next_opt != __table.end(); ++next_opt ) {
	    __options.push_back( const_cast<char *>(next_opt->first.c_str()) );
	}
	__options.push_back( nullptr );
    }
}



void
Options::Special::iteration_limit( const std::string& arg )
{
    if ( arg.empty() || (Model::__iteration_limit = (unsigned)strtol( arg.c_str(), 0, 10 )) == 0 ) {
	std::cerr << LQIO::io_vars.lq_toolname << "iteration-limit=" << arg << " is invalid, choose non-negative integer." << std::endl;
	(void) exit( INVALID_ARGUMENT );
    }
}

void
Options::Special::print_interval( const std::string& arg )
{
    if ( arg.empty() && (Model::__print_interval = (unsigned)strtol( arg.c_str(), 0, 10 )) == 0 ) {
	std::cerr << LQIO::io_vars.lq_toolname << "print-interval=" << arg << " is invalid, choose non-negative integer." << std::endl;
	(void) exit( INVALID_ARGUMENT );
    } else {
	flags.trace_intermediate = true;
    }
}

void
Options::Special::overtaking( const std::string& )
{
    flags.print_overtaking = true;
}

void
Options::Special::convergence_value( const std::string& arg )
{
    if ( arg.empty() || (Model::__convergence_value = strtod( arg.c_str(), 0 )) == 0 ) {
	std::cerr << LQIO::io_vars.lq_toolname << "convergence=" << arg << " is invalid, choose non-negative real." << std::endl;
	(void) exit( INVALID_ARGUMENT );
    }
}

void
Options::Special::single_step( const std::string& arg )
{
    if ( arg.empty() ) {
	flags.single_step = true;
    } else if ( (flags.single_step = atol( arg.c_str() )) <= 0 ) {
	std::cerr << LQIO::io_vars.lq_toolname << ": step=" << arg << " is invalid, choose non-negative integer." << std::endl;
	(void) exit( INVALID_ARGUMENT );
    }
}

void
Options::Special::underrelaxation( const std::string& arg )
{
    if ( arg.empty() || (Model::__underrelaxation = strtod( arg.c_str(), 0 )) <= 0.0 || 2.0 < Model::__underrelaxation ) {
	std::cerr << LQIO::io_vars.lq_toolname << "underrelaxation=" << arg << " is invalid, choose a value between 0.0 and 2.0." << std::endl;
	(void) exit( INVALID_ARGUMENT );
    }
}

void
Options::Special::generate_queueing_model( const std::string& arg )
{
    if ( arg.empty() ) {
	std::cerr << LQIO::io_vars.lq_toolname << "generate: missing filename argument.." << std::endl;
	(void) exit( INVALID_ARGUMENT );
    } else {
	flags.generate = true;
	Generate::file_name = const_cast<char *>(arg.c_str());
    }
}

void
Options::Special::mol_ms_underrelaxation( const std::string& arg )
{
    try {
	pragmas.insert(LQIO::DOM::Pragma::_mol_underrelaxation_,arg);
    }
    catch ( std::domain_error& e ) {
	std::cerr << LQIO::io_vars.lq_toolname << "underrelaxation=" << arg << " is invalid, choose real between 0.0 and 1.0." << std::endl;
	(void) exit( INVALID_ARGUMENT );
    }
}

void
Options::Special::make_man( const std::string& arg )
{
    HelpTroff man;
    if ( !arg.empty() ) {
	std::ofstream output;
	output.open( arg, std::ios::out );	/* NO \r's in output for windoze */
	if ( !output ) {
	    std::ostringstream msg; 
	    msg << "Cannot open output file " << arg << " - " << strerror( errno );
	    throw std::runtime_error( msg.str() );
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
	    std::ostringstream msg; 
	    msg << "Cannot open output file " << arg << " - " << strerror( errno );
	    throw std::runtime_error( msg.str() );
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
    if ( arg.empty() ) {
	std::cerr << LQIO::io_vars.lq_toolname << ": no value supplied to -zmin-steps." << std::endl;
    } else if ( (flags.min_steps = atoi( arg.c_str() )) < 1 ) {
	std::cerr << LQIO::io_vars.lq_toolname << ": min-steps=" << arg << " is invalid, choose value greater than 1." << std::endl;
	(void) exit( INVALID_ARGUMENT );
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
Options::Special::full_reinitialize( const std::string& )
{
    flags.full_reinitialize = true;
}

void
Options::Special::exec( const int ix, const std::string& arg )
{
    (*__table.at(std::string(__options.at(ix))).func())( arg );
}
