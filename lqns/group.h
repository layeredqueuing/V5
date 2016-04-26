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
 * $Id: group.h 11963 2014-04-10 14:36:42Z greg $
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
class Format;
class Server;
class Group;
class Processor;

ostream& operator<<( ostream&, const Group& );

class Group {
public:
    Group( const char * aStr, const Processor * aProcessor, const double aShare, const bool cap );
//    virtual ~Group();
    /* Printing */

    ostream& print( ostream& ) const;
    const char * name() const { return myName.c_str(); }

    /* DOM insertion of results */

    void insertDOMResults(void) const;

public:
    static Group * find( const char * );

private:

    const string myName;
    const Processor * myProcessor;	
    const double myShare;		/* group share.		*/
    const bool myCap;
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

void add_group (LQIO::DOM::Group* domGroup);
extern set<Group *, ltGroup> group;
#endif

