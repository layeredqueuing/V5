/* -*- c++ -*- */
/************************************************************************/
/* Copyright the Real-Time and Distributed Systems Group,		*/
/* Department of Systems and Computer Engineering,			*/
/* Carleton University, Ottawa, Ontario, Canada. K1S 5B6		*/
/* 									*/
/* May 1996.								*/
/************************************************************************/

/*
 * Lqsim-parasol entry interface.
 *
 * $Id: entry.h 17292 2024-09-16 17:28:53Z greg $
 */

#ifndef ENTRY_H
#define ENTRY_H

/* #include "task.h" */
#include <string>
#include <set>
#include <list>
#include <lqio/dom_task.h>
#include <lqio/dom_entry.h>
#include "model.h"
#include "activity.h"

class Task;

class Entry {				/* task entry struct	        */
    friend class Instance;
    
    struct Collect
    {
	typedef double (Activity::*fptr)( Entry::Collect& ) const;

	Collect( const Entry * e, fptr f ) : _e(e), rate(1.), phase(1), can_reply(true), _f(f) {};
	const Entry * _e;
	double rate;
	unsigned int phase;
	bool can_reply;
	fptr _f;
    };
	
    /*
     * Compare to entrys by their name.  Used by the set class to
     * insert items
     */

    struct ltEntry
    {
	bool operator()(const Entry * p1, const Entry * p2) const { return p1->name() < p2->name(); }
    };

    /*
     * Compare a entry name to a string.  Used by the find_if (and
     * other algorithm type things).
     */

    struct eqEntryStr 
    {
	eqEntryStr( const std::string& s ) : _s(s) {}
	bool operator()(const Entry * p1 ) const { return p1->name() == _s; }

    private:
	const std::string& _s;
    };

private:
    Entry( const Entry& ) = delete;
    Entry& operator=( const Entry& ) = delete;
    
public:
    enum class Type {
	NONE,
	RENDEZVOUS,
	SEND_NO_REPLY
    };

    static Entry * find( const char * entry_name );
    static bool find( const char * from_entry_name, Entry *&from_entry, const char * to_entry_name, Entry *&to_entry );

    static std::set<Entry *, ltEntry> __entries;	/* Entry table.	*/

    Entry( LQIO::DOM::Entry*, Task * task );
    virtual ~Entry();

    Task * task() const { return _task; }

    virtual const std::string& name() const { return _dom->getName(); }
    unsigned int index() const { return _local_id; }
    double open_arrival_rate() const { return _dom->hasOpenArrivalRate() ? _dom->getOpenArrivalRateValue() : 0; }
    int priority() const { return _dom->hasEntryPriority() ? (int)_dom->getEntryPriorityValue() : 0; }
    
    int get_port() const { return _port; }
    int entry_id() const { return _entry_id; }
    Activity * get_start_activity() const { return _activity; }
    Entry& set_start_activity( Activity * activity ) { _activity = activity; return *this; }
    Entry& set_reply();

    virtual double configure();
    Entry& initialize();
    
    void add_call( const unsigned int p, LQIO::DOM::Call* domCall );

    virtual bool is_defined() const { return getDOM()->getEntryType() != LQIO::DOM::Entry::Type::NOT_DEFINED; }
    virtual bool is_regular() const;
    virtual bool is_activity() const;
    virtual bool is_semaphore() const;
    virtual bool is_signal() const;
    virtual bool is_wait() const;
    virtual bool is_rwlock() const;
    virtual bool is_r_unlock() const;
    virtual bool is_r_lock() const;
    virtual bool is_w_unlock() const;
    virtual bool is_w_lock() const;

    bool is_send_no_reply() const { return _recv == Type::SEND_NO_REPLY; }
    bool is_rendezvous() const { return _recv == Type::RENDEZVOUS; }
    bool has_lost_messages() const;
    bool has_think_time() const;

    virtual bool test_and_set( LQIO::DOM::Entry::Type );			/* Sets _type too!		*/
    bool test_and_set_recv( Type );
    bool test_and_set_semaphore( LQIO::DOM::Entry::Semaphore );
    bool test_and_set_rwlock( LQIO::DOM::Entry::RWLock );

    Entry& set_DOM( unsigned ph, LQIO::DOM::Phase* phaseInfo );
    LQIO::DOM::Entry * getDOM() const { return _dom; }
    Entry& add_forwarding( Entry* toEntry, LQIO::DOM::Call * value );

    Entry& reset_stats();
    Entry& accumulate_data();
    virtual Entry& insertDOMResults();

    double compute_minimum_service_time( std::deque<Entry *>& );

    static Entry * add( LQIO::DOM::Entry* domEntry, Task * );
    
private:
    Entry& add_open_arrival_task();
    double throughput() const;
    double throughput_variance() const;
    double minimum_service_time() const;
    void print_debug_info();
    
public:
    std::vector<Activity> _phase;	/* phase info. Dim starts at 1 	*/
    std::vector<unsigned> _active;	/* Number of active instances.	*/
    result_t r_cycle;			/* cycle time for entry.	*/
    mutable std::vector<double> _minimum_service_time;	/* Computed. 	*/

    static Entry * entry_table[MAX_PORTS+1];

private:
    LQIO::DOM::Entry* _dom;		/* */
    const unsigned int _entry_id;	/* Global entry id.		*/
    const unsigned int _local_id;	/* Local offset (for instance)	*/
    int _port;				/* Parasol port.		*/
    Activity * _activity;		/* Activity list.		*/
    Type _recv;				/* flag...			*/
    Task * _task;			/* Owner of entry.		*/
    Targets _fwd;			/* forward info		        */
    ActivityList * _join_list;		/* For joins			*/
};

/*
 * This class is used to generate open arrival requests.
 */

class Pseudo_Entry : public Entry
{
public:
    Pseudo_Entry( LQIO::DOM::Entry *, Task * );

    virtual double configure();

    virtual const std::string& name() const { return _name; }
    virtual bool is_defined() const { return true; }
    virtual bool is_regular() const { return true; }
    virtual bool is_activity() const { return false; }
    virtual bool is_semaphore() const { return false; }
    virtual bool is_signal() const { return false; }
    virtual bool is_wait() const { return false; }
		
    virtual bool is_rwlock() const { return false; }
    virtual bool is_r_unlock() const { return false; }
    virtual bool is_r_lock() const { return false; }
    virtual bool is_w_unlock() const { return false; }
    virtual bool is_w_lock() const { return false; }

    virtual bool test_and_set( LQIO::DOM::Entry::Type ) { return true; }

    virtual Entry& insertDOMResults();

private:
    const std::string _name;
};

typedef double (*double_func_ptr)( const tar_t * );

extern unsigned open_arrival_count;	/* non-zero if any open arrivals*/

typedef double (*entry_func_ptr)( const Entry * ep );

void build_links( class Task * cp, unsigned link_tab[] );
#endif
