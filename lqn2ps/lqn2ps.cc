/* srvn2eepic.c	-- Greg Franks Sun Jan 26 2003
 *
 * $Id: lqn2ps.cc 13641 2020-07-03 15:59:38Z greg $
 */

#include "lqn2ps.h"
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <fstream>
#include <stdexcept>
#include <cstdlib>
#include <errno.h>
#include <cstring>
#include <sstream>
#include <lqio/filename.h>
#if !HAVE_GETSUBOPT
#include <lqio/getsbopt.h>
#endif
#include <lqio/dom_document.h>
#include <lqio/srvn_results.h>
#include <lqio/srvn_spex.h>
#include "runlqx.h"
#include "getopt2.h"
#include "layer.h"
#include "model.h"
#include "errmsg.h"
#include "help.h"
#include "graphic.h"
#include "task.h"
#include "entry.h"
#include "processor.h"
#include "activity.h"

extern "C" int LQIO_debug;
extern "C" int resultdebug;

bool SolverInterface::Solve::solveCallViaLQX = false;/* Flag when a solve() call was made */

static bool setAllResultOptions( const bool yesOrNo );
static char * parse_file_name = 0;

#if (defined(linux) || defined(__linux__)) && !defined(__USE_XOPEN_EXTENDED)
extern "C" int getsubopt (char **, char * const *, char **);
#endif

#if defined(__SUNPRO_CC)
#define INIT_TRUE	(~0)
#else
#define INIT_TRUE	static_cast<int>(true)
#endif
#define INIT_FALSE	0

/* 
 * Sort on field 5.  Don't forget to update lqn2ps.h if you
 * add/delete lines!  Value is interpreted as an INTEGER so initialize
 * FLOATS at run time.  Set result to true for flags controlling
 * result output and false otherwise. 
 */

option_type Flags::print[] = {
/*    name                      c   arg                    opts                    (init) value       result msg */
    { "aggregate",             'A', "objects",             Options::activity,      {AGGREGATE_NONE},    false, "Aggregate sequences,activities,phases,entries,threads,all into parent object." },
    { "border",                'B', "border",              Options::real,          {0},                 false, "Set the border (in points)." },
    { "colour",                'C', "colour",              Options::colouring,     {COLOUR_RESULTS},    false, "Colour output." },
    { "diff-file",	       'D', "filename",            0,                      {0},			false, "Load parseable results generated using srvndiff --difference from filename." },
    { "font-size",             'F', "font-size",           Options::integer,       {9},                 false, "Set the font size (from 6 to 36 points)." },
    { "gnuplot",	       'G', "[var[,var]*]",	   Options::string,	   {0},		        false, "Output GNUPLOT for var (SPEX input only)." },
    { "input-format",	       'I', "format",		   Options::io,		   {FORMAT_UNKNOWN},	false, "Input file format." },
    { "help",                  'H', 0,                     0,                      {0},                 false, "Print help." },
    { "justification",         'J', "object=justification",Options::justification, {DEFAULT_JUSTIFY},   false, "Justification." },
    { "key",                   'K', "key",                 Options::key,           {0},                 false, "Print key." },             
    { "layering",              'L', "layering",            Options::layering,      {LAYERING_BATCH},    false, "Layering." },
    { "magnification",         'M', "magnification",       Options::real,          {0},                 false, "Magnification." },
    { "precision",             'N', "precision",           Options::integer,       {3},                 false, "Number of digits of output precision." },
    { "format",                'O', "format",              Options::io,            {FORMAT_POSTSCRIPT}, false, "Output file format." },
    { "processors",            'P', "processors",          Options::processor,     {PROCESSOR_DEFAULT}, false, "Print processors." },
    { "queueing-model",        'Q', "queueing-model",      Options::integer,       {0},                 false, "Print queueing model <n>." },
#if defined(REP2FLAT)
    { "replication",           'R', "operation",           Options::replication,   {REPLICATION_NOP},   false, "Transform replication." },
#endif
    { "submodel",              'S', "submodel",            Options::integer,       {0},                 false, "Print submodel <n>." },
    { "version",               'V', 0,                     0,                      {INIT_FALSE},        false, "Tool version." },
    { "warnings",              'W', 0,                     0,                      {INIT_FALSE},        false, "Suppress warnings." },
    { "x-spacing",             'X', "spacing[,width]",     Options::real,          {0},                 false, "X spacing [and task width] (points)." },
    { "y-spacing",             'Y', "spacing[,height]",    Options::real,          {0},                 false, "Y spacing [and task height] (points)." },
    { "special",               'Z', "special[=arg]",       Options::pragma,        {0},                 false, "Special option." },
    { "open-wait",             'a', 0,                     0,                      {INIT_TRUE},         true,  "Print queue length results for open arrivals." },
    { "throughput-bounds",     'b', 0,                     0,                      {INIT_FALSE},        true,  "Print task throughput bounds." },
    { "confidence-intervals",  'c', 0,                     0,                      {INIT_FALSE},        true,  "Print confidence intervals." },
    { "entry-utilization",     'e', 0,                     0,                      {INIT_FALSE},        true,  "Print entry utilization." },
    { "entry-throughput",      'f', 0,                     0,                      {INIT_FALSE},        true,  "Print entry throughput." },
    { "histograms",            'g', 0,                     0,                      {INIT_FALSE},        true,  "Print histograms." },
    { "hold-times",            'h', 0,                     0,                      {INIT_FALSE},        true,  "Print hold times." },
    { "input-parameters",      'i', 0,                     0,                      {INIT_TRUE},         false, "Print input parameters." },
    { "join-delays",           'j', 0,                     0,                      {INIT_TRUE},         true,  "Print join delay results." },
    { "chain",                 'k', "client",              Options::integer,       {0},                 false, "Print all paths from client <n>." },
    { "loss-probability",      'l', 0,                     0,                      {INIT_TRUE},         true,  "Print message loss probabilities." },
    { "output",                'o', "filename",            Options::string,        {0},                 false, "Redirect output to filename." },
    { "processor-utilization", 'p', 0,                     0,                      {INIT_TRUE},         true,  "Print processor utilization results." },
    { "processor-queueing",    'q', 0,                     0,                      {INIT_TRUE},         true,  "Print processor waiting time results." },
    { "results",               'r', 0,                     0,                      {INIT_TRUE},         false, "Print results." },
    { "service",               's', 0,                     0,                      {INIT_TRUE},         true,  "Print execution time results." },
    { "task-throughput",       't', 0,                     0,                      {INIT_TRUE},         true,  "Print task throughput results." },
    { "task-utilization",      'u', 0,                     0,                      {INIT_TRUE},         true,  "Print task utilization results." },
    { "variance",              'v', 0,                     0,                      {INIT_FALSE},        true,  "Print execution time variance results." },
    { "waiting",               'w', 0,                     0,                      {INIT_TRUE},         true,  "Print waiting time results." },
    { "service-exceeded",      'x', 0,                     0,                      {INIT_FALSE},        true,  "Print maximum execution time exceeded." },
    { "comment",	   768+'#', 0,			   0,			   {INIT_FALSE},	true,  "Print model comment." },
    { "verbose",           512+'V', 0,                     0,                      {INIT_FALSE},        false, "Verbose output." },
    { "ignore-errors",     512+'E', 0,			   0,			   {INIT_FALSE},        false, "Ignore errors during model checking phase." },
    { "task-service-time", 512+'P', 0,                     0,                      {INIT_FALSE},        false, "Print task service times (for --tasks-only)." },
    { "run-lqx",	   512+'l', 0,			   0,			   {INIT_FALSE},	false, "\"Run\" the LQX program instantiating variables and generating model files." },
    { "reload-lqx",	   512+'r', 0,			   0,			   {INIT_FALSE},	false, "\"Run\" the LQX program reloading results generated earlier." },
    { "output-lqx",	   512+'o', 0,			   0,			   {INIT_FALSE},	false, "Convert SPEX to LQX for XML output." },	
    { "include-only",      512+'I', "regexp",              Options::string,        {0},                 false, "Include only objects with name matching <regexp>" },

    /* -- below here is not stored in flag_values enumeration -- */

    /* Layering shortcuts */
#if 0
    { "client-layering",   512+'x', 0,                     0,                      {0},                 false, "NEW LAYERING STRATEGEY (EXPERIMENTAL)." },
#endif
    { "hwsw-layering",     512+'h', 0,                     0,                      {0},                 false, "Use HW/SW layering instead of batched layering." },
    { "srvn-layering",     512+'w', 0,                     0,                      {0},                 false, "Use SRVN layering instead of batched layering." }, 
    { "method-of-layers",  512+'m', 0,                     0,                      {0},                 false, "Use the Method Of Layers instead of batched layering." },
    /* Pragma shortcuts */
    { "flatten",	   512+'f', 0,			   0,			   {0},			false, "Flatten submodel/queueing output by placing clients in one layer." },
    { "no-sort",           512+'s', 0,                     0,                      {0},                 false, "Do not sort objects for output." },
    { "number-layers",     512+'n', 0,                     0,                      {0},                 false, "Print layer numbers." },
    { "rename",            512+'N', 0,                     0,                      {0},                 false, "Rename all objects." },
    { "tasks-only",        512+'t', 0,                     0,                      {0},                 false, "Print tasks only." },
    /* Miscellaneous */
    { "no-colour",	   512+'C', 0,			   0,			   {0},		        false, "Use grey scale when colouring result." },
    { "no-header",	   512+'H', 0,			   0,			   {0},			false, "Do not output the variable name header on SPEX results." },
    { "surrogates",        768+'z', 0,                     0,                      {0},                 false, "[Don't] Add surrogate tasks for submodel/include-only output." },
#if defined(REP2FLAT)
    { "merge-replicas",    512+'R', 0,                     0,                      {0},                 false, "Merge replicas from a flattened model back to a replicated model." },
#endif
    { "jlqndef",	   512+'j', 0,                     0,                      {0},			false, "Use jlqnDef-style icons (rectangles)." },
    { "parse-file",        512+'p', "filename",            0,                      {0},                 false, "Load parseable results from filename." },
    { "print-comment",	   512+'c', 0,			   0,			   {0},			false, "Print the model comment on stdout." },
    { "print-submodels",   512+'D', 0,                     0,			   {0},			false, "Show submodels." },
    { "print-summary",	   512+'S', 0,			   0,			   {0},			false, "Print model summary on stdout." },
    { "debug-lqx",	   512+'L', 0,                     0,                      {0},                 false, "Output debugging information while parsing LQX input." },
    { "debug-srvn",	   512+'Y', 0,                     0,                      {0},                 false, "Output debugging information while parsing SRVN input." },
    { "debug-p",	   512+'Z', 0,                     0,                      {0},                 false, "Output debugging information while parsing parseable results input." },
    { "debug-xml",         512+'X', 0,                     0,                      {0},                 false, "Output debugging information while parsing XML input." },
    { "debug-formatting",  512+'F', 0,                     0,                      {0},                 false, "Output debugging information while formatting." },
    { "dump-graphviz",	   512+'G', 0,			   0, 			   {0},			false, "Output LQX parse tree in graphviz format." },
    { "generate-manual",   512+'M', 0,                     0,                      {0},                 false, "Generate manual suitable for input to man(1)." },
    { 0,                         0, 0,                     0,                      {0},                 false, 0 }
};
#if HAVE_GETOPT_H
static void makeopts( string& opts, std::vector<struct option>& );
#else
static void makeopts( string& opts );
#endif


static char copyrightDate[20];
static void process( const string& inputFileName,  const string& output_file_name, int model_no );

/*----------------------------------------------------------------------*/
/*			      Main line					*/
/*----------------------------------------------------------------------*/

int
lqn2ps( int argc, char *argv[] )
{
    extern char *optarg;
    extern int optind;
    char * options;
    char * value;
    int arg;
    string output_file_name = "";

    sscanf( "$Date: 2020-07-03 11:59:38 -0400 (Fri, 03 Jul 2020) $", "%*s %s %*s", copyrightDate );

    static string opts = "";
#if HAVE_GETOPT_H
    static std::vector<struct option> longopts;
    makeopts( opts, longopts );
#else
    makeopts( opts );
#endif

    for ( ;; ) {
	char * endptr = 0;

#if HAVE_GETOPT_LONG
	const int c = getopt2_long( argc, argv, opts.c_str(), longopts.data(), NULL );
#else	
	const int c = getopt2( argc, argv, opts.c_str() );
#endif
	if ( c == EOF) break;
	const bool enable = (optsign == '+');
	command_line += " ";
#if HAVE_GETOPT_LONG
	if ( (c & 0xff00) != 0 ) {
	    for ( option_type * f = Flags::print; f->name; ++f ) {
		if ( f->c == c ) {
		    command_line += "-";		/* Can't use "--" in XML output */
		    command_line += f->name;
		    if ( optarg ) {
			command_line += "=";
		    }
		    break;
		}
	    }
	} else {
	    command_line += optsign;
	    command_line += c;
	}
#else
	command_line += c;
#endif
	if ( optarg ) {
	    command_line += optarg;
	}

	switch( c ) {
	case 'A':
	    options = optarg;
	    arg = getsubopt( &options, const_cast<char * const *>(Options::activity), &value );
	    if ( arg < 0 ) {
		invalid_option( c, optarg );
		exit( 1 );
	    }
	    Flags::print[AGGREGATION].value.i = arg;
	    break;
	    
	case 'B':
	    Flags::print[BORDER].value.f = strtod( optarg, &endptr );
	    if ( Flags::print[BORDER].value.f < 0.0 || *endptr != '\0' ) {
		invalid_option( c, optarg );
		exit( 1 );
	    } 
	    break;

	case 'C':
	    options = optarg;
	    arg = getsubopt( &options, const_cast<char * const *>(Options::colouring), &value );
	    if ( arg < 0 ) {
		invalid_option( c, optarg );
		exit( 1 );
	    }
	    Flags::print[COLOUR].value.i = arg;
	    if ( arg == COLOUR_DIFFERENCES && Flags::print[PRECISION].value.i == 3 ) {
		Flags::print[PRECISION].value.i = 1;
	    }
	    break;

	case 512+'c':
	    Flags::print_comment = true;
	    break;

	case 512+'C':
	    Flags::use_colour = false;
	    break;

	case 'D':
	    Flags::print[COLOUR].value.i = COLOUR_DIFFERENCES;
	    if ( Flags::print[PRECISION].value.i == 3 ) {
		Flags::print[PRECISION].value.i = 2;
	    }
	    /* Fall through... */
	case 512+'p':
	    parse_file_name = optarg;
	    if ( strcmp( parse_file_name, "-" ) != 0 && access( parse_file_name, R_OK ) != 0 ) {
		cerr << io_vars.lq_toolname << ": Cannot open parseable output file " << parse_file_name << " - "  
		     << strerror( errno ) << endl;
		exit ( 1 );
	    }
	    break;
	    
	case (512+'D'):
	    Flags::print_submodels = true;
	    break;

	case 512+'E':
	    Flags::print[IGNORE_ERRORS].value.b = true;
	    break;
	    
	case 'F':
	    Flags::print[FONT_SIZE].value.i = strtol( optarg, &endptr, 10 );
	    if ( *endptr != '\0' || Flags::print[FONT_SIZE].value.i < min_fontsize || max_fontsize < Flags::print[FONT_SIZE].value.i ) {
		invalid_option( c, optarg );
		exit( 1 );
	    }
	    break;

	case 512+'f':
	    Flags::flatten_submodel = true;
	    break;

	case 512+'F':
	    Flags::debug = true;
	    break;
		
	case 'G':
	    Flags::print[RUN_LQX].value.b 	= true;		    /* Reload lqx */
	    Flags::print[RELOAD_LQX].value.b	= true;
	    LQIO::Spex::setGnuplotVars( optarg );
	    break;
								      
	case 512+'G':
	    Flags::print[RUN_LQX].value.b 	= true;		    /* Run lqx */
	    Flags::dump_graphviz 		= true;
	    break;

	case 'H':
	    usage();
	    exit(0);

	case 512+'h':
	    Flags::print[PROCESSORS].value.i = PROCESSOR_ALL;
	    Flags::print[LAYERING].value.i = LAYERING_HWSW;
	    break;
	    
	case 512+'H':
            LQIO::Spex::__no_header = true;
	    break;

	case 'I':
	    arg = getsubopt( &options, const_cast<char * const *>(Options::io), &value );
	    switch ( arg ) {
	    case FORMAT_LQX:
	    case FORMAT_XML:
	    case FORMAT_SRVN:
		Flags::print[INPUT_FILE_FORMAT].value.i = arg;
		break;
	    default:
		invalid_option( c, optarg );
		exit( 1 );
	    }
	    break;

#if HAVE_REGEX_T
	case 512+'I':
	    Flags::print[INCLUDE_ONLY].value.r = static_cast<regex_t *>(malloc( sizeof( regex_t ) ));
	    regexp_check( regcomp( Flags::print[INCLUDE_ONLY].value.r, optarg, REG_EXTENDED ), Flags::print[INCLUDE_ONLY].value.r );
	    break;
#endif

	case 'J':
	    options = optarg;
	    while ( *options ) {
		arg = getsubopt( &options, const_cast<char * const *>(Options::justification), &value );
		justification_type justify;

		if ( !value ) {
		    justify = DEFAULT_JUSTIFY;
		} else if ( strcmp( value, "center" ) == 0 ) {
		    justify = CENTER_JUSTIFY;
		} else if ( strcmp( value, "left" ) == 0 ) {
		    justify = LEFT_JUSTIFY;
		} else if ( strcmp( value, "right" ) == 0 ){
		    justify = RIGHT_JUSTIFY;
		} else if ( strcmp( value, "align" ) == 0 ){
		    justify = ALIGN_JUSTIFY;
		} else if ( strcmp( value, "above" ) == 0 ){
		    justify = ABOVE_JUSTIFY;
		} else {
		    invalid_option( c, optarg );
		    exit( 1 );
		}

		switch ( arg ) {
		case 0:	
		    if ( justify != ABOVE_JUSTIFY ) {
			Flags::node_justification = justify; 
		    } else {
			cerr << io_vars.lq_toolname << ": -J" << optarg << "is invalid." << endl;
			exit( 1 );
		    }
		    break;   
		case 1:	
		    if ( justify != ALIGN_JUSTIFY ) {
			Flags::label_justification = justify; 
		    } else {
			cerr << io_vars.lq_toolname << ": -J" << optarg << "is invalid." << endl;
			exit( 1 );
		    }
		    break;
		case 2:	
		    if ( justify != ABOVE_JUSTIFY ) {
			Flags::activity_justification = justify; 
		    } else {
			cerr << io_vars.lq_toolname << ": -J" << optarg << "is invalid." << endl;
			exit( 1 );
		    }
		    break;
		default:
		    invalid_option( c, optarg );
		    exit( 1 );
		}
	    }
	    break;

	case 512+'j':
	    Flags::graphical_output_style = JLQNDEF_STYLE;
	    Flags::icon_slope = 0;
	    Flags::print[Y_SPACING].value.f = 45;
	    break;

	case 'K':
	    options = optarg;
	    arg = getsubopt( &options, const_cast<char * const *>(Options::key), &value );
	    switch ( arg ) {
	    case KEY_ON:           Flags::print[KEY].value.i = KEY_BOTTOM_LEFT; break;
	    default:
		if ( arg < KEY_ON ) {
		    Flags::print[KEY].value.i = arg; break;
		} else {
		    invalid_option( c, optarg );
		    exit( 1 );
		}
	    }
	    break;

	case 'k':
	    Flags::print[CHAIN].value.i = strtol( optarg, &endptr, 10 );
	    if ( *endptr != '\0' || Flags::print[CHAIN].value.i < 1 ) {
		invalid_option( c, optarg );
		exit( 1 );
	    }
	    Flags::print[PROCESSORS].value.i = PROCESSOR_ALL;
	    break;

	case 'L':
	    options = optarg;
	    while ( *options ) {
		arg = getsubopt( &options, const_cast<char * const *>(Options::layering), &value );

		switch ( arg ) {
		case LAYERING_HWSW:
		case LAYERING_SRVN:
		case LAYERING_SQUASHED:
		case LAYERING_PROCESSOR_TASK:
		case LAYERING_MOL:
		case LAYERING_TASK_PROCESSOR:
		    Flags::print[PROCESSORS].value.i = PROCESSOR_ALL;
		    Flags::print[LAYERING].value.i = arg;
		    break;

		case LAYERING_PROCESSOR:
		    Flags::print[PROCESSORS].value.i = PROCESSOR_NONE;
		    Flags::print[LAYERING].value.i = LAYERING_PROCESSOR;
		    break;

		case LAYERING_SHARE:
		    Flags::print[PROCESSORS].value.i = PROCESSOR_NONE;
		    Flags::print[LAYERING].value.i = LAYERING_SHARE;
		    break;

		case LAYERING_BATCH:
		    Flags::print[LAYERING].value.i = arg;
		    break;

#if HAVE_REGEX_T
		case LAYERING_GROUP:
		    if ( value ) {
			Model::add_group( value );
		    }
		    Flags::print[LAYERING].value.i = LAYERING_GROUP;
		    Flags::print[PROCESSORS].value.i = PROCESSOR_ALL;
		    break;
#endif

		default:
		    invalid_option( c, optarg );
		    exit( 1 );
		}
	    }
	    break;

	case 512+'l':
	    Flags::print[RUN_LQX].value.b 	= true;		    /* Run lqx */
	    break;

	case 512+'L':
	    ModLangParserTrace(stderr, "lqx:");			    /* Debug Lqx */
	    break;

	case 'M':
	    Flags::print[MAGNIFICATION].value.f = strtod( optarg, &endptr );
	    if ( *endptr != '\0' || Flags::print[MAGNIFICATION].value.f <= 0.0 ) {
		invalid_option( c, optarg );
		exit( 1 );
	    } 
	    break;

	case 512+'M':
	    man();
	    exit(0);

	case 512+'m':
	    Flags::print[PROCESSORS].value.i = PROCESSOR_ALL;
	    Flags::print[LAYERING].value.i = LAYERING_MOL;
	    break;
	    
	case 'N':
	    Flags::print[PRECISION].value.i = strtol( optarg, &endptr, 10 );
	    if ( *endptr != '\0' || Flags::print[PRECISION].value.i < 1 ) {
		invalid_option( c, optarg );
		exit( 1 );
	    } 
	    break;

	case (512+'n'):
	    Flags::print_layer_number 		= true; 
	    break;

	case (512+'N'):
	    Flags::rename_model	 		= true;
	    break;

	case 'o':

	    /* Output to special file of some sort.  Do not map filename. */

	    output_file_name = optarg;
	    break;
	    
	case 'O':
	    options = optarg;
	    arg = getsubopt( &options, const_cast<char * const *>(Options::io), &value );
	    if ( arg < 0 ) {
		invalid_option( c, optarg );
		exit( 1 );
	    } 
	    setOutputFormat( arg );
	    break;

	case 512+'o':
	    Flags::print[OUTPUT_FORMAT].value.i = FORMAT_LQX;
	    setOutputFormat( arg );
	    break;
	    
	case 'P':
	    options = optarg;
	    arg = getsubopt( &options, const_cast<char * const *>(Options::processor), &value );
	    if ( arg < 0 ) {
		invalid_option( c, optarg );
		exit( 1 );
	    } 
	    Flags::print[PROCESSORS].value.i = arg;
	    break;
	    
	case 512+'P':
	    pragma( "tasks-only", "" );
	    Flags::print[AGGREGATION].value.i = AGGREGATE_ENTRIES;
	    Flags::print[PRINT_AGGREGATE].value.b = true;
	    break;

	case 'Q':
	    Flags::print[QUEUEING_MODEL].value.i = strtol( optarg, &endptr, 10 );
	    if ( *endptr != '\0' || Flags::print[QUEUEING_MODEL].value.i < 1 ) {
		invalid_option( c, optarg );
		exit( 1 );
	    }
	    break;

	case 'r':
	    Flags::print[RESULTS].value.b = setAllResultOptions( enable );
	    break;

#if defined(REP2FLAT)
	case 'R':
	    options = optarg;
	    arg = getsubopt( &options, const_cast<char * const *>(Options::replication), &value );
	    if ( arg < 0 ) {
		invalid_option( c, optarg );
		exit( 1 );
	    }
	    Flags::print[REPLICATION].value.i = arg;
	    break;

	case 512+'R':
	    Flags::print[REPLICATION].value.i = REPLICATION_RETURN;
	    break;
#endif

	case 512+'r':
	    Flags::print[RUN_LQX].value.b 	= true;		    /* Reload lqx */
	    Flags::print[RELOAD_LQX].value.b	= true;
	    break;

	case 'S':
	    Flags::print[SUBMODEL].value.i = strtol( optarg, &endptr, 10 );
	    if ( *endptr != '\0' || Flags::print[SUBMODEL].value.i < 1 ) {
		invalid_option( c, optarg );
		exit( 1 );
	    }
	    Flags::print[PROCESSORS].value.i = PROCESSOR_ALL;
	    break;

	case 512+'s':
	    Flags::sort = NO_SORT;
	    break;

	case 512+'t':
	    pragma( "tasks-only", "" );
	    break;

	case 'V':
	    Flags::print[XX_VERSION].value.b = true;
	    break;
	
	case 512+'S':
	case 512+'V':	/* Always set... :-) */
	    Flags::print[SUMMARY].value.b = true;
	    break;

	case 512+'w':
	    Flags::print[PROCESSORS].value.i = PROCESSOR_ALL;
	    Flags::print[LAYERING].value.i = LAYERING_SRVN;
	    break;
	    
	case 'W':
	    io_vars.severity_level = LQIO::ADVISORY_ONLY;		/* Ignore warnings. */
	    break;

	case 'X':
	    switch ( sscanf( optarg, "%lf,%lf", &Flags::print[X_SPACING].value.f, &Flags::icon_width ) ) {
	    case 1: break;

	    default:
		if ( sscanf( optarg, ",%lf", &Flags::icon_width ) < 1 ) {
		    invalid_option( c, optarg );
		    exit( 1 );
		}
		/* Fall through */
	    case 2: 
		Flags::entry_width = Flags::icon_width * 0.625; 
		break;
	    }

	    break;

        case 512+'X':
	    LQIO::DOM::Document::__debugXML = true;
	    break;

	case 'Y':
	    switch ( sscanf( optarg, "%lf,%lf", &Flags::print[Y_SPACING].value.f, &Flags::icon_height ) ) {
	    case 1: break;

	    default:
		if ( sscanf( optarg, ",%lf", &Flags::icon_height ) < 1 ) {
		    invalid_option( c, optarg );
		    exit( 1 );
		}
		/* Fall through */
	    case 2: 
		Flags::entry_height = Flags::icon_height * 0.6; 
	    }
	    break;

	case 512+'Y':
	    LQIO_debug = true;
	    break;

	case 'Z':	/* Always set... :-) */
	    if ( !process_pragma( optarg ) ) {
		exit( 1 );
	    }
	    break;

	case 512+'z':
	    Flags::surrogates = false;
	    break;

	case 768+'z':
	    Flags::surrogates = true;
	    break;

	case 512+'Z':
	    resultdebug = true;
	    break;

	case 768+'#':
	    Flags::print[MODEL_COMMENT].value.b = true;
	    break;
	    
	default:
	    for ( int i = 0; Flags::print[i].name != 0; ++i ) {
		if ( !Flags::print[i].arg && c == Flags::print[i].c ) {
		    if ( enable ) {
			Flags::print[i].value.b = true;
			if ( Flags::print[i].result ) {
			    Flags::print[RESULTS].value.b = true;	/* Enable results */
			}
		    } else {
			Flags::print[i].value.b = false;
		    }
		    goto found2;
		}
	    }
	    usage( false );
	    exit( 1 );
	found2:
	    break;
	}
    }
    io_vars.lq_command_line = command_line.c_str();

    if ( Flags::print[XX_VERSION].value.b ) {
	cout << "Layered Queueing Network file conversion program, Version " << VERSION << endl << endl;
	cout << "  Copyright " << copyrightDate << " the Real-Time and Distributed Systems Group," << endl;
	cout << "  Department of Systems and Computer Engineering," << endl;
	cout << "  Carleton University, Ottawa, Ontario, Canada. K1S 5B6" << endl << endl;
    }
	
    /* Check for sensible combinations of options. */

    if ( Flags::annotate_input && !input_output() ) {
	cerr << io_vars.lq_toolname << ": -Z " << Options::pragma[PRAGMA_ANNOTATE] 
	     << " and " << Options::io[Flags::print[OUTPUT_FORMAT].value.i]
	     << " output are mutually exclusive." << endl;
	Flags::annotate_input = false;
    }

    if ( Flags::print[AGGREGATION].value.i == AGGREGATE_ENTRIES && !(graphical_output() || queueing_output()) ) {
	cerr << io_vars.lq_toolname << ": -Z" << Options::pragma[PRAGMA_TASKS_ONLY] 
	     << " and " <<  Options::io[Flags::print[OUTPUT_FORMAT].value.i] 
	     << " output are mutually exclusive." << endl;
	exit( 1 );
    }

#if HAVE_REGEX_T
    if ( Flags::print[INCLUDE_ONLY].value.r && submodel_output() ) {
	cerr << io_vars.lq_toolname << ": -I<regexp> "
	     << "and -S" <<  Flags::print[SUBMODEL].value.i 
	     << " are mutually exclusive." << endl;
	exit( 1 );
    }
#endif

    if ( submodel_output() && Flags::print_submodels ) {
	cerr << io_vars.lq_toolname << ": -S" << Flags::print[SUBMODEL].value.i
	     << " and --debug-submodels are mutually exclusive." << endl;
	Flags::print_submodels = false;
    }

    if ( queueing_output() ) {
	Flags::arrow_scaling *= 0.75;
//	Flags::print[PROCESSORS].value.i = PROCESSOR_ALL;

	if ( submodel_output() ) {
	    cerr << io_vars.lq_toolname << ": -Q" << Flags::print[QUEUEING_MODEL].value.i
		 << "and -S" <<  Flags::print[SUBMODEL].value.i 
		 << " are mutually exclusive." << endl;
	    exit( 1 );
	} else if ( !graphical_output() 
		    && Flags::print[OUTPUT_FORMAT].value.i != FORMAT_LQX
		    && Flags::print[OUTPUT_FORMAT].value.i != FORMAT_XML
	    ) {
	    cerr << io_vars.lq_toolname << ": -Q" << Flags::print[QUEUEING_MODEL].value.i
		 << " and " << Options::io[Flags::print[OUTPUT_FORMAT].value.i] 
		 << " output are mutually exclusive." << endl;
	    exit( 1 );
	} else if ( Flags::print[AGGREGATION].value.i != AGGREGATE_ENTRIES && !graphical_output() ) {
	    Flags::print[AGGREGATION].value.i = AGGREGATE_ENTRIES;
	    cerr << io_vars.lq_toolname << ": aggregating entries to tasks with " 
		 << Options::io[Flags::print[OUTPUT_FORMAT].value.i] << " output." << endl;
	}
    }


    if ( Flags::print[OUTPUT_FORMAT].value.i == FORMAT_SRVN && !partial_output() ) {
	Flags::print[RESULTS].value.b = false;	/* Ignore results */
    }

    if ( Flags::flatten_submodel && !(submodel_output() || queueing_output()) ) {
	cerr << io_vars.lq_toolname << ": -Z" << Options::pragma[PRAGMA_FLATTEN_SUBMODEL]
	     << " can only be used with either -Q<n> -S<n>." << endl;
    }

    if ( submodel_output() && Flags::print[LAYERING].value.i == LAYERING_SQUASHED ) {
	cerr << io_vars.lq_toolname << ": -L" << Options::layering[LAYERING_SQUASHED]
	     << " can only be used with full models." << endl;
    }

    if ( Flags::print[PROCESSORS].value.i == PROCESSOR_NONE 
	 || Flags::print[LAYERING].value.i == LAYERING_PROCESSOR
	 || Flags::print[LAYERING].value.i == LAYERING_SHARE ) {
	Flags::print[PROCESSOR_QUEUEING].value.b = false;
    }

    /*
     * Change font size because scaleBy doesn't -- Fig doesn't use points for it's coordinates, 
     * but it does use points for it's labels.
     */

    Flags::print[FONT_SIZE].value.i = (int)(Flags::print[FONT_SIZE].value.i * Flags::print[MAGNIFICATION].value.f + 0.5);

#if HAVE_GD_H && HAVE_LIBGD
    switch( Flags::print[OUTPUT_FORMAT].value.i ) {
#if HAVE_GDIMAGEGIFPTR
    case FORMAT_GIF:
#endif
#if HAVE_LIBJPEG
    case FORMAT_JPEG:
#endif
#if HAVE_LIBPNG
    case FORMAT_PNG:
#endif
	GD::testForTTF();	/* Font selection changes */
	break;
    }
#endif

    /* If stdout is not a terminal For pipelines.	*/

#if !defined(WINNT) && !defined(MSDOS)
    if ( output_file_name == "" && LQIO::Filename::isWriteableFile( fileno( stdout ) ) > 0 ) {
	output_file_name = "-";
    }
#endif

    if ( output_file_name == "-" ) {
	switch( Flags::print[OUTPUT_FORMAT].value.i ) {
#if defined(EMF_OUTPUT)
	case FORMAT_EMF:
	    if ( LQIO::Filename::isRegularFile( fileno( stdout ) ) == 0 ) {
		cerr << io_vars.lq_toolname << ": Cannot write " 
		     << Options::io[Flags::print[OUTPUT_FORMAT].value.i] 
		     << " to stdout - stdout is not a regular file."  << endl;
		exit( 1 );
	    }
	    break;
#endif
#if defined(SXD_OUTPUT)
	case FORMAT_SXD:
	    cerr << io_vars.lq_toolname << ": Cannot write " 
		 << Options::io[Flags::print[OUTPUT_FORMAT].value.i] 
		 << " to stdout."  << endl;
	    exit( 1 );
	    break;
#endif
	}
    }

    if ( optind == argc ) {
	process( "-", output_file_name, 1 );
    } else {
	for ( int i = 1; optind < argc; ++optind, ++i ) {
	    process( argv[optind], output_file_name, i );
	}
    }

#if HAVE_REGEX_T
    if ( Flags::print[INCLUDE_ONLY].value.r ) {
	regfree( Flags::print[INCLUDE_ONLY].value.r );
	free( Flags::print[INCLUDE_ONLY].value.r );
    }
    if ( Flags::client_tasks ) {
	regfree( Flags::client_tasks );
	free( Flags::client_tasks );
    }
#endif

    return 0;
}


/*
 * Process input and save.
 */

static void
process( const string& input_file_name, const string& output_file_name, int model_no )
{
    Flags::have_results = false;		/* Reset for each run. */
    Flags::instantiate  = false;

    io_vars.reset();

    ::Task::reset();
    ::Entry::reset();
    ::Call::reset();
    ::Activity::reset();

    LQIO::DOM::Document::input_format input_format = LQIO::DOM::Document::AUTOMATIC_INPUT;
    switch ( Flags::print[INPUT_FILE_FORMAT].value.i ) {
    case FORMAT_LQX:
    case FORMAT_XML:
	input_format = LQIO::DOM::Document::XML_INPUT;
	break;
    case FORMAT_SRVN:
	input_format = LQIO::DOM::Document::LQN_INPUT;
	break;
    }

    /* This is a departure from before -- we begin by loading a model.  Load results if possible (except if overridden with a parseable output filename */

    unsigned int errorCode;
    LQIO::DOM::Document* document = LQIO::DOM::Document::load( input_file_name, input_format, &io_vars, errorCode, parse_file_name == 0 && Flags::print[RESULTS].value.b );
    if ( !document ) {
	cerr << io_vars.lq_toolname << ": Input model was not loaded successfully." << endl;
	io_vars.error_count += 1;
	return;
    }
    if ( parse_file_name && Flags::print[RESULTS].value.b ) {
	try {
	    Flags::have_results = LQIO::SRVN::loadResults( parse_file_name );
	} 
	catch ( const runtime_error &error ) {
	    cerr << io_vars.lq_toolname << ": Cannot load results file " << parse_file_name << " - " << error.what() << "." << endl;
	    Flags::have_results = false;
	    if ( output_output() ) return;
	}
	if ( !Flags::have_results ) {
	    cerr << io_vars.lq_toolname << ": Cannot load results file " << parse_file_name << " - " << strerror( errno ) << "." << endl;
	    if ( output_output() ) return;
	}
    } else {
	Flags::have_results = Flags::print[RESULTS].value.b && document->hasResults();
    }

    /* Now fold, mutiliate and spindle */

    Model::prepare( document );			/* This creates the various objects 	*/
    const unsigned n_layers = Model::topologicalSort();

    Model * aModel;
    switch ( Flags::print[LAYERING].value.i ) {
    case LAYERING_BATCH:
	aModel = new Batch_Model( document, input_file_name, output_file_name, n_layers );
	break;
    case LAYERING_GROUP:
    case LAYERING_SHARE:		/* ??? */
	aModel = new BatchGroup_Model( document, input_file_name, output_file_name, n_layers );
	break;
    case LAYERING_HWSW:
	aModel = new HWSW_Model( document, input_file_name, output_file_name, n_layers );
	break;
    case LAYERING_SRVN:
	aModel = new SRVN_Model( document, input_file_name, output_file_name, n_layers );
	break;
    case LAYERING_PROCESSOR:
	aModel = new BatchProcessor_Model( document, input_file_name, output_file_name, n_layers );
	break;
    case LAYERING_PROCESSOR_TASK:
    case LAYERING_TASK_PROCESSOR:
	aModel = new ProcessorTask_Model( document, input_file_name, output_file_name, n_layers );
	break;
    case LAYERING_SQUASHED:
	aModel = new Squashed_Model( document, input_file_name, output_file_name );
	break;
    case LAYERING_MOL:
	aModel = new MOL_Model( document, input_file_name, output_file_name, n_layers );
	break;
    default:	
	abort();
    }

    aModel->setModelNumber( model_no );

    if ( aModel->process() ) {		/* This layerizes and renders the model */
	LQX::Program * program = 0;

	try {
	    program = document->getLQXProgram();
	    if ( program && Flags::print[RUN_LQX].value.b ) {
		Flags::instantiate  = true;

		if (program == NULL) {
		    LQIO::solution_error( LQIO::ERR_LQX_COMPILATION, input_file_name.c_str() );
		} else { 
		    /* Attempt to run the program */
		    document->registerExternalSymbolsWithProgram(program);
		    if ( Flags::print[RELOAD_LQX].value.b ) {
			program->getEnvironment()->getMethodTable()->registerMethod(new SolverInterface::Solve(document, &Model::reload, aModel));
		    } else {
			program->getEnvironment()->getMethodTable()->registerMethod(new SolverInterface::Solve(document, &Model::store, aModel));
		    }
		    LQIO::RegisterBindings(program->getEnvironment(), document);
	
		    int status = 0;
		    FILE * output = 0;
		    if ( output_file_name.size() > 0 && output_file_name != "-" && LQIO::Filename::isRegularFile(output_file_name) ) {
			output = fopen( output_file_name.c_str(), "w" );
			if ( !output ) {
			    solution_error( LQIO::ERR_CANT_OPEN_FILE, output_file_name.c_str(), strerror( errno ) );
			    status = FILEIO_ERROR;
			} else {
			    program->getEnvironment()->setDefaultOutput( output );	/* Default is stdout */
			}
		    }

		    if ( Flags::dump_graphviz ) {
			program->getGraphvizRepresentation( std::cout );
		    } else if ( status == 0 ) {
			/* Invoke the LQX program itself */
			if ( !program->invoke() ) {
			    LQIO::solution_error( LQIO::ERR_LQX_EXECUTION, input_file_name.c_str() );
			} else if ( !SolverInterface::Solve::solveCallViaLQX ) {
			    /* There was no call to solve the LQX */
			    LQIO::solution_error( LQIO::ADV_LQX_IMPLICIT_SOLVE, input_file_name.c_str() );
			    std::vector<LQX::SymbolAutoRef> args;
			    program->getEnvironment()->invokeGlobalMethod("solve", &args);
			}
		    }
		    if ( output ) {
			fclose( output );
		    }
		}
	    } else {
		aModel->store();	/* This prints out the current values */
	    }
	    if ( program ) {
		delete program;
	    }
	}
#if !(__GNUC__ && __GNUC__ < 3)
	catch ( const ios_base::failure &error ) {
	    cerr << io_vars.lq_toolname << ": " << error.what() << endl;
	}
#endif
	catch ( const runtime_error &error ) {
	    cerr << io_vars.lq_toolname << ": " << error.what() << endl;
	}
    }

    delete document;
    delete aModel;
}

#if HAVE_GETOPT_H
static void
makeopts( string& opts, std::vector<struct option>& longopts ) 
{
    unsigned int k = 0;
    struct option opt;
    opt.flag = 0;
    opt.val  = 0;
    opt.name = 0;
    opt.has_arg = 0;
    const unsigned int count = sizeof( Flags::print ) / sizeof( Flags::print[0] );
    longopts.resize( count * 2, opt );
    for ( unsigned int i = 0; i < count; ++i, ++k ) {
	longopts[k].has_arg = (Flags::print[i].arg != 0 ? required_argument : no_argument);
	longopts[k].name    = Flags::print[i].name;
	longopts[k].val     = Flags::print[i].c;
	if ( (Flags::print[i].c & 0xff00) == 0 && islower( Flags::print[i].c ) && Flags::print[i].arg == 0 ) {
	    /* These are the +/- options */
	    longopts[k].val |= 0x0100;					/* + case is > 256 */
	    k += 1;
	    string name = "no-";					/* - case is no-<name> */
	    name += Flags::print[i].name;
	    longopts[k].name = strdup( name.c_str() );			/* Make a copy */
	    longopts[k].val = Flags::print[i].c;
	} else if ( (Flags::print[i].c & 0xff00) == 0x0300 ) {
	    /* These are the +/- options */
	    k += 1;
	    string name = "no-";					/* - case is no-<name> */
	    name += Flags::print[i].name;
	    longopts[k].name = strdup( name.c_str() );			/* Make a copy */
	    longopts[k].val = (Flags::print[i].c & ~0x0100);		/* Clear the bit */
	}

	if ( (Flags::print[i].c & 0xff00) == 0 ) {
	    opts +=  static_cast<char>(Flags::print[i].c);
	    if ( Flags::print[i].arg ) {
		opts += ':';
	    }
	}
    }
}
#else
static void
makeopts( string& opts ) 
{
    for ( unsigned int i = 0; Flags::print[i].name != 0; ++i ) {
	opts += static_cast<char>(Flags::print[i].c);
	if ( Flags::print[i].arg ) {
	    opts += ':';
	}
    }
}
#endif


void
setOutputFormat( const int i ) 
{
    Flags::print[OUTPUT_FORMAT].value.i = i;

    switch ( i ) {
    case FORMAT_OUTPUT:
    case FORMAT_PARSEABLE:
    case FORMAT_RTF:
    case FORMAT_LQX:
    case FORMAT_XML:
	Flags::print[LAYERING].value.i = LAYERING_PROCESSOR;	/* Order by processors */
	Flags::print[PROCESSORS].value.i = PROCESSOR_ALL;  	/* Print all processors. */
	Flags::print[PRECISION].value.i = 7;			/* Increase default precision */
	Flags::print[INPUT_PARAMETERS].value.b = true;     	/* input parameters. */
	Flags::print[CONFIDENCE_INTERVALS].value.b = true; 	/* Confidence Intervals */
	Flags::surrogates = true;				/* Always add surrogates */
	setAllResultOptions( true );
	break;

    case FORMAT_SRVN:
	Flags::print[LAYERING].value.i = LAYERING_PROCESSOR;	/* Order by processors */
	Flags::surrogates = true;				/* Always add surrogates */
	break;

    case FORMAT_NULL:
	break;

#if defined(X11_OUTPUT)
    case FORMAT_X11:
	break;
#endif
    }
}


/*
 * This function is used to set all of the output result options to
 * either true or false.
 */

static bool
setAllResultOptions( const bool yesOrNo )
{
    for ( unsigned i = 0; i < SERVICE_EXCEEDED; ++i ) {
	if ( Flags::print[i].result ) {
	    Flags::print[i].value.b = yesOrNo;     /* Print entry throughput. */
	}
    }

    return yesOrNo;
}
