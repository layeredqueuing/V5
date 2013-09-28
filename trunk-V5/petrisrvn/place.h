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
    Place( LQIO::DOM::Entity * dom ) : _dom(dom), _x_pos(0.0), _y_pos(0.0) {}
    virtual ~Place() {}

    unsigned id;

    virtual const char * name() const;
    LQIO::DOM::Entity * get_dom() const { return _dom; }

    virtual scheduling_type scheduling() const;
    bool scheduling_is_ok( const unsigned bits ) const;

    virtual unsigned int multiplicity() const;
    bool is_infinite() const;
    bool has_random_queueing() const;

    Place& set_origin( double x_pos, double y_pos ) { _x_pos = x_pos, _y_pos = y_pos; return *this; }
    double get_x_pos() const { return _x_pos; }
    double get_y_pos() const { return _y_pos; }

public:
#if (__GNUC__ < 4 || (__GNUC__ == 4 &&__GNUC_MINOR__ == 0))
    static const double	SERVER_Y_OFFSET;
    static const double CLIENT_Y_OFFSET;
    static const double PLACE_X_OFFSET;
    static const double PLACE_Y_OFFSET;
#else
    static const double	SERVER_Y_OFFSET	= 3.0;
    static const double CLIENT_Y_OFFSET = 0.5;
    static const double PLACE_X_OFFSET	= -0.25;
    static const double PLACE_Y_OFFSET	= 0.35;
#endif

private:
    LQIO::DOM::Entity* _dom;			/* The DOM Representation	*/
    double _x_pos;
    double _y_pos;
};
#endif
