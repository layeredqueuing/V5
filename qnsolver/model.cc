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
#include <cstring>
#include <fstream>
#include <numeric>
#include <sstream>
#include <string>
#include <lqio/bcmp_bindings.h>
#include <lqio/qnio_document.h>
#include <lqio/dom_document.h>
#include <lqio/qnio_document.h>
#include <lqio/srvn_spex.h>
#include <lqio/glblerr.h>
#include <lqx/Program.h>
#include <lqx/SyntaxTree.h>
#include <mva/fpgoop.h>
#include <mva/multserv.h>
#include <mva/mva.h>
#include "boundsmodel.h"
#include "closedmodel.h"
#include "model.h"
#include "openmodel.h"
#include "pragma.h"
#include "runlqx.h"

bool Model::print_program = false;			/* Print LQX program		*/
bool Model::debug_flag = false;
bool Model::verbose_flag = false;			/* Print steps			*/
bool Model::no_execute = false;

std::map<const Model::Solver,const std::string> Model::__solver_name = {
    { Model::Solver::BOUNDS,		"bounds" },
    { Model::Solver::EXACT_MVA,		LQIO::DOM::Pragma::_exact_ },
    { Model::Solver::BARD_SCHWEITZER,	LQIO::DOM::Pragma::_schweitzer_ },
    { Model::Solver::LINEARIZER,	LQIO::DOM::Pragma::_linearizer_ },
    { Model::Solver::LINEARIZER2,	LQIO::DOM::Pragma::_fast_ },
    { Model::Solver::EXPERIMENTAL,	"experimental" },
    { Model::Solver::OPEN,		"open" }
};

Model::Model( QNIO::Document& input, Model::Solver solver, const std::string& output_file_name )
    : _model(input.model()), _solver(solver), 
      _result(false), _input(input), _output_file_name(output_file_name), _closed_model(nullptr), _open_model(nullptr), _bounds_model(nullptr), Q()
{
    const size_t M = _model.n_stations(type());	// Size based on type.
    Q.resize(M);
}


Model::Model( QNIO::Document& input, Model::Solver solver )
    : _model(input.model()), _solver(solver),
      _result(false), _input(input), _output_file_name(), _closed_model(nullptr), _open_model(nullptr), _bounds_model(nullptr), Q()
{
    const size_t M = _model.n_stations(type());	// Size based on type.
    Q.resize(M);
}


Model::~Model()
{
    if ( _bounds_model ) delete _bounds_model;
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
    _result = true;
    try {
	std::for_each( chains().begin(), chains().end(), CreateChainIndex( *this, type() ) );
	std::for_each( stations().begin(), stations().end(), CreateStationIndex( *this, type() ) );
	if ( isParent() ) {
	    if ( _solver == Solver::BOUNDS ) {
		_bounds_model = new BoundsModel( *this, _input );
		_bounds_model->construct();
	    } else {
		/* Create open and closed models */
		std::for_each( stations().begin(), stations().end(), InstantiateStation( *this ) );
		if ( _model.n_stations(BCMP::Model::Chain::Type::CLOSED) > 0 ) {
		    _closed_model = new ClosedModel( *this, _input, _solver );
		    _closed_model->construct();
		}
		if ( _model.n_stations(BCMP::Model::Chain::Type::OPEN) > 0 ) {
		    _open_model = new OpenModel( *this, _input );
		    _open_model->construct();
		}
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


bool
Model::solve()
{
    bool ok = true;
    LQX::Program * lqx = _input.getLQXProgram();
    if ( lqx != nullptr ) {
	LQX::Environment * environment = lqx->getEnvironment();
	_input.setEnvironment( environment );
	_input.registerExternalSymbolsWithProgram( lqx );
	if ( print_program ) {
	    lqx->print( std::cerr );
	    std::cerr << std::endl;
	}
	Pragma::noDefaultOutput( _input.disableDefaultOutputWithLQX() );	// Suppress default output

	FILE * output = nullptr;
	environment->getMethodTable()->registerMethod(new SolverInterface::Solve(*this));
	BCMP::RegisterBindings(environment, &_input.model());
	if ( !_output_file_name.empty() ) {
	    output = fopen( _output_file_name.c_str(), "w" );
	    if ( !output ) {
		runtime_error( LQIO::ERR_CANT_OPEN_FILE, _output_file_name.c_str(), strerror( errno ) );
		return false;
	    } else {
		environment->setDefaultOutput( output );      /* Default is stdout */
	    }
	}

	/* Invoke the LQX program itself */
	if ( !lqx->invoke() ) {
	    LQIO::runtime_error( LQIO::ERR_LQX_EXECUTION, _input.getInputFileName().c_str() );
	    LQX::SymbolTable* symbol_table = environment->getSymbolTable();
//	    LQX::SymbolTable* symbol_table = environment->getSpecialSymbolTable();
	    std::stringstream output;
	    symbol_table->dump( output );
	    std::cerr << output.str() << std::endl;
	    ok = false;
	} else if ( !SolverInterface::Solve::solveCallViaLQX ) {
	    /* There was no call to solve the LQX */
	    LQIO::runtime_error( LQIO::ADV_LQX_IMPLICIT_SOLVE, _input.getInputFileName().c_str() );
	    std::vector<LQX::SymbolAutoRef> args;
	    environment->invokeGlobalMethod("solve", &args);
	}
	if ( output ) {
	    fclose( output );
	}
	
    } else if ( !compute() ) {
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
	if ( verbose_flag ) std::cerr << "pre hook..." ;
	_input.preSolve();
	
	if ( verbose_flag ) std::cerr << "construct... ";

	if ( !construct() || !instantiate() ) return false;
	if ( no_execute ) return true;
	
	if ( verbose_flag ) std::cerr << "solve using " << __solver_name.at(solver()) << "... ";

	if ( _bounds_model ) {
	    _bounds_model->solve();
	}
	if ( _closed_model ) {
	    if ( _open_model ) {
		_open_model->convert( _closed_model );
	    }
	    if ( debug_flag ) _closed_model->debug( std::cout );
	    _closed_model->solve();
	    if ( debug_flag ) _closed_model->print( std::cout );
	}
	if ( _open_model ) {
	    if ( debug_flag ) _open_model->debug( std::cout );
	    _open_model->solve( _closed_model );
	    if ( debug_flag ) _open_model->print( std::cout );
	}
	saveResults();

	if ( verbose_flag ) std::cerr << "post hook..." ;
	_input.postSolve();
	
	if ( Pragma::defaultOutput() ) {
	    if ( _output_file_name.empty() ) {
		print( std::cout );
	    } else {
		std::ofstream output;
		output.open( _output_file_name, std::ios::app );
		if ( !output ) {
		    runtime_error( LQIO::ERR_CANT_OPEN_FILE, _output_file_name.c_str(), strerror( errno ) );
		} else {
		    print( output );
		}
		output.close();
	    }
	}
	if ( verbose_flag ) std::cerr << "done." ;
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


std::ostream&
Model::print( std::ostream& output ) const
{
    _input.print( output );
    return output;
}

void
Model::saveResults()
{
    if ( _bounds_model ) {
	_bounds_model->saveResults();
    }
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


double
Model::getDoubleValue( LQX::SyntaxTreeNode * variable ) const
{
    if ( variable == nullptr ) return 0.0;
    LQX::SymbolAutoRef symbol = variable->invoke( _model.environment() );
    if ( symbol->getType() == LQX::Symbol::SYM_DOUBLE ) return symbol->getDoubleValue();
    else if ( symbol->getType() != LQX::Symbol::SYM_NULL ) throw std::domain_error( std::string("invalid double") );
    else return 0.0;
}



unsigned int
Model::getUnsignedValue( LQX::SyntaxTreeNode * variable, unsigned int default_value ) const
{
    if ( variable == nullptr ) return default_value;
    const double value = getDoubleValue( variable );
    if ( value != rint(value) ) throw std::domain_error( std::string("invalid integer") + std::to_string(value) );
    return static_cast<unsigned int>(value);
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
	const double service_time = getDoubleValue( demand.service_time() );
	const double visits = getDoubleValue( demand.visits() );
	if ( closed_model() != nullptr && chain.isClosed() ) {
	    const size_t k = indexAt(chain.type(),input.first);
	    _server.setService( k, service_time );
	    _server.setVisits( k, visits );
	} else if ( open_model() != nullptr && chain.isOpen() ) {
	    static const size_t k = 0;
	    const size_t e = indexAt(chain.type(),input.first);
	    const double arrival_rate = getDoubleValue( chain.arrival_rate() );
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
	    N[k] = getUnsignedValue( ki->second.customers() );
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
    const unsigned int copies = getUnsignedValue( station.copies(), 1 );
    
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
	    if ( dynamic_cast<Conway_Multi_Server *>(Q(m)) == nullptr || copies != Q(m)->getMarginalProbabilitiesSize() ) {
		Q(m) = replace_server( input.first, Q(m), new Conway_Multi_Server(copies,E,K) );
	    }
	    break;

	case Multiserver::DEFAULT:
	case Multiserver::REISER:
	case Multiserver::REISER_PS:
	    if ( station.scheduling() == SCHEDULE_FIFO ) {
		if ( dynamic_cast<Reiser_Multi_Server *>(Q(m)) == nullptr || copies != Q(m)->getMarginalProbabilitiesSize() ) {
		    Q(m) = replace_server( input.first, Q(m), new Reiser_Multi_Server(copies,E,K) );
		}
	    } else if ( station.scheduling() == SCHEDULE_PS ) {
		if ( dynamic_cast<Reiser_PS_Multi_Server *>(Q(m)) == nullptr || copies != Q(m)->getMarginalProbabilitiesSize() ) {
		    Q(m) = replace_server( input.first, Q(m), new Reiser_PS_Multi_Server(copies,E,K) );
		}
	    } else {
		throw std::runtime_error( "Invalid scheduling for load dependent server " + input.first );
	    }
	    break;

	case Multiserver::ROLIA:
	case Multiserver::ROLIA_PS:
	    if ( station.scheduling() == SCHEDULE_FIFO ) {
		if ( dynamic_cast<Rolia_Multi_Server *>(Q(m)) == nullptr || copies != Q(m)->mu() ) {
		    Q(m) = replace_server( input.first, Q(m), new Rolia_Multi_Server(copies,E,K) );
		}
	    } else if ( station.scheduling() == SCHEDULE_PS ) {
		if ( dynamic_cast<Rolia_PS_Multi_Server *>(Q(m)) == nullptr || copies != Q(m)->mu() ) {
		    Q(m) = replace_server( input.first, Q(m), new Rolia_Multi_Server(copies,E,K) );
		}
	    } else {
		throw std::runtime_error( "Invalid scheduling for load dependent server " + input.first );
	    }
	    break;

	case Multiserver::ZHOU:
	    if ( dynamic_cast<Zhou_Multi_Server *>(Q(m)) == nullptr || copies != Q(m)->mu() ) {
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

std::ostream& Model::debug( std::ostream& output ) const
{
#if 0
    for ( BCMP::Model::Station::map_t::const_iterator mi = stations().begin(); mi != stations().end(); ++mi ) {
	const BCMP::Model::Station::Class::map_t& classes = mi->second.classes();
	const unsigned int servers = getUnsignedValue( mi->second.copies(), 1 );
	output << "Station " << mi->first << ": servers=" << servers << std::endl;
	for ( BCMP::Model::Station::Class::map_t::const_iterator ki = classes.begin(); ki != classes.end(); ++ki ) {
	    const double visits = getDoubleValue( ki->second.visits() );
	    const double service_time = getDoubleValue( ki->second.service_time() );
	    output << "    Class " << ki->first << ": visits=" << visits << ", service time=" << service_time << std::endl;
	}
    }
#else
    for ( Vector<Server *>::const_iterator q = Q.begin(); q != Q.end(); ++q ) {
	output << **q << std::endl;
    }
#endif
    return output;
}
