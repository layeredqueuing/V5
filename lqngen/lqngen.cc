/* lqngen.cc	-- Greg Franks Thu Jul 29 2004
 * Model file generator.
 * This is actually part of lqn2ps, but if lqn2ps is invoked as lqngen, then this magically runs.
 *
 * $Id$
 */

#include "lqngen.h"
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
#include <lqio/filename.h>
#include <lqio/glblerr.h>
#include "generate.h"

#if (defined(linux) || defined(__linux__)) && !defined(__USE_XOPEN_EXTENDED)
extern "C" int getsubopt (char **, char * const *, char **);
#endif

/* Sort on field 5 */
option_type Generate::opt[] =
{
    /* ARRIVALS_PROB          */ { 'A', "open-arrivals",   {(void *)Generate::probability},   0.0,  0.0,  true,  "Probability entry has open arrival." },
    /* NUMBER_OF_CUSTOMERS    */ { 'C', "customers",       {(void *)Generate::uniform},       1.0,  8.0,  true,  "Number of Reference Tasks." },
    /* THINK_TIME	      */ { 'D', "think-time",	   {(void *)Generate::exponential},   0.0,  10.0, false, "Think time at Reference Tasks." },
    /* NUMBER_OF_ENTRIES      */ { 'E', "entries",         {(void *)Generate::exponential},   1.0,  1.5,  true,  "Number of Entries per task." },
    /* FORWARDING_REQUESTS    */ { 'F', "forwarding",      {(void *)Generate::probability},   0.0,  0.0,  true,  "Probability entry forwards RNV." },
    /* GENERAL_PARAMETERS     */ { 'G', "options",         {(void *)Generate::model_opts},    0.0,  0.0,  false, "Model parameters." },
    /* NUMBER_OF_LAYERS       */ { 'L', "layers",          {(void *)Generate::uniform},       3.0,  8.0,  true,  "Number of layers." },
    /* OUTPUT_FORMAT          */ { 'O', "format",          {(void *)Options::io},             0.0,  0.0,  false, "Output format." },
    /* NUMBER_OF_PROCESSORS   */ { 'P', "processors",      {(void *)Generate::exponential},   1.0,  3.0,  true,  "Number of processors." },
    /* SEED                   */ { 'S', "seed",            {(void *)Options::integer},        0.0,  0.0,  false, "Seed value for random number generator." },
    /* NUMBER_OF_TASKS        */ { 'T', "tasks",           {(void *)Generate::exponential},   2.0,  5.0,  true,  "Total number of tasks (excluding reference tasks)." },
    /* ARRIVAL_RATE           */ { 'a', "arrival-rate",    {(void *)Generate::exponential},   0.0,  0.0,  false, "Arrival rate at entry with open arrivals" },
    /* CUSTOMERS              */ { 'c', "customers",       {(void *)Generate::exponential},   1.0,  4.0,  false, "Number of customers in each Reference Task." },
    /* DETERMINISTIC          */ { 'd', "deterministic",   {(void *)0},                       0,    0,    false, "Deterministic only." },
    /* FORWARDING_PROB        */ { 'f', "forwarding",      {(void *)Generate::probability},   0.0,  0.0,  false, "Forwarding probability at entry with forwarding." },
    /* HELP                   */ { 'h', "help",            {(void *)0},                       0.0,  0.0,  false, "help!" },
    /* INFINITE_SERVER        */ { 'i', "infinite-server", {(void *)Generate::probability},   0.0,  0.15, false, "Probability object is an infinite server." },
    /* NUMBER		      */ { 'n', "number",          {(void *)Options::integer},        0.0,  0.0,  false, "Generate ARG experiments." },
    /* MULTI_SERVER           */ { 'm', "multi-server",    {(void *)Generate::probability},   0.0,  0.15, false, "Probability object is a multi-server." },
    /* PROCESSOR_MULTIPLICITY */ { 'p', "proc-mult",       {(void *)Generate::uniform},       2.0,  10.0, false, "Processor multiplicty." },
    /* PHASE2_PROBABILITY     */ { 'q', "second-phase",	   {(void *)Generate::probability},   0.0,  0.0,  true,  "Probability of sending a message from phase 2." },
    /* SERVICE_TIME           */ { 's', "service-time",    {(void *)Generate::loguniform},    0.1,  10.0, false, "Phase service time." },
    /* TASK_MULTIPLICITY      */ { 't', "task-mult",       {(void *)Generate::uniform},       2.0,  10.0, false, "Task multiplicity." },
    /* VERBOSE		      */ { 'v', "verbose",         {(void *)0},                       0,    0,    false, "Verbose." },
    /* RNV_PROB		      */ { 'x', "rendezvous",	   {(void *)Generate::probability},   0.0,  1.0,  true,  "Probability of sending a message is a rendezvous." },
    /* RNV_REQUESTS           */ { 'y', "rnv-rate",        {(void *)Generate::loguniform},    0.1,  10.0, false, "RNV rate at entries with RNVs." },
    /* SNR_REQUESTS           */ { 'z', "snr-rate",        {(void *)Generate::loguniform},    0.1,  10.0, false, "SNR rate at entries with SNRs." },
    /* INDIRECT		      */ { '@', "filename",	   {(void *)Options::string},	      0,    0,    false, "Read options from file ARG." },
    /* 			*/ { 0x100+'a', "no-annotate",     {(void *)0},			      0,    0,    false, "Suppress annotation of model file." },
    /*                  */ { 0x100+'f', "funnel",          {(void *)0},                       0,    0,    true,  "Generate a model with more tasks at the bottom than at the top." },
    /*                  */ { 0x100+'p', "pyramid",         {(void *)0},                       0,    0,    true,  "Generate a model with more tasks at the top than at the bottom." },
    /* 			*/ { 0x100+'x', "xml-output",	   {(void *)0},			      0,    0,    false, "Output XML." },
    /* 			*/ { 0x100+'l', "lqx-output",	   {(void *)0},			      0,    0,    false, "Output XML model with LQX." },
    /* 			*/ { 0x100+'s', "spex-output",	   {(void *)0},			      0,    0,    false, "Output LQN model with SPEX." },
    /*		        */ { 0x100+'P', "product-form",    {(void *)0},			      0,    0,    true,  "Generate models with product-form solutions." },
    /* End of list            */ { 0,   0,                 {(void *)0},                       0,    0,    false, 0 }
};

/* Initialize FLOATS at run time.  Value is interpreted as an INTEGER */

static int lqngen( int argc, char *argv[0] );
static int lqn2lqx( int argc, char *argv[0] );
static void severity_action(unsigned severity);

#if HAVE_GETOPT_H
static void makeopts( string& opts, struct option longopts[] );
#define N_LONG_OPTS 90			/* Have to guess on this one as -[no]- is generated automatically */
#else
static void makeopts( string& opts );
#endif

static void multi( const char *, const unsigned );
static void usage();
static void getargs( int argc, char **argv, bool indirect );
static bool getarg( const char c, const char * arg );
static void getargs_from_file( char * filename );
static bool is_random_variable( unsigned int i );


long seed = 12345678;
bool fixedModel = true;

bool Flags::verbose = false;
bool Flags::annotate_input = true;
bool Flags::lqx_output  = false;
bool Flags::xml_output  = false;
bool Flags::spex_output = false;
bool Flags::deterministic_only = false;
bool Flags::lqn2lqx = false;
model_structure Flags::structure = DEFAULT_STRUCTURE;
unsigned long Flags::number_of_runs = 0;

std::string command_line;

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

/*----------------------------------------------------------------------*/
/*			      Main line					*/
/*----------------------------------------------------------------------*/

int
main( int argc, char *argv[] )
{
    /* Set flags used by srvn2eepic */

    io_vars.lq_toolname = strrchr( argv[0], '/' );
    if ( io_vars.lq_toolname ) {
	io_vars.lq_toolname += 1;
    } else {
	io_vars.lq_toolname = argv[0];
    }
    command_line = io_vars.lq_toolname;

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
    getargs( argc, argv, false );
    io_vars.lq_command_line = command_line.c_str();

    if ( Generate::opt[INFINITE_SERVER].mean + Generate::opt[MULTI_SERVER].mean > 1.0 ) {
	cerr << io_vars.lq_toolname << ": Sum of Infinte and Mulitserver probabilities (" << Generate::opt[INFINITE_SERVER].mean + Generate::opt[MULTI_SERVER].mean << ") exceeds 1.0." << endl;
    }

#if defined(WINNT) || defined(MSDOS)
    srand( (int)seed );
#else
    srand48( seed );
#endif

    switch ( Flags::structure ) {
    case PRODUCT_FORM:
	fixedModel = false;
	Generate::opt[NUMBER_OF_LAYERS].opts.func = &Generate::deterministic;
	Generate::opt[NUMBER_OF_LAYERS].mean = 2;
	Generate::opt[PHASE2_PROBABILITY].opts.func = &Generate::deterministic;
	Generate::opt[PHASE2_PROBABILITY].mean = 0;
	break;
    }

    if ( argc == optind ) {
	output_file_name = "";
	
	LQIO::input_file_name = strdup( "" );

	if ( Flags::number_of_runs > 1 && !(Flags::lqx_output || Flags::spex_output) ) {
	    cerr << io_vars.lq_toolname << ": a directory name is required as an argument for the option '--number-of-runs=" << Flags::number_of_runs << "' unless SPEX or LQX output is being generated."  << endl;
	    exit ( 1 );
	}
	Generate aModel( Flags::number_of_runs );
	if ( fixedModel ) {
	    aModel.fixed( (Flags::lqx_output || Flags::spex_output)  ? &Generate::get_variable : &Generate::get_constant );
	} else {
	    aModel.random( (Flags::lqx_output || Flags::spex_output) ? &Generate::get_variable : &Generate::get_constant );
	}
	cout << aModel;

    } else if ( argc == optind + 1 ) {

	if ( Flags::number_of_runs <= 1 || Flags::lqx_output || Flags::spex_output ) {
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
	    Generate aModel( Flags::number_of_runs );
	    if ( fixedModel ) {
		aModel.fixed( (Flags::lqx_output || Flags::spex_output)  ? &Generate::get_variable : &Generate::get_constant );
	    } else {
		aModel.random( (Flags::lqx_output || Flags::spex_output) ? &Generate::get_variable : &Generate::get_constant );
	    }
	    output_file << aModel;
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
	    multi( argv[optind], Flags::number_of_runs );
	}

    } else {

	cerr << io_vars.lq_toolname << ": arg count." << endl;
	usage();
    }

    return 0;
}



/*
 * Convert input files to lqnx with lqx parameterization.
 */

static int
lqn2lqx( int argc, char **argv )
{
    Flags::lqx_output = true;	
    Flags::xml_output = true;	
    Flags::lqn2lqx    = true;

    unsigned int errorCode;
    extern char *optarg;

    static string opts = "";
#if HAVE_GETOPT_H
    static struct option longopts[N_LONG_OPTS];
    makeopts( opts, longopts );
#else
    makeopts( opts );
#endif

    optarg = 0;
    for ( ;; ) {
#if HAVE_GETOPT_LONG
	const int c = getopt_long( argc, argv, opts.c_str(), longopts, NULL );
#else	
	const int c = getopt( argc, argv, opts.c_str() );
#endif
	if ( c == EOF) break;

	command_line += " -";
	command_line += c;
	if ( optarg ) {
	    command_line += optarg;
	}

	switch( c ) {
	case 0x100+'a':
	    Flags::annotate_input = false;
	    break;

	case 'd':		/* Deterministic */
	    for ( unsigned int i = 0; Generate::opt[i].c || Generate::opt[i].name; ++i ) {
		if ( is_random_variable( i ) ) {
		    Generate::opt[i].opts.func = &Generate::deterministic;
		}
	    }

	case 'h':
	    usage();
	    break;

	case 'n':
	    if ( sscanf( optarg, "%ld", &Flags::number_of_runs ) != 1 ) {
		cerr << io_vars.lq_toolname << ": invalid argument to --number, " << optarg << endl;
		usage();
	    }
	    break;

	default:
	    if ( !getarg( c, optarg ) ) {
		usage();
	    }
	    break;
	}
    }

    if ( optind == argc ) {
	LQIO::input_file_name = "-";
	LQIO::DOM::Document* document = LQIO::DOM::Document::load( LQIO::input_file_name, LQIO::DOM::Document::AUTOMATIC_INPUT, "", &io_vars, errorCode, false );
	if ( document ) {
	    Generate aModel( Flags::number_of_runs, document );
	    aModel.reparameterize();
	    cout << aModel;
	}

    } else {
	for ( ;optind < argc; ++optind ) {
	    /* We need an Expat Document because we have to export our DOM.  The Xerces DOM isn't there */
	    LQIO::DOM::Document* document = LQIO::DOM::Document::load( argv[optind], LQIO::DOM::Document::AUTOMATIC_INPUT, "", &io_vars, errorCode, false );
	    if ( !document ) {
		continue;
	    } 
	    LQIO::Filename filename( argv[optind], "lqnx" );
	    Generate aModel( Flags::number_of_runs, document );
	    aModel.reparameterize();

	    if ( strcmp( filename(), argv[optind] ) == 0 ) {
		filename.backup();		/* Overwriting input file. -- back up */
	    }

	    ofstream output;
	    output.open( filename(), ios::out );
	    if ( !output ) {
		cerr << io_vars.lq_toolname << ": Cannot open output file " << filename() << " - "
		     << strerror( errno ) << endl;
		exit ( 1 );
	    }
	    output << aModel;
	    output.close();
	}
    }
    return 0;
}



void 
getargs( int argc, char **argv, bool indirect ) 
{
    extern char *optarg;
    char * options;
    char * value;
    int arg;

    static string opts = "";
#if HAVE_GETOPT_H
    static struct option longopts[N_LONG_OPTS];
    makeopts( opts, longopts );
#else
    makeopts( opts );
#endif

    optarg = 0;
    for ( ;; ) {
#if HAVE_GETOPT_LONG
	const int c = getopt_long( argc, argv, opts.c_str(), longopts, NULL );
#else	
	const int c = getopt( argc, argv, opts.c_str() );
#endif
	if ( c == EOF) break;

	command_line += " -";
	command_line += c;
	if ( optarg ) {
	    command_line += optarg;
	}

	switch( c ) {
	case 0x100+'a':
	    Flags::annotate_input = false;
	    break;

	case 'd':		/* Deterministic */
	    for ( unsigned int i = 0; Generate::opt[i].c || Generate::opt[i].name; ++i ) {
		if ( is_random_variable( i ) ) {
		    Generate::opt[i].opts.func = &Generate::deterministic;
		}
	    }

	case 0x100+'f':
	    Flags::structure = FUNNEL;
	    break;

	case 'G':
	    if ( !Generate::getGeneralArgs( optarg ) ) {
		usage();
	    }
	    break;

	case 'h':
	    usage();
	    break;

	case 0x100+'l':
	    Flags::lqx_output  = true;
	    Flags::xml_output  = true;
	    Flags::spex_output = false;
	    break;

	case 0x100+'L':
	    LQIO::input_file_name = optarg;
	    Flags::lqx_output  = true;
	    Flags::xml_output  = true;
	    Flags::spex_output = false;
	    break;

	case 'n':
	    if ( sscanf( optarg, "%ld", &Flags::number_of_runs ) != 1 ) {
		cerr << io_vars.lq_toolname << ": invalid argument to --number, " << optarg << endl;
		usage();
	    }
	    break;

	case 'O':
	    options = optarg;
	    arg = getsubopt( &options, const_cast<char * const *>(Options::io), &value );
	    switch ( arg ) {
	    case FORMAT_SRVN:	Flags::xml_output = false;	break;
#if defined(XML_OUTPUT)
	    case FORMAT_LQX:	Flags::lqx_output = true;	/* fall through */
	    case FORMAT_XML:	Flags::xml_output = true;	break;
#endif
	    default:
		usage();
	    }
	    break;

	case 0x100+'p':
	    Flags::structure = PYRAMID;
	    break;

	case 0x100+'P':
	    Flags::structure = PRODUCT_FORM;
	    break;
	
	case 'S':
	    if ( sscanf( optarg, "%ld", &seed ) != 1 ) {
		cerr << io_vars.lq_toolname << ": invalid seed value -- " << optarg << endl;
		usage();
	    } else {
		fixedModel = false;
	    }
	    break;

	case 0x100+'s':
	    Flags::spex_output = true;
	    Flags::xml_output = false;
	    Flags::lqx_output = false;
	    break;

	case 'v':
	    Flags::verbose = true;
	    break;

	case 0x100+'x':
	    Flags::xml_output = true;
	    Flags::spex_output = false;
	    break;

	case '?':
	    usage();
	    break;

	case '@':
	    if ( indirect ) {
		cerr << io_vars.lq_toolname << ": too many levels of indirection." << endl;
		exit( 1 );
	    } else {
		getargs_from_file( optarg );
	    }
	    break;

	default:
	    if ( !getarg( c, optarg ) ) {
		usage();
	    } else {
		fixedModel = false;
	    }
	    break;
	}
    }
}



/* Process all options here */

static bool
getarg( const char c, const char * optarg )
{
    for ( unsigned i = 0; Generate::opt[i].name != 0; ++i ) {
	if ( Generate::opt[i].c != c || Generate::opt[i].lqngen_only && Flags::lqn2lqx ) continue;
	if ( !is_random_variable( i ) ) break;

	char dist[32];
	double arg1;
	double arg2;
	int n = sscanf( optarg, "%30[a-zA-Z],%lf,%lf", &dist[0], &arg1, &arg2 );
	if ( n > 0 ) {
	    if ( strncmp( dist, "exponential", strlen(dist) ) == 0 ) {
		Generate::opt[i].opts.func = Generate::exponential;
	    } else if ( strncmp( dist, "loguniform", strlen(dist) ) == 0 ) {
		Generate::opt[i].opts.func = Generate::loguniform;
	    } else if ( strncmp( dist, "uniform", strlen(dist) ) == 0 ) {
		Generate::opt[i].opts.func = Generate::uniform;
	    } else if ( strncmp( dist, "deterministic", strlen(dist) ) == 0 ) {
		Generate::opt[i].opts.func = Generate::deterministic;
	    } else {
		cerr << io_vars.lq_toolname << ": Invalid argument for -" << c << " " << optarg << endl;
		return false;
	    } 
	    if ( n == 2 ) {
		Generate::opt[i].mean = arg1;
	    } else if ( n == 3 && Generate::opt[i].opts.func != Generate::deterministic ) {
		Generate::opt[i].floor = arg1;
		Generate::opt[i].mean = arg2;
	    } else {
		cerr << io_vars.lq_toolname << ": Invalid argument for -" << c << " " << optarg << endl;
		return false;
	    }

	} else {
	    n = sscanf( optarg, "%lf", &arg1 );
	    if ( n != 1 || arg1 < 0.0 ) {
		cerr << io_vars.lq_toolname << ": Invalid argument for -" << c << " " << optarg << " -- number expected." << endl;
		return false;
	    } else {
		Generate::opt[i].opts.func = Generate::deterministic;
		Generate::opt[i].mean = arg1;
	    }
	}

	/* Check args */

	if ( i == NUMBER_OF_LAYERS ) {
	    if ( Generate::opt[i].floor < 2 ) {
		cerr << io_vars.lq_toolname << ": Too few layers. " << endl;
		return false;
	    }
	} else if ( Generate::opt[i].opts.func == Generate::probability ) {
	    if ( Generate::opt[i].floor != 0.0 || Generate::opt[i].mean > 1.0 ) {
		cerr << io_vars.lq_toolname << ": Invalid probability for -" << c << endl;
		return false;
	    }
	}
	return true;
    }
    cerr << io_vars.lq_toolname << ": invalid flag -- " << static_cast<char>(c) << endl;
    return false;
}



/* 
 * Open file, tokenize the input, and call this function again. 
 */

static void
getargs_from_file( char * filename )
{
    char * argv[100];
    int argc = 0;
    char buf[BUFSIZ];

    FILE * fptr = fopen( filename, "r" );
    if ( !fptr ) {
	cerr << io_vars.lq_toolname << ": cannot open " << filename << ": " << strerror( errno ) << endl;
	exit( 1 );
    }

    argv[argc++] = "";

    size_t n = fread( buf, sizeof( char ), sizeof( buf ) - 1, fptr );	/* leave space for trailing nul */
    if ( n > 0 ) {

	/* Tokeninze input */

	for ( size_t i = 0; i < n && argc < 100; ++argc ) {
	    while ( isspace( buf[i] ) ) ++i;
	    if ( buf[i] == '\'' || buf[i] == '\"' ) {
		char stop = buf[i++];	/* Skip stop character */
		argv[argc] = &buf[i];
		while ( buf[i] && buf[i] != stop && i < n ) ++i;
	    } else {
		argv[argc] = &buf[i];
		while ( !isspace( buf[i] ) && i < n ) {
		    if ( buf[i] == '\\' && i+1 < n ) {
			i += 2;
		    } else {
			i += 1;
		    }
		}
	    }
	    if ( buf[i] ) buf[i++] = '\0';
	}

	int old_optind = optind;		/* Stack optind */
	optind = 1;
	getargs( argc, argv, true );
	if ( optind < argc ) {
	    cerr << io_vars.lq_toolname << ": Illegal input in " << filename << " starting from " << argv[optind] << endl;
	    exit( 1 );
	}
	optind = old_optind;			/* Reset optind */
    }
}


#if HAVE_GETOPT_H
static void
makeopts( string& opts, struct option longopts[] ) 
{
    int i = 0, k = 0; 
    for ( ; Generate::opt[i].name || Generate::opt[i].c ; ++i ) {
	longopts[k].has_arg = (Generate::opt[i].opts.null != 0 ? required_argument : no_argument);
	longopts[k].val = Generate::opt[i].c;
	longopts[k++].name = Generate::opt[i].name;
	if ( (Generate::opt[i].c & 0xff00) == 0 ) {
	    opts +=  static_cast<char>(Generate::opt[i].c);
	    if ( Generate::opt[i].opts.null != 0 ) {
		opts += ':';
	    }
	}
    }
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
	if ( Flags::verbose ) {
	    cerr << i << " ";
	}
        Generate aModel( Flags::number_of_runs );
	ofstream output_file;
	sprintf( file_name, "%s/case-%.*d.lqn", dir, w, i );
	output_file.open( file_name, ios::out );

	if ( !output_file ) {
	    cerr << io_vars.lq_toolname << ": Cannot open output file " << file_name << " - "
		 << strerror( errno ) << endl;
	    exit ( 1 );
	}

	LQIO::input_file_name = file_name;
	output_file_name = file_name;
	aModel.random( Flags::lqx_output ? &Generate::get_variable : &Generate::get_constant );
	output_file << aModel;
	output_file.close();
    }
    if ( Flags::verbose ) {
	cerr << endl;
    }
}



static bool
is_random_variable( unsigned int i )
{
    return Generate::opt[i].opts.func == &Generate::exponential
	|| Generate::opt[i].opts.func == &Generate::uniform 
	|| Generate::opt[i].opts.func == &Generate::loguniform
	|| Generate::opt[i].opts.func == &Generate::deterministic;
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

static void
usage()
{
    cerr << "Usage: " << io_vars.lq_toolname << " [OPTION]... [FILE|DIRECTORY]" << endl;
    cerr << "Options:" << endl;

    for ( unsigned int i = 0; Generate::opt[i].name != 0; ++i ) {
	if ( Generate::opt[i].lqngen_only && Flags::lqn2lqx ) continue;
	string s;
	if ( (Generate::opt[i].c & 0xff00) != 0 ) {
	    s = "      --";
	} else {
	    s = " -";
	    s += Generate::opt[i].c;
	    s += ",  --";
	}
	s += Generate::opt[i].name;

	if ( Generate::opt[i].opts.null != 0 ) {
	    s += "=";
	    if ( is_random_variable( i ) ) {
		s += "DIST[,LOW],MEAN";
	    } else if ( Generate::opt[i].opts.func == &Generate::probability ) {
		s += "PROB";
	    } else if ( Generate::opt[i].opts.s == Options::real ) {
		s += "N.N";
	    } else {
		s += "ARG";
	    }
	}
	cerr.setf( ios::left, ios::adjustfield );
	cerr << setw(40) << s << Generate::opt[i].msg << endl;
    }
    exit( 1 );
}
