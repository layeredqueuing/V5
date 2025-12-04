/************************************************************************/
/* Copyright the Real-Time and Distributed Systems Group,		*/
/* Department of Systems and Computer Engineering,			*/
/* Carleton University, Ottawa, Ontario, Canada. K1S 5B6		*/
/* 									*/
/* November 2025							*/
/************************************************************************/

/*
 * Test program.
 *
 * $Id: eventhandler.cc 17600 2025-11-25 20:26:11Z greg $
 */

#include <chrono>
#include <iostream>
#include <thread>
#include <iostream>
#include "eventhandler.h"

class Task {
public:
    Task( const std::string& name, EventHandler& scheduler ) : _name(name), _scheduler(scheduler) {}
    
    void operator()( double time ) {
	while ( _scheduler.get_time() < 10 ) {
	    std::condition_variable resume;
	    std::cout << _scheduler.get_time() << ": "<< _name << " start." << std::endl;
	    _scheduler.start_event( resume );
	    std::cout << _scheduler.get_time() << ": "<< _name << " acquire." << std::endl;
	    EventHandler::processor_acquire();
	    std::cout << _scheduler.get_time() << ": "<< _name << " stop." << std::endl;
	    _scheduler.stop_event( time );
	    std::cout << _scheduler.get_time() << ": "<< _name << " release." << std::endl;
	    EventHandler::processor_release();
	}
    }

private:
    std::string _name;
    EventHandler& _scheduler;
};

int main( int argc, char **argv )
{
    EventHandler scheduler;
	
    Task task_2( "Task 2", scheduler );
    Task task_3( "Task 3", scheduler );
    Task task_5( "Task 1", scheduler );
    std::thread task_2_thread( task_2, 2 );
    std::thread task_3_thread( task_3, 3 );
    std::thread task_5_thread( task_5, 5 );
    std::thread scheduler_thread( &EventHandler::run, &scheduler );
    scheduler_thread.join();
    task_2_thread.join();
    task_3_thread.join();
    task_5_thread.join();
}
