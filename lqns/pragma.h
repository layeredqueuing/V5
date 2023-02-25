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
 * $Id: pragma.h 16441 2023-02-23 21:39:47Z greg $
 *
 * ------------------------------------------------------------------------
 */

#if	!defined(PRAGMA_H)
#define	PRAGMA_H

#include <map>
#include <lqio/input.h>
#include <lqio/dom_pragma.h>
#include "help.h"

extern LQIO::DOM::Pragma pragmas;

/* -------------------------------------------------------------------- */

class Pragma {
    friend class Generate;
    
public:
    typedef void (Pragma::*fptr)(const std::string&);

    enum class ForceInfinite { NONE, FIXED_RATE, MULTISERVERS, ALL };
    enum class ForceMultiserver { NONE, PROCESSORS, TASKS, ALL };
    enum class Layering { BACKPROPOGATE_BATCHED, BATCHED, METHOD_OF_LAYERS, BACKPROPOGATE_METHOD_OF_LAYERS, SRVN, SQUASHED, HWSW };
    enum class MVA { LINEARIZER, EXACT, SCHWEITZER, FAST, ONESTEP, ONESTEP_LINEARIZER };
    enum class Multiserver { DEFAULT, CONWAY, EXPERIMENTAL, REISER, REISER_PS, ROLIA, ROLIA_PS, BRUELL, SCHMIDT, SURI, ZHOU };
    enum class Overtaking { MARKOV, ROLIA, SIMPLE, SPECIAL, NONE };
#if HAVE_LIBGSL && HAVE_LIBGSLCBLAS
    enum class QuorumDistribution { DEFAULT, THREEPOINT, GAMMA, CLOSEDFORM_GEOMETRIC, CLOSEDFORM_DETRMINISTIC };
    enum class QuorumDelayedCalls { DEFAULT, KEEP_ALL, ABORT_ALL, ABORT_LOCAL_ONLY, ABORT_REMOTE_ONLY };
    enum class QuorumIdleTime { DEFAULT, JOINDELAY, ROOTENTRY };
#endif
    enum class Replication { EXPAND, PRUNE, PAN };
    enum class Threads { MAK_LUNDSTROM, HYPER, NONE };
    enum class Variance { DEFAULT, NONE, STOCHASTIC, MOL };

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

    static unsigned int convergenceValue()
	{
	    assert( __cache != nullptr );
	    return __cache->_convergence_value;
	}

    static bool exponential_paths()
	{
	    assert( __cache != nullptr );
	    return __cache->_exponential_paths;
	}

    static bool forceInfinite( Pragma::ForceInfinite arg )
	{
	    assert( __cache != nullptr );
	    return (__cache->_force_infinite != ForceInfinite::NONE && arg == __cache->_force_infinite)
		|| (__cache->_force_infinite == ForceInfinite::ALL  && arg != ForceInfinite::NONE );
	}

    static bool forceMultiserver( Pragma::ForceMultiserver arg )
	{
	    assert( __cache != nullptr );
	    return (__cache->_force_multiserver != ForceMultiserver::NONE && arg == __cache->_force_multiserver)
		|| (__cache->_force_multiserver == ForceMultiserver::ALL  && arg != ForceMultiserver::NONE );
	}

    static bool interlock()
	{
	    assert( __cache != nullptr );
	    return __cache->_interlock;
	}

    static unsigned int iterationLimit()
	{
	    assert( __cache != nullptr );
	    return __cache->_iteration_limit;
	}
    
    static const std::string& getLayeringStr();
    
    static Layering layering()
	{
	    assert( __cache != nullptr );
	    return __cache->_layering;
	}

    static double molUnderrelaxation()
	{
	    assert( __cache != nullptr );
	    return __cache->_mol_underrelaxation;
	}
    
    static Multiserver multiserver()
	{
	    assert( __cache != nullptr );
	    return __cache->_multiserver;
	}

    static MVA mva()
	{
	    assert( __cache != nullptr );
	    return __cache->_mva;
	}

    static Overtaking overtaking()
	{
	    assert( __cache != nullptr );
	    return __cache->_overtaking;
	}

    static bool overtaking( Overtaking arg )
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
    static QuorumDistribution getQuorumDistribution()
	{
	    assert( __cache != nullptr );
	    return __cache->_quorum_distribution;
	}

    static QuorumDelayedCalls getQuorumDelayedCalls()
	{
	    assert( __cache != nullptr );
	    return __cache->_quorum_delayed_calls;
	}
    static QuorumIdleTime getQuorumIdleTime()
	{
	    assert( __cache != nullptr );
	    return __cache->_quorum_idle_time;
	}
#endif

    static Replication replication()
	{
	    assert( __cache != nullptr );
	    return __cache->_replication;
	}

    static bool pan_replication() { return replication() == Replication::PAN; }
    
#if RESCHEDULE
    static bool getRescheduleOnAsyncSend()
	{
	    assert( __cache != nullptr );
	    return __cache->_reschedule_on_async_send;
	}
#endif

    static bool saveMarginalProbabilities()
	{
	    return __cache->_save_marginal_probabilities;
	}
    
    static LQIO::error_severity severityLevel()
	{
	    assert( __cache != nullptr );
	    return __cache->_severity_level;
	}

    static bool spexComment()
	{
	    assert( __cache != nullptr );
	    return __cache->_spex_comment;
	}

    static bool spexConvergence()
	{
	    assert( __cache != nullptr );
	    return __cache->_spex_convergence;
	}

    static bool spexHeader()
	{
	    assert( __cache != nullptr );
	    return __cache->_spex_header;
	}

    static unsigned int spexIterationLimit()
	{
	    assert( __cache != nullptr );
	    return __cache->_spex_iteration_limit;
	}
    
    static double spexUnderrelaxation()
	{
	    assert( __cache != nullptr );
	    return __cache->_spex_underrelaxation;
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

    static scheduling_type taskScheduling()
	{
	    assert( __cache != nullptr );
	    return __cache->_task_scheduling;
	}

    static bool defaultTaskScheduling()
	{
	    assert( __cache != nullptr );
	    return __cache->_default_task_scheduling;
	}

    static double tau()
	{
	    assert( __cache != nullptr );
	    return __cache->_tau;
	}

    static Threads threads()
	{
	    assert( __cache != nullptr );
	    return __cache->_threads;
	}

    static bool threads( Threads arg )
	{
	    return threads() == arg;
	}

    static double underrelaxation()
	{
	    assert( __cache != nullptr );
	    return __cache->_underrelaxation;
	}
    
    static Variance variance()
	{
	    assert( __cache != nullptr );
	    return __cache->_variance;
	}

    static bool variance( Variance arg )
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
    void setConvergenceValue(const std::string&);
    void setExponential_paths(const std::string&);
    void setForceInfinite(const std::string&);
    void setForceMultiserver(const std::string&);
    void setInterlock(const std::string&);
    void setIterationLimit(const std::string&);
    void setLayering(const std::string&);
    void setMOLUnderrelaxation(const std::string&);
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
    void setReplication(const std::string&);
#if RESCHEDULE
    void setRescheduleOnAsyncSend(const std::string&);
#endif
    void setSaveMarginalProbabilities(const std::string&);
    void setSeverityLevel(const std::string&);
    void setSpexComment(const std::string&);
    void setSpexConvergence(const std::string&);
    void setSpexHeader(const std::string&);
    void setSpexIterationLimit(const std::string&);
    void setSpexUnderrelaxation(const std::string&);
    void setStopOnBogusUtilization(const std::string&);
    void setStopOnMessageLoss(const std::string&);
    void setTaskScheduling(const std::string&);
    void setTau(const std::string&);
    void setThreads(const std::string&);
    void setUnderrelaxation(const std::string&);
    void setVariance(const std::string&);

public:
    static void set( const std::map<std::string,std::string>& );
    static std::ostream& usage( std::ostream&  );
    static const std::map<const std::string,const Pragma::fptr>& getPragmas() { return __set_pragma; }

private:
    bool _allow_cycles;
    double _convergence_value;
    bool _exponential_paths;
    ForceInfinite _force_infinite;
    ForceMultiserver _force_multiserver;
    bool _interlock;
    unsigned int _iteration_limit;
    Layering  _layering;
    double _mol_underrelaxation;
    Multiserver _multiserver;
    MVA _mva;
    Overtaking _overtaking;
    scheduling_type _processor_scheduling;
#if BUG_270
    bool _prune;
#endif
#if HAVE_LIBGSL && HAVE_LIBGSLCBLAS
    QuorumDistribution _quorum_distribution;
    QuorumDelayedCalls _quorum_delayed_calls;
    QuorumIdleTime _quorum_idle_time;
#endif
    Replication _replication;
#if RESCHEDULE
    bool _reschedule_on_async_send;
#endif
    bool _save_marginal_probabilities;
    LQIO::error_severity _severity_level;
    bool _spex_comment;
    double _spex_convergence;
    bool _spex_header;
    unsigned int _spex_iteration_limit;
    double _spex_underrelaxation;
    double _stop_on_bogus_utilization;
    bool _stop_on_message_loss;
    scheduling_type _task_scheduling;
    unsigned  _tau;
    Threads _threads;
    double _underrelaxation;
    Variance _variance;
    /* bonus */
    bool _default_processor_scheduling;
    bool _default_task_scheduling;
    bool _entry_variance;
    bool _init_variance_only;

    /* --- */

    static Pragma * __cache;
    static const std::map<const std::string,const Pragma::fptr> __set_pragma;
    static const std::map<const std::string,const Pragma::Layering> __layering_pragma;
    static const std::map<const std::string,const Pragma::Multiserver> __multiserver_pragma;
};
#endif
