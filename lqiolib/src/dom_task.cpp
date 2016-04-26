/*
 *  $Id: dom_task.cpp 12458 2016-02-21 18:48:34Z greg $
 *
 *  Created by Martin Mroz on 24/02/09.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#include "dom_document.h"
#include "dom_task.h"
#include "dom_entry.h"
#include "dom_histogram.h"
#include "dom_extvar.h"
#include <cassert>
#include <iostream>
#include <cmath>
#include <algorithm>

namespace LQIO {
    namespace DOM {

	Task::Task(const Document * document, const char * task_name, const scheduling_type scheduling, const std::vector<Entry *>& entryList, 
		   const Processor* processor, ExternalVariable* queue_length, const int priority, 
		   ExternalVariable* n_copies, const int n_replicas, const Group * group, const void * task_element) 
	    : Entity(document, task_name, scheduling, n_copies, n_replicas, TYPE_TASK, task_element),
	      _entryList(entryList),
	      _queueLength(queue_length),
	      _processor(const_cast<Processor*>(processor)),
	      _priority(priority),
	      _thinkTime(NULL),
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
	    : Entity( src.getDocument(), "", src.getSchedulingType(), const_cast<LQIO::DOM::ExternalVariable*>(src.getCopies()), 1, TYPE_TASK, 0),
	      _entryList(),				/* Need to reset this. */
	      _queueLength(src.getQueueLength()),
	      _processor(),				/* Need to reset this */
	      _priority(src.getPriority()),
	      _thinkTime(src.getThinkTime()),
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
	}
    
	Task::~Task()
	{
	    /* Should delete precendence items here? */
	}
    
	/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- [Input Values] -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
    
	const std::vector<Entry*>& Task::getEntryList() const
	{
	    /* Return the top entry */
	    return _entryList;
	}
    
	unsigned int Task::getQueueLengthValue() const
	{
	    /* Retun the phase think time */
	    double value = 0.0;
	    assert(_queueLength->getValue(value) == true);
	    assert(value - floor(value) == 0);
	    return static_cast<unsigned int>(value);
	}

	ExternalVariable * Task::getQueueLength() const
	{
	    return _queueLength;
	}
    
	bool Task::hasQueueLength() const
	{
	    double value = 0.0;
	    return _queueLength && (!_queueLength->wasSet() || !_queueLength->getValue(value) || value > 0);	    /* Check whether we have it or not */
	}

	void Task::setQueueLengthValue( const unsigned int value )
	{
	    if ( _queueLength == NULL ) {
		_queueLength = new ConstantExternalVariable(value);
	    } else {
		_queueLength->set(value);
	    }
	}


	void Task::setQueueLength(ExternalVariable * queueLength)
	{
	    _queueLength = queueLength;
	}
    
	int Task::getPriority() const
	{
	    return _priority;
	}
    
	void Task::setPriority(const unsigned int priority)
	{
	    _priority = priority;
	}
    
	double Task::getThinkTimeValue() const
	{
	    /* Retun the phase think time */
	    double value;
	    if ( !_thinkTime || _thinkTime->getValue(value) != true || value < 0. ) {
		throw std::domain_error( "Invalid think time." );
	    }
	    return value;
	}

	ExternalVariable * Task::getThinkTime() const
	{
	    return _thinkTime;
	}
    
	bool Task::hasThinkTime() const
	{
	    double value = 0.0;
	    return _thinkTime && (!_thinkTime->wasSet() || !_thinkTime->getValue(value) || value > 0);	    /* Check whether we have it or not */
	}

	void Task::setThinkTimeValue( double value )
	{
	    if ( _thinkTime == NULL ) {
		_thinkTime = new ConstantExternalVariable(value);
	    } else {
		_thinkTime->set(value);
	    }
	    const_cast<Document *>(getDocument())->setTaskHasThinkTime(true);
	}

	void Task::setThinkTime(ExternalVariable * thinkTime)
	{
	    _thinkTime = thinkTime;
	    const_cast<Document *>(getDocument())->setTaskHasThinkTime(thinkTime != NULL);
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
    
	void Task::setFanOut( const std::string& task, unsigned int value )
	{
	    _fanOut[task] = value;
	}

	unsigned int Task::getFanOut( const std::string& task ) const
	{
	    std::map<const std::string,unsigned int>::const_iterator fanOut = _fanOut.find(task);
	    if ( fanOut != _fanOut.end() ) {
		return fanOut->second;
	    } else {
		return 1;
	    }
	}


	const std::map<const std::string,unsigned int>& Task::getFanOuts() const
	{
	    return _fanOut;
	}


	void Task::setFanIn( const std::string& task, unsigned int value )
	{
	    _fanIn[task] = value;
	}

	unsigned int Task::getFanIn( const std::string& task ) const
	{
	    std::map<const std::string,unsigned int>::const_iterator fanIn = _fanIn.find(task);
	    if ( fanIn != _fanIn.end() ) {
		return fanIn->second;
	    } else {
		return 1;
	    }
	}


	const std::map<const std::string,unsigned int>& Task::getFanIns() const
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
		return NULL;
	    }
	}
    
	Activity* Task::getActivity(const std::string& name, bool create )
	{
	    /* We managed to find it in the list of activities already */
	    std::map<std::string,Activity*>::const_iterator activity = _activities.find(name);
	    if ( activity != _activities.end()) {
		return activity->second;
	    } else if (create == false) {
		return NULL;
	    }
      
	    const_cast<Document *>(getDocument())->setMaximumPhase(1);	/* Set max phase for output */

	    /* Create a new one and map it into the list */
	    Activity* newActivity = new Activity(getDocument(),name.c_str());
	    addActivity( newActivity );
	    newActivity->setIndex( _activities.size() );
	    return newActivity;
	}
    
	const std::map<std::string,Activity*>& Task::getActivities() const
	{
	    return _activities;
	}
    
	void Task::addActivity( Activity * newActivity ) 
	{
	    const std::string& name = newActivity->getName();
	    _activities[name] = newActivity;
	}

	void Task::addActivityList(ActivityList * activityList)
	{
	    _precedences.insert(activityList);
	}
     
	const std::set<ActivityList*>& Task::getActivityLists() const
	{
	    return _precedences;
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
		double sum = 0;
		std::vector<Entry *>::const_iterator nextEntry;
		for ( nextEntry = _entryList.begin(); nextEntry != _entryList.end(); ++nextEntry ) {
		    sum += (*nextEntry)->getResultUtilization();
		}
		setResultUtilization( sum );
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
		double sum = 0;
		std::vector<Entry *>::const_iterator nextEntry;
		for ( nextEntry = _entryList.begin(); nextEntry != _entryList.end(); ++nextEntry ) {
		    sum += (*nextEntry)->getResultThroughput();
		}
		setResultThroughput( sum );
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
		double sum = 0.0;
		std::vector<Entry *>::const_iterator nextEntry;
		for ( nextEntry = _entryList.begin(); nextEntry != _entryList.end(); ++nextEntry ) {
		    sum += (*nextEntry)->getResultProcessorUtilization();
		}
		for ( std::map<std::string,Activity*>::const_iterator nextActivity = _activities.begin(); nextActivity != _activities.end(); ++nextActivity ) {
		    sum += nextActivity->second->getResultProcessorUtilization();
		}
		setResultProcessorUtilization( sum );
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

	void Task::Count::operator()( const std::pair<std::string,LQIO::DOM::Task *>& t ) 
	{
	    const std::vector<Entry*>& entries = t.second->getEntryList();
	    _count += for_each( entries.begin(), entries.end(), LQIO::DOM::Entry::Count( _f ) ).count();
	    
	    const std::map<std::string,Activity*>&  activities = t.second->getActivities();
	    for ( std::map<std::string,Activity*>::const_iterator next = activities.begin(); next != activities.end(); ++next ) {
		const Activity* a = next->second;
		if ( (a->*_f)() ) {
		    _count += 1;
		}
	    }
	}

	/* ------------------------------------------------------------------------ */

	SemaphoreTask::SemaphoreTask(const Document * document, const char * name, const std::vector<DOM::Entry *>& entryList, 
				     const Processor* processor, ExternalVariable* queue_length, const int priority, 
				     ExternalVariable* n_copies, const int n_replicas,
				     const Group * group, const void * task_element)
	    : Task(document, name, SCHEDULE_SEMAPHORE, entryList, processor, queue_length, priority, 
		   n_copies, n_replicas, group, task_element ),
	      _initialState(INITIALLY_FULL),
	      _resultHoldingTime(0.),
	      _resultHoldingTimeVariance(0.),
	      _resultVarianceHoldingTime(0.),
	      _resultVarianceHoldingTimeVariance(0.),
	      _resultHoldingUtilization(0.),
	      _resultHoldingUtilizationVariance(0.)
	{
	    const_cast<Document *>(document)->setHasSemaphoreWait(true);
	}

	void SemaphoreTask::setInitialState(InitialStateType state)
	{
	    /* Set the number of initialTokens */
	    _initialState = state;
	}
    
	const SemaphoreTask::InitialStateType SemaphoreTask::getInitialState() const
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
	    return _histogram && _histogram->isMaxServiceTime(); 
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
	    if ( _histogram && _histogram->isMaxServiceTime() ) {
		return _histogram->getBinMean( _histogram->getOverflowIndex() );
	    } else {
		return 0;
	    }
	}

	double SemaphoreTask::getResultMaxServiceTimeExceededVariance() const
	{
	    if ( _histogram && _histogram->isMaxServiceTime() ) {
		return _histogram->getBinVariance( _histogram->getOverflowIndex() );
	    } else {
		return 0;
	    }
	}

	RWLockTask::RWLockTask(const Document * document, const char * name, const std::vector<DOM::Entry *>& entryList, 
			       const Processor* processor, ExternalVariable* queue_length, const int priority, 
			       ExternalVariable* n_copies, const int n_replicas,
			       const Group * group, const void * task_element)
	    : Task(document, name, SCHEDULE_RWLOCK, entryList, processor, 
		   queue_length, priority, n_copies, n_replicas, group,  task_element ),
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
	    const_cast<Document *>(document)->setHasReaderWait(true);
	    //const_cast<Document *>(document)->setHasWriterWait(true);
	}
    }
}
