/* -*- c++ -*-
 *  $Id$
 *
 *  Created by Martin Mroz on 24/02/09.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#ifndef __LQIO_DOM_CALL__
#define __LQIO_DOM_CALL__

#include "dom_object.h"
#include "dom_object.h"

namespace LQIO {
    namespace DOM {

	class ExternalVariable;
	class Activity;
	class Entry;
	class Phase;
	class Histogram;

	class Call : public DocumentObject {
	public:

	    /*
	     * Compare a entry name to a string.  Used by the find_if (and other algorithm type things).
	     */

	    struct eqDestEntry {
	        eqDestEntry( const DOM::Entry * entry ) : _entry(entry) {}
		bool operator()(const DOM::Call * call ) const { return call->getDestinationEntry() == _entry; }

	    private:
		const DOM::Entry * _entry;
	    };

	    /* Different types of calls */
	    typedef enum CallType {
		NULL_CALL,
		SEND_NO_REPLY,
		RENDEZVOUS,
		FORWARD,
		QUASI_SEND_NO_REPLY,		// Special (lqsim/petrisrvn)
		QUASI_RENDEZVOUS		// Special (lqns)
	    } CallType;

	private:
	    Call& operator=( const Call& );		// Copying is verbotten
	    Call( const Call& );

	public:

	    /* Designated initializer for the call information */
	    Call(const Document * document, const CallType type, Phase* source, Entry* destination, ExternalVariable* callMean=0, const void * element=0);
	    Call(const Document * document, Entry *source, Entry* destination, ExternalVariable* callMean=0, const void * element=0);
	    virtual ~Call();
	    Call * clone() const;

	    /* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- [Input Values] -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */

	    /* Accessors and Mutators */
	    const CallType getCallType() const;
	    void setCallType(const CallType callType);
	    void setSourceEntry( Entry* );
	    void setDestinationEntry( Entry* );
	    const Histogram* getHistogram() const { return _histogram; }
	    void setHistogram(Histogram* histogram);

	    bool hasHistogram() const;
	    bool hasRendezvous() const { return _callType == Call::RENDEZVOUS; }
	    bool hasSendNoReply() const { return _callType == Call::SEND_NO_REPLY; }
	    bool hasForwarding() const { return _callType == Call::FORWARD; }

	    /* Accessors for Call Information */
	    const Entry* getSourceEntry() const;
	    const Entry* getDestinationEntry() const;

	    /* External Variable Access (Do not call until set) */
	    const ExternalVariable* getCallMean() const;
	    void setCallMean(ExternalVariable* callMean);
	    const double getCallMeanValue() const;

	    /* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- [Result Values] -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

	    /* The result values for the Call */
	    double getResultWaitingTime() const;
	    virtual Call& setResultWaitingTime(const double resultWaitingTime);
	    double getResultWaitingTimeVariance() const;
	    bool hasResultVarianceWaitingTime() const { return _hasResultVarianceWaitingTime != 0.0; }
	    virtual Call& setResultWaitingTimeVariance(const double resultWaitingTimeVariance);
	    double getResultVarianceWaitingTime() const;
	    virtual Call& setResultVarianceWaitingTime(const double resultVarianceWaitingTime);
	    double getResultVarianceWaitingTimeVariance() const;
	    virtual Call& setResultVarianceWaitingTimeVariance(const double resultVarianceWaitingTimeVariance);
	    bool hasResultDropProbability() const { return _hasResultDropProbability; }	/* Might be zero... */
	    double getResultDropProbability() const;
	    virtual Call& setResultDropProbability( const double resultDropProbability );
	    double getResultDropProbabilityVariance() const;
	    virtual Call& setResultDropProbabilityVariance( const double resultDropProbabilityVariance );

	private:

	    /* Type of Entry */
	    CallType _callType;
	    union {
		Phase* _phase;
		Entry * _entry;		/* Forwarding. */
	    } _source;
	    Entry* _destinationEntry;

	    /* Variables which can be scripted */
	    ExternalVariable* _callMean;
	    Histogram * _histogram;

	    /* Results for the call */
	    bool _hasResultVarianceWaitingTime;
	    bool _hasResultDropProbability;
	    double _resultWaitingTime;
	    double _resultWaitingTimeVariance;
	    double _resultVarianceWaitingTime;
	    double _resultVarianceWaitingTimeVariance;
	    double _resultDropProbability;
	    double _resultDropProbabilityVariance;

	};

    }
}

#endif /* __LQIO_DOM_CALL__ */
