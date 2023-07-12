/* -*- c++ -*-
 * entity.h	-- Greg Franks
 *
 * $Id: entity.h 16765 2023-07-03 09:20:10Z greg $
 */

#ifndef _ENTITY_H
#define _ENTITY_H
#include "lqn2ps.h"
#include <vector>
#include <map>
#include <lqio/bcmp_document.h>
#include "element.h"

class Processor;
class Task;
class Arc;
class EntityCall;
class Entity;

namespace LQX {
    class SyntaxTreeNode;
}

/* ----------------------- Abstract Superclass ------------------------ */

class Entity : public Element
{
public:
    struct count_callers {
	count_callers( const callPredicate predicate ) : _predicate(predicate) {}
	unsigned int operator()( unsigned int, const Entity * entity ) const;
	
    private:
	const callPredicate _predicate;
    };

    struct accumulate_demand {
	accumulate_demand( BCMP::Model& model ) : _model(model) {}
	void operator()( const Entity * entity ) const { entity->accumulateDemand( _model.stationAt(entity->name() ) ); }
    private:
	BCMP::Model& _model;
    };

    struct create_station {
	create_station( BCMP::Model& model, BCMP::Model::Station::Type type = BCMP::Model::Station::Type::NOT_DEFINED ) : _model(model) {}
	void operator()( const Entity * entity ) const;
    private:
	BCMP::Model& _model;
    };

    struct label_BCMP_server {
	label_BCMP_server( const BCMP::Model& model ) : _model(model) {}
	void operator()( Entity * entity ) const;
	
    private:
	const BCMP::Model& _model;
    };

    
    struct label_BCMP_client {
	label_BCMP_client( const BCMP::Model& model ) : _model(model) {}
	void operator()( Entity * entity ) const;
	
    private:
	const BCMP::Model& _model;
    };

public:    
    static double to_double( LQX::SyntaxTreeNode * );
    static unsigned int to_unsigned( LQX::SyntaxTreeNode * );
    static LQX::SyntaxTreeNode * getLQXVariable( const LQIO::DOM::ExternalVariable* );
    static LQX::SyntaxTreeNode * getLQXVariable( const LQIO::DOM::ExternalVariable*, double );
    static LQX::SyntaxTreeNode * addLQXExpressions( LQX::SyntaxTreeNode *, LQX::SyntaxTreeNode * );
    static LQX::SyntaxTreeNode * multiplyLQXExpressions( LQX::SyntaxTreeNode *, LQX::SyntaxTreeNode * );
    static LQX::SyntaxTreeNode * divideLQXExpressions( LQX::SyntaxTreeNode *, LQX::SyntaxTreeNode * );

public:
    Entity( const LQIO::DOM::Entity*, const size_t id );
    virtual ~Entity();
    static bool compare( const Entity *, const Entity * );
    static bool compareLevel( const Entity *, const Entity * );
    static bool compareCoord( const Entity * e1, const Entity * e2 ) { return e1->index() < e2->index(); }

    virtual Entity& aggregate() { return *this; }

    /* Instance Variable Access */
	   
    const LQIO::DOM::ExternalVariable& copies() const;
    unsigned int copiesValue() const;
    const LQIO::DOM::ExternalVariable& replicas() const;
    unsigned int replicasValue() const;
    const LQIO::DOM::ExternalVariable& fanIn( const Entity * ) const;
    const LQIO::DOM::ExternalVariable& fanOut( const Entity * ) const;
    unsigned fanInValue( const Entity * ) const;
    unsigned fanOutValue( const Entity * ) const;
    const scheduling_type scheduling() const;

    Entity& setLevel( size_t level ) { _level = level; return *this; }
    size_t level() const { return _level; }
    bool isSelected() const { return _isSelected; }
    bool isNotSelected() const { return !_isSelected; }
    Entity& setSelected( bool yesOrNo ) { _isSelected = yesOrNo; return *this; }
    bool isSurrogate() const { return _isSurrogate; }
    Entity& setSurrogate( bool yesOrNo ) { _isSurrogate = yesOrNo; return *this; }
    virtual bool hasBogusUtilization() const;

    virtual double utilization() const = 0;

    const std::vector<GenericCall *>& callers() const { return _callers; }
    void addDstCall( GenericCall * aCall ) { _callers.push_back( aCall ); }
    void removeDstCall( GenericCall * aCall);

    /* Queries */

    virtual bool forwardsTo( const Task * aTask ) const     { return false; }
    virtual bool hasForwardingLevel() const                 { return false; }
    virtual bool hasCalls( const callPredicate ) const      { return false; }
    virtual bool isForwardingTarget() const                 { return false; }
    virtual bool isCalled( const request_type ) const       { return false; }
    bool isInfinite() const;
    bool isMultiServer() const;
    virtual bool isPureServer() const                       { return false; }
    virtual bool isTask() const                             { return false; }
    virtual bool isProcessor() const                        { return false; }
    virtual bool isReferenceTask() const                    { return false; }
    bool isReplicated() const;
    virtual bool isSelectedIndirectly() const;
    virtual bool isServerTask() const                       { return false; }

    bool test( const taskPredicate ) const;
    
    virtual bool check() const { return true; }
    virtual unsigned referenceTasks( std::vector<Entity *>&, Element * ) const = 0;
    virtual unsigned clients( std::vector<Task *> &, const callPredicate = nullptr ) const = 0;
    virtual unsigned servers( std::vector<Entity *> & ) const = 0;

    virtual double getIndex() const { return index(); }
    virtual Entity& sort();
    virtual unsigned setChain( unsigned k, callPredicate aFunc ) const { return k; }

    virtual bool isInOpenModel( const std::vector<Entity *>& servers ) const { return false; }
    virtual bool isInClosedModel( const std::vector<Entity *>& servers ) const { return false; }

#if defined(REP2FLAT)
    virtual Entity& removeReplication();
#endif

    double align() const;

    virtual Graphic::Colour colour() const;

    virtual Entity& label();
    virtual Entity& labelBCMPModel( const BCMP::Model::Station::Class::map_t&, const std::string& class_name="" ) = 0;

    virtual void accumulateDemand( BCMP::Model::Station& ) const = 0;

    std::ostream& print( std::ostream& output ) const;

    std::ostream& drawQueueingNetwork( std::ostream&, const double, const double, std::vector<bool>&, std::vector<Arc *>& ) const;
    virtual std::ostream& drawClient( std::ostream& output, const bool is_in_open_model, const bool is_in_closed_model ) const { return output; }
    virtual std::ostream& drawServer( std::ostream& ) const;

    virtual std::ostream& printName( std::ostream& output, const int = 0 ) const;
   
protected:
    double radius() const;
    unsigned countCallers() const;
    
private:
    Graphic::Colour chainColour( unsigned int ) const;
    std::ostream& drawServerToClient( std::ostream&, const double, const double, const Entity *, std::vector<bool> &, const unsigned ) const;
    std::ostream& drawClientToServer( std::ostream&, const Entity *, std::vector<bool> &, const unsigned, std::vector<Arc *>& lastArc ) const;

    static unsigned offsetOf( const std::set<unsigned>&, unsigned );

private:
    std::vector<GenericCall *> _callers;/* who calls me			*/
    size_t _level;			/* For sorting (by Y)		*/
    bool _isSelected;			/* Flag for picking off parts.	*/
    bool _isSurrogate;			/* Flag for formatting.		*/
};

std::ostream& operator<<( std::ostream& output, const Entity& self );
#endif
