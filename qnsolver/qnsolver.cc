/*
 * $Id: qnsolver.cc 16695 2023-04-22 15:08:24Z greg $
 */

#include "config.h"
#include <algorithm>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <cstdlib>
#include <cstring>
#include <getopt.h>
#include <libgen.h>
#if HAVE_EXPAT_H
#include <lqio/jmva_document.h>
#endif
#include <lqio/dom_document.h>
#include <lqio/filename.h>
#include <lqio/gnuplot.h>
#include <lqio/qnap2_document.h>
#include "pragma.h"
#include "model.h"

static LQIO::DOM::Pragma pragmas;


static void makeopts( const struct option * longopts, std::string& opts );
static void usage() ;

extern "C" int qnap2debug;

#if HAVE_GETOPT_LONG
const struct option longopts[] =
    /* name */ /* has arg */ /*flag */ /* val */
{
    { "bounds",                                 no_argument,            0, 'b' },
    { LQIO::DOM::Pragma::_exact_,               no_argument,            0, 'e' },
    { LQIO::DOM::Pragma::_fast_,                no_argument,            0, 'f' },
    { LQIO::DOM::Pragma::_linearizer_,          no_argument,            0, 'l' },
    { LQIO::DOM::Pragma::_schweitzer_,          no_argument,            0, 's' },
    { LQIO::DOM::Pragma::_force_multiserver_,   no_argument,            0, 'M' },
    { "plot-queue-length",                      optional_argument,      0, 'q' },
    { "plot-response-time",                     no_argument,            0, 'r' },
    { "plot-throughput",                        optional_argument,      0, 't' },
    { "plot-utilization",                       optional_argument,      0, 'u' },
    { "plot-waiting-time",                      optional_argument,      0, 'w' },
    { "output",                                 required_argument,      0, 'o' },
    { "file-format",                            required_argument,      0, 'O' },
    { "colours",                                required_argument,      0, 'C' },
    { "help",                                   no_argument,            0, 'h' },
    { "multiserver",                            required_argument,      0, 'm' },
    { "no-execute",                             no_argument,            0, 'n' },
    { "verbose",                                no_argument,            0, 'v' },
    { "export-jaba",                            no_argument,            0, 'B' },
    { "export-jmva",                            no_argument,            0, 'J' },
    { "export-qnap",                            no_argument,            0, 'Q' },
    { "print-lqx",                              no_argument,            0, 'P' },
    { "debug-mva",                              no_argument,            0, 'd' },
    { "debug-qnap2",                            no_argument,            0, 'D' },
    { "debug-lqx",                              no_argument,            0, 'L' },
    { "debug-xml",                              no_argument,            0, 'X' },
    { nullptr, 0, 0, 0 }
};

static std::string opts;
#else
static std::string opts = "bdefhlo:rstvxDGJLQSX";
#endif

const static std::map<const std::string,const std::string> opthelp  = {
    { "bounds",                                 "Use the bounds solver." },
    { "colours",				"Use the colours for plotting.  ARG is a list of colours. (not implemented)" },
    { "debug-lqx",                              "Debug the LQX program." },
    { "debug-mva",                              "Enable debug code in the MVA solver." },
    { "debug-qnap2",                            "Debug the QNAP2 input parser." },
    { "debug-xml",                              "Debug XML input." },
    { "export-jmva",				"Export a JMVA model after solution (results embedded)." },
    { "export-jaba",				"Export a JABA model.  Results and reference stations are stripped out." },
    { "export-qnap",                            "Export a QNAP2 model.  Do not solve." },
    { "file-format",				"When plotting, add instructions to output as ARG to the output file." },
    { "help",                                   "Show this." },
    { "multiserver",                            "Use ARG for multiservers.  ARG={conway,reiser,rolia,zhou}." },
    { "no-execute",				"Load the model and run LQX, but do not solve the model." },
    { "output",                                 "Send output to ARG." },
    { "plot-queue-length",                      "Output gnuplot to plot station queue-length.  ARG specifies a class or station." },
    { "plot-response-time",                     "Output gnuplot to plot system response-time (and bounds)." },
    { "plot-throughput",                        "Output gnuplot to plot system throughput (and bounds), or for a class or station with ARG." },
    { "plot-utilization",                       "Output gnuplot to plot utilization.  ARG specifies a class or station." },
    { "plot-waiting-time",                      "Output gnuplot to plot station waiting-times.  ARG specifies a class or station." },
    { "print-lqx",                              "Print the LQX program used to solve the model." },
    { "verbose",                                "" },
    { LQIO::DOM::Pragma::_exact_,               "Use Exact MVA." },
    { LQIO::DOM::Pragma::_fast_,                "Use the Fast Linearizer solver." },
    { LQIO::DOM::Pragma::_force_multiserver_,   "Use the multiserver solution for load independent stations (copies=1)." },
    { LQIO::DOM::Pragma::_linearizer_,          "Use Linearizer." },
    { LQIO::DOM::Pragma::_schweitzer_,          "Use Bard-Schweitzer approximate MVA." },
};

const static std::map<const std::string,const LQIO::GnuPlot::Format> gnuplot_output_format = {
    { "emf",	LQIO::GnuPlot::Format::EMF },
    { "eps",	LQIO::GnuPlot::Format::EPS },
    { "fig",	LQIO::GnuPlot::Format::FIG },
    { "gif",	LQIO::GnuPlot::Format::GIF },
    { "pdf",	LQIO::GnuPlot::Format::PDF },
    { "png",	LQIO::GnuPlot::Format::PNG },
    { "svg",	LQIO::GnuPlot::Format::SVG },
    { "latex",	LQIO::GnuPlot::Format::LATEX }
};


/* Flags */

static bool print_qnap2 = false;		/* Export to qnap2.  		*/
static bool print_jmva = false;			/* Export to JMVA		*/
static bool print_gnuplot = false;		/* Output WhatIf as gnuplot	*/
static BCMP::Model::Result::Type plot_type = BCMP::Model::Result::Type::THROUGHPUT;

/* Globals */

std::string program_name;

#if HAVE_LIBEXPAT
QNIO::JMVA_Document* __input = nullptr;
#endif

/* Local procedures */

static void exec( const std::string& input_file_name, const std::string& output_file_name, const std::string&, LQIO::GnuPlot::Format );
static void exec( QNIO::Document& input, const std::string& output_file_name, const std::string&, LQIO::GnuPlot::Format );

int main (int argc, char *argv[])
{
    std::string output_file_name;
    std::string plot_arg;
    LQIO::GnuPlot::Format gnuplot_format = LQIO::GnuPlot::Format::TERMINAL;

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

	case 'B':
	    pragmas.insert(LQIO::DOM::Pragma::_mva_,LQIO::DOM::Pragma::_bounds_);
	    break;
	    
	case 'C':
	    std::cerr << "Colours unsupported..." << std::endl;
	    break;

	case 'd':
	    Model::debug_flag = true;
#if DEBUG_MVA
	    MVA::debug_D = true;
	    MVA::debug_L = true;
	    MVA::debug_P = true;
	    MVA::debug_U = true;
	    MVA::debug_W = true;
#endif
	    break;

	case 'D':
	    qnap2debug = 1;
	    break;

	case 'e':
	    pragmas.insert(LQIO::DOM::Pragma::_mva_,LQIO::DOM::Pragma::_exact_);
	    break;

	case 'f':
	    pragmas.insert(LQIO::DOM::Pragma::_mva_,LQIO::DOM::Pragma::_fast_);
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

	case 'M':
	    pragmas.insert(LQIO::DOM::Pragma::_force_multiserver_,LQIO::DOM::Pragma::_true_);
	    break;

	case 'n':
	    Model::no_execute = true;
	    break;
	    
	case 'o':
            output_file_name = optarg;
	    break;

	case 'O':
	    if ( optarg != nullptr ) {
		std::map<const std::string,const LQIO::GnuPlot::Format>::const_iterator format = gnuplot_output_format.find( optarg );
		if ( format == gnuplot_output_format.end() ) {
		    std::cerr << program_name << ": Unsupported gnuplot format: " << optarg << "." << std::endl;
		} else {
		    gnuplot_format = format->second;
		}
	    }
	    break;
	    
	case 'q':
	    print_gnuplot = true;			/* Output WhatIf as gnuplot	*/
	    plot_type = BCMP::Model::Result::Type::QUEUE_LENGTH;
	    if ( optarg != nullptr ) plot_arg = optarg;
	    break;

	case 'r':
	    print_gnuplot = true;			/* Output WhatIf as gnuplot	*/
	    plot_type = BCMP::Model::Result::Type::RESPONSE_TIME;
	    if ( optarg != nullptr ) plot_arg = optarg;
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
	    Model::verbose_flag = true;
	    break;

	case 'w':
	    print_gnuplot = true;			/* Output WhatIf as gnuplot	*/
	    plot_type = BCMP::Model::Result::Type::RESIDENCE_TIME;
	    if ( optarg != nullptr ) plot_arg = optarg;
	    break;

	case 'x':
	    /* Not implemented */
	    break;

	case 'J':
	    print_jmva = true;
	    break;
	    
	case 'Q':
	    print_qnap2 = true;
	    break;

	case 'P':
	    Model::print_program = true;
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
	exec( "-", output_file_name, plot_arg, gnuplot_format );
    } else {
        for ( ; optind < argc; ++optind ) {
	    exec( argv[optind], output_file_name, plot_arg, gnuplot_format );
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

static void exec( const std::string& input_file_name, const std::string& output_file_name, const std::string& plot_arg, LQIO::GnuPlot::Format plot_format )
{
    if ( Model::verbose_flag ) std::cerr << input_file_name << ": load... ";
    if ( LQIO::DOM::Document::getInputFormatFromFilename( input_file_name, LQIO::DOM::Document::InputFormat::JMVA ) == LQIO::DOM::Document::InputFormat::QNAP2 ) {
	QNIO::QNAP2_Document input( input_file_name );
	exec( input, output_file_name, plot_arg, plot_format );
#if HAVE_LIBEXPAT
    } else {
	QNIO::JMVA_Document input( input_file_name );
	exec( input, output_file_name, plot_arg, plot_format );
#endif
    }
    if ( Model::verbose_flag ) std::cerr << "done" << std::endl;
}


static void exec( QNIO::Document& input, const std::string& output_file_name, const std::string& plot_arg, LQIO::GnuPlot::Format plot_format )
{
    if ( !input.load() ) return;
    input.mergePragmas( pragmas.getList() );
    Pragma::set( input.getPragmaList() );		/* load pragmas here */

    const bool qnap_model = input.getInputFormat() == QNIO::Document::InputFormat::QNAP;
    
    if ( print_gnuplot && qnap_model ) {
	std::cerr << LQIO::io_vars.lq_toolname << ": plotting not supported with QNAP input." << std::endl;
    }
    std::ofstream output;
    const bool bounds = Pragma::mva() == Model::Solver::BOUNDS || input.boundsOnly();
    if ( !output_file_name.empty() ) {
	output.open( output_file_name, std::ios::out );
	if ( !output ) {
	    std::cerr << LQIO::io_vars.lq_toolname << ": Cannot open output file \"" << output_file_name << "\" -- " << strerror( errno ) << std::endl;
	}
    } else if ( print_jmva ) {
	const std::string extension = bounds ? "jmva" : "jaba";
	LQIO::Filename filename( input.getInputFileName(), extension );
	LQIO::Filename::backup( filename() );
	output.open( filename(), std::ios::out );
	if ( !output ) {
	    std::cerr << LQIO::io_vars.lq_toolname << ": Cannot open output file \"" << input.getInputFileName() << "\" -- " << strerror( errno ) << std::endl;
	}
    }

    if ( print_qnap2 ) {
	QNIO::QNAP2_Document qnap_model( input.getInputFileName(), input.model() );
	if ( output ) {
	    qnap_model.exportModel( output );
	} else {
	    qnap_model.exportModel( std::cout );
	}
    } else {
	try {
	    if ( print_gnuplot ) input.plot( plot_type, plot_arg, plot_format );
	}
	catch ( const std::invalid_argument& e ) {
	    std::cerr << LQIO::io_vars.lq_toolname << ": Invalid class or station name for --plot: " << e.what() << std::endl;
	}
	if ( print_jmva ) {
	    /* Since we might get QNAP in, rexport directly -- WhatIf?? */
	    Model model( input, Pragma::mva(), std::string() );
	    model.solve();
//	    QNIO::JMVA_Document new_model( output_file_name, input.model() );
//	    new_model.comprehensions() = input.comprehensions();
//	    new_model.exportModel( output );	/* Will save all results (if !bounds) */
	    input.exportModel( output );
	} else {
	    Model model( input, Pragma::mva(), output_file_name );
	    model.solve();
	}
    }
    if ( output ) {
	output.close();
    }
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
	    if ( o->has_arg != no_argument ) {
		s += "=ARG";
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
	std::cerr << std::setw(28) << s << opthelp.at(o->name) << std::endl;
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
