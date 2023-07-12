/* help.cc	-- Greg Franks Thu Mar 27 2003
 *
 * $Id: help.cc 16748 2023-06-18 13:18:38Z greg $
 */

#include "lqn2ps.h"
#include <cstring>
#include <cctype>
#include <algorithm>
#include <numeric>

class HelpManip {
public:
    HelpManip( std::ostream& (*ff)(std::ostream&, const Options::Type&, const int ), const Options::Type& o, const int j = 0 )
	: _o(o), _j(j), f(ff) {}
private:
    const Options::Type _o;
    const int _j;
    std::ostream& (*f)( std::ostream&, const Options::Type& o, const int j );

    friend std::ostream& operator<<(std::ostream & os, const HelpManip& m )
	{ return m.f(os,m._o,m._j); }
};


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
    std::cerr << "Usage: " << LQIO::io_vars.lq_toolname << " [OPTION]... [FILE]..." << std::endl;
    std::cerr << "Options:" << std::endl;
    if ( !full_usage ) {
	std::cerr << "[(+|-)";
	for ( std::vector<Options::Type>::const_iterator f = Flags::print.begin(); f != Flags::print.end(); ++f ) {
	    if ( (f->c & 0xff00) == 0 && !f->arg ) {
		std::cerr << static_cast<char>(f->c);
	    }
	}
	std::cerr << ']';

	unsigned j = 0;
	for ( std::vector<Options::Type>::const_iterator f = Flags::print.begin(); f != Flags::print.end(); ++f ) {
	    if ( (f->c & 0xff00) == 0 && f->arg ) {
		if ( (j % 4) == 0 ) std::cerr << std::endl << "	  ";
		std::cerr << " [-" << static_cast<char>(f->c) << " <" << f->arg << ">]";
		++j;
	    }
	}
	std::cerr << std::endl << std::endl;
	return;
    }

#if HAVE_GETOPT_LONG
    for ( std::vector<Options::Type>::const_iterator f = Flags::print.begin(); f != Flags::print.end(); ++f ) {
	std::string s;
	if ( (f->c & 0xff00) != 0 ) {
            s = "         --";
	    if ( (f->c & 0xff00) == 0x0300 ) {
		s += "[no-]";
	    }
	} else if ( f->arg == 0 && std::islower( f->c ) ) {
	    s = " -";
	    s += f->c;
	    s += ", +";
	    s += f->c;
	    s += ", --[no-]";
	} else {
	    s = " -";
	    s += f->c;
            s += ",     --";
	}
	s += f->name;
	if ( f->arg ) {
	    s += "=";
	    if ( strcmp( f->name, f->arg ) == 0 ) {
		if ( f->opts.param.s == &Options::real ) {
		    s += "N.N";
		} else if ( f->opts.param.s == &Options::integer ) {
		    s += "N";
		} else {
		    s += "ARG";
		}
	    } else {
		s += f->arg;
	    }
	}
	std::cerr.setf( std::ios::left, std::ios::adjustfield );
	std::cerr << std::setw(40) << s << f->msg << " " << print_args( f - Flags::print.begin() ) << std::endl;
    }
#else
    for ( unsigned i = 0; i < N_FLAG_VALUES; ++i ) {
	if ( !f->arg ) {
	    std::cerr << "(+|-)" << static_cast<char>(f->c) << "  " << f->msg
		      << " (" << (f->opts.value.b ? "true" : "false") << ")" << std::endl;
	}
    }
    for ( unsigned int i = 0; i < N_FLAG_VALUES; ++i ) {
	if ( f->arg ) {
	    std::cerr << "-" << static_cast<char>(f->c) << "  " << f->msg << std::endl;
	}
    }
#endif
}



void
invalid_option( char c, char * optarg )
{
#if HAVE_GETOPT_LONG
    for ( std::vector<Options::Type>::const_iterator f = Flags::print.begin(); f != Flags::print.end(); ++f ) {
	if ( f->c == c ) {
	    std::cerr << LQIO::io_vars.lq_toolname << ": Invalid argument to --" << f->name << ", ARG=" << optarg << std::endl;
	    std::cerr << "    " << print_args( f - Flags::print.begin() ) << std::endl;
	    break;
	}
    }
#else
    std::cerr << LQIO::io_vars.lq_toolname << ": Invalid argument to -" << static_cast<char>(c & 0x7f) << optarg << std::endl;
#endif
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

#if defined(HAVE_CTIME)
    strftime( date, 32, "%d %B %Y", localtime( &tloc ) );
#endif

    std::cout << comm << " -*- nroff -*-" << std::endl
	      << ".TH lqn2ps 1 \"" << date << "\"  \"" << VERSION << "\"" << std::endl;


    std::cout << comm << " $Id: help.cc 16748 2023-06-18 13:18:38Z greg $" << std::endl
	      << comm << std::endl
	      << comm << " --------------------------------" << std::endl;

    
    std::cout << ".SH \"NAME\"" << std::endl;
    bool printed = false;
    for ( std::map<const File_Format,const std::string>::const_iterator i = Options::file_format.begin(); i != Options::file_format.end(); ++i ) {
	switch ( i->first ) {
#if EMF_OUTPUT
	case File_Format::EMF:
#endif
	case File_Format::FIG:
#if HAVE_GD_H && HAVE_LIBGD && HAVE_GDIMAGEGIFPTR
	case File_Format::GIF:
#endif
#if HAVE_GD_H && HAVE_LIBGD && HAVE_LIBJPEG
	case File_Format::JPEG:
#endif
	case File_Format::OUTPUT:
#if HAVE_GD_H && HAVE_LIBGD && HAVE_LIBPNG
	case File_Format::PNG:
#endif
	case File_Format::POSTSCRIPT:
	case File_Format::SRVN:
	case File_Format::SXD:
	case File_Format::JSON:
	case File_Format::XML:
	    if ( printed ) {
		std::cout << ", ";
	    } else {
		printed = true;
	    }
	    std::cout << "lqn2" << i->second;
	}
    }
    std::cout << " \\- format and translate layered queueing network models."
	      << std::endl;

    std::cout << ".SH \"SYNOPSIS\"" << std::endl
	      << ".br" << std::endl
	      << ".B lqn2ps" << std::endl
	      << "[\\fIOPTION \\&.\\|.\\|.\\fP]" << std::endl;
    std::cout << "[" << std::endl
	      << "FILE \\&.\\|.\\|." << std::endl
	      << "]" << std::endl;

    std::cout << ".SH \"DESCRIPTION\"" << std::endl
	      << "\\fBlqn2ps\\fR, \\fBlqn2lqn\\fR, \\fBlqn2xml\\fR, and \\fBlqn2out\\fR" << std::endl
	      << "are used to transform an LQN input file (with results) into:" << std::endl
	      << "graphical output such as PostScript," << std::endl
	      << "a new old-style input file," << std::endl
	      << "a new XML input/output file," << std::endl
	      << "and textual output respectively." << std::endl
	      << "In general, the name of the program is used to specify the output format, " << std::endl
	      << "although the " << current_flag( OUTPUT_FORMAT ) << " switch always overrides this behaviour." << std::endl
	      << "Refer to the " << current_flag( OUTPUT_FORMAT ) << " switch described below for the output formats supported." << std::endl;
    std::cout << ".PP" << std::endl
	      << "\\fBlqn2ps\\fR reads its input from \\fIfilename\\fR, specified at the" << std::endl
	      << "command line if present, or from the standard input otherwise.  Output" << std::endl
	      << "for an input file \\fIfilename\\fR specified on the command line will be" << std::endl
	      << "placed in the file \\fIfilename.ext\\fR, where \\fI.ext\\fR depends on the" << std::endl
	      << "conversion taking place (see the " << current_flag( OUTPUT_FORMAT ) << " switch)." << std::endl
	      << "If \\fBlqn2lqn\\fR or \\fBlqn2xml\\fR is used to reformat an existing LQN-type or XML input file respectively," << std::endl
	      << "the output is written back to the original file name." << std::endl
	      << "The original file is renamed to \\fIfilename\\fB~\\fR." << std::endl
	      << "If several input files are given, then each is treated as a separate model and" << std::endl
	      << "output will be placed in separate output files.  This behaviour can be changed" << std::endl
	      << "using the " << current_flag( OUTPUT_FILE ) << " option, described below.  If input is from the" << std::endl
	      << "standard input, output will be directed to the standard output.  The" << std::endl
	      << "file name `\\-' is used to specify standard input." << std::endl;

    std::cout << ".SH \"OPTIONS\"" << std::endl;
    for ( std::vector<Options::Type>::const_iterator f = Flags::print.begin(); f != Flags::print.end(); ++f ) {
	unsigned int i = f - Flags::print.begin();
	std::cout << ".TP" << std::endl;
	std::cout << "\\fB";
	if ( (f->c & ~0x7f) == 0 ) {	/* Only lower ascii */
	    if ( std::islower(f->c) && f->arg == 0 ) {
		std::cout << "(\\-|+)";
	    } else {
		std::cout << "\\-";
	    }
	    std::cout << static_cast<char>(f->c) << "\\fR";
	    if ( f->name ) {
		std::cout << ", \\fB";
	    }
	}
	if ( f->name ) {
	    std::cout << "\\-\\-";
	    if ( ((f->c & ~0x7f) == 0) && std::islower(f->c) && f->arg == 0 ) {
		std::cout << "[no-]";
	    }
	    std::cout << f->name;
	}
	if ( f->arg ) {
	    std::cout << "=\\fI" << f->arg;
	}
	std::cout << "\\fR" << std::endl;

	/* Description goes here... */
	switch ( f->c ) {
	default:
	    std::cout << f->msg;
	    if ( ((f->c & ~0x7f) == 0) && std::islower(f->c) ) {
		std::cout << " " << default_setting(i);
	    }
	    std::cout  << std::endl;
	    break;

	case 'A':
	    std::cout << "The " << current_option(i) << "is used to aggregate objects." << std::endl
		      << ".RS" << std::endl;
	    std::cout << ".TP" << std::endl
		      << "\\fB" << Options::aggregate.at(Aggregate::NONE) << "\\fR" << std::endl
		      << "Don't aggregate objects." << std::endl;
	    std::cout << ".TP" << std::endl
		      << "\\fB" << Options::aggregate.at(Aggregate::SEQUENCES) << "\\fR" << std::endl
		      << "Aggregate sequences of activities into a single activity." << std::endl;
	    std::cout << ".TP" << std::endl
		      << "\\fB" << Options::aggregate.at(Aggregate::ACTIVITIES) << "\\fR" << std::endl
		      << "Aggregate activities called by an entry into the entry." << std::endl;
	    std::cout << ".TP" << std::endl
		      << "\\fB" << Options::aggregate.at(Aggregate::PHASES) << "\\fR" << std::endl
		      << "Aggregate activities called by an entry into the entry and remove all phases." << std::endl;
	    std::cout << ".PP" << std::endl
		      << "A new model that results from aggregation may not necessarily have the same solution as the original model." << std::endl
		      << "An aggregated model is smaller, so it will solve faster." << std::endl
		      << ".LP" << std::endl
		      << default_setting(i) << std::endl;
	    std::cout << ".RE" << std::endl;
	    break;

	case 'C':
	    std::cout << "The " << current_option(i) << "is used to choose how to colour objects." << std::endl
		      << ".RS" << std::endl;
	    std::cout << ".TP" << std::endl
		      << "\\fB" << Options::colouring.at(Colouring::NONE) << "\\fR" << std::endl
		      << "Use gray scale instead of colour for results." << std::endl;
	    std::cout << ".TP" << std::endl
		      << "\\fB" << Options::colouring.at(Colouring::RESULTS) << "\\fR" << std::endl
		      << "Colour nodes based on utilization and arcs based on the utilization of the destination.  This is the default." << std::endl;
	    std::cout << ".TP" << std::endl
		      << "\\fB" << Options::colouring.at(Colouring::LAYERS) << "\\fR" << std::endl
		      << "Colour nodes based on their layer." << std::endl;
	    std::cout << ".TP" << std::endl
		      << "\\fB" << Options::colouring.at(Colouring::CLIENTS) << "\\fR" << std::endl
		      << "Colour nodes based on their client(s)." << std::endl;
	    std::cout << ".TP" << std::endl
		      << "\\fB" << Options::colouring.at(Colouring::SERVER_TYPE) << "\\fR" << std::endl
		      << "Client tasks are coloured red, server tasks are coloured blue." << std::endl;
	    std::cout << ".TP" << std::endl
		      << "\\fB" << Options::colouring.at(Colouring::CHAINS) << "\\fR" << std::endl
		      << "Queueing output only: colour each chain differently." << std::endl;
	    std::cout << ".TP" << std::endl
		      << "\\fB" << Options::colouring.at(Colouring::DIFFERENCES) << "\\fR" << std::endl
		      << "Results are displayed as percent differences (found from srvndiff --difference file1 file2) with the colour reflecting" << std::endl
		      << "the percentage difference." << std::endl;
	    std::cout << ".RE" << std::endl;
	    break;

	case 'D':
	    std::cout << "The " << current_option(i) << " is used to load \\fIdifference\\fR data produced by srvndiff.";
	    std::cout << std::endl;
	    break;
	
	case 'I':
	    std::cout << "The " << current_option(i) << " is used to force the input file format to either \\fIxml\\fR, \\fIlqn\\fR, of \\fIjson\\fR." << std::endl
		      << "By default, if the suffix of the input filename is one of: \\fI.in\\fR, \\fI.lqn\\fR, \\fI.xlqn\\fR, of \\fI.txt\\fR," << std::endl
		      << "then the LQN parser will be used.  If the suffix of the input filename is one of: \\fI.json\\fR, \\fI.lqnj\\fR, \\fI.jlqn\\fR, of \\fI.lqjo\\fR," << std::endl
		      << "then the JSON parser will be used.  Otherwise, input is assumed to be XML." << std::endl;
	    break;

	case 512+'I':
	    std::cout << "The " << current_option(i) << "is used to include only those objects that match \\fIregexp\\fR" << std::endl
		      << "and those objects who call the matching objects in the output." << std::endl;
	    break;

	case 'J':
	    std::cout << "The " << current_option( i ) << "is used to set the justification for \\fIobject\\fP.	 \\fIObject\\fR is one of:" << std::endl
		      << "\\fBnodes\\fR, \\fBlabels\\fR, or \\fBactivities\\fR." << std::endl
		      << "\\fIJustification\\fR is one of "
		      << "\\fB" << Options::justification.at(Justification::LEFT) << "\\fR," 
		      << "\\fB" << Options::justification.at(Justification::CENTER) << "\\fR,"
		      << "\\fB" << Options::justification.at(Justification::RIGHT ) << "\\fR,"
		      << "\\fB" << Options::justification.at(Justification::ALIGN ) << " (for nodes)\\fR, or"
		      << "\\fB" << Options::justification.at(Justification::ABOVE ) << " (for activities)\\fR."
		      << std::endl
		      << "The default is center justifcation for all objects." << std::endl;
	    break;

	case 'L':
	    std::cout << "The " << current_option(i) << "is used to choose the layering strategy for output." << std::endl
		      << ".RS" << std::endl;
	    for ( std::map<const Layering,const std::string>::const_iterator j = Options::layering.begin(); j != Options::layering.end(); ++j ) {

		std::cout << ".TP" << std::endl
			  << "\\fB" << j->second;
		if ( j->first == Layering::GROUP ) {
		    std::cout << "=\\fIregexp\\fR[,\\fIregexp\\fR]";
		}
		std::cout << "\\fR" << std::endl;

		switch ( j->first ) {
		case Layering::BATCH:
		    std::cout << "Batch layering (default for lqns(1))" << std::endl;
		    break;
		case Layering::HWSW:
		    std::cout << "Hardware-Software layering (Clients and software servers in one layer," << std::endl
			      << "software servers and processors in the other)." << std::endl;
		    break;
		case Layering::PROCESSOR:
		    std::cout << "Batch layering, but tasks are grouped by processor." << std::endl
			      << "Processors are ordered by the level of their calling tasks," << std::endl
			      << "i.e., processors for reference tasks appear first." << std::endl;
		    break;
		case Layering::TASK_PROCESSOR:
		    std::cout << "Hardware-Software layering, but tasks are grouped by processor." << std::endl;
		    break;
		case Layering::PROCESSOR_TASK:
		    std::cout << "Hardware-Software layering, but tasks are grouped by processor." << std::endl;
		    break;
#if 0
		case Layering::FOLLOW_CLIENTS:
		    std::cout << "Select a subset of the model starting from the tasks which match the \\fIregexp\\fP." << std::endl
			      << "The tasks in  \\fIregexp\\fP must be reference tasks or tasks accepting open arrivals." << std::endl
			      << "This option does not affect the layering strategy selected." << std::endl
			      << "Note that the layering of tasks may be different than the output used from "
			      << current_option(CHAIN) << "." << std::endl;
		    break;
#endif
		case Layering::GROUP:
		    std::cout << "Batch layering, but tasks are grouped by the processors identified by a list of \\fIregexp\\fP." << std::endl
			      << "Processors not matching any group expression are assigned to the last \"default\" group." << std::endl;
		    break;
		case Layering::SHARE:
		    std::cout << "Batch layering, but tasks are grouped by their processor share." << std::endl
			      << "Shares are ordered by the level of their calling tasks," << std::endl
			      << "i.e., processors for reference tasks appear first." << std::endl;
		    break;
		case Layering::SRVN:
		    std::cout << "Each server is assigned to its own submodel." << std::endl;
		    break;
		case Layering::SQUASHED:
		    std::cout << "Place all tasks in level 1, and all processors in level 2.  There is only" << std::endl
			      << "one submodel overall." << std::endl;
		    break;
		case Layering::MOL:
		    std::cout << "Mol layering (all of the processors are on their own layer)." << std::endl;
		    break;
		}
	    }
	    std::cout << ".LP" << std::endl
		      << default_setting(i) << std::endl;
	    std::cout << ".RE" << std::endl;
	    break;

	case 'o':
	    std::cout << "The " << current_option(i) << "is used to direct all output to the" << std::endl
		      << "file \\fIoutput\\fR regardless of the source of input.  Multiple input" << std::endl
		      << "files cannot be specified when using this option except with" << std::endl
		      << "PostScript or EEPIC output.  Output can be directed to standard output by using" << std::endl
		      << "\\fB\\-o\\fI\\-\\fR (i.e., the output " << std::endl
		      << "file name is `\\fI\\-\\fR'.)" << std::endl;
	    break;

	case 'O':
	    std::cout << "Set the output format." << std::endl
		      << ".RS" << std::endl;
	    std::cout << ".TP" << std::endl
		      << "\\fB" << Options::file_format.at(File_Format::EEPIC) << "\\fR" << std::endl
		      << "Generate eepic macros for LaTeX." << std::endl;
#if EMF_OUTPUT
	    std::cout << ".TP" << std::endl
		      << "\\fB" << Options::file_format.at(File_Format::EMF) << "\\fR" << std::endl
		      << "Generate Windows Enhanced Meta File (vector) output." << std::endl;
#endif
	    std::cout << ".TP" << std::endl
		      << "\\fB" << Options::file_format.at(File_Format::FIG) << "\\fR" << std::endl
		      << "Generate input for xfig(1)." << std::endl;
#if HAVE_GD_H && HAVE_LIBGD && HAVE_GDIMAGEGIFPTR
	    std::cout << ".TP" << std::endl
		      << "\\fB" << Options::file_format.at(File_Format::GIF) << "\\fR" << std::endl
		      << "Generate GIF (bitmap) output." << std::endl;
#endif
#if HAVE_GD_H && HAVE_LIBGD && HAVE_LIBJPEG
	    std::cout << ".TP" << std::endl
		      << "\\fB" << Options::file_format.at(File_Format::JPEG) << "\\fR" << std::endl
		      << "Generate JPEG (bitmap) output." << std::endl;
#endif
	    std::cout << ".TP" << std::endl
		      << "\\fB" << Options::file_format.at(File_Format::JSON) << "\\fR" << std::endl
		      << "Generate an JSON input file.  If results are available, they are included." << std::endl
		      << "The " << current_flag( INCLUDE_ONLY ) << " and " << current_flag( SUBMODEL ) << " options can be used to generate new input models" << std::endl
		      << "consisting only of the objects selected." << std::endl
		      << "New input files are always \"cleaned up\"." << std::endl;
	    std::cout << ".TP" << std::endl
		      << "\\fB" << Options::file_format.at(File_Format::LQX) << "\\fR" << std::endl
		      << "Generate an XML input file.  If results are available, they are included." << std::endl
		      << "The " << current_flag( INCLUDE_ONLY ) << " and " << current_flag( SUBMODEL ) << " options can be used to generate new input models" << std::endl
		      << "consisting only of the objects selected.  If SPEX is present, it will be converted to LQX." << std::endl
		      << "New input files are always \"cleaned up\"." << std::endl;
	    std::cout << ".TP" << std::endl
		      << "\\fB" << Options::file_format.at(File_Format::NO_OUTPUT) << "\\fR" << std::endl
		      << "Generate no output except summary statistics about the model or models." << std::endl;
	    std::cout << ".TP" << std::endl
		      << "\\fB" << Options::file_format.at(File_Format::OUTPUT) << "\\fR" << std::endl
		      << "Generate a new output file using the results from a parseable output file or from the results found in an XML file." << std::endl;
	    std::cout << ".TP" << std::endl
		      << "\\fB" << Options::file_format.at(File_Format::PARSEABLE) << "\\fR" << std::endl
		      << "Generate a new parseable output file using the results from a parseable output file or from the results found in an XML file." << std::endl;
	    std::cout << ".TP" << std::endl
		      << "\\fB" << Options::file_format.at(File_Format::RTF) << "\\fR" << std::endl
		      << "Generate a new output file in Rich Text Format using the results from a parseable output file or from the results found in an XML file." << std::endl;
	    std::cout << ".TP" << std::endl
		      << "\\fB" << Options::file_format.at(File_Format::SRVN) << "\\fR" << std::endl
		      << "Generate a new input file.	 Results are ignored unless a subset of the input file is being generated." << std::endl
		      << "The " << current_flag( INCLUDE_ONLY ) << " and " << current_flag( SUBMODEL ) << " options can be used to generate new input models" << std::endl
		      << "consisting only of the objects selected." << std::endl
		      << "If a parseable output file is available, the transformed subset will derive service times based on results. " << std::endl
		      << "Refer to \\fI``SRVN Input File Format''\\fR for a complete" << std::endl
		      << "description of the input file format for the programs." << std::endl
		      << "New input files are always \"cleaned up\"." << std::endl;
	    std::cout << ".TP" << std::endl
		      << "\\fB" << Options::file_format.at(File_Format::POSTSCRIPT) << "\\fR" << std::endl
		      << "Generate Encapsulated Postscript." << std::endl;
	    std::cout << ".TP" << std::endl
		      << "\\fB" << Options::file_format.at(File_Format::PSTEX) << "\\fR" << std::endl
		      << "Generate PostScript and LaTeX (pstex)." << std::endl;
#if HAVE_GD_H && HAVE_LIBGD && HAVE_LIBPNG
	    std::cout << ".TP" << std::endl
		      << "\\fB" << Options::file_format.at(File_Format::PNG) << "\\fR" << std::endl
		      << "Generate Portable Network Graphics (bitmap) output." << std::endl;
#endif
#if SVG_OUTPUT
	    std::cout << ".TP" << std::endl
		      << "\\fB" << Options::file_format.at(File_Format::SVG) << "\\fR" << std::endl
		      << "Generate Scalable Vector Graphics (vector) output." << std::endl;
#endif
#if SXD_OUTPUT
	    std::cout << ".TP" << std::endl
		      << "\\fB" << Options::file_format.at(File_Format::SXD) << "\\fR" << std::endl
		      << "Generate OpenOffice Drawing (vector) output.  " << std::endl
		      << "The output file must be a regular file.  Output to special files is not supported." << std::endl;
#endif
	    std::cout << ".TP" << std::endl
		      << "\\fB" << Options::file_format.at(File_Format::XML) << "\\fR" << std::endl
		      << "Generate an XML input file.  If results are available, they are included." << std::endl
		      << "The " << current_flag( INCLUDE_ONLY ) << " and " << current_flag( SUBMODEL ) << " options can be used to generate new input models" << std::endl
		      << "consisting only of the objects selected." << std::endl
		      << "New input files are always \"cleaned up\"." << std::endl;
#if X11_OUTPUT
	    std::cout << ".TP" << std::endl
		      << "\\fB" << Options::file_format.at(File_Format::X11) << "\\fR" << std::endl
		      << "Not implemented." << std::endl;
#endif
	    std::cout << ".RE" << std::endl;
	    break;
	
	case 'P':
	    std::cout << "Specify which processors are displayed." << std::endl
		      << ".RS" << std::endl;
	    std::cout << ".TP" << std::endl
		      << "\\fB" << Options::processors.at(Processors::NONE) << "\\fR" << std::endl
		      << "Don't display any processors.." << std::endl;
	    std::cout << ".TP" << std::endl
		      << "\\fB" << Options::processors.at(Processors::ALL) << "\\fR" << std::endl
		      << "Show all processors." << std::endl;
	    std::cout << ".TP" << std::endl
		      << "\\fB" << Options::processors.at(Processors::NONINFINITE) << "\\fR" << std::endl
		      << "Show all non-infinite processors." << std::endl;
	    std::cout << ".TP" << std::endl
		      << "\\fB" << Options::processors.at(Processors::QUEUEABLE) << "\\fR" << std::endl
		      << "Only display those processors that might have contention." << std::endl;
	    std::cout << ".LP" << std::endl
		      << default_setting(i) << std::endl
		      << "This option has no effect for LQN input and output file generation." << std::endl;
	    std::cout << ".RE" << std::endl;
	    break;

	case 'Q':
	    std::cout << "The " << current_option(i) << "is used to generate a diagram of the underlying queueing" << std::endl
		      << "model for the submodel number given as an argument." << std::endl
		      << "This option has no effect for any input file format or LQN output file generation." << std::endl;
	    break;

	case 'R':
	    std::cout << "The " << current_option(i) << "is to expand or remove replication." << std::endl
		      << ".RS" << std::endl;
	    std::cout << ".TP" << std::endl
		      << "\\fB" << Options::replication.at(Replication::NONE) << "\\fR" << std::endl
		      << "Don't remove or expand replication." << std::endl;
	    std::cout << ".TP" << std::endl
		      << "\\fB" << Options::replication.at(Replication::EXPAND) << "\\fR" << std::endl
		      << "Exapand replicated models into a flat model.  Tasks and processors are renamed to <name>_1, <name>_2, etc." << std::endl;
	    std::cout << ".TP" << std::endl
		      << "\\fB" << Options::replication.at(Replication::REMOVE) << "\\fR" << std::endl
		      << "Remove all replication from the model." << std::endl
		      << ".LP" << std::endl
		      << default_setting(i) << std::endl;
	    std::cout << ".RE" << std::endl;
	    break;

	case 'S':
	    std::cout << "The " << current_option(i) << "is used to generate a diagram of the submodel number given as an argument." << std::endl
		      << "If this option is used with \\fBlqn2lqn\\fP, parameters will be derived to approximate the submodel at the time of the final solution." << std::endl;
	    break;

	case 'Z':
	    std::cout << "Special options:" << std::endl
		      << ".RS" << std::endl;
	    std::cout << ".TP" << std::endl
		      << "\\fB" << Options::special.at(Special::ANNOTATE) << "\\fR" << std::endl
		      << "Annotate the lqn input file (lqn output only)." << std::endl;
	    std::cout << ".TP" << std::endl
		      << "\\fB" << Options::special.at(Special::ARROW_SCALING) << "\\fR" << std::endl
		      << "Scale the size of arrow heads by the scaling factor \\fIarg\\fP." << std::endl;
	    std::cout << ".TP" << std::endl
		      << "\\fB" << Options::special.at(Special::CLEAR_LABEL_BACKGROUND) << "\\fR" << std::endl
		      << "Clear the area behind the label (fig output only)." << std::endl;
	    std::cout << ".TP" << std::endl
		      << "\\fB" << Options::special.at(Special::BCMP) << "\\fR" << std::endl
		      << "BCMP." << std::endl;
	    std::cout << ".TP" << std::endl
		      << "\\fB" << Options::special.at(Special::EXHAUSTIVE_TOPOLOGICAL_SORT) << "\\fR" << std::endl
		      << "Don't short circuit the topological sorter.  (Some models render better)." << std::endl;
	    std::cout << ".TP" << std::endl
		      << "\\fB" << Options::special.at(Special::FLATTEN_SUBMODEL) << "\\fR" << std::endl
		      << "Submodels drawn with \\-S or \\-Q normally place clients in their level found from the full model.  This option draws all clients for a given submodel in one layer." << std::endl;
	    std::cout << ".TP" << std::endl
		      << "\\fB" << Options::special.at(Special::FORWARDING_DEPTH) << "\\fR" << std::endl
		      << "Nest forwarding instead of keeping it at the current level (historical). " << std::endl;
	    std::cout << ".TP" << std::endl
		      << "\\fB" << Options::special.at(Special::GROUP) << "\\fR" << std::endl
		      << "When using \\-Lgroup, name a group.  Multiple groups are named using a comma separated list." << std::endl;
	    std::cout << ".TP" << std::endl
		      << "\\fB" << Options::special.at(Special::LAYER_NUMBER) << "\\fR" << std::endl
		      << "Print the layer number (valid for graphical output only)." << std::endl;
	    std::cout << ".TP" << std::endl
		      << "\\fB" << Options::special.at(Special::NO_ALIGNMENT_BOX) << "\\fR" << std::endl
		      << "Don't generate the alignment boxes (Fig output)." << std::endl;
	    std::cout << ".TP" << std::endl
		      << "\\fB" << Options::special.at(Special::NO_ASYNC_TOPOLOGICAL_SORT) << "\\fR" << std::endl
		      << "Don't follow asynchronous calls when doing the topological sort." << std::endl;
	    std::cout << ".TP" << std::endl
		      << "\\fB" << Options::special.at(Special::NO_CV_SQR) << "\\fR" << std::endl
		      << "Remove all coefficient of variation terms from a model.  This option is used when generating new models." << std::endl;
	    std::cout << ".TP" << std::endl
		      << "\\fB" << Options::special.at(Special::NO_PHASE_TYPE) << "\\fR" << std::endl
		      << "Remove all phase type flag terms from a model.  This option is used when generating new models." << std::endl;
	    std::cout << ".TP" << std::endl
		      << "\\fB" << Options::special.at(Special::NO_REF_TASK_CONVERSION) << "\\fR" << std::endl
		      << "When generating new models as submodels of existing models, servers in the original model are converted to reference tasks when possible.  This option overrides this conversion; these models use open-arrivals instead." << std::endl;
	    std::cout << ".TP" << std::endl
		      << "\\fB" << Options::special.at(Special::PROCESSOR_SCHEDULING) << "\\fR" << std::endl
		      << "Change the scheduling for all fixed-rate processors to ?." << std::endl;
	    std::cout << ".TP" << std::endl
		      << "\\fB" << Options::special.at(Special::PRUNE) << "\\fR" << std::endl
		      << "All tasks which are infinite servers are merged into non-infinite server tasks and clients" << std::endl;
	    std::cout << ".TP" << std::endl
		      << "\\fB" << Options::special.at(Special::RENAME) << "\\fR" << std::endl
		      << "Rename all of the icons to p\\fIn\\fP, t\\fIn\\fP, e\\fIn\\fP and a\\fIn\\fP where \\fIn\\fP is an integer starting from one." << std::endl;
	    std::cout << ".TP" << std::endl
		      << "\\fB" << Options::special.at(Special::SORT) << "\\fR" << std::endl
		      << "Set the order of sorting of objects in a layer (ascending, descending, topological, none)." << std::endl;
	    std::cout << ".TP" << std::endl
		      << "\\fB" << Options::special.at(Special::SQUISH_ENTRY_NAMES) << "\\fR" << std::endl
		      << "Rename entries/activities by taking only capital letters, letters following an underscore, or numbers." << std::endl;
	    std::cout << ".TP" << std::endl
		      << "\\fB" << Options::special.at(Special::SUBMODEL_CONTENTS) << "\\fR" << std::endl
		      << "For graphical output, output the submodels (though this only works for a strictly layered model)." << std::endl;
	    std::cout << ".TP" << std::endl
		      << "\\fB" << Options::special.at(Special::TASKS_ONLY) << "\\fR" << std::endl
		      << "Draw the model omitting all entries." << std::endl;
	    std::cout << ".TP" << std::endl
		      << "\\fB" << Options::special.at(Special::TASK_SCHEDULING) << "\\fR" << std::endl
		      << "Change the scheduling for all fixed-rate tasks to ?." << std::endl;
	    std::cout << ".RE" << std::endl;
	    break;
	}
    }
    std::cout << ".SH \"SEE ALSO\"" << std::endl;
}

static std::ostream&
print_args_str( std::ostream& output, const Options::Type &o, const int )
{
    if ( o.c == 'I' ) {
	std::cerr << "ARG=(lqn,xml)";
    } else if ( o.opts.param.f == &Options::file_format ) {
	std::cerr << "ARG=(";
	for ( std::map<const File_Format,const std::string>::const_iterator j = o.opts.param.f->begin(); j != o.opts.param.f->end(); ++j ) {
	    if ( j != o.opts.param.f->begin() ) std::cerr << '|';
	    std::cerr << j->second;
	}
	std::cerr << ")";
    } else if ( o.opts.param.a == &Options::aggregate ) {
	std::cerr << "ARG=(";
	for ( std::map<const Aggregate,const std::string>::const_iterator j = o.opts.param.a->begin(); j != o.opts.param.a->end(); ++j ) {
	    if ( j != o.opts.param.a->begin() ) std::cerr << '|';
	    std::cerr << j->second;
	}
	std::cerr << ")";
    } else if ( o.opts.param.c == &Options::colouring ) {
	std::cerr << "ARG=(";
	for ( std::map<const Colouring,const std::string>::const_iterator j = o.opts.param.c->begin(); j != o.opts.param.c->end(); ++j ) {
	    if ( j != o.opts.param.c->begin() ) std::cerr << '|';
	    std::cerr << j->second;
	}
	std::cerr << ")";
    } else if ( o.opts.param.j == &Options::justification ) {
	std::cerr << "ARG=(";
	for ( std::map<const Justification,const std::string>::const_iterator j = o.opts.param.j->begin(); j != o.opts.param.j->end(); ++j ) {
	    if ( j != o.opts.param.j->begin() ) std::cerr << '|';
	    std::cerr << j->second;
	}
	std::cerr << ")";
    } else if ( o.opts.param.k == &Options::key_position ) {
	std::cerr << "ARG=(";
	for ( std::map<const Key_Position,const std::string>::const_iterator j = o.opts.param.k->begin(); j != o.opts.param.k->end(); ++j ) {
	    if ( j != o.opts.param.k->begin() ) std::cerr << '|';
	    std::cerr << j->second;
	}
	std::cerr << ")";
    } else if ( o.opts.param.l == &Options::layering ) {
	std::cerr << "ARG=(";
	for ( std::map<const Layering,const std::string>::const_iterator j = o.opts.param.l->begin(); j != o.opts.param.l->end(); ++j ) {
	    if ( j != o.opts.param.l->begin() ) std::cerr << '|';
	    std::cerr << j->second;
	}
	std::cerr << ")";
    } else if ( o.opts.param.p == &Options::processors ) {
	std::cerr << "ARG=(";
	for ( std::map<const Processors,const std::string>::const_iterator j = o.opts.param.p->begin(); j != o.opts.param.p->end(); ++j ) {
	    if ( j != o.opts.param.p->begin() ) std::cerr << '|';
	    std::cerr << j->second;
	}
	std::cerr << ")";
    } else if ( o.opts.param.r == &Options::replication ) {
	std::cerr << "ARG=(";
	for ( std::map<const Replication,const std::string>::const_iterator j = o.opts.param.r->begin(); j != o.opts.param.r->end(); ++j ) {
	    if ( j != o.opts.param.r->begin() ) std::cerr << '|';
	    std::cerr << j->second;
	}
	std::cerr << ")";
    } else if ( o.opts.param.z == &Options::special ) {
	std::cerr << "ARG=(";
	for ( std::map<const Special,const std::string>::const_iterator j = o.opts.param.z->begin(); j != o.opts.param.z->end(); ++j ) {
	    if ( j != o.opts.param.z->begin() ) std::cerr << '|';
	    std::cerr << j->second;
	}
	std::cerr << ")";
    } else if ( ( (o.c & 0xff00) == 0x0300 || (o.c & 0xff00) == 0 && std::islower( o.c ) ) && o.arg == 0 ) {
	std::cerr << "(" << (o.opts.value.b ? "true" : "false") << ")";
    }
    return output;
}


static std::ostream&
current_option_str( std::ostream& output, const Options::Type& o, const int )
{
    output << "\\fB\\-" << static_cast<char>(o.c) << "\\fI " << o.arg << "\\fR option ";
    return output;
}

static std::ostream&
current_flag_str( std::ostream& output, const Options::Type& o, const int )
{
    output << "\\fB\\-" << static_cast<char>(o.c) << "\\fR";
    return output;
}


static std::ostream&
default_setting_str( std::ostream& output, const Options::Type& o, const int )
{
    output << "(The default is ";
    if ( o.arg == 0 ) {
	output << (o.opts.value.b ? "on" : "off");
    } else if ( o.opts.param.s == &Options::integer ) {
	output << o.opts.value.i;
    } else if ( o.opts.param.s == &Options::real ) {
	output << o.opts.value.d;
    } else if ( o.opts.param.a == &Options::aggregate ) {
	output << Options::aggregate.at(o.opts.value.a);
    } else if ( o.opts.param.l == &Options::layering ) {
	output << Options::layering.at(o.opts.value.l);
    } else if ( o.opts.param.p == &Options::processors ) {
	output << Options::processors.at(Processors::QUEUEABLE);	// Alias for default.  Bug 438.
    } else if ( o.opts.param.r == &Options::replication ) {
	output << Options::replication.at(o.opts.value.r);
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
