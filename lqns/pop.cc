/*  -*- c++ -*-
 * $Id: pop.cc 13413 2018-10-23 15:03:40Z greg $
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

/*
 */
Population::Population( unsigned int size )
{
    _N.grow( size );
}


/*
 * Return an "iterator" for the fist population (i.e., [0,0,0,...1]).
 */

population_iterator
Population::begin() const
{
    return population_iterator( *this, true );
}

/*
 * Return an "iterator" for just past the last population (i.e., [n1,n2,n3,...]).
 */

population_iterator
Population::end() const
{
    return population_iterator( *this, false );
}

unsigned
Population::sum() const
{
    unsigned sum = 0;

    const size_t n = _N.size();
    for ( size_t ix = 1; ix <= n; ++ix ) {
	sum += _N[ix];
    }
    return sum;
}

/* -------------------- Population Iterators -------------------------- */

/* 
 * Constructor.  _limit is the "end"
 */
population_iterator::population_iterator( const Population& N, bool begin )
    : _N(N), _end(end(N))
{
    if ( begin ) {
	const size_t k = N.size();
	_n.resize(k);		/* Init to zero. 		*/
	_n[k] = 1;		/* One customer in last class 	*/
	_i = 1;
    } else {
	_n = N;
	_i = _end;
    }
}

/* Copy constructor */
population_iterator::population_iterator( const population_iterator& i )
    : _N(i._N), _n(i._n), _i(i._i), _end(i._end)
{
}

population_iterator&
population_iterator::operator=( const population_iterator& i )
{
    *const_cast<Population*>(&_N) = i._N;
    _n = i._n;
    _i = i._i;
    *const_cast<size_t *>(&_end) = i._end;
    return *this;
}

/*
 * Advance the population by one customer starting from the right (least stride).
 */

population_iterator&
population_iterator::operator++() 
{
    _i += 1;				/* Advance index (for compare)	*/
    for ( size_t k = _N.size(); k > 0; --k ) {
	if ( _n[k] < _N[k] ) {
	    _n[k] += 1;			/* Add one to current class pop	*/
	    return *this;
	} else {
	    _n[k] = 0;			/* Rest current class pop.	*/
	}
    }
    /* _n should equal [0,...] and so... */
    assert( _i == _end );		/* wrapped around, so done.	*/
    return *this;
}


size_t
population_iterator::end( const Population& N ) const
{
    size_t end = 1;
    size_t n = _N.size();
    for( unsigned j = n; j > 0; --j ) {
	end *= (N[j] + 1);
    }
    return end;
}



/* ---------------------------- Iterators ----------------------------- */

/*
 * Generate next population according to Population rule. (eqn 12)
 */

int
Population::Iterator::operator()( Population& N ) 
{
    return step( N, limit.size() );
}


/*
 * Recursively generate population vectors.
 */

int
Population::Iterator::step( Population& N, const unsigned k )
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

Population::IteratorOffset::IteratorOffset( const Population& maxCust, const Population& aLimit )
    : Population::Iterator(aLimit), nClasses( maxCust.size() ), stride( maxCust.size() )
{
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
Population::IteratorOffset::offset( const Population& N ) const
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
Population::IteratorOffset::offset_e_j( const Population &N, const unsigned j ) const
{
    unsigned i = offset( N );

    assert( N[j] >= 1 && i >= stride[j] );

    return i - stride[j];
}

/* ----------------------- Population Maps ---------------------------- */

/*
 * Translate a population vector into an index value
 */

PopulationMap::PopulationMap( const Population & N ) 
    : NCustSize(N.size()) 
{
}


PopulationMap::~PopulationMap() 
{
}


/*
 * Should not implement for ExactMVA.  assert( c == 0 ) for schweitzer.
 */

unsigned
PopulationMap::offset_e_c_e_j( const unsigned c, const unsigned j ) const
{
    throw logic_error( "PopulationMap::offset_e_c_e_j" );
}

//Support mapping all combinations from 1->maxEntry for each
//customer size in the population vector.
FullPopulationMap::FullPopulationMap( const Population & N ) 
    : PopulationMap( N )
{
    stride = new unsigned[NCustSize + 1];	//Index of offsets per class
    dimension( N );
}

FullPopulationMap::~FullPopulationMap() 
{
    delete [] stride;
}


/*
 * (Re)set the size of the arrays.  We cannot add dimensions, just customers.
 */

size_t
FullPopulationMap::dimension( const Population& maxCust )
{
    assert( NCustSize == maxCust.size() );

    maximumOffset = 1;
    for( unsigned j = NCustSize; j > 0; --j ) {
	stride[j] = maximumOffset;
	maximumOffset *= (maxCust[j] + 1);
    }
    return maximumOffset;
}



unsigned 
FullPopulationMap::offset( const Population & N ) const
{
    unsigned j = 0;
    assert(N.size() == NCustSize);
    for ( unsigned i = 1; i <= NCustSize; ++i ) {
	j += N[i] * stride[i];
    }
    return j;
}



unsigned 
FullPopulationMap::offset_e_j( const Population & N, const unsigned j ) const
{
    unsigned i = offset( N );

    assert( N[j] > 0 && i >= stride[j] );

    return i - stride[j];
}

//Only support the maximum customer configuration and the 
//case of one less of each customer in the population.
PartialPopulationMap::PartialPopulationMap( const Population & N ) 
    : PopulationMap( N ), NCust( N )
{
    stride = new unsigned[NCustSize + 1];	//Index of offsets per class
    maximumOffset = 0;
    for ( unsigned j = NCustSize; j > 0; --j ) {
	stride[j] = maximumOffset;
	maximumOffset += (j + 1);
    }
    maximumOffset += 1;
}


PartialPopulationMap::~PartialPopulationMap() 
{
    delete [] stride;
}


/*
 * Reset the size of the arrays.  For Special Populations, this is trivial
 */

size_t
PartialPopulationMap::dimension( const Population& N )
{
    assert( NCust.size() == N.size() );

    NCust = N;
    return maximumOffset;
}



unsigned 
PartialPopulationMap::offset( const Population & N ) const
{
    assert(N.size() == NCust.size());
    for ( unsigned i = 1; i <= NCustSize; ++i ) {
	switch ( NCust[i] - N[i] ) {
	case 0:
	    break;
	case 1:
	    for (unsigned j = i + 1; j <= NCustSize; ++j ) {
		if ( NCust[j] - N[j] ) {
		    return offset_e_c_e_j( i, j );
		}
	    }
	    return offset_e_c_e_j( i, 0 );
	case 2:
	    return offset_e_c_e_j( i, i );
	default:
	    throw logic_error( "PartialPopulationMap::offset" );
	}
    }
    return offset_e_c_e_j( 0, 0 );
}



unsigned 
PartialPopulationMap::offset_e_j( const Population &N, const unsigned j ) const
{
    for ( unsigned i = 1; i <= NCustSize; ++i ) {
	switch ( NCust[i] - N[i] ) {
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
PartialPopulationMap::offset_e_c_e_j( const unsigned c, const unsigned j ) const 
{
    unsigned i;
	
    if ( c == 0 && j == 0 ) {
	i = maximumOffset - 1;		/* Special case - full population. */
    } else if ( c < j ) {
	i = stride[j] + c;
    } else {
	i = stride[c] + j;
    }
    assert( i < maximumOffset );
    return i;
}

//Only support the maximum customer configuration and the 
//case of one less of each customer in the population.
//Size is the number of dimensions of N, plus 1.
SinglePopulationMap::SinglePopulationMap( const Population & N ) 
    : PopulationMap( N ), NCust( N )
{
    maximumOffset = NCustSize + 1;
}


SinglePopulationMap::~SinglePopulationMap()
{
}


/*
 * Reset the size of the arrays.  For Special Populations, this is trivial
 */

size_t
SinglePopulationMap::dimension( const Population& N )
{
    assert( N.size() == NCustSize );

    NCust = N;
    return maximumOffset;
}



unsigned 
SinglePopulationMap::offset( const Population & N ) const
{
    assert( N.size() == NCustSize );
    for ( unsigned j = 1; j <= NCustSize; ++j ) {
	switch ( NCust[j] - N[j] ) {
	case 0:
	    break;
	case 1:
	    return offset_e_j( N, j );
	default:
	    throw logic_error( "SinglePopulationMap::offset" );
	}
    }
    return maximumOffset - 1;
}



unsigned 
SinglePopulationMap::offset_e_j( const Population &N, const unsigned j ) const
{
    if ( j == 0 ) {
	return maximumOffset - 1;	/* Full population	*/
    } else {
	assert( j <= NCustSize );
	return j - 1;
    }
}

