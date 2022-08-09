/*  -*- c++ -*-
 * $Id: lqn2csv.cc 15796 2022-08-08 20:04:28Z greg $
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
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iomanip>
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
size_t limit	    = 0;
int precision	    = 0;
int width	    = 0;

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
    { "think-time",	       required_argument, nullptr, 'z' },
    { "arguments",	       required_argument, nullptr, '@' },
    { "gnuplot",               no_argument,       &gnuplot_flag, 1 },
    { "limit",		       required_argument, nullptr, 0x100+'l' },
    { "no-header",             no_argument,       &no_header,    1 },
    { "precision",	       required_argument, nullptr, 0x100+'p' },
    { "width",		       required_argument, nullptr, 0x100+'w' },
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
    { 'z', "print think time for <task> (independent variable)" },
    { '@', "Read the argument list from <arg>.  --output-file and --arguments are ignored." },
    { 0x100+'h', "Print out this list." },
    { 0x100+'l', "Limit output to the first <arg> files read." },
    { 0x100+'w', "Set the width of the result columns to <arg>.  Suppress commas." },
    { 0x100+'p', "Set the precision for output to <arg>." },
    { 0x100+'v', "Print out version numbder." }
};

const static std::map<int,Model::Result::Type> result_type
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
    { 'z', Model::Result::Type::TASK_THINK_TIME	       }
};

enum class Disposition { handle, ignore, fault };

struct max_strlen {
    max_strlen( Model::Mode mode ) : _mode(mode) {}
    size_t operator()( size_t l, const char * const s ) {
	if ( s == nullptr ) return l;
	const char * p = std::strrchr( s, '/' );
	if ( _mode == Model::Mode::DIRECTORY_STRIP && p != nullptr ) {
	    return std::max( l, strlen( p + 1 ) );
	} else if ( _mode == Model::Mode::FILENAME_STRIP && p != nullptr ) {
	    return std::max( l, static_cast<size_t>(s - p) );
	} else {
	    return std::strlen( s );
	}
    }
private:
    const Model::Mode _mode;
};

struct filename_match {
    filename_match( const std::string& filename ) : _filename( filename.substr( filename.find_last_of( "/" ) ) ) {}
    bool operator()( const char * filename ) { return filename != nullptr && _filename != filename; }
private:
    const std::string _filename;
};
    
struct directory_match {
    directory_match( const std::string& directory ) : _directory( directory.substr( 0, directory.find_last_of( "/" ) ) ) {}
    bool operator()( const char * directory ) { return directory != nullptr && _directory != directory; }
private:
    const std::string _directory;
};
    
std::vector<Model::Result::result_t> results;

static void process( std::ostream& output, int argc, char **argv, const std::vector<Model::Result::result_t>& results, size_t limit );
static bool is_directory( const char * filename );
static void process_directory( std::ostream& output, const std::string& dirname, const Model::Process& );
static Model::Mode get_mode( int argc, char **argv, int optind );
static void fetch_arguments( const std::string& filename, std::vector<Model::Result::result_t>& results );
static void handle_arguments( int argc, char * argv[], Disposition, std::vector<Model::Result::result_t>& results );
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

    sscanf( "$Date: 2022-08-08 16:04:28 -0400 (Mon, 08 Aug 2022) $", "%*s %s %*s", copyrightDate );

    toolname = basename( argv[0] );
    opts = makeopts( longopts );	/* Convert to regular options */
    LQIO::io_vars.init( VERSION, toolname, nullptr );

    /* Scan for all '-@' and others */
    
    std::vector<std::string> files;
    
    for ( ;; ) {
	const int c = getopt_long( argc, argv, opts.c_str(), longopts.data(), nullptr );
	if ( c == EOF ) break;
	
	switch ( c ) {
	case 0: break;		/* flags not handled by case */

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
	    files.push_back( optarg );
	    break;
	}
    }

    /* Handle all -@ arguments */
    
    for ( const auto& filename : files ) {
	fetch_arguments( filename, results );
    }

    /* Now do everything at the command line.  optind should be set to the first file */
    
    try {
	handle_arguments( argc, argv, Disposition::ignore, results );
    }
    catch ( const std::invalid_argument& e ) {
	std::cerr << toolname << ": " << e.what() << "." << std::endl;
	usage();
	exit( 1 );
    }

    if ( gnuplot_flag ) {
	width = 0;
    }
    
    if ( optind == argc ) {
	std::cerr << toolname << ": arg count" << std::endl;
	exit ( 1 );
    }

    if ( !output_file_name.empty() ) {
	std::ofstream output;
	output.open( output_file_name, std::ios::out );
	if ( output ) {
	    process( output, argc, argv, results, limit );
	}
	output.close();
    } else {
	process( std::cout, argc, argv, results, limit );
    }
    exit( 0 );
}


/*
 *
 */

static void
process( std::ostream& output, int argc, char **argv, const std::vector<Model::Result::result_t>& results, size_t limit )
{
    extern int optind;
    const Model::Mode mode = get_mode( argc, argv, optind );
    
    Model::GnuPlot plot( output, output_file_name, results );

    /* Load results, then got through results printing them out. */

    const size_t filename_column_width = (width == 1) ? 1 : (mode == Model::Mode::DIRECTORY) ? width : std::accumulate( &argv[optind], &argv[argc], 1, max_strlen( mode ) );
    
    if ( gnuplot_flag ) {
	plot.preamble();
    } else if ( no_header == 0 ) {
	Model::Result::printHeader( output, "Object", results, &Model::Result::getObjectType, filename_column_width );
	Model::Result::printHeader( output, "Name",   results, &Model::Result::getObjectName, filename_column_width );
	Model::Result::printHeader( output, "Result", results, &Model::Result::getTypeName,   filename_column_width  );
    }

    /* For all files do... */

    try {
	if ( mode == Model::Mode::DIRECTORY ) {
	    process_directory( output, argv[optind], Model::Process( output, results, limit, filename_column_width, mode, plot.getSplotXIndex() ) );
	} else {
	    std::for_each( &argv[optind], &argv[argc], Model::Process( output, results, limit, filename_column_width, mode, plot.getSplotXIndex() ) );
	}

	if ( gnuplot_flag ) {
	    plot.plot();
	}
    }
    catch ( const std::runtime_error& e ) {
	std::cerr << toolname << ": " << e.what() << std::endl;
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
    /* look for foo-001.lqxo~001~, then for foo-001.lqxo, then foo.lqxo~00~,... (spex && print-interval, spex, print-interval). */

    static const std::vector<std::string> patterns = { "*-*.lqxo", "*-*.lqjo", "*.lqxo~*~", "*.lqjo~*~",  };

    glob_t dir_list;
    dir_list.gl_offs = 0;
    dir_list.gl_pathc = 0;
    int rc = -1;
    for ( std::vector<std::string>::const_iterator match = patterns.begin(); match != patterns.end() && dir_list.gl_pathc == 0 ; ++match ) {
	std::string pathname = dirname + "/" + *match;
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
 * Return the mode to be used.  If there are a set of filenames, and they are all the same, then strip them on output.
 */
 
static Model::Mode
get_mode( int argc, char **argv, int optind )
{
    if ( argc - optind == 1 && is_directory( argv[optind] ) ) return Model::Mode::DIRECTORY;
    else if ( std::all_of( &argv[optind+1], &argv[argc], directory_match( argv[optind] ) ) ) return Model::Mode::DIRECTORY_STRIP;
    else if ( std::all_of( &argv[optind+1], &argv[argc], filename_match( argv[optind] ) ) ) return Model::Mode::FILENAME_STRIP;
    else return Model::Mode::PATHNAME;
}



/*
 * Read each line and store in results.
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

	try {
	    handle_arguments( argc, argv.data(), Disposition::fault, results );
	}
	catch ( const std::invalid_argument& e ) {
	    std::cerr << toolname << ": File " << filename << ", line " << line_no << ": " << e.what() << "." << std::endl;
	    exit( 1 );
	}
    }

    input.close();
}


static void
handle_arguments( int argc, char * argv[], Disposition disposition, std::vector<Model::Result::result_t>& results )
{
#if __DARWIN_C_LEVEL >= 199209L
    extern int optreset;
    optreset = 1;
#endif
    extern int optind;
    optind = 1;				/* Reset getopt_long processing */

    /* Run the option processor.  */
    for ( ;; ) {
	char * endptr = nullptr;
	const int c = getopt_long( argc, argv, opts.c_str(), longopts.data(), nullptr );
	if ( c == EOF ) break;

	/* Handle all result args (and result options --gnuplot,...) */
	std::map<int,Model::Result::Type>::const_iterator result = result_type.find( c );
	if ( optarg != nullptr && result != result_type.end() ) {
	    results.emplace_back( optarg, result->second );
	} else {
	    /* Ignore non-results options with args. (-o, -a...) */
	    switch ( c ) {
	    case EOF:
		break;


	    case 0x100+'l':
		limit = strtol( optarg, &endptr, 10 );
		break;
	    
	    case 0x100+'p':
		precision = strtol( optarg, &endptr, 10 );
		break;
	    
	    case 0x100+'w':
		width = strtol( optarg, &endptr, 10 );
		break;
	    
	    case 'o':
	    case 0x100+'h':
	    case 'v':
	    case '@':
		if ( disposition == Disposition::ignore ) break;
		/* Fall through */
	    case '?':
		throw std::invalid_argument( std::string( "invalid option -- " ) + static_cast<char>(c) );
	    }

	    if ( endptr != 0 && *endptr != '\0' ) {
		throw std::invalid_argument( std::string( "invalid argument to -" ) + static_cast<char>(c) + "--" + optarg );
	    }
	}
    }
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
	std::map<int,Model::Result::Type>::const_iterator result = result_type.find( opt->val );
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
