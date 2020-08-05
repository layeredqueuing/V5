/* -*- C++ -*-
 * pragma.h	-- Greg Franks
 *
 * $URL$
 * ------------------------------------------------------------------------
 * $Id: pragma.h 13735 2020-08-05 15:54:22Z greg $
 * ------------------------------------------------------------------------
 */

#ifndef _PRAGMA_H
#define _PRAGMA_H

#if defined(HAVE_CONFIG_H)
#include <config.h>
#endif
#include <string>
#include <cstdio>
#include <cstring>
#include <map>
#include <lqio/dom_document.h>

struct lt_str
{
    bool operator()(const char* s1, const char* s2) const { return strcmp(s1, s2) < 0; }
};


class Pragma
{
private:
    Pragma();

public:
    typedef bool (Pragma::*fptr)( const std::string& );

    static void set( const std::map<std::string,std::string>& list );
    bool abort_on_dropped_message() const { return _abort_on_dropped_message; }
    bool get_spex_header() const { return _spex_header; }
    int quorum_delayed_calls() const { return _quorum_delayed_calls; }
    bool reschedule_on_async_send() const { return _reschedule_on_async_send; }
    int scheduling_model() const { return _scheduling_model; }
    LQIO::severity_t get_severity_level() { return _severity_level; }

    static void initialize();
    static void usage( std::ostream& output );

private:
    Pragma( const Pragma& );		/* No copy constructor */

    bool set_abort_on_dropped_message( const std::string& );
    bool set_block_period( const std::string& );
    bool set_initial_delay( const std::string& );
    bool set_initial_loops( const std::string& );
    bool set_max_blocks( const std::string& );
    bool set_nice( const std::string& );
    bool set_precision( const std::string& );
    bool set_quorum_delayed_calls( const std::string& );
    bool set_reschedule_on_async_send( const std::string& );
    bool set_run_time( const std::string& );
    bool set_scheduling_model( const std::string& );
    bool set_seed_value( const std::string& );
    bool set_severity_level(const std::string& );
    bool is_true( const string& s ) const;

private:
    bool _abort_on_dropped_message;
    int _nice_value;
    int _quorum_delayed_calls;
    bool _reschedule_on_async_send;
    int _scheduling_model;
    bool _spex_header;
    LQIO::severity_t _severity_level;

public:
    static Pragma * __pragmas;

private:
    static std::map<std::string,Pragma::fptr> __set_pragma;
};
#endif
