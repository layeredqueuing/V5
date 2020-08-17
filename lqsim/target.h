/* target.h	-- Greg Franks
 *
 * $HeadURL$
 *
 * ------------------------------------------------------------------------
 * $Id: target.h 13761 2020-08-12 02:14:55Z greg $
 * ------------------------------------------------------------------------
 */

#ifndef _TARGET_H
#define _TARGET_H

#include <cstdio>
#include <vector>
#include <assert.h>
#include "result.h"
#include <lqio/dom_call.h>

class Message;
class Entry;
class Activity;

using namespace std;

class tar_t {				/* send target struct		*/
    friend class Targets;
    typedef enum { undefined, call, constant } target_type;
    
public:
    tar_t();
    
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
    void insertDOMResults();

public:
    Entry * entry;			/* target entry 		*/
    result_t r_delay;			/* Delay to send to target.	*/
    result_t r_delay_sqr;		/* Delay to send to target.	*/
    result_t r_loss_prob;		/* Loss probability.		*/


private:
//    tar_t( const tar_t& );
//    tar_t& operator=( const tar_t& );		/* need for realloc */
    
    void initialize( Entry * to_entry, LQIO::DOM::Call* domCall ) { entry = to_entry; _type = call; _dom._call = domCall; }
    void initialize( Entry * to_entry, double value, bool reply=false ) { entry = to_entry; _type = constant; _calls = value; _reply = reply; }

private:
    int _link;				/* Link to send data on.	*/
    double _tprob;			/* test probability		*/
    double _calls;			/* # of calls.			*/
    bool _reply;			/* Generate reply.		*/
    target_type _type;			/* Types are different...	*/
    union {				/* ...so we use a  union	*/
	LQIO::DOM::Call* _call;		/* ...instead of dynamic_cast	*/
	LQIO::DOM::ExternalVariable* _extvar;
    } _dom;
};


class Targets {				/* send table struct		*/
public:
    Targets() {}
    ~Targets() {}

    unsigned size() const { return target.size(); }

    void store_target_info( Entry * to_entry, LQIO::DOM::Call* a_call );
    void store_target_info( Entry * to_entry, double );
    double configure( const LQIO::DOM::DocumentObject * dom, bool normalize );
    void initialize( const char * );
    tar_t * entry_to_send_to( unsigned int& i, unsigned int& j ) const;
    const Targets& print_raw_stat( FILE * ) const;

    Targets& reset_stats();
    Targets& accumulate_data();
    Targets& insertDOMResults();

    vector<tar_t> target;		/* target array			*/

private:
    Targets& operator=( const Targets& );
    Targets( const Targets& );
    
    bool alloc_target_info( Entry * to_entry ) ;

private:
    phase_type _type;			/* 				*/
};


#endif
