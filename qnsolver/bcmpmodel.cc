/* -*- c++ -*-
 * $HeadURL: http://rads-svn.sce.carleton.ca:8080/svn/lqn/trunk-V5/qnsolver/bcmpmodel.cc $
 *
 * SRVN command line interface.
 *
 * Copyright the Real-Time and Distributed Systems Group,
 * Department of Systems and Computer Engineering,
 * Carleton University, Ottawa, Ontario, Canada. K1S 5B6
 *
 * December 2020
 *
 * $Id: bcmpmodel.cc 14335 2021-01-05 04:10:54Z greg $
 *
 * ------------------------------------------------------------------------
 */

#include <config.h>
#include <string>
#include <sstream>
#include <algorithm>
#include <lqio/jmva_document.h>
#include <lqio/dom_extvar.h>
#include "bcmpmodel.h"
#include "bcmpresult.h"
#include <mva/fpgoop.h>
#include <mva/mva.h>
#include <mva/prob.h>

Model::Model( const BCMP::Model& model ) : _model(model), _index(), _reverse(), _N(), _Z(), _priority()
{
    try {
	_result = construct();
    }
    catch ( const std::domain_error& e ) {
	_result = false;
    }
}



void
Model::CreateClass::operator()( const BCMP::Model::Class::pair_t& input ) 
{
    const size_t k = index().size() + 1;
    index().insert( std::pair<std::string,size_t>( input.first, k ) );
    reverse().insert( std::pair<size_t,std::string>( k, input.first ) );

    N(k) = LQIO::DOM::to_unsigned(*input.second.customers());
    Z(k) = LQIO::DOM::to_double(*input.second.think_time());
    priority(k) = 0;
}

void
Model::CreateStation::CreateDemand::operator()( const BCMP::Model::Station::Demand::pair_t& input )
{
    const size_t k = indexAt(input.first);
    const BCMP::Model::Station::Demand& demand = input.second;	// From BCMP model.
    _server.setService( k, LQIO::DOM::to_double(*demand.service_time()) );
    _server.setVisits( k, LQIO::DOM::to_double(*demand.visits()) );
}

void
Model::CreateStation::operator()( const BCMP::Model::Station::pair_t& input )
{
    const size_t m = index().size() + 1;
    index().insert( std::pair<std::string,size_t>( input.first, m ) );
    reverse().insert( std::pair<size_t,std::string>( m, input.first ) );

    const BCMP::Model::Station& station = input.second;
    const size_t K = n_classes();
    Server * server = nullptr;
    switch ( station.scheduling() ) {
    case SCHEDULE_FIFO:		server = new FCFS_Server(K);	break;
    case SCHEDULE_DELAY:	server = new Infinite_Server(K);break;
    case SCHEDULE_PS:		server = new PS_Server(K);	break;
    default: abort(); break;
    }
    Q(m) = server;

    const BCMP::Model::Station::Demand::map_t& demands = station.demands();
    std::for_each ( demands.begin(), demands.end(), CreateDemand( _model, *server ) );
}

bool
Model::construct()
{
    /* Dimension the parameters */
    
    const size_t n_classes =  classes().size();
    const size_t n_stations = stations().size();
    _N.resize(n_classes);
    _Z.resize(n_classes);
    _priority.resize(n_classes);
    _Q.resize(n_stations);

    std::for_each( classes().begin(), classes().end(), CreateClass( *this ) );
    std::for_each( stations().begin(), stations().end(), CreateStation( *this ) );
    
    return true;
}

std::streamsize Model::__width = 10;
std::streamsize Model::__precision = 6;
std::string Model::__separator = "*";


std::ostream&
Model::print( std::ostream& output ) const
{
    const std::streamsize old_precision = output.precision(__precision);
    const std::map<size_t,std::string> class_names = _reverse.k;
    output.fill('*');
    output << std::setw(__width*6+7) << "*" << std::endl;
    output.fill(' ');
    output << __separator << std::setw(__width) << "name " << header() << __separator << std::endl;
    output.fill('*');
    output << std::setw(__width*6+7) << "*" << std::endl;
    for ( std::map<std::string,size_t>::const_iterator mx = _index.m.begin(); mx != _index.m.end(); ++mx ) {
	const size_t m = mx->second;		/* Index for station */
	const std::pair<BCMPResult::result_map::const_iterator,BCMPResult::result_map::const_iterator> results = _results->results().equal_range( m );
	unsigned int count = 0;
	BCMPResult::Item sum;
	for ( BCMPResult::result_map::const_iterator result = results.first; result != results.second; ++result ) {
	    sum += result->second.second;
	    ++count;
	}
	sum.deriveStationAverage();
	
	/* Sum will work for single class too. */
	output.setf(std::ios::left, std::ios::adjustfield);
	output.fill(' ');
	if ( count >= 1 ) {
	    output << __separator << std::setw(__width) << " " << Model::blankline() << __separator << std::endl;
	}
	output << __separator << std::setw(__width) << ( " " + mx->first );
	print(output,sum);
	output << __separator << std::endl;
	if ( count >= 1 ) {
	    for ( BCMPResult::result_map::const_iterator result = results.first; result != results.second; ++result ) {
		const BCMPResult::item_pair& kx = result->second;
		if (kx.second.throughput() == 0 ) continue;
		output << __separator << std::setw(__width) <<  ( "(" + class_names.at(kx.first) + ")");
		print(output,kx.second);
		output << __separator << std::endl;
	    }
	}
    }
    output << __separator << std::setw(__width) << " " << Model::blankline() << __separator << std::endl;
    output.fill('*');
    output << std::setw(__width*6+7) << "*" << std::endl;
    output.precision(old_precision);
    return output;
}

std::ostream&
Model::print( std::ostream& output, const BCMPResult::Item& item ) const
{
    output.unsetf( std::ios::floatfield );
    output << __separator << std::setw(__width) << item.service_time()
	   << __separator << std::setw(__width) << item.utilization()
	   << __separator << std::setw(__width) << item.customers()
	   << __separator << std::setw(__width) << item.residence_time()		// per visit.
	   << __separator << std::setw(__width) << item.throughput();
    return output;
}


std::string
Model::header()
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
Model::blankline()
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

