/*  -*- c++ -*-
 *
 * ------------------------------------------------------------------------
 * $Id: target.h 15317 2022-01-01 16:44:56Z greg $
 * ------------------------------------------------------------------------
 */

#ifndef _TARGET_H
#define _TARGET_H

#include <cstdio>
#include <vector>
#include <assert.h>
#include "result.h"
#include <lqio/dom_call.h>
#include <lqio/dom_phase.h>

class Message;
class Entry;
class Activity;

class tar_t {				/* send target struct		*/
    friend class Targets;
    enum class Type { undefined, call, constant };

public:
    tar_t() : _entry(nullptr), _link(-1), _tprob(0.0), _calls(0.0), _reply(false), _type(Type::undefined) {}

    Entry * entry() const { return _entry; }
    double calls() const { return _calls; }
    bool reply() const { return _reply; }
    int link() const { return _link; }
    tar_t& set_link( int link ) { assert( link < MAX_NODES ); _link = link; return *this; }

    bool dropped_messages() const;
    double mean_delay() const;		/* Result values 		*/
    double variance_delay() const;
    double compute_minimum_service_time() const;

    void configure();

    void send_synchronous ( const Entry *, const int priority,  const long reply_port );
    void send_asynchronous( const Entry *, const int priority );
    FILE * print( FILE * ) const;
    tar_t& insertDOMResults();

public:
    result_t r_delay;			/* Delay to send to target.	*/
    result_t r_delay_sqr;		/* Delay to send to target.	*/
    result_t r_loss_prob;		/* Loss probability.		*/


private:
//    tar_t( const tar_t& ) = delete;
//    tar_t& operator=( const tar_t& ) = delete;		/* need for realloc */

    void initialize( Entry * to_entry, LQIO::DOM::Call* domCall ) { _entry = to_entry; _type = Type::call; _dom._call = domCall; }
    void initialize( Entry * to_entry, double value, bool reply=false ) { _entry = to_entry; _type = Type::constant; _calls = value; _reply = reply; }

private:
    Entry * _entry;			/* target entry 		*/
    int _link;				/* Link to send data on.	*/
    double _tprob;			/* test probability		*/
    double _calls;			/* # of calls.			*/
    bool _reply;			/* Generate reply.		*/
    Type _type;				/* Types are different...	*/
    union {				/* ...so we use a  union	*/
	LQIO::DOM::Call* _call;		/* ...instead of dynamic_cast	*/
	LQIO::DOM::ExternalVariable* _extvar;
    } _dom;
};

class Targets {				/* send table struct		*/
public:
    typedef std::vector<tar_t>::const_iterator const_iterator;

    Targets() {}
    ~Targets() {}

    const tar_t& operator[]( size_t ix ) const { return _target[ix]; }
    size_t size() const { return _target.size(); }
    Targets::const_iterator begin() const { return _target.begin(); }
    Targets::const_iterator end() const { return _target.end(); }

    void store_target_info( Entry * to_entry, LQIO::DOM::Call* a_call );
    void store_target_info( Entry * to_entry, double );
    double configure( const LQIO::DOM::DocumentObject * dom, bool normalize );
    void initialize( const char * );
    tar_t * entry_to_send_to( unsigned int& i, unsigned int& j ) const;
    const Targets& print_raw_stat( FILE * ) const;

    Targets& reset_stats();
    Targets& accumulate_data();
    Targets& insertDOMResults();

private:
    std::vector<tar_t> _target;		/* target array			*/

    bool alloc_target_info( Entry * to_entry ) ;

private:
    LQIO::DOM::Phase::Type _type;			/* 				*/
};
#endif
