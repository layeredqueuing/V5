// -*- c++ -*-
//  dialog.h
//  lqneditor
//
//  Created by Greg Franks on 2012-11-22.
//  Copyright (c) 2012 Real Time and Distrubuted Systems Group. All rights reserved.
//

#ifndef __lqneditor__dialog__
#define __lqneditor__dialog__

#include <wx/wx.h>

namespace LQIO {
    namespace DOM {
	class Task;
    }
}

class TaskDialog : public wxDialog
{
public:
    TaskDialog( wxWindow * parent, const wxString& title, LQIO::DOM::Task& task );

    void onCancel(wxCommandEvent& event);
    void onOk(wxCommandEvent& event);

private:
    LQIO::DOM::Task& _task;
    wxWindow * _parent;
    
    wxTextCtrl * _name;
    wxTextCtrl * _multiplicity;
    wxTextCtrl * _think_time;
    
    DECLARE_EVENT_TABLE();
};

#endif /* defined(__lqneditor__dialog__) */
