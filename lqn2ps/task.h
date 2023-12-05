/* -*- c++ -*-
 *
 * Tasks.
 *
 * Copyright the Real-Time and Distributed Systems Group,
 * Department of Systems and Computer Engineering,
 * Carleton University, Ottawa, Ontario, Canada. K1S 5B6
 *
 * November, 1994
 * April 2010.
 *
 * ------------------------------------------------------------------------
 * $Id: task.h 16875 2023-12-02 22:48:35Z greg $
 * ------------------------------------------------------------------------
 */

#ifndef TASK_H
#define TASK_H

#include "lqn2ps.h"
#include <string>
#include <map>
#include <vector>
#include "entity.h"
#include "actlayer.h"

class ActivityList;
class Call;
class EntityCall;
class OpenArrival;
class Processor;
class Share;
class Task;
class TaskCall;

#if BUG_270
typedef std::multimap<const Entity *,EntityCall *>::const_iterator merge_iter;
typedef std::pair<const Entity *,EntityCall *> merge_pair;
#endif


/* ----------------------- Abstract Superclass ------------------------ */

class Task : public Entity {
    typedef double (Task::*taskPhaseFunc)( const unsigned ) const;

#if REP2FLAT
    class UpdateFanInOut {
    public:
	UpdateFanInOut( LQIO::DOM::Task& src ) : _src( src ) {}
	void operator()( Entry * entry ) const;
	void operator()( Activity * activity ) const;
	void updateFanInOut( const std::vector<Call *>& calls ) const;
    private:
	LQIO::DOM::Task& _src;
    };
#endif
    
private:
    friend class Layer;
    
    struct create_chain {
	create_chain( BCMP::Model& model, BCMP::Model::Station& terminals, const std::vector<Entity *>& servers ) : _model(model), _terminals(terminals), _servers(servers) {}
	void operator()( const Task * ) const;
    private:
	BCMP::Model& _model;
	BCMP::Model::Station& _terminals;
	const std::vector<Entity *>& _servers;
    };

    struct create_demand {
	create_demand( BCMP::Model& model, BCMP::Model::Station& terminals, const std::vector<Entity *>& servers ) : _model(model), _terminals(terminals), _servers(servers) {}
	void operator()( const Task * ) const;
    private:
	BCMP::Model& _model;
	BCMP::Model::Station& _terminals;
	const std::vector<Entity *>& servers() const { return _servers; }
	const std::vector<Entity *>& _servers;
    };

public:
    enum class root_level_t { IS_NON_REFERENCE, IS_REFERENCE, HAS_OPEN_ARRIVALS };

    static bool holdingTimePresent;
    static bool holdingVariancePresent;
    static void reset();
    static Task* create( const LQIO::DOM::Task* domTask, std::vector<Entry *>& entries );

protected:
    Task( const LQIO::DOM::Task* dom, const Processor * processor, const Share * share, const std::vector<Entry *>& entries );

public:
    virtual ~Task();
    virtual Task * clone( unsigned int, const std::string& name, const Processor * processor, const Share * share ) const = 0;

    Entity& addProcessor( const Processor * processor ) { _processors.insert(processor); return *this; }
    Entity& removeProcessor( const Processor * processor ) { _processors.erase(processor); return *this; }
    const std::set<const Processor *>& processors() const { return _processors; }
    bool hasProcessor( const Processor * ) const;
    const Processor * processor() const;
    
    const Share * share() const { return _share; }
    bool hasPriority() const;

    virtual root_level_t rootLevel() const;
    virtual Task& sort();
    virtual double getIndex() const;
    virtual int span() const;

    const std::vector<Entry *>& entries() const { return _entries; }
    Task& addEntry( Entry * );
    Task& removeEntry( Entry * );
    Activity * findActivity( const std::string& name ) const;
    Activity * findOrAddActivity( const LQIO::DOM::Activity * );
#if REP2FLAT
    Activity * addActivity( const Activity&, const unsigned );
    Activity * findActivity( const Activity&, const unsigned );
#endif
    Task& removeActivity( Activity * );
    void addPrecedence( ActivityList * );
    const std::vector<Activity *>& activities() const { return _activities; }
    const std::vector<ActivityList *>& precedences() const { return _precedences; }

    unsigned nEntries() const { return _entries.size(); }
    unsigned nActivities() const { return _activities.size(); }

    virtual unsigned setChain( unsigned, callPredicate aFunc ) const;
    virtual Task& setServerChain( unsigned );

    virtual bool forwardsTo( const Task * aTask ) const;
    virtual bool hasForwardingLevel() const;
    virtual bool isForwardingTarget() const;
    virtual bool isCalled( const request_type ) const;
    virtual bool hasThinkTime() const { return false; }

    double openArrivalRate() const;

    virtual double holdingTime( const unsigned p ) const { return 0.0; }
    virtual double holdingVariance( const unsigned p ) const { return 0.0; }
    double throughput() const;
    double utilization( const unsigned p ) const;
    virtual double utilization() const;
    double processorUtilization() const;
    double idleTime() const;

    bool hasActivities() const { return activities().size() != 0; }
    virtual bool hasCalls( const callPredicate ) const;
    bool hasOpenArrivals() const;
    bool hasQueueingTime() const;
    virtual bool hasHoldingTime() const { return false; }
    virtual bool hasHoldingVariance() const { return false; }

    virtual bool isTask() const { return true; }
    virtual bool isPureServer() const;
    virtual bool isSelectedIndirectly() const;

    virtual bool canConvertToReferenceTask() const;
    virtual bool canConvertToOpenArrivals() const;

    virtual bool check() const;
    virtual unsigned referenceTasks( std::vector<Entity *>&, Element * dst ) const;
    virtual unsigned clients( std::vector<Task *>&, const callPredicate = nullptr ) const;
    virtual unsigned servers( std::vector<Entity *>& ) const;
    unsigned maxPhase() const { return _maxPhase; }			/* Max phase over all entries	*/

    virtual size_t findChildren( CallStack&, const unsigned );
    unsigned countThreads() const;
    std::vector<Activity *> repliesTo( Entry * anEntry ) const;
    const std::vector<EntityCall *>& calls() const { return _calls; }
    EntityCall * findOrAddCall( Task *, const callPredicate aFunc );
    EntityCall * findOrAddPseudoCall( Entity * );		// For -Lclient
#if BUG_270
    void addSrcCall( EntityCall * );
    void removeSrcCall( EntityCall * aCall );
#endif

    virtual bool isInOpenModel( const std::vector<Entity *>& servers ) const;
    virtual bool isInClosedModel( const std::vector<Entity *>& servers  ) const;

#if BUG_270
    static LQX::SyntaxTreeNode * sum_rendezvous( LQX::SyntaxTreeNode *, const merge_pair& );
    static LQX::SyntaxTreeNode * sum_demand( LQX::SyntaxTreeNode *, const merge_pair& );
#endif
    static double sum_throughput( double augend, const Task * task ) { return augend + task->throughput(); }
    
    /* Activities */
    
    unsigned generate();
    virtual Task& format();
    virtual Task& reformat();
    double justify();
    double justifyByEntry();
    double alignActivities();

    /* movement */

    virtual Task& moveBy( const double dx, const double dy );
    virtual Task& moveTo( const double x, const double y );
    virtual Task& scaleBy( const double, const double );
    virtual Task& translateY( const double );
    virtual Task& depth( const unsigned );

    virtual Graphic::Colour colour() const;

    virtual Task& label();
    virtual Task& labelBCMPModel( const BCMP::Model::Station::Class::map_t&, const std::string& class_name="" );
    Task& labelQueueingNetwork( entryLabelFunc func, Label& label );

    virtual Task& rename();
    virtual Task& squish( std::map<std::string,unsigned>&, std::map<std::string,std::string>& );
    Task& aggregate();

#if BUG_270
    bool canPrune() const;
    virtual Task& relink();
    Task& unlinkFromProcessor();
    Task& mergeCalls();
#endif
#if REP2FLAT
    virtual Task& removeReplication();
    Task& expand();
    Task& replicateCall();
    Task& replicateActivity();
    Task& replicateTask( LQIO::DOM::DocumentObject ** );
    static void updateFanInOut();
#endif

    /* Printing */
    
    virtual const Task& draw( std::ostream& output ) const;
    std::ostream& printEntries( std::ostream& ) const;
    std::ostream& printActivities( std::ostream& ) const;

    virtual std::ostream& drawClient( std::ostream&, const bool is_in_open_model, const bool is_in_closed_model ) const;

private:
    size_t topologicalSort();
    unsigned countArcs( const callPredicate = 0 ) const;
    double countCalls( const callPredicate2 ) const;

    EntityCall * findCall( const Entity *, const callPredicate aFunc = 0 ) const;
    Task& moveDst();
    Task& moveSrc();
    Task& moveSrcBy( const double dx, const double dy );

    void renameFanInOut( const std::string&, const std::string& );
    void renameFanInOut( std::map<const std::string,const LQIO::DOM::ExternalVariable *>&, const std::string&, const std::string& );
    
#if REP2FLAT
    Task& expandActivities( const Task& src, int replica );

protected:
    LQIO::DOM::Task * cloneDOM( const std::string& name, LQIO::DOM::Processor * dom_processor ) const;
    const std::vector<Entry *>& groupEntries( int replica, std::vector<Entry *>& newEntryList  ) const;

public:
#endif

    static std::set<Task *,LT<Task> > __tasks;	/* All tasks in model		*/
    static std::map<std::string,unsigned> __key_table;		/* For squish	*/
    static std::map<std::string,std::string> __symbol_table;	/* For squish	*/

protected:
    std::vector<Entry *> _entries;		/* Entries for this entity.	*/
    std::vector<Activity *> _activities;	/* Activities for this entity.	*/
    std::vector<ActivityList *> _precedences;	/* Precendences for this entity	*/

private:
    Task( const Task& );
    Task& operator=( const Task& );

private:
    std::set<const Processor *> _processors;	/* proc(s). allocated to task. 	*/
    const Share * _share;			/* share for this task.		*/
    mutable unsigned _maxPhase;			/* Max phase over all entries	*/
    std::vector<EntityCall *> _calls;		/* Arc calling processor	*/
    std::vector<ActivityLayer> _layers;	
    double _entryWidthInPts;
    std::map<std::string,unsigned> _key_table;		/* For squishName 	*/
    std::map<std::string,std::string> _symbol_table;	/* For rename		*/

    static const double JLQNDEF_TASK_BOX_SCALING;
};

inline std::ostream& operator<<( std::ostream& output, const Task& self ) { self.draw( output ); return output; }

/* ------------------------- Reference Tasks -------------------------- */

class ReferenceTask : public Task {
public:
    ReferenceTask( const LQIO::DOM::Task* dom, const Processor * processor, const Share * share, const std::vector<Entry *>& aCltn );
    virtual ReferenceTask * clone( unsigned int, const std::string& name, const Processor * processor, const Share * share ) const;

    virtual double getIndex() const { return index(); }

    virtual bool isReferenceTask() const { return true; }
    virtual bool isPureServer() const { return false; }
    virtual bool hasThinkTime() const;
    const LQIO::DOM::ExternalVariable& thinkTime() const;
    virtual bool hasBogusUtilization() const;

    virtual bool canConvertToOpenArrivals() const { return false; }

    virtual root_level_t rootLevel() const { return root_level_t::IS_REFERENCE; }

    virtual Graphic::Colour colour() const;
    virtual size_t findChildren( CallStack&, const unsigned );

#if BUG_270
    bool canPrune() const;
    virtual Task& relink();
#endif

public:
    static const std::string __BCMP_station_name;
};


/* --------------------------- Server Tasks --------------------------- */

class ServerTask : public Task {
public:
    ServerTask( const LQIO::DOM::Task* dom, const Processor * processor, const Share * share, const std::vector<Entry *>& aCltn );
    virtual ServerTask * clone( unsigned int, const std::string& name, const Processor * processor, const Share * share ) const;

    virtual bool isServerTask() const   { return true; }
    virtual bool canConvertToReferenceTask() const;
};

/* ------------------------ Lock Server Tasks ------------------------- */

class SemaphoreTask : public Task {
public:
    SemaphoreTask( const LQIO::DOM::Task* dom, const Processor * processor, const Share * share, const std::vector<Entry *>& entries );
    virtual SemaphoreTask * clone( unsigned int, const std::string& name, const Processor * processor, const Share * share ) const;

    virtual bool isServerTask() const   { return true; }

private:
};

class RWLockTask : public Task {
public:
    RWLockTask( const LQIO::DOM::Task* dom, const Processor * processor, const Share * share, const std::vector<Entry *>& entries );
    virtual RWLockTask * clone( unsigned int, const std::string& name , const Processor * processor, const Share * share ) const;

    virtual bool isServerTask() const   { return true; }

private:
};

/*
 * Compare a task name to a string.  Used by the find_if (and other algorithm type things.
 */

struct eqTaskStr 
{
    eqTaskStr( const std::string& s ) : _s(s) {}
    bool operator()(const Task * p1 ) const { return p1->name() == _s; }

private:
    const std::string & _s;
};
#endif
