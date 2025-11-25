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
 * $Id: eventhandler.cc 17600 2025-11-25 20:26:11Z greg $
 */

#include <cassert>
#include "eventhandler.h"

std::atomic<unsigned int> EventHandler::__in_use{0};
EventHandler * EventHandler::__event_handler(nullptr);

EventHandler::EventHandler() : _event_list(), _current_time(0.0), _update_time(false)
{
    assert( __event_handler == nullptr );
    __event_handler = this;
}


/*
 * Barrier synchronization to ensure that all processors have run to
 * completion so that the event list is temporily ordered.
 */

void
EventHandler::processor_release()
{
    if ( __in_use.load() > 0 ) {
	__in_use -= 1;
    }
    if ( __in_use.load() == 0 ) {
	__event_handler->notify();
    }
}

void
EventHandler::run()
{
    for ( ;; ) {
	/* Do all events at current time */
	_update_time = false;
	while ( _event_list.top().getTime() == _current_time ) {
	    _event_list.pop();
	}

	/* wait for in_use == 0 */
	_update_time.wait(false);
	_current_time = _event_list.top().getTime();
    }
}


void
EventHandler::add_event( const Event& event )
{
    _event_list.emplace( event );
}
