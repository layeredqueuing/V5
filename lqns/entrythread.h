/* -*- C++ -*-
 * thread.h	-- Greg Franks
 *
 * Copyright the Real-Time and Distributed Systems Group,
 * Department of Systems and Computer Engineering,
 * Carleton University, Ottawa, Ontario, Canada. K1S 5B6
 *
 * January, 2005
 *
 *
 * $Id: entrythread.h 17195 2024-05-02 17:21:13Z greg $
 *
 * ------------------------------------------------------------------------
 */

#ifndef LQNS_ENTRYTHREAD_H
#define LQNS_ENTRYTHREAD_H

#include "entry.h"
#include "randomvar.h"

class Thread;
class AndForkActivityList;

double min( const Thread& a, const Thread& b );

class Thread : public VirtualEntry, public DiscretePoints
{
public:
    Thread( const Activity * anActivity, const AndForkActivityList * fork );

private:
    Thread( const Thread& src, unsigned int replica, const AndForkActivityList * fork );

public:
    virtual Entry * clone( unsigned int replica, const AndOrForkActivityList * fork=nullptr ) const { return new Thread( *this, replica, dynamic_cast<const AndForkActivityList *>(fork) ); }
    virtual Thread& configure( const unsigned );

    /* Instance variable access */

    Thread& setSubmodelThinkTime( const double );
    Exponential startTime() const;
    Thread& startTime( const unsigned, const double );
    virtual double getStartTime() const { return _start_time.sum(); }
    virtual double getStartTimeVariance() const { return _start_time_variance; }
    Thread& joinDelay(double aJoinDelay) { _join_delay = aJoinDelay; return *this; }
    double joinDelay() { return _join_delay; }

    /* Queries */

    bool isSiblingOf( const Thread * sibling ) const { return _fork == sibling->_fork; }
    bool isAncestorOf( const Thread * ) const;
    bool isDescendentOf( const Thread * ) const;
    bool isDisjointFrom( const Thread * ) const;

    /* Computation */

    double thinkTime() const { return _think_time; }
    Thread& estimateCDF();
    virtual double waitExcept( const unsigned, const unsigned, const unsigned ) const;	/* For client service times */
#if PAN_REPLICATION
    virtual double waitExceptChain( const unsigned, const unsigned, const unsigned ) const; //REP N-R
#endif

private:
    const AndForkActivityList * _fork;	/* For searching for parents	*/
    double _think_time;
    VectorMath<double> _start_time;	/* Time this thread starts 	*/
    double _start_time_variance;
    double _join_delay;
};

#endif
