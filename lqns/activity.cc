/* activity.c	-- Greg Franks Thu Feb 20 1997
 * $HeadURL: http://rads-svn.sce.carleton.ca:8080/svn/lqn/trunk/lqns/activity.cc $
 *
 * Everything you wanted to know about an activity, but were afraid to ask.
 *
 * Copyright the Real-Time and Distributed Systems Group,
 * Department of Systems and Computer Engineering,
 * Carleton University, Ottawa, Ontario, Canada. K1S 5B6
 *
 * February 1997
 * July 2007
 *
 * ------------------------------------------------------------------------
 * $Id: activity.cc 17190 2024-04-30 21:06:37Z greg $
 * ------------------------------------------------------------------------
 */


#include "lqns.h"
#include <numeric>
#include <cmath>
#include <lqio/error.h>
#include <lqio/dom_actlist.h>
#include "activity.h"
#include "actlist.h"
#include "call.h"
#include "entry.h"
#include "errmsg.h"
#include "flags.h"
#include "pragma.h"
#include "processor.h"
#include "task.h"
#if !HAVE_LIBGSL
#include "randomvar.h"
#endif

/* ---------------------------------------------------------------------- */

std::map<LQIO::DOM::ActivityList*, LQIO::DOM::ActivityList*> Activity::__actConnections;
std::map<LQIO::DOM::ActivityList*, ActivityList *> Activity::__domToNative;

/* static */ void
Activity::completeConnections()
{
    /* We stored all the necessary connections and resolved the list identifiers so finalize */
    std::map<LQIO::DOM::ActivityList*, LQIO::DOM::ActivityList*>::const_iterator iter;
    for (iter = __actConnections.begin(); iter != __actConnections.end(); ++iter) {
	ActivityList* src = __domToNative[iter->first];
	ActivityList* dst = __domToNative[iter->second];
	assert(src != nullptr && dst != nullptr);
	ActivityList::connect(src, dst);
    }
}


/* static */ void
Activity::clearConnectionMaps()
{
    __domToNative.clear();
    __actConnections.clear();
}


Activity::Count_If&
Activity::Count_If::operator=( const Activity::Count_If& src )
{
    _e = src._e;
    _f = src._f;
    _p = src._p;
    _replyAllowed = src._replyAllowed;
    _rate = src._rate;
    _sum = src._sum;
    return *this;
}

/*----------------------------------------------------------------------*/
/*                    Activities are like phases....                    */
/*----------------------------------------------------------------------*/

/*
 * Construct an activity.  Duplicate name.
 */

Activity::Activity( const Task * task, const std::string& name )
    : Phase(name),
      _task(task),			/* Owner */
      _prevFork(nullptr),
      _nextJoin(nullptr),
      _replyList(),
      _specified(false),
      _reachable(false),
      _replica_number(1),
#if HAVE_LIBGSL
      _remote_quorum_delay(),
      _local_quorum_delay(false),
#endif
      _throughput(0.)			/* result */
{
}


Activity::Activity( const Activity& src, const Task * task, unsigned int replica )
    : Phase(src,replica),
      _task(task),			/* Set by task constructor */
      _prevFork(nullptr),
      _nextJoin(nullptr),
      _replyList(),
      _specified(src._specified),
      _reachable(src._reachable),
      _replica_number(replica),
#if HAVE_LIBGSL
      _remote_quorum_delay(),
      _local_quorum_delay(false),
#endif
      _throughput(0.)			/* result */
{
    /* Reconstruct replylist by linking to the cloned entry */
    for ( std::set<const Entry *>::const_iterator entry = src._replyList.begin(); entry != src._replyList.end(); ++entry ) {
	_replyList.insert( Entry::find( (*entry)->name(), replica ) );
    }
}

/* ------------------------ Instance Methods -------------------------- */


/*
 * Check to see if this activity if referenced by any entry.
 */

Activity&
Activity::configure( const unsigned n )
{
    Phase::configure( n );
    return *this;
}


/*
 * Done before findChildren.
 */

bool
Activity::check() const
{
    if ( !isReachable() ) {
	getDOM()->runtime_error( LQIO::ERR_NOT_REACHABLE );
	return false;
    } else if ( !isSpecified() ) {
	getDOM()->runtime_error( LQIO::ERR_NOT_SPECIFIED );
	return false;
    } else {
	return Phase::check();
    }
}


ProcessorCall *
Activity::newProcessorCall( Entry * procEntry ) const
{
    return new ActivityProcessorCall( this, procEntry );
}


/*
 * Fork list (RValue)
 */

ActivityList *
Activity::prevFork( ActivityList * aList )
{
    if ( _prevFork ) {
	getDOM()->input_error( LQIO::ERR_DUPLICATE_ACTIVITY_RVALUE, _prevFork->getDOM()->getLineNumber() );
    } else if ( isStartActivity() && !entry()->isVirtualEntry() ) {
	getDOM()->input_error( LQIO::ERR_IS_START_ACTIVITY );
    } else {
	_prevFork = aList;
    }
    return aList;
}



/*
 * Join list (LValue)
 */

ActivityList *
Activity::nextJoin( ActivityList * aList )
{
    if ( _nextJoin ) {
	getDOM()->input_error( LQIO::ERR_DUPLICATE_ACTIVITY_LVALUE, _nextJoin->getDOM()->getLineNumber() );
    } else {
	_nextJoin = aList;
    }
    return aList;
}



/*
 * Clear the input and output lists as this activity is being reconnected as part of the quorum calculation.
 */

Activity&
Activity::resetInputOutputLists()
{
    _nextJoin = nullptr;
    _prevFork = nullptr;

    return *this;
}


/*
 * Set phase for this activity
 */

bool
Activity::repliesTo( const Entry * entry ) const
{
    return _replyList.find( entry ) != _replyList.end();
}

/* --------------------- Activity List Processing --------------------- */

/*
 * Recursively find all children and grand children from `this'.  As
 * we descend down, we bump the depth.  If our path's cross, we have a
 * loop and abort.  This method is called by the topological sorter
 * and is used to find fork-join pairs.
 */

unsigned
Activity::findChildren( Ancestors& ancestors ) const
{
    _reachable = true;

    if ( ancestors.find( this ) ) {
	throw activity_cycle( this, ancestors.getActivityStack() );
    }

    unsigned max_depth = Phase::findChildren( ancestors.getCallStack(), ancestors.isDirectPath() );
    if ( nextJoin() ) {
	ancestors.push_activity( this );
	max_depth = std::max( max_depth, nextJoin()->findChildren( ancestors ) );
	ancestors.pop_activity();
    }
    return max_depth;
}


const Activity&
Activity::backtrack( Backtrack::State& data ) const
{
    if ( prevFork() != nullptr ) prevFork()->backtrack( data );
    return *this;
}


/*
 * Follow the calls from the activity and whomever it calls.
 */

const Activity&
Activity::followInterlock( Interlock::CollectTable& path ) const
{
    Phase::followInterlock( path );		/* Follow calls from the activity. */
    if ( _nextJoin ) {
	Interlock::CollectTable branch( path, repliesTo( path.back() ) );
	_nextJoin->followInterlock( branch );    /* Now follow the activity path. */
    }
    return *this;
}



/*
 * Recursively search from this entry to any entry on myServer.  When
 * we pop back up the call stack we add all calling tasks for each arc
 * which calls myServer.  The task adder will ignore duplicates.
 *
 * Note: we can't short circuit the search because there may be interlocking
 * on multiple branches.
 */

bool
Activity::getInterlockedTasks( Interlock::CollectTasks& path ) const
{
    bool found = Phase::getInterlockedTasks( path );
    if ( ( !repliesTo( path.back() ) || !path.prune() ) && _nextJoin && _nextJoin->getInterlockedTasks( path ) ) found = true;
    return found;
}


unsigned
Activity::concurrentThreads( unsigned n ) const
{
    if ( _nextJoin ) {
	return _nextJoin->concurrentThreads( n );
    } else {
	return n;
    }
}



/*
 * Aggregate whatever aFunc into the entry at the top of stack.
 * Follow the activitylist and continue.
 */

Activity::Collect&
Activity::collect( std::deque<const Activity *>& activityStack, std::deque<Entry *>& entryStack, Activity::Collect& data ) const
{
    if ( std::find( activityStack.begin(), activityStack.end(), this ) != activityStack.end() ) {
	return data;
    }
    Collect::Function f = data.collect();
    (this->*f)( entryStack.back(), data );

    if ( repliesTo( entryStack.front() ) ) {
	data.setPhase(2);
    }
    if ( _nextJoin ) {
	activityStack.push_back( this );
	_nextJoin->collect( activityStack, entryStack, data );
	activityStack.pop_back();
    }
    return data;
}


Activity::Count_If&
Activity::count_if( std::deque<const Activity *>& activityStack, Activity::Count_If& data) const
{
    if ( std::find( activityStack.begin(), activityStack.end(), this ) != activityStack.end() ) {
	return data;
    }
    Predicate f = data.count_if();
    if ( (this->*f)( data ) ) {
	data += data.rate();
    }
    if ( _nextJoin ) {
	activityStack.push_back( this );
	_nextJoin->count_if( activityStack, data );
	activityStack.pop_back();
    }
    return data;
}



CallInfo::Item::collect_calls&
Activity::collect_calls( std::deque<const Activity *>& activityStack, CallInfo::Item::collect_calls& data ) const
{
    if ( std::find( activityStack.begin(), activityStack.end(), this ) != activityStack.end() ) {
	return data;
    }
    if ( repliesTo( &data.source() ) ) {
	data.setPhase(2);
    }
    data( *this );
    if ( _nextJoin ) {
	activityStack.push_back( this );
	_nextJoin->collect_calls( activityStack, data );
	activityStack.pop_back();
    }
    return data;
}


/*
 * Aggregate whatever aFunc into the entry at the top of stack.
 * Follow the activitylist and continue.
 */

void
Activity::callsPerform( Call::Perform& operation ) const
{
    Phase::callsPerform( operation );

    if ( _nextJoin ) {
	if ( repliesTo( operation.entry() ) ) {
	    Call::Perform g( operation, operation.rate(), 2 );
	    _nextJoin->callsPerform( g );
	} else {
	    _nextJoin->callsPerform( operation );
	}
    }
}


/*
 * Return index of destination entry.  If it is not found in the list
 * add it.
 */

Call *
Activity::findOrAddFwdCall( const Entry * entry )
{
    Call * aCall = findFwdCall( entry );

    if ( !aCall ) {
	aCall = new ActivityForwardedCall( this, entry );
    }
    return aCall;
}



/*
 * Return index of destination entry.  If it is not found in the list
 * add it.
 */

Call *
Activity::findOrAddCall( const Entry * entry, const queryFunc aFunc )
{
    Call * aCall = findCall( entry, aFunc );

    if ( !aCall ) {
	aCall = new ActivityCall( this, entry );
    }
    return aCall;
}

/* ----------------------- Quourm calculation ------------------------- */

//Estimate the Cumulative Distribution Function (CDF) for the thread service time.

bool
Activity::estimateQuorumJoinCDFs (DiscretePoints & sumTotal,
				  DiscreteCDFs & quorumCDFs,
				  DiscreteCDFs & localCDFs,
				  DiscreteCDFs & remoteCDFs,
				  const bool isThereQuorumDelayedThreads,
				  bool & isQuorumDelayedThreadsActive,
				  double &totalParallelLocal,
				  double &totalSequentialLocal)
{
    if (flags.trace_quorum) {
        std::cout <<"\n..............start Activity::estimateQuorumJoinCDFs ():";
	std::cout <<" currActivity is " << name() << std::endl;
    }

    bool anError = false;
    DiscretePoints sumLocal;
    DiscretePoints sumRemote;
    double level1Mean =0; //total local processing time for all slices.
    double level2Mean = 0; //per call remote processing time.
    double avgNumCallsToLevel2Tasks = 0;

    getLevelMeansAndNumberOfCalls(level1Mean, level2Mean,avgNumCallsToLevel2Tasks);

    if ( level2Mean > 0) {
	totalParallelLocal += level1Mean;
    } else {
	totalSequentialLocal += level1Mean;
    }

#if HAVE_LIBGSL && HAVE_LIBGSLCBLAS
    if ( localQuorumDelay() ) {

	switch ( Pragma::getQuorumDistribution() ) {
	case Pragma::QuorumDistribution::THREEPOINT:
	    estimateThreepointQuorumCDF(level1Mean, level2Mean,
					avgNumCallsToLevel2Tasks, sumTotal, sumLocal, sumRemote, quorumCDFs,
					localCDFs, remoteCDFs, isThereQuorumDelayedThreads,
					isQuorumDelayedThreadsActive );
	    break;

	default: //GAMMA_QUORUM_DISTRIBUTION
	    estimateGammaQuorumCDF(LQIO::DOM::Phase::Type::DETERMINISTIC ,level1Mean, level2Mean,
				   avgNumCallsToLevel2Tasks, sumTotal, sumLocal, sumRemote, quorumCDFs,
				   localCDFs,remoteCDFs,  isThereQuorumDelayedThreads, isQuorumDelayedThreadsActive);
	}

    } else {

	switch (phaseTypeFlag()){
	case LQIO::DOM::Phase::Type::DETERMINISTIC:
	    switch ( Pragma::getQuorumDistribution() ) {

	    case Pragma::QuorumDistribution::THREEPOINT:
		estimateThreepointQuorumCDF(level1Mean, level2Mean,
					    avgNumCallsToLevel2Tasks, sumTotal, sumLocal, sumRemote, quorumCDFs,
					    localCDFs, remoteCDFs, isThereQuorumDelayedThreads,
					    isQuorumDelayedThreadsActive );
		break;

	    case Pragma::QuorumDistribution::CLOSEDFORM_DETRMINISTIC:
		estimateClosedFormDetQuorumCDF(level1Mean, level2Mean,
					       avgNumCallsToLevel2Tasks, sumTotal, sumLocal, sumRemote, quorumCDFs,
					       localCDFs, remoteCDFs, isThereQuorumDelayedThreads,
					       isQuorumDelayedThreadsActive );
		break;

	    default: //GAMMA_QUORUM_DISTRIBUTION:
		estimateGammaQuorumCDF(LQIO::DOM::Phase::Type::DETERMINISTIC ,level1Mean, level2Mean,
				       avgNumCallsToLevel2Tasks, sumTotal, sumLocal, sumRemote, quorumCDFs,
				       localCDFs,remoteCDFs,  isThereQuorumDelayedThreads, isQuorumDelayedThreadsActive);
	    }

	    break;

	default: //PHASE_STOCHASTIC:
	    switch ( Pragma::getQuorumDistribution() ) {

	    case Pragma::QuorumDistribution::THREEPOINT:
		estimateThreepointQuorumCDF(level1Mean, level2Mean,
					    avgNumCallsToLevel2Tasks, sumTotal, sumLocal, sumRemote, quorumCDFs,
					    localCDFs, remoteCDFs, isThereQuorumDelayedThreads,
					    isQuorumDelayedThreadsActive );
		break;

	    case Pragma::QuorumDistribution::GAMMA:
		estimateGammaQuorumCDF(LQIO::DOM::Phase::Type::STOCHASTIC ,level1Mean, level2Mean,
				       avgNumCallsToLevel2Tasks, sumTotal, sumLocal, sumRemote, quorumCDFs,
				       localCDFs,remoteCDFs,  isThereQuorumDelayedThreads, isQuorumDelayedThreadsActive);
		break;

	    default: //CLOSEDFORM_GEOMETRIC_QUORUM_DISTRIBUTION:
		estimateClosedFormGeoQuorumCDF(level1Mean, level2Mean,
					       avgNumCallsToLevel2Tasks, sumTotal, sumLocal, sumRemote, quorumCDFs,
					       localCDFs, remoteCDFs, isThereQuorumDelayedThreads,
					       isQuorumDelayedThreadsActive );
	    }
	}
    }
#else
    estimateThreepointQuorumCDF(level1Mean, level2Mean,
				avgNumCallsToLevel2Tasks, sumTotal, sumLocal, sumRemote, quorumCDFs,
				localCDFs, remoteCDFs, isThereQuorumDelayedThreads,
				isQuorumDelayedThreadsActive );
#endif

    if (flags.trace_quorum) {
        std::cout <<"\nsumTotal.mean= " << sumTotal.mean() <<" ,Variance="<<sumTotal.variance() << std::endl;
        std::cout <<"sumLocal.mean= " << sumLocal.mean() <<" ,Variance="<<sumLocal.variance() << std::endl;
        std::cout <<"sumRemote.mean= " << sumRemote.mean() <<" ,Variance="<<sumRemote.variance() << std::endl;
#if HAVE_LIBGSL
        std::cout <<"_remote_quorum_delay.mean= " << _remote_quorum_delay.mean()
	     <<" ,Variance="<<remoteQuorumDelay.variance() << std::endl;
#endif

        std::cout <<".........................end Activity::estimateQuorumJoinCDFs ()" << std::endl;
    }
    return !anError;
}



bool
Activity::estimateThreepointQuorumCDF(double level1Mean,
				      double level2Mean,  double avgNumCallsToLevel2Tasks,
				      DiscretePoints & sumTotal, DiscretePoints & sumLocal,
				      DiscretePoints & sumRemote, DiscreteCDFs & quorumCDFs,
				      DiscreteCDFs & localCDFs, DiscreteCDFs & remoteCDFs,
				      const bool isThereQuorumDelayedThreads,
				      bool & isQuorumDelayedThreadsActive)
{
    bool anError = false;

    if (flags.trace_quorum) {
	std::cout <<"\nThreepoint fitting for quorum is used." << std::endl;
    }

#if HAVE_LIBGSL && HAVE_LIBGSLCBLAS
    if ( isThereQuorumDelayedThreads &&
	 Pragma::getQuorumDelayedCalls() == Pragma::QuorumDelayedCalls::KEEP_ALL) {
	//Three-point is not the recommended distribution to be used with quorum anyways.
	//But this might be used only for comparison purposes to other distributions.

	sumLocal.mean(level1Mean); //mean=k*theta for a gamma distribution.
	sumRemote.mean(level2Mean * avgNumCallsToLevel2Tasks);

	if (phaseTypeFlag() == LQIO::DOM::Phase::Type::DETERMINISTIC )  {
	    //the sum of deterministic number of exponentially distributed RVs is Erlang or Gamma.
	    sumLocal.variance( level1Mean * level1Mean / (avgNumCallsToLevel2Tasks +1 )  );
	} else {
	    //PHASE_STOCHASTIC or Geometric calls.
	    // The sum of a geometric number of exponentially distributed random variables
	    // is exponentially distributed.
	    sumLocal.variance( level1Mean * level1Mean );
	}

	//The quorum delay remote part does not necessarily have a gamma or a
	//closed-form-geo distribution.
	double level2Variance = 0;
	if (avgNumCallsToLevel2Tasks > 0) {
	    level2Variance= sumTotal.variance() - sumLocal.variance();
	}
	if (level2Variance > 0) {
	    sumRemote.variance(level2Variance ) ;
	} else {
	    if (level2Variance < 0 && flags.trace_quorum) {
		std::cout <<"Activity::estimateQuorumJoinCDFs(): Warning1:" << std::endl;
		std::cout <<"variance is less than zero. sumRemote.variance=" <<level2Variance << std::endl;
		std::cout <<"If this happens outside initialization, it might be problematic." << std::endl;
	    }
	    sumRemote.variance(0);
	}
	localCDFs.addCDF(sumLocal.estimateCDF());
	remoteCDFs.addCDF(sumRemote.estimateCDF());
    }
#endif

    quorumCDFs.addCDF(sumTotal.estimateCDF());


#if HAVE_LIBGSL && HAVE_LIBGSLCBLAS
    if ( _remote_quorum_delay.mean() > 0) {
	quorumCDFs.addCDF( _remote_quorum_delay.estimateCDF() );
	isQuorumDelayedThreadsActive = true;
    }
#endif

    return !anError;
}


#if HAVE_LIBGSL && HAVE_LIBGSLCBLAS
bool
Activity::estimateGammaQuorumCDF(LQIO::DOM::Phase::Type phaseTypeFlag,
				 double level1Mean,  double level2Mean,  double avgNumCallsToLevel2Tasks,
				 DiscretePoints & sumTotal, DiscretePoints & sumLocal,
				 DiscretePoints & sumRemote, DiscreteCDFs & quorumCDFs,
				 DiscreteCDFs & localCDFs, DiscreteCDFs & remoteCDFs,
				 const bool isThereQuorumDelayedThreads,
				 bool & isQuorumDelayedThreadsActive)
{
    bool anError = false;

    if (flags.trace_quorum) {
	std::cout <<"\nGamma fitting for quorum is used." << std::endl;
    }
    if ( isThereQuorumDelayedThreads &&
	 Pragma::getQuorumDelayedCalls() == Pragma::QuorumDelayedCalls::KEEP_ALL ) {

	sumLocal.mean(level1Mean); //mean=k*theta for a gamma distribution.
	sumRemote.mean(level2Mean * avgNumCallsToLevel2Tasks);

	if (phaseTypeFlag == LQIO::DOM::Phase::Type::DETERMINISTIC ) {
	    //the sum of deterministic number of exponentially distributed RVs is Erlang or Gamma.
	    sumLocal.variance( level1Mean * level1Mean / (avgNumCallsToLevel2Tasks + 1 )  );      					}
	else {
	    // The sum of a geometric number of exponentially distributed random variables
	    // is exponentially distributed.
	    sumLocal.variance( level1Mean * level1Mean );
	}

	//The remote part does not necessarily have a gamma or a
	//closed-form-geo distribution.
	double level2Variance = 0;
	if (avgNumCallsToLevel2Tasks > 0) {
	    level2Variance= sumTotal.variance() - sumLocal.variance();
	}
	if (level2Variance > 0) {
	    sumRemote.variance(level2Variance ) ;
	} else {
	    if (level2Variance < 0 && flags.trace_quorum) {
		std::cout <<"Activity::estimateQuorumJoinCDFs(): Warning2:" << std::endl;
		std::cout <<"variance is less than zero. sumRemote.variance="
		     <<level2Variance << std::endl;
		std::cout <<"If this happens outside initialization, it might be problematic." << std::endl;
	    }
	    sumRemote.variance(0);
	}

	localCDFs.addCDF(sumLocal.calcGammaPoints());
	remoteCDFs.addCDF(sumRemote.calcGammaPoints());
    }

    quorumCDFs.addCDF(sumTotal.calcGammaPoints());

    if ( _remote_quorum_delay.mean() > 0) {
	quorumCDFs.addCDF( _remote_quorum_delay.calcGammaPoints() );
	isQuorumDelayedThreadsActive = true;

    }

    return !anError;
}



bool
Activity::estimateClosedFormDetQuorumCDF(double level1Mean,
					 double level2Mean,  double avgNumCallsToLevel2Tasks,
					 DiscretePoints & sumTotal, DiscretePoints & sumLocal,
					 DiscretePoints & sumRemote, DiscreteCDFs & quorumCDFs,
					 DiscreteCDFs & localCDFs, DiscreteCDFs & remoteCDFs,
					 const bool isThereQuorumDelayedThreads,
					 bool & isQuorumDelayedThreadsActive)
{
    bool anError = false;

    if (flags.trace_quorum) {
	std::cout <<"\nClosed-form for deterministic fitting for quorum is used." << std::endl;
    }


    if (isThereQuorumDelayedThreads
	&& Pragma::getQuorumDelayedCalls() == Pragma::QuorumDelayedCalls::KEEP_ALL) {

	localCDFs.addCDF(sumLocal.closedFormDetPoints(0, level1Mean,0 ));
	remoteCDFs.addCDF(sumRemote.closedFormDetPoints(avgNumCallsToLevel2Tasks,
							0, level2Mean));
    }

    quorumCDFs.addCDF(sumTotal.closedFormDetPoints(avgNumCallsToLevel2Tasks,level1Mean, level2Mean));
    if (  _remote_quorum_delay.mean() > 0) {
	//quorumCDFs.addCDF( remoteQuorumDelay.closedFormGeoPoints
	//(1,0,remoteQuorumDelay.mean()) );
	quorumCDFs.addCDF( _remote_quorum_delay.calcGammaPoints() );
	isQuorumDelayedThreadsActive = true;
    }

    return !anError;
}


bool
Activity::estimateClosedFormGeoQuorumCDF(double level1Mean,
					 double level2Mean,  double avgNumCallsToLevel2Tasks,
					 DiscretePoints & sumTotal, DiscretePoints & sumLocal,
					 DiscretePoints & sumRemote, DiscreteCDFs & quorumCDFs,
					 DiscreteCDFs & localCDFs, DiscreteCDFs & remoteCDFs,
					 const bool isThereQuorumDelayedThreads,
					 bool & isQuorumDelayedThreadsActive)
{
    bool anError = false;

    if (flags.trace_quorum) {
	std::cout <<"\nClosed-form for geometric fitting for quorum is used. " << std::endl;
    }

    if (isThereQuorumDelayedThreads
	&& Pragma::getQuorumDelayedCalls() == Pragma::KEEP_ALL_QUORUM_DELAYED_CALLS) {

	// The sum of a geometric number of exponentially distributed random variables
	// is exponentially distributed.
	localCDFs.addCDF(sumLocal.closedFormGeoPoints(0, level1Mean,0 ));
	remoteCDFs.addCDF(sumRemote.closedFormGeoPoints(avgNumCallsToLevel2Tasks,
							0, level2Mean));
    }

    quorumCDFs.addCDF(sumTotal.closedFormGeoPoints(avgNumCallsToLevel2Tasks,level1Mean, level2Mean));
    if (  _remote_quorum_delay.mean() > 0) {
	//quorumCDFs.addCDF( remoteQuorumDelay.closedFormGeoPoints
	//(1,0,remoteQuorumDelay.mean()) );
	quorumCDFs.addCDF( _remote_quorum_delay.calcGammaPoints() );
	isQuorumDelayedThreadsActive = true;
    }

    return !anError;
}
#endif


bool
Activity::getLevelMeansAndNumberOfCalls(double & level1Mean,
					double & level2Mean,
					double & avgNumCallsToLevel2Tasks )
{
    bool anError = false;
    unsigned int currentSubmodel = owner()->submodel();
    currentSubmodel++; //As a client the actual submodel is submodel++.
    if (flags.trace_quorum) {
	std::cout <<"myActivityList[i]->owner()->submodel()=" << currentSubmodel<< std::endl;
    }

#if PAN_REPLICATION
    if (owner()->replicas() > 1) {
	level1Mean = getReplicationProcWait(currentSubmodel);
	// std::cout <<"\ngetReplicationProcWait=" <<
	//getReplicationProcWait(currentSubmodel,relax) << std::endl ;
    } else {
#endif
	level1Mean = getProcWait( currentSubmodel );
#if PAN_REPLICATION
    }
#endif
    if (flags.trace_quorum) {
	std::cout <<  "level1Mean=" <<  level1Mean << std::endl;
    }

#if PAN_REPLICATION
    if (owner()->replicas() > 1) {
	level2Mean =  getReplicationTaskWait();
	// std::cout <<"\ngetReplicationTaskWait=" <<
	//getReplicationTaskWait(currentSubmodel,relax) << std::endl ;
    } else {
#endif
	level2Mean = getTaskWait( currentSubmodel );
#if PAN_REPLICATION
    }
#endif
    if (flags.trace_quorum) {
	std::cout <<  "level2Mean=" <<  level2Mean << std::endl;
    }

#if PAN_REPLICATION
    if (owner()->replicas() > 1) {
	avgNumCallsToLevel2Tasks =getReplicationRendezvous( currentSubmodel );
	//std::cout <<"\ngetReplicationRendezvous=" <<
	//getReplicationRendezvous(currentSubmodel,relax) << std::endl ;
    } else {
#endif
	avgNumCallsToLevel2Tasks = getRendezvous(currentSubmodel );
#if PAN_REPLICATION
    }
#endif

    if (flags.trace_quorum) {
	std::cout << "Task rendevous = " <<  avgNumCallsToLevel2Tasks << std::endl;
    }

    return !anError;
}

/* --------------------------- List processing -------------------------- */
/*
 * Return the number of replies generated by this activity for this entry.
 */

bool
Activity::checkReplies( Activity::Count_If& data ) const
{
    const Entry * entry = data.entry();
    if ( repliesTo( entry ) ) {
	if (  entry->isCalledUsingSendNoReply() || entry->isCalledUsingOpenArrival() ) {
	    getDOM()->runtime_error( LQIO::ERR_INVALID_REPLY_FOR_SNR_ENTRY, entry->name().c_str() );
	} else if ( !data.canReply() ) {
	    getDOM()->runtime_error( LQIO::ERR_INVALID_REPLY_FROM_BRANCH, entry->name().c_str() );
	} else if ( data.phase() > 1 ) {
	    getDOM()->runtime_error( LQIO::ERR_INVALID_REPLY_DUPLICATE, entry->name().c_str() );
	}
	data.setPhase(2);
	return true;
    } else if ( data.phase() > 1 ) {
	const_cast<Entry *>(entry)->setMaxPhase( data.phase() );
    }
    return false;
}

/*
 * If submodel != 0, then we have the mean time for the submodel.
 * Overwise, we have the variance.
 */

void
Activity::collectWait( Entry * entry, const Activity::Collect& data ) const
{
    const unsigned int submodel = data.submodel();
    const unsigned int p = data.phase();
    if ( submodel > 0 ) {
	entry->_phase[p]._wait[submodel] += _wait[submodel];
	if ( flags.trace_activities ) {
	    std::cout << "Activity " << name() << " aggregate to ";
	    if ( dynamic_cast<VirtualEntry *>(entry) ) {
		std::cout << " virtual entry ";
	    } else {
		std::cout << " actual entry  ";
	    }
	    std::cout << entry->name() << ", submodel " << submodel << ", phase " << p
		 << ": wait " << _wait[submodel] << std::endl;
	}
    } else {
	entry->_phase[p].addVariance( variance() );
    }
}



#if PAN_REPLICATION
/*
 * Add up all of the surrogate delays.
 */

void
Activity::collectReplication( Entry * entry, const Activity::Collect& data ) const
{
    const unsigned int submodel = data.submodel();
    const unsigned int p = data.phase();
    assert( submodel > 0 );

    entry->_phase[p]._surrogateDelay += _surrogateDelay;
    if ( flags.trace_replication ) {
	std::cout << "\nActivity of (submodel ="<<submodel<< " )  " << name()<<" SurrogateDelay = " << _surrogateDelay << std::endl;
    }

}
#endif



/*
 * Add the service time from this activity to entry.
 * Access the instance variables directly because of side effects with setServicetime().
 */

void
Activity::collectServiceTime( Entry * entry, const Activity::Collect& data ) const
{
    entry->addServiceTime( data.phase(), serviceTime() );
}


/*+ BUG_425 */
void
Activity::collectCustomers( Entry * entry, const Activity::Collect& data ) const
{
    const_cast<Activity *>(this)->initCustomers( const_cast<Activity::Collect&>(data).taskStack(), data.customers() );
}
/*- BUG_425 */


/*
 * Set our throughput by chasing back to all entries that call me.
 */

void
Activity::setThroughput( Entry * entry, const Activity::Collect& ) const
{
    const_cast<Activity *>(this)->_throughput = entry->throughput();
}

/* ------------------------- Printing Functions ------------------------- */

/*
 *  For XML output
 */

const Activity&
Activity::insertDOMResults() const
{
    if ( getReplicaNumber() != 1 ) return *this;		/* NOP */

    Phase::insertDOMResults();
    LQIO::DOM::Activity* domActivity = getDOM();

    domActivity->setResultSquaredCoeffVariation(CV_sqr())
	.setResultThroughput(throughput())
	.setResultProcessorUtilization(processorUtilization());

    for ( std::set<Call *>::const_iterator call = callList().begin(); call != callList().end(); ++call ) {
	LQIO::DOM::Call* domCall = const_cast<LQIO::DOM::Call*>((*call)->getDOM());
	domCall->setResultWaitingTime((*call)->queueingTime());
    }
    return *this;
}

/* ---------------------------------------------------------------------- */
/*                           Left side (joins)                            */
/* ---------------------------------------------------------------------- */

/*
 * Add activity to the sequence list.
 */

ActivityList *
Activity::act_join_item( LQIO::DOM::ActivityList * dom_activitylist )
{
    ActivityList * activity_list = nextJoin( new JoinActivityList( const_cast<Task *>(dynamic_cast<const Task *>(owner())), dom_activitylist ) );
    activity_list->add( this );
    return activity_list;
}

/*
 * Add activity to the activity_list.  This list is for AND joining.  Used by task.cc for quorum.
 */

ActivityList *
Activity::act_and_join_list ( ActivityList * activityList, LQIO::DOM::ActivityList * dom_activitylist )
{
    if ( !activityList ) {
	activityList = new AndJoinActivityList( const_cast<Task *>(dynamic_cast<const Task *>(owner())), dom_activitylist );
    } else if ( !dynamic_cast<AndJoinActivityList *>(activityList) ) {
	abort();		// Wrong list type -- quoi?
    }
    nextJoin( activityList );
    activityList->add( this );

#if !HAVE_GSL_GSL_MATH_H       // QUORUM
    if ( dynamic_cast<LQIO::DOM::AndJoinActivityList*>(dom_activitylist) && dynamic_cast<LQIO::DOM::AndJoinActivityList*>(dom_activitylist)->hasQuorumCount() ) {
	LQIO::input_error( LQIO::ERR_NOT_SUPPORTED, "quorum" );
    }
#endif
    return activityList;
}



/*
 * Add activity to the activity_list.  This list is for OR joining.
 */

ActivityList *
Activity::act_or_join_list ( ActivityList * activityList, LQIO::DOM::ActivityList * dom_activitylist )
{
    if ( !activityList ) {
	activityList = new OrJoinActivityList( const_cast<Task *>(dynamic_cast<const Task *>(owner())), dom_activitylist );
    } else if ( !dynamic_cast<OrJoinActivityList *>(activityList) ) {
	abort();
    }
    nextJoin( activityList );
    activityList->add( this );
    return activityList;
}

/* ---------------------------------------------------------------------- */
/*                          Right side (forks)                            */
/* ---------------------------------------------------------------------- */

/*
 * Add activity to the sequence list.  Used by task.cc for quorum.
 */

ActivityList *
Activity::act_fork_item( LQIO::DOM::ActivityList * dom_activitylist )
{
    ActivityList * activityList = prevFork( new ForkActivityList( const_cast<Task *>(dynamic_cast<const Task *>(owner())), dom_activitylist ) );
    activityList->add( this );
    return activityList;
}



/*
 * Add activity to the activity_list.  This list is for AND forking.  Used by task.cc for quorum.
 */

ActivityList *
Activity::act_and_fork_list ( ActivityList * activityList, LQIO::DOM::ActivityList * dom_activitylist )
{
    if ( isStartActivity() ) {
	getDOM()->input_error( LQIO::ERR_IS_START_ACTIVITY );
	return activityList;
    } else if ( !activityList ) {
	activityList = new AndForkActivityList( const_cast<Task *>(dynamic_cast<const Task *>(owner())), dom_activitylist );
    } else if ( !dynamic_cast<AndForkActivityList *>(activityList) ) {
	abort();
    }

    activityList->add( this );
    prevFork( activityList );

    return activityList;
}



/*
 * Add activity to the activity_list.  This list is for OR forking.
 */

ActivityList *
Activity::act_or_fork_list ( ActivityList * activityList, LQIO::DOM::ActivityList * dom_activitylist )
{
    if ( isStartActivity() ) {
	getDOM()->input_error( LQIO::ERR_IS_START_ACTIVITY );
	return activityList;
    } else if ( !activityList ) {
	activityList = new OrForkActivityList( const_cast<Task *>(dynamic_cast<const Task *>(owner())), dom_activitylist );
    } else if ( !dynamic_cast<OrForkActivityList *>(activityList) ) {
	abort();
    }

    prevFork( activityList );
    activityList->add( this );

    return activityList;
}


/*
 * Add activity to the loop list.
 */

ActivityList *
Activity::act_loop_list ( ActivityList * activity_list, LQIO::DOM::ActivityList * dom_activitylist )
{
    if ( !activity_list ) {
	activity_list = new RepeatActivityList( const_cast<Task *>(dynamic_cast<const Task *>(owner())), dom_activitylist );
    } else if ( !dynamic_cast<RepeatActivityList *>(activity_list ) ) {
	abort();
    }

    activity_list->add( this );
    prevFork( activity_list );

    return activity_list;
}

/* ------------------------ Exception Handling ------------------------ */

activity_cycle::activity_cycle( const Activity * activity, const std::deque<const Activity *>& activityStack )
    : std::runtime_error( std::accumulate( activityStack.rbegin(), activityStack.rend(), activity->name(), &Activity::fold ) )
{
}

/************************************************************************/
/*                     Functions called by parser.                      */
/************************************************************************/


/*
 * Set the service time for the activity.
 */


void
store_activity_service_time ( void * task, const char * activity_name, const double service_time )
{
    if ( !task ) return;
    Activity * activity = static_cast<Task *>(task)->findOrAddActivity( activity_name );
    activity->isSpecified( true );
    if ( activity->serviceTime() ) {
	LQIO::input_error( LQIO::WRN_MULTIPLE_SPECIFICATION );
    }
    activity->setServiceTime( service_time );
}


/************************************************************************/
/*                     Functions called by loader.                      */
/************************************************************************/

Activity* add_activity( Task* newTask, LQIO::DOM::Activity* activity_dom )
{
    /* Create a new activity assigned to a given task and set the information DOM entry for it */
    Activity * activity = newTask->findOrAddActivity( activity_dom->getName() );
    activity->setDOM(activity_dom);

    /* Find out if we can specify the activity */
    if (activity_dom->isSpecified() == true) {
	activity->isSpecified(true);
    }

    return activity;
}


Activity&
Activity::add_calls()
{
    /* Go over all of the calls specified within this activity and do something similar to store_snr/rnv */
    LQIO::DOM::Activity* domActivity = getDOM();
    const std::vector<LQIO::DOM::Call*>& callList = domActivity->getCalls();
    std::vector<LQIO::DOM::Call*>::const_iterator iter;

    /* This provides us with the DOM Call which we can then take apart */
    for (iter = callList.begin(); iter != callList.end(); ++iter) {
	LQIO::DOM::Call* domCall = *iter;
	LQIO::DOM::Entry* toDOMEntry = const_cast<LQIO::DOM::Entry*>(domCall->getDestinationEntry());
	Entry* destEntry = Entry::find(toDOMEntry->getName());

	/* Make sure all is well */
	if (!destEntry) {
	    LQIO::input_error( LQIO::ERR_NOT_DEFINED, toDOMEntry->getName().c_str() );
	} else if (!destEntry->owner()->isReferenceTask()) {
	    isSpecified(true);
	    if (domCall->getCallType() == LQIO::DOM::Call::Type::SEND_NO_REPLY) {
		sendNoReply(destEntry, domCall);
	    } else if (domCall->getCallType() == LQIO::DOM::Call::Type::RENDEZVOUS) {
		rendezvous(destEntry, domCall);
	    }
	}
    }
    return *this;
}

/* ---------------------------------------------------------------------- */

Activity&
Activity::add_reply_list ()
{
    /* This information is stored in the LQIO DOM itself */
    const std::vector<LQIO::DOM::Entry*>& domReplyList = getDOM()->getReplyList();

    /* Walk over the list and do the equivalent of calling act_add_reply n times */
    for ( std::vector<LQIO::DOM::Entry*>::const_iterator domEntry = domReplyList.begin(); domEntry != domReplyList.end(); ++domEntry) {
	Entry* entry = Entry::find((*domEntry)->getName());

	/* Check it out and add it to the list */
	if (entry == nullptr) {
	    LQIO::input_error( LQIO::ERR_NOT_DEFINED, (*domEntry)->getName().c_str() );
	} else if ( owner()->isReferenceTask() ) {
	    getDOM()->input_error( LQIO::ERR_REFERENCE_TASK_REPLIES, (*domEntry)->getName().c_str() );
	} else if (entry->owner() != owner()) {
	    getDOM()->input_error( LQIO::ERR_WRONG_TASK_FOR_ENTRY, owner()->name().c_str() );
	} else {
	    _replyList.insert(entry);
	}
    }
    return *this;
}


Activity&
Activity::add_activity_lists()
{
    /* Obtain the Task and Activity information DOM records */
    LQIO::DOM::Activity* domAct = getDOM();
    if (domAct == nullptr) { return *this; }
    const Task * task = dynamic_cast<const Task *>(owner());

    /* May as well start with the nextJoin, this is done with various methods */
    LQIO::DOM::ActivityList* joinList = domAct->getOutputToList();
    ActivityList * localActivityList = nullptr;
    if (joinList != nullptr && __domToNative.find(joinList) == __domToNative.end()) {
	const std::vector<const LQIO::DOM::Activity*>& list = joinList->getList();
	for (std::vector<const LQIO::DOM::Activity*>::const_iterator iter = list.begin(); iter != list.end(); ++iter) {
	    const LQIO::DOM::Activity* domAct = *iter;

	    /* Add the activity to the appropriate list based on what kind of list we have */
	    Activity * nextActivity = task->findActivity( domAct->getName() );
	    if ( !nextActivity ) {
		LQIO::input_error( LQIO::ERR_NOT_DEFINED, domAct->getName().c_str() );
		continue;
	    }

	    switch ( joinList->getListType() ) {
	    case LQIO::DOM::ActivityList::Type::JOIN:
		localActivityList = nextActivity->act_join_item( joinList );
		break;
	    case LQIO::DOM::ActivityList::Type::AND_JOIN:
		localActivityList = nextActivity->act_and_join_list( localActivityList, joinList );
		break;
	    case LQIO::DOM::ActivityList::Type::OR_JOIN:
		localActivityList = nextActivity->act_or_join_list( localActivityList, joinList );
		break;
	    default:
		abort();
	    }
	}

	/* Create the association for the activity list */
	__domToNative[joinList] = localActivityList;
	if (joinList->getNext() != nullptr) {
	    __actConnections[joinList] = joinList->getNext();
	}
    }

    /* Now we move onto the inputList, or the fork list */
    LQIO::DOM::ActivityList* forkList = domAct->getInputFromList();
    localActivityList = nullptr;
    if (forkList != nullptr && __domToNative.find(forkList) == __domToNative.end()) {
	const std::vector<const LQIO::DOM::Activity*>& list = forkList->getList();
	for (std::vector<const LQIO::DOM::Activity*>::const_iterator iter = list.begin(); iter != list.end(); ++iter) {
	    const LQIO::DOM::Activity* domAct = *iter;
	    Activity * nextActivity = task->findActivity( domAct->getName() );
	    if ( !nextActivity ) {
		LQIO::input_error( LQIO::ERR_NOT_DEFINED, domAct->getName().c_str() );
		continue;
	    }

	    /* Add the activity to the appropriate list based on what kind of list we have */
	    switch ( forkList->getListType() ) {
	    case LQIO::DOM::ActivityList::Type::FORK:
		localActivityList = nextActivity->act_fork_item( forkList );
		break;
	    case LQIO::DOM::ActivityList::Type::AND_FORK:
		localActivityList = nextActivity->act_and_fork_list(localActivityList, forkList );
		break;
	    case LQIO::DOM::ActivityList::Type::OR_FORK:
		localActivityList = nextActivity->act_or_fork_list( localActivityList, forkList );
		break;
	    case LQIO::DOM::ActivityList::Type::REPEAT:
		localActivityList = nextActivity->act_loop_list( localActivityList, forkList );
		break;
	    default:
		abort();
	    }
	}

	/* Create the association for the activity list */
	__domToNative[forkList] = localActivityList;
	if (forkList->getNext() != nullptr) {
	    __actConnections[forkList] = forkList->getNext();
	}
    }
    return *this;

}
