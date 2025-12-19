/* -*- c++ -*-
 * Event handler.  Advances simulation time.
 *
 * $Id: eventhandler.h 17619 2025-12-11 19:27:54Z greg $
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

    Event( double time, Type type, std::condition_variable* resume ) : _time(time), _type(type), _resume(resume) {}

    double get_time() const { return _time; }
    Type get_type() const { return _type; }
    void resume() const;
    std::ostream& print( std::ostream& ) const;
    
private:
    double _time;
    Type _type;
    std::condition_variable* _resume;
};

inline bool operator>( const Event& e1, const Event& e2 ) { return e1.get_time() > e2.get_time(); }


class EventHandler {
public:
    EventHandler();

    void run();
    void start_event( std::condition_variable& resume );
    void stop_event( std::condition_variable& resume, double time );
    double get_time() const { return _current_time; }

private:

private:
    static EventHandler * __event_handler;

private:
    std::priority_queue<Event,std::vector<Event>,std::greater<Event>> _event_list;
    double _current_time;
    std::mutex _mutex;
    std::condition_variable _scheduler;
    unsigned int _in_use;

public:
    static std::condition_variable __start_scheduler;
};

/* while event.list[head] == now and type == start; resume thread; pop head */
/* while event.list[head].type == stop and time < now; now = time; pop head */
