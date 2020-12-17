/* -*- c++ -*-
 * demand.h	-- Greg Franks
 *
 * ------------------------------------------------------------------------
 * $Id: demand.h 14134 2020-11-25 18:12:05Z greg $
 * ------------------------------------------------------------------------
 */

#ifndef LQN2PS_DEMAND_H
#define LQN2PS_DEMAND_H

#include <map>

class Demand;
class Task;
class Entity;


class Demand {

public:
    typedef std::map<const Task *,Demand> item_t;
    typedef std::map<const Entity *,Demand::item_t> map_t;

    struct select{
	select( const Task* task ) : _task(task) {}
	Demand operator()( const Demand& augend, const std::pair<const Entity*,item_t>& ) const;
    private:
	const Task* _task;
    };

private:
    struct select_2 {
	select_2( const Task* task ) : _task(task) {}
	Demand operator()( const Demand& augend, const std::pair<const Task *,Demand>& ) const;
    private:
	const Task* _task;
    };
	
public:
    Demand() : _v(0.), _d(0.) {}
    Demand( double v, double d ) : _v(v), _d(d) {}
    Demand operator+( const Demand& augend ) const { return Demand( _v + augend._v, _d + augend._d ); }
    Demand& operator+=( const Demand& addend ) { _v += addend._v; _d += addend._d; return *this; }
    Demand& accumulate( double v, double d ) { _v += v; _d += d; return *this; }
    Demand& accumulate( const Demand& addend ) { _v += addend._v; _d += addend._d; return *this; }
    double visits() const { return _v; }
    double service() const { return _v > 0. ? _d / _v : 0.; }

    static double sum_of_visits( double augend, const std::pair<const Task *,Demand>& p ) { return augend + p.second.visits(); }
private:
    double _v;
    double _d;
};
#endif
