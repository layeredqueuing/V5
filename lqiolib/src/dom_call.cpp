/*
 *  $Id: dom_call.cpp 15698 2022-06-23 11:44:22Z greg $
 *
 *  Created by Martin Mroz on 24/02/09.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#include <cassert>
#include "dom_activity.h"
#include "dom_document.h"
#include "dom_entry.h"
#include "dom_histogram.h"
#include "dom_task.h"
#include "glblerr.h"

namespace LQIO {
    namespace DOM {
    
	const char * Call::__typeName = "call";

	Call::Call(const Document * document, const Type type, Phase* source, Entry* destination, const ExternalVariable* callMean ) :
	    DocumentObject(document,""),
	    _callType(type), _sourceObject(source), _destinationEntry(destination), 
	    _callMean(callMean), _histogram(nullptr),
	    _hasResultVarianceWaitingTime(false), _hasResultDropProbability(false),
	    _resultWaitingTime(0.0), _resultWaitingTimeVariance(0.0),
	    _resultVarianceWaitingTime(0.0), _resultVarianceWaitingTimeVariance(0.0),
	    _resultDropProbability(0.0), _resultDropProbabilityVariance(0.0)
	{
	}
        
	/* Special case for forwarding */
	Call::Call(const Document * document, Entry* source, Entry* destination, const ExternalVariable* callMean ) :
	    DocumentObject(document,""),
	    _callType(Call::Type::FORWARD), _sourceObject(source), _destinationEntry(destination), 
	    _callMean(callMean), _histogram(nullptr),
	    _hasResultVarianceWaitingTime(false),
	    _resultWaitingTime(0.0), _resultWaitingTimeVariance(0.0),
	    _resultVarianceWaitingTime(0.0), _resultVarianceWaitingTimeVariance(0.0),
	    _resultDropProbability(0.0), _resultDropProbabilityVariance(0.0)
	{
	}
        
	Call::Call(const Call& src) :
	    DocumentObject(src.getDocument(),""),
	    _callType(src._callType), _sourceObject(nullptr), _destinationEntry(nullptr), 
	    _callMean(ExternalVariable::clone(src._callMean)), _histogram(src._histogram),
	    _hasResultVarianceWaitingTime(false),
	    _resultWaitingTime(0.0), _resultWaitingTimeVariance(0.0),
	    _resultVarianceWaitingTime(0.0), _resultVarianceWaitingTimeVariance(0.0),
	    _resultDropProbability(0.0), _resultDropProbabilityVariance(0.0)
	{
	}


	Call::~Call()
	{
	}
    
	Call * Call::clone() const
	{
	    return new Call( *this );
	}

	/*
	 * Calls can go from phases, activities or entries, so generate the correct message.
	 */
	
	void Call::throw_invalid_parameter( const std::string& parameter, const std::string& error ) const
	{
	    const DocumentObject * src = getSourceObject();
	    const DocumentObject * dst = getDestinationEntry();
	    if ( dynamic_cast<const Entry *>(src) ) {
		solution_error2( LQIO::ERR_INVALID_FWDING_PARAMETER, getLineNumber(), getTypeName(), dst->getName().c_str(), error.c_str() );
	    } else {
		const DocumentObject * owner = nullptr;
		if ( dynamic_cast<const Activity *>(src) != nullptr ) {
		    owner = dynamic_cast<const Activity *>(src)->getTask();
		} else if ( dynamic_cast<const Phase *>(src) != nullptr ) {
		    owner = dynamic_cast<const Phase *>(src)->getSourceEntry();
		} else {
		    abort();
		}
		solution_error2( LQIO::ERR_INVALID_CALL_PARAMETER, getLineNumber(),
				 owner->getTypeName(), owner->getName().c_str(),
				 src->getTypeName(), src->getName().c_str(),
				 dst->getName().c_str(), error.c_str() );
	    }
	    throw std::domain_error( std::string( "invalid parameter: " ) + error );
	}
	
	/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- [Input Values] -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
    
	const Call::Type Call::getCallType() const
	{
	    /* Returns the CallType of the Call */
	    return _callType;
	}

	void Call::setCallType(const Call::Type callType)
	{
	    /* Stores the given CallType of the Call */ 
	    _callType = callType;
	}
    
	const DocumentObject* Call::getSourceObject() const
	{
	    return _sourceObject;
	}
    
	void Call::setSourceObject( DocumentObject* source ) 
	{
	    _sourceObject = source;
	}
    
	const Entry* Call::getDestinationEntry() const
	{
	    /* Return the destination DOM::Entry */
	    return _destinationEntry;
	}
    
	void Call::setDestinationEntry( Entry * destination )
	{
	    _destinationEntry = destination;
	}

	const ExternalVariable* Call::getCallMean() const
	{
	    return _callMean;
	}
    
	void Call::setCallMean(const ExternalVariable* callMean)
	{
	    if ( _callMean != nullptr ) delete _callMean;
	    _callMean = checkDoubleVariable( callMean, 0.0 );
	}
    
	double Call::getCallMeanValue() const
	{
	    return getDoubleValue( getCallMean(), 0.0 );
	}

	void Call::setCallMeanValue(double value)
	{
	    if ( _callMean == nullptr ) {
		_callMean = new ConstantExternalVariable( value );
	    } else {
		const_cast<ExternalVariable *>(_callMean)->set(value);
	    }
	}
    
	bool Call::hasHistogram() const
	{
	    return _histogram != nullptr && _histogram->getBins() > 0; 
	}
    
	void Call::setHistogram(Histogram* histogram)
	{
	    /* Stores the given Histogram of the Phase */
	    _histogram = histogram;
	}

	/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- [Result Values] -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */
    
	double Call::getResultWaitingTime() const
	{
	    /* Returns the ResultWaitingTime of the Call */
	    return _resultWaitingTime;
	}

	Call& Call::setResultWaitingTime(const double resultWaitingTime)
	{
	    /* Stores the given ResultWaitingTime of the Call */ 
	    _resultWaitingTime = resultWaitingTime;
	    return *this;
	}
    
	double Call::getResultWaitingTimeVariance() const
	{
	    /* Reurnts the given ResultWaitingTime of the Call */ 
	    return _resultWaitingTimeVariance;
	}
    
	Call& Call::setResultWaitingTimeVariance(const double resultWaitingTimeVariance)
	{
	    /* Stores the given ResultWaitingTime of the Call */ 
	    if ( resultWaitingTimeVariance > 0 ) {
		const_cast<Document *>(getDocument())->setResultHasConfidenceIntervals(true);
	    }
	    _resultWaitingTimeVariance = resultWaitingTimeVariance;
	    return *this;
	}
    
	double Call::getResultVarianceWaitingTime() const
	{
	    /* Returns the ResultWaitingTime of the Call */
	    return _resultVarianceWaitingTime;
	}

	Call& Call::setResultVarianceWaitingTime(const double resultVarianceWaitingTime)
	{
	    /* Stores the given ResultWaitingTime of the Call */ 
	    _hasResultVarianceWaitingTime = true;
	    _resultVarianceWaitingTime = resultVarianceWaitingTime;
	    return *this;
	}

	double Call::getResultVarianceWaitingTimeVariance() const
	{
	    /* Stores the given ResultWaitingTime of the Call */ 
	    return _resultVarianceWaitingTimeVariance;
	}
    
	Call& Call::setResultVarianceWaitingTimeVariance(const double resultVarianceWaitingTimeVariance)
	{
	    /* Stores the given ResultWaitingTime of the Call */ 
	    if ( resultVarianceWaitingTimeVariance > 0 ) {
		const_cast<Document *>(getDocument())->setResultHasConfidenceIntervals(true);
	    }
	    _resultVarianceWaitingTimeVariance = resultVarianceWaitingTimeVariance;
	    return *this;
	}
    
	double 
	Call::getResultDropProbability() const
	{ 
	    return _resultDropProbability;
	}

	Call& 
	Call::setResultDropProbability( const double resultDropProbability )
	{
	    _hasResultDropProbability = true;
	    _resultDropProbability = resultDropProbability;
	    return *this;
	}

	double 
	Call::getResultDropProbabilityVariance() const
	{
	    return _resultDropProbabilityVariance;
	}

	Call& 
	Call::setResultDropProbabilityVariance( const double resultDropProbabilityVariance )
	{
	    _resultDropProbabilityVariance = resultDropProbabilityVariance;
	    return *this;
	}
    }
}
