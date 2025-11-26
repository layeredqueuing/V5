/* -*- c++ -*-
 * Event handler.  Advances simulation time.
 *
 * $Id: eventhandler.h 17604 2025-11-26 22:37:54Z greg $
 */

/************************************************************************/
/* Copyright the Real-Time and Distributed Systems Group,		*/
/* Department of Systems and Computer Engineering,			*/
/* Carleton University, Ottawa, Ontario, Canada. K1S 5B6		*/
/* 									*/
/* Nov 2025.								*/
/************************************************************************/

#pragma once
#include <functional>
#include <condition_variable>
#include <mutex>
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

inline bool operator>( const Event& e1, const Event& e2 ) { return e1.getTime() > e2.getTime(); }


class EventHandler {
public:
    EventHandler();

    void run();
    void add_event( const Event& event );
    static void processor_acquire() { __event_handler->acquire(); }
    static void processor_release() { __event_handler->release(); }

private:
    void notify();
    void acquire();
    void release();

private:
    static EventHandler * __event_handler;

private:
    std::priority_queue<Event,std::vector<Event>,std::greater<Event>> _event_list;
    double _current_time;
    std::mutex _mutex;
    std::condition_variable _update_time;
    unsigned int _in_use;
};

/* while event.list[head] == now and type == start; resume thread; pop head */
/* while event.list[head].type == stop and time < now; now = time; pop head */
