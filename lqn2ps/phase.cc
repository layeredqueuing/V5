/*  -*- c++ -*-
 * $Id: phase.cc 11963 2014-04-10 14:36:42Z greg $
 *
 * Everything you wanted to know about a phase, but were afraid to ask.
 *
 * Copyright the Real-Time and Distributed Systems Group,
 * Department of Systems and Computer Engineering,
 * Carleton University, Ottawa, Ontario, Canada. K1S 5B6
 *
 * November, 2005
 */

#include "lqn2ps.h"
#include <cstdlib>
#include <string.h>
#include <cmath>
#include <lqio/error.h>
#include <lqio/dom_phase.h>
#include <lqio/dom_extvar.h>
#include "model.h"
#include "phase.h"
#include "cltn.h"
#include "vector.h"
#include "entry.h"
#include "entity.h"
#include "task.h"
#include "call.h"
#include "processor.h"
#include "errmsg.h"

/* ---------------------- Overloaded Operators ------------------------ */

ostream& 
operator<<( ostream& output, const Phase& self )
{
    switch( Flags::print[OUTPUT_FORMAT].value.i ) {
    case FORMAT_SRVN:
    case FORMAT_XML:
	break;
    }
    return output;
}

ostream& 
operator<<( ostream& output, const Phase::Histogram::Bin& self )
{
    return output;
}


Phase::Histogram::Histogram() 
    : myNumBins(0),
      myMin(0.0),
      myMax(0.0),
      myMean(0.0),
      myStdDev(0.0),
      mySkew(0.0),
      myKurtosis(0.0)
{
}

Phase::Histogram&
Phase::Histogram::set( const double hist_min, const double hist_max, const unsigned n_bins )
{
    if ( hist_min < 0 ) {
	LQIO::input_error2( LQIO::ERR_HISTOGRAM_INVALID_MIN, hist_min );
    }

    myMin       = hist_min;
    myMax       = hist_max;
    myNumBins   = n_bins + 2;

    return *this;
}

Phase::Histogram&
Phase::Histogram::moments( const double mean, const double stddev, const double skew, const double kurtosis )
{
    myMean = mean;
    myStdDev = stddev;
    mySkew = skew;
    myKurtosis = kurtosis;
    return *this;
}


Phase::Histogram&
Phase::Histogram::addBin( const double begin, const double end, const double prob, const double conf95, const double conf99 )
{
    bins.grow(1);
    const int n = bins.size();
    bins[n].myBegin  = begin;
    bins[n].myEnd    = end;
    bins[n].myProb   = prob;
    bins[n].myConf95 = conf95;
    bins[n].myConf99 = conf99;
    return *this;
}

Phase::Phase()
    : _documentObject(0),
      myEntry(0), 
      myPhase(0)
{
}



Phase::~Phase()
{
}



/*
 * Assignment.  Used by rep2flat.  See bug 246.
 */

Phase&
Phase::operator=( const Phase& src )
{
    if ( *this == src ) return *this;

    myHistogram = src.myHistogram;
    return *this;
}



/*
 * Set source entry and phase.
 */

void
Phase::initialize( Entry * src, const unsigned p )
{
    assert ( 0 < p && p <= MAX_PHASES );
		
    myEntry = src;
    myPhase = p;
}

/* --------------------- Instance Variable access --------------------- */
const string& 
Phase::name() const
{
    return getDOM()->getName();
}


bool 
Phase::hasServiceTime() const
{
#if 0
    const LQIO::DOM::Phase * dom = getDOM();
    if ( dom ) {
	const LQIO::DOM::ExternalVariable * var = dom->getServiceTime();
	return var && var->wasSet();
    } else {
	return false;
    }
#else
    return getDOM() && getDOM()->getServiceTime() != 0;
#endif
}


const LQIO::DOM::ExternalVariable&
Phase::serviceTime() const
{ 
    return *getDOM()->getServiceTime();
}


bool
Phase::hasThinkTime() const
{
    return getDOM() && getDOM()->getThinkTime() != 0;
}

    
const LQIO::DOM::ExternalVariable&
Phase::thinkTime() const 
{ 
    return *getDOM()->getThinkTime();
}


bool
Phase::hasCV_sqr() const 
{ 
    return getDOM() && getDOM()->getCoeffOfVariationSquared() != 0;
}


const LQIO::DOM::ExternalVariable&
Phase::Cv_sqr() const 
{ 
    return *getDOM()->getCoeffOfVariationSquared(); 
}


phase_type
Phase::phaseTypeFlag() const 
{ 
    const LQIO::DOM::Phase * dom = getDOM();
    return dom ? dom->getPhaseTypeFlag() : PHASE_STOCHASTIC;
}


Phase&
Phase::phaseTypeFlag( phase_type aType ) 
{
    LQIO::DOM::Phase * dom = const_cast<LQIO::DOM::Phase *>(getDOM());
    dom->setPhaseTypeFlag( aType );
    return *this;
}

/* -------------------------- Result Queries -------------------------- */

double
Phase::executionTime() const
{
    const LQIO::DOM::Phase * dom = getDOM();
    return dom ? dom->getResultServiceTime() : 0.0;
}

/* --- */

double
Phase::variance() const
{ 
    const LQIO::DOM::Phase * dom = getDOM();
    return dom ? dom->getResultVarianceServiceTime() : 0.0;
}

/* --- */

double
Phase::serviceExceeded() const
{ 
    return 0.0; 
}

/* -- */

double 
Phase::utilization() const
{
    const LQIO::DOM::Phase * dom = getDOM();
    return dom ? dom->getResultUtilization() : 0.0;
}

/* --- */

bool 
Phase::hasQueueingTime() const
{
    return getDOM() && getDOM()->getResultProcessorWaiting() != 0;
}

double 
Phase::queueingTime() const
{
    const LQIO::DOM::Phase * dom = getDOM();
    return dom ? dom->getResultProcessorWaiting() : 0.0;
}

/* ------------------------------ Queries ----------------------------- */

/*
 * Make sure deterministic phases are correct.
 */

void
Phase::check() const
{
    Sequence<Call *> nextCall( callList() );
    Call * aCall;

    while ( aCall = nextCall() ) {
	aCall->check();
    }

    if ( phaseTypeFlag() == PHASE_STOCHASTIC  && hasCV_sqr() ) {
	LQIO::solution_error( WRN_COEFFICIENT_OF_VARIATION, name().c_str() );	/* c, phase_flag are incompatible  */
    }

    Model::deterministicPhasesPresent  = Model::deterministicPhasesPresent  || phaseTypeFlag() == PHASE_DETERMINISTIC;
    Model::maxServiceTimePresent       = Model::maxServiceTimePresent       || maxServiceTime() > 0.0;
    Model::nonExponentialPhasesPresent = Model::nonExponentialPhasesPresent || isNonExponential();
    Model::serviceExceededPresent      = Model::serviceExceededPresent      || serviceExceeded() > 0.0;
    Model::variancePresent             = Model::variancePresent             || variance() > 0.0;
    Model::histogramPresent            = Model::histogramPresent            || hasHistogram();
}


const Phase&
Phase::setChain( const unsigned k, const Entity * aServer, callFunc aFunc ) const
{
    Sequence<Call *> nextCall( callList() );
    Call * aCall;

    while ( aCall = nextCall() ) {
	if ( aCall->isSelected() && (!aFunc || (aCall->*aFunc)()) && (!aServer || (aCall->dstTask() == aServer) ) ) {
	    aCall->setChain( k );
	}
    }

    return *this;
}



/*
 * Return the task of this phase.  Overridden by class activity.
 */

const Task *
Phase::owner() const
{
    return entry()->owner();
}

Phase&
Phase::histogram( const double min, const double max, const unsigned n_bins )
{
    myHistogram.set( min, max, n_bins );

    if ( n_bins == 0 ) {
	Model::maxServiceTimePresent = true;
    } else {
	Model::histogramPresent = true;
    }

    return *this;
}


Phase&
Phase::moments( const double mean, const double stddev, const double skew, const double kurtosis )
{
    myHistogram.moments( mean, stddev, skew, kurtosis );
    return *this;
}


Phase& 
Phase::histogramBin( const double begin, const double end, const double prob, const double conf95, const double conf99 )
{
    myHistogram.addBin( begin, end, prob, conf95, conf99 );
    return *this;
}


/*
 * Return the max service time (i.e., a histogram with 2 bins...)
 */

double 
Phase::maxServiceTime() const 
{ 
    return myHistogram.maxServiceTime();
}


const Cltn<Call *>& 
Phase::callList() const 
{ 
    return entry()->callList(); 
}


Phase&
Phase::recomputeCv_sqr( const Phase * aPhase )
{
    const double dst_time = LQIO::DOM::to_double(serviceTime());
    const double src_time = LQIO::DOM::to_double(aPhase->serviceTime());
    const double mean = dst_time + src_time;
    if ( mean > 0.0 ) {
	const LQIO::DOM::ConstantExternalVariable variance = Cv_sqr() * square(dst_time) + aPhase->Cv_sqr() * square(src_time);
	LQIO::DOM::ExternalVariable & cv_sqr = *const_cast<LQIO::DOM::ExternalVariable *>(getDOM()->getCoeffOfVariationSquared());
	cv_sqr = variance / square(mean);
    }
    return *this;
}


bool
Phase::isNonExponential() const
{
    const LQIO::DOM::Phase * dom = getDOM();
    if ( dom ) {
	const LQIO::DOM::ExternalVariable * var = dom->getCoeffOfVariationSquared();
	double value;
	return var && var->wasSet() && var->getValue(value) && value != 1.0;
    } else { 
	return false;
    }
}

bool 
Phase::hasCallsFor( unsigned p ) const
{
    Sequence<Call *> nextCall( callList() );
    const Call * aCall;
    while ( aCall = nextCall() ) {
	if ( aCall->isSelected() && (aCall->hasRendezvousForPhase( p ) || aCall->hasSendNoReplyForPhase( p ) ) ) return true;
    }
    return false;
}


/*
 * Compute the service time for this entry.
 * Used when generating queueing models and submodels (-S, -I, -T)
 * We subtract off all time to "selected" entries.
 */

double
Phase::serviceTimeForSRVNInput() const
{
#if 0
    double time = serviceTime();
    const unsigned p = phase();
    assert( p != 0 );

    /* Total time up to all lower level layers (not selected) */

    Sequence<Call *> nextCall( entry()->callList() );
    Call * aCall;
    while ( aCall = nextCall() ) {
	if ( !aCall->isSelected() && aCall->hasRendezvousForPhase(p) ) {
	    time += (*aCall->rendezvous(p)) * (aCall->waiting(p) + aCall->dstEntry()->executionTime(1));
	}
    }

    /* Add in processor queueing if it isn't selected */

    if ( !owner()->processor()->isSelected() ) {
	time += queueingTime();		/* queueing time is already multiplied my nRendezvous.  See lqns/parasrvn. */
    }
    
#else
    double time = 0.0;
#endif
    return time;
}


/*
 * Subtly different from serviceTimeForSRVNInput...  Clients don't get
 * their own service time.  It is always assigned to the processor.
 */

double
Phase::serviceTimeForQueueingNetwork() const
{
    double time = 0.0;

#if 0
    Sequence<Call *> nextCall( callList() );
    Call * aCall;

    const unsigned p = phase();
    while ( aCall = nextCall() ) {
	if ( !aCall->isSelected() && aCall->hasRendezvousForPhase(p) > 0.0 ) {
	    time += (*aCall->rendezvous(p)) * (aCall->waiting(p) + aCall->dstEntry()->executionTime(1));
	}
    }

    if ( !owner()->processor()->isSelected() ) {
	time += serviceTime() + queueingTime();		/* queueing time is already multiplied my nRendezvous.  See lqns/parasrvn. */
    }
#endif
    return time;
}



/*
 * Called by xxparse when we don't have a total.
 */

const Phase&
Phase::addThptUtil( double &util_sum ) const
{
    util_sum += utilization();
    return *this;
}
