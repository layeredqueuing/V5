//
//  entry.cpp
//  lqneditor
//
//  Created by Greg Franks on 2012-11-07.
//  Copyright (c) 2012 Real Time and Distrubuted Systems Group. All rights reserved.
//

#include <cassert>
#include "model.h"
#include "task.h"
#include "entry.h"
#include "phase.h"
#include "arc.h"
#include <lqio/dom_task.h>
#include <lqio/dom_entry.h>
#include <lqio/dom_phase.h>

Entry::Entry( const LQIO::DOM::Entry& entry, const Task& task, const Model& model ) 
    : Node(model), _phases(), _task(task), _entry(entry)
{
    for ( std::map<unsigned, LQIO::DOM::Phase*>::const_iterator next_phase = entry.getPhaseList().begin(); next_phase != entry.getPhaseList().end(); ++next_phase ) {
	_phases[next_phase->first] = new Phase( next_phase->first, *(next_phase->second), *this, model );
    }
    _extent.x = Model::DEFAULT_ENTRY_WIDTH;
    _extent.y = Model::DEFAULT_ENTRY_HEIGHT;
}

Entry::~Entry()
{
}

void Entry::connectCalls()
{
    for ( std::map<unsigned, Phase*>::const_iterator next_phase = _phases.begin(); next_phase != _phases.end(); ++next_phase ) {
	Phase * phase = next_phase->second;
	phase->connectCalls( _src_arcs );
    }
}

void Entry::addAsDestination( ArcForEntry * arc )
{
    _dst_arcs.push_back( arc );
}

void Entry::addAsSource( ArcForEntry * arc )
{
    _src_arcs.push_back( arc );
}

const std::string& Entry::getName() const
{
    return _entry.getName();
}


void Entry::findChildren( std::set<const Task *>& call_chain ) const throw( std::runtime_error ) 
{
    std::set<const Task *>::const_iterator item = call_chain.find( &_task );
    if ( item != call_chain.end() ) throw std::runtime_error( getName().c_str() );
    call_chain.insert( &_task );
    const_cast<Task&>(_task).setLayer( call_chain.size() );

    /* For all phases, do findChildren... */
    for ( std::map<unsigned, Phase*>::const_iterator next_phase = _phases.begin(); next_phase != _phases.end(); ++next_phase ) {
	next_phase->second->findChildren( call_chain );
    }
    call_chain.erase( &_task );
}


double Entry::utilization() const
{
    const LQIO::DOM::ExternalVariable * copies = _task.getDOM().getCopies();
    double value;
    if ( copies->wasSet() && copies->getValue(value) ) {
	return _entry.getResultUtilization() / value;
    } else {
	return 0.;
    }
}


Node& Entry::moveTo( const wxPoint& origin )
{
    Node::moveTo( origin );
    wxPoint point( origin.x, origin.y + _extent.y );
    double offset = static_cast<double>(_extent.x) / (_src_arcs.size() + 1.0);
    /* Move the sources of the arcs */
    for ( std::vector<ArcForEntry *>::const_iterator next_arc = _src_arcs.begin(); next_arc != _src_arcs.end(); ++next_arc ) {
	point.x += offset;
	wxPoint& src = (*next_arc)->src();
	src = point;
    }
    /* Move the destinations of the arcs */
    point.x = origin.x;
    point.y = origin.y;
    offset = static_cast<double>(_extent.x) / (_dst_arcs.size() + 1.0);
    for ( std::vector<ArcForEntry *>::const_iterator next_arc = _dst_arcs.begin(); next_arc != _dst_arcs.end(); ++next_arc ) {
	point.x += offset;
	wxPoint& dst = (*next_arc)->dst();
	dst = point;
    }
    return *this;
}


/*
 * change the order of the points to minimize arc intersection
 */

Node& Entry::reorder()
{
    unsigned int n = _src_arcs.size();
    if ( n > 1 ) {
	for ( unsigned i = 0; i < n - 1; ++i ) {
	    for ( unsigned j = i + 1; j < n; ++j ) {
		try {
		    wxPoint intersect = Arc::intersection( _src_arcs[i]->src(), _src_arcs[i]->dst(), _src_arcs[j]->src(), _src_arcs[j]->dst() );
		    wxPoint p1 = _src_arcs[i]->src();
		    wxPoint p2 = _src_arcs[j]->src();

		    ArcForEntry * temp = _src_arcs[i];		/* Swap arcs, but not their points. */
		    _src_arcs[i] = _src_arcs[j];
		    _src_arcs[j] = temp;

		    _src_arcs[i]->src() = p1;
		    _src_arcs[j]->src() = p2;
		} 
		catch ( std::out_of_range& e )
		{
		}
	    }
	}
    }

    n = _dst_arcs.size();
    if ( n > 1 ) {
	for ( unsigned i = 0; i < n - 1; ++i ) {
	    for ( unsigned j = i + 1; j < n; ++j ) {
		try {
		    wxPoint intersect = Arc::intersection( _dst_arcs[i]->src(), _dst_arcs[i]->dst(), _dst_arcs[j]->src(), _dst_arcs[j]->dst() );
		    wxPoint p1 = _dst_arcs[i]->dst();
		    wxPoint p2 = _dst_arcs[j]->dst();

		    ArcForEntry * temp = _dst_arcs[i];		/* Swap arcs, but not their points. */
		    _dst_arcs[i] = _dst_arcs[j];
		    _dst_arcs[j] = temp;

		    _dst_arcs[i]->dst() = p1;
		    _dst_arcs[j]->dst() = p2;
		} 
		catch ( std::out_of_range& e )
		{
		}
	    }
	}
    }
    return *this;
}

void Entry::render( wxDC& dc, bool draw_left, bool draw_right ) const
{
    dc.SetBrush(*wxWHITE_BRUSH); 		// white filling
    dc.SetPen( wxPen( getColour(), Model::TWIPS ) ); 	// 1-pixel-thick outline

    wxPoint points[4];
    unsigned int i = 0;
    if ( draw_right ) {
	points[i].x   = _extent.x;		/* top right  */
	points[i++].y = 0;
    }
    points[i].x   = _extent.x - 3*__dx;		/* Bottom right */
    points[i++].y = _extent.y;
    points[i].x   = 0;				/* bottom left */
    points[i++].y = _extent.y;
    if ( draw_left ) {
	points[i].x   = 3*__dx;			/* top left */
	points[i++].y = 0;
    }
    dc.DrawLines( i, points, _origin.x, _origin.y );
    
    wxString s( getName().c_str(), wxConvLibc );
    wxFont font( Model::DEFAULT_FONT_SIZE, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false );
    dc.SetFont(font);
    wxSize size = dc.GetTextExtent(s);
    dc.DrawText( s, _origin.x+(_extent.x-size.GetWidth())/2, _origin.y+(_extent.y-size.GetHeight())/2 );

    for ( std::vector<ArcForEntry *>::const_iterator next_arc = _src_arcs.begin(); next_arc != _src_arcs.end(); ++next_arc ) {
	(*next_arc)->render( dc );
    }
}
