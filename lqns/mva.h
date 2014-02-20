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
 * $Id$
 *
 * ------------------------------------------------------------------------
 */

#if	!defined(MVA_H)
#define	MVA_H
/* undef DEBUG_MVA 1 */

#include "pop.h"
#include "vector.h"
#include "server.h"
#include "prob.h"

class MVA;
class GeneralPopulationMap;
class SpecialPopulationMap;

ostream& operator<<( ostream &, MVA& );

/* -------------------------------------------------------------------- */

class MVA 
{
    friend class Multi_Server;
    friend class Synch_Server;
    friend class Server;

    /* The following is defined in the MVA test suite and only used there. */
#if defined(TESTMVA)
    friend bool check( const int solverId, const MVA& solver, const unsigned );
    friend void special_check( ostream&, const MVA&, const unsigned );
#endif
	
protected:
    /* Thrown to indicate iteration limits in internal computations.  */
    class iteration_limit : public runtime_error 
    {
    public:
	explicit iteration_limit( const string& aStr) : runtime_error( aStr ) {}
    };

    /*
     * This template provides access to a particular data type
     * (templated) that is indexed initially by population.  The
     * actual indicies for the population are decided by an input
     * function that works on population vectors.
     */

    template <class Type>
    class PopulationData {
    public:
	PopulationData() : maxSize(0), data(0) {}
	~PopulationData() {
	    if(maxSize != 0) {
		delete [] data;
	    }
	}

	void dimension( size_t size ) {
	    const size_t oldSize = maxSize;

	    if ( maxSize == size ) return;	/* NOP */
	    maxSize = size;

	    Type * oldArray = data;
	    if(maxSize != 0) {
		data = new Type [maxSize];
		for ( size_t i = 0; i < oldSize; ++i ) {
		    data[i] = oldArray[i];
		}
		for ( size_t i = oldSize; i < maxSize; ++i ) {
		    data[i] = 0;
		}
	    } else {
		data = 0;
	    }

	    if ( oldArray ) {
		delete [] oldArray;
	    }
	}

	size_t size() { return maxSize; }

	Type& operator[](const unsigned ix) const { 
	    assert(ix < maxSize); 
	    return data[ix]; 
	}

    protected:
	unsigned maxSize;
	Type * data;
    };

//Shorthand for [N][m][e][k]
typedef PopulationData<double ***> N_m_e_k; 


public:
    MVA( Vector<Server *>&, const PopVector &, const VectorMath<double>&,
	 const Vector<unsigned>&, const VectorMath<double>* );
    virtual ~MVA();

    virtual void solve() = 0;

    unsigned nChains() const { return K; }
    virtual double filter() const = 0;
	void setThreadChain(const unsigned k, const unsigned kk){ _isThread[k]=kk;}
	unsigned getThreadChain(const unsigned k) const { return _isThread[k];}

    virtual bool isExactMVA() const {return false;}
    virtual double sumOf_L_m( const Server& station, const PopVector &N, const unsigned j ) const;
    virtual double sumOf_SL_m( const Server& station, const PopVector &N, const unsigned j ) const;
    double sumOf_SU_m( const Server& station, const PopVector &N, const unsigned j ) const;
    double sumOf_SQ_m( const Server& station, const PopVector &N, const unsigned j ) const;
    double sumOf_rU_m( const Server& station, const PopVector &N, const unsigned j ) const;
    double sumOf_S2U_m( const Server& station, const PopVector &N, const unsigned j ) const;
    double sumOf_S2U_m( const Server& station, const unsigned e, const PopVector &N, const unsigned j ) const;
    double sumOf_S2_m( const Server& station, const PopVector &N, const unsigned j ) const;
    double sumOf_U_m( const Server& station, const PopVector& N, const unsigned j ) const;
    double sumOf_USPrOt_m( const Server& station, const unsigned e, const Probability& PrOt, const PopVector &N, const unsigned j ) const;
    double sumOf_U2_m( const Server& station, const unsigned k, const PopVector &N, const unsigned j ) const;
    double sumOf_U2_m( const Server& station, const PopVector &N, const unsigned j ) const;
    double sumOf_P( const Server& station, const PopVector &N, const unsigned j ) const;
    double sumOf_SP2( const Server& station, const PopVector &N, const unsigned j ) const;
    double sumOf_alphaP( const Server& station, const PopVector &N ) const;
    double PB(  const Server& station, const PopVector &N, const unsigned j ) const;
    double PB2( const Server& station, const PopVector &N, const unsigned j ) const;
    virtual Probability priorityInflation( const Server& station, const PopVector &N, const unsigned k ) const = 0;
    double thinkTime( const unsigned k ) const { return Z[k]; }
    double throughput( const unsigned k ) const;
    double throughput( const unsigned k, const PopVector& N ) const;
    double throughput( const Server&  ) const;
    double throughput( const Server&, const unsigned k ) const;
    double throughput( const Server&, const unsigned e, const unsigned k ) const;
    double entryThroughput( const Server&, const unsigned ) const;
    double normalizedThroughput( const Server&, const unsigned, const unsigned chainNum ) const;
    double utilization( const Server& station ) const { return utilization( station, NCust ); }
    double utilization( const Server&, const PopVector& N ) const;
    double utilization( const Server&, const unsigned k ) const;
    double utilization( const Server&, const unsigned k, const PopVector& N, const unsigned j ) const;
    double queueLength( const Server& station ) const { return queueLength( station, NCust ); }
    double queueLength( const Server&, const PopVector& N ) const;
    double queueLength( const Server&, const unsigned k ) const;
    double queueOnly( const Server&, const unsigned k, const PopVector& N, const unsigned j ) const;
    double responseTime( const Server&, const unsigned k ) const;
    double responseTime( const unsigned k ) const;

    Positive arrivalRate( const Server&, const unsigned e, const unsigned k, const PopVector& N ) const;
    double syncDelta( const Server&, const unsigned e, const unsigned k, const PopVector& N ) const;

    ostream& print( ostream& ) const;

    unsigned long iterations() const { return stepCount; }
    unsigned long waits() const { return waitCount; }
    unsigned long faults() const { return faultCount; }

    double nrFactor( const Server&, const unsigned, const unsigned ) const;

protected:
    void dimension( const size_t );
    bool dimension( PopulationData<double **>&, const size_t );
    virtual PopulationMap * getPopulationMap() const = 0;
    virtual unsigned offset( const PopVector& ) const = 0;
    virtual unsigned offset_e_j( const PopVector&, const unsigned ) const = 0;
    void clearCount() { waitCount = 0; stepCount = 0; faultCount = 0; }

    double utilization( const unsigned m, const unsigned k, const PopVector& N ) const;
    double utilization( const unsigned m, const PopVector& N ) const;

    void step( const PopVector& );
    void step( const PopVector&, const unsigned );
    double queueLength( const unsigned, const PopVector& N ) const;
    double queueLength( const unsigned m, const unsigned k, const PopVector& N ) const;
    virtual void marginalProbabilities( const unsigned m, const PopVector& N );
    virtual void marginalProbabilities2( const unsigned m, const PopVector& N );
	

    ostream& printL( ostream&, const PopVector& ) const;
    ostream& printW( ostream& ) const;
    ostream& printU( ostream&, const PopVector & N ) const;
    ostream& printP( ostream&, const PopVector & N ) const;
    ostream& printVectorP( ostream& output, const unsigned m, const PopVector& N ) const;

private:
#if defined(TESTMVA)
    double throughput( const unsigned m, const unsigned k ) const;
#endif
    double tau_overlap( const Server&, const unsigned j, const unsigned k, const PopVector& N ) const;
    double tau( const Server&, const unsigned j, const unsigned k, const PopVector& ) const;
	
    ostream& printZ( ostream& ) const;
    ostream& printX( ostream& ) const;
    ostream& printPri( ostream& output ) const;

    void initialize();

public:
    static int boundsLimit;		/* Enable bounds limiting.	*/
    static double MOL_multiserver_underrelaxation;
#if DEBUG_MVA
    static bool debug_P;
    static bool debug_L;
    static bool debug_D;
#endif
	
protected:
    const PopVector& NCust;		/* Number of Customers.		*/
    const unsigned M;			/* Number of stations.		*/
    const unsigned K;			/* Number of classes.		*/
    Vector<Server *>& Q;		/* Queue type.  SS/delay.	*/
private:
    const VectorMath<double>& Z;	/* Think time per class.	*/

protected:
    const Vector<unsigned>& priority; 	/* Priority by chain.		*/
    const VectorMath<double>* overlapFactor;/* Overlap factor (usually 1.)	*/
    N_m_e_k  L;				/* Queue length.		*/
    N_m_e_k  U;				/* Station utilization.		*/

    PopulationData<double **> P;	/* For marginal probabilities.	*/
    PopulationData<double *> X;		/* Throughput per class.	*/

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
    ExactMVA( Vector<Server *>&, const PopVector&, const VectorMath<double>&, 
	      const Vector<unsigned>&, const VectorMath<double>* of = 0 );

    void solve();
    virtual Probability priorityInflation( const Server& station, const PopVector &N, const unsigned k ) const;
    virtual double filter() const { return 1.0; }

protected:
    PopulationMap * getPopulationMap() const { return (PopulationMap *)&map; }
    unsigned offset( const PopVector& N ) const { return map.offset( N ); }
    unsigned offset_e_j( const PopVector& N, const unsigned j ) const { return map.offset_e_j( N, j ); }

private:
    GeneralPopulationMap map;
};

/* -------------------------------------------------------------------- */

class Schweitzer : public MVA {
public:
    Schweitzer( Vector<Server *>&, const PopVector&, const VectorMath<double>&, 
		const Vector<unsigned>&, const VectorMath<double>* of = 0 );
    virtual ~Schweitzer();

    virtual void solve();
    virtual Probability priorityInflation( const Server& station, const PopVector &N, const unsigned k ) const;
    virtual double filter() const { return MOL_multiserver_underrelaxation; };
	
protected:
    void initialize();

    PopulationMap * getPopulationMap() const { return (PopulationMap *)&map; }
    //TF NOTE: I don't like this conversion but the original code ignored the 
    //inputs and just used the class variable 'c' so we have to map vs that
    inline unsigned offset( const PopVector& N ) const { return map.offset_e_c_e_j(c, 0); }
    inline unsigned offset_e_j( const PopVector& N, const unsigned j ) const { return map.offset_e_c_e_j(c, j); }

    void core( const PopVector & );
    virtual void estimate_L( const PopVector & );
    void estimate_Lm( const unsigned m, const PopVector & N, const unsigned n ) const;
    virtual void estimate_P( const PopVector & );
    virtual double D_mekj( const unsigned, const unsigned, const unsigned, const unsigned ) const { return 0.0; }
	
    virtual void marginalProbabilities( const unsigned m, const PopVector& N );
    virtual void marginalProbabilities2( const unsigned m, const PopVector& N );

protected:
    SpecialPopulationMap map;
    unsigned c;				/* Customer from class c removed*/

private:
    double ***last_L;			/* For local comparison.	*/
};

/* -------------------------------------------------------------------- */

class OneStepMVA: public Schweitzer {
public:
    OneStepMVA( Vector<Server *>&, const PopVector&, const VectorMath<double>&, 
		const Vector<unsigned>&, const VectorMath<double>* of = 0 );
    virtual void solve();
};

/* -------------------------------------------------------------------- */

class Linearizer: public Schweitzer {
public:
    Linearizer( Vector<Server *>&, const PopVector&, const VectorMath<double>&, 
		const Vector<unsigned>&, const VectorMath<double>* of = 0 );
    virtual ~Linearizer();

    virtual void solve();
	
protected:
    virtual void update_Delta( const PopVector & N );
    double D_mekj( const unsigned m, const unsigned e, const unsigned k, const unsigned j ) const { return D[m][e][k][j]; }
	
    ostream& printD( ostream&, const PopVector& ) const;

protected:
    void save_L();
    void restore_L();

protected:
    N_m_e_k saved_L;			/* Saved queue length.		*/
    N_m_e_k saved_U;			/* Saved utilization.		*/
    PopulationData<double **> saved_P;	/* Saved marginal queue.	*/
    double ****D;			/* Delta Fraction of jobs.	*/
    bool initialized;			/* True if initialized.		*/
};

/* -------------------------------------------------------------------- */

class OneStepLinearizer: public Linearizer {
public:
    OneStepLinearizer( Vector<Server *>&, const PopVector&, const VectorMath<double>&, 
		       const Vector<unsigned>&, const VectorMath<double>* of = 0 );
    virtual void solve();
};

/* -------------------------------------------------------------------- */

class Linearizer2: public Linearizer {
public:
    Linearizer2( Vector<Server *>&, const PopVector&,
		 const VectorMath<double>&, const Vector<unsigned>&, const VectorMath<double>* of = 0 );
    virtual ~Linearizer2();
    virtual double sumOf_L_m( const Server& station, const PopVector &N, const unsigned j ) const;
    virtual double sumOf_SL_m( const Server&, const PopVector &, const unsigned ) const;

protected:
    void update_Delta( const PopVector & );
    void estimate_L( const PopVector & );

private:
    PopulationData<double *> Lm;	/* Queue length sum (Fast lin.)	*/
    double ***D_k;			/* Sum over k.			*/
};
#endif
