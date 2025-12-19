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
 * $Id: eventhandler.cc 17620 2025-12-12 19:54:15Z greg $
 */

#include <cassert>
#include <iostream>
#include <map>
#include <mutex>
#include <condition_variable>
#include "eventhandler.h"

EventHandler * EventHandler::__event_handler(nullptr);
std::condition_variable EventHandler::__start_scheduler;

void
Event::resume() const
{
    _resume->notify_one();
}

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

EventHandler::EventHandler() : _event_list(), _current_time(0.0), _mutex(), _scheduler(), _in_use(0)
{
    assert( __event_handler == nullptr );
    __event_handler = this;
}

void
EventHandler::start_event( std::condition_variable& resume )
{
    std::unique_lock<std::mutex> lock(_mutex);
    _event_list.emplace( _current_time, Event::Type::START, &resume );
    _in_use += 1;
    _scheduler.notify_one();
    resume.wait( lock );
}


void
EventHandler::stop_event( std::condition_variable& resume, double delta )
{
    std::unique_lock<std::mutex> lock(_mutex);	
    _event_list.emplace( _current_time + delta, Event::Type::STOP, &resume );
    assert( _in_use > 0 );
    _in_use -= 1;
    _scheduler.notify_one();
    resume.wait( lock );
}

void
EventHandler::run()
{
    std::cout << "Event handler running." << std::endl;
    while ( !_event_list.empty() ) {
	std::unique_lock<std::mutex> lock(_mutex);

	/* Do all events at current time */
	while ( !_event_list.empty() && _event_list.top().get_type() == Event::Type::START ) {
	    const Event& top = _event_list.top();
	    top.resume();
	    _event_list.pop();
	}

	/* wait for in_use == 0 */
	_scheduler.wait( lock, [this](){ return _in_use == 0; } );

	/* If I have a stop event, update the time.  Pop all duplicate times */
	if ( !_event_list.empty() && _event_list.top().get_type() == Event::Type::STOP ) {
	    const Event& top = _event_list.top();
	    _current_time = top.get_time();
	    top.resume();
	    _event_list.pop();
	    while ( !_event_list.empty() && _event_list.top().get_type() == Event::Type::STOP && _event_list.top().get_time() == _current_time ) {
		const Event& top = _event_list.top();
		top.resume();
		_event_list.pop();
	    }
	}

	/* wait for a start event */
	_scheduler.wait( lock, [this](){ return _in_use != 0; } );
    }
}
