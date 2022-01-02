/* -*- c++ -*- */
/************************************************************************/
/* Copyright the Real-Time and Distributed Systems Group,		*/
/* Department of Systems and Computer Engineering,			*/
/* Carleton University, Ottawa, Ontario, Canada. K1S 5B6		*/
/* 									*/
/* June 2012.								*/
/************************************************************************/

#ifndef _PROCESSOR_H
#define _PROCESSOR_H

/*
 * $Id: petrisrvn.h 10943 2012-06-13 20:21:13Z greg $
 *
 * Solve LQN using petrinets.
 */

#include <vector>
#include "place.h"


namespace LQIO {
    namespace DOM {
	class Processor;
    }
}
class Phase;

class Processor : public Place {
    
private:
    struct history_t {
	Task * task;				/* Task for this queue item.	*/
	struct place_object * request_place;	/* It's proc. request.		*/
	struct trans_object * grant_trans;	/* 				*/
	struct place_object * grant_place;
    };

    Processor( LQIO::DOM::Entity * );

public:
    virtual ~Processor() {}
    static void create( const std::pair<std::string,LQIO::DOM::Processor*>& );
    void clear();

    double rate() const;
    Processor& add_task( Task * task ) { _tasks.push_back( task ); return *this; }
    unsigned int n_tasks() const { return _tasks.size(); }
    unsigned int ref_count() const;

    bool is_single_place_processor() const;
    
    void remove_netobj() { PX = 0; }
    void initialize();
    void transmorgrify( unsigned max_count );

    void insert_DOM_results() const;

    static Processor * find( const std::string& name  );
    static unsigned set_queue_length();

private:
    double make_queue( double x_pos, double y_pos, const int priority,
		       struct place_object * prio_place,
		       const short trans_prio, history_t history[], 
		       const unsigned count, unsigned depth,
		       const unsigned low, const unsigned curr, const unsigned m );
    double make_fifo_queue( double x_pos, double y_pos, const int priority,
			    struct place_object * prio_place,
			    const short trans_prio, history_t history[], 
			    const unsigned count, unsigned depth,
			    const unsigned low, const unsigned curr, const unsigned m,
			    Phase * curr_phase );

    double get_waiting( const Phase& phase ) const;

public:
    struct place_object * PX;
    double proc_tokens[MAX_MULT];		/* Result. 			*/

    static double __x_offset;

private:
    std::vector<Task *> _tasks;
    history_t _history[DIME+1];
};

/*
 * Compare a processor name to a string.  Used by the find_if (and other algorithm type things.
 */

struct eqProcStr 
{
    eqProcStr( const std::string& s ) : _s(s) {}
    bool operator()(const Processor * p1 ) const { return _s == p1->name(); }

private:
    const std::string& _s;
};

extern std::vector<Processor *> processor;
#endif
