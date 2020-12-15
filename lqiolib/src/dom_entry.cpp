/*
 *  $Id: dom_entry.cpp 14213 2020-12-14 17:14:40Z greg $
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
#include <cmath>
#include <stdexcept>
#include <iostream>
#include <algorithm>

namespace LQIO {
    namespace DOM {

	const char * Entry::__typeName = "entry";

	Entry::Entry(const Document * document, const std::string& name ) 
	    : DocumentObject(document,name),
	      _type(Entry::ENTRY_NOT_DEFINED), _phases(), 
	      _maxPhase(0), _task(NULL), _histograms(),
	      _openArrivalRate(NULL), _entryPriority(NULL),
	      _semaphoreType(SEMAPHORE_NONE),
	      _rwlockType(RWLOCK_NONE),
	      _forwarding(),
	      _startActivity(NULL),
	      _resultWaitingTime(0.0), _resultWaitingTimeVariance(0.0),
	      _resultPhasePProcessorWaiting(), _resultPhasePProcessorWaitingVariance(),
	      _resultPhasePServiceTime(), _resultPhasePServiceTimeVariance(),
	      _resultPhasePVarianceServiceTime(), _resultPhasePVarianceServiceTimeVariance(),
	      _resultProcessorUtilization(0.0), _resultProcessorUtilizationVariance(0.0),
	      _resultSquaredCoeffVariation(0.0), _resultSquaredCoeffVariationVariance(0.0),
	      _resultThroughput(0.0), _resultThroughputVariance(0.0),
	      _resultThroughputBound(0.0), 
	      _resultUtilization(0.0), _resultUtilizationVariance(0.0),
	      _hasResultsForPhase(),  _hasOpenWait(false), _hasThroughputBound(false)
	{
	    clearPhaseResults();
	}
    
	Entry::Entry( const Entry& src ) 
	    : DocumentObject( src ),
	      _type(src._type), _phases(),
	      _maxPhase(src._maxPhase), _task(NULL), _histograms(),
	      _openArrivalRate(src._openArrivalRate->clone()), _entryPriority(src._entryPriority->clone()),
	      _semaphoreType(src._semaphoreType), _rwlockType(src._rwlockType), _forwarding(src._forwarding),
	      _startActivity(NULL),
	      _resultWaitingTime(0.0), _resultWaitingTimeVariance(0.0),
	      _resultPhasePProcessorWaiting(), _resultPhasePProcessorWaitingVariance(),
	      _resultPhasePServiceTime(), _resultPhasePServiceTimeVariance(),
	      _resultPhasePVarianceServiceTime(), _resultPhasePVarianceServiceTimeVariance(),
	      _resultProcessorUtilization(0.0), _resultProcessorUtilizationVariance(0.0),
	      _resultSquaredCoeffVariation(0.0), _resultSquaredCoeffVariationVariance(0.0),
	      _resultThroughput(0.0), _resultThroughputVariance(0.0),
	      _resultThroughputBound(0.0), 
	      _resultUtilization(0.0), _resultUtilizationVariance(0.0),
	      _hasResultsForPhase(),  _hasOpenWait(false), _hasThroughputBound(false)
	{
	    clearPhaseResults();
	}

	Entry::~Entry()
	{
	    /* Destroy our owned phase information */
	    for ( std::map<unsigned, Phase*>::iterator phase = _phases.begin(); phase != _phases.end(); ++phase) {
		delete phase->second;
	    }
	    for ( std::vector<Call *>::iterator forwarding = _forwarding.begin(); forwarding != _forwarding.end(); ++forwarding) {
		delete *forwarding;
	    }
	    for ( std::map<unsigned, Histogram*>::iterator histogram = _histograms.begin(); histogram != _histograms.end(); ++histogram) {
		delete histogram->second;
	    }
	    if ( _openArrivalRate != nullptr ) delete _openArrivalRate;
	    if ( _entryPriority != nullptr ) delete _entryPriority;
	}

	Entry& Entry::clearPhaseResults()
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
	    return *this;
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

	void Entry::erasePhase( unsigned p )
	{
	    std::map<unsigned, Phase*>::iterator phase = _phases.find(p);
	    if ( phase == _phases.end()) throw std::domain_error( "Phase not found" );

	    delete phase->second;
	    _phases.erase(phase);	    /* Erase the item in the map. */

	    /* Reset the maxPhase */
	    if ( p == _maxPhase ) {
		_maxPhase = 0;
		for ( phase = _phases.begin(); phase != _phases.end(); ++phase ) {
		    _maxPhase = std::max( phase->first, _maxPhase );
		}
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

    
	bool Entry::hasPhase( unsigned p ) const
	{
	    /* Return whether or not this phase exists yet, because getPhase() makes it */
	    assert( 0 < p && p <= Phase::MAX_PHASE );
	    return _phases.find(p) != _phases.end();
	}
    
	unsigned Entry::getMaximumPhase() const
	{
	    return _maxPhase;
	}
    
	Call* Entry::getCallToTarget(const Entry* target, unsigned phase) const
	{
	    /* Attempt to find the call to the given target */
	    if (hasPhase(phase)) {
		return getPhase(phase)->getCallToTarget(target);
	    } else {
		return NULL;
	    }
	}
    
	void Entry::setOpenArrivalRate(ExternalVariable* value)
	{
	    /* Store the given open arrival rate */
	    _openArrivalRate = checkDoubleVariable( value, 0.0 );
	}
    
	double Entry::getOpenArrivalRateValue() const
	{
	    return getDoubleValue( getOpenArrivalRate(), 0.0 );
	}
    
	bool Entry::hasOpenArrivalRate() const
	{
	    return ExternalVariable::isPresent( getOpenArrivalRate(), 0.0 );
	}
    
	void Entry::setEntryPriority(ExternalVariable* value)
	{
	    /* Store the entry priority */
	    _entryPriority = checkIntegerVariable( value, 0 );
	}
    
	const ExternalVariable* Entry::getEntryPriority() const
	{
	    return _entryPriority;
	}

	int Entry::getEntryPriorityValue() const
	{
	    return getIntegerValue( getEntryPriority(), 0 );
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
    
  
	bool Entry::isDefined() const
	{
	    return _type != Entry::ENTRY_NOT_DEFINED 
		&& _type != Entry::ENTRY_STANDARD_NOT_DEFINED  
		&& _type != Entry::ENTRY_ACTIVITY_NOT_DEFINED;
	}


	bool Entry::isStandardEntry() const
	{
	    return _type == Entry::ENTRY_STANDARD_NOT_DEFINED || _type == Entry::ENTRY_STANDARD;
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
	    if ( std::find_if( _phases.begin(), _phases.end(), LQIO::DOM::Entry::Predicate<LQIO::DOM::Phase>( &LQIO::DOM::Phase::hasHistogram ) ) != _phases.end() ) {
		return true;
	    }
	    /* Bug 668 - check for histogram at entry level (activity entry) */
	    return std::find_if( _histograms.begin(),  _histograms.end(), LQIO::DOM::Entry::Predicate<LQIO::DOM::Histogram>( &LQIO::DOM::Histogram::isHistogram ) ) != _histograms.end();
	}

	bool Entry::hasHistogramForPhase( unsigned p) const
	{ 
	    if ( isStandardEntry() ) {
		return hasPhase(p) && getPhase(p)->hasHistogram();
	    } else {
		std::map<unsigned, Histogram*>::const_iterator i = _histograms.find(p);
		return i != _histograms.end() && i->second && i->second->isHistogram();
	    }
	}
	    
	const Histogram* Entry::getHistogramForPhase( unsigned p ) const
	{
	    if ( isStandardEntry() ) {
		if ( hasPhase(p) ) {
		    return getPhase(p)->getHistogram();
		}
	    } else {
		std::map<unsigned, Histogram*>::const_iterator histogram = _histograms.find(p);
		if ( histogram != _histograms.end() && histogram->second->isHistogram() ) {
		    return histogram->second;
		}
	    }
	    return  0;
	}

	void Entry::setHistogramForPhase( unsigned p, Histogram* histogram )
	{
	    if ( isStandardEntry() ) {
		getPhase(p)->setHistogram( histogram );
	    } else {
		assert(0 < p && p <= Phase::MAX_PHASE);
		_histograms[p] = histogram;
	    }
	}

	bool Entry::hasMaxServiceTimeExceeded() const 
 	{
	    if ( std::find_if( _phases.begin(), _phases.end(), LQIO::DOM::Entry::Predicate<LQIO::DOM::Phase>( &LQIO::DOM::Phase::hasMaxServiceTimeExceeded ) ) != _phases.end() ) {
		return true;
	    }
	    return std::find_if( _histograms.begin(),  _histograms.end(), Predicate<LQIO::DOM::Histogram>( &LQIO::DOM::Histogram::isTimeExceeded ) ) != _histograms.end();
 	}


	bool Entry::hasMaxServiceTimeExceededForPhase( unsigned p ) const 
	{
	    if ( isStandardEntry() ) {
		return hasPhase(p) && getPhase(p)->hasMaxServiceTimeExceeded();
	    } else {
		std::map<unsigned, Histogram*>::const_iterator i = _histograms.find(p);
		return i != _histograms.end() && i->second && i->second->isTimeExceeded();
	    }
	    return false;
	}

	void Entry::addForwardingCall(Call * call)
	{
	    /* Add the all to the call list */
	    _forwarding.push_back(call);
	}
    
	void Entry::eraseForwardingCall(Call * call)
	{
	    std::vector<Call*>::iterator iter = std::find( _forwarding.begin(), _forwarding.end(), call );
	    if ( iter != _forwarding.end() ) {
		_forwarding.erase( iter );
	    }
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

	bool Entry::hasForwarding() const
	{
	    return _forwarding.size() > 0;
	}


	const Activity* Entry::getStartActivity() const
	{
	    /* Returns the start activity name of the Entry */
	    return _startActivity;
	}

	void Entry::setStartActivity(Activity* startActivity)
	{
	    /* Stores the given StartActivity of the Entry */ 
	    if ( _startActivity && startActivity != nullptr ) {
		input_error2( ERR_DUPLICATE_START_ACTIVITY, getName().c_str(), startActivity->getName().c_str() );
	    } else {
		_startActivity = startActivity;
		if ( startActivity != nullptr ) {
		    _startActivity->setSourceEntry(this);
		}
	    }
	}
    
	const bool Entry::hasThinkTime() const
	{
	    return std::find_if( _phases.begin(), _phases.end(), LQIO::DOM::Entry::Predicate<LQIO::DOM::Phase>( &LQIO::DOM::Phase::hasThinkTime ) ) != _phases.end();
	}

	const bool Entry::hasDeterministicPhases() const
	{
	    return std::find_if( _phases.begin(), _phases.end(), LQIO::DOM::Entry::Predicate<LQIO::DOM::Phase>( &LQIO::DOM::Phase::hasDeterministicCalls ) ) != _phases.end();
	}
	    
	const bool Entry::hasNonExponentialPhases() const
	{
	    return std::find_if( _phases.begin(), _phases.end(), LQIO::DOM::Entry::Predicate<LQIO::DOM::Phase>( &LQIO::DOM::Phase::isNonExponential ) ) != _phases.end();
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
	    if ( resultThroughputVariance > 0 ) {
		const_cast<Document *>(getDocument())->setResultHasConfidenceIntervals(true);
	    }
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
	    return _resultUtilizationVariance;
	}
    
	Entry& Entry::setResultUtilizationVariance(const double resultUtilizationVariance)
	{
	    /* Stores the given ResultUtilization of the Entry */ 
	    if ( resultUtilizationVariance > 0 ) {
		const_cast<Document *>(getDocument())->setResultHasConfidenceIntervals(true);
	    }
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
	    if ( resultProcessorUtilizationVariance > 0 ) {
		const_cast<Document *>(getDocument())->setResultHasConfidenceIntervals(true);
	    }
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
    
	double Entry::getResultWaitingTime() const
	{
	    /* Returns the ResultWaitingTime of the Entry */
	    return _resultWaitingTime;
	}

	Entry& Entry::setResultWaitingTime(const double resultWaitingTime)
	{
	    if ( resultWaitingTime > 0 ) {
		if ( !hasOpenArrivalRate() ) throw std::invalid_argument( "Open Wait Time" );
		/* Stores the given ResultWaitingTime of the Entry */ 
		_resultWaitingTime = resultWaitingTime;
		_hasOpenWait = true;
	    }
	    return *this;
	}

	double Entry::getResultWaitingTimeVariance() const
	{
	    /* Returns the given ResultWaitingTime of the Entry */ 
	    return _resultWaitingTimeVariance;
	}

	Entry& Entry::setResultWaitingTimeVariance(const double resultWaitingTimeVariance)
	{
	    if ( resultWaitingTimeVariance > 0 ) {
		if ( !hasOpenArrivalRate() ) throw std::invalid_argument( "Open Wait Time" );
		/* Stores the given ResultWaitingTime of the Entry */ 
		_resultWaitingTimeVariance = resultWaitingTimeVariance;
		_hasOpenWait = true;
		Document * document = const_cast<Document *>(getDocument());
		document->setResultHasConfidenceIntervals(true);
	    }
	    return *this;
	}
    
	/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */

	/*
	 * If I am a standard entry, proxy the call to the phase.  Otherwise, get/store
	 * the result locally (activity entries have phase results too).
	 */

	Entry& 
	Entry::setResultPhaseP( const unsigned p, double * result, double value ) 
	{
	    assert( 0 < p && p <= Phase::MAX_PHASE );
	    if ( value ) {
		_hasResultsForPhase[p-1] = true;
	    }
	    result[p-1] = value;
	    return *this;
	}

	/*
	 * getPhase() will create a phase if it doesn't exist, since
	 * this method is NON-const, so only 'create' a phase if
	 * results are present.  Of course, if the phase does exist,
	 * we can always nuke the result.
	 */
	
	Entry& Entry::setResultPhasePServiceTime(const unsigned p,const double resultServiceTime)
	{
	    if ( isStandardEntry() ) {
		if ( hasPhase( p ) || resultServiceTime > 0 ) {
		    getPhase( p )->setResultServiceTime( resultServiceTime );
		}
	    } else {
		setResultPhaseP( p, _resultPhasePServiceTime, resultServiceTime );
	    }
	    return *this;
	}

    
	double Entry::getResultPhasePServiceTime(unsigned p) const
	{
	    /* Returns the ResultPhasePServiceTime of the Entry */
	    std::map<unsigned, Phase*>::const_iterator phase = _phases.find(p);
	    if ( isStandardEntry() && phase != _phases.end() ) {
		return phase->second->getResultServiceTime();
	    } else {
		assert( 0 < p && p <= Phase::MAX_PHASE );
		return _resultPhasePServiceTime[p-1];
	    }
	}

	Entry& Entry::setResultPhasePServiceTimeVariance(const unsigned p, const double resultServiceTimeVariance) 
	{
	    if ( isStandardEntry() ) {
		if ( hasPhase( p ) || resultServiceTimeVariance > 0 ) {
		    getPhase( p )->setResultServiceTimeVariance( resultServiceTimeVariance );
		}
	    } else {
		setResultPhaseP( p, _resultPhasePServiceTimeVariance, resultServiceTimeVariance );
	    }
	    return *this;
	}

	double Entry::getResultPhasePServiceTimeVariance(unsigned p) const
	{
	    /* Returns the given ResultPhasePServiceTime of the Entry */ 
	    std::map<unsigned, Phase*>::const_iterator phase = _phases.find(p);
	    if ( isStandardEntry() && phase != _phases.end() ) {
		return phase->second->getResultServiceTimeVariance();
	    } else {
		assert( 0 < p && p <= Phase::MAX_PHASE );
		return _resultPhasePServiceTimeVariance[p-1];
	    }
	}
    
	Entry& Entry::setResultPhasePVarianceServiceTime(unsigned p, double resultVarianceServiceTime ) 
	{
	    if ( isStandardEntry() ) {
		if ( hasPhase( p ) || resultVarianceServiceTime > 0 ) {
		    getPhase( p )->setResultVarianceServiceTime( resultVarianceServiceTime );
		}
	    } else {
		setResultPhaseP( p, _resultPhasePVarianceServiceTime, resultVarianceServiceTime );
	    }
	    return *this;
	}

	double Entry::getResultPhasePVarianceServiceTime( unsigned p ) const
	{
	    /* Returns the given ResultPhasePServiceTime of the Entry */ 
	    std::map<unsigned, Phase*>::const_iterator phase = _phases.find(p);
	    if ( isStandardEntry() && phase != _phases.end() ) {
		return phase->second->getResultVarianceServiceTime();
	    } else {
		assert( 0 < p && p <= Phase::MAX_PHASE );
		return _resultPhasePVarianceServiceTime[p-1];
	    }
	}

	Entry& Entry::setResultPhasePVarianceServiceTimeVariance(unsigned p, double resultVarianceServiceTimeVariance) 
	{
	    if ( isStandardEntry() ) {
		if ( hasPhase( p ) || resultVarianceServiceTimeVariance > 0 ) {
		    getPhase( p )->setResultVarianceServiceTimeVariance( resultVarianceServiceTimeVariance );
		}
	    } else {
		setResultPhaseP( p, _resultPhasePVarianceServiceTimeVariance, resultVarianceServiceTimeVariance );
	    }
	    return *this;
	}

	double Entry::getResultPhasePVarianceServiceTimeVariance(unsigned p) const
	{
	    /* Returns the given ResultPhasePVarianceServiceTime of the Entry */
	    std::map<unsigned, Phase*>::const_iterator phase = _phases.find(p);
	    if ( isStandardEntry() && phase != _phases.end() ) {
		return phase->second->getResultVarianceServiceTimeVariance();
	    } else {
		assert( 0 < p && p <= Phase::MAX_PHASE );
		return _resultPhasePVarianceServiceTimeVariance[p-1];
	    }
	}

	Entry& Entry::setResultPhasePProcessorWaiting(unsigned p, double resultProcessorWaiting ) 
	{
	    if ( isStandardEntry() ) {
		if ( hasPhase( p ) || resultProcessorWaiting> 0 ) {
		    getPhase( p )->setResultProcessorWaiting( resultProcessorWaiting );
		}
	    } else {
		setResultPhaseP( p, _resultPhasePProcessorWaiting, resultProcessorWaiting );
	    }
	    return *this;
	}

	double Entry::getResultPhasePProcessorWaiting(unsigned p) const
	{
	    /* Returns the ResultPhasePProcessorWaiting of the Entry */
	    std::map<unsigned, Phase*>::const_iterator phase = _phases.find(p);
	    if ( isStandardEntry() && phase != _phases.end() ) {
		return phase->second->getResultProcessorWaiting();
	    } else {
		assert( 0 < p && p <= Phase::MAX_PHASE );
		return _resultPhasePProcessorWaiting[p-1];
	    }
	}

	Entry& Entry::setResultPhasePProcessorWaitingVariance(unsigned p, double resultProcessorWaitingVariance ) 
	{
	    if ( isStandardEntry() ) {
		if ( hasPhase( p ) || resultProcessorWaitingVariance > 0 ) {
		    getPhase( p )->setResultProcessorWaitingVariance( resultProcessorWaitingVariance );
		}
	    } else {
		setResultPhaseP( p, _resultPhasePProcessorWaitingVariance, resultProcessorWaitingVariance );
	    }
	    return *this;
	}

	double Entry::getResultPhasePProcessorWaitingVariance(unsigned p) const
	{
	    /* Returns the given ResultPhasePProcessorWaiting of the Entry */ 
	    std::map<unsigned, Phase*>::const_iterator phase = _phases.find(p);
	    if ( isStandardEntry() && phase != _phases.end() ) {
		return phase->second->getResultProcessorWaitingVariance();
	    } else {
		assert( 0 < p && p <= Phase::MAX_PHASE );
		return _resultPhasePProcessorWaitingVariance[p-1];
	    }
	}
    
	Entry& Entry::setResultPhasePUtilization( unsigned p, double resultUtilization ) 
	{
	    if ( isStandardEntry() ) {
		if ( hasPhase( p ) || resultUtilization > 0 ) {
		    getPhase( p )->setResultUtilization( resultUtilization );
		}
	    } else {
		setResultPhaseP( p, _resultPhasePUtilization, resultUtilization );
	    }
	    return *this;
	}

	double Entry::getResultPhasePUtilization( unsigned p ) const
	{
	    /* Returns the ResultPhasePServiceTime of the Entry */
	    std::map<unsigned, Phase*>::const_iterator phase = _phases.find(p);
	    if ( isStandardEntry() && phase != _phases.end() ) {
		return phase->second->getResultUtilization();
	    } else {
		assert( 0 < p && p <= Phase::MAX_PHASE );
		return _resultPhasePUtilization[p-1];
	    }
	}

	Entry& Entry::setResultPhasePUtilizationVariance( unsigned p, double resultUtilizationVariance ) 
	{
	    if ( isStandardEntry() ) {
		if ( hasPhase( p ) || resultUtilizationVariance > 0 ) {
		    getPhase( p )->setResultUtilizationVariance( resultUtilizationVariance );
		}
	    } else {
		setResultPhaseP( p, _resultPhasePUtilizationVariance, resultUtilizationVariance );
	    }
	    return *this;
	}

	double Entry::getResultPhasePUtilizationVariance( unsigned p ) const
	{
	    /* Returns the ResultPhasePServiceTime of the Entry */
	    std::map<unsigned, Phase*>::const_iterator phase = _phases.find(p);
	    if ( isStandardEntry() && phase != _phases.end() ) {
		return phase->second->getResultUtilizationVariance();
	    } else {
		assert( 0 < p && p <= Phase::MAX_PHASE );
		return _resultPhasePUtilizationVariance[p-1];
	    }
	}

	double Entry::getResultPhasePMaxServiceTimeExceeded( unsigned p ) const
	{
	    /* Returns the ResultPhasePServiceTime of the Entry */
	    if ( isStandardEntry() ) {
		if ( hasPhase(p) ) {
		    return getPhase(p)->getResultMaxServiceTimeExceeded();
		}
	    } else {
		std::map<unsigned, Histogram*>::const_iterator i = _histograms.find(p);
		if ( i != _histograms.end() ) {
		    LQIO::DOM::Histogram * histogram = i->second;
		    if ( histogram->isTimeExceeded() ) {
			return histogram->getBinMean( histogram->getOverflowIndex() );
		    }
		}
	    }
	    return 0;
	}

	double Entry::getResultPhasePMaxServiceTimeExceededVariance( unsigned p ) const
	{
	    /* Returns the ResultPhasePServiceTime of the Entry */
	    if ( isStandardEntry() ) {
		if ( hasPhase(p) ) {
		    return getPhase(p)->getResultMaxServiceTimeExceededVariance();
		}
	    } else {
		std::map<unsigned, Histogram*>::const_iterator i = _histograms.find(p);
		if ( i != _histograms.end() ) {
		    LQIO::DOM::Histogram * histogram = i->second;
		    if ( histogram->isTimeExceeded() ) {
			return histogram->getBinVariance( histogram->getOverflowIndex() );
		    }
		}
	    }
	    return 0.;
	}

	unsigned Entry::getResultPhaseCount() const
	{
	    unsigned int maxPhase = 0;
	    for ( unsigned int p = 1; p <= Phase::MAX_PHASE; ++p) {
		if ( hasResultsForPhase(p) ) maxPhase = p;
	    }
	    return maxPhase;
	}
	
	bool Entry::hasResultsForPhase(unsigned p) const
	{
	    assert(0 < p && p <= Phase::MAX_PHASE);
	    return _hasResultsForPhase[p-1];
	}
    
	bool Entry::hasResultsForOpenWait() const
	{
	    return _hasOpenWait;
	}

	bool Entry::hasResultsForThroughputBound() const
	{
	    return _hasThroughputBound;
	}
 
	/* ------------------------------------------------------------------------ */

	Entry::Count& Entry::Count::operator()( const LQIO::DOM::Entry * e ) 
	{
	    const std::map<unsigned, Phase*>& phases = e->getPhaseList();
	    _count += std::count_if( phases.begin(), phases.end(), LQIO::DOM::Entry::Predicate<LQIO::DOM::Phase>( _f ) );
	    return *this;
	}
    }
}
