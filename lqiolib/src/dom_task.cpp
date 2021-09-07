/*
 *  $Id: dom_task.cpp 14955 2021-09-07 16:52:38Z greg $
 *
 *  Created by Martin Mroz on 24/02/09.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#include <cassert>
#include <algorithm>
#include <numeric>
#include <iostream>
#include <cmath>
#include "dom_document.h"
#include "dom_task.h"
#include "dom_entry.h"
#include "dom_histogram.h"
#include "dom_extvar.h"

namespace LQIO {
    namespace DOM {

	const char * Task::__typeName = "task";

	Task::Task(const Document * document, const std::string& task_name, const scheduling_type scheduling, const std::vector<Entry *>& entryList,
		   const Processor* processor, const ExternalVariable* queue_length, const ExternalVariable * priority,
		   const ExternalVariable* n_copies, const ExternalVariable* n_replicas, const Group * group )
	    : Entity(document, task_name, scheduling, n_copies, n_replicas ),
	      _entryList(entryList),
	      _queueLength(queue_length),
	      _processor(const_cast<Processor*>(processor)),
	      _priority(priority),
	      _thinkTime(nullptr),
	      _group(const_cast<Group *>(group)),
	      _activities(), _precedences(),
	      _fanOut(), _fanIn(),
	      _resultPhaseCount(0),
	      _resultProcUtilization(0.0), _resultProcUtilizationVariance(0.0),
	      _resultThroughput(0.0), _resultThroughputVariance(0.0),
	      _resultUtilization(0.0), _resultUtilizationVariance(0.0),
	      _resultBottleneckStrength(0.0)
	{
	    /* Associate the entries with tasks */
	    std::vector<Entry *>::const_iterator nextEntry;
	    for ( nextEntry = entryList.begin(); nextEntry != entryList.end(); ++nextEntry ) {
		(*nextEntry)->setTask(this);
	    }

	    for ( unsigned int p = 0; p < Phase::MAX_PHASE; ++p ) {
		_resultPhaseUtilizations[p] = 0;
		_resultPhaseUtilizationVariances[p] = 0;
	    }
	}

	Task::Task( const Task& src )
	    : Entity( src.getDocument(), "", src.getSchedulingType(), src.getCopies(), src.getReplicas()),
	      _entryList(),				/* Need to reset this. */
	      _queueLength(ExternalVariable::clone(src._queueLength)),
	      _processor(),				/* Need to reset this */
	      _priority(ExternalVariable::clone(src._priority)),
	      _thinkTime(ExternalVariable::clone(src._thinkTime)),
	      _group(),					/* Need to reset this */
	      _activities(), _precedences(),
	      _fanOut(), _fanIn(),
	      _resultPhaseCount(0),
	      _resultProcUtilization(0.0), _resultProcUtilizationVariance(0.0),
	      _resultThroughput(0.0), _resultThroughputVariance(0.0),
	      _resultUtilization(0.0), _resultUtilizationVariance(0.0)
	{
	    for ( unsigned int p = 0; p < Phase::MAX_PHASE; ++p ) {
		_resultPhaseUtilizations[p] = 0;
		_resultPhaseUtilizationVariances[p] = 0;
	    }
	    for ( std::map<const std::string, const LQIO::DOM::ExternalVariable *>::iterator fan_out = _fanOut.begin(); fan_out != _fanOut.end(); ++fan_out ) {
		fan_out->second = ExternalVariable::clone(fan_out->second);
	    }
	    for ( std::map<const std::string, const LQIO::DOM::ExternalVariable *>::iterator fan_in = _fanIn.begin(); fan_in != _fanIn.end(); ++fan_in ) {
		fan_in->second = ExternalVariable::clone(fan_in->second);
	    }
	}

	Task::~Task()
	{
	    deleteActivities();
	    deleteActivityLists();
	}

	/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- [Input Values] -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */

	const std::vector<Entry*>& Task::getEntryList() const
	{
	    /* Return the top entry */
	    return _entryList;
	}

	unsigned int Task::getQueueLengthValue() const
	{
	    return getIntegerValue( getQueueLength(), 0 );
	}

	const ExternalVariable * Task::getQueueLength() const
	{
	    return _queueLength;
	}

	bool Task::hasQueueLength() const
	{
	    return ExternalVariable::isPresent( getQueueLength(), 0.0 );
	}

	void Task::setQueueLengthValue( const unsigned int value )
	{
	    if ( _queueLength == nullptr ) {
		_queueLength = new ConstantExternalVariable(value);
	    } else {
		const_cast<ExternalVariable *>(_queueLength)->set(value);
	    }
	}


	void Task::setQueueLength(const ExternalVariable * queueLength)
	{
	    _queueLength = checkIntegerVariable( queueLength, 0 );
	}

	int Task::getPriorityValue() const
	{
	    return getIntegerValue( getPriority(), 0 );
	}
	
	const ExternalVariable * Task::getPriority() const
	{
	    return _priority;
	}

	bool Task::hasPriority() const
	{
	    return ExternalVariable::isPresent( getPriority(), 0 );
	}

	void Task::setPriority(const ExternalVariable * priority)
	{
	    _priority = priority;
	}

	void Task::setPriorityValue( int value )
	{
	    if ( _priority == nullptr ) {
		_priority = new ConstantExternalVariable(value);
	    } else {
		const_cast<ExternalVariable *>(_priority)->set(value);
	    }
	}

	double Task::getThinkTimeValue() const
	{
	    return getDoubleValue( getThinkTime(), 0. );
	}

	const ExternalVariable * Task::getThinkTime() const
	{
	    return _thinkTime;
	}

	bool Task::hasThinkTime() const
	{
	    return ExternalVariable::isPresent( getThinkTime() );
	}

	void Task::setThinkTimeValue( double value )
	{
	    if ( _thinkTime == nullptr ) {
		_thinkTime = new ConstantExternalVariable(value);
	    } else {
		const_cast<ExternalVariable *>(_thinkTime)->set(value);
	    }
	}

	void Task::setThinkTime(const ExternalVariable * thinkTime)
	{
	    _thinkTime = checkDoubleVariable( thinkTime, 0.0 );
	}

	const Group* Task::getGroup() const
	{
	    return _group;
	}

	void Task::setGroup( Group* group )
	{
	    _group = group;
	}

	const Processor* Task::getProcessor() const
	{
	    return _processor;
	}

	void Task::setProcessor( Processor* processor )
	{
	    _processor = processor;
	}

	void Task::setFanOut( const std::string& task, const ExternalVariable *value )
	{
	    _fanOut[task] = checkIntegerVariable( value, 1 );
	}

	void Task::setFanOutValue( const std::string& task, unsigned int value )
	{
	    if ( _fanOut[task] == nullptr ) {
		_fanOut[task] = new ConstantExternalVariable(value);
	    } else {
		const_cast<ExternalVariable *>(_fanOut[task])->set(value);
	    }
	}

	const ExternalVariable * Task::getFanOut( const std::string& task ) const
	{
	    std::map<const std::string,const ExternalVariable *>::const_iterator fanOut = _fanOut.find(task);
	    if ( fanOut != _fanOut.end() ) {
		return fanOut->second;
	    } else {
		return nullptr;
	    }
	}

	unsigned int Task::getFanOutValue( const std::string& task ) const
	{
	    return getIntegerValue( getFanOut( task ), 1 );
	}


	const std::map<const std::string,const ExternalVariable *>& Task::getFanOuts() const
	{
	    return _fanOut;
	}


	void Task::setFanIn( const std::string& task, const ExternalVariable * value )
	{
	    _fanIn[task] = checkIntegerVariable( value, 1 );
	}

	void Task::setFanInValue( const std::string& task, unsigned int value )
	{
	    if ( _fanIn[task] == nullptr ) {
		_fanIn[task] = new ConstantExternalVariable(value);
	    } else {
		const_cast<ExternalVariable *>(_fanIn[task])->set(value);
	    }
	}

	const ExternalVariable * Task::getFanIn( const std::string& task ) const
	{
	    std::map<const std::string,const ExternalVariable *>::const_iterator fanIn = _fanIn.find(task);
	    if ( fanIn != _fanIn.end() ) {
		return fanIn->second;
	    } else {
		return nullptr;
	    }
	}

	unsigned int Task::getFanInValue( const std::string& task ) const
	{
	    return getIntegerValue( getFanIn( task ), 1 );
	}

	const std::map<const std::string,const ExternalVariable *>& Task::getFanIns() const
	{
	    return _fanIn;
	}


	Activity* Task::getActivity(const std::string& name ) const
	{
	    /* We managed to find it in the list of activities already */
	    std::map<std::string,Activity*>::const_iterator activity = _activities.find(name);
	    if ( activity != _activities.end()) {
		return activity->second;
	    } else {
		return nullptr;
	    }
	}

	Activity* Task::getActivity(const std::string& name, bool create )
	{
	    /* We managed to find it in the list of activities already */
	    std::map<std::string,Activity*>::const_iterator activity = _activities.find(name);
	    if ( activity != _activities.end()) {
		return activity->second;
	    } else if (create == false) {
		return nullptr;
	    }

	    const_cast<Document *>(getDocument())->setMaximumPhase(1);	/* Set max phase for output */

	    /* Create a new one and map it into the list */
	    Activity* newActivity = new Activity(getDocument(),name);
	    addActivity( newActivity );
	    newActivity->setTask( this );
	    return newActivity;
	}

	const std::map<std::string,Activity*>& Task::getActivities() const
	{
	    return _activities;
	}

	void Task::deleteActivities()
	{
	    for ( std::map<std::string,Activity*>::iterator activity = _activities.begin(); activity != _activities.end(); ++activity ) {
		delete activity->second;
	    }
	    _activities.clear();
	}
		
	void Task::addActivity( Activity * newActivity )
	{
	    const std::string& name = newActivity->getName();
	    _activities[name] = newActivity;
	}

	Activity * Task::removeActivity( Activity * activity )
	{
	    const std::string& name = activity->getName();
	    std::map<std::string,Activity*>::iterator i = _activities.find(name);
	    if ( i != _activities.end() ) {
		_activities.erase(i);
		return activity;
	    } else {
		return nullptr;
	    }
	}
	
	void Task::addActivityList(ActivityList * activityList)
	{
	    _precedences.insert(activityList);
	}

	ActivityList * Task::removeActivityList(ActivityList * activityList)
	{
	    std::set<ActivityList *>::iterator i = _precedences.find(activityList);
	    if ( i != _precedences.end() ) {
		_precedences.erase(i);
		return activityList;
	    } else {
		return nullptr;
	    }
	}
	
	const std::set<ActivityList*>& Task::getActivityLists() const
	{
	    return _precedences;
	}

	void Task::deleteActivityLists()
	{
	    for ( std::set<ActivityList*>::const_iterator precedence = _precedences.begin(); precedence != _precedences.end(); ++precedence ) {
		delete *precedence;
	    }
	    _precedences.clear();
	}

	bool Task::hasAndJoinActivityList() const
	{
	    const std::set<ActivityList*>& list = getActivityLists();
	    for ( std::set<ActivityList*>::const_iterator precedence = list.begin(); precedence != list.end(); ++precedence ) {
		if ( dynamic_cast<AndJoinActivityList *>(*precedence ) ) return true;
	    }
	    return false;
	}

	/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- [Result Values] -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

	unsigned Task::getResultPhaseCount() const
	{
	    return _resultPhaseCount;
	}

	Task& Task::setResultPhaseUtilizations(unsigned int count, double * resultPhaseUtilizations)
	{
	    assert( 0 < count && count <= Phase::MAX_PHASE );
	    for ( unsigned int p = 0; p < count; ++p ) {
		_resultPhaseUtilizations[p] = resultPhaseUtilizations[p];
		if ( resultPhaseUtilizations[p] > 0.0 && p+1 > _resultPhaseCount ) {
		    _resultPhaseCount = p+1;
		    const_cast<Document *>(getDocument())->setMaximumPhase(p+1);
		}
	    }
	    return *this;
	}

	Task& Task::setResultPhasePUtilization(unsigned int p, const double resultPhasePUtilization)
	{
	    assert( 0 < p && p <= Phase::MAX_PHASE );
	    _resultPhaseUtilizations[p-1] = resultPhasePUtilization;
	    if ( _resultPhaseUtilizations[p-1] > 0.0 && p > _resultPhaseCount ) {
		_resultPhaseCount = p;
		const_cast<Document *>(getDocument())->setMaximumPhase(p);
	    }
	    return *this;
	}

	double Task::getResultPhasePUtilization( const unsigned p ) const
	{
	    assert( 0 < p && p <= Phase::MAX_PHASE );
	    return _resultPhaseUtilizations[p-1];
	}

	double Task::getResultPhasePUtilizationVariance( const unsigned p ) const
	{
	    assert( 0 < p && p <= Phase::MAX_PHASE );
	    return _resultPhaseUtilizationVariances[p-1];
	}

	Task& Task::setResultPhaseUtilizationVariances(unsigned int count, double* resultPhaseUtilizationVariances)
	{
	    assert( 0 < count && count <= Phase::MAX_PHASE );
	    for ( unsigned int i = 0; i < count; ++i ) {
		_resultPhaseUtilizationVariances[i] = resultPhaseUtilizationVariances[i];
	    }
	    return *this;
	}

	Task& Task::setResultPhasePUtilizationVariance(unsigned int p, const double resultPhasePUtilizationVariance)
	{
	    assert( 0 < p && p <= Phase::MAX_PHASE );
	    _resultPhaseUtilizationVariances[p-1] = resultPhasePUtilizationVariance;
	    return *this;
	}

	double Task::getResultUtilization() const
	{
	    /* Returns the ResultUtilization of the Task */
	    return _resultUtilization;
	}

	double Task::computeResultUtilization()
	{
	    if ( getResultUtilization() == 0 || _entryList.size() == 1 ) {
		setResultUtilization( std::accumulate( _entryList.begin(), _entryList.end(), 0.0, add_using_const<Entry>( &Entry::getResultUtilization ) ) );
		setResultUtilizationVariance( std::accumulate( _entryList.begin(), _entryList.end(), 0.0, add_using_const<Entry>( &Entry::getResultUtilizationVariance ) ) );

		for ( unsigned int p = 1; p <= Phase::MAX_PHASE; ++p ) {
		    setResultPhasePUtilization( p, std::accumulate( _entryList.begin(), _entryList.end(), 0.0, Entry::add_phase_using( &Entry::getResultPhasePUtilization, p ) ) );
		    setResultPhasePUtilizationVariance( p, std::accumulate( _entryList.begin(), _entryList.end(), 0.0, Entry::add_phase_using( &Entry::getResultPhasePUtilizationVariance, p ) ) );
		}
	    }
	    return getResultUtilization();
	}


	Task& Task::setResultUtilization(const double resultUtilization)
	{
	    /* Stores the given ResultUtilization of the Task */
	    _resultUtilization = resultUtilization;
	    return *this;
	}

	double Task::getResultUtilizationVariance() const
	{
	    /* Stores the given ResultUtilization of the Task */
	    return _resultUtilizationVariance;
	}

	Task& Task::setResultUtilizationVariance(const double resultUtilizationVariance)
	{
	    /* Stores the given ResultUtilization of the Task */
	    if ( resultUtilizationVariance > 0 ) {
		const_cast<Document *>(getDocument())->setResultHasConfidenceIntervals(true);
	    }
	    _resultUtilizationVariance = resultUtilizationVariance;
	    return *this;
	}

	double Task::getResultThroughput() const
	{
	    /* Returns the ResultThroughput of the Task */
	    return _resultThroughput;
	}

	Task& Task::setResultThroughput(const double resultThroughput)
	{
	    /* Stores the given ResultThroughput of the Task */
	    _resultThroughput = resultThroughput;
	    return *this;
	}

	double Task::computeResultThroughput()
	{
	    if ( getResultThroughput() == 0 || _entryList.size() == 1 ) {
		setResultThroughput( std::accumulate( _entryList.begin(),_entryList.end(), 0.0, add_using_const<Entry>( &Entry::getResultThroughput ) ) );
		setResultThroughputVariance( std::accumulate( _entryList.begin(),_entryList.end(), 0.0, add_using_const<Entry>( &Entry::getResultThroughputVariance ) ) );
	    }
	    return getResultThroughput();
	}


	Task& Task::setResultThroughputVariance(const double resultThroughputVariance)
	{
	    /* Stores the given ResultThroughput of the Task */
	    if ( resultThroughputVariance ) {
		const_cast<Document *>(getDocument())->setResultHasConfidenceIntervals(true);	
	    }
	    _resultThroughputVariance = resultThroughputVariance;
	    return *this;
	}

	double Task::getResultThroughputVariance() const
	{
	    /* Stores the given ResultThroughput of the Task */
	    return _resultThroughputVariance;
	}

	double Task::getResultProcessorUtilization() const
	{
	    /* Returns the ResultProcessorUtilization of the Class */
	    return _resultProcUtilization;
	}

	double Task::computeResultProcessorUtilization()
	{
	    if ( getResultProcessorUtilization() == 0.0 || _entryList.size() == 1 ) {
		setResultProcessorUtilization( std::accumulate( _entryList.begin(), _entryList.end(), 0.0, add_using_const<Entry>( &Entry::getResultProcessorUtilization ) )
					       + std::accumulate( _activities.begin(), _activities.end(), 0.0, add_using_const<Activity>( &Activity::getResultProcessorUtilization ) ) );
		setResultProcessorUtilizationVariance( std::accumulate( _entryList.begin(), _entryList.end(), 0.0, add_using_const<Entry>( &Entry::getResultProcessorUtilizationVariance ) ) );
	    }
	    return getResultProcessorUtilization();
	}

	Task& Task::setResultProcessorUtilization(const double resultProcessorUtilization)
	{
	    /* Stores the given ResultProcessorUtilization of the Class */
	    _resultProcUtilization = resultProcessorUtilization;
	    return *this;
	}

	double Task::getResultProcessorUtilizationVariance() const
	{
	    /* Stores the given ResultProcessorUtilization of the Class */
	    return _resultProcUtilizationVariance;
	}

	Task& Task::setResultProcessorUtilizationVariance(const double resultProcessorUtilizationVariance)
	{
	    /* Stores the given ResultProcessorUtilization of the Class */
	    if ( resultProcessorUtilizationVariance > 0 ) {
		const_cast<Document *>(getDocument())->setResultHasConfidenceIntervals(true);
	    }
	    _resultProcUtilizationVariance = resultProcessorUtilizationVariance;
	    return *this;
	}

	double Task::getResultBottleneckStrength() const
	{
	    return _resultBottleneckStrength;
	}

	Task& Task::setResultBottleneckStrength( const double resultBottleneckStrength )
	{
	    /* Stores the given ResultProcessorUtilization of the Class */
	    if ( resultBottleneckStrength > 0 ) {
		const_cast<Document *>(getDocument())->setResultHasBottleneckStrength(true);
	    }
	    _resultBottleneckStrength = resultBottleneckStrength;
	    return *this;
	}

	/* ------------------------------------------------------------------------ */

	/* 
	 * Return true if any phase or activity satisties the predicate _f.
	 */
	
	bool Task::any_of::operator()( const std::pair<std::string,LQIO::DOM::Task *>& t ) const
	{
	    const std::vector<Entry*>& entries = t.second->getEntryList();
	    const std::map<std::string,Activity*>&  activities = t.second->getActivities();
	    return std::any_of( entries.begin(), entries.end(), LQIO::DOM::Entry::any_of( _f ) )
		|| std::any_of( activities.begin(), activities.end(), LQIO::DOM::DocumentObject::Predicate<LQIO::DOM::Activity>( _f ) );
	}

	/* ------------------------------------------------------------------------ */

	SemaphoreTask::SemaphoreTask(const Document * document, const char * name, const std::vector<DOM::Entry *>& entryList,
				     const Processor* processor, const ExternalVariable* queue_length, const ExternalVariable * priority,
				     const ExternalVariable* n_copies, const ExternalVariable* n_replicas,
				     const Group * group)
	    : Task(document, name, SCHEDULE_SEMAPHORE, entryList, processor, queue_length, priority,
		   n_copies, n_replicas, group ),
	      _initialState(InitialState::FULL),
	      _resultHoldingTime(0.),
	      _resultHoldingTimeVariance(0.),
	      _resultVarianceHoldingTime(0.),
	      _resultVarianceHoldingTimeVariance(0.),
	      _resultHoldingUtilization(0.),
	      _resultHoldingUtilizationVariance(0.),
	      _histogram(nullptr)
	{
	}

	SemaphoreTask::~SemaphoreTask()
	{
	    if ( _histogram != nullptr ) delete _histogram;
	}

	void SemaphoreTask::setInitialState(InitialState state)
	{
	    /* Set the number of initialTokens */
	    _initialState = state;
	}

	const SemaphoreTask::InitialState SemaphoreTask::getInitialState() const
	{
	    return _initialState;
	}


	bool SemaphoreTask::hasHistogram() const
	{
	    return _histogram != 0 && _histogram->getBins() > 0;
	}

	void SemaphoreTask::setHistogram(Histogram* histogram)
	{
	    /* Stores the given Histogram of the Phase */
	    _histogram = histogram;
	}

	bool SemaphoreTask::hasMaxServiceTimeExceeded() const
	{
	    return _histogram && _histogram->isTimeExceeded();
	}

	double SemaphoreTask::getMaxServiceTime() const
	{
	    if ( hasMaxServiceTimeExceeded() ) {
		return getHistogram()->getMax();
	    } else {
		return 0.0;
	    }
	}

	double SemaphoreTask::getResultMaxServiceTimeExceeded() const
	{
	    if ( _histogram && _histogram->isTimeExceeded() ) {
		return _histogram->getBinMean( _histogram->getOverflowIndex() );
	    } else {
		return 0.0;
	    }
	}

	double SemaphoreTask::getResultMaxServiceTimeExceededVariance() const
	{
	    if ( _histogram && _histogram->isTimeExceeded() ) {
		return _histogram->getBinVariance( _histogram->getOverflowIndex() );
	    } else {
		return 0.0;
	    }
	}

	RWLockTask::RWLockTask(const Document * document, const char * name, const std::vector<DOM::Entry *>& entryList,
			       const Processor* processor, const ExternalVariable* queue_length, const ExternalVariable * priority,
			       const ExternalVariable* n_copies, const ExternalVariable* n_replicas,
			       const Group * group )
	    : Task(document, name, SCHEDULE_RWLOCK, entryList, processor,
		   queue_length, priority, n_copies, n_replicas, group ),
	      _resultReaderHoldingTime(0.),
	      _resultReaderHoldingTimeVariance(0.),
	      _resultVarianceReaderHoldingTime(0.),
	      _resultVarianceReaderHoldingTimeVariance(0.),
	      _resultWriterHoldingTime(0.),
	      _resultWriterHoldingTimeVariance(0.),
	      _resultVarianceWriterHoldingTime(0.),
	      _resultVarianceWriterHoldingTimeVariance(0.),
	      _resultReaderHoldingUtilization(0.),
	      _resultReaderHoldingUtilizationVariance(0.),
	      _resultWriterHoldingUtilization(0.),
	      _resultWriterHoldingUtilizationVariance(0.),
	      _resultReaderBlockedTime(0.),
	      _resultReaderBlockedTimeVariance(0.),
	      _resultVarianceReaderBlockedTime(0.),
	      _resultVarianceReaderBlockedTimeVariance(0.),
	      _resultWriterBlockedTime(0.),
	      _resultWriterBlockedTimeVariance(0.),
	      _resultVarianceWriterBlockedTime(0.),
	      _resultVarianceWriterBlockedTimeVariance(0.)

	{
	}
    }
}
