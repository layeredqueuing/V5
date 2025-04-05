/* srvn2eepic.c	-- Greg Franks Sun Jan 26 2003
 *
 * $Id: option.cc 17536 2025-04-02 13:42:13Z greg $
 */

#include "lqn2ps.h"
#include <algorithm>
#include <lqio/dom_document.h>
#include <lqio/dom_pragma.h>
#include "model.h"
#include "option.h"
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
bool Flags::normalize_utilization	= false;
bool Flags::output_coefficient_of_variation = true;
bool Flags::output_phase_type		= true;
bool Flags::print_alignment_box		= true;
bool Flags::print_comment		= false;
bool Flags::print_forwarding_by_depth	= false;
bool Flags::print_layer_number  	= false;
bool Flags::print_spex			= false;
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

Sorting Flags::sort	 		= Sorting::FORWARD;

Justification Flags::node_justification = Justification::DEFAULT;
Justification Flags::label_justification = Justification::CENTER;
Justification Flags::activity_justification = Justification::DEFAULT;
Output_Style Flags::graphical_output_style = Output_Style::TIMEBENCH;

double Flags::icon_slope	        = 1.0/10.0;
unsigned int maxStrLen 		= 16;
const unsigned int maxDblLen	= 12;		/* Field width in srvnoutput. */

const std::map<const Aggregate, const std::string> Options::aggregate =
{
    { Aggregate::NONE,           LQIO::DOM::Pragma::_none_ },
    { Aggregate::SEQUENCES,      "sequences" },
    { Aggregate::ACTIVITIES,     "activities" },
    { Aggregate::PHASES,         "phases" },
    { Aggregate::ENTRIES,        "entries" },
    { Aggregate::THREADS,        LQIO::DOM::Pragma::_threads_ }
};

const std::map<const Colouring, const std::string> Options::colouring =
{
    { Colouring::NONE,		LQIO::DOM::Pragma::_none_ },
    { Colouring::RESULTS,    	"results" },
    { Colouring::LAYERS,     	"layers" },
    { Colouring::CLIENTS,    	"clients" },
    { Colouring::SERVER_TYPE,	"type" },
    { Colouring::CHAINS,     	"chains" },
    { Colouring::DIFFERENCES,	"differences" },
};

/* Discriminators for union */

const std::string Options::boolean  = "bool";
const std::string Options::integer  = "int";
const std::string Options::real     = "float";
const std::string Options::none     = "none";
const std::string Options::result   = "result";
/*
 * Input output format options
 */

const std::map<const File_Format,const std::string> Options::file_format =
{
    { File_Format::EEPIC,	"eepic" },
#if EMF_OUTPUT
    { File_Format::EMF,		"emf" },
#endif
    { File_Format::FIG,		"fig" },
#if HAVE_GD_H && HAVE_LIBGD && HAVE_GDIMAGEGIFPTR
    { File_Format::GIF,		"gif" },
#endif
#if JMVA_OUTPUT && HAVE_LIBEXPAT
    { File_Format::JMVA,	"jmva" },
#endif
#if HAVE_GD_H && HAVE_LIBGD && HAVE_LIBJPEG
    { File_Format::JPEG,	"jpeg" },
#endif
    { File_Format::JSON,	"json" },
    { File_Format::LQX,		"lqx" },
    { File_Format::NO_OUTPUT,	"null" },
    { File_Format::OUTPUT,	"out" },
#if QNAP2_OUTPUT
    { File_Format::QNAP2,	"qnap2" },
#endif
    { File_Format::PARSEABLE,	"parseable" },
#if HAVE_GD_H && HAVE_LIBGD && HAVE_LIBPNG
    { File_Format::PNG,		"png" },
#endif
    { File_Format::POSTSCRIPT,	"ps" },
    { File_Format::PSTEX,	"pstex" },
    { File_Format::RTF,		"rtf" },
    { File_Format::SRVN,	"lqn" },
#if SVG_OUTPUT
    { File_Format::SVG,		"svg" },
#endif
#if SXD_OUTPUT
    { File_Format::SXD,		"sxd" },
#endif
#if TXT_OUTPUT
    { File_Format::TXT,		"txt" },
#endif
#if X11_OUTPUT
    { File_Format::X11,		"x11" },
#endif
    { File_Format::XML,		"xml" }
};

const std::map<const Justification, const std::string> Options::justification =
{
    { Justification::DEFAULT,	LQIO::DOM::Pragma::_default_ },
    { Justification::CENTER,	"center" },
    { Justification::LEFT,	"left" },
    { Justification::RIGHT,	"right" },
    { Justification::ALIGN,	"align" },	/* For Nodes		*/
    { Justification::ABOVE,	"above" }	/* For labels on Arcs.	*/
};

const std::multimap<const Key_Position, const std::string> Options::key_position =
{
    { Key_Position::NONE,               LQIO::DOM::Pragma::_none_ },
    { Key_Position::TOP_LEFT,           "top-left" },
    { Key_Position::TOP_RIGHT,          "top-right" },
    { Key_Position::BOTTOM_LEFT,        "bottom-left" },
    { Key_Position::BOTTOM_RIGHT,	"bottom-right" },
    { Key_Position::BELOW_LEFT,         "below-left" },
    { Key_Position::ABOVE_LEFT,         "above-left" },
    { Key_Position::TOP_LEFT,           "on" }			/* alias */
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

const std::map<const Processors, const std::string> Options::processors = {
    { Processors::NONE,         LQIO::DOM::Pragma::_none_ },
    { Processors::QUEUEABLE,    "queueable" },
    { Processors::NONINFINITE,	"non-infinite" },
    { Processors::ALL,          LQIO::DOM::Pragma::_all_ }
};

const std::map<const Replication, const std::string> Options::replication =
{
    { Replication::NONE,    	LQIO::DOM::Pragma::_none_ },
    { Replication::REMOVE,    	"remove" },
    { Replication::EXPAND,    	"expand" },
    { Replication::RETURN,    	"return" }
};

const std::map<const Special, const std::string> Options::special = {
    { Special::ANNOTATE,                    "annotate" },
    { Special::ARROW_SCALING,		    "arrow-scaling" },
    { Special::BCMP,			    LQIO::DOM::Pragma::_bcmp_ },
    { Special::CLEAR_LABEL_BACKGROUND,	    "clear-label-background" },
    { Special::EXHAUSTIVE_TOPOLOGICAL_SORT, "exhaustive-topological-sort" },
    { Special::FLATTEN_SUBMODEL,	    "flatten" },
    { Special::FORCE_INFINITE,		    LQIO::DOM::Pragma::_force_infinite_ },
    { Special::FORWARDING_DEPTH,	    "forwarding" },
    { Special::GROUP,			    "group" },
    { Special::LAYER_NUMBER,		    "layer-number" },
    { Special::NO_ALIGNMENT_BOX,	    "no-alignment-box" },
    { Special::NO_ASYNC_TOPOLOGICAL_SORT,   "no-async" },
    { Special::NO_CV_SQR,		    "no-cv-sqr" },
    { Special::NO_PHASE_TYPE,		    "no-phase-type" },
    { Special::NO_REF_TASK_CONVERSION,	    "no-reference-task-conversion" },
    { Special::PRUNE,			    "prune" },
    { Special::PROCESSOR_SCHEDULING,	    LQIO::DOM::Pragma::_processor_scheduling_ },
    { Special::QUORUM_REPLY,		    "quorum-reply" },
    { Special::SORT,			    "sort" },
    { Special::SQUISH_ENTRY_NAMES,	    "squish" },
    { Special::SPEX_HEADER,		    "no-header" },
    { Special::SUBMODEL_CONTENTS,	    "submodels" },
    { Special::TASKS_ONLY,		    "tasks-only" },
    { Special::TASK_SCHEDULING,		    LQIO::DOM::Pragma::_task_scheduling_ }
};

const std::map<const Sorting,const std::string> Options::sorting=
{
    { Sorting::FORWARD,        "ascending" },
    { Sorting::REVERSE,        "descending" },
    { Sorting::TOPILOGICAL,    "topological" },
    { Sorting::NONE,           LQIO::DOM::Pragma::_none_ }
};

const std::string Options::string = "string";


static bool get_bool( const std::string&, bool default_value );

Aggregate
Options::get_aggregate( const std::string& value )
{
    for ( std::map<const Aggregate,const std::string>::const_iterator i = Options::aggregate.begin(); i != Options::aggregate.end(); ++i ) {
	if ( value == i->second ) return i->first;
    }
    throw std::invalid_argument( value );
}

Colouring
Options::get_colouring( const std::string& value )
{
    for ( std::map<const Colouring,const std::string>::const_iterator i = Options::colouring.begin(); i != Options::colouring.end(); ++i ) {
	if ( value == i->second ) return i->first;
    }
    throw std::invalid_argument( value );
}

File_Format
Options::get_file_format( const std::string& value )
{
    for ( std::map<const File_Format,const std::string>::const_iterator i = Options::file_format.begin(); i != Options::file_format.end(); ++i ) {
	if ( value == i->second ) return i->first;
    }
    throw std::invalid_argument( value );
}


/*
 * Get the layering method.  If there is an argument, split the comma-separated string into a vector of strings
 */

Layering
Options::get_layering( const std::string& str, std::vector<std::string>& values )
{
    /* Split str at '=' and stop at either null or ',' */

    const std::size_t pos = str.find( '=' );
    const std::string option = str.substr( 0, pos );

    for ( std::map<const Layering,const std::string>::const_iterator i = Options::layering.begin(); i != Options::layering.end(); ++i ) {
	if ( option == i->second ) {
	    if ( pos != std::string::npos ) {
		const std::string rest = str.substr( pos + 1 );
		const std::regex regexz("=");
		values.assign( std::sregex_token_iterator(rest.begin(), rest.end(), regexz, -1), std::sregex_token_iterator() );
	    }
	    return i->first;
	}
    }
    throw std::invalid_argument( option );
}


Key_Position
Options::get_key_position( const std::string& value )
{
    for ( std::map<const Key_Position,const std::string>::const_iterator i = Options::key_position.begin(); i != Options::key_position.end(); ++i ) {
	if ( value == i->second ) return i->first;
    }
    throw std::invalid_argument( value );
}


Justification
Options::get_justification( const std::string& value )
{
    for ( std::map<const Justification,const std::string>::const_iterator i = Options::justification.begin(); i != Options::justification.end(); ++i ) {
	if ( value == i->second ) return i->first;
    }
    throw std::invalid_argument( value );
}


Processors
Options::get_processors( const std::string& value )
{
    for ( std::map<const Processors,const std::string>::const_iterator i = Options::processors.begin(); i != Options::processors.end(); ++i ) {
	if ( value == i->second ) return i->first;
    }
    throw std::invalid_argument( value );
}


Replication
Options::get_replication( const::std::string& value )
{
    for ( std::map<const Replication,const std::string>::const_iterator i = Options::replication.begin(); i != Options::replication.end(); ++i ) {
	if ( value == i->second ) return i->first;
    }
    throw std::invalid_argument( value );
}


Sorting
Options::get_sorting( const::std::string& value )
{
    for ( std::multimap<const Sorting,const std::string>::const_iterator i = Options::sorting.begin(); i != Options::sorting.end(); ++i ) {
	if ( value == i->second ) return i->first;
    }
    throw std::invalid_argument( value );
}


Special
Options::get_special( const::std::string& value )
{
    for ( std::map<const Special,const std::string>::const_iterator i = Options::special.begin(); i != Options::special.end(); ++i ) {
	if ( value == i->second ) return i->first;
    }
    throw std::invalid_argument( value );
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

	switch ( Options::get_special( parameter ) ) {
	case Special::ANNOTATE:			    Flags::annotate_input		    = get_bool( value, true ); break;
	case Special::CLEAR_LABEL_BACKGROUND:	    Flags::clear_label_background	    = get_bool( value, true ); break;
	case Special::EXHAUSTIVE_TOPOLOGICAL_SORT:  Flags::exhaustive_toplogical_sort	    = get_bool( value, true ); break;
	case Special::FLATTEN_SUBMODEL:		    Flags::flatten_submodel		    = get_bool( value, true ); break;
	case Special::FORWARDING_DEPTH:		    Flags::print_forwarding_by_depth	    = get_bool( value, true ); break;
	case Special::LAYER_NUMBER:		    Flags::print_layer_number		    = get_bool( value, true ); break;
	case Special::NO_ALIGNMENT_BOX:		    Flags::print_alignment_box		    = get_bool( value, false ); break;
	case Special::NO_ASYNC_TOPOLOGICAL_SORT:    Flags::async_topological_sort	    = get_bool( value, false ); break;
	case Special::NO_CV_SQR:		    Flags::output_coefficient_of_variation  = get_bool( value, false ); break;
	case Special::NO_PHASE_TYPE:		    Flags::output_phase_type		    = get_bool( value, false ); break;
	case Special::NO_REF_TASK_CONVERSION:	    Flags::convert_to_reference_task	    = get_bool( value, false ); break;
	case Special::RENAME:			    Flags::rename_model			    = get_bool( value, true ); break;
	case Special::SQUISH_ENTRY_NAMES:	    Flags::squish_names			    = get_bool( value, true ); break;
	case Special::SUBMODEL_CONTENTS:	    Flags::print_submodels		    = get_bool( value, true ); break;

	case Special::BCMP:
	    pragmas.insert(LQIO::DOM::Pragma::_bcmp_, value );
	    break;

	case Special::FORCE_INFINITE:
	    pragmas.insert(LQIO::DOM::Pragma::_force_infinite_, value );
	    break;

	case Special::PROCESSOR_SCHEDULING:
	    pragmas.insert(LQIO::DOM::Pragma::_processor_scheduling_, value );
	    break;

	case Special::PRUNE:
	    pragmas.insert(LQIO::DOM::Pragma::_prune_, value );
	    break;

	case Special::QUORUM_REPLY:
	    LQIO::error_messages.at(LQIO::ERR_REPLY_NOT_GENERATED).severity = LQIO::error_severity::WARNING;
	    break;

	case Special::SORT:
	    Flags::sort = Options::get_sorting( value );
	    break;

	case Special::TASK_SCHEDULING:
	    pragmas.insert(LQIO::DOM::Pragma::_task_scheduling_, value );
	    break;

	case Special::TASKS_ONLY:
	    Flags::set_aggregation( Aggregate::ENTRIES );
	    if ( Flags::icon_height == DEFAULT_ICON_HEIGHT ) {
		if ( processor_output() || share_output() ) {
		    Flags::set_y_spacing(45);
		} else {
		    Flags::set_y_spacing(27);
		}
		Flags::icon_height = 18;
		Flags::entry_height = Flags::icon_height * 0.6;
	    }
	    break;

	case Special::ARROW_SCALING:
	    Flags::arrow_scaling = strtod( value.c_str(), &endptr );
	    if ( Flags::arrow_scaling <= 0 || *endptr != '\0' ) throw std::domain_error( value );
	    break;

	case Special::GROUP:
	    Model::add_group( value.c_str() );
	    break;

	case Special::NONE:
	    throw std::invalid_argument( parameter );
	}
    }
    catch ( const std::invalid_argument& e ) {
	std::cerr << LQIO::io_vars.lq_toolname << ": Unknown argument: \"" << e.what();
	if ( value.size() ) {
	    std::cerr << "=" << value;
	}
	std::cerr << "\"" << std::endl;
	return false;
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
 * This function is used to set all of the output result options to
 * either true or false.
 */

bool
Options::set_all_result_options( const bool yesOrNo )
{
    for ( unsigned i = 0; i < SERVICE_EXCEEDED; ++i ) {
        if ( Flags::print[i].opts.param.s != &Options::result ) continue;	/* Skip non-result */
	Flags::print[i].opts.value.b = yesOrNo;
    }

    return yesOrNo;
}

/*
 * Return true if we are generating graphical output of some form.
 */

bool
graphical_output()
{
    static const std::set<File_Format> reject = {
#if JMVA_OUTPUT && HAVE_LIBEXPAT
	File_Format::JMVA,
#endif
	File_Format::JSON,
	File_Format::LQX,
	File_Format::NO_OUTPUT,
	File_Format::OUTPUT,
	File_Format::PARSEABLE,
#if QNAP2_OUTPUT
	File_Format::QNAP2,
#endif
	File_Format::RTF,
	File_Format::SRVN,
#if TXT_OUTPUT
	File_Format::TXT,
#endif
	File_Format::XML
    };

    return std::find( reject.begin(), reject.end(), Flags::output_format() ) == reject.end();
}


/*
 * Return true if we are generating a new resutls file of some form.
 */

bool
output_output()
{
    return Flags::output_format() == File_Format::OUTPUT
	|| Flags::output_format() == File_Format::PARSEABLE
	|| Flags::output_format() == File_Format::RTF;
}


/*
 * Return true if we are generating a new input file of some form.
 */

bool
input_output()
{
    return Flags::output_format() == File_Format::SRVN
	|| Flags::output_format() == File_Format::JSON
	|| Flags::output_format() == File_Format::LQX
	|| Flags::output_format() == File_Format::XML
	;
}


/*
 * Return true if we are generating a new input file of some form.
 */

bool
bcmp_output()
{
    return Flags::output_format() == File_Format::QNAP2
#if JMVA_OUTPUT && HAVE_LIBEXPAT
	|| Flags::output_format() == File_Format::JMVA
#endif
	;
}


/*
 * Return true if we're only printing partial results.
 */

bool
partial_output()
{
    return submodel_output() || queueing_output()
	|| Flags::include_only() != nullptr;
}

bool
processor_output()
{
    return Flags::layering() == Layering::PROCESSOR
	|| Flags::layering() == Layering::PROCESSOR_TASK
	|| Flags::layering() == Layering::TASK_PROCESSOR;
}

bool
queueing_output()
{
    return Flags::queueing_model() != 0;
}


bool
share_output()
{
    return Flags::layering() == Layering::SHARE;
}

bool
submodel_output()
{
    return Flags::submodel() != 0;
}

bool
difference_output()
{
    return Flags::colouring() == Colouring::DIFFERENCES;
}
