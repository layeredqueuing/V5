/* thread.cc	-- Greg Franks Fri May  2 2003
 * $Id: entrythread.cc 14823 2021-06-15 18:07:36Z greg $
 *
 */


#include "lqns.h"
#include <mva/fpgoop.h>
#include "actlist.h"
#include "entity.h"
#include "entrythread.h"
#include "flags.h"
#include "pragma.h"
#include "task.h"

/* -------------------- Global External functions --------------------- */

double min( const Thread& a, const Thread& b )  
{ 
    DiscretePoints a_min = min( static_cast<const DiscretePoints>(a), static_cast<const DiscretePoints>(b) );
    return a_min.mean();
}

/*----------------------------------------------------------------------*/
/* Thread -- ...							*/
/*----------------------------------------------------------------------*/

Thread::Thread( const Activity * anActivity, const AndForkActivityList * fork ) 
    : VirtualEntry( anActivity ),
      DiscretePoints( 0.0, 0.0 ),
      _fork(fork),
      _think_time(0.0),
      _start_time_variance(0.0),
      _join_delay(0.0)
{
}



Thread::Thread( const Thread& src, unsigned int replica, const AndForkActivityList * fork )
    : VirtualEntry( src, replica ),
      DiscretePoints( 0.0, 0.0 ),
      _fork(fork),
      _think_time(0.0),
      _start_time_variance(0.0),
      _join_delay(0.0)
{
}



Thread&
Thread::configure( const unsigned nSubmodels )
{
    Entry::configure( nSubmodels );
    _start_time.resize( nSubmodels );
    return *this;
}


/*
 * Return the start time for this thread.
 */

Exponential
Thread::startTime() const
{
    return Exponential( _start_time.sum(), _start_time_variance );
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
    return _fork->isDescendentOf( aThread->_fork );
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



#if PAN_REPLICATION
/*
 * 
 */

double
Thread::waitExceptChain( const unsigned submodel, const unsigned k, const unsigned p ) const
{
    return _phase[p].waitExceptChain( submodel, k );
}
#endif



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

    if ( utilization() >= owner()->population() ) {
	z = 0.0;
    } else if ( throughput() > 0.0 ) {
#if HAVE_LIBGSL && HAVE_LIBGSLCBLAS
	switch ( Pragma::getQuorumIdleTime() ) {

	case Pragma::JOINDELAY_IDLETIME:
	    //The idle time of the thread in an AND fork activity list should not
	    //depend on the delay incurred outside the fork join list.... Bug 257
	    z = ( owner()->population() - utilization() ) * joinDelay();
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
	std::cout << "Thread::setIdleTime(): " << name() << std::endl
		  << "  utilization=" << utilization()
		  << ", population=" << owner()->population()
		  << ", calculated (root Entry) throughput= " << throughput()
		  << " Idle Time: " << z << std::endl;
    }
    under_relax( _think_time, z, relax );
    return *this;
}



/*
 * Initialize the start time of this thread.
 */

Thread&
Thread::startTime( const unsigned submodel, const double value )
{
    if ( submodel != 0 ) {
	_start_time[submodel] = value;
	if (flags.trace_throughput || flags.trace_idle_time) {
	    std::cout <<"Thread::startTime():Thread " << name() << ", _start_time[submodel="<<submodel<<"]=" << value << std::endl;
	}
    } else {
	_start_time_variance = value;
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
