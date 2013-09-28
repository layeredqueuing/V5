/*  -*- c++ -*-
 * $Id$
 *
 * Population vector functions.
 *
 * Copyright the Real-Time and Distributed Systems Group,
 * Department of Systems and Computer Engineering,
 * Carleton University, Ottawa, Ontario, Canada. K1S 5B6
 *
 * November, 1994
 *
 * ------------------------------------------------------------------------
 */


#include "dim.h"
#include <cstdlib>
#include <string.h>
#include "pop.h"
#include "vector.h"

/* ----------------------- Population Maps ---------------------------- */

/*
 * Translate a population vector into an index value
 */

PopulationMap::PopulationMap( const PopVector & maxCust ) 
    : maxCustSize(maxCust.size()) 
{
    stride  = new unsigned[maxCustSize + 1];	//Index of offsets per class
}


PopulationMap::~PopulationMap() 
{
    delete [] stride;
}


//Support mapping all combinations from 1->maxEntry for each
//customer size in the population vector.
GeneralPopulationMap::GeneralPopulationMap( const PopVector & maxCust ) 
    : PopulationMap( maxCust )
{
    dimension( maxCust );
}



/*
 * (Re)set the size of the arrays.  We cannot add dimensions, just customers.
 */

size_t
GeneralPopulationMap::dimension( const PopVector& maxCust )
{
    assert( maxCustSize == maxCust.size() );

    maximumPopulation = 1;
    for( unsigned j = maxCustSize; j > 0; --j ) {
	stride[j] = maximumPopulation;
	maximumPopulation *= (maxCust[j] + 1);
    }
    return maximumPopulation;
}



unsigned 
GeneralPopulationMap::offset( const PopVector & N ) const
{
    unsigned j = 0;
    assert(N.size() == maxCustSize);
    for ( unsigned i = 1; i <= maxCustSize; ++i ) {
	j += N[i] * stride[i];
    }
    return j;
}



unsigned 
GeneralPopulationMap::offset_e_j( const PopVector & N, const unsigned j ) const
{
    unsigned i = offset( N );

    assert( N[j] > 0 && i >= stride[j] );

    return i - stride[j];
}



//Only support the maximum customer configuration and the 
//case of one less of each customer in the population.
SpecialPopulationMap::SpecialPopulationMap( const PopVector & maxCustomers ) 
    : PopulationMap( maxCustomers ), maxCust( maxCustomers )
{
    maximumPopulation = 0;
    for ( unsigned j = maxCustSize; j > 0; --j ) {
	stride[j] = maximumPopulation;
	maximumPopulation += (j + 1);
    }
    maximumPopulation += 1;
}


/*
 * Reset the size of the arrays.  For Special Populations, this is trivial
 */

size_t
SpecialPopulationMap::dimension( const PopVector& maxCustomers )
{
    assert( maxCust.size() == maxCustomers.size() );

    maxCust = maxCustomers;
    return maximumPopulation;
}



unsigned 
SpecialPopulationMap::offset( const PopVector & N ) const
{
    assert(N.size() == maxCust.size());
    for ( unsigned i = 1; i <= maxCustSize; ++i ) {
	switch ( maxCust[i] - N[i] ) {
	case 0:
	    break;
	case 1:
	    for (unsigned j = i + 1; j <= maxCustSize; ++j ) {
		if ( maxCust[j] - N[j] ) {
		    return offset_e_c_e_j( i, j );
		}
	    }
	    return offset_e_c_e_j( i, 0 );
	case 2:
	    return offset_e_c_e_j( i, i );
	default:
	    throw logic_error( "SpecialPopulationMap::offset" );
	}
    }
    return offset_e_c_e_j( 0, 0 );
}



unsigned 
SpecialPopulationMap::offset_e_j( const PopVector &N, const unsigned j ) const
{
    for ( unsigned i = 1; i <= maxCustSize; ++i ) {
	switch ( maxCust[i] - N[i] ) {
	case 0:
	    break;

	case 1:
	    return offset_e_c_e_j( i, j );

	default:
	    assert(0);
	}
    }
    return offset_e_c_e_j( 0, j );
}



unsigned 
SpecialPopulationMap::offset_e_c_e_j( const unsigned c, const unsigned j ) const 
{
    unsigned i;
	
    if ( c == 0 && j == 0 ) {
	i = maximumPopulation - 1;		/* Special case. */
    } else if ( c < j ) {
	i = stride[j] + c;
    } else {
	i = stride[c] + j;
    }
    assert( i < maximumPopulation );
    return i;
}

/* ---------------------------- Iterators ----------------------------- */

/*
 * Generate next population according to Population rule. (eqn 12)
 */

int
PopulationIterator::operator()( PopVector& N ) 
{
    return step( N, limit.size() );
}


/*
 * Recursively generate population vectors.
 */

int
PopulationIterator::step( PopVector& N, const unsigned k )
{
    if ( N[k] < limit[k] ) {
	N[k] += 1;
	return offset( N );
    } else {
	N[k] = 0;
	if ( k > 1 ) {
	    return step( N, k - 1 );
	} else {
	    return 0;
	}
    }
}



/*
 * Special version of population iterator that returns offset
 * value.
 */

PopulationIteratorOffset::PopulationIteratorOffset( const PopVector& maxCust, const PopVector& aLimit )
    : PopulationIterator(aLimit), nClasses( maxCust.size() )
{
    stride.grow( maxCust.size() );
	
    assert( aLimit.size() == maxCust.size() );
	
    unsigned product = 1;

    for ( unsigned j = nClasses; j > 0; --j ) {
	assert ( aLimit[j] <= maxCust[j] );
		
	stride[j] = product;
	product *= (maxCust[j] + 1);
    }
    arraysize = product;
}



/*
 * Return offset of population vector.
 */

int
PopulationIteratorOffset::offset( const PopVector& N ) const
{
    unsigned j = 0;

    for ( unsigned i = 1; i <= nClasses; ++i ) {
	j += N[i] * stride[i];
    }
	
    return j;
}



/*
 * Compute offset of N with one customer removed from class j.  There
 * must be at least one customer in class j.
 */

unsigned
PopulationIteratorOffset::offset_e_j( const PopVector &N, const unsigned j ) const
{
    unsigned i = offset( N );

    assert( N[j] >= 1 && i >= stride[j] );

    return i - stride[j];
}
