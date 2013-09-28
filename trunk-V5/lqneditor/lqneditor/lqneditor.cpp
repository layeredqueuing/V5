//
//  lqneditor.cpp
//  lqneditor
//
//  Created by Greg Franks on 2012-10-25.
//  Copyright (c) 2012 Real Time and Distrubuted Systems Group. All rights reserved.
//

#include <iostream>
#include <wx/cmdline.h>
#include "lqneditor.h"
#include "frame.h"
#include "canvas.h"
#include "model.h"

/* Big hack.. in wxCmdLineEntryDesc, one version uses wxChar, the other uses char.  Bahh. */
#if wxMAJOR_VERSION >= 2 && wxMINOR_VERSION >= 9 
#define XwxT(x) (x)
#else
#define XwxT(x)	wxT(x)
#endif

static const wxCmdLineEntryDesc g_cmdLineDesc [] =
{
    { wxCMD_LINE_SWITCH, XwxT("h"), XwxT("help"), XwxT("displays help on the command line parameters."),
          wxCMD_LINE_VAL_NONE, wxCMD_LINE_OPTION_HELP },
//     { wxCMD_LINE_SWITCH, wxT("t"), wxT("test"), wxT("test switch"),
//          wxCMD_LINE_VAL_NONE, wxCMD_LINE_OPTION_MANDATORY  },
//     { wxCMD_LINE_SWITCH, wxT("s"), wxT("silent"), wxT("disables the GUI") },
 
     { wxCMD_LINE_NONE }
};
 
IMPLEMENT_APP(LqnEditApp)

bool LqnEditApp::OnInit()
{
#ifdef __WXMAC__
    ProcessSerialNumber PSN;
    GetCurrentProcess(&PSN);
    TransformProcessType(&PSN,kProcessTransformToForegroundApplication);
#endif
    if ( !wxApp::OnInit() ) {
        return false;
    }

    Model::init_errmsg();
    wxBoxSizer * sizer = new wxBoxSizer(wxHORIZONTAL);

    _frame = new EditorFrame( wxT("LQNEditor"), wxPoint(50,50), wxSize(DEFAULT_WINDOW_WIDTH,DEFAULT_WINDOW_HEIGHT) );
    _canvas = new Canvas( _frame );
    sizer->Add( _canvas, 1, wxEXPAND );

    _frame->SetCanvas( _canvas );
    _frame->SetSizer( sizer );
    _frame->SetAutoLayout( true );
    _frame->Show( true );
    SetTopWindow( _frame );
    return true;
}


void LqnEditApp::OnInitCmdLine(wxCmdLineParser& parser)
{
    parser.SetDesc (g_cmdLineDesc);
    // must refuse '/' as parameter starter or cannot use "/path" style paths
    parser.SetSwitchChars (wxT("-"));
}
 
bool LqnEditApp::OnCmdLineParsed(wxCmdLineParser& parser)
{
    if ( parser.Found(wxT("h")) ) {
	std::cerr << "Help!" << std::endl;
    }
 
    // // to get at your unnamed parameters use
    // wxArrayString files;
    // for (int i = 0; i < parser.GetParamCount(); i++)
    // {
    //         files.Add(parser.GetParam(i));
    // }
 
    // // and other command line parameters
 
    // // then do what you need with them.
 
    return true;
}
