// -*- c++ -*-
//  lqneditor.h
//  lqneditor
//
//  Created by Greg Franks on 2012-10-25.
//  Copyright (c) 2012 Real Time and Distrubuted Systems Group. All rights reserved.
//

#ifndef __lqneditor__lqneditor__
#define __lqneditor__lqneditor__

#include <wx/wxprec.h>
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#ifdef __WXMAC__
#include <ApplicationServices/ApplicationServices.h>
#endif

class Canvas;
class EditorFrame;

#define DEFAULT_WINDOW_WIDTH	450
#define DEFAULT_WINDOW_HEIGHT	340

class LqnEditApp: public wxApp
{
public:
    virtual bool OnInit();
    virtual void OnInitCmdLine(wxCmdLineParser& parser);
    virtual bool OnCmdLineParsed(wxCmdLineParser& parser);

private:
    EditorFrame * _frame;
    Canvas * _canvas;
};

DECLARE_APP(LqnEditApp)

#endif /* defined(__lqneditor__lqneditor__) */
