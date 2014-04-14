/*
 *  $Id$
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
		       const unsigned int replicas, const Type type, const void * xmlDOMElement ) :
	    DocumentObject(document, name, xmlDOMElement),
	    _entityId(const_cast<Document *>(document)->getNextEntityId()), 
	    _entitySchedulingType(schedulingType),
	    _copies(copies), _replicas(replicas),
	    _type(type)
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
	    double value = 0.0;
	    return _copies && (!_copies->wasSet() || !_copies->getValue(value) || value != 1);	    /* Check whether we have it or not */
	}

	const unsigned int Entity::getCopiesValue() const throw ()
	{
	    /* Obtain the copies count */
	    double value;
	    assert(_copies->getValue(value) == true);
	    assert(value - floor(value) == 0);
	    return static_cast<unsigned int>(value);
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
    
	void Entity::setCopies(ExternalVariable* newCopies)
	{
	    /* Set the number of copies */
	    _copies = newCopies;
	}
    
	const unsigned int Entity::getReplicas() const
	{
	    /* Return the replica count */
	    return _replicas;
	}
    
	void Entity::setReplicas(const unsigned int newReplicas)
	{
	    /* Set the replica count */
	    _replicas = newReplicas;
	}
    
	const Entity::Type Entity::getType()
	{
	    /* Return the entity type */
	    return _type;
	}

	const bool Entity::isMultiserver() const
	{
	    /* Return true if this is (or can be) a multiserver */
	    double v;
	    const LQIO::DOM::ExternalVariable * m = getCopies();
	    return m && (!m->wasSet() || !m->getValue(v) || v > 1.0);
	}
    
	const bool Entity::isInfinite() const
	{
	    if ( getSchedulingType() == SCHEDULE_DELAY ) {
		return true;
	    } else {
		const LQIO::DOM::ExternalVariable * copies = getCopies(); 
		double value;
		return copies && (copies->getValue(value) == true && std::isinf( value ) != 0);
	    }
	}
    }
}
