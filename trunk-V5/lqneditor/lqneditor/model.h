// -*- c++ -*-
//  model.h
//  lqneditor
//
//  Created by Greg Franks on 2012-10-31.
//  Copyright (c) 2012 Real Time and Distrubuted Systems Group. All rights reserved.
//

#ifndef __lqneditor__model__
#define __lqneditor__model__

#include <wx/wx.h>
#include <string>
#include <set>
#include <lqio/dom_document.h>


class Node;
class Processor;
class Task;
class Entry;

class Model
{
    friend class Node;

    struct compareXPos {
	bool operator() ( const Node *, const Node * ) const;
    };

private:
    Model( const Model& );
    Model& operator=( const Model& );

public:
    Model( const std::string& );
    ~Model();

    bool hasResults() const { return _document->hasResults(); }

    Entry * addEntry( Entry * entry );
    Entry * findEntry( const std::string& ) const;
    Node * findNode( const wxPoint& ) const;

    bool loadResults( const std::string& );
    void autoLayout();
    void render( wxDC& dc ) const;

    static void init_errmsg();

    static lqio_params_stats __io_vars;

    static const unsigned int TWIPS;
    static const unsigned int DEFAULT_FONT_SIZE;
    static const unsigned int DEFAULT_ICON_HEIGHT;
    static const unsigned int DEFAULT_ICON_WIDTH;
    static const unsigned int DEFAULT_Y_SPACING;
    static const unsigned int DEFAULT_ENTRY_HEIGHT;
    static const unsigned int DEFAULT_ENTRY_WIDTH;

private:
    void connectCalls(); 
    void topologicalSort();
    void layerize();

    static void severity_action (unsigned int severity);

    LQIO::DOM::Document * _document;
    mutable std::multiset<Node *,compareXPos> _nodes;	/* Ordered by x position for finding objects */
    std::set<Processor*> _processors;
    std::set<Task*> _tasks;
    std::map<std::string,Entry *> _entries;
};

#endif /* defined(__lqneditor__model__) */
