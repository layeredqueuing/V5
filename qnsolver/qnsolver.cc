/*
 * $Id: qnsolver.cc 14572 2021-03-21 02:43:53Z greg $
 */

#include <algorithm>
#include <iostream>
#include <iomanip>
#include <cstdlib>
#include <getopt.h>
#include <libgen.h>
#include <lqio/jmva_document.h>
#include <lqio/dom_document.h>
#include <lqio/qnap2_document.h>
#include "model.h"


static void makeopts( const struct option * longopts, std::string& opts );
static void usage() ;

#if HAVE_GETOPT_LONG
const struct option longopts[] =
    /* name */ /* has arg */ /*flag */ /* val */
{
    { "bounds",		    no_argument,	0, 'b' },
    { "exact-mva",          no_argument,       	0, 'e' },
    { "bard-schweitzer",    no_argument,       	0, 's' },
    { "linearizer",         no_argument,       	0, 'l' },
    { "fast-linearizer",    no_argument,       	0, 'f' },
    { "output", 	    required_argument, 	0, 'o' },
    { "plot-queue-length",  required_argument,  0, 'q' },
    { "plot-response-time", no_argument,        0, 'r' },
    { "plot-throughput",    optional_argument, 	0, 't' },
    { "plot-utilization",   required_argument,	0, 'u' },
    { "plot-waiting-time",  required_argument,  0, 'w' },
    { "verbose",            no_argument,        0, 'v' },
    { "help",               no_argument,       	0, 'h' },
    { "experimental",	    no_argument,	0, 'x' },
    { "export-qnap2",	    no_argument,	0, 'Q' },
    { "debug-mva",	    no_argument,    	0, 'd' },
    { "debug-xml",	    no_argument, 	0, 'X' },
    { "debug-spex",	    no_argument,	0, 'S' },
    { 0, 0, 0, 0 }
};

static std::string opts;
#else
static std::string opts = "bdefhlo:rstvxQSX";
#endif

const char * opthelp[]  = {
    /* "bounds"		  */    "Compute bounds",
    /* "exact-mva",       */    "Use Exact MVA.",
    /* "bard-schweitzer", */    "Use Bard-Schweitzer approximate MVA.",
    /* "linearizer",      */    "Use Linearizer.",
    /* "fast-linearizer", */    "Use the Fast Linearizer solver.",
    /* "output",	  */	"Send output to ARG.",
    /* "plot-queue-length */	"Output gnuplot to plot station queue-length.  ARG specifies a class or station.",
    /* "plot-response-time" */	"Output gnuplot to plot system response-time (and bounds).", 
    /* "plot-throughput", */    "Output gnuplot to plot system throughput (and bounds), or for a class or station with ARG.",
    /* "plot-utilization  */	"Output gnuplot to plot utilization.  ARG specifies a class or station.",
    /* "plot-waiting-time */	"Output gnuplot to plot station waiting-times.  ARG specifies a class or station.",
    /* "verbose",         */    "",
    /* "help",            */    "Show this.",
    /* "experimental",	  */	"",
    /* "export-qnap2",	  */	"Export a QNAP2 model.  Do not solve.",
    /* "debug-mva",       */    "Enable debug code.",
    /* "debug-xml"	  */    "Debug XML input.",
    /* "debug-spex"	  */	"Debug SPEX program.",
    nullptr
};

static bool verbose_flag = true;			/* Print results		*/

std::string program_name;

BCMP::JMVA_Document* __input = nullptr;

int main (int argc, char *argv[])
{
    std::string output_file_name;
    Model::Using solver = Model::Using::EXACT_MVA;
    bool print_qnap2 = false;			/* Export to qnap2.  		*/
    bool print_gnuplot = false;			/* Output WhatIf as gnuplot	*/
    BCMP::Model::Result::Type plot_type = BCMP::Model::Result::Type::THROUGHPUT;
    std::string plot_arg;

    program_name = basename( argv[0] );

    /* Process all command line arguments.  If none specified, then     */
#if HAVE_GETOPT_LONG
    makeopts( longopts, opts );
#endif
    
    LQIO::io_vars.init( VERSION, basename( argv[0] ), nullptr );

    for ( ;; ) {
#if HAVE_GETOPT_LONG
	const int c = getopt_long( argc, argv, opts.c_str(), longopts, NULL );
#else
	const int c = getopt( argc, argv, opts.c_str() );
#endif
	if ( c == EOF ) break;

	switch( c ) {
	case 'b':
	    break;
	    
	case 'd':
	    debug_flag = true;
#if DEBUG_MVA
	    MVA::debug_D = true;
	    MVA::debug_L = true;
	    MVA::debug_P = true;
	    MVA::debug_U = true;
	    MVA::debug_W = true;
#endif
	    break;

	case 'e':
	    solver = Model::Using::EXACT_MVA;
	    break;
			
	case 'f':
	    solver = Model::Using::LINEARIZER2;
	    break;

	case 'h':
	    usage();
	    return 0;

	case 'l':
	    solver = Model::Using::LINEARIZER;
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
	    solver = Model::Using::BARD_SCHWEITZER;
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
	    solver = Model::Using::EXPERIMENTAL;
	    break;

	case 'Q':
	    print_qnap2 = true;
	    break;
		
	case 'S':
	    print_spex = true;
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

    if ( optind == argc ) {
	std::cerr << LQIO::io_vars.lq_toolname << ": arg count." << std::endl;
	return 1;
    } else {
        for ( ; optind < argc; ++optind ) {
	    BCMP::JMVA_Document input( argv[optind] );
	    if ( !input.parse() ) continue;
	    if ( print_qnap2 ) {
		std::cout << BCMP::QNAP2_Document("",input.model()) << std::endl;
	    } else {
		try {
		    if ( print_gnuplot ) input.plot( plot_type, plot_arg );
		}
		catch ( const std::invalid_argument& e ) {
		    std::cerr << LQIO::io_vars.lq_toolname << ": Invalid class or station name for --plot: " << e.what() << std::endl;
		}
		Model model( input, solver, output_file_name );
		if ( model.construct() ) {
		    model.solve();
		}
	    }
	}
    }
    return 0;
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
    const char ** p = opthelp;
    for ( const struct option *o = longopts; (o->name || o->val) && *p; ++o, ++p ) {
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
	std::cerr << std::setw(24) << s << *p << std::endl;
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
