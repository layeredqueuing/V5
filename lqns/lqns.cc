/*  -*- c++ -*-
 * $Id: lqns.cc 16196 2022-12-24 12:40:53Z greg $
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
#include <fcntl.h>           /* Definition of AT_* constants */
#if HAVE_GETOPT_H
#include <getopt.h>
#endif
#if HAVE_LIBGEN_H
#include <libgen.h>
#endif
#include <sys/stat.h>
#include <unistd.h>
#include <lqio/commandline.h>
#include <lqio/filename.h>
#include <lqio/input.h>
#include <lqio/srvn_spex.h>
#include <lqio/dom_pragma.h>
#if !HAVE_GETSUBOPT
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

extern "C" int srvndebug;

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
    { "huge",					no_argument,	   nullptr, 'h' },
    { "help",					optional_argument, nullptr, 'H' },
    { LQIO::DOM::Pragma::_iteration_limit_,	required_argument, nullptr, 'i' },
    { "input-format",				required_argument, nullptr, 'I' },
    { "json",					no_argument,	   nullptr, 'j' },
    { LQIO::DOM::Pragma::_mol_underrelaxation_, required_argument, nullptr, 'M' },
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
    { "trace-mva",				optional_argument, nullptr, 256+'t' },
    { "debug-submodels",			no_argument,	   nullptr, 256+'S' },
    { "debug-json",				no_argument,	   nullptr, 512+'j' },
    { "debug-lqx",				no_argument,	   nullptr, 512+'l' },
    { "debug-srvn",				no_argument,	   nullptr, 512+'y' },
    { "debug-xml",				no_argument,	   nullptr, 512+'x' },
    { "print-lqx",				no_argument,	   nullptr, 512+'s' },
    { nullptr, 0, nullptr, 0 }
};
#else
const struct option * longopts = nullptr;
#endif
const char opts[]       = "abc:d:e:fhH:i:I:jM:no:pP:rt:u:vVwxz:";

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
    LQIO::DOM::Document::OutputFormat output_format = LQIO::DOM::Document::OutputFormat::DEFAULT;
    LQIO::CommandLine command_line( longopts );
    Options::Debug::initialize();
    Options::Trace::initialize();
    Options::Special::initialize();
    Model::solve_using solve_function = &Model::compute;

    unsigned global_error_flag = 0;     /* Error detected anywhere??    */

    char * options;

    LQIO::io_vars.init( VERSION, basename( argv[0] ), LQIO::severity_action );
    std::copy( local_error_messages.begin(), local_error_messages.end(), std::inserter( LQIO::error_messages, LQIO::error_messages.begin() ) );
    
    command_line = LQIO::io_vars.lq_toolname;

    sscanf( "$Date: 2022-12-24 07:40:53 -0500 (Sat, 24 Dec 2022) $", "%*s %s %*s", copyrightDate );

    matherr_disposition = fp_exception_reporting::DEFERRED_ABORT;

    init_flags();

    pragmas.insert( getenv( "LQNS_PRAGMAS" ) );

    LQIO::DOM::DocumentObject::setSeverity(LQIO::ERR_NO_QUANTUM_SCHEDULING, LQIO::error_severity::WARNING );	// Don't care for lqns.

    for ( ;; ) {
#if HAVE_GETOPT_LONG
        const int c = getopt_long( argc, argv, opts, longopts, nullptr );
#else
        const int c = getopt( argc, argv, opts );
#endif
        if ( c == EOF) break;

        command_line.append( c, optarg );

	try {
	    switch ( c ) {
	    case 'a':
		pragmas.insert( LQIO::DOM::Pragma::_severity_level_, LQIO::DOM::Pragma::_warning_ );
		break;

	    case 'b':
		flags.bounds_only = true;
		break;

	    case 256+'b':
		break;

	    case 'c':
		pragmas.insert( LQIO::DOM::Pragma::_convergence_value_, optarg != nullptr ? optarg : std::string("") );
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
		    if ( ix >= 0 ) {
			Options::Debug::exec( ix, (value == nullptr ? std::string("") : value) );
		    } else {
			throw std::invalid_argument( std::string("--debug=") + subopt );
		    }
		}
		break;

	    case 'e':                       /* Error handling.      */
		switch ( optarg[0] ) {
		case 'a':
		    matherr_disposition = fp_exception_reporting::IMMEDIATE_ABORT;
		    break;

		case 'd':
		    matherr_disposition = fp_exception_reporting::DEFERRED_ABORT;
		    break;

		case 'i':
		    matherr_disposition = fp_exception_reporting::IGNORE;
		    break;

		case 'w':
		    matherr_disposition = fp_exception_reporting::REPORT;
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
		    Model::__input_format = LQIO::DOM::Document::InputFormat::XML;
		} else if ( strcasecmp( optarg, "lqn" ) == 0 ) {
		    Model::__input_format = LQIO::DOM::Document::InputFormat::LQN;
		} else if ( strcasecmp( optarg, "json" ) == 0 ) {
		    Model::__input_format = LQIO::DOM::Document::InputFormat::JSON;
		} else {
		    std::cerr << LQIO::io_vars.lq_toolname << ": invalid argument to -I -- " << optarg << std::endl;
		    exit( 1 );
		}
		break;

	    case 'i':
		pragmas.insert( LQIO::DOM::Pragma::_iteration_limit_, optarg != nullptr ? optarg : std::string("") );
		break;

	    case 'j':
		output_format = LQIO::DOM::Document::OutputFormat::JSON;
		break;

	    case 256+'l':
		pragmas.insert(LQIO::DOM::Pragma::_layering_,LQIO::DOM::Pragma::_srvn_);
		break;

	    case (512+'l'):
		LQIO::DOM::Document::lqx_parser_trace(stderr);
		break;

	    case 'M':
		pragmas.insert( LQIO::DOM::Pragma::_mol_underrelaxation_, optarg != nullptr ? optarg : std::string("") );
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
		output_format = LQIO::DOM::Document::OutputFormat::PARSEABLE;
		break;

	    case 'P':       /* Pragma processing... */
		if ( !pragmas.insert( optarg ) ) {
		    Pragma::usage( std::cerr );
		    exit( INVALID_ARGUMENT );
		}
		break;

	    case 256+'p':
		pragmas.insert( LQIO::DOM::Pragma::_processor_scheduling_, LQIO::SCHEDULE::PS );
		break;

	    case 512+'p':
		Options::Special::print_interval( optarg != nullptr ? optarg : std::string("") );
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
		solve_function = &Model::reload;
		break;

	    case 512+'R':
		solve_function = &Model::restart;
		break;

	    case 256+'s':
		pragmas.insert( LQIO::DOM::Pragma::_mva_, LQIO::DOM::Pragma::_schweitzer_ );
		break;

	    case 256+'S':
		Options::Debug::submodels( optarg != nullptr ? optarg : std::string("") );
		break;
		    
	    case 512+'s':
		flags.print_lqx = true;
		break;

	    case 't':
		options = optarg;
		while ( *options ) {
		    char * value = nullptr;
		    const char * subopt = options;
		    const int ix = getsubopt( &options, Options::Trace::__options.data(), &value );
		    if ( ix >= 0 ) {
			Options::Trace::exec( ix, (value != nullptr ? value : std::string("") ) );
		    } else {
			throw std::invalid_argument( std::string("--trace=") + subopt );
		    }
		}
		break;

	    case 256+'t':
		Options::Trace::mva( optarg != nullptr ? optarg : std::string("") );
		break;

	    case 'u':
		pragmas.insert( LQIO::DOM::Pragma::_underrelaxation_, optarg != nullptr ? optarg : std::string("") );
		break;

	    case 'v':
		Options::Trace::verbose( optarg != nullptr ? optarg : std::string("") );
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
		pragmas.insert(LQIO::DOM::Pragma::_severity_level_,LQIO::DOM::Pragma::_run_time_);
		break;

	    case 'x':
		output_format = LQIO::DOM::Document::OutputFormat::XML;
		break;

	    case 512+'x':
		LQIO::DOM::Document::__debugXML = true;
		break;

	    case 512+'y':
		srvndebug = true;
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

    if ( flags.generate ) {
	if ( flags.no_execute ) {
	    std::cerr << LQIO::io_vars.lq_toolname << ": -n is incompatible with -zgenerate.  -zgenerate ignored." << std::endl;
	} else if ( access( Generate::__directory_name.c_str(), R_OK|W_OK|X_OK ) < 0 && ENOENT ) {
#if defined(__WINNT__)
	    int rc = mkdir( Generate::__directory_name.c_str() );
#else
	    int rc = mkdir( Generate::__directory_name.c_str(), S_IRUSR|S_IWUSR|S_IXUSR|S_IRGRP|S_IWGRP|S_IXGRP|S_IWOTH|S_IROTH|S_IXOTH );
#endif
	    if ( rc < 0 ) {
		std::cerr << LQIO::io_vars.lq_toolname << ": Cannot create directory " << Generate::__directory_name << ": " << strerror( errno ) << "." << std::endl;
	    }
	}
    }

    /* Process all command line arguments.  If none specified, then     */
    /* input is assumed to come in from stdin.                          */

    if ( optind == argc ) {

        global_error_flag = Model::solve( solve_function, "-", outputFileName, output_format );

    } else {

        const int file_count = argc - optind;           /* Number of files on cmd line  */

        if ( file_count > 1 ) {
            if ( LQIO::Filename::isFileName( outputFileName ) && LQIO::Filename::isDirectory( outputFileName ) == 0 ) {
                std::cerr << LQIO::io_vars.lq_toolname << ": Too many input files specified with the option: -o"
                     << outputFileName
                     << std::endl;
                exit( INVALID_ARGUMENT );
            }
            if ( !Generate::__directory_name.empty() ) {
                std::cerr << LQIO::io_vars.lq_toolname << ": Too many input files specified with the option: -zgenerate="
                     << Generate::__directory_name
                     << std::endl;
                exit( INVALID_ARGUMENT );
            }
        }

        for ( ; optind < argc; ++optind ) {
            if ( file_count > 1 ) {
                std::cout << argv[optind] << ':' << std::endl;
            }
            global_error_flag |= Model::solve( solve_function, argv[optind], outputFileName, output_format );
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
    flags.rtf_output            = false;
    flags.generate              = false;
    flags.reset_mva		= false;
    flags.print_overtaking      = false;
    flags.single_step           = false;
    flags.print_lqx		= false;

    flags.trace_activities      = false;
    flags.trace_convergence     = false;
    flags.trace_customers	= false;
    flags.trace_forks           = false;
    flags.trace_idle_time       = false;
    flags.trace_interlock       = false;
    flags.trace_intermediate    = false;
    flags.trace_joins           = false;
    flags.trace_overtaking      = false;
    flags.trace_replication     = false;
    flags.trace_virtual_entry   = false;
    flags.trace_wait            = false;

#if HAVE_LIBGSL && HAVE_LIBGSLCBLAS
    flags.min_steps             = 7;            /* Default of 2 steps. */
#else
    flags.min_steps             = 2;            /* Default of 2 steps. */
#endif

    flags.ignore_overhanging_threads = false;
    flags.full_reinitialize          = false;               /* Maybe a pragma?                      */
}

/*
 * Common underrelaxation code.
 */

double
under_relax( const double old_value, const double new_value, const double relax )
{
    if ( std::isfinite( new_value ) && std::isfinite( old_value ) ) {
	return new_value * relax + old_value * (1.0 - relax);
    } else {
	return new_value;
    }
}
