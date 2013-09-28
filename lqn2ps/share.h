/* share.h	-- Greg Franks
 *
 * $HeadURL$
 *
 * ------------------------------------------------------------------------
 * $Id$
 * ------------------------------------------------------------------------
 */

#ifndef _CFSGROUP_H
#define _CFSGROUP_H

#include "lqn2ps.h"
#include <cstring>
#include <lqio/dom_group.h>
#include <lqio/dom_extvar.h>
#include "cltn.h"

class Share;
class Processor;

ostream& operator<<( ostream&, const Share& );


class Share
{
public:
    Share( const LQIO::DOM::Group* dom, const Processor * aProcessor );

    const LQIO::DOM::Group * getDOM() const { return _documentObject; }
    const string& name() const { return getDOM()->getName(); }
    double share() const { return getDOM()->getGroupShareValue(); }
    bool cap() const { return getDOM()->getCap(); }
    const Processor * processor() const { return myProcessor; }

    /* Printing */

    ostream& draw( ostream& output ) const;
    ostream& print( ostream& output ) const;

    static Share * find( const string& );
    static void create( LQIO::DOM::Group* domGroup );

private:
    const LQIO::DOM::Group * _documentObject;
    const Processor * myProcessor;
};


/*
 * Compare to processors by their name.  Used by the set class to insert items
 */

struct ltShare
{
    bool operator()(const Share * p1, const Share * p2) const { return p1->name() < p2->name(); }
};


/*
 * Compare a share name to a string.  Used by the find_if (and other algorithm type things.
 */

struct eqShareStr 
{
    eqShareStr( const string& s ) : _s(s) {}
    bool operator()(const Share * p1 ) const { return p1->name() == _s; }

private:
    const string& _s;
};

extern set<Share *,ltShare> share;
#endif
