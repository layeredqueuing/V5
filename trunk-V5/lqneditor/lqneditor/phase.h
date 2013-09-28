// -*- c++ -*-
//  phase.h
//  lqneditor
//
//  Created by Greg Franks on 2012-11-07.
//  Copyright (c) 2012 Real Time and Distrubuted Systems Group. All rights reserved.
//

#ifndef __lqneditor__phase__
#define __lqneditor__phase__

#include <stdexcept>
#include <set>
#include <map>
#include <vector>

namespace LQIO {
    namespace DOM {
        class Phase;
    }
}

class Model;
class Task;
class Entry;
class Call;
class ArcForEntry;

class Phase {
private:
    Phase( const Phase& );
    Phase& operator=( const Phase& );

public:
    Phase( const unsigned int n, const LQIO::DOM::Phase& phase, const Entry& entry, const Model& model );
    virtual ~Phase();

    void connectCalls( std::vector<ArcForEntry *>& arcs );

    void findChildren( std::set<const Task *>& call_chain ) const throw( std::runtime_error );

public:
    std::vector<Call *> _calls;

private:
    const unsigned int _p;
    const Model& _model;
    const Entry& _entry;
    const LQIO::DOM::Phase& _phase;
};

#endif /* defined(__lqneditor__phase__) */
