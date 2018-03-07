/*
 *  $Id: dom_entity.h 13200 2018-03-05 22:48:55Z greg $
 *
 *  Created by Martin Mroz on 24/02/09.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#ifndef __LQIO_DOM_ENTITY__
#define __LQIO_DOM_ENTITY__

#include "input.h"
#include "dom_object.h"

namespace LQIO {
    namespace DOM {
	class Document;
	class ExternalVariable;

	class Entity : public DocumentObject {
	public:
      
	    /* All known entity types */
	    typedef enum Type {
		TYPE_PROCESSOR,
		TYPE_TASK,
		TYPE_ACTIVITY
	    } Type;
      
	public:
      
	    /* Designated initializers for the SVN DOM Entity type */
	    Entity(const Document * document, const char * name, 
		   const scheduling_type schedulingType,
		   ExternalVariable* copies,
		   const unsigned int replicas,
		   const Type type, const void * xmlDOMElement );
	    virtual ~Entity();
      
	    /* Accessors and Mutators */
	    const unsigned int getId() const;
	    void setId(const unsigned int newId);
	    const scheduling_type getSchedulingType() const;
	    void setSchedulingType(const scheduling_type type);
	    bool hasCopies() const;
	    const unsigned int getCopiesValue() const;
	    const ExternalVariable* getCopies() const;
	    void setCopies(ExternalVariable* newCopies);
	    void setCopiesValue(const unsigned int);
	    const unsigned int getReplicas() const;
	    void setReplicas(const unsigned int newReplicas);
	    const Type getType();

	    const bool isMultiserver() const;
	    const bool isInfinite() const;
      
	private:
	    Entity( const Entity& );
	    Entity& operator=( const Entity& );
      
	    /* Instance variables for Entities */
	    unsigned int _entityId;
	    scheduling_type _entitySchedulingType;
	    ExternalVariable* _copies;
	    unsigned int _replicas;
	    const Type _type;
      
	};
    }
}

#endif /* __LQIO_DOM_ENTITY__ */
