/* help.cc	-- Greg Franks Wed Oct 12 2005
 *
 * $Id: help.cc 12550 2016-04-06 22:33:52Z greg $
 */

#include <config.h>
#include "dim.h"
#include <ctype.h>
#include <cstdlib>
#if HAVE_UNISTD_H
#include <unistd.h>
#include <time.h>
#endif
#include <lqio/error.h>
#include <lqio/dom_document.h>
#include "lqns.h"
#include "help.h"
#include "option.h"
#include "pragma.h"

class HelpManip {
public:
    HelpManip( ostream& (*ff)(ostream&, const int c ), const int c )
	: _c(c), f(ff)  {}
private:
    const int _c;
    ostream& (*f)( ostream&, const int c );

    friend ostream& operator<<(ostream & os, const HelpManip& m )
	{ return m.f(os,m._c); }
};

/* -------------------------------------------------------------------- */
/* Help/Usage info.							*/
/* -------------------------------------------------------------------- */

/*
 * A useful message to the Luser.
 */

void
usage ( const char * optarg )
{
    if ( !optarg ) {
	cerr << "Usage: " << io_vars.lq_toolname;

	cerr << " [option] [file ...]" << endl << endl;
	cerr << "Options" << endl;
#if HAVE_GETOPT_LONG
	const char ** p = opthelp;
	for ( const struct option *o = longopts; (o->name || o->val) && *p; ++o, ++p ) {
	    string s;
	    if ( o->name ) {
		s = "--";
		s += o->name;
		switch ( o->val ) {
		case 'o': s += "=FILE"; break;
		case 'H': s += "=[dztP]"; break;
		case 'd': s += "=<debug>"; break;
		case 'I': s += "=ARG"; break;
		case 'e': s += "=[adiw]"; break;
		case 't': s += "=<trace>"; break;
		case 'z': s += "=<special>"; break;
		case 'P': s += "=<pragma>"; break;

		case (256+'c'):
		case (256+'i'):
		case (256+'k'):
		case (256+'u'):
		    s += "=<n>";
		    break;
		}
	    } else {
		s = " ";
	    }
	    if ( isascii(o->val) && isgraph(o->val) ) {
		cerr << " -" << static_cast<char>(o->val) << ", ";
	    } else {
	        cerr << "     ";
	    }
	    cerr.setf( ios::left, ios::adjustfield );
	    cerr << setw(28) << s << *p << endl;
	}
#else
	const char * s;
	cerr << " [-";
	for ( s = opts; *s; ++s ) {
	    if ( *(s+1) == ':' ) {
		++s;
	    } else {
		cerr.put( *s );
	    }
	}
	cerr << ']';

	for ( s = opts; *s; ++s ) {
	    if ( *(s+1) == ':' ) {
		cerr << " [-" << *s;
		switch ( *s ) {
		default:  cerr << "file"; break;
		case 'd': cerr << "<debug>"; break;
		case 'e': cerr << "adiw"; break;
		case 't': cerr << "<trace>"; break;
		case 'z': cerr << "<special>"; break;
		case 'P': cerr << "<pragma>"; break;
		}
		cerr << ']';
		++s;
	    }
	}
	cerr << " [file ...]" << endl;
#endif
    } else {
	switch ( optarg[0] ) {
	case 'd':
	case 't':
	case 'z':
	    usage( optarg[0] );
	    break;
	case 'P':
	    Pragma::usage( cerr );
	    break;
	default:
	    cerr << "Invalid argument to -H --help: " << optarg << endl;
	    cerr << "-Hd -- debug options; -Ht -- trace options; -Hz -- special options; -HP -- pragmas" << endl;
	    break;
	}
    }

    (void) exit( INVALID_ARGUMENT );
}



/*
 * Print out subusage stuff.
 */

void
usage( const char c )
{
    switch( c ) {
    case 'd':
	HelpPlain::print_debug( cerr );
	break;

    case 't':
	HelpPlain::print_trace( cerr );
	break;

    case 'z':
	HelpPlain::print_special( cerr );
	break;

    default:
	LQIO::internal_error( __FILE__, __LINE__, "usage()" );
	break;
    }
}


/*
 * generic error message.
 */

void
usage( const char c, const char * s )
{
    cerr << io_vars.lq_toolname << " -" << c << ": invalid argument -- " << s << endl;
    usage( c );
}

/* -------------------------------------------------------------------- */
/* Man page generation.							*/
/* -------------------------------------------------------------------- */

std::map<const int,Help::help_fptr,lt_int> Help::option_table;

Help::Help()
{
    initialize();
}



/* static */ void
Help::initialize()
{
    /* Load functions used to print option args. */

    if ( option_table.size() > 0 ) return;

    option_table['a']     = &Help::flagAdvisory;
    option_table['b']     = &Help::flagBound;
    option_table['d']     = &Help::flagDebug;
    option_table['e']     = &Help::flagError;
    option_table['f']	  = &Help::flagFast;
    option_table['I'] 	  = &Help::flagInputFormat;
    option_table['n']     = &Help::flagNoExecute;
    option_table['o']     = &Help::flagOutput;
    option_table['p']     = &Help::flagParseable;
    option_table['P']     = &Help::flagPragmas;
    option_table['r']	  = &Help::flagRTF;
    option_table['t']     = &Help::flagTrace;
    option_table['v']     = &Help::flagVerbose;
    option_table['V']     = &Help::flagVersion;
    option_table['w']     = &Help::flagWarning;
    option_table['x']     = &Help::flagXML;
    option_table['z']     = &Help::flagSpecial;
    option_table[256+'c'] = &Help::flagConvergence;
    option_table[256+'e'] = &Help::flagExactMVA;
    option_table[256+'h'] = &Help::flagHwSwLayering;
    option_table[256+'i'] = &Help::flagIterationLimit;
    option_table[256+'l'] = &Help::flagLoose;
    option_table[256+'m'] = &Help::flagMethoOfLayers;
    option_table[256+'p'] = &Help::flagProcessorSharing;
    option_table[256+'o'] = &Help::flagStopOnMessageLoss;
    option_table[256+'s'] = &Help::flagSchweitzerMVA;
    option_table[256+'t'] = &Help::flagTraceMVA;
    option_table[256+'u'] = &Help::flagUnderrelaxation;
    option_table[256+'z'] = &Help::flagSquashedLayering;
    option_table[256+'v'] = &Help::flagNoVariance;
    option_table[512+'h'] = &Help::flagNoHeader;
    option_table[512+'r'] = &Help::flagReloadLQX;
    option_table[512+'R'] = &Help::flagRestartLQX;
    option_table[512+'l'] = &Help::flagDebugLQX;
    option_table[512+'x'] = &Help::flagDebugXML;

    Options::Debug::initialize();
    Options::Trace::initialize();
    Options::Special::initialize();
    Pragma::initialize();
}



/*
 * Make a man page :-)
 */

ostream&
Help::print( ostream& output ) const
{
    preamble( output );
    output << bold( *this, "Lqns" ) << " reads its input from " << filename( *this, "filename" ) << ", specified at the" << endl
	   << "command line if present, or from the standard input otherwise.  By" << endl
	   << "default, output for an input file " << filename( *this, "filename" ) << " specified on the" << endl
	   << "command line will be placed in the file " << filename( *this, "filename", ".out" ) << ".  If the" << endl
	   << flag( *this, "p" ) << " switch is used, parseable output will also be written into" << endl
	   << filename( *this, "filename", ".p" ) << ". If XML input" << ix( *this, "input!XML" ) << " or the " << flag( *this, "x" ) << " switch is used, XML output" << ix( *this, "output!XML" ) << " will be written to " << endl
	   << filename( *this, "filename", ".lqxo" ) << ".  This behaviour can be changed using the" << endl
	   << flag( *this, "o" ) << filename( *this, "output" ) << " switch, described below.  If several files are" << endl
	   << "named, then each is treated as a separate model and output will be" << endl
	   << "placed in separate output files.  If input is from the standard input," << endl
	   << "output will be directed to the standard output.  The file name `" << filename( *this, "-" ) << "' is" << endl
	   << "used to specify standard input." << endl;
    pp( output );
    output << "The " << flag( *this, "o" ) << filename( *this, "output" ) << " option can be used to direct output to the file" << endl
	   << filename( *this, "output" ) << " regardless of the source of input.  Output will be XML" << ix( *this, "XML" ) << ix( *this, "output!XML" ) << endl
	   << "if XML input" << ix( *this, "XML!input" ) << " or if the " << flag( *this, "x" ) << " switch is used, parseable output if the " << flag( *this, "p" ) << " switch is used," << endl
	   << "and normal output otherwise.  Multiple input files cannot be specified" << endl
	   << "when using this option.  Output can be directed to standard output by" << endl
	   << "using " << flag( *this, "o" ) << filename( *this, "-" ) << " (i.e., the output file name is `" << filename( *this, "-" ) << "'.)" << endl;

    section( output, "OPTIONS", "Command Line Options" );
    label( output, "sec:options" );
    dl_begin( output );
#if HAVE_GETOPT_LONG
    for ( const struct option *o = longopts; (o->name || o->val); ++o ) {
	longopt( output, o );
	help_fptr f = option_table[o->val];
	if ( f ) {
	    (this->*f)( output, true );
	}
    }
#endif
    dl_end( output );

    pp( output );
    output << bold( *this, "Lqns" ) << " exits" << ix( *this, "exit!success" ) << " with 0 on success, 1 if the model failed to converge," << ix( *this, "convergence!failure" ) << endl
	   << "2 if the input was invalid" << ix( *this, "input!invalid" ) << ", 4 if a command line argument was" << ix( *this, "command line!incorrect" ) << endl
	   << "incorrect, 8 for file read/write problems and -1 for fatal errors" << ix( *this, "error!fatal" )  <<".  If" << endl
	   << "multiple input files are being processed, the exit code is the" << endl
	   << "bit-wise OR of the above conditions." << endl;

    section( output, "PRAGMAS", "Pragmas" );
    label( output, "sec:lqns-pragmas" );
    output << emph( *this, "Pragmas" ) << ix( *this, "pragma" ) << " are used to alter the behaviour of the solver in a" << endl
	   << "variety of ways.  They can be specified in the input file with" << endl
	   << "``#pragma'', on the command line with the " << flag( *this, "P" ) << " option, or through" << endl
	   << "the environment variable " << ix( *this, "environment variable" ) << emph( *this, "LQNS_PRAGMAS" ) << ix( *this, "LQNS\\_PRAGMAS@\\texttt{LQNS\\_PRAGMAS}" ) << ".  Command line" << endl
	   << "specification of pragmas overrides those defined in the environment" << endl
	   << "variable which in turn override those defined in the input file.  The" << endl
	   << "following pragmas are supported.  Invalid pragma" << ix( *this, "pragma!invalid" ) << " specification at the" << endl
	   << "command line will stop the solver.  Invalid pragmas defined in the" << endl
	   << "environment variable or in the input file are ignored as they might be" << endl
	   << "used by other solvers." << endl;

    std::map<const char *, Pragma::pragma_info, lt_str>::const_iterator next_pragma;
    dl_begin( output );
    
    const std::map<const char *, Pragma::pragma_info, lt_str>& pragmas = Pragma::getPragmas();
    for ( next_pragma = pragmas.begin(); next_pragma != pragmas.end(); ++next_pragma  ) {
	print_pragma( output, next_pragma->first, (void *)(&next_pragma->second) );
    }
    dl_end( output );

    section( output, "STOPPING CRITERIA", "Stopping Criteria" );
    label( output, "sec:lqns-stopping-criteria" );
    output << bold( *this, "Lqns" ) << " computes the model results by iterating through a set of" << endl
	   << "submodels until either convergence" << ix( *this, "convergence" ) << " is achieved, or the iteration limit" << ix( *this, "iteration limit|textbf" ) << endl
	   << "is hit. Convergence is determined by taking the root of the mean of" << endl
	   << "the squares of the difference in the utilization of all of the servers" << endl
	   << "from the last two iterations of the MVA solver over the all of the" << endl
	   << "submodels then comparing the result to the convergence value specified" << endl
	   << "in the input file. If the RMS change in utilization is less than" << endl
	   << "convergence value" << ix( *this, "convergence!value|textbf" )  << ", then the results are considered valid." << endl;
    pp( output );
    output << "If the model fails to converge," << ix( *this, "convergence!failure" ) << " three options are available:" << endl;
    ol_begin( output );
    li( output, "1." );
    output << "reduce the under-relaxation coefficient. Waiting and idle times are" << endl
	   << "propogated between submodels during each iteration. The" << endl
	   << "under-relaxation coefficient determines the amount a service time is" << endl
	   << "changed between each iteration. A typical value is 0.7 - 0.9; reducing" << endl
	   << "it to 0.1 may help." << endl;
    li( output, "2." );
    output << "increase the iteration limit." << ix( *this, "iteration limit" ) << " The iteration limit sets the upper bound" << endl
	   << "on the number of times all of the submodels are solved. This value may" << endl
	   << "have to be increased, especially if the under-relaxation coefficient" << endl
	   << "is small, or if the model is deeply nested. The default value is 50" << endl
	   << "iterations." << endl;
    li( output, "3." );
    output << "increase the convergence test value" << ix( *this, "convergence!value" ) << ". Note that the convergence value" << endl
	   << "is the standard deviation in the change in the utilization of the" << endl
	   << "servers, so a value greater than 1.0 makes no sense." << endl;
    ol_end( output );

    pp( output );
    output << "The convergence value can be observed using " << flag( *this, "t" ) << emph( *this, "convergence" ) << " flag." << endl;

    section( output, "MODEL LIMITS", "Model Limits" );
    label( output, "sec:model-limits" );
    output << "The following table lists the acceptable parameter types for"  << endl
	   << bold( *this, "lqns" ) << ".  An error will" << endl
	   << "be reported if an unsupported parameter is supplied except when the" << endl
	   << "value supplied is the same as the default."  << endl;

    pp( output );
    table_header( output );
    table_row( output, "Phases", "3", "phase!maximum" );
    table_row( output, "Scheduling", "FIFO, HOL, PPR", "scheduling" );
    table_row( output, "Open arrivals", "yes", "open arrival" );
    table_row( output, "Phase type", "stochastic, deterministic", "phase!type" );
    table_row( output, "Think Time", "yes", "think time" );
    table_row( output, "Coefficient of variation", "yes", "coefficient of variation" );
    table_row( output, "Interprocessor-delay", "yes", "interprocessor delay" );
    table_row( output, "Asynchronous connections", "yes", "asynchronous connections" );
    table_row( output, "Forwarding", "yes", "forwarding" );
    table_row( output, "Multi-servers", "yes", "multi-server" );
    table_row( output, "Infinite-servers", "yes", "infinite server" );
    table_row( output, "Max Entries", "1000", "entry!maximum" );
    table_row( output, "Max Tasks", "1000", "task!maximum" );
    table_row( output, "Max Processors", "1000", "processor!maximum" );
    table_row( output, "Max Entries per Task", "1000" );
    table_footer( output );

    section( output, "DIAGNOSTICS", "Diagnostics" );
    label( output, "sec:lqns-diagnostics" );

    output << "Most diagnostic messages result from errors in the input file." << endl
	   << "If the solver reports errors, then no solution will be generated for" << endl
	   << "the model being solved.  Models which generate warnings may not be" << endl
	   << "correct.  However, the solver will generate output." << endl;
    pp( output );
    output << "Sometimes the model fails to converge" << ix( *this, "convergence!failure" ) << ", particularly if there are several" << endl
	   << "heavily utilized servers in a submodel.  Sometimes, this problem can" << endl
	   << "be solved by reducing the value of the under-relaxation coefficient.  It"  << endl
	   << "may also be necessary to increase the iteration-limit" << ix( *this, "iteration limit" ) << ", particularly if" << endl
	   << "there are many submodels.  With replicated models, it may be necessary" << endl
	   << "to use `srvn' layering to get the model to converge.  Convergence can be tracked" << endl
	   << "using the "<< flag( *this, "t" ) << emph( *this, "convergence" ) << " option." << endl;
    pp( output );
    output << "The solver will sometimes report some servers with `high' utilization." << endl
	   << "This problem is the result of some of the approximations used, in particular, two-phase servers." << endl
	   << "Utilizations in excess of 10\\% are likely the result of failures in the solver." << endl
	   << "Please send us the model file so that we can improve the algorithms." << endl;
    see_also( output );
    trailer( output );
    return output;
}

ostream&
Help::flagAdvisory( ostream& output, bool verbose ) const
{
    output << "Ignore advisories.  The default is to print out all advisories." << ix( *this, "advisory!ignore" ) << endl;
    return output;
}

ostream&
Help::flagBound( ostream& output, bool verbose ) const
{
    output << "This option is used to compute the ``Type 1 throughput bounds''" << ix( *this, "throughput!bounds" ) << ix( *this, "bounds!throughput" ) << " only."  << endl
	   << "These bounds are computed assuming no contention anywhere in the model" << endl
	   << "and represent the guaranteed not to exceed values." << endl;
    return output;
}

ostream&
Help::flagDebug( ostream& output, bool verbose ) const
{
    output << "This option is used to enable debug output." << ix( *this, "debug" ) << endl
	   << emph( *this, "Arg" ) << " can be one of:" << endl;
    increase_indent( output );
    dl_begin( output );
    std::map<const char *, Options::Debug>::const_iterator opt;
    for ( opt = Options::Debug::__table.begin(); opt != Options::Debug::__table.end(); ++opt ) {
	print_option( output, opt->first, opt->second );
    }
    dl_end( output );
    decrease_indent( output );
    return output;
}

ostream&
Help::flagError( ostream& output, bool verbose ) const
{
    output << "This option is to enable floating point exception handling." << ix( *this, "floating point!exception" ) << endl
	   << emph( *this, "Arg" ) << " must be one of the following:" << endl;
    increase_indent( output );
    ol_begin( output );
    li( output ) << bold( *this, "a" ) << endl
		 << "Abort immediately on a floating point error (provided the floating point unit can do so)." << endl;
    li( output ) << bold( *this, "d" ) << endl
		 << "Abort on floating point errors. (default)" << endl;
    li( output ) << bold( *this, "i" ) << endl
		 << "Ignore floating point errors." << endl;
    li( output ) << bold( *this, "w" ) << endl
		 << "Warn on floating point errors." << endl;
    ol_end( output );
    output << "The solver checks for floating point overflow," << ix( *this, "overflow" ) << " division by zero and invalid operations." << endl
	   << "Underflow and inexact result exceptions are always ignored." << endl;
    pp( output );
    output << "In some instances, infinities " << ix( *this, "infinity" ) << ix( *this, "floating point!infinity" ) << " will be propogated within the solver.  Please refer to the" << endl
	   << bold( *this, "stop-on-message-loss" ) << " pragma below." << endl;
    decrease_indent( output );
    return output;
}

ostream& 
Help::flagFast( ostream& output, bool verbose ) const
{
    output << "This option is used to set options for quick solution of a model using One-Step (Bard-Schweitzer) MVA."  << endl
	   << "It is equivalent to setting " << bold( *this, "pragma" ) 
	   << " "  << emph( *this, "mva" ) << "=" << emph( *this, "one-step" )
	   << ", " << emph( *this, "layering" ) << "=" << emph( *this, "batched" )
	   << ", " << emph( *this, "multiserver" ) << "=" << emph( *this, "conway" ) << endl;
    return output;
}

ostream&
Help::flagInputFormat( ostream& output, bool verbose ) const
{
    output << "This option is used to force the input file format to either " << emph( *this, "xml" ) << " or " << emph( *this, "lqn" ) << "." << endl
	   << "By default, if the suffix of the input filename is one of: " << emph( *this, ".in" ) << ", " << emph( *this, ".lqn" ) << " or " << emph( *this, ".xlqn" ) << endl
	   << ", then the LQN parser will be used.  Otherwise, input is assumed to be XML." << endl;
    return output;
}

ostream&
Help::flagNoExecute( ostream& output, bool verbose ) const
{
    output << "Read input, but do not solve.  The input is checked for validity.  " << endl
	   << "No output is generated." << endl;
    return output;
}

ostream& 
Help::flagMethoOfLayers( ostream& output, bool verbose ) const
{
    output << "This option is to use the Method Of Layers solution approach to solving the layer submodels." << endl;
    return output;
}

ostream&
Help::flagOutput( ostream& output, bool verbose ) const
{
    output << "Direct analysis results to " << emph( *this, "output" ) << ix( *this, "output" ) << ".  A filename of `" << filename( *this, "-" )  << ix( *this, "standard input" ) << "'" << endl
	   << "directs output to standard output.  If " << filename( *this, "output" ) << " is a directory, all output is saved in " 
	   << filename( *this, "output/input.out" ) << ". If the input model contains a SPEX program with loops, the SPEX output is sent to "
	   << filename( *this, "output" ) << "; the individual model output files are found in the directory " 
	   << filename( *this, "output.d" ) << ". If " << bold( *this, "lqns" ) <<" is invoked with this" << endl
	   << "option, only one input file can be specified." << endl;
    return output;
}

ostream&
Help::flagParseable( ostream& output, bool verbose ) const
{
    output << "Generate parseable output suitable as input to other programs such as" << endl
	   << bold( *this, "lqn2ps(1)" ) << " and " << bold( *this, "srvndiff(1)" ) << ".  If input is from" << endl
	   << filename( *this, "filename" ) << ", parseable output is directed to " << filename( *this, "filename", ".p" ) << "." << endl
	   << "If standard input is used for input, then the parseable output is sent" << endl
	   << "to the standard output device.  If the " << flag( *this, "o" ) << filename( *this, "output" ) << " option is used, the" << endl
	   << "parseable output is sent to the file name " << filename( *this, "output" ) << "." << endl
	   << "(In this case, only parseable output is emitted.)" << endl;
    return output;
}


ostream&
Help::flagPragmas( ostream& output, bool verbose ) const
{
    output << "Change the default solution strategy.  Refer to the PRAGMAS section" << ix( *this, "pragma" ) << endl
	   << "below for more information." << endl;
    return output;
}

ostream& 
Help::flagProcessorSharing( ostream& output, bool verbose ) const
{
    output << "Use Processor Sharing scheduling at all fixed-rate processors." << endl;
    return output;
}

ostream&
Help::flagRTF( ostream& output, bool verbose ) const
{
    output << "Output results using Rich Text Format instead of plain text.  Processors, entries and tasks with high utilizations are coloured in red." << endl;
    return output;
}

ostream& 
Help::flagSquashedLayering( ostream& output, bool verbose ) const
{
    output << "Use only one submodel to solve the model." << endl;
    return output;
}

ostream&
Help::flagTrace( ostream& output, bool verbose ) const
{
    output << "This option is used to set tracing " << ix( *this, "tracing" ) << " options which are used to print out various" << endl
	   << "intermediate results " << ix( *this, "results!intermediate" ) << " while a model is being solved." << endl
	   << emph( *this, "arg" ) << " can be any combination of the following:" << endl;
    increase_indent( output );
    dl_begin( output );
    std::map<const char *, Options::Trace>::const_iterator opt;
    for ( opt = Options::Trace::__table.begin(); opt != Options::Trace::__table.end(); ++opt ) {
	print_option( output, opt->first, opt->second );
    }
    dl_end( output );
    decrease_indent( output );
    return output;
}


ostream&
Help::flagVerbose( ostream& output, bool verbose ) const
{
    output << "Generate output after each iteration of the MVA solver and the convergence value at the end of each outer iteration of the solver." << endl;
    return output;
}

ostream&
Help::flagVersion( ostream& output, bool verbose ) const
{
    output << "Print out version and copyright information." << ix( *this, "version" ) << ix( *this, "copyright" ) << endl;
    return output;
}

ostream&
Help::flagWarning( ostream& output, bool verbose ) const
{
    output << "Ignore warnings.  The default is to print out all warnings." << ix( *this, "warning!ignore" ) << endl;
    return output;
}

ostream&
Help::flagXML( ostream& output, bool verbose ) const
{
    output << "Generate XML output regardless of input format." << endl;
    return output;
}

ostream&
Help::flagSpecial( ostream& output, bool verbose ) const
{
    output << "This option is used to select special options.  Arguments of the form" << endl
	   << emph( *this, "nn" ) << " are integers while arguments of the form " << emph( *this, "nn.n" ) << " are real" << endl
	   << "numbers.  " << emph( *this, "Arg" ) << " can be any of the following:" << endl;
    increase_indent( output );
    dl_begin( output );
    std::map<const char *, Options::Special>::const_iterator opt;
    for ( opt = Options::Special::__table.begin(); opt != Options::Special::__table.end(); ++opt ) {
	print_option( output, opt->first, opt->second );
    }
    dl_end( output );
    br( output );
    output << "If any one of " << emph( *this, "convergence" ) << ", " << emph( *this, "iteration-limit" ) << ", or"
	   << emph( *this, "print-interval" ) << " are used as arguments, the corresponding " << endl
	   << "value specified in the input file for general information, `G', is" << endl
	   << "ignored.  " << endl;
    decrease_indent( output );
    return output;
}

ostream&
Help::flagConvergence( ostream& output, bool verbose ) const
{
    help_fptr f = Options::Special::__table["convergence-value"].help();
    (this->*f)(output,verbose) << ix( *this, "convergence!value" );
    return output;
}

ostream&
Help::flagUnderrelaxation( ostream& output, bool verbose ) const
{
    help_fptr f = Options::Special::__table["underrelaxation"].help();
    (this->*f)(output,verbose);
    return output;
}
ostream&
Help::flagIterationLimit( ostream& output, bool verbose ) const
{
    help_fptr f = Options::Special::__table["iteration-limit"].help();
    (this->*f)(output,verbose);
    return output;
}

ostream&
Help::flagExactMVA( ostream& output, bool verbose ) const
{
    output << "Use Exact MVA to solve all submodels." << ix( *this, "MVA!exact" ) << endl;
    return output;
}

ostream&
Help::flagSchweitzerMVA( ostream& output, bool verbose ) const
{
    output << "Use Bard-Schweitzer approximate MVA to solve all submodels." << ix( *this, "MVA!Bard-Schweitzer" ) << endl;
    return output;
}

ostream&
Help::flagHwSwLayering( ostream& output, bool verbose ) const
{
    return output;
}

ostream&
Help::flagLoose( ostream& output, bool verbose ) const
{
    output << "Solve the model using submodels containing exactly one server." << endl;
    return output;
}

ostream&
Help::flagStopOnMessageLoss( ostream& output, bool verbose ) const
{
    output << "Do not stop the solver on overflow (infinities) for open arrivals or send-no-reply messages to entries.  The default is to stop with an" << endl
	   << "error message indicating that the arrival rate is too high for the service time of the entry" << endl;
    return output;
}

ostream&
Help::flagTraceMVA( ostream& output, bool verbose ) const
{
    output << "Output the inputs and results of each MVA submodel for every iteration of the solver." << endl;
    return output;
}

ostream&
Help::flagNoHeader( ostream& output, bool verbose ) const
{
    output << "Do not print out the Result Variable header when running with SPEX input." << endl;
    if ( verbose ) {
	output << "This option has no effect otherwise." << endl;
    }
    return output;
}

ostream&
Help::flagNoVariance( ostream& output, bool verbose ) const
{
    output << "Do not use variances in the waiting time calculations." << endl;
    return output;
}

ostream&
Help::flagReloadLQX( ostream& output, bool verbose ) const
{
    output << "Re-run the LQX/SPEX" << ix( *this, "LQX" ) <<  " program without re-solving the models.  Results must exist from a previous solution run." << endl
	   << "This option is useful if LQX print statements or SPEX results are changed." << endl;
    return output;
}

ostream&
Help::flagRestartLQX( ostream& output, bool verbose ) const
{
    output << "Re-run the LQX/SPEX" << ix( *this, "LQX" ) <<  " program without re-solving models which were solved successfully.  Models which were not solved because of early termination, or which were not solved successfully because of convergence problems, will be solved." << endl
	   << "This option is useful for running a second pass with a new convergnece value and/or iteration limit." << endl;
    return output;
}

ostream&
Help::flagDebugLQX( ostream& output, bool verbose ) const
{
    output << "Output debugging information as an LQX" << ix( *this, "LQX!debug" ) << " program is being parsed." << endl;
    return output;
}

ostream&
Help::flagDebugXML( ostream& output, bool verbose ) const
{
    output << "Output XML" << ix( *this, "XML!debug" ) << " elements and attributes as they are being parsed.   Since the XML parser usually stops when it encounters an error," << endl
	   << "this option can be used to localize the error." << endl;
    return output;
}


ostream&
Help::debugAll( ostream & output, bool verbose ) const
{
    output << "Enable all debug output." << endl;
    return output;
}

ostream&
Help::debugActivities( ostream & output, bool verbose ) const
{
    output << "Activities -- not functional." << endl;
    return output;
}

ostream&
Help::debugCalls( ostream & output, bool verbose ) const
{
    output << "Print out the number of rendezvous between all tasks." << endl;
    return output;
}

ostream&
Help::debugForks( ostream & output, bool verbose ) const
{
    output << "Print out the fork-join matching process." << endl;
    return output;
}

ostream&
Help::debugInterlock( ostream & output, bool verbose ) const
{
    output << "Print out the interlocking table and the interlocking between all tasks and processors." << endl;
    return output;
}

ostream&
Help::debugJoins( ostream & output, bool verbose ) const
{
    output << "Joins -- not functional." << endl;
    return output;
}

ostream&
Help::debugLayers( ostream & output, bool verbose ) const
{
    output << "Print out the contents of all of the layers found in the model." << endl;
    return output;
}

ostream&
Help::debugOvertaking( ostream & output, bool verbose ) const
{
    output << "Print the overtaking probabilities in the output file." << endl;
    return output;
}

ostream&
Help::debugQuorum( ostream & output, bool verbose ) const
{
    output << "Print out results from pseudo activities used by quorum." << endl;
    return output;
}

ostream&
Help::debugXML( ostream & output, bool verbose ) const
{
    output << "Print out the actions of the Expat parser while reading XML input." << endl;
    return output;
}

ostream&
Help::debugLQX( ostream & output, bool verbose ) const
{
    output << "Print out the actions the LQX parser while reading an LQX program." << endl;
    return output;
}

ostream&
Help::traceActivities( ostream & output, bool verbose ) const
{
    output << "Print out results of activity aggregation."<< endl;
    return output;
}

ostream&
Help::traceConvergence( ostream & output, bool verbose ) const
{
    output << "Print out convergence value after each submodel is solved." << endl;
    if ( verbose ) {
	output << "This option is useful for tracking the rate of convergence for a model." << endl
	       << "The optional numeric argument supplied to this option will print out the convergence value for the specified MVA submodel, otherwise," << endl
	       << "the convergence value for all submodels will be printed." << endl;
    }
    return output;
}

ostream&
Help::traceDeltaWait( ostream & output, bool verbose ) const
{
    output << "Print out difference in entry service time after each submodel is solved." << endl;
    return output;
}

ostream&
Help::traceForks( ostream & output, bool verbose ) const
{
    output << "Print out overlap table for forks prior to submodel solution." << endl;
    return output;
}

ostream&
Help::traceIdleTime( ostream & output, bool verbose ) const
{
    output << "Print out computed idle time after each submodel is solved." << endl;
    return output;
}

ostream&
Help::traceInterlock( ostream & output, bool verbose ) const
{
    output << "Print out interlocking adjustment before each submodel is solved." << endl;
    return output;
}

ostream&
Help::traceJoins( ostream & output, bool verbose ) const
{
    output << "Print out computed join delay and join overlap table prior to submodel solution." << endl;
    return output;
}

ostream&
Help::traceMva( ostream & output, bool verbose ) const
{
    output << "Print out the MVA submodel and its solution." << endl;  
    if ( verbose ) {
	output << "A numeric argument supplied to this option will print out only the specified MVA submodel, otherwise, all submodels will be printed." << endl;
    }
    return output;
}

ostream&
Help::traceOvertaking( ostream & output, bool verbose ) const
{
    output << "Print out overtaking calculations." << endl;
    return output;
}

ostream&
Help::traceIntermediate( ostream & output, bool verbose ) const
{
    output << "Print out intermediate solutions at the print interval specified in the model." << endl;
    if ( verbose ) {
	output << "The print interval field in the input is ignored otherwise." << endl;
    }
    return output;
}

ostream&
Help::traceReplication( ostream & output, bool verbose ) const
{
    output << "" << endl;
    return output;
}

ostream&
Help::traceVariance( ostream & output, bool verbose ) const
{
    output << "Print out the variances calculated after each submodel is solved." << endl;
    return output;
}

ostream&
Help::traceWait( ostream & output, bool verbose ) const
{
    output << "Print waiting time for each rendezvous in the model after it has been computed." << endl;
    return output;
}

ostream&
Help::traceThroughput( ostream & output, bool verbose ) const
{
    output << "Print throughput's values." << endl;
    return output;
}

ostream&
Help::traceQuorum( ostream & output, bool verbose ) const
{
    output << "Print quorum traces." << endl;
    return output;
}

ostream&
Help::specialIterationLimit( ostream & output, bool verbose ) const
{
    output << "Set the maximum number of iterations to " << emph( *this, "arg" ) << "." << endl;
    if ( verbose ) {
	output << emph( *this, "Arg" ) << " must be an integer greater than 0.  The default value is 50." << endl;
    }
    return output;
}

ostream&
Help::specialPrintInterval( ostream & output, bool verbose ) const
{
    output << "Set the printing interval to " << emph( *this, "arg" ) << "." << endl;
    if ( verbose ) {
	output << "The " << flag( *this, "d" ) << " or " << flag( *this, "v" ) << " options must also be selected to display intermediate results." << endl
	       << "The default value is 10." << endl;
    }
    return output;
}

ostream&
Help::specialOvertaking( ostream & output, bool verbose ) const
{
    output << "Print out overtaking probabilities." << endl;
    return output;
}

ostream&
Help::specialConvergenceValue( ostream & output, bool verbose ) const
{
    output << "Set the convergence value to " << emph( *this, "arg" ) << ".  " << endl;
    if ( verbose ) {
	output << emph( *this, "Arg" ) << " must be a number between 0.0 and 1.0." << endl;
    }
    return output;
}

ostream&
Help::specialSingleStep( ostream & output, bool verbose ) const
{
    output << "Stop after each MVA submodel is solved." << endl;
    if ( verbose ) {
	output << "Any character typed at the terminal except end-of-file will resume the calculation.  End-of-file will cancel single-stepping altogether." << endl;
    }
    return output;
}

ostream&
Help::specialUnderrelaxation( ostream & output, bool verbose ) const
{
    output << "Set the underrelaxation to " << emph( *this, "arg" ) << "." << endl;
    if ( verbose ) {
	output << emph( *this, "Arg" ) << " must be a number between 0.0 and 1.0." << endl
	       << "The default value is 0.9." << endl;
    }
    return output;
}

ostream&
Help::specialGenerateQueueingModel( ostream & output, bool verbose ) const
{
    output << "This option is used for debugging the solver." << endl;
    if ( verbose ) {
	output << "A directory named " << emph( *this, "arg" )
	       << " will be created containing source code for invoking the MVA solver directly."  << endl;
    }
    return output;
}

ostream&
Help::specialMolMSUnderrelaxation( ostream & output, bool verbose ) const
{
    output << "Set the under-relaxation factor to " << emph( *this, "arg" ) << " for the MOL multiserver approximation." << endl;
    if ( verbose ) {
	output << emph( *this, "Arg" ) << " must be a number between 0.0 and 1.0." << endl
	       << "The default value is 0.5.";
    }
    return output;
}

ostream&
Help::speicalSkipLayer( ostream & output, bool verbose ) const
{
    output << "Ignore submodel " << emph( *this, "arg" ) << " during solution." << endl;
    return output;
}

ostream&
Help::specialMakeMan( ostream & output, bool verbose ) const
{
    output << "Output this manual page.  " << endl;
    if ( verbose ) {
	output << "If an optional " << emph( *this, "arg" ) << endl
	       << "is supplied, output will be written to the file named "  << emph( *this, "arg" )  << "." << endl
	       << "Otherwise, output is sent to stdout." << endl;
    }
    return output;
}

ostream&
Help::specialMakeTex( ostream & output, bool verbose ) const
{
    output << "Output this manual page in LaTeX format." << endl;
    if ( verbose ) {
	output << "If an optional " << emph( *this, "arg" ) << endl
	       << "is supplied, output will be written to the file named "  << emph( *this, "arg" )  << "." << endl
	       << "Otherwise, output is sent to stdout." << endl;
    }
    return output;
}

ostream&
Help::specialMinSteps( ostream & output, bool verbose ) const
{
    output << "Force the solver to iterate min-steps times." << endl;
    return output;
}

ostream&
Help::specialIgnoreOverhangingThreads( ostream & output, bool verbose ) const
{
    output << "Ignore the effect of the overhanging threads."<< endl;
    return output;
}

ostream&
Help::specialFullReinitialize( ostream & output, bool verbose ) const
{
    output << "For multiple runs, reinitialize all processors." << endl;
    return output;
}

ostream&
Help::pragmaCycles( ostream& output, bool verbose ) const
{
    output << "This pragma is used to enable or disable cycle detection" << ix( *this, "cycle!detection" ) << " in the call" << endl
	   << "graph." << ix( *this, "call graph" ) << "  Cycles may indicate the presence of deadlocks." << ix( *this, "deadlock" ) << endl
	   << emph( *this, "Arg" ) << " must be one of: " << endl;
    return output;
}

ostream&
Help::pragmaStopOnMessageLoss( ostream& output, bool verbose ) const
{
    output << "This pragma is used to control the operation of the solver when the" << endl
	   << "arrival rate" << ix( *this, "arrival rate" ) << " exceeds the service rate of a server." << endl
	   << emph( *this, "Arg" ) << " must be one of: " << endl;
    return output;
}

ostream&
Help::pragmaInterlock( ostream& output, bool verbose ) const
{
    output << "The interlocking" << ix( *this, "interlock" ) << " is used to correct the throughputs" << ix( *this, "throughput!interlock" ) << " at stations as a" << endl
	   << "result of solving the model using layers" << cite( *this, "perf:franks-95-ipds-interlock" ) << ".  This pragma is used to" << endl
	   << "choose the algorithm used." << endl
	   << emph( *this, "Arg" ) << " must be one of: " << endl;
    return output;
}

ostream&
Help::pragmaLayering( ostream& output, bool verbose ) const
{
    output << "This pragma is used to select the layering strategy" << ix( *this, "layering!strategy" ) << " used by the solver." << endl
	   << emph( *this, "Arg" ) << " must be one of: " << endl;
    return output;
}

ostream&
Help::pragmaMultiserver( ostream& output, bool verbose ) const
{
    output << "This pragma is used to choose the algorithm for solving multiservers" << ix( *this, "multiserver!algorithm" ) << "." << endl
	   << emph( *this, "Arg" ) << " must be one of: " << endl;
    return output;
}

ostream&
Help::pragmaMVA( ostream& output, bool verbose ) const
{
    output << "This pragma is used to choose the MVA" << ix( *this, "MVA" ) << " algorithm used to solve the submodels." << endl
	   << emph( *this, "Arg" ) << " must be one of: " << endl;
    return output;
}

#if HAVE_LIBGSL && HAVE_LIBGSLCBLAS
ostream&
Help::pragmaQuorumDistribution( ostream& output, bool verbose ) const
{
    output << "This pragma is used to choose the Quorum algorithm used to approximate " ;
    output <<"\nthe thread service time distibution." << endl;
    return output;
}

ostream&
Help::pragmaQuorumDelayedCalls( ostream& output, bool verbose ) const
{
    output << "This pragma is used to choose the Quorum semantics for the delayed calls" << endl; ;

    return output;
}

ostream&
Help::pragmaIdleTime( ostream& output, bool verbose ) const
{
    output << "This pragma is used to choose throughput used to calculate " ;
    output <<"\nthreads idle times." << endl;
    return output;
}
//// end tomari quorum idle time
#endif

ostream&
Help::pragmaOvertaking( ostream& output, bool verbose ) const
{
    output << "This pragma is usesd to choose the overtaking" << ix( *this, "overtaking" ) << " approximation." << endl
	   << emph( *this, "Arg" ) << " must be one of: " << endl;
    return output;
}

ostream&
Help::pragmaProcessor( ostream& output, bool verbose ) const
{
    output << "Force the scheduling type" << ix( *this, "scheduling!processor" ) << ix( *this, "processor!scheduling" ) << " of all uni-processors to the type specfied." << endl;
    return output;
}

#if RESCHEDULE
ostream&
Help::pragmaReschedule( ostream& output, bool verbose ) const
{
    output << "Tasks are normally blocked after every rendezvous request, but" << endl
	   << "continue to execute after for send-no-reply.  This option changes" << endl
	   << "this behaviour." << endl;
    return output;
}
#endif

ostream&
Help::pragmaTau( ostream& output, bool verbose ) const
{
    output << "Set the tau adjustment factor to " << emph( *this, "arg" ) << "." << endl
	   << emph( *this, "Arg" ) << " must be an integer between 0 and 25." << endl
	   << "A value of " << emph( *this, "zero" ) << " disables the adjustment." << endl;
    return output;
}

ostream&
Help::pragmaThreads( ostream& output, bool verbose ) const
{
    output << "This pragma is used to change the behaviour of the solver when solving" << endl
	   << "models with fork-join" << ix( *this, "fork" ) << ix( *this, "join" ) << " interactions." << endl;
    return output;
}

ostream&
Help::pragmaVariance( ostream& output, bool verbose ) const
{
    output << "This pragma is used to choose the variance" << ix( *this, "variance" ) << " calculation used by the solver." << endl;
    return output;
}


ostream& 
Help::pragmaSeverityLevel( ostream& output, bool verbose ) const
{
    output << "This pragma is used to enable or disable warning messages." << endl;
    return output;
}

ostream&
Help::pragmaCyclesAllow( ostream& output, bool verbose ) const
{
    output << "Allow cycles in the call graph.  The interlock" << ix( *this, "interlock" ) << " adjustment is disabled." << endl;
    return output;
}

ostream&
Help::pragmaCyclesDisallow( ostream& output, bool verbose ) const
{
    output << "Disallow cycles in the call graph." << endl;
    return output;
}

ostream& 
Help::pragmaStopOnMessageLossFalse( ostream& output, bool verbose ) const
{
    output << "Ignore queue overflows" << ix( *this, "overflow" ) << " for open arrivals" << ix( *this, "open arrival!overflow" ) << " and send-no-reply" << ix( *this, "send-no-reply!overflow" ) << " requests.  If a queue overflows, its waiting times is reported as infinite." << ix( *this, "infinity" ) << "";
    return output;
}

ostream& 
Help::pragmaStopOnMessageLossTrue( ostream& output, bool verbose ) const
{
    output << "Stop if messages are lost." << endl;
    return output;
}

ostream&
Help::pragmaInterlockThroughput( ostream& output, bool verbose ) const
{
    output << "Perform interlocking by adjusting throughputs." << endl;
    return output;
}

ostream&
Help::pragmaInterlockNone( ostream& output, bool verbose ) const
{
    output << "Do not perform interlock adjustment." << endl;
    return output;
}


ostream&
Help::pragmaLayeringBatched( ostream& output, bool verbose ) const
{
    output << "Batched layering" << ix( *this, "batched layers" ) << ix( *this, "layering!batched" ) << " -- solve layers composed of as many servers as possible from top to bottom." << endl;
    return output;
}

ostream&
Help::pragmaLayeringBatchedBack( ostream& output, bool verbose ) const
{
    output << "Batched layering with back propagation -- solve layers composed of as many servers as possible from top to bottom, then from bottom to top to improve solution speed."  << endl;
    return output;
}

ostream&
Help::pragmaLayeringHwSw( ostream& output, bool verbose ) const
{
  output << "Hardware/software layers" << ix( *this, "hardware-software layers" ) << ix( *this, "layers!hardware-software" ) << " -- The model is solved using two submodels:" << endl
	 << "One consisting solely of the tasks in the model, and the other with the tasks calling the processors." << endl;
    return output;
}

ostream&
Help::pragmaLayeringMOL( ostream& output, bool verbose ) const
{
    output << "Method Of layers" << ix( *this, "method of layers" ) << ix( *this, "layering!method of" ) << " -- solve layers using the Method of Layers" << cite( *this, "perf:rolia-95-ieeese-mol" ) << ix( *this, "Method of Layers" ) << ix( *this, "layering!Method of Layers" ) << ". Layer spanning is performed by allowing clients to appear in more than one layer." << endl;
    return output;
}

ostream&
Help::pragmaLayeringMOLBack( ostream& output, bool verbose ) const
{
    output << "Method Of layers -- solve layers using the Method of Layers.  Software submodels are solved top-down then bottom up to improve solution speed." << endl;
    return output;
}

ostream&
Help::pragmaLayeringSRVN( ostream& output, bool verbose ) const
{
    output << "SRVN layers" << ix( *this, "srvn layers" ) << ix( *this, "layering!srvn" ) << " -- solve layers composed of only one server." << endl
	   << "This method of solution is comparable to the technique used by the " << bold( *this, "srvn" ) << " solver.  See also " << flag( *this, "P" ) << emph( *this, "mva" ) << "." << endl;
    return output;
}

ostream&
Help::pragmaLayeringSquashed( ostream& output, bool verbose ) const
{
    output << "Squashed layers" << ix( *this, "squashed layers" ) << ix( *this, "layering!squashed" ) << " -- All the tasks and processors are placed into one submodel." << endl
	   << "Solution speed may suffer because this method generates the most number of chains in the MVA solution.  See also " << flag( *this, "P" ) << emph( *this, "mva" ) << "." << endl;
    return output;
}

ostream&
Help::pragmaMultiServerDefault( ostream& output, bool verbose ) const
{
    output <<  "The default multiserver" << ix( *this, "multiserver!default" ) << " calculation uses the the Conway multiserver for multiservers with less than five servers, and the Rolia multiserver otherwise." << endl;
    return output;
}

ostream&
Help::pragmaMultiServerConway( ostream& output, bool verbose ) const
{
    output <<  "Use the Conway multiserver" << ix( *this, "multiserver!Conway" ) << cite( *this, "queue:deSouzaeSilva-87,queue:conway-88" ) << " calculation for all multiservers." << endl;
    return output;
}

ostream&
Help::pragmaMultiServerReiser( ostream& output, bool verbose ) const
{
    output <<  "Use the Reiser multiserver" << ix( *this, "multiserver!Reiser" ) << cite( *this, "queue:reiser-79" ) << " calculation for all multiservers." << endl;
    return output;
}

ostream&
Help::pragmaMultiServerReiserPS( ostream& output, bool verbose ) const
{
    output << "Use the Reiser multiserver calculation for all multiservers. For multiservers with multiple entries, scheduling is processor sharing" << ix( *this, "processor!sharing" ) << ", not FIFO. " << endl;
    return output;
}

ostream&
Help::pragmaMultiServerRolia( ostream& output, bool verbose ) const
{
    output <<  "Use the Rolia" << ix( *this, "multiserver!Rolia" ) << cite( *this, "perf:rolia-92,perf:rolia-95-ieeese-mol" ) << " multiserver calculation for all multiservers." << endl;
    return output;
}

ostream&
Help::pragmaMultiServerRoliaPS( ostream& output, bool verbose ) const
{
    output << "Use the Rolia multiserver calculation for all multiservers. For multiservers with multiple entries, scheduling is processor sharing" << ix( *this, "processor!sharing" ) << ", not FIFO. " << endl;
    return output;
}

ostream&
Help::pragmaMultiServerBruell( ostream& output, bool verbose ) const
{
    output <<  "Use the Bruell multiserver" << ix( *this, "multiserver!Bruell" ) << cite( *this, "queue:bruell-84-peva-load-dependent" ) << " calculation for all multiservers." << endl;
    return output;
}

ostream&
Help::pragmaMultiServerSchmidt( ostream& output, bool verbose ) const
{
    output <<  "Use the Schmidt multiserver" << ix( *this, "multiserver!Schmidt" ) << cite( *this, "queue:schmidt-97" ) << " calculation for all multiservers." << endl;
    return output;
}

ostream&
Help::pragmaMultiServerSuri( ostream& output, bool verbose ) const
{
    output <<  "experimental." << endl;
    return output;
}

ostream&
Help::pragmaMVALinearizer( ostream& output, bool verbose ) const
{
    output << "Linearizer." << ix( *this, "MVA!Linearizer" ) << ix( *this, "Linearizer" ) <<endl;
    return output;
}

ostream&
Help::pragmaMVAExact( ostream& output, bool verbose ) const
{
    output << "Exact MVA" << ix( *this, "MVA!exact" ) << ".  Not suitable for large systems." <<endl;
    return output;
}

ostream&
Help::pragmaMVASchweitzer( ostream& output, bool verbose ) const
{
    output << "Bard-Schweitzer approximate MVA." << ix( *this, "MVA!Bard-Schweitzer" ) << ix( *this, "Bard-Schweitzer" ) << "" <<endl;
    return output;
}

ostream&
Help::pragmaMVAFast( ostream& output, bool verbose ) const
{
    output << "Fast Linearizer" <<endl;
    return output;
}

ostream&
Help::pragmaMVAOneStep( ostream& output, bool verbose ) const
{
    output << "Perform one step of Bard Schweitzer approximate MVA for each iteration of a submodel.  The default is to perform Bard Schweitzer approximate MVA until convergence for each submodel.  This option, combined with " << flag( *this, "P" ) << emph( *this, "layering=srvn") << " most closely approximates the solution technique used by the " << bold( *this, "srvn" ) << " solver." <<endl;
    return output;
}

ostream&
Help::pragmaMVAOneStepLinearizer( ostream& output, bool verbose ) const
{
    output << "Perform one step of Linearizer approximate MVA for each iteration of a submodel.  The default is to perform Linearizer approximate MVA until convergence for each submodel." <<endl;
    return output;
}

ostream&
Help::pragmaOvertakingMarkov( ostream& output, bool verbose ) const
{
    output << "Markov phase 2 calculation." << ix( *this, "overtaking!Markov" ) << endl;
    return output;
}

ostream&
Help::pragmaOvertakingRolia( ostream& output, bool verbose ) const
{
    output << "Use the method from the Method of Layers." << ix( *this, "overtaking!Method of Layers" ) << endl;
    return output;
}

ostream&
Help::pragmaOvertakingSimple( ostream& output, bool verbose ) const
{
    output << "Simpler, but faster approximation." << endl;
    return output;
}

ostream&
Help::pragmaOvertakingSpecial( ostream& output, bool verbose ) const
{
    output << "?" <<endl;
    return output;
}

ostream&
Help::pragmaOvertakingNone( ostream& output, bool verbose ) const
{
    output << "Disable all second phase servers.  All stations are modeled as having a single phase by summing the phase information." <<endl;
    return output;
}

ostream&
Help::pragmaProcessorDefault( ostream& output, bool verbose ) const
{
    output << "The default is to use the processor scheduling specified in the model." <<endl;
    return output;
}

ostream&
Help::pragmaProcessorFCFS( ostream& output, bool verbose ) const
{
    output << "All uni-processors are scheduled first-come, first-served." <<endl;
    return output;
}

ostream&
Help::pragmaProcessorHOL( ostream& output, bool verbose ) const
{
    output << "All uni-processors are scheduled using head-of-line priority." << ix( *this, "priority!head of line" ) << endl;
    return output;
}

ostream&
Help::pragmaProcessorPPR( ostream& output, bool verbose ) const
{
    output << "All uni-processors are scheduled using priority, pre-emptive resume." << ix( *this, "priority!preemptive-resume" ) << endl;
    return output;
}

ostream&
Help::pragmaProcessorPS( ostream& output, bool verbose ) const
{
    output << "All uni-processors are scheduled using processor sharing." << ix( *this, "processor sharing" ) << endl;
    return output;
}

ostream&
Help::pragmaThreadsNone( ostream& output, bool verbose ) const
{
    output << "Do not perform overlap calculation for forks." << ix( *this, "overlap calculation" ) << endl;
    return output;
}

ostream&
Help::pragmaThreadsMak( ostream& output, bool verbose ) const
{
    output << "Use Mak-Lundstrom" << ix( *this, "Mak-Lundstrom" ) << cite( *this, "perf:mak-90" ) << " approximations for join delays." << ix( *this, "join!delay" ) << endl;
    return output;
}

ostream&
Help::pragmaThreadsHyper( ostream& output, bool verbose ) const
{
    output << "Inflate overlap probabilities based on arrival instant estimates." <<endl;
    return output;
}

ostream&
Help::pragmaThreadsExponential( ostream& output, bool verbose ) const
{
    output << "Use exponential values instead of three-point approximations in all approximations" << ix( *this, "three-point approximation" ) << cite( *this, "perf:jiang-96" ) << "." <<endl;
    return output;
}


ostream&
Help::pragmaThreadsDefault( ostream& output, bool verbose ) const
{
    output << "?" <<endl;
    return output;
}

ostream& 
Help::pragmaVarianceDefault( ostream& output, bool verbose ) const
{
    return output;
}

ostream&
Help::pragmaVarianceNone( ostream& output, bool verbose ) const
{
    output << "Disable variance adjustment.  All stations in the MVA submodels are either delay- or FIFO-servers." <<endl;
    return output;
}

ostream&
Help::pragmaVarianceStochastic( ostream& output, bool verbose ) const
{
    output << "?" <<endl;
    return output;
}

ostream&
Help::pragmaVarianceMol( ostream& output, bool verbose ) const
{
    output << "Use the MOL variance calculation." << ix( *this, "variance!Method of Layers" ) << ix( *this, "Method of Layers!variance" ) << endl;
    return output;
}

ostream&
Help::pragmaVarianceNoEntry( ostream& output, bool verbose ) const
{
    output << "By default, any task with more than one entry will use the variance calculation.  This pragma will switch off the variance calculation for tasks with only one entry." <<endl;
    return output;
}

ostream&
Help::pragmaVarianceInitOnly( ostream& output, bool verbose ) const
{
    output << "Initialize the variances, but don't recompute as the model is solved." << ix( *this, "variance!initialize only" ) << endl;
    return output;
}

ostream& 
Help::pragmaSeverityLevelWarnings( ostream& output, bool verbose ) const
{
    return output;
}

ostream& 
Help::pragmaSeverityLevelRunTime( ostream& output, bool verbose ) const
{
    return output;
}

#if HAVE_LIBGSL && HAVE_LIBGSLCBLAS
ostream&
Help::pragmaQuorumDistributionDefault( ostream& output, bool verbose ) const
{
    output <<  "use default quorum settings" << endl;
    return output;
}

ostream&
Help::pragmaQuorumDistributionThreepoint( ostream& output, bool verbose ) const
{
    output <<  "estimation of a thread service time using a threepoint distribution." << endl;
    return output;
}

ostream&
Help::pragmaQuorumDistributionGamma( ostream& output, bool verbose ) const
{
    output <<  "estimation of a thread service time using a Gamma distribution" << endl;
    return output;
}

ostream&
Help::pragmaQuorumDistributionClosedFormGeo( ostream& output, bool verbose ) const
{
    output << "estimation of a thread service time using a closed-form  for geometric calls formula distribution." << endl;
    return output;
}

ostream&
Help::pragmaQuorumDistributionClosedformDet( ostream& output, bool verbose ) const
{
    output << "estimation of a thread service time using a closed-form  for deterministic calls formula distribution." << endl;
    return output;
}

ostream&
Help::pragmaMultiServerDefault( ostream& output, bool verbose ) const
{
    output <<  "use default quorum delayed calls settings" << endl;
    return output;
}

ostream&
Help::pragmaDelayedThreadsKeepAll( ostream& output, bool verbose ) const
{
    output <<  "keep both delayed local calls and delayed remote calls running after the quorum join." << endl;
    return output;
}

ostream&
Help::pragmaDelayedThreadsAbortAll( ostream& output, bool verbose ) const
{
    output <<  "abort both delayed local calls and delayed remote calls after the quorum join." << endl;
    return output;
}

ostream&
Help::pragmaDelayedThreadsAbortLocalOnly( ostream& output, bool verbose ) const
{
    output <<  "abort delayed local calls and keep delayed remote calls running after the quorum join." << endl;
    return output;
}

ostream&
Help::pragmaDelayedThreadsAbortRemoteOnly( ostream& output, bool verbose ) const
{
    output <<  "abort delayed remote calls and keep delayed local calls running after the quorum join." << endl;
    return output;
}

ostream&
Help::pragmaIdleTimeDefault( ostream& output, bool verbose ) const
{
    output <<  "use default idle settings" << endl;
    return output;
}

ostream&
Help::pragmaIdleTimeJoindelay( ostream& output, bool verbose ) const
{
    output <<  "Use And-Fork join delays throughputs to set threads idle times." << endl;
    return output;
}

ostream&
Help::pragmaIdleTimeRootentry( ostream& output, bool verbose ) const
{
    output <<  "Use root Entry throughput to set threads idle times." << endl;
    return output;
}
#endif

/* -------------------------------------------------------------------- */

const char * HelpTroff::__comment = ".\\\"";

ostream&
HelpTroff::preamble( ostream& output ) const
{
    char date[32];
    time_t tloc;
    time( &tloc );

#if defined(HAVE_CTIME)
    strftime( date, 32, "%d %B %Y", localtime( &tloc ) );
#endif

    output << __comment << " t -*- nroff -*-" << endl
	   << ".TH lqns 1 \"" << date << "\" \"" << VERSION << "\"" << endl;

    output << __comment << " $Id: help.cc 12550 2016-04-06 22:33:52Z greg $" << endl
	   << __comment << endl
	   << __comment << " --------------------------------" << endl;

    output << ".SH \"NAME\"" << endl;
    output << "lqns \\- solve layered queueing network models."
	   << endl;

    output << ".SH \"SYNOPSIS\"" << endl
	 << ".br" << endl
	 << ".B lqns" << endl
	 << "[\\fIOPTIONS\\fR].\\|.\\|. "
	 << "[\\fIFILE\\fR] \\&.\\|.\\|." << endl;

    output << ".SH \"DESCRIPTION\"" << endl;
    output << bold( *this, "Lqns" ) << ix( *this, "LQNS" ) << " is used to solve layered queueing network models using " << endl
	   << "analytic techniques.  Models can be specified using the LQN modeling" << endl
	   << "language, or with extensible markup language (XML).  Refer to the" << endl
	   << emph( *this, "``Layered Queueing Network Solver and Simulator User Manual''" ) << endl
	   << "for details of the model and for a complete description of the input file" << endl
	   << "formats for the program." << endl;
    pp( output );
    return output;
}


ostream&
HelpTroff::see_also( ostream& output ) const
{
    section( output, "SEE ALSO", "Refereneces" );
    output << "Greg Franks el. al., ``Enhanced Modeling and Solution of Layered" << endl
	   << "Queueing Networks'', " << emph( *this, "IEEE Trans. Soft. Eng." ) << ", Vol. 35, No. 2, Mar-Apr 2990, pp. 148-161." << endl;
    br( output );
    output << "C. M. Woodside et. al., ``The Stochastic Rendezvous Network" << endl
	   << "Model for Performance of Synchronous Multi-tasking Distributed" << endl
	   << "Software'', " << emph( *this, "IEEE Trans. Comp." ) << ", Vol. 44, No. 8, Aug 1995, pp. 20-34." << endl;
    br( output );
    output << "J. A. Rolia and K. A. Sevcik, ``The Method of Layers'', " << emph( *this, "IEEE Trans. SE" )
	   << ", Vol. 21, No. 8, Aug. 1995, pp 689-700." << endl;
    br( output );
    output << emph( *this, "``Layered Queueing Network Solver and Simulator User Manual''") << endl;
    br( output );
    output << emph( *this, "``Tutorial Introduction to Layered Modeling of Software Performance''" ) << endl;
    br( output );
    output << "lqsim(1), lqn2ps(1), srvndiff(1), egrep(1)," << endl
	   << "floating_point(3)" << endl;
    return output;
}



ostream&
HelpTroff::textbf( ostream& output, const char * s ) const
{
    output << "\\fB" << s << "\\fP";
    return output;
}


ostream&
HelpTroff::textit( ostream& output, const char * s ) const
{
    output << "\\fI" << s << "\\fP";
    return output;
}

ostream&
HelpTroff::filename( ostream& output, const char * s1, const char * s2 ) const
{
    output << "\\fI" << s1;
    if ( s2 ) {
	output << "\\fB" << s2;
    }
    output << "\\fR";
    return output;
}


ostream&
HelpTroff::pp( ostream& output ) const
{
    output << ".PP" << endl;
    return output;
}

ostream&
HelpTroff::br( ostream& output ) const
{
    output << ".LP" << endl;
    return output;
}

ostream&
HelpTroff::ol_begin( ostream& output ) const
{
    return output;
}


ostream&
HelpTroff::ol_end( ostream& output ) const
{
    return output;
}


ostream&
HelpTroff::dl_begin( ostream& output ) const
{
    return output;
}


ostream&
HelpTroff::dl_end( ostream& output ) const
{
    return output;
}


ostream&
HelpTroff::li( ostream& output, const char * s  ) const
{
    output << ".TP 3" << endl;
    if ( s ) output << s << endl;
    return output;
}

ostream&
HelpTroff::flag( ostream& output, const char * s ) const
{
    output << "\\fB\\-" << s << "\\fP";
    return output;
}

ostream&
HelpTroff::section( ostream& output, const char * s, const char * ) const
{
    output << ".SH \"" << s << "\"" << endl;
    return output;
}

ostream&
HelpTroff::label( ostream& output, const char * s ) const
{
    return output;
}

ostream&
HelpTroff::longopt( ostream& output, const struct option *o ) const
{
    output << ".TP" << endl;
    if ( isgraph(o->val) ) {
	const char b[2] = { static_cast<char>(o->val), '\0' };
	flag( output, &b[0] );
	if ( o->name ) {
	    output << ", ";
	}
    }
    if ( o->name ) {
	output << "\\fB\\-\\-" << o->name << "\\fR";
    }

    if ( o->has_arg ) {
	output << "=\\fIarg\\fR";
    }
    output << endl;
    return output;
}


ostream&
HelpTroff::increase_indent( ostream& output ) const
{
    output << ".RS" << endl;
    return output;
}

ostream&
HelpTroff::decrease_indent( ostream& output ) const
{
    output << ".RE" << endl;
    return output;
}

ostream&
HelpTroff::print_option( ostream& output, const char * name, const Options::Option& opt ) const
{
    output << ".TP" << endl
	   << "\\fB" << name;
    if ( opt.hasArg() ) {
	output << "\\fR=\\fI" << "arg";
    }
    output << "\\fR" << endl;
    help_fptr h = opt.help();
    if ( h ) {
	(this->*h)(output,true);
    }
    return output;
}

ostream&
HelpTroff::print_pragma( ostream& output, const char * name, const void * vp ) const
{
    const Pragma::pragma_info * p = static_cast<const Pragma::pragma_info *>(vp);
    if ( !p->_help ) return output;

    Help::help_fptr h = 0;
    const char * default_param = 0;

    output << ".TP" << endl
	   << "\\fB" << name << "\\fR=\\fIarg";
    /* Enumeration */
    output << "\\fR" << endl;
    (this->*(p->_help))( output,true );
    /* Comment */
    if ( p->_value && p->_value->size() > 1 ) {
	output << ".RS" << endl;
	std::map<const char *, Pragma::param_info, lt_str>::const_iterator arg;
	std::map<const char *, Pragma::param_info, lt_str> * arg_list = p->_value;
	for ( arg = arg_list->begin(); arg != arg_list->end(); ++arg ) {
	    if ( strcasecmp( arg->first, "default" ) == 0 ) {
		h = arg->second._h;
		continue;
	    } else {
		output << ".TP" << endl;
		output << "\\fB" << arg->first << "\\fP" << endl;
		(this->*(arg->second._h))( output, true );
		if ( arg->second._i == 0 ) {
		    default_param = arg->first;
		}
	    }
	}
	output << ".LP" << endl;
	if ( h ) {
	    (this->*h)( output, true ) << endl;
	} else if ( default_param ) {
	    output << "The default is " << default_param << "." << endl;
	}
	output << ".RE" << endl;
    }
    return output;
}

ostream&
HelpTroff::table_header( ostream& output ) const
{
    output << __comment << "--------------------------------------------------------------------" << endl
	   << __comment << " Table Begin" << endl
	   << __comment << "--------------------------------------------------------------------" << endl
	   << ".ne 20" << endl
	   << ".TS" << endl
	   << "center tab (&) ;" << endl
	   << "lw(30x) le ." << endl
	   << "Parameter&lqns" << endl
	   << "=" << endl;
    return output;
}


ostream&
HelpTroff::table_row( ostream& output, const char * col1, const char * col2, const char * index ) const
{
    output << "T{" << endl << col1 << endl << "T}&T{" << endl << col2 << endl << "T}" << endl;
    return output;
}

ostream&
HelpTroff::table_footer( ostream& output ) const
{
    output << "_" << endl
	   << ".TE" << endl;
    return output;
}

/* -------------------------------------------------------------------- */

const char * HelpLaTeX::__comment = "%%";

ostream&
HelpLaTeX::preamble( ostream& output ) const
{
    char date[32];
#if defined(HAVE_CTIME)
    time_t tloc;
    time( &tloc );

    strftime( date, 32, "%d %B %Y", localtime( &tloc ) );
#endif

    output << __comment << "  -*- mode: latex; mode: outline-minor; fill-column: 108 -*- " << endl
	   << __comment << " Title:  lqns" << endl
	   << __comment << "" << endl
	   << __comment << " $HeadURL: http://rads-svn.sce.carleton.ca:8080/svn/lqn/trunk-V5/lqns/help.cc $" << endl
	   << __comment << " Original Author:     Greg Franks <greg@sce.carleton.ca>" << endl
	   << __comment << " Created:             " << date << endl
	   << __comment << "" << endl
	   << __comment << " ----------------------------------------------------------------------" << endl
	   << __comment << " $Id: help.cc 12550 2016-04-06 22:33:52Z greg $" << endl
	   << __comment << " ----------------------------------------------------------------------" << endl << endl;

    output << "\\chapter{Invoking the Analytic Solver ``lqns''}" << endl
	   << "\\label{sec:invoking-lqns}" << endl
	   << "The Layered Queueing Network Solver (LQNS)\\index{LQNS} is used to" << endl
	   << "solved Layered Queueing Network models analytically." << endl;

    return output;
}


ostream&
HelpLaTeX::textbf( ostream& output, const char * s ) const
{
    output << "\\textbf{" << s << "}";
    return output;
}


ostream&
HelpLaTeX::textit( ostream& output, const char * s ) const
{
    output << "\\emph{" << s << "}";
    return output;
}

ostream&
HelpLaTeX::tr_( ostream& output, const char * s ) const
{
    for ( ; *s; ++s ) {
	if ( *s == '_' ) {
	    output << "\\";
	}
	output << *s;
    }
    return output;
}


ostream&
HelpLaTeX::filename( ostream& output, const char * s1, const char * s2 ) const
{
    output << "\\texttt{" << s1;
    if ( s2 ) {
	output << s2;
    }
    output << "}";
    return output;
}

ostream&
HelpLaTeX::pp( ostream& output ) const
{
    output << endl << endl;
    return output;
}

ostream&
HelpLaTeX::br( ostream& output ) const
{
    output << "\\linebreak[4]" << endl;
    return output;
}

ostream&
HelpLaTeX::ol_begin( ostream& output ) const
{
    output << "\\begin{enumerate}" << endl;
    return output;
}


ostream&
HelpLaTeX::ol_end( ostream& output ) const
{
    output << "\\end{enumerate}" << endl;
    return output;
}


ostream&
HelpLaTeX::dl_begin( ostream& output ) const
{
    output << "\\begin{description}" << endl;
    return output;
}


ostream&
HelpLaTeX::dl_end( ostream& output ) const
{
    output << "\\end{description}" << endl;
    return output;
}


ostream&
HelpLaTeX::li( ostream& output, const char * s  ) const
{
    output << "\\item ";
    return output;
}

ostream&
HelpLaTeX::flag( ostream& output, const char * s ) const
{
    output << "\\flag{" << s << "}{}";
    return output;
}

ostream&
HelpLaTeX::ix( ostream& output, const char * s ) const
{
    output << "\\index{" << s << "}";
    return output;
}

ostream&
HelpLaTeX::cite( ostream& output, const char * s ) const
{
    output << "~\\cite{" << s << "}";
    return output;
}

ostream&
HelpLaTeX::section( ostream& output, const char *, const char * s ) const
{
    output << "\\section{" << s << "}" << endl;
    return output;
}

ostream&
HelpLaTeX::label( ostream& output, const char * s ) const
{
    output << "\\label{" << s << "}" << endl;
    return output;
}


ostream&
HelpLaTeX::longopt( ostream& output, const struct option *o ) const
{
    output << "\\item[";
    if ( isgraph(o->val) ) {
	output << "\\flag{" << static_cast<char>(o->val) << "}{}";
    }
    if ( o->name ) {
	if ( isgraph(o->val) ) output << ", ";
	output << "\\longopt{" << o->name << "}";
    }
    if ( o->has_arg ) {
	output << "=\\emph{arg}";
    }
    output << "]~\\\\" << endl;
    return output;
}

ostream&
HelpLaTeX::increase_indent( ostream& output ) const
{
    return output;
}

ostream&
HelpLaTeX::decrease_indent( ostream& output ) const
{
    return output;
}

ostream&
HelpLaTeX::print_option( ostream& output, const char * name, const Options::Option& opt ) const
{
    output << "\\item[\\optarg{" << tr_( *this, name ) << "}{";
    if ( opt.hasArg() ) {
	output << "=" << emph( *this, "arg" );
    }
    output << "}]" << endl;
    help_fptr f = opt.help();
    if ( f ) {
	(this->*f)(output, true);
    }
    return output;
}

ostream&
HelpLaTeX::print_pragma( ostream& output, const char * name, const void * vp ) const
{
    const Pragma::pragma_info * p = static_cast<const Pragma::pragma_info *>(vp);
    if ( !p->_help ) return output;

    Help::help_fptr h = 0;
    const char * default_param = 0;

    output << "\\item[\\optarg{" << tr_( *this, name ) << "}{=\\emph{arg}}]~\\\\" << endl;
    (this->*(p->_help))( output, true );
    /* Comment */
    if ( p->_value && p->_value->size() > 1 ) {
	std::map<const char *, Pragma::param_info, lt_str>::const_iterator arg;
	std::map<const char *, Pragma::param_info, lt_str> * a = p->_value;
	dl_begin( output );
	for ( arg = a->begin(); arg != a->end(); ++arg ) {
	    if ( strcasecmp( arg->first, "default" ) == 0 ) {
		h = arg->second._h;
		continue;
	    } else {
		output << "\\item[\\optarg{" << tr_( *this, arg->first ) << "}{}]" << endl;
		(this->*(arg->second._h))( output, true );
		if ( arg->second._i == 0 ) {
		    default_param = arg->first;
		}
	    }
	}
	dl_end( output );
	if ( h ) {
	    (this->*h)( output, true ) << endl;
	} else if ( default_param ) {
	    output << "The default is " << default_param << "." << endl;
	}
    }
    return output;
}


ostream&
HelpLaTeX::table_header( ostream& output ) const
{
    output << __comment << "--------------------------------------------------------------------" << endl
	   << __comment << " Table Begin" << endl
	   << __comment << "--------------------------------------------------------------------" << endl
	   << "\\begin{table}[htbp]" << endl
	   << "  \\centering" << endl
	   << "  \\begin{tabular}[c]{ll}" << endl
	   << "    Parameter&lqns \\\\" << endl
	   << "    \\hline" << endl;
    return output;
}


ostream&
HelpLaTeX::table_row( ostream& output, const char * col1, const char * col2, const char * index ) const
{
    output << "    " << col1;
    if ( index ) {
	output << "\\index{ " << index << "}";
    }
    output << " & " << col2 << "\\\\" << endl;
    return output;
}


ostream&
HelpLaTeX::table_footer( ostream& output ) const
{
    output << "    \\hline" << endl
	   << "  \\end{tabular}" << endl
	   << "  \\caption{\\label{tab:lqns-model-limits}LQNS Model Limits\\index{limits!lqns}.}" << endl
	   << "\\end{table}" << endl;
    return output;
}


ostream&
HelpLaTeX::trailer( ostream& output ) const
{
    output << "%%% Local Variables: " << endl
	   << "%%% mode: latex" << endl
	   << "%%% mode: outline-minor " << endl
	   << "%%% fill-column: 108" << endl
	   << "%%% TeX-master: \"userman\"" << endl
	   << "%%% End: " << endl;
    return output;
}

/* -------------------------------------------------------------------- */

ostream&
HelpPlain::print_option( ostream& output, const char * name, const Options::Option& opt ) const
{
    ios_base::fmtflags oldFlags = output.setf( ios::left, ios::adjustfield );
    string s = name;
    if ( opt.hasArg() ) {
	s += "=arg";
    }
    output << " " << setw(26) << s << " ";
    help_fptr h = opt.help();
    if ( h ) {
	(this->*h)(output,false);
    }
    output.flags(oldFlags);
    return output;
}

ostream&
HelpPlain::print_pragma( ostream& output, const char * name, const void * vp ) const
{
    return output;
}


ostream&
HelpPlain::textbf( ostream& output, const char * s ) const
{
    output << s;
    return output;
}


ostream&
HelpPlain::textit( ostream& output, const char * s ) const
{
    output << s;
    return output;
}

ostream&
HelpPlain::filename( ostream& output, const char * s1, const char * s2 ) const
{
    output << s1;
    if ( s2 ) {
	output << s2;
    }
    return output;
}

void
HelpPlain::print_special( ostream& output ) 
{
    Options::Special::initialize();
    HelpPlain self;

    output << "Valid arguments for --special" << endl;
    for ( std::map<const char *, Options::Special, lt_str>::const_iterator tp = Options::Special::__table.begin(); tp != Options::Special::__table.end(); ++tp ) {
	self.print_option( output, tp->first, tp->second );
    }
}


void
HelpPlain::print_trace( ostream& output ) 
{
    Options::Trace::initialize();
    HelpPlain self;
    output << "Valid arguments for --trace" << endl;
    for ( std::map<const char *, Options::Trace, lt_str>::const_iterator tp = Options::Trace::__table.begin(); tp != Options::Trace::__table.end(); ++tp ) {
	self.print_option( output, tp->first, tp->second );
    }
}


void
HelpPlain::print_debug( ostream& output ) 
{
    Options::Debug::initialize();
    HelpPlain self;
    output << "Valid arguments for --debug" << endl;
    for ( std::map<const char *, Options::Debug, lt_str>::const_iterator tp = Options::Debug::__table.begin(); tp != Options::Debug::__table.end(); ++tp ) {
	self.print_option( output, tp->first, tp->second );
    }
}
