/*  -*- c++ -*-
 * MVA solvers: Exact, Bard-Schweitzer, Linearizer and Linearizer2.
 * Abstract superclass does no operation by itself.
 *
 * Copyright the Real-Time and Distributed Systems Group,
 * Department of Systems and Computer Engineering,
 * Carleton University, Ottawa, Ontario, Canada. K1S 5B6
 *
 * November, 1994
 * August, 2005
 *
 * $Id: mva.h 14871 2021-07-03 03:20:32Z greg $
 *
 * ------------------------------------------------------------------------
 */

#if	!defined(MVA_H)
#define	MVA_H
/* undef DEBUG_MVA 1 */

#include <vector>
#include "fpgoop.h"
#include "pop.h"
#include "prob.h"
#include "vector.h"

class MVA;
class Server;

std::ostream& operator<<( std::ostream &, MVA& );

/* -------------------------------------------------------------------- */

class MVA
{
    friend class Multi_Server;
    friend class Synch_Server;
    friend class Server;

    /* The following is defined in the MVA test suite and only used there. */
#if defined(TESTMVA)
    friend bool check( const int solverId, const MVA& solver, const unsigned );
    friend void special_check( std::ostream&, const MVA&, const unsigned );
#endif

protected:
    /* Thrown to indicate iteration limits in internal computations.  */
    class iteration_limit : public std::runtime_error
    {
    public:
	explicit iteration_limit( const std::string& aStr) : runtime_error( aStr ) {}
    };

    /*
     * This template provides access to a particular data type
     * (templated) that is indexed initially by population.  The
     * actual indicies for the population are decided by an input
     * function that works on population vectors.
     */

//Shorthand for [N][m][e][k]
    typedef std::vector<double ***> N_m_e_k;

protected:
    MVA( Vector<Server *>&, const Population &, const Vector<double>&, const Vector<unsigned>&, const Vector<double>* );

private:
    MVA( const MVA& ) = delete;
    MVA& operator=( const MVA& ) = delete;

public:
    virtual ~MVA();

    virtual void reset();
    virtual bool solve() = 0;
    virtual const char * getTypeName() const = 0;

    unsigned nChains() const { return K; }
    virtual double filter() const = 0;
    void setThreadChain(const unsigned k, const unsigned kk){ _isThread[k]=kk;}
    unsigned getThreadChain(const unsigned k) const { return _isThread[k];}

    virtual bool isExactMVA() const { return false; }
    virtual double sumOf_L_m( const Server& station, const Population &N, const unsigned j ) const;
    virtual double sumOf_SL_m( const Server& station, const Population &N, const unsigned j ) const;
    double sumOf_SU_m( const Server& station, const Population &N, const unsigned j ) const;
    double sumOf_SQ_m( const Server& station, const Population &N, const unsigned j ) const;
    double sumOf_rU_m( const Server& station, const Population &N, const unsigned j ) const;
    double sumOf_S2U_m( const Server& station, const Population &N, const unsigned j ) const;
    double sumOf_S2U_m( const Server& station, const unsigned e, const Population &N, const unsigned j ) const;
    double sumOf_S2_m( const Server& station, const Population &N, const unsigned j ) const;
    double sumOf_U_m( const Server& station, const Population& N, const unsigned j ) const;
    double sumOf_USPrOt_m( const Server& station, const unsigned e, const Probability& PrOt, const Population &N, const unsigned j ) const;
    double sumOf_U2_m( const Server& station, const unsigned k, const Population &N, const unsigned j ) const;
    double sumOf_U2_m( const Server& station, const Population &N, const unsigned j ) const;
    double sumOf_P( const Server& station, const Population &N, const unsigned j ) const;
    double sumOf_SP2( const Server& station, const Population &N, const unsigned j ) const;
    double sumOf_alphaP( const Server& station, const Population &N ) const;
    double PB(  const Server& station, const Population &N, const unsigned j ) const;
    double PB2( const Server& station, const Population &N, const unsigned j ) const;
    virtual Probability priorityInflation( const Server& station, const Population &N, const unsigned k ) const = 0;
    double thinkTime( const unsigned k ) const { return Z[k]; }
    double throughput( const unsigned k ) const;
    double throughput( const unsigned k, const Population& N ) const;
    double throughput( const Server&  ) const;
    double throughput( const Server&, const unsigned k ) const;
    double throughput( const Server&, const unsigned e, const unsigned k ) const;
    double entryThroughput( const Server&, const unsigned ) const;
    double normalizedThroughput( const Server&, const unsigned, const unsigned chainNum ) const;
    double utilization( const Server& station ) const { return utilization( station, NCust ); }
    double utilization( const Server&, const Population& N ) const;
    double utilization( const Server&, const unsigned k ) const;
    double utilization( const Server&, const unsigned k, const Population& N, const unsigned j ) const;
    double queueLength( const Server& station ) const { return queueLength( station, NCust ); }
    double queueLength( const Server&, const Population& N ) const;
    double queueLength( const Server&, const unsigned k ) const;
    double queueOnly( const Server&, const unsigned k, const Population& N, const unsigned j ) const;
    double responseTime( const Server&, const unsigned k ) const;
    double responseTime( const unsigned k ) const;

    Positive arrivalRate( const Server&, const unsigned e, const unsigned k, const Population& N ) const;
    double syncDelta( const Server&, const unsigned e, const unsigned k, const Population& N ) const;

    std::ostream& print( std::ostream& ) const;

    unsigned long iterations() const { return stepCount; }
    unsigned long waits() const { return waitCount; }
    unsigned long faults() const { return faultCount; }

    double nrFactor( const Server&, const unsigned, const unsigned ) const;

protected:
    void dimension( const size_t );
    bool dimension( std::vector<double **>&, const size_t );
    void setMaxP();
    virtual const PopulationMap& getMap() const = 0;
    unsigned offset( const Population& N ) const { return getMap().offset( N ); }
    unsigned offset_e_j( const Population& N, const unsigned j ) const { return getMap().offset_e_j( N, j ); }
    void clearCount() { waitCount = 0; stepCount = 0; faultCount = 0; }

public:
    double throughput( const unsigned m, const unsigned k, const Population& N ) const;
    double utilization( const unsigned m, const unsigned k, const Population& N ) const;
    double utilization( const unsigned m, const Population& N ) const;
    double queueLength( const unsigned m, const Population& N ) const;
    double queueLength( const unsigned m, const unsigned k, const Population& N ) const;

protected:
    void step( const Population& );
    void step( const Population&, const unsigned );
    virtual void marginalProbabilities( const unsigned m, const Population& N ) = 0;
    virtual void marginalProbabilities2( const unsigned m, const Population& N ) = 0;

#if	DEBUG_MVA
    std::ostream& printL( std::ostream&, const Population& ) const;
    std::ostream& printW( std::ostream& ) const;
    std::ostream& printU( std::ostream&, const Population & N ) const;
    std::ostream& printP( std::ostream&, const Population & N ) const;
#endif
    std::ostream& printVectorP( std::ostream& output, const unsigned m, const Population& N ) const;

private:
    double tau_overlap( const Server&, const unsigned j, const unsigned k, const Population& N ) const;
    double tau( const Server&, const unsigned j, const unsigned k, const Population& ) const;

    std::ostream& printZ( std::ostream& ) const;
    std::ostream& printX( std::ostream& ) const;
    std::ostream& printPri( std::ostream& output ) const;

protected:
    virtual void initialize();

public:
    static int __bounds_limit;		/* Enable bounds limiting.	*/
    static double MOL_multiserver_underrelaxation;
#if DEBUG_MVA
    static bool debug_D;
    static bool debug_L;
    static bool debug_N;
    static bool debug_P;
    static bool debug_U;
    static bool debug_W;
    static bool debug_X;
#endif

protected:
    const Population& NCust;		/* Number of Customers.		*/
    const unsigned M;			/* Number of stations.		*/
    const unsigned K;			/* Number of classes.		*/
    Vector<Server *>& Q;		/* Queue type.  SS/delay.	*/

private:
    const Vector<double>& Z;		/* Think time per class.	*/

protected:
    const Vector<unsigned>& priority; 	/* Priority by chain.		*/
    const Vector<double>* overlapFactor;/* Overlap factor (usually 1.)	*/
    N_m_e_k L;				/* Queue length.		*/
    N_m_e_k U;				/* Station utilization.		*/

    std::vector<double **> P;		/* For marginal probabilities.	*/
    std::vector<double *> X;		/* Throughput per class.	*/

    unsigned long faultCount;		/* Number of times sc. fails	*/
    Vector<size_t> maxP;		/* Dimension of P[][][J]	*/

private:
    unsigned nPrio;			/* Number of unique priorities	*/
    Vector<unsigned> sortedPrio;	/* sorted and uniq priorities	*/
    unsigned long stepCount;		/* Number of iterations of step	*/
    unsigned long waitCount;		/* Number of calls to wait	*/
    Vector<unsigned> _isThread;
    unsigned maxOffset;			/* For L, U, X and P dimensions	*/
};

/* -------------------------------------------------------------------- */

class ExactMVA : public MVA {
public:
    ExactMVA( Vector<Server *>&, const Population&, const Vector<double>&,
	      const Vector<unsigned>&, const Vector<double>* of = 0 );

    virtual bool solve();
    virtual const char * getTypeName() const { return __typeName; }
    virtual Probability priorityInflation( const Server& station, const Population &N, const unsigned k ) const;
    virtual double filter() const { return 1.0; }
    virtual bool isExactMVA() const {return true;}
private:
    unsigned offset( const Population& N ) const { return map.offset( N ); }
    unsigned offset_e_j( const Population& N, const unsigned j ) const { return map.offset_e_j( N, j ); }

private:
    virtual void marginalProbabilities( const unsigned m, const Population& N );
    virtual void marginalProbabilities2( const unsigned m, const Population& N );

    virtual const PopulationMap& getMap() const { return map; }

private:
    FullPopulationMap map;
    static const char * const __typeName;
};

/* -------------------------------------------------------------------- */

class SchweitzerCommon : public MVA {
public:
    SchweitzerCommon( Vector<Server *>&, const Population&, const Vector<double>&,
		      const Vector<unsigned>&, const Vector<double>* of = 0 );
    virtual ~SchweitzerCommon();

    virtual Probability priorityInflation( const Server& station, const Population &N, const unsigned k ) const;
    virtual double filter() const { return MOL_multiserver_underrelaxation; };

protected:
    virtual void reset();
    virtual void initialize();

    void core( const Population &, const unsigned );

    virtual void estimate_L( const Population & );
    void estimate_Lm( const unsigned m, const Population & N, const unsigned n ) const;
    virtual void estimate_P( const Population & N ) = 0;

private:
    void copy_L( const unsigned n ) const;
    double max_delta_L( const unsigned n, const Population &N ) const;

    virtual void marginalProbabilities( const unsigned m, const Population& N );
    virtual void marginalProbabilities2( const unsigned m, const Population& N );

    virtual double D_mekj( const unsigned, const unsigned, const unsigned, const unsigned ) const { return 0.0; }

protected:
    const double termination_test;
    bool initialized;			/* True if initialized.		*/
    double ***last_L;			/* For local comparison.	*/
};


/* -------------------------------------------------------------------- */

class Schweitzer : public SchweitzerCommon {
public:
    Schweitzer( Vector<Server *>&, const Population&, const Vector<double>&,
		const Vector<unsigned>&, const Vector<double>* of = 0 );
    virtual ~Schweitzer();

    virtual bool solve();
    virtual const char * getTypeName() const { return __typeName; }

protected:
    virtual const PopulationMap& getMap() const { return map; }

    virtual void estimate_P( const Population & );

protected:
    SinglePopulationMap map;
    static const char * const __typeName;
};

/* -------------------------------------------------------------------- */

class OneStepMVA: public Schweitzer {
public:
    OneStepMVA( Vector<Server *>&, const Population&, const Vector<double>&,
		const Vector<unsigned>&, const Vector<double>* of = 0 );
    virtual bool solve();
    virtual const char * getTypeName() const { return __typeName; }
private:
    static const char * const __typeName;
};

/* -------------------------------------------------------------------- */

class Linearizer: public SchweitzerCommon {
public:
    Linearizer( Vector<Server *>&, const Population&, const Vector<double>&,
		const Vector<unsigned>&, const Vector<double>* of = 0 );
    virtual ~Linearizer();

    virtual void reset();
    virtual bool solve();
    virtual const char * getTypeName() const { return __typeName; }

protected:
    virtual const PopulationMap& getMap() const { return map; }
    virtual unsigned offset_e_c_e_j( const unsigned c, const unsigned j ) const { return getMap().offset_e_c_e_j( c, j ); }

    virtual void update_Delta( const Population & N );
    double D_mekj( const unsigned m, const unsigned e, const unsigned k, const unsigned j ) const { return D[m][e][k][j]; }
    std::ostream& printD( std::ostream&, const Population& ) const;

    virtual void estimate_P( const Population & N );

    virtual void initialize();
    void save_L();
    void restore_L();

protected:
    N_m_e_k saved_L;			/* Saved queue length.		*/
    N_m_e_k saved_U;			/* Saved utilization.		*/
    std::vector<double **> saved_P;	/* Saved marginal queue.	*/
    double ****D;			/* Delta Fraction of jobs.	*/
    unsigned c;				/* Customer from class c removed*/
    PartialPopulationMap map;
private:
    static const char * const __typeName;
};

/* -------------------------------------------------------------------- */

class OneStepLinearizer: public Linearizer {
public:
    OneStepLinearizer( Vector<Server *>&, const Population&, const Vector<double>&,
		       const Vector<unsigned>&, const Vector<double>* of = 0 );
    virtual bool solve();
    virtual const char * getTypeName() const { return __typeName; }
private:
    static const char * const __typeName;
};

/* -------------------------------------------------------------------- */

class Linearizer2: public Linearizer {
public:
    Linearizer2( Vector<Server *>&, const Population&,
		 const Vector<double>&, const Vector<unsigned>&, const Vector<double>* of = 0 );
    virtual ~Linearizer2();
    virtual const char * getTypeName() const { return __typeName; }

    virtual double sumOf_L_m( const Server& station, const Population &N, const unsigned j ) const;
    virtual double sumOf_SL_m( const Server&, const Population &, const unsigned ) const;

protected:
    void update_Delta( const Population & );
    void estimate_L( const Population & );

private:
    std::vector<double *> Lm;		/* Queue length sum (Fast lin.)	*/
    double ***D_k;			/* Sum over k.			*/
    static const char * const __typeName;
};
#endif
