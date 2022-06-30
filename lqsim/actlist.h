/* -*- c++ -*-
 * Activity Lists (for linking the graph of activities).
 *
 * Copyright the Real-Time and Distributed Systems Group,
 * Department of Systems and Computer Engineering,
 * Carleton University, Ottawa, Ontario, Canada. K1S 5B6
 *
 * May  1996.
 * August 2009.
 * November 2020.
 *
 * ------------------------------------------------------------------------
 * $Id: actlist.h 15726 2022-06-28 17:04:56Z greg $
 * ------------------------------------------------------------------------
 */

#ifndef ACTLIST_H
#define ACTLIST_H

#include <set>
#include "result.h"

class Entry;
class Activity;
class Histogram;
class Task;
class Activity;

class InputActivityList;
class AndForkActivityList;
class OutputActivityList;
class AndJoinActivityList;


class ActivityList {
private:
    /* Used to concatentate activity list names into a string */
    struct fold {
	fold( const std::string& op ) : _op(op) {}
	std::string operator()( const std::string& s1, const Activity * a2 ) const;
    private:
	const std::string& _op;
    };

private:
    ActivityList( const ActivityList& ) = delete;
    ActivityList& operator=( const ActivityList& ) = delete;

public:
    enum class Type
    {
	FORK_LIST,
	OR_FORK_LIST,
	AND_FORK_LIST,
	LOOP_LIST,
	JOIN_LIST,
	AND_JOIN_LIST,
	OR_JOIN_LIST
    };


    typedef std::vector<Activity *>::const_iterator const_iterator;
    
    struct Collect
    {
	typedef double (Activity::*fptr)( ActivityList::Collect& ) const;

	Collect( const Entry * e, fptr f ) : _e(e), rate(1.), phase(1), can_reply(true), _f(f) {};
	const Entry * _e;
	double rate;
	unsigned int phase;
	bool can_reply;
	fptr _f;
    };
	
    ActivityList( Type type, LQIO::DOM::ActivityList* dom )
	: _type(type),
	  _dom(dom),
	  _list()
	{}
    virtual ~ActivityList() {}

    size_t size() const { return _list.size(); }
    Type get_type() const { return _type; }
    Activity * at( size_t ix ) const { return _list[ix]; }
    Activity * front() const { return _list.front(); }
    Activity * back() const { return _list.back(); }
    ActivityList::const_iterator begin() { return _list.begin(); }
    ActivityList::const_iterator end() { return _list.end(); }
    
    LQIO::DOM::ActivityList * getDOM() const { return _dom; }
    const std::string get_name() const;
    
    virtual ActivityList& configure() { return *this; }
    virtual ActivityList& push_back( Activity * activity ) { _list.push_back( activity ); return *this; }
    virtual double collect( std::deque<Activity *>& activity_stack, ActivityList::Collect& data ) const = 0;
    void shuffle();

private:
    const Type _type;
    LQIO::DOM::ActivityList* _dom;

protected:
    std::vector<Activity *> _list;		/* Array of activities.		*/
};

class InputActivityList : public ActivityList
{
public:
    InputActivityList( Type type, LQIO::DOM::ActivityList * dom )
	: ActivityList(type,dom),
	  _prev(nullptr)
	{}

    OutputActivityList * get_prev() const { return _prev; }
    InputActivityList& set_prev( OutputActivityList * list ) { _prev = list; return *this; }

    virtual double find_children( std::deque<Activity *>& activity_stack, std::deque<AndForkActivityList *>& fork_stack, const Entry * ep ) = 0;
    virtual void fork_backtrack( std::deque<AndForkActivityList *>&, std::deque<AndJoinActivityList *>&, std::set<AndForkActivityList *>& );

private:
    OutputActivityList * _prev;		/* Link to join list.		*/
};

class ForkActivityList : public InputActivityList
{
public:
    ForkActivityList( Type type, LQIO::DOM::ActivityList * dom )
	: InputActivityList(type,dom),
	  _join(nullptr)
	{}

    OutputActivityList * get_join() const { return _join; }
    ForkActivityList& set_join( OutputActivityList * list ) { _join = list; return *this; }

    virtual double find_children( std::deque<Activity *>& activity_stack, std::deque<AndForkActivityList *>& fork_stack, const Entry * ep );
    virtual double collect( std::deque<Activity *>& activity_stack, ActivityList::Collect& data ) const;
    
private:
    OutputActivityList * _join;		/* Link to fork from join.	*/
};

class OrForkActivityList : public ForkActivityList
{

public:
    OrForkActivityList( Type type, LQIO::DOM::ActivityList * dom )
	: ForkActivityList(type,dom),
	  _prob()
	{}
    
    double get_prob_at( size_t ix ) const { return _prob[ix]; }
    
    virtual OrForkActivityList& push_back( Activity * activity );
    virtual OrForkActivityList& configure();
    virtual double find_children( std::deque<Activity *>& activity_stack, std::deque<AndForkActivityList *>& fork_stack, const Entry * ep );
    virtual double collect( std::deque<Activity *>& activity_stack, ActivityList::Collect& data ) const;

private:
    std::vector<double> _prob;		/* Array of probabilities.	*/
};

class AndForkActivityList : public ForkActivityList
{
public:
    AndForkActivityList( Type type, LQIO::DOM::ActivityList * dom )
	: ForkActivityList(type,dom),
	  _visits(0)
	{}

    virtual AndForkActivityList& push_back( Activity * activity );
    AndForkActivityList& initialize();
    virtual AndForkActivityList& configure();
    unsigned int get_visits() const { return _visits; }
    
    virtual double find_children( std::deque<Activity *>& activity_stack, std::deque<AndForkActivityList *>& fork_stack, const Entry * ep );
    virtual void fork_backtrack( std::deque<AndForkActivityList *>&, std::deque<AndJoinActivityList *>&, std::set<AndForkActivityList *>& );
    virtual double collect( std::deque<Activity *>& activity_stack, ActivityList::Collect& data ) const;

private:
    unsigned _visits;			/* */
};

class LoopActivityList : public InputActivityList
{
public:
    LoopActivityList( Type type, LQIO::DOM::ActivityList * dom )
	: InputActivityList(type,dom),
	  _exit(nullptr),
	  _count(),
	  _total(0.0)
	{}

    Activity * get_exit() const { return _exit; }
    double get_total() const { return _total; }
    double get_count_at( size_t ix ) const { return _count[ix]; }
    
    virtual LoopActivityList& push_back( Activity * activity );
    LoopActivityList& end_list( Activity * activity ) { _exit = activity; return *this; }
    virtual LoopActivityList& configure();
    virtual double find_children( std::deque<Activity *>& activity_stack, std::deque<AndForkActivityList *>& fork_stack, const Entry * ep );
    virtual double collect( std::deque<Activity *>& activity_stack, ActivityList::Collect& data ) const;

private:
    Activity * _exit;			/* For repeat nodes. 		*/
    std::vector<double> _count;		/* array of iterations		*/
    double _total;			/* total iterations.		*/
};

/* ------------------------------------------------------------------------ */

class OutputActivityList : public ActivityList
{
public:
    OutputActivityList( Type type, LQIO::DOM::ActivityList * dom )
	: ActivityList(type,dom),
	  _next(nullptr)
	{}
	  
    InputActivityList * get_next() const { return _next; }
    OutputActivityList& set_next( InputActivityList * list ) { _next = list; return *this; }
    
    virtual double find_children( std::deque<Activity *>& activity_stack, std::deque<AndForkActivityList *>& fork_stack, const Entry * ep );
    virtual void join_backtrack( std::deque<AndForkActivityList *>&, std::deque<AndJoinActivityList *>&, std::set<AndForkActivityList *>& );
    virtual double collect( std::deque<Activity *>& activity_stack, ActivityList::Collect& data ) const;

private:
    InputActivityList * _next;		/* Link to fork list.		*/
};

class AndJoinActivityList : public OutputActivityList
{
private:
    class cycle_error : public std::runtime_error  {
    public:
	cycle_error( AndJoinActivityList& );
	virtual ~cycle_error() throw() {}
    private:
	static std::string fold( const std::string& s1, const Activity * a2 );
    };
    
public:
    enum class Join
    {
	UNDEFINED,
	INTERNAL_FORK_JOIN,
	SYNCHRONIZATION
    };

    AndJoinActivityList( Type type, LQIO::DOM::ActivityList * dom );
    virtual ~AndJoinActivityList();

    virtual AndJoinActivityList& configure();
    virtual AndJoinActivityList& push_back( Activity * activity );

    bool set_join_type( Join type );
    bool join_type_is( Join type ) const { return type == _join_type; }
    bool add_to_join_list( unsigned i, Activity * activity );
    unsigned int get_quorum_count() const { return _quorum_count; }

    virtual double find_children( std::deque<Activity *>& activity_stack, std::deque<AndForkActivityList *>& fork_stack, const Entry * ep );
    virtual void join_backtrack( std::deque<AndForkActivityList *>&, std::deque<AndJoinActivityList *>&, std::set<AndForkActivityList *>& );
    virtual double collect( std::deque<Activity *>& activity_stack, ActivityList::Collect& data ) const;

    AndJoinActivityList& reset_stats();
    AndJoinActivityList& accumulate_data();
    AndJoinActivityList& insertDOMResults();

private:
    const AndForkActivityList * _fork;		/* Link to join from fork.	*/
    std::vector<Activity *> _source;		/* Link to source activity 	*/
    Join _join_type;
    unsigned int _quorum_count; 		/* tomari quorum		*/

public:
    result_t r_join;				/* results for join delays	*/
    result_t r_join_sqr;			/* results for delays.		*/
    Histogram * _hist_data;
};



void print_activity_connectivity( FILE *, Activity * );

/* Used by load.cc */

void complete_activity_connections ();
#endif
