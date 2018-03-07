/* lqngen.cc	-- Greg Franks Thu Jul 29 2004
 * Model file generator.
 * This is actually part of lqn2ps, but if lqn2ps is invoked as lqngen, then this magically runs.
 *
 * $Id: lqngen.cc 12906 2017-01-26 03:32:30Z greg $
 */

#include "lqngen.h"

#include <fstream>
#include <sstream>
#include <cstring>
#include <getopt.h>
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#if !HAVE_GETSUBOPT
#include <lqio/getsbopt.h>
#endif
#include <lqio/commandline.h>
#include <lqio/filename.h>
#include <lqio/glblerr.h>
#include "generate.h"
#include "help.h"
#if !HAVE_DRAND48
#include "randomvar.h"
#endif

#if (defined(linux) || defined(__linux__)) && !defined(__USE_XOPEN_EXTENDED)
extern "C" int getsubopt (char **, char * const *, char **);
#endif
#if HAVE_GETOPT_H
void makeopts( std::string& opts, std::vector<struct option>& longopts, int * );
#else
void makeopts( std::string& opts );
#endif

/* Sort on field 5.  Note that 0x400 is [no-] */
option_type options[] =
{
    { 'A',  	 "automatic",		    required_argument,	 	LQNGEN_ONLY,  	"Create a model with ARG layers, clients, processor and ARG**2 tasks." },
    { 'C', 	 "clients",         	    required_argument,	 	LQNGEN_ONLY,  	"Create exactly ARG client (Reference) tasks." },
    { 'L', 	 "layers",          	    required_argument,	 	LQNGEN_ONLY,  	"Create exactly ARG layers." },
    { 'P', 	 "processors",      	    required_argument,	 	LQNGEN_ONLY,  	"Create exactly ARG processors." },
    { 'T', 	 "tasks",           	    required_argument,	 	LQNGEN_ONLY,  	"Create exactly ARG tasks (excluding reference tasks)." },
    { 'Y', 	 "outgoing-requests",	    required_argument,		LQNGEN_ONLY,	"Create an average of ARG reqeusts from each entry." },
    { 'M', 	 "models",	   	    required_argument,	 	LQNGEN_ONLY,  	"Create ARG different models." },
    { 'N', 	 "experiments",     	    required_argument,	 	BOTH, 		"Create ARG experiments." },
    { 'S', 	 "sensitivity",		    required_argument,	    	LQN2LQX_ONLY,   "Create a factorial experiment with services times of increased/decreased by multiplying by ARG ." },
    { 'O', 	 "format",          	    required_argument,          BOTH, 		"Set output format to ARG (lqn,xml)." },
    { 'H', 	 "help",            	    no_argument,                BOTH, 		"help!" },
    { 'V',	 "version",		    no_argument,		BOTH,		"Print out the version number." },
    { 'c', 	 "customers",	   	    optional_argument,	 	BOTH,	  	"Set the average number of customers per client to ARG." },
    { 'e', 	 "entries",	   	    required_argument,	 	LQNGEN_ONLY,  	"Set the average number of entries per task to ARG." },
    { 'i',	 "infinite-server",	    required_argument,		BOTH,	  	"Set the probability of a task being an infinite server to ARG." },
    { 'p', 	 "processor-multiplicity",  optional_argument,	 	BOTH,	  	"Set the average processor multiplicity to ARG." },
    { 's', 	 "service-time",	    optional_argument,	 	BOTH,	  	"Set the average phase service time to ARG." },
    { 't', 	 "task-multiplicity",	    optional_argument,	 	BOTH,	  	"Set the average task multiplicity to ARG." },
    { 'y', 	 "request-rate", 	    optional_argument,	 	BOTH,	  	"Set the average number of synchronous calls per rendezvous to ARG." },
    { 'z', 	 "think-time",	   	    optional_argument,	 	BOTH,	  	"Set the average client think time to ARG." },
    { '2', 	 "second-phase",	    required_argument,		LQNGEN_ONLY,  	"Set the probability of a second phase for an entry to ARG." },
    { 'o', 	 "output",		    required_argument,          LQN2LQX_ONLY,	"Set the output file name to ARG." },
    { 'v', 	 "verbose",         	    no_argument,                BOTH, 		"Verbose." },
    { 0x100+'x', "xml-output",      	    no_argument,                BOTH, 		"Output XML." },
    { 0x100+'X', "lqx-output",      	    no_argument,                BOTH, 		"Output XML model with LQX." },
    { 0x100+'S', "spex-output",     	    no_argument,                BOTH, 		"Output LQN model with SPEX." },
    { 0x100+'T', "transform",		    no_argument, 		LQNGEN_ONLY, 	"Transform the supplied input model (i.e., run as lqn2lqx)." },
    { 0x100+'4', "seed",            	    required_argument,		BOTH, 		"Seed value for random number generator." },
    { 0x100+'B', "binomial",		    no_argument,		BOTH, 		"Use a BINOMIAL distribution." },
    { 0x100+'C', "constant",		    no_argument,	        BOTH, 		"Use CONSTANT values." },
    { 0x100+'N', "normal",		    required_argument,		BOTH, 		"Use a NORMAL distribution with a standard deviation of ARG." },
    { 0x100+'G', "gamma",		    required_argument,		BOTH, 		"Use a GAMMA distribution with a shape of ARG." },
    { 0x100+'P', "poisson",		    no_argument,		BOTH, 		"Use a POISSON distribution." },
    { 0x100+'U', "uniform",		    required_argument,		BOTH, 		"Use a UNIFORM distribution with a spread of ARG." },
    { 0x100+'V', "funnel",          	    no_argument,                LQNGEN_ONLY,  	"Generate a model with more tasks at the bottom than at the top." },
    { 0x100+'A', "pyramid",         	    no_argument,                LQNGEN_ONLY,  	"Generate a model with more tasks at the top than at the bottom." },
    { 0x100+'F', "fat",			    no_argument,                LQNGEN_ONLY,  	"Generate a model with more tasks in the middle." },
    { 0x100+'H', "hour-glass", 		    no_argument,                LQNGEN_ONLY,  	"Generate a model with more tasks at the top and bottom than in the middle." },
    { 0x100+'R', "random",		    no_argument,		LQNGEN_ONLY,	"Generate a model with a random number of tasks in each layer." },
    { 0x100+'E', "deterministic",	    no_argument,		LQNGEN_ONLY,	"Assign tasks determinstically, evenly distributed among the layers." },
    { 0x100+'b', "breadth",		    no_argument,		LQNGEN_ONLY,    "Assign processors determinstically from left to right." },
    { 0x100+'d', "depth",		    no_argument,		LQNGEN_ONLY,    "Assign processors determinstically from top down." },
    { 0x100+'n', "no-variables",	    no_argument,		LQN2LQX_ONLY,   "Do not convert any constants to variables." },
    { 0x100+'o', "no-observation",	    no_argument,		BOTH,		"Do not insert any observation variables or code for SPEX/LQX output." },
    { 0x100+'c', "no-customers",	    no_argument, 		LQN2LQX_ONLY,	"Do not convert the number of customers to a variable." },
    { 0x100+'p', "no-processor-multiplicity",no_argument,  		LQN2LQX_ONLY,	"Do not convert the processor multiplicity to a variable." },
    { 0x100+'s', "no-service-time",	    no_argument, 		LQN2LQX_ONLY,	"Do not convert the phase service time to a variable." },
    { 0x100+'t', "no-task-multiplicity",    no_argument, 		LQN2LQX_ONLY,	"Do not convert the task multiplicity to a variable." },
    { 0x100+'y', "no-request-rate", 	    no_argument, 		LQN2LQX_ONLY,	"Do not convert the number of synchronous calls per rendezvous to a variable." },
    { 0x100+'z', "no-think-time",	    no_argument, 		LQN2LQX_ONLY,	"Do not convert the client think time to a variable." },
    { 0x200+'i', "input-parameters",	    no_argument,		BOTH,  		"[Do not] output non-constant input parameters for SPEX/LQX output." },
    { 0x200+'f', "throughput",	     	    no_argument,		BOTH,  		"[Do not] observe task throughput for SPEX/LQX output." },
    { 0x200+'r', "residence-time",	    no_argument,		BOTH,  		"[Do not] observe entry service (residence) time for SPEX/LQX output." },
    { 0x200+'w', "waiting-time",	    no_argument,		BOTH,  		"[Do not] observe request waiting (queueing) time for SPEX/LQX output." },
    { 0x200+'u', "utilization",	   	    no_argument,		BOTH,  		"[Do not] observe processor utilization for SPEX/LQX output." },
    { 0x200+'M', "mva-waits",		    no_argument,		BOTH,  		"[Do not] observe the number of calls to wait() for SPEX/LQX output." },
    { 0x200+'I', "iterations",		    no_argument,		BOTH,  		"[Do not] observe the number of solver iterations for SPEX/LQX output." },
    { 0x200+'E', "elapsed-time", 	    no_argument,		BOTH,  		"[Do not] observe the solver's ELAPSED time for SPEX/LQX output." },
    { 0x200+'U', "user-cpu-time",	    no_argument,		BOTH,  		"[Do not] observe the solver's USER CPU time for SPEX/LQX output." },
    { 0x200+'S', "system-cpu-time",	    no_argument,		BOTH,  		"[Do not] observe the solver's SYSTEM CPU time for SPEX/LQX output." },
    { 0x100+'%', "comment",		    required_argument,        	BOTH, 		"Set the model comment to ARG." },
    { 0x100+'1', "convergence-value",	    required_argument,        	BOTH,		"Set the model convergence value to ARG." },
    { 0x100+'2', "under-relaxation",	    required_argument,        	BOTH, 		"Set the model under-relaxation to ARG." },
    { 0x100+'3', "iteration-limit",	    required_argument,        	BOTH,		"Set the model iteration limit to ARG." },
    { 0x200+'a', "annotate",     	    no_argument,                BOTH, 		"[Do not] annotate the resulting model file." },
    { 0x100+'L', "long-names",		    no_argument,		LQNGEN_ONLY,    "Use long names (Processor,Task,Entry)." },
    { 0x100+'M', "manual-page",		    no_argument,		BOTH,  		"Make the manual page." },
    { 0,   	 0,                 	    no_argument,                BOTH, 		0 }
};

/* Initialize FLOATS at run time.  Value is interpreted as an INTEGER */

static int lqngen( int argc, char *argv[0] );
int lqn2lqx( int argc, char *argv[0] );
static void reset_RV( RV::RandomVariable ** rv, const RV::RandomVariable *, double mean );
static void severity_action(unsigned severity);
static void multi( const std::string& );
static bool check_multiplicity( const RV::RandomVariable * );
static bool check_argument( const RV::RandomVariable * );

bool fixedModel = true;

bool Flags::verbose = false;
bool Flags::annotate_input = true;
output_format_t Flags::output_format  = FORMAT_SRVN;
bool Flags::lqx_spex_output = false;
bool Flags::lqn2lqx = false;
bool Flags::long_names = false;
std::vector<bool> Flags::observe(Flags::N_OBSERVATIONS,false);
std::vector<bool> Flags::convert(Flags::N_PARAMETERS,true);
std::vector<bool> Flags::override(Flags::N_PARAMETERS,true);
unsigned int Flags::number_of_runs = 1;
unsigned int Flags::number_of_models = 1;
double Flags::sensitivity = 0;

lqio_params_stats io_vars =
{
    /* .n_processors =   */ 0,
    /* .n_tasks =	 */ 0,
    /* .n_entries =      */ 0,
    /* .n_groups =       */ 0,
    /* .lq_toolname =    */ NULL,
    /* .lq_version =	 */ VERSION,
    /* .lq_command_line =*/ NULL,
    /* .severity_action= */ severity_action,
    /* .max_error =      */ 0,
    /* .error_count =    */ LQIO::LSTGBLERRMSG,
    /* .severity_level = */ LQIO::NO_ERROR,
    /* .error_messages = */ &LQIO::global_error_messages[0],
    /* .anError =        */ 0
};

static RV::RandomVariable * continuous_default;
static RV::RandomVariable * discreet_default;
static bool some_randomness = false;
static const char * output_suffix[] = { "xlqn", "lqnx", "json" }; 		/* Match Options::io above */
static std::string output_file_name;

/*----------------------------------------------------------------------*/
/*			      Main line					*/
/*----------------------------------------------------------------------*/

int
main( int argc, char *argv[] )
{
    srand48( 12345678L );					/* Init now, may be reset with --seed */
    continuous_default = new RV::Gamma( 0.5, 2.0 );		/* scale,shape. Mean is one E[x] = k x gamma */
//    discreet_default = new RV::Binomial( 1, 3 );		/* Mean is (arg2 - arg1) / 2 + arg1 */
    discreet_default = new RV::Poisson( 2, 1 );			/* Mean is (arg1 - arg2) / 2 + arg1 */

    Generate::__service_time 		    = new RV::Constant( 1.0 );
    Generate::__think_time 		    = new RV::Constant( 1.0 );
    Generate::__forwarding_probability      = new RV::Constant( 0.0 );
    Generate::__rendezvous_rate 	    = new RV::Constant( 1.0 );
    Generate::__send_no_reply_rate 	    = new RV::Constant( 0.0 );
    Generate::__customers_per_client 	    = new RV::Constant( 1.0 );
    Generate::__task_multiplicity 	    = new RV::Constant( 1.0 );
    Generate::__processor_multiplicity      = new RV::Constant( 1.0 );
    Generate::__number_of_entries	    = new RV::Constant( 1.0 );
    Generate::__probability_second_phase    = 0.0;
    Generate::__probability_infinite_server = 0.0;

    Flags::observe[Flags::PARAMETERS] = true;
    Flags::observe[Flags::UTILIZATION] = true;
    Flags::observe[Flags::THROUGHPUT] = true;
    Flags::observe[Flags::RESIDENCE_TIME] = true;
    Flags::observe[Flags::QUEUEING_TIME] = true;

    io_vars.lq_toolname = strrchr( argv[0], '/' );
    if ( io_vars.lq_toolname ) {
        io_vars.lq_toolname += 1;
    } else {
        io_vars.lq_toolname = argv[0];
    }

    /* Set flags used by lqngen */

    if ( strcmp( "lqn2lqx", io_vars.lq_toolname ) == 0 ) {
	Flags::lqn2lqx = true;
	Flags::lqx_spex_output = true;
	Flags::annotate_input = false;
    }
    extern char *optarg;

    static string opts = "";
#if HAVE_GETOPT_H
    int optflag = 0;
    static std::vector<struct option> longopts;
    makeopts( opts, longopts, &optflag );
#if __cplusplus < 201103L
    LQIO::CommandLine command_line( opts, &longopts.front() );
#else
    LQIO::CommandLine command_line( opts, longopts.data() );
#endif
#else
    makeopts( opts );
    LQIO::CommandLine command_line( opts );
#endif
    command_line = io_vars.lq_toolname;
    
    optarg = 0;
    for ( ;; ) {
	char * endptr = 0;
	int optind = 0;
#if HAVE_GETOPT_LONG
#if __cplusplus < 201103L
	int c = getopt_long( argc, argv, opts.c_str(), &longopts.front(), &optind );
#else
	int c = getopt_long( argc, argv, opts.c_str(), longopts.data(), &optind );
#endif
#else	
	const int c = getopt( argc, argv, opts.c_str() );
#endif
	if ( c == EOF ) {
	    break;
#if HAVE_GETOPT_LONG
	} else if ( c == ':' ) {
	    c = optflag;			/* longopt with no arg. */
	    optarg = 0;
	} else if ( c == 0 ) {
	    c = optflag;			/* longopt with arg. */
#endif
	}

        command_line.append( c, optarg );

	try {
	    switch( c ) {
	    
	    case 0x100+'%':
		Generate::__comment = optarg;
		break;

	    case 0x100+'1':
		Generate::__convergence_value = strtod( optarg, &endptr );
		if ( Generate::__convergence_value <= 0 ) {
		    cerr << io_vars.lq_toolname << "convergence=" << Generate::__convergence_value << " is invalid, choose non-negative real." << endl;
		    (void) exit( 3 );
		}
		break;

	    case '2':
		if ( Flags::lqn2lqx ) throw c;
		Generate::__probability_second_phase = strtod( optarg, &endptr );
		break;

	    case 0x100+'2':
		Generate::__convergence_value = strtod( optarg, &endptr );
		if ( Generate::__convergence_value <= 0.0 ) {
		    cerr << io_vars.lq_toolname << "convergence=" <<  Generate::__convergence_value << " is invalid, choose non-negative real." << endl;
		    exit( 3 );
		}
		break;

	    case 0x100+'3':
		Generate::__iteration_limit = static_cast<unsigned int>(strtol( optarg, &endptr, 10 ));
		if ( Generate::__iteration_limit == 0 ) {
		    cerr << io_vars.lq_toolname << "iteration-limit=" << Generate::__iteration_limit << " is invalid, choose non-negative integer." << endl;
		}
		break;

	    case 0x100+'4':
		srand48( strtol( optarg, &endptr, 10 ) );
		break;

	    case 0x200+'a':
		Flags::annotate_input = true;
		break;

	    case 0x400+'a':
		Flags::annotate_input = false;
		break;

	    case 'A': 
		if ( Flags::lqn2lqx ) {
		    throw c;
		} else {
		    const double arg = static_cast<double>(strtol( optarg, &endptr, 10 ));
		    if ( arg < 1. ) {
			throw std::domain_error( "argument is not a positive integer." );
		    }
		    Flags::annotate_input = false;
		    Generate::__number_of_tasks = static_cast<unsigned>(arg);
		    RV::RandomVariable * rv = discreet_default->clone();
		    rv->setMean( sqrt( arg ) );
//		    cerr << *rv << endl;
		    Generate::__customers_per_client = rv;
		    Generate::__number_of_clients    = (*rv)();
		    Generate::__number_of_layers     = (*rv)();
		    Generate::__number_of_processors = (*rv)();
		    Generate::__task_layering =      Generate::RANDOM_LAYERING;
		    Generate::__processor_layering = Generate::RANDOM_LAYERING;
		    reset_RV( &Generate::__task_multiplicity, discreet_default, 1.5 );
		    reset_RV( &Generate::__processor_multiplicity, discreet_default, 1.5 );
		    reset_RV( &Generate::__service_time, continuous_default, 1.0 );
		    reset_RV( &Generate::__rendezvous_rate, continuous_default, 1.0 );
		    reset_RV( &Generate::__number_of_entries, discreet_default, 1.2 );
		    reset_RV( &Generate::__think_time, continuous_default, 1.0 );
		    Generate::__outgoing_requests = (*Generate::__number_of_entries)();
		}
		break;
		
	    case 0x100+'A':
		if ( Flags::lqn2lqx ) throw c;
		Generate::__task_layering = Generate::PYRAMID_LAYERING;
		break;

	    case 0x100+'b': /* Assign processors deterministically by depth." */
		if ( Flags::lqn2lqx ) throw c;
		Generate::__processor_layering = Generate::BREADTH_FIRST_LAYERING;
		break;
		
	    case 0x100+'B':
		if ( discreet_default ) delete discreet_default;
		discreet_default = new RV::Binomial( 1, 3 );		/* low, high */
		break;
		
	    case 'c':			/* customers */
		if ( optarg ) {
		    reset_RV( &Generate::__customers_per_client, discreet_default, strtod( optarg, &endptr ) );
		    if ( !check_multiplicity( Generate::__customers_per_client ) ) throw std::domain_error( "The mean number of clients must be greater than one." );
		    Flags::override[Flags::CUSTOMERS] = true;
		} else if ( !Flags::lqn2lqx ) {
		    throw std::domain_error( "option requires an argument" );
		}
		Flags::convert[Flags::CUSTOMERS] = true;
		break;

	    case 0x100+'c':		/* "no-customers" */
		if ( !Flags::lqn2lqx ) throw c;
		Flags::override[Flags::CUSTOMERS] = false;
		Flags::convert[Flags::CUSTOMERS] = false;
		break;

	    case 'C':
		if ( Flags::lqn2lqx ) throw c;
		Generate::__number_of_clients = strtol( optarg, &endptr, 10 );
		if ( (int)Generate::__number_of_clients < 1 ) throw std::domain_error( "The number of clients must be greater than zero." );
		break;

	    case 0x100+'C':		/* Constant */
		if ( continuous_default ) delete continuous_default;
		continuous_default = new RV::Constant( 1 );
		if ( discreet_default ) delete discreet_default;
		discreet_default = new RV::Constant( 1 );
		break;

	    case 0x100+'d': /* Assign processors deterministically by depth." */
		if ( Flags::lqn2lqx ) throw c;
		Generate::__processor_layering = Generate::DEPTH_FIRST_LAYERING;
		break;
		
	    case 'e':
		if ( Flags::lqn2lqx ) throw c;
		reset_RV( &Generate::__number_of_entries, discreet_default, strtod( optarg, &endptr ) );
		if ( dynamic_cast<RV::Constant *>(Generate::__number_of_entries) && floor((*Generate::__number_of_entries)()) != ceil((*Generate::__number_of_entries)()) ) {
		    throw std::domain_error( "argument is not a positive integer." );
		}
		break;

	    case 0x100+'E':
		if ( Flags::lqn2lqx ) throw c;
		Generate::__task_layering = Generate::DETERMINISTIC_LAYERING;
		break;
		
	    case 0x200+'E':
		Flags::observe[Flags::ELAPSED_TIME] = true;
		break;

	    case 0x400+'E':
		Flags::observe[Flags::ELAPSED_TIME] = false;
		break;

	    case 0x200+'f':
		Flags::observe[Flags::THROUGHPUT] = true;
		break;

	    case 0x400+'f':
		Flags::observe[Flags::THROUGHPUT] = false;
		break;

	    case 0x100+'F':
		if ( Flags::lqn2lqx ) throw c;
		Generate::__task_layering = Generate::FAT_LAYERING;
		break;

	    case 0x100+'G':
		if ( continuous_default ) delete continuous_default;
		continuous_default = new RV::Gamma( 1, strtod( optarg, &endptr ) );
		break;

	    case 'H':
		usage();
		exit( 1 );

	    case 0x100+'H':
		if ( Flags::lqn2lqx ) throw c;
		Generate::__task_layering = Generate::HOUR_GLASS_LAYERING;
		break;
		
	    case 'i':
		Generate::__probability_infinite_server = strtod( optarg, &endptr );
		break;

	    case 0x200+'i':
		Flags::observe[Flags::PARAMETERS] = true;
		break;
	
	    case 0x400+'i':
		Flags::observe[Flags::PARAMETERS] = false;
		break;
	
	    case 0x200+'I':
		Flags::observe[Flags::ITERATIONS] = true;
		break;

	    case 0x400+'I':
		Flags::observe[Flags::ITERATIONS] = false;
		break;

	    case 'L':
		if ( Flags::lqn2lqx ) throw c;
		Generate::__number_of_layers = strtol( optarg, &endptr, 10 );
		if ( (int)Generate::__number_of_layers < 1 ) throw std::domain_error( "The number of layers must be greater than zero." );
		break;

	    case 0x100+'L':
		if ( Flags::lqn2lqx ) throw c;
		Flags::long_names = true;
		break;
		
	    case 'M':
		Flags::number_of_models = strtol( optarg, &endptr, 10 );	/* Doesn't work with lqn2lqx	*/
		break;								/* nor does it make sense	*/

	    case 0x100+'M':
		man();
		exit( 0 );
	    
	    case 0x200+'M':
		Flags::observe[Flags::MVA_WAITS] = true;
		break;

	    case 0x400+'M':
		Flags::observe[Flags::MVA_WAITS] = false;
		break;

	    case 0x100+'n':
		std::transform( Flags::convert.begin(), Flags::convert.end(), Flags::convert.begin(), Flags::set_false );
		std::transform( Flags::override.begin(), Flags::override.end(), Flags::override.begin(), Flags::set_false );
		break;
		
	    case 0x100+'N':
		if ( continuous_default ) delete continuous_default;
		continuous_default = new RV::Normal( 1, strtod( optarg, &endptr ) );	/* Mean, stddev */
		break;

	    case 'N':
		Flags::number_of_runs = strtol( optarg, &endptr, 10 );
		break;

	    case 'o':
		if ( !Flags::lqn2lqx ) throw c;
		output_file_name = optarg;
		break;

	    case 0x100+'o':
		std::transform( Flags::observe.begin(), Flags::observe.end(), Flags::observe.begin(), Flags::set_false );
		break;
		
	    case 'O': {
		char * old_optarg = optarg;
		static const char * const strings [] = { "lqn", "xml", 0 };
		int arg = getsubopt( &optarg, const_cast<char *const *>(strings), &endptr );
		switch ( arg ) {
		case FORMAT_SRVN:
		case FORMAT_XML:
		    Flags::output_format = static_cast<output_format_t>(arg);
		    break;
		default:
		    ::invalid_argument( c, old_optarg );
		    exit( 1 );
		} }
		break;

	    case 'p':			/* processor-multiplicity */
		if ( optarg ) {
		    reset_RV( &Generate::__processor_multiplicity, discreet_default, strtod( optarg, &endptr ) );
		    if ( !check_multiplicity( Generate::__processor_multiplicity ) ) throw std::domain_error( "The mean processor multiplicity must be greater than one." );
		    Flags::override[Flags::PROCESSOR_MULTIPLICITY] = true;
		} else if ( !Flags::lqn2lqx ) {
		    throw std::domain_error( "option requires an argument" );
		}
		Flags::convert[Flags::PROCESSOR_MULTIPLICITY] = true;
		break;

	    case 0x100+'p':		/* "no-processor-multiplicity" */
		if ( !Flags::lqn2lqx ) throw c;
		Flags::override[Flags::PROCESSOR_MULTIPLICITY] = true;
		Flags::convert[Flags::PROCESSOR_MULTIPLICITY] = false;
		break;

	    case 'P':
		if ( Flags::lqn2lqx ) throw c;
		Generate::__number_of_processors = strtol( optarg, &endptr, 10 );
		if ( (int)Generate::__number_of_processors < 1 ) throw std::domain_error( "The number of processors must be greater than zero." );
		break;

	    case 0x100+'P':
		if ( discreet_default ) delete discreet_default;
		discreet_default = new RV::Poisson( 2, 1 );	/* Mean Offset */
		break;

	    case 0x100+'R':
		if ( Flags::lqn2lqx ) throw c;
		Generate::__task_layering = Generate::RANDOM_LAYERING;
		break;

	    case 0x200+'r':
		Flags::observe[Flags::RESIDENCE_TIME] = true;
		break;

	    case 0x400+'r':
		Flags::observe[Flags::RESIDENCE_TIME] = false;
		break;

	    case 's':			/* service-time */
		if ( optarg ) {
		    reset_RV( &Generate::__service_time, continuous_default, strtod( optarg, &endptr ) );
		    if ( !check_argument( Generate::__service_time ) ) throw std::domain_error( "Service time must be greater than zero." );
		    Flags::override[Flags::SERVICE_TIME] = true;
		} else if ( !Flags::lqn2lqx ) {
		    throw std::domain_error( "option requires an argument" );
		}
		Flags::convert[Flags::SERVICE_TIME] = true;
		break;

	    case 0x100+'s':		/* "no-service-time" */
		if ( !Flags::lqn2lqx ) throw c;
		Flags::override[Flags::SERVICE_TIME] = false;
		Flags::convert[Flags::SERVICE_TIME] = false;
		break;

	    case 'S':
		if ( !Flags::lqn2lqx ) throw c;
		Flags::sensitivity = strtod( optarg, &endptr );
		break;

	    case 0x100+'S':
		Flags::lqx_spex_output = true;
		Flags::output_format = FORMAT_SRVN;
		break;

	    case 0x200+'S':
		Flags::observe[Flags::SYSTEM_TIME] = true;
		break;

	    case 0x400+'S':
		Flags::observe[Flags::SYSTEM_TIME] = false;
		break;

	    case 't':			/* task-multiplicity */
		if ( optarg ) {
		    reset_RV( &Generate::__task_multiplicity, discreet_default, strtod( optarg, &endptr ) );
		    if ( !check_multiplicity( Generate::__task_multiplicity ) ) throw std::domain_error( "The mean task multiplicity must be greater than one." );
		    Flags::override[Flags::TASK_MULTIPLICITY] = true;
		} else if ( !Flags::lqn2lqx ) {
		    throw std::domain_error( "option requires an argument" );
		}
		Flags::convert[Flags::TASK_MULTIPLICITY] = true;
		break;

	    case 0x100+'t':		/* "no-task-multiplicity" */
		if ( !Flags::lqn2lqx ) throw c;
		Flags::override[Flags::TASK_MULTIPLICITY] = false;
		Flags::convert[Flags::TASK_MULTIPLICITY] = false;
		break;

	    case 'T':
		if ( Flags::lqn2lqx ) throw c;
		Generate::__number_of_tasks = strtol( optarg, &endptr, 10 );
		if ( (int)Generate::__number_of_tasks < 1 ) throw std::domain_error( "The number of tasks must be greater than zero." );
		break;

	    case 0x100+'T':
		Flags::lqn2lqx = true;
		Flags::lqx_spex_output = true;
		Flags::annotate_input = false;
		break;
		
	    case 0x200+'u':
		Flags::observe[Flags::UTILIZATION] = true;
		break;

	    case 0x400+'u':
		Flags::observe[Flags::UTILIZATION] = false;
		break;

	    case 0x100+'U':		/* Uniform Distribution */
		if ( continuous_default ) delete continuous_default;
		continuous_default = new RV::Uniform( 0, strtod( optarg, &endptr ) );
		if ( discreet_default ) delete discreet_default;		/* Arg and spread should be > 1 */
		discreet_default = new RV::Uniform( 1, strtod( optarg, &endptr ) + 1 );
		break;

	    case 0x200+'U':
		Flags::observe[Flags::USER_TIME] = true;
		break;

	    case 0x400+'U':
		Flags::observe[Flags::USER_TIME] = false;
		break;

	    case 0x200+'w':
		Flags::observe[Flags::QUEUEING_TIME] = true;
		break;
		
	    case 0x400+'w':
		Flags::observe[Flags::QUEUEING_TIME] = false;
		break;
		
	    case 0x100+'x':
		Flags::output_format = FORMAT_XML;
		break;

	    case 0x100+'X':
		Flags::output_format = FORMAT_XML;
		Flags::lqx_spex_output  = true;
		break;

	    case 'v':
		Flags::verbose = true;
		break;

	    case 'V':
		cerr << "Layered Queueing Network Generator, Version " << VERSION << endl << endl;
		break;
		
	    case 0x100+'V':
		if ( Flags::lqn2lqx ) throw c;
		Generate::__task_layering = Generate::FUNNEL_LAYERING;
		break;

	    case 'y':			/* rendezvous-rate */
		if ( optarg ) {
		    reset_RV( &Generate::__rendezvous_rate, continuous_default, strtod( optarg, &endptr ) );
		    if ( !check_argument( Generate::__service_time ) ) throw std::domain_error( "Request rate must be greater than zero." );
		    Flags::override[Flags::REQUEST_RATE] = true;
		} else if ( !Flags::lqn2lqx ) {
		    throw std::domain_error( "option requires an argument" );
		}
		Flags::convert[Flags::REQUEST_RATE] = true;
		break;

	    case 0x100+'y':		/* "no-request-rate" */
		if ( !Flags::lqn2lqx ) throw c;
		Flags::override[Flags::REQUEST_RATE] = false;
		Flags::convert[Flags::REQUEST_RATE] = false;
		break;

	    case 'Y':
		if ( Flags::lqn2lqx ) throw c;
		Generate::__outgoing_requests = strtod( optarg, &endptr );
		if ( Generate::__outgoing_requests < 1.0 ) {
		    ::invalid_argument( c, optarg );				/* Must be > 1.0 because we can't remove. */
		    exit( 1 );
		}
		break;
		
	    case 'z':
		if ( optarg ) {
		    reset_RV( &Generate::__think_time, continuous_default, strtod( optarg, &endptr ) );
		    if ( !check_argument( Generate::__service_time ) ) throw std::domain_error( "Think time must be greater than zero." );
		    Flags::override[Flags::THINK_TIME] = true;
		} else if ( !Flags::lqn2lqx ) {
		    throw std::domain_error( "option requires an argument" );
		}
		Flags::convert[Flags::THINK_TIME] = true;
		break;

	    case 0x100+'z':		/* "no-think-time" */
		if ( !Flags::lqn2lqx ) throw c;
		Flags::override[Flags::THINK_TIME] = false;
		Flags::convert[Flags::THINK_TIME] = false;
		break;

	    default:
		help();
		exit( 1 );
	    }

	    if ( endptr != 0 && *endptr != '\0' ) {
		::invalid_argument( c, optarg );
		exit( 1 );
	    }
	}

	catch ( std::domain_error err ) {
	    if ( !optarg ) {
		cerr << io_vars.lq_toolname << ": " << err.what() << " -- '"  << longopts.at(optind).name << "'." << endl;
		help();
	    } else {
		::invalid_argument( c, optarg, err.what() );
	    }
	    exit( 1 );
	}
	catch ( int e ) {
	    ::invalid_option( e );
	    exit( 1 );
	}
    }
    io_vars.lq_command_line = command_line.c_str();
    
    int rc = 0;
    if ( Flags::lqn2lqx ) {
	rc = lqn2lqx( argc, argv );		/* */
    } else {
	rc = lqngen( argc, argv );
    }
    return rc;
}


/*
 * Generate a model
 */

static int
lqngen( int argc, char *argv[0] )
{
    if ( Generate::__number_of_layers > Generate::__number_of_tasks ) {
	if ( Generate::__number_of_tasks == 1 ) {
	    Generate::__number_of_tasks = Generate::__number_of_layers;
	} else {
	    fprintf( stderr, "%s: Too many layers for %d tasks.\n", argv[0], Generate::__number_of_tasks );
	    exit( 1 );
	}
    }
    if ( Generate::__number_of_processors > Generate::__number_of_tasks ) {
	fprintf( stderr, "%s: Too many processors for %d tasks (warning only).\n", argv[0], Generate::__number_of_tasks );
	Generate::__number_of_processors = Generate::__number_of_tasks;
    }

    if ( !some_randomness && Flags::number_of_runs > 1 ) {
	fprintf( stderr, "%s: Multiple experiment runs, but no randomness in model.  Use any or all of -s, -t, -p, -y to set randomness.\n", argv[0] );
	exit( 1 );
    }

    /* If multiple experiments, force spex/lqx code */
    if ( Flags::number_of_runs > 1 ) {
	Flags::lqx_spex_output = true;
    }

    /* If not spex output, turn off all observations */
    if ( !Flags::lqx_spex_output ) {
	std::transform( Flags::observe.begin(), Flags::observe.end(), Flags::observe.begin(), Flags::set_false );
    }
    
    if ( Generate::__comment.size() == 0 ) {
	Generate::__comment = io_vars.lq_command_line;
    }

    if ( argc == optind ) {
	output_file_name = "";
	
	LQIO::input_file_name = strdup( "" );

	if ( Flags::number_of_models > 1 ) {
	    cerr << io_vars.lq_toolname << ": a directory name is required as an argument for the option '--models=" 
		 << Flags::number_of_models << "." << std::endl;
	    exit ( 1 );
	}
	Generate model( Generate::__number_of_layers, Flags::number_of_runs );
	cout << model();

    } else if ( argc == optind + 1 ) {

	if ( Flags::number_of_models <= 1 ) {
	    ofstream output_file;
	    output_file_name = argv[optind];
	    LQIO::Filename::backup( output_file_name.c_str() );
	    output_file.open( argv[optind], ios::out );
	    if ( !output_file ) {
		cerr << io_vars.lq_toolname << ": Cannot open output file " << output_file_name << " - "
		     << strerror( errno ) << endl;
		exit ( 1 );
	    }
	    
	    LQIO::input_file_name = strdup( output_file_name.c_str() );
	    Generate model( Generate::__number_of_layers, Flags::number_of_runs );
	    output_file << model();
	    output_file.close();

	} else {
	    multi( argv[optind] );
	}

    } else {

	cerr << io_vars.lq_toolname << ": arg count." << endl;

    }

    return 0;
}


int
lqn2lqx( int argc, char **argv )
{
    unsigned int errorCode;
    
    if ( Flags::number_of_runs > 1 && Flags::sensitivity > 0 ) {
	fprintf( stderr, "%s: --experiments=%d and --sensitivity=%g are mutually exclusive.\n", io_vars.lq_toolname, Flags::number_of_runs, Flags::sensitivity );
	exit( 1 );
    }

    if ( optind == argc ) {
	LQIO::DOM::Document* document = LQIO::DOM::Document::load( "-", LQIO::DOM::Document::AUTOMATIC_INPUT, "", &io_vars, errorCode, false );
	if ( document ) {
	    Generate aModel( document, Flags::number_of_runs );
	    aModel.reparameterize();
	    if ( output_file_name == "-" ) {
		cout << aModel;
	    } else {
		LQIO::Filename::backup( output_file_name.c_str() );
		
		std::ofstream output;
		output.open( output_file_name.c_str(), ios::out|ios::binary );
		if ( !output ) {
		    cerr << "Cannot open output file " << output_file_name << " - " << strerror( errno );
		} else {
		    output << aModel;
		    output.close();
		}
	    }
	}

    } else if ( output_file_name.size() > 0 && argc - optind != 1 ) {
	cerr << io_vars.lq_toolname << ": Only one input file can be specified when using --output=filename." << endl;
	exit( 1 );
    } else {
	for ( ;optind < argc; ++optind ) {
	    LQIO::DOM::Document* document = LQIO::DOM::Document::load( argv[optind], LQIO::DOM::Document::AUTOMATIC_INPUT, "", &io_vars, errorCode, false );
	    if ( !document ) {
		continue;
	    }

	    Generate aModel( document, Flags::number_of_runs );
	    aModel.reparameterize();
	    
	    if ( output_file_name == "-" ) {
		cout << aModel;
	    } else {
		LQIO::Filename filename;
		if ( output_file_name.size() ) {
		    filename = output_file_name.c_str();
		} else {
		    filename.generate( argv[optind], output_suffix[Flags::output_format] );
		}
		filename.backup();		/* Overwriting input file. -- back up */

		ofstream output;
		output.open( filename(), ios::out );
		if ( !output ) {
		    cerr << io_vars.lq_toolname << ": Cannot open output file " << filename() << " - " << strerror( errno ) << endl;
		    exit ( 1 );
		}
		output << aModel;
		output.close();
	    }
	}
    }
    return 0;
}

static void
reset_RV( RV::RandomVariable ** rv, const RV::RandomVariable * default_rv, double mean ) 
{
    if ( !dynamic_cast<const RV::Constant *>(default_rv) ) some_randomness = true;
    if ( (*rv) ) delete (*rv);
    (*rv) = default_rv->clone();
    (*rv)->setMean( mean );
}


/*
 * Constant can be 1, or mean > offset 
 */

static bool
check_multiplicity( const RV::RandomVariable * rv )
{
    if ( dynamic_cast<const RV::Constant *>(rv) ) return (*rv)() >= 1;
    else if ( dynamic_cast<const RV::Uniform *>(rv) ) return 1 <= rv->getArg(1) && rv->getArg(1) <= rv->getArg(2);
    else return rv->getMean() > 1.;
}


static bool
check_argument( const RV::RandomVariable * rv )
{
    if ( dynamic_cast<const RV::Constant *>(rv) ) return (*rv)() >= 0;
    else if ( dynamic_cast<const RV::Uniform *>(rv) ) return 0 <= rv->getArg(1) && rv->getArg(1) < rv->getArg(2);
    else return rv->getMean() > 0.;
}


#if HAVE_GETOPT_H
void
makeopts( string& opts, std::vector<struct option>& longopts, int * optflag ) 
{
    struct option opt;
    opt.flag = optflag;
    for ( unsigned int i = 0; options[i].name || options[i].c ; ++i ) {
	opt.has_arg = options[i].has_arg;
	opt.val = options[i].c;
	opt.name = options[i].name;
	longopts.push_back( opt );
	if ( (options[i].c & 0xff00) == 0 ) {
	    opts +=  static_cast<char>(options[i].c);
	    if ( options[i].has_arg != no_argument ) {
		opts += ':';
	    }
	}
	if ( (options[i].c & 0x0200) != 0 ) {
	    /* Make the "no-" version */
	    opt.val = (options[i].c & 0x00ff) | 0x0400;
	    std::string name = "no-";
	    name += options[i].name;
	    opt.name = strdup( name.c_str() );
	    longopts.push_back( opt );
	}
    }
    opt.name = 0;
    opt.val  = 0;
    opt.has_arg = 0;
    opt.flag = 0;
    longopts.push_back( opt );
}
#else
static void
makeopts( string& opts ) 
{
    for ( unsigned int i = 0; options[i].name || options[i].c ; ++i ) {
	if ( (options[i].c & 0xff00) == 0 ) {
	    opts +=  static_cast<char>(options[i].c);
	    if ( Geneate::options[i].arg ) {
		opts += ':';
	    }
	}
    }
}
#endif

/*
 * Generate multiple experiments.  This DOES not work for lqn2lqx because the
 * document is clobbered when we run reparameterize().
 */

static void
multi( const std::string& dir )
{
    struct stat sb;
    if ( stat ( dir.c_str(), &sb ) < 0 ) {
	if ( errno != ENOENT ) {
	    cerr << io_vars.lq_toolname << ": " << strerror( errno ) << endl;
	    exit ( 1 );
	} else if ( mkdir( dir.c_str()
#if !defined(WINNT) && !defined(MSDOS)
			   ,S_IRWXU|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH
#endif
			) < 0 ) {
	    cerr << io_vars.lq_toolname << ": " << strerror( errno ) << endl;
	    exit( 1 );
	}
    } else if ( !S_ISDIR( sb.st_mode ) ) {
	cerr << io_vars.lq_toolname << ": Cannot output multiple files to " << dir << endl;
	exit( 1 );
    }

    int w = static_cast<int>(log10( static_cast<double>(Flags::number_of_models) )) + 1;

    for ( unsigned i = 1; i <= Flags::number_of_models; ++i ) {
	std::ostringstream file_name;
	file_name << dir << "/case-" << setw( w ) << setfill( '0' ) << i << "." << output_suffix[Flags::output_format];
	ofstream output_file;
	output_file.open( file_name.str().c_str(), ios::out );

	if ( !output_file ) {
	    cerr << io_vars.lq_toolname << ": Cannot open output file " << file_name.str() << " - " << strerror( errno ) << endl;
	    exit ( 1 );
	}

	LQIO::input_file_name = strdup( file_name.str().c_str() );
	Generate aModel( Generate::__number_of_layers, Flags::number_of_runs );
	output_file << aModel();
	output_file.close();
    }
}



/*
 * What to do based on the severity of the error.
 */

static void
severity_action (unsigned severity)
{
    switch( severity ) {
	case LQIO::FATAL_ERROR:
	(void) abort();
	break;

    case LQIO::RUNTIME_ERROR:
	io_vars.anError = true;
	io_vars.error_count += 1;
	if  ( io_vars.error_count >= 10 ) {
	    throw runtime_error( "Too many errors" );
	}
	break;
    }
}
