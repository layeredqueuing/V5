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
 * $Id: processor.h 14644 2021-05-14 15:09:03Z greg $
 * ------------------------------------------------------------------------
 */

#if	!defined(PROCESSOR_H)
#define PROCESSOR_H

#include "lqn2ps.h"
#include <string>
#include <set>
#include "entity.h"
#include "share.h"

class Task;
class Share;
class Demand;

class Processor : public Entity {
public:
    Processor( const LQIO::DOM::Processor* aDomProcessor );

    virtual ~Processor();
private:
    Processor * clone( const std::string& ) const;

public:
    static bool compare( const void *, const void * );
    static Processor * find( const std::string& name );
    static Processor * create( const std::pair<std::string,LQIO::DOM::Processor *>& );

    /* Instance Variable access */

    const std::set<Task *>& tasks() const { return _tasks; }
    int nTasks() const { return _tasks.size(); }
    const std::set<Share *,LT<Share> >& shares() const { return _shares; }
    int nShares() const { return _shares.size(); }
    bool hasRate() const;
    const LQIO::DOM::ExternalVariable& rate() const;
    bool hasQuantum() const;
    const LQIO::DOM::ExternalVariable& quantum() const;
    size_t taskDepth() const;
    double meanLevel() const;
	
    virtual double utilization() const;

    /* Queries */
	
    bool hasPriorities() const;
    bool isInteresting() const;
    virtual bool isProcessor() const { return true; }
    virtual bool isPureServer() const { return true; }

    bool hasGroup() const { return _groupIsSelected; }
    Processor& hasGroup( const bool yesOrNo ) { _groupIsSelected = yesOrNo; return *this; }

    virtual unsigned nClients() const;
    virtual unsigned referenceTasks( std::vector<Entity *>&, Element * dst ) const;
    virtual unsigned clients( std::vector<Task *>&, const callPredicate = 0 ) const;
    virtual unsigned servers( std::vector<Entity *>& ) const { return 0; }	/* Processors don't have servers */

    /* Model Building. */

    virtual double getIndex() const;

    virtual Processor& moveBy( const double dx, const double dy );
    virtual Processor& moveTo( const double x, const double y );
    virtual Graphic::colour_type colour() const;

    virtual Processor& label();
    virtual Processor& labelBCMPModel( const BCMP::Model::Station::Class::map_t&, const std::string& class_name="" );

    virtual Processor& rename();

    Processor& addTask( Task * task ) { _tasks.insert(task); return *this; }
    Processor& removeTask( Task * task ) { _tasks.erase(task); return *this; }
    Processor& addShare( Share * share ) { _shares.insert(share); return *this; }
    Processor& removeShare( Share * share ) { _shares.erase(share); return *this; }

#if defined(REP2FLAT)
    static Processor * find_replica( const std::string&, const unsigned );
    Processor& expand();
    Processor& replicateProcessor( LQIO::DOM::DocumentObject ** );
#endif

    /* Printing */

    virtual const Processor& draw( std::ostream& output ) const;

protected:
    virtual void accumulateDemand( BCMP::Model::Station& ) const;

public:
    static std::ostream& printHeader( std::ostream& );
    static std::set<Processor *, LT<Processor> > __processors;
    static std::map<std::string,unsigned> __key_table;		/* For squish	*/
    static std::map<std::string,std::string> __symbol_table;	/* For squish	*/

private:
    Processor( const Processor& );
    Processor& operator=( const Processor& );

    Processor& moveDst();
    bool clientsCanQueue() const;

private:
    std::set<Task *> _tasks;
    std::set<Share *, LT<Share> > _shares;
    bool _groupIsSelected;
};

inline std::ostream& operator<<( std::ostream& output, const Processor& self ) { self.draw( output ); return output; }
#endif
