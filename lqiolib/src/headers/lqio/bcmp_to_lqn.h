/* -*- C++ -*-
 *  $Id: expat_document.h 13717 2020-08-03 00:04:28Z greg $
 *
 *  Created by Greg Franks 2020/12/28
 */

#ifndef __LQIO_BCMP_TO_LQN__
#define __LQIO_BCMP_TO_LQN__

#include <map>
#include <string>
#include "bcmp_document.h"

namespace LQIO {
    namespace DOM {
	class Document;
    }
}


namespace LQIO {
    namespace DOM {
	class BCMP_to_LQN {
	public:
	    BCMP_to_LQN( const BCMP::Model& bcmp, Document& lqn ) : _bcmp(bcmp), _lqn(lqn) {}

	    bool convert() const;

	private:
	    const BCMP::Model::Model::Station::map_t& stations() const { return _bcmp.stations(); }
	    const BCMP::Model::Class::map_t& classes() const { return _bcmp.classes(); }
	    
	
	private:
	    struct createLQNTaskProcessor
	    {
		createLQNTaskProcessor( LQIO::DOM::Document& lqn ) : _lqn(lqn) {}

		void operator()( const BCMP::Model::Station::pair_t& );
		void operator()( const BCMP::Model::Class::pair_t& );
	    private:
		LQIO::DOM::Document& _lqn;
	    };

	private:
	    const BCMP::Model& _bcmp;
	    Document& _lqn;
	};
    }
}
#endif /* */
