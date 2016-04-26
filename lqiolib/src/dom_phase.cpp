/*
 *  $Id: dom_phase.cpp 12458 2016-02-21 18:48:34Z greg $
 *
 *  Created by Martin Mroz on 24/02/09.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#include "dom_document.h"
#include "dom_call.h"
#include "dom_extvar.h"
#include "dom_histogram.h"
#include <cassert>
#include <algorithm>

namespace LQIO {
    namespace DOM {

	Phase::Phase(const Document * document,Entry* parentEntry) 
	    : DocumentObject(document,"",NULL), _calls(), _serviceTime(NULL),
	      _phaseTypeFlag(PHASE_STOCHASTIC), _entry(parentEntry),
	      _thinkTime(NULL), _coeffOfVariationSq(NULL), _histogram(NULL),
	      _resultServiceTime(0.0), _resultServiceTimeVariance(0.0),
	      _resultVarianceServiceTime(0.0), _resultVarianceServiceTimeVariance(0.0),
	      _resultUtilization(0.0), _resultUtilizationVariance(0.0),
	      _resultProcessorWaiting(0.0), _resultProcessorWaitingVariance(0.0)
	{
	}

	Phase::Phase( const LQIO::DOM::Phase& src ) 
	    : DocumentObject(src.getDocument(),src.getName().c_str(),NULL),_calls(), _serviceTime(const_cast<LQIO::DOM::ExternalVariable *>(src.getServiceTime())),
	      _phaseTypeFlag(src.getPhaseTypeFlag()), _entry(const_cast<LQIO::DOM::Entry*>(src.getSourceEntry())),
	      _thinkTime(const_cast<LQIO::DOM::ExternalVariable *>(src.getThinkTime())), _coeffOfVariationSq(const_cast<LQIO::DOM::ExternalVariable *>(src.getCoeffOfVariationSquared())), _histogram(NULL),
	      _resultServiceTime(0.0), _resultServiceTimeVariance(0.0),
	      _resultVarianceServiceTime(0.0), _resultVarianceServiceTimeVariance(0.0),
	      _resultUtilization(0.0), _resultUtilizationVariance(0.0),
	      _resultProcessorWaiting(0.0), _resultProcessorWaitingVariance(0.0)
	{
	}

	Phase::~Phase()
	{
	    /* Go through the list of calls after freeing service time */
	    std::vector<Call*>::iterator iter;
	    for (iter = _calls.begin(); iter != _calls.end(); ++iter) {
		delete(*iter);
	    }
	    if ( _histogram ) {
		delete _histogram;
	    }
	}

	/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- [Input Values] -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */

	bool Phase::isPresent() const 
	{
	    return _serviceTime || _thinkTime || _calls.size() > 0;
	}

	bool Phase::isNotNull() const
	{
	    /* Return true if it's a variable or non-zero constant */
	    return hasServiceTime() || hasThinkTime() || _calls.size() > 0;
	}

	double Phase::getServiceTimeValue() const
	{
	    double value = 0.0;
	    if ( !_serviceTime || _serviceTime->getValue(value) != true || value < 0 ) {
		/* LQX may set this negative, so we don't know it's wrong until it's used. */
		throw std::domain_error( "Invalid service time." );
	    }
	    return value;
	}

	const ExternalVariable* Phase::getServiceTime() const
	{
	    /* Returns the ServiceTime of the Phase */
	    return _serviceTime;
	}

	void Phase::setServiceTime(ExternalVariable* serviceTime)
	{
	    /* Stores the given ServiceTime of the Phase */
	    if (_serviceTime != NULL) {
//        printf("WARNING: Overwriting existing ExternalVariable in Phase.\n");
	    }

	    _serviceTime = serviceTime;
	}

	void Phase::setServiceTimeValue(double value)
	{
	    /* Store the value into the ExtVar */
	    if (_serviceTime == NULL) {
		_serviceTime = new ConstantExternalVariable(value);
	    } else {
		_serviceTime->set(value);
	    }
	}

	bool Phase::hasServiceTime() const
	{
	    double value = 0.0;
	    return _serviceTime && (!_serviceTime->wasSet() || !_serviceTime->getValue(value) || value > 0);	    /* Check whether we have it or not */
	}

	phase_type Phase::getPhaseTypeFlag() const
	{
	    /* Returns the PhaseTypeFlags of the Phase */
	    return _phaseTypeFlag;
	}

	void Phase::setPhaseTypeFlag(const phase_type phaseTypeFlag)
	{
	    /* Stores the given PhaseTypeFlags of the Phase */
	    _phaseTypeFlag = phaseTypeFlag;
	}

	const Entry* Phase::getSourceEntry() const
	{
	    /* Return the source entry */
	    return _entry;
	}

	void Phase::setSourceEntry(Entry* entry)
	{
	    /* Set a new source entry */
	    _entry = entry;
	}

	double Phase::getThinkTimeValue() const
	{
	    /* Retun the phase think time */
	    double value = 0.0;
	    if ( !_thinkTime || _thinkTime->getValue(value) != true || value < 0 ) {
		throw std::domain_error( "Invalid think time." );
	    }
	    return value;
	}

	const ExternalVariable* Phase::getThinkTime() const
	{
	    /* Return the think time */
	    return _thinkTime;
	}

	void Phase::setThinkTime(ExternalVariable* thinkTime)
	{
	    /* Store the new think time */
	    _thinkTime = thinkTime;
	}

	void Phase::setThinkTimeValue( double value )
	{
	    if ( _thinkTime == NULL ) {
		_thinkTime = new ConstantExternalVariable(value);
	    } else {
		_thinkTime->set(value);
	    }
	}

	bool Phase::hasThinkTime() const
	{
	    double value = 0.0;
	    return _thinkTime && (!_thinkTime->wasSet() || !_thinkTime->getValue(value) || value >= 0);	    /* Check whether we have it or not */
	}

	double Phase::getCoeffOfVariationSquaredValue() const
	{
	    /* Obtain the value */
	    double value;
	    if ( !_coeffOfVariationSq || _coeffOfVariationSq->getValue(value) != true || value < 0 ) {
		throw std::domain_error( "Invalid coefficient of variation squared." );
	    }
	    return value;
	}

	double Phase::getMaxServiceTime() const
	{
	    if ( hasMaxServiceTimeExceeded() ) {
		return getHistogram()->getMax();
	    } else {
		return 0.0;
	    }
	}

	const ExternalVariable* Phase::getCoeffOfVariationSquared() const
	{
	    /* Return the current variable */
	    return _coeffOfVariationSq;
	}

	void Phase::setCoeffOfVariationSquared(ExternalVariable* cvsq)
	{
	    if (_coeffOfVariationSq != NULL) {
//        printf("WARNING: Overwriting existing ExternalVariable in Phase.\n");
	    }

	    /* Store the new coefficient of variation */
	    _coeffOfVariationSq = cvsq;
	}

	void Phase::setCoeffOfVariationSquaredValue(double value)
	{
	    /* Set the coefficient of variation value */
	    if (_coeffOfVariationSq == NULL) {
		_coeffOfVariationSq = new ConstantExternalVariable(value);
	    } else {
		_coeffOfVariationSq->set(value);
	    }
	}

	bool Phase::hasCoeffOfVariationSquared() const
	{
	    /* Return whether this has been set or not */
	    return _coeffOfVariationSq != NULL;
	}

	bool Phase::isNonExponential() const
	{
	    /* Return true is CV != 1.0 (it must be set) */
	    double value = 0.0;
	    return _coeffOfVariationSq != NULL && (!_coeffOfVariationSq->getValue(value) || value != 1.0);
	}

	bool Phase::hasHistogram() const
	{ 
	    return _histogram != 0 && _histogram->getBins() > 0; 
	}

	void Phase::setHistogram(Histogram* histogram)
	{
	    /* Stores the given Histogram of the Phase */
	    _histogram = histogram;
	}

	bool Phase::hasMaxServiceTimeExceeded() const	
	{ 
	    return _histogram && _histogram->isMaxServiceTime(); 
	}

	void Phase::addCall(Call* call)
	{
	    /* Add the call to the list */
	    _calls.push_back(call);
	}

	const std::vector<Call*>& Phase::getCalls() const
	{
	    /* Return the calls */
	    return *&_calls;
	}

	Call* Phase::getCallToTarget(const Entry* entry) const
	{
	    /* Go through our list of calls for the one to the entry */
	    std::vector<Call*>::const_iterator iter = std::find_if( _calls.begin(), _calls.end(), Call::eqDestEntry(entry) );
	    if ( iter != _calls.end() ) return *iter;

	    return NULL;
	}

	/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- [Result Values] -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

	double Phase::getResultServiceTime() const
	{
	    /* Returns the ResultServiceTime of the Phase */
	    return _resultServiceTime;
	}

	Phase& Phase::setResultServiceTime(const double resultServiceTime)
	{
	    /* Stores the given ResultServiceTime of the Phase */
	    _resultServiceTime = resultServiceTime;
	    return *this;
	}

	double Phase::getResultServiceTimeVariance() const
	{
	    /* Stores the given ResultServiceTime of the Phase */
	    return _resultServiceTimeVariance;
	}

	Phase& Phase::setResultServiceTimeVariance(const double resultServiceTimeVariance)
	{
	    /* Stores the given ResultServiceTime of the Phase */
	    if ( resultServiceTimeVariance > 0 ) {
		const_cast<Document *>(getDocument())->setResultHasConfidenceIntervals(true);
	    }
	    _resultServiceTimeVariance = resultServiceTimeVariance;
	    return *this;
	}

	double Phase::getResultVarianceServiceTime() const
	{
	    /* Returns the ResultVarianceServiceTime of the Phase */
	    return _resultVarianceServiceTime;
	}

	Phase& Phase::setResultVarianceServiceTime(const double resultVarianceServiceTime)
	{
	    /* Stores the given ResultVarianceServiceTime of the Phase */
	    _resultVarianceServiceTime = resultVarianceServiceTime;
	    const_cast<Document *>(getDocument())->setEntryHasServiceTimeVariance(true);
	    return *this;
	}

	double Phase::getResultVarianceServiceTimeVariance() const
	{
	    /* Returns the given ResultVarianceServiceTime of the Phase */
	    return _resultVarianceServiceTimeVariance;
	}

	Phase& Phase::setResultVarianceServiceTimeVariance(const double resultVarianceServiceTimeVariance)
	{
	    /* Stores the given ResultVarianceServiceTime of the Phase */
	    if ( resultVarianceServiceTimeVariance > 0 ) {
		const_cast<Document *>(getDocument())->setResultHasConfidenceIntervals(true);
	    }
	    _resultVarianceServiceTimeVariance = resultVarianceServiceTimeVariance;
	    return *this;
	}

	double Phase::getResultMaxServiceTimeExceeded() const
	{
	    if ( _histogram && _histogram->isMaxServiceTime() ) {
		return _histogram->getBinMean( _histogram->getOverflowIndex() );
	    } else {
		return 0;
	    }
	}

	double Phase::getResultMaxServiceTimeExceededVariance() const
	{
	    if ( _histogram && _histogram->isMaxServiceTime() ) {
		return _histogram->getBinVariance( _histogram->getOverflowIndex() );
	    } else {
		return 0;
	    }
	}

	double Phase::getResultUtilization() const
	{
	    /* Returns the ResultUtilization of the Phase */
	    return _resultUtilization;
	}

	Phase& Phase::setResultUtilization(const double resultUtilization)
	{
	    /* Stores the given ResultUtilization of the Phase */
	    _resultUtilization = resultUtilization;
	    return *this;
	}

	double Phase::getResultUtilizationVariance() const
	{
	    /* Returns the given ResultUtilization of the Phase */
	    return _resultUtilizationVariance;
	}

	Phase& Phase::setResultUtilizationVariance(const double resultUtilizationVariance)
	{
	    /* Stores the given ResultUtilization of the Phase */
	    if ( resultUtilizationVariance > 0 ) {
		const_cast<Document *>(getDocument())->setResultHasConfidenceIntervals(true);
	    }
	    _resultUtilizationVariance = resultUtilizationVariance;
	    return *this;
	}

	double Phase::getResultProcessorWaiting() const
	{
	    /* Returns the ResultProcessorWaiting of the Phase */
	    return _resultProcessorWaiting;
	}

	Phase& Phase::setResultProcessorWaiting(const double resultProcessorWaiting)
	{
	    /* Stores the given ResultProcessorWaiting of the Phase */
	    _resultProcessorWaiting = resultProcessorWaiting;
	    return *this;
	}

	double Phase::getResultProcessorWaitingVariance() const
	{
	    /* Returns the given ResultProcessorWaiting of the Phase */
	    return _resultProcessorWaitingVariance;
	}

	Phase& Phase::setResultProcessorWaitingVariance(const double resultProcessorWaitingVariance)
	{
	    /* Stores the given ResultProcessorWaiting of the Phase */
	    if ( resultProcessorWaitingVariance > 0 ) {
		const_cast<Document *>(getDocument())->setResultHasConfidenceIntervals(true);
	    }
	    _resultProcessorWaitingVariance = resultProcessorWaitingVariance;
	    return *this;
	}

    }
}
