/* lqngen.cc	-- Greg Franks Thu Jul 29 2004
 * Model file generator.
 * This is actually part of lqn2ps, but if lqn2ps is invoked as lqngen, then this magically runs.
 *
 * $Id: lqngen.cc 14523 2021-03-06 22:53:02Z greg $
 */

#include "lqngen.h"

#include <fstream>
#include <sstream>
#include <cstring>
#include <getopt.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <libgen.h>
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
static void initialize();
static void get_pragma( const std::string& );
static RV::RandomVariable * get_RV( const std::string&, RV::RandomVariable * new_dist, RV::RandomVariable::distribution_t );
static RV::RandomVariable * get_RV_args( RV::RandomVariable *, const std::string& );

/* Sort on field 5.  Note that 0x400 is [no-] */
option_type options[] =
{
    { 'A',  	 "automatic",		    required_argument,	 	LQNGEN_ONLY,  	"Create a model with exactly ARG layers, clients, processor and ARG**2 tasks." },
    { 'Q',	 "queueing-model",	    required_argument,		LQNGEN_ONLY,	"Create a model for testing a queueing network with exactly ARG customers." },
    { 'C', 	 "clients",         	    required_argument,	 	LQNGEN_ONLY,  	"Create ARG (default: constant) client (Reference) tasks." },
    { 'L', 	 "layers",          	    required_argument,	 	LQNGEN_ONLY,  	"Create ARG (default: constant) layers." },
    { 'P', 	 "processors",      	    required_argument,	 	LQNGEN_ONLY,  	"Create ARG (default: constant) processors." },
    { 'G',	 "group",		    required_argument,		LQNGEN_ONLY,    "Set the probability of a processor using Completely Fair Scheduling to ARG." },
    { 'T', 	 "tasks",           	    required_argument,	 	LQNGEN_ONLY,  	"Create ARG (default: constant) tasks (excluding reference tasks)." },
    { 'E', 	 "entries",	   	    required_argument,	 	LQNGEN_ONLY,  	"Set the average number of entries per task to ARG." },
    { 'Y', 	 "outgoing-requests",	    required_argument,		LQNGEN_ONLY,	"Create an average of ARG reqeusts from each entry." },
    { 'M', 	 "models",	   	    required_argument,	 	LQNGEN_ONLY,  	"Create ARG different models." },
    { 'N', 	 "experiments",     	    required_argument,	 	BOTH, 		"Create ARG experiments." },
    { 'S', 	 "sensitivity",		    required_argument,	    	LQN2LQX_ONLY,   "Create a factorial experiment with services times of increased/decreased by multiplying by ARG ." },
    { 'O', 	 "format",          	    required_argument,          BOTH, 		"Set output format to ARG (lqn,xml,lqx)." },
    { 'H', 	 "help",            	    no_argument,                BOTH, 		"help!" },
    { 'V',	 "version",		    no_argument,		BOTH,		"Print out the version number." },
    { 'c', 	 "customers",	   	    optional_argument,	 	BOTH,	  	"Set the average number of customers per client to ARG." },
    { 'g',       "share",		    optional_argument,		BOTH,		"Set the share for the first group on the processor to ARG." },
    { 'd',	 "delay-server",	    optional_argument,      	BOTH,	  	"Set the probability of a task being an infinite server to ARG." },
    { 'i',	 "infinite-server",	    optional_argument,		BOTH,	  	"Set the probability of a task being an infinite server to ARG." },
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
    { 0x100+'8', "beta",		    required_argument,		BOTH,		"Set the Beta parameter of the BETA distribution to ARG.  Alpha is set from the mean." },
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
    { 0X100+'#', "total-customers",	    required_argument,		BOTH,		"Set the total number of customers, regardless of clients, to ARG." },
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
    { 0x200+'M', "mva-steps",		    no_argument,		BOTH,  		"[Do not] observe the number of calls to step() for SPEX/LQX output." },
    { 0x200+'W', "mva-waits",		    no_argument,		BOTH,  		"[Do not] observe the number of calls to wait() for SPEX/LQX output." },
    { 0x200+'I', "iterations",		    no_argument,		BOTH,  		"[Do not] observe the number of solver iterations for SPEX/LQX output." },
    { 0x200+'E', "elapsed-time", 	    no_argument,		BOTH,  		"[Do not] observe the solver's ELAPSED time for SPEX/LQX output." },
    { 0x200+'U', "user-cpu-time",	    no_argument,		BOTH,  		"[Do not] observe the solver's USER CPU time for SPEX/LQX output." },
    { 0x200+'S', "system-cpu-time",	    no_argument,		BOTH,  		"[Do not] observe the solver's SYSTEM CPU time for SPEX/LQX output." },
    { 0x200+'q', "pragma",		    optional_argument,		BOTH,		"Set a pragma for the generated model to ARG." },
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
static void execute( std::ostream& output, const std::string& file_name );
static bool check_multiplicity( const RV::RandomVariable * );
static bool check_argument( const RV::RandomVariable * );

bool queueing_model = false;

bool Flags::verbose = false;
bool Flags::annotate_input = true;
LQIO::DOM::Document::input_format Flags::output_format = LQIO::DOM::Document::AUTOMATIC_INPUT;
bool Flags::spex_output = false;
bool Flags::lqx_output = false;
bool Flags::lqn2lqx = false;
bool Flags::reset_pragmas = false;
bool Flags::long_names = false;
std::vector<bool> Flags::observe(Flags::N_OBSERVATIONS,false);
std::vector<bool> Flags::convert(Flags::N_PARAMETERS,true);
std::vector<bool> Flags::override(Flags::N_PARAMETERS,true);
unsigned int Flags::number_of_runs = 1;
unsigned int Flags::number_of_models = 1;
double Flags::sensitivity = 0;

RV::RandomVariable * continuous_default;
RV::RandomVariable * discreet_default;
RV::RandomVariable * constant_default;
static RV::RandomVariable * number_of_layers;
static RV::RandomVariable * number_of_clients;
static RV::RandomVariable * number_of_processors;
static RV::RandomVariable * number_of_tasks;
static RV::RandomVariable * total_customers;

static bool some_randomness = false;
static const char * const output_suffix[] = { "xlqn", "xlqn", "lqnx", 0 };
static std::string output_file_name;


std::map<std::string,RV::RandomVariable *> distributions;

/*----------------------------------------------------------------------*/
/*			      Main line					*/
/*----------------------------------------------------------------------*/

int
main( int argc, char *argv[] )
{
    initialize();

    bool customers_set = false;
    
    LQIO::io_vars.init( VERSION, basename( argv[0] ), severity_action );

    /* Set flags used by lqngen */

    if ( LQIO::io_vars.lq_toolname == "lqn2lqx" ) {
	Flags::lqn2lqx = true;
	Flags::spex_output = true;
	Flags::annotate_input = false;
    }
    extern char *optarg;

    static std::string opts = "";
#if HAVE_GETOPT_H
    int optflag = 0;
    static std::vector<struct option> longopts;
    makeopts( opts, longopts, &optflag );
#if __cplusplus < 201103L
    LQIO::CommandLine command_line( &longopts.front() );
#else
    LQIO::CommandLine command_line( longopts.data() );
#endif
#else
    makeopts( opts );
    LQIO::CommandLine command_line();
#endif
    command_line = LQIO::io_vars.lq_toolname;
    
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

	    case 0x100+'#':
		if ( total_customers ) delete total_customers;		/* out with the old... */
		total_customers = get_RV( std::string(optarg), discreet_default, RV::RandomVariable::DISCREET );
		if ( !check_multiplicity( total_customers ) ) throw std::domain_error( "The mean number of clients must be greater than zero." );
		Flags::override[Flags::CUSTOMERS] = false;		/* Use DOM value as we set that */
		break;
		
	    case 0x100+'1':
		Generate::__convergence_value = strtod( optarg, &endptr );
		if ( Generate::__convergence_value <= 0 ) {
		    std::cerr << LQIO::io_vars.lq_toolname << "convergence=" << Generate::__convergence_value << " is invalid, choose non-negative real." << std::endl;
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
		    std::cerr << LQIO::io_vars.lq_toolname << "convergence=" <<  Generate::__convergence_value << " is invalid, choose non-negative real." << std::endl;
		    exit( 3 );
		}
		break;

	    case 0x100+'3':
		Generate::__iteration_limit = static_cast<unsigned int>(strtol( optarg, &endptr, 10 ));
		if ( Generate::__iteration_limit == 0 ) {
		    std::cerr << LQIO::io_vars.lq_toolname << "iteration-limit=" << Generate::__iteration_limit << " is invalid, choose non-negative integer." << std::endl;
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
		    const double root_arg = sqrt( arg );
		    Flags::annotate_input = false;
		    reset_RV( &number_of_tasks, constant_default, arg );
		    reset_RV( &number_of_layers, discreet_default, root_arg );
		    reset_RV( &number_of_clients, discreet_default, root_arg );
		    reset_RV( &number_of_processors, discreet_default, root_arg );
		    Generate::__task_layering =      Generate::RANDOM_LAYERING;
		    Generate::__processor_layering = Generate::RANDOM_LAYERING;
		    reset_RV( &Generate::__customers_per_client, discreet_default, root_arg );
		    reset_RV( &Generate::__task_multiplicity, discreet_default, 1.5 );
		    reset_RV( &Generate::__processor_multiplicity, discreet_default, 1.5 );
		    reset_RV( &Generate::__service_time, continuous_default, 1.0 );
		    reset_RV( &Generate::__rendezvous_rate, continuous_default, 1.0 );
		    reset_RV( &Generate::__number_of_entries, discreet_default, 1.2 );
		    reset_RV( &Generate::__think_time, continuous_default, 1.0 );
		    reset_RV( &Generate::__outgoing_requests, continuous_default, Generate::__number_of_entries->getMean() );
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
		    if ( Generate::__customers_per_client ) delete Generate::__customers_per_client;
		    Generate::__customers_per_client = get_RV( std::string(optarg), discreet_default, RV::RandomVariable::DISCREET );
		    if ( !check_multiplicity( Generate::__customers_per_client ) ) throw std::domain_error( "The mean number of clients must be greater than one." );
		    Flags::override[Flags::CUSTOMERS] = true;
		    customers_set = true;
		} else if ( Flags::override[Flags::CUSTOMERS] ) {
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
		if ( number_of_clients ) delete number_of_clients;
		number_of_clients = get_RV( std::string( optarg ), constant_default, RV::RandomVariable::DISCREET );
		if ( !check_multiplicity( number_of_clients ) ) throw std::domain_error( "The number of clients must be greater than zero." );
		break;

	    case 0x100+'C':		/* Constant */
		if ( continuous_default ) delete continuous_default;
		continuous_default = new RV::Constant( 1 );
		if ( discreet_default ) delete discreet_default;
		discreet_default = new RV::Constant( 1 );
		break;

	    case 'd':
		Generate::__probability_delay_server = strtod( optarg, &endptr );
		break;

	    case 0x100+'d': /* Assign processors deterministically by depth." */
		if ( Flags::lqn2lqx ) throw c;
		Generate::__processor_layering = Generate::DEPTH_FIRST_LAYERING;
		break;
		
	    case 'E':
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

	    case 'g':
		Generate::__group_share = strtod( optarg, &endptr );
		break;
		
	    case 'G':
		if ( Flags::lqn2lqx ) throw c;
		Generate::__probability_cfs_processor = strtod( optarg, &endptr );
		break;
		
	    case 0x100+'G':
		if ( continuous_default ) delete continuous_default;
		continuous_default = new RV::Gamma( 1, strtod( optarg, &endptr ) );
		some_randomness = true;
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
		if ( number_of_layers ) delete number_of_layers;
		number_of_clients = get_RV( std::string( optarg ), constant_default, RV::RandomVariable::DISCREET );
		if ( !check_multiplicity( number_of_layers ) ) throw std::domain_error( "The number of layers must be greater than zero." );
		break;

	    case 0x100+'L':
		if ( Flags::lqn2lqx ) throw c;
		Flags::long_names = true;
		break;
		
	    case 'M':
		if ( Flags::lqn2lqx ) throw c;
		Flags::number_of_models = strtol( optarg, &endptr, 10 );	/* Doesn't work with lqn2lqx	*/
		break;								/* nor does it make sense	*/

	    case 0x100+'M':
		man();
		exit( 0 );
	    
	    case 0x200+'M':
		Flags::observe[Flags::MVA_STEPS] = true;
		break;

	    case 0x400+'M':
		Flags::observe[Flags::MVA_STEPS] = false;
		break;

	    case 0x100+'n':
		std::transform( Flags::convert.begin(), Flags::convert.end(), Flags::convert.begin(), Flags::set_false );
		std::transform( Flags::override.begin(), Flags::override.end(), Flags::override.begin(), Flags::set_false );
		break;
		
	    case 0x100+'N':
		if ( continuous_default ) delete continuous_default;
		continuous_default = new RV::Normal( 1, strtod( optarg, &endptr ) );	/* Mean, stddev */
		some_randomness = true;
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
		static const char * const strings[] = { "lqn", "xml", "lqx", nullptr };
		int arg = getsubopt( &optarg, const_cast<char * const *>(strings), &endptr );
		switch ( arg ) {
		case 0:
		    Flags::output_format = LQIO::DOM::Document::LQN_INPUT;
		    break;
		case 1:
		    Flags::output_format = LQIO::DOM::Document::XML_INPUT;
		    break;
		case 2:
		    Flags::output_format = LQIO::DOM::Document::XML_INPUT;
		    Flags::spex_output = true;
		    Flags::lqx_output  = true;
		    break;
		default:
		    ::invalid_argument( c, old_optarg );
		    exit( 1 );
		} }
		break;

	    case 'p':			/* processor-multiplicity */
		if ( optarg ) {
		    if ( Generate::__processor_multiplicity ) delete Generate::__processor_multiplicity;
		    Generate::__processor_multiplicity = get_RV( std::string( optarg ), discreet_default, RV::RandomVariable::DISCREET );
		    if ( !check_multiplicity( Generate::__processor_multiplicity ) ) throw std::domain_error( "The mean processor multiplicity must be greater than one." );
		    Flags::override[Flags::PROCESSOR_MULTIPLICITY] = true;
		} else if ( Flags::override[Flags::PROCESSOR_MULTIPLICITY] ) {
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
		if ( number_of_processors ) delete number_of_processors;
		number_of_processors = get_RV( std::string( optarg ), constant_default, RV::RandomVariable::DISCREET );
		if ( !check_multiplicity( number_of_processors ) ) throw std::domain_error( "The number of processors must be greater than zero." );
		break;

	    case 0x100+'P':
		if ( discreet_default ) delete discreet_default;
		discreet_default = new RV::Poisson( 2, 1 );	/* Mean Offset */
		break;

	    case 'Q': 
		if ( Flags::lqn2lqx ) {
		    throw c;
		} else {
		    queueing_model = true;
		    some_randomness = true;

		    const double high = static_cast<double>(strtol( optarg, &endptr, 10 ));
		    if ( (endptr != 0 && *endptr != '\0') || trunc(high) != high || high < 2. ) {
			throw std::domain_error( "The total number of customers must be an integer greater than one." );
		    }
		    const double low = ceil( high / 10. );
		    const double mid = ceil( high / 5. );

		    if ( number_of_clients ) delete number_of_clients;
		    if ( number_of_layers ) delete number_of_layers;
		    if ( number_of_tasks ) delete number_of_tasks;
		    if ( total_customers ) delete total_customers;
		    if ( Generate::__outgoing_requests ) delete Generate::__outgoing_requests;
		    if ( Generate::__service_time ) delete Generate::__service_time;
		    if ( Generate::__think_time ) delete Generate::__think_time;
		    if ( Generate::__rendezvous_rate ) delete Generate::__rendezvous_rate;
		    number_of_clients = new RV::Uniform( 1.0, low + 1.0 );
		    number_of_layers = new RV::Constant( 1.0 );
		    number_of_tasks = new RV::Uniform( low, mid + 1.0 );
		    total_customers = new RV::Uniform( low, high + 1.0 );
		    Generate::__outgoing_requests = new RV::Uniform( 1.0, mid + 1.0 );
		    Generate::__probability_delay_server = 1.0;		/* Ignore all processors */
		    Generate::__probability_infinite_server = 0.05;
		    Generate::__service_time = new RV::Uniform( 0.0, 1.0 );
		    Generate::__rendezvous_rate = new RV::Uniform( 0.0, 1.0 );
		    Generate::__think_time = new RV::Constant( 0.0 );

		    Flags::annotate_input = false;
		    Flags::override[Flags::CUSTOMERS] = false;		/* Use DOM value as we set that */
		    Generate::__iteration_limit = 1;
		    Generate::__pragma["prune"] = "true";
		    Generate::__pragma["variance"] = "no-entry";
		    Generate::__pragma["severity-level"] = "run-time";	/* Suppress no client service time warnings. */
		}
		break;
		
	    case 0x200+'q':
		get_pragma( std::string( optarg ) );
		break;

	    case 0x400+'q':
		if ( Flags::lqn2lqx ) throw c;
		Flags::reset_pragmas = true;
		if ( optarg != NULL ) {
		    throw std::domain_error( "option does not take an argument" );
		}
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
		    if ( Generate::__service_time ) delete Generate::__service_time;
		    Generate::__service_time = get_RV( std::string( optarg ), continuous_default, RV::RandomVariable::CONTINUOUS );
		    if ( !check_argument( Generate::__service_time ) ) throw std::domain_error( "Service time must be greater than zero." );
		    Flags::override[Flags::SERVICE_TIME] = true;
		} else if ( Flags::override[Flags::SERVICE_TIME] ) {
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
		Flags::spex_output = true;
		Flags::lqx_output = false;
		break;

	    case 0x200+'S':
		Flags::observe[Flags::SYSTEM_TIME] = true;
		break;

	    case 0x400+'S':
		Flags::observe[Flags::SYSTEM_TIME] = false;
		break;

	    case 't':			/* task-multiplicity */
		if ( optarg ) {
		    if ( Generate::__task_multiplicity ) delete Generate::__task_multiplicity;
		    Generate::__task_multiplicity = get_RV( std::string( optarg ), discreet_default, RV::RandomVariable::DISCREET );
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
		if ( number_of_tasks ) delete number_of_tasks;
		number_of_tasks = get_RV( std::string( optarg ), constant_default, RV::RandomVariable::DISCREET );
		if ( !check_multiplicity( number_of_tasks ) ) throw std::domain_error( "The number of tasks must be greater than zero." );
		break;

	    case 0x100+'T':
		Flags::lqn2lqx = true;
		Flags::spex_output = true;
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
		some_randomness = true;
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
		
	    case 0x200+'W':
		Flags::observe[Flags::MVA_WAITS] = true;
		break;

	    case 0x400+'W':
		Flags::observe[Flags::MVA_WAITS] = false;
		break;

	    case 0x100+'x':
		Flags::output_format = LQIO::DOM::Document::XML_INPUT;
		break;

	    case 0x100+'X':
		Flags::output_format = LQIO::DOM::Document::XML_INPUT;
		Flags::spex_output = true;
		Flags::lqx_output  = true;
		break;

	    case 'v':
		Flags::verbose = true;
		break;

	    case 'V':
		std::cerr << "Layered Queueing Network Generator, Version " << VERSION << std::endl << std::endl;
		break;
		
	    case 0x100+'V':
		if ( Flags::lqn2lqx ) throw c;
		Generate::__task_layering = Generate::FUNNEL_LAYERING;
		break;

	    case 'y':			/* rendezvous-rate */
		if ( optarg ) {
		    if ( Generate::__rendezvous_rate ) delete Generate::__rendezvous_rate;
		    Generate::__rendezvous_rate = get_RV( std::string( optarg ), continuous_default, RV::RandomVariable::CONTINUOUS );
		    if ( !check_argument( Generate::__service_time ) ) throw std::domain_error( "Request rate must be greater than zero." );
		    Flags::override[Flags::REQUEST_RATE] = true;
		} else if ( Flags::override[Flags::REQUEST_RATE] ) {
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
		if ( Generate::__outgoing_requests ) delete Generate::__outgoing_requests;
		Generate::__outgoing_requests = get_RV( std::string( optarg ), continuous_default, RV::RandomVariable::CONTINUOUS );
		if ( !check_argument( Generate::__outgoing_requests ) ) throw std::domain_error( "Outgoing requests must be greater than zero." );
		break;
		
	    case 'z':
		if ( optarg ) {
		    if ( Generate::__think_time ) delete Generate::__think_time;
		    Generate::__think_time = get_RV( std::string( optarg ), continuous_default, RV::RandomVariable::CONTINUOUS );
		    if ( !check_argument( Generate::__service_time ) ) throw std::domain_error( "Think time must be greater than zero." );
		    Flags::override[Flags::THINK_TIME] = true;
		} else if ( Flags::override[Flags::THINK_TIME] ) {
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

	    if ( customers_set && (dynamic_cast<const RV::Constant *>(total_customers) == NULL || (*total_customers)() != 0) ) {
		std::cerr << LQIO::io_vars.lq_toolname << ": --customers and --total-customers are mutually exclusive." << std::endl;
		exit( 1 );
	    }
	}

	catch ( const std::domain_error& err ) {
	    if ( !optarg ) {
		std::cerr << LQIO::io_vars.lq_toolname << ": " << err.what() << " -- '"  << longopts.at(optind).name << "'." << std::endl;
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
    LQIO::io_vars.lq_command_line = command_line.c_str();
    
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
    if ( !some_randomness && Flags::number_of_runs > 1 ) {
	fprintf( stderr, "%s: Multiple experiment runs, but no randomness in model.  Use any or all of -s, -t, -p, -y to set randomness.\n", argv[0] );
	exit( 1 );
    }

    /* If multiple experiments, force spex/lqx code */
    if ( Flags::number_of_runs > 1 ) {
	Flags::spex_output = true;
    }

    /* If not spex output, turn off all observations */
    if ( !Flags::spex_output ) {
	std::transform( Flags::observe.begin(), Flags::observe.end(), Flags::observe.begin(), Flags::set_false );
    }
    
    if ( argc == optind ) {
	output_file_name = "";
	

	if ( Flags::number_of_models > 1 ) {
	    std::cerr << LQIO::io_vars.lq_toolname << ": a directory name is required as an argument for the option '--models=" 
		 << Flags::number_of_models << "." << std::endl;
	    exit ( 1 );
	}

	execute( std::cout, "" );

    } else if ( argc == optind + 1 ) {

	if ( Flags::number_of_models <= 1 ) {
	    std::ofstream output_file;
	    output_file_name = argv[optind];
	    LQIO::Filename::backup( output_file_name );
	    output_file.open( argv[optind], std::ios::out );
	    if ( !output_file ) {
		std::cerr << LQIO::io_vars.lq_toolname << ": Cannot open output file " << output_file_name << " - " << strerror( errno ) << std::endl;
		exit ( 1 );
	    }
	    if ( Flags::output_format == LQIO::DOM::Document::AUTOMATIC_INPUT ) {
		Flags::output_format = LQIO::DOM::Document::getInputFormatFromFilename( output_file_name, LQIO::DOM::Document::LQN_INPUT );
	    }

	    execute( output_file, output_file_name );

	    output_file.close();

	} else {
	    multi( argv[optind] );
	}

    } else {

	std::cerr << LQIO::io_vars.lq_toolname << ": arg count." << std::endl;

    }

    return 0;
}


int
lqn2lqx( int argc, char **argv )
{
    unsigned int errorCode;
    
    if ( Flags::number_of_runs > 1 && Flags::sensitivity > 0 ) {
	fprintf( stderr, "%s: --experiments=%d and --sensitivity=%g are mutually exclusive.\n", LQIO::io_vars.toolname(), Flags::number_of_runs, Flags::sensitivity );
	exit( 1 );
    }

    if ( optind == argc ) {
	LQIO::DOM::Document* document = LQIO::DOM::Document::load( "-", LQIO::DOM::Document::AUTOMATIC_INPUT, errorCode, false );
	if ( document ) {
	    Generate aModel( document, Flags::output_format, Flags::number_of_runs, (*total_customers)() );
	    aModel.groupize().reparameterize();
	    if ( output_file_name.size() == 0 || output_file_name == "-" ) {
		std::cout << aModel;
	    } else {
		LQIO::Filename::backup( output_file_name );
		
		std::ofstream output;
		output.open( output_file_name.c_str(), std::ios::out|std::ios::binary );
		if ( !output ) {
		    std::cerr << "Cannot open output file " << output_file_name << " - " << strerror( errno );
		} else {
		    output << aModel;
		    output.close();
		}
	    }
	}

    } else if ( output_file_name.size() > 0 && argc - optind != 1 ) {
	std::cerr << LQIO::io_vars.lq_toolname << ": Only one input file can be specified when using --output=filename." << std::endl;
	exit( 1 );
    } else {
	for ( ;optind < argc; ++optind ) {
	    LQIO::DOM::Document* document = LQIO::DOM::Document::load( argv[optind], LQIO::DOM::Document::AUTOMATIC_INPUT, errorCode, false );
	    if ( !document ) {
		continue;
	    }

	    Generate aModel( document, Flags::output_format, Flags::number_of_runs, (*total_customers)() );
	    aModel.groupize().reparameterize();
	    
	    if ( output_file_name == "-" ) {
		std::cout << aModel;
	    } else {
		LQIO::Filename filename;
		if ( output_file_name.size() ) {
		    filename = output_file_name;
		} else {
		    filename.generate( argv[optind], output_suffix[Flags::output_format] );
		}
		filename.backup();		/* Overwriting input file. -- back up */

		std::ofstream output;
		output.open( filename().c_str(), std::ios::out );
		if ( !output ) {
		    std::cerr << LQIO::io_vars.lq_toolname << ": Cannot open output file " << filename() << " - " << strerror( errno ) << std::endl;
		    exit ( 1 );
		}
		output << aModel;
		output.close();
	    }
	}
    }
    return 0;
}


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
	    std::cerr << LQIO::io_vars.lq_toolname << ": " << strerror( errno ) << std::endl;
	    exit ( 1 );
	} else if ( mkdir( dir.c_str()
#if !defined(__WINNT__) && !defined(MSDOS)
			   ,S_IRWXU|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH
#endif
			) < 0 ) {
	    std::cerr << LQIO::io_vars.lq_toolname << ": " << strerror( errno ) << std::endl;
	    exit( 1 );
	}
    } else if ( !S_ISDIR( sb.st_mode ) ) {
	std::cerr << LQIO::io_vars.lq_toolname << ": Cannot output multiple files to " << dir << std::endl;
	exit( 1 );
    }

    int w = static_cast<int>(log10( static_cast<double>(Flags::number_of_models) )) + 1;

    for ( unsigned i = 1; i <= Flags::number_of_models; ++i ) {
	std::ostringstream file_name;
	file_name << dir << "/case-" << std::setw( w ) << std::setfill( '0' ) << i << "." << output_suffix[Flags::output_format];
	std::ofstream output_file;
	output_file.open( file_name.str().c_str(), std::ios::out );

	if ( !output_file ) {
	    std::cerr << LQIO::io_vars.lq_toolname << ": Cannot open output file " << file_name.str() << " - " << strerror( errno ) << std::endl;
	    exit ( 1 );
	}

	execute( output_file, file_name.str() );
	output_file.close();
    }
}


static void
execute( std::ostream& output, const std::string& file_name )
{
    LQIO::DOM::Document::__input_file_name = file_name;

    const unsigned int n_tasks = (*number_of_tasks)();
    const unsigned int n_procs = queueing_model ? n_tasks : (*number_of_processors)();
    Generate model( Flags::output_format, Flags::number_of_runs, (*number_of_layers)(), (*total_customers)(),
		    n_procs, (*number_of_clients)(), n_tasks );
    output << model();
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
makeopts( std::string& opts, std::vector<struct option>& longopts, int * optflag ) 
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


static void
get_pragma( const std::string& arg )
{
    std::size_t pos = arg.find( "=" );
    if ( pos != 0 && pos != std::string::npos ) {
	Generate::__pragma[arg.substr(0,pos)] = arg.substr(pos+1);
    } else {
	Generate::__pragma[arg] = "";
    }
}


static RV::RandomVariable *
get_RV( const std::string& arg, RV::RandomVariable * new_dist, RV::RandomVariable::distribution_t type )
{
    const std::size_t sep1 = arg.find( ":" );
    if ( sep1 != std::string::npos ) {
	std::string arg1 = arg.substr(0,sep1);
	/* Check for distribution */
	std::map<std::string,RV::RandomVariable *>::const_iterator dist = distributions.find( arg1 );
	if ( dist != distributions.end() ) {
	    RV::RandomVariable::distribution_t new_type = dist->second->getType();
	    if ( new_type != RV::RandomVariable::CONSTANT && new_type != RV::RandomVariable::BOTH && new_type != type ) throw std::domain_error( "invalid distribution for random variable" );
	    std::string arg2 = arg.substr(sep1+1);
	    return get_RV_args( dist->second->clone(), arg2 );
	}
    }
    /* default case for one or two args */
    return get_RV_args( new_dist->clone(), arg );
}

static RV::RandomVariable *
get_RV_args( RV::RandomVariable * dist, const std::string& arg )
{
    if ( dist->getType() != RV::RandomVariable::CONSTANT ) some_randomness = true;
    const std::size_t sep1 = arg.find( ":" );
    if ( sep1 != std::string::npos ) {
	if ( dist->nArgs() != 2 ) throw std::domain_error( "number of arguments" );
	std::string arg1 = arg.substr(0,sep1);
	std::string arg2 = arg.substr(sep1+1);
	dist->setArg( 1, arg1 );
	dist->setArg( 2, arg2 );
    } else {
	dist->setMean( arg );
    }
    return dist;
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
	LQIO::io_vars.error_count += 1;
	if  ( LQIO::io_vars.error_count >= LQIO::io_vars.max_error ) {
	    throw std::runtime_error( "Too many errors" );
	}
	break;
    }
}



static void
initialize()
{
    srand48( 12345678L );					/* Init now, may be reset with --seed */
    continuous_default = new RV::Gamma( 0.5, 2.0 );		/* scale,shape. Mean is one E[x] = k x gamma */
//    discreet_default = new RV::Binomial( 1, 3 );		/* Mean is (arg2 - arg1) / 2 + arg1 */
    discreet_default = new RV::Poisson( 2, 1 );			/* Mean is (arg1 - arg2) / 2 + arg1 */
    constant_default = new RV::Constant( 1 );

    Generate::__service_time 		    = new RV::Constant( 1.0 );
    Generate::__think_time 		    = new RV::Constant( 1.0 );
    Generate::__forwarding_probability      = new RV::Constant( 0.0 );
    Generate::__rendezvous_rate 	    = new RV::Constant( 1.0 );
    Generate::__send_no_reply_rate 	    = new RV::Constant( 0.0 );
    Generate::__customers_per_client 	    = new RV::Constant( 1.0 );
    Generate::__task_multiplicity 	    = new RV::Constant( 1.0 );
    Generate::__processor_multiplicity      = new RV::Constant( 1.0 );
    Generate::__number_of_entries	    = new RV::Constant( 1.0 );
    Generate::__outgoing_requests	    = new RV::Constant( 1.0 );
    Generate::__probability_second_phase    = 0.0;
    Generate::__probability_infinite_server = 0.0;
    number_of_tasks      = new RV::Constant( 1.0 );
    number_of_clients    = new RV::Constant( 1.0 );
    number_of_processors = new RV::Constant( 1.0 );
    number_of_layers	 = new RV::Constant( 1.0 );
    total_customers	 = new RV::Constant( 0.0 );

    Flags::observe[Flags::PARAMETERS] = true;
    Flags::observe[Flags::UTILIZATION] = true;
    Flags::observe[Flags::THROUGHPUT] = true;
    Flags::observe[Flags::RESIDENCE_TIME] = true;
    Flags::observe[Flags::QUEUEING_TIME] = true;

    distributions[RV::Exponential::__name]  = new RV::Exponential(1.0);
    distributions[RV::Pareto::__name]       = new RV::Pareto(1.0);
    distributions[RV::Uniform::__name] 	    = new RV::Uniform(0.0,1.0);
    distributions[RV::LogUniform::__name]   = new RV::LogUniform(0.0,1.0);
    distributions[RV::Constant::__name]     = new RV::Constant(1.0);
    distributions[RV::Normal::__name]       = new RV::Normal(1.0,1.0);
    distributions[RV::Gamma::__name]        = new RV::Gamma(1.0,1.0);
    distributions[RV::Beta::__name]         = new RV::Beta(1.0,1.0);
    distributions[RV::Poisson::__name]      = new RV::Poisson(2.0,1.0);
    distributions[RV::Binomial::__name]     = new RV::Binomial(0.0,1.0);
    distributions[RV::Probability::__name]  = new RV::Probability(0.0);
}
