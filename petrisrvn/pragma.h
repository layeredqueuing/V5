/* pragma.h	-- Greg Franks
 *
 * $HeadURL$
 * ------------------------------------------------------------------------
 * $Id: pragma.h 13799 2020-08-27 01:12:59Z greg $
 * ------------------------------------------------------------------------
 */

#ifndef _PRAGMA_H
#define _PRAGMA_H

#include <cstring>
#include <map>
#include <lqio/input.h>

class Pragma
{
    friend int main( int, char ** );
	
private:
    Pragma();
    Pragma( const Pragma& );		/* No copy constructor */
    
public:
    typedef bool (Pragma::*fptr)( const std::string& );

    static void set( const std::map<std::string,std::string>& );
    
    scheduling_type processor_scheduling() const { return _processor_scheduling; }
    bool reschedule_on_async_send() const { return _reschedule_on_async_send; }
    bool spex_header() const { return _spex_header; }
    bool stop_on_message_loss() const { return _stop_on_message_loss; }
    scheduling_type task_scheduling() const { return _task_scheduling; }

    bool default_processor_scheduling() const { return _default_processor_scheduling; }
    bool default_task_scheduling() const { return _default_task_scheduling; }
    
    
    static void initialize();
    static void usage( std::ostream& output );

private:
    bool set_processor_scheduling( const std::string& );
    bool set_reschedule_on_async_send( const std::string& );
    bool set_spex_header( const std::string& );
    bool set_stop_on_message_loss( const std::string& );
    bool set_task_scheduling( const std::string& );

    static scheduling_type str_to_scheduling_type( const std::string& s );
    static bool is_true( const std::string& );

private:
    scheduling_type _processor_scheduling;
    bool _reschedule_on_async_send;
    bool _spex_header;
    bool _stop_on_message_loss;
    scheduling_type _task_scheduling;
    bool _default_processor_scheduling;
    bool _default_task_scheduling;

public:
    static Pragma * __pragmas;

private:
    static std::map<std::string,Pragma::fptr> __set_pragma;
};

#endif
