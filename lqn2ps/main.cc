/* srvn2eepic.c	-- Greg Franks Sun Jan 26 2003
 *
 * $Id: main.cc 14516 2021-03-05 15:48:25Z greg $
 */

#include "lqn2ps.h"
#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <cstring>
#include <sstream>
#include <libgen.h>
#include <lqio/dom_object.h>
#include <lqio/dom_pragma.h>
#include "layer.h"
#include "model.h"
#include "errmsg.h"

static double DEFAULT_ICON_HEIGHT = 45;	/* multiple of 9 works well with xfig. */

std::string command_line;

bool Flags::annotate_input		= false;
bool Flags::async_topological_sort      = true;
bool Flags::bcmp_model			= false;
bool Flags::clear_label_background 	= false;
bool Flags::convert_to_reference_task	= true;
bool Flags::debug			= false;
bool Flags::dump_graphviz		= false;
bool Flags::exhaustive_toplogical_sort	= false;
bool Flags::flatten_submodel		= false;
bool Flags::have_results		= false;
bool Flags::instantiate			= false;
bool Flags::output_coefficient_of_variation = true;
bool Flags::output_phase_type		= true;
bool Flags::print_alignment_box		= true;
bool Flags::print_comment		= false;
bool Flags::print_forwarding_by_depth	= false;
bool Flags::print_layer_number  	= false;
bool Flags::print_submodels     	= false;
bool Flags::prune			= false;
bool Flags::rename_model		= false;
bool Flags::squish_names		= false;
bool Flags::surrogates			= false;
bool Flags::use_colour			= true;


double Flags::act_x_spacing		= 6.0;
double Flags::arrow_scaling		= 1.0;
double Flags::entry_height             	= 0.6 * DEFAULT_ICON_HEIGHT;	/* 27 */
double Flags::entry_width              	= DEFAULT_ICON_HEIGHT;		/* 45 */
double Flags::icon_height               = DEFAULT_ICON_HEIGHT;
double Flags::icon_width               	= DEFAULT_ICON_HEIGHT * 1.6;	/* 72 */

std::regex * Flags::client_tasks	= nullptr;

sort_type Flags::sort	 		= FORWARD_SORT;

justification_type Flags::node_justification = DEFAULT_JUSTIFY;
justification_type Flags::label_justification = CENTER_JUSTIFY;
justification_type Flags::activity_justification = DEFAULT_JUSTIFY;
graphical_output_style_type Flags::graphical_output_style = TIMEBENCH_STYLE;

double Flags::icon_slope	        = 1.0/10.0;
unsigned int maxStrLen 		= 16;
const unsigned int maxDblLen	= 12;		/* Field width in srvnoutput. */

const char * Options::activity [] =
{
    "none",
    "sequences",
    "activities",
    "phases",
    "entries",
    "threads",
    0
};

const char * Options::colouring[] =
{
    "off",
    "results",
    "layers",
    "clients",
    "type",
    "chains",
    "differences",
    0
};

const char * Options::integer [] =
{
    "int",
    0
};

/*
 * Input output format options
 */

const char * Options::io[] =
{
    "eepic",
#if EMF_OUTPUT
    "emf",
#endif
    "fig",
#if HAVE_GD_H && HAVE_LIBGD && HAVE_GDIMAGEGIFPTR
    "gif",
#endif
#if JMVA_OUTPUT
    "jmva",
#endif
#if HAVE_GD_H && HAVE_LIBGD && HAVE_LIBJPEG
    "jpeg",
#endif
    "lqx",
    "null",
    "out",
#if QNAP2_OUTPUT
    "qnap2",
#endif
    "parseable",
#if HAVE_GD_H && HAVE_LIBGD && HAVE_LIBPNG
    "png",
#endif
    "ps",
    "pstex",
    "rtf",
    "lqn",
#if defined(SVG_OUTPUT)
    "svg",
#endif
#if defined(SXD_OUTPUT)
    "sxd",
#endif
#if defined(TXT_OUTPUT)
    "txt",
#endif
#if defined(X11_OUTPUT)
    "x11",
#endif
    "xml",
    nullptr
};

const char * Options::justification[] =
{
    "nodes",
    "labels",
    "activities",
    0
};

const char * Options::key[] =
{
    "none",
    "top-left",
    "top-right",
    "bottom-left",
    "bottom-right",
    "below-left",
    "above-left",
    "on",
    0
};

const char * Options::layering[] =
{
    LQIO::DOM::Pragma::_batched_,       /* LAYERING_BATCH           */
    "group",                            /* LAYERING_GROUP           */
    LQIO::DOM::Pragma::_hwsw_,          /* LAYERING_HWSW            */
    LQIO::DOM::Pragma::_mol_,           /* LAYERING_MOL             */
    "processor",                        /* LAYERING_PROCESSOR       */
    "processor-task",                   /* LAYERING_PROCESSOR_TASK  */
    "share",                            /* LAYERING_SHARE           */
    LQIO::DOM::Pragma::_squashed_,      /* LAYERING_SQUASHED        */
    LQIO::DOM::Pragma::_srvn_,          /* LAYERING_SRVN            */
    "task-processor",                   /* LAYERING_TASK_PROCESSOR  */
    nullptr                             /* */
};


const char * Options::special[] = {
    "annotate",				/* SPECIAL_ANNOTATE,                    */
    "arrow-scaling",			/* SPECIAL_ARROW_SCALING,		*/
    "bcmp",				/* SPECIAL_BCMP				*/
    "clear-label-background", 		/* SPECIAL_CLEAR_LABEL_BACKGROUND,	*/
    "exhaustive-topological-sort",	/* SPECIAL_EXHAUSTIVE_TOPOLOGICAL_SORT,	*/
    "flatten",				/* SPECIAL_FLATTEN_SUBMODEL,		*/
    "forwarding",			/* SPECIAL_FORWARDING_DEPTH,		*/
    "group",				/* SPECIAL_GROUP,			*/
    "layer-number",			/* SPECIAL_LAYER_NUMBER,		*/
    "no-alignment-box",			/* SPECIAL_NO_ALIGNMENT_BOX,		*/
    "no-async",				/* SPECIAL_NO_ASYNC_TOPOLOGICAL_SORT	*/
    "no-cv-sqr",			/* SPECIAL_NO_CV_SQR,			*/
    "no-phase-type",			/* SPECIAL_NO_PHASE_TYPE,		*/
    "no-reference-task-conversion",	/* SPECIAL_NO_REF_TASK_CONVERSION,	*/
    "prune",				/* SPECIAL_PRUNE			*/
    "processor-scheduling"		/* SPECIAL_PROCESSOR_SCHEDULING		*/
    "quorum-reply",			/* SPECIAL_QUORUM_REPLY,		*/
    "rename",				/* SPECIAL_RENAME			*/
    "sort",				/* SPECIAL_SORT,			*/
    "squish",				/* SPECIAL_SQUISH_ENTRY_NAMES,		*/
    "no-header",			/* SPECIAL_SPEX_HEADER			*/
    "submodels",			/* SPECIAL_SUBMODEL_CONTENTS,		*/
    "tasks-only",			/* SPECIAL_TASKS_ONLY			*/
    "task-scheduling",			/* SPECIAL_TASK_SCHEDULING		*/
    nullptr
};

const char * Options::processor[] = {
    "none",
    "default",
    "non-infinite",
    "all",
    nullptr
};

const char * Options::real [] = {
    "float",
    nullptr
};

const char * Options::replication [] =
{
    "none",
    "remove",
    "expand",
    nullptr
};

const char * Options::sort [] = {
    "ascending",
    "descending",
    "topological",
    "none",
    nullptr
};

const char * Options::string [] = {
    "string",
    nullptr
};

static bool get_bool( const std::string&, bool default_value );

/*----------------------------------------------------------------------*/
/*			      Main line					*/
/*----------------------------------------------------------------------*/

int
main(int argc, char *argv[])
{
    /* We can only initialize integers in the Flags object -- initialize floats here. */

    Flags::print[MAGNIFICATION].value.f = 1.0;
    Flags::print[BORDER].value.f = 18.0;
    Flags::print[X_SPACING].value.f = DEFAULT_X_SPACING;
    Flags::print[Y_SPACING].value.f = DEFAULT_Y_SPACING;

    LQIO::io_vars.init( VERSION, basename( argv[0] ), severity_action, local_error_messages, LSTLCLERRMSG-LQIO::LSTGBLERRMSG );

    command_line += LQIO::io_vars.lq_toolname;

    /* If we are invoked as lqn2xxx or rep2flat, then enable other options. */

    const char * p = strrchr( LQIO::io_vars.toolname(), '2' );
    if ( p ) {
	p += 1;
	for ( int j = 0; Options::io[j]; ++j ) {
	    if ( strncmp( p, Options::io[j], strlen(Options::io[j]) ) == 0 ) {
		setOutputFormat( j );
		goto found1;
	    }
	}
#if defined(REP2FLAT)
	if ( strcmp( p, "flat" ) == 0 ) {
	    setOutputFormat( FORMAT_SRVN );
	    Flags::print[REPLICATION].value.i = REPLICATION_EXPAND;
	    goto found1;
	}
#endif
	std::cerr << LQIO::io_vars.lq_toolname << ": command not found." << std::endl;
	exit( 1 );
    found1: ;
    }

    return lqn2ps( argc, argv );
}

/*
 * construct the error message.
 */

class_error::class_error( const std::string& method, const char * file, const unsigned line, const std::string& error )
    : logic_error( message( method, file, line, error ) )
{
}


class_error::~class_error() throw()
{
}

std::string
class_error::message( const std::string& method, const char * file, const unsigned line, const std::string& error )
{
    std::ostringstream ss;
    ss << method << ": " << file << " " << line << ": " << error;
    return ss.str();
}


size_t
Options::find_if( const char** values, const std::string& s )
{
    size_t i = 0;
    for ( ; values[i] != nullptr; ++i ) {
	if ( s == values[i] ) return i;
    }
    return i+1;
}

bool
process_special( const char * p, LQIO::DOM::Pragma& pragmas )
{
    if ( !p ) return false;

    bool rc = true;

    do {
	while ( isspace( *p ) ) ++p;		/* Skip leading whitespace. */
	std::string param;
	std::string value;
	while ( *p && !isspace( *p ) && *p != '=' & *p != ',' ) {
	    param += *p++;			/* get parameter */
	}
	while ( isspace( *p ) ) ++p;
	if ( *p == '=' ) {
	    ++p;
	    while ( isspace( *p ) ) ++p;
	    while ( *p && !isspace( *p ) && *p != ',' ) {
		value += *p++;
	    }
	}
	while ( isspace( *p ) ) ++p;
	rc = rc && special( param, value, pragmas );
    } while ( *p++ == ',' );
    return rc;
}


bool
special( const std::string& parameter, const std::string& value, LQIO::DOM::Pragma& pragmas )
{
    try {
	char * endptr = nullptr;

	switch ( Options::find_if( Options::special, parameter ) ) {

	case SPECIAL_ANNOTATE:			  Flags::annotate_input			= get_bool( value, true ); break;
	case SPECIAL_CLEAR_LABEL_BACKGROUND:	  Flags::clear_label_background		= get_bool( value, true ); break;
	case SPECIAL_EXHAUSTIVE_TOPOLOGICAL_SORT: Flags::exhaustive_toplogical_sort	= get_bool( value, true ); break;
	case SPECIAL_FLATTEN_SUBMODEL:		  Flags::flatten_submodel		= get_bool( value, true ); break;
	case SPECIAL_FORWARDING_DEPTH:		  Flags::print_forwarding_by_depth	= get_bool( value, true ); break;
	case SPECIAL_LAYER_NUMBER:		  Flags::print_layer_number		= get_bool( value, true ); break;
	case SPECIAL_NO_ALIGNMENT_BOX:		  Flags::print_alignment_box		= get_bool( value, false ); break;
	case SPECIAL_NO_ASYNC_TOPOLOGICAL_SORT:	  Flags::async_topological_sort		= get_bool( value, false ); break;
	case SPECIAL_NO_CV_SQR:			  Flags::output_coefficient_of_variation= get_bool( value, false ); break;
	case SPECIAL_NO_PHASE_TYPE:		  Flags::output_phase_type		= get_bool( value, false ); break;
	case SPECIAL_NO_REF_TASK_CONVERSION:	  Flags::convert_to_reference_task	= get_bool( value, false ); break;
	case SPECIAL_RENAME:			  Flags::rename_model			= get_bool( value, true ); break;
	case SPECIAL_SQUISH_ENTRY_NAMES:	  Flags::squish_names			= get_bool( value, true ); break;
	case SPECIAL_SUBMODEL_CONTENTS:		  Flags::print_submodels		= get_bool( value, true ); break;

	case SPECIAL_BCMP:
	    pragmas.insert(LQIO::DOM::Pragma::_bcmp_, value );
	    break;
	    
	case SPECIAL_PROCESSOR_SCHEDULING:
	    pragmas.insert(LQIO::DOM::Pragma::_processor_scheduling_, value );
	    break;
	    
	case SPECIAL_PRUNE:
	    pragmas.insert(LQIO::DOM::Pragma::_prune_, value );
	    break;

	case SPECIAL_QUORUM_REPLY:
	    LQIO::io_vars.error_messages[LQIO::ERR_REPLY_NOT_GENERATED].severity = LQIO::WARNING_ONLY;
	    break;

	case SPECIAL_SORT:
	    Flags::sort = static_cast<sort_type>(Options::find_if( Options::sort, value ));
	    if ( Flags::sort == INVALID_SORT ) throw std::domain_error( value );
	    break;

	case SPECIAL_TASK_SCHEDULING:
	    pragmas.insert(LQIO::DOM::Pragma::_task_scheduling_, value );
	    break;
	    
	case SPECIAL_TASKS_ONLY:
	    Flags::print[AGGREGATION].value.i = AGGREGATE_ENTRIES;
	    if ( Flags::icon_height == DEFAULT_ICON_HEIGHT ) {
		if ( processor_output() || share_output() ) {
		    Flags::print[Y_SPACING].value.f = 45;
		} else {
		    Flags::print[Y_SPACING].value.f = 27;
		}
		Flags::icon_height = 18;
		Flags::entry_height = Flags::icon_height * 0.6;
	    }
	    break;

	case SPECIAL_ARROW_SCALING:
	    Flags::arrow_scaling = strtod( value.c_str(), &endptr );
	    if ( Flags::arrow_scaling <= 0 || *endptr != '\0' ) throw std::domain_error( value );
	    break;

	case SPECIAL_GROUP:
	    Model::add_group( value.c_str() );
	    break;

	default:
	    std::cerr << LQIO::io_vars.lq_toolname << ": Unknown argument: \"" << parameter;
	    if ( value.size() ) {
		std::cerr << "=" << value;
	    }
	    std::cerr << "\"" << std::endl;
	    return false;
	}
    }
    catch ( const std::domain_error& e ) {
	std::cerr << LQIO::io_vars.lq_toolname << ": Invalid value: \"" << parameter << "=" << value << "\"" << std::endl;
	return false;
    }

    return true;
}


/* static */ bool
get_bool( const std::string& arg, const bool default_value )
{
    if ( arg.size() == 0 ) return default_value;
    return LQIO::DOM::Pragma::isTrue( arg );
}

/*
 * Return true if we are generating graphical output of some form.
 */

bool
graphical_output()
{
    return Flags::print[OUTPUT_FORMAT].value.i != FORMAT_LQX
	&& Flags::print[OUTPUT_FORMAT].value.i != FORMAT_NULL
	&& Flags::print[OUTPUT_FORMAT].value.i != FORMAT_OUTPUT
	&& Flags::print[OUTPUT_FORMAT].value.i != FORMAT_PARSEABLE
 	&& Flags::print[OUTPUT_FORMAT].value.i != FORMAT_RTF
	&& Flags::print[OUTPUT_FORMAT].value.i != FORMAT_SRVN
#if defined(TXT_OUTPUT)
	&& Flags::print[OUTPUT_FORMAT].value.i != FORMAT_TXT
#endif
	&& Flags::print[OUTPUT_FORMAT].value.i != FORMAT_XML
	;
}


/*
 * Return true if we are generating a new resutls file of some form.
 */

bool
output_output()
{
    return Flags::print[OUTPUT_FORMAT].value.i == FORMAT_OUTPUT
	|| Flags::print[OUTPUT_FORMAT].value.i == FORMAT_PARSEABLE
	|| Flags::print[OUTPUT_FORMAT].value.i == FORMAT_RTF;
}


/*
 * Return true if we are generating a new input file of some form.
 */

bool
input_output()
{
    return Flags::print[OUTPUT_FORMAT].value.i == FORMAT_SRVN
	|| Flags::print[OUTPUT_FORMAT].value.i == FORMAT_LQX
	|| Flags::print[OUTPUT_FORMAT].value.i == FORMAT_XML
	;
}


/*
 * Return true if we're only printing partial results.
 */

bool
partial_output()
{
    return submodel_output() || queueing_output()
	|| Flags::print[INCLUDE_ONLY].value.r != nullptr;
}

bool
processor_output()
{
    return Flags::print[LAYERING].value.i == LAYERING_PROCESSOR
	|| Flags::print[LAYERING].value.i == LAYERING_PROCESSOR_TASK
	|| Flags::print[LAYERING].value.i == LAYERING_TASK_PROCESSOR;
}

bool
queueing_output()
{
    return Flags::print[QUEUEING_MODEL].value.i != 0;
}


bool
share_output()
{
    return Flags::print[LAYERING].value.i == LAYERING_SHARE;
}

bool
submodel_output()
{
    return Flags::print[SUBMODEL].value.i != 0;
}

bool
difference_output()
{
    return Flags::print[COLOUR].value.i == COLOUR_DIFFERENCES;
}

#if defined(REP2FLAT)
void
update_mean( LQIO::DOM::DocumentObject * dst, set_function set, const LQIO::DOM::DocumentObject * src, get_function get, unsigned int replica )
{
    (dst->*set)( ((dst->*get)() * static_cast<double>(replica - 1) + (src->*get)()) / static_cast<double>(replica) );
}


void
update_variance( LQIO::DOM::DocumentObject * dst, set_function set, const LQIO::DOM::DocumentObject * src, get_function get )
{
    (dst->*set)( (dst->*get)() + (src->*get)() );
}
#endif

static int current_indent = 1;

int
set_indent( const int anInt )
{
    const int old_indent = current_indent;
    current_indent = anInt;
    return old_indent;
}

std::ostream&
pluralize( std::ostream& output, const std::string& aStr, const unsigned int i )
{
    output << aStr;
    if ( i != 1 ) output << "s";
    return output;
}

std::ostream&
indent_str( std::ostream& output, const int anInt )
{
    if ( anInt < 0 ) {
	if ( current_indent + anInt < 0 ) {
	    current_indent = 0;
	} else {
	    current_indent += anInt;
	}
    }
    if ( current_indent != 0 ) {
	output << std::setw( current_indent * 3 ) << " ";
    }
    if ( anInt > 0 ) {
	current_indent += anInt;
    }
    return output;
}

std::ostream&
temp_indent_str( std::ostream& output, const int anInt )
{
    output << std::setw( (current_indent + anInt) * 3 ) << " ";
    return output;
}

std::ostream&
opt_pct_str( std::ostream& output, const double aDouble )
{
    output << aDouble;
    if ( difference_output() ) {
	output << "%";
    }
    return output;
}


static std::ostream&
conf_level_str( std::ostream& output, const int fill, const int level )
{
    std::ios_base::fmtflags flags = output.setf( std::ios::right, std::ios::adjustfield );
    output << std::setw( fill-4 ) << "+/- " << std::setw(2) << level << "% ";
    output.flags( flags );
    return output;
}

IntegerManip indent( const int i ) { return IntegerManip( &indent_str, i ); }
IntegerManip temp_indent( const int i ) { return IntegerManip( &temp_indent_str, i ); }
Integer2Manip conf_level( const int fill, const int level ) { return Integer2Manip( &conf_level_str, fill, level ); }
StringPlural plural( const std::string& s, const unsigned i ) { return StringPlural( &pluralize, s, i ); }
DoubleManip opt_pct( const double aDouble ) { return DoubleManip( &opt_pct_str, aDouble ); }
