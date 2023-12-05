/* -*- C++ -*-
 *  $Id: bcmp_to_lqn.h 16879 2023-12-04 18:08:54Z greg $
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
	class Entry;
	class Task;
	class Document;
	class ExternalVariable;
    }
}
namespace LQX {
    class Environment;
}


namespace LQIO {
    namespace DOM {
	class BCMP_to_LQN {
	    typedef std::pair<std::string,LQIO::DOM::Entry *> entry_item;
	    typedef std::map<std::string,LQIO::DOM::Entry *> entry_type;

	public:
	    BCMP_to_LQN( const BCMP::Model& bcmp, Document& lqn ) : _bcmp(bcmp), _lqn(lqn), _client_entries(), _server_entries() {}

	    bool convert();

	    static LQIO::DOM::ExternalVariable * getExternalVariable( const LQX::SyntaxTreeNode *, LQX::Environment * = nullptr );

	private:
	    const BCMP::Model::Model::Station::map_t& stations() const { return _bcmp.stations(); }
	    const BCMP::Model::Chain::map_t& chains() const { return _bcmp.chains(); }
	
	private:
	    class Self {
	    public:
		Self( BCMP_to_LQN& self ) : _self(self) {}
		
		const BCMP::Model::Model::Station::map_t& stations() const { return _self.stations(); }
		const BCMP::Model::Chain::map_t& chains() const { return _self.chains(); }
		Document& lqn() { return _self._lqn; }
		entry_type& client_entries() { return _self._client_entries; };
//		const entry_type& client_entries() const { return _self._client_entries; };
		entry_type& server_entries() { return _self._server_entries; };
//		const entry_type& server_entries() const { return _self._server_entries; };
		LQX::Environment * environment() const;

	    private:
		BCMP_to_LQN& _self;
	    };
	    
	    class createLQNTaskProcessor : private Self
	    {
	    public:
		createLQNTaskProcessor( BCMP_to_LQN& self ) : Self(self) {}

		void operator()( const BCMP::Model::Station::pair_t& );
		void operator()( const BCMP::Model::Chain::pair_t& );
	    };

	    class connectClassToStation : private Self
	    {
	    public:
		connectClassToStation( BCMP_to_LQN& self ) : Self(self) {}

		void operator()( const BCMP::Model::Chain::pair_t& );
	    };

	private:
	    const BCMP::Model& _bcmp;
	    Document& _lqn;
	    entry_type _client_entries;
	    entry_type _server_entries;
	};
    }
}
#endif /* */
