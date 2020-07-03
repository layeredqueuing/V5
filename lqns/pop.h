/* -*- c++ -*-
 * $HeadURL: http://rads-svn.sce.carleton.ca:8080/svn/lqn/trunk-V5/lqns/pop.h $
 *
 * Exported goodies.
 *
 * Copyright the Real-Time and Distributed Systems Group,
 * Department of Systems and Computer Engineering,
 * Carleton University, Ottawa, Ontario, Canada. K1S 5B6
 *
 * November, 1994
 *
 * $Id: pop.h 13413 2018-10-23 15:03:40Z greg $
 *
 * ------------------------------------------------------------------------
 */

#if	!defined(POPULATION_H)
#define	POPULATION_H

#include "vector.h"

class population_iterator;

class Population 
{
private:
    class Sum {
    public:
	Sum() : _sum(0) {}
	void operator() ( unsigned value ) { _sum += value; }
	unsigned sum() { return _sum; }

    private:
	unsigned int _sum;
    };
    
public:
    class Iterator 
    {
    public:
	Iterator( const Population& aLimit ) : limit(aLimit._N) {}
	virtual ~Iterator() {}
	virtual int operator()( Population& );
	virtual int offset( const Population& ) const { return 1; }

    private:
	int step( Population& N, const unsigned k );

    protected:
	Vector<unsigned> limit;
//	Population limit;
    };


    class IteratorOffset : public Iterator 
    {
    public:
	IteratorOffset( const Population& maxCust, const Population& aLimit );
	virtual int offset( const Population& ) const;
	unsigned maxOffset() const { return arraysize; }
	unsigned offset_e_j( const Population &N, const unsigned j ) const;
	
    private:
	const unsigned nClasses;
	Vector<unsigned> stride;
	unsigned arraysize;
    };

public:
    Population( unsigned int size=0 );
    Population( const Population& N ) : _N(N._N) {}
    Population& operator=( const Population& N ) { _N = N._N; return *this; }
    unsigned operator[]( size_t i ) const { return _N[i]; }
    unsigned& operator[]( size_t i ) { return _N[i]; }
    bool operator==( const Population& N ) const { return _N == N._N; }
    bool operator!=( const Population& N ) const { return !(_N == N._N); }

    size_t size() const { return _N.size(); }
    void resize( size_t size ) { if ( size > this->size() ) _N.grow( size - this->size() ); }
    void clear() { _N.clear(); }
    std::ostream& print( std::ostream& output ) const { return _N.print( output ); }

    population_iterator begin() const;
    population_iterator end() const;
    
    unsigned sum() const;

private:
    Vector<unsigned> _N;

    
};

inline std::ostream& operator<<( std::ostream& output, const Population& self ) { return self.print( output ); }

/* ------------------------------------------------------------------------ */
    
class population_iterator {
public:
    population_iterator( const Population& N, bool );
    population_iterator( const population_iterator& i );
    population_iterator& operator=( const population_iterator& i );

    bool operator!=( const population_iterator& i ) { return _i != i._i; }
    population_iterator& operator++();	/* Advance population by 1.	*/
    const Population& operator*() const { return _n; }

private:
    size_t end( const Population& N ) const;

private:
    const Population _N;		/* Max population over all K	*/
    Population _n;			/* Current population		*/
    size_t _i;				/* index from [0,...,1]		*/
    const size_t _end;			/* end index.			*/
};


/**
   This class provides the basic mapping that allows a population vector
   to be translated into a single dimension index.  It assumes that the
   population that is being examined is fixed for a particular instance.
**/
class PopulationMap {
public:
    PopulationMap( const Population & N );
    virtual ~PopulationMap();

private:
    PopulationMap( const PopulationMap& );		/* copying ist verbotten! */
    PopulationMap& operator=( const PopulationMap& );

public:
    /**
       Reset the size of the arrays.
    **/
    virtual size_t dimension( const Population & ) = 0;
    /**
       Return offset for Population N.  
       The offset will be between 1 and maxOffset()
    **/
    virtual unsigned offset( const Population & N ) const = 0;
    /**
       Return offset for Population N with one customer removed from class j.
       There must be at least one customer in class j.
       The offset will be between 1 and maxOffset()
    **/
    virtual unsigned offset_e_j( const Population & N, const unsigned j ) const = 0;
    virtual unsigned offset_e_c_e_j( const unsigned c, const unsigned j ) const;
    size_t maxOffset() const { return maximumOffset; }

protected:
    const unsigned NCustSize;
    size_t maximumOffset;
};

//Support mapping all combinations from 1->maxEntry for each
//customer size in the population vector.
class FullPopulationMap : public PopulationMap
{
public:
    FullPopulationMap( const Population & N );
    virtual ~FullPopulationMap();

    size_t dimension( const Population & );

    unsigned offset( const Population & N ) const;
    unsigned offset_e_j( const Population & N, const unsigned j ) const;

private:
    unsigned * stride;
};

//Only support the maximum customer configuration and the 
//case of one less of each customer in the population.
//This is a triangular array since c <= j.
class PartialPopulationMap : public PopulationMap
{
public:
    PartialPopulationMap( const Population & );
    virtual ~PartialPopulationMap();

    size_t dimension( const Population & );

    unsigned offset( const Population & N ) const;
    unsigned offset_e_j( const Population & N, const unsigned j ) const;

    /*
     * Return array offset with a customer removed from class `c' and
     * a customer removed from class `j'.  If either `c' or `j' is zero,
     * then NO customer is removed from the corresponding class.
     */
    unsigned offset_e_c_e_j( const unsigned c, const unsigned j ) const;
    unsigned offset_e_j_mc( const Population & N, const unsigned j , const unsigned mc) const;

private:
    Population NCust;		/* Max. Number of customers */
    unsigned * stride;
};

//Only support the maximum customer configuration.
class SinglePopulationMap : public PopulationMap
{
public:
    SinglePopulationMap( const Population & );
    virtual ~SinglePopulationMap();
    size_t dimension( const Population & );
    
    unsigned offset( const Population & N ) const;
    unsigned offset_e_j( const Population & N, const unsigned j ) const;
    unsigned offset_e_j_mc( const Population & N, const unsigned j, const unsigned mc) const;

private:
    Population NCust;
};
#endif
