// -*- c++ -*-
//  label.h
//  lqneditor
//
//  Created by Greg Franks on 2012-11-19.
//  Copyright (c) 2012 Real Time and Distrubuted Systems Group. All rights reserved.
//

#ifndef __lqneditor__label__
#define __lqneditor__label__

#include <wx/wx.h>
#include <wx/gdicmn.h>
#include "node.h"

namespace LQIO {
    namespace DOM {
        class Task;
    }
}

class Model;

class Label : public Node {
private:
    Label( const Label& );
    Label& operator=( const Label& );

public:
    Label( Model& model );
    virtual ~Label();

    virtual const unsigned int focusPriority() const { return 1; }

    Node& moveTo( const wxPoint& );

protected:
    wxPoint _center;
    wxFont _font;
};


class TaskLabel : public Label {
public:
    TaskLabel( const LQIO::DOM::Task& task, Model& model );
    virtual ~TaskLabel();

    virtual const std::string& getName() const;

    void render( wxDC& dc ) const;

private:
    const LQIO::DOM::Task& _task;
};

#endif /* defined(__lqneditor__label__) */
