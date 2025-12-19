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
#include <lqio/dom_document.h>
#include <lqio/dom_processor.h>
#include "eventhandler.h"
#include "processor.h"
#include "model.h"

Model * Model::__model = nullptr;

class Task {
public:
    Task( const std::string& name, EventHandler& scheduler, Processor& processor )
	: _name(name), _scheduler(scheduler), _processor(processor) {}
    
    void operator()( double time ) {
	while ( _scheduler.get_time() < 30 ) {
	    std::condition_variable resume;
	    std::cout << _scheduler.get_time() << ": "<< _name << " start." << std::endl;
	    _processor.acquire( resume );
	    _scheduler.start_event( resume );
	    _processor.release( resume );
	    _scheduler.stop_event( resume, time );
	    std::cout << _scheduler.get_time() << ": "<< _name << " stop." << std::endl;
	}
    }

private:
    std::string _name;
    EventHandler& _scheduler;
    Processor& _processor;
};

int main( int argc, char **argv )
{
    LQIO::DOM::Document document( LQIO::DOM::Document::InputFormat::LQN );
    LQIO::DOM::Processor dom( &document, "processor", SCHEDULE_FIFO );
    document.addProcessorEntity( &dom );
    EventHandler scheduler;
    Processor processor( &dom );
	
    Task task_2( "Task 2", scheduler, processor );
    Task task_3( "Task 3", scheduler, processor );
    Task task_5( "Task 5", scheduler, processor );
    std::thread task_2_thread( task_2, 2 );
    std::thread task_3_thread( task_3, 3 );
    std::thread task_5_thread( task_5, 5 );
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    std::thread scheduler_thread( &EventHandler::run, &scheduler );
    scheduler_thread.join();
    task_2_thread.join();
    task_3_thread.join();
    task_5_thread.join();
}
