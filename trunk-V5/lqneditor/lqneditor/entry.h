// =*- c++ -*-
//  entry.h
//  lqneditor
//
//  Created by Greg Franks on 2012-11-07.
//  Copyright (c) 2012 Real Time and Distrubuted Systems Group. All rights reserved.
//

#ifndef __lqneditor__entry__
#define __lqneditor__entry__

#include <stdexcept>
#include <set>
#include <map>
#include <wx/wx.h>
#include "node.h"

namespace LQIO {
    namespace DOM {
        class Entry;
    }
}

class Model;
class Task;
class Phase;
class ArcForEntry;

class Entry : public Node {
private:
    Entry( const Entry& );
    Entry& operator=( const Entry& );

public:
    Entry( const LQIO::DOM::Entry& entry, const Task& task, const Model& model );
    virtual ~Entry();
    
    void connectCalls();
    void addAsDestination( ArcForEntry * arc );
    void addAsSource( ArcForEntry * arc );
    
    virtual const unsigned int focusPriority() const { return 5; }
    const std::string& getName() const;

    void findChildren( std::set<const Task *>& call_chain ) const throw( std::runtime_error );

    virtual double utilization() const;

    Node& moveTo( const wxPoint& );
    Node& reorder();
    void render( wxDC& dc, bool draw_left, bool draw_right ) const;

public:
    std::map<unsigned,Phase *> _phases;

private:
    const Task& _task;
    const LQIO::DOM::Entry& _entry;
    std::vector<ArcForEntry *> _src_arcs;		/* from phase to dst entry... 	*/
    std::vector<ArcForEntry *> _dst_arcs;		/* from whatever to us.		*/
};

#endif /* defined(__lqneditor__entry__) */
