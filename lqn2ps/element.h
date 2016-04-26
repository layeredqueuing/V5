/* -*- c++ -*-
 * element.h	-- Greg Franks
 *
 * ------------------------------------------------------------------------
 * $Id: element.h 11963 2014-04-10 14:36:42Z greg $
 * ------------------------------------------------------------------------
 */

#ifndef SRVN2EEPIC_ELEMENT_H
#define SRVN2EEPIC_ELEMENT_H

#include "lqn2ps.h"
#include <lqio/dom_object.h>
#include <lqio/dom_extvar.h>
#include "node.h"
#include "vector.h"

template <class type> class Stack;
template <class type> class Sequence;
class Label;
class Call;
class Task;
class CallStack;
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
    virtual const string& name() const { return getDOM()->getName(); }
    Element& setName( const string& s ) { const_cast<LQIO::DOM::DocumentObject *>(getDOM())->setName(s); return *this; }
    const LQIO::DOM::DocumentObject * getDOM() const { return _documentObject; }
    Element& setDOM( const LQIO::DOM::DocumentObject * dom ) { _documentObject = dom; return *this; }

    size_t elementId() const { return myElementId; }

    virtual void rename();
    virtual void squishName();
    Element& addPath( const unsigned );

    bool hasPath( const unsigned aPath ) const { return myPaths.find( aPath ); }
    bool pathTest() const;
    bool isReachable() const { return myPaths.size() > 0; }
    virtual int span() const { return 0; }
    double index() const { return (left() + right()) / 2.0; }
    const Vector<unsigned>& paths() const { return myPaths; }
    virtual Element& reformat() { return *this; }

    virtual Element& setClientClosedChain( unsigned );
    virtual Element& setClientOpenChain( unsigned );
    virtual Element& setServerChain( unsigned );
    bool hasClientChain( unsigned ) const;
    bool hasClientClosedChain( unsigned ) const;
    bool hasClientOpenChain( unsigned ) const;
    bool hasServerChain( unsigned ) const;
    unsigned closedChainAt( unsigned ) const;
    unsigned openChainAt( unsigned ) const;
    unsigned serverChainAt( unsigned ) const;

    virtual Graphic::colour_type colour() const = 0;

    Point& origin() const { return myNode->origin; }
    Point& extent() const { return myNode->extent; }
    Point center() const { return myNode->center(); }
    Point bottomLeft() const { return myNode->bottomLeft(); }
    Point bottomCenter() const { return myNode->bottomCenter(); }
    Point bottomRight() const { return myNode->bottomRight(); }
    Point topCenter() const { return myNode->topCenter(); }
    Point topLeft() const { return myNode->topLeft(); }
    Point topRight() const { return myNode->topRight(); }
    double width() const { return myNode->extent.x(); }
    double height() const { return myNode->extent.y(); }
    double left() const { return myNode->origin.x(); }
    double right() const { return myNode->origin.x() + myNode->extent.x(); }
    double top() const { return myNode->origin.y()  + myNode->extent.y(); }
    double bottom() const { return myNode->origin.y(); }

    virtual Element& moveTo( const double x, const double y ) { myNode->moveTo( x, y ); return *this; }
    virtual Element& moveBy( const double, const double );
    virtual Element& scaleBy( const double, const double );
    virtual Element& translateY( const double );
    virtual Element& depth( const unsigned );

    virtual Element& label() = 0;

    /* Printing */

    virtual ostream& draw( ostream& output ) const = 0;

protected:
    unsigned followCalls( const Task *, Sequence<Call *>&, CallStack&, const unsigned ) const;
    virtual double getIndex() const = 0;

    static int compare( const Element *, const Element * );
    static double adjustForSlope( double y );

private:
    Element const& addForwardingRendezvous( CallStack& callStack ) const;

protected:
    const LQIO::DOM::DocumentObject * _documentObject;
    size_t myElementId;			/* Index			*/
    Label * myLabel;			/* Label (for drawing).		*/
    Node * myNode;			/* Graphical thingy.		*/

    Vector<unsigned> myPaths;		/* Who calls me.		*/
    Vector<unsigned> myClientOpenChains;	/* Chains when I am a client 	*/
    Vector<unsigned> myClientClosedChains;	/* Chains when I am a client 	*/
    Vector<unsigned> myServerChains;	/* Chains when I am a server	*/

public:
    static const LQIO::DOM::ConstantExternalVariable ZERO;
};

#if defined(QNAP_OUTPUT) || defined(PMIF_OUTPUT)
class QNAPElementManip {
public:
    QNAPElementManip( ostream& (*ff)(ostream&, const Element &, const bool, const unsigned ), 
		     const Element & theElement, const bool multi_server, const unsigned theChain ) 
	: f(ff), anElement(theElement), aMultiServer(multi_server), aChain(theChain) {}

private:
    ostream& (*f)( ostream&, const Element&, const bool, const unsigned );
    const Element & anElement;
    const bool aMultiServer;
    const unsigned aChain;

    friend ostream& operator<<(ostream & os, const QNAPElementManip& m ) 
	{ return m.f(os,m.anElement,m.aMultiServer,m.aChain); }
};

QNAPElementManip server_chain( const Element& anElement, const bool, const unsigned );
QNAPElementManip closed_chain( const Element& anElement, const bool, const unsigned );
QNAPElementManip open_chain( const Element& anElement, const bool, const unsigned );
typedef QNAPElementManip (*QNAP_Element_func)( const Element& anElement, const bool multi_class, const unsigned i );

#endif
#endif
