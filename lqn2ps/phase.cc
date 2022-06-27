/*  -*- c++ -*-
 * $Id: phase.cc 15718 2022-06-27 12:52:14Z greg $
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
#include <algorithm>
#include <cstdlib>
#include <string.h>
#include <cmath>
#include <lqio/error.h>
#include <lqio/dom_phase.h>
#include <lqio/dom_extvar.h>
#include "activity.h"
#include "call.h"
#include "entity.h"
#include "entry.h"
#include "errmsg.h"
#include "model.h"
#include "phase.h"
#include "processor.h"
#include "task.h"

Phase::Phase()
    : _dom(nullptr),
      _entry(0), 
      _phase(0)
{
}



Phase::~Phase()
{
}



Phase::Phase( const Phase& src )
    : _histogram( src._histogram ),
      _dom( src._dom ),
      _entry( src._entry ),
      _phase( src._phase )
{
}

/*
 * Assignment.  Used by rep2flat.  See bug 246.
 */

Phase&
Phase::operator=( const Phase& src )
{
    if ( *this == src ) return *this;

    _dom = src._dom;
    _phase = src._phase;
    _histogram = src._histogram;
    return *this;
}



/*
 * Set source entry and phase.
 */

Phase&
Phase::initialize( Entry * src, const unsigned p )
{
    assert ( 0 < p && p <= MAX_PHASES );
		
    _entry = src;
    _phase = p;
    return *this;
}

/* --------------------- Instance Variable access --------------------- */
const std::string& 
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
    return getDOM() != nullptr && !LQIO::DOM::ExternalVariable::isDefault( getDOM()->getCoeffOfVariationSquared(), 1.0 );
}


const LQIO::DOM::ExternalVariable&
Phase::Cv_sqr() const 
{ 
    return *getDOM()->getCoeffOfVariationSquared(); 
}


LQIO::DOM::Phase::Type
Phase::phaseTypeFlag() const 
{ 
    const LQIO::DOM::Phase * dom = getDOM();
    return dom ? dom->getPhaseTypeFlag() : LQIO::DOM::Phase::Type::STOCHASTIC;
}


Phase&
Phase::phaseTypeFlag( LQIO::DOM::Phase::Type aType ) 
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



/* +BUG_270 */

/* static */
const LQIO::DOM::ExternalVariable *
Phase::accumulate_service_time( const LQIO::DOM::ExternalVariable * augend, const std::pair<unsigned int, Phase>& phase )
{
    return Entity::addExternalVariables( augend, phase.second.getDOM()->getServiceTime() ); 
}


/* static */
const LQIO::DOM::ExternalVariable *
Phase::accumulate_think_time( const LQIO::DOM::ExternalVariable * augend, const std::pair<unsigned int, Phase>& phase )
{
    return Entity::addExternalVariables( augend, phase.second.getDOM()->getThinkTime() ); 
}
/* -BUG_270 */


/*
 * I only visit the processor once for all intents and purposes.
 */

/* static */ BCMP::Model::Station::Class
Phase::accumulate_demand( const BCMP::Model::Station::Class& augend, const std::pair<unsigned,Phase>& p )
{
    return augend + BCMP::Model::Station::Class( &Element::ONE, &p.second.serviceTime() );
}

/* static */ double
Phase::accumulate_execution( double augend, const std::pair<unsigned int, Phase>& addend )
{
    return augend + addend.second.executionTime();
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

bool 
Phase::check() const
{
    bool rc = for_each( calls().begin(), calls().end(), AndPredicate<Call>( &Call::check ) ).result();

    if ( phaseTypeFlag() == LQIO::DOM::Phase::Type::STOCHASTIC && hasCV_sqr() ) {
	if ( dynamic_cast<const Activity *>(this) ) {			/* c, phase_flag are incompatible  */
	    LQIO::solution_error( WRN_COEFFICIENT_OF_VARIATION, "Task", owner()->name().c_str(), getDOM()->getTypeName(), name().c_str() );
	} else {
	    LQIO::solution_error( WRN_COEFFICIENT_OF_VARIATION, "Entry", entry()->name().c_str(), getDOM()->getTypeName(), name().c_str() );
	}
    }

    Model::deterministicPhasesPresent  = Model::deterministicPhasesPresent  || phaseTypeFlag() == LQIO::DOM::Phase::Type::DETERMINISTIC;
    Model::maxServiceTimePresent       = Model::maxServiceTimePresent       || maxServiceTime() > 0.0;
    Model::nonExponentialPhasesPresent = Model::nonExponentialPhasesPresent || isNonExponential();
    Model::serviceExceededPresent      = Model::serviceExceededPresent      || serviceExceeded() > 0.0;
    Model::variancePresent             = Model::variancePresent             || variance() > 0.0;
    Model::histogramPresent            = Model::histogramPresent            || hasHistogram();
    return rc;
}


const Phase&
Phase::setChain( const unsigned k, const Entity * aServer, callPredicate aFunc ) const
{
    for_each ( calls().begin(), calls().end(), Phase::SetChain( k, aServer, aFunc ) );
    return *this;
}



void
Phase::SetChain::operator()( Call * call ) const
{
    if ( call->isSelected() && (!_f || (call->*_f)()) && (!_server || (call->dstTask() == _server) ) ) {
	call->setChain( _k );
    }
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
    _histogram.set( min, max, n_bins );

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
    _histogram.moments( mean, stddev, skew, kurtosis );
    return *this;
}


Phase& 
Phase::histogramBin( const double begin, const double end, const double prob, const double conf95, const double conf99 )
{
    _histogram.addBin( begin, end, prob, conf95, conf99 );
    return *this;
}


/*
 * Return the max service time (i.e., a histogram with 2 bins...)
 */

double 
Phase::maxServiceTime() const 
{ 
    return _histogram.hasMaxServiceTime();
}


const std::vector<Call *>& 
Phase::calls() const 
{ 
    return entry()->calls(); 
}


Phase&
Phase::recomputeCv_sqr( const Phase * aPhase )
{
    const double dst_time = LQIO::DOM::to_double(serviceTime());
    const double src_time = LQIO::DOM::to_double(aPhase->serviceTime());
    const double mean = dst_time + src_time;
    if ( mean > 0.0 ) {
	double variance = to_double(Cv_sqr()) * square(dst_time) + to_double(aPhase->Cv_sqr()) * square(src_time);
	const_cast<LQIO::DOM::Phase *>(getDOM())->setCoeffOfVariationSquaredValue(variance / square(mean));
    }
    return *this;
}


bool
Phase::isNonExponential() const
{
    const LQIO::DOM::Phase * dom = getDOM();
    if ( dom && dom->hasCoeffOfVariationSquared() ) {
	return dom->getCoeffOfVariationSquaredValue() != 1.0;
    } else { 
	return false;
    }
}


/*
 * merge values from src to dst.
 */

void
Phase::merge( LQIO::DOM::Phase& dst, const LQIO::DOM::Phase& src, double rate )
{
    dst.setServiceTimeValue( dst.getServiceTimeValue() + src.getServiceTimeValue() * rate );
    if ( (src.hasStochasticCalls() && dst.hasDeterministicCalls()) || (src.hasDeterministicCalls() && dst.hasStochasticCalls()) ) {
//	LQIO::solution_error( WRN_MIXED_PHASE_TYPE, ... );
    } else if ( src.hasDeterministicCalls() ) {
	dst.setPhaseTypeFlag( LQIO::DOM::Phase::Type::DETERMINISTIC );
    }
}



/*
 * Compute the service time for this entry.
 * Used when generating queueing models and submodels (-S, -I, -T)
 * We subtract off all time to "selected" entries.
 */

double
Phase::serviceTimeForSRVNInput() const
{
    double time = 0.0;
    try {
	double time = to_double(serviceTime());
	const unsigned p = phase();
	assert( p != 0 );

	/* Total time up to all lower level layers (not selected) */

	for ( std::vector<Call *>::const_iterator call = calls().begin(); call != calls().end(); ++call ) {
	    if ( !(*call)->isSelected() && (*call)->hasRendezvousForPhase(p) ) {
		time += to_double((*call)->rendezvous(p)) * ((*call)->waiting(p) + (*call)->dstEntry()->executionTime(1));
	    }
	}

	/* Add in processor queueing if it isn't selected */

	if ( std::any_of( owner()->processors().begin(), owner()->processors().end(), Predicate<Processor>( &Processor::isSelected ) ) ) {
	    time += queueingTime();		/* queueing time is already multiplied my nRendezvous.  See lqns/parasrvn. */
	}
    }
    catch ( const std::domain_error( &e ) ) {
    }
    return time;
}


#if defined(REP2FLAT)
static struct {
    set_function first;
    get_function second;
} phase_mean[] = { 
// static std::pair<set_function,get_function> phase_mean[] = {
    { &LQIO::DOM::DocumentObject::setResultServiceTime, &LQIO::DOM::DocumentObject::getResultServiceTime },
    { &LQIO::DOM::DocumentObject::setResultVarianceServiceTime, &LQIO::DOM::DocumentObject::getResultVarianceServiceTime },
    { &LQIO::DOM::DocumentObject::setResultUtilization, &LQIO::DOM::DocumentObject::getResultUtilization },
    { &LQIO::DOM::DocumentObject::setResultProcessorWaiting, &LQIO::DOM::DocumentObject::getResultProcessorWaiting },
    { NULL, NULL }
};

static struct {
    set_function first;
    get_function second;
} phase_variance[] = { 
//static std::pair<set_function,get_function> phase_variance[] = {
    { &LQIO::DOM::DocumentObject::setResultServiceTime, &LQIO::DOM::DocumentObject::getResultServiceTimeVariance },
    { &LQIO::DOM::DocumentObject::setResultVarianceServiceTimeVariance, &LQIO::DOM::DocumentObject::getResultVarianceServiceTimeVariance },
    { &LQIO::DOM::DocumentObject::setResultUtilizationVariance, &LQIO::DOM::DocumentObject::getResultUtilizationVariance },
    { &LQIO::DOM::DocumentObject::setResultProcessorWaitingVariance, &LQIO::DOM::DocumentObject::getResultProcessorWaitingVariance },
    { NULL, NULL }
};

/*
 * Strip suffix _<N>.  Merge results from replicas 2..N to 1.
 */

Phase&
Phase::replicatePhase( LQIO::DOM::Phase * root, unsigned int replica )
{
    if ( root == nullptr || getDOM() == nullptr ) return *this;

    if ( replica == 1 ) {
	std::string& name = const_cast<std::string&>(getDOM()->getName());
	if ( name.size() <= 2 ) return *this;
	size_t pos = name.rfind( '_' );
	char * end_ptr = NULL;
	if ( pos != std::string::npos && (strtol( &name[pos+1], &end_ptr, 10 ) == 1 && *end_ptr == '\0') ) {
	    name = name.substr( 0, pos );
	}
    } else {
	for ( unsigned int i = 0; phase_mean[i].first != NULL; ++i ) {
	    update_mean( root, phase_mean[i].first, getDOM(), phase_mean[i].second, replica );
	    update_variance( root, phase_variance[i].first, getDOM(), phase_variance[i].second );
	}
    }
    return *this;
}


/*
 * This is used to MERGE all calls to a single call.  Adjust back to original value??
 */

Phase&
Phase::replicateCall()
{
    if ( !getDOM() ) return *this;
    std::vector<LQIO::DOM::Call *>& calls = const_cast<std::vector<LQIO::DOM::Call *>&>(getDOM()->getCalls());
    std::vector<LQIO::DOM::Call *> old_calls = calls;
    calls.clear();
    for ( std::vector<LQIO::DOM::Call *>::iterator call = old_calls.begin(); call != old_calls.end(); ++call ) {
	const std::string& dst_name = (*call)->getDestinationEntry()->getName();
	size_t pos = dst_name.rfind( '_' );
	char * end_ptr = NULL;
	if ( pos == std::string::npos || (strtol( &dst_name[pos+1], &end_ptr, 10 ) == 1 && *end_ptr == '\0') ) {
	    calls.push_back( *call );
	}
    }
    return *this;
}
#endif

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
    bins.push_back( Bin( begin, end, prob, conf95, conf99 ) );
    return *this;
}
