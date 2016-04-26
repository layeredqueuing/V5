/* -*- c++ -*-
 * actlist.h	-- Greg Franks
 *
 * $Id: actlist.h 11963 2014-04-10 14:36:42Z greg $
 */

#ifndef _ACTLIST_H
#define _ACTLIST_H

#include "lqn2ps.h"
#include <lqio/input.h>
#include "cltn.h"
#include "vector.h"
#include "arc.h"

class Activity;
class ActivityList;
class ForkJoinActivityList;
class AndForkActivityList;
class AndJoinActivityList;
class Task;
class Entry;
class Node;
class Entity;
class Arc;
class CallStack;
template <class Type> class Stack;

typedef ostream& (Activity::*printFunc)( ostream& ) const;

ostream& operator<<( ostream&, const ActivityList& );

class bad_internal_join : public path_error 
{
public:
    bad_internal_join( const Stack<const Activity *>& );
};

/* -------------------------------------------------------------------- */
/*                             ActivityList                             */
/* -------------------------------------------------------------------- */

class ActivityList
{
    friend class Activity;		/* For next() and prev() */
    friend class OrForkActivityList;
    friend class ForkActivityList;
    friend class JoinActivityList;
    friend class AndOrJoinActivityList;
    friend class AndOrForkActivityList;
	
public:
    static void act_connect(ActivityList *, ActivityList *);

private:
    ActivityList( const ActivityList& );		/* Copying is verbotten */
    ActivityList& operator=( const ActivityList& );

public:
    typedef enum { SEQUENCE, REPEAT, AND_FORK, AND_JOIN, AND_SYNCH, OR_FORK, OR_JOIN } activity_type;

    ActivityList( const Task * owner, const LQIO::DOM::ActivityList * dom_activitylist );
    virtual ~ActivityList();

    virtual ActivityList* clone() const = 0;   
    /* Instance Variable Access */
	
    virtual activity_type myType() const = 0;
    virtual ActivityList& add( Activity * anActivity ) = 0;
    virtual unsigned size() const = 0;

    const Task * owner() const { return myOwner; }
    virtual ActivityList& resetLevel() { return *this; }
    const LQIO::DOM::ActivityList * getDOM() const { return myDOM; }

    virtual ActivityList& label() { return *this; }

    virtual unsigned findChildren( CallStack&, const unsigned, Stack<const Activity *>& ) const = 0;
    virtual unsigned findActivityChildren( Stack<const Activity *>&, Stack<const AndForkActivityList *>&, Entry *, const unsigned, const unsigned, const double ) const = 0;
    virtual unsigned backtrack( Stack<const AndForkActivityList *>& ) const = 0;
    virtual double aggregate( const Entry *, const unsigned, unsigned&, const double, Stack<const Activity *>&, aggregateFunc ) = 0;
    virtual unsigned setChain( Stack<const Activity *>&, unsigned, unsigned, const Entity * aServer, const callFunc aFunc ) const = 0;
    virtual double getIndex() const = 0;
    virtual ActivityList& reconnect( Activity *, Activity * );
    virtual ActivityList const& sort( compare_func_ptr compare ) const { return *this; }

    virtual double height() const { return interActivitySpace; }
    Graphic::colour_type colour() const;
	
    virtual Point srcPoint() const = 0;
    virtual Point dstPoint() const = 0;

    virtual ActivityList& moveSrcTo( const Point& aPoint, Activity * = 0 ) = 0;
    virtual ActivityList& moveDstTo( const Point& aPoint, Activity * = 0 ) = 0;
    virtual ActivityList& scaleBy( const double, const double ) = 0;
    virtual ActivityList& translateY( const double ) = 0;
    virtual ActivityList& depth( const unsigned ) = 0;
    virtual double align() const { return 0.0; }

    /* Printing */

    virtual ostream& draw( ostream& ) const = 0;

    static ostream& newline( ostream& output );
    static bool first;			/* True if we haven't printed 	*/
    static const int interActivitySpace = 20;

    virtual ActivityList * next() const;		/* Link to fork list 		*/
    virtual ActivityList * prev() const;		/* Link to Join list		*/
    virtual ActivityList& next( ActivityList * );	/* Link to fork list 		*/
    virtual ActivityList& prev( ActivityList * );	/* Link to Join list		*/

protected:
    virtual Point findSrcPoint() const = 0;
    virtual Point findDstPoint() const = 0;

    virtual const char * typeStr() const { return " "; }
    virtual bool drawn() const { return false; }

private:
    const Task * myOwner;
    const LQIO::DOM::ActivityList * myDOM;
};

/* ==================================================================== */

class SequentialActivityList : public ActivityList
{
    friend class JoinActivityList;		/* for access to myActivity */
    friend class OrForkActivityList;		/* for access to myActivity */

public:

    SequentialActivityList( const Task * owner, const LQIO::DOM::ActivityList * dom_activitylist ); 
    virtual ~SequentialActivityList();
    Activity * getMyActivity() const { return myActivity; }
    virtual unsigned size() const { return myActivity ? 1 : 0; }
    virtual SequentialActivityList& add( Activity * anActivity );

    virtual Point srcPoint() const { return myArc->srcPoint(); }
    virtual Point dstPoint() const { return myArc->dstPoint(); }

    virtual SequentialActivityList& scaleBy( const double, const double );
    virtual SequentialActivityList& translateY( const double );
    virtual SequentialActivityList& depth( const unsigned );

    virtual ostream& draw( ostream& ) const;

protected:
    virtual const char * typeStr() const { return "+"; }

    virtual Point findSrcPoint() const;
    virtual Point findDstPoint() const;

protected:
    Activity * myActivity;
    Arc * myArc;
};

/* -------------------------------------------------------------------- */

class ForkActivityList : public SequentialActivityList
{
    friend class Activity;

public:
    ForkActivityList( const Task * owner, const LQIO::DOM::ActivityList * dom_activitylist );
    ForkActivityList * clone() const;
    virtual activity_type myType() const { return SEQUENCE; }

    virtual unsigned findChildren( CallStack&, const unsigned, Stack<const Activity *>& ) const;
    virtual unsigned findActivityChildren( Stack<const Activity *>&, Stack<const AndForkActivityList *>&, Entry *, const unsigned, const unsigned, const double ) const;
    virtual unsigned backtrack( Stack<const AndForkActivityList *>& ) const;
    virtual double aggregate( const Entry *, const unsigned, unsigned&, const double, Stack<const Activity *>&, aggregateFunc );
    virtual unsigned setChain( Stack<const Activity *>&, unsigned, unsigned, const Entity * aServer, const callFunc aFunc ) const;
    virtual double getIndex() const;
    virtual ForkActivityList& reconnect( Activity *, Activity * );

    virtual double height() const { return interActivitySpace / 2; }

    virtual ForkActivityList& moveSrcTo( const Point& aPoint, Activity * );
    virtual ForkActivityList& moveDstTo( const Point& aPoint, Activity * );
    virtual double align() const;

    /* Printing */

protected:
    virtual ActivityList * prev() const { return prevLink; }	/* Link to fork list 		*/
    virtual ForkActivityList& prev( ActivityList * aList) { prevLink = aList; return *this; }

private:
    ActivityList * prevLink;
};

/* -------------------------------------------------------------------- */

class JoinActivityList : public SequentialActivityList
{
public:
    JoinActivityList( const Task * owner, const LQIO::DOM::ActivityList * dom_activitylist );
    JoinActivityList * clone() const;
	
    virtual activity_type myType() const { return SEQUENCE; }

    virtual unsigned findChildren( CallStack&, const unsigned, Stack<const Activity *>& ) const;
    virtual unsigned findActivityChildren( Stack<const Activity *>&, Stack<const AndForkActivityList *>&, Entry *, const unsigned, const unsigned, const double ) const;
    virtual unsigned backtrack( Stack<const AndForkActivityList *>& ) const;
    virtual double aggregate( const Entry *, const unsigned, unsigned&, const double, Stack<const Activity *>&, aggregateFunc );
    virtual unsigned setChain( Stack<const Activity *>&, unsigned, unsigned, const Entity * aServer, const callFunc aFunc ) const;
    virtual double getIndex() const;
    virtual JoinActivityList& reconnect( Activity *, Activity * );

    virtual double height() const;

    virtual JoinActivityList& moveSrcTo( const Point& aPoint, Activity * );
    virtual JoinActivityList& moveDstTo( const Point& aPoint, Activity * );

    /* Printing */

public:
    virtual ActivityList * next() const { return nextLink; }	/* Link to Join list		*/
    virtual ActivityList& next( ActivityList * aList ) { nextLink = aList; return *this; }	/* Link to Join list		*/

private:
    ActivityList *nextLink;
};

/* ==================================================================== */

class ForkJoinActivityList : public ActivityList
{
public:
    class ForkJoinName
    {
    public:
	ForkJoinName( const ForkJoinActivityList& );
	const char * operator()();

    private:
	string aString;
    };
	
    friend class ForkJoinActivityList::ForkJoinName;

public:
    ForkJoinActivityList( const Task * owner, const LQIO::DOM::ActivityList* );
    virtual ~ForkJoinActivityList();
    
#if defined(REP2FLAT)
    const Cltn<Activity *> & getMyActivityList() const { return myActivityList; }
#endif

    virtual ForkJoinActivityList& add( Activity * anActivity );
    virtual ForkJoinActivityList const& sort( compare_func_ptr compare ) const { myActivityList.sort( compare ); return *this; }

    virtual unsigned size() const { return myActivityList.size(); }
	
    virtual Point srcPoint() const = 0;
    virtual Point dstPoint() const = 0;

    virtual ForkJoinActivityList& scaleBy( const double, const double );
    virtual ForkJoinActivityList& translateY( const double );
    virtual ForkJoinActivityList& depth( const unsigned );

    double radius() const;
    virtual ostream& draw( ostream& ) const;

protected:
    virtual Point findSrcPoint() const;
    virtual Point findDstPoint() const;

    bool testAndSetDrawn() const { bool rc = iHaveBeenDrawn; iHaveBeenDrawn = true; return rc; }
    virtual bool drawn() const { return iHaveBeenDrawn; }
    bool labelled() const { bool rc = iHaveBeenLabelled; iHaveBeenLabelled = true; return rc; }
	
protected:
    Cltn<Activity *> myActivityList;
    Cltn<Arc *> myArcList;
    Node * myNode;

private:
    mutable bool iHaveBeenDrawn;		/* True if we have been drawn	*/
    mutable bool iHaveBeenLabelled;
};


/* -------------------------------------------------------------------- */

class AndOrForkActivityList : public ForkJoinActivityList
{
    friend class ActivityLayer;		/* for prev() */

public:
    AndOrForkActivityList( const Task * owner, const LQIO::DOM::ActivityList * dom_activitylist );

    virtual unsigned findChildren( CallStack&, const unsigned, Stack<const Activity *>& ) const;
    virtual double getIndex() const;
    virtual AndOrForkActivityList& reconnect( Activity *, Activity * );

    virtual Point srcPoint() const;
    virtual Point dstPoint() const;
    virtual AndOrForkActivityList& moveSrcTo( const Point& aPoint, Activity * );
    virtual AndOrForkActivityList& moveDstTo( const Point& aPoint, Activity * );
    virtual double align() const;
    virtual ostream& draw( ostream& ) const;

protected:
    virtual ActivityList * prev() const { return prevLink; }	/* Link to join list 		*/
    virtual AndOrForkActivityList& prev( ActivityList * aList) { prevLink = aList; return *this; }

private:
    ActivityList * prevLink;
};


/* -------------------------------------------------------------------- */

class OrForkActivityList : public AndOrForkActivityList
{
public:
    OrForkActivityList( const Task * owner, const LQIO::DOM::ActivityList * dom_activitylist );
    virtual ~OrForkActivityList();
    OrForkActivityList * clone() const;

    virtual unsigned findActivityChildren( Stack<const Activity *>&, Stack<const AndForkActivityList *>&, Entry *, const unsigned, const unsigned, const double ) const;
    virtual unsigned backtrack( Stack<const AndForkActivityList *>& ) const;
    virtual double aggregate( const Entry *, const unsigned, unsigned&, const double, Stack<const Activity *>&, aggregateFunc );
    virtual unsigned setChain( Stack<const Activity *>&, unsigned, unsigned, const Entity * aServer, const callFunc aFunc ) const;
    virtual OrForkActivityList& add( Activity * anActivity );
    virtual activity_type myType() const { return OR_FORK; }

    virtual OrForkActivityList& moveSrcTo( const Point& src, Activity * anActivity );
    virtual OrForkActivityList& moveDstTo( const Point& src, Activity * anActivity );
    virtual OrForkActivityList& scaleBy( const double, const double );
    virtual OrForkActivityList& translateY( const double );
    virtual OrForkActivityList& label();

    virtual ostream& draw( ostream& ) const;

    LQIO::DOM::ExternalVariable& prBranch( const unsigned int i ) const;
    double sum() const;

protected:
    virtual const char * typeStr() const { return "+"; }

private:
    Cltn<Label *> myLabelList;
};

/* -------------------------------------------------------------------- */

class AndForkActivityList : public AndOrForkActivityList
{
    friend class AndJoinActivityList;

public:
    AndForkActivityList( const Task * owner, const LQIO::DOM::ActivityList * dom_activitylist  ) 
	: AndOrForkActivityList( owner, dom_activitylist ), myJoinList(0), myQuorumCount(0) 
	{}

    AndForkActivityList * clone() const;

    virtual AndForkActivityList& add( Activity * anActivity );
    virtual activity_type myType() const { return AND_FORK; }

    AndForkActivityList& quorumCount (int quorumCount) { myQuorumCount = quorumCount; return *this; }
    int quorumCount () const { return myQuorumCount; }

    virtual unsigned findActivityChildren( Stack<const Activity *>&, Stack<const AndForkActivityList *>&, Entry *, const unsigned, const unsigned, const double ) const;
    virtual unsigned backtrack( Stack<const AndForkActivityList *>& ) const;
    virtual double aggregate( const Entry *, const unsigned, unsigned&, const double, Stack<const Activity *>&, aggregateFunc );
    virtual unsigned setChain( Stack<const Activity *>&, unsigned, unsigned, const Entity * aServer, const callFunc aFunc ) const;

protected:
    virtual const char * typeStr() const { return "&"; }

private:
    const AndJoinActivityList * myJoinList;
    int myQuorumCount;
};


/* -------------------------------------------------------------------- */

class AndOrJoinActivityList : public ForkJoinActivityList
{
    friend class Activity;

public:
    AndOrJoinActivityList( const Task * owner, const LQIO::DOM::ActivityList * dom_activitylist );

    virtual unsigned findChildren( CallStack&, const unsigned, Stack<const Activity *>& ) const;
    virtual double getIndex() const;
    virtual unsigned backtrack( Stack<const AndForkActivityList *>& ) const;

    virtual AndOrJoinActivityList& reconnect( Activity *, Activity * );
    virtual double height() const;
	
    virtual Point srcPoint() const;
    virtual Point dstPoint() const;
    virtual AndOrJoinActivityList& moveSrcTo( const Point& aPoint, Activity * );
    virtual AndOrJoinActivityList& moveDstTo( const Point& aPoint, Activity * );

    /* Printing */

    virtual ostream& draw( ostream& ) const;
 
    virtual ActivityList * next() const { return nextLink; }	/* Link to fork list		*/
    virtual AndOrJoinActivityList& next( ActivityList * aList ) { nextLink = aList; return *this; }

private:
    ActivityList *nextLink;
};


/* -------------------------------------------------------------------- */

class OrJoinActivityList : public AndOrJoinActivityList
{
public:
    OrJoinActivityList( const Task * owner, const LQIO::DOM::ActivityList * dom_activitylist ) 
	: AndOrJoinActivityList( owner, dom_activitylist ) 
	{}
    OrJoinActivityList * clone() const;

    virtual activity_type myType() const { return OR_JOIN; }
    virtual OrJoinActivityList& add( Activity * anActivity );

    virtual unsigned findActivityChildren( Stack<const Activity *>&, Stack<const AndForkActivityList *>&, Entry *, const unsigned, const unsigned, const double ) const;
    virtual double aggregate( const Entry *, const unsigned, unsigned&, const double, Stack<const Activity *>&, aggregateFunc );
    virtual unsigned setChain( Stack<const Activity *>&, unsigned, unsigned, const Entity * aServer, const callFunc aFunc ) const;


protected:
    virtual const char * typeStr() const { return "+"; }
};

/* -------------------------------------------------------------------- */

class AndJoinActivityList : public AndOrJoinActivityList
{
    friend class AndForkActivityList;

public:
    typedef enum { JOIN_NOT_DEFINED, INTERNAL_FORK_JOIN, SYNCHRONIZATION_POINT } join_type;

    AndJoinActivityList( const Task * owner, const LQIO::DOM::ActivityList * dom_activitylist );
    AndJoinActivityList * clone() const;

    ~AndJoinActivityList();
    ActivityList& resetLevel() { myDepth = 0; return *this; }

    int quorumCount() const;
    AndJoinActivityList& quorumCount( int );

    virtual AndJoinActivityList& add( Activity * anActivity );
    virtual double aggregate( const Entry *, const unsigned, unsigned&, const double, Stack<const Activity *>&, aggregateFunc );
    virtual unsigned setChain( Stack<const Activity *>&, unsigned, unsigned, const Entity * aServer, const callFunc aFunc ) const;

    bool isSynchPoint() const { return myJoinType == SYNCHRONIZATION_POINT; }
    bool isInternalForkJoin() const { return myJoinType == INTERNAL_FORK_JOIN; }

    double joinDelay() const;
    double joinVariance() const;

    virtual unsigned findActivityChildren( Stack<const Activity *>&, Stack<const AndForkActivityList *>&, Entry *, const unsigned, const unsigned, const double ) const;

    virtual AndJoinActivityList& moveSrcTo( const Point& src, Activity * anActivity );
    virtual AndJoinActivityList& scaleBy( const double, const double );
    virtual AndJoinActivityList& translateY( const double );
    virtual AndJoinActivityList& label();

    virtual activity_type myType() const { return AND_JOIN; }

    ostream& draw( ostream& output ) const;

protected:
    virtual const char * typeStr() const;

private:
    bool addToEntryList( unsigned i, Entry * anEntry ) const;

private:
    Label * myLabel;
    string myTypeStr;
    bool joinType( const join_type );
    mutable join_type myJoinType;		/* Barrier synch point.	*/
    mutable Cltn<const AndForkActivityList *> myForkList;
    mutable Cltn<Entry *> myEntryList;

    mutable unsigned myDepth;
};

/* -------------------------------------------------------------------- */

class RepeatActivityList : public ForkActivityList
{
public:
    RepeatActivityList( const Task * owner, const LQIO::DOM::ActivityList * dom_activitylist );
    RepeatActivityList * clone() const;

    virtual ~RepeatActivityList();
    virtual RepeatActivityList& add( Activity * anActivity );
	
#if defined(REP2FLAT)
    const Cltn<Activity *> & getMyActivityList() const { return myActivityList; }
#endif

    virtual activity_type myType() const { return REPEAT; }

    virtual unsigned size() const;
    virtual unsigned findChildren( CallStack&, const unsigned, Stack<const Activity *>& ) const;
    virtual unsigned findActivityChildren( Stack<const Activity *>&, Stack<const AndForkActivityList *>&, Entry *, const unsigned, const unsigned, const double ) const;
    virtual double aggregate( const Entry *, const unsigned, unsigned&, const double, Stack<const Activity *>&, aggregateFunc );
    virtual unsigned setChain( Stack<const Activity *>&, unsigned, unsigned, const Entity * aServer, const callFunc aFunc ) const;
    virtual double getIndex() const;

    virtual double height() const { return interActivitySpace; }
    bool testAndSetDrawn() const { bool rc = iHaveBeenDrawn; iHaveBeenDrawn = true; return rc; }
    bool labelled() const { bool rc = iHaveBeenLabelled; iHaveBeenLabelled = true; return rc; }

    virtual RepeatActivityList& moveSrcTo( const Point& aPoint, Activity * );
    virtual RepeatActivityList& moveDstTo( const Point& aPoint, Activity * );
    virtual RepeatActivityList& scaleBy( const double, const double );
    virtual RepeatActivityList& translateY( const double );
    virtual RepeatActivityList& depth( const unsigned );
    virtual double align() const { return 0.0; }

    virtual RepeatActivityList& label();

    double radius() const;

    /* Printing */

    virtual ostream& draw( ostream& ) const;

protected:
    virtual const char * typeStr() const { return "*"; }

    virtual Point findSrcPoint() const;

    virtual ActivityList * prev() const { return prevLink; }	/* Link to join list 		*/
    virtual RepeatActivityList& prev( ActivityList * aList) { prevLink = aList; return *this; }

    LQIO::DOM::ExternalVariable* rateBranch( const unsigned int i ) const;

private:
    Cltn<Activity *> myActivityList;
    Cltn<Arc *> myArcList;
    Cltn<Label *> myLabelList;
    Node * myNode;
    ActivityList * prevLink;
    mutable bool iHaveBeenLabelled;
    mutable bool iHaveBeenDrawn;		/* True if we have been drawn	*/
};

/* Used by model.cc */

void add_reply_list ( Task* task, Activity* activity );
void add_activity_lists ( Task* task, Activity* activity );
#endif
