/* help.cc	-- Greg Franks Thu Mar 27 2003
 *
 * $Id$
 */

#include "lqn2ps.h"
#if HAVE_UNISTD_H
#include <unistd.h>
#include <time.h>
#endif
#include <cstring>

class HelpManip {
public:
    HelpManip( ostream& (*ff)(ostream&, const option_type&, const int ), const option_type& o, const int j = 0 )
	: _o(o), _j(j), f(ff) {}
private:
    const option_type _o;
    const int _j;
    ostream& (*f)( ostream&, const option_type& o, const int j );

    friend ostream& operator<<(ostream & os, const HelpManip& m ) 
	{ return m.f(os,m._o,m._j); }
};


static HelpManip current_arg( unsigned int i, const int j );
static HelpManip current_flag( unsigned int i );
static HelpManip current_option( unsigned int i );
static HelpManip default_setting( unsigned int i );
static HelpManip print_args( unsigned int i );


/*
 * Print out usage string.
 */

void 
usage( const bool full_usage ) 
{
    cerr << "Usage: " << io_vars.lq_toolname << " [OPTION]... [FILE]..." << endl;
    cerr << "Options:" << endl;
    if ( !full_usage ) {
	cerr << "[(+|-)";
	for ( unsigned i = 0; Flags::print[i].name != 0; ++i ) {
	    if ( (Flags::print[i].c & 0xff00) == 0 && !Flags::print[i].arg ) {
		cerr << static_cast<char>(Flags::print[i].c);
	    }
	}
	cerr << ']';

	for ( unsigned int j = 0, i = 0; Flags::print[i].name != 0; ++i ) {
	    if ( (Flags::print[i].c & 0xff00) == 0 && Flags::print[i].arg ) {
		if ( (j % 4) == 0 ) cerr << endl << "   ";
		cerr << " [-" << static_cast<char>(Flags::print[i].c) << " <" << Flags::print[i].arg << ">]";
		++j;
	    }
	}
	cerr << endl << endl;
	return;
    }

#if HAVE_GETOPT_LONG
    for ( unsigned int i = 0; Flags::print[i].name != 0; ++i ) {
	string s;
	if ( (Flags::print[i].c & 0xff00) != 0 ) {
	    s = "         --";
	    if ( (Flags::print[i].c & 0xff00) == 0x0300 ) {
		s += "[no]-";
	    } 
	} else if ( Flags::print[i].arg == 0 && islower( Flags::print[i].c ) ) {
	    s = " -";
	    s += Flags::print[i].c;
	    s += ", +";
	    s += Flags::print[i].c;
	    s += ", --[no-]";
	} else {
	    s = " -";
	    s += Flags::print[i].c;
	    s += ",     --";
	}
	s += Flags::print[i].name;
	if ( Flags::print[i].arg ) {
	    s += "=";
	    if ( strcmp( Flags::print[i].name, Flags::print[i].arg ) == 0 ) {
		if ( Flags::print[i].opts == Options::real ) {
		    s += "N.N"; 
		} else if ( Flags::print[i].opts == Options::integer ) {
		    s += "N"; 
		} else {
		    s += "ARG";
		}
	    } else {
		s += Flags::print[i].arg;
	    }
	}
	cerr.setf( ios::left, ios::adjustfield );
	cerr << setw(40) << s << Flags::print[i].msg << " " << print_args( i ) << endl;
    }
#else
    for ( unsigned i = 0; i < N_FLAG_VALUES; ++i ) {
	if ( !Flags::print[i].arg ) {
	    cerr << "(+|-)" << Flags::print[i].c << "  " << Flags::print[i].msg 
		 << " (" << (Flags::print[i].value.b ? "true" : "false") << ")" << endl;
	}
    }
    for ( i = 0; i < N_FLAG_VALUES; ++i ) {
	if ( Flags::print[i].arg ) {
	    cerr << flag_value(i) << endl;
	}
    }
#endif
}



void
invalid_option( char c, char * optarg )
{
#if HAVE_GETOPT_LONG
    for ( unsigned int i = 0; i < N_FLAG_VALUES; ++i ) {
	if ( Flags::print[i].c == c ) {
    	    cerr << io_vars.lq_toolname << ": Invalid argument to --" << Flags::print[i].name << ", ARG=" << optarg << endl;
	    cerr << "    " << print_args( i ) << endl;
	}
    }
#else
    cerr << io_vars.lq_toolname << ": Invalid argument to -" << static_cast<char>(c & 0x7f) << optarg << endl;
#endif
}



/*
 * Make a man page :-)
 */

void
man()
{
    unsigned i;
    static const char * comm = ".\\\"";
    char date[32];
    time_t tloc;
    time( &tloc );

#if defined(HAVE_CTIME)
    strftime( date, 32, "%d %B %Y", localtime( &tloc ) );
#endif

    cout << comm << " -*- nroff -*-" << endl
	 << ".TH lqn2ps 1 \"" << date << "\"  \"" << VERSION << "\"" << endl;


    cout << comm << " $Id$" << endl
	 << comm << endl
	 << comm << " --------------------------------" << endl;

    cout << ".SH \"NAME\"" << endl;
    bool printed = false;
    for ( i = 0; Options::io[i]; ++i ) {
	switch ( i ) {
#if defined(EMF_OUTPUT)
	case FORMAT_EMF:
#endif
	case FORMAT_FIG:
#if HAVE_GD_H && HAVE_LIBGD && HAVE_GDIMAGEGIFPTR
	case FORMAT_GIF:
#endif
#if HAVE_GD_H && HAVE_LIBGD && HAVE_LIBJPEG 
	case FORMAT_JPEG:
#endif
	case FORMAT_OUTPUT:
#if HAVE_GD_H && HAVE_LIBGD && HAVE_LIBPNG
	case FORMAT_PNG:
#endif
	case FORMAT_POSTSCRIPT:
	case FORMAT_SRVN:
	case FORMAT_SXD:
	case FORMAT_XML:
	    if ( printed ) {
		cout << ", ";
	    } else {
		printed = true;
	    }
	    cout << "lqn2" << Options::io[i];
	}
    }
    cout << " \\- format and translate layered queueing network models."
	 << endl;

    cout << ".SH \"SYNOPSIS\"" << endl 
	 << ".br" << endl
	 << ".B lqn2ps" << endl
	 << "[\\fIOPTION \\&.\\|.\\|.\\fP]" << endl;
    cout << "[" << endl
	 << "FILE \\&.\\|.\\|." << endl
	 << "]" << endl;

    cout << ".SH \"DESCRIPTION\"" << endl
	 << "\\fBlqn2ps\\fR, \\fBlqn2lqn\\fR, \\fBlqn2xml\\fR, and \\fBlqn2out\\fR" << endl
    	 << "are used to transform an LQN input file (with results) into:" << endl
	 << "graphical output such as PostScript," << endl
	 << "a new old-style input file," << endl
	 << "a new XML input/output file," << endl
	 << "and textual output respectively." << endl
	 << "In general, the name of the program is used to specify the output format, " << endl
	 << "although the " << current_flag( OUTPUT_FORMAT ) << " switch always overrides this behaviour." << endl
	 << "Refer to the " << current_flag( OUTPUT_FORMAT ) << " switch described below for the output formats supported." << endl;
    cout << ".PP" << endl
	 << "\\fBlqn2ps\\fR reads its input from \\fIfilename\\fR, specified at the" << endl
	 << "command line if present, or from the standard input otherwise.  Output" << endl
	 << "for an input file \\fIfilename\\fR specified on the command line will be" << endl
	 << "placed in the file \\fIfilename.ext\\fR, where \\fI.ext\\fR depends on the" << endl
	 << "conversion taking place (see the " << current_flag( OUTPUT_FORMAT ) << " switch)." << endl
	 << "If \\fBlqn2lqn\\fR or \\fBlqn2xml\\fR is used to reformat an existing LQN-type or XML input file respectively," << endl
	 << "the output is written back to the original file name." << endl
	 << "The original file is renamed to \\fIfilename\\fB~\\fR." << endl
	 << "If several input files are given, then each is treated as a separate model and" << endl
	 << "output will be placed in separate output files.  This behaviour can be changed" << endl
	 << "using the " << current_flag( OUTPUT_FILE ) << " option, described below.  If input is from the" << endl
	 << "standard input, output will be directed to the standard output.  The" << endl
	 << "file name `\\-' is used to specify standard input." << endl;

    cout << ".SH \"OPTIONS\"" << endl;
    for ( i = 0; i < N_FLAG_VALUES; ++i ) {
	cout << ".TP" << endl;
	cout << "\\fB";
	if ( isascii(Flags::print[i].c) && isgraph(Flags::print[i].c) ) {
	    if ( islower(Flags::print[i].c) && Flags::print[i].arg == 0 ) {
		cout << "(\\-|+)";
	    } else {
		cout << "\\-";
	    }
	    cout << static_cast<char>(Flags::print[i].c) << "\\fR";
	    if ( Flags::print[i].name ) {
		cout << ", \\fB";
	    }
	}
	if ( Flags::print[i].name ) {
	    cout << "\\-\\-";
	    if ( isascii(Flags::print[i].c) && isgraph(Flags::print[i].c)  && islower(Flags::print[i].c) && Flags::print[i].arg == 0 ) {
		cout << "[no-]";
	    }
	    cout << Flags::print[i].name;
	}
	if ( Flags::print[i].arg ) {
	    cout << "=\\fI" << Flags::print[i].arg;
	}
	cout << "\\fR" << endl;

	/* Description goes here... */
	switch ( Flags::print[i].c ) {
	default:
	    cout << Flags::print[i].msg;
	    if ( isascii(Flags::print[i].c) && isgraph( Flags::print[i].c ) && islower(Flags::print[i].c) ) {
		cout << " " << default_setting(i);
	    }
	    cout  << endl;
	    break;

	case 'A':
	    cout << "The " << current_option(i) << "is used to aggregate objects." << endl
		 << ".RS" << endl;
	    cout << ".TP" << endl 
		 << "\\fB" << current_arg(i,AGGREGATE_NONE) << "\\fR" << endl
		 << "Don't aggregate objects." << endl;
	    cout << ".TP" << endl
		 << "\\fB" << current_arg(i,AGGREGATE_SEQUENCES) << "\\fR" << endl
		 << "Aggregate sequences of activities into a single activity." << endl;
	    cout << ".TP" << endl
		 << "\\fB" << current_arg(i,AGGREGATE_ACTIVITIES) << "\\fR" << endl
		 << "Aggregate activities called by an entry into the entry." << endl;
	    cout << ".TP" << endl
		 << "\\fB" << current_arg(i,AGGREGATE_PHASES) << "\\fR" << endl
		 << "Aggregate activities called by an entry into the entry and remove all phases." << endl;
	    cout << ".PP" << endl
		 << "A new model that results from aggregation may not necessarily have the same solution as the original model." << endl
		 << "An aggregated model is smaller, so it will solve faster." << endl
		 << ".LP" << endl
		 << default_setting(i) << endl;
	    cout << ".RE" << endl;
	    break;

	case 'C':
	    cout << "The " << current_option(i) << "is used to choose how to colour objects." << endl
		 << ".RS" << endl;
	    cout << ".TP" << endl 
		 << "\\fB" << current_arg(i,COLOUR_OFF) << "\\fR" << endl
		 << "Use gray scale instead of colour for results.";
	    cout << ".TP" << endl 
		 << "\\fB" << current_arg(i,COLOUR_RESULTS) << "\\fR" << endl
		 << "Colour nodes based on utilization and arcs based on the utilization of the destination.  This is the default.";
	    cout << ".TP" << endl 
		 << "\\fB" << current_arg(i,COLOUR_LAYERS) << "\\fR" << endl
		 << "Colour nodes based on their layer.";
	    cout << ".TP" << endl 
		 << "\\fB" << current_arg(i,COLOUR_CLIENTS) << "\\fR" << endl
		 << "Colour nodes based on their client(s).";
	    cout << ".TP" << endl 
		 << "\\fB" << current_arg(i,COLOUR_SERVER_TYPE) << "\\fR" << endl
		 << "Client tasks are coloured red, server tasks are coloured blue.";
	    cout << ".TP" << endl 
		 << "\\fB" << current_arg(i,COLOUR_CHAINS) << "\\fR" << endl
		 << "Queueing output only: colour each chain differently.";
	    cout << ".RE" << endl;
	    break;

	case 'I':
	    cout << "The " << current_option(i) << " is used to force the input file format to either \\fIxml\\fR, or \\fIlqn\\fR." << endl
		 << "By default, if the suffix of the input filename is one of: \\fI.in\\fR, \\fI.lqn\\fR, \\fI.xlqn\\fR, of \\fI.txt\\fR," << endl
		 << "then the LQN parser will be used.  Otherwise, input is assumed to be XML." << endl;
	    break;

	case 512+'I':
	    cout << "The " << current_option(i) << "is used to include only those objects that match \\fIregexp\\fR" << endl
		 << "and those objects who call the matching objects in the output." << endl;
	    break;

	case 'J':
	    cout << "The " << current_option( i ) << "is used to set the justification for \\fIobject\\fP.  \\fIObject\\fR is one of:"
		 << current_arg(i,0) << ", "
		 << current_arg(i,1) << ", or "
		 << current_arg(i,2) << "." << endl
		 << "\\fIJustification\\fR is one of \\fBleft\\fR, \\fBcenter\\fR, \\fBright\\fR or \\fBalign\\fR." << endl
		 << "The default is center justifcation for all objects." << endl;
	    break;

	case 'L':
	    cout << "The " << current_option(i) << "is used to choose the layering strategy for output." << endl
		 << ".RS" << endl;
	    for ( unsigned j = 0; Options::layering[j]; ++j ) {

		cout << ".TP" << endl
		     << "\\fB" << current_arg(i,j);
		if ( j == LAYERING_GROUP ) {
		    cout << "=\\fIregexp\\fR";
		}
		cout << "\\fR" << endl;

		switch ( j ) {
		case LAYERING_BATCH:
		    cout << "Batch layering (default for lqns(1))" << endl;
		    break;
		case LAYERING_HWSW:
		    cout << "Hardware-Software layering (Clients and software servers in one layer," << endl
			 << "software servers and processors in the other)." << endl;
		    break;
		case LAYERING_PROCESSOR:
		    cout << "Batch layering, but tasks are grouped by processor." << endl
			 << "Processors are ordered by the level of their calling tasks," << endl
			 << "i.e., processors for reference tasks appear first." << endl;
		    break;
		case LAYERING_TASK_PROCESSOR:
		    cout << "Hardware-Software layering, but tasks are grouped by processor." << endl;
		    break;
		case LAYERING_PROCESSOR_TASK:
		    cout << "Hardware-Software layering, but tasks are grouped by processor." << endl;
		    break;
#if 0
		case LAYERING_FOLLOW_CLIENTS:
		    cout << "Select a subset of the model starting from the tasks which match the \\fIregexp\\fP." << endl
			 << "The tasks in  \\fIregexp\\fP must be reference tasks or tasks accepting open arrivals." << endl
			 << "This option does not affect the layering strategy selected." << endl
			 << "Note that the layering of tasks may be different than the output used from " 
			 << current_option(CHAIN) << "." << endl;
		    break;
#endif
		case LAYERING_GROUP:
		    cout << "Batch layering, but tasks are grouped by the processors identified by \\fIregexp\\fP." << endl
			 << "Multiple occurances of this option can be used to specify multiple groups." << endl
			 << "Processors not matching any group expression are assigned to the last \"default\" group." << endl
			 << "Groups may also be identified in the input file using the \\fIgroup\\fP pragma." << endl;
		    break;
		case LAYERING_SHARE:
		    cout << "Batch layering, but tasks are grouped by their processor share." << endl
			 << "Shares are ordered by the level of their calling tasks," << endl
			 << "i.e., processors for reference tasks appear first." << endl;
		    break;
		case LAYERING_SRVN:
		    cout << "Each server is assigned to its own submodel." << endl;
		    break;
		case LAYERING_SQUASHED:
		    cout << "Place all tasks in level 1, and all processors in level 2.  There is only" << endl
			 << "one submodel overall." << endl;
		    break;
		case LAYERING_MOL:
		    cout << "Mol layering (all of the processors are on their own layer)." << endl;
		    break;
		}
	    }
	    cout << ".LP" << endl
		 << default_setting(i) << endl;
	    cout << ".RE" << endl;
	    break;

	case 'o':
	    cout << "The " << current_option(i) << "is used to direct all output to the" << endl
		 << "file \\fIoutput\\fR regardless of the source of input.  Multiple input" << endl
		 << "files cannot be specified when using this option except with" << endl
		 << "PostScript or EEPIC output.  Output can be directed to standard output by using" << endl
		 << "\\fB\\-o\\fI\\-\\fR (i.e., the output " << endl
		 << "file name is `\\fI\\-\\fR'.)" << endl;
	    break;

	case 'O':
	    cout << "Set the output format." << endl
		 << ".RS" << endl;
	    cout << ".TP" << endl
		 << "\\fB" << current_arg(i,FORMAT_EEPIC) << "\\fR" << endl
		 << "Generate eepic macros for LaTeX." << endl;
#if defined(EMF_OUTPUT)
	    cout << ".TP" << endl
		 << "\\fB" << current_arg(i,FORMAT_EMF) << "\\fR" << endl
		 << "Generate Windows Enhanced Meta File (vector) output." << endl;
#endif
	    cout << ".TP" << endl
		 << "\\fB" << current_arg(i,FORMAT_FIG) << "\\fR" << endl
		 << "Generate input for xfig(1)." << endl;
#if HAVE_GD_H && HAVE_LIBGD && HAVE_GDIMAGEGIFPTR
	    cout << ".TP" << endl
		 << "\\fB" << current_arg(i,FORMAT_GIF) << "\\fR" << endl
		 << "Generate GIF (bitmap) output." << endl;
#endif
#if HAVE_GD_H && HAVE_LIBGD && HAVE_LIBJPEG 
	    cout << ".TP" << endl
		 << "\\fB" << current_arg(i,FORMAT_JPEG) << "\\fR" << endl
		 << "Generate JPEG (bitmap) output." << endl;
#endif
	    cout << ".TP" << endl
		 << "\\fB" << current_arg(i,FORMAT_NULL) << "\\fR" << endl
		 << "Generate no output except summary statistics about the model or models." << endl;
	    cout << ".TP" << endl
		 << "\\fB" << current_arg(i,FORMAT_OUTPUT) << "\\fR" << endl
		 << "Generate a new output file using the results from a parseable output file or from the results found in an XML file." << endl;
	    cout << ".TP" << endl
		 << "\\fB" << current_arg(i,FORMAT_PARSEABLE) << "\\fR" << endl
		 << "Generate a new parseable output file using the results from a parseable output file or from the results found in an XML file." << endl;
	    cout << ".TP" << endl
		 << "\\fB" << current_arg(i,FORMAT_RTF) << "\\fR" << endl
		 << "Generate a new output file in Rich Text Format using the results from a parseable output file or from the results found in an XML file." << endl;
	    cout << ".TP" << endl
		 << "\\fB" << current_arg(i,FORMAT_SRVN) << "\\fR" << endl
		 << "Generate a new input file.  Results are ignored unless a subset of the input file is being generated." << endl
		 << "The " << current_flag( INCLUDE_ONLY ) << " and " << current_flag( SUBMODEL ) << " options can be used to generate new input models" << endl
		 << "consisting only of the objects selected." << endl
		 << "If a parseable output file is available, the transformed subset will derive service times based on results. " << endl
		 << "Refer to \\fI``SRVN Input File Format''\\fR for a complete" << endl
		 << "description of the input file format for the programs." << endl
		 << "New input files are always \"cleaned up\"." << endl;
	    cout << ".TP" << endl
		 << "\\fB" << current_arg(i,FORMAT_POSTSCRIPT) << "\\fR" << endl
		 << "Generate Encapsulated Postscript." << endl;
	    cout << ".TP" << endl
		 << "\\fB" << current_arg(i,FORMAT_PSTEX) << "\\fR" << endl
		 << "Generate PostScript and LaTeX (pstex)." << endl;
#if HAVE_GD_H && HAVE_LIBGD && HAVE_LIBPNG
	    cout << ".TP" << endl
		 << "\\fB" << current_arg(i,FORMAT_PNG) << "\\fR" << endl
		 << "Generate Portable Network Graphics (bitmap) output." << endl;
#endif
#if defined(SVG_OUTPUT)
	    cout << ".TP" << endl
		 << "\\fB" << current_arg(i,FORMAT_SVG) << "\\fR" << endl
		 << "Generate Scalable Vector Graphics (vector) output." << endl;
#endif
#if defined(SXD_OUTPUT)
	    cout << ".TP" << endl
		 << "\\fB" << current_arg(i,FORMAT_SXD) << "\\fR" << endl
		 << "Generate OpenOffice Drawing (vector) output.  " << endl
		 << "The output file must be a regular file.  Output to special files is not supported." << endl;
#endif
	    cout << ".TP" << endl
		 << "\\fB" << current_arg(i,FORMAT_XML) << "\\fR" << endl
		 << "Generate an XML input file.  If results are available, they are included." << endl
		 << "The " << current_flag( INCLUDE_ONLY ) << " and " << current_flag( SUBMODEL ) << " options can be used to generate new input models" << endl
		 << "consisting only of the objects selected." << endl
		 << "New input files are always \"cleaned up\"." << endl;
#if defined(X11_OUTPUT)
	    cout << ".TP" << endl
		 << "\\fB" << current_arg(i,FORMAT_X11) << "\\fR" << endl
		 << "Not implemented." << endl;
#endif
	    cout << ".RE" << endl;
	    break;
	    
	case 'P':
	    cout << "Specify which processors are displayed." << endl
		 << ".RS" << endl;
	    cout << ".TP" << endl
		 << "\\fB" << current_arg(i,PROCESSOR_NONE) << "\\fR" << endl
		 << "Don't display any processors.." << endl;
	    cout << ".TP" << endl
		 << "\\fB" << current_arg(i,PROCESSOR_DEFAULT) << "\\fR" << endl
		 << "Only display those processors that might have contention." << endl;
	    cout << ".TP" << endl
		 << "\\fB" << current_arg(i,PROCESSOR_ALL) << "\\fR" << endl
		 << "Show all processors." << endl;
	    cout << ".TP" << endl
		 << "\\fB" << current_arg(i,PROCESSOR_NONINFINITE) << "\\fR" << endl
		 << "Show all non-infinite processors." << endl;
	    cout << ".LP" << endl
		 << default_setting(i) << endl
		 << "This option has no effect for LQN input and output file generation." << endl;
	    cout << ".RE" << endl;
	    break;

	case 'Q':
	    cout << "The " << current_option(i) << "is used to generate a diagram of the underlying queueing" << endl
		 << "model for the submodel number given as an argument." << endl
		 << "This option has no effect for LQN input and output file generation." << endl;
	    break;

	case 'R':
	    cout << "The " << current_option(i) << "is to expand or remove replication." << endl
		 << ".RS" << endl;
	    cout << ".TP" << endl 
		 << "\\fB" << current_arg(i,REPLICATION_NOP) << "\\fR" << endl
		 << "Don't remove or expand replication." << endl;
	    cout << ".TP" << endl
		 << "\\fB" << current_arg(i,REPLICATION_EXPAND) << "\\fR" << endl
		 << "Exapand replicated models into a flat model.  Tasks and processors are renamed to <name>_1, <name>_2, etc." << endl;
	    cout << ".TP" << endl
		 << "\\fB" << current_arg(i,REPLICATION_REMOVE) << "\\fR" << endl
		 << "Remove all replication from the model." << endl
		 << ".LP" << endl
		 << default_setting(i) << endl;
	    cout << ".RE" << endl;
	    break;

	case 'S':
	    cout << "The " << current_option(i) << "is used to generate a diagram of the submodel number given as an argument." << endl
		 << "If this option is used with \\fBlqn2lqn\\fP, parameters will be derived to approximate the submodel at the time of the final solution." << endl;
	    break;

	case 'Z':
	    cout << "Special options:" << endl
		 << ".RS" << endl;
	    cout << ".TP" << endl
		 << "\\fB" << current_arg(i,PRAGMA_ANNOTATE) << "\\fR" << endl
		 << "Annotate the lqn input file (lqn output only)." << endl;
	    cout << ".TP" <<endl
		 << "\\fB" << current_arg(i,PRAGMA_ARROW_SCALING) << "\\fR" << endl
		 << "Scale the size of arrow heads by the scaling factor \\fIarg\\fP." << endl;
	    cout << ".TP" << endl
		 << "\\fB" << current_arg(i,PRAGMA_CLEAR_LABEL_BACKGROUND) << "\\fR" << endl
		 << "Clear the area behind the label (fig output only)." << endl;
	    cout << ".TP" <<endl
		 << "\\fB" << current_arg(i,PRAGMA_EXHAUSTIVE_TOPOLOGICAL_SORT) << "\\fR" << endl
		 << "Don't short circuit the topological sorter.  (Some models render better)." << endl;
	    cout << ".TP" <<endl
		 << "\\fB" << current_arg(i,PRAGMA_FLATTEN_SUBMODEL) << "\\fR" << endl
		 << "Submodels drawn with \\-S or \\-Q normally place clients in their level found from the full model.  This option draws all clients for a given submodel in one layer." << endl;
	    cout << ".TP" << endl
		 << "\\fB" << current_arg(i,PRAGMA_FORWARDING_DEPTH) << "\\fR" << endl
		 << "Nest forwarding instead of keeping it at the current level (historical). " << endl;
	    cout << ".TP" <<endl
		 << "\\fB" << current_arg(i,PRAGMA_GROUP) << "\\fR" << endl
		 << "When using \\-Lgroup, name a group.  Multiple groups are named using a comma separated list." << endl;
	    cout << ".TP" << endl
		 << "\\fB" << current_arg(i,PRAGMA_LAYER_NUMBER) << "\\fR" << endl
		 << "Print the layer number (valid for graphical output only)." << endl;
	    cout << ".TP" <<endl
		 << "\\fB" << current_arg(i,PRAGMA_NO_ALIGNMENT_BOX) << "\\fR" << endl
		 << "Don't generate the alignment boxes (Fig output)." << endl;
	    cout << ".TP" <<endl
		 << "\\fB" << current_arg(i,PRAGMA_NO_ASYNC_TOPOLOGICAL_SORT) << "\\fR" << endl
		 << "Don't follow asynchronous calls when doing the topological sort." << endl;
	    cout << ".TP" <<endl
		 << "\\fB" << current_arg(i,PRAGMA_NO_CV_SQR) << "\\fR" << endl
		 << "Remove all coefficient of variation terms from a model.  This option is used when generating new models." << endl;
	    cout << ".TP" <<endl
		 << "\\fB" << current_arg(i,PRAGMA_NO_PHASE_TYPE) << "\\fR" << endl
		 << "Remove all phase type flag terms from a model.  This option is used when generating new models." << endl;
	    cout << ".TP" <<endl
		 << "\\fB" << current_arg(i,PRAGMA_NO_REF_TASK_CONVERSION) << "\\fR" << endl
		 << "When generating new models as submodels of existing models, servers in the original model are converted to reference tasks when possible.  This option overrides this conversion; these models use open-arrivals instead." << endl;
	    cout << ".TP" << endl
		 << "\\fB" << current_arg(i,PRAGMA_RENAME) << "\\fR" << endl
		 << "Rename all of the icons to p\\fIn\\fP, t\\fIn\\fP, e\\fIn\\fP and a\\fIn\\fP where \\fIn\\fP is an integer starting from one." << endl;
	    cout << ".TP" << endl
		 << "\\fB" << current_arg(i,PRAGMA_SORT) << "\\fR" << endl
		 << "Set the order of sorting of objects in a layer (ascending, descending, topological, none)." << endl;
	    cout << ".TP" << endl
		 << "\\fB" << current_arg(i,PRAGMA_SQUISH_ENTRY_NAMES) << "\\fR" << endl
		 << "Rename entries/activities by taking only capital letters, letters following an underscore, or numbers." << endl; 
	    cout << ".TP" << endl
		 << "\\fB" << current_arg(i,PRAGMA_SUBMODEL_CONTENTS) << "\\fR" << endl
		 << "Output to terminal the clients and servers of each submodel." << endl;
	    cout << ".TP" << endl
		 << "\\fB" << current_arg(i,PRAGMA_TASKS_ONLY) << "\\fR" << endl
		 << "Draw the model omitting all entries." << endl;
	    cout << ".RE" << endl;
	    break;
	}
    }
    cout << ".SH \"SEE ALSO\"" << endl;
}

static ostream&
print_args_str( ostream& output, const option_type &o, const int ) 
{
    if ( o.c == 'I' ) {
	cerr << "ARG=(lqn,xml)";
    } else if ( o.opts 
	&& o.opts != Options::integer
	&& o.opts != Options::real
	&& o.opts != Options::string ) {
	cerr << "ARG=(";
	for ( int j = 0; o.opts[j]; ++j ) {
	    if ( j != 0 ) cerr << '|';
	    cerr << o.opts[j];
	} 
	cerr << ")";
    } else if ( (o.c & 0xff00) == 0 && islower( o.c ) && o.arg == 0 ) {
	cerr << "(" << (o.value.b ? "true" : "false") << ")";
    }
    return output;
}


static ostream&
current_option_str( ostream& output, const option_type& o, const int )
{
    output << "\\fB\\-" << static_cast<char>(o.c) << "\\fI " << o.arg << "\\fR option ";
    return output;
}

static ostream&
current_arg_str( ostream& output, const option_type& o, const int j )
{
    output << "\\fB" << o.opts[j] << "\\fR";
    return output;
}


static ostream&
current_flag_str( ostream& output, const option_type& o, const int )
{
    output << "\\fB\\-" << static_cast<char>(o.c) << "\\fR";
    return output;
}


static ostream&
default_setting_str( ostream& output, const option_type& o, const int )
{
    output << "(The default is ";
    if ( o.arg == 0 ) {
	output << (o.value.b ? "on" : "off");
    } else if ( o.opts == Options::integer ) {
	output << o.value.i;
    } else if ( o.opts == Options::real ) {
	output << o.value.f;
    } else if ( o.opts && o.opts != Options::string ) {
	output << o.opts[o.value.i];
    } else {
	output << "N/A";
    }
    output << ").";
    return output;
}



static HelpManip
current_option( unsigned int i )
{
    return HelpManip( &current_option_str, Flags::print[i]  );
}

static HelpManip
current_arg( unsigned int i, const int j )
{
    return HelpManip( &current_arg_str, Flags::print[i], j );
}

static HelpManip
current_flag( unsigned int i )
{
    return HelpManip( &current_flag_str, Flags::print[i] );
}

static HelpManip
default_setting( unsigned int i )
{
    return HelpManip( &default_setting_str, Flags::print[i] );
}

static HelpManip
print_args( unsigned int i )
{
    return HelpManip( &print_args_str, Flags::print[i] );
}
