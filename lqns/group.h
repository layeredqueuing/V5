/* -*- c++ -*-
 * $HeadURL: svn://localhost/lqn/trunk/lqns/group.h $
 *
 * Groups.
 *
 * Copyright the Real-Time and Distributed Systems Group,
 * Department of Systems and Computer Engineering,
 * Carleton University, Ottawa, Ontario, Canada. K1S 5B6
 *
 * November, 2008
 *
 * $Id: group.h 13982 2020-10-21 21:22:08Z greg $
 *
 * ------------------------------------------------------------------------
 */

#if	!defined(GROUP_H)
#define GROUP_H

#include "dim.h"
#include <set>
#include <lqio/dom_group.h>
#include "entity.h"

class Task;
class Processor;
class DeviceEntry;
class Server;
class Group;
class Processor;

class Group {
public:

    Group( LQIO::DOM::Group *, const Processor * aProcessor );
    virtual ~Group();
    static void create( const std::pair<std::string,LQIO::DOM::Group*>& );  

    /*  */

    bool check() const;

    Group& addTask( Task * aTask ) { _taskList.insert(aTask); return *this; }
    Group& removeTask( Task * aTask )  { _taskList.erase(aTask); return *this; }
    Group& recalculateDynamicValues();
    void initialize();
    void reinitialize();
    Group& reset();
    Group& initGroupTask();

    /* Printing */

    ostream& print( ostream& output ) const { return output; }
    const std::string& name() const { return myDOMGroup->getName(); }

    /* DOM insertion of results */

    virtual const Group& insertDOMResults() const;

public:
    static Group * find( const std::string& );

    
private:
    LQIO::DOM::Group* myDOMGroup;       /* DOM Element to Store Data	*/
    std::set<Task *> _taskList;	        /* List of processor's tasks	*/
    const Processor * _processor;
    const double myShare;		/* group share.		*/
    const bool myCap;
};

inline ostream& operator<<( ostream& output, const Group& self ) { return self.print( output ); }

#endif

