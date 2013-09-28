//
//  frame.h
//  lqneditor
//
//  Created by Greg Franks on 2012-10-29.
//  Copyright (c) 2012 Real Time and Distrubuted Systems Group. All rights reserved.
//

#ifndef __lqneditor__frame__
#define __lqneditor__frame__

#include "lqneditor.h"
#include <wx/splitter.h>
#include <wx/dcclient.h>
#include "canvas.h"

#ifndef wxDECLARE_EVENT_TABLE
#define wxDECLARE_EVENT_TABLE DECLARE_EVENT_TABLE
#define wxBEGIN_EVENT_TABLE   BEGIN_EVENT_TABLE  
#define wxEND_EVENT_TABLE     END_EVENT_TABLE    
#define wxIMPLEMENT_APP       IMPLEMENT_APP       
#endif

class Model;


class EditorFrame: public wxFrame
{
public:
    EditorFrame(const wxString& title, const wxPoint& pos, const wxSize& size);

    void SetCanvas( Canvas * canvas ) { _canvas = canvas; }
    
private:
    void OnAbout(wxCommandEvent& event);
    void OnExit(wxCommandEvent& event);
    void OnNew(wxCommandEvent& event);
    void OnOpen(wxCommandEvent& event);
    void OnSave(wxCommandEvent& event);
    void OnSolveLqns(wxCommandEvent& event);
    void OnSolveLqsim(wxCommandEvent& event);

    void runSolver( const wxString& );
    
    wxSplitterWindow* _splitter;
    wxTextCtrl *_textCtrl;
//    wxClientDC * _canvas;
    wxMenuBar *_menuBar;
    Canvas * _canvas;
    
    wxDECLARE_EVENT_TABLE();

    wxString _input_file_name;
    Model * _model;
    bool _changed;
    wxArrayString _solver_output;
};


enum
{
    ID_New = 1,
    ID_Save = 2,
    ID_Open = 3,
    ID_Solve = 4,
    ID_LQNS = 5,
    ID_LQSIM = 6,
    ID_EditParameters = 7,
    ID_OpenObject = 8
};

#endif /* defined(__lqneditor__frame__) */
