/* -*- c++ -*-
 *
 *  Created by Martin Mroz on 24/02/09.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 *  $Id: dom_actlist.h 14387 2021-01-21 14:09:16Z greg $
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
	    enum class Type {
		JOIN,
		FORK,
		AND_FORK,
		AND_JOIN,
		OR_FORK,
		OR_JOIN,
		REPEAT
	    };
      
	public:
	    /* Designated constructor and destructor */
	    ActivityList(const Document * document,const Task *,ActivityList::Type type );
	    virtual ~ActivityList();
      
	    /* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- [Input Values] -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
      
	    /* Accessors and Mutators */
	    const char * getTypeName() const { return __typeName; }

	    bool isJoinList() const;
	    bool isForkList() const;
      
	    /* Managing membership */
	    void setTask( const Task * task );
	    const Task * getTask() const;
	    const std::vector<const Activity*>& getList() const;
	    const ActivityList::Type getListType() const;
	    ActivityList& add(const Activity* activity, const ExternalVariable * arg=NULL);

	    void addValue( const Activity* activity, double arg );
	    const ExternalVariable * getParameter(const Activity* activity) const;
	    double getParameterValue(const Activity* activity) const;
      
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
	    std::map<const Activity*,const ExternalVariable *> _arguments;
	    ActivityList::Type _type;
	    ActivityList* _next;
	    ActivityList* _prev;
	    bool _processed;
      
	public:
	    static const char * __typeName;
	};

	/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
	/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
	/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
    
	class AndJoinActivityList : public ActivityList {
	public:
	    AndJoinActivityList(const Document * document, const Task * task, const ExternalVariable *quorumCount );
	    AndJoinActivityList( const AndJoinActivityList& );
	    virtual ~AndJoinActivityList();

	    AndJoinActivityList& setQuorumCountValue(const unsigned quorum);
	    unsigned getQuorumCountValue() const;
	    AndJoinActivityList& setQuorumCount(const ExternalVariable * quorum);
	    const ExternalVariable * getQuorumCount() const;
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
	    const ExternalVariable * _quorum;
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
