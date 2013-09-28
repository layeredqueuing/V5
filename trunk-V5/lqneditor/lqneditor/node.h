// -*- c++ -*-
//  node.h
//  lqneditor
//
//  Created by Greg Franks on 2012-11-06.
//  Copyright (c) 2012 Real Time and Distrubuted Systems Group. All rights reserved.
//

#ifndef __lqneditor__node__
#define __lqneditor__node__

#include <wx/gdicmn.h>
#include <wx/colour.h>
#include <vector>

class Model;
class Arc;

class Node
{
    friend class Model::compareXPos;

public:
    struct containsPoint {
	containsPoint( const wxPoint& point );
	bool operator()( const Node * ) const;

    private:
	const wxPoint _point;
    };

public:
    Node( const Model& model );
    virtual ~Node();
    
    virtual const std::string& getName() const = 0;
    virtual const unsigned int focusPriority() const = 0;

    Node& setLayer( unsigned int layer );
    unsigned int getLayer() const { return _layer; }

    bool hasResults() const;
    virtual double utilization() const { return 0.; }

    virtual const wxColour getColour() const;

    virtual Node& moveTo( const wxPoint& );
    virtual Node& reorder() { return *this; }

protected:
    wxPoint _origin;
    wxSize  _extent;

    static const unsigned int __dx;

private:
    const Model& _model;
    mutable unsigned int _layer;
};

#endif /* defined(__lqneditor__node__) */
