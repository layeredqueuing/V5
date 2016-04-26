/* -*- c++ -*-
 * open.h	-- Greg Franks
 *
 * $Id: open.h 11963 2014-04-10 14:36:42Z greg $
 */

#ifndef _OPEN_H
#define _OPEN_H

#include "lqn2ps.h"
#include "cltn.h"
#include "entity.h"

class Task;
class OpenArrival;
class OpenArrivalSource;

extern Cltn<OpenArrivalSource *> opensource;

/* ------------- Open Arrival Pseudo Tasks (for drawing)  ------------- */

class OpenArrivalSource : public Entity {
public:
    OpenArrivalSource( const Entry * source );
    ~OpenArrivalSource();

    virtual const string& name() const;
    virtual void rename() {}			/* Don't bother */
    virtual void squishName() {}		/* Don't bother */
    virtual ostream& draw( ostream& output ) const;
    virtual ostream& print( ostream& output ) const;
    virtual OpenArrivalSource& moveSrc( const Point& aPoint );
    double utilization() const { return 0.0; }
    virtual OpenArrivalSource& processor( const Processor * );
    const Processor * processor() const { return 0; }

    virtual unsigned referenceTasks( Cltn<const Entity *>&, Element * ) const { return 0; }	/* We don't have clients */
    virtual unsigned clients( Cltn<const Entity *>&, const callFunc = 0 ) const { return 0; }	/* We don't have clients */
    virtual unsigned servers( Cltn<const Entity *>& ) const;
    virtual bool isInOpenModel( const Cltn<Entity *>& servers ) const;
    virtual bool isSelectedIndirectly() const;
    
    virtual unsigned setChain( unsigned, callFunc aFunc ) const;
    virtual OpenArrivalSource& aggregate();

    virtual double radius() const;
    virtual OpenArrivalSource& moveTo( const double x, const double y );
    virtual OpenArrivalSource& label();

    virtual Graphic::colour_type colour() const;

    virtual OpenArrivalSource& scaleBy( const double, const double );
    virtual OpenArrivalSource& translateY( const double );
    virtual OpenArrivalSource& depth( const unsigned );

#if defined(PMIF_OUTPUT)
    virtual ostream& printPMIFServer( ostream& output ) const;
    virtual ostream& printPMIFClient( ostream& output ) const;
    virtual ostream& printPMIFRequests( ostream& output ) const;
    virtual ostream& printPMIFArcs( ostream& output ) const;
#endif
    virtual ostream& drawClient( ostream&, const bool is_in_open_model, const bool is_in_closed_model ) const;

#if defined(QNAP_OUTPUT)
    virtual ostream& printQNAPClient( ostream& output, const bool is_in_open_model, const bool is_in_closed_model, const bool multi_class ) const;
#endif

private:
#if defined(QNAP_OUTPUT)
    ostream& printQNAPRequests( ostream& output, const bool multi_class ) const;
#endif

private:
    const Entry * myEntry;		/* Entry requesting arrivals.	*/
    Cltn<OpenArrival *> myCalls;	/* Arc calling entry		*/
};
#endif
