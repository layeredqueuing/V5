/* -*- c++ -*-
 * $HeadURL: http://rads-svn.sce.carleton.ca:8080/svn/lqn/trunk/lqns/processor.h $
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
 * $Id: processor.h 16961 2024-01-28 02:12:54Z greg $
 * ------------------------------------------------------------------------
 */

#ifndef LQNS_PROCESSOR_H
#define LQNS_PROCESSOR_H

#include <lqio/dom_processor.h>
#include <set>
#include "entity.h"

class Task;
class Processor;
class Server;
class Group;

class Processor : public Entity {

public:
    static void create( const std::pair<std::string,LQIO::DOM::Processor*>& );

protected:
    Processor( LQIO::DOM::Processor* );
    Processor( const Processor& processor, unsigned int replica );

public:
    virtual ~Processor();

protected:
    virtual Processor * clone( unsigned int replica ) { return new Processor( *this, replica ); }
    
public:
    /* Initialization */
    virtual void initializeServer();

    virtual bool check() const;
    virtual Processor& configure( const unsigned );
    virtual void recalculateDynamicValues() {}

    /* Instance Variable access */

    virtual LQIO::DOM::Processor * getDOM() const { return dynamic_cast<LQIO::DOM::Processor *>(Entity::getDOM()); }
    virtual double rate() const;
    virtual unsigned int fanOut( const Entity * ) const;
    virtual unsigned int fanIn( const Task * ) const;

    /* Queries */

    virtual bool isProcessor() const { return true; }
    bool isInteresting() const;
    virtual bool hasVariance() const;
    bool hasPriorities() const;

    virtual const Entity& sanityCheck() const;

    /* Model Building. */

    Processor& addGroup( Group * group ) { _groups.insert( group ); return *this; }
    const std::set<Group *>& groups() const { return _groups; }

    virtual Entity* mapToReplica( size_t ) const;
    Processor& expand();
    
    Server * makeServer( const unsigned nChains );
    virtual double computeUtilization( const MVASubmodel&, const Server& );

    /* DOM insertion of results */

    virtual const Processor& insertDOMResults() const;
    virtual std::ostream& print( std::ostream& ) const;
    std::ostream& printTasks( std::ostream& output, unsigned int submodel ) const;

protected:
    virtual bool schedulingIsOK() const;

public:
    static Processor * find( const std::string&, unsigned int=1 );

private:
    std::set<Group *> _groups;		/* List of processor's Group	*/
};


/*
 * Special processor class which is used to implement delays in the
 * model such as phase think times.  There is no DOM attached to it,
 * so we have to fake out some of the get methods.
 */

class DelayServer : public Processor
{
public:
    DelayServer() : Processor( nullptr ) {}		/* No Dom */
    virtual ~DelayServer() = default;

private:
    DelayServer( const DelayServer& ) = delete;
    DelayServer& operator=( const DelayServer& ) = delete;

public:
    virtual Processor * clone() { throw std::runtime_error( "DelayServer::clone()" ); return nullptr; }
    virtual bool check() const { return true; }

    virtual double rate() const { return 1.0; }
    virtual const std::string& name() const { static const std::string s="DELAY"; return s; }
    virtual scheduling_type scheduling() const { return SCHEDULE_DELAY; }
    virtual unsigned copies() const { return 1; }
    virtual unsigned replicas() const { return 1; }
    virtual bool isInfinite() const { return true; }

    virtual const DelayServer& insertDOMResults() const { return *this; }	/* NOP */
};
#endif
