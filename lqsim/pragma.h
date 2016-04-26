/* pragma.h	-- Greg Franks
 *
 * $URL$
 * ------------------------------------------------------------------------
 * $Id: pragma.h 11963 2014-04-10 14:36:42Z greg $
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
    typedef enum { 
	NICE,
	PROCESSOR_SCHEDULING,
	QUORUM_REPLY,	
	RESCHEDULE_ON_SNR, 
	STOP_ON_MESSAGE_LOSS,
	XML_SCHEMA
    } PRAGMA_PARAM;

public:
    typedef bool (Pragma::*set_pragma_fptr)( const std::string& );
    typedef const char * (Pragma::*get_pragma_fptr)() const;
    typedef bool (Pragma::*eq_pragma_fptr)( const Pragma& ) const;

    struct pragma_info {
	pragma_info() : _p(NICE), _set(0), _get(0), _eq(0) {}
	pragma_info( PRAGMA_PARAM p, set_pragma_fptr f, get_pragma_fptr g, eq_pragma_fptr e ) : _p(p), _set(f), _get(g), _eq(e) {}

	PRAGMA_PARAM _p;
	set_pragma_fptr _set;
	get_pragma_fptr _get;
	eq_pragma_fptr _eq;
    };
    
    Pragma();
    Pragma& operator=( const Pragma& );
    bool operator()( const std::string&, const std::string& );
    void operator()( const char * );

    void set_abort_on_dropped_message( bool abort_on_dropped_message )  { _abort_on_dropped_message = abort_on_dropped_message; }
    bool abort_on_dropped_message() const { return _abort_on_dropped_message; }
    int quorum_delayed_calls() const { return _quorum_delayed_calls; }
    bool reschedule_on_async_send() const { return _reschedule_on_async_send; }
    int scheduling_model() const { return _scheduling_model; }

    bool eq_abort_on_dropped_message( const Pragma& p ) const { return _abort_on_dropped_message == p._abort_on_dropped_message; }
    bool eq_nice( const Pragma& p ) const { return true; }
    bool eq_quorum_delayed_calls( const Pragma& p ) const { return _quorum_delayed_calls == p._quorum_delayed_calls; }
    bool eq_reschedule_on_async_send( const Pragma& p ) const { return _reschedule_on_async_send == p._reschedule_on_async_send; }
    bool eq_scheduling_model( const Pragma& p ) const { return _scheduling_model == p._scheduling_model; }
    bool eq_xml_schema( const Pragma& p ) const { return true; }
    
    void updateDOM( LQIO::DOM::Document* document ) const;

    static void initialize();
    static void usage();

private:
    Pragma( const Pragma& );		/* No copy constructor */

    bool set_abort_on_dropped_message( const std::string& );
    bool set_nice( const std::string& );
    bool set_quorum_delayed_calls( const std::string& );
    bool set_reschedule_on_async_send( const std::string& );
    bool set_scheduling_model( const std::string& );
    bool set_xml_schema( const std::string& );

    const char * get_abort_on_dropped_message() const;
    const char * get_nice() const;
    const char * get_quorum_delayed_calls() const;
    const char * get_reschedule_on_async_send() const;
    const char * get_scheduling_model() const;
    const char * get_xml_schema() const;

    int str_to_scheduling_type( const std::string&, int default_sched );
    bool true_or_false( const std::string& ) const;

private:
    bool _abort_on_dropped_message;
    std::string _nice_value;
    int _quorum_delayed_calls;
    bool _reschedule_on_async_send;
    int _scheduling_model;

private:
    static std::map<const char *, pragma_info, lt_str> __pragmas;
};

extern Pragma pragma;
#endif
