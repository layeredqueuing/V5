/* -*- c++ -*-
 *
 *  Created by Martin Mroz on 24/02/09.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 *  $Id: dom_actlist.h 11963 2014-04-10 14:36:42Z greg $
 */

#ifndef __LQIO_DOM_ACTLIST__
#define __LQIO_DOM_ACTLIST__

#include <stdexcept>
#include <vector>
#include <map>
#include "dom_object.h"
#include "dom_activity.h"

namespace LQIO {
    namespace DOM {
    
	class Entry;
	class ExternalVariable;
	class Task;

	class ActivityList : public DocumentObject {
	public:
      
	    /* Descriminator for the list type */
	    typedef enum ActivityListType {
		JOIN_ACTIVITY_LIST = 1,
		FORK_ACTIVITY_LIST,
		AND_FORK_ACTIVITY_LIST,
		AND_JOIN_ACTIVITY_LIST,
		OR_FORK_ACTIVITY_LIST,
		OR_JOIN_ACTIVITY_LIST,
		REPEAT_ACTIVITY_LIST
	    } ActivityListType;
      
	public:
	    /* Designated constructor and destructor */
	    ActivityList(const Document * document,const Task *,ActivityListType type,const void *element=0);
	    virtual ~ActivityList();
      
	    /* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- [Input Values] -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
      
	    /* Accessors and Mutators */
	    bool isJoinList() const;
	    bool isForkList() const;
      
	    /* Managing membership */
	    void setTask( const Task * task );
	    const Task * getTask() const;
	    const std::vector<const Activity*>& getList() const;
	    const ActivityListType getListType() const;
	    ActivityList& add(const Activity* activity, ExternalVariable * arg=NULL);
	    void addValue( const Activity* activity, double arg );
	    ExternalVariable * getParameter(const Activity* activity) const throw( std::domain_error );
	    double getParameterValue(const Activity* activity) const throw( std::domain_error );
      
	    /* Managing connections */
	    void setNext(ActivityList* next);
	    ActivityList* getNext() const;
	    void setPrevious(ActivityList* previous);
	    ActivityList* getPrevious() const;
      
	    /* 
	     * Processed flag for clients -- multiple clients may have the same
	     * activity list (ptr) leading to possible duplications upon 
	     * re-processing. Setting this flag will help keep track of that. Optional.
	     */
	    void setProcessed(const bool flag) { _processed = flag; }
	    const bool getProcessed() const { return _processed; }

	    /*
	     * Return activities that make up the "name" of the list.
	     */

	    void activitiesForName( const Activity** first, const Activity** last ) const;
      
	private:
	    ActivityList& operator=(const ActivityList&);
	    ActivityList( const ActivityList& );

	private:
      
	    /* Instance variables */
	    const Task * _task;
	    std::vector<const Activity*> _list;
	    std::map<const Activity*,ExternalVariable *> _arguments;
	    ActivityListType _type;
	    ActivityList* _next;
	    ActivityList* _prev;
	    bool _processed;
      
	};

	/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
	/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
	/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
    
	class AndJoinActivityList : public ActivityList {
	public:
	    AndJoinActivityList(const Document * document, const Task * task, ExternalVariable *quorumCount, const void * element=0 );
	    AndJoinActivityList( const AndJoinActivityList& );
	    virtual ~AndJoinActivityList();

	    AndJoinActivityList& setQuorumCountValue(const unsigned quorum);
	    unsigned getQuorumCountValue() const;
	    AndJoinActivityList& setQuorumCount(ExternalVariable * quorum);
	    ExternalVariable * getQuorumCount() const;
	    bool hasQuorumCount() const;

	    virtual bool hasHistogram() const;
	    virtual const Histogram* getHistogram() const { return _histogram; }
	    virtual void setHistogram(Histogram* histogram);

	    /* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- [Result Values] -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */
      
	    double getResultJoinDelay() const;
	    AndJoinActivityList& setResultJoinDelay(const double joinDelay );
	    double getResultJoinDelayVariance() const;
	    AndJoinActivityList& setResultJoinDelayVariance(const double joinDelayVariance );
	    double getResultVarianceJoinDelay() const;
	    AndJoinActivityList& setResultVarianceJoinDelay(const double varianceJoinDelay );
	    double getResultVarianceJoinDelayVariance() const;
	    AndJoinActivityList& setResultVarianceJoinDelayVariance(const double varianceJoinDelayVariance );
	    const bool hasResultVarianceJoinDelay() const { return  _hasResultVarianceJoinDelay; }

	private:
	    ExternalVariable * _quorum;
	    Histogram * _histogram;

	    double _resultJoinDelay;
	    double _resultJoinDelayVariance;
	    bool _hasResultVarianceJoinDelay;
	    double _resultVarianceJoinDelay;
	    double _resultVarianceJoinDelayVariance;
	};

    }
}
#endif /* __LQIO_DOM_ACTLIST__ */
