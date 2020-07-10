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
 * $Id: entrythread.h 13676 2020-07-10 15:46:20Z greg $
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

    Thread( const Activity * anActivity, AndForkActivityList * aFork ) 
	: VirtualEntry( anActivity ), DiscretePoints( 0.0, 0.0 ),
	  myThinkTime(0.0), myStartTimeVariance(0.0), myFork(aFork), myJoinDelay(0.0) {}

    virtual Thread& configure( const unsigned );
    bool check() const;

    /* Instance variable access */

    Thread& setIdleTime( const double );
    Exponential startTime() const;
    Thread& startTime( const unsigned, const double );
    virtual double getStartTime() const { return myStartTime.sum(); }
    virtual double getStartTimeVariance() const { return myStartTimeVariance; }
    Thread& joinDelay(double aJoinDelay) { myJoinDelay = aJoinDelay; return *this; }
    double joinDelay() { return myJoinDelay; }

    /* Queries */

    bool isSiblingOf( const Thread * sibling ) const { return myFork == sibling->myFork; }
    bool isAncestorOf( const Thread * ) const;
    bool isDescendentOf( const Thread * ) const;
    bool isDisjointFrom( const Thread * ) const;

    /* Computation */

    double thinkTime() const { return myThinkTime; }
    Thread& estimateCDF();
    virtual double waitExcept( const unsigned, const unsigned, const unsigned ) const;	/* For client service times */
    virtual double waitExceptChain( const unsigned, const unsigned, const unsigned ) const; //REP N-R

private:
    double myThinkTime;
    VectorMath<double> myStartTime;	/* Time this thread starts 	*/
    double myStartTimeVariance;
    const AndForkActivityList * myFork;	/* For searching for parents	*/
    double myJoinDelay;
};

#endif
