/* -*- c++ -*-
 * layer.h	-- Greg Franks
 *
 * $Id$
 */

#ifndef _LEVEL_H
#define _LEVEL_H

#include "lqn2ps.h"
#include "point.h"
#include "cltn.h"

class Entity;
class Task;
class Model;
class Layer;
class Label;
class Processor;
class Call;
namespace LQIO {
    namespace DOM {
	class Document;
    }
}

ostream& operator<<( ostream&, const Layer& );

typedef ostream& (Layer::*layerFunc)( ostream& ) const;


class Layer
{
private:
    Layer( const Layer& );

public:
    Layer();
    virtual ~Layer();

    Layer& operator<<( Entity * );
    Layer& operator+=( Entity * );
    Layer& operator-=( Entity * );
    int operator!() const { return myEntities.size() == 0; }	/* Layer is empty! */
    const Cltn<Entity *>& entities() const { return myEntities; }
    const Cltn<const Entity *>& clients() const { return myClients; }
    Layer& number( const unsigned n );
    unsigned number() const { return myNumber; }
    unsigned nChains() const { return myChains; }

    Layer const& rename() const;
    Layer const& check() const;
    Layer& prune();

    Layer const& aggregate() const;
    Layer const& sort( compare_func_ptr ) const;
    Layer const& format( const double ) const;
    Layer const& reformat() const;
    Layer const& label() const;
    Layer const& scaleBy( const double, const double ) const;
    Layer const& moveBy( const double, const double ) const;
    Layer const& moveLabelTo( const double, const double ) const;
    Layer const& translateY( const double ) const;
    Layer const& depth( const unsigned ) const;
    Layer const& fill( const double ) const;
    Layer const& justify( const double ) const;
    Layer const& justify( const double, const justification_type ) const;
    Layer const& align( const double ) const;
    Layer const& alignEntities() const;
    Layer const& shift( unsigned index, double amount ) const;

    Layer const& selectSubmodel() const;
    Layer const& deselectSubmodel() const;
    Layer const& generateSubmodel() const;
    Layer const& transmorgrify( LQIO::DOM::Document *, Processor *&, Task *& ) const;			/* BUG_626. */
    Layer const& generateClientSubmodel() const;

    unsigned int size() const { return entities().size(); }
    double width() const { return myExtent.x(); }
    double height() const { return myExtent.y(); }
    double x() const { return myOrigin.x(); }
    double y() const { return myOrigin.y(); }
    double labelWidth() const;

    ostream& print( ostream& ) const;
    ostream& printSummary( ostream& ) const;
    ostream& printSubmodel( ostream& ) const;
    ostream& drawQueueingNetwork( ostream& ) const;
#if defined(QNAP_OUTPUT)
    ostream& printQNAP( ostream& ) const;
#endif
#if defined(PMIF_OUTPUT)
    ostream& printPMIF( ostream& ) const;
#endif

private:
    double moveTo( double x, const double y, Entity * ) const;
    Processor * findOrAddSurrogateProcessor( LQIO::DOM::Document * document, Processor *& processor, Task * task, const unsigned level ) const;
    Task * findOrAddSurrogateTask( LQIO::DOM::Document * document, Processor *& processor, Task *& task, Entry * call, const unsigned level ) const;
    Entry * findOrAddSurrogateEntry( LQIO::DOM::Document * document, Task * task, Entry * call ) const;
    const Layer& resetServerPhaseParameters( LQIO::DOM::Document* document, LQIO::DOM::Phase * ) const;

private:
    Cltn<Entity *> myEntities;
    mutable Point myOrigin;
    mutable Point myExtent;
    unsigned myNumber;
    Label * myLabel;

    mutable Cltn<const Entity *> myClients;	/* Only if doing a submodel */
    mutable unsigned myChains;			/* Only set if doing a submodel */
};

#endif
