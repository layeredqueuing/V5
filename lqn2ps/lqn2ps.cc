/*  -*- c++ -*-
 * $Id: lqn2ps.cc 15170 2021-12-07 23:33:05Z greg $
 *
 * Command line processing.
 *
 * Copyright the Real-Time and Distributed Systems Group,
 * Department of Systems and Computer Engineering,
 * Carleton University, Ottawa, Ontario, Canada. K1S 5B6
 *
 * December 15, 2020
 *
 * ------------------------------------------------------------------------
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
#include <libgen.h>
#include <lqio/filename.h>
#if !HAVE_GETSUBOPT
#include <lqio/getsbopt.h>
#endif
#include <lqio/dom_document.h>
#include <lqio/srvn_spex.h>
#include "runlqx.h"
#include "errmsg.h"
#include "getopt2.h"
#include "model.h"
#include "help.h"

extern "C" int LQIO_debug;
extern "C" int resultdebug;

bool SolverInterface::Solve::solveCallViaLQX = false;/* Flag when a solve() call was made */

static bool setAllResultOptions( const bool yesOrNo );
static char * parse_file_name = nullptr;

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

Options::Type Flags::print[] = {
/*    name                      c   arg                    opts                     (init) value        result msg */
    { "aggregate",             'A', "objects",             {&Options::aggregate,    Aggregate::NONE},   false, "Aggregate sequences,activities,phases,entries,threads,all into parent object." },
    { "border",                'B', "border",              {&Options::real,	    static_cast<std::string *>(nullptr)},	false, "Set the border (in points)." },
    { "colour",                'C', "colour",              {&Options::colouring,    Colouring::RESULTS},false, "Colour output." },
    { "diff-file",	       'D', "filename",            {0,                      0},			false, "Load parseable results generated using srvndiff --difference from filename." },
    { "font-size",             'F', "font-size",           {&Options::integer,      9},                 false, "Set the font size (from 6 to 36 points)." },
    { "input-format",	       'I', "ARG",		   {&Options::file_format,  File_Format::UNKNOWN}, false, "Input file format." },
    { "help",                  'H', 0,                     {0,                      0},                 false, "Print help." },
    { "justification",         'J', "object=ARG",	   {&Options::justification,Justification::DEFAULT},   false, "Justification." },
    { "key",                   'K', "key",                 {&Options::key_position, 0},                 false, "Print key." },             
    { "layering",              'L', "layering",            {&Options::layering,     Layering::BATCH},   false, "Layering." },
    { "magnification",         'M', "magnification",       {&Options::real,         0.0},               false, "Magnification." },
    { "precision",             'N', "precision",           {&Options::integer,      3},                 false, "Number of digits of output precision." },
    { "format",                'O', "format",              {&Options::file_format,  File_Format::POSTSCRIPT}, false, "Output file format." },
    { "processors",            'P', "processors",          {&Options::processors,   Processors::DEFAULT}, false, "Print processors." },
    { "queueing-model",        'Q', "queueing-model",      {&Options::integer,      0},                 false, "Print queueing model <n>." },
#if REP2FLAT
    { "replication",           'R', "ARG",	           {&Options::replication,  Replication::NONE}, false, "Transform replication." },
#endif
    { "submodel",              'S', "submodel",            {&Options::integer,	    0},                 false, "Print submodel <n>." },
    { "version",               'V', 0,                     {0,                      INIT_FALSE},        false, "Tool version." },
    { "warnings",              'W', 0,                     {0,                      INIT_FALSE},        false, "Suppress warnings." },
    { "x-spacing",             'X', "spacing[,width]",     {&Options::real,         0.0},               false, "X spacing [and task width] (points)." },
    { "y-spacing",             'Y', "spacing[,height]",    {&Options::real,         0.0},               false, "Y spacing [and task height] (points)." },
    { "special",               'Z', "ARG[=value]",	   {&Options::special,      Special::NONE},     false, "Special option." },
    { "open-wait",             'a', 0,                     {0,                      INIT_TRUE},         true,  "Print queue length results for open arrivals." },
    { "throughput-bounds",     'b', 0,                     {0,                      INIT_FALSE},        true,  "Print task throughput bounds." },
    { "confidence-intervals",  'c', 0,                     {0,                      INIT_FALSE},        true,  "Print confidence intervals." },
    { "entry-utilization",     'e', 0,                     {0,                      INIT_FALSE},        true,  "Print entry utilization." },
    { "entry-throughput",      'f', 0,                     {0,                      INIT_FALSE},        true,  "Print entry throughput." },
    { "histograms",            'g', 0,                     {0,                      INIT_FALSE},        true,  "Print histograms." },
    { "hold-times",            'h', 0,                     {0,                      INIT_FALSE},        true,  "Print hold times." },
    { "input-parameters",      'i', 0,                     {0,                      INIT_TRUE},         false, "Print input parameters." },
    { "join-delays",           'j', 0,                     {0,                      INIT_TRUE},         true,  "Print join delay results." },
    { "chain",                 'k', "client",              {&Options::integer,      0},                 false, "Print all paths from client <n>." },
    { "loss-probability",      'l', 0,                     {0,                      INIT_TRUE},         true,  "Print message loss probabilities." },
    { "output",                'o', "filename",            {&Options::string,       static_cast<std::string *>(nullptr)},	false, "Redirect output to filename." },
    { "processor-utilization", 'p', 0,                     {0,                      INIT_TRUE},         true,  "Print processor utilization results." },
    { "processor-queueing",    'q', 0,                     {0,                      INIT_TRUE},         true,  "Print processor waiting time results." },
    { "results",               'r', 0,                     {0,                      INIT_TRUE},         false, "Print results." },
    { "service",               's', 0,                     {0,                      INIT_TRUE},         true,  "Print execution time results." },
    { "task-throughput",       't', 0,                     {0,                      INIT_TRUE},         true,  "Print task throughput results." },
    { "task-utilization",      'u', 0,                     {0,                      INIT_TRUE},         true,  "Print task utilization results." },
    { "variance",              'v', 0,                     {0,                      INIT_FALSE},        true,  "Print execution time variance results." },
    { "waiting",               'w', 0,                     {0,                      INIT_TRUE},         true,  "Print waiting time results." },
    { "service-exceeded",      'x', 0,                     {0,                      INIT_FALSE},        true,  "Print maximum execution time exceeded." },
    { "comment",	 0x300+'#', 0,			   {0,      		    INIT_FALSE},	true,  "Print model comment." },
    { "solver-info",	   512+'!', 0,                     {0,      		    INIT_FALSE},	true,  "Print solver information." },
    { "verbose",           512+'V', 0,                     {0,                      INIT_FALSE},        false, "Verbose output." },
    { "ignore-errors",     512+'E', 0,			   {0,      		    INIT_FALSE},        false, "Ignore errors during model checking phase." },
    { "task-service-time", 512+'P', 0,                     {0,                      INIT_FALSE},        false, "Print task service times (for --tasks-only)." },
    { "run-lqx",	   512+'l', 0,			   {0,       		    INIT_FALSE},	false, "\"Run\" the LQX program instantiating variables and generating model files." },
    { "reload-lqx",	   512+'r', 0,			   {0,      		    INIT_FALSE},	false, "\"Run\" the LQX program reloading results generated earlier." },
    { "output-lqx",	   512+'o', 0,			   {0,      		    INIT_FALSE},	false, "Convert SPEX to LQX for XML output." },	
    { "include-only",      512+'I', "regexp",              {&Options::string,       static_cast<std::string *>(nullptr)},	false, "Include only objects with name matching <regexp>" },

    /* -- below here is not stored in flag_values enumeration -- */

    /* Layering shortcuts */
#if 0
    { "client-layering",   512+'x', 0,                     {0,                      0},                 false, "NEW LAYERING STRATEGEY (EXPERIMENTAL)." },
#endif
    { "hwsw-layering",     512+'h', 0,                     {0,                      0},                 false, "Use HW/SW layering instead of batched layering." },
    { "srvn-layering",     512+'w', 0,                     {0,                      0},                 false, "Use SRVN layering instead of batched layering." }, 
    { "method-of-layers",  512+'m', 0,                     {0,                      0},                 false, "Use the Method Of Layers instead of batched layering." },
    /* Special shortcuts */
    { "flatten",	   512+'f', 0,			   {0,			    0},			false, "Flatten submodel/queueing output by placing clients in one layer." },
    { "no-sort",           512+'s', 0,                     {0,                      0},                 false, "Do not sort objects for output." },
    { "number-layers",     512+'n', 0,                     {0,                      0},                 false, "Print layer numbers." },
    { "rename",            512+'N', 0,                     {0,                      0},                 false, "Rename all objects." },
    { "tasks-only",        512+'t', 0,                     {0,                      0},                 false, "Print tasks only." },
#if BUG_270
    { "bcmp",		 0x300+'B', 0,			   {0,			    0},			false, "[Don't] perform BCMP model conversion." },
#endif
    /* Miscellaneous */
    { "no-activities",	   512+'A', 0,			   {0,			    0},			false, "Don't print activities." },
    { "no-colour",	   512+'C', 0,			   {0,			    0},		        false, "Use grey scale when colouring result." },
    { "no-header",	   512+'H', 0,			   {0,			    0},			false, "Do not output the variable name header on SPEX results." },
    { "surrogates",      0x300+'z', 0,                     {0,                      0},                 false, "[Don't] add surrogate tasks for submodel/include-only output." },
#if REP2FLAT
    { "merge-replicas",    512+'R', 0,                     {0,                      0},                 false, "Merge replicas from a flattened model back to a replicated model." },
#endif
    { "jlqndef",	   512+'j', 0,                     {0,                      0},			false, "Use jlqnDef-style icons (rectangles)." },
    { "parse-file",        512+'p', "filename",            {0,                      0},                 false, "Load parseable results from filename." },
    { "print-comment",	   512+'c', 0,			   {0,		       	    0},			false, "Print the model comment on stdout." },
    { "print-submodels",   512+'D', 0,                     {0,			    0},			false, "Show submodels." },
    { "print-summary",	   512+'S', 0,			   {0,			    0},			false, "Print model summary on stdout." },
    { "debug-json",	   512+'J', 0,			   {0,			    0},			false, "Output debugging information while parsing JSON input." },
    { "debug-lqx",	   512+'L', 0,                     {0,                      0},                 false, "Output debugging information while parsing LQX input." },
    { "debug-srvn",	   512+'Y', 0,                     {0,                      0},                 false, "Output debugging information while parsing SRVN input." },
    { "debug-p",	   512+'Z', 0,                     {0,                      0},                 false, "Output debugging information while parsing parseable results input." },
    { "debug-xml",         512+'X', 0,                     {0,                      0},                 false, "Output debugging information while parsing XML input." },
    { "debug-formatting",  512+'F', 0,                     {0,                      0},                 false, "Output debugging information while formatting." },
    { "dump-graphviz",	   512+'G', 0,			   {0, 			    0},			false, "Output LQX parse tree in graphviz format." },
    { "generate-manual",   512+'M', 0,                     {0,                      0},                 false, "Generate manual suitable for input to man(1)." }
};

const unsigned int Flags::size = sizeof( Flags::print ) / sizeof( Flags::print[0] );


#if HAVE_GETOPT_H
static void makeopts( std::string& opts, std::vector<struct option>& );
#else
static void makeopts( std::string& opts );
#endif

static char copyrightDate[20];
static LQIO::DOM::Pragma pragmas;

/*----------------------------------------------------------------------*/
/*			      Main line					*/
/*----------------------------------------------------------------------*/

int
main(int argc, char *argv[])
{
    /* We can only initialize integers in the Flags object -- initialize floats here. */

    Flags::print[MAGNIFICATION].opts.value.d = 1.0;
    Flags::print[BORDER].opts.value.d = 18.0;
    Flags::print[X_SPACING].opts.value.d = DEFAULT_X_SPACING;
    Flags::print[Y_SPACING].opts.value.d = DEFAULT_Y_SPACING;

    LQIO::io_vars.init( VERSION, basename( argv[0] ), severity_action, local_error_messages, LSTLCLERRMSG-LQIO::LSTGBLERRMSG );

    command_line += LQIO::io_vars.lq_toolname;

    /* If we are invoked as lqn2xxx or rep2flat, then enable other options. */

    const char * p = strrchr( LQIO::io_vars.toolname(), '2' );
    if ( p ) {
	p += 1;
	for ( std::map<const File_Format,const std::string>::const_iterator j = Options::file_format.begin(); j != Options::file_format.end(); ++j ) {
	    if ( j->second == p ) {
		setOutputFormat( j->first );
		goto found1;
	    }
	}
#if REP2FLAT
	if ( strcmp( p, "flat" ) == 0 ) {
	    setOutputFormat( File_Format::SRVN );
	    Flags::print[REPLICATION].opts.value.r = Replication::EXPAND;
	    goto found1;
	}
#endif
	std::cerr << LQIO::io_vars.lq_toolname << ": command not found." << std::endl;
	exit( 1 );
    found1: ;
    }

    return lqn2ps( argc, argv );
}



int
lqn2ps( int argc, char *argv[] )
{
    extern char *optarg;
    extern int optind;
    char * options;
    std::string output_file_name = "";

    sscanf( "$Date: 2021-12-07 18:33:05 -0500 (Tue, 07 Dec 2021) $", "%*s %s %*s", copyrightDate );

    static std::string opts = "";
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
	    for ( Options::Type * f = Flags::print; f->name; ++f ) {
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

	pragmas.insert( getenv( "LQNS_PRAGMAS" ) );

	switch( c ) {
	case 'A':
	    Flags::print[AGGREGATION].opts.value.a = Options::get_aggregate( optarg );
	    break;
	    
	case 512+'A':;
	    Flags::print[AGGREGATION].opts.value.a = Aggregate::ACTIVITIES;
	    Flags::print[PRINT_AGGREGATE].opts.value.b = true;
	    break;
	    
	case 'B':
	    Flags::print[BORDER].opts.value.d = strtod( optarg, &endptr );
	    if ( Flags::print[BORDER].opts.value.d < 0.0 || *endptr != '\0' ) {
		invalid_option( c, optarg );
		exit( 1 );
	    } 
	    break;

	case 0x300+'B':
	    pragmas.insert(LQIO::DOM::Pragma::_bcmp_,(enable ? LQIO::DOM::Pragma::_true_ : LQIO::DOM::Pragma::_false_));
	    break;
	    
	case 'C':
	    Flags::print[COLOUR].opts.value.c = Options::get_colouring( optarg );
	    if ( Flags::print[COLOUR].opts.value.c == Colouring::DIFFERENCES && Flags::print[PRECISION].opts.value.i == 3 ) {
		Flags::print[PRECISION].opts.value.i = 1;
	    }
	    break;

	case 512+'c':
	    Flags::print_comment = true;
	    break;

	case 512+'C':
	    Flags::use_colour = false;
	    break;

	case 'D':
	    Flags::print[COLOUR].opts.value.c = Colouring::DIFFERENCES;
	    if ( Flags::print[PRECISION].opts.value.i == 3 ) {
		Flags::print[PRECISION].opts.value.i = 2;
	    }
	    /* Fall through... */
	case 512+'p':
	    parse_file_name = optarg;
	    if ( strcmp( parse_file_name, "-" ) != 0 && access( parse_file_name, R_OK ) != 0 ) {
		std::cerr << LQIO::io_vars.lq_toolname << ": Cannot open parseable output file " << parse_file_name << " - "  
		     << strerror( errno ) << std::endl;
		exit ( 1 );
	    }
	    break;
	    
	case (512+'D'):
	    Flags::print_submodels = true;
	    break;

	case 512+'E':
	    Flags::print[IGNORE_ERRORS].opts.value.b = true;
	    break;
	    
	case 'F':
	    Flags::print[FONT_SIZE].opts.value.i = strtol( optarg, &endptr, 10 );
	    if ( *endptr != '\0' || Flags::print[FONT_SIZE].opts.value.i < min_fontsize || max_fontsize < Flags::print[FONT_SIZE].opts.value.i ) {
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
		
	case 512+'G':
	    Flags::print[RUN_LQX].opts.value.b 	= true;		    /* Run lqx */
	    Flags::dump_graphviz 		= true;
	    break;

	case 'H':
	    usage();
	    exit(0);

	case 512+'h':
	    Flags::print[PROCESSORS].opts.value.p = Processors::ALL;
	    Flags::print[LAYERING].opts.value.l = Layering::HWSW;
	    break;
	    
	case 512+'H':
	    /* Set immediately, as it can't be changed once the SPEX program is loaded */
            LQIO::Spex::__no_header = true;
	    break;

	case 'I':
	    try {
		Flags::print[INPUT_FORMAT].opts.value.f = Options::get_file_format( optarg );
	    }
	    catch ( const std::invalid_argument& e ) {
		invalid_option( c, optarg );
		exit( 1 );
	    }
	    break;

	case 512+'I':
	    Flags::print[INCLUDE_ONLY].opts.value.m = new std::regex( optarg );
	    break;

	case 'J':
	    options = optarg;
	    while ( *options ) {
		static char const * object[] = { "nodes", "labels", "activities" };

		char * value = nullptr;
		int arg = getsubopt( &options, const_cast<char **>(object), &value );
		Justification justify;

		if ( !value ) {
		    justify = Justification::DEFAULT;
		} else {
		    justify = Options::get_justification( value );
		}

		switch ( arg ) {
		case 0:	
		    if ( justify != Justification::ABOVE ) {
			Flags::node_justification = justify; 
		    } else {
			std::cerr << LQIO::io_vars.lq_toolname << ": -J" << optarg << "is invalid." << std::endl;
			exit( 1 );
		    }
		    break;   
		case 1:	
		    if ( justify != Justification::ALIGN ) {
			Flags::label_justification = justify; 
		    } else {
			std::cerr << LQIO::io_vars.lq_toolname << ": -J" << optarg << "is invalid." << std::endl;
			exit( 1 );
		    }
		    break;
		case 2:	
		    if ( justify != Justification::ABOVE ) {
			Flags::activity_justification = justify; 
		    } else {
			std::cerr << LQIO::io_vars.lq_toolname << ": -J" << optarg << "is invalid." << std::endl;
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
	    Flags::print[Y_SPACING].opts.value.d = 45;
	    break;

	case 512+'J':
	    LQIO::DOM::Document::__debugJSON = true;
	    break;

	case 'K':
	    Flags::print[KEY].opts.value.k = Options::get_key_position( optarg );
	    break;

	case 'k':
	    Flags::print[CHAIN].opts.value.i = strtol( optarg, &endptr, 10 );
	    if ( *endptr != '\0' || Flags::print[CHAIN].opts.value.i < 1 ) {
		invalid_option( c, optarg );
		exit( 1 );
	    }
	    Flags::print[PROCESSORS].opts.value.p = Processors::ALL;
	    break;

	case 'L':
	    try {
		Layering arg = Options::get_layering( optarg );
		Flags::print[LAYERING].opts.value.l = arg;

		switch ( arg ) {
		case Layering::HWSW:
		case Layering::MOL:
		case Layering::SQUASHED:
		case Layering::SRVN:
		    pragmas.insert(LQIO::DOM::Pragma::_layering_,Options::layering.at(arg));
		    Flags::print[PROCESSORS].opts.value.p = Processors::ALL;
		    break;

		case Layering::BATCH:
		    pragmas.insert(LQIO::DOM::Pragma::_layering_,Options::layering.at(arg));
		    break;

		    /* Non-pragma layering */
		case Layering::PROCESSOR_TASK:
		case Layering::TASK_PROCESSOR:
		    Flags::print[PROCESSORS].opts.value.p = Processors::ALL;
		    break;

		case Layering::PROCESSOR:
		case Layering::SHARE:
		    Flags::print[PROCESSORS].opts.value.p = Processors::NONE;
		    break;

		case Layering::GROUP:
		    if ( optarg ) {
			Model::add_group( optarg );
		    }
		    Flags::print[PROCESSORS].opts.value.p = Processors::ALL;
		    break;

		default:
		    invalid_option( c, optarg );
		    exit( 1 );
		}
	    }
	    catch ( const std::invalid_argument& e ) {
		invalid_option( c, optarg );
		exit( 1 );
	    }
	    break;

	case 512+'l':
	    Flags::print[RUN_LQX].opts.value.b 	= true;		    /* Run lqx */
	    break;

	case 512+'L':
	    LQIO::DOM::Document::lqx_parser_trace(stderr);
	    break;

	case 'M':
	    Flags::print[MAGNIFICATION].opts.value.d = strtod( optarg, &endptr );
	    if ( *endptr != '\0' || Flags::print[MAGNIFICATION].opts.value.d <= 0.0 ) {
		invalid_option( c, optarg );
		exit( 1 );
	    } 
	    break;

	case 512+'M':
	    man();
	    exit(0);

	case 512+'m':
	    Flags::print[PROCESSORS].opts.value.p = Processors::ALL;
	    Flags::print[LAYERING].opts.value.l = Layering::MOL;
	    break;
	    
	case 'N':
	    Flags::print[PRECISION].opts.value.i = strtol( optarg, &endptr, 10 );
	    if ( *endptr != '\0' || Flags::print[PRECISION].opts.value.i < 1 ) {
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
	    try {
		setOutputFormat( Options::get_file_format( optarg ) );
	    }
	    catch ( const std::invalid_argument& e ) {
		invalid_option( c, optarg );
		exit( 1 );
	    } 
	    break;

	case 512+'o':
	    Flags::print[OUTPUT_FORMAT].opts.value.f = File_Format::LQX;
	    setOutputFormat( File_Format::LQX );
	    break;
	    
	case 'P':
	    Flags::print[PROCESSORS].opts.value.p = Options::get_processors( optarg );
	    break;
	    
	case 512+'P':
//	    pragma( "tasks-only", "" );
	    Flags::print[AGGREGATION].opts.value.a = Aggregate::ENTRIES;
	    Flags::print[PRINT_AGGREGATE].opts.value.b = true;
	    break;

	case 'Q':
	    Flags::print[QUEUEING_MODEL].opts.value.i = strtol( optarg, &endptr, 10 );
	    if ( *endptr != '\0' || Flags::print[QUEUEING_MODEL].opts.value.i < 1 ) {
		invalid_option( c, optarg );
		exit( 1 );
	    }
	    break;

	case 'r':
	    Flags::print[RESULTS].opts.value.b = setAllResultOptions( enable );
	    break;

#if REP2FLAT
	case 'R':
	    Flags::print[REPLICATION].opts.value.r = Options::get_replication( optarg );
	    break;

	case 512+'R':
	    Flags::print[REPLICATION].opts.value.r = Replication::RETURN;
	    break;
#endif

	case 512+'r':
	    Flags::print[RUN_LQX].opts.value.b 		= true;		    /* Reload lqx */
	    Flags::print[RELOAD_LQX].opts.value.b	= true;
	    break;

	case 'S':
	    Flags::print[SUBMODEL].opts.value.i = strtol( optarg, &endptr, 10 );
	    if ( *endptr != '\0' || Flags::print[SUBMODEL].opts.value.i < 1 ) {
		invalid_option( c, optarg );
		exit( 1 );
	    }
	    Flags::print[PROCESSORS].opts.value.p = Processors::ALL;
	    break;

	case 512+'s':
	    Flags::sort = Sorting::NONE;
	    break;

	case 512+'t':
	    special( "tasks-only", "true", pragmas );
	    break;

	case 'V':
	    Flags::print[XX_VERSION].opts.value.b = true;
	    break;
	
	case 512+'S':
	case 512+'V':	/* Always set... :-) */
	    Flags::print[SUMMARY].opts.value.b = true;
	    break;

	case 512+'w':
	    Flags::print[PROCESSORS].opts.value.p = Processors::ALL;
	    Flags::print[LAYERING].opts.value.l = Layering::SRVN;
	    break;
	    
	case 'W':
	    LQIO::io_vars.severity_level = LQIO::ADVISORY_ONLY;		/* Ignore warnings. */
	    break;

	case 'X':
	    switch ( sscanf( optarg, "%lf,%lf", &Flags::print[X_SPACING].opts.value.d, &Flags::icon_width ) ) {
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
	    switch ( sscanf( optarg, "%lf,%lf", &Flags::print[Y_SPACING].opts.value.d, &Flags::icon_height ) ) {
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
	    if ( !process_special( optarg, pragmas ) ) {
		exit( 1 );
	    }
	    break;

	case 512+'z':
	    Flags::surrogates = false;
	    break;

	case 0x300+'z':
	    Flags::surrogates = enable;
	    break;

	case 512+'Z':
	    resultdebug = true;
	    break;

	case 0x300+'#':
	    Flags::print[MODEL_COMMENT].opts.value.b = enable;
	    break;
	    
	case 512+'!':
	    Flags::print[SOLVER_INFO].opts.value.b = enable;
	    break;
	    
	default:
	    for ( int i = 0; Flags::print[i].name != 0; ++i ) {
		if ( !Flags::print[i].arg && c == Flags::print[i].c ) {
		    if ( enable ) {
			Flags::print[i].opts.value.b = true;
			if ( Flags::print[i].result ) {
			    Flags::print[RESULTS].opts.value.b = true;	/* Enable results */
			}
		    } else {
			Flags::print[i].opts.value.b = false;
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
    LQIO::io_vars.lq_command_line = command_line.c_str();

    if ( Flags::print[XX_VERSION].opts.value.b ) {
	std::cout << "Layered Queueing Network file conversion program, Version " << VERSION << std::endl << std::endl;
	std::cout << "  Copyright " << copyrightDate << " the Real-Time and Distributed Systems Group," << std::endl;
	std::cout << "  Department of Systems and Computer Engineering," << std::endl;
	std::cout << "  Carleton University, Ottawa, Ontario, Canada. K1S 5B6" << std::endl << std::endl;
    }
	
    /* Check for sensible combinations of options. */

    if ( Flags::bcmp_model ) {
	Flags::surrogates = false;					/* Never add surrogates */
    }

    if ( Flags::annotate_input && !input_output() ) {
	std::cerr << LQIO::io_vars.lq_toolname << ": -Z " << Options::special.at(Special::ANNOTATE)
		  << " and " << Options::file_format.at(Flags::print[OUTPUT_FORMAT].opts.value.f)
		  << " output are mutually exclusive." << std::endl;
	Flags::annotate_input = false;
    }

    if ( Flags::print[AGGREGATION].opts.value.a == Aggregate::ENTRIES && !(graphical_output() || queueing_output()) ) {
	std::cerr << LQIO::io_vars.lq_toolname << ": -Z" << Options::special.at(Special::TASKS_ONLY)
		  << " and " <<  Options::file_format.at(Flags::print[OUTPUT_FORMAT].opts.value.f)
		  << " output are mutually exclusive." << std::endl;
	exit( 1 );
    }

    if ( Flags::print[INCLUDE_ONLY].opts.value.m != nullptr && submodel_output() ) {
	std::cerr << LQIO::io_vars.lq_toolname << ": -I<regexp> "
	     << "and -S" <<  Flags::print[SUBMODEL].opts.value.i 
	     << " are mutually exclusive." << std::endl;
	exit( 1 );
    }

    if ( submodel_output() && Flags::print_submodels ) {
	std::cerr << LQIO::io_vars.lq_toolname << ": -S" << Flags::print[SUBMODEL].opts.value.i
	     << " and --debug-submodels are mutually exclusive." << std::endl;
	Flags::print_submodels = false;
    }

    if ( queueing_output() ) {
	Flags::arrow_scaling *= 0.75;
//	Flags::print[PROCESSORS].opts.value.p = Processors::ALL;

	if ( submodel_output() ) {
	    std::cerr << LQIO::io_vars.lq_toolname << ": -Q" << Flags::print[QUEUEING_MODEL].opts.value.i
		 << "and -S" <<  Flags::print[SUBMODEL].opts.value.i 
		 << " are mutually exclusive." << std::endl;
	    exit( 1 );
	} else if ( !graphical_output() 
#if QNAP2_OUTPUT
		    && Flags::print[OUTPUT_FORMAT].opts.value.f != File_Format::QNAP2
#endif
#if JMVA_OUTPUT && HAVE_EXPAT_H
		    && Flags::print[OUTPUT_FORMAT].opts.value.f != File_Format::JMVA
#endif
		    && Flags::print[OUTPUT_FORMAT].opts.value.f != File_Format::LQX
		    && Flags::print[OUTPUT_FORMAT].opts.value.f != File_Format::XML
		    && Flags::print[OUTPUT_FORMAT].opts.value.f != File_Format::JSON
	    ) {
	    std::cerr << LQIO::io_vars.lq_toolname << ": -Q" << Flags::print[QUEUEING_MODEL].opts.value.i
		      << " and " << Options::file_format.at(Flags::print[OUTPUT_FORMAT].opts.value.f)
		      << " output are mutually exclusive." << std::endl;
	    exit( 1 );
	} else if ( Flags::print[AGGREGATION].opts.value.a != Aggregate::ENTRIES && !graphical_output() ) {
	    Flags::print[AGGREGATION].opts.value.a = Aggregate::ENTRIES;
	    std::cerr << LQIO::io_vars.lq_toolname << ": aggregating entries to tasks with " 
		      << Options::file_format.at(Flags::print[OUTPUT_FORMAT].opts.value.f) << " output." << std::endl;
	}
#if QNAP2_OUTPUT
	if ( Flags::print[OUTPUT_FORMAT].opts.value.f == File_Format::QNAP2 ) {
	    Flags::squish_names	= true;
	}
#endif
    }

    if ( Flags::print[OUTPUT_FORMAT].opts.value.f == File_Format::SRVN && !partial_output() ) {
	Flags::print[RESULTS].opts.value.b = false;	/* Ignore results */
    }

    if ( Flags::flatten_submodel && !(submodel_output() || queueing_output()) ) {
	std::cerr << LQIO::io_vars.lq_toolname << ": -Z" << Options::special.at(Special::FLATTEN_SUBMODEL)
	     << " can only be used with either -Q<n> -S<n>." << std::endl;
	exit( 1 );
    }

    if ( submodel_output() && Flags::print[LAYERING].opts.value.l == Layering::SQUASHED ) {
	std::cerr << LQIO::io_vars.lq_toolname << ": -L" << Options::layering.at(Layering::SQUASHED)
	     << " can only be used with full models." << std::endl;
	exit( 1 );
    }

    if ( Flags::print[PROCESSORS].opts.value.p == Processors::NONE 
	 || Flags::print[LAYERING].opts.value.l == Layering::PROCESSOR
	 || Flags::print[LAYERING].opts.value.l == Layering::SHARE ) {
	Flags::print[PROCESSOR_QUEUEING].opts.value.b = false;
    }

    if ( Flags::bcmp_model ) {
	Flags::print[AGGREGATION].opts.value.a = Aggregate::ENTRIES;
    }

    /*
     * Change font size because scaleBy doesn't -- Fig doesn't use points for it's coordinates, 
     * but it does use points for it's labels.
     */

    Flags::print[FONT_SIZE].opts.value.i = (int)(Flags::print[FONT_SIZE].opts.value.i * Flags::print[MAGNIFICATION].opts.value.d + 0.5);

#if HAVE_GD_H && HAVE_LIBGD
    switch( Flags::print[OUTPUT_FORMAT].opts.value.f ) {
#if HAVE_GDIMAGEGIFPTR
    case File_Format::GIF:
#endif
#if HAVE_LIBJPEG
    case File_Format::JPEG:
#endif
#if HAVE_LIBPNG
    case File_Format::PNG:
#endif
	GD::testForTTF();	/* Font selection changes */
	break;
    }
#endif

    /* If stdout is not a terminal For pipelines.	*/

#if !defined(__WINNT__) && !defined(MSDOS)
    if ( output_file_name == "" && LQIO::Filename::isWriteableFile( fileno( stdout ) ) > 0 ) {
	output_file_name = "-";
    }
#endif

    if ( output_file_name == "-" ) {
	switch( Flags::print[OUTPUT_FORMAT].opts.value.f ) {
#if defined(EMF_OUTPUT)
	case File_Format::EMF:
	    if ( LQIO::Filename::isRegularFile( fileno( stdout ) ) == 0 ) {
		std::cerr << LQIO::io_vars.lq_toolname << ": Cannot write " 
			  << Options::file_format.at(Flags::print[OUTPUT_FORMAT].opts.value.f) 
			  << " to stdout - stdout is not a regular file."  << std::endl;
		exit( 1 );
	    }
	    break;
#endif
#if defined(SXD_OUTPUT)
	case File_Format::SXD:
	    std::cerr << LQIO::io_vars.lq_toolname << ": Cannot write " 
		      << Options::file_format.at(Flags::print[OUTPUT_FORMAT].opts.value.f) 
		      << " to stdout."  << std::endl;
	    exit( 1 );
	    break;
#endif
	}
    }

    if ( optind == argc ) {
	Model::create( "-", output_file_name, parse_file_name, 1 );
    } else {
	for ( int i = 1; optind < argc; ++optind, ++i ) {
	    Model::create( argv[optind], output_file_name, parse_file_name, i );
	}
    }

    if ( Flags::print[INCLUDE_ONLY].opts.value.m != nullptr ) {
	delete Flags::print[INCLUDE_ONLY].opts.value.m;
    }
    if ( Flags::client_tasks ) {
	delete Flags::client_tasks;
    }

#if HAVE_GETOPT_H
    for ( std::vector<struct option>::iterator opt = longopts.begin(); opt != longopts.end(); ++opt ) {
	if ( opt->name != nullptr ) free( const_cast<char *>(opt->name) );
    }
#endif
    return 0;
}

#if HAVE_GETOPT_H
static void
makeopts( std::string& opts, std::vector<struct option>& longopts ) 
{
    unsigned int k = 0;
    struct option opt;
    opt.flag = 0;
    opt.val  = 0;
    opt.name = 0;
    opt.has_arg = 0;
    longopts.resize( Flags::size * 2, opt );
    for ( unsigned int i = 0; i < Flags::size; ++i, ++k ) {
	longopts[k].has_arg = (Flags::print[i].arg != 0 ? required_argument : no_argument);
	longopts[k].name    = strdup(Flags::print[i].name);
	longopts[k].val     = Flags::print[i].c;
	if ( (Flags::print[i].c & ~0x7f) == 0 && std::islower( Flags::print[i].c ) && Flags::print[i].arg == 0 ) {
	    /* These are the +/- options */
	    longopts[k].val |= 0x0100;					/* + case is > 256 */
	    k += 1;
	    std::string name = "no-";					/* - case is no-<name> */
	    name += Flags::print[i].name;
	    longopts[k].name = strdup( name.c_str() );			/* Make a copy */
	    longopts[k].val = Flags::print[i].c;
	} else if ( (Flags::print[i].c & 0xff00) == 0x0300 ) {
	    /* These are the +/- options */
	    k += 1;
	    std::string name = "no-";					/* - case is no-<name> */
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
    for ( unsigned int i = 0; Flags::print[i].name != nullptr; ++i ) {
	opts += static_cast<char>(Flags::print[i].c);
	if ( Flags::print[i].arg ) {
	    opts += ':';
	}
    }
}
#endif


void
setOutputFormat( const File_Format f ) 
{
    Flags::print[OUTPUT_FORMAT].opts.value.f = f;

    switch ( f ) {
    case File_Format::OUTPUT:
    case File_Format::PARSEABLE:
    case File_Format::RTF:
	Flags::print[PRECISION].opts.value.i = 7;			/* Increase default precision */
	Flags::print[INPUT_PARAMETERS].opts.value.b = true;     	/* input parameters. */
	Flags::print[CONFIDENCE_INTERVALS].opts.value.b = true; 	/* Confidence Intervals */
	setAllResultOptions( true );
	/* Fall through */
    case File_Format::JSON:
    case File_Format::LQX:
    case File_Format::XML:
    case File_Format::SRVN:
	Flags::print[PROCESSORS].opts.value.p = Processors::ALL;  	/* Print all processors. */
	Flags::print[LAYERING].opts.value.l = Layering::PROCESSOR;	/* Order by processors */
	Flags::surrogates = true;					/* Always add surrogates */
	break;

    case File_Format::NO_OUTPUT:
	break;

#if JMVA_OUTPUT && HAVE_EXPAT_H
    case File_Format::JMVA:
	Flags::bcmp_model = true;					/* No entries. */
	break;
#endif
#if QNAP2_OUTPUT
    case File_Format::QNAP2:
#warning .. need to separate by class and station
	Flags::squish_names = true;					/* Always */ 
	Flags::bcmp_model = true;					/* No entries. */
	break;
#endif
	
#if defined(X11_OUTPUT)
    case File_Format::X11:
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
	    Flags::print[i].opts.value.b = yesOrNo;     /* Print entry throughput. */
	}
    }

    return yesOrNo;
}
