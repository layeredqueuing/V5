/************************************************************************/
/* Copyright the Real-Time and Distributed Systems Group,		*/
/* Department of Systems and Computer Engineering,			*/
/* Carleton University, Ottawa, Ontario, Canada. K1S 5B6		*/
/* 									*/
/* November 2025							*/
/************************************************************************/

/*
 * Event handling (the scheduler)
 *
 * $Id: eventhandler.cc 17611 2025-12-02 19:54:19Z greg $
 */

#include <cassert>
#include <iostream>
#include <map>
#include <mutex>
#include <condition_variable>
#include "eventhandler.h"

EventHandler * EventHandler::__event_handler(nullptr);

std::ostream&
Event::print( std::ostream& output ) const
{
    static std::map<Event::Type,const std::string> event_name = {
	{ Event::Type::START, "Start" },
	{ Event::Type::STOP, "Stop" }
    };

    output << get_time() << ": " <<  event_name[get_type()] << std::endl;
    return output;
}

EventHandler::EventHandler() : _event_list(), _current_time(0.0), _mutex(), _update_time(), _in_use(0)
{
    assert( __event_handler == nullptr );
    __event_handler = this;
}

void
EventHandler::start_event( std::condition_variable& resume )
{
    std::unique_lock<std::mutex> lock(_mutex);
    _event_list.emplace( _current_time, Event::Type::START, &resume );
    resume.wait( lock );
}


void
EventHandler::stop_event( double delta )
{
    std::unique_lock<std::mutex> lock(_mutex);
    _event_list.emplace( _current_time + delta, Event::Type::STOP, nullptr );
}

/*
 * Barrier synchronization to ensure that all processors have run to
 * completion so that the event list is temporily ordered.
 */

void
EventHandler::acquire()
{
    std::unique_lock<std::mutex> lock(_mutex);
    _in_use += 1;
}

void
EventHandler::release()
{
    std::lock_guard<std::mutex> lock(_mutex);
    if ( _in_use > 0 ) {
	_in_use -= 1;
    }
    if ( _in_use == 0 ) {
	_update_time.notify_one();
    }
}

void
EventHandler::run()
{
    std::cout << "Event handler running." << std::endl;
    while ( !_event_list.empty() ) {
	/* Do all events at current time */
	std::unique_lock<std::mutex> lock(_mutex);
	while ( !_event_list.empty() && _event_list.top().get_type() == Event::Type::START ) {
	    const Event& top = _event_list.top();
	    top.print( std::cout );
	    if ( top.get_resume() ) {
		// if start event, then allow task to run.
		top.get_resume()->notify_one();
	    }
	    _event_list.pop();
	}

	/* wait for in_use == 0 */
	_update_time.wait( lock, [this](){ return _in_use == 0; } );

	/* If I have a stop event, update the time.  Pop all duplicate times */
	if ( !_event_list.empty() && _event_list.top().get_type() == Event::Type::STOP ) {
	    _current_time = _event_list.top().get_time();
	    _event_list.pop();
	    while ( !_event_list.empty() && _event_list.top().get_type() == Event::Type::STOP && _event_list.top().get_time() == _current_time ) {
		_event_list.pop();
	    }
	}
    }
}
