/* -*- c++ -*-
 *  $Id: dom_phase.h 10069 2010-12-03 18:58:14Z greg $
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
      
	public:
      
	    /* Designated initializer for the call information */
	    Phase(const Document * document, Entry* parentEntry);
	    Phase(const LQIO::DOM::Phase& );
	    virtual ~Phase();
      
	    /* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- [Input Values] -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
      
	    /* Accessors and Mutators */
	    bool isPresent() const;
	    bool isNotNull() const;
	    double getServiceTimeValue() const;
	    const ExternalVariable* getServiceTime() const;
	    void setServiceTime(ExternalVariable* serviceTime);
	    void setServiceTimeValue(double value);
	    bool hasServiceTime() const;
	    phase_type getPhaseTypeFlag() const;
	    void setPhaseTypeFlag(const phase_type phaseTypeFlag);
	    bool hasDeterministicCalls() const { return _phaseTypeFlag == PHASE_DETERMINISTIC; }
	    const Entry* getSourceEntry() const;
	    void setSourceEntry(Entry* entry);
	    double getThinkTimeValue() const;
	    const ExternalVariable* getThinkTime() const;
	    void setThinkTime(ExternalVariable* thinkTime);
	    void setThinkTimeValue( double value );
	    bool hasThinkTime() const;
	    double getCoeffOfVariationSquaredValue() const;
	    const ExternalVariable* getCoeffOfVariationSquared() const;
	    void setCoeffOfVariationSquared(ExternalVariable* cvsq);
	    void setCoeffOfVariationSquaredValue(double value);
	    bool hasCoeffOfVariationSquared() const;
	    bool isNonExponential() const;
	    virtual bool hasHistogram() const;
	    virtual const Histogram* getHistogram() const { return _histogram; }
	    virtual void setHistogram(Histogram* histogram);
	    virtual bool hasMaxServiceTimeExceeded() const;
	    virtual double getMaxServiceTime() const;
	    double getResultMaxServiceTimeExceeded() const;
	    double getResultMaxServiceTimeExceededVariance() const;
      
	    /* Managing Phase Calls */
	    void addCall(Call* call);
	    const std::vector<Call*>& getCalls() const;
	    Call* getCallToTarget(const Entry* target) const;
      
	    /* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- [Result Values] -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */
      
	    /* Storing the Result Parameters */
	    double getResultServiceTime() const;
	    Phase& setResultServiceTime(const double resultServiceTime);
	    double getResultServiceTimeVariance() const;
	    Phase& setResultServiceTimeVariance(const double resultServiceTimeVariance);
	    double getResultVarianceServiceTime() const;
	    Phase& setResultVarianceServiceTime(const double resultVarianceServiceTime);
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
      
	private:
      
	    /* Type of Entry */

	    std::vector<Call*> _calls;
	    ExternalVariable* _serviceTime;
	    phase_type _phaseTypeFlag;
	    Entry* _entry;
	    ExternalVariable* _thinkTime;
	    ExternalVariable* _coeffOfVariationSq;
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
      
	};

    }
}
#endif /* __LQIO_DOM_PHASE__ */
