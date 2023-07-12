//
//  model.cpp
//  Our interface to the document.  We'll do the drawing bit (eventually).
//
//  Created by Greg Franks on 2012-10-31.
//  Copyright (c) 2012 Real Time and Distrubuted Systems Group. All rights reserved.
//

#include "lqneditor.h"
#include "model.h"
#include "processor.h"
#include "task.h"
#include "entry.h"
#include <stdexcept>
#include <algorithm>
#include <lqio/glblerr.h>
#include <lqio/dom_processor.h>

#if !defined(HAVE_CONFIG_H)
#define VERSION "5"
#endif

lqio_params_stats Model::__io_vars = 
{
    /* .n_processors =   */ 0,
    /* .n_tasks =	 */ 0,
    /* .n_entries =      */ 0,
    /* .n_groups =       */ 0,
    /* .lq_toolname =    */ NULL,
    /* .lq_version =	 */ VERSION,
    /* .lq_command_line =*/ NULL,
    /* .severity_action= */ &Model::severity_action,
    /* .max_error =      */ 0,
    /* .error_count =    */ 0,
    /* .severity_level = */ LQIO::FATAL_ERROR,		/* Suppress error reporting by expat parser */
    /* .error_messages = */ NULL,
    /* .anError =        */ 0
};

const unsigned int Model::TWIPS 		= 1;
const unsigned int Model::DEFAULT_FONT_SIZE	= 9 * TWIPS;
const unsigned int Model::DEFAULT_ICON_HEIGHT	= 45 * TWIPS;
const unsigned int Model::DEFAULT_ICON_WIDTH	= DEFAULT_ICON_HEIGHT * 1.6;	/* 72 */
const unsigned int Model::DEFAULT_Y_SPACING	= DEFAULT_ICON_HEIGHT + 27 * TWIPS;
const unsigned int Model::DEFAULT_ENTRY_HEIGHT	= DEFAULT_ICON_HEIGHT * 3/5;	/* 27 */
const unsigned int Model::DEFAULT_ENTRY_WIDTH	= DEFAULT_ICON_HEIGHT;


Model::Model( const std::string& input_file_name )
    : _document(0), _processors(), _tasks()
{
    unsigned int error_code;
    _document = LQIO::DOM::Document::load( input_file_name,	/* Input file name */
					   "",			/* Output file name - used by SPEX. */
					   &Model::__io_vars,	/* Trivia. */
					   error_code,		/* */
					   true );		/* Load results */
    if ( !_document ) throw std::invalid_argument( input_file_name );

    /* Add all nodes to the model */

    for ( std::map<std::string,LQIO::DOM::Processor *>::const_iterator next_proc = _document->getProcessors().begin(); next_proc != _document->getProcessors().end(); ++next_proc ) {
	LQIO::DOM::Processor * dom_processor = next_proc->second;
	Processor * processor = new Processor( *dom_processor, *this );
        _processors.insert( processor );
	for ( std::vector<LQIO::DOM::Task *>::const_iterator next_task = dom_processor->getTaskList().begin(); next_task != dom_processor->getTaskList().end(); ++next_task ) {
	    const LQIO::DOM::Task * task = *next_task;
	    _tasks.insert( new Task( *task, processor, *this ) );
	}
    }

    /* Arcs are created, but not connected above, since the entries may not exist.  Do that now. */

    connectCalls();
}

Model::~Model()
{
    for ( std::set<Processor *>::const_iterator next_processor = _processors.begin(); next_processor != _processors.end(); ++next_processor ) {
        delete *next_processor;
    }
    for ( std::set<Task *>::const_iterator task = _tasks.begin(); task != _tasks.end(); ++task ) {
        delete *task;
    }
    delete _document;
}

bool Model::loadResults( const std::string& filename )
{
    unsigned int errorCode;
    return _document->loadResults( filename.c_str(), errorCode );

}
Entry * Model::addEntry( Entry * entry ) 
{
    _entries[entry->getName()] = entry;
    return entry;
}
  
Entry * Model::findEntry( const std::string& s ) const
{
    std::map<std::string,Entry *>::const_iterator entry = _entries.find( s );
    if ( entry != _entries.end() ) return entry->second;
    return 0;
}

Node * Model::findNode( const wxPoint& position ) const
{
    std::multiset<Node *,Model::compareXPos>::const_iterator curr_node = _nodes.end();
    for ( std::multiset<Node *,Model::compareXPos>::const_iterator next_node = _nodes.begin(); 
	  (next_node = find_if( next_node, _nodes.end(), Node::containsPoint( position ) )) != _nodes.end(); 
	  ++next_node ) {
	if ( curr_node == _nodes.end() || (*next_node)->focusPriority() < (*curr_node)->focusPriority() ) {
	    curr_node = next_node;
	}	
    }
    return curr_node != _nodes.end() ? *curr_node : 0;
}

void Model::connectCalls()
{
    for ( std::set<Task *>::const_iterator next_task = _tasks.begin(); next_task != _tasks.end(); ++next_task ) {
	(*next_task)->connectCalls();
    }
}

void Model::autoLayout()
{
    topologicalSort();
    layerize();
}

void Model::topologicalSort()
{
    /* Find reference tasks, then recursively sort */
    for ( std::set<Task *>::const_iterator next_task = _tasks.begin(); next_task != _tasks.end(); ++next_task ) {
        Task * task = *next_task;
	if ( task->isReferenceTask() ) {
	    std::set<const Task *> call_chain;
	    for ( std::vector<Entry *>::const_iterator next_entry = task->_entries.begin(); next_entry != task->_entries.end(); ++next_entry ) {
		try {
		    (*next_entry)->findChildren( call_chain );
		}
		catch ( std::runtime_error &error ) {
		    /* Ignore... */
		}
	    }
	    call_chain.insert( task );
	}
    }
}

void Model::layerize()
{
    std::multimap<unsigned,Node *> nodes;	/* Ordered by layer. */
    for ( std::set<Task *>::const_iterator next_task = _tasks.begin(); next_task != _tasks.end(); ++next_task ) {
        Task * task = *next_task;
	nodes.insert( std::pair<unsigned int, Node *>(task->getLayer(),task) );
	const Processor * processor = task->getProcessor();
	const_cast<Processor *>(processor)->setLayer( task->getLayer() + 1 );
    }

    for ( std::set<Processor *>::const_iterator next_processor = _processors.begin(); next_processor != _processors.end(); ++next_processor ) {
        Processor * processor = *next_processor;
	nodes.insert( std::pair<unsigned int, Node *>(processor->getLayer(),processor) );
    }

    /* Now assign x and y positions based on the multimap */

    unsigned int last_layer = 0;
    wxPoint point( 0, 0 );
    for ( std::multimap<unsigned,Node *>::const_iterator next_node = nodes.begin(); next_node != nodes.end(); ++next_node ) {
	unsigned int layer = next_node->first;
	if ( layer != last_layer ) {
	    last_layer = layer;
	    point.x = 50 * TWIPS;
	    point.y += DEFAULT_Y_SPACING;
	}

	Node * node = next_node->second;
	node->moveTo( point );
	point.x += 100 * TWIPS;
    }

    /* Reorder arcs to get rid of crossing */

    for ( std::multimap<unsigned,Node *>::const_iterator next_node = nodes.begin(); next_node != nodes.end(); ++next_node ) {
	Node * node = next_node->second;
	node->reorder();
    }
}


void Model::render( wxDC& dc ) const
{
    for ( std::set<Processor *>::const_iterator iter = _processors.begin(); iter != _processors.end(); ++iter ) {
        Processor * proc = *iter;
	proc->render( dc );
    }
    for ( std::set<Task *>::const_iterator iter = _tasks.begin(); iter != _tasks.end(); ++iter ) {
        Task * task = *iter;
	task->render( dc );
    }
}  

/*
 * Copy over common error messages and set max_error.
 */

void
Model::init_errmsg()
{
    __io_vars.error_messages = LQIO::global_error_messages;
o    __io_vars.max_error = LQIO::LSTGBLERRMSG;
}

/*
 * What to do based on the severity of the error.
 */

void
Model::severity_action (unsigned int severity)
{
    switch( severity ) {
    case LQIO::FATAL_ERROR:
	(void) exit( 1 );
	break;

    case LQIO::RUNTIME_ERROR:
	__io_vars.anError = true;
	__io_vars.error_count += 1;
	break;
    }
}

bool Model::compareXPos::operator()( const Node * n1, const Node * n2 ) const
{
    return n1->_origin.x < n2->_origin.x;
}


