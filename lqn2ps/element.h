/* -*- c++ -*-
 * element.h	-- Greg Franks
 *
 * ------------------------------------------------------------------------
 * $Id: element.h 16872 2023-11-29 15:56:00Z greg $
 * ------------------------------------------------------------------------
 */

#ifndef SRVN2EEPIC_ELEMENT_H
#define SRVN2EEPIC_ELEMENT_H

#include "lqn2ps.h"
#include <set>
#include <map>
#include <lqio/dom_object.h>
#include <lqio/dom_extvar.h>
#include "node.h"

class CallStack;
class Label;
class Call;
class Task;
class Element;

enum class request_type { NOT_CALLED, RENDEZVOUS, SEND_NO_REPLY, OPEN_ARRIVAL };
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
    size_t elementId() const { return _elementId; }

    virtual Element& rename() = 0;
    virtual Element& squish( std::map<std::string,unsigned>&, std::map<std::string,std::string>& );

    Element& addPath( const unsigned );

    bool hasPath( const unsigned path ) const { return _paths.find( path ) != _paths.end(); }
    bool pathTest() const;
    bool isReachable() const { return !_paths.empty(); }
    virtual int span() const { return 0; }
    double index() const { return (left() + right()) / 2.0; }
    const std::set<unsigned>& paths() const { return _paths; }
    virtual Element& reformat() { return *this; }

    virtual Element& setClientClosedChain( unsigned );
    virtual Element& setClientOpenChain( unsigned );
    virtual Element& setServerChain( unsigned );
    bool hasClientChain( unsigned ) const;
    bool hasClientClosedChain( unsigned ) const;
    bool hasClientOpenChain( unsigned ) const;
    bool hasServerChain( unsigned ) const;

    virtual Graphic::Colour colour() const = 0;

    Point center() const { return _node->center(); }
    Point bottomLeft() const { return _node->bottomLeft(); }
    Point bottomCenter() const { return _node->bottomCenter(); }
    Point bottomRight() const { return _node->bottomRight(); }
    Point topCenter() const { return _node->topCenter(); }
    Point topLeft() const { return _node->topLeft(); }
    Point topRight() const { return _node->topRight(); }
    double width() const { return _node->width(); }
    double height() const { return _node->height(); }
    double left() const { return _node->left(); }
    double right() const { return _node->left() + _node->width(); }
    double top() const { return _node->bottom() + _node->height(); }
    double bottom() const { return _node->bottom(); }

    virtual Element& moveTo( const double x, const double y ) { _node->moveTo( x, y ); return *this; }
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
    Graphic::Colour colourForUtilization( double ) const;
    Graphic::Colour colourForDifference( double ) const;

    static double adjustForSlope( double y );

public:
#if defined(REP2FLAT)
    std::string baseReplicaName( unsigned int& ) const;
    static void cloneObservations( const LQIO::DOM::DocumentObject *, const LQIO::DOM::DocumentObject * );
#endif    

    static bool compare( const Element *, const Element * );

private:
    Element const& addForwardingRendezvous( CallStack& callStack ) const;

private:
    const LQIO::DOM::DocumentObject * _documentObject;
    const size_t _elementId;		/* Index			*/

protected:
    Label * _label;			/* Label (for drawing).		*/
    Node * _node;			/* Graphical thingy.		*/

    std::set<unsigned> _paths;		/* Who calls me.		*/
    std::set<unsigned> _clientOpenChains;	/* Chains when I am a client 	*/
    std::set<unsigned> _clientClosedChains;	/* Chains when I am a client 	*/
    std::set<unsigned> _serverChains;	/* Chains when I am a server	*/
};

typedef bool (* compare_func_ptr)( const Element *, const Element * );
#endif
