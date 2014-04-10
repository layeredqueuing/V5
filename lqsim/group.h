/* -*- c++ -*-
 * $HeadURL$
 * Global vars for simulation.
 *
 * $Id$
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
#include "model.h"
#include "entry.h"
#include "task.h"

class Group 
{
public:
    static Group * find( const char * );
    static void add( LQIO::DOM::Group* domGroup );

public:
    Group( LQIO::DOM::Group * group, const Processor& processor );

    LQIO::DOM::Group * getDOMGroup() const { return _domGroup; }
    const char * name() const { return _domGroup->getName().c_str(); }
    bool cap() const { return _domGroup->getCap(); }		/* Cap share		*/
    const Processor& processor() const { return _processor; }
    bool create();

    void insertDOMResults();

public:
    map<Task *,int> _tasks;		/* Maps task to group 		*/

    result_t r_util;			/* Utilization.			*/

private:
    LQIO::DOM::Group * _domGroup;
    const Processor &_processor;
    const unsigned int _total_tasks;

    set<Task*> _task_list;
};

/*
 * Compare to processors by their name.  Used by the set class to insert items
 */

struct ltGroup
{
    bool operator()(const Group * p1, const Group * p2) const { return strcmp( p1->name(), p2->name() ) < 0; }
};


/*
 * Compare a group name to a string.  Used by the find_if (and other algorithm type things.
 */

struct eqGroupStr 
{
    eqGroupStr( const char * s ) : _s(s) {}
    bool operator()(const Group * p1 ) const { return strcmp( p1->name(), _s ) == 0; }

private:
    const char * _s;
};

extern set<Group *, ltGroup> group;
#endif
