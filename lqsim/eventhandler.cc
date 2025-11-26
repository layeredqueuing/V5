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
 * $Id: eventhandler.cc 17602 2025-11-26 19:53:26Z greg $
 */

#include <cassert>
#include <mutex>
#include <condition_variable>
#include "eventhandler.h"

EventHandler * EventHandler::__event_handler(nullptr);

EventHandler::EventHandler() : _event_list(), _current_time(0.0), _mutex(), _update_time(), _in_use(0)
{
    assert( __event_handler == nullptr );
    __event_handler = this;
}

void
EventHandler::add_event( const Event& event )
{
    std::lock_guard<std::mutex> lock(_mutex);
    _event_list.emplace( event );
}

/*
 * Barrier synchronization to ensure that all processors have run to
 * completion so that the event list is temporily ordered.
 */

void
EventHandler::acquire()
{
    std::lock_guard<std::mutex> lock(_mutex);
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
    while ( !_event_list.empty() ) {
	/* Do all events at current time */
	std::unique_lock<std::mutex> lock(_mutex);
	while ( _event_list.top().getTime() == _current_time ) {
	    _event_list.pop();
	}

	/* wait for in_use == 0 */
	_update_time.wait( lock, [this](){ return _in_use == 0; } );
	_current_time = _event_list.top().getTime();
    }
}
