/*
 *  $Id$
 *
 *  Created by Martin Mroz on 24/02/09.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#include "dom_document.h"
#include "dom_histogram.h"
#include <cassert>

namespace LQIO {
    namespace DOM {
    
	Call::Call(const Document * document, const CallType type, Phase* source, Entry* destination, 
		   ExternalVariable* callMean, const void * element ) :
	    DocumentObject(document,"",element),
	    _callType(type), _destinationEntry(destination), 
	    _callMean(callMean), _histogram(0),
	    _hasResultVarianceWaitingTime(false), _hasResultDropProbability(false),
	    _resultWaitingTime(0.0), _resultWaitingTimeVariance(0.0),
	    _resultVarianceWaitingTime(0.0), _resultVarianceWaitingTimeVariance(0.0),
	    _resultDropProbability(0.0), _resultDropProbabilityVariance(0.0)
	{
	    const_cast<Document *>(document)->setCallType(type);
	    _source._phase = source;
	}
        
	/* Special case for forwarding */
	Call::Call(const Document * document, Entry* source, Entry* destination, ExternalVariable* callMean, const void * element ) :
	    DocumentObject(document,"",element),
	    _callType(Call::FORWARD), _destinationEntry(destination), 
	    _callMean(callMean), _histogram(0),
	    _hasResultVarianceWaitingTime(false),
	    _resultWaitingTime(0.0), _resultWaitingTimeVariance(0.0),
	    _resultVarianceWaitingTime(0.0), _resultVarianceWaitingTimeVariance(0.0),
	    _resultDropProbability(0.0), _resultDropProbabilityVariance(0.0)
	{
	    const_cast<Document *>(document)->setCallType(Call::FORWARD);
	    _source._entry = source;
	}
        
	Call::Call(const Call& src) :
	    DocumentObject(src.getDocument(),"",0),
	    _callType(src._callType), _destinationEntry(src._destinationEntry), 
	    _callMean(src._callMean), _histogram(src._histogram),
	    _hasResultVarianceWaitingTime(false),
	    _resultWaitingTime(0.0), _resultWaitingTimeVariance(0.0),
	    _resultVarianceWaitingTime(0.0), _resultVarianceWaitingTimeVariance(0.0),
	    _resultDropProbability(0.0), _resultDropProbabilityVariance(0.0)
	{
	}


	Call::~Call()
	{
	    /* Delete the variables */
	    if ( _histogram ) {
		delete _histogram;
	    }
	}
    
	Call * Call::clone() const
	{
	    return new Call( *this );
	}

	/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- [Input Values] -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
    
	const Call::CallType Call::getCallType() const
	{
	    /* Returns the CallType of the Call */
	    return _callType;
	}

	void Call::setCallType(const Call::CallType callType)
	{
	    /* Stores the given CallType of the Call */ 
	    _callType = callType;
	}
    
	const Entry* Call::getSourceEntry() const
	{
	    /* Return the source DOM::Entry */
	    if ( _callType == Call::FORWARD ) {
		return _source._entry;
	    } else {
		return _source._phase->getSourceEntry();
	    }
	}
    
	void Call::setSourceEntry( Entry * source ) 
	{
	    assert ( _callType == Call::FORWARD );
	    _source._entry = source;
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
    
	void Call::setCallMean(ExternalVariable* callMean)
	{
	    _callMean = callMean;
	}
    
	const double Call::getCallMeanValue() const
	{
	    /* Obtain the call mean */
	    double result = 0.0;
	    assert (_callMean->getValue(result) == true);
	    return result;
	}

	bool Call::hasHistogram() const
	{
	    return _histogram != 0 && _histogram->getBins() > 0; 
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
	    const_cast<Document *>(getDocument())->setEntryHasWaitingTimeVariance(true);
	    _hasResultVarianceWaitingTime = true;
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
	    const_cast<Document *>(getDocument())->setEntryHasDropProbability(true);
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
