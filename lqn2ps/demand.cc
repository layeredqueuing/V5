/* -*- c++ -*-
 * $Id: entity.cc 14228 2020-12-16 15:50:10Z greg $
 *
 * Data structure for collecting demands at stations for queueing models.
 *
 * Copyright the Real-Time and Distributed Systems Group,
 * Department of Systems and Computer Engineering,
 * Carleton University, Ottawa, Ontario, Canada. K1S 5B6
 *
 * December 2020
 */

#include <numeric>
#include "demand.h"
#include "task.h"


Demand
Demand::select::operator()( const Demand& augend, const std::pair<const Entity*,Demand::item_t>& demands ) const
{
    return std::accumulate( demands.second.begin(), demands.second.end(), augend, select_2( _task ) );
}


Demand
Demand::select_2::operator()( const Demand& augend, const std::pair<const Task *,Demand>& demand ) const
{
    if ( demand.first == _task ) return augend + demand.second;
    else return augend;
}
