/*
 *  $Id: dom_processor.cpp 15895 2022-09-23 17:21:55Z greg $
 *
 *  Created by Martin Mroz on 24/02/09.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#include <algorithm>
#include <numeric>
#include "dom_document.h"
#include "dom_processor.h"
#include "dom_group.h"
#include "dom_task.h"
#include "glblerr.h"
#include "labels.h"

namespace LQIO {
    namespace DOM {

	const char * Processor::__typeName = "processor";

	Processor::Processor(const Document * document, const std::string& name, scheduling_type scheduling_flag,
			     const ExternalVariable* n_cpus, const ExternalVariable* n_replicas )
	    : Entity(document, name, scheduling_flag, n_cpus, n_replicas ),
	      _processorRate(nullptr),
	      _processorQuantum(nullptr),
	      _taskList(),
	      _groupList(),
	      _resultUtilization(0.0),
	      _resultUtilizationVariance(0.0)
	{
	}

	Processor::Processor(const Processor& src )
	    : Entity(src),
	      _processorRate(ExternalVariable::clone(src._processorRate)),
	      _processorQuantum(ExternalVariable::clone(src._processorQuantum)),
	      _taskList(),
	      _groupList(),
	      _resultUtilization(0.0),
	      _resultUtilizationVariance(0.0)
	{
	}

	Processor::~Processor()
	{
	    /* Delete all owned tasks */
	    for ( std::set<Task*>::const_iterator task = _taskList.begin(); task != _taskList.end(); ++task) {
		delete *task;
	    }
	}

	/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- [Input Values] -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */

	bool Processor::hasRate() const
	{
	    return ExternalVariable::isPresent( getRate(), 1.0 );
	}

	double Processor::getRateValue() const
	{
	    /* Return the processor rate */
	    double value = getDoubleValue( _processorRate, 0. );
	    if ( value == 0. ) {
		throw std::domain_error( "zero" );
	    }
	    return value;
	}

	void Processor::setRateValue(const double value)
	{
	    /* Store the new processor rate */
	    _processorRate = new ConstantExternalVariable( value );
	}

	void
	Processor::setRate(const ExternalVariable * newRate)
	{
	    _processorRate = newRate;
	}

	bool Processor::hasQuantum() const
	{
	    return ExternalVariable::isPresent( getQuantum(), 0.0 );
	}

	double Processor::getQuantumValue() const
	{
	    /* Return the processor quantum */
	    double value = 0.0;
	    if ( !_processorQuantum || _processorQuantum->getValue(value) != true || value <= 0.0 ) {
		throw std::domain_error( "invalid quantum" );
	    }
	    return value;
	}

	void Processor::setQuantumValue(const double value )
	{
	    /* Store the new CPU Time Quantum */
	    _processorQuantum = new ConstantExternalVariable( value );
	}

	bool Processor::hasQuantumScheduling() const
	{
	    const scheduling_type s = getSchedulingType();
	    return s == SCHEDULE_CFS
		|| s == SCHEDULE_PS;
	}


	bool Processor::hasPriorityScheduling() const
	{
	    const scheduling_type s = getSchedulingType();
	    return s == SCHEDULE_HOL
		|| s == SCHEDULE_PPR;
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
		setResultUtilization( std::accumulate( _taskList.begin(), _taskList.end(), 0.0, add_using<Task>( &Task::computeResultProcessorUtilization ) ) );
		setResultUtilizationVariance( std::accumulate( _taskList.begin(), _taskList.end(), 0., add_using_const<Task>( &Task::getResultProcessorUtilizationVariance ) ) );
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
