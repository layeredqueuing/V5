//
//  processor.cpp
//  lqneditor
//
//  Created by Greg Franks on 2012-11-06.
//  Copyright (c) 2012 Real Time and Distrubuted Systems Group. All rights reserved.
//

#include <wx/font.h>
#include "model.h"
#include "processor.h"
#include "arc.h"
#include <lqio/dom_processor.h>


Processor::Processor( const LQIO::DOM::Processor& processor, Model& model ) 
    : Node(model), _processor( processor )
{
    _extent.x = Model::DEFAULT_ICON_WIDTH;
    _extent.y = Model::DEFAULT_ICON_HEIGHT;
}

const std::string& Processor::getName() const
{
    return _processor.getName();
}


bool Processor::isMultiServer() const
{
    const LQIO::DOM::ExternalVariable * m = _processor.getCopies();
    double v;
    return !m->wasSet() || !m->getValue(v) || v > 1.0;
}

bool Processor::isInfinite() const
{
    return _processor.getSchedulingType() == SCHEDULE_DELAY;
}


void Processor::addAsDestination( ArcForProcessor * arc )
{
    _dst_arcs.push_back( arc );
}

double Processor::utilization() const
{
    const LQIO::DOM::ExternalVariable * copies = _processor.getCopies();
    double value;
    if ( copies->wasSet() && copies->getValue(value) ) {
	return _processor.getResultUtilization() / value;
    } else {
	return 0.;
    }
}

Node& Processor::moveTo( const wxPoint& origin )
{
    /* Move self */
    Node::moveTo( origin );

    /* Move the destinations of the arcs */
    const double radius = _extent.y / 2;
    const wxPoint center( _origin.x + _extent.x / 2, _origin.y + radius  );
    for ( std::vector<ArcForProcessor *>::const_iterator next_arc = _dst_arcs.begin(); next_arc != _dst_arcs.end(); ++next_arc ) {
	ArcForProcessor& dst_arc = **next_arc;
	dst_arc.dst() = Arc::intersectionCircle( dst_arc.src(), center, center, radius );
    }
    return *this;
}

void Processor::render( wxDC& dc ) const
{
    // draw a circle
    dc.SetBrush(*wxWHITE_BRUSH); // white filling
    const wxColor &colour = getColour();
    dc.SetPen( wxPen( colour, Model::TWIPS ) ); // 5-pixels-thick red outline
    const unsigned int radius = _extent.y/2;
    wxPoint center(_origin.x+_extent.x/2,_origin.y+radius);
    dc.DrawCircle( center, radius );

    wxString s( _processor.getName().c_str(), wxConvLibc );	// 2.9 wxString s( _processor.getName() );
    wxFont font( Model::DEFAULT_FONT_SIZE, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false );
    dc.SetFont(font);
    wxSize size = dc.GetTextExtent(s);
    dc.DrawText( s, center.x-size.GetWidth()/2, center.y-size.GetHeight()/2 );
}
