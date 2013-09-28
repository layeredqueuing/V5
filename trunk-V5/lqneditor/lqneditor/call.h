//
//  call.h
//  lqneditor
//
//  Created by Greg Franks on 2012-11-07.
//  Copyright (c) 2012 Real Time and Distrubuted Systems Group. All rights reserved.
//

#ifndef __lqneditor__call__
#define __lqneditor__call__

#include <stdexcept>
#include <set>

namespace LQIO {
    namespace DOM {
        class Call;
    }
}

class Model;
class Task;
class Entry;
class Phase;

class Call {
private:
    Call( const Call& );
    Call& operator=( const Call& );

public:
    Call( const LQIO::DOM::Call& call, const Phase* source, const Model& );
    virtual ~Call();

    void connect();
    void setDestination( const Entry* destination ) { _destination = destination; }
    const Entry * getDestination() const { return _destination; }

    void findChildren( std::set<const Task *>& call_chain ) const throw( std::runtime_error );

public:
    const LQIO::DOM::Call& _call;
    
private:
    const Model& _model;
    const Phase* _source;
    const Entry* _destination;
};


#endif /* defined(__lqneditor__call__) */
