/*
 *  $Id: dom_entity.cpp 13559 2020-05-26 14:38:45Z greg $
 *
 *  Created by Martin Mroz on 24/02/09.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#include "dom_entity.h"
#include "dom_extvar.h"
#include "dom_document.h"
#include <cmath>

namespace LQIO {
    namespace DOM {
    
	Entity::Entity(const Document * document, const char * name, 
		       const scheduling_type schedulingType, ExternalVariable* copies,
		       ExternalVariable* replicas) :
	    DocumentObject(document, name),
	    _entityId(const_cast<Document *>(document)->getNextEntityId()), 
	    _entitySchedulingType(schedulingType),
	    _copies(copies), _replicas(replicas)
	{
	    /* Empty Constructor */
	}
    
	Entity::~Entity()
	{
	    /* Empty Destructor */
	}
    
	const unsigned int Entity::getId() const
	{
	    /* Return the entity ID */
	    return _entityId;
	}
    
	void Entity::setId(const unsigned int newId)
	{
	    /* Set the entity ID */
	    _entityId = newId;
	}
    
	const scheduling_type Entity::getSchedulingType() const
	{
	    /* Return the scheduling type */
	    return _entitySchedulingType;
	}
    
	void Entity::setSchedulingType(const scheduling_type type)
	{
	    /* Set the new scheduling type */
	    _entitySchedulingType = type;
	}
    
	bool Entity::hasCopies() const
	{
	    return ExternalVariable::isPresent( _copies, 1.0 );	    /* Check whether we have it or not */
	}

	const unsigned int Entity::getCopiesValue() const
	{
	    return getIntegerValue( getCopies(), 1 );
	}
    
	const ExternalVariable* Entity::getCopies() const
	{
	    return _copies;
	}
    

	void Entity::setCopiesValue( const unsigned int value )
	{
	    /* Set the number of copies */
	    if ( _copies == NULL ) {
		_copies = new ConstantExternalVariable(value);
	    } else {
		_copies->set(value);
	    }
	}
    
	void Entity::setCopies( ExternalVariable* var )
	{
	    /* Set the number of copies.  Allow infinity (so don't use checkIntegerVariable) */
	    double value = 1.;
	    if ( var != NULL && var->wasSet() && ( var->getValue(value) != true || value != rint(value) || value < 1. ) ) {
		throw std::domain_error( "invalid integer" );
	    }
	    _copies = var;
	}
    
	bool Entity::hasReplicas() const
	{
	    return ExternalVariable::isPresent( getReplicas(), 0.0 );	    /* Check whether we have it or not */
	}

	const unsigned int Entity::getReplicasValue() const
	{
	    /* Return the replica count */
	    return getIntegerValue( getReplicas(), 1 );
	}
    
	const ExternalVariable* Entity::getReplicas() const
	{
	    /* Return the replica count */
	    return _replicas;
	}
    
	void Entity::setReplicasValue( const unsigned int value )
	{
	    /* Set the number of replicas */
	    if ( _replicas == NULL ) {
		_replicas = new ConstantExternalVariable(value);
	    } else {
		_replicas->set(value);
	    }
	}
    
	void Entity::setReplicas(ExternalVariable * newReplicas)
	{
	    /* Set the replica count */
	    _replicas = newReplicas;
	}
    
	const bool Entity::isMultiserver() const
	{
	    /* Return true if this is (or can be) a multiserver */
	    return ExternalVariable::isPresent( getCopies(), 1.0 );
	}
    
	const bool Entity::isInfinite() const
	{
	    if ( getSchedulingType() == SCHEDULE_DELAY ) {
		return true;
	    } else {
		double v;
		const LQIO::DOM::ExternalVariable * m = getCopies(); 
		return m && m->wasSet() && m->getValue(v) && std::isinf(v);
	    }
	}

    }
}
