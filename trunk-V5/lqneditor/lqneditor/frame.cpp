//
//  frame.cpp
//  lqneditor
//
//  Created by Greg Franks on 2012-10-29.
//  Copyright (c) 2012 Real Time and Distrubuted Systems Group. All rights reserved.
//

#include "frame.h"
#include <lqio/filename.h>
#include <stdexcept>
#include <wx/filefn.h> 
#include <wx/textctrl.h> 
#include "model.h"

wxBEGIN_EVENT_TABLE(EditorFrame, wxFrame)
EVT_MENU(ID_New,     EditorFrame::OnNew)
EVT_MENU(ID_Open,    EditorFrame::OnOpen)
EVT_MENU(ID_Save,    EditorFrame::OnSave)
EVT_MENU(ID_LQNS,    EditorFrame::OnSolveLqns)
EVT_MENU(ID_LQSIM,   EditorFrame::OnSolveLqsim)
EVT_MENU(wxID_EXIT,  EditorFrame::OnExit)
EVT_MENU(wxID_ABOUT, EditorFrame::OnAbout)
wxEND_EVENT_TABLE()

EditorFrame::EditorFrame(const wxString& title, const wxPoint& pos, const wxSize& size)
    : wxFrame(NULL, wxID_ANY, title, pos, size),
      _splitter(0), _textCtrl(0), /* _canvas(0), */ _menuBar(0), _input_file_name(), _model(0), _changed(false)
{
    wxMenu *menuFile = new wxMenu;
    menuFile->Append(ID_New, _T("&New...\tCtrl-H"),
                     _T("Help string shown in status bar for this menu item"));
    menuFile->Append(ID_Open,  _T("&Open\tCtrl-O"), _T("Open an existing model."));
    menuFile->AppendSeparator();
    menuFile->Append(ID_Save, _T("&Save\tCtrl-S"), _T("Save the model."));
    menuFile->Enable(ID_Save, false);

    wxMenu * menuSolve = new wxMenu;
    menuSolve->Append(ID_LQNS, _T("&Lqns"), _T("Solve analytically with lqns"));
    menuSolve->Append(ID_LQSIM, _T("&Lqsim"), _T("Solve using simulation with lqsim"));
    menuSolve->Enable(ID_LQNS, false );
    menuSolve->Enable(ID_LQSIM, false );
    menuFile->AppendSubMenu(menuSolve, _T("S&olve"), _T("Solve the model."));

    menuFile->Append(wxID_EXIT);
    
    wxMenu *menuHelp = new wxMenu;
    menuHelp->Append(wxID_ABOUT);
    
    _menuBar = new wxMenuBar;
    _menuBar->Append( menuFile, _T("&File") );
    _menuBar->Append( menuHelp, _T("&Help") );
    
    SetMenuBar( _menuBar );

//    _splitter = new wxSplitterWindow( this );
//    _canvas   = new wxClientDC( _splitter );
    _textCtrl = new wxTextCtrl( this, wxID_ANY, _T(""), wxPoint(0,0), wxSize(DEFAULT_WINDOW_WIDTH,DEFAULT_WINDOW_HEIGHT), wxTE_READONLY|wxTE_MULTILINE );		/* Appears in frame */
    
    CreateStatusBar();
    SetStatusText( _T("Welcome to lqnedit!") );
}

void EditorFrame::OnExit(wxCommandEvent& event)
{
    Close( true );
}

void EditorFrame::OnAbout(wxCommandEvent& event)
{
    wxMessageBox( _T("This is a wxWidgets' Hello world sample"),
                 _T("About Hello World"), wxOK | wxICON_INFORMATION );
}

void EditorFrame::OnNew(wxCommandEvent& event)
{
    wxLogMessage(_T("New world from wxWidgets!"));
}

void EditorFrame::OnOpen(wxCommandEvent& event)
{
    /* Do we already have a model, and has it been modified? */
    wxFileDialog dialog( this, 				/* Parent window */
			 wxEmptyString, 		/* Message for dialog */
			 wxGetCwd(),			/* Default directory */
			 wxEmptyString,			/* Default filename */
			 _T("Layered Queueing Network model files (*.lqnx;*.lqxo,*.xlqn;*.lqn;*.in)|*.lqnx;lqxo;*.xlqn;*.lqn;*.in") );
    dialog.CenterOnParent();
    if ( dialog.ShowModal() == wxID_OK ) {
	if ( _model ) {
	    delete _model;
	    _model = 0;
	    _menuBar->Enable(ID_LQNS, false );
	    _menuBar->Enable(ID_LQSIM, false );
	}

	wxString message;
	_input_file_name = dialog.GetPath();
	std::string input_file_name( _input_file_name.mb_str() );
	try { 
	    _model = new Model( input_file_name );
	    _model->autoLayout();
	    _menuBar->Enable(ID_LQNS, true );
	    _menuBar->Enable(ID_LQSIM, true );
	    message = _T( "Loaded " );
	}
	catch ( std::invalid_argument &error ) {
	    message = _T( "Cannot load ");
	}
	message << _input_file_name;
	SetStatusText( message );
	_canvas->setModel( _model );
	_canvas->Refresh();
    }
}

void EditorFrame::OnSave(wxCommandEvent& event)
{
    wxLogMessage(_T("Hello world from wxWidgets!"));
}

void EditorFrame::OnSolveLqns(wxCommandEvent& event)
{
    wxString command_line;
    command_line << _T("lqns -x ") << _input_file_name;		/* Force lqxo output */
    runSolver( command_line );
}

void EditorFrame::OnSolveLqsim(wxCommandEvent& event)
{
    wxString command_line;
    command_line << _T("lqsim -S123456 -C2.0 -x ") << _input_file_name;	/* Force lqxo output */
    runSolver( command_line );
}

void EditorFrame::runSolver( const wxString& command_line ) 
{
    SetStatusText( _T("Running...") );
    int rc = wxExecute( command_line, _solver_output, wxEXEC_SYNC );
    if ( rc < 0 ) {
	SetStatusText( _T("Fatal runtime error.") );
    } else if ( rc & 0x08 ) {
	SetStatusText( _T("File I/O problems.") );
    } else if ( rc & 0x04 ) {
	SetStatusText( _T("Invalid command line argument.") );
    } else if ( rc & 0x02 ) {
	SetStatusText( _T("Invalid model file.") );
    } else {
	if ( rc & 0x01 ) {
	    SetStatusText( _T("Model failed to converge.") );
	} else {
	    SetStatusText( _T("Done.") );
	}
	LQIO::Filename filename( _input_file_name.mb_str(), "lqxo" );		/* Get the base file name */
	if ( _model->loadResults( filename() ) ) {
	    _canvas->Refresh();
	}
    }

    const unsigned count = _solver_output.Count();
    wxString pattern = _input_file_name;
    pattern << _T(": ");
    for ( unsigned int i = 0; i < count; ++i ) {
	_solver_output[i].Replace( pattern, _T(""), false );
	_textCtrl->AppendText(_solver_output[i] );
	_textCtrl->AppendText( _T("\n") );
    }
}
