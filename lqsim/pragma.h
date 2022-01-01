/* -*- C++ -*-
 * pragma.h	-- Greg Franks
 *
 * ------------------------------------------------------------------------
 * $Id: pragma.h 15317 2022-01-01 16:44:56Z greg $
 * ------------------------------------------------------------------------
 */

#ifndef _LQSIM_PRAGMA_H
#define _LQSIM_PRAGMA_H

#if HAVE_CONFIG_H
#include <config.h>
#endif
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
    ForceInfinite force_infinite() const { return _force_infinite; }
    double initial_delay() const { return _initial_delay; }
    unsigned int initial_loops() const { return _initial_loops; }
    int nice() const { return _nice_value; }
    unsigned int number_of_blocks() const { return _number_of_blocks; }
    double precision() const { return _precision; }
    int quorum_delayed_calls() const { return _quorum_delayed_calls; }
    bool reschedule_on_async_send() const { return _reschedule_on_async_send; }
    int scheduling_model() const { return _scheduling_model; }
    double seed_value() const { return _seed_value; }
    LQIO::severity_t severity_level() { return _severity_level; }
    bool spex_comment() const { return _spex_comment; }
    bool spex_header() const { return _spex_header; }

    static void usage( std::ostream& output );

private:
    void set_abort_on_dropped_message( const std::string& );
    void set_block_period( const std::string& );
    void set_force_infinite( const std::string& );
    void set_initial_delay( const std::string& );
    void set_initial_loops( const std::string& );
    void set_max_blocks( const std::string& );
    void set_nice( const std::string& );
    void set_precision( const std::string& );
    void set_quorum_delayed_calls( const std::string& );
    void set_reschedule_on_async_send( const std::string& );
    void set_run_time( const std::string& );
    void set_scheduling_model( const std::string& );
    void set_seed_value( const std::string& );
    void set_severity_level(const std::string& );

private:
    bool _abort_on_dropped_message;
    ForceInfinite _force_infinite;
    int _nice_value;
    int _quorum_delayed_calls;
    bool _reschedule_on_async_send;
    int _scheduling_model;
    LQIO::severity_t _severity_level;
    bool _spex_comment;
    bool _spex_header;

    double _block_period;
    unsigned int _number_of_blocks;
    unsigned int _max_blocks;
    double _precision;
    double _run_time;
    double _seed_value;
    unsigned int _initial_loops;
    double _initial_delay;
    

public:
    static Pragma * __pragmas;

private:
    static const std::map<std::string,Pragma::fptr> __set_pragma;
};
#endif
