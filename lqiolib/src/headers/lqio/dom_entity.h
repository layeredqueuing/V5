/*
 *  $Id: dom_entity.h 13675 2020-07-10 15:29:36Z greg $
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
	    template <class Type> struct Sum {
		typedef double (Type::*fp)();
		Sum<Type>( fp f ) : _f(f), _sum(0.) {}
		void operator()( Type * object ) { _sum += (object->*_f)(); }
		void operator()( const std::pair<std::string,Type *>& object ) { _sum += (object.second->*_f)(); }
		double sum() const { return _sum; }
	    private:
		fp _f;
		double _sum;
	    };

	    template <class Type> struct ConstSum {
		typedef double (Type::*fp)() const;
		ConstSum<Type>( fp f ) : _f(f), _sum(0.) {}
		void operator()( const Type * object ) { _sum += (object->*_f)(); }
		void operator()( const std::pair<std::string,Type *>& object ) { _sum += (object.second->*_f)(); }
		double sum() const { return _sum; }
	    private:
		fp _f;
		double _sum;
	    };

	    template <class Type> struct ConstSumP {
		typedef double (Type::*fp)( unsigned int p ) const;
		ConstSumP<Type>( fp f, unsigned int p ) : _f(f), _p(p), _sum(0.) {}
		void operator()( const Type * object ) { _sum += (object->*_f)(_p); }
		double sum() const { return _sum; }
	    private:
		fp _f;
		const unsigned int _p;
		double _sum;
	    };

	protected:
	    Entity(const Entity&);

	public:
      
	    /* Designated initializers for the SVN DOM Entity type */
	    Entity(const Document * document, const std::string& name, 
		   const scheduling_type schedulingType,
		   ExternalVariable * copies,
		   ExternalVariable* replicas );
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
	    bool hasReplicas() const;
	    const unsigned int getReplicasValue() const;
	    const ExternalVariable* getReplicas() const;
	    void setReplicasValue(const unsigned int newReplicas);
	    void setReplicas(ExternalVariable* newReplicas);

	    const bool isMultiserver() const;
	    const bool isInfinite() const;

	private:
	    Entity& operator=( const Entity& );
      
	    /* Instance variables for Entities */
	    unsigned int _entityId;
	    scheduling_type _entitySchedulingType;
	    ExternalVariable* _copies;
	    ExternalVariable* _replicas;
      
	};
    }
}

#endif /* __LQIO_DOM_ENTITY__ */
