/* -*- c++ -*- */
/************************************************************************/
/* Copyright the Real-Time and Distributed Systems Group,		*/
/* Department of Systems and Computer Engineering,			*/
/* Carleton University, Ottawa, Ontario, Canada. K1S 5B6		*/
/* 									*/
/* June 2012.								*/
/************************************************************************/

#ifndef _PLACE_H
#define _PLACE_H

#include <lqio/input.h>

/*
 * $Id: petrisrvn.h 10943 2012-06-13 20:21:13Z greg $
 *
 * Solve LQN using petrinets.
 */

class Task;
struct place_object;
struct trans_object;
namespace LQIO {
    namespace DOM {
	class Entity;
    }
}

class Place {
public:
    Place( const LQIO::DOM::Entity * dom ) : _dom(dom), _scheduling(SCHEDULE_FIFO), _x_pos(0.0), _y_pos(0.0) {}
    virtual ~Place() {}

    unsigned id;

    virtual const char * name() const;
    const LQIO::DOM::Entity * get_dom() const { return _dom; }

    scheduling_type get_scheduling() const;
    bool has_priority_scheduling() const { return _scheduling == SCHEDULE_PPR || _scheduling == SCHEDULE_HOL; }

protected:
    void set_scheduling( scheduling_type scheduling ) { _scheduling = scheduling; }
    virtual bool scheduling_is_ok() const = 0;

public:
    virtual unsigned int multiplicity() const;
    virtual bool is_infinite() const;
    bool has_random_queueing() const;

    Place& set_origin( double x_pos, double y_pos ) { _x_pos = x_pos, _y_pos = y_pos; return *this; }
    double get_x_pos() const { return _x_pos; }
    double get_y_pos() const { return _y_pos; }

public:
    static const double	SERVER_Y_OFFSET;
    static const double CLIENT_Y_OFFSET;
    static const double PLACE_X_OFFSET;
    static const double PLACE_Y_OFFSET;

private:
    const LQIO::DOM::Entity* _dom;		/* The DOM Representation	*/
    scheduling_type _scheduling;
    double _x_pos;
    double _y_pos;
};
#endif
