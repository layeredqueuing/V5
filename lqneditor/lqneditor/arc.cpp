//
//  arc.cpp
//  lqneditor
//
//  Created by Greg Franks on 2012-11-13.
//  Copyright (c) 2012 Real Time and Distrubuted Systems Group. All rights reserved.
//

#include <cassert>
#include <wx/wx.h>
#include "model.h"
#include "arc.h"
#include "call.h"
#include "entry.h"
#include "processor.h"

static double square( const double x ) { return x * x; }

Arc::Arc( const Model& model )
    : _model( model )
{
    _points.resize(2,wxPoint(0,0));
}

Arc::~Arc()
{
}

wxPoint& Arc::src()
{
    return _points.front();
}

wxPoint& Arc::dst()
{
    return _points.back();
}

const wxPoint& Arc::src() const
{
    return _points.front();
}

const wxPoint& Arc::dst() const
{
    return _points.back();
}


const wxColour Arc::getColour() const
{
    return *wxBLACK;
}

void Arc::render( wxDC& dc ) const
{
    dc.SetPen( wxPen( getColour(), Model::TWIPS ) );
    dc.DrawLine( src(), dst() );
    arrowHead( dc, src(), dst() );
}


void Arc::arrowHead( wxDC& dc, const wxPoint& src, const wxPoint& dst ) const
{
    const double theta = atan2( static_cast<double>(dst.y - src.y), static_cast<double>(dst.x - src.x) );
    const double sinTheta = sin( theta );
    const double cosTheta = cos( theta );
    const unsigned n_points = 4;

    wxPoint arrow[n_points];
    arrow[0].x = 0;  arrow[0].y = 0;
    arrow[1].x = -6*Model::TWIPS; arrow[1].y = -2*Model::TWIPS;
    arrow[2].x = -5*Model::TWIPS; arrow[2].y =  0*Model::TWIPS;
    arrow[3].x = -6*Model::TWIPS; arrow[3].y =  2*Model::TWIPS;
    wxPoint origin( 0, 0 );
    for ( unsigned i = 0; i < n_points; ++i ) {
	const double x = arrow[i].x * 1.5;
	const double y = arrow[i].y * 1.5;
	arrow[i].x = static_cast<int>(x * cosTheta - y * sinTheta + 0.5);
	arrow[i].y = static_cast<int>(x * sinTheta + y * cosTheta + 0.5);
    }
    dc.SetBrush( wxBrush( getColour() ) ); 		// white filling
    dc.DrawPolygon( n_points, arrow, dst.x, dst.y );
}

wxPoint Arc::intersection( const wxPoint& src_1, const wxPoint& dst_1, const wxPoint& src_2, const wxPoint& dst_2 ) throw( std::out_of_range )
{
    const double bx = dst_1.x - src_1.x; 
    const double by = dst_1.y - src_1.y; 
    const double dx = dst_2.x - src_2.x; 
    const double dy = dst_2.y - src_2.y;
    const double b_dot_d_perp = bx * dy - by * dx;
    if ( b_dot_d_perp == 0 ) throw std::out_of_range( "intersection" );	/* Parallel */

    const double cx = src_2.x - src_1.x;
    const double cy = src_2.y - src_1.y;
    const double t = (cx * dy - cy * dx) / b_dot_d_perp;
    if (t < 0 || t > 1 ) throw std::out_of_range( "intersection" );
    const double u = (cx * by - cy * bx) / b_dot_d_perp;
    if (u < 0 || u > 1 ) throw std::out_of_range( "intersection" );
    return wxPoint( static_cast<int>(src_1.x+t*bx), static_cast<int>(src_1.y+t*by));
}

wxPoint Arc::intersectionCircle( const wxPoint& src, const wxPoint& dst, const wxPoint& center, const double radius ) throw( std::out_of_range )
{
    const double dx = dst.x - src.x;
    const double dy = dst.y - src.y;

    const double a = square( dx ) + square( dy );
    const double b = 2.0 * ( ( dx * (src.x - center.x))
			   + ( dy * (src.y - center.y)) );
    const double c = square( center.x ) + square( center.y )
	+ square( src.x ) + square( src.y ) 
	- 2.0 * ( center.x * src.x + center.y * src.y )
	- square( radius );
    
    const double temp = square( b ) - 4.0 * a * c;
    if ( temp < 0 ) throw std::out_of_range( "intersectsCircle" );
    
    double mu1 = (-b + sqrt( temp )) / (2.0 * a);
    double mu2 = (-b - sqrt( temp )) / (2.0 * a);
    double ty1 = src.y + mu1 * dy;
    double ty2 = src.y + mu2 * dy;
    if ( (src.y >= ty1 && ty1 >= dst.y)	|| (dst.y >= ty1 && ty1 >= src.y) ) {
	return wxPoint( static_cast<int>(src.x + mu1 * dx), static_cast<int>(ty1) );
    } else {
	return wxPoint( static_cast<int>(src.x + mu2 * dx), static_cast<int>(ty2) );
    }
}

void Arc::sortArcs( std::vector<Arc *> &arcs )
{
    const unsigned int n = arcs.size();
    if ( n <= 1 ) return;
    for ( unsigned i = 0; i < n - 1; ++i ) {
	for ( unsigned j = i + 1; j < n; ++j ) {
	    try {
		wxPoint intersect = Arc::intersection( arcs[i]->src(), arcs[i]->dst(), arcs[j]->src(), arcs[j]->dst() );
		Arc * temp = arcs[i];
		arcs[i] = arcs[j];
		arcs[j] = temp;
	    } 
	    catch ( std::out_of_range& e )
	    {
	    }
	}
    }
}

ArcForEntry::ArcForEntry( const Model& model )
    : Arc( model )
{
    _calls.resize(3,0);		/* Three phases. */
}

void ArcForEntry::addCall( const unsigned int p, Call * call )
{
    assert( 0 < p && p <= 3 );
    _calls[p-1] = call;
}

const wxColour ArcForEntry::getColour() const
{
    for ( std::vector<Call *>::const_iterator next_call = _calls.begin(); next_call != _calls.end(); ++next_call ) {
	const Call * call = *next_call;
	if ( call ) return call->getDestination()->getColour();
    }
    return *wxBLACK;
}

bool ArcForEntry::eqDestination::operator()( const ArcForEntry * arc ) const
{
    /* Search through calls to see if we match destination */
    for ( std::vector<Call *>::const_iterator next_call = arc->_calls.begin(); next_call != arc->_calls.end(); ++next_call ) {
	const Call * call = *next_call;
	if ( call && call->getDestination() == _entry ) return true;
    }
    return false;
}

ArcForProcessor::ArcForProcessor( const Model& model, const Processor * processor ) 
    : Arc( model ), _processor( processor )
{
}


const wxColour ArcForProcessor::getColour() const
{
    return _processor->getColour();
}
