/* -*- c++ -*-
 *  $Id: dom_phase.h 14387 2021-01-21 14:09:16Z greg $
 *
 *  Created by Martin Mroz on 24/02/09.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#ifndef __LQIO_DOM_PHASE__
#define __LQIO_DOM_PHASE__

#include "input.h"
#include "dom_object.h"

#include <vector>
#include <string>

namespace LQIO {
    namespace DOM {
	class Entry;
	class Call;
	class ExternalVariable;

	class Phase : public DocumentObject {
	public:  
      
	    /* The maximum allowable phase number */
	    static const unsigned int MAX_PHASE = 3;
      
	    struct eqPhase {
	        eqPhase( const DOM::Phase * phase ) : _phase(phase) {}
		bool operator()(const std::pair<unsigned, Phase *>& p ) const { return p.second == _phase; }

	    private:
		const DOM::Phase * _phase;
	    };

	    enum Type { STOCHASTIC, DETERMINISTIC };


	public:
      
	    /* Designated initializer for the call information */
	    Phase();
	    Phase(const Document * document, Entry* parentEntry);
	    Phase(const LQIO::DOM::Phase& );
	    virtual ~Phase();
      
	    /* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- [Input Values] -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
      
	    /* Accessors and Mutators */
	    const char * getTypeName() const { return __typeName; }

	    bool isPresent() const;
	    double getServiceTimeValue() const;
	    const ExternalVariable* getServiceTime() const;
	    void setServiceTime(const ExternalVariable* serviceTime);
	    void setServiceTimeValue(double value);
	    bool hasServiceTime() const;
	    Phase::Type getPhaseTypeFlag() const;
	    void setPhaseTypeFlag(const Phase::Type phaseTypeFlag);
	    bool hasDeterministicCalls() const { return getCalls().size() > 0 && _phaseTypeFlag == Type::DETERMINISTIC; }
	    bool hasStochasticCalls() const { return getCalls().size() > 0 && _phaseTypeFlag == Type::STOCHASTIC; }
	    const Entry* getSourceEntry() const;
	    void setSourceEntry(Entry* entry);
	    double getThinkTimeValue() const;
	    const ExternalVariable* getThinkTime() const;
	    void setThinkTime(const ExternalVariable* thinkTime);
	    void setThinkTimeValue( double value );
	    bool hasThinkTime() const;
	    double getCoeffOfVariationSquaredValue() const;
	    const ExternalVariable* getCoeffOfVariationSquared() const;
	    void setCoeffOfVariationSquared(const ExternalVariable* cvsq);
	    void setCoeffOfVariationSquaredValue(double value);
	    bool hasCoeffOfVariationSquared() const;
	    bool isNonExponential() const;
	    void setMaxServiceTime(const ExternalVariable* serviceTime);
	    void setMaxServiceTimeValue(double value);
	    virtual double getMaxServiceTime() const;
	    virtual bool hasMaxServiceTimeExceeded() const;
	    virtual bool hasHistogram() const;
	    virtual const Histogram* getHistogram() const { return _histogram; }
	    virtual void setHistogram(Histogram* histogram);
      
	    /* Managing Phase Calls */
	    void addCall(Call* call);
	    void eraseCall( Call * call );
	    const std::vector<Call*>& getCalls() const;
	    Call* getCallToTarget(const Entry* target) const;
	    bool hasRendezvous() const;
	    bool hasSendNoReply() const;
	    bool hasResultVarianceWaitingTime() const;
	    bool hasResultDropProbability() const;
      
	    /* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- [Result Values] -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */
      
	    /* Storing the Result Parameters */
	    double getResultServiceTime() const;
	    Phase& setResultServiceTime(const double resultServiceTime);
	    double getResultServiceTimeVariance() const;
	    Phase& setResultServiceTimeVariance(const double resultServiceTimeVariance);
	    double getResultVarianceServiceTime() const;
	    Phase& setResultVarianceServiceTime(const double resultVarianceServiceTime);
	    bool hasResultServiceTimeVariance() const { return _hasResultServiceTimeVariance; }
	    double getResultVarianceServiceTimeVariance() const;
	    Phase& setResultVarianceServiceTimeVariance(const double resultVarianceServiceTimeVariance);
	    double getResultUtilization() const;
	    Phase& setResultUtilization(const double resultUtilization);
	    double getResultUtilizationVariance() const;
	    Phase& setResultUtilizationVariance(const double resultUtilizationVariance);
	    double getResultProcessorWaiting() const;
	    Phase& setResultProcessorWaiting(const double resultProcessorWaiting);
	    double getResultProcessorWaitingVariance() const;
	    Phase& setResultProcessorWaitingVariance(const double resultProcessorWaitingVariance);
	    double getResultMaxServiceTimeExceeded() const;
	    double getResultMaxServiceTimeExceededVariance() const;
      
	private:
      
	    /* Type of Entry */

	    std::vector<Call*> _calls;
	    const ExternalVariable* _serviceTime;
	    Phase::Type _phaseTypeFlag;
	    Entry* _entry;
	    const ExternalVariable* _thinkTime;
	    const ExternalVariable* _coeffOfVariationSq;
	    Histogram* _histogram;
      
	    /* Computation Results from LQNS */
	    double _resultServiceTime;
	    double _resultServiceTimeVariance;
	    double _resultVarianceServiceTime;
	    double _resultVarianceServiceTimeVariance;
	    double _resultUtilization;
	    double _resultUtilizationVariance;
	    double _resultProcessorWaiting;
	    double _resultProcessorWaitingVariance;
	    bool _hasResultServiceTimeVariance;
      
	public:
	    static const char * __typeName;

	};

    }
}
#endif /* __LQIO_DOM_PHASE__ */
