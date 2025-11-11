/* -*- c++ -*-
 *  $Id: dom_entity.h 17577 2025-11-10 18:41:17Z greg $
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
#include <vector>

#define BUG_393 1

namespace LQIO {
    namespace DOM {
	class Document;
	class ExternalVariable;

	class Entity : public DocumentObject {
	protected:
	    Entity(const Entity&);

	private:
	    Entity& operator=( const Entity& ) = delete;

	public:

	    /* Designated initializers for the SVN DOM Entity type */
	    Entity(const Document * document, const std::string& name,
		   const scheduling_type schedulingType,
		   const ExternalVariable * copies,
		   const ExternalVariable* replicas );
	    virtual ~Entity();

	    /* Accessors and Mutators */
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
	    double getReplicasValueAsDouble() const { return static_cast<double>( getReplicasValue() ); }
	    const ExternalVariable* getReplicas() const;
	    void setReplicasValue(const unsigned int newReplicas);
	    void setReplicas(const ExternalVariable* newReplicas);

	    const bool isMultiserver() const;
	    const bool isInfinite() const;

#if defined(BUG_393)
	    const std::vector<double>& getResultMarginalQueueProbabilities() const { return _resultMarginalQueueProbabilities; }
	    std::vector<double>& getResultMarginalQueueProbabilities() { return _resultMarginalQueueProbabilities; }
	    void setResultMarginalQueueProbabilitiesSize( size_t i ) { _resultMarginalQueueProbabilities.resize( i ); }
	    size_t getResultMarginalQueueProbabilitiesSize() const { return _resultMarginalQueueProbabilities.size(); }
	    void setResultMarginalQueueProbability( size_t i, double value ) { _resultMarginalQueueProbabilities.at(i) = value; }
	    double getResultMarginalQueueProbability( size_t i ) const { return _resultMarginalQueueProbabilities.at(i); }
#endif
	    
	private:
	    /* Instance variables for Entities */
	    scheduling_type _entitySchedulingType;
	    const ExternalVariable* _copies;
	    const ExternalVariable* _replicas;

#if defined(BUG_393)
	    std::vector<double> _resultMarginalQueueProbabilities;
#endif
	};
    }
}

#endif /* __LQIO_DOM_ENTITY__ */
