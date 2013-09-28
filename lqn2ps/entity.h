/* -*- c++ -*-
 * entity.h	-- Greg Franks
 *
 * $Id$
 */

#ifndef _ENTITY_H
#define _ENTITY_H
#include "lqn2ps.h"
#include "vector.h"
#include "cltn.h"
#include "element.h"
#include <lqio/input.h>

class Entity;
class Processor;
class Task;
class Arc;

ostream& operator<<( ostream&, const Entity& );


/* ----------------------- Abstract Superclass ------------------------ */

class Entity : public Element {

public:
    Entity( const LQIO::DOM::Entity*, const size_t id );
    virtual ~Entity();
    static int compare( const void *, const void * );
    static int compareLevel( const void *, const void * );
    static int compareCoord( const void *, const void * );

    virtual Entity& aggregate() { return *this; }

    /* Instance Variable Access */
	   
    virtual const Processor * processor() const = 0;
    virtual Entity& processor( const Processor * aProcessor ) = 0;
    virtual Entity& setCopies( const unsigned anInt );
    const LQIO::DOM::ExternalVariable& copies() const;
    const unsigned replicas() const;
    const scheduling_type scheduling() const;

    Entity& setLevel( unsigned aLevel ) { myLevel = aLevel; return *this; }
    unsigned level() const { return myLevel; }
    Entity& setSubmodel( unsigned aSubmodel ) { mySubmodel = aSubmodel; return *this; }
    unsigned submodel() const { return mySubmodel; }
    bool isSelected() const { return iAmSelected; }
    Entity& isSelected( bool yesOrNo ) { iAmSelected = yesOrNo; return *this; }
    bool isSurrogate() const { return iAmASurrogate; }
    Entity& isSurrogate( bool yesOrNo ) { iAmASurrogate = yesOrNo; return *this; }
    bool hasBogusUtilization() const;

    virtual double variance() const { return myVariance; }
    virtual double utilization() const = 0;

    const Cltn<GenericCall *>& callerList() const { return myCallers; }
    void addDstCall( GenericCall * aCall ) { myCallers << aCall; }
    void removeDstCall( GenericCall * aCall) { myCallers -= aCall; }

    /* Queries */

    virtual bool forwardsTo( const Task * aTask ) const     { return false; }
    virtual bool hasForwardingLevel() const                 { return false; }
    virtual bool hasCalls( const callFunc ) const           { return false; }
    virtual bool isForwardingTarget() const                 { return false; }
    virtual bool isCalled( const requesting_type ) const    { return false; }
    bool isInfinite() const;
    bool isMultiServer() const;
    virtual bool isPureServer() const                       { return false; }
    virtual bool isTask() const                             { return false; }
    virtual bool isProcessor() const                        { return false; }
    virtual bool isReferenceTask() const                    { return false; }
    bool isReplicated() const                               { return replicas() > 1; }
    virtual bool isSelectedIndirectly() const;
    virtual bool isServerTask() const                       { return false; }
    unsigned fanIn( const Entity * ) const;
    unsigned fanOut( const Entity * ) const;

    virtual void check() const {}
    virtual unsigned referenceTasks( Cltn<const Entity *>&, Element * ) const = 0;
    virtual unsigned clients( Cltn<const Entity *> &, const callFunc = 0 ) const = 0;
    virtual unsigned servers( Cltn<const Entity *> & ) const = 0;

    virtual double getIndex() const { return index(); }
    virtual Entity const & sort() const;
    virtual unsigned setChain( unsigned k, callFunc aFunc ) const { return k; }

    virtual bool isInOpenModel( const Cltn<Entity *>& servers ) const { return false; }
    virtual bool isInClosedModel( const Cltn<Entity *>& servers  ) const { return false; }

    Entity& serviceTime( const unsigned k, const double s );
    double serviceTime( const unsigned k ) const;
    virtual double serviceTimeForQueueingNetwork( const unsigned k, chainTestFunc ) const { return 0.0; }
#if defined(REP2FLAT)
    virtual Entity& removeReplication();
#endif

    double align() const;

    virtual Graphic::colour_type colour() const;

    virtual Entity& label();

    ostream& print( ostream& output ) const;

    ostream& drawQueueingNetwork( ostream&, const double, const double, Vector<bool> &, Cltn<Arc *>& ) const;
    virtual ostream& drawClient( ostream& output, const bool is_in_open_model, const bool is_in_closed_model ) const { return output; }
    virtual ostream& drawServer( ostream& ) const;
#if defined(QNAP_OUTPUT)
    virtual ostream& printQNAPClient( ostream& output, const bool is_in_open_model, const bool is_in_closed_model, const bool multi_class ) const { return output; }
    ostream& printQNAPServer( ostream& output, const bool multi_class ) const;
#endif
#if defined(PMIF_OUTPUT)
    virtual ostream& printPMIFServer( ostream& output ) const;
    virtual ostream& printPMIFClient( ostream& output ) const { return output; }
    virtual ostream& printPMIFArcs( ostream& output ) const { return output; }
    virtual ostream& printPMIFReplies( ostream& output ) const;
#endif

    virtual ostream& printName( ostream& output, const int = 0 ) const;

protected:
    double radius() const;
    unsigned countCallers() const;

private:
    Graphic::colour_type chainColour( unsigned int ) const;
    ostream& drawServerToClient( ostream&, const double, const double, const Entity *, Vector<bool> &, const unsigned, const unsigned ) const;
    ostream& drawClientToServer( ostream&, const Entity *, Vector<bool> &, const unsigned, const unsigned, Cltn<Arc *>& lastArc ) const;
#if defined(QNAP_OUTPUT)
    ostream& printQNAPReplies( ostream& output, const bool multi_class ) const;
#endif

protected:
    double myVariance;			/* Computed variance.		*/
    Cltn<GenericCall *> myCallers;	/* Arc calling processor	*/

    Vector<double> myServiceTime;	/* Service time by chain.	*/
    Vector<double> myWait;		/* Wait to other tasks by chain.*/

    unsigned myLevel;			/* For sorting (by Y)		*/
    unsigned mySubmodel;		/* For printing submodels.	*/
    unsigned myIndex;			/* For sorting arcs. 		*/

    bool iAmSelected;			/* Flag for picking off parts.	*/
    bool iAmASurrogate;			/* Flag for formatting.		*/
};


/* -------------------------------------------------------------------- */
/* Funky Formatting functions for inline with <<.			*/
/* -------------------------------------------------------------------- */

class SRVNEntityManip {
public:
    SRVNEntityManip( ostream& (*ff)(ostream&, const Entity & ), const Entity & theEntity ) 
	: f(ff), anEntity(theEntity) {}

private:
    ostream& (*f)( ostream&, const Entity& );
    const Entity & anEntity;

    friend ostream& operator<<(ostream & os, const SRVNEntityManip& m ) 
	{ return m.f(os,m.anEntity); }
};


SRVNEntityManip copies_of( const Entity & anEntity );
SRVNEntityManip replicas_of( const Entity & anEntity );
SRVNEntityManip scheduling_of( const Entity & anEntity );
#if defined(QNAP_OUTPUT)
SRVNEntityManip qnap_name( const Entity & anEntity );
SRVNEntityManip qnap_replicas( const Entity & anEntity );
#endif
#endif
