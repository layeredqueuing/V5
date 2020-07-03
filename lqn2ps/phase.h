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
 * $Id: phase.h 13477 2020-02-08 23:14:37Z greg $
 *
 * ------------------------------------------------------------------------
 */

#if	!defined(PHASE_H)
#define PHASE_H

#include <lqio/input.h>

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
    virtual ~Phase();
    Phase& operator=( const Phase& );
    Phase( const Phase& );		/* Copying is verbotten */ 
	
    int operator==( const Phase& aPhase ) const { return &aPhase == this; }

    /* Initialialization */
	
    void initCv_sqr( const double );
    Phase& initialize( Entry * src, const unsigned p );
    virtual bool check() const;
    const Phase& setChain( const unsigned curr_k, const Entity * aServer, callPredicate aFunc ) const;

    /* Instance variable access */
	
    Phase& setDOM( const LQIO::DOM::Phase * dom ) { _documentObject = dom; return *this; }
    virtual const LQIO::DOM::Phase * getDOM() const { return _documentObject; }
    int phase() const { return _phase; }
    const Entry * entry() const { return _entry; }
    virtual const Task * owner() const;

    virtual const string& name() const;

    Phase& phaseTypeFlag( phase_type aType );
    phase_type phaseTypeFlag() const;

    bool hasServiceTime() const;
    const LQIO::DOM::ExternalVariable& serviceTime() const;

    bool hasThinkTime() const;
    const LQIO::DOM::ExternalVariable& thinkTime() const;

    bool hasCV_sqr() const;
    const LQIO::DOM::ExternalVariable& Cv_sqr() const;

    double executionTime() const;
    double variance() const;
    double serviceExceeded() const;
    double queueingTime() const;
    double utilization() const;

    bool hasQueueingTime() const;
    bool isNonExponential() const;
    bool isDeterministic() const { return phaseTypeFlag() == PHASE_DETERMINISTIC; }

    bool hasHistogram() const { return _histogram.hasHistogram(); }
    Phase& histogram( const double min, const double max, const unsigned n_bins );
    Phase& moments( const double, const double, const double, const double );
    Phase& histogramBin( const double begin, const double end, const double prob, const double conf95, const double conf99 );
    double maxServiceTime() const;
    bool hasMaxServiceTime() const { return _histogram.hasMaxServiceTime(); }

    virtual const std::vector<Call *>& calls() const;
    virtual const Entry * rootEntry() const { return 0; }

    virtual double serviceTimeForSRVNInput() const;

#if defined(REP2FLAT)
    virtual Phase& replicatePhase();
    virtual Phase& replicateCall();
#endif

protected:
    Phase& recomputeCv_sqr( const Phase * );

protected:
    Histogram _histogram;		/* Histogram information	*/

private:
    const LQIO::DOM::Phase * _documentObject;
    Entry *_entry;
    unsigned _phase;
};


inline ostream& operator<<( ostream& output, const Phase& ) { return output; }
inline ostream& operator<<( ostream& output, const Phase::Histogram::Bin& ) { return output; }

template <> struct Predicate<Phase>
{
    typedef bool (Phase::*predicate)() const;
    Predicate<Phase>( const predicate p ) : _p(p) {};
    bool operator()( const std::pair<unsigned,Phase>& phase ) const { return (phase.second.*_p)(); }
private:
    const predicate _p;
};

template <class Type2> struct Sum<Phase,Type2>
{
    typedef Type2 (Phase::*funcPtr)() const;
    Sum<Phase,Type2>( funcPtr f ) : _f(f), _sum(0) {}
    void operator()( const std::pair<unsigned,Phase>& phase ) { _sum += (phase.second.*_f)(); }
    Type2 sum() const { return _sum; }
private:
    funcPtr _f;
    Type2 _sum;
};

template <> struct Sum<Phase,LQIO::DOM::ExternalVariable>
{
    typedef const LQIO::DOM::ExternalVariable& (Phase::*funcPtr)() const;
    Sum<Phase,LQIO::DOM::ExternalVariable>( funcPtr f ) : _f(f), _sum(0) {}
    void operator()( const std::pair<unsigned,Phase>& phase ) { _sum += LQIO::DOM::to_double((phase.second.*_f)()); }
    double sum() const { return _sum; }
private:
    funcPtr _f;
    double _sum;
};
#endif
