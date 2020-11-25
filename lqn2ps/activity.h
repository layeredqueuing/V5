/* -*- c++ -*-  activity.h	-- Greg Franks
 *
 * $Id: activity.h 14134 2020-11-25 18:12:05Z greg $
 */

#ifndef _ACTIVITY_H
#define _ACTIVITY_H
#include "lqn2ps.h"
#include <vector>
#include <deque>
#include "element.h"
#include "phase.h"

class Activity;
class ActivityList;
class AndForkActivityList;
class AndOrJoinActivityList;
class Arc;
class Reply;
class EntryActivityCall;
class ProxyActivityCall;
class CallStack;

/* -------------------- Nodes in the graph are... --------------------- */

class Activity : public Element,
		 public Phase
{
public:
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

    virtual bool check() const;

    Activity& merge( const Activity&, const double=1.0 );

    virtual const Task * owner() const { return _task; }
    virtual const std::string& name() const { return Element::name(); }
    Activity& level( const size_t level ) { _level = level; return *this; }
    size_t level() const { return _level; }
    Activity& resetLevel();
    size_t findChildren( CallStack&, const unsigned, std::deque<const Activity *>& ) const;
    size_t findActivityChildren( std::deque<const Activity *>&, std::deque<const AndForkActivityList *>&, Entry *, size_t, unsigned, const double ) const;
    const Activity& backtrack( const std::deque<const AndForkActivityList *>&, std::set<const AndForkActivityList *>&, std::set<const AndOrJoinActivityList *>& ) const;
    double aggregate( Entry * anEntry, const unsigned curr_p, unsigned &next_p, const double rate, std::deque<const Activity *>& activityStack, const aggregateFunc aFunc );
    virtual unsigned setChain( std::deque<const Activity *>&, unsigned, unsigned, const Entity * aServer, const callPredicate aFunc  );
    virtual Activity& setClientClosedChain( unsigned );
    virtual Activity& setClientOpenChain( unsigned );
    virtual Activity& setServerChain( unsigned );
    virtual double getIndex() const;
    virtual const LQIO::DOM::Phase * getDOM() const;

    ActivityList * inputFrom( ActivityList * );
    ActivityList * inputFrom () const { return _inputFrom; }
    Activity& replies(const std::vector<Entry *>& );
    const std::vector<Entry *>& replies() const { return _replies; }
    const std::map<Entry *,Reply *>& replyArcs() const { return _replyArcs; }
    ActivityList * outputTo ( ActivityList * );
    ActivityList * outputTo() const { return _outputTo; }

    const LQIO::DOM::ExternalVariable & rendezvous ( const Entry *) const;
    Activity& rendezvous( Entry *, const LQIO::DOM::Call * );
    const LQIO::DOM::ExternalVariable & sendNoReply ( const Entry *) const;
    Activity& sendNoReply( Entry *, const LQIO::DOM::Call * );
    Call * forwardingRendezvous( Entry *, const double );

    virtual const std::vector<Call *>& calls() const { return _calls; }
    Activity& rootEntry( const Entry * anEntry, const Arc * );
    virtual const Entry * rootEntry() const { return _rootEntry; }

    double processorUtilization() const;
    double throughput() const;

    unsigned countArcs( const callPredicate = 0 ) const;

    bool hasRendezvous() const;
    bool hasSendNoReply() const;
    bool hasCalls( const callPredicate aFunc ) const;
    bool repliesTo( const Entry * anEntry ) const;
    bool reachable() const { return _reachableFrom != 0; }
    bool isSelectedIndirectly() const;
    Activity& isSpecified( const bool yesOrNo ) { iAmSpecified = yesOrNo; return *this; }
    bool isSpecified() const { return iAmSpecified; }
    bool isStartActivity() const { return _rootEntry != NULL; }
    const Activity * reachedFrom() const { return _reachableFrom; }
    Activity& setReachedFrom( const Activity * reachedFrom ) { _reachableFrom = reachedFrom; return *this; }

    virtual double serviceTimeForSRVNInput() const;
    Activity& disconnect( Activity * );

    Activity& sort();
    static bool compareCoord( const Activity *, const Activity * );

    double aggregateReplies( Entry * anEntry, const unsigned p, const double rate );
    double aggregateService( Entry * anEntry, const unsigned p, const double rate );
    void transmorgrify( std::deque<const Activity *>& activityStack, const double rate );

    virtual Activity& rename();
    
#if defined(REP2FLAT)
    Activity& expandActivityCalls( const Activity& src, int replica);
    virtual Activity& replicateActivity( LQIO::DOM::Activity * root, unsigned int replica );
    virtual Activity& replicateCall();
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

    virtual const Activity& draw(std::ostream &) const;

protected:
    Call * findCall( const Entry * anEntry ) const;
    Call * findFwdCall( const Entry * anEntry ) const;
    Call * findOrAddCall( Entry * anEntry );
    ProxyActivityCall * findOrAddFwdCall(Entry * anEntry );

private:
    Activity( const Activity& anActivity );
    Activity& operator=( const Activity& );

    void addSrcCall( Call * aCall ) { _calls.push_back(aCall); }
    Activity& appendReplyList( const Activity& src );

    ActivityList * act_join_item( LQIO::DOM::ActivityList * dom_activitylist );
    ActivityList * act_and_join_list( ActivityList * activityList, LQIO::DOM::ActivityList * dom_activitylist );
    ActivityList * act_or_join_list( ActivityList * activityList, LQIO::DOM::ActivityList * dom_activitylist );
    ActivityList * act_fork_item( LQIO::DOM::ActivityList * dom_activitylist );
    ActivityList * act_and_fork_list( ActivityList * activityList, LQIO::DOM::ActivityList * dom_activitylist );
    ActivityList * act_or_fork_list( ActivityList * activityList, LQIO::DOM::ActivityList * dom_activitylist );
    ActivityList * act_loop_list( ActivityList * activity_list, LQIO::DOM::ActivityList * dom_activitylist );

    std::ostream& printName( std::ostream& output, int& ) const;
    std::ostream& printNameWithReply( std::ostream& ) const;

public:
    static void complete_activity_connections();

private:
    static std::map<LQIO::DOM::ActivityList*, ActivityList *> domToNative;
    static std::map<LQIO::DOM::ActivityList*, LQIO::DOM::ActivityList*> actConnections;

private:
    const Task * _task;			/* Container for activity	*/
    ActivityList * _inputFrom;		/* Node which calls me		*/
    ActivityList * _outputTo;		/* Node which I call.		*/
    std::vector<Entry *> _replies;	/* Who I generate replies to.	*/
    std::vector<Call *> _calls;		/* Who I call.			*/
    std::map<Entry *,Reply *> _replyArcs;	/* arcs for replies.	*/
    const Entry * _rootEntry;		/* Set if root activity.	*/
    const Arc * _caller;		/* from the entry		*/
    bool iAmSpecified;			/* Set if defined		*/
    size_t _level;			/* For topological sorting	*/
    mutable const Activity * _reachableFrom;	/* Set if activity is reachable	*/
};

inline std::ostream& operator<<( std::ostream& output, const Activity& self ) { self.draw( output ); return output; }

class activity_cycle : public path_error 
{
public:
    activity_cycle( const Activity *, std::deque<const Activity *>& );
};
#endif
