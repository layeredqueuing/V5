/*
 *  $Id: dom_actlist.cpp 14603 2021-04-16 15:53:36Z greg $
 *
 *  Created by Martin Mroz on 24/02/09.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#include <cmath>
#include "dom_actlist.h"
#include "dom_task.h"
#include "dom_extvar.h"
#include "dom_histogram.h"

namespace LQIO {
    namespace DOM {

	const char * ActivityList::__typeName = "activity_list";
	
	ActivityList::ActivityList(const Document * document, const Task * task, ActivityList::Type type ) 
	    : DocumentObject(document,""),		/* By default, no name :-) */
	      _task(task), _list(), _arguments(), _type(type), 
	      _next(nullptr), _prev(nullptr),
	      _processed(false)
	{
	    if ( task != nullptr ) {
		const_cast<Task *>(task)->addActivityList(this);
	    }
	}

	ActivityList::~ActivityList()
	{
	}

	bool ActivityList::isJoinList() const
	{
	    return _type == Type::JOIN || _type == Type::AND_JOIN || _type == Type::OR_JOIN;
	}

	bool ActivityList::isForkList() const
	{
	    return _type == Type::FORK || _type == Type::AND_FORK || _type == Type::OR_FORK || _type == Type::REPEAT;
	}

	void ActivityList::setTask( const Task * task )
	{
	    _task = task;
	    const_cast<Task *>(task)->addActivityList(this);
	}

	const Task * ActivityList::getTask() const
	{
	    return _task;
	}

	const std::vector<const Activity*>& ActivityList::getList() const
	{
	    return *&_list;
	}

	const ActivityList::Type ActivityList::getListType() const
	{
	    return _type;
	}

	ActivityList& ActivityList::add(const Activity* activity, const ExternalVariable * arg )
	{
	    _list.push_back(activity);
	    _arguments[activity] = arg;
	    return *this;
	}

	void ActivityList::addValue(const Activity* activity, double arg )
	{
	    _list.push_back(activity);
	    _arguments[activity] = new ConstantExternalVariable( arg );
	}

	const ExternalVariable * ActivityList::getParameter(const Activity* activity) const
	{
	    std::map<const Activity*,const ExternalVariable *>::const_iterator item = _arguments.find(activity);
	    if ( item == _arguments.end() ) throw std::domain_error( activity->getName() );
	    return item->second;
	}

	double ActivityList::getParameterValue(const Activity* activity) const
	{
	    std::map<const Activity*,const ExternalVariable *>::const_iterator item = _arguments.find(activity);
	    double value;
	    if ( item == _arguments.end() || item->second->getValue( value ) != true ) {
		throw std::domain_error( activity->getName() );
	    }
	    return value;
	}

	void ActivityList::setNext(ActivityList* next)
	{
	    _next = next;
	}

	ActivityList* ActivityList::getNext() const
	{
	    return _next;
	}

	void ActivityList::setPrevious(ActivityList* previous)
	{
	    _prev = previous;
	}

	ActivityList* ActivityList::getPrevious() const
	{
	    return _prev;
	}

	void ActivityList::activitiesForName( const Activity** first, const Activity** last ) const
	{
	    *first = _list[0];
	    *last  = _list[_list.size()-1];
	}

	/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
	/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
	/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */


	AndJoinActivityList::AndJoinActivityList(const Document * document, const Task * task, const ExternalVariable * quorum ) 
	    : ActivityList(document,task,Type::AND_JOIN), _quorum(quorum), _histogram(nullptr),
	      _resultJoinDelay(0.0),
	      _resultJoinDelayVariance(0.0),
	      _hasResultVarianceJoinDelay(false),
	      _resultVarianceJoinDelay(0.0),
	      _resultVarianceJoinDelayVariance(0.0)
	{
	}

	AndJoinActivityList::AndJoinActivityList( const AndJoinActivityList& src ) 
	    : ActivityList(src.getDocument(),src.getTask(),src.getListType()), 
	      _quorum(src.getQuorumCount()), 
	      _histogram(nullptr),
	      _resultJoinDelay(0.0),
	      _resultJoinDelayVariance(0.0),
	      _hasResultVarianceJoinDelay(false),
	      _resultVarianceJoinDelay(0.0),
	      _resultVarianceJoinDelayVariance(0.0)
	{
	}
	
	AndJoinActivityList::~AndJoinActivityList()
	{
	}

	AndJoinActivityList& AndJoinActivityList::setQuorumCountValue(const unsigned value)
	{
	    /* Store the value into the ExtVar */
	    if (_quorum == nullptr) {
		_quorum = new ConstantExternalVariable(value);
	    } else {
		const_cast<ExternalVariable *>(_quorum)->set(value);
	    }
	    return *this;
	}

	unsigned AndJoinActivityList::getQuorumCountValue() const
	{
	    return getIntegerValue( getQuorumCount(), 0 );
	}

	AndJoinActivityList& AndJoinActivityList::setQuorumCount(const ExternalVariable * quorum)
	{
	    _quorum = checkIntegerVariable( quorum, 0 );
	    return *this;
	}

	const ExternalVariable * AndJoinActivityList::getQuorumCount() const
	{
	    return _quorum;
	}

	bool AndJoinActivityList::hasQuorumCount() const
	{
	    return ExternalVariable::isPresent( getQuorumCount(), 0. );		// 0 implies no quorum.
	}

	bool AndJoinActivityList::hasHistogram() const
	{ 
	    return _histogram != 0 && _histogram->getBins() > 0; 
	}

	void AndJoinActivityList::setHistogram(Histogram* histogram)
	{
	    /* Stores the given Histogram of the Phase */
	    _histogram = histogram;
	}

	double AndJoinActivityList::getResultJoinDelay() const
	{
	    return _resultJoinDelay;
	}

	AndJoinActivityList& AndJoinActivityList::setResultJoinDelay(const double joinDelay )
	{
	    _resultJoinDelay = joinDelay;
	    return *this;
	}

	double AndJoinActivityList::getResultJoinDelayVariance() const
	{
	    return _resultJoinDelayVariance;
	}

	AndJoinActivityList& AndJoinActivityList::setResultJoinDelayVariance(const double joinDelayVariance )
	{
	    _resultJoinDelayVariance = joinDelayVariance;
	    return *this;
	}

	double AndJoinActivityList::getResultVarianceJoinDelay() const
	{
	    return _resultVarianceJoinDelay;
	}

	AndJoinActivityList& AndJoinActivityList::setResultVarianceJoinDelay(const double varianceJoinDelay )
	{
	    _hasResultVarianceJoinDelay = true;
	    _resultVarianceJoinDelay = varianceJoinDelay;
	    return *this;
	}

	double AndJoinActivityList::getResultVarianceJoinDelayVariance() const
	{
	    return _resultVarianceJoinDelayVariance;
	}

	AndJoinActivityList& AndJoinActivityList::setResultVarianceJoinDelayVariance(const double varianceJoinDelayVariance )
	{
	    _resultVarianceJoinDelayVariance = varianceJoinDelayVariance;
	    return *this;
	}


    }
}
