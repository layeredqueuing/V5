/*  -*- c++ -*-
 *
 * ------------------------------------------------------------------------
 * $Id: target.h 17501 2024-11-27 21:32:50Z greg $
 * ------------------------------------------------------------------------
 */

#ifndef _TARGET_H
#define _TARGET_H

#include <deque>
#include <vector>
#include <assert.h>
#include <lqio/dom_call.h>
#include <lqio/dom_phase.h>
#include "result.h"

class Activity;
class Entry;
class Message;

class Call {				/* send target struct		*/
    friend class Targets;

public:
    Call( Entry * entry, LQIO::DOM::Call * dom );
    Call( Entry * entry, double calls );

    Entry * entry() const { return _entry; }
    double calls() const { return _calls; }
    bool reply() const { return _reply; }
    int link() const { return _link; }
    Call& set_link( int link ) { assert( link < MAX_NODES ); _link = link; return *this; }

    void initialize();
    bool dropped_messages() const;
    double mean_delay() const;		/* Result values 		*/
    double variance_delay() const;
    double compute_minimum_service_time( std::deque<Entry *>& ) const;

    void configure();

    void send_synchronous ( const Entry *, const int priority,  const long reply_port );
    void send_asynchronous( const Entry *, const int priority );
    FILE * print( FILE * ) const;
    Call& insertDOMResults();
    std::ostream& print( std::ostream& output ) const;

public:
    SampleResult r_delay;		/* Delay to send to target.	*/
    SampleResult r_delay_sqr;		/* Delay to send to target.	*/
    SampleResult r_loss_prob;		/* Loss probability.		*/

private:
    Entry * _entry;			/* target entry 		*/
    int _link;				/* Link to send data on.	*/
    double _tprob;			/* test probability		*/
    double _calls;			/* # of calls.			*/
    bool _reply;			/* Generate reply.		*/
    LQIO::DOM::Call* _call;		/* ...instead of dynamic_cast	*/
};

class Targets {				/* send table struct		*/
public:
    typedef std::vector<Call>::const_iterator const_iterator;

    Targets() : _type(LQIO::DOM::Phase::STOCHASTIC), _target() {}
    ~Targets() {}

    size_t size() const { return _target.size(); }
    Targets::const_iterator begin() const { return _target.begin(); }
    Targets::const_iterator end() const { return _target.end(); }

    void store_target_info( Entry * to_entry, LQIO::DOM::Call* a_call );
    void store_target_info( Entry * to_entry, double );
    double configure( LQIO::DOM::Phase::Type, bool=true );
    void initialize();
    Call * get_next_target( std::pair<size_t,size_t>& ) const;
    std::ostream& print( std::ostream& ) const;

    Targets& reset_stats();
    Targets& accumulate_data();
    Targets& insertDOMResults();

private:
    LQIO::DOM::Phase::Type _type;	/* 				*/
    std::vector<Call> _target;		/* target array			*/
};
#endif


