/************************************************************************/
/* Copyright the Real-Time and Distributed Systems Group,		*/
/* Department of Systems and Computer Engineering,			*/
/* Carleton University, Ottawa, Ontario, Canada. K1S 5B6		*/
/* 									*/
/* May 1996.								*/
/************************************************************************/

/*
 * Global vars for simulation.
 *
 * $Id: entry.h 13353 2018-06-25 20:27:13Z greg $
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
    
private:
    Entry( const Entry& );
    Entry& operator=( const Entry& );
    
public:
    typedef enum receive_type {
	RECEIVE_NONE,
	RECEIVE_RENDEZVOUS,
	RECEIVE_SEND_NO_REPLY
    } receive_type;

    static Entry * find( const char * entry_name );
    static bool find( const char * from_entry_name, Entry *&from_entry, const char * to_entry_name, Entry *&to_entry );

    Entry( LQIO::DOM::Entry*, Task * task );
    virtual ~Entry();

    Task * task() const { return _task; }

    virtual const char * name() const { return _dom->getName().c_str(); }
    unsigned int index() const { return _local_id; }
    double open_arrival_rate() const { return _dom->hasOpenArrivalRate() ? _dom->getOpenArrivalRateValue() : 0; }
    int priority() const { return _dom->hasEntryPriority() ? (int)_dom->getEntryPriorityValue() : 0; }
    
    Entry& set_reply();

    virtual Entry& initialize();
    virtual double configure();
    void add_call( const unsigned int p, LQIO::DOM::Call* domCall );

    virtual bool is_defined() const { return get_DOM()->getEntryType() != LQIO::DOM::Entry::ENTRY_NOT_DEFINED; }
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

    bool is_send_no_reply() const { return _recv == RECEIVE_SEND_NO_REPLY; }
    bool is_rendezvous() const { return _recv == RECEIVE_RENDEZVOUS; }
    bool has_lost_messages() const;

    virtual bool test_and_set( LQIO::DOM::Entry::EntryType );			/* Sets _type too!		*/
    bool test_and_set_recv( receive_type );
    bool test_and_set_semaphore( semaphore_entry_type );
    bool test_and_set_rwlock( rwlock_entry_type );

    Entry& set_DOM( unsigned ph, LQIO::DOM::Phase* phaseInfo );
    LQIO::DOM::Entry * get_DOM() const { return _dom; }
    Entry& add_forwarding( Entry* toEntry, LQIO::DOM::Call * value );

    Entry& reset_stats();
    Entry& accumulate_data();
    virtual Entry& insertDOMResults();

    static Entry * add( LQIO::DOM::Entry* domEntry, Task * );
    
private:
    double throughput() const;
    double throughput_variance() const;
    void print_debug_info();
    
public:
    const unsigned int entry_id;	/* Global entry id.		*/
    int port;				/* Parasol port.		*/
    Activity * _activity;		/* Activity list.		*/
    std::vector<Activity> _phase;	/* phase info. Dim starts at 1 	*/
    std::vector<unsigned> _active;	/* Number of active instances.	*/
    Targets _fwd;			/* forward info		        */
    result_t r_cycle;			/* cycle time for entry.	*/

    static Entry * entry_table[MAX_PORTS+1];

private:
    const unsigned int _local_id;	/* Local offset (for instance)	*/
    LQIO::DOM::Entry* _dom;		/* */
    receive_type _recv;			/* flag...			*/
    Task * _task;			/* Owner of entry.		*/
    ActivityList * _join_list;		/* For joins			*/
};

/*
 * This class is used to generate open arrival requests.
 */

class Pseudo_Entry : public Entry
{
public:
    Pseudo_Entry( LQIO::DOM::Entry *, Task * );

    virtual Entry& initialize() { return *this; }
    virtual double configure();

    virtual const char * name() const { return _name.c_str(); }
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

    virtual bool test_and_set( LQIO::DOM::Entry::EntryType ) { return true; }

    virtual Entry& insertDOMResults();

private:
    const std::string _name;
};

typedef double (*double_func_ptr)( const tar_t * );

extern unsigned open_arrival_count;	/* non-zero if any open arrivals*/

typedef double (*entry_func_ptr)( const Entry * ep );

void build_links( class Task * cp, unsigned link_tab[] );

/* ------------------------------------------------------------------------ */
/*
 * Compare to entrys by their name.  Used by the set class to insert items
 */

struct ltEntry
{
    bool operator()(const Entry * p1, const Entry * p2) const { return strcmp( p1->name(), p2->name() ) < 0; }
};


/*
 * Compare a entry name to a string.  Used by the find_if (and other algorithm type things).
 */

struct eqEntryStr 
{
eqEntryStr( const char * s ) : _s(s) {}
    bool operator()(const Entry * p1 ) const { return strcmp( p1->name(), _s ) == 0; }

private:
    const char * _s;
};

extern set <Entry *, ltEntry> entry;	/* Entry table.	*/
#endif
