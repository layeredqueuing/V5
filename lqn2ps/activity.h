/* -*- c++ -*-  activity.h	-- Greg Franks
 *
 * $Id: activity.h 16980 2024-01-30 00:59:22Z greg $
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
class AndOrForkActivityList;
class AndOrJoinActivityList;
class Arc;
class CallStack;
class EntryActivityCall;
class ProxyActivityCall;
class Reply;

/* -------------------- Nodes in the graph are... --------------------- */

class Activity : public Element,
		 public Phase
{
public:
    friend class ActivityLayer;		/* For align activities? 	*/

    class cycle_error : public std::runtime_error
    {
    public:
	cycle_error( const std::deque<const Activity *>& );
	size_t depth() const { return _depth; }
    private:
	static std::string fold( const std::string& s1, const Activity * a2 );
	const size_t _depth;
    };
    

    class Ancestors {
    public:
	Ancestors( const Entry * source_entry ) : _activity_stack(), _fork_stack(), _source_entry( source_entry ), _phase( 1 ), _rate( 1.0 ), _reply_allowed( true ) {}
	Ancestors( const Ancestors& ancestors, bool reply_allowed ) : _activity_stack(ancestors._activity_stack), _fork_stack(), _source_entry( ancestors._source_entry ), _phase( ancestors._phase ), _rate( ancestors._rate ), _reply_allowed( reply_allowed ) {}

	const std::deque<const Activity *>& getActivityStack() const { return _activity_stack; }
	const std::deque<const AndOrForkActivityList *>& getForkStack() const { return _fork_stack; }

	size_t depth() const { return _activity_stack.size(); }
	const Entry * sourceEntry() const { return _source_entry; }
	void setPhase( unsigned int phase ) { _phase = phase; }
	unsigned int getPhase() const { return _phase; }
	void setRate( double rate ) { _rate = rate; }
	double getRate() const { return _rate; }
	bool canReply() const { return _reply_allowed; }
	
	bool find( const Activity * activity ) const { return std::find( _activity_stack.begin(), _activity_stack.end(), activity ) != _activity_stack.end(); }
	void push_activity( const Activity * activity ) { _activity_stack.push_back( activity ); }
	void pop_activity() { _activity_stack.pop_back(); }
	const Activity * top_activity() { return _activity_stack.empty() ? nullptr : _activity_stack.back(); }
	void push_fork( const AndOrForkActivityList * fork_list ) { _fork_stack.push_back( fork_list ); }
	void pop_fork() { _fork_stack.pop_back(); }
	const AndOrForkActivityList * top_fork() { return _fork_stack.empty() ? nullptr : _fork_stack.back(); }
	
    private:
	std::deque<const Activity *> _activity_stack;
	std::deque<const AndOrForkActivityList *> _fork_stack;
	const Entry * _source_entry;
	unsigned _phase;
	double _rate;
	bool _reply_allowed;
    };
    
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
    size_t findActivityChildren( Ancestors& ) const;
    void backtrack( const std::deque<const AndOrForkActivityList *>&, std::set<const AndOrForkActivityList *>&, std::set<const AndOrJoinActivityList *>& ) const;
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

    const LQIO::DOM::ExternalVariable& rendezvous ( const Entry *) const;
    Activity& rendezvous( Entry *, const LQIO::DOM::Call * );
    const LQIO::DOM::ExternalVariable& sendNoReply ( const Entry *) const;
    Activity& sendNoReply( Entry *, const LQIO::DOM::Call * );
    Call * forwardingRendezvous( Entry *, const double );

    virtual const std::vector<Call *>& calls() const { return _calls; }
    void addSrcCall( Call * call ) { _calls.push_back(call); }
    void removeSrcCall( Call * call );

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
    
#if REP2FLAT
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

    virtual Graphic::Colour colour() const;

    virtual const Activity& draw(std::ostream &) const;

protected:
    Call * findCall( const Entry * anEntry ) const;
    Call * findFwdCall( const Entry * anEntry ) const;
    Call * findOrAddCall( Entry * anEntry );
    ProxyActivityCall * findOrAddFwdCall(Entry * anEntry );

private:
    Activity( const Activity& anActivity );
    Activity& operator=( const Activity& );

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
#endif
