/* srvn2eepic.c	-- Greg Franks Sun Jan 26 2003
 *
 * $Id$
 */

#include "lqn2ps.h"
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <cstring>
#if HAVE_IEEEFP_H
#include <ieeefp.h>
#endif
#if !HAVE_GETSUBOPT
#include <lqio/getsbopt.h>
#endif
#include "vector.h"
#include "layer.h"
#include "model.h"
#include "errmsg.h"

static double DEFAULT_ICON_HEIGHT = 45;	/* multiple of 9 works well with xfig. */

std::string command_line;

lqio_params_stats io_vars =
{
    /* .n_processors =   */ 0,
    /* .n_tasks =	 */ 0,
    /* .n_entries =      */ 0,
    /* .n_groups =       */ 0,
    /* .lq_toolname =    */ NULL,
    /* .lq_version =	 */ VERSION,
    /* .lq_command_line =*/ NULL,
    /* .severity_action= */ severity_action,
    /* .max_error =      */ 0,
    /* .error_count =    */ 0,
    /* .severity_level = */ LQIO::NO_ERROR,
    /* .error_messages = */ NULL,
    /* .anError =        */ 0
};

bool Flags::annotate_input		= false;
bool Flags::async_topological_sort      = true;
bool Flags::clear_label_background 	= false;
bool Flags::convert_to_reference_task	= true; 
bool Flags::exhaustive_toplogical_sort	= false;
bool Flags::flatten_submodel		= false;
bool Flags::have_results		= false;
bool Flags::instantiate			= false;
bool Flags::output_coefficient_of_variation = true;
bool Flags::output_phase_type		= true;
bool Flags::print_alignment_box		= true;
bool Flags::print_forwarding_by_depth	= false;
bool Flags::print_layer_number  	= false;
bool Flags::print_submodels     	= false;
bool Flags::rename_model		= false;
bool Flags::squish_names		= false;
bool Flags::surrogates			= false;
bool Flags::use_colour			= true;
bool Flags::debug_submodels	        = false;

double Flags::act_x_spacing		= 6.0;
double Flags::arrow_scaling		= 1.0;
double Flags::entry_height             	= 0.6 * DEFAULT_ICON_HEIGHT;	/* 27 */
double Flags::entry_width              	= DEFAULT_ICON_HEIGHT;		/* 45 */
double Flags::icon_height               = DEFAULT_ICON_HEIGHT;
double Flags::icon_width               	= DEFAULT_ICON_HEIGHT * 1.6;	/* 72 */

#if HAVE_REGEX_T
regex_t * Flags::client_tasks		= 0;
#endif

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
#if defined(EMF_OUTPUT)
    "emf",
#endif
    "fig",
#if HAVE_GD_H && HAVE_LIBGD && HAVE_GDIMAGEGIFPTR
    "gif",
#endif
#if HAVE_GD_H && HAVE_LIBGD && HAVE_LIBJPEG 
    "jpeg",
#endif
    "null",
    "out",
    "parseable",
#if HAVE_GD_H && HAVE_LIBGD && HAVE_LIBPNG
    "png",
#endif
    "ps",
    "pstex",
#if defined(QNAP_OUTPUT)
    "qnap",
#endif
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
    0
};

const char * Options::justification [] = 
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
    "batch",                            /* LAYERING_BATCH           */
    "group",                            /* LAYERING_GROUP           */
    "hwsw",                             /* LAYERING_HWSW            */
    "srvn",                             /* LAYERING_SPAN            */
    "processor",                        /* LAYERING_PROCESSOR       */
    "processor-task",                   /* LAYERING_PROCESSOR_TASK  */
    "task-processor",                   /* LAYERING_TASK_PROCESSOR  */
    "share",                            /* LAYERING_SHARE           */
    "squashed",                         /* LAYERING_SQUASHED        */
    "method-of-layers",                 /* LAYERING_MOL             */
    "client",                           /* LAYERING_CLIENT          */
    0                                   /* */
};                                         


const char * Options::pragma[] = {
    "annotate",				/* PRAGMA_ANNOTATE,                     */
    "arrow-scaling",			/* PRAGMA_ARROW_SCALING,		*/
    "clear-label-background", 		/* PRAGMA_CLEAR_LABEL_BACKGROUND,	*/
    "exhaustive-topological-sort",	/* PRAGMA_EXHAUSTIVE_TOPOLOGICAL_SORT,	*/
    "flatten",				/* PRAGMA_FLATTEN_SUBMODEL,		*/
    "forwarding",			/* PRAGMA_FORWARDING_DEPTH,		*/
    "group",				/* PRAGMA_GROUP,			*/
    "layer-number",			/* PRAGMA_LAYER_NUMBER,			*/
    "no-alignment-box",			/* PRAGMA_NO_ALIGNMENT_BOX,		*/
    "no-async",				/* PRAGMA_NO_ASYNC_TOPOLOGICAL_SORT	*/
    "no-cv-sqr",			/* PRAGMA_NO_CV_SQR,			*/
    "no-phase-type",			/* PRAGMA_NO_PHASE_TYPE,		*/
    "no-reference-task-conversion",	/* PRAGMA_NO_REF_TASK_CONVERSION,	*/
    "quorum-reply",			/* PRAGMA_QUORUM_REPLY,			*/
    "rename",				/* PRAGMA_RENAME			*/
    "sort",				/* PRAGMA_SORT,				*/
    "squish",				/* PRAGMA_SQUISH_ENTRY_NAMES,		*/
    "submodels",			/* PRAGMA_SUBMODEL_CONTENTS,		*/
    "tasks-only",			/* PRAGMA_TASKS_ONLY			*/
    0					
};

const char * Options::processor[] = {
    "none",
    "default",
    "non-infinite",
    "all",
    0
};

const char * Options::real [] = {
    "float",
    0
};

const char * Options::replication [] = 
{
    "none",
    "remove",
    "expand",
    0
};

const char * Options::sort [] = {
    "ascending",
    "descending",
    "topological",
    "none",
    0
};

const char * Options::string [] = {
    "string",
    0
};

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

    io_vars.lq_toolname = strrchr( argv[0], '/' );
    if ( io_vars.lq_toolname ) {
	io_vars.lq_toolname += 1;
    } else {
	io_vars.lq_toolname = argv[0];
    }

    command_line += io_vars.lq_toolname;

    init_errmsg();

    /* If we are invoked as lqn2xxx or rep2flat, then enable other options. */

    const char * p = strrchr( io_vars.lq_toolname, '2' );
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
	cerr << io_vars.lq_toolname << ": command not found." << endl;
	exit( 1 );
    found1: ;
    }
    
    return lqn2ps( argc, argv );
}

/*
 * construct the error message.
 */

class_error::class_error( const string& aStr, const char * file, const unsigned line, const char * anError )
    : exception()
{
    char temp[10];
    sprintf( temp, "%d", line );

    myMsg = aStr;
    myMsg += ": ";
    myMsg += file;
    myMsg += " ";
    myMsg += temp;
    myMsg += ": ";
    myMsg += anError;
}


class_error::~class_error() throw()
{
}

const char * 
class_error::what() const throw()
{
    return myMsg.c_str();
}


const char * 
path_error::what() const throw()
{
    return myMsg.c_str();
}

#if HAVE_REGEX_T
void 
regexp_check( const int errcode, regex_t * r ) throw( runtime_error )
{
    if ( errcode ) {
	char buf[BUFSIZ];
	regerror( errcode, r, buf, BUFSIZ );
	throw runtime_error( buf );
    }
}
#endif

bool
process_pragma( const char * p )
{
    if ( !p ) return false;

    bool rc = true;

    do {
	while ( isspace( *p ) ) ++p;		/* Skip leading whitespace. */
	string param;
	string value;
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
	rc = rc && pragma( param, value );
    } while ( *p++ == ',' );
    return rc;
}


bool
pragma( const string& parameter, const string& value )
{
    for ( unsigned int i = 0; Options::pragma[i] != 0; ++i ) {
	if ( parameter.compare( Options::pragma[i] ) != 0 ) continue;

	switch ( i ) {
	case PRAGMA_ANNOTATE:                   Flags::annotate_input 			= true; break;
	case PRAGMA_CLEAR_LABEL_BACKGROUND:     Flags::clear_label_background 		= true; break;
	case PRAGMA_EXHAUSTIVE_TOPOLOGICAL_SORT:Flags::exhaustive_toplogical_sort 	= true; break;
	case PRAGMA_FLATTEN_SUBMODEL:		Flags::flatten_submodel			= true;	break;
	case PRAGMA_FORWARDING_DEPTH:           Flags::print_forwarding_by_depth 	= true; break;
	case PRAGMA_LAYER_NUMBER:               Flags::print_layer_number 		= true; break;
	case PRAGMA_NO_ALIGNMENT_BOX:		Flags::print_alignment_box		= false; break;
	case PRAGMA_NO_ASYNC_TOPOLOGICAL_SORT:  Flags::async_topological_sort		= false; break;
	case PRAGMA_NO_CV_SQR:			Flags::output_coefficient_of_variation  = false; break;
	case PRAGMA_NO_PHASE_TYPE:		Flags::output_phase_type                = false; break;
	case PRAGMA_NO_REF_TASK_CONVERSION:	Flags::convert_to_reference_task	= false; break;
	case PRAGMA_RENAME:			Flags::rename_model	 		= true; break;
	case PRAGMA_SQUISH_ENTRY_NAMES:         Flags::squish_names	 		= true; break;
	case PRAGMA_SUBMODEL_CONTENTS:          Flags::print_submodels 			= true; break;

	case PRAGMA_QUORUM_REPLY:
	    io_vars.error_messages[LQIO::ERR_REPLY_NOT_GENERATED].severity = LQIO::WARNING_ONLY;
	    break;

	case PRAGMA_TASKS_ONLY:
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

	case PRAGMA_SORT:
	    Flags::sort = INVALID_SORT;
	    if ( value.size() ) {
		for ( int i = 0; i < INVALID_SORT; ++i ) {
		    if ( value.compare( Options::sort[i] ) == 0 ) {
			Flags::sort = static_cast<sort_type>(i);
		    }
		} 
	    }
	    if ( Flags::sort == INVALID_SORT ) {
		cerr << io_vars.lq_toolname << ": Invalid argument to 'sort=' :" << value << endl; 
		return false;
	    }
	    break;

	case PRAGMA_ARROW_SCALING:
	    Flags::arrow_scaling = strtod( value.c_str(), 0 );
	    break;

#if HAVE_REGEX_T
	case PRAGMA_GROUP:
	    Model::add_group( value.c_str() );
	    break;
#endif

	default:
	    cerr << io_vars.lq_toolname << ": Unknown pragma: \"" << parameter;
	    if ( value.size() ) {
		cerr << "\"=\"" << value;
	    }
	    cerr << "\"" << endl;
	    return false;
	}
    }
    return true;
}



/*
 * Return true if we are generating graphical output of some form.
 */

bool
graphical_output()
{
    return Flags::print[OUTPUT_FORMAT].value.i != FORMAT_SRVN 
	&& Flags::print[OUTPUT_FORMAT].value.i != FORMAT_OUTPUT
	&& Flags::print[OUTPUT_FORMAT].value.i != FORMAT_PARSEABLE
	&& Flags::print[OUTPUT_FORMAT].value.i != FORMAT_RTF
	&& Flags::print[OUTPUT_FORMAT].value.i != FORMAT_NULL
#if defined(TXT_OUTPUT)
	&& Flags::print[OUTPUT_FORMAT].value.i != FORMAT_TXT
#endif
#if defined(QNAP_OUTPUT)
	&& Flags::print[OUTPUT_FORMAT].value.i != FORMAT_QNAP
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
#if HAVE_REGEX_T
	|| Flags::print[INCLUDE_ONLY].value.r != 0
#endif
      ;
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


static int current_indent = 1;

int
set_indent( const unsigned int anInt ) 
{
    unsigned int old_indent = current_indent;   
    current_indent = max( anInt, 0 );
    return old_indent;
}

static ostream&
value_str_str( ostream& output, const char * aStr )
{
    output << '"' << aStr << '"';
    return output;
}

static ostream&
value_int_str( ostream& output, const int anInt )
{
    output << '"' << anInt << '"';
    return output;
}

static ostream&
pluralize( ostream& output, const string& aStr, const unsigned int i ) 
{
    output << aStr;
    if ( i != 1 ) output << "s";
    return output;
}

static ostream&
indent_str( ostream& output, const int anInt )
{
    if ( anInt < 0 ) {
	if ( static_cast<int>(current_indent) + anInt < 0 ) {
	    current_indent = 0;
	} else {
	    current_indent += anInt;
	}
    }
    if ( current_indent != 0 ) {
	output << setw( current_indent * 3 ) << " ";
    }
    if ( anInt > 0 ) {
	current_indent += anInt;
    }
    return output;
}

static ostream&
temp_indent_str( ostream& output, const int anInt )
{
    output << setw( (current_indent + anInt) * 3 ) << " ";
    return output;
}

static ostream&
value_bool_str( ostream& output, const bool aBool )
{
    output << '"' << (aBool ? "yes" : "no") << '"';
    return output;
}

static ostream&
value_double_str( ostream& output, const double aDouble )
{
    output << '"' << aDouble << '"';
    return output;
}


static ostream&
conf_level_str( ostream& output, const int fill, const int level ) 
{	
    FMT_FLAGS flags = output.setf( ios::right, ios::adjustfield );
    output << setw( fill-4 ) << "+/- " << setw(2) << level << "% ";
    output.flags( flags );
    return output;
}

/*
 * Instantiate var if we are running the lqx program.
 */

static ostream&
instantiate_str( ostream& output, const LQIO::DOM::ExternalVariable& var )
{
    if ( Flags::instantiate ) {
	output << to_double( var );
    } else {
	output << var;
    }
    return output;
}

StringManip value_str( const char * aStr )
{
    return StringManip( &value_str_str, aStr );
}

StringPlural plural( const string& s, const unsigned i )
{
    return StringPlural( &pluralize, s, i );
}

IntegerManip value_int( const int anInt )
{
    return IntegerManip( &value_int_str, anInt );
}

IntegerManip indent( const int anInt )
{
    return IntegerManip( &indent_str, anInt );
}

IntegerManip temp_indent( const int anInt )
{
    return IntegerManip( &temp_indent_str, anInt );
}

BooleanManip value_bool( const bool aBool )
{
    return BooleanManip( &value_bool_str, aBool );
}

DoubleManip value_double( const double aDouble )
{
    return DoubleManip( &value_double_str, aDouble );
}

Integer2Manip conf_level( const int fill, const int level )
{
    return Integer2Manip( &conf_level_str, fill, level );
}

ExtVarManip instantiate( const LQIO::DOM::ExternalVariable& var )
{
    return ExtVarManip( instantiate_str, var );
}

#if defined(__GNUC__) || (defined(__INTEL_COMPILER) && (__INTEL_COMPILER >= 700))
#include "cltn.h"
#include "stack.h"
#include "arc.h"
#include "phase.h"
#include "call.h"
#include "share.h"
#include "entity.h"
#include "entry.h"
#include "activity.h"
#include "actlayer.h"
#include "actlist.h"
#include "open.h"
#include "group.h"
#include "label.h"
#include "processor.h"
#include "task.h"
#include "vector.h"
#include "vector.cc"
#include "cltn.cc"
#include "stack.cc"
#include <lqio/dom_call.h>

template class BackwardsSequence<Arc *>;
template class BackwardsSequence<Entry *>;
template class BackwardsSequence<Label *>;
template class BackwardsSequence<Activity *>;
template class Cltn<Activity *>;
template class Cltn<ActivityCall *>;
template class Cltn<ActivityList *>;  
template class Cltn<Arc *>;
template class Cltn<Call *>;
template class Cltn<Share *>;
template class Cltn<Entity *>;
template class Cltn<Entity const *>;
template class Cltn<EntityCall *>;
template class Cltn<Entry *>;
template class Cltn<GenericCall *>;
template class Cltn<Group *>;
template class Cltn<Label *>;
template class Cltn<Layer *>;
template class Cltn<OpenArrival *>;
template class Cltn<OpenArrivalSource *>;
template class Cltn<Processor *>;
template class Cltn<Reply *>;
template class Cltn<Task *>;
template class Cltn<const AndForkActivityList *>;
template class Cltn<const Entry *>;
template class Cltn<const char *>;
template class Cltn<ostringstream *>;
template class Cltn<istringstream *>;
template class Sequence<Activity *>;
template class Sequence<ActivityCall *>;
template class Sequence<ActivityList *>;  
template class Sequence<Arc *>;
template class Sequence<Call *>;
template class Sequence<Share *>;
template class Sequence<Entity *>;
template class Sequence<Entity const *>;
template class Sequence<EntityCall *>;
template class Sequence<Entry *>;
template class Sequence<GenericCall *>;
template class Sequence<Group *>;
template class Sequence<Label *>;
template class Sequence<OpenArrival *>;
template class Sequence<Processor*>;
template class Sequence<Reply *>;
template class Sequence<Task *>;
template class Sequence<const Entry *>;
template class Sequence<const char *>;
template class Sequence<istringstream *>;
template class Stack<const Activity *>;
template class Stack<const AndForkActivityList *>;
template class Stack<const Call *>;
template class Vector2<ActivityLayer>;
template class Vector2<LQIO::DOM::Call const*>;
template class Vector2<Layer>;
template class Vector2<Phase::Histogram::Bin>;
template class Vector2<Phase>;
template class Vector2<Point>;
template class Vector<bool>;
template class Vector<double>;
template class Vector<int>;
template class Vector<unsigned>;

template ostream& operator<<( ostream& output, const Vector<double>& self );
#endif
