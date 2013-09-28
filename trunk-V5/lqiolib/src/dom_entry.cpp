/*
 *  $Id$
 *
 *  Created by Martin Mroz on 24/02/09.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#include "dom_document.h"
#include "dom_entry.h"
#include "dom_histogram.h"
#include "glblerr.h"
#include <cassert>
#include <stdexcept>
#include <iostream>
#include <algorithm>

namespace LQIO {
    namespace DOM {

	Entry::Entry(const Document * document, const char * name, const void * xmlDOMElement) 
	    : DocumentObject(document,name,xmlDOMElement),
	      _type(Entry::ENTRY_NOT_DEFINED), _phases(), 
	      _maxPhase(0), _task(NULL), _histograms(),
	      _openArrivalRate(NULL), _entryPriority(NULL),
	      _semaphoreType(SEMAPHORE_NONE),
	      _rwlockType(RWLOCK_NONE),
	      _forwarding(),
	      _startActivity(NULL),
	      _resultPhasePProcessorWaiting(), _resultPhasePProcessorWaitingVariance(),
	      _resultPhasePServiceTime(), _resultPhasePServiceTimeVariance(),
	      _resultPhasePVarianceServiceTime(), _resultPhasePVarianceServiceTimeVariance(),
	      _resultProcessorUtilization(0.0), _resultProcessorUtilizationVariance(0.0),
	      _resultSquaredCoeffVariation(), _resultSquaredCoeffVariationVariance(),
	      _resultThroughput(0.0), _resultThroughputVariance(0.0),
	      _resultThroughputBound(0.0), 
	      _resultUtilization(0.0), _resultUtilizationVariance(0.0),
	      _hasResultsForPhase(),  _hasOpenWait(false), _hasThroughputBound(false)
	{
	    for ( unsigned p = 0; p < Phase::MAX_PHASE; ++p ) {
		_resultPhasePProcessorWaiting[p] = 0.0;
		_resultPhasePProcessorWaitingVariance[p] = 0.0;
		_resultPhasePServiceTime[p] = 0.0;
		_resultPhasePServiceTimeVariance[p] = 0.0;
		_resultPhasePVarianceServiceTime[p] = 0.0;
		_resultPhasePVarianceServiceTimeVariance[p] = 0.0;
		_resultPhasePUtilization[p] = 0.0;
		_resultPhasePUtilizationVariance[p] = 0.0;
		_hasResultsForPhase[p] = false;
	    }
	}
    
	Entry::Entry( const Entry& src ) 
	    : DocumentObject( src.getDocument(), "", 0 ),
	      _type(src._type), _phases(),
	      _maxPhase(src._maxPhase), _task(NULL), _histograms(),
	      _openArrivalRate(src._openArrivalRate), _entryPriority(src._entryPriority),
	      _semaphoreType(src._semaphoreType), _rwlockType(src._rwlockType), _forwarding(src._forwarding),
	      _startActivity(NULL)
	{
	}

	Entry::~Entry()
	{
	    /* Destroy our owned phase information */
	    std::map<unsigned, Phase*>::iterator phaseIter;
	    for (phaseIter = _phases.begin(); phaseIter != _phases.end(); ++phaseIter) {
		Phase* phase = phaseIter->second;
		delete phase;
	    }
	    std::vector<Call *>::iterator forwardingIter;
	    for (forwardingIter = _forwarding.begin(); forwardingIter != _forwarding.end(); ++forwardingIter) {
		delete *forwardingIter;
	    }

	    std::map<unsigned, Histogram*>::iterator histogramIter;
	    for (histogramIter = _histograms.begin(); histogramIter != _histograms.end(); ++histogramIter) {
		Histogram* histogram = histogramIter->second;
		delete histogram;
	    }
	}

	Entry * Entry::clone() const
	{	
	    return new Entry( *this );
	}
    
	/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- [Input Values] -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
    
	void Entry::setTask(Task* task)
	{
	    _task = task;
	}
    
	const Task* Entry::getTask() const
	{
	    return _task;
	}
    
	Phase* Entry::getPhase(unsigned p) 
	{
	    assert(0 < p && p <= Phase::MAX_PHASE);
      
	    /* Return all of the calls for the given phase */
	    if (_phases.find(p) == _phases.end()) {
		setPhase( p, new Phase(getDocument(),this) );
	    }
      
	    return _phases[p];
	}

	Phase* Entry::getPhase(unsigned p) const
	{
	    assert(0 < p && p <= Phase::MAX_PHASE);
      
	    std::map<unsigned, Phase*>::const_iterator phase = _phases.find(p);
	    if ( phase == _phases.end() ) {
		return  0;
	    } else {
		return phase->second;
	    }
	}

	void Entry::setPhase( unsigned p, Phase * phase )
	{
	    assert(0 < p && p <= Phase::MAX_PHASE);

	    _phases[p] = phase;
	    if (_maxPhase < p) {
		_maxPhase = p;
		const_cast<Document *>(getDocument())->setMaximumPhase(p);
	    }
	}

    
	bool Entry::hasPhase(unsigned phase) const
	{
	    /* Return whether or not this phase exists yet, because getPhase() makes it */
	    return _phases.find(phase) != _phases.end();
	}
    
	unsigned Entry::getMaximumPhase() const
	{
	    /* Return the maximum phase */
	    return _maxPhase;
	}
    
	void Entry::appendOriginatingCall(Call* call)
	{
	    /* Push back the call that we originate */
	    unsigned phaseNumber = call->getPhase();
	    Phase* phase = getPhase(phaseNumber);
	    phase->addCall(call);
	}
    
	Call* Entry::getCallToTarget(Entry* target, unsigned phase) const
	{
	    /* Attempt to find the call to the given target */
	    if (hasPhase(phase)) {
		const Phase* domPhase = getPhase(phase);
		return domPhase->getCallToTarget(target);
	    }
      
	    return NULL;
	}
    
	void Entry::setOpenArrivalRate(ExternalVariable* value)
	{
	    /* Store the given open arrival rate */
	    const_cast<Document *>(getDocument())->setEntryHasOpenArrivals(true);
	    _openArrivalRate = value;
	}
    
	double Entry::getOpenArrivalRateValue() const
	{
	    /* Get the external variable */
	    double v = 0.0;
	    if (hasOpenArrivalRate() == false) return v;
	    _openArrivalRate->getValue(v);
	    return v;
	}
    
	bool Entry::hasOpenArrivalRate() const
	{
	    /* Find out whether a value was set */
	    return _openArrivalRate != NULL;
	}
    
	void Entry::setEntryPriority(ExternalVariable* value)
	{
	    /* Store the entry priority */
	    _entryPriority = value;
	}
    
	const ExternalVariable* Entry::getEntryPriority() const
	{
	    return _entryPriority;
	}

	double Entry::getEntryPriorityValue()
	{
	    /* Get the external variable */
	    double v = 0.0;
	    if (hasEntryPriority() == false) return v;
	    _entryPriority->getValue(v);
	    return v;
	}
    
	bool Entry::hasEntryPriority() const
	{
	    /* Find out whether a value was set */
	    return _entryPriority != NULL;
	}
    
	bool Entry::entrySemaphoreTypeOk(semaphore_entry_type newType)
	{
	    /* Set the type only if it was undefined to begin with */
	    if (_semaphoreType == SEMAPHORE_NONE ) {
		_semaphoreType = newType;
		return true;
	    }
      
	    return _semaphoreType == newType;
	}

	void Entry::setSemaphoreFlag(semaphore_entry_type set)
	{
	    /* Set the semaphore flag */
	    _semaphoreType = set;
	}
    
	semaphore_entry_type Entry::getSemaphoreFlag() const
	{
	    /* Return the semaphore type */
	    return _semaphoreType;
	}
    
	bool Entry::entryRWLockTypeOk(rwlock_entry_type newType)
	{
	    /* Set the type only if it was undefined to begin with */
	    if (_rwlockType == RWLOCK_NONE ) {
		_rwlockType = newType;
		return true;
	    }
      
	    return _rwlockType == newType;
	}

	void Entry::setRWLockFlag(rwlock_entry_type set)
	{
	    /* Set the rwlock flag */
	    _rwlockType = set;
	}
    
	rwlock_entry_type Entry::getRWLockFlag() const
	{
	    /* Return the rwlock type */
	    return _rwlockType;
	}
    
  
	bool Entry::entryTypeOk(EntryType newType)
	{
	    static const char * entry_types [] = { "?", "Ph1Ph2", "None", "Ph1Ph2", "None", "?" };

	    /* Set the type only if it was undefined to begin with */
	    if (_type == Entry::ENTRY_NOT_DEFINED 
		|| (_type == Entry::ENTRY_STANDARD_NOT_DEFINED && newType == Entry::ENTRY_STANDARD)
		|| (_type == Entry::ENTRY_ACTIVITY_NOT_DEFINED && newType == Entry::ENTRY_ACTIVITY) ) {
		_type = newType;
		return true;
	    } else if ( (_type == Entry::ENTRY_STANDARD_NOT_DEFINED && newType != Entry::ENTRY_STANDARD)
			|| (_type == Entry::ENTRY_ACTIVITY_NOT_DEFINED && newType != Entry::ENTRY_ACTIVITY) ) {
		LQIO::solution_error( LQIO::WRN_ENTRY_TYPE_MISMATCH, getName().c_str(), entry_types[_type], entry_types[newType] );
		_type = newType;
		return true;
	    }
	    return _type == newType;
	}
    
	void Entry::setEntryType(EntryType newType) 
	{
	    _type = newType;
	}
    
	const Entry::EntryType Entry::getEntryType() const
	{
	    return _type;
	}
    
	bool Entry::hasHistogram() const 
	{
	    for ( std::map<unsigned, Phase*>::const_iterator phaseIter = _phases.begin(); phaseIter != _phases.end(); ++phaseIter) {
		const Phase* phase = phaseIter->second;
		if ( phase->hasHistogram() ) return true;
	    }
	    /* Bug 668 - check for histogram at entry level (activity entry) */
	    for ( std::map<unsigned, Histogram*>::const_iterator histogramIter = _histograms.begin(); histogramIter != _histograms.end(); ++histogramIter) {
		const Histogram* histogram = histogramIter->second;
		if ( histogram ) return true;
	    }
	    return false;
	}

	bool Entry::hasHistogramForPhase( unsigned p) const
	{ 
	    return _histograms.find(p) != _histograms.end();
	}
	    
	const Histogram* Entry::getHistogramForPhase ( unsigned p ) const
	{
	    assert(0 < p && p <= Phase::MAX_PHASE);
	    std::map<unsigned, Histogram*>::const_iterator histogram = _histograms.find(p);
	    if ( histogram == _histograms.end() ) {
		return  0;
	    } else {
		return histogram->second;
	    }
	}

	void Entry::setHistogramForPhase( unsigned p, Histogram* histogram )
	{
	    assert(0 < p && p <= Phase::MAX_PHASE);

	    _histograms[p] = histogram;
	}

	bool Entry::hasMaxServiceTimeExceeded() const 
	{
	    for ( std::map<unsigned, Phase*>::const_iterator phaseIter = _phases.begin(); phaseIter != _phases.end(); ++phaseIter) {
		const Phase* phase = phaseIter->second;
		if ( phase->hasMaxServiceTimeExceeded() ) return true;
	    }
	    return false;
	}

	void Entry::addForwardingCall(Call * call)
	{
	    /* Add the all to the call list */
	    _forwarding.push_back(call);
	}
    
	const std::vector<Call *>& Entry::getForwarding() const
	{
	    /* Return a const ref to the map */
	    return _forwarding;
	}
    
	Call* Entry::getForwardingToTarget(const Entry* entry) const
	{
	    /* Go through our list of forwardings for the one to the entry */
	    std::vector<Call*>::const_iterator iter = std::find_if( _forwarding.begin(), _forwarding.end(), Call::eqDestEntry(entry) );
	    if ( iter != _forwarding.end() ) return *iter;

	    return NULL;
	}

	const Activity* Entry::getStartActivity() const
	{
	    /* Returns the start activity name of the Entry */
	    return _startActivity;
	}

	void Entry::setStartActivity(Activity* startActivity)
	{
	    /* Stores the given StartActivity of the Entry */ 
	    if ( _startActivity ) {
		input_error2( ERR_DUPLICATE_START_ACTIVITY, getName().c_str(), startActivity->getName().c_str() );
	    } else {
		_startActivity = startActivity;
		_startActivity->setSourceEntry(this);
	    }
	}
    
	const bool Entry::hasThinkTime() const
	{
	    std::map<unsigned, Phase*>::const_iterator phaseIter;
	    for (phaseIter = _phases.begin(); phaseIter != _phases.end(); ++phaseIter) {
		Phase* phase = phaseIter->second;
		if ( phase->hasThinkTime() ) return true;
	    }
	    return false;
	}

	const bool Entry::hasDeterministicPhases() const
	{
	    std::map<unsigned, Phase*>::const_iterator phaseIter;
	    for (phaseIter = _phases.begin(); phaseIter != _phases.end(); ++phaseIter) {
		Phase* phase = phaseIter->second;
		if ( phase->hasDeterministicCalls() ) return true;
	    }
	    return false;
	}
	    
	const bool Entry::hasNonExponentialPhases() const
	{
	    std::map<unsigned, Phase*>::const_iterator phaseIter;
	    for (phaseIter = _phases.begin(); phaseIter != _phases.end(); ++phaseIter) {
		Phase* phase = phaseIter->second;
		if ( phase->isNonExponential() ) return true;
	    }
	    return false;
	}

	/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- [Result Values] -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */
    
	double Entry::getResultThroughput() const
	{
	    /* Returns the ResultThroughput of the Entry */
	    return _resultThroughput;
	}

	Entry& Entry::setResultThroughput(const double resultThroughput)
	{
	    /* Stores the given ResultThroughput of the Entry */ 
	    _resultThroughput = resultThroughput;
	    return *this;
	}
    
	double Entry::getResultThroughputVariance() const
	{
	    /* Returns the given ResultThroughput of the Entry */ 
	    return _resultThroughputVariance;
	}
    
	Entry& Entry::setResultThroughputVariance(const double resultThroughputVariance)
	{
	    /* Stores the given ResultThroughput of the Entry */ 
	    const_cast<Document *>(getDocument())->setResultHasConfidenceIntervals(true);
	    _resultThroughputVariance = resultThroughputVariance;
	    return *this;
	}
    
	double Entry::getResultThroughputBound() const
	{
	    /* Returns the ResultThroughputBound of the Entry */
	    return _resultThroughputBound;
	}

	Entry& Entry::setResultThroughputBound(const double resultThroughputBound)
	{
	    /* Stores the given ResultThroughputBound of the Entry */ 
	    _resultThroughputBound = resultThroughputBound;
	    _hasThroughputBound = true;
	    const_cast<Document *>(getDocument())->setEntryHasThroughputBound(true);
	    return *this;
	}
    
	double Entry::getResultUtilization() const
	{
	    /* Returns the ResultUtilization of the Entry */
	    return _resultUtilization;
	}

	Entry& Entry::setResultUtilization(const double resultUtilization)
	{
	    /* Stores the given ResultUtilization of the Entry */ 
	    _resultUtilization = resultUtilization;
	    return *this;
	}
    
	double Entry::getResultUtilizationVariance() const
	{
	    /* Returns the given ResultUtilization of the Entry */ 
	    const_cast<Document *>(getDocument())->setResultHasConfidenceIntervals(true);
	    return _resultUtilizationVariance;
	}
    
	Entry& Entry::setResultUtilizationVariance(const double resultUtilizationVariance)
	{
	    /* Stores the given ResultUtilization of the Entry */ 
	    _resultUtilizationVariance = resultUtilizationVariance;
	    return *this;
	}
    
	double Entry::getResultProcessorUtilization() const
	{
	    /* Returns the ResultProcessorUtilization of the Entry */
	    return _resultProcessorUtilization;
	}

	Entry& Entry::setResultProcessorUtilization(const double resultProcessorUtilization)
	{
	    /* Stores the given ResultProcessorUtilization of the Entry */ 
	    _resultProcessorUtilization = resultProcessorUtilization;
	    return *this;
	}
    
	double Entry::getResultProcessorUtilizationVariance() const
	{
	    /* Returns the given ResultProcessorUtilization of the Entry */ 
	    return _resultProcessorUtilizationVariance;
	}
    
	Entry& Entry::setResultProcessorUtilizationVariance(const double resultProcessorUtilizationVariance)
	{
	    /* Stores the given ResultProcessorUtilization of the Entry */ 
	    const_cast<Document *>(getDocument())->setResultHasConfidenceIntervals(true);
	    _resultProcessorUtilizationVariance = resultProcessorUtilizationVariance;
	    return *this;
	}
    
	double Entry::getResultSquaredCoeffVariation() const
	{
	    /* Returns the ResultSquaredCoeffVariation of the Entry */
	    return _resultSquaredCoeffVariation;
	}

	Entry& Entry::setResultSquaredCoeffVariation(const double resultSquaredCoeffVariation)
	{
	    /* Stores the given ResultSquaredCoeffVariation of the Entry */ 
	    _resultSquaredCoeffVariation = resultSquaredCoeffVariation;
	    return *this;
	}
    
	double Entry::getResultSquaredCoeffVariationVariance() const
	{
	    return _resultSquaredCoeffVariationVariance;
	}
    
	Entry& Entry::setResultSquaredCoeffVariationVariance(const double resultSquaredCoeffVariationVariance)
	{
	    /* Stores the given ResultSquaredCoeffVariation of the Entry */ 
	    _resultSquaredCoeffVariationVariance = resultSquaredCoeffVariationVariance;
	    return *this;
	}
    
	double Entry::getResultOpenWaitTime() const
	{
	    /* Returns the ResultOpenWaitTime of the Entry */
	    return _resultOpenWaitTime;
	}

	Entry& Entry::setResultOpenWaitTime(const double resultOpenWaitTime)
	{
	    if ( resultOpenWaitTime > 0 ) {
		if ( !hasOpenArrivalRate() ) throw std::invalid_argument( "Open Wait Time" );
		/* Stores the given ResultOpenWaitTime of the Entry */ 
		_resultOpenWaitTime = resultOpenWaitTime;
		_hasOpenWait = true;
		const_cast<Document *>(getDocument())->setEntryHasOpenWait(true);
	    }
	    return *this;
	}

	double Entry::getResultOpenWaitTimeVariance() const
	{
	    /* Returns the given ResultOpenWaitTime of the Entry */ 
	    return _resultOpenWaitTimeVariance;
	}

	Entry& Entry::setResultOpenWaitTimeVariance(const double resultOpenWaitTimeVariance)
	{
	    if ( resultOpenWaitTimeVariance > 0 ) {
		if ( !hasOpenArrivalRate() ) throw std::invalid_argument( "Open Wait Time" );
		/* Stores the given ResultOpenWaitTime of the Entry */ 
		_resultOpenWaitTimeVariance = resultOpenWaitTimeVariance;
		_hasOpenWait = true;
		Document * document = const_cast<Document *>(getDocument());
		document->setEntryHasOpenWait(true);
		document->setResultHasConfidenceIntervals(true);
	    }
	    return *this;
	}
    
	/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */

	Entry& 
	Entry::setResultPhaseP( const unsigned p, double * result, double value ) 
	{
	    _hasResultsForPhase[p-1] = true;
	    result[p-1] = value;
	    return *this;
	}


	double Entry::getResultPhasePServiceTime(unsigned phase) const
	{
	    /* Returns the ResultPhasePServiceTime of the Entry */
	    return _resultPhasePServiceTime[phase-1];
	}
    
	double Entry::getResultPhasePServiceTimeVariance(unsigned phase) const
	{
	    /* Returns the given ResultPhasePServiceTime of the Entry */ 
	    return _resultPhasePServiceTimeVariance[phase-1];
	}
    
	double Entry::getResultPhasePVarianceServiceTime(unsigned phase) const
	{
	    /* Returns the ResultPhasePServiceTimeVariance of the Entry */
	    return _resultPhasePServiceTimeVariance[phase-1];
	}

	double Entry::getResultPhasePVarianceServiceTimeVariance(unsigned phase) const
	{
	    /* Returns the given ResultPhasePVarianceServiceTime of the Entry */
	    return _resultPhasePVarianceServiceTimeVariance[phase-1];
	}

	double Entry::getResultPhasePProcessorWaiting(unsigned phase) const
	{
	    /* Returns the ResultPhasePProcessorWaiting of the Entry */
	    return _resultPhasePProcessorWaiting[phase-1];
	}

	double Entry::getResultPhasePProcessorWaitingVariance(unsigned phase) const
	{
	    /* Returns the given ResultPhasePProcessorWaiting of the Entry */ 
	    return _resultPhasePProcessorWaitingVariance[phase-1];
	}
    
	double Entry::getResultPhasePUtilization( unsigned phase ) const
	{
	    return _resultPhasePUtilization[phase-1];
	}

	double Entry::getResultPhasePUtilizationVariance( unsigned phase ) const
	{
	    return _resultPhasePUtilizationVariance[phase-1];
	}

	bool Entry::hasResultsForPhase(unsigned phase) const
	{
	    assert(0 < phase && phase <= Phase::MAX_PHASE);
	    return _hasResultsForPhase[phase-1];
	}
    
	bool Entry::hasResultsForOpenWait() const
	{
	    return _hasOpenWait;
	}

	bool Entry::hasResultsForThroughputBound() const
	{
	    return _hasThroughputBound;
	}
    
	void Entry::resetResultFlags()
	{
	    /* Zero them back out */
	    _hasOpenWait = false;
      
	    /* Reset the values at the same time */
	    _resultOpenWaitTime = 0.0;
	    _resultOpenWaitTimeVariance = 0.0;
	    for ( unsigned p = 0; p < Phase::MAX_PHASE; ++p ) {
		_hasResultsForPhase[p] = false;
		_resultPhasePProcessorWaiting[p] = 0.0;
		_resultPhasePProcessorWaitingVariance[p] = 0.0;
		_resultPhasePServiceTime[p] = 0.0;
		_resultPhasePServiceTimeVariance[p] = 0.0;
		_resultPhasePVarianceServiceTime[p] = 0.0;
		_resultPhasePVarianceServiceTimeVariance[p] = 0.0;
	    }
	}
    
    }
}
