/*  -*- c++ -*-
 * $Id: model.cc 15304 2021-12-31 15:51:38Z greg $
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

#include <algorithm>
#include <limits>
#include <numeric>
#include <regex>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <lqio/dom_activity.h>
#include <lqio/dom_document.h>
#include <lqio/dom_entry.h>
#include <lqio/dom_object.h>
#include <lqio/dom_processor.h>
#include <lqio/dom_task.h>
#include "model.h"
#include "lqn2csv.h"

/*
 * Maps result type to the dom-object member function that gets the
 * results.  All of the getResult functions take no arguments, so they
 * must be used on the final object (phases, and not entries, for
 * phase results, etc).
 */

const std::map<Model::Result::Type,Model::Result::result_fields> Model::Result::__results =
{
    { Model::Result::Type::ACTIVITY_DEMAND,            { Model::Object::Type::ACTIVITY,      "Demand",      &LQIO::DOM::DocumentObject::getServiceTimeValue          } },
    { Model::Result::Type::ACTIVITY_PROCESSOR_WAITING, { Model::Object::Type::ACTIVITY,      "Waiting",     &LQIO::DOM::DocumentObject::getResultProcessorWaiting    } },
    { Model::Result::Type::ACTIVITY_PR_RQST_LOST,      { Model::Object::Type::ACTIVITY_CALL, "Drop Prob",   &LQIO::DOM::DocumentObject::getResultDropProbability     } },
    { Model::Result::Type::ACTIVITY_PR_SVC_EXCD,       { Model::Object::Type::ACTIVITY,      "Pr. Exceed",  &LQIO::DOM::DocumentObject::getResultMaxServiceTimeExceeded } },
    { Model::Result::Type::ACTIVITY_REQUEST_RATE,      { Model::Object::Type::ACTIVITY_CALL, "Request",     &LQIO::DOM::DocumentObject::getCallMeanValue             } },
    { Model::Result::Type::ACTIVITY_SERVICE,           { Model::Object::Type::ACTIVITY,      "Service",     &LQIO::DOM::DocumentObject::getResultServiceTime         } },
    { Model::Result::Type::ACTIVITY_THROUGHPUT,        { Model::Object::Type::ACTIVITY,      "Throughput",  &LQIO::DOM::DocumentObject::getResultThroughput          } },
    { Model::Result::Type::ACTIVITY_VARIANCE,          { Model::Object::Type::ACTIVITY,      "Variance",    &LQIO::DOM::DocumentObject::getResultServiceTimeVariance } },
    { Model::Result::Type::ACTIVITY_WAITING,           { Model::Object::Type::ACTIVITY_CALL, "Waiting",     &LQIO::DOM::DocumentObject::getResultWaitingTime         } },
    { Model::Result::Type::ENTRY_THROUGHPUT,           { Model::Object::Type::ENTRY,         "Throughput",  &LQIO::DOM::DocumentObject::getResultThroughput          } },
    { Model::Result::Type::ENTRY_UTILIZATION,          { Model::Object::Type::ENTRY,         "Utilization", &LQIO::DOM::DocumentObject::getResultUtilization         } },
    { Model::Result::Type::HOLD_TIMES,                 { Model::Object::Type::JOIN,          "Hold Time",   &LQIO::DOM::DocumentObject::getResultHoldingTime         } },
    { Model::Result::Type::JOIN_DELAYS,                { Model::Object::Type::JOIN,          "Join Delay",  &LQIO::DOM::DocumentObject::getResultJoinDelay           } },
    { Model::Result::Type::OPEN_WAIT,                  { Model::Object::Type::ENTRY,         "Waiting",     &LQIO::DOM::DocumentObject::getResultWaitingTime         } },
    { Model::Result::Type::PHASE_DEMAND,               { Model::Object::Type::PHASE,         "Demand",      &LQIO::DOM::DocumentObject::getServiceTimeValue          } },
    { Model::Result::Type::PHASE_PROCESSOR_WAITING,    { Model::Object::Type::PHASE,         "Waiting",     &LQIO::DOM::DocumentObject::getResultProcessorWaiting    } },
    { Model::Result::Type::PHASE_PR_RQST_LOST,         { Model::Object::Type::PHASE_CALL,    "Drop Prob",   &LQIO::DOM::DocumentObject::getResultDropProbability     } },
    { Model::Result::Type::PHASE_PR_SVC_EXCD,          { Model::Object::Type::PHASE,         "Pr. Exceed",  &LQIO::DOM::DocumentObject::getResultMaxServiceTimeExceeded } },
    { Model::Result::Type::PHASE_REQUEST_RATE,         { Model::Object::Type::PHASE_CALL,    "Request",     &LQIO::DOM::DocumentObject::getCallMeanValue             } },
    { Model::Result::Type::PHASE_SERVICE,              { Model::Object::Type::PHASE,         "Service",     &LQIO::DOM::DocumentObject::getResultServiceTime         } },
    { Model::Result::Type::PHASE_VARIANCE,             { Model::Object::Type::PHASE,         "Variance",    &LQIO::DOM::DocumentObject::getResultVarianceServiceTime } },
    { Model::Result::Type::PHASE_WAITING,              { Model::Object::Type::PHASE_CALL,    "Waiting",     &LQIO::DOM::DocumentObject::getResultWaitingTime         } },
    { Model::Result::Type::PROCESSOR_MULTIPLICITY,     { Model::Object::Type::PROCESSOR,     "Copies",      &LQIO::DOM::DocumentObject::getCopiesValueAsDouble       } },
    { Model::Result::Type::PROCESSOR_UTILIZATION,      { Model::Object::Type::PROCESSOR,     "Utilization", &LQIO::DOM::DocumentObject::getResultUtilization         } },
    { Model::Result::Type::TASK_MULTIPLICITY,          { Model::Object::Type::TASK,          "Copies",      &LQIO::DOM::DocumentObject::getCopiesValueAsDouble       } },
    { Model::Result::Type::TASK_THROUGHPUT,            { Model::Object::Type::TASK,          "Throughput",  &LQIO::DOM::DocumentObject::getResultThroughput          } },
    { Model::Result::Type::TASK_UTILIZATION,           { Model::Object::Type::TASK,          "Utilization", &LQIO::DOM::DocumentObject::getResultUtilization         } },
    { Model::Result::Type::THROUGHPUT_BOUND,           { Model::Object::Type::ENTRY,         "Bound",       &LQIO::DOM::DocumentObject::getResultThroughputBound     } }
};

const std::map<Model::Object::Type, const std::string> Model::Object::__object_type =
{
    { Model::Object::Type::ACTIVITY,  "Activity"  },
    { Model::Object::Type::ENTRY,     "Entry"     },
    { Model::Object::Type::JOIN,      "Join"      },
    { Model::Object::Type::PHASE,     "Phase"     },
    { Model::Object::Type::PROCESSOR, "Processor" },
    { Model::Object::Type::TASK,      "Task"      }
};

bool Model::Result::isIndependentVariable( Model::Result::Type type )
{
    static const std::set<Result::Type> independent = {
	Type::PHASE_DEMAND,
	Type::PHASE_REQUEST_RATE,
	Type::ACTIVITY_DEMAND,
	Type::ACTIVITY_REQUEST_RATE,
	Type::TASK_MULTIPLICITY,
	Type::PROCESSOR_MULTIPLICITY };

    return independent.find( type ) != independent.end();
}

bool Model::Result::isDependentVariable( Model::Result::Type type )
{
    return type != Type::NONE && !isIndependentVariable( type );
}

/*
 * Load a file then extract results using Model::Result::operator().
 */

void Model::Process::operator()( const char * filename )
{
    /* Load results */
    unsigned int error_code = 0;
    LQIO::DOM::Document * dom = LQIO::DOM::Document::load( filename, LQIO::DOM::Document::InputFormat::AUTOMATIC, error_code, true );
    if ( !dom ) {
	throw std::runtime_error( "Input model was not loaded successfully." );
    }

    /* Extract into vector of doubles */

    _i += 1;
    std::vector<double> data;
    data.push_back( static_cast<double>(_i) );
    data = std::accumulate( _results.begin(), _results.end(), data, Model::Result( *dom ) );

    /* We now have a vector of doubles */
    /* if splot, and data.x2 changes, then output newline if not first record */

    if ( x_index() != 0 && x_value() != data.at( x_index() ) ) {
	set_x_value( data.at( x_index() ) );
	if ( _i != 1 ) {
	    _output << std::endl;
	}
    }

    std::for_each( data.begin(), data.end(), Model::Print( _output ) );
    _output << std::endl;
}

/* Output an item, separated by commas. */

void
Model::Print::operator()( double item )
{
    if ( _first ) {
	_first = false;
    } else {
	_output << ",";
    }

    if ( item != std::numeric_limits<double>::quiet_NaN() ) {
	_output << item;
    } else {
	_output << "NULL";
    }
}

/*
 * Get the value of the result and append it to the string "in" (fold operation for std::accumulate).
 */

std::vector<double>
Model::Result::operator()( const std::vector<double>& in, const std::pair<std::string,Model::Result::Type>& arg ) const
{
    const Model::Result::result_fields& result = __results.at( arg.second );
    const LQIO::DOM::DocumentObject * object = findObject( arg.first, result.type );
    std::vector<double> out = in;

    if ( object != nullptr ) {
	out.push_back( (object->*(result.f))() );		/* Invoke function to find value	*/
    } else {
	out.push_back( std::numeric_limits<double>::quiet_NaN() );
    }

    return out;
}


/*
 * Find the object.  Some may be more complicated than others (i.e., phases).
 */

const LQIO::DOM::DocumentObject *
Model::Result::findObject( const std::string& arg, Model::Object::Type type ) const
{
    const std::regex regex(",");
    const std::vector<std::string> tokens( std::sregex_token_iterator(arg.begin(), arg.end(), regex, -1), std::sregex_token_iterator() );
    if ( tokens.empty() ) return nullptr;

    const LQIO::DOM::DocumentObject * object = nullptr;
    const std::string& name = tokens.front();

    switch ( type ) {
    case Object::Type::ACTIVITY:	/* task, activity */
	if ( tokens.size() == 2 ) {
	    object = findActivity( dom().getTaskByName( name ), tokens.at(1) );
	}
	break;

    case Object::Type::ACTIVITY_CALL:	/* task, activity, entry */
	if ( tokens.size() == 3 ) {
	    object = findCall( findActivity( dom().getTaskByName( name ), tokens.at(1) ), tokens.at(2) );
	}
	break;

    case Object::Type::ENTRY:		/* entry */
	object = dom().getEntryByName( name );
	break;

    case Object::Type::PHASE:		/* entry, phase */
	if ( tokens.size() == 2 ) {
	    object = findPhase( dom().getEntryByName( name ), tokens.at(1) );
	}
	break;

    case Object::Type::PHASE_CALL:	/* entry, phase -> entry */
	if ( tokens.size() == 3 ) {
	    object = findCall( findPhase( dom().getEntryByName( name ), tokens.at(1) ), tokens.at(2) );
	}
	break;

    case Object::Type::TASK:		/* task */
	object = dom().getTaskByName( name );
	break;

    case Object::Type::PROCESSOR:	/* processor */
	object = dom().getProcessorByName( name );
	break;

//    case result::Type::HOLD_TIMES:
//    case Result::Type::JOIN_DELAYS:
	/* Who knows! */
//	break;

    default:
	abort();
	break;
    }

    return object;
}



/*
 * Return the phase object for the entry.
 */

const LQIO::DOM::Phase *
Model::Result::findPhase( const LQIO::DOM::Entry * entry, const std::string& phase ) const
{
    if ( entry == nullptr ) return nullptr;
    try {
	std::string::size_type i = 0;
	const unsigned int p = std::stoi( phase, &i );
	if ( 1 <= p && p <= 3 && i == phase.size() ) return entry->getPhase( p );
    }
    catch ( const std::invalid_argument& )
    {
    }
    return nullptr;	/* Error */
}


/*
 * Return the call object for the phase.
 */

const LQIO::DOM::Call *
Model::Result::findCall( const LQIO::DOM::Phase * phase, const std::string& destination ) const
{
    if ( phase == nullptr ) return nullptr;
    LQIO::DOM::Entry * target = dom().getEntryByName( destination );
    if ( target == nullptr ) return nullptr;
    return phase->getCallToTarget( target );
}


/*
 * Return the activity object for the task.
 */

const LQIO::DOM::Activity *
Model::Result::findActivity( const LQIO::DOM::Task * task, const std::string& activity ) const
{
    if ( task == nullptr ) return nullptr;
    return task->getActivity( activity );
}



std::string
Model::Result::ObjectType( const std::string& in, const std::pair<std::string,Model::Result::Type>& result )
{
    std::string out = in;
    if ( !in.empty() ) out += ",";
    return out + "\"" + Model::Object::__object_type.at(__results.at(result.second).type) + "\"";
}



std::string
Model::Result::ObjectName( const std::string& in, const std::pair<std::string,Model::Result::Type>& result )
{
    std::string out = in;
    if ( !in.empty() ) out += ",";
    return out + "\"" + result.first + "\"";
}



std::string
Model::Result::TypeName( const std::string& in, const std::pair<std::string,Model::Result::Type>& result )
{
    std::string out = in;
    if ( !in.empty() ) out += ",";
    return out + "\"" + __results.at(result.second).name + "\"";
}



bool
Model::Result::equal( Model::Result::Type type_1, Model::Result::Type type_2 )
{
    return (throughput( type_1 ) && throughput( type_2 ))
	|| (utilization( type_1 ) && utilization( type_2 ))
	|| (service_time( type_1 ) && service_time( type_2 ))
	|| (variance( type_1 ) && variance( type_2 ))
	|| (waiting( type_1 ) && waiting( type_2 ));
}
