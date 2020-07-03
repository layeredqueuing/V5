/* -*- c++ -*-
 * $HeadURL: http://rads-svn.sce.carleton.ca:8080/svn/lqn/trunk-V5/lqns/processor.h $
 *
 * Processors.
 *
 * Copyright the Real-Time and Distributed Systems Group,
 * Department of Systems and Computer Engineering,
 * Carleton University, Ottawa, Ontario, Canada. K1S 5B6
 *
 * November, 1994
 * May, 2009
 *
 * ------------------------------------------------------------------------
 * $Id: processor.h 13548 2020-05-21 14:27:18Z greg $
 * ------------------------------------------------------------------------
 */

#if	!defined(PROCESSOR_H)
#define PROCESSOR_H

#include "dim.h"
#include <lqio/dom_processor.h>
#include <set>
#include <cstring>
#include "entity.h"

class Task;
class Processor;
class DeviceEntry;
class Format;
class Server;

class Processor : public Entity {

public:
    static void create( LQIO::DOM::Processor* );

protected:
    Processor( LQIO::DOM::Processor* );

public:
    virtual ~Processor();

    /* Initialization */

    virtual void check() const;
    virtual void configure( const unsigned );
    virtual Processor& initPopulation();

    /* Instance Variable access */

    const Cltn<Task *>& tasks() const { return taskList; }
    virtual Entity& processor( Processor * aProcessor );
    virtual const Processor * processor() const;
    virtual double rate() const;
    virtual unsigned int fanOut( const Entity * ) const;
    virtual unsigned int fanIn( const Task * ) const;

    /* Queries */
	
    virtual bool isProcessor() const { return true; }
    virtual unsigned nClients() const { return taskList.size(); }
    virtual bool hasVariance() const;
    bool hasPriorities() const;
    virtual unsigned validScheduling() const;

    /* Model Building. */

    Processor& addTask( Task * );
    Processor& removeTask( Task * );
    Server * makeServer( const unsigned nChains );

    /* DOM insertion of results */

    virtual void insertDOMResults(void) const;
    virtual ostream& print( ostream& ) const;

public:
    static Processor * find( const char * processor_name );

protected:
    LQIO::DOM::Processor* getDOM() const { return dynamic_cast<LQIO::DOM::Processor *>(domEntity); }	/* DOM Element to Store Data	*/
	
private:
    Cltn<Task *> taskList;			/* List of processor's tasks	*/
};


/*
 * Special processor class which is used to implement delays in the
 * model such as phase think times.  There is no DOM attached to it,
 * so we have to fake out some of the get methods.
 */

class DelayServer : public Processor
{
public:
    DelayServer() : Processor( 0 ) {}		/* No Dom */
    
    virtual double rate() const { return 1.0; }
    virtual const char * name() const { return "DELAY"; }
    virtual scheduling_type scheduling() const { return SCHEDULE_DELAY; }
    virtual unsigned copies() const { return 1; }
    virtual unsigned replicas() const { return 1; }
    virtual bool isInfinite() const { return true; }

    virtual void insertDOMResults(void) const {};	/* NOP */
};


/*
 * Compare two processors by their name.  Used by the set class to insert items
 */

struct ltProcessor
{
    bool operator()(const Processor * p1, const Processor * p2) const { return strcmp( p1->name(), p2->name() ) < 0; }
};


/*
 * Compare a processor name to a string.  Used by the find_if (and other algorithm type things.
 */

struct eqProcStr 
{
    eqProcStr( const char * s ) : _s(s) {}
    bool operator()(const Processor * p1 ) const { return strcmp( p1->name(), _s ) == 0; }

private:
    const char * _s;
};

extern set<Processor *, ltProcessor> processor;

/* -------------------------------------------------------------------- */
/* Funky Formatting functions for inline with <<.			*/
/* -------------------------------------------------------------------- */

class SRVNProcessorManip {
public:
    SRVNProcessorManip( ostream& (*ff)(ostream&, const Processor & ), const Processor & theProcessor ) : f(ff), aProcessor(theProcessor) {}
private:
    ostream& (*f)( ostream&, const Processor& );
    const Processor & aProcessor;

    friend ostream& operator<<(ostream & os, const SRVNProcessorManip& m ) { return m.f(os,m.aProcessor); }
};

SRVNProcessorManip processor_type( const Processor& aProcessor );

#endif
