/*  -*- c++ -*-
 * $Id: lqn2csv.cc 15274 2021-12-27 16:01:46Z greg $
 *
 * Command line processing.
 *
 * Copyright the Real-Time and Distributed Systems Group,
 * Department of Systems and Computer Engineering,
 * Carleton University, Ottawa, Ontario, Canada. K1S 5B6
 *
 * October, 2021
 *
 * ------------------------------------------------------------------------
 */

#include "config.h"
#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iostream>
#include <numeric>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <vector>
#include <errno.h>
#include <ctype.h>
#include <getopt.h>
#if HAVE_GLOB_H
#include <glob.h>
#endif
#if HAVE_LIBGEN_H
#include <libgen.h>
#endif
#include <unistd.h>
#include <sys/stat.h>
#include <lqio/dom_document.h>
#include <lqio/dom_task.h>
#include "model.h"
#include "gnuplot.h"

int gnuplot_flag    = 0;
int no_header       = 0;

const std::vector<struct option> longopts = {
    /* name */ /* has arg */ /*flag */ /* val */
    { "open-wait",             required_argument, nullptr, 'a' }, 
    { "bounds",                required_argument, nullptr, 'b' }, 
    { "demand",		       required_argument, nullptr, 'd' },
    { "activity-demand",       required_argument, nullptr, 'D' },
    { "entry-utilization",     required_argument, nullptr, 'e' }, 
    { "entry-throughput",      required_argument, nullptr, 'f' }, 
    { "activity-throughput",   required_argument, nullptr, 'F' }, 
    { "hold-times",            required_argument, nullptr, 'h' }, 
    { "join-delays",           required_argument, nullptr, 'j' }, 
    { "loss-probability",      required_argument, nullptr, 'l' },
    { "processor-multiplicity",required_argument, nullptr, 'm' },
    { "task-multiplicity",     required_argument, nullptr, 'n' },
    { "output",                required_argument, nullptr, 'o' },
    { "processor-utilization", required_argument, nullptr, 'p' }, 
    { "processor-waiting",     required_argument, nullptr, 'q' }, 
    { "request-rate",	       required_argument, nullptr, 'r' },
    { "activity-request-rate", required_argument, nullptr, 'R' },
    { "service",               required_argument, nullptr, 's' }, 
    { "activity-service",      required_argument, nullptr, 'S' }, 
    { "task-throughput",       required_argument, nullptr, 't' }, 
    { "task-utilization",      required_argument, nullptr, 'u' }, 
    { "variance",              required_argument, nullptr, 'v' }, 
    { "activity-variance",     required_argument, nullptr, 'V' }, 
    { "waiting",               required_argument, nullptr, 'w' }, 
    { "activity-waiting",      required_argument, nullptr, 'W' }, 
    { "service-exceeded",      required_argument, nullptr, 'x' }, 
    { "arguments",	       required_argument, nullptr, '@' },
    { "gnuplot",               no_argument,       &gnuplot_flag, 1 },
    { "no-header",             no_argument,       &no_header,    1 },
    { "help",		       no_argument,	  nullptr, 0x100+'h' },
    { "version",	       no_argument,	  nullptr, 0x100+'v' },
    { nullptr,                 0,                 nullptr, 0 }
};
std::string opts;

const static std::map<int,const std::string> help_str
{
    { 'a', "print open arrival waiting time for <entry>." }, 
    { 'b', "print throughput bound for <entry>." }, 
    { 'd', "print demand for <entry>, phase <n> (independent variable)." },
    { 'D', "print demand for <task>, <activity> (independent variable)." },
    { 'e', "print utilization for <entry>." }, 
    { 'f', "print throughput for <entry>." }, 
    { 'F', "print througput for <task>, <activity>." }, 
    { 'h', "Hold time." }, 
    { 'j', "Join delay." }, 
    { 'l', "print asynchronous send drop probability from source <entry>, phase <n> to destination <entry>." }, 
    { 'L', "print asynchronous send drop probability from source <task>, <activity> to destination <entry>." }, 
    { 'm', "print processor multiplicity (independent variable)." },
    { 'n', "print task multiplicity (independent variable)." },
    { 'o', "write output to the file named <arg>." },
    { 'p', "print utilization for <processor>." }, 
    { 'q', "print waiting time at the processor for <entry>, phase <n>." }, 
    { 'q', "print waiting time at the processor for <task>, <activity>." }, 
    { 'r', "print request rate from source <entry>, phase <n> to destination <entry> (independent variable)." }, 
    { 'R', "print request rate from source <task>, <activity> to destination <entry> (independent variable)." }, 
    { 's', "print service time for <entry>, phase <n>." }, 
    { 'S', "print service time for <task>, <activity>." }, 
    { 't', "print throughput for <task>." }, 
    { 'u', "print utilization for <task>." }, 
    { 'v', "print service time variance for <entry>, phase <n>." }, 
    { 'V', "print service time variance for <task>, <activity>." }, 
    { 'w', "print waiting time from source <entry>, phase <n> to destination <entry>." }, 
    { 'W', "print waiting time from source <task>, <activity> to destination <entry>." }, 
    { 'x', "print probability service time exceeded for <entry>, phase <n>." },
    { 'X', "print probability service time exceeded for <task>, <activity>." },
    { '@', "Read the argument list from <arg>.  --output-file and --arguments are ignored." },
    { 0x100+'h', "Print out this list." },
    { 0x100+'v', "Print out version numbder." }
};

const static std::map<char,Model::Result::Type> result_type
{
    { 'a', Model::Result::Type::OPEN_WAIT              }, 
    { 'b', Model::Result::Type::THROUGHPUT_BOUND       }, 
    { 'd', Model::Result::Type::PHASE_DEMAND	       }, 
    { 'D', Model::Result::Type::ACTIVITY_DEMAND	       }, 
    { 'e', Model::Result::Type::ENTRY_UTILIZATION      }, 
    { 'f', Model::Result::Type::ENTRY_THROUGHPUT       }, 
    { 'F', Model::Result::Type::ACTIVITY_THROUGHPUT    }, 
    { 'h', Model::Result::Type::HOLD_TIMES             }, 
    { 'j', Model::Result::Type::JOIN_DELAYS            }, 
    { 'l', Model::Result::Type::PHASE_PR_RQST_LOST     }, 
    { 'L', Model::Result::Type::ACTIVITY_PR_RQST_LOST  }, 
//  { 0,   Model::Result::Type::PHASE_UTILIZATION      },
    { 'm', Model::Result::Type::PROCESSOR_MULTIPLICITY }, 
    { 'p', Model::Result::Type::PROCESSOR_UTILIZATION  }, 
    { 'q', Model::Result::Type::PHASE_PROCESSOR_WAITING}, 
    { 'Q', Model::Result::Type::ACTIVITY_PROCESSOR_WAITING }, 
    { 'r', Model::Result::Type::PHASE_WAITING          }, 
    { 's', Model::Result::Type::PHASE_SERVICE          }, 
    { 'n', Model::Result::Type::TASK_MULTIPLICITY      }, 
    { 't', Model::Result::Type::TASK_THROUGHPUT        }, 
    { 'u', Model::Result::Type::TASK_UTILIZATION       }, 
    { 'v', Model::Result::Type::PHASE_VARIANCE         }, 
    { 'V', Model::Result::Type::ACTIVITY_VARIANCE      }, 
    { 'w', Model::Result::Type::PHASE_WAITING          }, 
    { 'x', Model::Result::Type::PHASE_PR_SVC_EXCD      }, 
    { 'X', Model::Result::Type::ACTIVITY_PR_SVC_EXCD   }, 
};
    
std::vector<Model::Result::result_t> results;

static void process( std::ostream& output, int argc, char **argv, const std::vector<Model::Result::result_t>& results );
static bool is_directory( const char * filename );
static void process_directory( std::ostream& output, const std::string& dirname, const Model::Process& );
static void fetch_arguments( const std::string& filename, std::vector<Model::Result::result_t>& results );
static void usage();
static std::string makeopts( const std::vector<struct option>& );

std::string toolname;
std::string output_file_name;

/*----------------------------------------------------------------------*/
/*			      Main line					*/
/*----------------------------------------------------------------------*/

int
main( int argc, char *argv[] )
{
    extern char *optarg;
    extern int optind;
    static char copyrightDate[20];

    sscanf( "$Date: 2021-10-02 09:33:15 -0400 (Sat, 02 Oct 2021) $", "%*s %s %*s", copyrightDate );

    toolname = basename( argv[0] );
    opts = makeopts( longopts );	/* Convert to regular options */

    for ( ;; ) {
	const int c = getopt_long( argc, argv, opts.c_str(), longopts.data(), nullptr );
	if ( c == EOF ) break;
	
	/* Find the option */
	std::map<char,Model::Result::Type>::const_iterator result = result_type.find( c );
	if ( optarg != nullptr && result != result_type.end() ) {
	    results.emplace_back( optarg, result->second );
	}
	switch ( c ) {
	case 0: break;

	case 'o':
	    output_file_name = optarg;
	    break;

	case 0x100+'h':
	    usage();
	    exit( 0 );
	    break;

	case 0x100+'v':
            std::cout << "Lqn2csv, Version " << VERSION << std::endl << std::endl;
            std::cout << "  Copyright " << copyrightDate << " the Real-Time and Distributed Systems Group," << std::endl;
            std::cout << "  Department of Systems and Computer Engineering," << std::endl;
            std::cout << "  Carleton University, Ottawa, Ontario, Canada. K1S 5B6" << std::endl << std::endl;
            break;
	    
	case '@':
	    fetch_arguments( optarg, results );
	    break;
	    
	case '?':
	    std::cerr << toolname << ": invalid argument -- " << argv[optind-1] << "." << std::endl; // 
            usage();
	    exit( 1 );
	}
    }

    if ( optind == argc ) {
	std::cerr << toolname << ": arg count" << std::endl;
	exit ( 1 );
    }

    if ( !output_file_name.empty() ) {
	std::ofstream output;
	output.open( output_file_name, std::ios::out );
	if ( output ) {
	    process( output, argc, argv, results );
	}
	output.close();
    } else {
	process( std::cout, argc, argv, results );
    }
    exit( 0 );
}


/*
 *
 */

static void
process( std::ostream& output, int argc, char **argv, const std::vector<Model::Result::result_t>& results )
{
    extern int optind;
    Model::GnuPlot plot( output, output_file_name, results );

    /* Load results, then got through results printing them out. */
    
    if ( gnuplot_flag ) {
	plot.preamble();
    } else if ( no_header == 0 ) {
	output << std::accumulate( results.begin(), results.end(), std::string("Object"), Model::Result::ObjectType ) << std::endl;
	output << std::accumulate( results.begin(), results.end(), std::string("Name"),   Model::Result::ObjectName ) << std::endl;
	output << std::accumulate( results.begin(), results.end(), std::string("Result"), Model::Result::TypeName   ) << std::endl;
    }

    /* For all files do... */

    if ( argc - optind == 1 && is_directory( argv[optind] ) ) {
	process_directory( output, argv[optind], Model::Process( output, results, plot.getSplotXIndex() ) );
    } else {
	std::for_each( &argv[optind], &argv[argc], Model::Process( output, results, plot.getSplotXIndex() ) );
    }

    if ( gnuplot_flag ) {
	plot.plot();
    }
}


/*
 * Search dirname for all possible parseable output file (except .p.,
 * for now), then run Model::Process::operator()() on this list.
 */

static void
process_directory( std::ostream& output, const std::string& dirname, const Model::Process& process )
{
#if HAVE_GLOB
    static const std::vector<std::string> patterns = { "-*.lqxo", "-*.lqjo", ".lqxo~*~", ".lqjo~*~" };

    /* if there is a / followed by a dot, take the stuff between the slash and the dot as filename
     * else if there is no slash, but a dot, take the stuff up to the dot as filename
     * else take it all as filename.
     */
    
    std::string filename = dirname;
    std::string::iterator back;
    for ( back = std::prev(filename.end()); back != filename.begin() && *back == '/'; --back ) {
	filename.erase(back);				/* Strip trailing slashes	*/
    }

    size_t i = filename.find_last_of( "/" );
    if ( i != std::string::npos ) {
	filename = filename.substr( i );		/* Strip everything up to and including last '/' */
    }
    filename.erase( filename.find_last_of( "." ) );	/* Strip suffix (if any)	*/

    glob_t dir_list;
    dir_list.gl_offs = 0;
    dir_list.gl_pathc = 0;
    int rc = -1;
    for ( std::vector<std::string>::const_iterator match = patterns.begin(); match != patterns.end() && dir_list.gl_pathc == 0 ; ++match ) {
	std::string pathname = dirname + "/" + filename + *match;
	rc = glob( pathname.c_str(), 0, NULL, &dir_list );
    }
	  
    if ( rc == GLOB_NOSPACE ) {
	std::cerr << toolname << ": not enough space for glob." << std::endl;
	return;
    } else if ( dir_list.gl_pathc == 0 ) {
	std::cerr << toolname << ": no matches for -*.lqxo, .lqxo-~*~" << std::endl;
	return;
    }
    
    std::for_each( &dir_list.gl_pathv[0], &dir_list.gl_pathv[dir_list.gl_pathc], process );
#else
    std::cerr << toolname << ": the directory option is not supported with this version." << std::endl;
#endif
}


/*
 * query where filename is a directory or not.
 */

static bool
is_directory( const char * filename )
{
    struct stat stat_buf;
    extern int errno;
    if ( stat( filename, &stat_buf ) < 0 ) {
	std::cerr << toolname << ": cannot stat " << filename << ", " << strerror( errno ) << std::endl;
	return false;
    } else {
	return S_ISDIR( stat_buf.st_mode );
    }
}


/*
 * Read each line and store in results 
 */

static void
fetch_arguments( const std::string& filename, std::vector<Model::Result::result_t>& results )
{
    std::ifstream input;
    input.open( filename, std::ios::in );
    if ( !input ) {
	std::cerr << toolname << ": cannot open " << filename << ", " << std::endl;
	return;
    }

    const int saved_optind = optind;		/* getopt is not reenterant... */
    size_t line_no = 1;
    for ( std::string str; std::getline( input, str ); line_no += 1 ) {

	/* Tokenize input line. */
	if ( str.empty() ) continue;
	const std::regex regex(" ");
	const std::vector<std::string> tokens( std::sregex_token_iterator(str.begin(), str.end(), regex, -1), std::sregex_token_iterator() );
	const int argc = tokens.size() + 1;

	/* Get pointers to tokens for longopt */
	std::vector<char *> argv( argc );
	argv.at(0) = const_cast<char *>(filename.c_str());
	for ( int i = 1; i < argc; ++i ) {
	    argv.at(i) = const_cast<char *>(tokens.at(i-1).c_str());
	}

	/* Run the option processor.  */
	optind = 1;				/* Reset getopt_long processing */
	const int c = getopt_long( argc, argv.data(), opts.c_str(), longopts.data(), nullptr );

	/* Handle all result args (and result options --gnuplot,...) */
	std::map<char,Model::Result::Type>::const_iterator result = result_type.find( c );
	if ( optarg != nullptr && result != result_type.end() ) {
	    results.emplace_back( optarg, result->second );
	    continue;
	}

	/* Ignore non-results options with args. (-o, -a...) */
	switch ( c ) {
	case EOF:
	    break;
	case '?':
	    std::cerr << toolname << ": File " << filename << ", line " << line_no << ": invalid argument -- " << str << "." << std::endl; // 
	    break;
	}
    }
    optind = saved_optind;
    input.close();
}


/*
 * Convert longopts to short string
 */

static std::string
makeopts( const std::vector<struct option>& longopts )
{
    std::string opts;

    for ( std::vector<struct option>::const_iterator opt = longopts.begin(); opt != longopts.end() && opt->name != nullptr; ++opt ) {
	if ( !isgraph( opt->val ) ) continue;
	opts += opt->val;
	if ( opt->has_arg != no_argument ) {
	    opts += ':';
	}
    }

    return opts;
}



static void
usage()
{
    std::cerr << "Usage: " << toolname << " [option] file..." << std::endl;
    std::cerr << "       " << toolname << " [option] directory" << std::endl;
    std::cerr << "Options:" << std::endl;
    for ( std::vector<struct option>::const_iterator opt = longopts.begin(); opt != std::prev( longopts.end() ); ++opt ) {

	if ( isgraph( opt->val ) ) {
	    std::cerr <<  "-" << static_cast<const char>(opt->val) << ",";
	} else {
	    std::cerr << "   ";
	}
	std::cerr << " --";

	std::string s = opt->name;
	std::map<char,Model::Result::Type>::const_iterator result = result_type.find( opt->val );
	if ( opt->has_arg == required_argument ) {
	    if ( result != result_type.end() ) {
		switch ( Model::Result::__results.at(result->second).type ) {
		case Model::Object::Type::ACTIVITY:       s += "=<task>,<activity>"; break;
		case Model::Object::Type::ACTIVITY_CALL:  s += "=<task>,<activity>,<entry>"; break;
		case Model::Object::Type::ENTRY:      	  s += "=<entry>"; break;
		case Model::Object::Type::JOIN:	          s += " Not implemented."; break;
		case Model::Object::Type::PHASE:          s += "=<entry>,<n>"; break;
		case Model::Object::Type::PHASE_CALL:     s += "=<entry>,<n>,<entry>"; break;
		case Model::Object::Type::PROCESSOR:      s += "=<processor>"; break;
		case Model::Object::Type::TASK:	          s += "=<task>"; break;
		default: break;
		}
	    } else {
		s += "=<arg>";
	    }
	}
	std::cerr << std::left << std::setw(40) << s;

	const std::map<int,const std::string>::const_iterator i = help_str.find( opt->val );
	if ( i != help_str.end() ) {
	    std::cerr << i->second; 
	} else if ( strcmp( opt->name, "gnuplot" ) == 0 ) {
	    std::cerr << "generate gnuplot ouptut.";
	} else if ( strcmp( opt->name, "no-header" ) == 0 ) {
	    std::cerr << "do not output header information.";
	}

	std::cerr << std::endl;
    }
}
