/*
 *  $Id$
 *
 *  Created by Martin Mroz on 24/02/09.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#include "dom_document.h"
#include "dom_processor.h"
#include "dom_group.h"
#include "dom_task.h"
#include "glblerr.h"
#include "labels.h"

namespace LQIO {
    namespace DOM {
    
	Processor::Processor(const Document * document, const char * processor_name, scheduling_type scheduling_flag, 
			     ExternalVariable* n_cpus, int n_replicas, const void* processor_element) 
	    : Entity(document, processor_name, scheduling_flag, n_cpus, n_replicas, TYPE_PROCESSOR, processor_element),
	      _processorRate(0),
	      _processorQuantum(0),
	      _taskList(),
	      _groupList(),
	      _resultUtilization(0.0),
	      _resultUtilizationVariance(0.0)
	{
	}
    
	Processor::Processor(const Processor& src ) 
	    : Entity(src.getDocument(), "", src.getSchedulingType(), const_cast<LQIO::DOM::ExternalVariable*>(src.getCopies()), 1, TYPE_PROCESSOR, 0),
	      _processorRate(src.getRate()),
	      _processorQuantum(src.getQuantum()),
	      _taskList(),
	      _groupList(),
	      _resultUtilization(0.0),
	      _resultUtilizationVariance(0.0)
	{
	}
    
	Processor::~Processor()
	{
	    /* Delete all owned tasks */
	    std::set<Task*>::iterator iter;
	    for (iter = _taskList.begin(); iter != _taskList.end(); ++iter) {
		delete (*iter);
	    }
	}
    
	/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- [Input Values] -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
    
	bool Processor::hasRate() const
	{
	    double value = 0.0;
	    return _processorRate && (!_processorRate->wasSet() || !_processorRate->getValue(value) || value != 1);	    /* Check whether we have it or not */
	}

	const double Processor::getRateValue() const
	{
	    /* Return the processor rate */
	    double value = 0.0;
	    assert(_processorRate->getValue(value) == true);
	    return value;
	}
    
	void Processor::setRateValue(const double value)
	{
	    /* Store the new processor rate */
	    if ( _processorRate == NULL ) {
		_processorRate = new ConstantExternalVariable( value );
	    } else {
		_processorRate->set(value);
	    }
	    const_cast<Document *>(getDocument())->setProcessorHasRate(true);
	}

	void
	Processor::setRate(ExternalVariable * newRate) 
	{ 
	    _processorRate = newRate;
	    const_cast<Document *>(getDocument())->setProcessorHasRate( newRate != NULL );
	}	
    
	const double Processor::getQuantumValue() const
	{
	    /* Return the processor quantum */
	    double value = 0.0;
	    assert(_processorQuantum->getValue(value) == true);
	    return value;
	}
    
	void Processor::setQuantumValue(const double value )
	{
	    /* Store the new CPU Time Quantum */
	    if ( _processorQuantum == NULL ) {
		_processorQuantum = new ConstantExternalVariable( value );
	    } else {
		_processorQuantum->set(value);
	    }
	}
    
	bool Processor::hasPriorityScheduling() const
	{
	    const scheduling_type s = getSchedulingType();
	    return s == SCHEDULE_HOL 	       
		|| s == SCHEDULE_PPR
		|| s == SCHEDULE_PS_HOL
		|| s == SCHEDULE_PS_PPR;
	}

	void Processor::addTask(Task* task)
	{
	    /* Link the task into the list */
	    _taskList.insert(task);
	}
    
	void Processor::addGroup(Group* group)
	{
	    /* Link the group into the list */
	    _groupList.insert(group);
	}
    
	/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- [Result Values] -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */
    
	double Processor::getResultUtilization() const
	{
	    /* Returns the ResultUtilization of the Processor */
	    return _resultUtilization;
	}

	Processor& Processor::setResultUtilization(const double resultUtilization)
	{
	    /* Stores the given ResultUtilization of the Processor */ 
	    _resultUtilization = resultUtilization;
	    return *this;
	}

	double Processor::computeResultUtilization()
	{
	    if ( getResultUtilization() == 0 || _taskList.size() == 1 ) {
		double sum = 0;
		std::set<Task*>::const_iterator taskIter;
		for (taskIter = _taskList.begin(); taskIter != _taskList.end(); ++taskIter) {
		    sum += (*taskIter)->computeResultProcessorUtilization();
		}
		setResultUtilization( sum );
	    }
	    return getResultUtilization();
	}
    
	double Processor::getResultUtilizationVariance() const
	{
	    return _resultUtilizationVariance;
	}
    
	Processor& Processor::setResultUtilizationVariance(const double resultUtilizationVariance)
	{
	    /* Stores the given ResultUtilization of the Processor */ 
	    if ( resultUtilizationVariance > 0 ) {
		const_cast<Document *>(getDocument())->setResultHasConfidenceIntervals(true);
	    }
	    _resultUtilizationVariance = resultUtilizationVariance;
	    return *this;
	}
    
    }
}
