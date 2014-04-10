/* lqngen.cc	-- Greg Franks Thu Jul 29 2004
 * Model file generator.
 * This is actually part of lqn2ps, but if lqn2ps is invoked as lqngen, then this magically runs.
 *
 * $Id$
 */

#include "lqngen.h"
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <cmath>
#include <cstring>
#include <getopt.h>
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <cstdlib>
#if !HAVE_GETSUBOPT
#include <lqio/getsbopt.h>
#endif
#include <lqio/commandline.h>
#include <lqio/filename.h>
#include <lqio/glblerr.h>
#include "generate.h"
#if !HAVE_DRAND48
#include "randomvar.h"
#endif

#if (defined(linux) || defined(__linux__)) && !defined(__USE_XOPEN_EXTENDED)
extern "C" int getsubopt (char **, char * const *, char **);
#endif

/* Sort on field 5 */
option_type Generate::opt[] =
{
    { 'A',  	 "automatic",		    {(void *)Options::integer}, LQNGEN_ONLY,  	"Create a model with ARG layers, clients, processor and ARG**2 tasks." },
    { 'C', 	 "clients",         	    {(void *)Options::integer}, LQNGEN_ONLY,  	"Create exactly ARG client (Reference) tasks." },
    { 'L', 	 "layers",          	    {(void *)Options::integer}, LQNGEN_ONLY,  	"Create exactly ARG layers." },
    { 'M', 	 "models",	   	    {(void *)Options::integer}, LQNGEN_ONLY,  	"Create ARG different models." },
    { 'N', 	 "experiments",     	    {(void *)Options::integer}, BOTH, 		"Create ARG experiments." },
    { 'O', 	 "format",          	    {(void *)Options::io},      BOTH, 		"Set output format to ARG (lqn,xml)." },
    { 'P', 	 "processors",      	    {(void *)Options::integer}, LQNGEN_ONLY,  	"Create exactly ARG processors." },
    { 'S', 	 "seed",            	    {(void *)Options::integer}, BOTH, 		"Seed value for random number generator." },
    { 'T', 	 "tasks",           	    {(void *)Options::integer}, LQNGEN_ONLY,  	"Create exactly ARG tasks (excluding reference tasks)." },
    { 'c', 	 "customers",	   	    {(void *)Options::integer}, BOTH,	  	"Set the average number of customers per client to ARG." },
    { 'd', 	 "deterministic",	    {(void *)0},	        BOTH, 		"Use DETERMINISTIC values everywhere." },
    { 'e', 	 "entries",	   	    {(void *)Options::integer}, LQNGEN_ONLY,  	"Set the average number of entries per task to ARG." },
    { 'h', 	 "help",            	    {(void *)0},                BOTH, 		"help!" },
    { 'i',	 "infinite-server",	    {(void *)Options::real},	BOTH,	  	"Set the probability of a task being an infinite server to ARG." },
    { 'o', 	 "output",		    {(void *)Options::string},  LQN2LQX_ONLY,	"Set the output file name to ARG." },
    { 'p', 	 "processor-multiplicity",  {(void *)Options::integer}, BOTH,	  	"Set the average processor multiplicity to ARG." },
    { 's', 	 "service-time",	    {(void *)Options::integer}, BOTH,	  	"Set the average phase service time to ARG." },
    { 't', 	 "task-multiplicity",	    {(void *)Options::integer}, BOTH,	  	"Set the average task multiplicity to ARG." },
    { 'v', 	 "verbose",         	    {(void *)0},                BOTH, 		"Verbose." },
    { 'y', 	 "rendezvous-rate", 	    {(void *)Options::integer}, BOTH,	  	"Set the average number of synchronous calls from a phase to ARG." },
    { 'z', 	 "think-time",	   	    {(void *)Options::integer}, BOTH,	  	"Set the average client think time to ARG." },
    { '2', 	 "second-phase",	    {(void *)Options::real},	LQNGEN_ONLY,  	"Set the probability of a second phase for an entry to ARG." },
    { 0x200+'a', "annotate",     	    {(void *)0},                BOTH, 		"[Do not] annotate the resulting model file." },
    { 0x100+'C', "comment",		    {(void *)Options::string},	BOTH, 		"Set the model comment to ARG." },
    { 0x100+'u', "under-relaxation",	    {(void *)Options::string},	BOTH, 		"Set the model under-relaxation to ARG." },
    { 0x100+'i', "iteration-limit",	    {(void *)Options::string},	BOTH,		"Set the model iteration limit to ARG." },
    { 0x100+'c', "convergence-value",	    {(void *)Options::string},	BOTH,		"Set the model convergence value to ARG." },
    { 0x100+'N', "normal",		    {(void *)Options::real},	BOTH, 		"Use a NORMAL distribution with a standard deviation of ARG." },
    { 0x100+'G', "gamma",		    {(void *)Options::real},	BOTH, 		"Use a GAMMA distribution with a shape of ARG." },
    { 0x100+'P', "poisson",		    {(void *)0},		BOTH, 		"Use a POISSON distribution." },
    { 0x100+'U', "uniform",		    {(void *)Options::real},	BOTH, 		"Use a UNIFORM distribution with a spread of ARG." },
    { 0x100+'v', "funnel",          	    {(void *)0},                LQNGEN_ONLY,  	"Generate a model with more tasks at the bottom than at the top." },
    { 0x100+'p', "pyramid",         	    {(void *)0},                LQNGEN_ONLY,  	"Generate a model with more tasks at the top than at the bottom." },
    { 0x100+'f', "fat",			    {(void *)0},                LQNGEN_ONLY,  	"Generate a model with more tasks in the middle." },
    { 0x100+'r', "random",		    {(void *)0},		LQNGEN_ONLY,	"Randomly choose between fat, funnel, pyramid and uniform layering." },
    { 0x100+'S', "sensitivity",		    {(void *)Options::real},    LQN2LQX_ONLY,   "Create a factorial experiment with services times of increased/decreased by multiplying by ARG ." },
    { 0x100+'j', "json-output",     	    {(void *)0},                BOTH, 		"Output JSON." },
    { 0x100+'x', "xml-output",      	    {(void *)0},                BOTH, 		"Output XML." },
    { 0x100+'l', "lqx-output",      	    {(void *)0},                BOTH, 		"Output XML model with LQX." },
    { 0x100+'s', "spex-output",     	    {(void *)0},                BOTH, 		"Output LQN model with SPEX." },
    { 0x200+'u', "utilization",	   	    {(void *)0},		BOTH,  		"[Do not] observe processor utilization for SPEX/LQX output." },
    { 0x200+'f', "throughput",	     	    {(void *)0},		BOTH,  		"[Do not] observe task throughput for SPEX/LQX output." },
    { 0x200+'s', "residence-time",	    {(void *)0},		BOTH,  		"[Do not] observe entry service time for SPEX/LQX output." },
    { 0x200+'m', "mva-waits",		    {(void *)0},		BOTH,  		"[Do not] observe the number of calls to wait() for SPEX/LQX output." },
    { 0x200+'i', "iterations",		    {(void *)0},		BOTH,  		"[Do not] observe the number of solver iterations for SPEX/LQX output." },
    { 0x200+'U', "user-cpu-time",	    {(void *)0},		BOTH,  		"[Do not] observe the solver's USER CPU time for SPEX/LQX output." },
    { 0x200+'S', "system-cpu-time",	    {(void *)0},		BOTH,  		"[Do not] observe the solver's SYSTEM CPU time for SPEX/LQX output." },
    { 0,   	 0,                 	    {(void *)0},                BOTH, 		0 }
};

/* Initialize FLOATS at run time.  Value is interpreted as an INTEGER */

static int lqngen( int argc, char *argv[0] );
int lqn2lqx( int argc, char *argv[0] );
static void reset_RV( RV::RandomVariable ** rv, const RV::RandomVariable *, double mean );
static void severity_action(unsigned severity);
static void multi( const char *, const unsigned );

long seed = 12345678;
bool fixedModel = true;

bool Flags::verbose = false;
bool Flags::annotate_input = true;
bool Flags::lqx_output  = false;
bool Flags::xml_output  = false;
bool Flags::spex_output = false;
bool Flags::deterministic_only = false;
bool Flags::lqn2lqx = false;
Flags::observations_t Flags::observe = { true, true, true, false, false, false, false };
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

const char * Options::integer [] = 
{
    "int",
    0
};

/*
 * Input output format options
 */

const char * Options::io[] = 
{
    "lqn",
#if defined(XML_OUTPUT)
    "lqx",
    "xml",
#endif
    0
};

const char * Options::real [] = {
    "float",
    0
};


const char * Options::string [] = {
    "string",
    0
};

const char * output_file_name	= 0;
static RV::RandomVariable * continuous_default;
static RV::RandomVariable * discreet_default;
static bool some_randomness = false;


/*----------------------------------------------------------------------*/
/*			      Main line					*/
/*----------------------------------------------------------------------*/

int
main( int argc, char *argv[] )
{
    continuous_default = new RV::Gamma( 2, 2 );
    discreet_default = new RV::Poisson( 1 );

    /* Set flags used by lqngen */

    io_vars.lq_toolname = strrchr( argv[0], '/' );
    if ( io_vars.lq_toolname ) {
        io_vars.lq_toolname += 1;
    } else {
        io_vars.lq_toolname = argv[0];
    }

    int rc = 0;
    if ( strcmp( "lqn2lqx", io_vars.lq_toolname ) == 0 ) {
	rc = lqn2lqx( argc, argv );		/* */
    } else {
	rc = lqngen( argc, argv );
    }
    exit( rc );
}


/*
 * Generate a model
 */

static int
lqngen( int argc, char *argv[0] )
{
    extern char *optarg;

    Generate::__service_time 		    = new RV::Deterministic( 1.0 );
    Generate::__think_time 		    = new RV::Deterministic( 0.0 );
    Generate::__forwarding_probability      = new RV::Deterministic( 0.0 );
    Generate::__rendezvous_rate 	    = new RV::Deterministic( 1.0 );
    Generate::__send_no_reply_rate 	    = new RV::Deterministic( 0.0 );
    Generate::__customers_per_client 	    = new RV::Deterministic( 1.0 );
    Generate::__task_multiplicity 	    = new RV::Deterministic( 1.0 );
    Generate::__processor_multiplicity      = new RV::Deterministic( 1.0 );
    Generate::__probability_second_phase    = new RV::Deterministic( 0.0 );
    Generate::__probability_infinite_server = new RV::Deterministic( 0.0 );
    Generate::__number_of_entries	    = new RV::Deterministic( 1.0 );

    static string opts = "";
#if HAVE_GETOPT_H
    static std::vector<struct option> longopts;
    makeopts( opts, longopts );
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
#if HAVE_GETOPT_LONG
#if __cplusplus < 201103L
	const int c = getopt_long( argc, argv, opts.c_str(), &longopts.front(), NULL );
#else
	const int c = getopt_long( argc, argv, opts.c_str(), longopts.data(), NULL );
#endif
#else	
	const int c = getopt( argc, argv, opts.c_str() );
#endif
	if ( c == EOF) break;

        command_line.append( c, optarg );

	try {
	    switch( c ) {
	    case '2':
		if ( Generate::__probability_second_phase ) delete Generate::__probability_second_phase;
		Generate::__probability_second_phase = new RV::Probability( strtod( optarg, &endptr ) );
		break;

	    case 'A': {
		const unsigned int arg = strtod( optarg, &endptr );
		Generate::__number_of_clients = arg;
		Generate::__number_of_layers = arg;
		Generate::__number_of_processors = arg;
		Generate::__number_of_tasks = arg * arg;
		Generate::__layering_type = Generate::RANDOM_LAYERING;
		reset_RV( &Generate::__customers_per_client, discreet_default, arg );
		reset_RV( &Generate::__task_multiplicity, discreet_default, 1.5 );
		reset_RV( &Generate::__processor_multiplicity, discreet_default, 1.5 );
		reset_RV( &Generate::__service_time, continuous_default, 1.0 );
		reset_RV( &Generate::__rendezvous_rate, continuous_default, 1.0 );
		reset_RV( &Generate::__think_time, continuous_default, arg );
		reset_RV( &Generate::__number_of_entries, discreet_default, 1.2 );
		some_randomness = true;
	    } break;
		
	    case 'C':
		Generate::__number_of_clients = strtod( optarg, &endptr );
		break;

	    case 'e':
		reset_RV( &Generate::__number_of_entries, discreet_default, strtod( optarg, &endptr ) );
		break;

	    case 0x100+'f':
		Generate::__layering_type = Generate::FAT_LAYERING;
		break;

	    case 'L':
		Generate::__number_of_layers = strtod( optarg, &endptr );
		break;

	    case 'M':
		Flags::number_of_models = strtol( optarg, &endptr, 10 );
		break;

	    case 'P':
		Generate::__number_of_processors = strtod( optarg, &endptr );
		break;

	    case 0x100+'p':
		Generate::__layering_type = Generate::PYRAMID_LAYERING;
		break;

	    case 0x100+'r':
		Generate::__layering_type = Generate::RANDOM_LAYERING;
		break;

	    case 'T':
		Generate::__number_of_tasks = strtod( optarg, &endptr );
		break;

	    case 0x100+'v':
		Generate::__layering_type = Generate::FUNNEL_LAYERING;
		break;

	    default:
		if ( !common_arg( c, optarg, &endptr ) ) {
		    usage();
		    exit( 1 );
		}
		break;
	    }

	    if ( endptr != 0 && *endptr != '\0' ) {
		fprintf( stderr, "%s: Invalid argumement to -%c: %s\n", argv[0], c, optarg );
		exit( 1 );
	    }
	}
	catch ( std::domain_error ) {
	    fprintf( stderr, "%s: Invalid argumement to -%c: %s\n", argv[0], c, optarg );
	    exit( 1 );
	}
    }

    if ( Generate::__number_of_layers > Generate::__number_of_tasks ) {
	if ( Generate::__number_of_tasks == 1 ) {
	    Generate::__number_of_tasks = Generate::__number_of_layers;
	} else {
	    fprintf( stderr, "%s: Too many layers for %d tasks\n", argv[0], Generate::__number_of_tasks );
	    exit( 1 );
	}
    }

    if ( !some_randomness && Flags::number_of_runs > 1 ) {
	fprintf( stderr, "%s: Multiple experiment runs, but no randomness in model.  Use any or all of -s, -t, -p, -y to set randomness.\n", argv[0] );
	exit( 1 );
    }

    /* If multiple experiments, force spex/lqx code */
    if ( Flags::number_of_runs > 1 && !(Flags::lqx_output || Flags::spex_output) ) {
	if ( Flags::xml_output ) {
	    Flags::lqx_output = true;
	} else {
	    Flags::spex_output = true;
	}
    }

    io_vars.lq_command_line = command_line.c_str();

    if ( !Generate::__comment || strlen(Generate::__comment) == 0 ) {
	Generate::__comment = command_line.c_str();
    }

    srand48( seed );

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
	if ( Flags::verbose ) {
	    model.verbose( cerr );
	}

    } else if ( argc == optind + 1 ) {

	if ( Flags::number_of_models <= 1 ) {
	    ofstream output_file;
	    output_file_name = argv[optind];
	    LQIO::Filename::backup( output_file_name );
	    output_file.open( argv[optind], ios::out );
	    if ( !output_file ) {
		cerr << io_vars.lq_toolname << ": Cannot open output file " << output_file_name << " - "
		     << strerror( errno ) << endl;
		exit ( 1 );
	    }
	    
	    LQIO::input_file_name = strdup( output_file_name );
	    Generate model( Generate::__number_of_layers, Flags::number_of_runs );
	    output_file << model();
	    if ( Flags::verbose ) {
		model.verbose( cout );
	    }
	    output_file.close();

	} else {
	    struct stat sb;
	    if ( stat ( argv[optind], &sb ) < 0 ) {
		if ( errno != ENOENT ) {
		    cerr << io_vars.lq_toolname << ": " << strerror( errno ) << endl;
		    exit ( 1 );
		} else if ( mkdir( argv[optind]
#if !defined(WINNT) && !defined(MSDOS)
				   ,S_IRWXU|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH
#endif
				) < 0 ) {
		    cerr << io_vars.lq_toolname << ": " << strerror( errno ) << endl;
		    exit( 1 );
		}
	    } else if ( !S_ISDIR( sb.st_mode ) ) {
		cerr << io_vars.lq_toolname << ": Cannot output multiple files to " << output_file_name << endl;
		exit( 1 );
	    }
	    multi( argv[optind], Flags::number_of_models );
	}

    } else {

	cerr << io_vars.lq_toolname << ": arg count." << endl;
	usage();
    }

    return 0;
}



bool
common_arg( const int c, char * optarg, char ** endptr )
{
    switch ( c ) {
    case 0x200+'a':
	Flags::annotate_input = true;
	break;

    case 0x400+'a':
	Flags::annotate_input = false;
	break;

    case 0x100+'c':
	Generate::__convergence_value = strtod( optarg, endptr );
	if ( Generate::__convergence_value <= 0 ) {
	    cerr << io_vars.lq_toolname << "convergence=" << Generate::__convergence_value << " is invalid, choose non-negative real." << endl;
	    (void) exit( 3 );
	}
	break;

    case 0x100+'C':
	Generate::__comment = optarg;
	break;

    case 'd':		/* Deterministic */
	if ( continuous_default ) delete continuous_default;
	continuous_default = new RV::Deterministic( 0 );
	if ( discreet_default ) delete discreet_default;
	discreet_default = new RV::Deterministic( 0 );
	Generate::__layering_type = Generate::DETERMINISTIC_LAYERING;
	break;

    case 0x200+'f':
	Flags::observe.throughput = true;
	break;

    case 0x400+'f':
	Flags::observe.throughput = false;
	break;

    case 0x100+'G':
	if ( continuous_default ) delete continuous_default;
	continuous_default = new RV::Gamma( 1, strtod( optarg, endptr ) );
	break;

    case 'h':
	usage();
	exit( 1 );

    case 'i':
	if ( Generate::__probability_infinite_server ) delete Generate::__probability_infinite_server;
	Generate::__probability_infinite_server = new RV::Probability( strtod( optarg, endptr ) );
	break;

    case 0x100+'i':
	Generate::__iteration_limit = (unsigned)strtol( optarg, endptr, 10 );
	if ( Generate::__iteration_limit == 0 ) {
	    cerr << io_vars.lq_toolname << "iteration-limit=" << Generate::__iteration_limit << " is invalid, choose non-negative integer." << endl;
	}
	break;

    case 0x200+'i':
	Flags::observe.iterations = true;
	break;

    case 0x400+'i':
	Flags::observe.iterations = false;
	break;

    case 0x100+'j':
	Flags::xml_output  = false;
	Flags::spex_output = false;
	break;

    case 0x100+'l':
	Flags::lqx_output  = true;
	Flags::xml_output  = true;
	Flags::spex_output = false;
	break;

    case 0x200+'m':
	Flags::observe.mva_waits = true;
	break;

    case 0x400+'m':
	Flags::observe.mva_waits = false;
	break;

    case 'c':
	reset_RV( &Generate::__customers_per_client, discreet_default, strtod( optarg, endptr ) );
	some_randomness = true;
	break;

    case 0x100+'N':
	if ( continuous_default ) delete continuous_default;
	continuous_default = new RV::Normal( 1, strtod( optarg, endptr ) );
	break;

    case 'N':
	Flags::number_of_runs = strtol( optarg, endptr, 10 );
	break;

    case 'O': 
	switch ( getsubopt( &optarg, const_cast<char * const *>(Options::io), endptr ) ) {
	case FORMAT_SRVN:	Flags::xml_output = false;	break;
#if defined(XML_OUTPUT)
	case FORMAT_LQX:	Flags::lqx_output = true;	/* fall through */
	case FORMAT_XML:	Flags::xml_output = true;	break;
#endif
	default:
	    usage();
	}
	break;

    case 'p':
	reset_RV( &Generate::__processor_multiplicity, discreet_default, strtod( optarg, endptr ) );
	some_randomness = true;
	break;

    case 0x100+'P':
	if ( discreet_default ) delete discreet_default;
	discreet_default = new RV::Normal( 1, strtod( optarg, endptr ) );
	break;

    case 's':
	reset_RV( &Generate::__service_time, continuous_default, strtod( optarg, endptr ) );
	some_randomness = true;
	break;

    case 'S':
	seed = strtol( optarg, endptr, 10 );
	break;

    case 0x100+'s':
	Flags::spex_output = true;
	Flags::xml_output  = false;
	Flags::lqx_output  = false;
	break;

    case 0x200+'s':
	Flags::observe.residence_time = true;
	break;

    case 0x400+'s':
	Flags::observe.residence_time = false;
	break;

    case 0x100+'S':
	Flags::sensitivity = strtod( optarg, endptr );
	break;

    case 0x200+'S':
	Flags::observe.system_time = true;
	break;

    case 0x400+'S':
	Flags::observe.system_time = false;
	break;

    case 't':
	reset_RV( &Generate::__task_multiplicity, discreet_default, strtod( optarg, endptr ) );
	some_randomness = true;
	break;

    case 0x100+'u':
	Generate::__convergence_value = strtod( optarg, endptr );
	if ( Generate::__convergence_value <= 0.0 ) {
	    cerr << io_vars.lq_toolname << "convergence=" <<  Generate::__convergence_value << " is invalid, choose non-negative real." << endl;
	    exit( 3 );
	}
	break;

    case 0x200+'u':
	Flags::observe.utilization = true;
	break;

    case 0x400+'u':
	Flags::observe.utilization = false;
	break;

    case 0x100+'U':
	if ( continuous_default ) delete continuous_default;
	continuous_default = new RV::Uniform( 1, strtod( optarg, endptr ) );
	break;

    case 0x200+'U':
	Flags::observe.user_time = true;
	break;

    case 0x400+'U':
	Flags::observe.user_time = false;
	break;

    case 'v':
	Flags::verbose = true;
	break;

    case 0x100+'x':
	Flags::xml_output  = true;
	Flags::spex_output = false;
	break;

    case 'y':
	reset_RV( &Generate::__rendezvous_rate, continuous_default, strtod( optarg, endptr ) );
	some_randomness = true;
	break;

    case 'z':
	reset_RV( &Generate::__think_time, continuous_default, strtod( optarg, endptr ) );
	some_randomness = true;
	break;

    default:
	return false;
    }

    return true;
}


static void
reset_RV( RV::RandomVariable ** rv, const RV::RandomVariable * default_rv, double mean )
{
    if ( (*rv) ) delete (*rv);
    (*rv) = default_rv->clone();
    (*rv)->setMean( mean );
}


#if HAVE_GETOPT_H
void
makeopts( string& opts, std::vector<struct option>& longopts ) 
{
    struct option opt;
    opt.flag = 0;
    for ( unsigned int i = 0; Generate::opt[i].name || Generate::opt[i].c ; ++i ) {
	opt.has_arg = (Generate::opt[i].opts.null != 0 ? required_argument : no_argument);
	opt.val = Generate::opt[i].c;
	opt.name = Generate::opt[i].name;
	longopts.push_back( opt );
	if ( (Generate::opt[i].c & 0xff00) == 0 ) {
	    opts +=  static_cast<char>(Generate::opt[i].c);
	    if ( Generate::opt[i].opts.null != 0 ) {
		opts += ':';
	    }
	}
	if ( (Generate::opt[i].c & 0x0200) != 0 ) {
	    /* Make the "no-" version */
	    opt.val = (Generate::opt[i].c & 0x00ff) | 0x0400;
	    std::string name = "no-";
	    name += Generate::opt[i].name;
	    opt.name = strdup( name.c_str() );
	    longopts.push_back( opt );
	}
    }
    opt.name = 0;
    opt.val  = 0;
    opt.has_arg = 0;
    longopts.push_back( opt );
}
#else
static void
makeopts( string& opts ) 
{
    for ( unsigned int i = 0; Generate::opt[i].name || Generate::opt[i].c ; ++i ) {
	if ( (Generate::opt[i].c & 0xff00) == 0 ) {
	    opts +=  static_cast<char>(Generate::opt[i].c);
	    if ( Geneate::opt[i].arg ) {
		opts += ':';
	    }
	}
    }
}
#endif

/*
 * Generate multiple experiments.
 */

static void
multi( const char * dir, const unsigned n )
{
    char file_name[132];
    int w = static_cast<int>(log10( static_cast<double>(n) )) + 1;

    for ( unsigned i = 1; i <= n; ++i ) {
	sprintf( file_name, "%s/case-%.*d.lqn", dir, w, i );
	if ( Flags::verbose ) {
	    cerr << file_name << ": ";
	}
	ofstream output_file;
	output_file.open( file_name, ios::out );

	if ( !output_file ) {
	    cerr << io_vars.lq_toolname << ": Cannot open output file " << file_name << " - "
		 << strerror( errno ) << endl;
	    exit ( 1 );
	}

        Generate model( Generate::__number_of_layers, Flags::number_of_runs );
	LQIO::input_file_name = strdup( file_name );
	output_file_name = file_name;
	output_file << model();
	output_file.close();
    }
    if ( Flags::verbose ) {
	cerr << endl;
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


/*
 * Print out usage string.
 */

void
usage()
{
    cerr << "Usage: " << io_vars.lq_toolname << " [OPTION]... [FILE|DIRECTORY]" << endl;
    cerr << "Generate LQN model files." << endl;
    cerr << "Options:" << endl;

    for ( unsigned int i = 0; Generate::opt[i].name != 0; ++i ) {
	if ( (Generate::opt[i].program == LQNGEN_ONLY && Flags::lqn2lqx) || (Generate::opt[i].program == LQN2LQX_ONLY && !Flags::lqn2lqx) ) continue;
	string s;
	if ( (Generate::opt[i].c & 0xff00) != 0 ) {
	    s = "      --";
	} else {
	    s = " -";
	    s += Generate::opt[i].c;
	    s += ",  --";
	}
	if ( (Generate::opt[i].c & 0x0200) != 0 ) {
	    s += "[no-]";
	}
	s += Generate::opt[i].name;

	if ( Generate::opt[i].opts.null != 0 ) {
	    s += "=ARG";
	}
	cerr.setf( ios::left, ios::adjustfield );
	cerr << setw(40) << s << Generate::opt[i].msg << endl;
    }
    exit( 1 );
}
