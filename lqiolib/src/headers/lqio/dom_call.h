/* -*- c++ -*-
 *  $Id: dom_call.h 17332 2024-10-03 15:25:44Z greg $
 *
 *  Created by Martin Mroz on 24/02/09.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#ifndef __LQIO_DOM_CALL__
#define __LQIO_DOM_CALL__

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
	    /* Different types of calls */
	    enum class Type {
		NULL_CALL,
		SEND_NO_REPLY,
		RENDEZVOUS,
		FORWARD
	    };

	    typedef bool (DOM::Call::*boolCallFunc)() const;

	private:
	    Call& operator=( const Call& ) = delete;
	    Call( const Call& );		/* For clone */

	public:
	    /* Designated initializer for the call information */
	    Call(const Document * document, const Type type, Phase* source, Entry* destination, const ExternalVariable* callMean=nullptr );
	    Call(const Document * document, Entry *source, Entry* destination, const ExternalVariable* callMean=nullptr );
	    virtual ~Call();
	    Call * clone() const;

	    virtual void runtime_error( unsigned code, ... ) const;
	    void throw_invalid_parameter( const std::string& parameter, const std::string& error ) const;

	    /* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- [Input Values] -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */

	    /* Accessors and Mutators */
	    const char * getTypeName() const { return __typeName; }

	    const Type getCallType() const;
	    void setCallType(const Type callType);
	    void setSourceObject( DocumentObject* );
	    void setDestinationEntry( Entry* );
	    const Histogram* getHistogram() const { return _histogram; }
	    void setHistogram(Histogram* histogram);

	    bool hasHistogram() const;
	    bool hasRendezvous() const { return _callType == Call::Type::RENDEZVOUS; }
	    bool hasSendNoReply() const { return _callType == Call::Type::SEND_NO_REPLY; }
	    bool hasForwarding() const { return _callType == Call::Type::FORWARD; }

	    /* Accessors for Call Information */
	    const DocumentObject* getSourceObject() const;
	    const Entry* getDestinationEntry() const;

	    /* External Variable Access (Do not call until set) */
	    const ExternalVariable* getCallMean() const;
	    void setCallMean(const ExternalVariable* callMean);
	    virtual double getCallMeanValue() const;
	    void setCallMeanValue(double);

	    /* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- [Result Values] -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

	    /* The result values for the Call */
	    double getResultWaitingTime() const;
	    virtual Call& setResultWaitingTime(const double resultWaitingTime);
	    double getResultWaitingTimeVariance() const;
	    virtual Call& setResultWaitingTimeVariance(const double resultWaitingTimeVariance);
	    bool hasResultVarianceWaitingTime() const { return _hasResultVarianceWaitingTime; }
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
	    Type _callType;
	    DocumentObject * _sourceObject;
	    Entry* _destinationEntry;

	    /* Variables which can be scripted */
	    const ExternalVariable* _callMean;
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

	public:
	    static const char * __typeName;

	};

    }
}

#endif /* __LQIO_DOM_CALL__ */
