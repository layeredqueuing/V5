/* -*- c++ -*- */
/************************************************************************/
/* Copyright the Real-Time and Distributed Systems Group,		*/
/* Department of Systems and Computer Engineering,			*/
/* Carleton University, Ottawa, Ontario, Canada. K1S 5B6		*/
/* 									*/
/* June 2012.								*/
/************************************************************************/

#ifndef _ENTRY_H
#define _ENTRY_H

/*
 * $Id: petrisrvn.h 10943 2012-06-13 20:21:13Z greg $
 *
 * Solve LQN using petrinets.
 */

#include <vector>
#include "activity.h"
#include <lqio/dom_entry.h>

class Task;
class Phase;
class Activity;
struct place_object;
struct trans_object;

typedef enum {
    NOT_CALLED,
    RENDEZVOUS_REQUEST,
    SEND_NO_REPLY_REQUEST,
    FORWARD_REQUEST
} requesting_type;

#define RENDEZVOUS_REQUEST_TYPE 	(1<<RENDEZVOUS_REQUEST)
#define SEND_NO_REPLY_REQUEST_TYPE	(1<<SEND_NO_REPLY_REQUEST)
#define FORWARD_REQUEST_TYPE		(1<<FORWARD_REQUEST)

struct Forwarding {
    Forwarding(const Phase * root, unsigned int slice, unsigned int m, double y ) 
	: _root(root), _slice_no(slice), _m(m), _y(y), f_trans(0), f_place(0)
	{}

    const Phase * _root;		/* Source Phase.		*/
    const unsigned _slice_no;		/* Slice number.		*/
    const unsigned _m;			/* multiplicity.		*/
    const double _y;			/* Rate.			*/
    struct trans_object * f_trans;
    struct place_object * f_place;
};

class Entry {
private:
    Entry( const Entry& );
    Entry& operator=( const Entry& );
    friend class Phase;

public:
    Entry( LQIO::DOM::Entry * dom, Task * task );
    virtual ~Entry() {}
    static Entry * create( LQIO::DOM::Entry *, Task * );

    LQIO::DOM::Entry * get_dom() const { return _dom; }

    const char * name() const;
    const Task * task() const { return _task; }
    LQIO::DOM::Entry::EntryType type() const { return get_dom()->getEntryType(); }
    semaphore_entry_type semaphore_type() const { return get_dom()->getSemaphoreFlag(); }
    double openArrivalRate() const { return get_dom()->getOpenArrivalRateValue(); }

    requesting_type requests() const { return _requests; }
    unsigned int entry_id() const { return _entry_id; }
    unsigned int n_phases() const { return _n_phases; }
    Entry & set_n_phases( unsigned int n ) { if ( n > _n_phases ) _n_phases = n; return *this; }
    double release_prob() const { return _rel_prob; }
    Entry& set_random_queueing( bool random_queueing ) { _random_queueing = random_queueing; return *this; }
    bool random_queueing( ) const { return _random_queueing; }
    Activity * start_activity() const { return _start_activity; }
    Entry& set_start_activity( Activity * activity );
    double prob_fwd( const Entry * ) const;		/* Forwarding probabilites.	*/
    double yy( const Entry * e ) const;
    double zz( const Entry * e ) const;

    bool is_regular_entry() const { return type() == LQIO::DOM::Entry::ENTRY_STANDARD; }
    bool is_activity_entry() const { return type() == LQIO::DOM::Entry::ENTRY_ACTIVITY; }
    bool test_and_set( LQIO::DOM::Entry::EntryType );			/* Sets _type too!		*/
    bool test_and_set_recv( requesting_type );

    static Entry * find( const std::string& );
    static bool find( const std::string& from_entry_name, Entry *&from_entry, const std::string& to_entry_name, Entry *&to_entry );

    void add_call( const unsigned int p, LQIO::DOM::Call * );
    static void add_fwd_call( LQIO::DOM::Call * );

    void initialize();
    bool check();
    void remove_netobj();
    double transmorgrify( double base_x_pos, double base_y_pos, unsigned ix_e, struct place_object * d_place, 
			  unsigned m, short enabling );
    void create_forwarding_gspn();

    void insert_DOM_results();
    
    double task_utilization( unsigned p );
    double queueing_time( const Entry * entry ) const;
    bool check_open_result();


public:
    Phase phase[DIMPH+1];			/* Phases			*/
    std::vector<Forwarding *> forwards; 	/* For fowarding.		*/

    struct place_object * DX[MAX_MULT];		/* done				*/
    struct place_object * GdX[MAX_MULT];	/* Guard (for joins).		*/
#if defined(BUFFER_BY_ENTRY)
    struct place_object * ZZ;			/* For open requests.		*/
#endif
    double _throughput[MAX_MULT];		/* Results.			*/

private:
    LQIO::DOM::Entry * _dom;
    const Task * _task;				/* Owning task.			*/
    Activity * _start_activity;
    const unsigned int _entry_id;		/* Only for layer number	*/
    requesting_type _requests;
    bool _replies;				/* true if reply generated.	*/
    bool _random_queueing;			/* true if random queueing.	*/
    double _rel_prob;				/* Release prob at entry.	*/
    unsigned int _n_phases;			/* number of phases.		*/
    std::map<const Entry *,Call> _fwd;		/* Forwarding probabilites.	*/

public:
    static unsigned int __next_entry_id;
};


/*
 * Compare a processor name to a string.  Used by the find_if (and other algorithm type things.
 */

struct eqEntryStr 
{
    eqEntryStr( const std::string& s ) : _s(s) {}
    bool operator()(const Entry * e ) const { return _s == e->name(); }

private:
    const std::string& _s;
};

extern std::vector<Entry *> entry;
#endif
