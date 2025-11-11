/*  -*- c++ -*-
 * $Id: lqn2csv.cc 17576 2025-11-10 18:11:11Z greg $
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
#include <filesystem>
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
#include <lqio/dom_document.h>
#include <lqio/dom_task.h>
#include "model.h"
#include "gnuplot.h"
#include "output.h"

int gnuplot_flag    = 0;
int no_header       = 0;
int debug_xml	    = 0;
size_t limit	    = 0;
int precision	    = 0;
size_t width        = 0;
size_t path_width   = 0;
bool transpose 	    = false;

const std::vector<struct option> longopts = {
    /* name */ /* has arg */ /*flag */ /* val */
    { "open-wait",                  required_argument, nullptr, 'a' }, 
    { "arrival-rate",		    required_argument, nullptr, 'A' },
    { "bounds",                     required_argument, nullptr, 'b' }, 
    { "calls",	                    required_argument, nullptr, 'c' },
    { "activity-calls",             required_argument, nullptr, 'C' },
    { "demand",		            required_argument, nullptr, 'd' },
    { "activity-demand",            required_argument, nullptr, 'D' },
    { "entry-utilization",          required_argument, nullptr, 'e' }, 
    { "entry-throughput",           required_argument, nullptr, 'f' }, 
    { "activity-throughput",        required_argument, nullptr, 'F' }, 
    { "hold-times",                 required_argument, nullptr, 'h' }, 
    { "solver-information",         no_argument,       nullptr, 'i' },
    { "join-delays",                required_argument, nullptr, 'j' }, 
    { "loss-probability",           required_argument, nullptr, 'l' },
    { "activity-loss-probability",  required_argument, nullptr, 'L' },
    { "processor-multiplicity",     required_argument, nullptr, 'm' },
    { "task-multiplicity",          required_argument, nullptr, 'n' },
    { "output",                     required_argument, nullptr, 'o' }, /* */
    { "processor-utilization",      required_argument, nullptr, 'p' }, 
    { "marginal-probabilities",     required_argument, nullptr, 'P' },
    { "processor-waiting",          required_argument, nullptr, 'q' }, 
    { "activity-processor-waiting", required_argument, nullptr, 'Q' }, 
    { "marginal-probability",       required_argument, nullptr, 'P' }, 
    { "task-replication",           required_argument, nullptr, 'r' }, 
    { "processor-replication",      required_argument, nullptr, 'R' }, 
    { "service",                    required_argument, nullptr, 's' }, 
    { "activity-service",           required_argument, nullptr, 'S' }, 
    { "task-throughput",            required_argument, nullptr, 't' }, 
    { "task-utilization",           required_argument, nullptr, 'u' }, 
    { "variance",                   required_argument, nullptr, 'v' }, 
    { "activity-variance",          required_argument, nullptr, 'V' }, 
    { "waiting",                    required_argument, nullptr, 'w' }, 
    { "activity-waiting",           required_argument, nullptr, 'W' }, 
    { "service-exceeded",           required_argument, nullptr, 'x' },
    { "activity-service-exceeded",  required_argument, nullptr, 'X' },
    { "think-time",	            required_argument, nullptr, 'z' },
    { "comment",	            no_argument,       nullptr, '#' },
    { "arguments",	            required_argument, nullptr, '@' },
    { "gnuplot",                    no_argument,       &gnuplot_flag, 1 },
    { "mva-steps",	            no_argument,       nullptr, 0x100+'S' },
    { "mva-waits",	            no_argument,       nullptr, 0x100+'W' },
    { "space",			    no_argument,       nullptr, 0x100+'x' },
    { "width",		            required_argument, nullptr, 0x100+'w' },
    { "precision",	            required_argument, nullptr, 0x100+'p' },
    { "limit",		            required_argument, nullptr, 0x100+'l' },
    { "no-header",                  no_argument,       &no_header, 1 },
    { "transpose",		    no_argument,       nullptr, 0x100+'t' },
    { "help",		            no_argument,       nullptr, 0x100+'h' },
    { "version",	            no_argument,       nullptr, 0x100+'v' },
    { "debug-xml",	            no_argument,       &debug_xml, 1 },
    { nullptr,                      0,                 nullptr, 0 }
};
std::string opts;

const static std::map<int,const std::string> help_str
{
    { 'a', "print open arrival waiting time for <entry>." }, 
    { 'A', "print open arrival rate for <entry> (independent variable)." },
    { 'b', "print throughput bound for <entry>." }, 
    { 'c', "print the number of requests from source <entry>, phase <n> to destination <entry> (independent variable)." }, 
    { 'C', "print the number of requests from source <task>, <activity> to destination <entry> (independent variable)." }, 
    { 'd', "print demand for <entry>, phase <n> (independent variable)." },
    { 'D', "print demand for <task>, <activity> (independent variable)." },
    { 'e', "print utilization for <entry>." }, 
    { 'f', "print throughput for <entry>." }, 
    { 'F', "print througput for <task>, <activity>." }, 
    { 'h', "Hold time." }, 
    { 'i', "print the solver used and its version number." },
    { 'j', "Join delay." }, 
    { 'l', "print asynchronous send drop probability from source <entry>, phase <n> to destination <entry>." }, 
    { 'L', "print asynchronous send drop probability from source <task>, <activity> to destination <entry>." }, 
    { 'm', "print processor multiplicity (independent variable)." },
    { 'n', "print task multiplicity (independent variable)." },
    { 'o', "write output to the file named <arg>." },
    { 'p', "print utilization for <processor>." }, 
    { 'P', "print out the marginal queue probabilities for the processor or task <entity>." },
    { 'q', "print waiting time at the processor for <entry>, phase <n>." }, 
    { 'Q', "print waiting time at the processor for <task>, <activity>." }, 
    { 'r', "print task replication (independent variable). " },
    { 'R', "print proceesor replication (independent variable). " },
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
    { '#', "print out the model comment field." },
    { 0x100+'S', "print out the number of times the MVA step() function was called."  },
    { 0x100+'W', "print out the number of times the MVA wait() function was called."  },
    { 0x100+'x', "Insert an empty entry." },
    { 0x100+'l', "Limit output to the first <arg> files read." },
    { 0x100+'w', "Set the width of the result columns to <arg>.  Suppress commas." },
    { 0x100+'p', "Set the precision for output to <arg>." },
    { 0x100+'t', "Transpose the output such that results are in rows and files are in columns." },
    { 0x100+'v', "Print out version number." },
    { 0x100+'h', "Print out this list." }
};

const static std::map<int,Model::Result::Type> result_type
{
    { 'a', Model::Result::Type::OPEN_ARRIVAL_WAIT      }, 
    { 'A', Model::Result::Type::OPEN_ARRIVAL_RATE      }, 
    { 'b', Model::Result::Type::THROUGHPUT_BOUND       }, 
    { 'c', Model::Result::Type::PHASE_CALLS            }, 
    { 'C', Model::Result::Type::ACTIVITY_CALLS         }, 
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
    { 'n', Model::Result::Type::TASK_MULTIPLICITY      }, 
    { 'p', Model::Result::Type::PROCESSOR_UTILIZATION  }, 
    { 'P', Model::Result::Type::MARGINAL_PROBABILITIES },
    { 'q', Model::Result::Type::PHASE_PROCESSOR_WAITING}, 
    { 'Q', Model::Result::Type::ACTIVITY_PROCESSOR_WAITING }, 
    { 'r', Model::Result::Type::TASK_REPLICATION       }, 
    { 'R', Model::Result::Type::PROCESSOR_REPLICATION  }, 
    { 's', Model::Result::Type::PHASE_SERVICE          }, 
    { 'S', Model::Result::Type::ACTIVITY_SERVICE       }, 
    { 't', Model::Result::Type::TASK_THROUGHPUT        }, 
    { 'u', Model::Result::Type::TASK_UTILIZATION       }, 
    { 'v', Model::Result::Type::PHASE_VARIANCE         }, 
    { 'V', Model::Result::Type::ACTIVITY_VARIANCE      }, 
    { 'w', Model::Result::Type::PHASE_WAITING          }, 
    { 'W', Model::Result::Type::ACTIVITY_WAITING       }, 
    { 'x', Model::Result::Type::PHASE_PR_SVC_EXCD      }, 
    { 'X', Model::Result::Type::ACTIVITY_PR_SVC_EXCD   }, 
    { 'z', Model::Result::Type::TASK_THINK_TIME	       },
    { '#', Model::Result::Type::COMMENT	       	       },
    { 'i', Model::Result::Type::SOLVER_VERSION         },
    { 0x100+'S', Model::Result::Type::MVA_STEPS	       },
    { 0x100+'W', Model::Result::Type::MVA_WAITS	       },
    { 0x100+'x', Model::Result::Type::NONE	       }
};

enum class Disposition { handle, ignore, fault };

struct max_strlen {
    max_strlen( Model::Mode mode ) : _mode(mode) {}
    size_t operator()( size_t l, std::filesystem::path name ) {
	if ( name.empty() ) return l;
	switch ( _mode ) {
	case Model::Mode::EXTENSION_ONLY: name = name.extension(); break;
	case Model::Mode::FILENAME_ONLY:  name = name.stem(); break;
	case Model::Mode::DIRECTORY_ONLY: name.remove_filename(); break;
	default: break;
	}
	return std::max( l, name.native().size() );
    }
private:
    const Model::Mode _mode;
};

struct filename_match {
    filename_match( const std::filesystem::path& filename ) : _filename(filename.filename()) {}
    bool operator()( const std::filesystem::path& filename ) { return _filename == filename.filename(); }
private:
    const std::filesystem::path _filename;
};
    
struct directory_match {
    directory_match( const std::filesystem::path& directory ) : _directory(directory) { _directory.remove_filename(); }
    bool operator()( std::filesystem::path directory ) { directory.remove_filename(); return directory == _directory; }
private:
    std::filesystem::path _directory;
};
    
struct dir_stem_match {
    dir_stem_match( const std::filesystem::path& path ) : _path(path) { _path.replace_extension(""); }	/* strip extention */
    bool operator()( std::filesystem::path path ) { path.replace_extension(""); return path == _path; }
private:
    std::filesystem::path _path;
};
    
std::vector<Model::Result::result_t> results;

static void process( std::ostream& output, int argc, char **argv, const std::vector<Model::Result::result_t>& results, size_t limit );
static void process_directory( const std::string& dirname, const Model::Process& );
static Model::Mode get_mode( int argc, char **argv, int optind );
static void fetch_arguments( const std::filesystem::path& filename, std::vector<Model::Result::result_t>& results );
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

    sscanf( "$Date: 2025-11-10 13:11:11 -0500 (Mon, 10 Nov 2025) $", "%*s %s %*s", copyrightDate );

    toolname = basename( argv[0] );
    opts = makeopts( longopts );	/* Convert to regular options */
    LQIO::io_vars.init( VERSION, toolname, nullptr );

    /* Scan for all '-@' and others */
    
    std::vector<std::string> files;
    
    for ( ;; ) {
	const int c = getopt_long( argc, argv, opts.c_str(), longopts.data(), nullptr );
	if ( c == EOF ) break;
	
	switch ( c ) {
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

	case ':':
	    usage();
	    exit( 1 );
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
    
    LQIO::DOM::Document::__debugXML = static_cast<bool>(debug_xml);
    
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

    path_width = (width == 1) ? 1 : (mode == Model::Mode::DIRECTORY) ? width : std::accumulate( &argv[optind], &argv[argc], 1, max_strlen( mode ) );
    
    std::vector<std::vector<Model::Value>> data;
    if ( gnuplot_flag ) {
	plot.preamble();
    } else if ( no_header == 0 ) {
	Model::Result::insertHeader( data, "Object", results, &Model::Result::getObjectType );
	Model::Result::insertHeader( data, "Name",   results, &Model::Result::getObjectName );
	Model::Result::insertHeader( data, "Result", results, &Model::Result::getTypeName );
    }

    /* For all files do... */

    try {
	if ( mode == Model::Mode::DIRECTORY ) {
	    process_directory( argv[optind], Model::Process( data, results, limit, mode, plot.getSplotXIndex() ) );
	} else {
	    std::for_each( &argv[optind], &argv[argc], Model::Process( data, results, limit, mode, plot.getSplotXIndex() ) );
	}

	if ( precision > 0 ) {
	    output.precision(precision);
	} 
	if ( transpose ) {
	    size_t columns = std::accumulate( data.begin(), data.end(), 0, []( size_t l, std::vector<Model::Value>& row ) { return std::max( l, row.size() ); } );
	    Output::columns( output, data, columns );
	} else {
	    Output::rows( output, data );
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
process_directory( const std::string& dirname, const Model::Process& process )
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
 * Return the mode to be used.  If there are a set of filenames, then strip the common components them on output.
 */
 
static Model::Mode
get_mode( int argc, char **argv, int optind )
{
    if ( argc - optind == 1 && std::filesystem::is_directory( argv[optind] ) ) return Model::Mode::DIRECTORY;
    else if ( std::all_of( &argv[optind+1], &argv[argc], dir_stem_match( argv[optind] ) ) ) return Model::Mode::EXTENSION_ONLY;
    else if ( std::all_of( &argv[optind+1], &argv[argc], directory_match( argv[optind] ) ) ) return Model::Mode::FILENAME_ONLY;
    else if ( std::all_of( &argv[optind+1], &argv[argc], filename_match( argv[optind] ) ) ) return Model::Mode::DIRECTORY_ONLY;
    else return Model::Mode::PATHNAME;
}



/*
 * Read each line and store in results.
 */

static void
fetch_arguments( const std::filesystem::path& filename, std::vector<Model::Result::result_t>& results )
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
	argv.at(0) = const_cast<char *>(filename.string().c_str());
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
	if ( c == 0 ) continue;		/* longopts done with &flag */

	/* Handle all result args (and result options --gnuplot,...) */
	std::map<int,Model::Result::Type>::const_iterator result = result_type.find( c );
	if ( result != result_type.end() ) {
	    if ( optarg != nullptr ) {
		results.emplace_back( optarg, result->second );
	    } else {
		results.emplace_back( "", result->second );
	    }
	} else {
	    /* Ignore non-results options with args. (-o, -a...) */
	    switch ( c ) {

	    case 0x100+'l':
		limit = strtol( optarg, &endptr, 10 );
		break;
	    
	    case 0x100+'p':
		precision = strtol( optarg, &endptr, 10 );
		break;
	    
	    case 0x100+'w':
		width = strtol( optarg, &endptr, 10 );
		break;
	    
	    case 0x100+'t':
		transpose = true;
		break;
	    
	    case 'o':
	    case 0x100+'h':
	    case 'v':
	    case '@':
		if ( disposition != Disposition::ignore ) {
		    throw std::invalid_argument( std::string( "invalid option -- " ) + static_cast<char>(c) );
		}
		break;

	    case ':':
		usage();
		exit( 1 );
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
    std::string opts = ":";	/* use : as error, rather than ? */

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
		case Model::Object::Type::ENTITY:	  s += "=<entity>"; break;
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
	} else if ( strcmp( opt->name, "debug-xml" ) == 0 ) {
	    std::cerr << "debug XML input.";
	}

	std::cerr << std::endl;
    }
}
