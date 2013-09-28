//
//  label.cpp
//  lqneditor
//
//  Created by Greg Franks on 2012-11-19.
//  Copyright (c) 2012 Real Time and Distrubuted Systems Group. All rights reserved.
//

#include "model.h"
#include "label.h"

Label::Label( Model& model )
    : Node(model), _center(0,0), _font(Model::DEFAULT_FONT_SIZE, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false)
{
}

Label::~Label()
{
}

Node& Label::moveTo( const wxPoint& center )
{
    _center = center;
    return *this;
}

TaskLabel::TaskLabel( const LQIO::DOM::Task& task, Model& model )
    : Label(model), _task(task)
{
}

TaskLabel::~TaskLabel()
{
}

const std::string& TaskLabel::getName() const
{
    return _task.getName();
}

void TaskLabel::render( wxDC& dc ) const
{
    wxString s1( _task.getName().c_str(), wxConvLibc );
    const LQIO::DOM::ExternalVariable * copies = _task.getCopies();
    double result;
    if ( dynamic_cast<const LQIO::DOM::ConstantExternalVariable *>(copies) && copies->getValue( result ) && result != 1 ) {
	s1 << wxT(" {");
	s1 << result;
	s1 << wxT("}");
    } else if ( copies ) {
	s1 << wxT(" {");
	s1 << wxT("}");
    }

    dc.SetFont(_font);
    wxSize size1 = dc.GetTextExtent(s1);
    const_cast<TaskLabel*>(this)->_extent.x = size1.GetWidth();
    const_cast<TaskLabel*>(this)->_extent.y = size1.GetHeight();
    const_cast<TaskLabel*>(this)->_origin.x = _center.x - _extent.x/2;
    const_cast<TaskLabel*>(this)->_origin.y = _center.y - _extent.y/2;
    dc.DrawText( s1, _origin );
}
