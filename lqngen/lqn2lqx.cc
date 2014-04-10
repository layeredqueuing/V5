/* lqngen.cc	-- Greg Franks Thu Jul 29 2004
 * Convert input files to lqnx with lqx parameterization.
 *
 * $Id: lqngen.cc 11811 2013-11-08 22:10:39Z greg $
 */

#include <errno.h>
#include <fstream>
#include <cstring>
#include <lqio/commandline.h>
#include <lqio/filename.h>
#include "lqngen.h"
#include "generate.h"

int
lqn2lqx( int argc, char **argv )
{
    Flags::lqx_output 	  = true;	
    Flags::xml_output 	  = true;	
    Flags::lqn2lqx    	  = true;
    Flags::annotate_input = false;

    unsigned int errorCode;
    extern char *optarg;
    char * output_file_name = 0;

    Generate::__service_time 		= new RV::Deterministic( 1.0 );
    Generate::__think_time 		= new RV::Deterministic( 0.0 );
    Generate::__forwarding_probability  = new RV::Deterministic( 0.0 );
    Generate::__rendezvous_rate 	= new RV::Deterministic( 1.0 );
    Generate::__send_no_reply_rate 	= new RV::Deterministic( 0.0 );
    Generate::__customers_per_client 	= new RV::Deterministic( 1.0 );
    Generate::__task_multiplicity 	= new RV::Deterministic( 1.0 );
    Generate::__processor_multiplicity  = new RV::Deterministic( 1.0 );
    Generate::__probability_second_phase= new RV::Deterministic( 0.0 );
    Generate::__number_of_entries	= new RV::Deterministic( 1.0 );

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

	switch( c ) {
	case 'o':
	    output_file_name = optarg;
	    break;

	default:
	    if ( !common_arg( c, optarg, &endptr ) ) {
		usage();
		exit( 1 );
	    }
	    break;
	}

	if ( endptr != 0 && *endptr != '\0' ) {
	    fprintf( stderr, "%s: Invalid argumement to -%c: %s\n", io_vars.lq_toolname, c, optarg );
	    exit( 1 );
	}
    }

    io_vars.lq_command_line = command_line.c_str();

    if ( Flags::xml_output ) {
	Flags::lqx_output = true;	
	Flags::spex_output = false;
    } else {
	Flags::lqx_output = false;	
	Flags::spex_output = true;
    }

    if ( Flags::number_of_runs > 1 && Flags::sensitivity > 0 ) {
	fprintf( stderr, "%s: --experiments=%d and --sensitivity=%g are mutually exclusive.\n", io_vars.lq_toolname, Flags::number_of_runs, Flags::sensitivity );
	exit( 1 );
    }

    if ( optind == argc ) {
	LQIO::DOM::Document* document = LQIO::DOM::Document::load( "-", LQIO::DOM::Document::AUTOMATIC_INPUT, "", &io_vars, errorCode, false );
	if ( document ) {
	    Generate aModel( document, Flags::number_of_runs );
	    aModel.reparameterize();
	    if ( output_file_name && strcmp( output_file_name, "-" ) != 0 ) {
		std::ofstream output;
		output.open( output_file_name, ios::out|ios::binary );
		if ( !output ) {
		    cerr << "Cannot open output file " << output_file_name << " - " << strerror( errno );
		} else {
		    output << aModel;
		    output.close();
		}
	    } else {
		cout << aModel;
	    }
	}

    } else {
	for ( ;optind < argc; ++optind ) {
	    /* We need an Expat Document because we have to export our DOM.  The Xerces DOM isn't there */
	    LQIO::DOM::Document* document = LQIO::DOM::Document::load( argv[optind], LQIO::DOM::Document::AUTOMATIC_INPUT, "", &io_vars, errorCode, false );
	    if ( !document ) {
		continue;
	    } 
	    Generate aModel( document, Flags::number_of_runs );
	    aModel.reparameterize();
	    
	    if ( output_file_name && strcmp( output_file_name, "-" ) == 0 ) {
		cout << aModel;
	    } else {
		LQIO::Filename filename;
		if ( output_file_name ) {
		    filename.generate( output_file_name );
		} else {
		    const char * suffix = 0;
		    if ( Flags::xml_output ) suffix = "lqnx";
		    else suffix = "lqn";
		    filename.generate( argv[optind], suffix );
		}
		filename.backup();		/* Overwriting input file. -- back up */

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
    }
    return 0;
}



