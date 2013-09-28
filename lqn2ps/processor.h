/* -*- c++ -*-
 *
 * Processors.
 *
 * Copyright the Real-Time and Distributed Systems Group,
 * Department of Systems and Computer Engineering,
 * Carleton University, Ottawa, Ontario, Canada. K1S 5B6
 *
 * November, 1994
 *
 * ------------------------------------------------------------------------
 * $Id$
 * ------------------------------------------------------------------------
 */

#if	!defined(PROCESSOR_H)
#define PROCESSOR_H

#include "lqn2ps.h"
#include <cstring>
#include "vector.h"
#include "entity.h"
#include "share.h"


class Task;
class Share;
class Processor;

ostream& operator<<( ostream&, const Processor& );

class Processor : public Entity {
public:
    static ostream& printHeader( ostream& );

public:
    Processor( const LQIO::DOM::Processor* aDomProcessor );

    virtual ~Processor();
    Processor * clone( unsigned int ) const;

    static int compare( const void *, const void * );
    static Processor * find( const string& name );
    static Processor * create( const LQIO::DOM::Processor* processor );

    /* Instance Variable access */

    const set<Task *,ltTask>& tasks() const { return taskList; }
    int nTasks() const { return taskList.size(); }
    const set<Share *,ltShare>& shares() const { return shareList; }
    int nShares() const { return shareList.size(); }
    virtual Entity& processor( const Processor * aProcessor );
    virtual const Processor * processor() const;
    bool hasRate() const;
    LQIO::DOM::ExternalVariable& rate() const;
    LQIO::DOM::ExternalVariable& quantum() const;
    unsigned taskDepth() const;
    double meanLevel() const;
	
    virtual double utilization() const;

    /* Queries */
	
    bool hasPriorities() const;
    bool isInteresting() const;
    virtual bool isProcessor() const { return true; }
    virtual bool isPureServer() const { return true; }
    virtual bool isSelectedIndirectly() const;

    bool hasGroup() const { return myGroupSelected; }
    Processor& hasGroup( const bool yesOrNo ) { myGroupSelected = yesOrNo; return *this; }

    virtual unsigned nClients() const;
    virtual unsigned referenceTasks( Cltn<const Entity *>&, Element * dst ) const;
    virtual unsigned clients( Cltn<const Entity *>&, const callFunc = 0 ) const;
    virtual unsigned servers( Cltn<const Entity *>& ) const { return 0; }	/* Processors don't have servers */

    virtual double serviceTimeForQueueingNetwork( const unsigned k, chainTestFunc ) const;

    /* Model Building. */

    virtual double getIndex() const;

    virtual Processor& moveBy( const double dx, const double dy );
    virtual Processor& moveTo( const double x, const double y );
    virtual Graphic::colour_type colour() const;
    virtual Processor& label();

    Processor& addTask( Task * );
    Processor& removeTask( Task * );
    Processor& addShare( Share * );
    Processor& removeShare( Share * );

#if defined(REP2FLAT)
    static Processor * find_replica( const string&, const unsigned ) throw( runtime_error );
    Processor * expandProcessor( const int extention ) const;
#endif

    /* Printing */

    virtual ostream& draw( ostream& output ) const;

private:
    Processor( const Processor& );
    Processor& operator=( const Processor& );

    Processor& moveDst();
    bool clientsCanQueue() const;

private:
    LQIO::DOM::Processor* myDOMProcessor;	/* DOM Element to Store Data	*/
    set<Task *,ltTask> taskList;
    set<Share *,ltShare> shareList;
    bool myGroupSelected;
};

/*
 * Compare to processors by their name.  Used by the set class to insert items
 */

struct ltProcessor
{
    bool operator()(const Processor * p1, const Processor * p2) const { return p1->name() < p2->name(); }
};


/*
 * Compare a processor name to a string.  Used by the find_if (and other algorithm type things.
 */

struct eqProcStr 
{
    eqProcStr( const string & s ) : _s(s) {}
    bool operator()(const Processor * p1 ) const { return p1->name() == _s; }

private:
    const string & _s;
};

extern set<Processor *, ltProcessor> processor;
#endif
