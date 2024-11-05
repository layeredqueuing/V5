/* -*- c++ -*-
 *
 * Processor Group handling.
 *
 * $Id: group.h 17410 2024-10-31 13:54:12Z greg $
 */

/************************************************************************/
/* Copyright the Real-Time and Distributed Systems Group,		*/
/* Department of Systems and Computer Engineering,			*/
/* Carleton University, Ottawa, Ontario, Canada. K1S 5B6		*/
/* 									*/
/* Sept.2008								*/
/************************************************************************/

#ifndef	GROUP_H
#define GROUP_H

#include <lqio/dom_group.h>
#include "task.h"

class Group 
{
    /*
     * Compare to processors by their name.  Used by the set class to insert items
     */

    struct ltGroup
    {
	bool operator()(const Group * p1, const Group * p2) const { return strcmp( p1->name(), p2->name() ) < 0; }
    };

private:
    Group( const Group& ) = delete;
    Group operator=( const Group& ) = delete;
    
public:
    static std::set<Group *, ltGroup> __groups;

    static Group * find( const std::string& );
    static void add( const std::pair<std::string,LQIO::DOM::Group*>& );

public:
    Group( LQIO::DOM::Group * group, const Processor& processor );

    LQIO::DOM::Group * getDOM() const { return _dom; }
    const char * name() const { return _dom->getName().c_str(); }
    bool cap() const { return _dom->getCap(); }		/* Cap share		*/
    const Processor& processor() const { return _processor; }
    const std::set<Task*>& tasks() { return _tasks; }
    
    Group& create();

    Group& reset_stats() { r_util.reset(); return *this; }
    Group& accumulate_data() { r_util.accumulate(); return *this; }
    Group& insertDOMResults();

private:
    LQIO::DOM::Group * _dom;
    const Processor &_processor;
    const unsigned int _total_tasks;
    VariableResult r_util;			/* Utilization.			*/

    std::set<Task*> _tasks;
};
#endif
