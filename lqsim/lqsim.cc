/************************************************************************/
/* Copyright the Real-Time and Distributed Systems Group,		*/
/* Department of Systems and Computer Engineering,			*/
/* Carleton University, Ottawa, Ontario, Canada. K1S 5B6		*/
/* 									*/
/* September 1991.							*/
/************************************************************************/

/*
 * $Id: lqsim.cc 14982 2021-09-22 22:55:38Z greg $
 */

#define STACK_TESTING

#include "lqsim.h"
#include <cstdlib>
#include <unistd.h>
#include <math.h>
#include <time.h>
#include <cstring>
#include <time.h>
#include <errno.h>
#include <libgen.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <stdexcept>
#include <sstream>		/* LQX */
#if HAVE_SYS_RESOURCE_H
#include <sys/resource.h>
#endif
#if HAVE_FENV_H
#include <fenv.h>
#endif
#if HAVE_IEEEFP_H && !defined(MSDOS)
#include <ieeefp.h>
#endif

#if HAVE_FLOAT_H
#include <float.h>
#endif

#if HAVE_GETOPT_H
#include <getopt.h>
#endif
#include <lqio/input.h>
#if !HAVE_GETSUBOPT
#include <lqio/getsbopt.h>
#endif
#include <lqio/error.h>
#include <lqio/filename.h>
#include <lqio/commandline.h>
#include <lqio/dom_bindings.h>
#include <lqio/srvn_spex.h>
#include <lqio/dom_pragma.h>
#include "errmsg.h"
#include "model.h"
#include "runlqx.h"		// Coupling here is ugly at the moment
#include "pragma.h"

extern FILE* Timeline_Open(char* file_name); /* Open the timeline output stream */

#if defined(HAVE_IEEEFP_H) && !defined(MSDOS) && !defined(__WINNT__)
typedef	fp_except_t fp_bit_type;
#elif defined(_AIX)
typedef	fpflag_t fp_bit_type;
#else
typedef	int fp_bit_type;
#endif

#if HAVE_IEEEFP_H && defined(__GNUC__) && (__GNUC__ >= 3) && (defined(MSDOS) || defined(__CYGWIN__))
#define	FP_X_DZ		FP_X_DX		/* Redefined by GNU...		*/
#define FP_CLEAR	0		/* If this works like solaris... */
#endif

/*----------------------------------------------------------------------*/
/* Global variables.							*/
/*----------------------------------------------------------------------*/

bool debug_flag	       	      = false;	/* Debugging flag set?      	*/
bool global_parse_flag	      = false;	/* Parsable output desired? 	*/
bool global_xml_flag	      = false;	/* Output XML results.		*/
bool global_rtf_flag	      = false;	/* Output RTF			*/
bool raw_stat_flag 	      = false;	/* Verbose text output?	    	*/
bool verbose_flag 	      = false;	/* Verbose text output?	    	*/
bool no_execute_flag	      = false;	/* Run simulation if false	*/
bool timeline_flag	      = false;	/* Generate output for timeline	*/
bool trace_msgbuf_flag        = false;	/* Observe msg buffer operation	*/
bool reload_flag	      = false;	/* Reload results from LQX run.	*/
bool restart_flag	      = false;	/* Re-run any missing results	*/
bool override_print_int       = false;	/* Override input file.		*/
bool check_stacks	      = false;	/* Enable parasol stack check.	*/

bool debug_interactive_stepping = false;

unsigned max_num_bins	      = 20;
unsigned long watched_events = 0xffffffff;	/* trace everything	*/
int print_interval;			/* Value set by input file.	*/

matherr_type matherr_disposition;	/* What to do on FPE error.	*/

int trace_driver	= 0;		/* Trace driver.		*/
double inter_proc_delay	= 0.0;		/* Inter-processor delay.	*/

bool messages_lost	= false;	/* will be set true if nec.	*/
int nice_value		= 10;		/* For nicing. 			*/

char copyright_date[20];
char * histogram_output_file = 0;

FILE * stddbg;				/* debugging output goes here.	*/

unsigned link_tab[MAX_NODES];		/* Link table.			*/

/*
 * Options for getopt.
 */

#if HAVE_GETOPT_LONG
static const struct option longopts[] =
    /* name */ /* has arg */ /*flag */ /* val */
{
    { "automatic",        required_argument, 0, 'A' },
    { "blocks",           required_argument, 0, 'B' },
    { "confidence",       required_argument, 0, 'C' },
    { "debug",	          no_argument,       0, 'd' },
    { "error",	          required_argument, 0, 'e' },
    { "help",             no_argument,       0, 'H' },
    { "input-format", 	  required_argument, 0, 'I' },
    { "max-blocks",	  required_argument, 0, 'M' },
    { "no-execute",       no_argument,	     0, 'n' },
    { "output",           required_argument, 0, 'o' },
    { "parseable",        no_argument,	     0, 'p' },
    { "pragma",           required_argument, 0, 'P' },
    { "rtf",		  no_argument,	     0, 'r' },
    { "raw-statistics",   no_argument,       0, 'R' },
    { "seed",             required_argument, 0, 'S' },
    { "trace",	          required_argument, 0, 't' },
    { "run-time",         required_argument, 0, 'T' },
    { "verbose",          no_argument,	     0, 'v' },
    { "version",          no_argument,	     0, 'V' },
    { "no-warnings",      no_argument,	     0, 'w' },
    { "xml",		  no_argument,	     0, 'x' },
    { "print-interval",   required_argument, 0, 256+'p' },
    { "global-delay",	  required_argument, 0, 256+'z' },
    { "no-stop-on-message-loss", no_argument,0, 256+'o' },
    { "reload-lqx",	  no_argument,       0, 256+'r' },
    { "restart",	  no_argument,	     0, 256+'R' },
    { "no-header",	  no_argument,	     0, 256+'h' },
    { "print-comment",	  no_argument,	     0, 256+'c' },
    { "debug-lqx",        no_argument,       0, 256+'l' },
    { "debug-xml",        no_argument,       0, 256+'x' },
#if defined(STACK_TESTING)
    { "check-stacks",	  no_argument,	     0, 256+'s' },
#endif
    { 0, 0, 0, 0 }
};
#endif
static const char opts[] = "A:B:C:de:G:h:HI:jm:Mno:pP:rRsS:t:T:vVwx";
static const char * opthelp[]  = {
    /* "automatic"	*/    "Set the block time to <t>, the precision to <p> and the initial skip period to <s>.",
    /* "blocks"		*/    "Set the number of blocks to <b>, the block time to <t> and the initial skip period to <s>.",
    /* "confidence"     */    "Set the precision to <p>, the number of initial loops to skip to <l> and the block time to <t>.",
    /* "debug"		*/    "Enable debug code.",
    /* "error"		*/    "Set the floating pint exeception mode.",
    /* "help"		*/    "Show this help.",
    /* input-format	*/    "Force input format to ARG.  Arg is either 'lqn' or 'xml'.",
    /* "max-blocks"	*/    "Set the maximum number of blocks for the simulation (default is 30).",
    /* "no-execute"	*/    "Build the simulation, but do not solve.",
    /* "output"		*/    "Redirect ouptut to FILE.",
    /* "parseable"	*/    "Generate parseable (.p) output.",
    /* "pragma"		*/    "Set simulation options.",
    /* "rtf"		*/    "Output results in Rich Text Format.",
    /* "raw-statistics"	*/    "Print out the raw statistics from the simulation.",
    /* "seed"		*/    "Set the seed value to ARG.",
    /* "trace"		*/    "Trace the execution of the simulation.",
    /* "run-time"	*/    "Set the run time of the simulation to ARG.",
    /* "verbose"	*/    "Output on standard error the progress of the simulation.",
    /* "version"	*/    "Print the version of the simulator.",
    /* "no-warnings"	*/    "Do not output warning messages.",
    /* "xml"		*/    "Output results in XML format.",
    /* "print-interval"	*/    "Ouptut results after n iterations.",
    /* "global-delay"	*/    "Set the inter-processor communication delay to n.n.",
    /* no-stop-on-mess  */    "Do not stop the simulator if asynchronous messages are lost due to queue overfull.",
    /* "reload-lqx"	*/    "Run the LQX program, but re-use the results from a previous invocation.",
    /* "restart"	*/    "Reuse existing valid results.  Otherwise, run the simulation.",
    /* no-header	*/    "Do not output the variable name header on SPEX results.",
    /* "debug-lqx"	*/    "Output debugging information while parsing LQX input.",
    /* "debug-xml"	*/    "Output debugging information while parsing XML input.",
#if defined(STACK_TESTING)
    /* check-stacks	*/    "Check stack size after simulation run.",
#endif
    0
};

static const char * trace_opts[] = {
#define	DRIVER	0
    "driver",
#define	PROCESSOR 1
    "processor",
#define	TASK	2
    "task",
#define	EVENTS	3
    "events",
#define	TIMELINE 4
    "timeline",
#define	MSGBUF	5
    "msgbuf",
#define GROUP  6
	"group",
    0
};

static const char * events[] = {
    "msg-async",	/* ASYNC_INTERACTION_INITIATED, */
    "msg-send",		/* SYNC_INTERACTION_INITIATED, */
    "msg-receive", 	/* SYNC_INTERACTION_ESTABLISHED, */
    "msg-reply",	/* SYNC_INTERACTION_REPLIES, */
    "msg-done",		/* SYNC_INTERACTION_COMPLETED, */
    "msg-abort",	/* SYNC_INTERACTION_ABORTED, */
    "msg-forward",	/* SYNC_INTERACTION_FORWARDED, */
    "worker-dispatch",	/* WORKER_DISPATCH, */
    "worker-idle",	/* WORKER_IDLE, */
    "task-created",	/* TASK_CREATED, */
    "task-ready",	/* TASK_IS_READY, */
    "task-running",	/* TASK_IS_RUNNING, */
    "task-computing",	/* TASK_IS_COMPUTING */
    "task-waiting",	/* TASK_IS_WAITING, */
    "thread-start",	/* THREAD_START, */
    "thread-enqueue",	/* THREAD_ENQUEUE_MSG, */
    "thread-dequeue",	/* THREAD_DEQUEUE_MSG, */
    "thread-idle",	/* THREAD_IDLE,	 */
    "thread-create",	/* THREAD_CREATE, */
    "thread-reap",	/* THREAD_REAP, */
    "thread-stop",	/* THREAD_STOP, */
    "activity-start",	/* ACTIVITY-START */   
    "activity_execute",	/* ACTIVITY_EXECUTE */
    "activity_fork",	/* ACTIVITY_FORK */
    "activity_join",    /* ACTIVITY_JOIN */
    "dequeue_reader",	/* DEQUEUE_READER */
    "dequeue_writer",	/* DEQUEUE_WRITER */
    "enqueue_reader",	/* ENQUEUE_READER */
    "enqueue_writer"	/* ENQUEUE_WRITER */
};

#if HAVE_REGEX_H
regex_t * processor_match_pattern = 0;
regex_t * task_match_pattern 	= 0;
#endif

/*
 * IEEE exceptions.
 */

static struct {
    fp_bit_type bit;
    const char * str;
} fp_op_str[] = {
#if defined(__hpux) || (defined(HAVE_IEEEFP_H) && !defined(MSDOS) && !defined(__WINNT__))
    { FP_X_INV, "Invalid operation" },
    { FP_X_DZ, "Overflow" },
    { FP_X_OFL, "Underflow" },
    { FP_X_UFL, "Divide by zero" },
    { FP_X_IMP, "Inexact result" },
#elif defined(HAVE_FENV_H)
    { FE_DIVBYZERO, "Divide by zero" },
#if defined(FE_DENORMAL)
    { FE_DENORMAL, "Denormal" },
#endif
    { FE_INEXACT, "Inexact result" },
    { FE_INVALID, "Invalid operation" },
    { FE_OVERFLOW, "Overflow" },
    { FE_UNDERFLOW, "Underflow" },
#elif defined(_AIX)
    { FP_INVALID, "Invalid operation" },
    { FP_OVERFLOW, "Overflow" },
    { FP_UNDERFLOW, "Underflow" },
    { FP_DIV_BY_ZERO, "Divide by zero" },
    { FP_INEXACT, "Inexact result" },
#elif  defined(MSDOS)
    { SW_INVALID, "Invalid operation" },
    { SW_UNDERFLOW, "Underflow" },
    { SW_OVERFLOW, "Overflow" },
    { SW_ZERODIVIDE, "Divide by zero" },
    { SW_INEXACT, "Inexact result" },
#endif
    { 0, 0 }
};

static LQIO::DOM::Pragma pragmas;

static void usage(void);
static int process( const string& input_file, const string& output_file );
#if HAVE_REGCOMP
static void trace_event_list ( char * );
static bool regexec_check( int errcode, regex_t *r );
#endif
static void set_fp_abort();

/*----------------------------------------------------------------------*/
/* Main.								*/
/*----------------------------------------------------------------------*/

int
main( int argc, char * argv[] )
{   				
    int global_error_flag	= 0;
    string output_file		= "";		/* Command line filename?   	*/
	
    /* optarg(3) stuff */
	
    extern char *optarg;
    char * options;
    char * value;
    extern int optind;

#if HAVE_GETOPT_LONG
    LQIO::CommandLine command_line( longopts );
#else
    LQIO::CommandLine command_line();
#endif

    /* Set the program name and revision numbers.			*/


    LQIO::io_vars.init( VERSION, basename( argv[0] ), severity_action, local_error_messages, LSTLCLERRMSG-LQIO::LSTGBLERRMSG );

    command_line = LQIO::io_vars.lq_toolname;
    (void) sscanf( "$Date: 2021-09-22 18:55:38 -0400 (Wed, 22 Sep 2021) $", "%*s %s %*s", copyright_date );
    stddbg    = stdout;

    /* Stuff set from the input file.				*/
	
    print_interval  = 0;

    /* Handy defaults.						*/
	
    raw_stat_flag   = false;
    verbose_flag    = false;
    debug_flag      = false;
    no_execute_flag = false;

    matherr_disposition = REPORT_MATHERR;
	
    pragmas.insert( getenv( "LQSIM_PRAGMAS" ) );

    /* Process all the command line arguments.  			*/
	
    for ( ;; ) {
#if HAVE_GETOPT_LONG
	const int c = getopt_long( argc, argv, opts, longopts, NULL );
#else	
	const int c = getopt( argc, argv, opts );
#endif
	if ( c == EOF) break;
	
	command_line.append( c, optarg );

	try { 
	    char * token;
	    switch ( c ) {

	    case 'A':		/* Auto blocking	*/
		token = strtok( optarg, "," );
		pragmas.insert(LQIO::DOM::Pragma::_block_period_, token );
		token = strtok(nullptr, "," );
		if ( token == nullptr ) break;
		pragmas.insert(LQIO::DOM::Pragma::_precision_, token );
		token = strtok(nullptr, "," );
		if ( token == nullptr ) break;
		pragmas.insert(LQIO::DOM::Pragma::_initial_delay_, token );
		break;
			
	    case 'B':
		token = strtok( optarg, "," );
		pragmas.insert(LQIO::DOM::Pragma::_max_blocks_, token );
		token = strtok(nullptr, "," );
		if ( token == nullptr ) break;
		pragmas.insert(LQIO::DOM::Pragma::_block_period_, token );
		token = strtok(nullptr, "," );
		if ( token == nullptr ) break;
		pragmas.insert(LQIO::DOM::Pragma::_initial_delay_, token );
		break;
			
	    case 'C':
		token = strtok( optarg, "," );
		pragmas.insert(LQIO::DOM::Pragma::_precision_, token );
		token = strtok(nullptr, "," );
		if ( token == nullptr ) break;
		pragmas.insert(LQIO::DOM::Pragma::_initial_loops_, token );
		token = strtok(nullptr, "," );
		if ( token == nullptr ) break;
		pragmas.insert(LQIO::DOM::Pragma::_run_time_, token );
		break;
	    
	    case 'c'+256:
		LQIO::Spex::__print_comment = true;
		pragmas.insert(LQIO::DOM::Pragma::_spex_comment_,"true");
		break;
		
	    case 'd':
		debug_flag    = true;
		raw_stat_flag = true;
		break;
	 
	    case 'e':			/* Error handling.	*/
		switch ( optarg[0] ) {
		case 'a':
		    set_fp_abort();
		    matherr_disposition = ABORT_MATHERR;
		    break;

		case 'i':
		    matherr_disposition = IGNORE_MATHERR;
		    break;
				
		case 'w':
		    matherr_disposition = REPORT_MATHERR;
		    break;

		default:
		    throw std::invalid_argument( optarg );
		}
		break;
			
	    case 'H':
		usage();
		exit(0);

	    case 'h':
		histogram_output_file = optarg;
		break;

	    case 256+'h':
		/* Set immediately, as it can't be changed once the SPEX program is loaded */
		LQIO::Spex::__no_header = true;
		pragmas.insert(LQIO::DOM::Pragma::_spex_header_,"false");
		break;

	    case 'I':
		if ( strcasecmp( optarg, "xml" ) == 0 ) {
		    Model::input_format = LQIO::DOM::Document::InputFormat::XML;
		} else if ( strcasecmp( optarg, "lqn" ) == 0 ) {
		    Model::input_format = LQIO::DOM::Document::InputFormat::LQN;
		} else {
		    throw std::invalid_argument( optarg );
		}
		break;

	    case (256+'l'):
		LQIO::DOM::Document::lqx_parser_trace(stderr);
		break;

	    case 'M': {
		pragmas.insert(LQIO::DOM::Pragma::_max_blocks_, optarg);
		break;
	    }

	    case 'm':
		if ( strcmp( optarg, "-" ) == 0 ) {
		    stddbg = stdout;
		} else if ( !(stddbg = fopen( optarg, "w" )) ) {
		    (void) fprintf( stderr, "%s: cannot open ", LQIO::io_vars.toolname() );
		    perror( optarg );
		    exit( FILEIO_ERROR );
		}
		break;
			
	    case 'n':
		no_execute_flag = true;
		break;
			
	    case 'N':
		nice_value = strtol( optarg, &value, 10 );
		if ( nice_value < -20 || 20 <= nice_value || *value != '\0' ) {
		    throw std::invalid_argument( optarg );
		} else {
		    override_print_int = true;
		}
		break;
		
	    case 'o':
		output_file = optarg;
		break;

	    case 256+'o':
		pragmas.insert(LQIO::DOM::Pragma::_stop_on_message_loss_,LQIO::DOM::Pragma::_no_);
		break;

	    case 'p':
		global_parse_flag = true;
		break;

	    case 'P':
		if ( !pragmas.insert( optarg ) ) {
		    Pragma::usage( cerr );
		    exit( INVALID_ARGUMENT );
		}
		break;
			    
	    case 256+'p':
		print_interval = strtol( optarg, &value, 10 );
		if ( print_interval < 0 || *value != '\0' ) {
		    throw std::invalid_argument( optarg );
		} else {
		    override_print_int = true;
		}
		break;
				
	    case 'r':
		global_rtf_flag = true;
		break;

	    case 256+'r':
		reload_flag = true;
		break;

	    case 'R':
		raw_stat_flag = true;
		break;
			
	    case 256+'R':
		restart_flag = true;
		break;

	    case 'S':
		pragmas.insert(LQIO::DOM::Pragma::_seed_value_,optarg);
		break;

	    case 's':
		debug_interactive_stepping = true;
		fprintf(stdout, "\ndebug interactive stepping option is turned on\n" ) ; 
		break;
			
#if defined(STACK_TESTING)
	    case 256+'s':
		check_stacks = true;
		break;
#endif

	    case 'T':
		pragmas.insert(LQIO::DOM::Pragma::_run_time_,optarg);
		break;

	    case 't':
		options = optarg;
		while ( *options ) switch( getsubopt( &options, const_cast<char * const *>(trace_opts), &value ) ) {
		    case DRIVER:
			trace_driver = 1;
			break;

#if HAVE_REGCOMP
		    case PROCESSOR:
			processor_match_pattern = (regex_t *)my_malloc( sizeof( regex_t ) );
			regexec_check( regcomp( processor_match_pattern, value ? value : ".*", REG_EXTENDED ), processor_match_pattern );
			break;

				
		    case TASK:
			task_match_pattern = (regex_t *)my_malloc( sizeof( regex_t ) );
			regexec_check( regcomp( task_match_pattern, value ? value : ".*", REG_EXTENDED ), task_match_pattern );
			break;

		    case EVENTS:
			trace_event_list( value );
			break;
#endif

		    case TIMELINE:
			timeline_flag = true;
			break;

		    case MSGBUF:
			trace_msgbuf_flag = true;
			break;
				
		    default:
			throw std::invalid_argument( optarg );
			break;
		    }
		break;
			
	    case 'v':
		verbose_flag = true;
		LQIO::Spex::__verbose = true;
		break;
			
	    case 'V':
		(void) fprintf( stdout, "\nStochastic Rendezvous Network Simulator, Version %s\n\n", VERSION );
		(void) fprintf( stdout, "  Copyright %s the Real-Time and Distributed Systems Group,\n", copyright_date );
		(void) fprintf( stdout, "  Department of Systems and Computer Engineering,\n" );
		(void) fprintf( stdout, "  Carleton University, Ottawa, Ontario, Canada. K1S 5B6\n\n");
		break;
	    
	    case 'w':
		pragmas.insert(LQIO::DOM::Pragma::_severity_level_,LQIO::DOM::Pragma::_advisory_);
		break;
			
	    case 'x':
		global_xml_flag = true;
		break;

	    case (256+'x'):
		LQIO::DOM::Document::__debugXML = true;
		break;

	    case 256+'z':
		inter_proc_delay = strtod( optarg, &value );
		if ( inter_proc_delay < 0. || *value != '\0' ) throw std::invalid_argument( optarg );
		break;

	    default:
		
		break;
	    }
	}
	catch ( const std::invalid_argument& e ) {
	    std::cerr << LQIO::io_vars.lq_toolname << ": option --" << longopts[optind].name  << ": invalid argument";
	    if ( e.what() ) {
		std::cerr << " -- '"  << e.what();
	    }
	    std::cerr << "'." << std::endl;
	    usage();
	    exit( INVALID_ARGUMENT );
	}
    }

    LQIO::io_vars.lq_command_line = command_line.c_str();

    if ( reload_flag && restart_flag ) {
	(void) fprintf( stderr, "%s: --reload-lqx and --restart are mutually exclusive: --restart assumed.\n", LQIO::io_vars.toolname() );
	reload_flag = false;
    }
	
#if !defined(__WINNT__)
    if ( nice_value > 0 ) {
	nice( nice_value );	/* Lower nice level for run */
    }
#endif

    /* Process all command line arguments.  If none specified, then	*/
    /* input is assumed to come in from stdin.				*/
	
    if ( optind == argc ) {

	/* If stdout is not a terminal and output to stdout.  		*/
	/* For pipelines.						*/
	
	if ( output_file.size() == 0 && LQIO::Filename::isWriteableFile( fileno( stdout ) ) > 0 ) {
	    output_file = "-";
	}
	
	try {
	    global_error_flag |= process( "-", output_file );
	}
	catch ( const runtime_error &e ) {
	    fprintf( stderr, "%s: %s\n", LQIO::io_vars.toolname(), e.what() );
	    global_error_flag = true;
	}

#if defined(DEBUG) || defined(__WINNT__)
    } else if ( argc - optind > 1 ) {
	(void) fprintf( stderr, "%s: Too many input files specified -- only one can be specified with this version.\n", LQIO::io_vars.toolname() );
	exit( INVALID_ARGUMENT );
#endif

    } else {
  
	int file_count = argc - optind;
		
	if ( output_file.size() > 0  && file_count > 1 && LQIO::Filename::isDirectory( output_file.c_str() ) == 0 ) {
	    (void) fprintf( stderr, "%s: Too many input files specified with -o <file> option.\n", LQIO::io_vars.toolname() );
	    exit( INVALID_ARGUMENT );
	}
	for ( ; optind < argc; ++optind ) {

	    if ( file_count > 1 ) {
		(void) printf( "%s:\n", argv[optind] );
	    }
	    try {
		global_error_flag |= process( argv[optind], output_file );
	    }
	    catch ( const runtime_error &e ) {
		fprintf( stderr, "%s: %s\n", LQIO::io_vars.toolname(), e.what() );
		global_error_flag = true;
	    }
	}
    }

    return global_error_flag;
}
	

static void
usage(void)
{
    fprintf( stderr, "Usage: %s", LQIO::io_vars.toolname() );

    fprintf( stderr, " [option] [file ...]\n\n" );
    fprintf( stderr, "Options:\n" );

#if HAVE_GETOPT_LONG
    const char ** p = opthelp;
    for ( const struct option *o = longopts; (o->name || o->val) && *p; ++o, ++p ) {
	string s;
	if ( o->name ) {
	    s = "--";
	    s += o->name;
	    switch ( o->val ) {
	    case 'A': s += "=<t[,p[,s]]>"; break;
	    case 'B': s += "=<b[,t[,s]]>"; break;
	    case 'C': s += "=<p[,l[,t]]>"; break;
	    case 'e': s += "=[eiw]"; break;
	    case 'G': s += "[=<var>[,<var>]]"; break;
	    case 'h': s += "=<file|dir>"; break;
	    case 'o': s += "=<file>"; break;
	    case 'S': s += "=<n>"; break;
	    case 't': s += "=<trace>"; break;
	    case 'T': s += "=<n>"; break;
	    }
	} else {
	    s = " ";
	}
	if ( isascii(o->val) && isgraph(o->val) ) {
	    fprintf( stderr, " -%c, ", static_cast<char>(o->val) );
	} else {
	    fprintf( stderr, "     " );
	}
	fprintf( stderr, "%-28s %s\n", s.c_str(), *p );
    }

#else
    for ( const char * s = opts; *s; ++s ) {
	if ( *(s+1) == ':' ) {
	    ++s;
	} else {
	    fputc( *s, stderr );
	}
    }
    cerr << ']';
	
    for ( const char * s = opts; *s; ++s ) {
	if ( *(s+1) == ':' ) {
	    fprintf( stderr, " [-%c", *s );
	    switch ( *s ) {
	    default:  fprintf( stderr, "file" ); break;
	    case 'A': fprintf( stderr, "time[,precision[,skip]]" ); break;
	    case 'B': fprintf( stderr, "blocks[,time[,skip]]" ); break;
	    case 'C': fprintf( stderr, "precision[,loops[,run-time]]" ); break;
	    case 'e': fprintf( stderr, "(e|i|w)" ); break;
	    case 'h': fprintf( stderr, "(file|dir)" ); break;
	    case 'G': fprintf( stderr, "<var>[,<var>]..." ); break;
	    case 'S': fprintf( stderr, "seed" ); break;
	    case 't': fprintf( stderr, "trace" ); break;
	    case 'T': fprintf( stderr, "time" ); break;
	    } 
	    fputc( ']', stderr );
	    ++s;
	}
    }
    fprintf( stderr, " [file ...]\n" );
	
    (void) fprintf( stderr, "\t-t " );
    for ( const char ** sp = &trace_opts[0]; *sp; ++sp ) {
	(void) fprintf( stderr, "%s%c", *sp, *(sp+1) ? ',' : '\n' );
    }
    Pragma::usage();
#endif
}

/*
 * PARASOL mainline - initializes the simulation, processes command line
 * arguments, kick starts genesis, & drives the simulation.
 * The simulation itself runs as a child to simplify exit processing by
 * the simulator, redirection of input and output, and a host of other
 * things.
 */

/*ARGSUSED*/
static int
process( const string& input_file, const string& output_file )	 
{
    LQIO::DOM::Document* document = Model::load( input_file, output_file );
    Model aModel( document, input_file, output_file );

    /* Make sure we got a document */
    if ( document == NULL || LQIO::io_vars.anError() ) return INVALID_INPUT;

    document->mergePragmas( pragmas.getList() );       /* Save pragmas */
    if( !aModel.construct() ) return INVALID_INPUT;

    int status = 0;

    /* We can simply run if there's no control program */

    LQX::Program* program = document->getLQXProgram();
    FILE * output = 0;
    try { 
	if ( program ) {
	
	    /* Attempt to run the program */
	    document->registerExternalSymbolsWithProgram(program);
	    if ( restart_flag ) {
		program->getEnvironment()->getMethodTable()->registerMethod(new SolverInterface::Solve(document, &Model::restart, &aModel));
	    } else if ( reload_flag ) {
		program->getEnvironment()->getMethodTable()->registerMethod(new SolverInterface::Solve(document, &Model::reload, &aModel));
	    } else {
		program->getEnvironment()->getMethodTable()->registerMethod(new SolverInterface::Solve(document, &Model::start, &aModel));
	    }

	    LQIO::RegisterBindings(program->getEnvironment(), document);
	
	    if ( output_file.size() > 0 && output_file != "-" && LQIO::Filename::isRegularFile(output_file.c_str()) ) {
		output = fopen( output_file.c_str(), "w" );
		if ( !output ) {
		    solution_error( LQIO::ERR_CANT_OPEN_FILE, output_file.c_str(), strerror( errno ) );
		    status = FILEIO_ERROR;
		} else {
		    program->getEnvironment()->setDefaultOutput( output );	/* Default is stdout */
		}
	    }

	    if ( status == 0 ) {
		/* Invoke the LQX program itself */
		if ( !program->invoke() ) {
		    LQIO::solution_error( LQIO::ERR_LQX_EXECUTION, input_file.c_str() );
		    status = INVALID_INPUT;
		} else if ( !SolverInterface::Solve::solveCallViaLQX ) {
		    /* There was no call to solve the LQX */
		    LQIO::solution_error( LQIO::ADV_LQX_IMPLICIT_SOLVE, input_file.c_str() );
		    std::vector<LQX::SymbolAutoRef> args;
		    SolverInterface::Solve::implicitSolve = true;
		    program->getEnvironment()->invokeGlobalMethod("solve", &args);
		}
	    }

	} else {
	    /* There is no control flow program, check for $-variables */
	    if ( aModel.hasVariables() ) {
		LQIO::solution_error( LQIO::ERR_LQX_VARIABLE_RESOLUTION, input_file.c_str() );
		status = INVALID_INPUT;
	    } else if ( !aModel.start() ) {
		status = INVALID_OUTPUT;
	    }
	}
    }
    catch ( const std::domain_error& e ) {
	status = INVALID_INPUT;
    }
    catch ( const std::runtime_error& e ) {
	status = INVALID_INPUT;
    }

    /* Clean up */
    
    if ( output ) fclose( output );
    if ( program ) delete program;
//    delete document;
    
    return status;
}
/* ------------------------------------------------------------------------ */

void
report_matherr( FILE * output )
{
    unsigned i;
    unsigned count = 0;
    
#if  HAVE_FENV_H && HAVE_FETESTEXCEPT
    fp_bit_type bits = fetestexcept( FE_DIVBYZERO|FE_INVALID|FE_OVERFLOW|FE_UNDERFLOW );
#elif HAVE_IEEEFP_H && HAVE_FPGETSTICKY
    fp_bit_type bits = fpgetsticky() & (FP_X_INV|FP_X_OFL|FP_X_UFL|FP_X_DZ);
#elif defined(__WINNT__)
    fp_bit_type bits =  _status87() & (FE_DIVBYZERO|FE_INVALID|FE_OVERFLOW|FE_UNDERFLOW );
#else
    fp_bit_type bits = 0;
#endif

    for ( i = 0; fp_op_str[i].str; ++i ) {
	if ( bits & fp_op_str[i].bit ) {
	    if ( count == 0 ) {
		fprintf( output, "Floating Point exception (" );
	    } else {
		fprintf( output, "," );
	    }
	    fprintf( output, "%s", fp_op_str[i].str );
	    count += 1;
	}
    }
    if ( count ) {
	fprintf( output, ")\n" );
    }
}

#if HAVE_REGCOMP
static void
trace_event_list( char * s )
{
    regex_t pattern;
    
    watched_events = 0;		/* reset event list */

    while ( s ) {
	int j;
	char * p = strchr( s, ':' );
	
	if ( p ) {
	    *p = '\0';
	}
	regexec_check( regcomp( &pattern, s, REG_EXTENDED ), &pattern );
	for ( j = 0; j < ACTIVITY_JOIN; ++j ) {
	    if ( regexec( &pattern, events[j], 0, 0, 0 ) != REG_NOMATCH ) {
		watched_events |= (1 << j );
	    }
	}
	regfree( &pattern );
	if ( p ) {
	    s = p + 1;
	} else {
	    s = 0;
	}
    }
}
#endif

/*
 * Allocate memory. Exit on failure.
 */

void *
my_malloc( size_t size )
{
    void * p;

    if ( size == 0 ) return (void *)0;

    p = (void *)malloc( size );

    if ( !p ) {
#if defined(HAVE_GETRUSAGE)
	struct rusage r;
#endif
	(void) fprintf( stderr, "%s: Out of memory!\n", LQIO::io_vars.toolname() );
#if defined(HAVE_GETRUSAGE)
	getrusage( RUSAGE_SELF, &r );
	(void) fprintf( stderr, "\tmaxrss=%ld, idrss=%ld\n", r.ru_maxrss, r.ru_idrss );
#endif
	exit( EXCEPTION_EXIT );
    }
    return p;
}

void *
my_realloc( void * ptr, size_t size )
{
    void * p = (void *)realloc( ptr, size );

    if ( !p ) {
#if defined(HAVE_GETRUSAGE)
	struct rusage r;
#endif
	(void) fprintf( stderr, "%s: Out of memory!\n", LQIO::io_vars.toolname() );
#if defined(HAVE_GETRUSAGE)
	getrusage( RUSAGE_SELF, &r );
	(void) fprintf( stderr, "\tmaxrss=%ld, idrss=%ld\n", r.ru_maxrss, r.ru_idrss );
#endif
	exit( EXCEPTION_EXIT );
    }
    return p;
}



#if HAVE_REGCOMP
static bool
regexec_check( int errcode, regex_t *r )
{
    if ( errcode ) {
	char buf[BUFSIZ];
	regerror( errcode, r, buf, BUFSIZ );
	fprintf( stderr, "%s: %s\n", LQIO::io_vars.toolname(), buf );
	exit( INVALID_ARGUMENT );
	return false;
    } else {
	return true;
    }
}
#endif


static void
set_fp_abort()
{
#if HAVE_FENV_H && HAVE_FEENABLEEXCEPT
    feenableexcept( FE_DIVBYZERO | FE_INVALID | FE_OVERFLOW | FE_UNDERFLOW );
#elif HAVE_FENV_H && HAVE_FESETEXCEPTFLAG
    fexcept_t fe_flags;
    fegetexceptflag( &fe_flags, FE_DIVBYZERO | FE_INVALID | FE_OVERFLOW | FE_UNDERFLOW );
    fesetexceptflag( &fe_flags, FE_DIVBYZERO | FE_INVALID | FE_OVERFLOW | FE_UNDERFLOW );
#elif HAVE_IEEEFP_H && HAVE_FPSETMASK
    fpsetmask( FP_X_INV | FP_X_OFL | FP_X_DZ );
#endif
}
