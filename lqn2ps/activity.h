/* -*- c++ -*-  activity.h	-- Greg Franks
 *
 * $Id$
 */

#ifndef _ACTIVITY_H
#define _ACTIVITY_H
#include "lqn2ps.h"
#include "element.h"
#include "phase.h"
#include "vector.h"

class Activity;
class ActivityList;
class ActivityManip;
class AndForkActivityList;
class Arc;
class Reply;
class EntryActivityCall;
class ProxyActivityCall;
template <class Type> class Stack;

/*
 * Printing functions.
 */

ostream& operator<<( ostream&, const Activity& );

/* -------------------- Nodes in the graph are... --------------------- */

class Activity : public Element,
		 public Phase
{
public:
    friend ActivityManip connection_list( Activity * anActivity );
    friend ActivityManip reply_activity_name( Activity * anActivity );
    friend class ForkActivityList;	/* For assigning myInputFromList*/
    friend class JoinActivityList;	/* For assigning myOutputToList	*/
    friend class AndOrForkActivityList;	/* For assigning myInputFromList*/
    friend class AndOrJoinActivityList;	/* For assigning myOutputToList */
    friend class OrForkActivityList;	/* For aggregation		*/
    friend class ActivityLayer;		/* For align activities? 	*/

    static bool hasJoins;		/* Joins present in results.	*/
    static Activity* create( Task* newTask, LQIO::DOM::Activity* activity );
    static void reset();

    Activity& add_calls();
    Activity& add_reply_list();
    Activity& add_activity_lists();

public:
    Activity( const Task * aTask, const LQIO::DOM::DocumentObject * dom );
    virtual ~Activity();

    virtual void check() const;

    Activity& merge( const Activity&, const double=1.0 );

    virtual const Task * owner() const { return myTask; }
    virtual const string& name() const { return Element::name(); }
    Activity& level( const unsigned aLevel ) { myLevel = aLevel; return *this; }
    unsigned level() const { return myLevel; }
    Activity& resetLevel();
    unsigned findChildren( CallStack&, const unsigned, Stack<const Activity *>& ) const;
    unsigned findActivityChildren( Stack<const Activity *>&, Stack<const AndForkActivityList *>&, Entry *, const unsigned, unsigned, const double ) const;
    unsigned backtrack( Stack<const AndForkActivityList *>& ) const;
    double aggregate( const Entry * anEntry, const unsigned curr_p, unsigned &next_p, const double rate, Stack<const Activity *>& activityStack, const aggregateFunc aFunc ) const;
    virtual unsigned setChain( Stack<const Activity *>&, unsigned, unsigned, const Entity * aServer, const callFunc aFunc  );
    virtual Activity& setClientClosedChain( unsigned );
    virtual Activity& setClientOpenChain( unsigned );
    virtual Activity& setServerChain( unsigned );
    virtual double getIndex() const;
    virtual const LQIO::DOM::Phase * getDOM() const;

    ActivityList * inputFrom( ActivityList * );
    ActivityList * inputFrom () const { return inputFromList; }
    Activity& replyList(Cltn<const Entry *> *);
    Cltn<const Entry *> * replyList() const { return myReplyList; }
    ActivityList * outputTo ( ActivityList * );
    ActivityList * outputTo() const { return outputToList;}

    const LQIO::DOM::ExternalVariable & rendezvous ( const Entry *) const;
    Activity& rendezvous( Entry *, const LQIO::DOM::Call * );
    const LQIO::DOM::ExternalVariable & sendNoReply ( const Entry *) const;
    Activity& sendNoReply( Entry *, const LQIO::DOM::Call * );
    Call * forwardingRendezvous( Entry *, const double );

    virtual const Cltn<Call *>& callList() const { return myCalls; }
    Activity& rootEntry( const Entry * anEntry, const Arc * );
    virtual const Entry * rootEntry() const { return myRootEntry; }

    double processorUtilization() const;
    double throughput() const;

    unsigned countArcs( const callFunc = 0 ) const;

    virtual bool hasCallsFor( unsigned ) const;
    bool hasRendezvous() const;
    bool hasSendNoReply() const;
    bool hasCalls( const callFunc aFunc ) const;
    int repliesTo( const Entry * anEntry ) const;		/* Return index to entry or 0 if not found. */
    bool reachable() const { return reachableFrom != 0; }
    bool isSelectedIndirectly() const;
    Activity& isSpecified( const bool yesOrNo ) { iAmSpecified = yesOrNo; return *this; }
    bool isSpecified() const { return iAmSpecified; }
    bool isStartActivity() const { return myRootEntry != 0; }
    const Activity * reachedFrom() const { return reachableFrom; }

    virtual double serviceTimeForSRVNInput() const;
    Activity& disconnect( Activity * );
    bool transmorgrify();

    Activity const& sort() const;
    static int compare( const void *, const void * );
    static int compareCoord( const void * n1, const void *n2 );

    double aggregateReplies( const Entry * anEntry, const unsigned p, const double rate ) const;
    double aggregateService( const Entry * anEntry, const unsigned p, const double rate ) const;

#if defined(REP2FLAT)
    Activity& expandActivityCalls( const Activity& src, int replica) ;
#endif

    /* movement */

    virtual double height() const;
    virtual Activity& moveTo( const double x, const double y );
    virtual Activity& label();
    virtual Activity& scaleBy( const double, const double );
    virtual Activity& translateY( const double );
    virtual Activity& depth( const unsigned );
    double align() const;

    virtual Graphic::colour_type colour() const;

    virtual ostream& draw(ostream &) const;

protected:
    Call * findCall( const Entry * anEntry ) const;
    Call * findFwdCall( const Entry * anEntry ) const;
    Call * findOrAddCall( Entry * anEntry );
    ProxyActivityCall * findOrAddFwdCall(Entry * anEntry );

private:
    Activity( const Activity& anActivity );
    Activity& operator=( const Activity& );

    void addSrcCall( Call * aCall ) { myCalls << aCall; }
    Activity& appendReplyList( const Activity& src );

    ActivityList * act_join_item( LQIO::DOM::ActivityList * dom_activitylist );
    ActivityList * act_and_join_list( ActivityList * activityList, LQIO::DOM::ActivityList * dom_activitylist );
    ActivityList * act_or_join_list( ActivityList * activityList, LQIO::DOM::ActivityList * dom_activitylist );
    ActivityList * act_fork_item( LQIO::DOM::ActivityList * dom_activitylist );
    ActivityList * act_and_fork_list( ActivityList * activityList, LQIO::DOM::ActivityList * dom_activitylist );
    ActivityList * act_or_fork_list( ActivityList * activityList, LQIO::DOM::ActivityList * dom_activitylist );
    ActivityList * act_loop_list( ActivityList * activity_list, LQIO::DOM::ActivityList * dom_activitylist );

    ostream& printName( ostream& output, int& ) const;
    ostream& printNameWithReply( ostream& ) const;

    static ostream& print_reply_activity_name( ostream& output, const Activity * anActivity );

public:
    static void complete_activity_connections();

private:
    static std::map<LQIO::DOM::ActivityList*, ActivityList *> domToNative;
    static std::map<LQIO::DOM::ActivityList*, LQIO::DOM::ActivityList*> actConnections;

private:
    const Task * myTask;		/* */
    ActivityList * inputFromList;	/* Node which calls me		*/
    ActivityList * outputToList;	/* Node which I call.		*/
    Cltn<const Entry *> * myReplyList;	/* Who I generate replies to.	*/
    Cltn<Call *> myCalls;		/* Who I call.			*/
    Cltn<Reply *> myReplyArcList;	/* arcs for replies.		*/
    const Entry * myRootEntry;		/* Set if root activity.	*/
    const Arc * myCaller;		/* from the entry		*/
    bool iAmSpecified;			/* Set if defined		*/
    unsigned myLevel;			/* For topological sorting	*/
    unsigned myIndex;			/* For sorting			*/
    mutable const Activity * reachableFrom;	/* Set if activity is reachable	*/
};

class activity_cycle : public path_error 
{
public:
    activity_cycle( const Activity *, Stack<const Activity *>& );
};

/* -------------------------------------------------------------------- */
/* Funky Formatting functions for inline with <<.			*/
/* -------------------------------------------------------------------- */

class ActivityManip {
public:
    ActivityManip( ostream& (*ff)(ostream&, const Activity * ), const Activity * theActivity )
	: f(ff), anActivity(theActivity) {}
private:
    ostream& (*f)( ostream&, const Activity* );
    const Activity * anActivity;

    friend ostream& operator<<(ostream & os, const ActivityManip& m ) 
	{ return m.f(os,m.anActivity); }
};


ActivityManip connection_list( Activity * anActivity );
ActivityManip reply_activity_name( Activity * anActivity );
#endif
