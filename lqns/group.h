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
 * $Id: group.h 14140 2020-11-25 20:24:15Z greg $
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

    Group( LQIO::DOM::Group *, const Processor * );
    virtual ~Group();
    static void create( const std::pair<std::string,LQIO::DOM::Group*>& );  

    /*  */

    bool check() const;

    Group& addTask( Task * task ) { _taskList.insert(task); return *this; }
    Group& removeTask( Task * task )  { _taskList.erase(task); return *this; }
    Group& recalculateDynamicValues();
    void initialize();
    void reinitialize();
    Group& reset();
    Group& initGroupTask();

    /* Printing */

    std::ostream& print( std::ostream& output ) const { return output; }
    const std::string& name() const { return _dom->getName(); }

    /* DOM insertion of results */

    virtual const Group& insertDOMResults() const;

public:
    static Group * find( const std::string& );

    
private:
    LQIO::DOM::Group* _dom; 		/* DOM Element to Store Data	*/
    std::set<Task *> _taskList;	        /* List of processor's tasks	*/
    const Processor * _processor;
    const double _share;		/* group share.		*/
    const bool _cap;
};

inline std::ostream& operator<<( std::ostream& output, const Group& self ) { return self.print( output ); }

#endif

