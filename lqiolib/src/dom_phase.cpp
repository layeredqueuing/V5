/*
 *  $Id: dom_phase.cpp 14955 2021-09-07 16:52:38Z greg $
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
#include <cmath>

namespace LQIO {
    namespace DOM {

	const char * Phase::__typeName = "phase";

	/* Dummy Phase */
	Phase::Phase() 
	    : DocumentObject(), _serviceTime(nullptr),
	      _phaseTypeFlag(Type::STOCHASTIC), _entry(nullptr),
	      _thinkTime(nullptr), _coeffOfVariationSq(nullptr), _histogram(nullptr),
	      _resultServiceTime(0.0), _resultServiceTimeVariance(0.0),
	      _resultVarianceServiceTime(0.0), _resultVarianceServiceTimeVariance(0.0),
	      _resultUtilization(0.0), _resultUtilizationVariance(0.0),
	      _resultProcessorWaiting(0.0), _resultProcessorWaitingVariance(0.0),
	      _hasResultServiceTimeVariance(false)
	{
	}

	/* Normal constructor */
	Phase::Phase(const Document * document,Entry* parentEntry) 
	    : DocumentObject(document,""), _calls(), _serviceTime(nullptr),
	      _phaseTypeFlag(Type::STOCHASTIC), _entry(parentEntry),
	      _thinkTime(nullptr), _coeffOfVariationSq(nullptr), _histogram(nullptr),
	      _resultServiceTime(0.0), _resultServiceTimeVariance(0.0),
	      _resultVarianceServiceTime(0.0), _resultVarianceServiceTimeVariance(0.0),
	      _resultUtilization(0.0), _resultUtilizationVariance(0.0),
	      _resultProcessorWaiting(0.0), _resultProcessorWaitingVariance(0.0),
	      _hasResultServiceTimeVariance(false)
	{
	}

	Phase::Phase( const LQIO::DOM::Phase& src )
	    : DocumentObject(src.getDocument(),src.getName()),
	      _calls(),		/* WARNING! Not copied */
	      _serviceTime(src._serviceTime),
	      _phaseTypeFlag(src.getPhaseTypeFlag()), _entry(const_cast<LQIO::DOM::Entry*>(src.getSourceEntry())),
	      _thinkTime(src._thinkTime),
	      _coeffOfVariationSq(ExternalVariable::clone(src._coeffOfVariationSq)),
	      _histogram(nullptr),	/* not copied */
	      _resultServiceTime(0.0), _resultServiceTimeVariance(0.0),
	      _resultVarianceServiceTime(0.0), _resultVarianceServiceTimeVariance(0.0),
	      _resultUtilization(0.0), _resultUtilizationVariance(0.0),
	      _resultProcessorWaiting(0.0), _resultProcessorWaitingVariance(0.0),
	      _hasResultServiceTimeVariance(false)
	{
	}

	Phase::~Phase()
	{
	    /* Go through the list of calls after freeing service time */
	    for ( std::vector<Call*>::iterator call = _calls.begin(); call != _calls.end(); ++call) {
		delete *call;
	    }
	    if ( _histogram != nullptr ) delete _histogram;
	}

	/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- [Input Values] -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */

	bool Phase::isPresent() const
	{
	    return hasServiceTime() || hasThinkTime() || _calls.size() > 0;
	}

	double Phase::getServiceTimeValue() const
	{
	    return getDoubleValue( getServiceTime(), 0.0 );
	}

	const ExternalVariable* Phase::getServiceTime() const
	{
	    /* Returns the ServiceTime of the Phase */
	    return _serviceTime;
	}

	void Phase::setServiceTime(const ExternalVariable* serviceTime)
	{
	    /* Stores the given ServiceTime of the Phase */
	    _serviceTime = checkDoubleVariable( serviceTime, 0.0 );
	}

	void Phase::setServiceTimeValue(double value)
	{
	    /* Store the value into the ExtVar */
	    if (_serviceTime == nullptr) {
		_serviceTime = new ConstantExternalVariable(value);
	    } else {
		const_cast<ExternalVariable *>(_serviceTime)->set(value);	// Should just overwrite.
	    }
	}

	bool Phase::hasServiceTime() const
	{
	    return ExternalVariable::isPresent( getServiceTime() );
	}

	Phase::Type Phase::getPhaseTypeFlag() const
	{
	    /* Returns the PhaseTypeFlags of the Phase */
	    return _phaseTypeFlag;
	}

	void Phase::setPhaseTypeFlag(const Phase::Type phaseTypeFlag)
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
	    return getDoubleValue( getThinkTime(), 0. );
	}

	const ExternalVariable* Phase::getThinkTime() const
	{
	    /* Return the think time */
	    return _thinkTime;
	}

	void Phase::setThinkTime(const ExternalVariable* thinkTime)
	{
	    /* Store the new think time */
	    _thinkTime = checkDoubleVariable( thinkTime, 0.0 );
	}

	void Phase::setThinkTimeValue( double value )
	{
	    if ( _thinkTime == nullptr ) {
		_thinkTime = new ConstantExternalVariable(value);
	    } else {
		const_cast<ExternalVariable *>(_thinkTime)->set(value);
	    }
	}

	bool Phase::hasThinkTime() const
	{
	    return ExternalVariable::isPresent( getThinkTime() );
	}

	void Phase::setMaxServiceTime(const ExternalVariable* time)
	{
	    /* Weird one as can't be changed dynamically */
	    double value = 0.0;
	    /* LQX may set this negative, so we don't know it's wrong until it's used. */
	    if ( time != nullptr && (time->getValue(value) == true && value > 0 )) {
		setMaxServiceTimeValue( value );
	    } else {
		throw std::domain_error( "invalid max service time" );
	    }

	}

	void Phase::setMaxServiceTimeValue(double time)
	{
	    if ( _histogram != nullptr ) {
		_histogram->setTimeExceeded( time );
	    } else {
		_histogram = new LQIO::DOM::Histogram( getDocument(), Histogram::Type::CONTINUOUS, 0, time, time );
	    }
	}

	double Phase::getMaxServiceTime() const
	{
	    if ( hasMaxServiceTimeExceeded() ) {
		return getHistogram()->getMax();
	    } else {
		return 0.0;
	    }
	}

	double Phase::getCoeffOfVariationSquaredValue() const
	{
	    double value = 1.0;		/* Default is one, but zero allowed */
	    if ( _coeffOfVariationSq != nullptr && ( _coeffOfVariationSq->getValue(value) != true || std::isinf(value) || value < 0.0 ) ) {
		throw std::domain_error( "invalid external variable" );
	    }
	    return value;
	}

	const ExternalVariable* Phase::getCoeffOfVariationSquared() const
	{
	    /* Return the current variable */
	    return _coeffOfVariationSq;
	}

	void Phase::setCoeffOfVariationSquared(const ExternalVariable* cvsq)
	{
	    /* Store the new coefficient of variation */
	    _coeffOfVariationSq = cvsq;
	}

	void Phase::setCoeffOfVariationSquaredValue(double value)
	{
	    /* Set the coefficient of variation value */
	    if (_coeffOfVariationSq == nullptr) {
		_coeffOfVariationSq = new ConstantExternalVariable(value);
	    } else {
		const_cast<ExternalVariable *>(_coeffOfVariationSq)->set(value);
	    }
	}

	bool Phase::hasCoeffOfVariationSquared() const
	{
	    /* Return whether this has been set or not */
	    double value = 1.0;
	    return _coeffOfVariationSq && (!_coeffOfVariationSq->wasSet() || !_coeffOfVariationSq->getValue(value) || (std::isfinite(value) && value > 0. && value != 1.));
	}

	bool Phase::isNonExponential() const
	{
	    /* Return true is CV != 1.0 (it must be set) */
	    double value = 1.0;
	    return _coeffOfVariationSq != nullptr && (!_coeffOfVariationSq->getValue(value) || value != 1.0);
	}

	bool Phase::hasHistogram() const
	{
	    return _histogram != 0 && _histogram->isHistogram();
	}

	void Phase::setHistogram(Histogram* histogram)
	{
	    /* Stores the given Histogram of the Phase */
	    _histogram = histogram;
	}

	bool Phase::hasMaxServiceTimeExceeded() const
	{
	    return _histogram && _histogram->isTimeExceeded();
	}

	void Phase::addCall(Call* call)
	{
	    /* Add the call to the list */
	    _calls.push_back(call);
	}

	void Phase::eraseCall(Call * call)
	{
	    std::vector<Call*>::iterator iter = std::find( _calls.begin(), _calls.end(), call );
	    if ( iter != _calls.end() ) {
		_calls.erase( iter );
	    }
	}


	const std::vector<Call*>& Phase::getCalls() const
	{
	    /* Return the calls */
	    return _calls;
	}

	Call* Phase::getCallToTarget(const Entry* entry) const
	{
	    /* Go through our list of calls for the one to the entry */
	    std::vector<Call*>::const_iterator iter = std::find_if( _calls.begin(), _calls.end(), Call::eqDestEntry(entry) );
	    if ( iter != _calls.end() ) return *iter;

	    return nullptr;
	}

	bool Phase::hasRendezvous() const
	{
	    return std::any_of( _calls.begin(), _calls.end(), Predicate<LQIO::DOM::Call>( &Call::hasRendezvous ) );
	}

	bool Phase::hasSendNoReply() const
	{
	    return std::any_of( _calls.begin(), _calls.end(), Predicate<LQIO::DOM::Call>( &Call::hasSendNoReply ) );
	}

	bool Phase::hasResultVarianceWaitingTime() const
	{
	    return std::any_of( _calls.begin(), _calls.end(), Predicate<LQIO::DOM::Call>( &Call::hasResultVarianceWaitingTime ) );
	}

	bool Phase::hasResultDropProbability() const
	{
	    return std::any_of( _calls.begin(), _calls.end(), Predicate<LQIO::DOM::Call>( &Call::hasResultDropProbability ) );
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
	    _hasResultServiceTimeVariance = true;
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
	    if ( _histogram && _histogram->isTimeExceeded() ) {
		return _histogram->getBinMean( _histogram->getOverflowIndex() );
	    } else {
		return 0;
	    }
	}

	double Phase::getResultMaxServiceTimeExceededVariance() const
	{
	    if ( _histogram && _histogram->isTimeExceeded() ) {
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
