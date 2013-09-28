// -*- c++ -*-
//  arc.h
//  lqneditor
//
//  Created by Greg Franks on 2012-11-13.
//  Copyright (c) 2012 Real Time and Distrubuted Systems Group. All rights reserved.
//

#ifndef __lqneditor__arc__
#define __lqneditor__arc__

#include <stdexcept>
#include <vector>
#include <wx/gdicmn.h>

class Model;
class Processor;
class Entry;
class Call;

class Arc {
private:
    Arc( const Arc& );
    Arc& operator=( const Arc& );

public:
    Arc( const Model& model );
    virtual ~Arc();

    wxPoint& src();
    wxPoint& dst();
    const wxPoint& src() const;
    const wxPoint& dst() const;

    virtual const wxColour getColour() const;

    void render( wxDC& dc ) const;

    static wxPoint intersection( const wxPoint& src_1, const wxPoint& dst_1, const wxPoint& src_2, const wxPoint& dst_2 ) throw( std::out_of_range );
    static wxPoint intersectionCircle( const wxPoint& src, const wxPoint& dst, const wxPoint& center, const double radius ) throw( std::out_of_range );
    static void sortArcs( std::vector<Arc *> &arcs );		/* from phase to dst entry... 	*/

private:

    void arrowHead( wxDC& dc, const wxPoint& src, const wxPoint& dst ) const;

protected:
    const Model& _model;

private:
    std::vector<wxPoint> _points;
};


class ArcForEntry : public Arc {
public:
    struct eqDestination {
	eqDestination( const Entry * entry ) : _entry(entry) {}
	bool operator()( const ArcForEntry * ) const;

    private:
	const Entry * _entry;
    };

public:
    ArcForEntry( const Model& model );
    void addCall( const unsigned int n, Call * call );
    
    virtual const wxColour getColour() const;

private:
    std::vector<Call *> _calls;
};

class ArcForProcessor : public Arc {
public:
    ArcForProcessor( const Model& model, const Processor * processor );
//    void addCall( const Processor* processor );
    
    virtual const wxColour getColour() const;

private:
    const Processor * _processor;
};
#endif /* defined(__lqneditor__arc__) */
