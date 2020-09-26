/*  -*- c++ -*-
 * $Id: lqns.cc 13854 2020-09-24 13:34:24Z greg $
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

#include <config.h>
#include "dim.h"
#include <sstream>
#include <fstream>
#include <cstdio>
#include <cstdlib>
#include <stdexcept>
#include <cstring>
#include <cmath>
#include <errno.h>
#include <libgen.h>
#include <lqio/input.h>
#include <lqio/error.h>
#include <lqio/dom_document.h>
#include <lqio/filename.h>
#include <lqio/commandline.h>
#include <lqio/dom_bindings.h>
#include <lqio/srvn_spex.h>
#include <lqio/dom_pragma.h>
#if !defined(HAVE_GETSUBOPT)
#include <lqio/getsbopt.h>
#endif
#include "generate.h"
#include "help.h"
#include "option.h"
#include "pragma.h"
#include "errmsg.h"
#include "lqns.h"
#include "model.h"
#include "fpgoop.h"
#include "mva.h"
#include "runlqx.h"

extern "C" int LQIO_debug;
static bool print_lqx = false;

static char copyrightDate[20];

/* -- */

struct FLAGS flags;
static LQIO::DOM::Pragma pragmas;

#if HAVE_GETOPT_LONG
const struct option longopts[] =
    /* name */ /* has arg */ /*flag */ /* val */
{
    { "no-advisories",        no_argument,       0, 'a' },
    { "bounds-only",          no_argument,       0, 'b' },
    { "convergence",          required_argument, 0, 'c' },
    { "debug",                required_argument, 0, 'd' },
    { "error",                required_argument, 0, 'e' },
    { LQIO::DOM::Pragma::_fast_, no_argument,       0, 'f' },
    { "gnuplot",	      optional_argument, 0, 'G' },
    { "help",                 optional_argument, 0, 'H' },
    { "huge",		      no_argument,	 0, 'h' },
    { "iteration-limit",      required_argument, 0, 'i' },
    { "input-format",         required_argument, 0, 'I' },
    { "no-execute",           no_argument,       0, 'n' },
    { "output",               required_argument, 0, 'o' },
    { "parseable",            no_argument,       0, 'p' },
    { "pragma",               required_argument, 0, 'P' },
    { "rtf",                  no_argument,       0, 'r' },
    { "trace",                required_argument, 0, 't' },
    { "underrelaxation",      required_argument, 0, 'u' },
    { "verbose",              no_argument,       0, 'v' },
    { "version",              no_argument,       0, 'V' },
    { "no-warnings",          no_argument,       0, 'w' },
    { "xml",                  no_argument,       0, 'x' },
    { "special",              required_argument, 0, 'z' },
    { "exact-mva",            no_argument,       0, 256+'e' },
    { "schweitzer-amva",      no_argument,       0, 256+'s' },

    { "batch-layering",       no_argument,	 0, 256+'b' },	/* NOP */
    { "hwsw-layering",        no_argument,       0, 256+'h' },
    { "method-of-layers",     no_argument,       0, 256+'m' },
    { "squashed-layering",    no_argument,       0, 256+'z' },
    { "srvn-layering",        no_argument,       0, 256+'l' },

    { "processor-sharing",    no_argument,       0, 256+'p' },
#if HAVE_LIBGSL && HAVE_LIBGSLCBLAS
    { "quorum",               no_argument,       0, 256+'q' },
#endif
    { "no-stop-on-message-loss", no_argument,    0, 256+'o' },  /* Ignore open */
    { "no-variance",          no_argument,       0, 256+'v' },
    { "reload-lqx",           no_argument,       0, 512+'r' },
    { "restart",	      no_argument,	 0, 512+'R' },
    { "no-header", 	      no_argument,       0, 512+'h' },
    { "reset-mva", 	      no_argument,       0, 256+'r' },
    { "trace-mva",            no_argument,       0, 256+'t' },
    { "debug-lqx",            no_argument,       0, 512+'l' },
    { "debug-xml",            no_argument,       0, 512+'x' },
    { "debug-srvn",           no_argument,       0, 512+'y' },
    { "debug-spex",	      no_argument,	 0, 512+'s' },
    { 0, 0, 0, 0 }
};
#endif
const char opts[]       = "abc:d:e:fhH:i:I:jno:pP:rt:u:vVwxz:";
const char * opthelp[]  = {
    /* ignore-advisories*/      "Do not output advisory messages",
    /* bounds-only"     */      "Compute throughput bounds only.",
    /* convergence"     */      "Set the convergence value to ARG.",
    /* debug"           */      "Enable debug code.  See -Hd.",
    /* error"           */      "Set floating point exception mode.",
    /* fast             */      "Solve using one-step-linearizer, batch layering and Conway multiserver.",
    /* gnuplot		*/	"Output code for gnuplot(1).  ARG is a list of SPEX result variables. (SPEX only).",
    /* help"            */      "Show this help.  The optional argument shows help for -d, -t, -z, and -P respectively.",
    /* huge		*/	"Solve using one-step-schweitzer, no interlocking, and Rolia multiserver.",
    /* iteration-limit" */      "Set the iteration limit to ARG.",
    /* input-format     */      "Force input format to ARG.  ARG is either 'lqn' or 'xml'.",
    /* no-execute"      */      "Build the model, but do not solve.",
    /* output"          */      "Redirect ouptut to FILE.",
    /* parseable"       */      "Generate parseable (.p) output.",
    /* pragma"          */      "Set solver options.  See -HP.",
    /* rtf              */      "Output results in Rich Text Format instead of plain text.",
    /* trace"           */      "Trace solver operation.  See -Ht.",
    /* underrelaxation" */      "Set the under-relaxation value to ARG.",
    /* verbose"         */      "Output on standard error the progress of the solver.",
    /* version"         */      "Print the version of the solver.",
    /* no-warnings"     */      "Do not output warning messages.",
    /* xml              */      "Ouptut results in XML format.",
    /* special"         */      "Set special options.  See -Hz.",

    /* exact-mva"       */      "Use exact MVA instead of Linearizer for solving submodels.",
    /* schweitzer-amva" */      "Use Schweitzer approximate MVA instead of Linearizer.",

    /* batch-layering   */	"Default layering strategy.",
    /* hwsw-layering"   */      "Use HW/SW layering instead of batched layering.",
    /* method-of-layers */      "Use the Method of Layers instead of batched layering.",
    /* squashed-layering */     "Use only one submodel to solve the model.",
    /* srvn-layering"   */      "Use one server per layer instead of batched layering.",

    /* processor-sharing */     "Use processor sharing scheduling at fifo scheduled processors.",
#if HAVE_LIBGSL && HAVE_LIBGSLCBLAS
    /* quorum"          */      "Quorum.",
#endif
    /* no-stop-on-message-loss*/"Ignore infinities caused by open arrivals or asynchronous sends.",
    /* no-variance"     */      "Ignore the variance computation during solution.",
    /* reload_lqx"      */      "Run the LQX program, but re-use the results from a previous invocation.",
    /* restart		*/	"Reuse existing valid results.  Otherwise, run the solver.",
    /* no-header        */      "Do not output the variable name header on SPEX results.",
    /* reset-mva	*/	"Reset the MVA calculation prior to solving a submodel.", 
    /* trace-mva"       */      "Trace the operation of the MVA solver.",
    /* debug-lqx"       */      "Output debugging information while parsing LQX input.",
    /* debug-xml"       */      "Output debugging information while parsing XML input.",
    /* debug-srvn       */      "Output debugging information while parsing SRVN input.",
    /* debug-spex	*/	"Output LQX progam corresponding to SPEX input.",
    0
};

static int process ( const string&, const string& );
static void init_flags ();

#if (defined(linux) || defined(__linux__)) && !defined(__USE_XOPEN_EXTENDED)
extern "C" int getsubopt (char **, char * const *, char **);
#endif

/*
 * Main line.
 */

int main (int argc, char *argv[])
{
    string outputFileName = "";
#if HAVE_GETOPT_LONG
    LQIO::CommandLine command_line( opts, longopts );
#else
    LQIO::CommandLine command_line( opts );
#endif

    unsigned global_error_flag = 0;     /* Error detected anywhere??    */

    char * options;

    LQIO::io_vars.init( VERSION, basename( argv[0] ), severity_action, local_error_messages, LSTLCLERRMSG-LQIO::LSTGBLERRMSG );
    command_line = LQIO::io_vars.lq_toolname;

    sscanf( "$Date: 2020-09-24 09:34:24 -0400 (Thu, 24 Sep 2020) $", "%*s %s %*s", copyrightDate );

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
            if ( !optarg || (Model::convergence_value = strtod( optarg, 0 )) == 0 ) {
                cerr << LQIO::io_vars.lq_toolname << "convergence=" << optarg << " is invalid, choose a non-negative real." << endl;
                (void) exit( INVALID_ARGUMENT );
            } else {
                flags.override_convergence = true;
            }
            break;

        case 'd':
            Options::Debug::initialize();
            options = optarg;
            while ( *options ) {
                char * value = 0;
                const int ix = getsubopt( &options, const_cast<char * const *>(Options::Debug::__options), &value );
                Options::Debug::exec( ix, value );
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
                cerr << LQIO::io_vars.lq_toolname << ": invalid argument to -e -- " << optarg << endl;
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
	    pragmas.insert(LQIO::DOM::Pragma::_spex_header_,"false");
            break;

        case 'I':
            if ( strcasecmp( optarg, "xml" ) == 0 ) {
                Model::input_format = LQIO::DOM::Document::XML_INPUT;
            } else if ( strcasecmp( optarg, "lqn" ) == 0 ) {
                Model::input_format = LQIO::DOM::Document::LQN_INPUT;
            } else {
                cerr << LQIO::io_vars.lq_toolname << ": invalid argument to -I -- " << optarg << endl;
            }
            break;

        case 'i':
            if ( !optarg || (Model::iteration_limit = (unsigned)strtol( optarg, 0, 10 )) == 0 ) {
                cerr << LQIO::io_vars.lq_toolname << "iteration-limit=" << optarg << " is invalid, choose a non-negative integer." << endl;
                (void) exit( INVALID_ARGUMENT );
            } else {
                flags.override_iterations = true;
            }
            break;

        case 256+'l':
            pragmas.insert(LQIO::DOM::Pragma::_layering_,LQIO::DOM::Pragma::_srvn_);
            break;

        case (512+'l'):
            ModLangParserTrace(stderr, "lqx:");
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
                Pragma::usage( cerr );
                exit( INVALID_ARGUMENT );
            }
            break;

        case 256+'p':
            pragmas.insert( LQIO::DOM::Pragma::_processor_scheduling_, scheduling_label[SCHEDULE_PS].XML );
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
	    print_lqx = true;
	    break;
	    
        case 't':
            Options::Trace::initialize();
            options = optarg;
            while ( *options ) {
                char * value = 0;
                const int ix = getsubopt( &options, const_cast<char * const *>(Options::Trace::__options), &value );
                Options::Trace::exec( ix, value );
            }
            break;

        case 256+'t':
            flags.trace_mva = true;
            break;

        case 'u':
            if ( !optarg || (Model::underrelaxation = strtod( optarg, 0 )) <= 0.0 || 2.0 < Model::underrelaxation ) {
                cerr << LQIO::io_vars.lq_toolname << "underrelaxation=" << optarg << " is invalid, choose a value between 0.0 and 2.0." << endl;
                (void) exit( INVALID_ARGUMENT );
            } else {
                flags.override_underrelaxation = true;
            }
            break;

        case 'v':
            flags.verbose = true;
            LQIO::Spex::__verbose = true;
            break;

        case 'V':
            cout << "Layered Queueing Network Analyser, Version " << VERSION << endl << endl;
            cout << "  Copyright " << copyrightDate << " the Real-Time and Distributed Systems Group," << endl;
            cout << "  Department of Systems and Computer Engineering," << endl;
            cout << "  Carleton University, Ottawa, Ontario, Canada. K1S 5B6" << endl << endl;
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
            Options::Special::initialize();
            options = optarg;
            while ( *options ) {
                char * value = 0;
                const int ix = getsubopt( &options, const_cast<char * const *>(Options::Special::__options), &value );
                Options::Special::exec( ix, value );
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
    LQIO::io_vars.lq_command_line = command_line.c_str();

    if ( flags.generate && flags.no_execute ) {
        cerr << LQIO::io_vars.lq_toolname << ": -n is incompatible with -zgenerate.  -zgenerate ignored." << endl;
    }

    if ( flags.reload_only && flags.restart ) {
	cerr << LQIO::io_vars.lq_toolname << ": --reload-lqx and --restart are mutually exclusive: --restart assumed."  << endl;
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

        global_error_flag = process( "-", outputFileName );

    } else {

        const int file_count = argc - optind;           /* Number of files on cmd line  */

        if ( file_count > 1 ) {
            if ( outputFileName != "" ) {
                cerr << LQIO::io_vars.lq_toolname << ": Too many input files specified with the option: -o"
                     << outputFileName
                     << endl;
                exit( INVALID_ARGUMENT );
            }
            if ( Generate::file_name.size() ) {
                cerr << LQIO::io_vars.lq_toolname << ": Too many input files specified with the option: -zgenerate="
                     << Generate::file_name
                     << endl;
                exit( INVALID_ARGUMENT );
            }
        }

        for ( ; optind < argc; ++optind ) {
            if ( file_count > 1 ) {
                cout << argv[optind] << ':' << endl;
            }
            global_error_flag |= process( argv[optind], outputFileName );
        }
    }

    return global_error_flag;
}

/*
 * Open output files, solve, and print.
 */

static int
process ( const string& inputFileName, const string& outputFileName )
{
    /* Open input file. */

    if ( !flags.no_execute && flags.generate && Generate::file_name.size() == 0 ) {
        Generate::file_name = LQIO::Filename( inputFileName )();
    }

    /* This is a departure from before -- we begin by loading a model */
    LQIO::DOM::Document* document = Model::load(inputFileName,outputFileName);

    /* Make sure we got a document */

    if ( document == NULL || LQIO::io_vars.anError() ) return INVALID_INPUT;

    document->mergePragmas( pragmas.getList() );       /* Save pragmas -- prepare will process */
    if ( Model::prepare(document) == false ) return INVALID_INPUT;
        

    if ( document->getInputFormat() != LQIO::DOM::Document::LQN_INPUT && LQIO::Spex::__no_header ) {
        cerr << LQIO::io_vars.lq_toolname << ": --no-header is ignored for " << inputFileName << "." << endl;
    }

    /* declare Model * at this scope but don't instantiate due to problems with LQX programs and registering external symbols*/
    Model * aModel = NULL;
    int rc = 0;

    /* We can simply run if there's no control program */
    LQX::Program * program = document->getLQXProgram();
    FILE * output = 0;
    try {
	if ( !program ) {

	    /* There is no control flow program, check for $-variables */
	    if (document->getSymbolExternalVariableCount() != 0) {
		LQIO::solution_error( LQIO::ERR_LQX_VARIABLE_RESOLUTION, inputFileName.c_str() );
		rc = INVALID_INPUT;
	    } else {
		/* Make sure values are up to date */
		Model::recalculateDynamicValues( document );

		/* create Model just before it is needed */
		aModel = Model::createModel( document, inputFileName, outputFileName );
		if ( !aModel ) throw std::runtime_error( "could not create model" );

		if ( flags.verbose ) {
		    cerr << "Solve..." << endl;
		}

		/* Simply invoke the solver for the current DOM state */
		aModel->solve();
	    }
	} else {

	    if ( flags.verbose ) {
		cerr << "Compile LQX..." << endl;
	    }

	    /* Attempt to run the program */
	    document->registerExternalSymbolsWithProgram( program );

	    if ( print_lqx ) {
		program->print( std::cout );
	    }
	
	    /* create Model after registering external symbols above, disabling checking at this stage */
	    aModel = Model::createModel( document, inputFileName, outputFileName, false );
	    if ( !aModel ) throw std::runtime_error( "could not create model" );
		
	    LQX::Environment * environment = program->getEnvironment();
	    if ( flags.restart ) {
		environment->getMethodTable()->registerMethod(new SolverInterface::Solve(document, &Model::restart, aModel));
	    } else if ( flags.reload_only ) {
		environment->getMethodTable()->registerMethod(new SolverInterface::Solve(document, &Model::reload, aModel));
	    } else {
		environment->getMethodTable()->registerMethod(new SolverInterface::Solve(document, &Model::solve, aModel));
	    }
	    LQIO::RegisterBindings(environment, document);

	    if ( outputFileName.size() > 0 && outputFileName != "-" && LQIO::Filename::isRegularFile(outputFileName.c_str()) ) {
		output = fopen( outputFileName.c_str(), "w" );
		if ( !output ) {
		    solution_error( LQIO::ERR_CANT_OPEN_FILE, outputFileName.c_str(), strerror( errno ) );
		    rc = FILEIO_ERROR;
		} else {
		    environment->setDefaultOutput( output );      /* Default is stdout */
		}
	    }

	    if ( rc == 0 ) {
		/* Invoke the LQX program itself */
		if ( !program->invoke() ) {
		    LQIO::solution_error( LQIO::ERR_LQX_EXECUTION, inputFileName.c_str() );
		    rc = INVALID_INPUT;
		} else if ( !SolverInterface::Solve::solveCallViaLQX ) {
		    /* There was no call to solve the LQX */
		    LQIO::solution_error( LQIO::ADV_LQX_IMPLICIT_SOLVE, inputFileName.c_str() );
		    std::vector<LQX::SymbolAutoRef> args;
		    environment->invokeGlobalMethod("solve", &args);
		}
	    }
	}
    }
    catch ( const std::domain_error& e ) {
	rc = INVALID_INPUT;
    }
    catch ( const range_error& e ) {
	cerr << LQIO::io_vars.lq_toolname << ": range error - " << e.what() << endl;
	rc = INVALID_OUTPUT;
    }
    catch ( const floating_point_error& e ) {
	cerr << LQIO::io_vars.lq_toolname << ": floating point error - " << e.what() << endl;
	rc = INVALID_OUTPUT;
    }
    catch ( const std::runtime_error& e ) {
	rc = INVALID_INPUT;
    }
    catch ( const exception_handled& e ) {
	rc = INVALID_OUTPUT;
    }

    /* Clean things up */
    if ( aModel ) delete aModel;
    if ( output ) fclose( output );
    if ( program ) delete program;
    delete document;
    return rc;
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

    flags.override_iterations        = false;
    flags.override_convergence       = false;
    flags.override_print_interval    = false;
    flags.override_underrelaxation   = false;
    flags.ignore_overhanging_threads = false;
    flags.full_reinitialize          = false;               /* Maybe a pragma?                      */
    flags.reload_only                = false;
    flags.restart		     = false;
}




#if !defined(TESTMVA)
/*
 * Common underrelaxation code.  
 */

void
under_relax( double& old_value, const double new_value, const double relax ) 
{
    if ( isfinite( new_value ) && isfinite( old_value ) ) {
	old_value = new_value * relax + old_value * (1.0 - relax);
    } else {
	old_value = new_value;
    }
}
#endif

#if defined(__GNUC__) || (defined(__INTEL_COMPILER) && (__INTEL_COMPILER >= 700))
#include "prob.h"
#include "server.h"
#if !defined(TESTMVA) && !defined(TESTDIST)
#include "randomvar.h"
#include "activity.h"
#include "phase.h"
#include "call.h"
#include "entity.h"
#include "entry.h"
#include "processor.h"
#include "task.h"
#include "group.h"
#include "report.h"
#include "phase.h"
#include "randomvar.h"
#include "submodel.h"
#include "interlock.h"
#include "actlist.h"
#include "entrythread.h"
#endif
#if !defined(TESTMVA) || defined(TESTDIST)
#include "randomvar.h"
#endif
#include "vector.h"
#include "vector.cc"

#if	!defined(TESTMVA) && !defined(TESTDIST)
template class Vector<Activity *>;
template class Vector<const Activity *>;
template class Vector<Entry *>;
template class Vector<Submodel *>;
template class Vector<Thread *>;
template class Vector<const AndForkActivityList*>;
template class Vector<Exponential>;
template class Vector<Phase>;
template class Vector<InterlockInfo>;
template class Vector<MVACount>;
#if _WIN64
template class Vector<unsigned long long>;
#endif
template class Vector<Vector<Exponential> >;
template class Vector<Vector<unsigned> >;
template class Vector<VectorMath<double> >;
template class Vector<VectorMath<unsigned> >;
template class Vector<unsigned short>;
#endif
template class Vector<double>;
template class Vector<unsigned int>;
template class Vector<unsigned long>;
template class Vector<Server *>;
template class Vector<Probability>;
template class VectorMath<unsigned int>;
template class VectorMath<double>;
template class VectorMath<Probability>;

template ostream& operator<< ( ostream& output, const Vector<unsigned int>& self );
template ostream& operator<< ( ostream& output, const VectorMath<unsigned int>& self );
template ostream& operator<< ( ostream& output, const VectorMath<Probability>& self );
template ostream& operator<< ( ostream& output, const VectorMath<double>& self );
#endif
