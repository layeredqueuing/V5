/*
 *  $Id$
 *
 *  Created by Martin Mroz on 1/07/09.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#include "dom_document.h"
#include "dom_group.h"
#include "dom_processor.h"
#include "glblerr.h"

namespace LQIO {
    namespace DOM {
    
	Group::Group(const Document * document, const char * name, Processor* proc, ExternalVariable * share, bool cap, const void * group_element ) 
	    : DocumentObject(document,name,group_element), _processor(proc), _groupShare(share), _cap(cap), 
	      _resultUtilization(0.0), _resultUtilizationVariance(0.0)
	      
	{ 
	}
    
	Group::Group(const Group& src ) 
	    : DocumentObject(src.getDocument(),"",NULL), _processor(NULL), _groupShare(src.getGroupShare()), _cap(src.getCap())
	{ 
	}
    
	Group::~Group()
	{
	}
    
	const Processor* Group::getProcessor() const
	{
	    /* Returns the Processor of the Group */
	    return _processor;
	}

	void Group::setProcessor(Processor* processor)
	{
	    /* Stores the given Processor of the Group */ 
	    _processor = processor;
	}
    
	double Group::getGroupShareValue() const
	{
	    /* Returns the GroupShare of the Group */
	    double value = 0.0;
	    assert(_groupShare->getValue(value) == true);
	    return value;
	}

	ExternalVariable * Group::getGroupShare() const
	{
	    /* Returns the GroupShare of the Group */
	    return _groupShare;
	}

	void Group::setGroupShare( ExternalVariable * groupShare )
	{
	    /* Stores the given GroupShare of the Group */ 
	    _groupShare = groupShare;
	}
    
	void Group::setGroupShareValue(const double value)
	{
	    if ( _groupShare == NULL ) {
		_groupShare = new ConstantExternalVariable(value);
	    } else {
		_groupShare->set(value);
	    }
	}
    
	const bool Group::getCap() const
	{
	    /* Returns the Cap of the Group */
	    return _cap;
	}

	void Group::setCap(const bool cap)
	{
	    /* Stores the given Cap of the Group */ 
	    _cap = cap;
	}
    
	void Group::addTask(Task* task)
	{
	    /* Link the task into the list */
	    _taskList.insert(task);
	}
    
	/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- [Result Values] -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */
    
	double Group::getResultUtilization() const
	{
	    /* Returns the ResultUtilization of the Group */
	    return _resultUtilization;
	}

	Group& Group::setResultUtilization(const double resultUtilization)
	{
	    /* Stores the given ResultUtilization of the Group */ 
	    _resultUtilization = resultUtilization;
	    return *this;
	}

	double Group::getResultUtilizationVariance() const
	{
	    return _resultUtilizationVariance;
	}
    
	Group& Group::setResultUtilizationVariance(const double resultUtilizationVariance)
	{
	    /* Stores the given ResultUtilization of the Group */ 
	    if ( resultUtilizationVariance > 0 ) {
		const_cast<Document *>(getDocument())->setResultHasConfidenceIntervals(true);
	    }
	    _resultUtilizationVariance = resultUtilizationVariance;
	    return *this;
	}
    }
}
