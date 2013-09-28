//
//  canvas.cpp
//  lqneditor
//
//  Created by Greg Franks on 2012-11-05.
//  Copyright (c) 2012 Real Time and Distrubuted Systems Group. All rights reserved.
//

#include <iostream>
#include "canvas.h"
#include "frame.h"
#include "model.h"
#include "node.h"
#include "task.h"
#include "dialog.h"

BEGIN_EVENT_TABLE(Canvas, wxPanel)
// some useful events
/*
 EVT_MOTION(Canvas::mouseMoved)
 EVT_LEFT_DOWN(Canvas::mouseDown)
 EVT_LEFT_UP(Canvas::mouseReleased)
 EVT_LEAVE_WINDOW(Canvas::mouseLeftWindow)
 EVT_KEY_DOWN(Canvas::keyPressed)
 EVT_KEY_UP(Canvas::keyReleased)
 EVT_MOUSEWHEEL(Canvas::mouseWheelMoved)
 */
 
EVT_RIGHT_DOWN(Canvas::rightClick)
EVT_MENU(ID_EditParameters, Canvas::editParameters)
EVT_MENU(ID_OpenObject,	    Canvas::openObject)
// catch paint events
EVT_PAINT(Canvas::paintEvent)
 
END_EVENT_TABLE()

Canvas::Canvas( wxFrame * parent )
    : wxPanel(parent), _model(0), _menu(0), _node(0)
{
}


void Canvas::paintEvent( wxPaintEvent& evt )
{
    wxPaintDC dc(this);
    if ( Model::TWIPS == 20 ) {
	dc.SetMapMode( wxMM_TWIPS );
    }
    render(dc);
}


void Canvas::paintNow()
{
    wxPaintDC dc(this);
    if ( Model::TWIPS == 20 ) {
	dc.SetMapMode( wxMM_TWIPS );
    }
    render(dc);
}


void Canvas::rightClick( wxMouseEvent& event )
{
    if ( !_menu ) {
	_menu = new wxMenu;
	_menu->Append( ID_EditParameters, wxT("&Edit") );
	_menu->Append( ID_OpenObject, wxT("&Open") );
    }
    wxPoint position( event.GetPosition() );
    _node = _model->findNode( position );
    if ( _node ) {
//	std::cerr << "Found node " << node->getName() << std::endl;
	PopupMenu( _menu, position );
    }
}

void Canvas::editParameters( wxCommandEvent& event )
{
    TaskDialog *custom = 0;
    if ( dynamic_cast<Task *>(_node) ) {
	custom = new TaskDialog(this, wxT("Task Parameters"), const_cast<LQIO::DOM::Task&>(dynamic_cast<Task *>(_node)->getDOM()) );
    }
    if ( custom ) {
	custom->Show(true);
    }
}

void Canvas::openObject( wxCommandEvent& event )
{
    std::cerr << "Open object" << std::endl;
}

void Canvas::closeObject( wxCommandEvent& event )
{
    std::cerr << "Close object" << std::endl;
}

void Canvas::render( wxDC& dc ) 
{
    if ( _model ) _model->render( dc );

    // // draw some text
    // dc.DrawText(wxT("Testing"), 40, 60); 
    
    // // draw a circle
    // dc.SetBrush(*wxGREEN_BRUSH); // green filling
    // dc.SetPen( wxPen( wxColor(255,0,0), 5 ) ); // 5-pixels-thick red outline
    // dc.DrawCircle( wxPoint(200,100), 25 /* radius */ );
    
    // // draw a rectangle
    // dc.SetBrush(*wxBLUE_BRUSH); // blue filling
    // dc.SetPen( wxPen( wxColor(255,175,175), 10 ) ); // 10-pixels-thick pink outline
    // dc.DrawRectangle( 300, 100, 400, 200 );
    
    // // draw a line
    // dc.SetPen( wxPen( wxColor(0,0,0), 3 ) ); // black line, 3 pixels thick
    // dc.DrawLine( 300, 100, 700, 300 ); // draw line across the rectangle
    
    // Look at the wxDC docs to learn how to draw other stuff
}
