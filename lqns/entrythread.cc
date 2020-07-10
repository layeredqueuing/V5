/* thread.cc	-- Greg Franks Fri May  2 2003
 * $Id: entrythread.cc 13676 2020-07-10 15:46:20Z greg $
 *
 */


#include "dim.h"
#include "entrythread.h"
#include "entity.h"
#include "task.h"
#include "lqns.h"
#include "actlist.h"
#include "fpgoop.h"
#include "pragma.h"
/* -------------------- Global External functions --------------------- */

double min( const Thread& a, const Thread& b )  
{ 
    DiscretePoints a_min = min( static_cast<const DiscretePoints>(a), static_cast<const DiscretePoints>(b) );
    return a_min.mean();
}

/*----------------------------------------------------------------------*/
/* Thread -- ...							*/
/*----------------------------------------------------------------------*/

Thread&
Thread::configure( const unsigned nSubmodels )
{
    Entry::configure( nSubmodels );
    myStartTime.resize( nSubmodels );
    return *this;
}


/*
 * Check the forks-versus joins.
 */

bool
Thread::check() const
{
    return myFork->check();
}



/*
 * Return the start time for this thread.
 */

Exponential
Thread::startTime() const
{
    return Exponential( myStartTime.sum(), myStartTimeVariance );
}



bool
Thread::isAncestorOf( const Thread * aThread ) const
{
    if ( !aThread ) return true;

    return aThread->isDescendentOf( this );
}


/*
 * Return true if the receiver is a descendent of aThread.
 */

bool
Thread::isDescendentOf( const Thread * aThread ) const
{
    return myFork->isDescendentOf( aThread->myFork );
}



/*
 * Return the waiting time for all submodels except submodel for phase
 * `p'.  If this is an activity entry, we have to return the chain k
 * component of waiting time.  Note that if submodel == 0, we return
 * the elapsedTime().  For servers in a submodel, submodel == 0; for
 * clients in a submodel, submodel == aSubmodel.number().
 */

double
Thread::waitExcept( const unsigned submodel, const unsigned k, const unsigned p ) const
{
    return _phase[p].waitExcept( submodel );
}



/*
 * 
 */

double
Thread::waitExceptChain( const unsigned submodel, const unsigned k, const unsigned p ) const
{
    return _phase[p].waitExceptChain( submodel, k );
}



/*
 * Calculate and set myIdleTime.  Note that population returns the
 * maximum number of customers possible at a station.  It is used,
 * rather than copies, because some multi-servers may have more
 * threads specified than can possibly be active.
 */

Thread&
Thread::setIdleTime( const double relax )
{
    double z;

    double joinDelayThroughput = 0;
    if (joinDelay() != 0.0) {joinDelayThroughput = (1.0/joinDelay());};

    if ( utilization() >= owner()->population() ) {
	z = 0.0;
    } else if ( throughput() > 0.0 ) {
#if HAVE_LIBGSL && HAVE_LIBGSLCBLAS
	switch ( pragma.getIdleTime() ) {

	case JOINDELAY_IDLETIME:
	    //The idle time of the thread in an AND fork activity list should not
	    //depend on the delay incurred outside the fork join list.... Bug 257
	    if (joinDelayThroughput != 0.0) {
		z = ( owner()->population() - utilization() ) / joinDelayThroughput ;
	    } else {
		z = 0;
	    }
	    break;

	default:
	    z = ( owner()->population() - utilization() ) /  throughput();
	    break;
	}
	/// end tomari quorum
#else
	z = ( owner()->population() - utilization() ) /  throughput();
#endif

    } else {
	z = get_infinity();	/* INFINITY */
    }

    if ( flags.trace_idle_time || flags.trace_throughput  ) {
	cout <<"\nThread::setIdleTime(): " << name() << endl ;
	cout <<"utilization=" << utilization();
	cout <<", population=" << owner()->population();
	cout <<", calculated (root Entry) throughput= " << throughput();
	cout <<", \njoinDelayThroughput= " << joinDelayThroughput << endl;
	cout << "Idle Time: " << z << endl;
    }
    under_relax( myThinkTime, z, relax );
    return *this;
}



/*
 * Initialize the start time of this thread.
 */

Thread&
Thread::startTime( const unsigned submodel, const double value )
{
    if ( submodel != 0 ) {
	myStartTime[submodel] = value;
	if (flags.trace_throughput || flags.trace_idle_time) {
	    cout <<"Thread::startTime():Thread " << name() << ", myStartTime[submodel="<<submodel<<"]=" << value << endl;
	}
    } else {
	myStartTimeVariance = value;
    }
    return *this;
}



/*
 * Find the three-point cummulative distribution function.
 * Initialize variables used in calculation here.  The 
 * superclass does the real work.
 */

Thread& 
Thread::estimateCDF()
{
    DiscretePoints::init( elapsedTime(), Entry::variance() );
    DiscretePoints::estimateCDF();    
    return *this;
}
