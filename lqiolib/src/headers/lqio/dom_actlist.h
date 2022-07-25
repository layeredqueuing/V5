/* -*- c++ -*-
 *
 *  Created by Martin Mroz on 24/02/09.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 *  $Id: dom_actlist.h 15753 2022-07-22 10:59:11Z greg $
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
      
	private:
	    ActivityList& operator=(const ActivityList&) = delete;
	    ActivityList( const ActivityList& ) = delete;

	public:
	    /* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- [Input Values] -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
      
	    /* Accessors and Mutators */
	    const char * getTypeName() const { return __typeName; }
	    const std::string getListName() const;
	    void activitiesForName( const Activity*& first, const Activity*& last ) const;
	    
	    bool isJoinList() const;
	    bool isForkList() const;
      
	    /* Managing membership */
	    void setTask( const Task * task );
	    const Task * getTask() const;
	    const std::vector<const Activity*>& getList() const;
	    const ActivityList::Type getListType() const;
	    const std::string& getListTypeName() const;
	    ActivityList& add(const Activity* activity, const ExternalVariable * arg=nullptr);
	    ActivityList& addValue( const Activity* activity, double arg );
	    
	    const ExternalVariable * getParameter(const Activity* activity) const;
	    double getParameterValue(const Activity* activity) const;
      
	    /* Managing connections */
	    void setNext(ActivityList* next);
	    ActivityList* getNext() const;
	    void setPrevious(ActivityList* previous);
	    ActivityList* getPrevious() const;
      
	    virtual std::string inputErrorPreamble( unsigned int code ) const;
	    virtual std::string runtimeErrorPreamble( unsigned int code ) const;

	private:
	    /* Instance variables */
	    const Task * _task;
	    std::vector<const Activity*> _list;
	    std::map<const Activity*,const ExternalVariable *> _arguments;
	    ActivityList::Type _type;
	    ActivityList* _next;
	    ActivityList* _prev;
      
	public:
	    static const char * __typeName;
	    static const std::map<const Type, const std::string> __op;
	    static const std::map<const Type, const std::string> __listTypeName;

	private:
	    /* Used to concatentate activity list names into a string */
	    struct fold {
		fold( const std::string& op ) : _op(op) {}
		std::string operator()( const std::string& s1, const Activity * a2 ) const { return s1 + " " + _op + " " + a2->getName(); }
	    private:
		const std::string& _op;
	    };
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
