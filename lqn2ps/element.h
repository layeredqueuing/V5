/* -*- c++ -*-
 * element.h	-- Greg Franks
 *
 * ------------------------------------------------------------------------
 * $Id: element.h 14134 2020-11-25 18:12:05Z greg $
 * ------------------------------------------------------------------------
 */

#ifndef SRVN2EEPIC_ELEMENT_H
#define SRVN2EEPIC_ELEMENT_H

#include "lqn2ps.h"
#include <set>
#include <lqio/dom_object.h>
#include <lqio/dom_extvar.h>
#include "node.h"

class CallStack;
class Label;
class Call;
class Task;
class Element;

typedef enum { NOT_CALLED, RENDEZVOUS_REQUEST, SEND_NO_REPLY_REQUEST, OPEN_ARRIVAL_REQUEST } requesting_type;
typedef bool (Element::*chainTestFunc)( unsigned ) const;

class Element
{
public:
    Element( const LQIO::DOM::DocumentObject *, const size_t );
    virtual ~Element();

private:
    Element( const Element& );		/* Copying is verbotten */
    Element& operator=( const Element& );

public:
    virtual const std::string& name() const { return getDOM()->getName(); }
    Element& setName( const std::string& s ) { const_cast<LQIO::DOM::DocumentObject *>(getDOM())->setName(s); return *this; }
    const LQIO::DOM::DocumentObject * getDOM() const { return _documentObject; }
    Element& setDOM( const LQIO::DOM::DocumentObject * dom ) { _documentObject = dom; return *this; }

    virtual Element& rename() = 0;
    virtual Element& squishName();
    Element& addPath( const unsigned );

    bool hasPath( const unsigned aPath ) const { return myPaths.find( aPath ) != myPaths.end(); }
    bool pathTest() const;
    bool isReachable() const { return myPaths.size() > 0; }
    virtual int span() const { return 0; }
    double index() const { return (left() + right()) / 2.0; }
    const std::set<unsigned>& paths() const { return myPaths; }
    virtual Element& reformat() { return *this; }

    virtual Element& setClientClosedChain( unsigned );
    virtual Element& setClientOpenChain( unsigned );
    virtual Element& setServerChain( unsigned );
    bool hasClientChain( unsigned ) const;
    bool hasClientClosedChain( unsigned ) const;
    bool hasClientOpenChain( unsigned ) const;
    bool hasServerChain( unsigned ) const;

    virtual Graphic::colour_type colour() const = 0;

    Point center() const { return myNode->center(); }
    Point bottomLeft() const { return myNode->bottomLeft(); }
    Point bottomCenter() const { return myNode->bottomCenter(); }
    Point bottomRight() const { return myNode->bottomRight(); }
    Point topCenter() const { return myNode->topCenter(); }
    Point topLeft() const { return myNode->topLeft(); }
    Point topRight() const { return myNode->topRight(); }
    double width() const { return myNode->width(); }
    double height() const { return myNode->height(); }
    double left() const { return myNode->left(); }
    double right() const { return myNode->left() + myNode->width(); }
    double top() const { return myNode->bottom() + myNode->height(); }
    double bottom() const { return myNode->bottom(); }

    virtual Element& moveTo( const double x, const double y ) { myNode->moveTo( x, y ); return *this; }
    virtual Element& moveBy( const double, const double );
    virtual Element& scaleBy( const double, const double );
    virtual Element& translateY( const double );
    virtual Element& depth( const unsigned );

    virtual Element& label() = 0;

    /* Printing */

    virtual const Element& draw( std::ostream& output ) const = 0;

protected:
    size_t followCalls( std::pair<std::vector<Call *>::const_iterator,std::vector<Call *>::const_iterator>, CallStack&, const unsigned ) const;
    virtual double getIndex() const = 0;
    Graphic::colour_type colourForUtilization( double ) const;
    Graphic::colour_type colourForDifference( double ) const;

    static double adjustForSlope( double y );

public:
#if defined(REP2FLAT)
    std::string baseReplicaName( unsigned int& ) const;
#endif    

    size_t elementId() const { return _elementId; }
    static bool compare( const Element *, const Element * );

private:
    Element const& addForwardingRendezvous( CallStack& callStack ) const;

private:
    const LQIO::DOM::DocumentObject * _documentObject;
    const size_t _elementId;		/* Index			*/

protected:
    Label * myLabel;			/* Label (for drawing).		*/
    Node * myNode;			/* Graphical thingy.		*/

    std::set<unsigned> myPaths;		/* Who calls me.		*/
    std::set<unsigned> myClientOpenChains;	/* Chains when I am a client 	*/
    std::set<unsigned> myClientClosedChains;	/* Chains when I am a client 	*/
    std::set<unsigned> myServerChains;	/* Chains when I am a server	*/

public:
    static const LQIO::DOM::ConstantExternalVariable ZERO;
};

typedef bool (* compare_func_ptr)( const Element *, const Element * );
#endif
