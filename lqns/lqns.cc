/*  -*- c++ -*-
 * $Id: lqns.cc 12548 2016-04-06 15:13:47Z greg $
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
#include <errno.h>
#include <libgen.h>
#include <lqio/input.h>
#include <lqio/error.h>
#include <lqio/dom_document.h>
#include <lqio/filename.h>
#include <lqio/commandline.h>
#include <lqio/dom_bindings.h>
#include <lqio/srvn_spex.h>
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

extern void ModLangParserTrace(FILE *TraceFILE, char *zTracePrompt);
extern void init_errmsg(void);

lqio_params_stats io_vars =
{
    /* .n_processors =   */ 0,
    /* .n_tasks =        */ 0,
    /* .n_entries =      */ 0,
    /* .n_groups =       */ 0,
    /* .lq_toolname =    */ NULL,
    /* .lq_version =     */ VERSION,
    /* .lq_command_line =*/ NULL,
    /* .severity_action= */ severity_action,
    /* .max_error =      */ 0,
    /* .error_count =    */ 0,
    /* .severity_level = */ LQIO::NO_ERROR,
    /* .error_messages = */ NULL,
    /* .anError =        */ 0
};

static char copyrightDate[20];

/* -- */

struct FLAGS flags;
static Pragma old_pragma;

#if HAVE_GETOPT_LONG
const struct option longopts[] =
    /* name */ /* has arg */ /*flag */ /* val */
{
    { "ignore-advisories",    no_argument,       0, 'a' },
    { "bounds-only",          no_argument,       0, 'b' },
    { "debug",                required_argument, 0, 'd' },
    { "error",                required_argument, 0, 'e' },
    { "fast",                 no_argument,       0, 'f' },
    { "help",                 optional_argument, 0, 'H' },
    { "input-format",         required_argument, 0, 'I' },
    { "no-execute",           no_argument,       0, 'n' },
    { "output",               required_argument, 0, 'o' },
    { "parseable",            no_argument,       0, 'p' },
    { "pragma",               required_argument, 0, 'P' },
#if HAVE_LIBGSL && HAVE_LIBGSLCBLAS
    { "quorum",               no_argument,       0, 'q' },
#endif
    { "rtf",                  no_argument,       0, 'r' },
    { "trace",                required_argument, 0, 't' },
    { "verbose",              no_argument,       0, 'v' },
    { "version",              no_argument,       0, 'V' },
    { "no-warnings",          no_argument,       0, 'w' },
    { "xml",                  no_argument,       0, 'x' },
    { "special",              required_argument, 0, 'z' },
    { "convergence",          required_argument, 0, 256+'c' },
    { "iteration-limit",      required_argument, 0, 256+'i' },
    { "underrelaxation",      required_argument, 0, 256+'u' },
    { "exact-mva",            no_argument,       0, 256+'e' },
    { "schweitzer-amva",      no_argument,       0, 256+'s' },

    { "batch-layering",       no_argument,	 0, 256+'b' },	/* NOP */
    { "hwsw-layering",        no_argument,       0, 256+'h' },
    { "method-of-layers",     no_argument,       0, 256+'m' },
    { "squashed-layering",    no_argument,       0, 256+'z' },
    { "srvn-layering",        no_argument,       0, 256+'l' },

    { "processor-sharing",    no_argument,       0, 256+'p' },
    { "no-stop-on-message-loss", no_argument,    0, 256+'o' },  /* Ignore open */
    { "no-variance",          no_argument,       0, 256+'v' },
    { "reload-lqx",           no_argument,       0, 512+'r' },
    { "restart",	      no_argument,	 0, 512+'R' },
    { "no-header",            no_argument,       0, 512+'h' },
    { "reset-mva", 	      no_argument,       0, 256+'r' },
    { "trace-mva",            no_argument,       0, 256+'t' },
    { "debug-lqx",            no_argument,       0, 512+'l' },
    { "debug-xml",            no_argument,       0, 512+'x' },
    { "debug-srvn",           no_argument,       0, 512+'y' },
    { 0, 0, 0, 0 }
};
#endif
const char opts[]       = "abd:e:fH:I:Mno:pP:qrt:vVwxz:";
const char * opthelp[]  = {
    /* ignore-advisories*/      "Do not output advisory messages",
    /* bounds-only"     */      "Compute throughput bounds only.",
    /* debug"           */      "Enable debug code.  See -Hd.",
    /* error"           */      "Set floating point exception mode.",
    /* fast             */      "Solve using one-step-linearizer, batch layering and conway multiserver.",
    /* help"            */      "Show this help.  The optional argument shows help for -d, -t, -z, and -P respectively.",
    /* input-format     */      "Force input format to ARG.  ARG is either 'lqn' or 'xml'.",
    /* no-execute"      */      "Build the model, but do not solve.",
    /* output"          */      "Redirect ouptut to FILE.",
    /* parseable"       */      "Generate parseable (.p) output.",
    /* pragma"          */      "Set solver options.  See -HP.",
#if HAVE_LIBGSL && HAVE_LIBGSLCBLAS
    /* quorum"          */      "Quorum.",
#endif
    /* rtf              */      "Output results in Rich Text Format instead of plain text.",
    /* trace"           */      "Trace solver operation.  See -Ht.",
    /* verbose"         */      "Output on standard error the progress of the solver.",
    /* version"         */      "Print the version of the solver.",
    /* no-warnings"     */      "Do not output warning messages.",
    /* xml              */      "Ouptut results in XML format.",
    /* special"         */      "Set special options.  See -Hz.",

    /* convergence"     */      "Set the convergence value to ARG.",
    /* iteration-limit" */      "Set the iteration limit to ARG.",
    /* underrelaxation" */      "Set the under-relaxation value to ARG.",
    /* exact-mva"       */      "Use exact MVA instead of Linearizer for solving submodels.",
    /* schweitzer-amva" */      "Use Schweitzer approximate MVA instead of Linearizer.",

    /* batch-layering   */	"Default layering strategy.",
    /* hwsw-layering"   */      "Use HW/SW layering instead of batched layering.",
    /* method-of-layers */      "Use the Method of Layers instead of batched layering.",
    /* squashed-layering */     "Use only one submodel to solve the model.",
    /* srvn-layering"   */      "Use one server per layer instead of batched layering.",

    /* processor-sharing */     "Use processor sharing scheduling at fifo scheduled processors.",
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
    0
};

static int process ( const string&, const string& );
static void init_flags ();
static bool get_pragma ( const char * );

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

    io_vars.lq_toolname = basename( argv[0] );
    command_line = io_vars.lq_toolname;

    sscanf( "$Date: 2016-04-06 11:13:47 -0400 (Wed, 06 Apr 2016) $", "%*s %s %*s", copyrightDate );

    matherr_disposition = FP_IMMEDIATE_ABORT;

    init_flags();
    init_errmsg();

    get_pragma( getenv( "LQNS_PRAGMAS" ) );

    for ( ;; ) {
#if HAVE_GETOPT_LONG
        const int c = getopt_long( argc, argv, opts, longopts, NULL );
#else
        const int c = getopt( argc, argv, opts );
#endif
        if ( c == EOF) break;

        command_line.append( c, optarg );

        switch ( c ) {
        case 'b':
            flags.bounds_only = true;
            break;

	case 256+'b':
	    break;

        case (256+'c'):
            if ( !optarg || (Model::convergence_value = strtod( optarg, 0 )) == 0 ) {
                cerr << io_vars.lq_toolname << "convergence=" << optarg << " is invalid, choose a non-negative real." << endl;
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
                cerr << io_vars.lq_toolname << ": invalid argument to -e -- " << optarg << endl;
                break;
            }
            break;

        case 256+'e':
            pragma.setMVA(EXACT_MVA);
            break;

        case 'f':
            pragma.setLayering(BATCHED_LAYERS);
            pragma.setMVA(ONESTEP_MVA);
            pragma.setMultiserver(CONWAY_MULTISERVER);
            break;

        case 'H':
            usage( optarg );
            exit(0);

        case 256+'h':
            pragma.setLayering(HWSW_LAYERS);
            break;

        case 512+'h':
            LQIO::DOM::Spex::__no_header = true;
            break;

        case 'I':
            if ( strcasecmp( optarg, "xml" ) == 0 ) {
                Model::input_format = LQIO::DOM::Document::XML_INPUT;
            } else if ( strcasecmp( optarg, "lqn" ) == 0 ) {
                Model::input_format = LQIO::DOM::Document::LQN_INPUT;
            } else {
                cerr << io_vars.lq_toolname << ": invalid argument to -I -- " << optarg << endl;
            }
            break;

        case 256+'i':
            if ( !optarg || (Model::iteration_limit = (unsigned)strtol( optarg, 0, 10 )) == 0 ) {
                cerr << io_vars.lq_toolname << "iteration-limit=" << optarg << " is invalid, choose a non-negative integer." << endl;
                (void) exit( INVALID_ARGUMENT );
            } else {
                flags.override_iterations = true;
            }
            break;

        case 256+'l':
            pragma.setLayering(SRVN_LAYERS);
            break;

        case (512+'l'):
            ModLangParserTrace(stderr, "lqx:");
            break;

        case 256+'m':
            pragma.setLayering(METHOD_OF_LAYERS);
            break;

        case 'n':
            flags.no_execute = true;
            break;

        case 'o':
            outputFileName = optarg;
            break;

        case 256+'o':
            pragma.setStopOnMessageLoss(false);
            break;

        case 'p':
            flags.parseable_output = true;
            break;

        case 'P':       /* Pragma processing... */
            if ( !get_pragma( optarg ) ) {
                Pragma::usage( cerr );
                exit( INVALID_ARGUMENT );
            }
            break;

        case 256+'p':
            pragma.setProcessor(PROCESSOR_PS);
            break;

        case 'q': //tomari quorum options
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
            pragma.setMVA(SCHWEITZER_MVA);
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

        case 256+'u':
            if ( !optarg || (Model::underrelaxation = strtod( optarg, 0 )) <= 0.0 || 2.0 < Model::underrelaxation ) {
                cerr << io_vars.lq_toolname << "underrelaxation=" << optarg << " is invalid, choose a value between 0.0 and 2.0." << endl;
                (void) exit( INVALID_ARGUMENT );
            } else {
                flags.override_underrelaxation = true;
            }
            break;

        case 'v':
            flags.verbose = true;
            LQIO::DOM::Spex::__verbose = true;
            break;

        case 'V':
            cout << "Layered Queueing Network Analyser, Version " << VERSION << endl << endl;
            cout << "  Copyright " << copyrightDate << " the Real-Time and Distributed Systems Group," << endl;
            cout << "  Department of Systems and Computer Engineering," << endl;
            cout << "  Carleton University, Ottawa, Ontario, Canada. K1S 5B6" << endl << endl;
            break;

        case (256+'v'):
            pragma.setVariance(NO_VARIANCE);
            break;

        case 'w':
            pragma.setSeverityLevel( LQIO::ADVISORY_ONLY );
            break;

        case 'a':
            pragma.setSeverityLevel( LQIO::RUNTIME_ERROR );
            break;

        case 'x':
            flags.xml_output = true;
            break;

        case (512+'x'):
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
            pragma.setLayering(SQUASHED_LAYERS);
            break;

        default:
            usage();
            break;
        }
    }
    io_vars.lq_command_line = command_line.c_str();

    if ( flags.generate && flags.no_execute ) {
        cerr << io_vars.lq_toolname << ": -n is incompatible with -zgenerate.  -zgenerate ignored." << endl;
    }

    if ( flags.reload_only && flags.restart ) {
	cerr << io_vars.lq_toolname << ": --reload-lqx and --restart are mutually exclusive: --restart assumed."  << endl;
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
                cerr << io_vars.lq_toolname << ": Too many input files specified with the option: -o"
                     << outputFileName
                     << endl;
                exit( INVALID_ARGUMENT );
            }
            if ( Generate::file_name ) {
                cerr << io_vars.lq_toolname << ": Too many input files specified with the option: -zgenerate="
                     << Generate::file_name
                     << endl;
                exit( INVALID_ARGUMENT );
            }
        }

        old_pragma = pragma;

        for ( ; optind < argc; ++optind ) {
            if ( file_count > 1 ) {
                cout << argv[optind] << ':' << endl;
                pragma = old_pragma;
            }
            global_error_flag |= process( argv[optind], outputFileName );
            Model::dispose();
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
    const int FILE_NAME_SIZE    = 64;
    static char generate_file_name[FILE_NAME_SIZE];

    /* Open input file. */

    if ( !flags.no_execute && flags.generate && !Generate::file_name ) {
        LQIO::Filename tempname( inputFileName.c_str() );
        strncpy( generate_file_name, tempname(), FILE_NAME_SIZE-1 );
        generate_file_name[FILE_NAME_SIZE-1] = '\0';
        Generate::file_name = generate_file_name;
    }


    /* This is a departure from before -- we begin by loading a model */
    LQIO::DOM::Document* document = Model::load(inputFileName,outputFileName);

    /* Make sure we got a document */
    if (document == NULL || io_vars.anError || Model::prepare(document) == false) {
        cerr << io_vars.lq_toolname << ": The input model " << inputFileName << " was not loaded successfully." << endl;
        return FILEIO_ERROR;
    }

    pragma.updateDOM( document );       /* Save pragmas */

    if ( document->getInputFormat() != LQIO::DOM::Document::LQN_INPUT && LQIO::DOM::Spex::__no_header ) {
        cerr << io_vars.lq_toolname << ": --no-header is ignored for " << inputFileName << "." << endl;
    }

    /* declare Model * at this scope but don't instantiate due to problems with LQX programs and registering external symbols*/
    Model * aModel = NULL;
    int rc = 0;

    /* We can simply run if there's no control program */
    LQX::Program * program = document->getLQXProgram();
    if ( !program ) {

        /* There is no control flow program, check for $-variables */
        if (document->getSymbolExternalVariableCount() != 0) {
            LQIO::solution_error( LQIO::ERR_LQX_VARIABLE_RESOLUTION, inputFileName.c_str() );
            rc = FILEIO_ERROR;
        } else {
            /* Make sure values are up to date */
            Model::recalculateDynamicValues( document );

            /* create Model just before it is needed */
            aModel = Model::createModel( document, inputFileName, outputFileName );

            if ( aModel ) {
                if ( flags.verbose ) {
                    cerr << "Solve..." << endl;
                }

                /* Simply invoke the solver for the current DOM state */
                aModel->solve();
                delete aModel;
            } else {
		cerr << io_vars.lq_toolname << ": The input model " << inputFileName << " was not solved." << endl;
	    }
        }
    } else {

        if ( flags.verbose ) {
            cerr << "Compile LQX..." << endl;
        }

        /* Attempt to run the program */
        document->registerExternalSymbolsWithProgram( program );

        /* create Model after registering external symbols above, disabling checking at this stage */
        aModel = Model::createModel( document, inputFileName, outputFileName, false );

        if ( aModel ) {
	    if ( flags.restart ) {
                program->getEnvironment()->getMethodTable()->registerMethod(new SolverInterface::Solve(document, &Model::restart, aModel));
	    } else if ( flags.reload_only ) {
                program->getEnvironment()->getMethodTable()->registerMethod(new SolverInterface::Solve(document, &Model::reload, aModel));
            } else {
                program->getEnvironment()->getMethodTable()->registerMethod(new SolverInterface::Solve(document, &Model::solve, aModel));
            }
            LQIO::RegisterBindings(program->getEnvironment(), document);

            FILE * output = 0;
            if ( outputFileName.size() > 0 && outputFileName != "-" && LQIO::Filename::isRegularFile(outputFileName.c_str()) ) {
                output = fopen( outputFileName.c_str(), "w" );
                if ( !output ) {
                    solution_error( LQIO::ERR_CANT_OPEN_FILE, outputFileName.c_str(), strerror( errno ) );
                    rc = FILEIO_ERROR;
                } else {
                    program->getEnvironment()->setDefaultOutput( output );      /* Default is stdout */
                }
            }

            if ( rc == 0 ) {
                /* Invoke the LQX program itself */
                if ( !program->invoke() ) {
                    LQIO::solution_error( LQIO::ERR_LQX_EXECUTION, inputFileName.c_str() );
                    rc = FILEIO_ERROR;
                } else if ( !SolverInterface::Solve::solveCallViaLQX ) {
                    /* There was no call to solve the LQX */
                    LQIO::solution_error( LQIO::ADV_LQX_IMPLICIT_SOLVE, inputFileName.c_str() );
                    std::vector<LQX::SymbolAutoRef> args;
                    program->getEnvironment()->invokeGlobalMethod("solve", &args);
                }
            }

            if ( output ) {
                fclose( output );
            }
            delete program;
            delete aModel;
	} else {
	    cerr << io_vars.lq_toolname << ": The input model " << inputFileName << "  was not solved." << endl;
        }
    }

    /* Clean things up */
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

    flags.trace_activities      = false;
    flags.trace_convergence     = false;
    flags.trace_delta_wait      = false;
    flags.trace_forks           = false;
    flags.trace_idle_time       = false;
    flags.trace_interlock       = false;
    flags.trace_joins           = false;
    flags.trace_mva             = false;
    flags.trace_overtaking      = false;
    flags.trace_intermediate    = false;
    flags.trace_wait            = false;
    flags.trace_replication     = false;

    flags.print_overtaking      = false;
    flags.single_step           = false;
    flags.skip_submodel         = 0;
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



/*
 * Process pragmas from aStr.  Set up for magical processing.
 */

static bool
get_pragma( const char * p )
{
    if ( !p ) return false;

    bool rc = true;
    do {
        while ( isspace( *p ) ) ++p;            /* Skip leading whitespace. */
        string param;
        string value;
        while ( *p && !isspace( *p ) && *p != '=' && *p != ',' ) {
            param += *p++;                      /* get parameter */
        }
        while ( isspace( *p ) ) ++p;
        if ( *p == '=' ) {
            ++p;
            while ( isspace( *p ) ) ++p;
            while ( *p && !isspace( *p ) && *p != ',' ) {
                value += *p++;
            }
        }
        while ( isspace( *p ) ) ++p;
        if ( !pragma.set( param, value ) ) {
            cerr << io_vars.lq_toolname << ": Invalid pragma -- " << param << "=" << value << endl;
            rc = false;
        }
    } while ( *p++ == ',' );
    return rc;
}
