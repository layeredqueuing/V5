/*  -*- c++ -*-
 * $Id: lqns.cc 15275 2021-12-27 19:25:01Z greg $
 *
 * Command line processing.
 *
 * Copyright the Real-Time and Distributed Systems Group,
 * Department of Systems and Computer Engineering,
 * Carleton University, Ottawa, Ontario, Canada. K1S 5B6
 *
 * November, 1994
 *
 * ------------------------------------------------------------------------
 */

#include "lqns.h"
#include <sstream>
#include <fstream>
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <errno.h>
#include <libgen.h>
#include <lqio/filename.h>
#include <lqio/commandline.h>
#include <lqio/srvn_spex.h>
#include <lqio/dom_pragma.h>
#if HAVE_GETOPT_H
#include <getopt.h>
#endif
#if !defined(HAVE_GETSUBOPT)
#include <lqio/getsbopt.h>
#endif
#include <mva/fpgoop.h>
#include "errmsg.h"
#include "generate.h"
#include "help.h"
#include "flags.h"
#include "model.h"
#include "option.h"
#include "pragma.h"

extern "C" int LQIO_debug;

static char copyrightDate[20];

/* -- */

struct FLAGS flags;

#if HAVE_GETOPT_LONG
const struct option longopts[] =
    /* name */ /* has arg */ /*flag */ /* val */
{
    { "no-advisories",				no_argument,	   nullptr, 'a' },
    { "bounds-only",				no_argument,	   nullptr, 'b' },
    { LQIO::DOM::Pragma::_convergence_value_,	required_argument, nullptr, 'c' },
    { "debug",					required_argument, nullptr, 'd' },
    { "error",					required_argument, nullptr, 'e' },
    { LQIO::DOM::Pragma::_fast_,		no_argument,	   nullptr, 'f' },
    { "help",					optional_argument, nullptr, 'H' },
    { "huge",					no_argument,	   nullptr, 'h' },
    { LQIO::DOM::Pragma::_iteration_limit_,	required_argument, nullptr, 'i' },
    { "json",					no_argument,	   nullptr, 'j' },
    { "input-format",				required_argument, nullptr, 'I' },
    { "no-execute",				no_argument,	   nullptr, 'n' },
    { "output",					required_argument, nullptr, 'o' },
    { "parseable",				no_argument,	   nullptr, 'p' },
    { "pragma",					required_argument, nullptr, 'P' },
    { "rtf",					no_argument,	   nullptr, 'r' },
    { "trace",					required_argument, nullptr, 't' },
    { LQIO::DOM::Pragma::_underrelaxation_,	required_argument, nullptr, 'u' },
    { "verbose",				no_argument,	   nullptr, 'v' },
    { "version",				no_argument,	   nullptr, 'V' },
    { "no-warnings",				no_argument,	   nullptr, 'w' },
    { "xml",					no_argument,	   nullptr, 'x' },
    { "special",				required_argument, nullptr, 'z' },
    { LQIO::DOM::Pragma::_exact_,		no_argument,	   nullptr, 256+'e' },
    { LQIO::DOM::Pragma::_schweitzer_,		no_argument,	   nullptr, 256+'s' },
    { "batch-layering",				no_argument,	   nullptr, 256+'b' },	/* NOP */
    { "hwsw-layering",				no_argument,	   nullptr, 256+'h' },
    { "method-of-layers",			no_argument,	   nullptr, 256+'m' },
    { "squashed-layering",			no_argument,	   nullptr, 256+'z' },
    { "srvn-layering",				no_argument,	   nullptr, 256+'l' },
    { "processor-sharing",			no_argument,	   nullptr, 256+'p' },
#if HAVE_LIBGSL && HAVE_LIBGSLCBLAS
    { "quorum",					no_argument,	   nullptr, 256+'q' },
#endif
    { "no-stop-on-message-loss",		no_argument,	   nullptr, 256+'o' },	/* Ignore open */
    { "no-variance",				no_argument,	   nullptr, 256+'v' },
    { "reload-lqx",				no_argument,	   nullptr, 512+'r' },
    { "restart",				no_argument,	   nullptr, 512+'R' },
    { "no-header",				no_argument,	   nullptr, 512+'h' },
    { "print-comment",				no_argument,	   nullptr, 512+'c' },
    { "print-interval",				optional_argument, nullptr, 512+'p' },
    { "reset-mva",				no_argument,	   nullptr, 256+'r' },
    { "trace-mva",				no_argument,	   nullptr, 256+'t' },
    { "debug-json",				no_argument,	   nullptr, 512+'j' },
    { "debug-lqx",				no_argument,	   nullptr, 512+'l' },
    { "debug-spex",				no_argument,	   nullptr, 512+'s' },
    { "debug-srvn",				no_argument,	   nullptr, 512+'y' },
    { "debug-xml",				no_argument,	   nullptr, 512+'x' },
    { nullptr, 0, nullptr, 0 }
};
#endif
const char opts[]       = "abc:d:e:fhH:i:I:jno:pP:rt:u:vVwxz:";

static void init_flags ();

#if (defined(linux) || defined(__linux__)) && !defined(__USE_XOPEN_EXTENDED)
extern "C" int getsubopt (char **, char * const *, char **);
#endif

/*
 * Main line.
 */

int main (int argc, char *argv[])
{
    std::string outputFileName = "";
    LQIO::CommandLine command_line;
    Options::Debug::initialize();
    Options::Trace::initialize();
    Options::Special::initialize();

    unsigned global_error_flag = 0;     /* Error detected anywhere??    */

    char * options;

    LQIO::io_vars.init( VERSION, basename( argv[0] ), severity_action, local_error_messages, LSTLCLERRMSG-LQIO::LSTGBLERRMSG );
    command_line = LQIO::io_vars.lq_toolname;

    sscanf( "$Date: 2020-12-31 10:17:22 -0500 (Thu, 31 Dec 2020) $", "%*s %s %*s", copyrightDate );

    matherr_disposition = FP_IMMEDIATE_ABORT;

    init_flags();

    pragmas.insert( getenv( "LQNS_PRAGMAS" ) );

    for ( ;; ) {
#if HAVE_GETOPT_LONG
        const int c = getopt_long( argc, argv, opts, longopts, NULL );
#else
        const int c = getopt( argc, argv, opts );
#endif
        if ( c == EOF) break;

        command_line.append( c, optarg );

	try {
	    switch ( c ) {
	    case 'a':
		pragmas.insert(LQIO::DOM::Pragma::_severity_level_,LQIO::DOM::Pragma::_run_time_);
		break;

	    case 'b':
		flags.bounds_only = true;
		break;

	    case 256+'b':
		break;

	    case 'c':
		Options::Special::convergence_value( optarg );
		break;

	    case 512+'c':
		/* Set immediately, as it can't be changed once the SPEX program is loaded */
		LQIO::Spex::__print_comment = true;
		pragmas.insert(LQIO::DOM::Pragma::_spex_comment_,"true");
		break;

	    case 'd':
		options = optarg;
		while ( *options ) {
		    char * value = nullptr;
		    const char * subopt = options;
		    const int ix = getsubopt( &options, Options::Debug::__options.data(), &value );
		    if ( ix >= 0 && value != nullptr ) {
			Options::Debug::exec( ix, (value == nullptr ? std::string("") : value) );
		    } else {
			throw std::invalid_argument( std::string("--debug=") + subopt );
		    }
		}
		break;

	    case 'e':                       /* Error handling.      */
		switch ( optarg[0] ) {
		case 'a':
		    matherr_disposition = FP_IMMEDIATE_ABORT;
		    break;

		case 'd':
		    matherr_disposition = FP_DEFERRED_ABORT;
		    break;

		case 'i':
		    matherr_disposition = FP_IGNORE;
		    break;

		case 'w':
		    matherr_disposition = FP_REPORT;
		    break;

		default:
		    std::cerr << LQIO::io_vars.lq_toolname << ": invalid argument to -e -- " << optarg << std::endl;
		    break;
		}
		break;

	    case 256+'e':
		pragmas.insert(LQIO::DOM::Pragma::_mva_,LQIO::DOM::Pragma::_exact_);
		break;

	    case 'f':
		pragmas.insert(LQIO::DOM::Pragma::_layering_,LQIO::DOM::Pragma::_batched_);
		pragmas.insert(LQIO::DOM::Pragma::_mva_,LQIO::DOM::Pragma::_one_step_linearizer_);
		pragmas.insert(LQIO::DOM::Pragma::_multiserver_,LQIO::DOM::Pragma::_conway_);
		break;

	    case 'h':
		pragmas.insert(LQIO::DOM::Pragma::_interlocking_,LQIO::DOM::Pragma::_no_);
		pragmas.insert(LQIO::DOM::Pragma::_mva_,LQIO::DOM::Pragma::_one_step_);
		pragmas.insert(LQIO::DOM::Pragma::_multiserver_,LQIO::DOM::Pragma::_rolia_);
		break;

	    case 'H':
		usage( optarg );
		exit(0);

	    case 256+'h':
		pragmas.insert(LQIO::DOM::Pragma::_layering_,LQIO::DOM::Pragma::_hwsw_);
		break;

	    case 512+'h':
		/* Set immediately, as it can't be changed once the SPEX program is loaded */
		LQIO::Spex::__no_header = true;
		pragmas.insert(LQIO::DOM::Pragma::_spex_header_,"false");
		break;

	    case 'I':
		if ( strcasecmp( optarg, "xml" ) == 0 ) {
		    Model::input_format = LQIO::DOM::Document::InputFormat::XML;
		} else if ( strcasecmp( optarg, "lqn" ) == 0 ) {
		    Model::input_format = LQIO::DOM::Document::InputFormat::LQN;
		} else if ( strcasecmp( optarg, "json" ) == 0 ) {
		    Model::input_format = LQIO::DOM::Document::InputFormat::JSON;
		} else {
		    std::cerr << LQIO::io_vars.lq_toolname << ": invalid argument to -I -- " << optarg << std::endl;
		    exit( 1 );
		}
		break;

	    case 'i':
		Options::Special::iteration_limit( optarg );
		break;

	    case 'j':
		flags.json_output = true;
		break;

	    case 256+'l':
		pragmas.insert(LQIO::DOM::Pragma::_layering_,LQIO::DOM::Pragma::_srvn_);
		break;

	    case (512+'l'):
		LQIO::DOM::Document::lqx_parser_trace(stderr);
		break;

	    case 256+'m':
		pragmas.insert(LQIO::DOM::Pragma::_layering_,LQIO::DOM::Pragma::_mol_);
		break;

	    case 'n':
		flags.no_execute = true;
		break;

	    case 'o':
		outputFileName = optarg;
		break;

	    case 256+'o':
		pragmas.insert(LQIO::DOM::Pragma::_stop_on_message_loss_,LQIO::DOM::Pragma::_no_);
		break;

	    case 'p':
		flags.parseable_output = true;
		break;

	    case 'P':       /* Pragma processing... */
		if ( !pragmas.insert( optarg ) ) {
		    Pragma::usage( std::cerr );
		    exit( INVALID_ARGUMENT );
		}
		break;

	    case 256+'p':
		pragmas.insert( LQIO::DOM::Pragma::_processor_scheduling_, scheduling_label[SCHEDULE_PS].XML );
		break;

	    case 512+'p':
		Options::Special::print_interval( optarg );
		break;

	    case 256+'q': //tomari quorum options
		flags.disable_expanding_quorum_tree = true;
		break;

	    case 'r':
		flags.rtf_output = true;
		break;

	    case 256+'r':
		flags.reset_mva = true;
		break;

	    case 512+'r':
		flags.reload_only = true;
		break;

	    case 512+'R':
		flags.restart = true;
		break;

	    case 256+'s':
		pragmas.insert( LQIO::DOM::Pragma::_mva_, LQIO::DOM::Pragma::_schweitzer_ );
		break;

	    case 512+'s':
		flags.print_lqx = true;
		break;

	    case 't':
		options = optarg;
		while ( *options ) {
		    char * value = nullptr;
		    const char * subopt = options;
		    const int ix = getsubopt( &options, Options::Debug::__options.data(), &value );
		    if ( ix >= 0 ) {
			Options::Trace::exec( ix, (value != nullptr ? value : std::string("") ) );
		    } else {
			throw std::invalid_argument( std::string("--trace=") + subopt );
		    }
		}
		break;

	    case 256+'t':
		flags.trace_mva = true;
		break;

	    case 'u':
		Options::Special::underrelaxation( optarg );
		break;

	    case 'v':
		flags.verbose = true;
		LQIO::Spex::__verbose = true;
		break;

	    case 'V':
		std::cout << "Layered Queueing Network Analyser, Version " << VERSION << std::endl << std::endl;
		std::cout << "  Copyright " << copyrightDate << " the Real-Time and Distributed Systems Group," << std::endl;
		std::cout << "  Department of Systems and Computer Engineering," << std::endl;
		std::cout << "  Carleton University, Ottawa, Ontario, Canada. K1S 5B6" << std::endl << std::endl;
		break;

	    case (256+'v'):
		pragmas.insert(LQIO::DOM::Pragma::_variance_, LQIO::DOM::Pragma::_none_);
		break;

	    case 'w':
		pragmas.insert(LQIO::DOM::Pragma::_severity_level_,LQIO::DOM::Pragma::_advisory_);
		break;

	    case 'x':
		flags.xml_output = true;
		break;

	    case 512+'x':
		LQIO::DOM::Document::__debugXML = true;
		break;

	    case 512+'y':
		LQIO_debug = true;
		break;

	    case 'z':
		options = optarg;
		while ( *options ) {
		    char * value = nullptr;
		    const char * subopt = options;
		    const int ix = getsubopt( &options, Options::Special::__options.data(), &value );
		    if ( ix >= 0 ) {
			Options::Special::exec( ix, (value == nullptr ? "" : value) );
		    } else {
			throw std::invalid_argument( std::string("--special=") + subopt );
		    }
		}
		break;

	    case (256+'z'):
		pragmas.insert(LQIO::DOM::Pragma::_layering_,LQIO::DOM::Pragma::_squashed_);
		break;

	    default:
		usage();
		break;
	    }
	}
	catch ( const std::invalid_argument& e )
	{
	    std::cerr << LQIO::io_vars.lq_toolname << ": invalid argument to " << e.what() << "." << std::endl;
	    exit( 1 );
	}
    }
    LQIO::io_vars.lq_command_line = command_line.c_str();

    if ( flags.generate && flags.no_execute ) {
        std::cerr << LQIO::io_vars.lq_toolname << ": -n is incompatible with -zgenerate.  -zgenerate ignored." << std::endl;
    }

    if ( flags.reload_only && flags.restart ) {
	std::cerr << LQIO::io_vars.lq_toolname << ": --reload-lqx and --restart are mutually exclusive: --restart assumed."  << std::endl;
	flags.reload_only = false;
    }

    /* Process all command line arguments.  If none specified, then     */
    /* input is assumed to come in from stdin.                          */

    if ( optind == argc ) {

        /* If stdout is not a terminal route output to stdout.          */
        /* For pipelines.                                               */

        if ( outputFileName == "" && LQIO::Filename::isWriteableFile( fileno( stdout ) ) > 0 ) {
            outputFileName = "-";
        }

        global_error_flag = Model::create( "-", outputFileName );

    } else {

        const int file_count = argc - optind;           /* Number of files on cmd line  */

        if ( file_count > 1 ) {
            if ( outputFileName != "" ) {
                std::cerr << LQIO::io_vars.lq_toolname << ": Too many input files specified with the option: -o"
                     << outputFileName
                     << std::endl;
                exit( INVALID_ARGUMENT );
            }
            if ( Generate::file_name.size() ) {
                std::cerr << LQIO::io_vars.lq_toolname << ": Too many input files specified with the option: -zgenerate="
                     << Generate::file_name
                     << std::endl;
                exit( INVALID_ARGUMENT );
            }
        }

        for ( ; optind < argc; ++optind ) {
            if ( file_count > 1 ) {
                std::cout << argv[optind] << ':' << std::endl;
            }
            global_error_flag |= Model::create( argv[optind], outputFileName );
        }
    }

    return global_error_flag;
}



/*
 * Initialize flags to their default values;
 */

static
void init_flags()
{
    flags.no_execute            = false;
    flags.bounds_only           = false;
    flags.parseable_output      = false;
    flags.rtf_output            = false;
    flags.xml_output            = false;
    flags.generate              = false;
    flags.reset_mva		= false;
    flags.print_overtaking      = false;
    flags.single_step           = false;
    flags.print_lqx		= false;

    flags.trace_activities      = false;
    flags.trace_convergence     = false;
    flags.trace_customers	= false;
    flags.trace_delta_wait      = false;
    flags.trace_forks           = false;
    flags.trace_idle_time       = false;
    flags.trace_interlock       = false;
    flags.trace_intermediate    = false;
    flags.trace_joins           = false;
    flags.trace_mva             = false;
    flags.trace_overtaking      = false;
    flags.trace_replication     = false;
    flags.trace_virtual_entry   = false;
    flags.trace_wait            = false;

#if HAVE_LIBGSL && HAVE_LIBGSLCBLAS
    flags.min_steps             = 7;            /* Default of 2 steps. */
#else
    flags.min_steps             = 2;            /* Default of 2 steps. */
#endif
    flags.verbose               = false;

    flags.ignore_overhanging_threads = false;
    flags.full_reinitialize          = false;               /* Maybe a pragma?                      */
    flags.reload_only                = false;
    flags.restart		     = false;
}

/*
 * Common underrelaxation code.
 */

void
under_relax( double& old_value, const double new_value, const double relax )
{
    if ( std::isfinite( new_value ) && std::isfinite( old_value ) ) {
	old_value = new_value * relax + old_value * (1.0 - relax);
    } else {
	old_value = new_value;
    }
}
