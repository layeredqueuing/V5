/* -*- C++ -*-
 * pragma.h	-- Greg Franks
 *
 * ------------------------------------------------------------------------
 * $Id: pragma.h 17502 2024-12-02 19:37:48Z greg $
 * ------------------------------------------------------------------------
 */

#ifndef _LQSIM_PRAGMA_H
#define _LQSIM_PRAGMA_H

#include <string>
#include <map>
#include <lqio/dom_pragma.h>

class Pragma
{
private:
    Pragma();

    Pragma( const Pragma& ) = delete;
    Pragma& operator=( const Pragma& ) = delete;

public:
    enum class ForceInfinite { NONE, FIXED_RATE, MULTISERVERS, ALL };

    typedef void (Pragma::*fptr)( const std::string& );

    static void set( const std::map<std::string,std::string>& list );

    bool abort_on_dropped_message() const { return _abort_on_dropped_message; }
    double block_period() const { return _block_period; }
    double convergence_value() const { return _convergence_value; }
    ForceInfinite force_infinite() const { return _force_infinite; }
    double initial_delay() const { return _initial_delay; }
    unsigned int initial_loops() const { return _initial_loops; }
    int nice() const { return _nice_value; }
    unsigned int number_of_blocks() const { return _number_of_blocks; }
    double precision() const { return _precision; }
    unsigned long queue_size() const { return _queue_size; }
    int quorum_delayed_calls() const { return _quorum_delayed_calls; }
    bool reschedule_on_async_send() const { return _reschedule_on_async_send; }
    int scheduling_model() const { return _scheduling_model; }
    double seed_value() const { return _seed_value; }
    LQIO::error_severity severity_level() { return _severity_level; }
    bool spex_comment() const { return _spex_comment; }
    double spex_convergence() const { return _spex_convergence; }
    bool spex_header() const { return _spex_header; }
    unsigned int spex_iteration_limit() const { return _spex_iteration_limit; }
    double spex_underrelaxation() const { return _spex_underrelaxation; }

    static void usage( std::ostream& output );

private:
    void set_abort_on_dropped_message( const std::string& );
    void set_block_period( const std::string& );
    void set_convergence_value( const std::string& );
    void set_force_infinite( const std::string& );
    void set_initial_delay( const std::string& );
    void set_initial_loops( const std::string& );
    void set_max_blocks( const std::string& );
    void set_nice( const std::string& );
    void set_precision( const std::string& );
    void set_queue_size( const std::string& );
    void set_quorum_delayed_calls( const std::string& );
    void set_reschedule_on_async_send( const std::string& );
    void set_run_time( const std::string& );
    void set_scheduling_model( const std::string& );
    void set_seed_value( const std::string& );
    void set_severity_level(const std::string& );
    void set_spex_comment( const std::string& );
    void set_spex_convergence( const std::string& );
    void set_spex_header( const std::string& );
    void set_spex_iteration_limit(  const std::string& );
    void set_spex_underrelaxation( const std::string& );

private:
    bool _abort_on_dropped_message;
    double _block_period;
    double _convergence_value;		/* SPEX */
    ForceInfinite _force_infinite;
    double _initial_delay;
    unsigned int _initial_loops;
    unsigned int _max_blocks;
    int _nice_value;
    unsigned int _number_of_blocks;
    double _precision;
    unsigned long _queue_size;
    int _quorum_delayed_calls;
    bool _reschedule_on_async_send;
    double _run_time;
    int _scheduling_model;
    double _seed_value;
    LQIO::error_severity _severity_level;
    bool _spex_comment;
    double _spex_convergence;
    bool _spex_header;
    unsigned int _spex_iteration_limit;
    double _spex_underrelaxation;

public:
    static Pragma * __pragmas;

private:
    static const std::map<const std::string,Pragma::fptr> __set_pragma;
};
#endif
