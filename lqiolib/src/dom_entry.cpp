/*
 *  $Id: dom_entry.cpp 17596 2025-11-21 20:04:52Z greg $
 *
 *  Created by Martin Mroz on 24/02/09.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#include <algorithm>
#include <cassert>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include "dom_activity.h"
#include "dom_document.h"
#include "dom_entry.h"
#include "dom_histogram.h"
#include "glblerr.h"

namespace LQIO {
    namespace DOM {

	const char * Entry::__typeName = "entry";

	Entry::Entry(const Document * document, const std::string& name ) 
	    : DocumentObject(document,name),
	      _type(Entry::Type::NOT_DEFINED), _phases(), 
	      _maxPhase(0), _task(nullptr), _histograms(),
	      _openArrivalRate(nullptr), _entryPriority(nullptr), _visitProbability(nullptr),
	      _semaphoreType(Semaphore::NONE),
	      _rwlockType(RWLock::NONE),
	      _forwarding(),
	      _startActivity(nullptr),
	      _resultDropProbability(0.0), _resultDropProbabilityVariance(0.0),
	      _resultPhasePProcessorWaiting(), _resultPhasePProcessorWaitingVariance(),
	      _resultPhasePServiceTime(), _resultPhasePServiceTimeVariance(),
	      _resultPhasePVarianceServiceTime(), _resultPhasePVarianceServiceTimeVariance(),
	      _resultProcessorUtilization(0.0), _resultProcessorUtilizationVariance(0.0),
	      _resultSquaredCoeffVariation(0.0), _resultSquaredCoeffVariationVariance(0.0),
	      _resultThroughput(0.0), _resultThroughputBound(0.0), _resultThroughputVariance(0.0),
	      _resultUtilization(0.0), _resultUtilizationVariance(0.0),
	      _resultWaitingTime(0.0), _resultWaitingTimeVariance(0.0),
	      _hasResultsForPhase(),  _hasOpenWait(), _hasThroughputBound(), _hasResultSquaredCoeffVariation(), _hasResultDropProbability()
	{
	}
    
	Entry::Entry( const Entry& src ) 
	    : DocumentObject( src ),
	      _type(src._type), _phases(),
	      _maxPhase(src._maxPhase), _task(nullptr), _histograms(),
	      _openArrivalRate(src._openArrivalRate), _entryPriority(src._entryPriority), _visitProbability(src._visitProbability),
	      _semaphoreType(src._semaphoreType), _rwlockType(src._rwlockType), _forwarding(src._forwarding),
	      _startActivity(nullptr),
	      _resultDropProbability(src._resultDropProbability), _resultDropProbabilityVariance(src._resultDropProbabilityVariance),
	      _resultPhasePProcessorWaiting(src._resultPhasePProcessorWaiting), _resultPhasePProcessorWaitingVariance(src._resultPhasePProcessorWaitingVariance),
	      _resultPhasePServiceTime(src._resultPhasePServiceTime), _resultPhasePServiceTimeVariance(src._resultPhasePServiceTimeVariance),
	      _resultPhasePVarianceServiceTime(src._resultPhasePVarianceServiceTime), _resultPhasePVarianceServiceTimeVariance(src._resultPhasePVarianceServiceTimeVariance),
	      _resultProcessorUtilization(src._resultProcessorUtilization), _resultProcessorUtilizationVariance(src._resultProcessorUtilizationVariance),
	      _resultSquaredCoeffVariation(src._resultSquaredCoeffVariation), _resultSquaredCoeffVariationVariance(src._resultSquaredCoeffVariationVariance),
	      _resultThroughput(src._resultThroughput), _resultThroughputBound(src._resultThroughputBound), _resultThroughputVariance(src._resultThroughputVariance),
	      _resultUtilization(src._resultUtilization), _resultUtilizationVariance(src._resultUtilizationVariance),
	      _resultWaitingTime(src._resultWaitingTime), _resultWaitingTimeVariance(src._resultWaitingTimeVariance),
	      _hasResultsForPhase(src._hasResultsForPhase), _hasOpenWait(src._hasOpenWait), _hasThroughputBound(src._hasThroughputBound), _hasResultSquaredCoeffVariation(src._hasResultSquaredCoeffVariation), _hasResultDropProbability(src._hasResultDropProbability)
	{
	}

	Entry::~Entry()
	{
	    /* Destroy our owned phase information */
	    std::for_each( _phases.begin(), _phases.end(), []( const std::pair<unsigned, Phase*>& phase ){ delete phase.second; } );
	    std::for_each( _forwarding.begin(), _forwarding.end(), []( Call * forwarding) { delete forwarding; } );
	    std::for_each( _histograms.begin(), _histograms.end(), []( const std::pair<unsigned, Histogram*>& histogram ){ delete histogram.second; } );
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
		return nullptr;
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
		return nullptr;
	    }
	}
    
	void Entry::setOpenArrivalRate(const ExternalVariable* value)
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
	    return ExternalVariable::isPresent( getOpenArrivalRate() );
	}
    
	void Entry::setEntryPriority(const ExternalVariable* value)
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
	    return _entryPriority != nullptr;
	}
    
	void Entry::setVisitProbabilityValue(double value)
	{
	    if ( _visitProbability == nullptr ) {
		_visitProbability = new ConstantExternalVariable(value);
	    } else {
		const_cast<ExternalVariable *>(_visitProbability)->set(value);
	    }
	}

	void Entry::setVisitProbability(const ExternalVariable* value)
	{
	    /* Store the visit probability */
	    _visitProbability = checkDoubleVariable( value, 0.0, 1.0 );
	}
    
	const ExternalVariable* Entry::getVisitProbability() const
	{
	    return _visitProbability;
	}

	double Entry::getVisitProbabilityValue() const
	{
	    return getDoubleValue( getVisitProbability(), 0.0, 1.0 );
	}
    
	bool Entry::hasVisitProbability() const
	{
	    /* Find out whether a value was set */
	    return _visitProbability != nullptr;
	}
    
	bool Entry::entrySemaphoreTypeOk(Semaphore newType)
	{
	    /* Set the type only if it was undefined to begin with */
	    if (_semaphoreType == Entry::Semaphore::NONE ) {
		_semaphoreType = newType;
		return true;
	    }
      
	    return _semaphoreType == newType;
	}

	void Entry::setSemaphoreFlag(Semaphore set)
	{
	    /* Set the semaphore flag */
	    _semaphoreType = set;
	}
    
	Entry::Semaphore Entry::getSemaphoreFlag() const
	{
	    /* Return the semaphore type */
	    return _semaphoreType;
	}
    
	bool Entry::entryRWLockTypeOk(RWLock newType)
	{
	    /* Set the type only if it was undefined to begin with */
	    if (_rwlockType == RWLock::NONE ) {
		_rwlockType = newType;
		return true;
	    }
      
	    return _rwlockType == newType;
	}

	void Entry::setRWLockFlag(RWLock set)
	{
	    /* Set the rwlock flag */
	    _rwlockType = set;
	}
    
	Entry::RWLock Entry::getRWLockFlag() const
	{
	    /* Return the rwlock type */
	    return _rwlockType;
	}
    
  
	bool Entry::isDefined() const
	{
	    return _type != Entry::Type::NOT_DEFINED 
		&& _type != Entry::Type::STANDARD_NOT_DEFINED  
		&& _type != Entry::Type::ACTIVITY_NOT_DEFINED;
	}


	bool Entry::isStandardEntry() const
	{
	    return _type == Entry::Type::STANDARD_NOT_DEFINED || _type == Entry::Type::STANDARD;
	}

	bool Entry::entryTypeOk(Entry::Type newType)
	{
	    static const std::map<const Entry::Type,const std::string> entry_types = { {Type::NOT_DEFINED, "?"}, {Type::STANDARD, "Ph1Ph2"}, {Type::ACTIVITY, "None"}, {Type::STANDARD_NOT_DEFINED, "Ph1Ph2"}, {Type::ACTIVITY_NOT_DEFINED, "None"}, {Type::DEVICE, "?"} };

	    /* Set the type only if it was undefined to begin with */
	    if (_type == Entry::Type::NOT_DEFINED 
		|| (_type == Entry::Type::STANDARD_NOT_DEFINED && newType == Entry::Type::STANDARD)
		|| (_type == Entry::Type::ACTIVITY_NOT_DEFINED && newType == Entry::Type::ACTIVITY) ) {
		_type = newType;
	    } else if ( (_type == Entry::Type::STANDARD_NOT_DEFINED && newType != Entry::Type::STANDARD)
			|| (_type == Entry::Type::ACTIVITY_NOT_DEFINED && newType != Entry::Type::ACTIVITY) ) {
		const std::map<const Entry::Type,const std::string>::const_iterator i = entry_types.find(_type);
		const std::map<const Entry::Type,const std::string>::const_iterator j = entry_types.find(newType);
		assert ( i != entry_types.end() && j != entry_types.end() );
		runtime_error( LQIO::WRN_MIXED_ENTRY_TYPES, i->second.c_str(), j->second.c_str() );
		_type = newType;
	    }
	    return _type == newType;
	}
    
	void Entry::setEntryType(Entry::Type newType) 
	{
	    _type = newType;
	}
    
	const Entry::Type Entry::getEntryType() const
	{
	    return _type;
	}
    
	bool Entry::hasHistogram() const 
	{
	    return std::any_of( _phases.begin(), _phases.end(), []( const auto& phase ){ return phase.second->hasHistogram(); } )
		|| std::any_of( _histograms.begin(),  _histograms.end(), []( const auto& histogram ){ return histogram.second->isHistogram(); } );
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
	    return nullptr;
	}

	void Entry::setHistogramForPhase( unsigned p, Histogram* histogram )
	{
	    if ( isStandardEntry() ) {
		getPhase(p)->setHistogram( histogram );
	    } else {
		_histograms.at(p-1) = histogram;
	    }
	}

	bool Entry::hasMaxServiceTimeExceeded() const 
 	{
	    return std::any_of( _phases.begin(), _phases.end(), []( const auto& phase ){ return phase.second->hasMaxServiceTimeExceeded(); } )
		|| std::any_of( _histograms.begin(),  _histograms.end(), []( const auto& histogram ){ return histogram.second->isTimeExceeded(); } );
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
	    std::vector<Call*>::const_iterator iter = std::find_if( _forwarding.begin(), _forwarding.end(), [=]( const Call * call ){ return call->getDestinationEntry() == entry; } );
	    if ( iter != _forwarding.end() ) return *iter;

	    return nullptr;
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
		input_error( ERR_DUPLICATE_START_ACTIVITY, startActivity->getName().c_str() );
	    } else {
		_startActivity = startActivity;
		if ( startActivity != nullptr ) {
		    _startActivity->setSourceEntry(this);
		}
	    }
	}
    
	const bool Entry::hasThinkTime() const
	{
 	    return std::any_of( _phases.begin(), _phases.end(), []( const auto& phase ){ return phase.second->hasThinkTime(); } );
	}

	const bool Entry::hasDeterministicPhases() const
	{
	    return std::any_of( _phases.begin(), _phases.end(), []( const auto& phase ){ return phase.second->hasDeterministicCalls(); } );
	}
	    
	const bool Entry::hasNonExponentialPhases() const
	{
	    return std::any_of( _phases.begin(), _phases.end(), []( const auto& phase ){ return phase.second->hasCoeffOfVariationSquared(); } );
	}

	/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- [Result Values] -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */
    
	double Entry::getResultThroughput() const
	{
	    /* Returns the ResultThroughput of the Entry */
	    return _resultThroughput;
	}

	Entry& Entry::setResultThroughput(const double resultThroughput)
	{
	    assert( resultThroughput >= 0.0 );
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
	    assert( resultThroughputVariance >= 0.0 );
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
	    assert( resultThroughputBound >= 0.0 );
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
	    assert( resultUtilization >= 0.0 );
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
	    assert( resultUtilizationVariance >= 0.0 );
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
	    assert( resultProcessorUtilization >= 0.0 );
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
	    assert( resultProcessorUtilizationVariance >= 0.0 );
	    /* Stores the given ResultProcessorUtilization of the Entry */ 
	    if ( resultProcessorUtilizationVariance > 0 ) {
		const_cast<Document *>(getDocument())->setResultHasConfidenceIntervals(true);
	    }
	    _resultProcessorUtilizationVariance = resultProcessorUtilizationVariance;
	    return *this;
	}

	bool Entry::hasResultsForSquaredCoeffVariation() const
	{
	    return _hasResultSquaredCoeffVariation;
	}
	
	double Entry::getResultSquaredCoeffVariation() const
	{
	    /* Returns the ResultSquaredCoeffVariation of the Entry */
	    return _resultSquaredCoeffVariation;
	}

	Entry& Entry::setResultSquaredCoeffVariation(const double resultSquaredCoeffVariation)
	{
	    assert( resultSquaredCoeffVariation >= 0.0 );
	    /* Stores the given ResultSquaredCoeffVariation of the Entry */ 
	    _resultSquaredCoeffVariation = resultSquaredCoeffVariation;
	    _hasResultSquaredCoeffVariation = true;
	    return *this;
	}
    
	double Entry::getResultSquaredCoeffVariationVariance() const
	{
	    return _resultSquaredCoeffVariationVariance;
	}
    
	Entry& Entry::setResultSquaredCoeffVariationVariance(const double resultSquaredCoeffVariationVariance)
	{
	    assert( resultSquaredCoeffVariationVariance >= 0.0 );
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
	    if ( resultWaitingTime > 0. ) {
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
	    if ( resultWaitingTimeVariance > 0. ) {
		if ( !hasOpenArrivalRate() ) throw std::invalid_argument( "Open Wait Time" );
		/* Stores the given ResultWaitingTime of the Entry */ 
		_resultWaitingTimeVariance = resultWaitingTimeVariance;
		_hasOpenWait = true;
		Document * document = const_cast<Document *>(getDocument());
		document->setResultHasConfidenceIntervals(true);
	    }
	    return *this;
	}
    
	Entry& Entry::setResultDropProbability( double resultDropProbability )
	{
	    _hasResultDropProbability = true;
	    _resultDropProbability = resultDropProbability;
	    return *this;
	}
	
	double Entry::getResultDropProbability() const
	{
	    return _resultDropProbability;
	}

	Entry& Entry::setResultDropProbabilityVariance( double resultDropProbabilityVariance )
	{
	    _hasResultDropProbability = true;
	    _resultDropProbabilityVariance = resultDropProbabilityVariance;
	    return *this;
	}
	
	double Entry::getResultDropProbabilityVariance() const
	{
	    return _resultDropProbabilityVariance;
	}

	/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */

 	/*
	 * If I am a standard entry, proxy the call to the phase.  Otherwise, get/store
	 * the result locally (activity entries have phase results too).
	 */

	void
	Entry::setResultPhaseP( const unsigned p, std::array<double,Phase::MAX_PHASE>& result, double value ) 
	{
	    result.at(p-1) = value;
	    _hasResultsForPhase[p-1] |= (value != 0.0);
	}

	/*
	 * getPhase() will create a phase if it doesn't exist, since
	 * this method is NON-const, so only 'create' a phase if
	 * results are present.  Of course, if the phase does exist,
	 * we can always nuke the result.
	 */
	
	Entry& Entry::setResultPhasePServiceTime(const unsigned p,const double resultServiceTime)
	{
	    assert( resultServiceTime >= 0. );
	    if ( isStandardEntry() ) {
		if ( hasPhase( p ) || resultServiceTime > 0. ) {
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
		return _resultPhasePServiceTime.at(p-1);
	    }
	}

	Entry& Entry::setResultPhasePServiceTimeVariance(const unsigned p, const double resultServiceTimeVariance) 
	{
	    assert( resultServiceTimeVariance >= 0. );
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
		return _resultPhasePServiceTimeVariance.at(p-1);
	    }
	}
    
	Entry& Entry::setResultPhasePVarianceServiceTime(unsigned p, double resultVarianceServiceTime ) 
	{
	    assert( resultVarianceServiceTime >= 0. );
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
		return _resultPhasePVarianceServiceTime.at(p-1);
	    }
	}

	Entry& Entry::setResultPhasePVarianceServiceTimeVariance(unsigned p, double resultVarianceServiceTimeVariance) 
	{
	    assert( resultVarianceServiceTimeVariance >= 0. );
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
		return _resultPhasePVarianceServiceTimeVariance.at(p-1);
	    }
	}

	Entry& Entry::setResultPhasePProcessorWaiting(unsigned p, double resultProcessorWaiting ) 
	{
	    if ( isStandardEntry() ) {
		if ( hasPhase( p ) || resultProcessorWaiting > 0 ) {
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
		return _resultPhasePProcessorWaiting.at(p-1);
	    }
	}

	Entry& Entry::setResultPhasePProcessorWaitingVariance(unsigned p, double resultProcessorWaitingVariance ) 
	{
	    assert( resultProcessorWaitingVariance >= 0.0 );
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
		return _resultPhasePProcessorWaitingVariance.at(p-1);
	    }
	}
    
	Entry& Entry::setResultPhasePUtilization( unsigned p, double resultUtilization ) 
	{
	    assert( resultUtilization >= 0.0 );
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
		return _resultPhasePUtilization.at(p-1);
	    }
	}

	Entry& Entry::setResultPhasePUtilizationVariance( unsigned p, double resultUtilizationVariance ) 
	{
	    assert( resultUtilizationVariance >= 0. );
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
		return _resultPhasePUtilizationVariance.at(p-1);
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
	    return 0.;
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
	    for ( unsigned int p = Phase::MAX_PHASE; p > 0; p -= 1 ) {
		if ( _hasResultsForPhase[p-1] ) maxPhase = p;
	    }
	    return maxPhase;
	}
	
	bool Entry::hasResultsForPhase(unsigned p) const
	{
	    return _hasResultsForPhase.at(p-1);
	}
    
	bool Entry::hasResultsForOpenWait() const
	{
	    return _hasOpenWait;
	}

	bool Entry::hasResultsForThroughputBound() const
	{
	    return _hasThroughputBound;
	}
    }
}
