/* share.h	-- Greg Franks
 *
 * $HeadURL: http://rads-svn.sce.carleton.ca:8080/svn/lqn/trunk-V5/lqn2ps/share.h $
 *
 * ------------------------------------------------------------------------
 * $Id: share.h 14381 2021-01-19 18:52:02Z greg $
 * ------------------------------------------------------------------------
 */

#ifndef _CFSGROUP_H
#define _CFSGROUP_H

#include "lqn2ps.h"
#include <lqio/dom_group.h>
#include <lqio/dom_extvar.h>

class Processor;

class Share
{
    friend class Model;
    
public:
    Share( const LQIO::DOM::Group* dom, const Processor * aProcessor );

    const LQIO::DOM::Group * getDOM() const { return _documentObject; }
    const std::string& name() const { return getDOM()->getName(); }
    double share() const { return getDOM()->getGroupShareValue(); }
    bool cap() const { return getDOM()->getCap(); }
    const Processor * processor() const { return _processor; }

    /* Printing */

    std::ostream& draw( std::ostream& output ) const { return output; }
    std::ostream& print( std::ostream& output ) const;

    static Share * find( const std::string& );
    static void create( const std::pair<std::string,LQIO::DOM::Group*>& );


private:
    const LQIO::DOM::Group * _documentObject;
    const Processor * _processor;

    static std::set<Share *,LT<Share> > __share;
};


inline std::ostream& operator<<( std::ostream& output, const Share& self ) { return output; }
#endif
