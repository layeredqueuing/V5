/* -*- c++ -*-
 *  $Id: dom_task.h 14387 2021-01-21 14:09:16Z greg $
 *
 *  Created by Martin Mroz on 24/02/09.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#ifndef __LQIO_DOM_TASK__
#define __LQIO_DOM_TASK__

#include <set>
#include <map>
#include "dom_entity.h"
#include "dom_call.h"
#include "dom_phase.h"

namespace LQIO {
    namespace DOM {

	class Activity;
	class ActivityList;
	class ExternalVariable;
	class Processor;
	class Task;
	class Entry;
	class Group;
        class Decision;

	class Task : public Entity {
	public:
	    class any_of {
	    protected:
		typedef bool (LQIO::DOM::Phase::*test_fn)() const;
		
	    public:
		any_of( test_fn f ) : _f(f) {}
		
		bool operator()( const std::pair<std::string,LQIO::DOM::Task *>& ) const;

	    private:
		const test_fn _f;
	    };

	    static bool isSemaphoreTask( const std::pair<std::string,LQIO::DOM::Task *>& task ) { return task.second->getSchedulingType() == SCHEDULE_SEMAPHORE; }
	    static bool isRWLockTask( const std::pair<std::string,LQIO::DOM::Task *>& task ) { return task.second->getSchedulingType() == SCHEDULE_RWLOCK; }

	public:
	    /* Designated initializer for the Task entity */
	    Task(const Document * document, const std::string& name, const scheduling_type scheduling,
		 const std::vector<DOM::Entry *>& entryList,
		 const Processor* processor=nullptr, const ExternalVariable* queue_length=nullptr, const ExternalVariable * priority=nullptr,
		 const ExternalVariable* n_copies=nullptr, const ExternalVariable* n_replicas=nullptr,
		 const Group * group=nullptr );

	    Task( const Task& );
	    virtual ~Task();

	    /* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- [Input Values] -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */

	    /* Accessors and Mutators */
	    const char * getTypeName() const { return __typeName; }

	    const std::vector<Entry*>& getEntryList() const;

	    /* Variable Accessors and Mutators */
	    unsigned int getQueueLengthValue() const;
	    const ExternalVariable * getQueueLength() const;
	    void setQueueLengthValue(const unsigned int queueLength);
	    void setQueueLength(const ExternalVariable * queueLength);
	    bool hasQueueLength() const;
	    int getPriorityValue() const;
	    const ExternalVariable * getPriority() const;
	    void setPriority(const ExternalVariable *);
	    void setPriorityValue( int );
	    bool hasPriority() const;
	    double getThinkTimeValue() const;
	    const ExternalVariable * getThinkTime() const;
	    void setThinkTime(const ExternalVariable * thinkTime);
	    void setThinkTimeValue( double value );
	    bool hasThinkTime() const;
	    void setFanOut( const std::string&, const ExternalVariable * );
	    void setFanOutValue( const std::string&, unsigned int );
	    const ExternalVariable * getFanOut( const std::string& ) const;
	    unsigned int getFanOutValue( const std::string& ) const;
	    const std::map<const std::string,const ExternalVariable *>& getFanOuts() const;
	    void setFanIn( const std::string&, const ExternalVariable * );
	    void setFanInValue( const std::string&, unsigned int );
	    const ExternalVariable * getFanIn( const std::string& ) const;
	    unsigned int getFanInValue( const std::string& ) const;
	    const std::map<const std::string,const ExternalVariable *>& getFanIns() const;

	    /* Access to the "constant" elements */
	    void setProcessor( Processor * );		// Used for cloning only.
	    const Processor* getProcessor() const;
	    void setGroup( Group * );			// Used for cloning only.
	    const Group* getGroup() const;

	    /* Accessors and Mutators for Activities */
	    Activity* getActivity(const std::string& name) const;
	    Activity* getActivity(const std::string& name, bool create );
	    const std::map<std::string,Activity*>& getActivities() const;
	    void addActivity( Activity * newActivity );
	    Activity * removeActivity( Activity * );
	    void deleteActivities();
	    void addActivityList(ActivityList *);
	    ActivityList * removeActivityList( ActivityList * );
	    const std::set<ActivityList*>& getActivityLists() const;
	    void deleteActivityLists();
	    bool hasAndJoinActivityList() const;


	    /* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- [Result Values] -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

	    /* Storing the Result Parameters */
	    unsigned getResultPhaseCount() const;
	    double getResultPhasePUtilization( const unsigned ) const;
	    double getResultPhase1Utilization() const { return getResultPhasePUtilization(1); }
	    double getResultPhase2Utilization() const { return getResultPhasePUtilization(2); }
	    double getResultPhase3Utilization() const { return getResultPhasePUtilization(3); }
	    virtual Task& setResultPhaseUtilizations(unsigned int count, double* resultPhaseUtilizations);
	    virtual Task& setResultPhasePUtilization(unsigned int phase, const double resultPhasePUtilization);
	    virtual Task& setResultPhase1Utilization(const double resultPhasePUtilization) { return setResultPhasePUtilization(1,resultPhasePUtilization); }
	    virtual Task& setResultPhase2Utilization(const double resultPhasePUtilization) { return setResultPhasePUtilization(2,resultPhasePUtilization); }
	    virtual Task& setResultPhase3Utilization(const double resultPhasePUtilization) { return setResultPhasePUtilization(3,resultPhasePUtilization); }
	    double getResultPhasePUtilizationVariance( const unsigned ) const;
	    double getResultPhase1UtilizationVariance() const { return getResultPhasePUtilizationVariance(1); }
	    double getResultPhase2UtilizationVariance() const { return getResultPhasePUtilizationVariance(2); }
	    double getResultPhase3UtilizationVariance() const { return getResultPhasePUtilizationVariance(3); }
	    virtual Task& setResultPhaseUtilizationVariances(unsigned int count, double* resultPhaseUtilizationsVariance);
	    virtual Task& setResultPhase1UtilizationVariance(const double resultPhasePUtilization) { return setResultPhasePUtilizationVariance(1,resultPhasePUtilization); }
	    virtual Task& setResultPhase2UtilizationVariance(const double resultPhasePUtilization) { return setResultPhasePUtilizationVariance(2,resultPhasePUtilization); }
	    virtual Task& setResultPhase3UtilizationVariance(const double resultPhasePUtilization) { return setResultPhasePUtilizationVariance(3,resultPhasePUtilization); }
	    virtual Task& setResultPhasePUtilizationVariance(unsigned int phase, const double resultPhasePUtilization);
	    double getResultUtilization() const;
	    double computeResultUtilization();
	    virtual Task& setResultUtilization(const double resultUtilization);
	    double getResultUtilizationVariance() const;
	    virtual Task& setResultUtilizationVariance(const double resultUtilizationVariance);
	    double getResultThroughput() const;
	    virtual Task& setResultThroughput(const double resultThroughput);
	    double getResultThroughputVariance() const;
	    double computeResultThroughput();
	    virtual Task& setResultThroughputVariance(double resultThroughputVariance);
	    double getResultProcessorUtilization() const;
	    double computeResultProcessorUtilization();
	    virtual Task& setResultProcessorUtilization(const double resultProcessorUtilization);
	    double getResultProcessorUtilizationVariance() const;
	    virtual Task& setResultProcessorUtilizationVariance(const double resultProcessorUtilizationVariance);
	    double getResultBottleneckStrength() const;
	    virtual Task& setResultBottleneckStrength( const double resultBottleneckStrength );

	private:
	    Task& operator=( const Task& );

	private:

	    /* Input Variables from the Document */
	    std::vector<Entry*> _entryList;
	    const ExternalVariable * _queueLength;
	    Processor* _processor;
	    const ExternalVariable * _priority;
	    const ExternalVariable * _thinkTime;
	    Group * _group;
  	
	    /* Variables for Activities */
	    std::map<std::string,Activity*> _activities;
	    std::set<ActivityList *> _precedences;

	    /* Variables for replication */
	    std::map<const std::string, const LQIO::DOM::ExternalVariable *> _fanOut;
	    std::map<const std::string, const LQIO::DOM::ExternalVariable *> _fanIn;
  	
	    /* Computation Results from LQNS */
	    unsigned int _resultPhaseCount;
	    double _resultPhaseUtilizations[Phase::MAX_PHASE];
	    double _resultPhaseUtilizationVariances[Phase::MAX_PHASE];
	    double _resultProcUtilization;
	    double _resultProcUtilizationVariance;
	    double _resultThroughput;
	    double _resultThroughputVariance;
	    double _resultUtilization;
	    double _resultUtilizationVariance;
	    double _resultBottleneckStrength;

	public:
	    static const char * __typeName;
	};

	class SemaphoreTask : public Task {
	public:
	    /* Different types of calls */
	    enum class InitialState { EMPTY, FULL };

	    SemaphoreTask(const Document * document, const char * name, const std::vector<DOM::Entry *>& entryList,
			  const Processor* processor, const ExternalVariable* queue_length=nullptr, const ExternalVariable * priority=nullptr,
			  const ExternalVariable* n_copies=nullptr, const ExternalVariable* n_replicas=nullptr,
			  const Group * group=nullptr );
	    SemaphoreTask( const SemaphoreTask& );
	    virtual ~SemaphoreTask();

	    const InitialState getInitialState() const;
	    void setInitialState(InitialState);

	    virtual double getResultHoldingTime() const { return _resultHoldingTime; }
	    virtual SemaphoreTask& setResultHoldingTime( const double resultHoldingTime ) { _resultHoldingTime = resultHoldingTime; return *this; }
	    virtual double getResultHoldingTimeVariance() const { return _resultHoldingTimeVariance; }
	    virtual SemaphoreTask& setResultHoldingTimeVariance( const double resultHoldingTimeVariance ) { _resultHoldingTimeVariance = resultHoldingTimeVariance; return *this; }
	    virtual double getResultVarianceHoldingTime() const { return _resultVarianceHoldingTime; }
	    virtual SemaphoreTask& setResultVarianceHoldingTime( const double resultVarianceHoldingTime ) { _resultVarianceHoldingTime = resultVarianceHoldingTime; return *this; }
	    virtual double getResultVarianceHoldingTimeVariance() const { return _resultVarianceHoldingTimeVariance; }
	    virtual SemaphoreTask& setResultVarianceHoldingTimeVariance( const double resultVarianceHoldingTimeVariance) { _resultVarianceHoldingTimeVariance = resultVarianceHoldingTimeVariance; return *this; }
	    virtual double getResultHoldingUtilization() const { return _resultHoldingUtilization; }
	    virtual SemaphoreTask& setResultHoldingUtilization( const double resultHoldingUtilization ) { _resultHoldingUtilization = resultHoldingUtilization; return *this; }
	    virtual double getResultHoldingUtilizationVariance() const { return _resultHoldingUtilizationVariance; }
	    virtual SemaphoreTask& setResultHoldingUtilizationVariance( const double resultHoldingUtilizationVariance) { _resultHoldingUtilizationVariance = resultHoldingUtilizationVariance; return *this; }
	    virtual bool hasHistogram() const;
	    virtual const Histogram* getHistogram() const { return _histogram; }
	    virtual void setHistogram(Histogram* histogram);
	    virtual bool hasMaxServiceTimeExceeded() const;
	    virtual double getMaxServiceTime() const;
	    virtual double getResultMaxServiceTimeExceeded() const;
	    virtual double getResultMaxServiceTimeExceededVariance() const;

	private:
	    InitialState _initialState;

	    double _resultHoldingTime;
	    double _resultHoldingTimeVariance;
	    double _resultVarianceHoldingTime;
	    double _resultVarianceHoldingTimeVariance;
	    double _resultHoldingUtilization;
	    double _resultHoldingUtilizationVariance;
	    Histogram* _histogram;
	};

	class RWLockTask : public Task {
	public:
	
	    RWLockTask(const Document * document, const char * name, const std::vector<DOM::Entry *>& entryList,
		       const Processor* processor, const ExternalVariable* queue_length=nullptr, const ExternalVariable * priority=nullptr,
		       const ExternalVariable* n_copies=nullptr, const ExternalVariable* n_replicas=nullptr,
		       const Group * group=nullptr );
	    //  n_copies is the number of concurrent readers

	    RWLockTask( const RWLockTask& );

	    /* rwlock holding time */
	    virtual double getResultReaderHoldingTime() const { return _resultReaderHoldingTime; }
	    virtual double getResultWriterHoldingTime() const { return _resultWriterHoldingTime; }
	    virtual RWLockTask& setResultReaderHoldingTime( const double resultReaderHoldingTime ) { _resultReaderHoldingTime = resultReaderHoldingTime; return *this; }
	    virtual RWLockTask& setResultWriterHoldingTime( const double resultWriterHoldingTime ) { _resultWriterHoldingTime = resultWriterHoldingTime; return *this; }
	    virtual double getResultReaderHoldingTimeVariance() const { return _resultReaderHoldingTimeVariance; }
	    virtual double getResultWriterHoldingTimeVariance() const { return _resultWriterHoldingTimeVariance; }
	    virtual RWLockTask& setResultReaderHoldingTimeVariance( const double resultReaderHoldingTimeVariance ) { _resultReaderHoldingTimeVariance = resultReaderHoldingTimeVariance; return *this; }
	    virtual RWLockTask& setResultWriterHoldingTimeVariance( const double resultWriterHoldingTimeVariance ) { _resultWriterHoldingTimeVariance = resultWriterHoldingTimeVariance; return *this; }
	    virtual double getResultVarianceReaderHoldingTime() const { return _resultVarianceReaderHoldingTime; }
	    virtual double getResultVarianceWriterHoldingTime() const { return _resultVarianceWriterHoldingTime; }
	    virtual RWLockTask& setResultVarianceReaderHoldingTime( const double resultVarianceReaderHoldingTime ) { _resultVarianceReaderHoldingTime = resultVarianceReaderHoldingTime; return *this; }
	    virtual RWLockTask& setResultVarianceWriterHoldingTime( const double resultVarianceWriterHoldingTime ) { _resultVarianceWriterHoldingTime = resultVarianceWriterHoldingTime; return *this; }
	    virtual double getResultVarianceReaderHoldingTimeVariance() const { return _resultVarianceReaderHoldingTimeVariance; }
	    virtual double getResultVarianceWriterHoldingTimeVariance() const { return _resultVarianceWriterHoldingTimeVariance; }
	    virtual RWLockTask& setResultVarianceReaderHoldingTimeVariance( const double resultVarianceReaderHoldingTimeVariance) { _resultVarianceReaderHoldingTimeVariance = resultVarianceReaderHoldingTimeVariance; return *this; }
	    virtual RWLockTask& setResultVarianceWriterHoldingTimeVariance( const double resultVarianceWriterHoldingTimeVariance) { _resultVarianceWriterHoldingTimeVariance = resultVarianceWriterHoldingTimeVariance; return *this; }
	
	    /* rwlock holding time utilization */
	    virtual double getResultReaderHoldingUtilization() const { return _resultReaderHoldingUtilization; }
	    virtual RWLockTask& setResultReaderHoldingUtilization( const double resultReaderHoldingUtilization ) { _resultReaderHoldingUtilization = resultReaderHoldingUtilization; return *this; }
	    virtual double getResultWriterHoldingUtilization() const { return _resultWriterHoldingUtilization; }
	    virtual RWLockTask& setResultWriterHoldingUtilization( const double resultWriterHoldingUtilization ) { _resultWriterHoldingUtilization = resultWriterHoldingUtilization; return *this; }
	    virtual double getResultReaderHoldingUtilizationVariance() const { return _resultReaderHoldingUtilizationVariance; }
	    virtual RWLockTask& setResultReaderHoldingUtilizationVariance( const double resultReaderHoldingUtilizationVariance ) { _resultReaderHoldingUtilizationVariance = resultReaderHoldingUtilizationVariance; return *this; }
	    virtual double getResultWriterHoldingUtilizationVariance() const { return _resultWriterHoldingUtilizationVariance; }
	    virtual RWLockTask& setResultWriterHoldingUtilizationVariance( const double resultWriterHoldingUtilizationVariance ) { _resultWriterHoldingUtilizationVariance = resultWriterHoldingUtilizationVariance; return *this; }

	    /* rwlock Blocked time */
	    virtual double getResultReaderBlockedTime() const { return _resultReaderBlockedTime; }
	    virtual double getResultWriterBlockedTime() const { return _resultWriterBlockedTime; }
	    virtual RWLockTask& setResultReaderBlockedTime( const double resultReaderBlockedTime ) { _resultReaderBlockedTime = resultReaderBlockedTime; return *this; }
	    virtual RWLockTask& setResultWriterBlockedTime( const double resultWriterBlockedTime ) { _resultWriterBlockedTime = resultWriterBlockedTime; return *this; }
	    virtual double getResultReaderBlockedTimeVariance() const { return _resultReaderBlockedTimeVariance; }
	    virtual double getResultWriterBlockedTimeVariance() const { return _resultWriterBlockedTimeVariance; }
	    virtual RWLockTask& setResultReaderBlockedTimeVariance( const double resultReaderBlockedTimeVariance ) { _resultReaderBlockedTimeVariance = resultReaderBlockedTimeVariance; return *this; }
	    virtual RWLockTask& setResultWriterBlockedTimeVariance( const double resultWriterBlockedTimeVariance ) { _resultWriterBlockedTimeVariance = resultWriterBlockedTimeVariance; return *this; }
	    virtual double getResultVarianceReaderBlockedTime() const { return _resultVarianceReaderBlockedTime; }
	    virtual double getResultVarianceWriterBlockedTime() const { return _resultVarianceWriterBlockedTime; }
	    virtual RWLockTask& setResultVarianceReaderBlockedTime( const double resultVarianceReaderBlockedTime ) { _resultVarianceReaderBlockedTime = resultVarianceReaderBlockedTime; return *this; }
	    virtual RWLockTask& setResultVarianceWriterBlockedTime( const double resultVarianceWriterBlockedTime ) { _resultVarianceWriterBlockedTime = resultVarianceWriterBlockedTime; return *this; }
	    virtual double getResultVarianceReaderBlockedTimeVariance() const { return _resultVarianceReaderBlockedTimeVariance; }
	    virtual double getResultVarianceWriterBlockedTimeVariance() const { return _resultVarianceWriterBlockedTimeVariance; }
	    virtual RWLockTask& setResultVarianceReaderBlockedTimeVariance( const double resultVarianceReaderBlockedTimeVariance) { _resultVarianceReaderBlockedTimeVariance = resultVarianceReaderBlockedTimeVariance; return *this; }
	    virtual RWLockTask& setResultVarianceWriterBlockedTimeVariance( const double resultVarianceWriterBlockedTimeVariance) { _resultVarianceWriterBlockedTimeVariance = resultVarianceWriterBlockedTimeVariance; return *this; }

/*
  virtual bool hasHistogram() const;
  virtual const Histogram* getHistogram() const { return _histogram; }
  virtual void setHistogram(Histogram* histogram);
  virtual bool hasMaxServiceTimeExceeded() const;
  virtual double getMaxServiceTime() const;
  virtual double getResultMaxServiceTimeExceeded() const;
  virtual double getResultMaxServiceTimeExceededVariance() const;
  //virtual int getNumberOfReaders() const { return _n_readers; }
  */
	private:

	    //int		_n_readers;

	    double _resultReaderHoldingTime;
	    double _resultReaderHoldingTimeVariance;
	    double _resultVarianceReaderHoldingTime;
	    double _resultVarianceReaderHoldingTimeVariance;
	    double _resultWriterHoldingTime;
	    double _resultWriterHoldingTimeVariance;
	    double _resultVarianceWriterHoldingTime;
	    double _resultVarianceWriterHoldingTimeVariance;

	    double _resultReaderHoldingUtilization;
	    double _resultReaderHoldingUtilizationVariance;
	    double _resultWriterHoldingUtilization;
	    double _resultWriterHoldingUtilizationVariance;

	    double _resultReaderBlockedTime;
	    double _resultReaderBlockedTimeVariance;
	    double _resultVarianceReaderBlockedTime;
	    double _resultVarianceReaderBlockedTimeVariance;
	    double _resultWriterBlockedTime;
	    double _resultWriterBlockedTimeVariance;
	    double _resultVarianceWriterBlockedTime;
	    double _resultVarianceWriterBlockedTimeVariance;

//	    Histogram* _histogram;
	};
    }
}

#endif /* __LQIO_DOM_TASK__ */
