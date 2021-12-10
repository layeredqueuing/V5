/*  -*- c++ -*-
 * $Id: lqn2ps.cc 15186 2021-12-10 00:32:18Z greg $
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

static std::string parse_file_name = "";

#if (defined(linux) || defined(__linux__)) && !defined(__USE_XOPEN_EXTENDED)
extern "C" int getsubopt (char **, char * const *, char **);
#endif

/*
 * Sort on field 5.  Don't forget to update lqn2ps.h if you
 * add/delete lines!  Value is interpreted as an INTEGER so initialize
 * FLOATS at run time.  Set result to true for flags controlling
 * result output and false otherwise.
 */

std::vector<Options::Type> Flags::print = {
/*    name                      c   arg                    type                     (init) value        result msg */
    { "aggregate",             'A', "objects",             {&Options::aggregate,    Aggregate::NONE},   "Aggregate sequences,activities,phases,entries,threads,all into parent object." },
    { "border",                'B', "border",              {&Options::real,         18.0},              "Set the border (in points)." },
    { "colour",                'C', "colour",              {&Options::colouring,    Colouring::RESULTS},"Colour output." },
    { "diff-file",             'D', "filename",            {&Options::none,         0},                 "Load parseable results generated using srvndiff --difference from filename." },
    { "font-size",             'F', "font-size",           {&Options::integer,      9},                 "Set the font size (from 6 to 36 points)." },
    { "input-format",          'I', "ARG",                 {&Options::file_format,  File_Format::UNKNOWN}, "Input file format." },
    { "help",                  'H', nullptr,               {&Options::none,         0},                 "Print help." },
    { "justification",         'J', "object=ARG",          {&Options::justification,Justification::DEFAULT},   "Justification." },
    { "key",                   'K', "key",                 {&Options::key_position, 0},                 "Print key." },
    { "layering",              'L', "layering",            {&Options::layering,     Layering::BATCH},   "Layering." },
    { "magnification",         'M', "magnification",       {&Options::real,         1.0},               "Magnification." },
    { "precision",             'N', "precision",           {&Options::integer,      3},                 "Number of digits of output precision." },
    { "format",                'O', "format",              {&Options::file_format,  File_Format::UNKNOWN}, "Output file format." },
    { "processors",            'P', "processors",          {&Options::processors,   Processors::DEFAULT}, "Print processors." },
    { "queueing-model",        'Q', "queueing-model",      {&Options::integer,      0},                 "Print queueing model <n>." },
#if REP2FLAT
    { "replication",           'R', "ARG",                 {&Options::replication,  Replication::NONE}, "Transform replication." },
#endif
    { "submodel",              'S', "submodel",            {&Options::integer,      0},                 "Print submodel <n>." },
    { "version",               'V', nullptr,               {&Options::boolean,      false},             "Tool version." },
    { "warnings",              'W', nullptr,               {&Options::boolean,      false},             "Suppress warnings." },
    { "x-spacing",             'X', "spacing[,width]",     {&Options::real,         DEFAULT_X_SPACING}, "X spacing [and task width] (points)." },
    { "y-spacing",             'Y', "spacing[,height]",    {&Options::real,         DEFAULT_Y_SPACING}, "Y spacing [and task height] (points)." },
    { "special",               'Z', "ARG[=value]",         {&Options::special,      Special::NONE},     "Special option." },
    { "open-wait",             'a', nullptr,               {&Options::result,       true},              "Print queue length results for open arrivals." },
    { "throughput-bounds",     'b', nullptr,               {&Options::result,       false},             "Print task throughput bounds." },
    { "confidence-intervals",  'c', nullptr,               {&Options::result,       false},             "Print confidence intervals." },
    { "entry-utilization",     'e', nullptr,               {&Options::result,       false},             "Print entry utilization." },
    { "entry-throughput",      'f', nullptr,               {&Options::result,       false},             "Print entry throughput." },
    { "histograms",            'g', nullptr,               {&Options::result,       false},             "Print histograms." },
    { "hold-times",            'h', nullptr,               {&Options::result,       false},             "Print hold times." },
    { "input-parameters",      'i', nullptr,               {&Options::boolean,      true},              "Print input parameters." },
    { "join-delays",           'j', nullptr,               {&Options::result,       true},              "Print join delay results." },
    { "chain",                 'k', "client",              {&Options::integer,      0},                 "Print all paths from client <n>." },
    { "loss-probability",      'l', nullptr,               {&Options::result,       true},              "Print message loss probabilities." },
    { "output",                'o', "filename",            {&Options::string,       static_cast<std::string *>(nullptr)},       "Redirect output to filename." },
    { "processor-utilization", 'p', nullptr,               {&Options::result,       true},              "Print processor utilization results." },
    { "processor-queueing",    'q', nullptr,               {&Options::result,       true},              "Print processor waiting time results." },
    { "results",               'r', nullptr,               {&Options::result,       true},              "Print results." },
    { "service",               's', nullptr,               {&Options::result,       true},              "Print execution time results." },
    { "task-throughput",       't', nullptr,               {&Options::result,       true},              "Print task throughput results." },
    { "task-utilization",      'u', nullptr,               {&Options::result,       true},              "Print task utilization results." },
    { "variance",              'v', nullptr,               {&Options::result,       false},             "Print execution time variance results." },
    { "waiting",               'w', nullptr,               {&Options::result,       true},              "Print waiting time results." },
    { "service-exceeded",      'x', nullptr,               {&Options::result,       false},             "Print maximum execution time exceeded." },
    { "comment",         0x300+'#', nullptr,               {&Options::result,       false},             "Print model comment." },
    { "solver-info",     0x300+'!', nullptr,               {&Options::none,         0},                 "Print solver information." },
    { "verbose",         0x200+'V', nullptr,               {&Options::none,         0},                 "Verbose output." },
    { "ignore-errors",   0x200+'E', nullptr,               {&Options::none,         0},                 "Ignore errors during model checking phase." },
    { "task-service-time", 512+'P', nullptr,               {&Options::none,         0},                 "Print task service times (for --tasks-only)." },
    { "run-lqx",         0x200+'l', nullptr,               {&Options::none,         0},                 "\"Run\" the LQX program instantiating variables and generating model files." },
    { "reload-lqx",      0x200+'r', nullptr,               {&Options::none,         0},                 "\"Run\" the LQX program reloading results generated earlier." },
    { "output-lqx",      0x200+'o', nullptr,               {&Options::none,         0},                 "Convert SPEX to LQX for XML output." },
    { "include-only",    0x200+'I', "regexp",              {&Options::string,       static_cast<std::string *>(nullptr)},       "Include only objects with name matching <regexp>" },

    /* -- below here is not stored in flag_values enumeration -- */

    /* Layering shortcuts */
#if 0
    { "client-layering", 0x200+'x', nullptr,               {&Options::none,         0},                 "NEW LAYERING STRATEGEY (EXPERIMENTAL)." },
#endif
    { "hwsw-layering",   0x200+'h', nullptr,               {&Options::none,         0},                 "Use HW/SW layering instead of batched layering." },
    { "srvn-layering",   0x200+'w', nullptr,               {&Options::none,         0},                 "Use SRVN layering instead of batched layering." },
    { "method-of-layers",0x200+'m', nullptr,               {&Options::none,         0},                 "Use the Method Of Layers instead of batched layering." },
    /* Special shortcuts */
    { "flatten",         0x200+'f', nullptr,               {&Options::none,         0},                 "Flatten submodel/queueing output by placing clients in one layer." },
    { "no-sort",         0x200+'s', nullptr,               {&Options::none,         0},                 "Do not sort objects for output." },
    { "number-layers",   0x200+'n', nullptr,               {&Options::none,         0},                 "Print layer numbers." },
    { "rename",          0x200+'N', nullptr,               {&Options::none,         0},                 "Rename all objects." },
    { "tasks-only",      0x200+'t', nullptr,               {&Options::none,         0},                 "Print tasks only." },
#if BUG_270
    { "bcmp",            0x300+'B', nullptr,               {&Options::boolean,      false},             "[Don't] perform BCMP model conversion." },
#endif
    /* Miscellaneous */
    { "no-activities",   0x200+'A', nullptr,               {&Options::none,         0},                 "Don't print activities." },
    { "no-colour",       0x200+'C', nullptr,               {&Options::none,         0},                 "Use grey scale when colouring result." },
    { "no-header",       0x200+'H', nullptr,               {&Options::none,         0},                 "Do not output the variable name header on SPEX results." },
    { "surrogates",      0x300+'z', nullptr,               {&Options::none,         false},             "[Don't] add surrogate tasks for submodel/include-only output." },
#if REP2FLAT
    { "merge-replicas",  0x200+'R', nullptr,               {&Options::none,         0},                 "Merge replicas from a flattened model back to a replicated model." },
#endif
    { "jlqndef",         0x200+'j', nullptr,               {&Options::none,         0},                 "Use jlqnDef-style icons (rectangles)." },
    { "parse-file",      0x200+'p', "filename",            {&Options::none,         0},                 "Load parseable results from filename." },
    { "print-comment",   0x200+'c', nullptr,               {&Options::none,         0},                 "Print the model comment on stdout." },
    { "print-submodels", 0x200+'D', nullptr,               {&Options::none,         0},                 "Show submodels." },
    { "print-summary",   0x200+'S', nullptr,               {&Options::none,         0},                 "Print model summary on stdout." },
    { "debug-json",      0x200+'J', nullptr,               {&Options::none,         0},                 "Output debugging information while parsing JSON input." },
    { "debug-lqx",       0x200+'L', nullptr,               {&Options::none,         0},                 "Output debugging information while parsing LQX input." },
    { "debug-srvn",      0x200+'Y', nullptr,               {&Options::none,         0},                 "Output debugging information while parsing SRVN input." },
    { "debug-p",         0x200+'Z', nullptr,               {&Options::none,         0},                 "Output debugging information while parsing parseable results input." },
    { "debug-xml",       0x200+'X', nullptr,               {&Options::none,         0},                 "Output debugging information while parsing XML input." },
    { "debug-formatting",0x200+'F', nullptr,               {&Options::none,         0},                 "Output debugging information while formatting." },
    { "dump-graphviz",   0x200+'G', nullptr,               {&Options::none,         0},                 "Output LQX parse tree in graphviz format." },
    { "generate-manual", 0x200+'M', nullptr,               {&Options::none,         0},                 "Generate manual suitable for input to man(1)." }
};

#if HAVE_GETOPT_H
static void makeopts( std::string& opts, std::vector<struct option>& longopts );
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

    Flags::set_magnification(1.0);
    Flags::set_border(18.0);
    Flags::set_x_spacing(DEFAULT_X_SPACING);
    Flags::set_y_spacing(DEFAULT_Y_SPACING);

    LQIO::io_vars.init( VERSION, basename( argv[0] ), severity_action, local_error_messages, LSTLCLERRMSG-LQIO::LSTGBLERRMSG );

    command_line += LQIO::io_vars.lq_toolname;

    /* If we are invoked as lqn2xxx or rep2flat, then enable other options. */

    const char * p = strrchr( LQIO::io_vars.toolname(), '2' );
    if ( p != nullptr ) {
	p += 1;
	try {
	    setOutputFormat( Options::get_file_format( p ) );		// Throws if ext not found
	}
	catch ( std::invalid_argument& e ) {
#if REP2FLAT
	    if ( strcmp( p, "flat" ) == 0 ) {
		Flags::set_replication( Replication::EXPAND );
	    } else {
#endif
		std::cerr << LQIO::io_vars.lq_toolname << ": command not found." << std::endl;
		exit( 1 );
#if REP2FLAT
	    }
#endif
	}
    }

    extern char *optarg;
    extern int optind;
    char * options;
    std::string output_file_name = "";

    sscanf( "$Date: 2021-12-09 19:32:18 -0500 (Thu, 09 Dec 2021) $", "%*s %s %*s", copyrightDate );

    static std::string opts = "";
#if HAVE_GETOPT_H
    static std::vector<struct option> longopts;
    makeopts( opts, longopts );
#else
    makeopts( opts );
#endif

    for ( ;; ) {
	char * endptr = nullptr;

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
	    for ( std::vector<Options::Type>::const_iterator f = Flags::print.begin(); f != Flags::print.end(); ++f ) {
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
	    Flags::set_aggregation( Options::get_aggregate( optarg ) );
	    break;

	case 0x200+'A':;
	    Flags::set_aggregation( Aggregate::ACTIVITIES );
	    Flags::print[PRINT_AGGREGATE].opts.value.b = true;
	    break;

	case 'B':
	    Flags::set_border( strtod( optarg, &endptr ) );
	    if ( Flags::border() < 0.0 || *endptr != '\0' ) {
		invalid_option( c, optarg );
		exit( 1 );
	    }
	    break;

	case 0x300+'B':
	    pragmas.insert(LQIO::DOM::Pragma::_bcmp_,(enable ? LQIO::DOM::Pragma::_true_ : LQIO::DOM::Pragma::_false_));
	    break;

	case 'C':
	    Flags::set_colouring( Options::get_colouring( optarg ) );
	    if ( Flags::colouring() == Colouring::DIFFERENCES && Flags::precision() == 3 ) {
		Flags::set_precision(1);
	    }
	    break;

	case 0x200+'c':
	    Flags::print_comment = true;
	    break;

	case 0x200+'C':
	    Flags::use_colour = false;
	    break;

	case 'D':
	    Flags::set_colouring( Colouring::DIFFERENCES );
	    if ( Flags::precision() == 3 ) {
		Flags::set_precision(2);
	    }
	    /* Fall through... */
	case 0x200+'p':
	    parse_file_name = optarg;
	    if ( parse_file_name != "-" && access( parse_file_name.c_str(), R_OK ) != 0 ) {
		std::cerr << LQIO::io_vars.lq_toolname << ": Cannot open parseable output file " << parse_file_name << " - "
			  << strerror( errno ) << std::endl;
		exit ( 1 );
	    }
	    break;

	case (0x200+'D'):
	    Flags::print_submodels = true;
	    break;

	case 0x200+'E':
	    Flags::set_ignore_errors( true );
	    break;

	case 'F':
	    Flags::set_font_size( strtoul( optarg, &endptr, 10 ) );
	    if ( *endptr != '\0' || Flags::font_size() < min_fontsize || max_fontsize < Flags::font_size() ) {
		invalid_option( c, optarg );
		exit( 1 );
	    }
	    break;

	case 0x200+'f':
	    Flags::flatten_submodel = true;
	    break;

	case 0x200+'F':
	    Flags::debug = true;
	    break;

	case 0x200+'G':
	    Flags::set_run_lqx( true );		    /* Run lqx */
	    Flags::dump_graphviz 		= true;
	    break;

	case 'H':
	    usage();
	    exit(0);

	case 0x200+'h':
	    Flags::set_processors( Processors::ALL );
	    Flags::set_layering( Layering::HWSW );
	    break;

	case 0x200+'H':
	    /* Set immediately, as it can't be changed once the SPEX program is loaded */
            LQIO::Spex::__no_header = true;
	    break;

	case 'I':
	    try {
		Flags::set_input_format( Options::get_file_format( optarg ) );
	    }
	    catch ( const std::invalid_argument& e ) {
		invalid_option( c, optarg );
		exit( 1 );
	    }
	    break;

	case 0x200+'I':
	    Flags::set_include_only( new std::regex( optarg ) );
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

	case 0x200+'j':
	    Flags::graphical_output_style = Output_Style::JLQNDEF;
	    Flags::icon_slope = 0;
	    Flags::set_y_spacing(45);
	    break;

	case 0x200+'J':
	    LQIO::DOM::Document::__debugJSON = true;
	    break;

	case 'K':
	    Flags::set_key_position( Options::get_key_position( optarg ) );
	    break;

	case 'k':
	    Flags::set_chain( strtoul( optarg, &endptr, 10 ) );
	    if ( *endptr != '\0' || Flags::chain() < 1 ) {
		invalid_option( c, optarg );
		exit( 1 );
	    }
	    Flags::set_processors( Processors::ALL );
	    break;

	case 'L':
	    try {
		Layering l = Flags::set_layering( Options::get_layering( optarg ) );
		switch ( l ) {
		case Layering::HWSW:
		case Layering::MOL:
		case Layering::SQUASHED:
		case Layering::SRVN:
		    pragmas.insert(LQIO::DOM::Pragma::_layering_,Options::layering.at(l));
		    Flags::set_processors( Processors::ALL );
		    break;

		case Layering::BATCH:
		    pragmas.insert(LQIO::DOM::Pragma::_layering_,Options::layering.at(l));
		    break;

		    /* Non-pragma layering */
		case Layering::PROCESSOR_TASK:
		case Layering::TASK_PROCESSOR:
		    Flags::set_processors( Processors::ALL );
		    break;

		case Layering::PROCESSOR:
		case Layering::SHARE:
		    Flags::set_processors( Processors::NONE );
		    break;

		case Layering::GROUP:
		    if ( optarg ) {
			Model::add_group( optarg );
		    }
		    Flags::set_processors( Processors::ALL );
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

	case 0x200+'l':
	    Flags::set_run_lqx( true );		    /* Run lqx */
	    break;

	case 0x200+'L':
	    LQIO::DOM::Document::lqx_parser_trace(stderr);
	    break;

	case 'M':
	    Flags::set_magnification( strtod( optarg, &endptr ) );
	    if ( *endptr != '\0' || Flags::magnification() <= 0.0 ) {
		invalid_option( c, optarg );
		exit( 1 );
	    }
	    break;

	case 0x200+'M':
	    man();
	    exit(0);

	case 0x200+'m':
	    Flags::set_processors( Processors::ALL );
	    Flags::set_layering( Layering::MOL );
	    break;

	case 'N':
	    Flags::set_precision( strtoul( optarg, &endptr, 10 ) );
	    if ( *endptr != '\0' || Flags::precision() < 1 ) {
		invalid_option( c, optarg );
		exit( 1 );
	    }
	    break;

	case (0x200+'n'):
	    Flags::print_layer_number 		= true;
	    break;

	case (0x200+'N'):
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

	case 0x200+'o':
	    setOutputFormat( File_Format::LQX );
	    break;

	case 'P':
	    Flags::set_processors( Options::get_processors( optarg ) );
	    break;

	case 0x200+'P':
//	    pragma( "tasks-only", "" );
	    Flags::set_aggregation( Aggregate::ENTRIES );
	    Flags::print[PRINT_AGGREGATE].opts.value.b = true;
	    break;

	case 'Q':
	    Flags::set_queueing_model( strtoul( optarg, &endptr, 10 ) );
	    if ( *endptr != '\0' || Flags::queueing_model() < 1 ) {
		invalid_option( c, optarg );
		exit( 1 );
	    }
	    break;

	case 'r':
	    Flags::set_print_results( Options::set_all_result_options( enable ) );
	    break;

#if REP2FLAT
	case 'R':
	    Flags::set_replication( Options::get_replication( optarg ) );
	    break;

	case 0x200+'R':
	    Flags::set_replication( Replication::RETURN );
	    break;
#endif

	case 0x200+'r':
	    Flags::set_run_lqx( true );		    /* Reload lqx */
	    Flags::print[RELOAD_LQX].opts.value.b	= true;
	    break;

	case 'S':
	    Flags::set_submodel( strtoul( optarg, &endptr, 10 ) );
	    if ( *endptr != '\0' || Flags::submodel() < 1 ) {
		invalid_option( c, optarg );
		exit( 1 );
	    }
	    Flags::set_processors( Processors::ALL );
	    break;

	case 0x200+'s':
	    Flags::sort = Sorting::NONE;
	    break;

	case 0x200+'t':
	    special( "tasks-only", "true", pragmas );
	    break;

	case 'V':
	    Flags::print[XX_VERSION].opts.value.b = true;
	    break;

	case 0x200+'S':
	case 0x200+'V':	/* Always set... :-) */
	    Flags::print[SUMMARY].opts.value.b = true;
	    break;

	case 0x200+'w':
	    Flags::set_processors( Processors::ALL );
	    Flags::set_layering( Layering::SRVN );
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

        case 0x200+'X':
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

	case 0x200+'Y':
	    LQIO_debug = true;
	    break;

	case 'Z':	/* Always set... :-) */
	    if ( !process_special( optarg, pragmas ) ) {
		exit( 1 );
	    }
	    break;

	case 0x200+'z':
	    Flags::surrogates = false;
	    break;

	case 0x300+'z':
	    Flags::surrogates = enable;
	    break;

	case 0x200+'Z':
	    resultdebug = true;
	    break;

	case 0x300+'#':
	    Flags::print[MODEL_COMMENT].opts.value.b = enable;
	    break;

	case 0x300+'!':
	    Flags::print[SOLVER_INFO].opts.value.b = enable;
	    break;

	default:
	    /* If not in the above list, try the table */
	    std::vector<Options::Type>::iterator f = std::find_if( Flags::print.begin(), Flags::print.end(), Options::find_option( c ) );
	    if ( f != Flags::print.end() ) {
		f->opts.value.b = enable;
	    } else {
		usage( false );
		exit( 1 );
	    }
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
		  << " and " << Options::file_format.at(Flags::output_format())
		  << " output are mutually exclusive." << std::endl;
	Flags::annotate_input = false;
    }

    if ( Flags::aggregation() == Aggregate::ENTRIES && !(graphical_output() || queueing_output()) ) {
	std::cerr << LQIO::io_vars.lq_toolname << ": -Z" << Options::special.at(Special::TASKS_ONLY)
		  << " and " <<  Options::file_format.at(Flags::output_format())
		  << " output are mutually exclusive." << std::endl;
	exit( 1 );
    }

    if ( Flags::include_only() != nullptr && submodel_output() ) {
	std::cerr << LQIO::io_vars.lq_toolname << ": -I<regexp> "
		  << "and -S" <<  Flags::submodel()
		  << " are mutually exclusive." << std::endl;
	exit( 1 );
    }

    if ( submodel_output() && Flags::print_submodels ) {
	std::cerr << LQIO::io_vars.lq_toolname << ": -S" << Flags::submodel()
		  << " and --debug-submodels are mutually exclusive." << std::endl;
	Flags::print_submodels = false;
    }

    if ( queueing_output() ) {
	Flags::arrow_scaling *= 0.75;
//	Flags::print[PROCESSORS].opts.value.p = Processors::ALL;

	if ( submodel_output() ) {
	    std::cerr << LQIO::io_vars.lq_toolname << ": -Q" << Flags::queueing_model()
		      << "and -S" <<  Flags::submodel()
		      << " are mutually exclusive." << std::endl;
	    exit( 1 );
	} else if ( !graphical_output()
#if QNAP2_OUTPUT
		    && Flags::output_format() != File_Format::QNAP2
#endif
#if JMVA_OUTPUT && HAVE_EXPAT_H
		    && Flags::output_format() != File_Format::JMVA
#endif
		    && Flags::output_format() != File_Format::LQX
		    && Flags::output_format() != File_Format::XML
		    && Flags::output_format() != File_Format::JSON
	    ) {
	    std::cerr << LQIO::io_vars.lq_toolname << ": -Q" << Flags::queueing_model()
		      << " and " << Options::file_format.at(Flags::output_format())
		      << " output are mutually exclusive." << std::endl;
	    exit( 1 );
	} else if ( Flags::aggregation() != Aggregate::ENTRIES && !graphical_output() ) {
	    Flags::set_aggregation( Aggregate::ENTRIES );
	    std::cerr << LQIO::io_vars.lq_toolname << ": aggregating entries to tasks with "
		      << Options::file_format.at(Flags::output_format()) << " output." << std::endl;
	}
#if QNAP2_OUTPUT
	if ( Flags::output_format() == File_Format::QNAP2 ) {
	    Flags::squish_names	= true;
	}
#endif
    }

    if ( Flags::output_format() == File_Format::SRVN && !partial_output() ) {
	Flags::set_print_results( false );	/* Ignore results */
    }

    if ( Flags::flatten_submodel && !(submodel_output() || queueing_output()) ) {
	std::cerr << LQIO::io_vars.lq_toolname << ": -Z" << Options::special.at(Special::FLATTEN_SUBMODEL)
		  << " can only be used with either -Q<n> -S<n>." << std::endl;
	exit( 1 );
    }

    if ( submodel_output() && Flags::layering() == Layering::SQUASHED ) {
	std::cerr << LQIO::io_vars.lq_toolname << ": -L" << Options::layering.at(Layering::SQUASHED)
		  << " can only be used with full models." << std::endl;
	exit( 1 );
    }

    if ( Flags::processors() == Processors::NONE
	 || Flags::layering() == Layering::PROCESSOR
	 || Flags::layering() == Layering::SHARE ) {
	Flags::print[PROCESSOR_QUEUEING].opts.value.b = false;
    }

    if ( Flags::bcmp_model ) {
	Flags::set_aggregation( Aggregate::ENTRIES );
    }

    /*
     * Change font size because scaleBy doesn't -- Fig doesn't use points for it's coordinates,
     * but it does use points for it's labels.
     */

    Flags::set_font_size( static_cast<int>(Flags::font_size() * Flags::magnification() + 0.5) );

#if HAVE_GD_H && HAVE_LIBGD
    switch( Flags::output_format() ) {
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
	switch( Flags::output_format() ) {
#if defined(EMF_OUTPUT)
	case File_Format::EMF:
	    if ( LQIO::Filename::isRegularFile( fileno( stdout ) ) == 0 ) {
		std::cerr << LQIO::io_vars.lq_toolname << ": Cannot write "
			  << Options::file_format.at(Flags::output_format())
			  << " to stdout - stdout is not a regular file."  << std::endl;
		exit( 1 );
	    }
	    break;
#endif
#if defined(SXD_OUTPUT)
	case File_Format::SXD:
	    std::cerr << LQIO::io_vars.lq_toolname << ": Cannot write "
		      << Options::file_format.at(Flags::output_format())
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

    if ( Flags::include_only() != nullptr ) {
	delete Flags::include_only();
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
    longopts.resize( Flags::print.size() * 2, opt );
    for ( std::vector<Options::Type>::const_iterator f = Flags::print.begin(); f != Flags::print.end(); ++f, ++k ) {
	longopts[k].has_arg = (f->arg != 0 ? required_argument : no_argument);
	longopts[k].name    = strdup(f->name);
	longopts[k].val     = f->c;
	if ( (f->c & ~0x7f) == 0 && std::islower( f->c ) && f->arg == 0 ) {
	    /* These are the +/- options */
	    longopts[k].val |= 0x0100;					/* + case is > 256 */
	    k += 1;
	    std::string name = "no-";					/* - case is no-<name> */
	    name += f->name;
	    longopts[k].name = strdup( name.c_str() );			/* Make a copy */
	    longopts[k].val = f->c;
	} else if ( (f->c & 0xff00) == 0x0300 ) {
	    /* These are the +/- options */
	    k += 1;
	    std::string name = "no-";					/* - case is no-<name> */
	    name += f->name;
	    longopts[k].name = strdup( name.c_str() );			/* Make a copy */
	    longopts[k].val = (f->c & ~0x0100);		/* Clear the bit */
	}

	if ( (f->c & 0xff00) == 0 ) {
	    opts +=  static_cast<char>(f->c);
	    if ( f->arg ) {
		opts += ':';
	    }
	}
    }
}
#else
static void
makeopts( string& opts )
{
    for ( std::vector<Options::Type>::const_iterator f = Flags::print.begin(); f != Flags::print.end(); ++f, ++k ) {
	opts += static_cast<char>(f->c);
	if ( f->arg ) {
	    opts += ':';
	}
    }
}
#endif


void
setOutputFormat( const File_Format f )
{
    Flags::set_output_format( f );
    
    switch ( f ) {
    case File_Format::OUTPUT:
    case File_Format::PARSEABLE:
    case File_Format::RTF:
	Flags::set_precision(7);					/* Increase default precision */
	Flags::set_print_input_parameters( true );		     	/* input parameters. */
	Flags::print[CONFIDENCE_INTERVALS].opts.value.b = true; 	/* Confidence Intervals */
	Options::set_all_result_options( true );
	/* Fall through */
    case File_Format::JSON:
    case File_Format::LQX:
    case File_Format::XML:
    case File_Format::SRVN:
	Flags::set_processors( Processors::ALL ); 		 	/* Print all processors. */
	Flags::set_layering( Layering::PROCESSOR );			/* Order by processors */
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

#if X11_OUTPUT
    case File_Format::X11:
	break;
#endif
    }
}
