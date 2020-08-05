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
 *
 * $Id: pragma.h 13735 2020-08-05 15:54:22Z greg $
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
    typedef enum { MAK_LUNDSTROM_THREADS, HYPER_THREADS, NO_THREADS } pragma_threads;

private:
    Pragma();
    virtual ~Pragma() 
	{
	    __cache = nullptr;
	}

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
    void setInterlock(const std::string&);
    void setLayering(const std::string&);
    void setMultiserver(const std::string&);
    void setMva(const std::string&);
    void setOvertaking(const std::string&);
    void setProcessorScheduling(const std::string&);
    void setSeverityLevel(const std::string&);
    void setSpexHeader(const std::string&);
    void setStopOnMessageLoss(const std::string&);
    void setTau(const std::string&);
    void setThreads(const std::string&);
    void setVariance(const std::string&);
    

    static bool isTrue(const std::string&);

#if HAVE_LIBGSL && HAVE_LIBGSLCBLAS
/// start tomari quorum

    PRAGMA_QUORUM_DISTRIBUTION getQuorumDistribution() const { return _quorumDistribution; }
    PRAGMA_QUORUM_DELAYED_CALLS getQuorumDelayedCalls() const { return _quorumDelayedCalls; }
    PRAGMA_IDLETIME getIdleTime() const { return _idletime; }
//// end tomari quorum idle time
#endif


#if RESCHEDULE
    PRAGMA_RESCHEDULE  getReschedule() const { return _reschedule; }
#endif

public:
    static void set( const std::map<std::string,std::string>& );
    static void initialize();
    static ostream& usage( ostream&  );
    static const std::map<std::string,Pragma::fptr>& getPragmas() { return __set_pragma; }
    
private:
    bool _allow_cycles;
    bool _exponential_paths;
    bool _interlock;
    pragma_layering  _layering;
    pragma_multiserver _multiserver;
    pragma_mva _mva;
    pragma_overtaking _overtaking;
    bool _default_processor_scheduling;
    scheduling_type _processor_scheduling;
    bool _spex_header;
    bool _stop_on_message_loss;
    unsigned  _tau;
    pragma_threads _threads;
    pragma_variance _variance;
    bool _init_variance_only;
    bool _entry_variance;
    LQIO::severity_t _severity_level;
    
    /* --- */
    

 #if HAVE_LIBGSL && HAVE_LIBGSLCBLAS
    PRAGMA_QUORUM_DISTRIBUTION  _quorumDistribution;
    PRAGMA_QUORUM_DELAYED_CALLS  _quorumDelayedCalls;
    PRAGMA_IDLETIME  _idletime;
#endif
#if RESCHEDULE
    PRAGMA_RESCHEDULE   _reschedule;
#endif

    static Pragma * __cache;
    static std::map<std::string,Pragma::fptr> __set_pragma;
};
#endif
