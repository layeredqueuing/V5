//
//  node.cpp
//  lqneditor
//
//  Created by Greg Franks on 2012-11-06.
//  Copyright (c) 2012 Real Time and Distrubuted Systems Group. All rights reserved.
//

#include "model.h"
#include "node.h"

const unsigned int Node::__dx = Model::TWIPS;	/* rise=9, run=1 for slope */

Node::Node( const Model& model ) 
    : _origin(0,0), _extent(0,0), _model(model), _layer(0)
{
    model._nodes.insert(this);
}


Node::~Node()
{
    _model._nodes.erase(this);
}


Node& Node::setLayer( unsigned int layer )
{
    if ( layer > _layer ) _layer = layer;
    return *this;
}


bool Node::hasResults() const 
{ 
    return _model.hasResults(); 
}

const wxColour Node::getColour() const
{
    const double u = utilization();
    if ( u < 0.4 ) {
	return *wxBLACK;
    } else if ( u < 0.5 ) {
	return wxColour(0,0,255);
    } else if ( u < 0.6 ) {
	return wxColour(0,255,0);
    } else if ( u < 0.8 ) {
	return wxColour(255,165,0);
    } else {
	return wxColour(255,0,0);
    }
}


Node& Node::moveTo( const wxPoint& point )
{
    _origin = point;
    return *this;
}

Node::containsPoint::containsPoint( const wxPoint& point )
    : _point( point.x * Model::TWIPS, point.y * Model::TWIPS )
{
}

bool Node::containsPoint::operator()( const Node * node ) const
{
    return node->_origin.x <= _point.x && _point.x <= node->_origin.x + node->_extent.x 
	&& node->_origin.y <= _point.y && _point.y <= node->_origin.y + node->_extent.y; 
}
