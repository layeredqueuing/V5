/*  -*- C++ -*-
 *  $Id: dom_object.h 11987 2014-04-16 20:57:40Z greg $
 *
 *  Created by Martin Mroz on 24/02/09.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#ifndef __LQIO_DOM_OBJECT__
#define __LQIO_DOM_OBJECT__

#include <string>

namespace LQIO {
    namespace DOM {
	class Document;
	class Histogram;

	class DocumentObject {
	protected:
	    DocumentObject(const Document * document, const char *, const void * xmlDOMElement );

	public:
	    virtual ~DocumentObject();

	    /* Accessors and Mutators */
	    unsigned long getSequenceNumber() const { return _sequenceNumber; }
	    const Document * getDocument() const { return _document; }
	    const void * getXMLDOMElement() const { return _xmlDOMElement; }	/* Return the DOM Element Pointer */
	    void setXMLDOMElement( const void * xmlDOMElement ) { _xmlDOMElement = xmlDOMElement; }
	    const std::string& getName() const;
	    void setName( const std::string& );
	    bool hasResults() const;
	    void setComment( const std::string& );
	    const std::string& getComment() const;

	    virtual bool hasHistogram() const { return false; }		/* Any object could have a histogram... */
	    virtual const Histogram* getHistogram() const { subclass(); return 0; }	/* But don't... */
	    virtual void setHistogram( Histogram* ) { subclass(); }
	    virtual bool hasHistogramForPhase( unsigned ) const { return false; };
	    virtual const Histogram* getHistogramForPhase ( unsigned ) const { subclass(); return 0; }	/* But don't... */
	    virtual void setHistogramForPhase( unsigned, Histogram* ) { subclass(); }

	    /* Results -- overridden by subclasses */

	    virtual DocumentObject& setResultBottleneckStrength( const double resultBottleneckStrength ) { subclass(); return *this; }
	    virtual DocumentObject& setResultDropProbability( const double resultDropProbability ) { subclass(); return *this; }
	    virtual DocumentObject& setResultDropProbabilityVariance( const double resultDropProbabilityVariance ) { subclass(); return *this; }
	    virtual DocumentObject& setResultJoinDelay(const double joinDelay ) { subclass(); return *this; }
	    virtual DocumentObject& setResultJoinDelayVariance(const double joinDelayVariance ) { subclass(); return *this; }
	    virtual DocumentObject& setResultOpenWaitTime(const double resultOpenWaitTime) { subclass(); return *this; }
	    virtual DocumentObject& setResultOpenWaitTimeVariance(const double resultOpenWaitTimeVariance) { subclass(); return *this; }
	    virtual DocumentObject& setResultPhase1ProcessorWaiting(const double resultPhasePProcessorWaiting) { subclass(); return *this; }
	    virtual DocumentObject& setResultPhase1ProcessorWaitingVariance(const double resultPhasePProcessorWaitingVariance) { subclass(); return *this; }
	    virtual DocumentObject& setResultPhase1ServiceTime(const double resultPhasePServiceTime) { subclass(); return *this; }
	    virtual DocumentObject& setResultPhase1ServiceTimeVariance(const double resultPhasePServiceTimeVariance) { subclass(); return *this; }
	    virtual DocumentObject& setResultPhase1Utilization(const double resultPhasePUtilization) { subclass(); return *this; }
	    virtual DocumentObject& setResultPhase1UtilizationVariance(const double resultPhasePUtilizationVariance) { subclass(); return *this; }
	    virtual DocumentObject& setResultPhase1VarianceServiceTime(const double resultPhasePVarianceServiceTime) { subclass(); return *this; }
	    virtual DocumentObject& setResultPhase1VarianceServiceTimeVariance(const double resultPhasePVarianceServiceTimeVariance) { subclass(); return *this; }
	    virtual DocumentObject& setResultPhase2ProcessorWaiting(const double resultPhasePProcessorWaiting) { subclass(); return *this; }
	    virtual DocumentObject& setResultPhase2ProcessorWaitingVariance(const double resultPhasePProcessorWaitingVariance) { subclass(); return *this; }
	    virtual DocumentObject& setResultPhase2ServiceTime(const double resultPhasePServiceTime) { subclass(); return *this; }
	    virtual DocumentObject& setResultPhase2ServiceTimeVariance(const double resultPhasePServiceTimeVariance) { subclass(); return *this; }
	    virtual DocumentObject& setResultPhase2Utilization(const double resultPhasePUtilization) { subclass(); return *this; }
	    virtual DocumentObject& setResultPhase2UtilizationVariance(const double resultPhasePUtilizationVariance) { subclass(); return *this; }
	    virtual DocumentObject& setResultPhase2VarianceServiceTime(const double resultPhasePVarianceServiceTime) { subclass(); return *this; }
	    virtual DocumentObject& setResultPhase2VarianceServiceTimeVariance(const double resultPhasePVarianceServiceTimeVariance) { subclass(); return *this; }
	    virtual DocumentObject& setResultPhase3ProcessorWaiting(const double resultPhasePProcessorWaiting) { subclass(); return *this; }
	    virtual DocumentObject& setResultPhase3ProcessorWaitingVariance(const double resultPhasePProcessorWaitingVariance) { subclass(); return *this; }
	    virtual DocumentObject& setResultPhase3ServiceTime(const double resultPhasePServiceTime) { subclass(); return *this; }
	    virtual DocumentObject& setResultPhase3ServiceTimeVariance(const double resultPhasePServiceTimeVariance) { subclass(); return *this; }
	    virtual DocumentObject& setResultPhase3Utilization(const double resultPhasePUtilization) { subclass(); return *this; }
	    virtual DocumentObject& setResultPhase3UtilizationVariance(const double resultPhasePUtilizationVariance) { subclass(); return *this; }
	    virtual DocumentObject& setResultPhase3VarianceServiceTime(const double resultPhasePVarianceServiceTime) { subclass(); return *this; }
	    virtual DocumentObject& setResultPhase3VarianceServiceTimeVariance(const double resultPhasePVarianceServiceTimeVariance) { subclass(); return *this; }
	    virtual DocumentObject& setResultPhaseUtilizationVariances(unsigned int count, double* resultPhaseUtilizationsVariance) { subclass(); return *this; }
	    virtual DocumentObject& setResultPhaseUtilizations(unsigned int count, double* resultPhaseUtilizations) { subclass(); return *this; }
	    virtual DocumentObject& setResultPhasePProcessorWaiting(unsigned p, double) { subclass(); return *this; }
	    virtual DocumentObject& setResultPhasePProcessorWaitingVariance(unsigned p, double) { subclass(); return *this; }
	    virtual DocumentObject& setResultPhasePServiceTime(unsigned p, double) { subclass(); return *this; }
	    virtual DocumentObject& setResultPhasePServiceTimeVariance(unsigned p, double) { subclass(); return *this; }
	    virtual DocumentObject& setResultPhasePUtilization(unsigned p, double) { subclass(); return *this; }
	    virtual DocumentObject& setResultPhasePUtilizationVariance(unsigned p, double) { subclass(); return *this; }
	    virtual DocumentObject& setResultPhasePVarianceServiceTime(unsigned p, double) { subclass(); return *this; }
	    virtual DocumentObject& setResultPhasePVarianceServiceTimeVariance(unsigned p, double) { subclass(); return *this; }
	    virtual DocumentObject& setResultProcessorUtilization(const double resultProcessorUtilization) { subclass(); return *this; }
	    virtual DocumentObject& setResultProcessorUtilizationVariance(const double resultProcessorUtilizationVariance) { subclass(); return *this; }
	    virtual DocumentObject& setResultProcessorWaiting(const double resultProcessorWaiting) { subclass(); return *this; }
	    virtual DocumentObject& setResultProcessorWaitingVariance(const double resultProcessorWaitingVariance) { subclass(); return *this; }
	    virtual DocumentObject& setResultServiceTime(const double resultServiceTime) { subclass(); return *this; }
	    virtual DocumentObject& setResultServiceTimeVariance(const double resultServiceTimeVariance) { subclass(); return *this; }
	    virtual DocumentObject& setResultSquaredCoeffVariation(const double resultSquaredCoeffVariation) { subclass(); return *this; }
	    virtual DocumentObject& setResultSquaredCoeffVariationVariance(const double resultSquaredCoeffVariationVariance) { subclass(); return *this; }
	    virtual DocumentObject& setResultThroughput(const double resultThroughput) { subclass(); return *this; }
	    virtual DocumentObject& setResultThroughputBound(const double resultThroughputBound) { subclass(); return *this; }
	    virtual DocumentObject& setResultThroughputVariance(const double resultThroughputVariance) { subclass(); return *this; }
	    virtual DocumentObject& setResultUtilization(const double resultUtilization) { subclass(); return *this; }
	    virtual DocumentObject& setResultUtilizationVariance(const double resultUtilizationVariance) { subclass(); return *this; }
	    virtual DocumentObject& setResultVarianceJoinDelay(const double varianceJoinDelay ) { subclass(); return *this; }
	    virtual DocumentObject& setResultVarianceJoinDelayVariance(const double varianceJoinDelayVariance ) { subclass(); return *this; }
	    virtual DocumentObject& setResultVarianceServiceTime(const double resultVarianceServiceTime) { subclass(); return *this; }
	    virtual DocumentObject& setResultVarianceServiceTimeVariance(const double resultVarianceServiceTimeVariance) { subclass(); return *this; }
	    virtual DocumentObject& setResultVarianceWaitingTime(const double resultVarianceWaitingTime) { subclass(); return *this; }
	    virtual DocumentObject& setResultVarianceWaitingTimeVariance(const double resultVarianceWaitingTimeVariance) { subclass(); return *this; }
	    virtual DocumentObject& setResultWaitingTime(const double resultWaitingTime) { subclass(); return *this; }
	    virtual DocumentObject& setResultWaitingTimeVariance(const double resultWaitingTimeVariance) { subclass(); return *this; }
 
	    /*  For Semaphore */
            virtual DocumentObject& setResultHoldingTime( const double ) { subclass(); return *this; }
            virtual DocumentObject& setResultHoldingTimeVariance( const double ) { subclass(); return *this; }
            virtual DocumentObject& setResultHoldingUtilization( const double ) { subclass(); return *this; }
            virtual DocumentObject& setResultHoldingUtilizationVariance( const double ) { subclass(); return *this; }
            virtual DocumentObject& setResultVarianceHoldingTime(const double ) { subclass(); return *this; }
            virtual DocumentObject& setResultVarianceHoldingTimeVariance( const double ) { subclass(); return *this; }

	    /* For RWlock*/
	    virtual DocumentObject& setResultReaderHoldingTime( const double ) { subclass(); return *this; }
            virtual DocumentObject& setResultReaderHoldingTimeVariance( const double ) { subclass(); return *this; }
            virtual DocumentObject& setResultReaderHoldingUtilization( const double ) { subclass(); return *this; }
            virtual DocumentObject& setResultReaderHoldingUtilizationVariance( const double ) { subclass(); return *this; }
            virtual DocumentObject& setResultVarianceReaderHoldingTime(const double ) { subclass(); return *this; }
            virtual DocumentObject& setResultVarianceReaderHoldingTimeVariance( const double ) { subclass(); return *this; }
	    virtual DocumentObject& setResultReaderBlockedTime( const double ) { subclass(); return *this; }
	    virtual DocumentObject& setResultReaderBlockedTimeVariance( const double ) { subclass(); return *this; }
	    virtual DocumentObject& setResultVarianceReaderBlockedTime( const double ) { subclass();; return *this; }
	    virtual DocumentObject& setResultVarianceReaderBlockedTimeVariance( const double ) { subclass(); return *this; }

	    virtual DocumentObject& setResultWriterHoldingTime( const double ) { subclass(); return *this; }
            virtual DocumentObject& setResultWriterHoldingTimeVariance( const double ) { subclass(); return *this; }
            virtual DocumentObject& setResultWriterHoldingUtilization( const double ) { subclass(); return *this; }
            virtual DocumentObject& setResultWriterHoldingUtilizationVariance( const double ) { subclass(); return *this; }
            virtual DocumentObject& setResultVarianceWriterHoldingTime(const double ) { subclass(); return *this; }
            virtual DocumentObject& setResultVarianceWriterHoldingTimeVariance( const double ) { subclass(); return *this; }
	    virtual DocumentObject& setResultWriterBlockedTime( const double ) { subclass(); return *this; }
	    virtual DocumentObject& setResultWriterBlockedTimeVariance( const double ) { subclass(); return *this; }
	    virtual DocumentObject& setResultVarianceWriterBlockedTime( const double ) { subclass(); return *this; }
	    virtual DocumentObject& setResultVarianceWriterBlockedTimeVariance( const double ) { subclass(); return *this; }
	    
	    double getResultZero( unsigned phase=0 ) const { return 0; }
            virtual double getResultConvergenceValue() const { subclass(); return 0; }
            virtual double getResultDropProbability() const { subclass(); return 0; }
            virtual double getResultDropProbabilityVariance() const { subclass(); return 0; }
            virtual double getResultJoinDelay() const { subclass(); return 0; }
            virtual double getResultJoinDelayVariance() const { subclass(); return 0; }
            virtual double getResultMaxServiceTimeExceeded() const { subclass(); return 0; }
            virtual double getResultMaxServiceTimeExceededVariance() const { subclass(); return 0; }
            virtual double getResultOpenWaitTime() const { subclass(); return 0; }
            virtual double getResultOpenWaitTimeVariance() const { subclass(); return 0; }
            virtual double getResultPhase1ProcessorWaiting() const { subclass(); return 0; }
            virtual double getResultPhase1ProcessorWaitingVariance() const { subclass(); return 0; }
            virtual double getResultPhase1ServiceTime() const { subclass(); return 0; }
            virtual double getResultPhase1ServiceTimeVariance() const { subclass(); return 0; }
            virtual double getResultPhase1Utilization() const { subclass(); return 0; }
            virtual double getResultPhase1UtilizationVariance() const { subclass(); return 0; }
            virtual double getResultPhase1VarianceServiceTime() const { subclass(); return 0; }
            virtual double getResultPhase1VarianceServiceTimeVariance() const { subclass(); return 0; }
            virtual double getResultPhase2ProcessorWaiting() const { subclass(); return 0; }
            virtual double getResultPhase2ProcessorWaitingVariance() const { subclass(); return 0; }
            virtual double getResultPhase2ServiceTime() const { subclass(); return 0; }
            virtual double getResultPhase2ServiceTimeVariance() const { subclass(); return 0; }
            virtual double getResultPhase2Utilization() const { subclass(); return 0; }
            virtual double getResultPhase2UtilizationVariance() const { subclass(); return 0; }
            virtual double getResultPhase2VarianceServiceTime() const { subclass(); return 0; }
            virtual double getResultPhase2VarianceServiceTimeVariance() const { subclass(); return 0; }
            virtual double getResultPhase3ProcessorWaiting() const { subclass(); return 0; }
            virtual double getResultPhase3ProcessorWaitingVariance() const { subclass(); return 0; }
            virtual double getResultPhase3ServiceTime() const { subclass(); return 0; }
            virtual double getResultPhase3ServiceTimeVariance() const { subclass(); return 0; }
            virtual double getResultPhase3Utilization() const { subclass(); return 0; }
            virtual double getResultPhase3UtilizationVariance() const { subclass(); return 0; }
            virtual double getResultPhase3VarianceServiceTime() const { subclass(); return 0; }
            virtual double getResultPhase3VarianceServiceTimeVariance() const { subclass(); return 0; }
            virtual double getResultPhasePMaxServiceTimeExceeded(unsigned p) const { subclass(); return 0; }
            virtual double getResultPhasePMaxServiceTimeExceededVariance(unsigned p) const { subclass(); return 0; }
            virtual double getResultPhasePProcessorWaiting(unsigned p) const { subclass(); return 0; }
            virtual double getResultPhasePProcessorWaitingVariance(unsigned p) const { subclass(); return 0; }
            virtual double getResultPhasePServiceTime(unsigned p) const { subclass(); return 0; }
            virtual double getResultPhasePServiceTimeVariance(unsigned p) const { subclass(); return 0; }
            virtual double getResultPhasePUtilization(unsigned p) const { subclass(); return 0; }
            virtual double getResultPhasePUtilizationVariance(unsigned p) const { subclass(); return 0; }
            virtual double getResultPhasePVarianceServiceTime(unsigned p) const { subclass(); return 0; }
            virtual double getResultPhasePVarianceServiceTimeVariance(unsigned p) const { subclass(); return 0; }
            virtual double getResultProcessorUtilization() const { subclass(); return 0; }
            virtual double getResultProcessorUtilizationVariance() const { subclass(); return 0; }
            virtual double getResultProcessorWaiting() const { subclass(); return 0; }
            virtual double getResultProcessorWaitingVariance() const { subclass(); return 0; }
            virtual double getResultServiceTime() const { subclass(); return 0; }
            virtual double getResultServiceTimeVariance() const { subclass(); return 0; }
            virtual double getResultSquaredCoeffVariation() const { subclass(); return 0; }
            virtual double getResultSquaredCoeffVariationVariance() const { subclass(); return 0; }
            virtual double getResultThroughput() const { subclass(); return 0; }
            virtual double getResultThroughputBound() const { subclass(); return 0; }
            virtual double getResultThroughputVariance() const { subclass(); return 0; }
            virtual double getResultUtilization() const { subclass(); return 0; }
            virtual double getResultUtilizationVariance() const { subclass(); return 0; }
            virtual double getResultVarianceJoinDelay() const { subclass(); return 0; }
            virtual double getResultVarianceJoinDelayVariance() const { subclass(); return 0; }
            virtual double getResultVarianceServiceTime() const { subclass(); return 0; }
            virtual double getResultVarianceServiceTimeVariance() const { subclass(); return 0; }
            virtual double getResultVarianceWaitingTime() const { subclass(); return 0; }
            virtual double getResultVarianceWaitingTimeVariance() const { subclass(); return 0; }
            virtual double getResultWaitingTime() const { subclass(); return 0; }
            virtual double getResultWaitingTimeVariance() const { subclass(); return 0; }

	    /*  For Semaphore */
	    virtual double getResultHoldingTime() const { subclass(); return 0; }
	    virtual double getResultHoldingTimeVariance() const { subclass(); return 0; }
	    virtual double getResultHoldingUtilization() const { subclass(); return 0; }
	    virtual double getResultHoldingUtilizationVariance() const { subclass(); return 0; }
	    virtual double getResultVarianceHoldingTime() const { subclass(); return 0; }
	    virtual double getResultVarianceHoldingTimeVariance() const { subclass(); return 0; }

	    /* For RWlock*/
	    virtual double getResultReaderHoldingTime() const { subclass(); return 0; }
	    virtual double getResultReaderHoldingTimeVariance() const { subclass(); return 0; }
	    virtual double getResultReaderHoldingUtilization() const { subclass(); return 0; }
	    virtual double getResultReaderHoldingUtilizationVariance() const { subclass(); return 0; }
	    virtual double getResultVarianceReaderHoldingTime() const { subclass(); return 0; }
	    virtual double getResultVarianceReaderHoldingTimeVariance() const { subclass(); return 0; }
	    virtual double getResultReaderBlockedTime () const { subclass(); return 0; }
	    virtual double getResultReaderBlockedTimeVariance () const { subclass(); return 0; }
	    virtual double getResultVarianceReaderBlockedTime () const { subclass(); return 0; }
	    virtual double getResultVarianceReaderBlockedTimeVariance () const { subclass(); return 0; }

	    virtual double getResultWriterHoldingTime() const { subclass(); return 0; }
	    virtual double getResultWriterHoldingTimeVariance() const { subclass(); return 0; }
	    virtual double getResultWriterHoldingUtilization() const { subclass(); return 0; }
	    virtual double getResultWriterHoldingUtilizationVariance() const { subclass(); return 0; }
	    virtual double getResultVarianceWriterHoldingTime() const { subclass(); return 0; }
	    virtual double getResultVarianceWriterHoldingTimeVariance() const { subclass(); return 0; }
	    virtual double getResultWriterBlockedTime () const { subclass(); return 0; }
	    virtual double getResultWriterBlockedTimeVariance () const { subclass(); return 0; }
	    virtual double getResultVarianceWriterBlockedTime () const { subclass(); return 0; }
	    virtual double getResultVarianceWriterBlockedTimeVariance () const { subclass(); return 0; }

	private:
	    void subclass() const;

	private:
	    const Document * _document;			/* Pointer to the root. */
	    const unsigned long _sequenceNumber;
	    std::string _name;
	    std::string _comment;
	    const void * _xmlDOMElement;		/* Used by XERCES to point to DOM. */

	    static unsigned long sequenceNumber;
	};
    }
}

#endif /*  __LQIO_DOM_OBJECT__ */
