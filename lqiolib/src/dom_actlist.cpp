/*
 *  $Id: dom_actlist.cpp 12458 2016-02-21 18:48:34Z greg $
 *
 *  Created by Martin Mroz on 24/02/09.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#include "dom_document.h"
#include "dom_actlist.h"
#include "dom_histogram.h"

namespace LQIO {
    namespace DOM {

	ActivityList::ActivityList(const Document * document, const Task * task, ActivityListType type, const void * xmlDOMElement) 
	    : DocumentObject(document,"",xmlDOMElement),		/* By default, no name :-) */
	      _task(task), _list(), _arguments(), _type(type), 
	      _next(NULL), _prev(NULL),
	      _processed(false)
	{
	    const_cast<Task *>(task)->addActivityList(this);
	}

	ActivityList::~ActivityList()
	{
	}

	bool ActivityList::isJoinList() const
	{
	    return _type == JOIN_ACTIVITY_LIST || _type == AND_JOIN_ACTIVITY_LIST || _type == OR_JOIN_ACTIVITY_LIST;
	}

	bool ActivityList::isForkList() const
	{
	    return _type == FORK_ACTIVITY_LIST || _type == AND_FORK_ACTIVITY_LIST || _type == OR_FORK_ACTIVITY_LIST || _type == REPEAT_ACTIVITY_LIST;
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

	const ActivityList::ActivityListType ActivityList::getListType() const
	{
	    return _type;
	}

	ActivityList& ActivityList::add(const Activity* activity, ExternalVariable * arg )
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

	ExternalVariable * ActivityList::getParameter(const Activity* activity) const throw( std::domain_error )
	{
	    std::map<const Activity*,ExternalVariable *>::const_iterator item = _arguments.find(activity);
	    if ( item == _arguments.end() ) throw std::domain_error( activity->getName().c_str() );
	    return item->second;
	}

	double ActivityList::getParameterValue(const Activity* activity) const throw( std::domain_error )
	{
	    std::map<const Activity*,ExternalVariable *>::const_iterator item = _arguments.find(activity);
	    double value;
	    if ( item == _arguments.end() || item->second->getValue( value ) != true ) {
		throw std::domain_error( activity->getName().c_str() );
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


	AndJoinActivityList::AndJoinActivityList(const Document * document, const Task * task, ExternalVariable * quorum, const void * xmlDOMElement) 
	    : ActivityList(document,task,AND_JOIN_ACTIVITY_LIST,xmlDOMElement), _quorum(quorum), _histogram(0),
	      _resultJoinDelay(0.0),
	      _resultJoinDelayVariance(0.0),
	      _hasResultVarianceJoinDelay(false),
	      _resultVarianceJoinDelay(0.0),
	      _resultVarianceJoinDelayVariance(0.0)
	{
	    const_cast<Document *>(document)->setTaskHasAndJoin(true);
	}

	AndJoinActivityList::AndJoinActivityList( const AndJoinActivityList& src ) 
	    : ActivityList(src.getDocument(),src.getTask(),src.getListType(),0), 
	      _quorum(src.getQuorumCount()), 
	      _histogram(0),
	      _resultJoinDelay(0.0),
	      _resultJoinDelayVariance(0.0),
	      _hasResultVarianceJoinDelay(false),
	      _resultVarianceJoinDelay(0.0),
	      _resultVarianceJoinDelayVariance(0.0)
	{
	}
	
	AndJoinActivityList::~AndJoinActivityList()
	{
	    if ( _histogram ) {
		delete _histogram;
	    }
	}

	AndJoinActivityList& AndJoinActivityList::setQuorumCountValue(const unsigned value)
	{
	    /* Store the value into the ExtVar */
	    if (_quorum == NULL) {
		_quorum = new ConstantExternalVariable(value);
	    } else {
		_quorum->set(value);
	    }
	    return *this;
	}

	unsigned AndJoinActivityList::getQuorumCountValue() const
	{
	    double value = 0.0;
	    if ( _quorum ) {
		assert(_quorum->getValue(value) == true);
	    }
	    return static_cast<unsigned int>(value);
	}

	AndJoinActivityList& AndJoinActivityList::setQuorumCount(ExternalVariable * quorum)
	{
	    _quorum = quorum;
	    return *this;
	}

	ExternalVariable * AndJoinActivityList::getQuorumCount() const
	{
	    return _quorum;
	}

	bool AndJoinActivityList::hasQuorumCount() const
	{
	    double value = 0.0;
	    return _quorum && (!_quorum->wasSet() || !_quorum->getValue(value) || value > 0);	    /* Check whether we have it or not */
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
