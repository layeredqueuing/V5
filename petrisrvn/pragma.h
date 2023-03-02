/* pragma.h	-- Greg Franks
 *
 * ------------------------------------------------------------------------
 * $Id: pragma.h 16448 2023-02-27 13:04:14Z greg $
 * ------------------------------------------------------------------------
 */

#ifndef _PRAGMA_H
#define _PRAGMA_H

#include <cstring>
#include <map>
#include <lqio/dom_document.h>

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
    double spex_convergence() const { return _spex_convergence; }
    bool spex_header() const { return _spex_header; }
    unsigned int spex_iteration_limit() const { return _spex_iteration_limit; }
    double spex_underrelaxation() const { return _spex_underrelaxation; }
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
    void set_spex_convergence( const std::string& );
    void set_spex_header( const std::string& );
    void set_spex_iteration_limit(  const std::string& );
    void set_spex_underrelaxation( const std::string& );
    void set_stop_on_message_loss( const std::string& );
    void set_task_scheduling( const std::string& );

    static scheduling_type str_to_scheduling_type( const std::string& s );

private:
    scheduling_type _processor_scheduling;
    bool _reschedule_on_async_send;
    bool _save_marginal_probabilities;
    LQIO::error_severity _severity_level;
    bool _spex_comment;
    double _spex_convergence;
    bool _spex_header;
    unsigned int _spex_iteration_limit;
    double _spex_underrelaxation;
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
