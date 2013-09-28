/*
 *  $Id$
 *
 *  Created by Martin Mroz on 24/02/09.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#ifndef __LQIO_DOM_PROCESSOR__
#define __LQIO_DOM_PROCESSOR__

#include <vector>
#include "dom_entity.h"

namespace LQIO {
    namespace DOM {

	/* Forward references */
	class Task;
	class Group;
	class ExternalVariable;

	class Processor : public Entity {
	public:

	    /* Designated initializers for the SVN DOM Entity type */
	    Processor(const Document *document, const char * processor_name, scheduling_type scheduling_flag,
		      ExternalVariable* n_cpus, int n_replicas, 
		      const void * processor_element);
	    Processor( const Processor& );
	    virtual ~Processor();

	    /* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- [Input Values] -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */

	    /* Accessors and Mutators */

	    bool hasRate() const;
	    const double getRateValue() const;
	    ExternalVariable * getRate() const { return _processorRate; }
	    void setRateValue(const double newRate);
	    Processor& setRate(ExternalVariable * newRate);

	    bool hasQuantum() const { return _processorQuantum != 0; }
	    const double getQuantumValue() const;
	    ExternalVariable * getQuantum() const { return _processorQuantum; }
	    Processor& setQuantum( ExternalVariable * newQuantum ) { _processorQuantum = newQuantum; return *this; }
	    void setQuantumValue(const double newQuantum);

	    bool hasPriorityScheduling() const;

	    /* Managing the Task List */
	    void addTask(Task* task);
	    void addGroup(Group* group);
	    const std::vector<Task*>& getTaskList() const { return _taskList; }
	    const std::vector<Group*>& getGroupList() const { return _groupList; }

	    /* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- [Result Values] -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

	    /* Storing the Result Parameters */
	    double getResultUtilization() const;
	    Processor& setResultUtilization(const double resultUtilization);
	    double computeResultUtilization();
	    double getResultUtilizationVariance() const;
	    Processor& setResultUtilizationVariance(const double resultUtilizationVariance);

	private:
	    Processor& operator=( const Processor& );

	private:

	    /* Instance variables for Processors */
	    ExternalVariable * _processorRate;
	    ExternalVariable * _processorQuantum;
	    std::vector<Task*> _taskList;
	    std::vector<Group*> _groupList;

	    /* Computation Results from LQNS */
	    double _resultUtilization;
	    double _resultUtilizationVariance;

	};

    };
};

#endif /* __LQIO_DOM_PROCESSOR__ */
