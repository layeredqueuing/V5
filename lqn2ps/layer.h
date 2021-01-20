/* -*- c++ -*-
 * layer.h	-- Greg Franks
 *
 * $Id: layer.h 14381 2021-01-19 18:52:02Z greg $
 */

#ifndef _LQN2PS_LAYER_H
#define _LQN2PS_LAYER_H

#include "lqn2ps.h"
#include <vector>
#include <lqio/bcmp_document.h>
#include "entity.h"
#include "point.h"

class Label;
class Task;
class Processor;

class Layer
{
private:
    typedef Task& (Task::*taskFPtr)();

    /*
     * Position tasks and processors
     */

    class Position
    {
    public:
	Position( taskFPtr f, double y=MAXDOUBLE ) : _f(f), _x(0.0), _y(y), _h(0.0) {}
	void operator()( Entity * );
	double x() const { return _x; }
	double y() const { return _y; }
	double height() const { return _h; }
    private:
	taskFPtr _f;
	double _x;
	double _y;
	double _h;
    };

public:
    Layer();
    Layer( const Layer& );
    Layer& operator=( const Layer& );
    virtual ~Layer();

    Layer& append( Entity * );
    Layer& erase( std::vector<Entity *>::iterator );
    int operator!() const { return _entities.size() == 0; }	/* Layer is empty! */
    const std::vector<Entity *>& entities() const { return _entities; }
    const std::vector<Entity *>& clients() const { return _clients; }
    Layer& number( const unsigned n );
    unsigned number() const { return _number; }
    unsigned nChains() const { return _chains; }

    bool check() const;
    Layer& prune();

    Layer& sort( compare_func_ptr );
    Layer& format( const double );
    Layer& reformat();
    Layer& label();
    Layer& scaleBy( double, double );
    Layer& moveBy( double, double );
    Layer& moveLabelTo( double, double );
    Layer& translateY( double );
    Layer& depth(  unsigned );
    Layer& fill( double );
    Layer& justify( double );
    Layer& justify( double,  justification_type );
    Layer& align();
    Layer& alignEntities();
    Layer& shift( unsigned index, double amount );

    Layer& selectSubmodel();
    Layer& deselectSubmodel();
    Layer& generateSubmodel();
    Layer& transmorgrify( LQIO::DOM::Document *, Processor *&, Task *& );			/* BUG_626. */
    Layer& aggregate();
    Layer& createBCMPModel();

    unsigned int size() const { return entities().size(); }
    double width() const { return _extent.x(); }
    double height() const { return _extent.y(); }
    double x() const { return _origin.x(); }
    double y() const { return _origin.y(); }
    double labelWidth() const;

    unsigned count( const taskPredicate ) const;
    unsigned count( const callPredicate ) const;

    std::ostream& print( std::ostream& ) const;
    std::ostream& printSummary( std::ostream& ) const;
    std::ostream& printSubmodelSummary( std::ostream& ) const;
    std::ostream& printSubmodel( std::ostream& ) const;
    std::ostream& drawQueueingNetwork( std::ostream& ) const;
#if JMVA_OUTPUT || QNAP2_OUTPUT
    std::ostream& printBCMPQueueingNetwork( std::ostream& ) const;
#endif

private:
    Processor * findOrAddSurrogateProcessor( LQIO::DOM::Document * document, Processor *& processor, Task * task, const size_t level ) const;
    Task * findOrAddSurrogateTask( LQIO::DOM::Document * document, Processor *& processor, Task *& task, Entry * call, const size_t level ) const;
    Entry * findOrAddSurrogateEntry( LQIO::DOM::Document * document, Task * task, Entry * call ) const;
    const Layer& resetServerPhaseParameters( LQIO::DOM::Document* document, LQIO::DOM::Phase * ) const;

private:
    std::vector<Entity *> _entities;
    Point _origin;
    Point _extent;
    unsigned _number;
    Label * _label;

    std::vector<Entity *> _clients;		/* Only if doing a submodel 	*/
    mutable unsigned _chains;			/* Only set if doing a submodel */
    BCMP::Model  _bcmp_model;			/* For queuing output		*/
};

inline std::ostream& operator<<( std::ostream& output, const Layer& self ) { return self.print( output ); }
#endif
