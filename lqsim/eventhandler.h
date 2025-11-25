/* -*- c++ -*-
 * Event handler.  Advances simulation time.
 *
 * $Id: eventhandler.h 17600 2025-11-25 20:26:11Z greg $
 */

/************************************************************************/
/* Copyright the Real-Time and Distributed Systems Group,		*/
/* Department of Systems and Computer Engineering,			*/
/* Carleton University, Ottawa, Ontario, Canada. K1S 5B6		*/
/* 									*/
/* Nov 2025.								*/
/************************************************************************/

#pragma once
#include <atomic>
#include <functional>
#include <queue>
#include <vector>

class Event {
public:
    enum class Type { START, STOP };

    double getTime() const { return time; }
    
private:
    double time;
    Type type;
};

bool operator>( const Event& e1, const Event& e2 ) { return e1.getTime() > e2.getTime(); }


class EventHandler {
public:
    EventHandler();

    void run();
    void add_event( const Event& event );
    static void processor_request() { __in_use += 1; }
    static void processor_release();

private:
    void notify() { _update_time = true; _update_time.notify_one(); }

private:
    static EventHandler * __event_handler;
    static std::atomic<unsigned int> __in_use;

private:
    std::priority_queue<Event,std::vector<Event>,std::greater<Event>> _event_list;
    double _current_time;
    std::atomic<bool> _update_time;
};

/* while event.list[head] == now and type == start; resume thread; pop head */
/* while event.list[head].type == stop and time < now; now = time; pop head */
