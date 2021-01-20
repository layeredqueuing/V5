/*
 * $Id: qnsolver.cc 14386 2021-01-20 23:58:29Z greg $
 */

#include <lqio/jmva_document.h>
#include <lqio/dom_document.h>
#include <algorithm>
#include <iostream>
#include <iomanip>
#include <cstdlib>
#include <getopt.h>
#include <libgen.h>
#include <mva/mva.h>
#include "bcmpmodel.h"
#include "bcmpresult.h"


typedef enum {EXACT_MVA, LINEARIZER, LINEARIZER2, BARD_SCHWEITZER, EXPERIMENTAL } solver_type;

static void makeopts( const struct option * longopts, std::string& opts );
static void usage() ;

#if HAVE_GETOPT_LONG
const struct option longopts[] =
    /* name */ /* has arg */ /*flag */ /* val */
{
    { "bard-schweitzer", no_argument,       0, 'b' },
#if DEBUG_MVA
    { "debug-mva",	 no_argument,	    0, 'd' },
#endif
    { "exact-mva",       no_argument,       0, 'e' },
    { "fast-linearizer", no_argument,       0, 'f' },
    { "help",            no_argument,       0, 'h' },
    { "linearizer",      no_argument,       0, 'l' },
    { "silent",          no_argument,       0, 's' },
    { "verbose",         no_argument,       0, 'v' },
    { "experimental",	 no_argument,	    0, 'x' },
    { "debug-xml",	 no_argument, 	    0, 'X' },
    { 0, 0, 0, 0 }
};

static std::string opts;
#else
#if DEBUG_MVA
static std::string opts = "bdefhlsvx";
#else
static std::string opts = "befhlsvx";
#endif
#endif

const char * opthelp[]  = {
    /* "bard-schweitzer", */    "Test using Bard-Schweitzer solver.",
#if DEBUG_MVA
    /* "debug",           */    "Enable debug code.",
#endif
    /* "exact-mva",       */    "Test using Exact MVA solver.",
    /* "fast-linearizer", */    "Test using the Fast Linearizer solver.",
    /* "help",            */    "Show this.",
    /* "linearizer",      */    "Test using Generic Linearizer.",
    /* "silent",          */    "",
    /* "verbose",         */    "",
    /* "experimental",	  */	"",
    /* "debug-xml"	  */    "Debug XML input.",
    nullptr
};

static bool silencio_flag = false;			/* Don't print results if 1	*/
static bool verbose_flag = true;			/* Print results		*/

std::string program_name;

int main (int argc, char *argv[])
{
    solver_type solver = EXACT_MVA;
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
	    solver = BARD_SCHWEITZER;
	    break;

#if DEBUG_MVA
	case 'd':
	    MVA::debug_D = true;
	    MVA::debug_L = true;
	    MVA::debug_P = true;
	    MVA::debug_U = true;
	    MVA::debug_W = true;
	    break;
#endif

	case 'e':
	    solver = EXACT_MVA;
	    break;
			
	case 'f':
	    solver = LINEARIZER2;
	    break;

	case 'h':
	    usage();
	    return 0;

	case 'l':
	    solver = LINEARIZER;
	    break;
			
	case 's':
	    silencio_flag = true;
	    break;

	case 'v':
	    verbose_flag = true;
	    break;
			
	case 'x':
	    solver = EXPERIMENTAL;
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
//	Model document( std::string( "-" ) );
//	solve( document, EXACT_MVA );
    } else {
        for ( ; optind < argc; ++optind ) {
	    BCMP::JMVA_Document input( argv[optind] );
	    if ( !input.parse() ) continue;
	    Model model( input.model() );
	    if ( !model ) continue;
	    MVA * mva = nullptr;
	    switch ( solver ) {
	    case EXACT_MVA:	    mva = new ExactMVA( model.Q(), model.N(), model.Z(), model.priority() ); break;
	    case BARD_SCHWEITZER:   mva = new Schweitzer( model.Q(), model.N(), model.Z(), model.priority() ); break;
	    case LINEARIZER:	    mva = new Linearizer( model.Q(), model.N(), model.Z(), model.priority() ); break;
	    }
		
	    if ( mva != nullptr ) {
		mva->solve();
//		mva->print(std::cout);
		model.insertResult( new BCMPResult( model.N(), model.Q(), *mva ) );
	    }
	    model.print( std::cout );
	    delete mva;
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
