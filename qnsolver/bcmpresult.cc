/* -*- c++ -*-
 * $HeadURL: http://rads-svn.sce.carleton.ca:8080/svn/lqn/trunk-V5/qnsolver/bcmpresult.cc $
 *
 * SRVN command line interface.
 *
 * Copyright the Real-Time and Distributed Systems Group,
 * Department of Systems and Computer Engineering,
 * Carleton University, Ottawa, Ontario, Canada. K1S 5B6
 *
 * December 2020
 *
 * $Id: bcmpresult.cc 14324 2021-01-03 04:11:49Z greg $
 *
 * ------------------------------------------------------------------------
 */

#include <config.h>
#include <iostream>
#include <sstream>
#include <iomanip>
#include "bcmpresult.h"

std::streamsize BCMPResult::__width = 10;
std::streamsize BCMPResult::__precision = 6;
std::string BCMPResult::__separator = "*";


void
BCMPResult::get( const MVA& solver )
{
    for ( std::map<std::string,size_t>::const_iterator mx = stations().begin(); mx != stations().end(); ++mx ) {
	const size_t m = mx->second;
	for ( std::map<std::string,size_t>::const_iterator kx = classes().begin(); kx != classes().end(); ++kx ) {
	    const size_t k = kx->second;
	    Item item( Q[m].S(k),
		       Q[m].V(k),
		       solver.throughput( m, k, N() ),
		       solver.queueLength( m, k, N() ),
		       Q[m].R(k),
		       solver.utilization( m, k, N() ) );
	    _results.insert( result_pair( m, item_pair(k, item ) ) );
	}
    }
}


std::ostream&
BCMPResult::print( std::ostream& output ) const
{
    const std::streamsize old_precision = output.precision(__precision);
    const std::map<size_t,std::string> class_names = this->class_names();
    output.fill('*');
    output << std::setw(__width*6+7) << "*" << std::endl;
    output.fill(' ');
    output << __separator << std::setw(__width) << "name " << Item::header() << __separator << std::endl;
    output.fill('*');
    output << std::setw(__width*6+7) << "*" << std::endl;
    for ( std::map<std::string,size_t>::const_iterator mx = stations().begin(); mx != stations().end(); ++mx ) {
	const size_t m = mx->second;		/* Index for station */
	const std::pair<result_map::const_iterator,result_map::const_iterator> results = _results.equal_range( m );
	unsigned int count = 0;
	Item sum;
	for ( result_map::const_iterator result = results.first; result != results.second; ++result ) {
	    sum += result->second.second;
	    ++count;
	}
	sum.deriveStationAverage();
	
	/* Sum will work for single class too. */
	output.setf(std::ios::left, std::ios::adjustfield);
	output.fill(' ');
	if ( count >= 1 ) {
	    output << __separator << std::setw(__width) << " " << Item::blankline() << __separator << std::endl;
	}
	output << __separator << std::setw(__width) << ( " " + mx->first ) << sum << __separator << std::endl;
	if ( count >= 1 ) {
	    for ( result_map::const_iterator result = results.first; result != results.second; ++result ) {
		const item_pair& kx = result->second;
		output << __separator << std::setw(__width) <<  ( "(" + class_names.at(kx.first) + ")") << kx.second << __separator << std::endl;
	    }
	}
    }
    output << __separator << std::setw(__width) << " " << Item::blankline() << __separator << std::endl;
    output.fill('*');
    output << std::setw(__width*6+7) << "*" << std::endl;
    output.precision(old_precision);
    return output;
}

BCMPResult::Item& BCMPResult::Item::operator+=( const Item& addend )
{
//    _S = _S + addend._S;		/* Service Time (QNAP) 	*/
    _S = 0.0;
    _V = _V + addend._V;		/* Visits		*/
    _X = _X + addend._X;		/* Throughput		*/
    _L = _L + addend._L;		/* Length (n cust)	*/
    _R = 0.0;				/* Need to derive R	*/
    _U = _U + addend._U;		/* Utilization		*/
    return *this;
}


/*
 * Derive waiting time over all classes.
 */

BCMPResult::Item&
BCMPResult::Item::deriveStationAverage()
{
    if ( _X == 0 ) return *this;
    _S = _U / _X;
    _R = _L / _X;
    return *this;
}

std::ostream&
BCMPResult::Item::print( std::ostream& output ) const
{
    output.unsetf( std::ios::floatfield );
    output << __separator << std::setw(__width) << service_time()
	   << __separator << std::setw(__width) << utilization()
	   << __separator << std::setw(__width) << customers()
	   << __separator << std::setw(__width) << residence_time()		// per visit.
	   << __separator << std::setw(__width) << throughput();
    return output;
}


std::string
BCMPResult::Item::header()
{
    std::ostringstream output;
    output << __separator << std::setw(__width) << "service "
	   << __separator << std::setw(__width) << "busy pct "
	   << __separator << std::setw(__width) << "cust nb "
	   << __separator << std::setw(__width) << "response "
	   << __separator << std::setw(__width) << "thruput ";
    return output.str();
}

std::string
BCMPResult::Item::blankline()
{
    std::ostringstream output;
    for ( unsigned i = 0; i < 5; ++i ) {
	output << std::setfill(' ');
	output << __separator << std::setw(__width) << " ";
    }
    return output.str();
}

/*
 - mean value analysis ("mva") -
 *******************************************************************
 *  name    *  service * busy pct *  cust nb * response *  thruput *
 *******************************************************************
 *          *          *          *          *          *          *
 * terminal *0.2547    * 0.000    * 1.579    *0.2547    * 6.201    *
 *(c1      )*0.3333    * 0.000    * 1.417    *0.3333    * 4.250    *
 *(c2      )*0.8333E-01* 0.000    *0.1625    *0.8333E-01* 1.951    *
 *          *          *          *          *          *          *
 * p1       *0.4000    *0.8267    * 2.223    * 1.076    * 2.067    *
 *(c1      )*0.4000    *0.5667    * 1.512    * 1.067    * 1.417    *
 *(c2      )*0.4000    *0.2601    *0.7109    * 1.093    *0.6502    *
 *          *          *          *          *          *          *
 * p2       *0.2000    *0.6317    * 1.215    *0.3848    * 3.158    *
 *(c1      )*0.2000    *0.5667    * 1.071    *0.3780    * 2.833    *
 *(c2      )*0.2000    *0.6502E-01*0.1442    *0.4436    *0.3251    *
 *          *          *          *          *          *          *
 * p3       *0.7000    *0.6827    *0.9823    * 1.007    *0.9753    *
 *(c2      )*0.7000    *0.6827    *0.9823    * 1.007    *0.9753    *
 *          *          *          *          *          *          *
 *******************************************************************
              memory used:       4024 words of 4 bytes
               (  1.55  % of total memory)     
*/

