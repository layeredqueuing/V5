/* srvn2eepic.c	-- Greg Franks Sun Jan 26 2003
 *
 * $Id: option.cc 15154 2021-12-03 22:16:10Z greg $
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
#include <lqio/json_document.h>
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

const std::map<const file_format,const std::string> Options::io =
{
    { file_format::EEPIC,	"eepic" },
#if EMF_OUTPUT
    { file_format::EMF,		"emf" },
#endif
    { file_format::FIG,		"fig" },
#if HAVE_GD_H && HAVE_LIBGD && HAVE_GDIMAGEGIFPTR
    { file_format::GIF,		"gif" },
#endif
#if JMVA_OUTPUT && HAVE_EXPAT_H
    { file_format::JMVA,	"jmva" },
#endif
#if HAVE_GD_H && HAVE_LIBGD && HAVE_LIBJPEG
    { file_format::JPEG,	"jpeg" },
#endif
    { file_format::JSON,	"json" },
    { file_format::LQX,		"lqx" },
    { file_format::NO_OUTPUT,	"null" },
    { file_format::OUTPUT,	"out" },
#if QNAP2_OUTPUT
    { file_format::QNAP2,	"qnap2" },
#endif
    { file_format::PARSEABLE,	"parseable" },
#if HAVE_GD_H && HAVE_LIBGD && HAVE_LIBPNG
    { file_format::PNG,		"png" },
#endif
    { file_format::POSTSCRIPT,	"ps" },
    { file_format::PSTEX,	"pstex" },
    { file_format::RTF,		"rtf" },
    { file_format::SRVN,	"lqn" },
#if SVG_OUTPUT
    { file_format::SVG,		"svg" },
#endif
#if SXD_OUTPUT
    { file_format::SXD,		"sxd" },
#endif
#if TXT_OUTPUT
    { file_format::TXT,		"txt" },
#endif
#if X11_OUTPUT
    { file_format::X11,		"x11" },
#endif
    { file_format::XML,		"xml" }
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

const std::map<const Layering, const std::string> Options::layering =
{
    { Layering::BATCH,          LQIO::DOM::Pragma::_batched_ },
    { Layering::GROUP,          "group" },
    { Layering::HWSW,           LQIO::DOM::Pragma::_hwsw_ },
    { Layering::MOL,            LQIO::DOM::Pragma::_mol_ },
    { Layering::PROCESSOR,      "processor" },
    { Layering::PROCESSOR_TASK, "processor-task" },
    { Layering::SHARE,          "share" },
    { Layering::SQUASHED,       LQIO::DOM::Pragma::_squashed_ },
    { Layering::SRVN,           LQIO::DOM::Pragma::_srvn_ },
    { Layering::TASK_PROCESSOR,	"task-processor" }
};


const char * Options::special[] = {
    "annotate",				/* SPECIAL_ANNOTATE,                    */
    "arrow-scaling",			/* SPECIAL_ARROW_SCALING,		*/
    LQIO::DOM::Pragma::_bcmp_,  	/* SPECIAL_BCMP				*/
    "clear-label-background", 		/* SPECIAL_CLEAR_LABEL_BACKGROUND,	*/
    "exhaustive-topological-sort",	/* SPECIAL_EXHAUSTIVE_TOPOLOGICAL_SORT,	*/
    "flatten",				/* SPECIAL_FLATTEN_SUBMODEL,		*/
    LQIO::DOM::Pragma::_force_infinite_,	/* SPECIAL_FORCE_INFINITE	*/
    "forwarding",			/* SPECIAL_FORWARDING_DEPTH,		*/
    "group",				/* SPECIAL_GROUP,			*/
    "layer-number",			/* SPECIAL_LAYER_NUMBER,		*/
    "no-alignment-box",			/* SPECIAL_NO_ALIGNMENT_BOX,		*/
    "no-async",				/* SPECIAL_NO_ASYNC_TOPOLOGICAL_SORT	*/
    "no-cv-sqr",			/* SPECIAL_NO_CV_SQR,			*/
    "no-phase-type",			/* SPECIAL_NO_PHASE_TYPE,		*/
    "no-reference-task-conversion",	/* SPECIAL_NO_REF_TASK_CONVERSION,	*/
    "prune",				/* SPECIAL_PRUNE			*/
    LQIO::DOM::Pragma::_processor_scheduling_,	/* SPECIAL_PROCESSOR_SCHEDULING	*/
    "quorum-reply",			/* SPECIAL_QUORUM_REPLY,		*/
    "rename",				/* SPECIAL_RENAME			*/
    "sort",				/* SPECIAL_SORT,			*/
    "squish",				/* SPECIAL_SQUISH_ENTRY_NAMES,		*/
    "no-header",			/* SPECIAL_SPEX_HEADER			*/
    "submodels",			/* SPECIAL_SUBMODEL_CONTENTS,		*/
    "tasks-only",			/* SPECIAL_TASKS_ONLY			*/
    LQIO::DOM::Pragma::_task_scheduling_,	/* SPECIAL_TASK_SCHEDULING	*/
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

    Flags::print[MAGNIFICATION].opts.value.f = 1.0;
    Flags::print[BORDER].opts.value.f = 18.0;
    Flags::print[X_SPACING].opts.value.f = DEFAULT_X_SPACING;
    Flags::print[Y_SPACING].opts.value.f = DEFAULT_Y_SPACING;

    LQIO::io_vars.init( VERSION, basename( argv[0] ), severity_action, local_error_messages, LSTLCLERRMSG-LQIO::LSTGBLERRMSG );

    command_line += LQIO::io_vars.lq_toolname;

    /* If we are invoked as lqn2xxx or rep2flat, then enable other options. */

    const char * p = strrchr( LQIO::io_vars.toolname(), '2' );
    if ( p ) {
	p += 1;
	for ( std::map<const file_format,const std::string>::const_iterator j = Options::io.begin(); j != Options::io.end(); ++j ) {
	    if ( j->second == p ) {
		setOutputFormat( j->first );
		goto found1;
	    }
	}
#if defined(REP2FLAT)
	if ( strcmp( p, "flat" ) == 0 ) {
	    setOutputFormat( file_format::SRVN );
	    Flags::print[REPLICATION].opts.value.i = REPLICATION_EXPAND;
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
	
	case SPECIAL_FORCE_INFINITE:
	    pragmas.insert(LQIO::DOM::Pragma::_force_infinite_, value );
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
	    Flags::print[AGGREGATION].opts.value.i = AGGREGATE_ENTRIES;
	    if ( Flags::icon_height == DEFAULT_ICON_HEIGHT ) {
		if ( processor_output() || share_output() ) {
		    Flags::print[Y_SPACING].opts.value.f = 45;
		} else {
		    Flags::print[Y_SPACING].opts.value.f = 27;
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
    static const std::set<file_format> reject = {
#if JMVA_OUTPUT
	file_format::JMVA,
#endif
	file_format::JSON,
	file_format::LQX,
	file_format::NO_OUTPUT,
	file_format::OUTPUT,
	file_format::PARSEABLE,
#if QNAP2_OUTPUT
	file_format::QNAP2,
#endif
	file_format::RTF,
	file_format::SRVN,
#if TXT_OUTPUT
	file_format::TXT,
#endif
	file_format::XML
    };

    return std::find( reject.begin(), reject.end(), Flags::print[OUTPUT_FORMAT].opts.value.o ) != reject.end();
}


/*
 * Return true if we are generating a new resutls file of some form.
 */

bool
output_output()
{
    return Flags::print[OUTPUT_FORMAT].opts.value.o == file_format::OUTPUT
	|| Flags::print[OUTPUT_FORMAT].opts.value.o == file_format::PARSEABLE
	|| Flags::print[OUTPUT_FORMAT].opts.value.o == file_format::RTF;
}


/*
 * Return true if we are generating a new input file of some form.
 */

bool
input_output()
{
    return Flags::print[OUTPUT_FORMAT].opts.value.o == file_format::SRVN
	|| Flags::print[OUTPUT_FORMAT].opts.value.o == file_format::JSON
	|| Flags::print[OUTPUT_FORMAT].opts.value.o == file_format::LQX
	|| Flags::print[OUTPUT_FORMAT].opts.value.o == file_format::XML
	;
}


/*
 * Return true if we're only printing partial results.
 */

bool
partial_output()
{
    return submodel_output() || queueing_output()
	|| Flags::print[INCLUDE_ONLY].opts.value.r != nullptr;
}

bool
processor_output()
{
    return Flags::print[LAYERING].opts.value.l == Layering::PROCESSOR
	|| Flags::print[LAYERING].opts.value.l == Layering::PROCESSOR_TASK
	|| Flags::print[LAYERING].opts.value.l == Layering::TASK_PROCESSOR;
}

bool
queueing_output()
{
    return Flags::print[QUEUEING_MODEL].opts.value.i != 0;
}


bool
share_output()
{
    return Flags::print[LAYERING].opts.value.l == Layering::SHARE;
}

bool
submodel_output()
{
    return Flags::print[SUBMODEL].opts.value.i != 0;
}

bool
difference_output()
{
    return Flags::print[COLOUR].opts.value.i == COLOUR_DIFFERENCES;
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
