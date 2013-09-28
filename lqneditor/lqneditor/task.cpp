//
//  task.cpp
//  lqneditor
//
//  Created by Greg Franks on 2012-11-06.
//  Copyright (c) 2012 Real Time and Distrubuted Systems Group. All rights reserved.
//

#include <algorithm>
#include <wx/font.h>
#include "model.h"
#include "processor.h"
#include "task.h"
#include "entry.h"
#include "arc.h"
#include "label.h"
#include <lqio/dom_task.h>

Task::Task( const LQIO::DOM::Task& task, const Processor * processor, Model& model )
    : Node( model ), _entries(), _task( task ), _processor( processor ), _processor_call(0)
{
    _processor_call = new ArcForProcessor( model, processor );
    const_cast<Processor *>(processor)->addAsDestination( _processor_call );

    for ( std::vector<LQIO::DOM::Entry*>::const_iterator next_entry = task.getEntryList().begin(); next_entry != task.getEntryList().end(); ++next_entry ) {
	LQIO::DOM::Entry * dom_entry = *next_entry;
	_entries.push_back( model.addEntry( new Entry( *dom_entry, *this, model ) ) );
    }

    _label = new TaskLabel( task, model );

    const unsigned int width = _entries.size() * Model::DEFAULT_ENTRY_WIDTH;
    _extent.x = width > Model::DEFAULT_ICON_WIDTH ? width : Model::DEFAULT_ICON_WIDTH;
    _extent.y = Model::DEFAULT_ICON_HEIGHT;
}

Task::~Task()
{
    delete _processor_call;
    for ( std::vector<Entry *>::const_iterator next_entry = _entries.begin(); next_entry != _entries.end(); ++next_entry ) {
	Entry * entry = *next_entry;
	delete entry;
    }
}

void Task::connectCalls() 
{
    for ( std::vector<Entry *>::const_iterator next_entry = _entries.begin(); next_entry != _entries.end(); ++next_entry ) {
	(*next_entry)->connectCalls();
    }
}


const std::string& Task::getName() const
{
    return _task.getName();
}


bool Task::isReferenceTask() const
{
    switch ( _task.getSchedulingType() ) {
    case SCHEDULE_UNIFORM:
    case SCHEDULE_BURST:
    case SCHEDULE_CUSTOMER:
	return true;
    default:
	return false;
    }
}

bool Task::isMultiServer() const
{
    const LQIO::DOM::ExternalVariable * m = _task.getCopies();
    double v;
    return !m->wasSet() || !m->getValue(v) || v > 1.0;
}

bool Task::isInfinite() const
{
    return _task.getSchedulingType() == SCHEDULE_DELAY;
}


double Task::utilization() const
{
    const LQIO::DOM::ExternalVariable * copies = _task.getCopies();
    double value;
    if ( copies->wasSet() && copies->getValue(value) ) {
	return _task.getResultUtilization() / value;
    } else {
	return 0.;
    }
}

Node& Task::moveTo( const wxPoint& origin )
{
    /* Move self */
    Node::moveTo( origin );

    /* Mode entries */

    wxPoint point( origin.x + 5*__dx, origin.y );
    double fill = _entries.size() > 1 ? -3*__dx : static_cast<double>(_extent.x - 7.5*__dx - Model::DEFAULT_ENTRY_WIDTH)/static_cast<double>(_entries.size() + 1);
    point.x += static_cast<unsigned int>(fill);
    for ( std::vector<Entry *>::const_iterator next_entry = _entries.begin(); next_entry != _entries.end(); ++next_entry ) {
	(*next_entry)->moveTo( point );
	point.x += Model::DEFAULT_ENTRY_WIDTH - 3*__dx;
    }

    /* Center the label */

    point.x = origin.x + _extent.x/2;
    point.y = origin.y + _extent.y*4/5;
    _label->moveTo( point );

    /* Move arc to processor */

    _processor_call->src().x = origin.x + _extent.x/2;
    _processor_call->src().y = origin.y + _extent.y;
    return *this;
}


Node& Task::reorder()
{
    for ( std::vector<Entry *>::const_iterator next_entry = _entries.begin(); next_entry != _entries.end(); ++next_entry ) {
	(*next_entry)->reorder();
    }
    return *this;
}


void Task::render( wxDC& dc ) const
{
    dc.SetBrush(*wxWHITE_BRUSH); 		// white filling
    const wxColor &colour = getColour();
    dc.SetPen( wxPen( colour, Model::TWIPS ) ); 		// 1-pixel-thick outline

    wxPoint points[4];
    points[0].x = 0 + 5*__dx;
    points[0].y = 0;
    points[1].x = 0 + _extent.x;
    points[1].y = 0;
    points[2].x = 0 + _extent.x - 5*__dx;
    points[2].y = 0 + _extent.y;
    points[3].x = 0;
    points[3].y = 0 + _extent.y;
    dc.DrawPolygon( 4, points, _origin.x, _origin.y );

    /* Draw the label */

    _label->render( dc );

    /* Draw the entries. */

    const unsigned int n = _entries.size();
    for ( std::vector<Entry *>::const_iterator next_entry = _entries.begin(); next_entry != _entries.end(); ++next_entry ) {
	Entry * entry = *next_entry;
	const bool draw_left = n <= 1;
	const bool draw_right = n <= 1 || entry != _entries.back();
	entry->render( dc, draw_left, draw_right );
    }

    /* Draw the call to the processor */

    _processor_call->render( dc );
}
