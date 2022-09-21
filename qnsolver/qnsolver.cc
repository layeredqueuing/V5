/*
 * $Id: qnsolver.cc 15876 2022-09-20 20:02:15Z greg $
 */

#include "config.h"
#include <algorithm>
#include <iostream>
#include <iomanip>
#include <cstdlib>
#include <getopt.h>
#include <libgen.h>
#if HAVE_EXPAT_H
#include <lqio/jmva_document.h>
#endif
#include <lqio/dom_document.h>
#include <lqio/qnap2_document.h>
#include "pragma.h"
#include "model.h"

static LQIO::DOM::Pragma pragmas;


static void makeopts( const struct option * longopts, std::string& opts );
static void usage() ;

#if HAVE_GETOPT_LONG
const struct option longopts[] =
    /* name */ /* has arg */ /*flag */ /* val */
{
    { "bounds",					no_argument,		0, 'b' },
    { LQIO::DOM::Pragma::_exact_,		no_argument,		0, 'e' },
    { LQIO::DOM::Pragma::_schweitzer_,		no_argument,		0, 's' },
    { LQIO::DOM::Pragma::_linearizer_,		no_argument,		0, 'l' },
    { LQIO::DOM::Pragma::_fast_,		no_argument,		0, 'f' },
    { "output",					required_argument,	0, 'o' },
    { "plot-queue-length",			required_argument,	0, 'q' },
    { "plot-response-time",			no_argument,		0, 'r' },
    { "plot-throughput",			optional_argument,	0, 't' },
    { "plot-utilization",			optional_argument,	0, 'u' },
    { "plot-waiting-time",			required_argument,	0, 'w' },
    { "multiserver",				required_argument,	0, 'm' },
    { LQIO::DOM::Pragma::_force_multiserver_,	no_argument,		0, 'F' },
    { "verbose",				no_argument,		0, 'v' },
    { "help",					no_argument,		0, 'h' },
    { "export-qnap2",				no_argument,		0, 'Q' },
    { "debug-mva",				no_argument,		0, 'D' },
    { "debug-lqx",				no_argument,		0, 'L' },
    { "debug-xml",				no_argument,		0, 'X' },
    { "debug-spex",				no_argument,		0, 'S' },
    { nullptr, 0, 0, 0 }
};

static std::string opts;
#else
static std::string opts = "bdefhlo:rstvxQSX";
#endif

const static std::map<const std::string,const std::string> opthelp  = {
    { "bounds",		    			"Use the bounds solver." },
    { LQIO::DOM::Pragma::_exact_,		"Use Exact MVA." },
    { LQIO::DOM::Pragma::_schweitzer_,		"Use Bard-Schweitzer approximate MVA." },
    { LQIO::DOM::Pragma::_linearizer_,		"Use Linearizer." },
    { LQIO::DOM::Pragma::_fast_,		"Use the Fast Linearizer solver." },
    { "output",					"Send output to ARG." },
    { "plot-queue-length",			"Output gnuplot to plot station queue-length.  ARG specifies a class or station." },
    { "plot-response-time",			"Output gnuplot to plot system response-time (and bounds)." },
    { "plot-throughput",			"Output gnuplot to plot system throughput (and bounds), or for a class or station with ARG." },
    { "plot-utilization",			"Output gnuplot to plot utilization.  ARG specifies a class or station." },
    { "plot-waiting-time",			"Output gnuplot to plot station waiting-times.	ARG specifies a class or station." },
    { "multiserver",				"Use ARG for multiservers.  ARG={conway,reiser,rolia,zhou}." },
    { LQIO::DOM::Pragma::_force_multiserver_,	"Use the multiserver solution for load independent stations (copies=1)." },
    { "verbose",				"" },
    { "help",					"Show this." },
    { "export-qnap2",				"Export a QNAP2 model.	Do not solve." },
    { "debug-lqx",				"Debug the LQX program." },
    { "debug-mva",				"Enable debug code in the MVA solver." },
    { "debug-xml",				"Debug XML input." },
    { "debug-spex",				"Debug SPEX program." },
};

/* Flags */

static bool verbose_flag = false;		/* Print steps		*/
static bool print_qnap2 = false;		/* Export to qnap2.  		*/
static bool print_gnuplot = false;		/* Output WhatIf as gnuplot	*/
static BCMP::Model::Result::Type plot_type = BCMP::Model::Result::Type::THROUGHPUT;

/* Globals */

std::string program_name;

#if HAVE_EXPAT_H
BCMP::JMVA_Document* __input = nullptr;
#endif

/* Local procedures */

static void exec( const std::string& input_file_name, const std::string& output_file_name, const std::string& );

int main (int argc, char *argv[])
{
    std::string output_file_name;
    std::string plot_arg;

    program_name = basename( argv[0] );

    /* Process all command line arguments.  If none specified, then     */
#if HAVE_GETOPT_LONG
    makeopts( longopts, opts );
#endif

    LQIO::io_vars.init( VERSION, basename( argv[0] ), nullptr );

    pragmas.insert( getenv( "QNSOLVER_PRAGMAS" ) );

    for ( ;; ) {
#if HAVE_GETOPT_LONG
	const int c = getopt_long( argc, argv, opts.c_str(), longopts, NULL );
#else
	const int c = getopt( argc, argv, opts.c_str() );
#endif
	if ( c == EOF ) break;

	switch( c ) {
	case 'b':
	    pragmas.insert(LQIO::DOM::Pragma::_mva_,LQIO::DOM::Pragma::_bounds_);
	    break;

	case 'd':
#if HAVE_EXPAT_H
	    debug_flag = true;
#endif
#if DEBUG_MVA
	    MVA::debug_D = true;
	    MVA::debug_L = true;
	    MVA::debug_P = true;
	    MVA::debug_U = true;
	    MVA::debug_W = true;
#endif
	    break;

	case 'e':
	    pragmas.insert(LQIO::DOM::Pragma::_mva_,LQIO::DOM::Pragma::_exact_);
	    break;

	case 'f':
	    pragmas.insert(LQIO::DOM::Pragma::_mva_,LQIO::DOM::Pragma::_fast_);
	    break;

	case 'F':
	    pragmas.insert(LQIO::DOM::Pragma::_force_multiserver_,LQIO::DOM::Pragma::_true_);
	    break;

	case 'h':
	    usage();
	    return 0;

	case 'l':
	    pragmas.insert(LQIO::DOM::Pragma::_mva_,LQIO::DOM::Pragma::_linearizer_);
	    break;

	case 'L':
	    LQIO::DOM::Document::lqx_parser_trace(stderr);
	    break;
	    
	case 'm':
	    pragmas.insert(LQIO::DOM::Pragma::_multiserver_,optarg);
	    break;

	case 'o':
            output_file_name = optarg;
	    break;

	case 'q':
	    print_gnuplot = true;			/* Output WhatIf as gnuplot	*/
	    plot_type = BCMP::Model::Result::Type::QUEUE_LENGTH;
	    if ( optarg != nullptr ) plot_arg = optarg;
	    break;

	case 'r':
	    print_gnuplot = true;			/* Output WhatIf as gnuplot	*/
	    plot_type = BCMP::Model::Result::Type::RESPONSE_TIME;
	    break;

	case 's':
	    pragmas.insert(LQIO::DOM::Pragma::_mva_,LQIO::DOM::Pragma::_schweitzer_);
	    break;

	case 't':
	    print_gnuplot = true;			/* Output WhatIf as gnuplot	*/
	    plot_type = BCMP::Model::Result::Type::THROUGHPUT;
	    if ( optarg != nullptr ) plot_arg = optarg;
	    break;

	case 'u':
	    print_gnuplot = true;			/* Output WhatIf as gnuplot	*/
	    plot_type = BCMP::Model::Result::Type::UTILIZATION;
	    if ( optarg != nullptr ) plot_arg = optarg;
	    break;

	case 'v':
	    verbose_flag = true;
	    break;

	case 'w':
	    print_gnuplot = true;			/* Output WhatIf as gnuplot	*/
	    plot_type = BCMP::Model::Result::Type::RESIDENCE_TIME;
	    if ( optarg != nullptr ) plot_arg = optarg;
	    break;

	case 'x':
	    /* Not implemented */
	    break;

	case 'Q':
	    print_qnap2 = true;
	    break;

	case 'S':
#if HAVE_EXPAT_H
	    print_spex = true;
#endif
	    break;

	case 'X':
            LQIO::DOM::Document::__debugXML = true;
	    break;

	default:
	    usage();
	    return 1;
	}
    }

    /* input is assumed to come in from stdin.                          */

#if HAVE_EXPAT_H
    if ( optind == argc ) {
	exec( "-", output_file_name, plot_arg );
    } else {
        for ( ; optind < argc; ++optind ) {
	    exec( argv[optind], output_file_name, plot_arg );
	}
    }
#else
    std::cerr << LQIO::io_vars.lq_toolname << ": No expat library available." << std::endl;
#endif
    return 0;
}


/*
 * Run the solver.
 */

static void exec( const std::string& input_file_name, const std::string& output_file_name, const std::string& plot_arg )
{
    static std::map<const Model::Solver,const std::string> solver_name = {
	{ Model::Solver::BOUNDS,		"bounds" },
	{ Model::Solver::EXACT_MVA,		LQIO::DOM::Pragma::_exact_ },
	{ Model::Solver::BARD_SCHWEITZER,	LQIO::DOM::Pragma::_schweitzer_ },
	{ Model::Solver::LINEARIZER,		LQIO::DOM::Pragma::_linearizer_ },
	{ Model::Solver::LINEARIZER2,		LQIO::DOM::Pragma::_fast_ },
	{ Model::Solver::EXPERIMENTAL,		"experimental" },
	{ Model::Solver::OPEN,			"open" }
    };

    if ( verbose_flag ) std::cerr << input_file_name << ": load... ";
    if ( LQIO::DOM::Document::getInputFormatFromFilename( input_file_name, LQIO::DOM::Document::InputFormat::JMVA ) == LQIO::DOM::Document::InputFormat::QNAP2 ) {
	BCMP::QNAP2_Document input( input_file_name );
    }
#if HAVE_EXPAT_H
    BCMP::JMVA_Document input( input_file_name );
    if ( input.load() ) {
	if ( print_qnap2 ) {
	    std::cout << BCMP::QNAP2_Document("",input.model()) << std::endl;
	} else {
	    input.mergePragmas( pragmas.getList() );
	    Pragma::set( input.getPragmaList() );		/* load pragmas here */

	    try {
		if ( print_gnuplot ) input.plot( plot_type, plot_arg );
	    }
	    catch ( const std::invalid_argument& e ) {
		std::cerr << LQIO::io_vars.lq_toolname << ": Invalid class or station name for --plot: " << e.what() << std::endl;
	    }
	    Model model( input, Pragma::solver(), output_file_name );
	    if ( verbose_flag ) std::cerr << "construct... ";
	    if ( model.construct() ) {
		if ( verbose_flag ) std::cerr << "solve using " << solver_name.at(model.solver()) << "... ";
		model.solve();
	    }
	}
    }
#endif
    if ( verbose_flag ) std::cerr << "done" << std::endl;
}

static void
makeopts( const struct option * longopts, std::string& opts )
{
    for ( int i = 0; longopts[i].name != 0; ++i ) {
	if ( isgraph( longopts[i].val ) ) {
	    opts += longopts[i].val;
	    if ( longopts[i].has_arg == required_argument ) {
		opts += ':';
	    }
	}
    }
}

static void
usage()
{
    std::cerr << "Usage: " << program_name;

#if HAVE_GETOPT_LONG
    std::cerr << " [option]" << std::endl << std::endl;
    std::cerr << "Options" << std::endl;
    for ( const struct option *o = longopts; (o->name || o->val); ++o ) {
	std::string s;
	if ( o->name ) {
	    s = "--";
	    s += o->name;
	    switch ( o->val ) {
	    }
	} else {
	    s = " ";
	}
	if ( isascii(o->val) && isgraph(o->val) ) {
	    std::cerr << " -" << static_cast<char>(o->val) << ", ";
	} else {
	    std::cerr << "     ";
	}
	std::cerr.setf( std::ios::left, std::ios::adjustfield );
	std::cerr << std::setw(24) << s << opthelp.at(o->name) << std::endl;
    }
#else
    const char * s;
    std::cerr << " [-";
    for ( s = opts.c_str(); *s; ++s ) {
	if ( *(s+1) == ':' ) {
	    ++s;
	} else {
	    std::cerr.put( *s );
	}
    }
    std::cerr << ']';

    for ( s = opts.c_str(); *s; ++s ) {
	if ( *(s+1) == ':' ) {
	    std::cerr << " [-" << *s;
	    switch ( *s ) {
	    }
	    std::cerr << ']';
	    ++s;
	}
    }
#endif
    std::cerr << std::endl;
}
