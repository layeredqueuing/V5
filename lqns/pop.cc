/*  -*- c++ -*-
 * $Id: pop.cc 13996 2020-10-24 22:01:20Z greg $
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
#include <algorithm>
#include "pop.h"
#include "vector.h"

/*
 */
Population::Population( unsigned int size )
{
    _N.resize( size, 0 );
}


unsigned
Population::sum() const
{
    return std::for_each( _N.begin(), _N.end(), Sum() ).sum();
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
    : _maxN( N ), _dimN(N.size()), _stride()
{
}


PopulationMap::~PopulationMap() 
{
}


/*
 * Return an "iterator" for the fist population (i.e., [0,0,0,...1]).
 */

PopulationMap::iterator
PopulationMap::begin() const
{
    return PopulationMap::iterator( _maxN );
}

/*
 * Return an "iterator" for just past the last population (i.e., [n1,n2,n3,...]).
 */

PopulationMap::iterator
PopulationMap::end() const
{
    return PopulationMap::iterator( _maxN, _maxN );
}

/*
 * Should not implement for ExactMVA.  assert( c == 0 ) for schweitzer.
 */

unsigned
PopulationMap::offset_e_c_e_j( const unsigned c, const unsigned j ) const
{
    throw logic_error( "PopulationMap::offset_e_c_e_j" );
}

/* -------------------- Population Iterators -------------------------- */

/* 
 * Constructor for "begin".  _limit is the "end"
 */

PopulationMap::iterator::iterator( const Population& N )
    : _N(N), _n(N.size()), _end(end(N))
{
    const size_t k = N.size();
    _n[k] = 1;			/* One customer in last class 	*/
    _i = 1;
}

/* 
 * Constructor for "end" (limit is the "end" and is less than or equal to N).
 */

PopulationMap::iterator::iterator( const Population& N, const Population& limit  )
    : _N(N), _n(limit), _end(end(N))
{
    assert(N.size() == limit.size());
    _i = _end;
}

/* Copy constructor */
PopulationMap::iterator::iterator( const PopulationMap::iterator& i )
    : _N(i._N), _n(i._n), _i(i._i), _end(i._end)
{
}

PopulationMap::iterator&
PopulationMap::iterator::operator=( const PopulationMap::iterator& i )
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

PopulationMap::iterator&
PopulationMap::iterator::operator++() 
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
PopulationMap::iterator::end( const Population& N ) const
{
    size_t end = 1;
    size_t n = _N.size();
    for( unsigned j = n; j > 0; --j ) {
	end *= (N[j] + 1);
    }
    return end;
}

//Support mapping all combinations from 1->maxEntry for each
//customer size in the population vector.
FullPopulationMap::FullPopulationMap( const Population & N ) 
    : PopulationMap( N )
{
    _stride.resize(_dimN);	//Index of offsets per class
    dimension( N );
}

FullPopulationMap::~FullPopulationMap() 
{
}


/*
 * (Re)set the size of the arrays.  We cannot add dimensions, just customers.
 */

const PopulationMap&
FullPopulationMap::dimension( const Population& maxCust )
{
    assert( _dimN == maxCust.size() );

    _maxN = maxCust;
    _end = 1;
    for( unsigned j = _dimN; j > 0; --j ) {
	_stride[j] = _end;
	_end *= (maxCust[j] + 1);
    }
    return *this;
}



unsigned 
FullPopulationMap::offset( const Population & N ) const
{
    unsigned j = 0;
    assert(N.size() == _dimN);
    for ( unsigned i = 1; i <= _dimN; ++i ) {
	j += N[i] * _stride[i];
    }
    return j;
}



unsigned 
FullPopulationMap::offset_e_j( const Population & N, const unsigned j ) const
{
    unsigned i = offset( N );

    assert( N[j] > 0 && i >= _stride[j] );

    return i - _stride[j];
}


//Only support the maximum customer configuration and the 
//case of one less of each customer in the population.
PartialPopulationMap::PartialPopulationMap( const Population & N ) 
    : PopulationMap( N )
{
    _stride.resize(_dimN);	//Index of offsets per class
    _end = 0;
    for ( unsigned j = _dimN; j > 0; --j ) {
	_stride[j] = _end;
	_end += (j + 1);
    }
    _end += 1;
}


PartialPopulationMap::~PartialPopulationMap() 
{
}


/*
 * Reset the size of the arrays.  For Special Populations, this is trivial
 */

const PopulationMap&
PartialPopulationMap::dimension( const Population& N )
{
    assert( _maxN.size() == N.size() );

    _maxN = N;
    return *this;
}



unsigned 
PartialPopulationMap::offset( const Population & N ) const
{
    assert(N.size() == _dimN);
    for ( unsigned i = 1; i <= _dimN; ++i ) {
	switch ( _maxN[i] - N[i] ) {
	case 0:
	    break;
	case 1:
	    for (unsigned j = i + 1; j <= _dimN; ++j ) {
		if ( _maxN[j] - N[j] ) {
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
    for ( unsigned i = 1; i <= _dimN; ++i ) {
	switch ( _maxN[i] - N[i] ) {
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
	i = _end - 1;		/* Special case - full population. */
    } else if ( c < j ) {
	i = _stride[j] + c;
    } else {
	i = _stride[c] + j;
    }
    assert( i < _end );
    return i;
}

//Only support the maximum customer configuration and the 
//case of one less of each customer in the population.
//Size is the number of dimensions of N, plus 1.
SinglePopulationMap::SinglePopulationMap( const Population & N ) 
    : PopulationMap( N )
{
    _end = _dimN + 1;
}


SinglePopulationMap::~SinglePopulationMap()
{
}


/*
 * Reset the size of the arrays.  For Special Populations, this is trivial
 */

const PopulationMap&
SinglePopulationMap::dimension( const Population& N )
{
    assert( N.size() == _dimN );

    _maxN = N;
    return *this;
}



unsigned 
SinglePopulationMap::offset( const Population & N ) const
{
    assert( N.size() == _dimN );
    for ( unsigned j = 1; j <= _dimN; ++j ) {
	switch ( _maxN[j] - N[j] ) {
	case 0:
	    break;
	case 1:
	    return offset_e_j( N, j );
	default:
	    throw logic_error( "SinglePopulationMap::offset" );
	}
    }
    return _end - 1;
}



unsigned 
SinglePopulationMap::offset_e_j( const Population &N, const unsigned j ) const
{
    if ( j == 0 ) {
	return _end - 1;	/* Full population	*/
    } else {
	assert( j <= _dimN );
	return j - 1;
    }
}

