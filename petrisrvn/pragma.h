/* pragma.h	-- Greg Franks
 *
 * $HeadURL$
 * ------------------------------------------------------------------------
 * $Id: pragma.h 15837 2022-08-15 23:04:45Z greg $
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
    typedef void (Pragma::*fptr)( const std::string& );

    static void set( const std::map<std::string,std::string>& );
    
    scheduling_type processor_scheduling() const { return _processor_scheduling; }
    bool reschedule_on_async_send() const { return _reschedule_on_async_send; }
    bool save_marginal_probabilities() const { return _save_marginal_probabilities; }
    LQIO::error_severity severity_level() { return _severity_level; }
    bool spex_comment() const { return _spex_comment; }
    bool spex_header() const { return _spex_header; }
    bool stop_on_message_loss() const { return _stop_on_message_loss; }
    scheduling_type task_scheduling() const { return _task_scheduling; }

    bool default_processor_scheduling() const { return _default_processor_scheduling; }
    bool default_task_scheduling() const { return _default_task_scheduling; }
    
    static void usage( std::ostream& output );

private:
    void set_force_random_queueing( const std::string& );
    void set_processor_scheduling( const std::string& );
    void set_queue_size( const std::string& );
    void set_reschedule_on_async_send( const std::string& );
    void set_save_marginal_probabilities( const std::string& );
    void set_severity_level( const std::string& );
    void set_spex_comment( const std::string& );
    void set_spex_header( const std::string& );
    void set_stop_on_message_loss( const std::string& );
    void set_task_scheduling( const std::string& );

    static scheduling_type str_to_scheduling_type( const std::string& s );

private:
    scheduling_type _processor_scheduling;
    bool _reschedule_on_async_send;
    bool _save_marginal_probabilities;
    LQIO::error_severity _severity_level;
    bool _spex_comment;
    bool _spex_header;
    bool _stop_on_message_loss;
    scheduling_type _task_scheduling;
    bool _default_processor_scheduling;
    bool _default_task_scheduling;

public:
    static Pragma * __pragmas;

private:
    static const std::map<const std::string,Pragma::fptr> __set_pragma;
};

#endif
