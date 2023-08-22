/* -*- c++ -*-
 *
 * Everything you wanted to know about an entry, but were afraid to ask.
 *
 * Copyright the Real-Time and Distributed Systems Group,
 * Department of Systems and Computer Engineering,
 * Carleton University, Ottawa, Ontario, Canada. K1S 5B6
 *
 * November, 1994
 *
 * $Id: phase.h 16787 2023-07-17 14:22:14Z greg $
 *
 * ------------------------------------------------------------------------
 */

#if	!defined(PHASE_H)
#define PHASE_H

#include <lqio/bcmp_document.h>
#include <lqio/dom_phase.h>

class Call;
class Entry;
class Entity;
class Task;

class Phase {

public:
    class Histogram {
    public:
	class Bin {
	public:
	    Bin( double begin, double end, double prob, double conf95, double conf99 )
	    //		: myBegin(begin), myEnd(end), myProb(prob), myConf95(conf95), myConf99(conf99)
		{}
	    
	private:
	    // const double myBegin;    
	    // const double myEnd;
	    // const double myProb;
	    // const double myConf95;
	    // const double myConf99;
	};

	Histogram();
	Phase::Histogram& set( const double min, const double max, const unsigned n_bins );
	Phase::Histogram& moments( const double, const double, const double, const double );
	Phase::Histogram& addBin( const double begin, const double end, const double prob, const double conf95, const double conf99 );
	bool hasHistogram() const { return myNumBins > 2 && myMax > 0; }
	bool hasMaxServiceTime() const { return myNumBins == 2 && myMax > 0; }

    private:
	unsigned myNumBins;		/* Number of bins */
	double myMin;                   /* Lower range limit on the histogram */
	double myMax;                   /* Upper range limit on the histogram */

	double myMean;			/* Distribution results. */
	double myStdDev;
	double mySkew;
	double myKurtosis;

	std::vector<Phase::Histogram::Bin> bins;	/* */
    };

    struct mem_fn
    {
	typedef bool (Phase::*predicate)() const;
	mem_fn( const predicate p ) : _p(p) {};
	bool operator()( const std::pair<unsigned,Phase>& phase ) const { return (phase.second.*_p)(); }
    private:
	const predicate _p;
    };
    

private:
    struct SetChain {
	SetChain( const unsigned k, const Entity * server, callPredicate f ) : _k(k), _server(server), _f(f) {}
	void operator()( Call * ) const;
    private:
	const unsigned _k;
	const Entity * _server;
	callPredicate _f;
    };


public:
    Phase();
    Phase( const Phase& );
    virtual ~Phase();
    Phase& operator=( const Phase& );
	
    int operator==( const Phase& aPhase ) const { return &aPhase == this; }

    /* Initialialization */
	
    void initCv_sqr( const double );
    Phase& initialize( Entry * src, const unsigned p );
    virtual bool check() const;
    const Phase& setChain( const unsigned curr_k, const Entity * aServer, callPredicate aFunc ) const;

    /* Instance variable access */
	
    Phase& setDOM( const LQIO::DOM::Phase * dom ) { _dom = dom; return *this; }
    virtual const LQIO::DOM::Phase * getDOM() const { return _dom; }
    int phase() const { return _phase; }
    const Entry * entry() const { return _entry; }
    virtual const Task * owner() const;

    virtual const std::string& name() const;

    Phase& phaseTypeFlag( LQIO::DOM::Phase::Type aType );
    LQIO::DOM::Phase::Type phaseTypeFlag() const;

    bool hasServiceTime() const;
    const LQIO::DOM::ExternalVariable& serviceTime() const;

    bool hasThinkTime() const;
    const LQIO::DOM::ExternalVariable& thinkTime() const;

    bool hasCV_sqr() const;
    const LQIO::DOM::ExternalVariable& Cv_sqr() const;

    double residenceTime() const;
    double variance() const;
    double serviceExceeded() const;
    double queueingTime() const;
    double utilization() const;
    static LQX::SyntaxTreeNode * accumulate_service_time( LQX::SyntaxTreeNode *, const std::pair<unsigned int, Phase>& );
    static LQX::SyntaxTreeNode * accumulate_think_time( LQX::SyntaxTreeNode *, const std::pair<unsigned int, Phase>& );
    static BCMP::Model::Station::Class accumulate_demand( const BCMP::Model::Station::Class& augend, const std::pair<unsigned,Phase>& );
    static double accumulate_execution( double, const std::pair<unsigned int, Phase>& );

    bool hasQueueingTime() const;
    bool isNonExponential() const;
    bool isDeterministic() const { return phaseTypeFlag() == LQIO::DOM::Phase::Type::DETERMINISTIC; }

    bool hasHistogram() const { return _histogram.hasHistogram(); }
    Phase& histogram( const double min, const double max, const unsigned n_bins );
    Phase& moments( const double, const double, const double, const double );
    Phase& histogramBin( const double begin, const double end, const double prob, const double conf95, const double conf99 );
    double maxServiceTime() const;
    bool hasMaxServiceTime() const { return _histogram.hasMaxServiceTime(); }

    virtual const std::vector<Call *>& calls() const;
    virtual const Entry * rootEntry() const { return 0; }

    static void merge( LQIO::DOM::Phase&, const LQIO::DOM::Phase&, double rate );
    virtual double serviceTimeForSRVNInput() const;

#if defined(REP2FLAT)
    virtual Phase& replicatePhase( LQIO::DOM::Phase * root, unsigned int replica );
    virtual Phase& replicateCall();
#endif

protected:
    Phase& recomputeCv_sqr( const Phase * );

protected:
    Histogram _histogram;		/* Histogram information	*/

private:
    const LQIO::DOM::Phase * _dom;
    Entry *_entry;
    unsigned _phase;
};


inline std::ostream& operator<<( std::ostream& output, const Phase& ) { return output; }
inline std::ostream& operator<<( std::ostream& output, const Phase::Histogram::Bin& ) { return output; }
#endif
