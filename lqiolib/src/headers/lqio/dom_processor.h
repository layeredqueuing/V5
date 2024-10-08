/* -*- c++ -*-
 *  $Id: dom_processor.h 17333 2024-10-03 19:51:55Z greg $
 *
 *  Created by Martin Mroz on 24/02/09.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#ifndef __LQIO_DOM_PROCESSOR__
#define __LQIO_DOM_PROCESSOR__

#include <set>
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
	    Processor(const Document *document, const std::string& name, scheduling_type scheduling_flag,
		      const ExternalVariable* n_cpus=nullptr, const ExternalVariable* n_replicas=nullptr );
	    Processor( const Processor& );
	    virtual ~Processor();

	    /* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- [Input Values] -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */

	    /* Accessors and Mutators */

	    const char * getTypeName() const { return __typeName; }

	    bool hasRate() const;
	    double getRateValue() const;
	    const ExternalVariable * getRate() const { return _processorRate; }
	    void setRateValue(const double newRate);
	    void setRate(const ExternalVariable * newRate);

	    bool hasQuantum() const;
	    double getQuantumValue() const;
	    const ExternalVariable * getQuantum() const { return _processorQuantum; }
	    void setQuantum( const ExternalVariable * newQuantum ) { _processorQuantum = newQuantum; }
	    void setQuantumValue(const double newQuantum);

	    bool hasQuantumScheduling() const;
	    bool hasPriorityScheduling() const;

	    /* Managing the Task List */
	    void addTask(Task* task);
	    void addGroup(Group* group);
	    const std::set<Task*>& getTaskList() const { return _taskList; }
	    const std::set<Group*>& getGroupList() const { return _groupList; }
	    void clearTaskList() { _taskList.clear(); }

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
	    const ExternalVariable * _processorRate;
	    const ExternalVariable * _processorQuantum;
	    std::set<Task*> _taskList;
	    std::set<Group*> _groupList;

	    /* Computation Results from LQNS */
	    double _resultUtilization;
	    double _resultUtilizationVariance;

	public:
	    static const char * __typeName;

	};

    }
}

#endif /* __LQIO_DOM_PROCESSOR__ */
