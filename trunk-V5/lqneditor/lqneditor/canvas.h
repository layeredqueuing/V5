// -*- c++ -*-
//   canvas.h
//  lqneditor
//
//  Created by Greg Franks on 2012-11-05.
//  Copyright (c) 2012 Real Time and Distrubuted Systems Group. All rights reserved.
//

#ifndef __lqneditor___canvas__
#define __lqneditor___canvas__

#include <iostream>
#include "wx/wx.h"

class Model;
class Node;

class Canvas : public wxPanel
{
public:
    Canvas( wxFrame* parent );

    void setModel( Model * model ) { _model = model; }

    void openObject( wxCommandEvent& event );
    void editParameters( wxCommandEvent& event );
    void rightClick( wxMouseEvent& evt );
    void paintEvent( wxPaintEvent& evt );
    void closeObject( wxCommandEvent& event );
    void paintNow();
    
    void render( wxDC& dc );

    DECLARE_EVENT_TABLE()

private:
    Model * _model;
    wxMenu * _menu;
    mutable Node * _node;
};

#endif /* defined(__lqneditor___canvas__) */
