/* -*- c++ -*-
 * $HeadURL$
 *
 * Pragma processing.
 *
 * Copyright the Real-Time and Distributed Systems Group,
 * Department of Systems and Computer Engineering,
 * Carleton University, Ottawa, Ontario, Canada. K1S 5B6
 *
 * November, 1994
 *
 * $Id$
 *
 * ------------------------------------------------------------------------
 */

#if	!defined(PRAGMA_H)
#define	PRAGMA_H

#include "dim.h"
#include <map>
#include <string>
#include <lqio/input.h>
#include <lqio/dom_document.h>
#include "help.h"
#include "ph2serv.h"

typedef enum { THROUGHPUT_INTERLOCK, NO_INTERLOCK } PRAGMA_INTERLOCK;
typedef enum { BACKPROPOGATE_LAYERS, BATCHED_LAYERS, METHOD_OF_LAYERS, BACKPROPOGATE_METHOD_OF_LAYERS, SRVN_LAYERS, SQUASHED_LAYERS, HWSW_LAYERS } PRAGMA_LAYERING;
typedef	enum { LINEARIZER_MVA, EXACT_MVA, SCHWEITZER_MVA, FAST_MVA, ONESTEP_MVA, ONESTEP_LINEARIZER } PRAGMA_MVA;
typedef enum { DEFAULT_MULTISERVER, CONWAY_MULTISERVER, REISER_MULTISERVER, REISER_PS_MULTISERVER, ROLIA_MULTISERVER, ROLIA_PS_MULTISERVER, BRUELL_MULTISERVER, SCHMIDT_MULTISERVER, SURI_MULTISERVER } PRAGMA_MULTISERVER;
typedef enum { MARKOV_OVERTAKING, ROLIA_OVERTAKING, SIMPLE_OVERTAKING, SPECIAL_OVERTAKING, NO_OVERTAKING } PRAGMA_OVERTAKING;
typedef enum { NO_ASYNC_RESCHEDULE, ASYNC_RESCHEDULE } PRAGMA_RESCHEDULE;
typedef enum { DEFAULT_VARIANCE, NO_VARIANCE, STOCHASTIC_VARIANCE, MOL_VARIANCE, ENTRY_VARIANCE, INIT_VARIANCE_ONLY } PRAGMA_VARIANCE;
typedef enum { DEFAULT_PROCESSOR, PROCESSOR_FCFS, PROCESSOR_HOL, PROCESSOR_PR, PROCESSOR_PS, PROCESSOR_PS_HOL, PROCESSOR_PS_PPR } PRAGMA_PROCESSOR;
typedef enum { MAK_LUNDSTROM_THREADS, HYPER_THREADS, EXPONENTIAL_THREADS, NO_THREADS } PRAGMA_THREADS;
typedef enum { DISALLOW_CYCLES, ALLOW_CYCLES } PRAGMA_CYCLES;
#if HAVE_LIBGSL && HAVE_LIBGSLCBLAS
typedef enum { DEFAULT_QUORUM_DISTRIBUTION, THREEPOINT_QUORUM_DISTRIBUTION,
	       GAMMA_QUORUM_DISTRIBUTION, CLOSEDFORM_GEOMETRIC_QUORUM_DISTRIBUTION,
	       CLOSEDFORM_DETRMINISTIC_QUORUM_DISTRIBUTION } PRAGMA_QUORUM_DISTRIBUTION;
typedef enum { DEFAULT_QUORUM_DELAYED_CALLS,KEEP_ALL_QUORUM_DELAYED_CALLS,
	       ABORT_ALL_QUORUM_DELAYED_CALLS, ABORT_LOCAL_ONLY_QUORUM_DELAYED_CALLS,
	       ABORT_REMOTE_ONLY_QUORUM_DELAYED_CALLS } PRAGMA_QUORUM_DELAYED_CALLS;
typedef enum { DEFAULT_IDLETIME, JOINDELAY_IDLETIME, ROOTENTRY_IDLETIME } PRAGMA_IDLETIME;
#endif

/* -------------------------------------------------------------------- */

class Pragma {
public:

    typedef bool (Pragma::*set_pragma_fptr)( const string& );
    typedef const char * (Pragma::*get_pragma_fptr)() const;
    typedef bool (Pragma::*eq_pragma_fptr)( const Pragma& ) const;

    struct param_info {
	param_info() : _i(0), _h(0) {}
	param_info( unsigned int i, Help::help_fptr h ) : _i(i), _h(h) {}
	unsigned int _i;
	Help::help_fptr _h;
    };

    struct pragma_info {
	pragma_info() : _set(0), _get(0), _eq(0), _help(0), _value(0) {}
	pragma_info( set_pragma_fptr f, get_pragma_fptr g, eq_pragma_fptr e, Help::help_fptr h, std::map<const char *, param_info, lt_str>* a=0 ) : _set(f), _get(g), _eq(e), _help(h), _value(a) {}
	set_pragma_fptr _set;
	get_pragma_fptr _get;
	eq_pragma_fptr _eq;
	Help::help_fptr _help;
	std::map<const char *, param_info, lt_str>* _value;
    };

    struct eq_pragma_value
    {
	eq_pragma_value( const unsigned int v ) : _v(v) {}
	bool operator()( std::pair<const char* const, Pragma::param_info>& v ) const { return v.second._i == _v; }
    private:
	const unsigned int _v;
    };

    Pragma();
    virtual ~Pragma() {}

    bool operator==( const Pragma& ) const;
    bool operator!=( const Pragma& p ) const { return !(*this == p); }

    PRAGMA_CYCLES getCycles() const { return _cycles; }
    Pragma& setCycles( PRAGMA_CYCLES cycles ) { _cycles = cycles; return *this; }
    bool eqCycles( const Pragma& p ) const { return _cycles == p._cycles; }

    bool getStopOnMessageLoss() const { return _stopOnMessageLoss; }
    Pragma& setStopOnMessageLoss( const bool stopOnMessageLoss ) { _stopOnMessageLoss = stopOnMessageLoss; return *this; }
    bool eqStopOnMessageLoss( const Pragma& p ) const { return _stopOnMessageLoss == p._stopOnMessageLoss; }

    PRAGMA_INTERLOCK getInterlock() const { return _interlock; }
    Pragma& setInterlock( PRAGMA_INTERLOCK interlock ) { _interlock = interlock; return *this; }
    bool eqInterlock( const Pragma& p ) const { return _interlock == p._interlock; }

    PRAGMA_LAYERING getLayering() const { return _layering; }
    Pragma& setLayering( PRAGMA_LAYERING layering ) {  _layering = layering; return *this; }
    bool eqLayering( const Pragma& p ) const { return _layering == p._layering; }

    PRAGMA_MULTISERVER getMultiserver() const { return _multiserver; }
    Pragma& setMultiserver( PRAGMA_MULTISERVER multiserver ) { _multiserver = multiserver; return *this; }
    bool eqMultiserver( const Pragma& p ) const { return _multiserver == p._multiserver; }

    PRAGMA_MVA getMVA() const { return _mva; }
    Pragma& setMVA( PRAGMA_MVA mva ) { _mva = mva; return *this; }
    bool eqMVA( const Pragma& p ) const { return _mva == p._mva; }

#if HAVE_LIBGSL && HAVE_LIBGSLCBLAS
/// start tomari quorum

    PRAGMA_QUORUM_DISTRIBUTION getQuorumDistribution() const { return _quorumDistribution; }
    Pragma& setQuorumDistribution( PRAGMA_QUORUM_DISTRIBUTION quorumDistribution) const { _quorumDistribution = quorumDistribution; return *this; }
    bool eqQuorumDistribution( const Pragma& p ) const { return _quorumDistribution == p._quorumDistribution; }

    PRAGMA_QUORUM_DELAYED_CALLS getQuorumDelayedCalls() const { return _quorumDelayedCalls; }
    Pragma& setQuorumDelayedCalls( PRAGMA_QUORUM_DELAYED_CALLS quorumDelayedCalls ) const { _quorumDelayedCalls = quorumDelayedCalls; return *this; }
    bool eqQuorumDelayedCalls( const Pragma& p ) const { return _quorumDelayedCalls == p._quorumDelayedCalls; }

    PRAGMA_IDLETIME getIdleTime() const { return _idletime; }
    Pramga&  setIdleTime( PRAGMA_IDLETIME idleTime ) const { _idletime = idleTime; return *this; }
    bool eqIdleTime( const Pragma& p ) const { return _idleTime == p._idleTime; }
//// end tomari quorum idle time
#endif

    PRAGMA_OVERTAKING getOvertaking () const { return _overtaking; }
    Pragma& setOvertaking ( PRAGMA_OVERTAKING overtaking ) { _overtaking = overtaking; return *this; }
    bool eqOvertaking( const Pragma& p ) const { return _overtaking == p._overtaking; }
    bool eqPhase2Correction( const Pragma& p ) const { return _phase2_correction == p._phase2_correction; }

    static const scheduling_type processor_scheduling[];
    PRAGMA_PROCESSOR getProcessor() const { return _processor; }
    scheduling_type getProcessorScheduling() const;
    Pragma& setProcessor( PRAGMA_PROCESSOR processor ) { _processor = processor;  return *this; }
    bool eqProcessor( const Pragma& p ) const { return _processor == p._processor; }

#if RESCHEDULE
    PRAGMA_RESCHEDULE  getReschedule() const { return _reschedule; }
    Pragma& setReschedule( PRAGMA_RESCHEDULE reschedule) { _reschedule = reschedule; return *this; }
    bool eqReschedule( const Pragma& p ) const { return _reschedule == p._reschedule; }
#endif

    unsigned getTau() const { return _tau; }
    Pragma& setTau( unsigned tau ) { _tau = tau; return *this; }
    bool eqTau( const Pragma& p ) const { return _tau == p._tau; }

    PRAGMA_THREADS getThreads() const { return _threads; }
    Pragma& setThreads( PRAGMA_THREADS threads ) { _threads = threads;  return *this; }
    bool eqThreads( const Pragma& p ) const { return _threads == p._threads; }
    bool eqExponentialPaths( const Pragma& p ) const { return _exponential_paths == p._exponential_paths; }
    bool exponential_paths() const { return _exponential_paths; }

    PRAGMA_VARIANCE getVariance() const { return _variance; }
    Pragma& setVariance( PRAGMA_VARIANCE variance ) { _variance = variance; return *this; }
    bool eqVariance( const Pragma& p ) const { return _variance == p._variance; }
    bool eqEntryVariance( const Pragma& p ) const { return _entry_variance == p._entry_variance; }
    bool eqInitVarianceOnly( const Pragma& p ) const { return _init_variance_only == p._init_variance_only; }
    bool entry_variance() const { return _entry_variance; }
    bool init_variance_only() const { return _init_variance_only; }

    bool getSeverityLevel() const { return _severity_level; }
    Pragma& setSeverityLevel( const LQIO::severity_t );
    bool eqSeverityLevel( const Pragma& p ) const { return _severity_level == p._severity_level; }

    bool set( const string&, const string& );
    void updateDOM( LQIO::DOM::Document * ) const;

public:
    static void initialize();
    static ostream& usage( ostream&  );

private:
    Pragma( const Pragma& );		/* NO copy constructor */

    const char * getCyclesStr() const;
    bool setCyclesTo( const string& );

    const char * getStopOnMessageLossStr() const;
    bool setStopOnMessageLossTo( const string& );

    const char * getInterlockStr() const;
    bool setInterlockTo( const string& );

    const char * getLayeringStr() const;
    bool setLayeringTo( const string& );

    const char * getMultiserverStr() const;
    bool setMultiserverTo( const string& );

    const char * getMVAStr() const;
    bool setMVATo( const string& );

#if HAVE_LIBGSL && HAVE_LIBGSLCBLAS
    bool setQuorumDistributionTo( const string& );
    bool setQuorumDelayedCallsTo( const string& );
    bool setIdleTimeTo( const string& );
#endif

    const char * getOvertakingStr() const;
    bool setOvertakingTo( const string& );

    const char * getProcessorStr() const;
    bool setProcessorTo( const string& );

    const char * getRescheduleStr() const;
    bool setRescheduleTo( const string& );

    const char * getTauStr() const;
    bool setTauTo( const string& );

    const char * getThreadsStr() const;
    bool setThreadsTo( const string& );

    const char * getVarianceStr() const;
    bool setVarianceTo( const string& );

    const char * getSeverityLevelStr() const;
    bool setSeverityLevelTo( const string& );

    static bool is_true( const string& );

public:
    static const std::map<const char *, pragma_info, lt_str>& getPragmas() { return __pragmas; }

private:
    PRAGMA_CYCLES _cycles;
    bool _stopOnMessageLoss;
    PRAGMA_INTERLOCK  _interlock;
    PRAGMA_LAYERING  _layering;
    PRAGMA_MULTISERVER  _multiserver;
    PRAGMA_MVA  _mva;
#if HAVE_LIBGSL && HAVE_LIBGSLCBLAS
    PRAGMA_QUORUM_DISTRIBUTION  _quorumDistribution;
    PRAGMA_QUORUM_DELAYED_CALLS  _quorumDelayedCalls;
    PRAGMA_IDLETIME  _idletime;
#endif
    PRAGMA_OVERTAKING  _overtaking;
    PRAGMA_PROCESSOR  _processor;
#if RESCHEDULE
    PRAGMA_RESCHEDULE   _reschedule;
#endif
    unsigned  _tau;
    string _tau_str;
    PRAGMA_THREADS  _threads;
    PRAGMA_VARIANCE  _variance;

    /* Special variables (for save/restore) */
    PHASE2_CORRECTION _phase2_correction;
    bool _entry_variance;
    bool _init_variance_only;
    bool _exponential_paths;
    LQIO::severity_t _severity_level;

    static std::map<const char *, param_info, lt_str>  __cycles_args;
    static std::map<const char *, param_info, lt_str>  __stop_on_message_loss_args;
    static std::map<const char *, param_info, lt_str>  __interlock_args;
    static std::map<const char *, param_info, lt_str>  __layering_args;
    static std::map<const char *, param_info, lt_str>  __multiserver_args;
    static std::map<const char *, param_info, lt_str>  __mva_args;
    static std::map<const char *, param_info, lt_str>  __overtaking_args;
    static std::map<const char *, param_info, lt_str>  __processor_args;
    static std::map<const char *, param_info, lt_str>  __threads_args;
    static std::map<const char *, param_info, lt_str>  __variance_args;
    static std::map<const char *, param_info, lt_str>  __warning_args;
#if HAVE_LIBGSL && HAVE_LIBGSLCBLAS
    static std::map<const char *, param_info, lt_str>  __quorum_distribution_args;
    static std::map<const char *, param_info, lt_str>  __quorum_delayed_calls_args;
    static std::map<const char *, param_info, lt_str>  __idle_time_args;
#endif

private:
    static std::map<const char *, pragma_info, lt_str> __pragmas;
};

ostream& operator<<( ostream&, Pragma& );

extern Pragma pragma;
#endif
