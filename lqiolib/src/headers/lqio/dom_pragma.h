/* -*- c++ -*- */
/************************************************************************/
/* Copyright the Real-Time and Distributed Systems Group,		*/
/* Department of Systems and Computer Engineering,			*/
/* Carleton University, Ottawa, Ontario, Canada. K1S 5B6		*/
/* 									*/
/* 2020.								*/
/************************************************************************/

/*
 * $Id: dom_pragma.h 14178 2020-12-07 21:16:43Z greg $
 */

#ifndef __LQIO_DOM_PRAGMA_H__
#define __LQIO_DOM_PRAGMA_H__

#include <map>
#include <set>
#include <string>

namespace LQIO {
    namespace DOM {
	class Pragma {
	private:
	    struct Merge {
		Merge( Pragma& p ) : _p(p) {}
		void operator()( const std::pair<std::string,std::string> obj ) const { _p.insert( obj.first, obj.second ); }
	    private:
		Pragma& _p;
	    };
	    
	public:
	    Pragma();

	    bool insert( const std::string&,const std::string&);
	    bool insert( const char * );		/* pragma=value */
	    void merge( const std::map<std::string,std::string>& list );
	    const std::map<std::string,std::string>& getList() const;
	    size_t size() const { return getList().size(); }
	    const std::string get( const std::string& ) const;
	    static const std::set<std::string>* getValues( const std::string& );
	    void clear();

	private:
	    bool check( const std::string&, const std::string& ) const;
	    void initialize();
	    
	private:
	    static std::map<std::string,std::set<std::string>*> __pragmas;
	    std::map<std::string,std::string> _loadedPragmas;

	public:
	    static const char * _abort_all_;		// Quorum
	    static const char * _abort_local_;		// Quorum
	    static const char * _abort_remote_;		// Quorum
	    static const char * _advisory_;
	    static const char * _all_;
	    static const char * _batched_;
	    static const char * _batched_back_;
	    static const char * _block_period_;
	    static const char * _bruell_;
	    static const char * _conway_;
	    static const char * _custom_;
	    static const char * _custom_natural_;
	    static const char * _cycles_;
	    static const char * _default_;
	    static const char * _default_natural_;
	    static const char * _deterministic_;	// Quorum
	    static const char * _exact_;
	    static const char * _exponential_;
	    static const char * _false_;
	    static const char * _fast_;
	    static const char * _force_multiserver_;
	    static const char * _gamma_;		// Quorum
	    static const char * _geometric_;		// Quorum
	    static const char * _hwsw_;
	    static const char * _hyper_;
	    static const char * _idle_time_;
	    static const char * _init_only_;
	    static const char * _initial_delay_;
	    static const char * _initial_loops_;
	    static const char * _interlocking_;
	    static const char * _layering_;
	    static const char * _linearizer_;
	    static const char * _join_delay_;		// Quorum
	    static const char * _keep_all_;		// Quorum
	    static const char * _mak_;
	    static const char * _markov_;
	    static const char * _max_blocks_;
	    static const char * _mol_;
	    static const char * _mol_back_;
	    static const char * _multiserver_;
	    static const char * _mva_;
	    static const char * _nice_;
	    static const char * _no_;
	    static const char * _no_entry_;
	    static const char * _none_;
	    static const char * _one_step_;
	    static const char * _one_step_linearizer_;
	    static const char * _overtaking_;
	    static const char * _precision_;
	    static const char * _processor_scheduling_;
	    static const char * _processors_;
	    static const char * _prune_;
	    static const char * _quorum_delayed_calls_;	// Quroum
	    static const char * _quorum_distribution_;	// Quroum
	    static const char * _quorum_idle_time_;	// Qurom
	    static const char * _quorum_reply_;		// Quroum
	    static const char * _reiser_;
	    static const char * _reiser_ps_;
	    static const char * _reschedule_on_async_send_;
	    static const char * _rolia_;
	    static const char * _rolia_ps_;
	    static const char * _run_time_;
	    static const char * _scheduling_model_;
	    static const char * _schmidt_;
	    static const char * _schweitzer_;
	    static const char * _seed_value_;
	    static const char * _severity_level_;
	    static const char * _simple_;
	    static const char * _special_;
	    static const char * _spex_header_;
	    static const char * _squashed_;
	    static const char * _srvn_;
	    static const char * _stochastic_;
	    static const char * _stop_on_bogus_utilization_;
	    static const char * _stop_on_message_loss_;
	    static const char * _suri_;
	    static const char * _task_scheduling_;
	    static const char * _tasks_;
	    static const char * _tau_;
	    static const char * _threads_;
	    static const char * _threepoint_;		// Quorum
	    static const char * _true_;
	    static const char * _underrelaxation_;
	    static const char * _variance_;
	    static const char * _warning_;
	    static const char * _yes_;
	};
    }
}


#endif /* __LQIO_DOM_PRAGMA_H__ */
