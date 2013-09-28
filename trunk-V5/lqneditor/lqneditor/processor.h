//
//  processor.h
//  lqneditor
//
//  Created by Greg Franks on 2012-11-06.
//  Copyright (c) 2012 Real Time and Distrubuted Systems Group. All rights reserved.
//

#ifndef __lqneditor__processor__
#define __lqneditor__processor__

#include <wx/wx.h>
#include <vector>
#include "node.h"

namespace LQIO {
    namespace DOM {
        class Processor;
    }
}

class Model;
class ArcForProcessor;

class Processor : public Node
{
public:
    Processor( const LQIO::DOM::Processor& processor, Model& model );

    virtual const unsigned int focusPriority() const { return 10; }
    virtual const std::string& getName() const;

    bool isMultiServer() const;
    bool isInfinite() const;

    void addAsDestination( ArcForProcessor * arc );

    virtual double utilization() const;

    Node& moveTo( const wxPoint& );
    void render( wxDC& dc ) const;
    
private:
    const LQIO::DOM::Processor& _processor;
    std::vector<ArcForProcessor *> _dst_arcs;		/* from whatever to us.		*/
};

#endif /* defined(__lqneditor__processor__) */
