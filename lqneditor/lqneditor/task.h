// -*- c++ -*-
//  task.h
//  lqneditor
//
//  Created by Greg Franks on 2012-11-06.
//  Copyright (c) 2012 Real Time and Distrubuted Systems Group. All rights reserved.
//

#ifndef __lqneditor__task__
#define __lqneditor__task__

#include <vector>
#include <wx/wx.h>
#include "node.h"

namespace LQIO {
    namespace DOM {
        class Task;
    }
}

class Model;
class Processor;
class Entry;
class ArcForProcessor;
class TaskLabel;

class Task : public Node
{
public:
    Task( const LQIO::DOM::Task& task, const Processor* processor, Model& model );
    virtual ~Task();

    virtual const unsigned int focusPriority() const { return 10; }
    const Processor * getProcessor() const { return _processor; }
    const LQIO::DOM::Task& getDOM() const { return _task; }
    virtual const std::string& getName() const;

    void connectCalls();
    
    bool isReferenceTask() const;
    bool isMultiServer() const;
    bool isInfinite() const;

    virtual double utilization() const;

    Node& moveTo( const wxPoint& );
    Node& reorder();
    void render( wxDC& dc ) const;
    
public:
    std::vector<Entry *> _entries;

private:
    const LQIO::DOM::Task& _task;
    const Processor * _processor;
    TaskLabel * _label;
    ArcForProcessor * _processor_call;
};

#endif /* defined(__lqneditor__task__) */
