/* -*- c++ -*-
 * $HeadURL: svn://192.168.2.10/lqn/trunk-V5/lqns/pop.h $
 *
 * Exported goodies.
 *
 * Copyright the Real-Time and Distributed Systems Group,
 * Department of Systems and Computer Engineering,
 * Carleton University, Ottawa, Ontario, Canada. K1S 5B6
 *
 * November, 1994
 *
 * $Id: pop.h 11963 2014-04-10 14:36:42Z greg $
 *
 * ------------------------------------------------------------------------
 */

#if	!defined(POPULATION_H)
#define	POPULATION_H

#include "vector.h"

typedef	 VectorMath<unsigned> PopVector;


/**
   This class provides the basic mapping that allows a population vector
   to be translated into a single dimension index.  It assumes that the
   population that is being examined is fixed for a particular instance.
**/
class PopulationMap {
public:
    PopulationMap( const PopVector & maxCust );
    virtual ~PopulationMap();

private:
    PopulationMap( const PopulationMap& );		/* copying ist verbotten! */
    PopulationMap& operator=( const PopulationMap& );

public:
    /**
       Reset the size of the arrays.
    **/
    virtual size_t dimension( const PopVector & ) = 0;
    /**
       Return offset for Population N.  
       The offset will be between 1 and maxOffset()
    **/
    virtual unsigned offset( const PopVector & N ) const = 0;
    /**
       Return offset for Population N with one customer removed from class j.
       There must be at least one customer in class j.
       The offset will be between 1 and maxOffset()
    **/
    virtual unsigned offset_e_j( const PopVector & N, const unsigned j ) const = 0;

    size_t maxOffset() { return maximumPopulation; }

protected:
    const unsigned maxCustSize;
    size_t maximumPopulation;
    unsigned * stride;
};

//Support mapping all combinations from 1->maxEntry for each
//customer size in the population vector.
class GeneralPopulationMap : public PopulationMap 
{
public: 
    GeneralPopulationMap( const PopVector & maxCustomers );
    size_t dimension( const PopVector & );

    unsigned offset( const PopVector & N ) const;
    unsigned offset_e_j( const PopVector & N, const unsigned j ) const;
};

//Only support the maximum customer configuration and the 
//case of one less of each customer in the population.
class SpecialPopulationMap : public PopulationMap 
{
public: 
    SpecialPopulationMap( const PopVector & maxCustomers );
    size_t dimension( const PopVector & );

    unsigned offset( const PopVector & N ) const;
    unsigned offset_e_j( const PopVector & N, const unsigned j ) const;

    /*
     * Return array offset with a customer removed from class `c' and
     * a customer removed from class `j'.  If either `c' or `j' is zero,
     * then NO customer is removed from the corresponding class.
     */
    unsigned offset_e_c_e_j( const unsigned c, const unsigned j ) const;

private:
    PopVector maxCust;
};

class PopulationIterator 
{
public:
    PopulationIterator( const PopVector& aLimit ) : limit(aLimit) {}
    virtual ~PopulationIterator() {}
    virtual int operator()( PopVector& );
    virtual int offset( const PopVector& ) const { return 1; }

private:
    int step( PopVector& N, const unsigned k );

protected:
    PopVector limit;
};



class PopulationIteratorOffset : public PopulationIterator 
{
public:
    PopulationIteratorOffset( const PopVector& maxCust, const PopVector& aLimit );
    virtual int offset( const PopVector& ) const;
    unsigned maxOffset() const { return arraysize; }
    unsigned offset_e_j( const PopVector &N, const unsigned j ) const;
	
private:
    const unsigned nClasses;
    Vector<unsigned> stride;
    unsigned arraysize;
};
#endif
