/*
 * $Id: qnsolver.cc 14427 2021-01-28 23:13:01Z greg $
 */

#include <algorithm>
#include <iostream>
#include <iomanip>
#include <cstdlib>
#include <getopt.h>
#include <libgen.h>
#include <mva/mva.h>
#include <lqio/jmva_document.h>
#include <lqio/dom_document.h>
#include <lqio/bcmp_bindings.h>
#include <lqio/srvn_spex.h>
#include <lqx/Program.h>
#include "closedmodel.h"
#include "runlqx.h"


static void makeopts( const struct option * longopts, std::string& opts );
static void usage() ;

#if HAVE_GETOPT_LONG
const struct option longopts[] =
    /* name */ /* has arg */ /*flag */ /* val */
{
    { "bard-schweitzer", no_argument,       0, 'b' },
    { "debug-mva",	 no_argument,	    0, 'd' },
    { "exact-mva",       no_argument,       0, 'e' },
    { "fast-linearizer", no_argument,       0, 'f' },
    { "help",            no_argument,       0, 'h' },
    { "linearizer",      no_argument,       0, 'l' },
    { "silent",          no_argument,       0, 's' },
    { "verbose",         no_argument,       0, 'v' },
    { "experimental",	 no_argument,	    0, 'x' },
    { "debug-xml",	 no_argument, 	    0, 'X' },
    { "debug-spex",	 no_argument,	    0, 'S' },
    { 0, 0, 0, 0 }
};

static std::string opts;
#else
static std::string opts = "bdefhlsvx";
#endif

const char * opthelp[]  = {
    /* "bard-schweitzer", */    "Test using Bard-Schweitzer solver.",
    /* "debug",           */    "Enable debug code.",
    /* "exact-mva",       */    "Test using Exact MVA solver.",
    /* "fast-linearizer", */    "Test using the Fast Linearizer solver.",
    /* "help",            */    "Show this.",
    /* "linearizer",      */    "Test using Generic Linearizer.",
    /* "silent",          */    "",
    /* "verbose",         */    "",
    /* "experimental",	  */	"",
    /* "debug-xml"	  */    "Debug XML input.",
    /* "debug-spex"	  */	"Debug SPEX program.",
    nullptr
};

static bool silencio_flag = false;			/* Don't print results if 1	*/
static bool verbose_flag = true;			/* Print results		*/
static bool print_spex = false;				/* Print LQX program		*/
bool debug_flag = false;

std::string program_name;

BCMP::JMVA_Document* __input = nullptr;

int main (int argc, char *argv[])
{
    ClosedModel::Using solver = ClosedModel::Using::EXACT_MVA;
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
	    solver = ClosedModel::Using::BARD_SCHWEITZER;
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
	    solver = ClosedModel::Using::EXACT_MVA;
	    break;
			
	case 'f':
	    solver = ClosedModel::Using::LINEARIZER2;
	    break;

	case 'h':
	    usage();
	    return 0;

	case 'l':
	    solver = ClosedModel::Using::LINEARIZER;
	    break;
			
	case 's':
	    silencio_flag = true;
	    break;

	case 'v':
	    verbose_flag = true;
	    break;
			
	case 'x':
	    solver = ClosedModel::Using::EXPERIMENTAL;
	    break;

	case 'X':
            LQIO::DOM::Document::__debugXML = true;
	    break;
	    
	case 'S':
	    print_spex = true;
	    break;
	    
	default:
	    usage();
	    return 1;
	}
    }

    /* input is assumed to come in from stdin.                          */

    if ( optind == argc ) {
//	Model document( std::string( "-" ) );
//	solve( document, EXACT_MVA );
    } else {
        for ( ; optind < argc; ++optind ) {
	    BCMP::JMVA_Document input( argv[optind] );
	    if ( !input.parse() ) continue;
	    ClosedModel model( input.model() );
	    if ( !model ) continue;

	    if ( input.hasSPEX() ) {
		std::vector<LQX::SyntaxTreeNode *> * program = new std::vector<LQX::SyntaxTreeNode *>;
		if ( !LQIO::spex.construct_program( program, input.getResultVariables(), nullptr ) ) continue;
		LQX::Program * lqx = LQX::Program::loadRawProgram( program );
		input.registerExternalSymbolsWithProgram( lqx );
		if ( print_spex ) {
		    lqx->print( std::cout );
		}
		LQX::Environment * environment = lqx->getEnvironment();
		environment->getMethodTable()->registerMethod(new SolverInterface::Solve(model, solver));
		BCMP::RegisterBindings(environment, &input.model());
//		set output...
//		    environment->setDefaultOutput( output );      /* Default is stdout */
		/* Invoke the LQX program itself */
		if ( !lqx->invoke() ) {
//		    LQIO::solution_error( LQIO::ERR_LQX_EXECUTION, inputFileName.c_str() );
//		    rc = INVALID_INPUT;
		} else if ( !SolverInterface::Solve::solveCallViaLQX ) {
		    /* There was no call to solve the LQX */
//		    LQIO::solution_error( LQIO::ADV_LQX_IMPLICIT_SOLVE, inputFileName.c_str() );
		    std::vector<LQX::SymbolAutoRef> args;
		    environment->invokeGlobalMethod("solve", &args);
		}
		
	    } else if ( model.instantiate() ) {
		if ( debug_flag ) {
		    model.debug( std::cout );
		}
		if ( model.solve( solver ) ) {
		    model.print( std::cout );		/* for now. */
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
