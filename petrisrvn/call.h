/* -*- c++ -*- */
/************************************************************************/
/* Copyright the Real-Time and Distributed Systems Group,		*/
/* Department of Systems and Computer Engineering,			*/
/* Carleton University, Ottawa, Ontario, Canada. K1S 5B6		*/
/* 									*/
/* June 2012								*/
/************************************************************************/

/*
 * $Id: call.h 17069 2024-02-27 23:16:21Z greg $
 *
 * Solve LQN using petrinets.
 */

#ifndef PETRISRVN_CALL_H
#define PETRISRVN_CALL_H

#include <vector>
namespace LQIO {
    namespace DOM {
	class Call;
    }
}
class Phase;

class Call {
public:
    Call() : _dom(nullptr), _rpar_y(0), _w(0), _dp(0) {}

    bool is_rendezvous() const;
    bool is_send_no_reply() const;
    double value( const Phase *, double = 0.0 ) const;
    void set_dom( LQIO::DOM::Call * dom ) { _dom = dom; }
    LQIO::DOM::Call * get_dom() const { return _dom; }

private:
    LQIO::DOM::Call * _dom;			/* DOMs for the calls		*/

public:
    short _rpar_y;				/* Rendezvous rate (by phase).	*/
    double _w;					/* Waiting from entry to entry	*/
    double _dp;					/* Drop prob from entry to entry*/
    std::vector<double> _bin;			/* Histogram of customers	*/
};
#endif
