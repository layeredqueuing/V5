/* -*- c++ -*-
 * $HeadURL: http://rads-svn.sce.carleton.ca:8080/svn/lqn/trunk-V5/qnsolver/closedmodel.cc $
 *
 * SRVN command line interface.
 *
 * Copyright the Real-Time and Distributed Systems Group,
 * Department of Systems and Computer Engineering,
 * Carleton University, Ottawa, Ontario, Canada. K1S 5B6
 *
 * December 2020
 *
 * $Id: closedmodel.cc 14440 2021-02-02 12:44:31Z greg $
 *
 * ------------------------------------------------------------------------
 */

#include "config.h"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <numeric>
#include <sstream>
#include <string>
#include <lqio/bcmp_bindings.h>
#include <lqio/jmva_document.h>
#include <lqio/srvn_spex.h>
#include <lqio/glblerr.h>
#include <lqx/Program.h>
#include <mva/multserv.h>
#include <mva/mva.h>
#include "closedmodel.h"
#include "model.h"
#include "openmodel.h"
#include "pragma.h"
#include "runlqx.h"

bool print_spex = false;				/* Print LQX program		*/
bool debug_flag = false;

Model::Model( BCMP::JMVA_Document& input, Model::Solver solver, const std::string& output_file_name )
    : _model(input.model()), _solver(solver),
      Q(), _result(false), _input(input), _output_file_name(output_file_name), _closed_model(nullptr), _open_model(nullptr)
{
    const size_t M = _model.n_stations(type());	// Size based on type.
    if ( M == 0 ) {
	_result = false;
	return;
    }

    Q.resize(M);
}


Model::Model( BCMP::JMVA_Document& input, Model::Solver solver )
    : _model(input.model()), _solver(solver),
      Q(), _result(false), _input(input), _output_file_name(), _closed_model(nullptr), _open_model(nullptr)
{
    const size_t M = _model.n_stations(type());	// Size based on type.
    if ( M == 0 ) {
	_result = false;
	return;
    }

    Q.resize(M);
}


Model::~Model()
{
    if ( _closed_model ) delete _closed_model;
    if ( _open_model ) delete _open_model;
}


/*
 * Dimension the parameters.  This is done once.  Instantiate will set
 * the values for the MVA sovler.
 */

bool
Model::construct()
{
    try {
	_result = true;
	std::for_each( chains().begin(), chains().end(), CreateChainIndex( *this, type() ) );
	std::for_each( stations().begin(), stations().end(), CreateStationIndex( *this, type() ) );
	if ( isParent() ) {
	    /* Create open and closed models */
	    std::for_each( stations().begin(), stations().end(), InstantiateStation( *this ) );
	    if (  _model.n_stations(BCMP::Model::Chain::Type::CLOSED) > 0 ) {
		_closed_model = new ClosedModel( *this, _input, _solver );
		_closed_model->construct();
	    }
	    if (  _model.n_stations(BCMP::Model::Chain::Type::OPEN) > 0 ) {
		_open_model = new OpenModel( *this, _input );
		_open_model->construct();
	    }
	}
    }
    catch ( const std::domain_error& e ) {
	_result = false;
    }
    return _result;
}

/*
 * (Re)Create the stations in the parent, otherwise, copy the stations
 * from the parent to the child.
 */

bool
Model::instantiate()
{
    assert( isParent() );
    try { 
	std::for_each( stations().begin(), stations().end(), InstantiateStation( *this ) );
	if ( _open_model ) _open_model->instantiate();
	if ( _closed_model ) _closed_model->instantiate();
    }
    catch ( const std::domain_error& e ) {
	return false;
    }
    return true;
}


/*
 * For each chain, find the station with the highest demand, and find the total demand.
 * Highest demand gives throughput bound, total demand gives response bound.
 */

void
Model::bounds()
{
    std::map<std::string,BCMP::Model::Bound> bounds;
    for ( BCMP::Model::Chain::map_t::const_iterator chain = chains().begin(); chain != chains().end(); ++chain ) {
	bounds.emplace( chain->first, BCMP::Model::Bound(*chain,stations()) );
    }
}


bool
Model::solve()
{
    bool ok = true;
    LQX::Program * lqx = _input.getLQXProgram();
    std::vector<LQX::SyntaxTreeNode *> * program = _input.getSPEXProgram();
    expr_list result_vars;		/* needs to be scoped here, else println args are cleared */
    if ( program != nullptr ) {
	if ( lqx != nullptr ) {
	    LQIO::solution_error( LQIO::ERR_LQX_SPEX, _input.getInputFileName().c_str() );
	    return false;
	}
		
	/* Get the result variables and convert to an expression list for construct */
	const std::vector<LQIO::Spex::var_name_and_expr>& result_variables = LQIO::Spex::result_variables();
	for ( std::vector<LQIO::Spex::var_name_and_expr>::const_iterator result = result_variables.begin(); result != result_variables.end(); ++result ) {
	    result_vars.push_back( result->second );
	}
	if ( !LQIO::spex.construct_program( program, &result_vars, nullptr, _input.getGNUPlotProgram() ) ) return false;
	lqx = LQX::Program::loadRawProgram( program );
    }

    if ( lqx != nullptr ) {
	_input.registerExternalSymbolsWithProgram( lqx );
	if ( print_spex ) {
	    lqx->print( std::cerr );
	    std::cerr << std::endl;
	}
	FILE * output = nullptr;
	LQX::Environment * environment = lqx->getEnvironment();
	environment->getMethodTable()->registerMethod(new SolverInterface::Solve(*this));
	BCMP::RegisterBindings(environment, &_input.model());
	if ( !_output_file_name.empty() ) {
	    output = fopen( _output_file_name.c_str(), "w" );
	    if ( !output ) {
		solution_error( LQIO::ERR_CANT_OPEN_FILE, _output_file_name.c_str(), strerror( errno ) );
		return false;
	    } else {
		environment->setDefaultOutput( output );      /* Default is stdout */
	    }
	}
	/* Invoke the LQX program itself */
	if ( !lqx->invoke() ) {
	    LQIO::solution_error( LQIO::ERR_LQX_EXECUTION, _input.getInputFileName().c_str() );
	    ok = false;
	} else if ( !SolverInterface::Solve::solveCallViaLQX ) {
	    /* There was no call to solve the LQX */
	    LQIO::solution_error( LQIO::ADV_LQX_IMPLICIT_SOLVE, _input.getInputFileName().c_str() );
	    std::vector<LQX::SymbolAutoRef> args;
	    environment->invokeGlobalMethod("solve", &args);
	}
	if ( output ) {
	    fclose( output );
	}
	
    } else if ( compute() ) {
	if ( _output_file_name.empty() ) {
	    print( std::cout );
	} else {
	    std::ofstream output;
	    output.open( _output_file_name.c_str(), std::ios::out );
	    if ( !output ) {
		solution_error( LQIO::ERR_CANT_OPEN_FILE, _output_file_name.c_str(), strerror( errno ) );
	    } else {
		print ( output );
	    }
	    output.close();
	}

    } else {
	ok = false;
    }

    return ok;
}


/*
 * Instantiate the variables, then invoke the various solvers.  Mixed
 * models are the most complicated.
 */

bool
Model::compute()
{
    bool ok = true;
    try {
	instantiate();
	
	if ( _closed_model ) {
	    if ( _open_model ) {
		_open_model->convert( _closed_model );
	    }
	    if ( debug_flag ) _closed_model->debug(std::cout);
	    _closed_model->solve();
	    if ( debug_flag ) _closed_model->print(std::cout);
	}
	if ( _open_model ) {
	    if ( debug_flag ) _open_model->debug(std::cout);
	    _open_model->solve( _closed_model );
	    if ( debug_flag ) _open_model->print(std::cout);
	}
	saveResults();
    }
    catch ( const floating_point_error& error ) {
	std::cerr << LQIO::io_vars.lq_toolname << ": floating point error - " << error.what() << std::endl;
	LQIO::io_vars.error_count += 1;
	ok = false;
    }
    catch ( const std::runtime_error& error ) {
	throw LQX::RuntimeException( error.what() );
	ok = false;
    }
    catch ( const std::logic_error& error ) {
	throw LQX::RuntimeException( error.what() );
	ok = false;
    }
    return ok;
}


void
Model::saveResults()
{
    if ( _closed_model ) {
	_closed_model->saveResults();
    }
    if ( _open_model ) {
	_open_model->saveResults();
    }
}

size_t
Model::indexAt(BCMP::Model::Chain::Type type, const std::string& name) const
{
    if ( type == BCMP::Model::Chain::Type::CLOSED ) {
	return _closed_model->_index.k.at(name);
    } else {
	return _open_model->_index.k.at(name);
    }
}

/*
 * Create and index entry only if chain is of the right type (or not defined).
 */

void
Model::CreateChainIndex::operator()( const BCMP::Model::Chain::pair_t& k ) 
{
    if ( _type != BCMP::Model::Chain::Type::UNDEFINED && k.second.type() != _type ) return;
    index().emplace( k.first, index().size() + 1 );
}

/*
 * Create and index entry only if station is of the right type (or not defined).
 */

void
Model::CreateStationIndex::operator()( const BCMP::Model::Station::pair_t& m )
{
    if ( _type != BCMP::Model::Chain::Type::UNDEFINED && !m.second.any_of( chains(), _type ) ) return;
    index().emplace( m.first, index().size() + 1 );
}

/*
 * For open models, index 0 is used.  Classes are respresented using
 * the entries.  If variables are not set (i.e., construction prior to
 * LQX running), just set dummy values.
 */

void
Model::InstantiateStation::InstantiateClass::operator()( const BCMP::Model::Station::Class::pair_t& input )
{
    try {
	const BCMP::Model::Chain& chain = chainAt(input.first);
	const BCMP::Model::Station::Class& demand = input.second;	// From BCMP model.
	const double service_time = demand.service_time()->wasSet() ? LQIO::DOM::to_double( *demand.service_time() ) : 0.;
	const double visits = demand.visits()->wasSet() ? LQIO::DOM::to_double( *demand.visits() ) : 0.;
	if ( closed_model() != nullptr && chain.isClosed() ) {
	    const size_t k = indexAt(chain.type(),input.first);
	    _server.setService( k, service_time );
	    _server.setVisits( k, visits );
	} else if ( open_model() != nullptr && chain.isOpen() ) {
	    static const size_t k = 0;
	    const size_t e = indexAt(chain.type(),input.first);
	    const double arrival_rate = chain.arrival_rate()->wasSet() ? LQIO::DOM::to_double( *chain.arrival_rate() ) : 0.;
	    _server.setService( e, k, service_time );
	    _server.setVisits( e, k, visits * arrival_rate );
	}
    }
    catch ( const std::out_of_range& e ) {
	/* Open class, ignore */
    }
}

/*
 * Set the population vector in case there's a mulitserver
 */

Model::InstantiateStation::InstantiateStation( const Model& model ) : _model(model), N()
{
    if ( closed_model() != nullptr ) {
	const size_t K = _model._model.n_chains(BCMP::Model::Chain::Type::CLOSED);
	N.resize(K);
	for ( BCMP::Model::Chain::map_t::const_iterator ki = chains().begin(); ki != chains().end(); ++ki ) {
	    if ( !ki->second.isClosed() ) continue;
	    const size_t k = indexAt( BCMP::Model::Chain::Type::CLOSED, ki->first );	/* Grab index from closed model */
	    N[k] = ki->second.customers()->wasSet() ? to_unsigned( *ki->second.customers() ) : 1;
	}
    }
}

/*

 * (Re)Create the stations as neccesary and udpate the Q vector in the
 * parent and children.
 */

void
Model::InstantiateStation::operator()( const BCMP::Model::Station::pair_t& input )
{
    const size_t m = indexAt(input.first);
    const size_t K = model().n_chains(BCMP::Model::Chain::Type::CLOSED);
    /* If this station has open clases, then the number of those, otherwise 1 */
    const size_t E = std::max( static_cast<size_t>(1), input.second.count_if( chains(), BCMP::Model::Chain::Type::CLOSED ) );
    
    /* Swap stations if necessary */
    
    const BCMP::Model::Station& station = input.second;
    const unsigned int copies = station.copies()->wasSet() ? LQIO::DOM::to_unsigned( *station.copies() ) : 1;
    
    if ( station.type() == BCMP::Model::Station::Type::DELAY || station.scheduling() == SCHEDULE_DELAY ) {
	if ( copies != 1 ) {
	} else if ( dynamic_cast<Infinite_Server *>(Q(m)) == nullptr ) {
	    Q(m) = replace_server( input.first, Q(m), new Infinite_Server(E,K) );
	}

    } else if ( !Pragma::forceMultiserver() && station.type() == BCMP::Model::Station::Type::LOAD_INDEPENDENT ) {
	if ( copies != 1 ) {
	    throw std::runtime_error( "Number of servers does not equal 1 for load independent server " + input.first );
	} else if ( station.scheduling() == SCHEDULE_FIFO ) {
	    if ( dynamic_cast<FCFS_Server *>(Q(m)) == nullptr ) {
		Q(m) = replace_server( input.first, Q(m), new FCFS_Server(E,K) );
	    }
	} else if ( station.scheduling() == SCHEDULE_PS ) {
	    if ( dynamic_cast<PS_Server *>(Q(m)) == nullptr ) {
		Q(m) = replace_server( input.first, Q(m), new PS_Server(E,K) );
	    }
	} else {
	    throw std::runtime_error( "Invalid scheduling for load independent server " + input.first );
	}

    } else {
	switch ( Pragma::multiserver() ) {
	case Multiserver::CONWAY:
	    if ( dynamic_cast<Conway_Multi_Server *>(Q(m)) == nullptr || copies != Q(m)->marginalProbabilitiesSize() ) {
		Q(m) = replace_server( input.first, Q(m), new Conway_Multi_Server(copies,E,K) );
	    }
	    break;

	case Multiserver::DEFAULT:
	case Multiserver::REISER:
	case Multiserver::REISER_PS:
	    if ( station.scheduling() == SCHEDULE_FIFO ) {
		if ( dynamic_cast<Reiser_Multi_Server *>(Q(m)) == nullptr || copies != Q(m)->marginalProbabilitiesSize() ) {
		    Q(m) = replace_server( input.first, Q(m), new Reiser_Multi_Server(copies,E,K) );
		}
	    } else if ( station.scheduling() == SCHEDULE_PS ) {
		if ( dynamic_cast<Reiser_PS_Multi_Server *>(Q(m)) == nullptr || copies != Q(m)->marginalProbabilitiesSize() ) {
		    Q(m) = replace_server( input.first, Q(m), new Reiser_PS_Multi_Server(copies,E,K) );
		}
	    } else {
		throw std::runtime_error( "Invalid scheduling for load dependent server " + input.first );
	    }
	    break;

	case Multiserver::ROLIA:
	case Multiserver::ROLIA_PS:
	    if ( station.scheduling() == SCHEDULE_FIFO ) {
		if ( dynamic_cast<Rolia_Multi_Server *>(Q(m)) == nullptr || copies != Q(m)->copies() ) {
		    Q(m) = replace_server( input.first, Q(m), new Rolia_Multi_Server(copies,E,K) );
		}
	    } else if ( station.scheduling() == SCHEDULE_PS ) {
		if ( dynamic_cast<Rolia_PS_Multi_Server *>(Q(m)) == nullptr || copies != Q(m)->copies() ) {
		    Q(m) = replace_server( input.first, Q(m), new Rolia_Multi_Server(copies,E,K) );
		}
	    } else {
		throw std::runtime_error( "Invalid scheduling for load dependent server " + input.first );
	    }
	    break;

	case Multiserver::ZHOU:
	    if ( dynamic_cast<Zhou_Multi_Server *>(Q(m)) == nullptr || copies != Q(m)->copies() ) {
		Q(m) = replace_server( input.first, Q(m), new Zhou_Multi_Server(copies,E,K) );
	    }
	    break;

	default:
	    abort();
	}

    }

    const BCMP::Model::Station::Class::map_t& classes = station.classes();
    std::for_each ( classes.begin(), classes.end(), InstantiateClass( _model, *Q(m) ) );
}


/*
 * Return the new station.  Update the indecies if they were set previously.
 */

Server * 
Model::InstantiateStation::replace_server( const std::string& name, Server * old_server, Server * new_server ) const
{
    size_t closed_index = 0;
    size_t open_index = 0;
    if ( old_server ) {
	closed_index = old_server->closedIndex;
	open_index = old_server->openIndex;
	delete old_server;
    }
    new_server->setMarginalProbabilitiesSize( N );
    new_server->closedIndex = closed_index;
    new_server->openIndex = open_index;

    /* Update the Q[m] in the appropriate submodel(s) */
    if ( closed_model() != nullptr ) {
	size_t m = closed_model()->_index.m.at(name);
	closed_model()->Q[m] = new_server;
    }
    if ( open_model() != nullptr ) {
	size_t m = open_model()->_index.m.at(name);
	open_model()->Q[m] = new_server;
    }
    return new_server;
}

std::streamsize Model::__width = 10;
std::streamsize Model::__precision = 6;
std::string Model::__separator = "*";


std::ostream&
Model::print( std::ostream& output ) const
{
    const std::streamsize old_precision = output.precision(__precision);
//    output << " - (" << _solver << ") - " << std::endl;
    output.fill('*');
    output << std::setw(__width*6+7) << "*" << std::endl;
    output.fill(' ');
    output << __separator << std::setw(__width) << "name " << header() << __separator << std::endl;
    output.fill('*');
    output << std::setw(__width*6+7) << "*" << std::endl;
    output.fill(' ');
    output << __separator << std::setw(__width) << " " << Model::blankline() << __separator << std::endl;
    for ( BCMP::Model::Station::map_t::const_iterator mi = stations().begin(); mi != stations().end(); ++mi ) {
	const size_t m = _index.m.at(mi->first);

	const BCMP::Model::Station::Class::map_t& results = mi->second.classes();
	const BCMP::Model::Station::Class sum = std::accumulate( std::next(results.begin()), results.end(), results.begin()->second, &BCMP::Model::Station::sumResults ).deriveResidenceTime();
	const double service = sum.throughput() > 0 ? sum.utilization() / sum.throughput() : 0.0;
	
	/* Sum will work for single class too. */
	output.setf(std::ios::left, std::ios::adjustfield);
	output << __separator << std::setw(__width) << ( " " + mi->first );
	print(output,service,sum);
	output << __separator << std::endl;
	if ( results.size() > 1 ) {
	    for ( BCMP::Model::Station::Class::map_t::const_iterator result = results.begin(); result != results.end(); ++result ) {
		if (result->second.throughput() == 0 ) continue;
		output << __separator << std::setw(__width) <<  ( "(" + result->first + ")");
		const BCMP::Model::Chain& chain = _model.chainAt(result->first);
		if ( chain.isClosed() ) {
		    const size_t k = indexAt(BCMP::Model::Chain::Type::CLOSED,result->first);
		    print(output,Q[m]->S(k),result->second);
		} else {
		    static const size_t k = 0;
		    const size_t e = indexAt(BCMP::Model::Chain::Type::OPEN,result->first);
		    print(output,Q[m]->S(e,k),result->second);
		}
		output << __separator << std::endl;
	    }
	}
	output << __separator << std::setw(__width) << " " << Model::blankline() << __separator << std::endl;
    }
    output.fill('*');
    output << std::setw(__width*6+7) << "*" << std::endl;
    output.fill(' ');
    output.precision(old_precision);
    return output;
}

std::ostream&
Model::print( std::ostream& output, double service_time, const BCMP::Model::Station::Result& item ) const
{
    output.unsetf( std::ios::floatfield );
    output << __separator << std::setw(__width) << service_time
	   << __separator << std::setw(__width) << item.utilization()
	   << __separator << std::setw(__width) << item.queue_length()
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
