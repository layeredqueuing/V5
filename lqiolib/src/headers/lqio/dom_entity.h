/*
 *  $Id: dom_entity.h 15220 2021-12-15 15:18:47Z greg $
 *
 *  Created by Martin Mroz on 24/02/09.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#ifndef __LQIO_DOM_ENTITY__
#define __LQIO_DOM_ENTITY__

#include "input.h"
#include "dom_object.h"
#include <utility>

namespace LQIO {
    namespace DOM {
	class Document;
	class ExternalVariable;

	class Entity : public DocumentObject {
	protected:
	    template <class Type> struct add_using {
		typedef double (Type::*fp)();
		add_using<Type>( fp f ) : _f(f) {}
		double operator()( double sum, Type * object ) { return sum + (object->*_f)(); }
		double operator()( double sum, const std::pair<std::string,Type *>& object ) { return sum + (object.second->*_f)(); }
	    private:
		const fp _f;
	    };

	    template <class Type> struct add_using_const {
		typedef double (Type::*fp)() const;
		add_using_const<Type>( fp f ) : _f(f) {}
		double operator()( double sum, const Type * object ) { return sum + (object->*_f)(); }
		double operator()( double sum, const std::pair<std::string,Type *>& object ) { return sum + (object.second->*_f)(); }
	    private:
		const fp _f;
	    };

	protected:
	    Entity(const Entity&);

	public:

	    /* Designated initializers for the SVN DOM Entity type */
	    Entity(const Document * document, const std::string& name,
		   const scheduling_type schedulingType,
		   const ExternalVariable * copies,
		   const ExternalVariable* replicas );
	    virtual ~Entity();

	    /* Accessors and Mutators */
	    const unsigned int getId() const;
	    void setId(const unsigned int newId);
	    const scheduling_type getSchedulingType() const;
	    void setSchedulingType(const scheduling_type type);
	    bool hasCopies() const;
	    const unsigned int getCopiesValue() const;
	    double getCopiesValueAsDouble() const { return static_cast<double>( getCopiesValue() ); }
	    const ExternalVariable* getCopies() const;
	    void setCopies(const ExternalVariable* newCopies);
	    void setCopiesValue(const unsigned int);
	    bool hasReplicas() const;
	    const unsigned int getReplicasValue() const;
	    const ExternalVariable* getReplicas() const;
	    void setReplicasValue(const unsigned int newReplicas);
	    void setReplicas(const ExternalVariable* newReplicas);

	    const bool isMultiserver() const;
	    const bool isInfinite() const;

	private:
	    Entity& operator=( const Entity& );

	    /* Instance variables for Entities */
	    unsigned int _entityId;
	    scheduling_type _entitySchedulingType;
	    const ExternalVariable* _copies;
	    const ExternalVariable* _replicas;
	};
    }
}

#endif /* __LQIO_DOM_ENTITY__ */
