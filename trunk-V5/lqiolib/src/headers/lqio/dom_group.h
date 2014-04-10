/*
 *  $Id$
 *
 *  Created by Martin Mroz on 1/07/09.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#ifndef __LQIO_DOM_GROUP__
#define __LQIO_DOM_GROUP__

#include <string>
#include <set>
#include "dom_object.h"

namespace LQIO {
    namespace DOM {

	class Document;
	class Processor;
	class ExternalVariable;
	class Task;
	class Group : public DocumentObject {
	public:
      
	    /* Designated initializers for the Group type */
	    Group(const Document * document, const char * name, Processor* proc=0, ExternalVariable * share=0, bool cap=false, const void * group_element=0);
	    Group( const Group & );
	    virtual ~Group();
      
	    /* Accessors and Mutators */
	    const Processor* getProcessor() const;
	    void setProcessor(Processor* processor);
	    double getGroupShareValue() const;
	    ExternalVariable * getGroupShare() const;
	    void setGroupShare(ExternalVariable * groupShare);
	    void setGroupShareValue( double value );
	    const bool getCap() const;
	    void setCap(const bool cap);
      
	    void addTask(Task* task);
	    const std::set<Task*>& getTaskList() const { return _taskList; }

	    /* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- [Result Values] -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

	    /* Storing the Result Parameters */
	    double getResultUtilization() const;
	    Group& setResultUtilization(const double resultUtilization);
	    double getResultUtilizationVariance() const;
	    Group& setResultUtilizationVariance(const double resultUtilizationVariance);

	private:
	    Group& operator=( const Group& );
      
	    /* Instance Variables */

	    Processor* _processor;
	    ExternalVariable * _groupShare;
	    bool _cap;

	    std::set<Task*> _taskList;

	    /* Results */
	    double _resultUtilization;
	    double _resultUtilizationVariance;
	};
    }
}
#endif /* __LQIO_DOM_GROUP__ */
