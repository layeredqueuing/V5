/* pragma.h	-- Greg Franks
 *
 * $HeadURL$
 * ------------------------------------------------------------------------
 * $Id: pragma.h 13200 2018-03-05 22:48:55Z greg $
 * ------------------------------------------------------------------------
 */

#ifndef _PRAGMA_H
#define _PRAGMA_H

#include <string>
#include <cstring>
#include <map>
#include <stdexcept>
#include <lqio/dom_document.h>
#include <lqio/input.h>

struct lt_str
{
    bool operator()(const char* s1, const char* s2) const { return strcmp(s1, s2) < 0; }
};


class Pragma
{
    friend int main( int, char ** );
	
private:
    typedef enum { 
	PROCESSOR_SCHEDULING,
	RESCHEDULE_ON_ASYNC_SEND,
	STOP_ON_MESSAGE_LOSS,
	TASK_SCHEDULING
    } PRAGMA_PARAM;

public:
    typedef bool (Pragma::*set_pragma_fptr)( const std::string& );
    typedef const char * (Pragma::*get_pragma_fptr)() const;
    typedef bool (Pragma::*eq_pragma_fptr)( const Pragma& ) const;

    struct pragma_info {
	pragma_info() : _p(TASK_SCHEDULING), _set(0), _get(0), _eq(0) {}
	pragma_info( PRAGMA_PARAM p, set_pragma_fptr f, get_pragma_fptr g, eq_pragma_fptr e ) : _p(p), _set(f), _get(g), _eq(e) {}

	PRAGMA_PARAM _p;
	set_pragma_fptr _set;
	get_pragma_fptr _get;
	eq_pragma_fptr _eq;
    };
    
    Pragma();
    Pragma& operator=( const Pragma& );
    bool operator()( const std::string&, const std::string& );
    bool operator()( const char * );

    scheduling_type processor_scheduling() const { return _processor_scheduling; }
    bool reschedule_on_async_send() const { return _reschedule_on_async_send; }
    bool stop_on_message_loss() const { return _stop_on_message_loss; }
    scheduling_type task_scheduling() const { return _task_scheduling; }

    bool eq_processor_scheduling( const Pragma& p ) const { return _processor_scheduling == p._processor_scheduling; }
    bool eq_reschedule_on_async_send( const Pragma& p ) const { return _reschedule_on_async_send == p._reschedule_on_async_send; }
    bool eq_stop_on_message_loss( const Pragma& p ) const { return _stop_on_message_loss == p._stop_on_message_loss; }
    bool eq_task_scheduling( const Pragma& p ) const { return _task_scheduling == p._task_scheduling; }

    void updateDOM( LQIO::DOM::Document* document ) const;

    static void initialize();
    static void usage();

private:
    Pragma( const Pragma& );		/* No copy constructor */

    bool set_processor_scheduling( const std::string& );
    bool set_reschedule_on_async_send( const std::string& );
    bool set_stop_on_message_loss( const std::string& );
    bool set_task_scheduling( const std::string& );

    const char * get_processor_scheduling() const;
    const char * get_reschedule_on_async_send() const;
    const char * get_stop_on_message_loss() const;
    const char * get_task_scheduling() const;

    scheduling_type str_to_scheduling_type( const std::string&, scheduling_type default_sched=SCHEDULE_FIFO); 
    static bool is_true( const std::string& );

private:
    scheduling_type _processor_scheduling;
    bool _reschedule_on_async_send;
    bool _stop_on_message_loss;
    scheduling_type _task_scheduling;

private:
    static std::map<const char *, pragma_info, lt_str> __pragmas;
};

extern Pragma pragma;
#endif
