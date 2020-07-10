/* help.cc	-- Greg Franks Thu Mar 27 2003
 *
 * $Id: help.cc 13675 2020-07-10 15:29:36Z greg $
 */

#include "lqngen.h"
#include "help.h"
#include <iostream>
#include <iomanip>
#include <cstring>
#include <getopt.h>

class HelpManip {
public:
    HelpManip( std::ostream& (*f)(std::ostream&, const std::string& ), const std::string& s )
	: _f(f), _s(s) {}
private:
    std::ostream& (*_f)( std::ostream&, const std::string& );
    const std::string& _s;

    friend std::ostream& operator<<(std::ostream & os, const HelpManip& m ) 
	{ return m._f(os,m._s); }
};

static std::ostream& emph_str( std::ostream& output, const std::string& s );
static std::ostream& bf_str( std::ostream& output, const std::string& s );
static std::ostream& tt_str( std::ostream& output, const std::string& s );
static std::ostream& example_str( std::ostream& output, const std::string& s );

static inline HelpManip example( const std::string& s ) { return HelpManip( &example_str, s ); }
static inline HelpManip emph( const std::string& s )    { return HelpManip( &emph_str, s ); }
static inline HelpManip bf( const std::string& s )      { return HelpManip( &bf_str, s ); }
static inline HelpManip tt( const std::string& s )      { return HelpManip( &tt_str, s ); }

class HelpManip2 {
public:
    HelpManip2( std::ostream& (*f)(std::ostream&, const std::string&, const RV::RandomVariable& ),
		const std::string& opt, const RV::RandomVariable& rv )
	: _f(f), _opt(opt), _rv(rv) {}
private:
    std::ostream& (*_f)(std::ostream&, const std::string&, const RV::RandomVariable& );
    const std::string _opt;
    const RV::RandomVariable& _rv;

    friend std::ostream& operator<<(std::ostream& os, const HelpManip2& m )
	{ return m._f(os,m._opt,m._rv); }
};

static std::ostream& boilerplate_str( std::ostream&, const std::string&, const RV::RandomVariable& );
static inline HelpManip2 boilerplate( const std::string& option, const RV::RandomVariable& rv ) { return HelpManip2( &boilerplate_str, option, rv ); }

class HelpManip3 {
public:
    HelpManip3( std::ostream& (*f)(std::ostream&, const std::string&, const std::string&, const std::string& ),
		const std::string& b, const std::string& r, const std::string& e )
	: _f(f), _b(b), _r(r), _e(e) {}
private:
    std::ostream& (*_f)(std::ostream&, const std::string&, const std::string&, const std::string& );
    const std::string _b;
    const std::string _r;
    const std::string _e;

    friend std::ostream& operator<<(std::ostream& os, const HelpManip3& m )
	{ return m._f(os,m._b,m._r,m._e); }
};

static std::ostream& flag_str( std::ostream&, const std::string& , const std::string& , const std::string& );

static inline HelpManip3 flag( const std::string& b, const std::string& r="", const std::string& e="" ) { return HelpManip3( &flag_str, b, r, e ); }

class FlagManip {
public:
    FlagManip( std::ostream& (*f)(std::ostream&, const int ), const int i )
	: _f(f), _i(i) {}
private:
    std::ostream& (*_f)( std::ostream&, const int );
    const int _i;

    friend std::ostream& operator<<(std::ostream & os, const FlagManip& m ) 
	{ return m._f(os,m._i); }
};

static std::ostream& is_not_set_str( std::ostream&, const int );
static inline FlagManip is_not_set( const int i ) { return FlagManip( &is_not_set_str, i ); }

static bool flag_value( int c );


static const char * const distribution_type[] = {
    "discreet",		/* RV::RandomVariable::CONSTANT */
    "",			/* RV::RandomVariable::BOTH */
    "continuous",	/* RV::RandomVariable::CONTINUOUS */
    "discreet",		/* RV::RandomVariable::DISCREET */
    ""			/* RV::RandomVariable::PROBABILITY */
};

using namespace std;

void invalid_option( int c ) 
{
    cerr << io_vars.lq_toolname << ": Invalid option '";
    if ( (c & 0xff80) == 0 ) {
	cerr << "-" << static_cast<char>(c);
    } else {
	for ( unsigned int i = 0; options[i].name != 0; ++i ) {
	    if ( options[i].c == (0x03FF & c) ) {
		cerr << "--" << options[i].name;
		break;
	    }
	}
    }
    cerr << "'." << endl;
    help();
}

void invalid_argument( int c, const string& arg, const string& extra )
{
    cerr << io_vars.lq_toolname << ": Invalid argument to '";
    if ( (c & 0xff80) == 0 ) {
	cerr << "-" << static_cast<char>(c) << "' -- '" << arg;
    } else {
	for ( unsigned int i = 0; options[i].name != 0; ++i ) {
	    if ( options[i].c == (c & 0x03FF) ) {
		cerr << "'--" << options[i].name << "' -- '" << arg;
		break;
	    }
	}
    }
    cerr << "'.";
    if ( extra.size() ) {
	cerr << " " << extra;
    }
    cerr << endl; 
    help();
}


void help()
{
    cerr << "Try '" << io_vars.lq_toolname << " -H" << "' for more information." << endl;
}

/*
 * Print out usage string.
 */

void
usage()
{
    const string program_name = Flags::lqn2lqx ? "lqn2lqx" : "lqngen";

    cerr << "Usage: " << program_name << " [OPTION]... [FILE|DIRECTORY]" << endl;
    if ( Flags::lqn2lqx ) {
	cerr << "Convert an existing model file to LQNX or SPEX." << endl;
    } else {
	cerr << "Generate LQN model files." << endl;
    }
    cerr << "Options:" << endl;

    for ( unsigned int i = 0; options[i].name != 0; ++i ) {
	if ( (options[i].program == LQNGEN_ONLY && Flags::lqn2lqx) || (options[i].program == LQN2LQX_ONLY && !Flags::lqn2lqx) ) continue;
	string s;
#if HAVE_GETOPT_LONG
	if ( (options[i].c & 0xff00) != 0 ) {
	    s = "      --";
	} else {
	    s = " -";
	    s += options[i].c;
	    s += ",  --";
	}
	if ( (options[i].c & 0x0200) != 0 ) {
	    s += "[no-]";
	}
	s += options[i].name;
	if ( options[i].has_arg == required_argument || (options[i].has_arg == optional_argument && !Flags::lqn2lqx)) {
	    s += "=ARG";
	} else if ( options[i].has_arg == optional_argument ) {
	    s += "[=ARG]";
	}
	cerr << setw(40);
#else
	if ( (options[i].c & 0xff00) != 0 ) continue;
	s = " -";
	s += options[i].c;
	if ( options[i].has_arg == required_argument || options[i].has_arg == optional_argument ) {
	    s += "<ARG>";
	}
	cerr << setw(10);
#endif

	cerr.setf( ios::left, ios::adjustfield );
	cerr << s << options[i].msg;
	if ( (options[i].c & 0x0200) != 0 ) {
	    cerr << " (" << (flag_value(i) ? "on" : "off") << ")";
	}
	cerr << endl;
    }
    cerr << endl
	 << "For most parameter arguments, ARG can be [distribution:]a[:b].  The optional 'distribution'" << endl
	 << "overrides the current default distribution in use.  The 'a' argument is the mean value except if the" << endl
	 << "optional distribution is specified and takes two arguments (e.g, uniform:low:high)." << endl
	 << "If only the 'a' argument is present, then the mean value of the parameter is set using the default distribution." << endl;
    cerr << endl << "Examples:" << endl;
    if ( Flags::lqn2lqx ) {
	cerr << "To convert an existing model file to SPEX:" << endl
	     << "  lqn2lqx model.lqn" << endl << endl;
	cerr << "To convert an existing model file to SPEX, running two experiments and setting the service time:" << endl
	     << "  lqn2lqx -N2 -s2 model.lqn" << endl << endl;
	cerr << "To convert an existing model file to LQX, varying the service time at all entries by 1.5x:" << endl
#if HAVE_GETOPT_LONG
	     << "  lqn2lqx --lqx-output -sensitivity=1.5 --no-customers --no-request-rate model.lqn" << endl << endl;
#else
	     << "  lqn2lqx -Oxml -S1.5 model.lqn" << endl << endl;
#endif
    } else {
	cerr << "To generate a basic annotated input file with a single client and a single server:" << endl
	     << "  lqngen model.lqn" << endl << endl;
	cerr << "To generate two experiments with random service times uniformly distributed" << endl
	     << "between [0.5,1.5]: " << endl
	     << "  lqngen -N2 --uniform=1 -s1 model.lqn" << endl << endl;
	cerr << "To generate two XML experiment files with different random three-tier models" << endl
	     << "with two classes of customers and five tasks: " << endl
	     << "  lqngen -M2 -T5 -L2 -C2 -Oxml model.dir" << endl << endl;
    }
    exit( 1 );
}



/*
 * Make a man page :-)
 */

void
man()
{
    static const char * comm = ".\\\"";
    char date[32];
    time_t tloc;
    time( &tloc );

    const string program_name = Flags::lqn2lqx ? "lqn2lqx" : "lqngen";

#if defined(HAVE_CTIME)
    strftime( date, 32, "%d %B %Y", localtime( &tloc ) );
#endif

    cout << comm << " -*- nroff -*-" << endl
	 << ".TH " << program_name << " 1 \"" << date << "\"  \"" << VERSION << "\"" << endl;

    cout << comm << " $Id: help.cc 13675 2020-07-10 15:29:36Z greg $" << endl
	 << comm << endl
	 << comm << " --------------------------------" << endl;

    cout << ".SH \"NAME\"" << endl
	 << program_name;
    if ( Flags::lqn2lqx ) {
	cout << " \\- generate layered queueing network models from an existing model file.";
    } else {
	cout << " \\- generate layered queueing network models.";
    }
    cout << endl;

    cout << ".SH \"SYNOPSIS\"" << endl 
	 << ".br" << endl
	 << ".B lqngen" << endl
	 << "[\\fIOPTION \\&.\\|.\\|.\\fP]" << endl;
    cout << "[" << endl
	 << "FILE" << endl
	 << "]" << endl;

    cout << ".SH \"DESCRIPTION\"" << endl;
    if ( Flags::lqn2lqx ) {
	cout << bf( "lqn2lqx" ) << " is used to convert an LQN input file to use either SPEX or LQX to control execution." << endl
	     << "All constant input parameters are converted to variables initialized with the values from the original" << endl
	     << "input file.  Existing variables within the input file are not" << endl
	     << "modified.  Note however, that variables initialized in an XML input file with LQX are " << bf("not") << " initialized" << endl
	     << "in the converted output file because the LQX program must be executed in order to do so.  To convert input formats without the conversion of parameters to"  << endl
	     << "variables, use " << bf( "lqn2ps" ) << "(1) with " << bf( "--format" ) << "=" << emph( "xml" ) << " or " << emph( "lqn" ) 
	     << ")." << endl;
	cout << ".PP" << endl
	     << bf( "lqn2lqx" ) << " reads its input from " << emph( "filename" ) << ", specified at the" << endl
	     << "command line if present, or from the standard input otherwise.  Output" << endl
	     << "for an input file " << emph( "filename" ) << " specified on the command line will be" << endl
	     << "placed in the file " << emph( "filename.ext" ) << ", where " << emph( ".ext" ) << " is " << emph( "xlqn" ) << endl
	     << "for SPEX conversion, and " << emph( "lqnx" ) << "for LQX conversion. "<< endl
	     << "If the output file name is the same as the input file name, " << endl
	     << "the output is written back to the original file name." << endl
	     << "The original file is renamed to " << emph( "filename.ext~" ) << endl
	     << "Output can be directed to a new file with the " << bf( "\\-\\-output" ) << " option." << endl
	     << "If several input files are given, then each is treated as a separate model and" << endl
	     << "output will be placed in separate output files.";

    } else {
	cout << bf( "lqngen" ) << " is used to create one or more LQN input files.  If no " << emph( "FILE" ) << " is" << endl
	     << "specified, the model will be output to " << emph( "stdout" ) << ".  If no options" << endl
	     << "are specified, a simple annotated model consisting of one reference task, one" << endl
	     << "serving task and their respective processors is produced.  " << endl
	     << "To create a random model with exactly " << emph( "n" ) << " tasks, use " << flag( "\\-\\-automatic", "=", "n" ) << " (see options below)." << endl
	     << "If " << emph( "FILE" ) << " exists, it will be over written with a new model file.  To convert a model" << endl
	     << "to use either SPEX or LQX, use " << bf( "lqn2lqx" ) << "(1). or the " << flag( "\\-\\-transform" ) << " option."  << endl;
	cout << ".PP" << endl
	     << "The size of a model is controlled by setting the number of server-task layers with " << flag( "\\-L", "", "n" ) << ", " << endl
	     << "tasks with " << flag( "\\-T", "", "n" ) << ", " << endl
	     << "processors with " << flag( "\\-T", "", "n" ) << endl
	     << "and customers with " << flag( "\\-C", "", "n" ) << " where " << emph("n") << " is greater than or equal to 1." << endl
	     << "By default, tasks are added starting from the reference tasks and working down ensuring that each layer has" << endl
	     << "the same number of tasks.  The structure of the model can be changed using one of" << endl
	     << flag( "\\-\\-fat" ) << ", " << flag( "\\-\\-pyramid" ) << ", " << flag( "\\-\\-hour-glass" ) << " or " << endl
	     << flag( "\\-\\-random" ) << "." << endl;
	cout << ".PP" << endl
	     << "All parameters are initialized to 1 by default.  To change the default value or to randomize the values, use one or all of" << endl
	     << flag( "\\-\\-customers", "=", "n" ) << ", " << flag( "\\-\\-processor-multiplicity", "=", "n" ) << ", " << flag( "\\-\\-service-time", "=", "n.n" ) << ", " << endl
	     << flag( "\\-\\-task-multiplicity", "=", "n" ) << ", " << flag( "\\-\\-request-rate", "=", "n.n" ) << " and " << flag( "\\-\\-think-time", "=", "n.n" ) << "." << endl
	     << "Two different random number generators (including " << emph( "constant" ) << ") are used, one for integer values to set multiplicities, and one for continuous values to set " << endl
	     << "service times and request rates.  The default continuous random number distribution is " << emph( "Gamma(0.5,2)" ) << " (Erlang 2 with a mean value of 1)." << endl
	     << "The default discreet random number distribution is " << emph( "Poisson(1)" ) << " (Poisson with a mean of 1 starting from 1, not zero)." << endl
	     << "An optional " << emph( "ARG" ) << " is used to set the spread or shape parameter for the generator, not the mean value." << endl
	     << "The random number generator option must appear before the parameter option" << endl
	     << "and different random number generators can be used for each of the parameter types." << endl
	     << "Finally, The various random number generators are described below.  " << endl;
	cout << ".PP" << endl
	     << "To create a simple model with with LQX or SPEX code, use" << endl
	     << bf( "\\-\\-lqx-output" ) << " or " << bf( "\\-\\-spex-output" ) << " respectively." << endl
	     << "In this case, all scalar parameters in the input file are replaced with variables." << endl;
	cout << ".PP" << endl
	     << "Two choices exist to create multiple models:" << flag( "\\-\\-experiments", "=", "n" ) << " and " << flag( "\\-\\-models", "=", "n" )  << "." << endl
	     << "For the first case, SPEX arrays or LQX " << tt( "for" ) << " loops are created to run one model file with multiple values."  << endl
	     << "For the second case, different model files are created in a subdirectory called " << emph( "filename.d" ) << " where " << emph( "fileanme" ) << endl
	     << "is specified on the command line.  Randomness in the input specification is required for this option." << endl;
    }

    cout << endl << ".SH \"OPTIONS\"" << endl;
    for ( unsigned int i = 0; options[i].name; ++i ) {
	if ( (options[i].program == LQNGEN_ONLY && Flags::lqn2lqx) || (options[i].program == LQN2LQX_ONLY && !Flags::lqn2lqx) ) continue;

	ios_base::fmtflags oldFlags = cout.flags();
	streamsize         oldPrec  = cout.precision();
	char               oldFill  = cout.fill();

	cout << ".TP" << endl;
	cout << "\\fB";
	if ( (options[i].c & 0xff00) == 0 ) {
	    cout << "\\-";
	    cout << static_cast<char>(options[i].c) << "\\fR";
	    if ( options[i].name ) {
		cout << ", \\fB";
	    }
	}
	if ( options[i].name ) {
	    cout << "\\-\\-";
	    if ( (options[i].c & 0x0200) != 0 ) {
		cout << "[no-]";
	    }
	    cout << options[i].name;
	}
	if ( options[i].has_arg == required_argument || (options[i].has_arg == optional_argument && !Flags::lqn2lqx)) {
	    cout << "=\\fI" << "ARG"; // options[i].arg;
	} else if ( options[i].has_arg == optional_argument ) {
	    cout << "[=\\fI" << "ARG]"; // options[i].arg;
	}
	cout << "\\fR" << endl;

	/* Description goes here... */
	switch ( options[i].c ) {
	case 0x100+'%':
	    cout << "Set the model comment to " << emph( "ARG" ) << ".  By default, the comment is set to" << endl
		 << "the command line options of the invocation of " << bf( program_name ) << "." << endl;
	    break;

	case 0x100+'#':
	    cout << "Set the total number of customers to " << emph( "ARG" ) << "." << endl
		 << boilerplate( "total-customers", *discreet_default ) << endl
		 << "This option cannot be used with " << flag( "\\-\\-customers", "=", "n" ) << "." << endl;
	    break;

	case 'A':
	    cout << "Create a model with exactly " << emph( "ARG" ) << " tasks.  The number of entries of a task" << endl
		 << "is a random variable with a mean of 1.2.  This value can be changed using " << flag( "\\-e", "", "n" ) << "." << endl
		 << "The number of clients, layers, processors and clients" << endl
		 << "is a random variable with a mean of the square root of " << emph( "ARG" ) << "." << endl
		 << "The processor and task multiplicity is a random variable with a mean of 1.5." << endl
		 << "Multiplicities for processors and tasks can be changed using " << flag( "\\-p", "", "n" ) << " and " << endl
		 << flag( "\\-m", "", "n" ) << " respectively.  Phase service times, rendezvous rates, and client think times " << endl
		 << "are all random variables with a mean of 1.0. These values can be changed using "  << endl
		 << flag( "\\-s", "", "n" ) << ", " << flag( "\\-y", "", "n" ) << " and " << flag( "\\-z", "","n" ) << " respectively." << endl;
	    break;
	    
	case 0x100+'A':
	    cout << "Create a model with a ``pyramid'' shape, that is there are more serving" << endl
		 << "tasks at the bottom of the model than at the top.  The default is to" << endl
		 << "create a model with tasks randomly distributed among the layers." << endl;
	    break;

	case 0x100+'b':
	    cout << "Assign processors deterministically from left to right, i.e., the first group of tasks are assigned processor 1, the next set gets processor 2, etc.." << endl;
	    break;

	case 0x100+'B':
	    cout << "Use a Binomial distribution for all subsequent options that use a" << endl
		 << "discreet random variable generator." << endl;
	    break;
	    
	case 0x200+'a':
	    cout << "Annotate LQN-type input files with syntax help.  This option has no effect for" << endl
		 << "XML output.  The default is to " << is_not_set( i ) << "annotate LQN model files." << endl;
	    break;

	case 'c':
	    cout << "Set the " << emph( "mean" ) << " number of customers at each of the client reference tasks to " << emph( "ARG" ) << "." << endl
		 << boilerplate( "customers", *discreet_default ) << endl;
	    break;
	    
	case 0x100+'c':
	    cout << "The default operation is to convert all constants in the input file into variables." << endl
		 << "Do not convert constant parameters for reference task multiplicites (customers)" << endl
		 << "to variables." << endl;
	    break;
	    
	case 'C':
	    cout << "Create " << emph( "ARG" ) << " client " << emph( "(reference)" ) << " tasks.  " << endl
		 << boilerplate( "clients", *constant_default ) << endl
		 << "Use " << flag( "\\-\\-customers" ) << " to specify the average number of customers (copies) at each client task." << endl;
	    break;

	case 0x100+'C':
	    cout << "Use " << emph( "constant" ) << " values for all subsequent parameters." << endl;
	    break;

	case 'd':	/* */
	    cout << "Set the probability that a processor is a delay server to " << emph( "ARG" ) << "." << endl
		 << "Tasks are not affected." << endl;
	    if ( Flags::lqn2lqx ) {
		cout << "All processors that have constant multiplicities are eligible for conversion to infinite servers." << endl
		     << "Processors whose multiplicity is set using a variable in the input file are never changed." << endl;
	    }
	    break;
	    
	case 0x100+'d':
	    cout << "Assign processors deterministically from top to bottom, i.e., task 1 gets processor 1, task 2 gets processor 2, etc." << endl;
	    break;
	    
	case 'E':
	    cout << "Create an average of " << emph( "ARG" ) << " entries on each serving task." << endl
		 << boilerplate( "entries", *discreet_default ) << endl
		 << "This number must not be less than one." << endl;
	    break;
	    
	case 0x100+'E':	/* Deterministic task layering */
	    cout << "Deterministically add tasks from top to bottom.  The first task is called by it's immediate client." << endl
		 << "Any additional entries, (see " << flag( "\\-E" ) << ") can be called by any higher-level task. " << endl;
	    break;
	    
	case 0x0200+'E':
	    cout << "Insert LQX code or SPEX observation variables to output the solver's ELAPSED time." << endl
		 << "The default is to " << is_not_set( i ) << "insert observation variables." << endl;
	    break;
	    
	case 0x100+'F':
	    cout << "Create a model with a ``fat'' shape, that is there are more serving" << endl
		 << "tasks in the middle of the model than at either the top or the bottom." << endl
		 << "The default is to create a model with tasks randomly distributed among" << endl
		 << "the layers." << endl;
	    break;

	case 0x200+'f':
	    cout << "Insert LQX code or SPEX observation variables to output task throughput for all ``intersting'' tasks." << endl
		 << "Interesting tasks are those which might have contention present."  << endl
		 << "The default is to " << is_not_set( i ) << "insert observation variables." << endl;
	    break;

	case 'G':
	    cout << "Set the probability that a processor is using Completely Fair Scheduling to " << emph( "ARG" ) << endl
		 << "provided that the processor is not an infinite server and that it provicdes service" << endl
		 << "to more than one task." << endl
		 << "See also " << flag( "\\-g" ) << "." << endl;
	    break;
	    
	case 'g':
	    cout << "Set the share of the first group running on the processor to " << emph( "ARG" ) << "." << endl
		 << "The share for a group is based on the Beta distribution." << endl;
	    break;
	    
	case 0x100+'G':
	    cout << "Set the continuous random variable generator to use a Gamma" << endl
		 << "distribution with a shape parameter of " << emph( "ARG" ) << "  for any subsequent" << endl
		 << "flag that uses a continuous distribution.  Integer values of" << endl
		 << emph( "ARG" ) << " will generate random numbers with an Erlang distribution.  A" << endl
		 << "shape value of 1 will generate random numbers with an Exponential" << endl
		 << "distribution."  << endl;
	    break;

	case 'H':
	    cout << "Print out a brief help summary and exit." << endl;
	    break;

	case 0x100+'H':		/* Hour glass */
	    cout << "Generate a model with more tasks at both the top and bottom than in the middle."
		 << endl;
	    break;

	case 'i':	/* */
	    cout << "Set the probability that a server task is infinite to " << emph( "ARG" ) << "." << endl
		 << "Processors are not affected." << endl;
	    if ( Flags::lqn2lqx ) {
		cout << "All tasks that have constant multiplicities are eligible for conversion to infinite servers." << endl
		     << "Tasks whose multiplicity is set using a variable in the input file are never changed." << endl;
	    }
	    break;
	    
	case 0x200+'i':
	    cout << "When generating SPEX or LQX output, either include or not include in the output the value of the parameters that change" << endl
		 << "as a result of executing the program.  Constant parameters are not included."  << endl
		 << "The default is to " << is_not_set( i ) << "include the parameters." << endl;
	    break;
	    
	case 0x200+'I':
	    cout << "Insert LQX code or SPEX observation variables to output the number of solver iterations." << endl
		 << "The default is to " << is_not_set( i ) << "insert observation variables." << endl;
	    break;

	case 'L':
	    cout << "Create " << emph( "ARG" ) << " layers of server tasks." << endl
		 << boilerplate( "layers", *constant_default ) << endl
		 << "The total number of layers in the model will be " << emph( "ARG+2" ) << " because one layer is used for" << endl
		 << "client tasks, and one layer will be used for one or more processors." << endl;
	    break;

	case 0x100+'L':
	    cout << "Use ``long'' names such as " << tt( "Processor0" ) << ", " << tt( "Client0" ) << ", " << tt( "Task0" ) << ", and " << tt( "Entry0" )
		 << ", rather than short names such as " << tt( "p0" ) << ", " << tt( "c0" ) << ", " << tt( "t0" ) << ", and " << tt( "e0" ) << endl;
	    break;

	case 'M':	/* LQNGEN only */
	    cout << "Generate " << emph( "ARG" ) << " model files, provided some randomness is present" << endl
		 << "in layer creation (see " << bf( "\\-\\-fat" ) << ", " << bf( "\\-\\-funnel" ) << ")." << endl
		 << "With this option, the file name arguments are treated as directory names, and the " << endl
		 << "generated model files are named " << tt( "case-1.lqn" ) << ", " << tt( "case-2.lqn" ) << ", etc." << endl;
	    break;

	case 0x100+'M':
	    cout << "Generate the manual page and send the output to " << emph( "stdout" ) << "." << endl;
	    break;

	case 0x200+'M':
	    cout << "Insert LQX code or SPEX observation variables to output the number of calls to step()." << endl
		 << "The default is to " << is_not_set( i ) << "insert observation variables." << endl;
	    break;

	case 'N':
	    cout << "Generate " << emph( "ARG" ) << " experiments within one model file. The experiments" << endl
		 << "will be run using either SPEX (for LQN input), or LQX (for XML input)." << endl
		 << "This option will turn on either " << bf( "\\-\\-spex-output" ) << ", or " << bf( "\\-\\-lqx-output" ) << endl
		 << "depending on the output format." << endl;
	    break;

	case 0x100+'n':
	    cout << "Do not convert constant values for customers, processor and task multiplicities, service and think times and request" << endl
		 << "rates into variables.  Observation variables are not affected.  " << endl;
	    break;
	    
	case 0x100+'N':
	    cout << "Set the continuous random variable generator to use a Normal" << endl
		 << "distribution with a standard deviation of " << emph( "ARG" ) << " for any subsequent" << endl
		 << "flag that uses a continuous distribution." << endl;
	    break;

	case 'o':	/* LQN2LQX */
	    cout << "Redirect output to the file named " << emph( "ARG" ) << ".  If " << emph( "ARG" ) << " is " << bf( "-" ) << "," << endl
		 << "output is redirected to " << emph( "stdout" ) << ".  This option cannot be used with " << bf( "\\-\\-models" ) << "." << endl;
	    break;
	    
	case 0x100+'o':
	    cout << "Do not insert any LQX code or spex observation variables." << endl;
	    break;
	    
	case 'O':
	    cout << "Set the output file format, " << emph( "ARG" ) << ", to: " << emph( "xml" ) << ", " << emph( "lqn" ) << "." << endl;
	    break;

	case 'p':
	    cout << "Set the " << emph( "mean" ) << " processor multiplicity for each processor to " << emph( "ARG" ) << "." << endl
		 << boilerplate( "processor-multiplicity", *discreet_default ) << endl;
	    break;

	case 0x100+'p':
	    cout << "The default operation is to convert all constants in the input file into variables." << endl
		 << "Do not convert constant parameters for processor multiplicities to variables." << endl;
	    break;
	    
	case 'Q':
	    cout << "Generate a model which can be solved by LQNS as a conventional queueing network with a maximum of " << emph( "ARG" ) << " customers." << endl
		 << "By default, a queuing network similar to experiment 1 of Chandy and Neuse for Linearizer" << endl
		 << "will be created with 1 to 0.10*ARG classes, 0.10*ARG to 0.20*ARG stations and 0.10*ARG to ARG customers," << endl
		 << "using a uniform distribution.  Stations will have a 0.05 probability of being a delay queue." << endl
		 << "Set the number of customers with " << flag( "\\-\\-total-customers" )
		 << ", the number of customer classes with " << flag( "\\-\\-clients" )
		 << ", the number of stations with " << flag( "\\-\\-tasks" )
		 << ", and the average number of stations visited by a client with " << flag( "\\-\\-outgoing-requests" )
		 << "." << endl;
	    break;
	    
	case 0x200+'q':
	    cout << "Insert a pragma " << emph( "ARG" ) << " into all " << (Flags::lqn2lqx ? "translated" : "generated files.") << endl
		 << "This option can be repeated for multiple pragmas." << endl;
	    if ( Flags::lqn2lqx ) {
		cout << "If the pragma was already present in the input file, it is reset to the new value." << endl
		     << flag( "\\-\\-no\\-pragma" ) << " will remove all existing pragmas." << endl;
	    }
	    break;
	    
	case 'P':
	    cout << "Create  " << emph( "ARG" ) << " processors in each model file." << endl
		 << boilerplate( "processor", *constant_default ) << endl;
	    break;

	case 0x100+'P':
	    cout << "Use a Poisson distribution for all subsequent options that use a" << endl
		 << "discreet random variable generator.  The distribution is shifted right by one so that" << endl
		 << "the lower bound on generated values is always one.  Mean values are adjusted accordingly." << endl;
	    break;

	case 0x100+'R':
	    cout << "Choose a random number of tasks at each layer." << endl;
	    break;
	    
	case 0x200+'r':
	    cout << "Insert LQX code or SPEX observation variables to output entry service (residence) time." << endl
		 << "The default is to " << is_not_set( i ) << "insert observation variables." << endl;
	    break;

	case 's':
	    cout << "Set the " << emph( "mean" ) << " phase service time to " << emph( "ARG" ) << "."  << endl
		 << boilerplate( "service-time", *continuous_default ) << endl;
	    break;

	case 0x100+'s':
	    cout << "The default operation is to convert all constants in the input file into variables." << endl
		 << "Do not convert constant parameters for phase service times to variables." << endl;
	    break;
	    
	case 'S':
	    cout << "Create a factorial expermiment varying all non-reference task service times by a factor of plus or minus "
		 << emph( "ARG" ) << ".  This option precludes the use of " <<  flag( "\\-N" ) << "." << endl;
	    break;
		      
	case 0x100+'S':
	    cout << "Generate SPEX control code for LQN output files.  Variables will be" << endl
		 << "created for all parameters set by the options above and will be" << endl
		 << "initialized using the current random number generator.  If" << endl
		 << flag( "\\-\\-experiments", "=", "ARG" ) << " is also used, loop code will be" << endl
		 << "produce to generate " << emph( "ARG" ) << " runs.  This option will enable LQN" << endl
		 << "output." << endl;
	    break;

	case 0x200+'S':
	    cout << "Insert LQX code or SPEX observation variables to output the solver's SYSTEM CPU time." << endl
		 << "The default is to " << is_not_set( i ) << "insert observation variables." << endl;
	    break;

	case 't':
	    cout << "Set the " << emph( "mean" ) << " task multiplicity for each task to " << emph( "ARG" ) << "." << endl
		 << boilerplate( "task-multiplicity", *discreet_default ) << endl;
	    if ( Flags::lqn2lqx ) {
		cout << "Tasks in the input file which are infinite servers may be converted to fixed-rate or multi-servers." << endl;
	    }
	    break;

	case 0x100+'t':
	    cout << "The default operation is to convert all constants in the input file into variables." << endl
		 << "Do not convert constant parameters for task multiplicities to variables." << endl;
	    break;
	    
	case 'T':
	    cout << "Create  " << emph( "ARG" ) << " tasks in each model file." << endl
		 << boilerplate( "task", *constant_default ) << endl
		 << "The number of tasks must be greater than or equal to the number of layers." << endl;
	    break;

	case 0x100+'T':
	    cout << "Transform the input model, rather than creating it (i.e., run the program as " << bf( "lqn2lqx" ) << "(1).)"
		 << endl;
	    break;
	    
	case 0x100+'U':
	    cout << "Use a uniform distribution for all subsequent options that use a" << endl
		 << "discreet or continuous random variable generator.  The spread of the" << endl
		 << "distribution is set to " << emph( "ARG" ) << " although this value will be" << endl
		 << "ignored if a mean value is set.  The lower bound for continuous distributions" << endl
		 << "is zero.  For discreet distributions, the lower bound is one.  The upper bound" << endl
		 << "for either distribution is set to twice the mean plus the lower bound." << endl;
	    break;

	case 0x200+'u':
	    cout << "Insert LQX code or SPEX observation variables to either observe or not observe processor utilization for all ``interesting'' processors." << endl
		 << "Interesting processors are those which might have contention present."  << endl
		 << "The default is to " << is_not_set( i ) << "observe processor utilization." << endl;
	    break;
	
	case 0x200+'U':
	    cout << "Insert LQX code or SPEX observation variables to output the solver's USER CPU time." << endl
		 << "The default is to " << is_not_set( i ) << "insert observation variables." << endl;
	    break;

	case 'v':
	    cout << "Verbose output. List the actual number of Clients, Server Tasks, Processors, Entries and Calls created." << endl;
	    break;

	case 'V':
	    cout << "Print the version number of " << program_name << "." << endl;
	    break;
	    
	case 0x100+'V':
	    cout << "Create a model with a ``funnel'' shape, that is there are more serving" << endl
		 << "tasks at the top of the model than at the bottom.  The default is to" << endl
		 << "create a model with tasks randomly distributed among the layers." << endl;
	    break;

	case 0x200+'w':
	    cout << "Insert LQX code or SPEX observation variables to either observe or not observe the waiting time between phases and entries." << endl
		 << "The default is to " << is_not_set( i ) << "observe waiting time." << endl;
	    break;
	    
	case 0x200+'W':
	    cout << "Insert LQX code or SPEX observation variables to output the number of calls to wait()." << endl
		 << "The default is to " << is_not_set( i ) << "insert observation variables." << endl;
	    break;

	case 0x100+'x':
	    cout << "Output the input model in eXtensible Markup Language (XML)." << endl;
	    break;

	case 0x100+'X':
	    cout << "Generate LQX control code for XML output files.  Variables will be" << endl
		 << "created for all parameters set by the options above and will be" << endl
		 << "initialized using the current random number generator.  If" << endl
		 << flag( "\\-\\-experiments", "=", "ARG" ) << " is also used, loop code will be" << endl
		 << "produced to generate " << emph( "ARG" ) << " runs.  This option will enable XML output." << endl;
		break;

	case 'y':
	    cout << "Set the mean rendezous (synchronous call) rate to " << emph( "ARG" ) << ".  " << endl
		 << boilerplate( "request-rate", *continuous_default ) << endl;
	    break;

	case 0x100+'y':
	    cout << "The default operation is to convert all constants in the input file into variables." << endl
		 << "Do not convert constant parameters for request rates to variables." << endl;
	    break;
	    
	case 'Y':
	    cout << "Generate an average of " << emph( "ARG" ) << " outgoing requests from each entry." << endl
		 << boilerplate( "outgoing-requests", *continuous_default ) << endl
		 << "Connections are made at random from a higher level task to a lower level task." << endl
		 << "So that all tasks are reachable, this number must not be less than one." << endl;
	    break;
	    
	case 'z':
	    cout << "Set the mean think time at reference tasks to " << emph( "ARG" ) << "." << endl
		 << boilerplate( "think-time", *continuous_default ) << endl;
	    break;

	case 0x100+'z':
	    cout << "The default operation is to convert all constants in the input file into variables." << endl
		 << "Do not convert constant parameters for reference task (customer) think times to variables." << endl;
	    break;
	    
	case 0x100+'1':
	    cout << "Set the model convergence limit to " << emph( "ARG" ) << ".  By default, the" << endl
		 << "convergence limit is set to 0.00001." << endl;
	    break;

	case '2':
	    cout << "Set the probability that an entry at a server task has a second phase" << endl
		 << "to " << emph( "ARG" ) << ".  " << emph( "ARG" ) << " must be between 0 and 1." << endl;
	    break;

	case 0x100+'2':
	    cout << "Set the model under-relaxation to " << emph( "ARG" ) << ".  By default, the" << endl
		 << "under-relaxation is set to 0.9." << endl;
	    break;

	case 0x100+'3':
	    cout << "Set the model iteration limit to " << emph( "ARG" ) << ".  By default, the iteration" << endl
		 << "limit is set to 50.  For models with many layers, this  value should" << endl
		 << "be higher." << endl;
	    break;
	    
	case 0x100+'4':
	    cout << "Set the seed value for the random number generator to " << emph( "ARG" ) << "." << endl;
	    break;

	case 0x100+'8':
	    cout << "The BETA distribution is only used for choosing the share of a group when using a processor" << endl
		 << "using Completely Fair Scheduling."  << endl
		 << "Set the beta argument of the distribution to " << emph( "ARG" ) << ".  The alpha" << endl
		 << "argument is set based on the value of the group \"share\" (set using " << flag( "\\-g", "", "n" ) << ")." << endl;
	    break;
	    
	default:	
	    cerr << "Update help.cc for ";
	    if ( (options[i].c & 0xff80) == 0 ) {
		cerr << "-" << static_cast<char>(options[i].c);
	    } else {
		cerr << showbase << internal << setfill( '0' ) << hex << setw(6) << (options[i].c & 0xff00) << "+"
		     << static_cast<char>(options[i].c & 0x00ff);
	    }
	    cerr << ", --" << options[i].name << endl;
	    break;
	}

	cout.flags(oldFlags);
	cout.precision(oldPrec);
	cout.fill(oldFill);
    }

    cout << endl << ".SH \"SEE ALSO\"" << endl;
    if ( Flags::lqn2lqx ) {
	cout << bf( "lqn2lqx" ) << "(1), ";
    } else {
	cout << bf( "lqngen" ) << "(1), ";
    }
    cout << bf( "lqns" ) << "(1), " << bf( "lqsim" ) << "(1), " << bf( "lqn2ps" ) << "(1)" << endl;
    cout << endl << ".SH \"EXAMPLES\"" << endl;
    if ( Flags::lqn2lqx ) {
	cout << "To convert an existing model file to SPEX:" << endl
	     << example( "lqn2lqx model.lqn" )
	     << "Note that the output will be in a file named " << emph( "model.xlqn" ) << "." << endl
	     << ".sp" << endl;
	cout << "To convert an existing model file to SPEX with running two experiments varying service time:" << endl
	     << example( "lqn2lqx -N2 -s2 model.lqn" )
	     << "Note that the output will be in a file named " << emph( "model.xlqn" ) << "." << endl
	     << ".sp" << endl;
	cout << "To convert an existing model file to LQX, varying the service time at all entries by 1.5x:" << endl
	     << example( "lqn2lqx --lqx-output --sensitivity=1.5 --no-customers --no-request-rate model.lqn" ) << "." << endl
	     << ".sp" << endl;
	cout << "To convert an existing model file to SPEX, and converting all serving tasks to infinite servers:" << endl
	     << example( "lqn2lqx --no-conversion -i1 model.lqn" ) << "." << endl
	     << ".sp" << endl;
	cout << "To convert an existing model file to SPEX, and converting all serving tasks to fixed rate servers:" << endl
	     << example( "lqn2lqx --no-conversion --constant -t1 model.lqn" ) << "." << endl
	     << ".sp" << endl;
    } else {
	cout << "To generate an annontated input file consisting of a single client calling a single server, both with their own processor: " << endl
	     << example( "lqngen output.lqn" );
	cout << "To create a model with two tiers with two classes of customers and where each tier is running on its own processor: " << endl
	     << example( "lqngen -L2 -C2 -T4 -P2 output.lqn" );
	cout << "To add exactly two entries to each server task: " << endl
	     << example( "lqngen -L2 -T4 -P2 -C2 -c -e2 output.lqn" )
	     << "Note that entries always accept at least one request, but an entry may not necessarily generate requests to lower layers." << endl
	     << ".sp" << endl;
	cout << "To create two separate randomly generated models with nine tasks: " << endl
	     << example( "lqngen -A9 -M2 model" ) << endl
	     << "The directory " << emph( "model" ) << " will contain two files named " << emph( "case-1.xlqn" ) << " and "  << emph( "case-2.xlqn" ) << "." <<endl
	     << ".sp" << endl;
	cout << "To generate two experiments with random service times uniformly distributed between [0.5,1.5]: " << endl
	     << example( "lqngen -N2 --uniform=1 -s1" )
	     << "Note that the distribution " << emph( "must be" ) << " specified prior to the parameter." << endl;

    }
}


static bool flag_value( int i )
{
    switch ( options[i].c  ) {
    case 0x200+'f': return Flags::observe[Flags::THROUGHPUT];
    case 0x200+'r': return Flags::observe[Flags::RESIDENCE_TIME];
    case 0x200+'u': return Flags::observe[Flags::UTILIZATION];
    case 0x200+'w': return Flags::observe[Flags::QUEUEING_TIME];
    case 0x200+'E': return Flags::observe[Flags::ELAPSED_TIME];
    case 0x200+'I': return Flags::observe[Flags::ITERATIONS];
    case 0x200+'M': return Flags::observe[Flags::MVA_STEPS];
    case 0x200+'W': return Flags::observe[Flags::MVA_WAITS];
    case 0x200+'S': return Flags::observe[Flags::SYSTEM_TIME];
    case 0x200+'U': return Flags::observe[Flags::USER_TIME];
    case 0x200+'i': return Flags::observe[Flags::PARAMETERS];
    case 0x200+'a': return Flags::annotate_input;
    }
    return false;
}

static ostream&
emph_str( ostream& output, const string& s )
{
    output << "\\fI" << s << "\\fP";
    return output;
}

static ostream&
bf_str( ostream& output, const string& s )
{
    output << "\\fB" << s << "\\fP";
    return output;
}

static ostream&
tt_str( ostream& output, const string& s )
{
    output << "\\f(CW" << s << "\\fP";
    return output;
}

static ostream&
example_str( ostream& output, const string& s )
{
    output << ".sp" << endl
	   << ".ti 0.75i" << endl
	   << tt( s ) << endl
	   << ".sp" << endl;
    return output;
}

static ostream&
boilerplate_str( std::ostream& output, const std::string& opt, const RV::RandomVariable& rv )
{
    std::string option( "\\-\\-" );
    option += opt;
    output << emph( "ARG" ) << " can be take the form of "; 
    if ( rv.getType() == RV::RandomVariable::CONSTANT ) {
	output << flag( option, "=", "n" ) << ",";
    } else {
	output << flag( option, "=", "mean" ) << ", " << endl
	       << flag( option, "=", "a:b" ) << " where " << emph( "a" ) << " and " << emph( "b" ) << " are parameters" << endl
	       << "for the default " << distribution_type[rv.getType()] << " distribution,";
    }
    output << " or " << flag( option, "=", "distribution:a[:b]" ) << ", where " << endl
	   << emph( "distribution" ) << " is a " << bf( distribution_type[rv.getType()] ) << " distribution, and " << emph( "a[:b]" ) << " are parameters to the distribution." << endl
	   << "By default, a " << emph( rv.name() ) << " distribution is used.";
    return output;
}

static ostream&
flag_str( std::ostream& output, const std::string& b, const std::string& r, const std::string& e )
{
    output << bf( b );
    if ( r.size() ) output << r;
    if ( e.size() ) output << emph( e );
    return output;
}

static ostream&
is_not_set_str( std::ostream& output, int i )
{
    if ( !flag_value(i) ) {
	output << "not ";
    }
    return output;
}
