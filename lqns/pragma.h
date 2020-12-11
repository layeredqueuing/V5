/* -*- c++ -*-
 * $HeadURL: http://rads-svn.sce.carleton.ca:8080/svn/lqn/trunk-V5/lqns/pragma.h $
 *
 * Pragma processing.
 *
 * Copyright the Real-Time and Distributed Systems Group,
 * Department of Systems and Computer Engineering,
 * Carleton University, Ottawa, Ontario, Canada. K1S 5B6
 *
 * November, 1994
 * December, 2020
 *
 * $Id: pragma.h 14204 2020-12-11 12:50:59Z greg $
 *
 * ------------------------------------------------------------------------
 */

#if	!defined(PRAGMA_H)
#define	PRAGMA_H

#include "dim.h"
#include <string>
#include <map>
#include <lqio/input.h>
#include <lqio/dom_document.h>
#include "help.h"

/* -------------------------------------------------------------------- */

class Pragma {

public:
    typedef void (Pragma::*fptr)(const std::string&);

    typedef enum { BACKPROPOGATE_LAYERS, BATCHED_LAYERS, METHOD_OF_LAYERS, BACKPROPOGATE_METHOD_OF_LAYERS, SRVN_LAYERS, SQUASHED_LAYERS, HWSW_LAYERS } pragma_layering;
    typedef enum { LINEARIZER_MVA, EXACT_MVA, SCHWEITZER_MVA, FAST_MVA, ONESTEP_MVA, ONESTEP_LINEARIZER } pragma_mva;
    typedef enum { DEFAULT_MULTISERVER, CONWAY_MULTISERVER, REISER_MULTISERVER, REISER_PS_MULTISERVER, ROLIA_MULTISERVER, ROLIA_PS_MULTISERVER, BRUELL_MULTISERVER, SCHMIDT_MULTISERVER, SURI_MULTISERVER } pragma_multiserver;
    typedef enum { MARKOV_OVERTAKING, ROLIA_OVERTAKING, SIMPLE_OVERTAKING, SPECIAL_OVERTAKING, NO_OVERTAKING } pragma_overtaking;
    typedef enum { DEFAULT_VARIANCE, NO_VARIANCE, STOCHASTIC_VARIANCE, MOL_VARIANCE } pragma_variance;
#if HAVE_LIBGSL && HAVE_LIBGSLCBLAS
    typedef enum { DEFAULT_QUORUM_DISTRIBUTION, THREEPOINT_QUORUM_DISTRIBUTION,
	GAMMA_QUORUM_DISTRIBUTION, CLOSEDFORM_GEOMETRIC_QUORUM_DISTRIBUTION,
	CLOSEDFORM_DETRMINISTIC_QUORUM_DISTRIBUTION } pragma_quorum_distribution;
    typedef enum { DEFAULT_QUORUM_DELAYED_CALLS,KEEP_ALL_QUORUM_DELAYED_CALLS,
	ABORT_ALL_QUORUM_DELAYED_CALLS, ABORT_LOCAL_ONLY_QUORUM_DELAYED_CALLS,
	ABORT_REMOTE_ONLY_QUORUM_DELAYED_CALLS } pragma_quorum_delayed_calls;
    typedef enum { DEFAULT_IDLETIME, JOINDELAY_IDLETIME, ROOTENTRY_IDLETIME } pragma_quorum_idle_time;
#endif
    typedef enum { MAK_LUNDSTROM_THREADS, HYPER_THREADS, NO_THREADS } pragma_threads;
    typedef enum { FORCE_NONE, FORCE_PROCESSORS, FORCE_TASKS, FORCE_ALL } pragma_force_multiserver;

private:
    Pragma();
    virtual ~Pragma() 
	{
	    __cache = nullptr;
	}

    static void set_pragma( const std::pair<std::string,std::string>& p );
    
public:
    static bool allowCycles()
	{
	    assert( __cache != nullptr );
	    return __cache->_allow_cycles;
	}

    static bool exponential_paths()
	{
	    assert( __cache != nullptr );
	    return __cache->_exponential_paths;
	}

    static bool forceMultiserver( pragma_force_multiserver arg )
	{
	    assert( __cache != nullptr );
	    return (__cache->_force_multiserver != FORCE_NONE && arg == __cache->_force_multiserver)
		|| (__cache->_force_multiserver == FORCE_ALL  && arg != FORCE_NONE );
	}
    
    static bool interlock()
	{
	    assert( __cache != nullptr );
	    return __cache->_interlock;
	}
    
    static pragma_layering layering()
	{
	    assert( __cache != nullptr );
	    return __cache->_layering;
	}

    static pragma_multiserver multiserver()
	{
	    assert( __cache != nullptr );
	    return __cache->_multiserver;
	}

    static pragma_mva mva() 
	{
	    assert( __cache != nullptr );
	    return __cache->_mva;
	}

    static pragma_overtaking overtaking()
	{
	    assert( __cache != nullptr );
	    return __cache->_overtaking;
	}
    
    static bool overtaking( pragma_overtaking arg )
	{
	    return overtaking() == arg;
	}

    static bool defaultProcessorScheduling()
	{
	    assert( __cache != nullptr );
	    return __cache->_default_processor_scheduling;
	}
    
    static scheduling_type processorScheduling()
	{
	    assert( __cache != nullptr );
	    return __cache->_processor_scheduling;
	}
    
#if BUG_270
    static bool prune()
	{
	    assert( __cache != nullptr );
	    return __cache->_prune;
	}
#endif
	    
#if HAVE_LIBGSL && HAVE_LIBGSLCBLAS
    static pragma_quorum_distribution getQuorumDistribution()
	{
	    assert( __cache != nullptr );
	    return __cache->_quorum_distribution;
	}
    
    static pragma_quorum_delayed_calls getQuorumDelayedCalls()
	{
	    assert( __cache != nullptr );
	    return __cache->_quorum_delayed_calls;
	}
    static pragma_quorum_idle_time getQuorumIdleTime()
	{
	    assert( __cache != nullptr );
	    return __cache->_quorum_idle_time;
	}
#endif
#if RESCHEDULE
    static bool getRescheduleOnAsyncSend()
	{
	    assert( __cache != nullptr );
	    return __cache->_reschedule_on_async_send;
	}
#endif

    static LQIO::severity_t severityLevel()
	{
	    assert( __cache != nullptr );
	    return __cache->_severity_level;
	}
    
    static bool spexHeader()
	{
	    assert( __cache != nullptr );
	    return __cache->_spex_header;
	}

    static bool stopOnMessageLoss()
	{
	    assert( __cache != nullptr );
	    return __cache->_stop_on_message_loss;
	}

    static double stopOnBogusUtilization()
	{
	    assert( __cache != nullptr );
	    return __cache->_stop_on_bogus_utilization;
	}

    static double tau()
	{
	    assert( __cache != nullptr );
	    return __cache->_tau;
	}

    static pragma_threads threads()
	{
	    assert( __cache != nullptr );
	    return __cache->_threads;
	}

    static bool threads( pragma_threads arg ) 
	{
	    return threads() == arg;
	}

    static pragma_variance variance()
	{
	    assert( __cache != nullptr );
	    return __cache->_variance;
	}

    static bool variance( pragma_variance arg )
	{
	    assert( __cache != nullptr );
	    return variance() == arg;
	}

    static bool entry_variance()
	{
	    assert( __cache != nullptr );
	    return __cache->_entry_variance;
	}

    static bool init_variance_only()
	{
	    assert( __cache != nullptr );
	    return __cache->_init_variance_only;
	}

private:
    void setAllowCycles(const std::string&);
    void setExponential_paths(const std::string&);
    void setForceMultiserver(const std::string&);
    void setInterlock(const std::string&);
    void setLayering(const std::string&);
    void setMultiserver(const std::string&);
    void setMva(const std::string&);
    void setOvertaking(const std::string&);
    void setProcessorScheduling(const std::string&);
#if BUG_270
    void setPrune(const std::string&);
#endif
#if HAVE_LIBGSL && HAVE_LIBGSLCBLAS
    void setQuorumDistribution(const std::string&);
    void setQuorumDelayedCalls(const std::string&);
    void setQuorumIdleTime(const std::string&);
#endif
#if RESCHEDULE
    void setRescheduleOnAsyncSend(const std::string&);
#endif
    void setSeverityLevel(const std::string&);
    void setSpexHeader(const std::string&);
    void setStopOnBogusUtilization(const std::string&);
    void setStopOnMessageLoss(const std::string&);
    void setTau(const std::string&);
    void setThreads(const std::string&);
    void setVariance(const std::string&);
    
    static bool isTrue(const std::string&);

public:
    static void set( const std::map<std::string,std::string>& );
    static void initialize();
    static std::ostream& usage( std::ostream&  );
    static const std::map<std::string,Pragma::fptr>& getPragmas() { return __set_pragma; }
    
private:
    bool _allow_cycles;
    bool _exponential_paths;
    pragma_force_multiserver _force_multiserver;
    bool _interlock;
    pragma_layering  _layering;
    pragma_multiserver _multiserver;
    pragma_mva _mva;
    pragma_overtaking _overtaking;
    scheduling_type _processor_scheduling;
#if BUG_270
    bool _prune;
#endif
#if HAVE_LIBGSL && HAVE_LIBGSLCBLAS
    pragma_quorum_distribution _quorum_distribution; 
    pragma_quorum_delayed_calls _quorum_delayed_calls;
    pragma_quorum_idle_time _quorum_idle_time;           
#endif
#if RESCHEDULE
    bool _reschedule_on_async_send;
#endif
    LQIO::severity_t _severity_level;
    bool _spex_header;
    double _stop_on_bogus_utilization;
    bool _stop_on_message_loss;
    unsigned  _tau;
    pragma_threads _threads;
    pragma_variance _variance;
    /* bonus */
    bool _default_processor_scheduling;
    bool _init_variance_only;
    bool _entry_variance;
    
    /* --- */

    static Pragma * __cache;
    static std::map<std::string,Pragma::fptr> __set_pragma;

    static std::map<std::string,LQIO::severity_t> __serverity_level_pragma;
    static std::map<std::string,pragma_force_multiserver> __force_multiserver;
    static std::map<std::string,pragma_layering> __layering_pragma;
    static std::map<std::string,pragma_multiserver> __multiserver_pragma;
    static std::map<std::string,pragma_mva> __mva_pragma;
    static std::map<std::string,pragma_overtaking> __overtaking_pragma;
#if HAVE_LIBGSL && HAVE_LIBGSLCBLAS
    static std::map<std::string,pragma_quorum_distribution> __quorum_distribution_pragma;
    static std::map<std::string,pragma_quorum_delayed_calls> __quorum_delayed_calls_pragma;
    static std::map<std::string,pragma_quorum_idle_time> __quorum_idle_time_pragma;
#endif
    static std::map<std::string,pragma_threads> __threads_pragma;
    static std::map<std::string,pragma_variance> __variance_pragma;
    static std::map<std::string,scheduling_type> __processor_scheduling_pragma;
};
#endif
