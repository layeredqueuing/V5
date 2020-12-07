/* help.cc	-- Greg Franks Wed Oct 12 2005
 *
 * $Id: help.cc 14174 2020-12-07 16:59:53Z greg $
 */

#include <config.h>
#include <iostream>
#include <iomanip>
#include "dim.h"
#include <ctype.h>
#include <cstdlib>
#include <lqio/error.h>
#include <lqio/dom_document.h>
#include "lqns.h"
#include "help.h"
#include "option.h"
#include "pragma.h"

class HelpManip {
public:
    HelpManip( std::ostream& (*ff)(std::ostream&, const int c ), const int c )
	: _c(c), f(ff)  {}
private:
    const int _c;
    std::ostream& (*f)( std::ostream&, const int c );

    friend std::ostream& operator<<(std::ostream & os, const HelpManip& m )
	{ return m.f(os,m._c); }
};

Help::pragma_map_t Help::__pragmas;

Help::parameter_map_t  Help::__cycles_args;
Help::parameter_map_t  Help::__force_multiserver_args;
Help::parameter_map_t  Help::__interlock_args;
Help::parameter_map_t  Help::__layering_args;
Help::parameter_map_t  Help::__multiserver_args;
Help::parameter_map_t  Help::__mva_args;
Help::parameter_map_t  Help::__overtaking_args;
#if RESCHEDULE
Help::parameter_map_t  Help::__reschedule_args;
#endif
Help::parameter_map_t  Help::__processor_args;
Help::parameter_map_t  Help::__prune_args;
Help::parameter_map_t  Help::__spex_header_args;
Help::parameter_map_t  Help::__stop_on_message_loss_args;
Help::parameter_map_t  Help::__threads_args;
Help::parameter_map_t  Help::__variance_args;
Help::parameter_map_t  Help::__warning_args;
#if HAVE_LIBGSL && HAVE_LIBGSLCBLAS
Help::parameter_map_t  Help::__quorum_distribution_args;
Help::parameter_map_t  Help::__quorum_delayed_calls_args;
Help::parameter_map_t  Help::__idle_time_args;
#endif


/* -------------------------------------------------------------------- */
/* Help/Usage info.							*/
/* -------------------------------------------------------------------- */

/*
 * A useful message to the Luser.
 */

void
usage ( const char * optarg )
{
    if ( !optarg ) {
	std::cerr << "Usage: " << LQIO::io_vars.lq_toolname;

	std::cerr << " [option] [file ...]" << std::endl << std::endl;
	std::cerr << "Options" << std::endl;
#if HAVE_GETOPT_LONG
	const char ** p = opthelp;
	for ( const struct option *o = longopts; (o->name || o->val) && *p; ++o, ++p ) {
	    std::string s;
	    if ( o->name ) {
		s = "--";
		s += o->name;
		switch ( o->val ) {
		case 'G': s += "=ARG"; break;
		case 'H': s += "=[dztP]"; break;
		case 'I': s += "=ARG"; break;
		case 'P': s += "=<pragma>"; break;
		case 'd': s += "=<debug>"; break;
		case 'e': s += "=[adiw]"; break;
		case 'o': s += "=FILE"; break;
		case 't': s += "=<trace>"; break;
		case 'z': s += "=<special>"; break;

		case (256+'c'):
		case (256+'i'):
		case (256+'k'):
		case (256+'u'):
		    s += "=<n>";
		    break;
		}
	    } else {
		s = " ";
	    }
	    if ( isascii(o->val) && isgraph(o->val) ) {
		std::cerr << " -" << static_cast<char>(o->val) << ", ";
	    } else {
	        std::cerr << "     ";
	    }
	    std::cerr.setf( std::ios::left, std::ios::adjustfield );
	    std::cerr << std::setw(28) << s << *p << std::endl;
	}
#else
	for ( const char * o = opts; *o && *p; ++o, ++p ) {
	    string s;
	    s = "-";
	    s += *o;
	    if ( *(o+1) == ':' ) {
		switch ( *o ) {
		default:  s += "<file>"; break;
		case 'H': s += "[dztP]"; break;
		case 'd': s += "<debug>"; break;
		case 'e': s += "[adiw]"; break;
		case 't': s += "<trace>"; break;
		case 'z': s += "<special>"; break;
		case 'P': s += "<pragma>"; break;
		}
		++o;	/* Skip ':' */
	    }
	    cerr.setf( ios::left, ios::adjustfield );
	    std::cerr << setw(14) << s << *p << std::endl;
	}
#endif
    } else {
	switch ( optarg[0] ) {
	case 'd':
	case 't':
	case 'z':
	    usage( optarg[0] );
	    break;
	case 'P':
	    Pragma::usage( std::cerr );
	    break;
	default:
	    std::cerr << "Invalid argument to -H --help: " << optarg << std::endl;
	    std::cerr << "-Hd -- debug options; -Ht -- trace options; -Hz -- special options; -HP -- pragmas" << std::endl;
	    break;
	}
    }

    (void) exit( INVALID_ARGUMENT );
}



/*
 * Print out subusage stuff.
 */

void
usage( const char c )
{
    switch( c ) {
    case 'd':
	HelpPlain::print_debug( std::cerr );
	break;

    case 't':
	HelpPlain::print_trace( std::cerr );
	break;

    case 'z':
	HelpPlain::print_special( std::cerr );
	break;

    default:
	LQIO::internal_error( __FILE__, __LINE__, "usage()" );
	break;
    }
}


/*
 * generic error message.
 */

void
usage( const char c, const char * s )
{
    std::cerr << LQIO::io_vars.lq_toolname << " -" << c << ": invalid argument -- " << s << std::endl;
    usage( c );
}

/* -------------------------------------------------------------------- */
/* Man page generation.							*/
/* -------------------------------------------------------------------- */

std::map<const int,Help::help_fptr,lt_int> Help::option_table;

Help::Help()
{
    initialize();
}



/* static */ void
Help::initialize()
{
    /* Load functions used to print option args. */

    if ( option_table.size() > 0 ) return;

    option_table['I'] 	  = &Help::flagInputFormat;
    option_table['P']     = &Help::flagPragmas;
    option_table['V']     = &Help::flagVersion;
    option_table['a']     = &Help::flagAdvisory;
    option_table['b']     = &Help::flagBound;
    option_table['c']     = &Help::flagConvergence;
    option_table['d']     = &Help::flagDebug;
    option_table['e']     = &Help::flagError;
    option_table['f']	  = &Help::flagFast;
    option_table['G']     = &Help::flagGnuplot;
    option_table['I'] 	  = &Help::flagInputFormat;
    option_table['i']     = &Help::flagIterationLimit;
    option_table['n']     = &Help::flagNoExecute;
    option_table['o']     = &Help::flagOutput;
    option_table['p']     = &Help::flagParseable;
    option_table['r']	  = &Help::flagRTF;
    option_table['t']     = &Help::flagTrace;
    option_table['u']     = &Help::flagUnderrelaxation;
    option_table['v']     = &Help::flagVerbose;
    option_table['w']     = &Help::flagWarning;
    option_table['x']     = &Help::flagXML;
    option_table['z']     = &Help::flagSpecial;
    option_table[256+'e'] = &Help::flagExactMVA;
    option_table[256+'h'] = &Help::flagHwSwLayering;
    option_table[256+'l'] = &Help::flagLoose;
    option_table[256+'m'] = &Help::flagMethoOfLayers;
    option_table[256+'o'] = &Help::flagStopOnMessageLoss;
    option_table[256+'p'] = &Help::flagProcessorSharing;
    option_table[256+'s'] = &Help::flagSchweitzerMVA;
    option_table[256+'t'] = &Help::flagTraceMVA;
    option_table[256+'z'] = &Help::flagSquashedLayering;
    option_table[256+'v'] = &Help::flagNoVariance;
    option_table[512+'h'] = &Help::flagNoHeader;
    option_table[512+'r'] = &Help::flagReloadLQX;
    option_table[512+'R'] = &Help::flagRestartLQX;
    option_table[512+'l'] = &Help::flagDebugLQX;
    option_table[512+'x'] = &Help::flagDebugXML;

    Options::Debug::initialize();
    Options::Trace::initialize();
    Options::Special::initialize();

    __pragmas[LQIO::DOM::Pragma::_cycles_] =		    pragma_info( &Help::pragmaCycles, &__cycles_args );
    __cycles_args[LQIO::DOM::Pragma::_yes_] =		    parameter_info(&Help::pragmaCyclesAllow);
    __cycles_args[LQIO::DOM::Pragma::_no_] =		    parameter_info(&Help::pragmaCyclesDisallow,true);

    __pragmas[LQIO::DOM::Pragma::_stop_on_message_loss_] =  pragma_info( &Help::pragmaStopOnMessageLoss, &__stop_on_message_loss_args );
    __stop_on_message_loss_args[LQIO::DOM::Pragma::_yes_]=  parameter_info(&Help::pragmaStopOnMessageLossFalse);
    __stop_on_message_loss_args[LQIO::DOM::Pragma::_no_] =  parameter_info(&Help::pragmaStopOnMessageLossTrue,true);

    __pragmas[LQIO::DOM::Pragma::_force_multiserver_] =	    pragma_info( &Help::pragmaForceMultiserver, &__force_multiserver_args );
    __force_multiserver_args[LQIO::DOM::Pragma::_none_] =    parameter_info(&Help::pragmaForceMultiserverNone,true);
    __force_multiserver_args[LQIO::DOM::Pragma::_processors_] = parameter_info(&Help::pragmaForceMultiserverProcessors);
    __force_multiserver_args[LQIO::DOM::Pragma::_tasks_] =  parameter_info(&Help::pragmaForceMultiserverTasks);
    __force_multiserver_args[LQIO::DOM::Pragma::_all_] =    parameter_info(&Help::pragmaForceMultiserverAll);

    __pragmas[LQIO::DOM::Pragma::_interlocking_] =	    pragma_info( &Help::pragmaInterlock, &__interlock_args );
    __interlock_args[LQIO::DOM::Pragma::_yes_] =	    parameter_info(&Help::pragmaInterlockThroughput,true);
    __interlock_args[LQIO::DOM::Pragma::_no_] =		    parameter_info(&Help::pragmaInterlockNone);

    __pragmas[LQIO::DOM::Pragma::_layering_] =		    pragma_info( &Help::pragmaLayering, &__layering_args );
    __layering_args[LQIO::DOM::Pragma::_batched_] =	    parameter_info(&Help::pragmaLayeringBatched,true);
    __layering_args[LQIO::DOM::Pragma::_batched_back_] =    parameter_info(&Help::pragmaLayeringBatchedBack);
    __layering_args[LQIO::DOM::Pragma::_mol_] =		    parameter_info(&Help::pragmaLayeringMOL);
    __layering_args[LQIO::DOM::Pragma::_mol_back_] =	    parameter_info(&Help::pragmaLayeringMOLBack);
    __layering_args[LQIO::DOM::Pragma::_squashed_] =	    parameter_info(&Help::pragmaLayeringSquashed);
    __layering_args[LQIO::DOM::Pragma::_srvn_] =	    parameter_info(&Help::pragmaLayeringSRVN);
    __layering_args[LQIO::DOM::Pragma::_hwsw_] =	    parameter_info(&Help::pragmaLayeringHwSw);

    __pragmas[LQIO::DOM::Pragma::_multiserver_] =	    pragma_info( &Help::pragmaMultiserver, &__multiserver_args );
    __multiserver_args[LQIO::DOM::Pragma::_default_] =	    parameter_info(&Help::pragmaMultiServerDefault);
    __multiserver_args[LQIO::DOM::Pragma::_conway_] =	    parameter_info(&Help::pragmaMultiServerConway);
    __multiserver_args[LQIO::DOM::Pragma::_reiser_]  =	    parameter_info(&Help::pragmaMultiServerReiser);
    __multiserver_args[LQIO::DOM::Pragma::_reiser_ps_] =    parameter_info(&Help::pragmaMultiServerReiserPS);
    __multiserver_args[LQIO::DOM::Pragma::_rolia_] =	    parameter_info(&Help::pragmaMultiServerRolia);
    __multiserver_args[LQIO::DOM::Pragma::_rolia_ps_] =	    parameter_info(&Help::pragmaMultiServerRoliaPS);
    __multiserver_args[LQIO::DOM::Pragma::_bruell_] =	    parameter_info(&Help::pragmaMultiServerBruell);
    __multiserver_args[LQIO::DOM::Pragma::_schmidt_] =	    parameter_info(&Help::pragmaMultiServerSchmidt);
    __multiserver_args[LQIO::DOM::Pragma::_suri_] =	    parameter_info(&Help::pragmaMultiServerSuri);

    __pragmas[LQIO::DOM::Pragma::_mva_] =		    pragma_info( &Help::pragmaMVA, &__mva_args );
    __mva_args[LQIO::DOM::Pragma::_linearizer_] =	    parameter_info(&Help::pragmaMVALinearizer,true);
    __mva_args[LQIO::DOM::Pragma::_exact_] =		    parameter_info(&Help::pragmaMVAExact);
    __mva_args[LQIO::DOM::Pragma::_schweitzer_] =	    parameter_info(&Help::pragmaMVASchweitzer);
    __mva_args[LQIO::DOM::Pragma::_fast_] =		    parameter_info(&Help::pragmaMVAFast);
    __mva_args[LQIO::DOM::Pragma::_one_step_] =		    parameter_info(&Help::pragmaMVAOneStep);
    __mva_args[LQIO::DOM::Pragma::_one_step_linearizer_] =  parameter_info(&Help::pragmaMVAOneStepLinearizer);

#if HAVE_LIBGSL && HAVE_LIBGSLCBLAS
    __pragmas["quorum-distribution"] =			    pragma_info( &Help::pragmaQuorumDistribution, &__quorum_distribution_args );
    __pragmas[LQIO::DOM::Pragma::_quorum_delayed_calls_] =  pragma_info( &Help::pragmaQuorumDelayedCalls, &__quorum_delayed_calls_args );
    __pragmas["idletime"] =				    pragma_info( &Help::pragmaIdleTime, &__idle_time_args );
#endif

    __pragmas[LQIO::DOM::Pragma::_overtaking_] =	    pragma_info( &Help::pragmaOvertaking, &__overtaking_args );
    __overtaking_args[LQIO::DOM::Pragma::_markov_] =	    parameter_info(&Help::pragmaOvertakingMarkov,true);
    __overtaking_args[LQIO::DOM::Pragma::_rolia_] =	    parameter_info(&Help::pragmaOvertakingRolia);
    __overtaking_args[LQIO::DOM::Pragma::_simple_] =	    parameter_info(&Help::pragmaOvertakingSimple);
    __overtaking_args[LQIO::DOM::Pragma::_special_] =	    parameter_info(&Help::pragmaOvertakingSpecial);
    __overtaking_args[LQIO::DOM::Pragma::_none_] =	    parameter_info(&Help::pragmaOvertakingNone);

    __pragmas[LQIO::DOM::Pragma::_processor_scheduling_] =  pragma_info( &Help::pragmaProcessor, &__processor_args );
    __processor_args[LQIO::DOM::Pragma::_default_] =	    parameter_info(&Help::pragmaProcessorDefault);
    __processor_args["fcfs"] =				    parameter_info(&Help::pragmaProcessorFCFS);
    __processor_args["hol"] =				    parameter_info(&Help::pragmaProcessorHOL);
    __processor_args["ppr"] =				    parameter_info(&Help::pragmaProcessorPPR);
    __processor_args["ps"] =				    parameter_info(&Help::pragmaProcessorPS);

#if RESCHEDULE
    __pragmas[LQIO::DOM::Pragma::_reschedule_on_async_send_] = pragma_info( &Help::pragmaReschedule, &__reschedule_args );
    __reschedule_args[LQIO::DOM::Pragma::_false_] =	    parameter_info(&Help::pragmaRescheduleFalse,true);
    __reschedule_args[LQIO::DOM::Pragma::_true_] =	    parameter_info(&Help::pragmaRescheduleTrue);
#endif
    __pragmas[LQIO::DOM::Pragma::_tau_] =		    pragma_info( &Help::pragmaTau );

    __pragmas[LQIO::DOM::Pragma::_threads_] =		    pragma_info( &Help::pragmaThreads, &__threads_args );
    __threads_args[LQIO::DOM::Pragma::_hyper_] =	    parameter_info(&Help::pragmaThreadsHyper,true);
    __threads_args[LQIO::DOM::Pragma::_mak_] =		    parameter_info(&Help::pragmaThreadsMak);
    __threads_args[LQIO::DOM::Pragma::_none_] =		    parameter_info(&Help::pragmaThreadsNone);
    __threads_args[LQIO::DOM::Pragma::_exponential_] =	    parameter_info(&Help::pragmaThreadsExponential);

    __pragmas[LQIO::DOM::Pragma::_variance_] =		    pragma_info( &Help::pragmaVariance, &__variance_args );
    __variance_args[LQIO::DOM::Pragma::_default_] =	    parameter_info(&Help::pragmaVarianceDefault);
    __variance_args[LQIO::DOM::Pragma::_none_] =	    parameter_info(&Help::pragmaVarianceNone);
    __variance_args[LQIO::DOM::Pragma::_stochastic_] =	    parameter_info(&Help::pragmaVarianceStochastic,true);
    __variance_args[LQIO::DOM::Pragma::_mol_] =		    parameter_info(&Help::pragmaVarianceMol);
    __variance_args[LQIO::DOM::Pragma::_no_entry_] =	    parameter_info(&Help::pragmaVarianceNoEntry);
    __variance_args[LQIO::DOM::Pragma::_init_only_] =	    parameter_info(&Help::pragmaVarianceInitOnly);

    __pragmas[LQIO::DOM::Pragma::_severity_level_] =	    pragma_info( &Help::pragmaSeverityLevel, &__warning_args );
    __warning_args[LQIO::DOM::Pragma::_all_] =		    parameter_info(&Help::pragmaSeverityLevelWarnings);
    __warning_args[LQIO::DOM::Pragma::_warning_] =	    parameter_info(&Help::pragmaSeverityLevelWarnings);
    __warning_args[LQIO::DOM::Pragma::_advisory_] =	    parameter_info(&Help::pragmaSeverityLevelRunTime);
    __warning_args[LQIO::DOM::Pragma::_run_time_] =	    parameter_info(&Help::pragmaSeverityLevelRunTime);

    __pragmas[LQIO::DOM::Pragma::_spex_header_] =	    pragma_info( &Help::pragmaSpexHeader, &__spex_header_args );
    __spex_header_args[LQIO::DOM::Pragma::_false_] =	    parameter_info(&Help::pragmaSpexHeaderFalse,true);
    __spex_header_args[LQIO::DOM::Pragma::_true_] =	    parameter_info(&Help::pragmaSpexHeaderTrue);

    __pragmas[LQIO::DOM::Pragma::_prune_] =		    pragma_info( &Help::pragmaPrune, &__prune_args );
    __prune_args[LQIO::DOM::Pragma::_false_] =		    parameter_info(&Help::pragmaPruneFalse,true);
    __prune_args[LQIO::DOM::Pragma::_true_] =		    parameter_info(&Help::pragmaPruneTrue);
}



/*
 * Make a man page :-)
 */

std::ostream&
Help::print( std::ostream& output ) const
{
    Pragma::initialize();
    preamble( output );
    output << bold( *this, "Lqns" ) << " reads its input from " << filename( *this, "filename" ) << ", specified at the" << std::endl
	   << "command line if present, or from the standard input otherwise.  By" << std::endl
	   << "default, output for an input file " << filename( *this, "filename" ) << " specified on the" << std::endl
	   << "command line will be placed in the file " << filename( *this, "filename", ".out" ) << ".  If the" << std::endl
	   << flag( *this, "p" ) << " switch is used, parseable output will also be written into" << std::endl
	   << filename( *this, "filename", ".p" ) << ". If XML input" << ix( *this, "input!XML" ) << " or the " << flag( *this, "x" ) << " switch is used, XML output" << ix( *this, "output!XML" ) << " will be written to " << std::endl
	   << filename( *this, "filename", ".lqxo" ) << ".  This behaviour can be changed using the" << std::endl
	   << flag( *this, "o" ) << filename( *this, "output" ) << " switch, described below.  If several files are" << std::endl
	   << "named, then each is treated as a separate model and output will be" << std::endl
	   << "placed in separate output files.  If input is from the standard input," << std::endl
	   << "output will be directed to the standard output.  The file name `" << filename( *this, "-" ) << "' is" << std::endl
	   << "used to specify standard input." << std::endl;
    pp( output );
    output << "The " << flag( *this, "o" ) << filename( *this, "output" ) << " option can be used to direct output to the file" << std::endl
	   << filename( *this, "output" ) << " regardless of the source of input.  Output will be XML" << ix( *this, "XML" ) << ix( *this, "output!XML" ) << std::endl
	   << "if XML input" << ix( *this, "XML!input" ) << " or if the " << flag( *this, "x" ) << " switch is used, parseable output if the " << flag( *this, "p" ) << " switch is used," << std::endl
	   << "and normal output otherwise.  Multiple input files cannot be specified" << std::endl
	   << "when using this option.  Output can be directed to standard output by" << std::endl
	   << "using " << flag( *this, "o" ) << filename( *this, "-" ) << " (i.e., the output file name is `" << filename( *this, "-" ) << "'.)" << std::endl;

    section( output, "OPTIONS", "Command Line Options" );
    label( output, "sec:options" );
    dl_begin( output );
#if HAVE_GETOPT_LONG
    for ( const struct option *o = longopts; (o->name || o->val); ++o ) {
	longopt( output, o );
	help_fptr f = option_table[o->val];
	if ( f ) {
	    (this->*f)( output, true );
	}
    }
#endif
    dl_end( output );

    pp( output );
    output << bold( *this, "Lqns" ) << " exits" << ix( *this, "exit!success" ) << " with 0 on success, 1 if the model failed to converge," << ix( *this, "convergence!failure" ) << std::endl
	   << "2 if the input was invalid" << ix( *this, "input!invalid" ) << ", 4 if a command line argument was" << ix( *this, "command line!incorrect" ) << std::endl
	   << "incorrect, 8 for file read/write problems and -1 for fatal errors" << ix( *this, "error!fatal" )  <<".  If" << std::endl
	   << "multiple input files are being processed, the exit code is the" << std::endl
	   << "bit-wise OR of the above conditions." << std::endl;

    section( output, "PRAGMAS", "Pragmas" );
    label( output, "sec:lqns-pragmas" );
    output << emph( *this, "Pragmas" ) << ix( *this, "pragma" ) << " are used to alter the behaviour of the solver in a" << std::endl
	   << "variety of ways.  They can be specified in the input file with" << std::endl
	   << "``#pragma'', on the command line with the " << flag( *this, "P" ) << " option, or through" << std::endl
	   << "the environment variable " << ix( *this, "environment variable" ) << emph( *this, "LQNS_PRAGMAS" ) << ix( *this, "LQNS\\_PRAGMAS@\\texttt{LQNS\\_PRAGMAS}" ) << ".  Command line" << std::endl
	   << "specification of pragmas overrides those defined in the environment" << std::endl
	   << "variable which in turn override those defined in the input file.  The" << std::endl
	   << "following pragmas are supported.  Invalid pragma" << ix( *this, "pragma!invalid" ) << " specification at the" << std::endl
	   << "command line will stop the solver.  Invalid pragmas defined in the" << std::endl
	   << "environment variable or in the input file are ignored as they might be" << std::endl
	   << "used by other solvers." << std::endl;


    dl_begin( output );
    const std::map<std::string, Pragma::fptr>& pragmas = Pragma::getPragmas();
    for ( std::map<std::string, Pragma::fptr>::const_iterator pragma = pragmas.begin(); pragma != pragmas.end(); ++pragma  ) {
	print_pragma( output, pragma->first );
    }
    dl_end( output );

    section( output, "STOPPING CRITERIA", "Stopping Criteria" );
    label( output, "sec:lqns-stopping-criteria" );
    output << bold( *this, "Lqns" ) << " computes the model results by iterating through a set of" << std::endl
	   << "submodels until either convergence" << ix( *this, "convergence" ) << " is achieved, or the iteration limit" << ix( *this, "iteration limit|textbf" ) << std::endl
	   << "is hit. Convergence is determined by taking the root of the mean of" << std::endl
	   << "the squares of the difference in the utilization of all of the servers" << std::endl
	   << "from the last two iterations of the MVA solver over the all of the" << std::endl
	   << "submodels then comparing the result to the convergence value specified" << std::endl
	   << "in the input file. If the RMS change in utilization is less than" << std::endl
	   << "convergence value" << ix( *this, "convergence!value|textbf" )  << ", then the results are considered valid." << std::endl;
    pp( output );
    output << "If the model fails to converge," << ix( *this, "convergence!failure" ) << " three options are available:" << std::endl;
    ol_begin( output );
    li( output, "1." );
    output << "reduce the under-relaxation coefficient. Waiting and idle times are" << std::endl
	   << "propogated between submodels during each iteration. The" << std::endl
	   << "under-relaxation coefficient determines the amount a service time is" << std::endl
	   << "changed between each iteration. A typical value is 0.7 - 0.9; reducing" << std::endl
	   << "it to 0.1 may help." << std::endl;
    li( output, "2." );
    output << "increase the iteration limit." << ix( *this, "iteration limit" ) << " The iteration limit sets the upper bound" << std::endl
	   << "on the number of times all of the submodels are solved. This value may" << std::endl
	   << "have to be increased, especially if the under-relaxation coefficient" << std::endl
	   << "is small, or if the model is deeply nested. The default value is 50" << std::endl
	   << "iterations." << std::endl;
    li( output, "3." );
    output << "increase the convergence test value" << ix( *this, "convergence!value" ) << ". Note that the convergence value" << std::endl
	   << "is the standard deviation in the change in the utilization of the" << std::endl
	   << "servers, so a value greater than 1.0 makes no sense." << std::endl;
    ol_end( output );

    pp( output );
    output << "The convergence value can be observed using " << flag( *this, "t" ) << emph( *this, "convergence" ) << " flag." << std::endl;

    section( output, "MODEL LIMITS", "Model Limits" );
    label( output, "sec:model-limits" );
    output << "The following table lists the acceptable parameter types for"  << std::endl
	   << bold( *this, "lqns" ) << ".  An error will" << std::endl
	   << "be reported if an unsupported parameter is supplied except when the" << std::endl
	   << "value supplied is the same as the default."  << std::endl;

    pp( output );
    table_header( output );
    table_row( output, "Phases", "3", "phase!maximum" );
    table_row( output, "Scheduling", "FIFO, HOL, PPR", "scheduling" );
    table_row( output, "Open arrivals", LQIO::DOM::Pragma::_yes_, "open arrival" );
    table_row( output, "Phase type", "stochastic, deterministic", "phase!type" );
    table_row( output, "Think Time", LQIO::DOM::Pragma::_yes_, "think time" );
    table_row( output, "Coefficient of variation", LQIO::DOM::Pragma::_yes_, "coefficient of variation" );
    table_row( output, "Interprocessor-delay", LQIO::DOM::Pragma::_yes_, "interprocessor delay" );
    table_row( output, "Asynchronous connections", LQIO::DOM::Pragma::_yes_, "asynchronous connections" );
    table_row( output, "Forwarding", LQIO::DOM::Pragma::_yes_, "forwarding" );
    table_row( output, "Multi-servers", LQIO::DOM::Pragma::_yes_, "multi-server" );
    table_row( output, "Infinite-servers", LQIO::DOM::Pragma::_yes_, "infinite server" );
    table_row( output, "Max Entries", "1000", "entry!maximum" );
    table_row( output, "Max Tasks", "1000", "task!maximum" );
    table_row( output, "Max Processors", "1000", "processor!maximum" );
    table_row( output, "Max Entries per Task", "1000" );
    table_footer( output );

    section( output, "DIAGNOSTICS", "Diagnostics" );
    label( output, "sec:lqns-diagnostics" );

    output << "Most diagnostic messages result from errors in the input file." << std::endl
	   << "If the solver reports errors, then no solution will be generated for" << std::endl
	   << "the model being solved.  Models which generate warnings may not be" << std::endl
	   << "correct.  However, the solver will generate output." << std::endl;
    pp( output );
    output << "Sometimes the model fails to converge" << ix( *this, "convergence!failure" ) << ", particularly if there are several" << std::endl
	   << "heavily utilized servers in a submodel.  Sometimes, this problem can" << std::endl
	   << "be solved by reducing the value of the under-relaxation coefficient.  It"  << std::endl
	   << "may also be necessary to increase the iteration-limit" << ix( *this, "iteration limit" ) << ", particularly if" << std::endl
	   << "there are many submodels.  With replicated models, it may be necessary" << std::endl
	   << "to use `srvn' layering to get the model to converge.  Convergence can be tracked" << std::endl
	   << "using the "<< flag( *this, "t" ) << emph( *this, "convergence" ) << " option." << std::endl;
    pp( output );
    output << "The solver will sometimes report some servers with `high' utilization." << std::endl
	   << "This problem is the result of some of the approximations used, in particular, two-phase servers." << std::endl
	   << "Utilizations in excess of 10\\% are likely the result of failures in the solver." << std::endl
	   << "Please send us the model file so that we can improve the algorithms." << std::endl;
    see_also( output );
    trailer( output );
    return output;
}

std::ostream&
Help::flagAdvisory( std::ostream& output, bool verbose ) const
{
    output << "Ignore advisories.  The default is to print out all advisories." << ix( *this, "advisory!ignore" ) << std::endl;
    return output;
}

std::ostream&
Help::flagBound( std::ostream& output, bool verbose ) const
{
    output << "This option is used to compute the ``Type 1 throughput bounds''" << ix( *this, "throughput!bounds" ) << ix( *this, "bounds!throughput" ) << " only."  << std::endl
	   << "These bounds are computed assuming no contention anywhere in the model" << std::endl
	   << "and represent the guaranteed not to exceed values." << std::endl;
    return output;
}

std::ostream&
Help::flagDebug( std::ostream& output, bool verbose ) const
{
    output << "This option is used to enable debug output." << ix( *this, "debug" ) << std::endl
	   << emph( *this, "Arg" ) << " can be one of:" << std::endl;
    increase_indent( output );
    dl_begin( output );
    std::map<const char *, Options::Debug>::const_iterator opt;
    for ( opt = Options::Debug::__table.begin(); opt != Options::Debug::__table.end(); ++opt ) {
	print_option( output, opt->first, opt->second );
    }
    dl_end( output );
    decrease_indent( output );
    return output;
}

std::ostream&
Help::flagError( std::ostream& output, bool verbose ) const
{
    output << "This option is to enable floating point exception handling." << ix( *this, "floating point!exception" ) << std::endl
	   << emph( *this, "Arg" ) << " must be one of the following:" << std::endl;
    increase_indent( output );
    ol_begin( output );
    li( output ) << bold( *this, "a" ) << std::endl
		 << "Abort immediately on a floating point error (provided the floating point unit can do so)." << std::endl;
    li( output ) << bold( *this, "d" ) << std::endl
		 << "Abort on floating point errors. (default)" << std::endl;
    li( output ) << bold( *this, "i" ) << std::endl
		 << "Ignore floating point errors." << std::endl;
    li( output ) << bold( *this, "w" ) << std::endl
		 << "Warn on floating point errors." << std::endl;
    ol_end( output );
    output << "The solver checks for floating point overflow," << ix( *this, "overflow" ) << " division by zero and invalid operations." << std::endl
	   << "Underflow and inexact result exceptions are always ignored." << std::endl;
    pp( output );
    output << "In some instances, infinities " << ix( *this, "infinity" ) << ix( *this, "floating point!infinity" ) << " will be propogated within the solver.  Please refer to the" << std::endl
	   << bold( *this, LQIO::DOM::Pragma::_stop_on_message_loss_ ) << " pragma below." << std::endl;
    decrease_indent( output );
    return output;
}

std::ostream& 
Help::flagFast( std::ostream& output, bool verbose ) const
{
    output << "This option is used to set options for quick solution of a model using One-Step (Bard-Schweitzer) MVA."  << std::endl
	   << "It is equivalent to setting " << bold( *this, "pragma" ) 
	   << " "  << emph( *this, LQIO::DOM::Pragma::_mva_ ) << "=" << emph( *this, LQIO::DOM::Pragma::_one_step_ )
	   << ", " << emph( *this, LQIO::DOM::Pragma::_layering_ ) << "=" << emph( *this, LQIO::DOM::Pragma::_batched_ )
	   << ", " << emph( *this, LQIO::DOM::Pragma::_multiserver_ ) << "=" << emph( *this, LQIO::DOM::Pragma::_conway_ ) << std::endl;
    return output;
}

std::ostream&
Help::flagGnuplot( std::ostream& output, bool verbose ) const
{
    output << "This option is used to generate gnuplot(1) output.  The optional argument is a list of" << std::endl
	   << "result variables found in the input file.  This option only works for SPEX input." << std::endl;
    return output;
}

std::ostream&
Help::flagInputFormat( std::ostream& output, bool verbose ) const
{
    output << "This option is used to force the input file format to either " << emph( *this, "xml" ) << " or " << emph( *this, "lqn" ) << "." << std::endl
	   << "By default, if the suffix of the input filename is one of: " << emph( *this, ".in" ) << ", " << emph( *this, ".lqn" ) << " or " << emph( *this, ".xlqn" ) << std::endl
	   << ", then the LQN parser will be used.  Otherwise, input is assumed to be XML." << std::endl;
    return output;
}

std::ostream&
Help::flagNoExecute( std::ostream& output, bool verbose ) const
{
    output << "Read input, but do not solve.  The input is checked for validity.  " << std::endl
	   << "No output is generated." << std::endl;
    return output;
}

std::ostream& 
Help::flagMethoOfLayers( std::ostream& output, bool verbose ) const
{
    output << "This option is to use the Method Of Layers solution approach to solving the layer submodels." << std::endl;
    return output;
}

std::ostream&
Help::flagOutput( std::ostream& output, bool verbose ) const
{
    output << "Direct analysis results to " << emph( *this, "output" ) << ix( *this, "output" ) << ".  A filename of `" << filename( *this, "-" )  << ix( *this, "standard input" ) << "'" << std::endl
	   << "directs output to standard output.  If " << filename( *this, "output" ) << " is a directory, all output is saved in " 
	   << filename( *this, "output/input.out" ) << ". If the input model contains a SPEX program with loops, the SPEX output is sent to "
	   << filename( *this, "output" ) << "; the individual model output files are found in the directory " 
	   << filename( *this, "output.d" ) << ". If " << bold( *this, "lqns" ) <<" is invoked with this" << std::endl
	   << "option, only one input file can be specified." << std::endl;
    return output;
}

std::ostream&
Help::flagParseable( std::ostream& output, bool verbose ) const
{
    output << "Generate parseable output suitable as input to other programs such as" << std::endl
	   << bold( *this, "lqn2ps(1)" ) << " and " << bold( *this, "srvndiff(1)" ) << ".  If input is from" << std::endl
	   << filename( *this, "filename" ) << ", parseable output is directed to " << filename( *this, "filename", ".p" ) << "." << std::endl
	   << "If standard input is used for input, then the parseable output is sent" << std::endl
	   << "to the standard output device.  If the " << flag( *this, "o" ) << filename( *this, "output" ) << " option is used, the" << std::endl
	   << "parseable output is sent to the file name " << filename( *this, "output" ) << "." << std::endl
	   << "(In this case, only parseable output is emitted.)" << std::endl;
    return output;
}


std::ostream&
Help::flagPragmas( std::ostream& output, bool verbose ) const
{
    output << "Change the default solution strategy.  Refer to the PRAGMAS section" << ix( *this, "pragma" ) << std::endl
	   << "below for more information." << std::endl;
    return output;
}

std::ostream& 
Help::flagProcessorSharing( std::ostream& output, bool verbose ) const
{
    output << "Use Processor Sharing scheduling at all fixed-rate processors." << std::endl;
    return output;
}

std::ostream&
Help::flagRTF( std::ostream& output, bool verbose ) const
{
    output << "Output results using Rich Text Format instead of plain text.  Processors, entries and tasks with high utilizations are coloured in red." << std::endl;
    return output;
}

std::ostream& 
Help::flagSquashedLayering( std::ostream& output, bool verbose ) const
{
    output << "Use only one submodel to solve the model." << std::endl;
    return output;
}

std::ostream&
Help::flagTrace( std::ostream& output, bool verbose ) const
{
    output << "This option is used to set tracing " << ix( *this, "tracing" ) << " options which are used to print out various" << std::endl
	   << "intermediate results " << ix( *this, "results!intermediate" ) << " while a model is being solved." << std::endl
	   << emph( *this, "arg" ) << " can be any combination of the following:" << std::endl;
    increase_indent( output );
    dl_begin( output );
    std::map<const char *, Options::Trace>::const_iterator opt;
    for ( opt = Options::Trace::__table.begin(); opt != Options::Trace::__table.end(); ++opt ) {
	print_option( output, opt->first, opt->second );
    }
    dl_end( output );
    decrease_indent( output );
    return output;
}


std::ostream&
Help::flagVerbose( std::ostream& output, bool verbose ) const
{
    output << "Generate output after each iteration of the MVA solver and the convergence value at the end of each outer iteration of the solver." << std::endl;
    return output;
}

std::ostream&
Help::flagVersion( std::ostream& output, bool verbose ) const
{
    output << "Print out version and copyright information." << ix( *this, "version" ) << ix( *this, "copyright" ) << std::endl;
    return output;
}

std::ostream&
Help::flagWarning( std::ostream& output, bool verbose ) const
{
    output << "Ignore warnings.  The default is to print out all warnings." << ix( *this, "warning!ignore" ) << std::endl;
    return output;
}

std::ostream&
Help::flagXML( std::ostream& output, bool verbose ) const
{
    output << "Generate XML output regardless of input format." << std::endl;
    return output;
}

std::ostream&
Help::flagSpecial( std::ostream& output, bool verbose ) const
{
    output << "This option is used to select special options.  Arguments of the form" << std::endl
	   << emph( *this, "nn" ) << " are integers while arguments of the form " << emph( *this, "nn.n" ) << " are real" << std::endl
	   << "numbers.  " << emph( *this, "Arg" ) << " can be any of the following:" << std::endl;
    increase_indent( output );
    dl_begin( output );
    std::map<const char *, Options::Special>::const_iterator opt;
    for ( opt = Options::Special::__table.begin(); opt != Options::Special::__table.end(); ++opt ) {
	print_option( output, opt->first, opt->second );
    }
    dl_end( output );
    br( output );
    output << "If any one of " << emph( *this, "convergence" ) << ", " << emph( *this, "iteration-limit" ) << ", or"
	   << emph( *this, "print-interval" ) << " are used as arguments, the corresponding " << std::endl
	   << "value specified in the input file for general information, `G', is" << std::endl
	   << "ignored.  " << std::endl;
    decrease_indent( output );
    return output;
}

std::ostream&
Help::flagConvergence( std::ostream& output, bool verbose ) const
{
    help_fptr f = Options::Special::__table["convergence-value"].help();
    (this->*f)(output,verbose) << ix( *this, "convergence!value" );
    return output;
}

std::ostream&
Help::flagUnderrelaxation( std::ostream& output, bool verbose ) const
{
    help_fptr f = Options::Special::__table[LQIO::DOM::Pragma::_underrelaxation_].help();
    (this->*f)(output,verbose);
    return output;
}
std::ostream&
Help::flagIterationLimit( std::ostream& output, bool verbose ) const
{
    help_fptr f = Options::Special::__table["iteration-limit"].help();
    (this->*f)(output,verbose);
    return output;
}

std::ostream&
Help::flagExactMVA( std::ostream& output, bool verbose ) const
{
    output << "Use Exact MVA to solve all submodels." << ix( *this, "MVA!exact" ) << std::endl;
    return output;
}

std::ostream&
Help::flagSchweitzerMVA( std::ostream& output, bool verbose ) const
{
    output << "Use Bard-Schweitzer approximate MVA to solve all submodels." << ix( *this, "MVA!Bard-Schweitzer" ) << std::endl;
    return output;
}

std::ostream&
Help::flagHwSwLayering( std::ostream& output, bool verbose ) const
{
    return output;
}

std::ostream&
Help::flagLoose( std::ostream& output, bool verbose ) const
{
    output << "Solve the model using submodels containing exactly one server." << std::endl;
    return output;
}

std::ostream&
Help::flagStopOnMessageLoss( std::ostream& output, bool verbose ) const
{
    output << "Do not stop the solver on overflow (infinities) for open arrivals or send-no-reply messages to entries.  The default is to stop with an" << std::endl
	   << "error message indicating that the arrival rate is too high for the service time of the entry" << std::endl;
    return output;
}

std::ostream&
Help::flagTraceMVA( std::ostream& output, bool verbose ) const
{
    output << "Output the inputs and results of each MVA submodel for every iteration of the solver." << std::endl;
    return output;
}

std::ostream&
Help::flagNoHeader( std::ostream& output, bool verbose ) const
{
    output << "Do not print out the Result Variable header when running with SPEX input." << std::endl;
    if ( verbose ) {
	output << "This option has no effect otherwise." << std::endl;
    }
    return output;
}

std::ostream&
Help::flagNoVariance( std::ostream& output, bool verbose ) const
{
    output << "Do not use variances in the waiting time calculations." << std::endl;
    return output;
}

std::ostream&
Help::flagReloadLQX( std::ostream& output, bool verbose ) const
{
    output << "Re-run the LQX/SPEX" << ix( *this, "LQX" ) <<  " program without re-solving the models.  Results must exist from a previous solution run." << std::endl
	   << "This option is useful if LQX print statements or SPEX results are changed." << std::endl;
    return output;
}

std::ostream&
Help::flagRestartLQX( std::ostream& output, bool verbose ) const
{
    output << "Re-run the LQX/SPEX" << ix( *this, "LQX" ) <<  " program without re-solving models which were solved successfully.  Models which were not solved because of early termination, or which were not solved successfully because of convergence problems, will be solved." << std::endl
	   << "This option is useful for running a second pass with a new convergnece value and/or iteration limit." << std::endl;
    return output;
}

std::ostream&
Help::flagDebugLQX( std::ostream& output, bool verbose ) const
{
    output << "Output debugging information as an LQX" << ix( *this, "LQX!debug" ) << " program is being parsed." << std::endl;
    return output;
}

std::ostream&
Help::flagDebugXML( std::ostream& output, bool verbose ) const
{
    output << "Output XML" << ix( *this, "XML!debug" ) << " elements and attributes as they are being parsed.   Since the XML parser usually stops when it encounters an error," << std::endl
	   << "this option can be used to localize the error." << std::endl;
    return output;
}


std::ostream&
Help::debugAll( std::ostream & output, bool verbose ) const
{
    output << "Enable all debug output." << std::endl;
    return output;
}

std::ostream&
Help::debugActivities( std::ostream & output, bool verbose ) const
{
    output << "Activities -- not functional." << std::endl;
    return output;
}

std::ostream&
Help::debugCalls( std::ostream & output, bool verbose ) const
{
    output << "Print out the number of rendezvous between all tasks." << std::endl;
    return output;
}

std::ostream&
Help::debugForks( std::ostream & output, bool verbose ) const
{
    output << "Print out the fork-join matching process." << std::endl;
    return output;
}

std::ostream&
Help::debugInterlock( std::ostream & output, bool verbose ) const
{
    output << "Print out the interlocking table and the interlocking between all tasks and processors." << std::endl;
    return output;
}

std::ostream&
Help::debugJoins( std::ostream & output, bool verbose ) const
{
    output << "Joins -- not functional." << std::endl;
    return output;
}

std::ostream&
Help::debugLayers( std::ostream & output, bool verbose ) const
{
    output << "Print out the contents of all of the layers found in the model." << std::endl;
    return output;
}

std::ostream&
Help::debugLQX( std::ostream & output, bool verbose ) const
{
    output << "Print out the actions the LQX parser while reading an LQX program." << std::endl;
    return output;
}

std::ostream&
Help::debugMVA( std::ostream & output, bool verbose ) const
{
    output << "Print out oodles of information while the MVA solver is running.  The MVA solver must be compiled with DEBUG_MVA for this option to have effect." << std::endl;
    return output;
}

std::ostream&
Help::debugOvertaking( std::ostream & output, bool verbose ) const
{
    output << "Print the overtaking probabilities in the output file." << std::endl;
    return output;
}

std::ostream&
Help::debugQuorum( std::ostream & output, bool verbose ) const
{
    output << "Print out results from pseudo activities used by quorum." << std::endl;
    return output;
}

std::ostream&
Help::debugVariance( std::ostream & output, bool verbose ) const
{
    output << "Print out variance calculation." << std::endl;
    return output;
}

std::ostream&
Help::debugXML( std::ostream & output, bool verbose ) const
{
    output << "Print out the actions of the Expat parser while reading XML input." << std::endl;
    return output;
}

std::ostream&
Help::traceActivities( std::ostream & output, bool verbose ) const
{
    output << "Print out results of activity aggregation."<< std::endl;
    return output;
}

std::ostream&
Help::traceConvergence( std::ostream & output, bool verbose ) const
{
    output << "Print out convergence value after each submodel is solved." << std::endl;
    if ( verbose ) {
	output << "This option is useful for tracking the rate of convergence for a model." << std::endl
	       << "The optional numeric argument supplied to this option will print out the convergence value for the specified MVA submodel, otherwise," << std::endl
	       << "the convergence value for all submodels will be printed." << std::endl;
    }
    return output;
}

std::ostream&
Help::traceDeltaWait( std::ostream & output, bool verbose ) const
{
    output << "Print out difference in entry service time after each submodel is solved." << std::endl;
    return output;
}

std::ostream&
Help::traceForks( std::ostream & output, bool verbose ) const
{
    output << "Print out overlap table for forks prior to submodel solution." << std::endl;
    return output;
}

std::ostream&
Help::traceIdleTime( std::ostream & output, bool verbose ) const
{
    output << "Print out computed idle time after each submodel is solved." << std::endl;
    return output;
}

std::ostream&
Help::traceInterlock( std::ostream & output, bool verbose ) const
{
    output << "Print out interlocking adjustment before each submodel is solved." << std::endl;
    return output;
}

std::ostream&
Help::traceIntermediate( std::ostream & output, bool verbose ) const
{
    output << "Print out intermediate solutions at the print interval specified in the model." << std::endl;
    if ( verbose ) {
	output << "The print interval field in the input is ignored otherwise." << std::endl;
    }
    return output;
}

std::ostream&
Help::traceJoins( std::ostream & output, bool verbose ) const
{
    output << "Print out computed join delay and join overlap table prior to submodel solution." << std::endl;
    return output;
}

std::ostream&
Help::traceMva( std::ostream & output, bool verbose ) const
{
    output << "Print out the MVA submodel and its solution." << std::endl;  
    if ( verbose ) {
	output << "A numeric argument supplied to this option will print out only the specified MVA submodel, otherwise, all submodels will be printed." << std::endl;
    }
    return output;
}

std::ostream&
Help::traceOvertaking( std::ostream & output, bool verbose ) const
{
    output << "Print out overtaking calculations." << std::endl;
    return output;
}

std::ostream&
Help::traceQuorum( std::ostream & output, bool verbose ) const
{
    output << "Print quorum traces." << std::endl;
    return output;
}

std::ostream&
Help::traceReplication( std::ostream & output, bool verbose ) const
{
    output << "" << std::endl;
    return output;
}

std::ostream&
Help::traceThroughput( std::ostream & output, bool verbose ) const
{
    output << "Print throughput's values." << std::endl;
    return output;
}

std::ostream&
Help::traceVariance( std::ostream & output, bool verbose ) const
{
    output << "Print out the variances calculated after each submodel is solved." << std::endl;
    return output;
}

std::ostream&
Help::traceVirtualEntry( std::ostream & output, bool verbose ) const
{
    output << "Print waiting time for each rendezvous in the model after it has been computed; include virtual entries." << std::endl;
    return output;
}

std::ostream&
Help::traceWait( std::ostream & output, bool verbose ) const
{
    output << "Print waiting time for each rendezvous in the model after it has been computed." << std::endl;
    return output;
}

std::ostream&
Help::specialIterationLimit( std::ostream & output, bool verbose ) const
{
    output << "Set the maximum number of iterations to " << emph( *this, "arg" ) << "." << std::endl;
    if ( verbose ) {
	output << emph( *this, "Arg" ) << " must be an integer greater than 0.  The default value is 50." << std::endl;
    }
    return output;
}

std::ostream&
Help::specialPrintInterval( std::ostream & output, bool verbose ) const
{
    output << "Set the printing interval to " << emph( *this, "arg" ) << "." << std::endl;
    if ( verbose ) {
	output << "The " << flag( *this, "d" ) << " or " << flag( *this, "v" ) << " options must also be selected to display intermediate results." << std::endl
	       << "The default value is 10." << std::endl;
    }
    return output;
}

std::ostream&
Help::specialOvertaking( std::ostream & output, bool verbose ) const
{
    output << "Print out overtaking probabilities." << std::endl;
    return output;
}

std::ostream&
Help::specialConvergenceValue( std::ostream & output, bool verbose ) const
{
    output << "Set the convergence value to " << emph( *this, "arg" ) << ".  " << std::endl;
    if ( verbose ) {
	output << emph( *this, "Arg" ) << " must be a number between 0.0 and 1.0." << std::endl;
    }
    return output;
}

std::ostream&
Help::specialSingleStep( std::ostream & output, bool verbose ) const
{
    output << "Stop after each MVA submodel is solved." << std::endl;
    if ( verbose ) {
	output << "Any character typed at the terminal except end-of-file will resume the calculation.  End-of-file will cancel single-stepping altogether." << std::endl;
    }
    return output;
}

std::ostream&
Help::specialUnderrelaxation( std::ostream & output, bool verbose ) const
{
    output << "Set the underrelaxation to " << emph( *this, "arg" ) << "." << std::endl;
    if ( verbose ) {
	output << emph( *this, "Arg" ) << " must be a number between 0.0 and 1.0." << std::endl
	       << "The default value is 0.9." << std::endl;
    }
    return output;
}

std::ostream&
Help::specialGenerateQueueingModel( std::ostream & output, bool verbose ) const
{
    output << "This option is used for debugging the solver." << std::endl;
    if ( verbose ) {
	output << "A directory named " << emph( *this, "arg" )
	       << " will be created containing source code for invoking the MVA solver directly."  << std::endl;
    }
    return output;
}

std::ostream&
Help::specialMolMSUnderrelaxation( std::ostream & output, bool verbose ) const
{
    output << "Set the under-relaxation factor to " << emph( *this, "arg" ) << " for the MOL multiserver approximation." << std::endl;
    if ( verbose ) {
	output << emph( *this, "Arg" ) << " must be a number between 0.0 and 1.0." << std::endl
	       << "The default value is 0.5.";
    }
    return output;
}

std::ostream&
Help::specialMakeMan( std::ostream & output, bool verbose ) const
{
    output << "Output this manual page.  " << std::endl;
    if ( verbose ) {
	output << "If an optional " << emph( *this, "arg" ) << std::endl
	       << "is supplied, output will be written to the file named "  << emph( *this, "arg" )  << "." << std::endl
	       << "Otherwise, output is sent to stdout." << std::endl;
    }
    return output;
}

std::ostream&
Help::specialMakeTex( std::ostream & output, bool verbose ) const
{
    output << "Output this manual page in LaTeX format." << std::endl;
    if ( verbose ) {
	output << "If an optional " << emph( *this, "arg" ) << std::endl
	       << "is supplied, output will be written to the file named "  << emph( *this, "arg" )  << "." << std::endl
	       << "Otherwise, output is sent to stdout." << std::endl;
    }
    return output;
}

std::ostream&
Help::specialMinSteps( std::ostream & output, bool verbose ) const
{
    output << "Force the solver to iterate min-steps times." << std::endl;
    return output;
}

std::ostream&
Help::specialIgnoreOverhangingThreads( std::ostream & output, bool verbose ) const
{
    output << "Ignore the effect of the overhanging threads."<< std::endl;
    return output;
}

std::ostream&
Help::specialFullReinitialize( std::ostream & output, bool verbose ) const
{
    output << "For multiple runs, reinitialize all service times at processors." << std::endl;
    return output;
}

std::ostream&
Help::pragmaCycles( std::ostream& output, bool verbose ) const
{
    output << "This pragma is used to enable or disable cycle detection" << ix( *this, "cycle!detection" ) << " in the call" << std::endl
	   << "graph." << ix( *this, "call graph" ) << "  Cycles may indicate the presence of deadlocks." << ix( *this, "deadlock" ) << std::endl
	   << emph( *this, "Arg" ) << " must be one of: " << std::endl;
    return output;
}

std::ostream&
Help::pragmaForceMultiserver( std::ostream& output, bool verbose ) const
{
    output << "This pragma is used to force the use of a multiserver" << ix(*this, "multiserver!force" ) << std::endl
	   << "instead of a fixed-rate server whenever the multiplicity of a server is one." << std::endl
	   << emph( *this, "Arg" ) << " must be one of: " << std::endl;
    return output;
}

std::ostream&
Help::pragmaInterlock( std::ostream& output, bool verbose ) const
{
    output << "The interlocking" << ix( *this, "interlock" ) << " is used to correct the throughputs" << ix( *this, "throughput!interlock" ) << " at stations as a" << std::endl
	   << "result of solving the model using layers" << cite( *this, "perf:franks-95-ipds-interlock" ) << ".  This pragma is used to" << std::endl
	   << "choose the algorithm used." << std::endl
	   << emph( *this, "Arg" ) << " must be one of: " << std::endl;
    return output;
}

std::ostream&
Help::pragmaLayering( std::ostream& output, bool verbose ) const
{
    output << "This pragma is used to select the layering strategy" << ix( *this, "layering!strategy" ) << " used by the solver." << std::endl
	   << emph( *this, "Arg" ) << " must be one of: " << std::endl;
    return output;
}

std::ostream&
Help::pragmaMultiserver( std::ostream& output, bool verbose ) const
{
    output << "This pragma is used to choose the algorithm for solving multiservers" << ix( *this, "multiserver!algorithm" ) << "." << std::endl
	   << emph( *this, "Arg" ) << " must be one of: " << std::endl;
    return output;
}

std::ostream&
Help::pragmaMVA( std::ostream& output, bool verbose ) const
{
    output << "This pragma is used to choose the MVA" << ix( *this, "MVA" ) << " algorithm used to solve the submodels." << std::endl
	   << emph( *this, "Arg" ) << " must be one of: " << std::endl;
    return output;
}

#if HAVE_LIBGSL && HAVE_LIBGSLCBLAS
std::ostream&
Help::pragmaQuorumDistribution( std::ostream& output, bool verbose ) const
{
    output << "This pragma is used to choose the Quorum algorithm used to approximate " ;
    output <<"\nthe thread service time distibution." << std::endl;
    return output;
}

std::ostream&
Help::pragmaQuorumDelayedCalls( std::ostream& output, bool verbose ) const
{
    output << "This pragma is used to choose the Quorum semantics for the delayed calls" << std::endl; ;

    return output;
}

std::ostream&
Help::pragmaIdleTime( std::ostream& output, bool verbose ) const
{
    output << "This pragma is used to choose throughput used to calculate " ;
    output <<"\nthreads idle times." << std::endl;
    return output;
}
//// end tomari quorum idle time
#endif

std::ostream&
Help::pragmaOvertaking( std::ostream& output, bool verbose ) const
{
    output << "This pragma is usesd to choose the overtaking" << ix( *this, LQIO::DOM::Pragma::_overtaking_ ) << " approximation." << std::endl
	   << emph( *this, "Arg" ) << " must be one of: " << std::endl;
    return output;
}

std::ostream&
Help::pragmaProcessor( std::ostream& output, bool verbose ) const
{
    output << "Force the scheduling type" << ix( *this, "scheduling!processor" ) << ix( *this, "processor!scheduling" ) << " of all uni-processors to the type specfied." << std::endl;
    return output;
}

#if RESCHEDULE
std::ostream&
Help::pragmaReschedule( std::ostream& output, bool verbose ) const
{
    output << "Tasks are normally blocked after every rendezvous request, but" << std::endl
	   << "continue to execute after for send-no-reply.  This option changes" << std::endl
	   << "this behaviour." << std::endl;
    return output;
}

std::ostream&
Help::pragmaRescheduleTrue( std::ostream& output, bool verbose ) const
{
    output << "Reschedule after an asynchronous send." << std::endl;
    return output;
}


std::ostream&
Help::pragmaRescheduleFalse( std::ostream& output, bool verbose ) const
{
    output << "Don't reschedule after an asynchronous send." << std::endl;
    return output;
}
#endif

std::ostream&
Help::pragmaStopOnMessageLoss( std::ostream& output, bool verbose ) const
{
    output << "This pragma is used to control the operation of the solver when the" << std::endl
	   << "arrival rate" << ix( *this, "arrival rate" ) << " exceeds the service rate of a server." << std::endl
	   << emph( *this, "Arg" ) << " must be one of: " << std::endl;
    return output;
}

std::ostream&
Help::pragmaTau( std::ostream& output, bool verbose ) const
{
    output << "Set the tau adjustment factor to " << emph( *this, "arg" ) << "." << std::endl
	   << emph( *this, "Arg" ) << " must be an integer between 0 and 25." << std::endl
	   << "A value of " << emph( *this, "zero" ) << " disables the adjustment." << std::endl;
    return output;
}

std::ostream&
Help::pragmaThreads( std::ostream& output, bool verbose ) const
{
    output << "This pragma is used to change the behaviour of the solver when solving" << std::endl
	   << "models with fork-join" << ix( *this, "fork" ) << ix( *this, "join" ) << " interactions." << std::endl;
    return output;
}

std::ostream&
Help::pragmaVariance( std::ostream& output, bool verbose ) const
{
    output << "This pragma is used to choose the variance" << ix( *this, LQIO::DOM::Pragma::_variance_ ) << " calculation used by the solver." << std::endl;
    return output;
}


std::ostream& 
Help::pragmaSeverityLevel( std::ostream& output, bool verbose ) const
{
    output << "This pragma is used to enable or disable warning messages." << std::endl;
    return output;
}

std::ostream&
Help::pragmaSpexHeader( std::ostream& output, bool verbose ) const
{
    output << "This pragma is used to enable or disable the header line of SPEX output." << std::endl
	   << emph( *this, "Arg" ) << " must be one of: " << std::endl;
    return output;
}

std::ostream&
Help::pragmaPrune( std::ostream& output, bool verbose ) const
{
    output << "This pragma is used to prune \"useless\" processors when solving the model.  Useless" << std::endl
	   << "processors are any processor which will always have a queue length of zero (i.e., delay servers" << std::endl
	   << "and processors with only one task, etc.)" << std::endl
	   << emph( *this, "Arg" ) << " must be one of: " << std::endl;
    return output;
}

std::ostream&
Help::pragmaCyclesAllow( std::ostream& output, bool verbose ) const
{
    output << "Allow cycles in the call graph.  The interlock" << ix( *this, "interlock" ) << " adjustment is disabled." << std::endl;
    return output;
}

std::ostream&
Help::pragmaCyclesDisallow( std::ostream& output, bool verbose ) const
{
    output << "Disallow cycles in the call graph." << std::endl;
    return output;
}

std::ostream&
Help::pragmaForceMultiserverNone( std::ostream& output, bool verbose ) const
{
    output << "Use fixed-rate servers whenever a server multiplicity is one (1)." << std::endl
	   << "Note that fixed-rate" << ix( *this, "server!fixed-rate!variance" ) << "servers with variance" << std::endl
	   << "may have results that differ from fixed-rate servers that don't and that the" << std::endl
	   << "multiserver servers never take variance into consideration." << std::endl;
    return output;
}

std::ostream&
Help::pragmaForceMultiserverProcessors( std::ostream& output, bool verbose ) const
{
    output << "Always use a multiserver solution for non-delay processors even if the number of servers is one (1)." << std::endl
	   << "The Rolia mutliserver approximation" << ix( *this, "mutliserver!rolia" ) << "is known to fail for this case." << std::endl;
    return output;
}

std::ostream&
Help::pragmaForceMultiserverTasks( std::ostream& output, bool verbose ) const
{
    output << "Always use a multiserver solution for non-delay server tasks even if the number of servers is one (1)." << std::endl
	   << "The Rolia mutliserver approximation" << ix( *this, "mutliserver!rolia" ) << "is known to fail for this case." << std::endl;
    return output;
}

std::ostream&
Help::pragmaForceMultiserverAll( std::ostream& output, bool verbose ) const
{
    output << "Always use a multiserver solution for non-delay servers (tasks and processors) even if the number of servers is one (1)." << std::endl
	   << "The Rolia mutliserver approximation" << ix( *this, "mutliserver!rolia" ) << "is known to fail for this case." << std::endl;
    return output;
}

std::ostream& 
Help::pragmaStopOnMessageLossFalse( std::ostream& output, bool verbose ) const
{
    output << "Ignore queue overflows" << ix( *this, "overflow" ) << " for open arrivals" << ix( *this, "open arrival!overflow" ) << " and send-no-reply" << ix( *this, "send-no-reply!overflow" ) << " requests.  If a queue overflows, its waiting times is reported as infinite." << ix( *this, "infinity" ) << "";
    return output;
}

std::ostream& 
Help::pragmaStopOnMessageLossTrue( std::ostream& output, bool verbose ) const
{
    output << "Stop if messages are lost." << std::endl;
    return output;
}

std::ostream&
Help::pragmaInterlockThroughput( std::ostream& output, bool verbose ) const
{
    output << "Perform interlocking by adjusting throughputs." << std::endl;
    return output;
}

std::ostream&
Help::pragmaInterlockNone( std::ostream& output, bool verbose ) const
{
    output << "Do not perform interlock adjustment." << std::endl;
    return output;
}


std::ostream&
Help::pragmaLayeringBatched( std::ostream& output, bool verbose ) const
{
    output << "Batched layering" << ix( *this, "batched layers" ) << ix( *this, "layering!batched" ) << " -- solve layers composed of as many servers as possible from top to bottom." << std::endl;
    return output;
}

std::ostream&
Help::pragmaLayeringBatchedBack( std::ostream& output, bool verbose ) const
{
    output << "Batched layering with back propagation -- solve layers composed of as many servers as possible from top to bottom, then from bottom to top to improve solution speed."  << std::endl;
    return output;
}

std::ostream&
Help::pragmaLayeringHwSw( std::ostream& output, bool verbose ) const
{
  output << "Hardware/software layers" << ix( *this, "hardware-software layers" ) << ix( *this, "layers!hardware-software" ) << " -- The model is solved using two submodels:" << std::endl
	 << "One consisting solely of the tasks in the model, and the other with the tasks calling the processors." << std::endl;
    return output;
}

std::ostream&
Help::pragmaLayeringMOL( std::ostream& output, bool verbose ) const
{
    output << "Method Of layers" << ix( *this, "method of layers" ) << ix( *this, "layering!method of" ) << " -- solve layers using the Method of Layers" << cite( *this, "perf:rolia-95-ieeese-mol" ) << ix( *this, "Method of Layers" ) << ix( *this, "layering!Method of Layers" ) << ". Layer spanning is performed by allowing clients to appear in more than one layer." << std::endl;
    return output;
}

std::ostream&
Help::pragmaLayeringMOLBack( std::ostream& output, bool verbose ) const
{
    output << "Method Of layers -- solve layers using the Method of Layers.  Software submodels are solved top-down then bottom up to improve solution speed." << std::endl;
    return output;
}

std::ostream&
Help::pragmaLayeringSRVN( std::ostream& output, bool verbose ) const
{
    output << "SRVN layers" << ix( *this, "srvn layers" ) << ix( *this, "layering!srvn" ) << " -- solve layers composed of only one server." << std::endl
	   << "This method of solution is comparable to the technique used by the " << bold( *this, LQIO::DOM::Pragma::_srvn_ ) << " solver.  See also " << flag( *this, "P" ) << emph( *this, LQIO::DOM::Pragma::_mva_ ) << "." << std::endl;
    return output;
}

std::ostream&
Help::pragmaLayeringSquashed( std::ostream& output, bool verbose ) const
{
    output << "Squashed layers" << ix( *this, "squashed layers" ) << ix( *this, "layering!squashed" ) << " -- All the tasks and processors are placed into one submodel." << std::endl
	   << "Solution speed may suffer because this method generates the most number of chains in the MVA solution.  See also " << flag( *this, "P" ) << emph( *this, LQIO::DOM::Pragma::_mva_ ) << "." << std::endl;
    return output;
}

std::ostream&
Help::pragmaMultiServerDefault( std::ostream& output, bool verbose ) const
{
    output <<  "The default multiserver" << ix( *this, "multiserver!default" ) << " calculation uses the the Conway multiserver for multiservers with less than five servers, and the Rolia multiserver otherwise." << std::endl;
    return output;
}

std::ostream&
Help::pragmaMultiServerConway( std::ostream& output, bool verbose ) const
{
    output <<  "Use the Conway multiserver" << ix( *this, "multiserver!Conway" ) << cite( *this, "queue:deSouzaeSilva-87,queue:conway-88" ) << " calculation for all multiservers." << std::endl;
    return output;
}

std::ostream&
Help::pragmaMultiServerReiser( std::ostream& output, bool verbose ) const
{
    output <<  "Use the Reiser multiserver" << ix( *this, "multiserver!Reiser" ) << cite( *this, "queue:reiser-79" ) << " calculation for all multiservers." << std::endl;
    return output;
}

std::ostream&
Help::pragmaMultiServerReiserPS( std::ostream& output, bool verbose ) const
{
    output << "Use the Reiser multiserver calculation for all multiservers. For multiservers with multiple entries, scheduling is processor sharing" << ix( *this, "processor!sharing" ) << ", not FIFO. " << std::endl;
    return output;
}

std::ostream&
Help::pragmaMultiServerRolia( std::ostream& output, bool verbose ) const
{
    output <<  "Use the Rolia" << ix( *this, "multiserver!Rolia" ) << cite( *this, "perf:rolia-92,perf:rolia-95-ieeese-mol" ) << " multiserver calculation for all multiservers." << std::endl;
    return output;
}

std::ostream&
Help::pragmaMultiServerRoliaPS( std::ostream& output, bool verbose ) const
{
    output << "Use the Rolia multiserver calculation for all multiservers. For multiservers with multiple entries, scheduling is processor sharing" << ix( *this, "processor!sharing" ) << ", not FIFO. " << std::endl;
    return output;
}

std::ostream&
Help::pragmaMultiServerBruell( std::ostream& output, bool verbose ) const
{
    output <<  "Use the Bruell multiserver" << ix( *this, "multiserver!Bruell" ) << cite( *this, "queue:bruell-84-peva-load-dependent" ) << " calculation for all multiservers." << std::endl;
    return output;
}

std::ostream&
Help::pragmaMultiServerSchmidt( std::ostream& output, bool verbose ) const
{
    output <<  "Use the Schmidt multiserver" << ix( *this, "multiserver!Schmidt" ) << cite( *this, "queue:schmidt-97" ) << " calculation for all multiservers." << std::endl;
    return output;
}

std::ostream&
Help::pragmaMultiServerSuri( std::ostream& output, bool verbose ) const
{
    output <<  "experimental." << std::endl;
    return output;
}

std::ostream&
Help::pragmaMVALinearizer( std::ostream& output, bool verbose ) const
{
    output << "Linearizer." << ix( *this, "MVA!Linearizer" ) << ix( *this, "Linearizer" ) << std::endl;
    return output;
}

std::ostream&
Help::pragmaMVAExact( std::ostream& output, bool verbose ) const
{
    output << "Exact MVA" << ix( *this, "MVA!exact" ) << ".  Not suitable for large systems." << std::endl;
    return output;
}

std::ostream&
Help::pragmaMVASchweitzer( std::ostream& output, bool verbose ) const
{
    output << "Bard-Schweitzer approximate MVA." << ix( *this, "MVA!Bard-Schweitzer" ) << ix( *this, "Bard-Schweitzer" ) << "" << std::endl;
    return output;
}

std::ostream&
Help::pragmaMVAFast( std::ostream& output, bool verbose ) const
{
    output << "Fast Linearizer" << std::endl;
    return output;
}

std::ostream&
Help::pragmaMVAOneStep( std::ostream& output, bool verbose ) const
{
    output << "Perform one step of Bard Schweitzer approximate MVA for each iteration of a submodel.  The default is to perform Bard Schweitzer approximate MVA until convergence for each submodel.  This option, combined with " << flag( *this, "P" ) << emph( *this, "layering=srvn") << " most closely approximates the solution technique used by the " << bold( *this, LQIO::DOM::Pragma::_srvn_ ) << " solver." << std::endl;
    return output;
}

std::ostream&
Help::pragmaMVAOneStepLinearizer( std::ostream& output, bool verbose ) const
{
    output << "Perform one step of Linearizer approximate MVA for each iteration of a submodel.  The default is to perform Linearizer approximate MVA until convergence for each submodel." << std::endl;
    return output;
}

std::ostream&
Help::pragmaOvertakingMarkov( std::ostream& output, bool verbose ) const
{
    output << "Markov phase 2 calculation." << ix( *this, "overtaking!Markov" ) << std::endl;
    return output;
}

std::ostream&
Help::pragmaOvertakingRolia( std::ostream& output, bool verbose ) const
{
    output << "Use the method from the Method of Layers." << ix( *this, "overtaking!Method of Layers" ) << std::endl;
    return output;
}

std::ostream&
Help::pragmaOvertakingSimple( std::ostream& output, bool verbose ) const
{
    output << "Simpler, but faster approximation." << std::endl;
    return output;
}

std::ostream&
Help::pragmaOvertakingSpecial( std::ostream& output, bool verbose ) const
{
    output << "?" << std::endl;
    return output;
}

std::ostream&
Help::pragmaOvertakingNone( std::ostream& output, bool verbose ) const
{
    output << "Disable all second phase servers.  All stations are modeled as having a single phase by summing the phase information." << std::endl;
    return output;
}

std::ostream&
Help::pragmaProcessorDefault( std::ostream& output, bool verbose ) const
{
    output << "The default is to use the processor scheduling specified in the model." << std::endl;
    return output;
}

std::ostream&
Help::pragmaProcessorFCFS( std::ostream& output, bool verbose ) const
{
    output << "All uni-processors are scheduled first-come, first-served." << std::endl;
    return output;
}

std::ostream&
Help::pragmaProcessorHOL( std::ostream& output, bool verbose ) const
{
    output << "All uni-processors are scheduled using head-of-line priority." << ix( *this, "priority!head of line" ) << std::endl;
    return output;
}

std::ostream&
Help::pragmaProcessorPPR( std::ostream& output, bool verbose ) const
{
    output << "All uni-processors are scheduled using priority, pre-emptive resume." << ix( *this, "priority!preemptive-resume" ) << std::endl;
    return output;
}

std::ostream&
Help::pragmaProcessorPS( std::ostream& output, bool verbose ) const
{
    output << "All uni-processors are scheduled using processor sharing." << ix( *this, "processor sharing" ) << std::endl;
    return output;
}

std::ostream&
Help::pragmaThreadsNone( std::ostream& output, bool verbose ) const
{
    output << "Do not perform overlap calculation for forks." << ix( *this, "overlap calculation" ) << std::endl;
    return output;
}

std::ostream&
Help::pragmaThreadsMak( std::ostream& output, bool verbose ) const
{
    output << "Use Mak-Lundstrom" << ix( *this, "Mak-Lundstrom" ) << cite( *this, "perf:mak-90" ) << " approximations for join delays." << ix( *this, "join!delay" ) << std::endl;
    return output;
}

std::ostream&
Help::pragmaThreadsHyper( std::ostream& output, bool verbose ) const
{
    output << "Inflate overlap probabilities based on arrival instant estimates." << std::endl;
    return output;
}

std::ostream&
Help::pragmaThreadsExponential( std::ostream& output, bool verbose ) const
{
    output << "Use exponential values instead of three-point approximations in all approximations" << ix( *this, "three-point approximation" ) << cite( *this, "perf:jiang-96" ) << "." << std::endl;
    return output;
}


std::ostream&
Help::pragmaThreadsDefault( std::ostream& output, bool verbose ) const
{
    output << "?" << std::endl;
    return output;
}

std::ostream& 
Help::pragmaVarianceDefault( std::ostream& output, bool verbose ) const
{
    return output;
}

std::ostream&
Help::pragmaVarianceNone( std::ostream& output, bool verbose ) const
{
    output << "Disable variance adjustment.  All stations in the MVA submodels are either delay- or FIFO-servers." << std::endl;
    return output;
}

std::ostream&
Help::pragmaVarianceStochastic( std::ostream& output, bool verbose ) const
{
    output << "?" << std::endl;
    return output;
}

std::ostream&
Help::pragmaVarianceMol( std::ostream& output, bool verbose ) const
{
    output << "Use the MOL variance calculation." << ix( *this, "variance!Method of Layers" ) << ix( *this, "Method of Layers!variance" ) << std::endl;
    return output;
}

std::ostream&
Help::pragmaVarianceNoEntry( std::ostream& output, bool verbose ) const
{
    output << "By default, any task with more than one entry will use the variance calculation.  This pragma will switch off the variance calculation for tasks with only one entry." << std::endl;
    return output;
}

std::ostream&
Help::pragmaVarianceInitOnly( std::ostream& output, bool verbose ) const
{
    output << "Initialize the variances, but don't recompute as the model is solved." << ix( *this, "variance!initialize only" ) << std::endl;
    return output;
}

std::ostream& 
Help::pragmaSeverityLevelWarnings( std::ostream& output, bool verbose ) const
{
    return output;
}

std::ostream& 
Help::pragmaSeverityLevelRunTime( std::ostream& output, bool verbose ) const
{
    return output;
}

std::ostream& 
Help::pragmaSpexHeaderFalse( std::ostream& output, bool verbose ) const
{
    output << "Do not output a header line (the output can then be fed into gnuplot easily)." << std::endl;
    return output;
}

std::ostream& 
Help::pragmaSpexHeaderTrue( std::ostream& output, bool verbose ) const
{
    output << "Output a header line consisting of the names of all of the variables used in the Result section on the input file." << std::endl;
    return output;
}

std::ostream& 
Help::pragmaPruneFalse( std::ostream& output, bool verbose ) const
{
    output << "Solve model with all processors present." << std::endl;
    return output;
}

std::ostream& 
Help::pragmaPruneTrue( std::ostream& output, bool verbose ) const
{
    output << "Solve model without including \"useless\" processors." << std::endl;
    return output;
}

#if HAVE_LIBGSL && HAVE_LIBGSLCBLAS
std::ostream&
Help::pragmaQuorumDistributionDefault( std::ostream& output, bool verbose ) const
{
    output <<  "use default quorum settings" << std::endl;
    return output;
}

std::ostream&
Help::pragmaQuorumDistributionThreepoint( std::ostream& output, bool verbose ) const
{
    output <<  "estimation of a thread service time using a threepoint distribution." << std::endl;
    return output;
}

std::ostream&
Help::pragmaQuorumDistributionGamma( std::ostream& output, bool verbose ) const
{
    output <<  "estimation of a thread service time using a Gamma distribution" << std::endl;
    return output;
}

std::ostream&
Help::pragmaQuorumDistributionClosedFormGeo( std::ostream& output, bool verbose ) const
{
    output << "estimation of a thread service time using a closed-form  for geometric calls formula distribution." << std::endl;
    return output;
}

std::ostream&
Help::pragmaQuorumDistributionClosedformDet( std::ostream& output, bool verbose ) const
{
    output << "estimation of a thread service time using a closed-form  for deterministic calls formula distribution." << std::endl;
    return output;
}

std::ostream&
Help::pragmaDelayedThreadsKeepAll( std::ostream& output, bool verbose ) const
{
    output <<  "keep both delayed local calls and delayed remote calls running after the quorum join." << std::endl;
    return output;
}

std::ostream&
Help::pragmaDelayedThreadsAbortAll( std::ostream& output, bool verbose ) const
{
    output <<  "abort both delayed local calls and delayed remote calls after the quorum join." << std::endl;
    return output;
}

std::ostream&
Help::pragmaDelayedThreadsAbortLocalOnly( std::ostream& output, bool verbose ) const
{
    output <<  "abort delayed local calls and keep delayed remote calls running after the quorum join." << std::endl;
    return output;
}

std::ostream&
Help::pragmaDelayedThreadsAbortRemoteOnly( std::ostream& output, bool verbose ) const
{
    output <<  "abort delayed remote calls and keep delayed local calls running after the quorum join." << std::endl;
    return output;
}

std::ostream&
Help::pragmaIdleTimeDefault( std::ostream& output, bool verbose ) const
{
    output <<  "use default idle settings" << std::endl;
    return output;
}

std::ostream&
Help::pragmaIdleTimeJoindelay( std::ostream& output, bool verbose ) const
{
    output <<  "Use And-Fork join delays throughputs to set threads idle times." << std::endl;
    return output;
}

std::ostream&
Help::pragmaIdleTimeRootentry( std::ostream& output, bool verbose ) const
{
    output <<  "Use root Entry throughput to set threads idle times." << std::endl;
    return output;
}
#endif

/* -------------------------------------------------------------------- */

const char * HelpTroff::__comment = ".\\\"";

std::ostream&
HelpTroff::preamble( std::ostream& output ) const
{
    char date[32];
    time_t tloc;
    time( &tloc );

#if defined(HAVE_CTIME)
    strftime( date, 32, "%d %B %Y", localtime( &tloc ) );
#endif

    output << __comment << " t -*- nroff -*-" << std::endl
	   << ".TH lqns 1 \"" << date << "\" \"" << VERSION << "\"" << std::endl;

    output << __comment << " $Id: help.cc 14174 2020-12-07 16:59:53Z greg $" << std::endl
	   << __comment << std::endl
	   << __comment << " --------------------------------" << std::endl;

    output << ".SH \"NAME\"" << std::endl;
    output << "lqns \\- solve layered queueing network models."
	   << std::endl;

    output << ".SH \"SYNOPSIS\"" << std::endl
	 << ".br" << std::endl
	 << ".B lqns" << std::endl
	 << "[\\fIOPTIONS\\fR].\\|.\\|. "
	 << "[\\fIFILE\\fR] \\&.\\|.\\|." << std::endl;

    output << ".SH \"DESCRIPTION\"" << std::endl;
    output << bold( *this, "Lqns" ) << ix( *this, "LQNS" ) << " is used to solve layered queueing network models using " << std::endl
	   << "analytic techniques.  Models can be specified using the LQN modeling" << std::endl
	   << "language, or with extensible markup language (XML).  Refer to the" << std::endl
	   << emph( *this, "``Layered Queueing Network Solver and Simulator User Manual''" ) << std::endl
	   << "for details of the model and for a complete description of the input file" << std::endl
	   << "formats for the program." << std::endl;
    pp( output );
    return output;
}


std::ostream&
HelpTroff::see_also( std::ostream& output ) const
{
    section( output, "SEE ALSO", "Refereneces" );
    output << "Greg Franks el. al., ``Enhanced Modeling and Solution of Layered" << std::endl
	   << "Queueing Networks'', " << emph( *this, "IEEE Trans. Soft. Eng." ) << ", Vol. 35, No. 2, Mar-Apr 2990, pp. 148-161." << std::endl;
    br( output );
    output << "C. M. Woodside et. al., ``The Stochastic Rendezvous Network" << std::endl
	   << "Model for Performance of Synchronous Multi-tasking Distributed" << std::endl
	   << "Software'', " << emph( *this, "IEEE Trans. Comp." ) << ", Vol. 44, No. 8, Aug 1995, pp. 20-34." << std::endl;
    br( output );
    output << "J. A. Rolia and K. A. Sevcik, ``The Method of Layers'', " << emph( *this, "IEEE Trans. SE" )
	   << ", Vol. 21, No. 8, Aug. 1995, pp 689-700." << std::endl;
    br( output );
    output << emph( *this, "``Layered Queueing Network Solver and Simulator User Manual''") << std::endl;
    br( output );
    output << emph( *this, "``Tutorial Introduction to Layered Modeling of Software Performance''" ) << std::endl;
    br( output );
    output << "lqsim(1), lqn2ps(1), srvndiff(1), egrep(1)," << std::endl
	   << "floating_point(3)" << std::endl;
    return output;
}



std::ostream&
HelpTroff::textbf( std::ostream& output, const char * s ) const
{
    output << "\\fB" << s << "\\fP";
    return output;
}


std::ostream&
HelpTroff::textit( std::ostream& output, const char * s ) const
{
    output << "\\fI" << s << "\\fP";
    return output;
}

std::ostream&
HelpTroff::filename( std::ostream& output, const char * s1, const char * s2 ) const
{
    output << "\\fI" << s1;
    if ( s2 ) {
	output << "\\fB" << s2;
    }
    output << "\\fR";
    return output;
}


std::ostream&
HelpTroff::pp( std::ostream& output ) const
{
    output << ".PP" << std::endl;
    return output;
}

std::ostream&
HelpTroff::br( std::ostream& output ) const
{
    output << ".LP" << std::endl;
    return output;
}

std::ostream&
HelpTroff::ol_begin( std::ostream& output ) const
{
    return output;
}


std::ostream&
HelpTroff::ol_end( std::ostream& output ) const
{
    return output;
}


std::ostream&
HelpTroff::dl_begin( std::ostream& output ) const
{
    return output;
}


std::ostream&
HelpTroff::dl_end( std::ostream& output ) const
{
    return output;
}


std::ostream&
HelpTroff::li( std::ostream& output, const char * s  ) const
{
    output << ".TP 3" << std::endl;
    if ( s ) output << s << std::endl;
    return output;
}

std::ostream&
HelpTroff::flag( std::ostream& output, const char * s ) const
{
    output << "\\fB\\-" << s << "\\fP";
    return output;
}

std::ostream&
HelpTroff::section( std::ostream& output, const char * s, const char * ) const
{
    output << ".SH \"" << s << "\"" << std::endl;
    return output;
}

std::ostream&
HelpTroff::label( std::ostream& output, const char * s ) const
{
    return output;
}

std::ostream&
HelpTroff::longopt( std::ostream& output, const struct option *o ) const
{
    output << ".TP" << std::endl;
    if ( isgraph(o->val) ) {
	const char b[2] = { static_cast<char>(o->val), '\0' };
	flag( output, &b[0] );
	if ( o->name ) {
	    output << ", ";
	}
    }
    if ( o->name ) {
	output << "\\fB\\-\\-" << o->name << "\\fR";
    }

    if ( o->has_arg ) {
	output << "=\\fIarg\\fR";
    }
    output << std::endl;
    return output;
}


std::ostream&
HelpTroff::increase_indent( std::ostream& output ) const
{
    output << ".RS" << std::endl;
    return output;
}

std::ostream&
HelpTroff::decrease_indent( std::ostream& output ) const
{
    output << ".RE" << std::endl;
    return output;
}

std::ostream&
HelpTroff::print_option( std::ostream& output, const char * name, const Options::Option& opt ) const
{
    output << ".TP" << std::endl
	   << "\\fB" << name;
    if ( opt.hasArg() ) {
	output << "\\fR=\\fI" << "arg";
    }
    output << "\\fR" << std::endl;
    help_fptr h = opt.help();
    if ( h ) {
	(this->*h)(output,true);
    }
    return output;
}

std::ostream&
HelpTroff::print_pragma( std::ostream& output, const std::string& name ) const
{
    const pragma_map_t::const_iterator pragma = __pragmas.find( name );
    if ( pragma == __pragmas.end() ) return output;

    const parameter_map_t* value = pragma->second._value;
    std::string default_param;

    output << ".TP" << std::endl
	   << "\\fB" << name << "\\fR=\\fIarg";
    /* Enumeration */
    output << "\\fR" << std::endl;
    (this->*(pragma->second._help))( output, true );
    /* Comment */
    if ( value && value->size() > 1 ) {
	Help::help_fptr h = nullptr;
	output << ".RS" << std::endl;
	for ( parameter_map_t::const_iterator arg = value->begin(); arg != value->end(); ++arg ) {
	    if ( arg->first == LQIO::DOM::Pragma::_default_ ) {
		h = arg->second._help;
		continue;
	    } else {
		output << ".TP" << std::endl;
		output << "\\fB" << arg->first << "\\fP" << std::endl;
		(this->*(arg->second._help))( output, true );
		if ( arg->second._default == true ) {
		    default_param = arg->first;
		}
	    }
	}
	output << ".LP" << std::endl;
	if ( h ) {
	    (this->*h)( output, true ) << std::endl;
	} else if ( default_param.size() > 0 ) {
	    output << "The default is " << default_param << "." << std::endl;
	}
	output << ".RE" << std::endl;
    }
    return output;
}

std::ostream&
HelpTroff::table_header( std::ostream& output ) const
{
    output << __comment << "--------------------------------------------------------------------" << std::endl
	   << __comment << " Table Begin" << std::endl
	   << __comment << "--------------------------------------------------------------------" << std::endl
	   << ".ne 20" << std::endl
	   << ".TS" << std::endl
	   << "center tab (&) ;" << std::endl
	   << "lw(30x) le ." << std::endl
	   << "Parameter&lqns" << std::endl
	   << "=" << std::endl;
    return output;
}


std::ostream&
HelpTroff::table_row( std::ostream& output, const char * col1, const char * col2, const char * index ) const
{
    output << "T{" << std::endl << col1 << std::endl << "T}&T{" << std::endl << col2 << std::endl << "T}" << std::endl;
    return output;
}

std::ostream&
HelpTroff::table_footer( std::ostream& output ) const
{
    output << "_" << std::endl
	   << ".TE" << std::endl;
    return output;
}

/* -------------------------------------------------------------------- */

const char * HelpLaTeX::__comment = "%%";

std::ostream&
HelpLaTeX::preamble( std::ostream& output ) const
{
    char date[32];
#if defined(HAVE_CTIME)
    time_t tloc;
    time( &tloc );

    strftime( date, 32, "%d %B %Y", localtime( &tloc ) );
#endif

    output << __comment << "  -*- mode: latex; mode: outline-minor; fill-column: 108 -*- " << std::endl
	   << __comment << " Title:  lqns" << std::endl
	   << __comment << "" << std::endl
	   << __comment << " $HeadURL: http://rads-svn.sce.carleton.ca:8080/svn/lqn/trunk-V5/lqns/help.cc $" << std::endl
	   << __comment << " Original Author:     Greg Franks <greg@sce.carleton.ca>" << std::endl
	   << __comment << " Created:             " << date << std::endl
	   << __comment << "" << std::endl
	   << __comment << " ----------------------------------------------------------------------" << std::endl
	   << __comment << " $Id: help.cc 14174 2020-12-07 16:59:53Z greg $" << std::endl
	   << __comment << " ----------------------------------------------------------------------" << std::endl << std::endl;

    output << "\\chapter{Invoking the Analytic Solver ``lqns''}" << std::endl
	   << "\\label{sec:invoking-lqns}" << std::endl
	   << "The Layered Queueing Network Solver (LQNS)\\index{LQNS} is used to" << std::endl
	   << "solved Layered Queueing Network models analytically." << std::endl;

    return output;
}


std::ostream&
HelpLaTeX::textbf( std::ostream& output, const char * s ) const
{
    output << "\\textbf{" << s << "}";
    return output;
}


std::ostream&
HelpLaTeX::textit( std::ostream& output, const char * s ) const
{
    output << "\\emph{" << s << "}";
    return output;
}

std::ostream&
HelpLaTeX::tr_( std::ostream& output, const char * s ) const
{
    for ( ; *s; ++s ) {
	if ( *s == '_' ) {
	    output << "\\";
	}
	output << *s;
    }
    return output;
}


std::ostream&
HelpLaTeX::filename( std::ostream& output, const char * s1, const char * s2 ) const
{
    output << "\\texttt{" << s1;
    if ( s2 ) {
	output << s2;
    }
    output << "}";
    return output;
}

std::ostream&
HelpLaTeX::pp( std::ostream& output ) const
{
    output << std::endl << std::endl;
    return output;
}

std::ostream&
HelpLaTeX::br( std::ostream& output ) const
{
    output << "\\linebreak[4]" << std::endl;
    return output;
}

std::ostream&
HelpLaTeX::ol_begin( std::ostream& output ) const
{
    output << "\\begin{enumerate}" << std::endl;
    return output;
}


std::ostream&
HelpLaTeX::ol_end( std::ostream& output ) const
{
    output << "\\end{enumerate}" << std::endl;
    return output;
}


std::ostream&
HelpLaTeX::dl_begin( std::ostream& output ) const
{
    output << "\\begin{description}" << std::endl;
    return output;
}


std::ostream&
HelpLaTeX::dl_end( std::ostream& output ) const
{
    output << "\\end{description}" << std::endl;
    return output;
}


std::ostream&
HelpLaTeX::li( std::ostream& output, const char * s  ) const
{
    output << "\\item ";
    return output;
}

std::ostream&
HelpLaTeX::flag( std::ostream& output, const char * s ) const
{
    output << "\\flag{" << s << "}{}";
    return output;
}

std::ostream&
HelpLaTeX::ix( std::ostream& output, const char * s ) const
{
    output << "\\index{" << s << "}";
    return output;
}

std::ostream&
HelpLaTeX::cite( std::ostream& output, const char * s ) const
{
    output << "~\\cite{" << s << "}";
    return output;
}

std::ostream&
HelpLaTeX::section( std::ostream& output, const char *, const char * s ) const
{
    output << "\\section{" << s << "}" << std::endl;
    return output;
}

std::ostream&
HelpLaTeX::label( std::ostream& output, const char * s ) const
{
    output << "\\label{" << s << "}" << std::endl;
    return output;
}


std::ostream&
HelpLaTeX::longopt( std::ostream& output, const struct option *o ) const
{
    output << "\\item[";
    if ( isgraph(o->val) ) {
	output << "\\flag{" << static_cast<char>(o->val) << "}{}";
    }
    if ( o->name ) {
	if ( isgraph(o->val) ) output << ", ";
	output << "\\longopt{" << o->name << "}";
    }
    if ( o->has_arg ) {
	output << "=\\emph{arg}";
    }
    output << "]~\\\\" << std::endl;
    return output;
}

std::ostream&
HelpLaTeX::increase_indent( std::ostream& output ) const
{
    return output;
}

std::ostream&
HelpLaTeX::decrease_indent( std::ostream& output ) const
{
    return output;
}

std::ostream&
HelpLaTeX::print_option( std::ostream& output, const char * name, const Options::Option& opt ) const
{
    output << "\\item[\\optarg{" << tr_( *this, name ) << "}{";
    if ( opt.hasArg() ) {
	output << "=" << emph( *this, "arg" );
    }
    output << "}]" << std::endl;
    help_fptr f = opt.help();
    if ( f ) {
	(this->*f)(output, true);
    }
    return output;
}

std::ostream&
HelpLaTeX::print_pragma( std::ostream& output, const std::string& name ) const
{
    const std::map<std::string,Help::pragma_info>::const_iterator pragma = __pragmas.find( name );
    if ( pragma == __pragmas.end() ) return output;

    const parameter_map_t* value = pragma->second._value;
    std::string default_param;

    output << "\\item[\\optarg{" << tr_( *this, name.c_str() ) << "}{=\\emph{arg}}]~\\\\" << std::endl;
    (this->*(pragma->second._help))( output, true );
    /* Comment */
    if ( value && value->size() > 1 ) {
	Help::help_fptr h = nullptr;
	dl_begin( output );
	for ( parameter_map_t::const_iterator arg = value->begin(); arg != value->end(); ++arg ) {
	    if ( arg->first == LQIO::DOM::Pragma::_default_ ) {
		h = arg->second._help;
		continue;
	    } else {
		output << "\\item[\\optarg{" << tr_( *this, arg->first.c_str() ) << "}{}]" << std::endl;
		(this->*(arg->second._help))( output, true );
		if ( arg->second._default == true ) {
		    default_param = arg->first;
		}
	    }
	}
	dl_end( output );
	if ( h ) {
	    (this->*h)( output, true ) << std::endl;
	} else if ( default_param.size() > 0 ) {
	    output << "The default is " << default_param << "." << std::endl;
	}
    }
    return output;
}


std::ostream&
HelpLaTeX::table_header( std::ostream& output ) const
{
    output << __comment << "--------------------------------------------------------------------" << std::endl
	   << __comment << " Table Begin" << std::endl
	   << __comment << "--------------------------------------------------------------------" << std::endl
	   << "\\begin{table}[htbp]" << std::endl
	   << "  \\centering" << std::endl
	   << "  \\begin{tabular}[c]{ll}" << std::endl
	   << "    Parameter&lqns \\\\" << std::endl
	   << "    \\hline" << std::endl;
    return output;
}


std::ostream&
HelpLaTeX::table_row( std::ostream& output, const char * col1, const char * col2, const char * index ) const
{
    output << "    " << col1;
    if ( index ) {
	output << "\\index{ " << index << "}";
    }
    output << " & " << col2 << "\\\\" << std::endl;
    return output;
}


std::ostream&
HelpLaTeX::table_footer( std::ostream& output ) const
{
    output << "    \\hline" << std::endl
	   << "  \\end{tabular}" << std::endl
	   << "  \\caption{\\label{tab:lqns-model-limits}LQNS Model Limits\\index{limits!lqns}.}" << std::endl
	   << "\\end{table}" << std::endl;
    return output;
}


std::ostream&
HelpLaTeX::trailer( std::ostream& output ) const
{
    output << "%%% Local Variables: " << std::endl
	   << "%%% mode: latex" << std::endl
	   << "%%% mode: outline-minor " << std::endl
	   << "%%% fill-column: 108" << std::endl
	   << "%%% TeX-master: \"userman\"" << std::endl
	   << "%%% End: " << std::endl;
    return output;
}

/* -------------------------------------------------------------------- */

std::ostream&
HelpPlain::print_option( std::ostream& output, const char * name, const Options::Option& opt ) const
{
    std::ios_base::fmtflags oldFlags = output.setf( std::ios::left, std::ios::adjustfield );
    std::string s = name;
    if ( opt.hasArg() ) {
	s += "=arg";
    }
    output << " " << std::setw(26) << s << " ";
    help_fptr h = opt.help();
    if ( h ) {
	(this->*h)(output,false);
    }
    output.flags(oldFlags);
    return output;
}

std::ostream&
HelpPlain::print_pragma( std::ostream& output, const std::string& ) const
{
    return output;
}


std::ostream&
HelpPlain::textbf( std::ostream& output, const char * s ) const
{
    output << s;
    return output;
}


std::ostream&
HelpPlain::textit( std::ostream& output, const char * s ) const
{
    output << s;
    return output;
}

std::ostream&
HelpPlain::filename( std::ostream& output, const char * s1, const char * s2 ) const
{
    output << s1;
    if ( s2 ) {
	output << s2;
    }
    return output;
}

void
HelpPlain::print_special( std::ostream& output ) 
{
    Options::Special::initialize();
    HelpPlain self;

    output << "Valid arguments for --special" << std::endl;
    for ( std::map<const char *, Options::Special, lt_str>::const_iterator tp = Options::Special::__table.begin(); tp != Options::Special::__table.end(); ++tp ) {
	self.print_option( output, tp->first, tp->second );
    }
}


void
HelpPlain::print_trace( std::ostream& output ) 
{
    Options::Trace::initialize();
    HelpPlain self;
    output << "Valid arguments for --trace" << std::endl;
    for ( std::map<const char *, Options::Trace, lt_str>::const_iterator tp = Options::Trace::__table.begin(); tp != Options::Trace::__table.end(); ++tp ) {
	self.print_option( output, tp->first, tp->second );
    }
}


void
HelpPlain::print_debug( std::ostream& output ) 
{
    Options::Debug::initialize();
    HelpPlain self;
    output << "Valid arguments for --debug" << std::endl;
    for ( std::map<const char *, Options::Debug, lt_str>::const_iterator tp = Options::Debug::__table.begin(); tp != Options::Debug::__table.end(); ++tp ) {
	self.print_option( output, tp->first, tp->second );
    }
}
