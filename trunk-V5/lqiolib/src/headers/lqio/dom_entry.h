/* -*- c++ -*-
 *  $Id$
 *
 *  Created by Martin Mroz on 24/02/09.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#ifndef __LQIO_DOM_ENTRY__
#define __LQIO_DOM_ENTRY__

#include <map>
#include "dom_object.h"
#include "dom_entity.h"
#include "dom_call.h"
#include "dom_phase.h"

namespace LQIO {
    namespace DOM {

	class Activity;
	class ExternalVariable;
	class Task;

	class Entry : public DocumentObject {
	public:
	    class Count {
	    protected:
		typedef bool (LQIO::DOM::Phase::*test_fn)() const;
		
	    public:
		Count( test_fn f ) :  _f(f), _count( 0 ) {}
		
		void operator()( const LQIO::DOM::Entry * );
		bool count() const { return _count; }

	    private:
		test_fn _f;
		bool _count;
	    };
	    
	private:
	    Entry(const Entry& );
	    Entry& operator=( const Entry& );

	public:

	    typedef enum EntryType {
		ENTRY_NOT_DEFINED,
		ENTRY_STANDARD,
		ENTRY_ACTIVITY,
		ENTRY_STANDARD_NOT_DEFINED,
		ENTRY_ACTIVITY_NOT_DEFINED,
		ENTRY_DEVICE
	    } EntryType;

	    /* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- [Structors] -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

	    /* Designated initializer for entries */
	    Entry(const Document * document, const char * name, const void * element=0 );
	    virtual ~Entry();

	    Entry * clone() const;	// Copy constructor is private */

	    /* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- [Input Values] -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */

	    /* Accessors and Mutators */

	    /* Managing the Times */
	    void setTask(Task* task);
	    const Task* getTask() const;
	    Phase* getPhase(unsigned phase);
	    Phase* getPhase(unsigned phase) const;
	    bool hasPhase(unsigned phase) const;
	    void setPhase( unsigned p, Phase * phase );
	    unsigned getMaximumPhase() const;
	    bool isStandardEntry() const;
	    Call* getCallToTarget(const Entry* target, unsigned phase) const;

	    /* Additional Entry Parameters */
	    void setOpenArrivalRate(ExternalVariable* value);
	    const ExternalVariable * getOpenArrivalRate() const { return _openArrivalRate; }
	    double getOpenArrivalRateValue() const;
	    bool hasOpenArrivalRate() const;
	    void setEntryPriority(ExternalVariable* value);
	    const ExternalVariable* getEntryPriority() const;
	    double getEntryPriorityValue();
	    bool hasEntryPriority() const;

	    bool entrySemaphoreTypeOk(semaphore_entry_type newType);
	    void setSemaphoreFlag(semaphore_entry_type set);
	    semaphore_entry_type getSemaphoreFlag() const;

	    bool entryRWLockTypeOk(rwlock_entry_type newType);
	    void setRWLockFlag(rwlock_entry_type set);
	    rwlock_entry_type getRWLockFlag() const;

	    bool entryTypeOk(EntryType newType);
	    const EntryType getEntryType() const;
	    void setEntryType(EntryType newType);
	    virtual bool hasHistogram() const;
	    virtual bool hasHistogramForPhase( unsigned ) const;
	    virtual const Histogram* getHistogramForPhase ( unsigned ) const;
	    virtual void setHistogramForPhase( unsigned, Histogram* );
	    bool hasMaxServiceTimeExceeded() const;

	    /* Forwarding Probabilities */
	    void addForwardingCall(Call *);
	    const std::vector<Call*>& getForwarding() const;
	    Call* getForwardingToTarget(const Entry* target) const;

	    /* Activity Support */
	    void setStartActivity(Activity* startActivity);
	    const Activity* getStartActivity() const;

	    /* Other */
	    const std::map<unsigned, Phase*>& getPhaseList() const { return _phases; }

	    /* Queries */
	    const bool hasThinkTime() const;
	    const bool hasDeterministicPhases() const;
	    const bool hasNonExponentialPhases() const;

	    /* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- [Result Values] -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

	    /* Storing the Result Parameters */
	    double getResultThroughput() const;
	    Entry& setResultThroughput(const double resultThroughput);
	    double getResultThroughputVariance() const;
	    Entry& setResultThroughputVariance(const double resultThroughputVariance);
	    double getResultThroughputBound() const;
	    Entry& setResultThroughputBound(const double resultThroughputBound);
	    double getResultUtilization() const;
	    Entry& setResultUtilization(const double resultUtilization);
	    double getResultUtilizationVariance() const;
	    Entry& setResultUtilizationVariance(const double resultUtilizationVariance);
	    double getResultProcessorUtilization() const;
	    Entry& setResultProcessorUtilization(const double resultProcessorUtilization);
	    double getResultProcessorUtilizationVariance() const;
	    Entry& setResultProcessorUtilizationVariance(const double resultProcessorUtilizationVariance);
	    double getResultSquaredCoeffVariation() const;
	    Entry& setResultSquaredCoeffVariation(const double resultSquaredCoeffVariation);
	    double getResultSquaredCoeffVariationVariance() const;
	    Entry& setResultSquaredCoeffVariationVariance(const double resultSquaredCoeffVariationVariance);
	    double getResultOpenWaitTime() const;
	    Entry& setResultOpenWaitTime(const double resultOpenWaitTime);
	    double getResultOpenWaitTimeVariance() const;
	    Entry& setResultOpenWaitTimeVariance(const double resultOpenWaitTimeVariance);

	    /* Result Information for Phase Service Info */
	    double getResultPhasePServiceTime( unsigned ) const;
	    double getResultPhase1ServiceTime() const { return getResultPhasePServiceTime(1); }
	    double getResultPhase2ServiceTime() const { return getResultPhasePServiceTime(2); }
	    double getResultPhase3ServiceTime() const { return getResultPhasePServiceTime(3); }
	    Entry& setResultPhasePServiceTime(const unsigned p,const double resultPhasePServiceTime);
	    Entry& setResultPhase1ServiceTime(const double resultPhasePServiceTime) { return setResultPhasePServiceTime( 1, resultPhasePServiceTime ); }
	    Entry& setResultPhase2ServiceTime(const double resultPhasePServiceTime) { return setResultPhasePServiceTime( 2, resultPhasePServiceTime ); }
	    Entry& setResultPhase3ServiceTime(const double resultPhasePServiceTime) { return setResultPhasePServiceTime( 3, resultPhasePServiceTime ); }
	    double getResultPhasePServiceTimeVariance(unsigned p) const;
       	    double getResultPhase1ServiceTimeVariance() const { return getResultPhasePServiceTimeVariance(1); }
	    double getResultPhase2ServiceTimeVariance() const { return getResultPhasePServiceTimeVariance(2); }
	    double getResultPhase3ServiceTimeVariance() const { return getResultPhasePServiceTimeVariance(3); }
	    Entry& setResultPhasePServiceTimeVariance(const unsigned p, const double resultPhasePServiceTimeVariance);
	    Entry& setResultPhase1ServiceTimeVariance(const double resultPhasePServiceTimeVariance) { return setResultPhasePServiceTimeVariance( 1, resultPhasePServiceTimeVariance ); }
	    Entry& setResultPhase2ServiceTimeVariance(const double resultPhasePServiceTimeVariance) { return setResultPhasePServiceTimeVariance( 2, resultPhasePServiceTimeVariance ); }
	    Entry& setResultPhase3ServiceTimeVariance(const double resultPhasePServiceTimeVariance) { return setResultPhasePServiceTimeVariance( 3, resultPhasePServiceTimeVariance ); }
	    double getResultPhasePVarianceServiceTime(unsigned p) const;
	    double getResultPhase1VarianceServiceTime() const { return getResultPhasePVarianceServiceTime(1); }
	    double getResultPhase2VarianceServiceTime() const { return getResultPhasePVarianceServiceTime(2); }
	    double getResultPhase3VarianceServiceTime() const { return getResultPhasePVarianceServiceTime(3); }
	    Entry& setResultPhasePVarianceServiceTime(const unsigned p, const double resultPhasePVarianceServiceTime);
	    Entry& setResultPhase1VarianceServiceTime(const double resultPhasePVarianceServiceTime) { return setResultPhasePVarianceServiceTime( 1, resultPhasePVarianceServiceTime ); }
	    Entry& setResultPhase2VarianceServiceTime(const double resultPhasePVarianceServiceTime) { return setResultPhasePVarianceServiceTime( 2, resultPhasePVarianceServiceTime ); }
	    Entry& setResultPhase3VarianceServiceTime(const double resultPhasePVarianceServiceTime) { return setResultPhasePVarianceServiceTime( 3, resultPhasePVarianceServiceTime ); }
	    double getResultPhasePVarianceServiceTimeVariance(unsigned p) const;
	    double getResultPhase1VarianceServiceTimeVariance() const { return getResultPhasePVarianceServiceTimeVariance(1); }
       	    double getResultPhase2VarianceServiceTimeVariance() const { return getResultPhasePVarianceServiceTimeVariance(2); }
	    double getResultPhase3VarianceServiceTimeVariance() const { return getResultPhasePVarianceServiceTimeVariance(3); }
	    Entry& setResultPhasePVarianceServiceTimeVariance(const unsigned p, const double resultPhasePVarianceServiceTimeVariance);
	    Entry& setResultPhase1VarianceServiceTimeVariance(const double resultPhasePVarianceServiceTimeVariance) { return setResultPhasePVarianceServiceTimeVariance( 1, resultPhasePVarianceServiceTimeVariance ); }
	    Entry& setResultPhase2VarianceServiceTimeVariance(const double resultPhasePVarianceServiceTimeVariance) { return setResultPhasePVarianceServiceTimeVariance( 2, resultPhasePVarianceServiceTimeVariance ); }
	    Entry& setResultPhase3VarianceServiceTimeVariance(const double resultPhasePVarianceServiceTimeVariance) { return setResultPhasePVarianceServiceTimeVariance( 3, resultPhasePVarianceServiceTimeVariance ); }
	    double getResultPhasePProcessorWaiting(unsigned p) const;
 	    double getResultPhase1ProcessorWaiting() const { return getResultPhasePProcessorWaiting(1); }
 	    double getResultPhase2ProcessorWaiting() const { return getResultPhasePProcessorWaiting(2); }
	    double getResultPhase3ProcessorWaiting() const { return getResultPhasePProcessorWaiting(3); }
	    Entry& setResultPhasePProcessorWaiting(const unsigned p, const double resultPhasePProcessorWaiting);
	    Entry& setResultPhase1ProcessorWaiting(const double resultPhasePProcessorWaiting) { return setResultPhasePProcessorWaiting( 1, resultPhasePProcessorWaiting ); }
	    Entry& setResultPhase2ProcessorWaiting(const double resultPhasePProcessorWaiting) { return setResultPhasePProcessorWaiting( 2, resultPhasePProcessorWaiting ); }
	    Entry& setResultPhase3ProcessorWaiting(const double resultPhasePProcessorWaiting) { return setResultPhasePProcessorWaiting( 3, resultPhasePProcessorWaiting ); }
       	    double getResultPhasePProcessorWaitingVariance(unsigned p) const;
 	    double getResultPhase1ProcessorWaitingVariance() const { return getResultPhasePProcessorWaitingVariance(1); }
	    double getResultPhase2ProcessorWaitingVariance() const { return getResultPhasePProcessorWaitingVariance(2); }
	    double getResultPhase3ProcessorWaitingVariance() const { return getResultPhasePProcessorWaitingVariance(3); }
	    Entry& setResultPhasePProcessorWaitingVariance(const unsigned p, const double resultPhasePProcessorWaitingVariance);
	    Entry& setResultPhase1ProcessorWaitingVariance(const double resultPhasePProcessorWaitingVariance) { return setResultPhasePProcessorWaitingVariance( 1, resultPhasePProcessorWaitingVariance ); }
	    Entry& setResultPhase2ProcessorWaitingVariance(const double resultPhasePProcessorWaitingVariance) { return setResultPhasePProcessorWaitingVariance( 2, resultPhasePProcessorWaitingVariance ); }
	    Entry& setResultPhase3ProcessorWaitingVariance(const double resultPhasePProcessorWaitingVariance) { return setResultPhasePProcessorWaitingVariance( 3, resultPhasePProcessorWaitingVariance ); }
	    double getResultPhasePUtilization(unsigned p) const;
       	    double getResultPhase1Utilization() const { return getResultPhasePUtilization(1); }
 	    double getResultPhase2Utilization() const { return getResultPhasePUtilization(2); }
	    double getResultPhase3Utilization() const { return getResultPhasePUtilization(3); }
	    Entry& setResultPhasePUtilization(const unsigned p, const double resultPhasePUtilization);
	    Entry& setResultPhase1Utilization(const double resultPhasePUtilization) { return setResultPhasePUtilization( 1, resultPhasePUtilization ); }
	    Entry& setResultPhase2Utilization(const double resultPhasePUtilization) { return setResultPhasePUtilization( 2, resultPhasePUtilization ); }
	    Entry& setResultPhase3Utilization(const double resultPhasePUtilization) { return setResultPhasePUtilization( 3, resultPhasePUtilization ); }
	    double getResultPhasePUtilizationVariance(unsigned p) const;
       	    double getResultPhase1UtilizationVariance() const { return getResultPhasePUtilizationVariance(1); }
	    double getResultPhase2UtilizationVariance() const { return getResultPhasePUtilizationVariance(2); }
	    double getResultPhase3UtilizationVariance() const { return getResultPhasePUtilizationVariance(3); }
	    Entry& setResultPhasePUtilizationVariance(const unsigned p, const double resultPhasePUtilizationVariance);
	    Entry& setResultPhase1UtilizationVariance(const double resultPhasePUtilizationVariance) { return setResultPhasePUtilizationVariance( 1, resultPhasePUtilizationVariance ); }
	    Entry& setResultPhase2UtilizationVariance(const double resultPhasePUtilizationVariance) { return setResultPhasePUtilizationVariance( 2, resultPhasePUtilizationVariance ); }
	    Entry& setResultPhase3UtilizationVariance(const double resultPhasePUtilizationVariance) { return setResultPhasePUtilizationVariance( 3, resultPhasePUtilizationVariance ); }
	    double getResultPhasePMaxServiceTimeExceeded(unsigned p) const;
	    double getResultPhasePMaxServiceTimeExceededVariance(unsigned p) const;

	    /* Actually store the results in the XML */
	    bool hasResultsForPhase(unsigned phase) const;
	    bool hasResultsForOpenWait() const;
	    bool hasResultsForThroughputBound() const;
	    void resetResultFlags();

	private:
	    Entry& setResultPhaseP( const unsigned p, double * result, double value );

	    /* Instance variables */

	    EntryType _type;
	    std::map<unsigned, Phase*> _phases;
	    unsigned _maxPhase;
	    Task* _task;
	    std::map<unsigned, Histogram*> _histograms;

	    /* Additional Entry Parameters */
	    ExternalVariable* _openArrivalRate;
	    ExternalVariable* _entryPriority;
	    semaphore_entry_type _semaphoreType;
	    rwlock_entry_type _rwlockType;
	    std::vector<Call *> _forwarding;

	    /* Variables for Activities */
	    Activity* _startActivity;

	    /* Computation Results from LQNS */
	    double _resultOpenWaitTime;
	    double _resultOpenWaitTimeVariance;
	    double _resultPhasePProcessorWaiting[Phase::MAX_PHASE];
	    double _resultPhasePProcessorWaitingVariance[Phase::MAX_PHASE];
	    double _resultPhasePUtilization[Phase::MAX_PHASE];
	    double _resultPhasePUtilizationVariance[Phase::MAX_PHASE];
	    double _resultPhasePServiceTime[Phase::MAX_PHASE];
	    double _resultPhasePServiceTimeVariance[Phase::MAX_PHASE];
	    double _resultPhasePVarianceServiceTime[Phase::MAX_PHASE];
	    double _resultPhasePVarianceServiceTimeVariance[Phase::MAX_PHASE];
	    double _resultProcessorUtilization;
	    double _resultProcessorUtilizationVariance;
	    double _resultSquaredCoeffVariation;
	    double _resultSquaredCoeffVariationVariance;
	    double _resultThroughput;
	    double _resultThroughputVariance;
	    double _resultThroughputBound;
	    double _resultUtilization;
	    double _resultUtilizationVariance;
	    bool _hasResultsForPhase[Phase::MAX_PHASE], _hasOpenWait, _hasThroughputBound;

	};

    }
}

#endif /* __LQIO_DOM_ENTRY_ */


