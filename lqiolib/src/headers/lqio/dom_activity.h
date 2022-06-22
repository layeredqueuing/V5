/* -*- c++ -*-
 *  $Id: dom_activity.h 15685 2022-06-21 21:20:52Z greg $
 *
 *  Created by Martin Mroz on 24/02/09.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#ifndef __LQIO_DOM_ACTIVITY__
#define __LQIO_DOM_ACTIVITY__

#include "dom_phase.h"

namespace LQIO {
    namespace DOM {
    
	class Document;
	class Entry;
	class ActivityList;
	class Task;
	
	class Activity : public Phase {
	public:
      
	    /* Constructors and Destructors */
	    Activity( const Document * document, const std::string& name );
	    Activity( const Activity& );
	    virtual ~Activity();
      
	    /* Basic Activity Accessors and Mutators */
	    const char * getTypeName() const { return __typeName; }

	    std::vector<DOM::Entry*>& getReplyList();
	    const std::vector<DOM::Entry*>& getReplyList() const { return _replyList; }
	    const bool isSpecified() const;
	    void setIsSpecified(const bool isSpecified);
	    const DOM::Task * getTask() const;
	    void setTask( DOM::Task * );
      
	    /* Simple information covering start activities */
	    bool isStartActivity() const { return this->getSourceEntry() != nullptr; }
	    void outputTo(ActivityList* outputList);
	    void inputFrom(ActivityList* inputList);
	    ActivityList* getOutputToList() const { return _outputList; }
	    ActivityList* getInputFromList() const { return _inputList; }
      
	    /* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- [More Result Values] -=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */
      
	    /* Storage of the additional parameters */
	    double getResultSquaredCoeffVariation() const;
	    Activity& setResultSquaredCoeffVariation(const double resultCVSquared);
	    double getResultThroughput() const;
	    Activity& setResultThroughput(const double resultThroughput);
	    double getResultThroughputVariance() const;
	    Activity& setResultThroughputVariance(const double resultThroughputVariance);
	    double getResultProcessorUtilization() const;
	    Activity& setResultProcessorUtilization(const double resultProcessorUtilization);
	    double getResultProcessorUtilizationVariance() const;
	    Activity& setResultProcessorUtilizationVariance(const double resultProcessorUtilizationVariance);
      
	private:
      
	    /* Instance Variables */
	    std::vector<DOM::Entry*> _replyList;
	    bool _isSpecified;
	    Task * _task;
      
	    /* Activity Lists */
	    ActivityList* _outputList;
	    ActivityList* _inputList;
      
	    /* Extra Results */
	    double _resultCVSquared;
	    double _resultThroughput;
	    double _resultThroughputVariance;
	    double _resultProcessorUtilization;
	    double _resultProcessorUtilizationVariance;
      
	public:
	    static const char * __typeName;
	};
    
    }
}
#endif /* __LQIO_DOM_ACTIVITY__ */
