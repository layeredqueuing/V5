/* -*- c++ -*-
 * layer.h	-- Greg Franks
 *
 * $Id: layer.h 16972 2024-01-29 19:23:49Z greg $
 */

#ifndef _LQN2PS_LAYER_H
#define _LQN2PS_LAYER_H

#include "lqn2ps.h"
#include <limits>
#include <vector>
#include <lqio/bcmp_document.h>
#include "entity.h"
#include "point.h"

class Label;
class Task;
class Phase;
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
	Position( taskFPtr f, double y=std::numeric_limits<double>::max() ) : _f(f), _x(0.0), _y(y), _h(0.0) {}
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

    class ResetServerPhaseParameters
    {
    public:
	ResetServerPhaseParameters( bool hasResults ) : _hasResults(hasResults) {}
	void operator()( const std::pair<unsigned,LQIO::DOM::Phase*>& p ) const { reset( p.second ); }
	void operator()( const std::pair<std::string,LQIO::DOM::Activity*>& p  ) const { reset( p.second ); }
    private:
	void reset( LQIO::DOM::Phase * phase ) const;
	void reset( LQIO::DOM::Activity * activity ) const;
	bool _hasResults;
    };

public:
    Layer();
    Layer( const Layer& );
    Layer& operator=( const Layer& );
    virtual ~Layer();

    Layer& append( Entity * );
    Layer& remove( Entity * );
    Layer& erase( std::vector<Entity *>::iterator );
    int operator!() const { return _entities.size() == 0; }	/* Layer is empty! */
    const std::vector<Entity *>& entities() const { return _entities; }
    const std::vector<Task *>& clients() const { return _clients; }
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
    Layer& justify( double,  Justification );
    Layer& align();
    Layer& alignEntities();
    Layer& shift( unsigned index, double amount );

    Layer& selectSubmodel();
    Layer& deselectSubmodel();
    Layer& generateSubmodel();
    Layer& transmorgrifyClients( LQIO::DOM::Document * );		/* BUG_440 */
    Layer& transmorgrifyServers( LQIO::DOM::Document * );		/* BUG_440 */
    Layer& aggregate();
    bool createBCMPModel();

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
    const BCMP::Model& getBCMPModel() const { return _bcmp_model; }
#endif

private:
    Layer& addSurrogateProcessor( LQIO::DOM::Document * document, Task * task, const size_t level );
    void resetClientPhaseParameters( Entry * entry );

private:
    std::vector<Entity *> _entities;
    Point _origin;
    Point _extent;
    unsigned _number;
    Label * _label;

    std::vector<Task *> _clients;		/* Only if doing a submodel 	*/
    mutable unsigned _chains;			/* Only set if doing a submodel */
    BCMP::Model _bcmp_model;			/* For queuing output		*/
};

inline std::ostream& operator<<( std::ostream& output, const Layer& self ) { return self.print( output ); }
#endif
