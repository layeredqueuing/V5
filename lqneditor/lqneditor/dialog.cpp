//
//  dialog.cpp
//  lqneditor
//
//  Created by Greg Franks on 2012-11-22.
//  Copyright (c) 2012 Real Time and Distrubuted Systems Group. All rights reserved.
//

#include <iostream>
#include "dialog.h"
#include "model.h"
#include <lqio/dom_task.h>

BEGIN_EVENT_TABLE(TaskDialog, wxDialog)
EVT_BUTTON(wxID_OK,  TaskDialog::onOk)
EVT_BUTTON(wxID_CANCEL,  TaskDialog::onCancel)
//EVT_CLOSE( TaskDialog::onClose )
END_EVENT_TABLE()

TaskDialog::TaskDialog( wxWindow * parent, const wxString& title, LQIO::DOM::Task& task )
    : wxDialog( NULL, -1, title, wxDefaultPosition, wxSize(250,140)), _task(task), _parent(parent),
    _name(0), _multiplicity(0), _think_time(0)
{
    wxPanel *panel = new wxPanel(this, -1);

    wxBoxSizer *vbox = new wxBoxSizer(wxVERTICAL);
    wxBoxSizer *hbox = new wxBoxSizer(wxHORIZONTAL);

    wxStaticText * task_name = new wxStaticText( panel, -1, wxT("Task Name"), wxPoint(5, 5));
    wxString name( task.getName().c_str(), wxConvLibc );
    _name = new wxTextCtrl(panel, -1, name,  wxPoint(95, 5), wxSize(100,20));

    wxStaticText * multiplicity = new wxStaticText( panel, -1, wxT("Multiplicity"), wxPoint(5, 25));
    _multiplicity = new wxTextCtrl(panel, -1, wxT("0"),  wxPoint(95, 25), wxSize(50,20));

    wxStaticText * think_time = new wxStaticText( panel, -1, wxT("Think Time"), wxPoint(5, 45));
    _think_time = new wxTextCtrl(panel, -1, wxT("0"),  wxPoint(95, 45), wxSize(50,20));

//    wxDialog::CreateStdDialogButtonSizer( wxOK | wxCANCEL );
    wxButton *okButton    = new wxButton(this, wxID_OK, wxT("Ok"),    wxDefaultPosition, wxSize(70, 30));
    wxButton *closeButton = new wxButton(this, wxID_CANCEL, wxT("Cancel"), wxDefaultPosition, wxSize(70, 30));

    hbox->Add(okButton, 1);
    hbox->Add(closeButton, 1, wxLEFT, 5);

    vbox->Add(panel, 1);
    vbox->Add(hbox, 0, wxALIGN_CENTER | wxTOP | wxBOTTOM, 10);

    SetSizer(vbox);

    Centre();
    ShowModal();

    Destroy(); 
}


void TaskDialog::onOk(wxCommandEvent& event) 
{
    std::string new_name( _name->GetLineText( 0 ).mb_str() );

    if ( new_name != _task.getName() ) {
	std::cerr << "Name changed from " << _task.getName() << " to " << new_name << std::endl;
	_task.setName( new_name );
	_parent->Refresh();
    }

    wxDialog::EndModal( wxID_CANCEL );
}

void TaskDialog::onCancel(wxCommandEvent& event) 
{
    std::cerr << "TaskDialog::OnClose" << std::endl;
    wxDialog::EndModal( wxID_CANCEL );
}
