/* -*- c++ -*-
 * $Id: qnap2_document.cpp 15731 2022-06-29 18:22:10Z greg $
 *
 * Read in XML input files.
 *
 * Copyright the Real-Time and Distributed Systems Group,
 * Department of Systems and Computer Engineering,
 * Carleton University, Ottawa, Ontario, Canada. K1S 5B6
 *
 * ------------------------------------------------------------------------
 * December 2020.
 * ------------------------------------------------------------------------
 */

#if HAVE_CONFIG_H
#include <config.h>
#endif
#include <algorithm>
#include <cstring>
#include <iomanip>
#include <iterator>
#include <numeric>
#include <regex>
#include <sstream>
#include <lqx/SyntaxTree.h>
#include "common_io.h"
#include "dom_entry.h"
#include "dom_phase.h"
#include "dom_processor.h"
#include "dom_task.h"
#include "error.h"
#include "glblerr.h"
#include "input.h"
#include "qnap2_document.h"

namespace BCMP {

    std::map<scheduling_type,std::string> QNAP2_Document::__scheduling_str = {
	{ SCHEDULE_CUSTOMER,"infinite" },
	{ SCHEDULE_DELAY,   "infinite" },
	{ SCHEDULE_FIFO,    "fifo" },
	{ SCHEDULE_PPR,     "prior" },
	{ SCHEDULE_PS, 	    "ps" }
    };

    QNAP2_Document::QNAP2_Document( const std::string& input_file_name, const BCMP::Model& model ) :
	_input_file_name(input_file_name), _model(model)
    {
    }

    /*
     * "&" is a comment.
     * identifiers are limited to the first 8 characters.
     * input is limited to 80 characters.
     * statements are terminated with ";"
     * If there are SPEX variables, then use those.
     */

    std::ostream&
    QNAP2_Document::print( std::ostream& output ) const
    {
	std::ios_base::fmtflags flags = output.setf( std::ios::left, std::ios::adjustfield );
	output << "& " << LQIO::DOM::Common_IO::svn_id() << std::endl;
	if ( LQIO::io_vars.lq_command_line.size() > 0 ) {
	    output << "& " << LQIO::io_vars.lq_command_line << std::endl;
	}


	/* Special stations for closed and open classes */
	bool has_closed_chain = false;
	bool has_open_chain = false;
	std::string customers;		/* Special station names */
	Model::Station::map_t::const_iterator customer = std::find_if( stations().begin(), stations().end(), &Model::Station::isCustomer );
	if ( customer != stations().end() ) {
	    has_closed_chain = true;
	    const std::string name = customer->first;
	    _model.computeCustomerDemand( name );	/* compute terminal service times based on visits */
	    customers = name;
	}
	Model::Chain::map_t::const_iterator chain = std::find_if( chains().begin(), chains().end(), &Model::Chain::openChain );
	if ( chain != chains().end() ) {
	    has_open_chain = true;
	    const std::string name = "source";
	    Model::Station& source = const_cast<Model&>(model()).insertStation( name, Model::Station::Type::SOURCE ).first->second;
	    source.insertClass( chain->first, nullptr, chain->second.arrival_rate() );
	    if ( !customers.empty() ) customers += ',';
	    customers += name;
	}


	/* 1) Declare all SPEX variables */
	output << qnap2_keyword( "declare" ) << std::endl;
	std::set<const LQIO::DOM::ExternalVariable *> symbol_table;
	const std::string customer_vars = std::accumulate( chains().begin(), chains().end(), std::string(""), getIntegerVariables( model(), symbol_table ) );
	if ( !customer_vars.empty() )   output << qnap2_statement( "integer " + customer_vars, "SPEX customers vars." ) << std::endl;
	const std::string integer_vars  = std::accumulate( stations().begin(), stations().end(), std::string(""), getIntegerVariables( model(), symbol_table ) );
	if ( !integer_vars.empty() )    output << qnap2_statement( "integer " + integer_vars, "SPEX multiplicity vars." ) << std::endl;
	const std::string real_vars    	= std::accumulate( stations().begin(), stations().end(), std::string(""), getRealVariables( model(), symbol_table ) );
	if ( !real_vars.empty() )       output << qnap2_statement( "real " + real_vars, "SPEX service time vars." ) << std::endl;
	const std::string deferred_vars	= std::accumulate( Spex::inline_expressions().begin(), Spex::inline_expressions().end(), std::string(""), &getDeferredVariables );
	if ( !deferred_vars.empty() )   output << qnap2_statement( "real " + deferred_vars, "SPEX deferred vars." ) << std::endl;
	const std::string result_vars	= std::accumulate( Spex::result_variables().begin(), Spex::result_variables().end(), std::string(""), getResultVariables(symbol_table) );
	if ( !result_vars.empty() )     output << qnap2_statement( "real " + result_vars, "SPEX result vars." ) << std::endl;

	/* 2) Declare all stations */
	output << qnap2_statement( "queue " + std::accumulate( stations().begin(), stations().end(), customers, fold_station() ), "Station identifiers" ) << std::endl;

	/* 3) Declare the chains */
	if ( !multiclass() ) {
	    if ( has_closed_chain ) {
		output << qnap2_statement( "integer n_users", "Population" ) << std::endl
		       << qnap2_statement( "real think_t", "Think time." ) << std::endl;
	    }
	    if ( has_open_chain ) {
		output << qnap2_statement( "real arrivals", "Arrival Rate." ) << std::endl;
	    }
	    output << qnap2_statement( "real " + std::accumulate( stations().begin(), stations().end(), std::string(""), fold_station( "_t") ), "Station service time" ) << std::endl;
	} else {
	    /* Variables */
	    output << qnap2_statement( "class string name", "Name (for output)" ) << std::endl;		// LQN client.
	    if ( has_closed_chain ) {
		output << qnap2_statement( "class integer n_users", "Population." ) << std::endl
		       << qnap2_statement( "class real think_t", "Think time." ) << std::endl;
	    }
	    if ( has_open_chain ) {
		output << qnap2_statement( "real arrivals", "Arrival Rate." ) << std::endl;
	    }
	    output << qnap2_statement( "class real " + std::accumulate( stations().begin(), stations().end(), std::string(""), fold_station( "_t") ), "Station service time" ) << std::endl;
	    /* Chains */
	    output << qnap2_statement( "class " + std::accumulate( chains().begin(), chains().end(), std::string(""), Model::Chain::fold() ), "Class names" ) << std::endl;
	}

	/* 4) output the statations */
	std::for_each( stations().begin(), stations().end(), printStation( output, model() ) );		// Stations.

	/* Output control stuff if necessary */
	if ( multiclass() || !Spex::result_variables().empty() ) {
	    output << "&" << std::endl
		   << qnap2_keyword( "control" ) << std::endl;
	    if ( multiclass() ) {
		output << qnap2_statement( "class=all queue", "Compute for all classes" ) << std::endl;	// print for all classes.
	    }
	    if ( !Spex::result_variables().empty() ) {
		output << qnap2_statement( "option=nresult", "Suppress default output" ) << std::endl;
	    }
	}

	/* 6) Finally, assign the parameters defined in steps 2 & 3 from the constants and variables in the model */

	output << "&" << std::endl
	       << qnap2_keyword( "exec" ) << std::endl
	       << "   begin" << std::endl;

	if ( !Spex::result_variables().empty() && !LQIO::Spex::__no_header ) {
	    printResultsHeader( output, Spex::result_variables() );
	}

	if ( Spex::input_variables().size() > Spex::array_variables().size() ) {	// Only care about scalars
	    output << "&  -- SPEX scalar variables --" << std::endl;
	    std::for_each( Spex::scalar_variables().begin(), Spex::scalar_variables().end(), printSPEXScalars( output ) );	/* Scalars */
	}

	/* Insert QNAP for statements for arrays and completions. */
	if ( !Spex::array_variables().empty() ) {
	    output << "&  -- SPEX arrays and completions --" << std::endl;
	    std::for_each( Spex::array_variables().begin(), Spex::array_variables().end(), for_loop( output ) );
	}

	if ( !Spex::inline_expressions().empty() ) {
	    output << "&  -- SPEX deferred assignments --" << std::endl;
	    std::for_each( Spex::inline_expressions().begin(), Spex::inline_expressions().end(), printSPEXDeferred( output ) );	/* Arrays and completions */
	}

	output << "&  -- Class variables --" << std::endl;
        printClassVariables( output );

	output << "&  -- Station variables --" << std::endl;
	std::for_each( stations().begin(), stations().end(), printStationVariables( output, model() ) );

	/* Let 'er rip! */
	output << "&  -- Let 'er rip! --" << std::endl;
	output << qnap2_statement( "solve" ) << std::endl;
	if ( !Spex::result_variables().empty() ) {
	    output << "&  -- SPEX results for QNAP2 solutions are converted to" << std::endl
		   << "&  -- the LQN output for throughput, service and waiting time." << std::endl
		   << "&  -- QNAP2 throughput for a reference task is per-slice," << std::endl
		   << "&  -- and not the aggregate so divide by the number of transits." << std::endl
		   << "&  -- Service time is mservice() + sum of mresponse()." << std::endl
		   << "&  -- Waiting time is mresponse() - mservice()." << std::endl;
	    /* Output statements to have Qnap2 compute LQN results */
	    std::for_each( Spex::result_variables().begin(), Spex::result_variables().end(), getObservations( output, model() ) );
	    /* Print them */
	    printResults( output, Spex::result_variables() );
	}

	/* insert end's for each for. */
	std::for_each( Spex::array_variables().rbegin(), Spex::array_variables().rend(), end_for( output ) );

	/* End of program */
	output << qnap2_statement( "end" ) << std::endl;
        output.flags(flags);

	output << qnap2_keyword( "end" ) << std::endl;

	return output;
    }

    /*
     * Think times. Only add an item to the string if it's a variable (the name), and the variable
     * has not been seen before.
     */

    std::string
    QNAP2_Document::getIntegerVariables::operator()( const std::string& s1, const Model::Chain::pair_t& k ) const
    {
	std::string s = s1;
	if ( k.second.isClosed() && dynamic_cast<const LQIO::DOM::SymbolExternalVariable *>(k.second.customers()) && _symbol_table.insert(k.second.customers()).second == true ) {
	    if ( !s.empty() ) s += ",";
	    s += k.second.customers()->getName();
	}
	return s;
    }

    std::string
    QNAP2_Document::getRealVariables::operator()( const std::string& s1, const Model::Chain::pair_t& k ) const
    {
	std::string s = s1;
	if ( k.second.isClosed() && dynamic_cast<const LQIO::DOM::SymbolExternalVariable *>(k.second.think_time()) && _symbol_table.insert(k.second.think_time()).second == true ) {
	    if ( !s.empty() ) s += ",";
	    s += k.second.think_time()->getName();
	} else if ( k.second.isOpen() && dynamic_cast<const LQIO::DOM::SymbolExternalVariable *>(k.second.arrival_rate()) && _symbol_table.insert(k.second.arrival_rate()).second == true ) {
	    if ( !s.empty() ) s += ",";
	    s += k.second.arrival_rate()->getName();
	}
	return s;
    }

    std::string
    QNAP2_Document::getRealVariables::operator()( const std::string& s1, const Model::Station::pair_t& m ) const
    {
	const Model::Station::Class::map_t& classes = m.second.classes();
	return std::accumulate( classes.begin(), classes.end(), s1, getRealVariables( model(), _symbol_table ) );
    }

    /*
     * Collect all variables.  Only add an itemm to the string if it's
     * a variable (the name), and the variable has not been seen
     * before (insert will return true).
     */

    std::string
    QNAP2_Document::getIntegerVariables::operator()( const std::string& s1, const Model::Station::pair_t& m ) const
    {
	std::string s = s1;
	if ( Model::Station::isServer(m) && dynamic_cast<const LQIO::DOM::SymbolExternalVariable *>(m.second.copies()) && _symbol_table.insert(m.second.copies()).second == true ) {
	    if ( !s.empty() ) s += ",";
	    s += m.second.copies()->getName();
	}
	return s;
    }
    
    std::string
    QNAP2_Document::getRealVariables::operator()( const std::string& s1, const Model::Station::Class::pair_t& demand ) const
    {
	if ( dynamic_cast<const LQIO::DOM::SymbolExternalVariable *>(demand.second.service_time()) && _symbol_table.insert(demand.second.service_time()).second == true ) {
	    std::ostringstream ss;
	    ss << s1;
	    if ( !s1.empty() ) ss << ",";
	    ss << *demand.second.service_time();		/* Will print out name */
	    return ss.str();
	} else {
	    return s1;
	}
    }


    std::string
    QNAP2_Document::getDeferredVariables( const std::string& s1, const std::pair<const DOM::ExternalVariable *, const LQX::SyntaxTreeNode *>& p2 )
    {
	std::string s3;
	if ( !s1.empty() ) {
	    return s1 + "," + p2.first->getName();
	} else {
	    return p2.first->getName();
	}
    }


    /*
     * Convert External Variables to Strings.  Exclude them from the
     * list as they have been declared earlier.
     */

    QNAP2_Document::getResultVariables::getResultVariables( const std::set<const LQIO::DOM::ExternalVariable *>& symbol_table ) : _symbol_table()
    {
	for ( std::set<const LQIO::DOM::ExternalVariable *>::const_iterator var = symbol_table.begin(); var != symbol_table.end(); ++var ) {
	    if ( !dynamic_cast<const LQIO::DOM::SymbolExternalVariable *>(*var) ) continue;
	    _symbol_table.insert(dynamic_cast<const LQIO::DOM::SymbolExternalVariable *>(*var)->getName());
	}
    }

    std::string
    QNAP2_Document::getResultVariables::operator()( const std::string& s1, const Spex::var_name_and_expr& var ) const
    {
	if ( _symbol_table.find(var.first) != _symbol_table.end() ) {
	    return s1;
	} else if ( !s1.empty() ) {
	    return s1 + "," + var.first;
	} else {
	    return var.first;
	}
    }

    /*
     * Prints stations.
     */

    void
    QNAP2_Document::printStation::operator()( const Model::Station::pair_t& m ) const
    {
	const Model::Station& station = m.second;
	_output << "&" << std::endl
		<< "/station/ name=" << m.first << ";" << std::endl;
	switch ( station.type() ) {
	case Model::Station::Type::SOURCE:
	    _output << qnap2_statement( "type=source" ) << std::endl;
	    printInterarrivalTime();
	    printCustomerTransit();
	    return;
	case Model::Station::Type::DELAY:
	    _output << qnap2_statement( "type=" + __scheduling_str.at(SCHEDULE_DELAY) ) << std::endl;
	    break;
	case Model::Station::Type::LOAD_INDEPENDENT:
	case Model::Station::Type::MULTISERVER:
	    try {
		_output << qnap2_statement( "sched=" + __scheduling_str.at(station.scheduling()) ) << std::endl;
	    }
	    catch ( const std::out_of_range& ) {
		LQIO::runtime_error( LQIO::WRN_SCHEDULING_NOT_SUPPORTED,
				      scheduling_label[static_cast<unsigned int>(station.scheduling())].str,
				      "station",
				      m.first.c_str() );
	    }
	    if ( station.type() == Model::Station::Type::MULTISERVER ) {
		_output << qnap2_statement( "type=multiple(" + to_unsigned(station.copies()) + ")" ) << std::endl;
	    } else if ( !LQIO::DOM::ExternalVariable::isDefault( station.copies() ), 1.0 ) {
	    } 
	    break;
	case Model::Station::Type::NOT_DEFINED:
	    throw std::range_error( "QNAP2_Document::printStation::operator(): Undefined station type." );
	}

	/* Print out service time variables and visits for non-special stations */

	if ( station.reference() ) {
	    _output << qnap2_statement( "init=n_users", "Population by class" ) << std::endl
		    << qnap2_statement( "service=exp(think_t)" ) << std::endl;
	    printCustomerTransit();
	} else {
	    _output << qnap2_statement( "service=exp(" + m.first + "_t)" ) << std::endl;
	    printServerTransit( m );
	}
    }

    void
    QNAP2_Document::printStation::printCustomerTransit() const
    {
	if ( !multiclass() ) {
	    _output << qnap2_statement("transit=" + std::accumulate( stations().begin(), stations().end(), std::string(""), fold_transit(chains().begin()->first) ), "visits to servers" ) << std::endl;
	} else {
	    for ( Model::Chain::map_t::const_iterator k = chains().begin(); k != chains().end(); ++k ) {
		_output << qnap2_statement("transit(" + k->first + ")=" + std::accumulate( stations().begin(), stations().end(), std::string(""), fold_transit(k->first) ), "visits to servers" ) << std::endl;
	    }
	}
    }

    /*
     * If I have mixed models, then I have to transit by class
     */

    void
    QNAP2_Document::printStation::printServerTransit( const Model::Station::pair_t& m ) const
    {
	const Model::Station::Class::map_t& classes = m.second.classes();
	const std::string closed_classes = std::accumulate( classes.begin(), classes.end(), std::string(), fold_class( chains(), Model::Chain::Type::CLOSED ) );
	const std::string open_classes = std::accumulate( classes.begin(), classes.end(), std::string(), fold_class( chains(), Model::Chain::Type::OPEN ) );
	const Model::Station::map_t::const_iterator terminal = std::find_if( stations().begin(), stations().end(), &Model::Station::isCustomer );

	if ( !closed_classes.empty() & !open_classes.empty() ) {
	    _output << qnap2_statement( "transit( " + closed_classes + ")=" + terminal->first );
	    _output << qnap2_statement( "transit( " + open_classes + ")=out" );
	} else {
	    std::string name;
	    if ( terminal != stations().end() ) name = terminal->first;
	    else name = "out";
	    if ( multiclass() ) {
		_output << qnap2_statement( "transit(all class)=" + name ) << std::endl;
	    } else {
		_output << qnap2_statement( "transit=" + name ) << std::endl;
	    }
	}
    }

    /*
     * Convert arrival rate to inter-arrival time, then adjust for the
     * fact that the visits for open chains are converted to a
     * probability in QNAP2 by multipling the rate by the total visits
     * to all open classes.
     */

    void
    QNAP2_Document::printStation::printInterarrivalTime() const
    {
	if ( !multiclass() ) {
	    const std::string visits = std::accumulate( stations().begin(), stations().end(), std::string(""), fold_visits( chains().begin()->first ) );
	    _output << qnap2_statement( "service=exp(1./((" + visits + ")*arrivals))", "Convert to inter-arrival time" ) << std::endl;
	} else {
	    for ( Model::Chain::map_t::const_iterator k = chains().begin(); k != chains().end(); ++k ) {
		const std::string visits = std::accumulate( stations().begin(), stations().end(), std::string(""), fold_visits( k->first ) );
		_output << qnap2_statement( "service=(" + k->first + ")=exp(1./((" + visits + ")*arrivals))", "Convert to inter-arrival time." ) << std::endl;
	    }
	}
    }

    std::string
    QNAP2_Document::fold_transit::operator()( const std::string& s1, const Model::Station::pair_t& m2 ) const
    {
	const Model::Station& station = m2.second;
	if ( station.reference() || !station.hasClass(_name) || LQIO::DOM::ExternalVariable::isDefault(station.classAt(_name).visits(),0.) ) {
	    return s1;
	} else {
	    std::string s = s1;
	    if ( !s.empty() ) s += ",";
	    s += m2.first + "," + to_real( station.classAt(_name).visits() );
	    return s;
	}
    }

    void
    QNAP2_Document::printClassVariables( std::ostream& output ) const
    {
	/*
	 * Sum service time over all clients and visits over all
	 * servers.  The demand at the reference station is the demand
	 * (service time) at the reference station but the visits over
	 * all non-customer stations.
	 */

	const Model::Station::Class::map_t visits = std::accumulate( stations().begin(), stations().end(), Model::Station::Class::map_t(), Model::Station::select( &Model::Station::isServer ) );
	const Model::Station::Class::map_t service_times = std::accumulate( stations().begin(), stations().end(), Model::Station::Class::map_t(), Model::Station::select( &Model::Station::isCustomer ) );
	Model::Station::Class::map_t classes = std::accumulate( service_times.begin(), service_times.end(), Model::Station::Class::map_t(), Model::sum_visits(visits) );

	/*
	 * QNAP Does everything by "transit", so the service time at
	 * the terminal station has to be adjusted by the number of
	 * visits to all other stations.  Let QNAP do it as it's
	 * easier to see the values from the origial file. Force
	 * floating point math.
	 */

	for ( Model::Chain::map_t::const_iterator k = chains().begin(); k != chains().end(); ++k ) {
	    if ( !k->second.isClosed() ) continue;
	    std::string comment;
	    std::ostringstream think_time;
	    std::ostringstream customers;
	    think_time << *classes.at(k->first).service_time();
	    customers  << *k->second.customers();
	    if ( !k->second.customers()->wasSet() ) {
		comment = "SPEX variable " + k->second.customers()->getName();
	    }
	    std::string think_visits = std::accumulate( stations().begin(), stations().end(), std::string(""), fold_visits( k->first ) );
	    if ( multiclass() ) {
		output << qnap2_statement( k->first + ".name:=\"" + k->first + "\"", "Class (client) name" ) << std::endl;
		output << qnap2_statement( k->first + ".think_t:=" + think_time.str() + "/(" + think_visits + ")", "Slice time at client" ) << std::endl;
		output << qnap2_statement( k->first + ".n_users:=" + customers.str(), comment  ) << std::endl;
	    } else {
		output << qnap2_statement( "think_t:=" + think_time.str() + "/(" + think_visits + ")", "Slice time at client" ) << std::endl;
		output << qnap2_statement( "n_users:=" + customers.str(), comment  ) << std::endl;
	    }
	}
    }


    /*
     * Print out all services times.  Since they are all external
     * variables *var will print either the value or the name of the
     * variable.
     */

    void
    QNAP2_Document::printStationVariables::operator()( const Model::Station::pair_t& m ) const
    {
	const Model::Station& station = m.second;
	for ( Model::Chain::map_t::const_iterator k = chains().begin(); k != chains().end(); ++k ) {
	    std::string name;

	    if ( station.type() == Model::Station::Type::SOURCE ) {
		name = "arrivals";
	    } else if ( station.reference() ) {
		continue;
	    } else {
		name = m.first + "_t";
	    }

	    std::string time;
	    std::string comment;
	    if ( !station.hasClass(k->first) || LQIO::DOM::ExternalVariable::isDefault(station.classAt(k->first).service_time(),0.) ) {
		time = "0.000001";		// Qnap doesn't like zero for service time.
		comment = "QNAP does not like zero (0)";
	    } else {
		const LQIO::DOM::ExternalVariable * service_time = station.classAt(k->first).service_time();
		if ( !service_time->wasSet() ) {
		    time = service_time->getName();
		    comment = "SPEX variable " + service_time->getName();
		} else {
		    time = to_real(service_time);
		}
	    }
	    if ( multiclass() ) {
		_output << qnap2_statement( k->first + "." + name + ":=" + time, comment ) << std::endl;
	    } else {
		_output << qnap2_statement(  name + ":=" + time, comment ) << std::endl;
	    }
	}
    }


    /*
     * Print out all deferred assignment variables (they go inside all loop bodies that
     * arise for array and completion assignements
     */

    void
    QNAP2_Document::printSPEXScalars::operator()( const std::string& var ) const
    {
	const std::map<std::string,LQX::SyntaxTreeNode *>::const_iterator expr = Spex::input_variables().find(var);

	if ( !expr->second ) return;		/* Comprehension or array.  I could check array_variables. */
	std::ostringstream ss;
	ss << expr->first << ":=";
	expr->second->print(ss,0);
	std::string s(ss.str());
	s.erase(std::remove(s.begin(), s.end(), ' '), s.end());	/* Strip blanks */
	_output << qnap2_statement( s ) << std::endl;	/* Swaps $ to _ and appends ;	*/
    }

    void
    QNAP2_Document::printSPEXDeferred::operator()( const std::pair<const DOM::ExternalVariable *, const LQX::SyntaxTreeNode *>& expr ) const
    {
	std::ostringstream ss;
	ss << expr.first->getName() << ":=";
	expr.second->print(ss,0);
	std::string s(ss.str());
	s.erase(std::remove(s.begin(), s.end(), ' '), s.end());	/* Strip blanks */
	_output << qnap2_statement( s ) << std::endl;	/* Swaps $ to _ and appends ;	*/
    }


    /*
     * Print out the Header for the results.   Format the same as printResults (next)
     */

    void
    QNAP2_Document::printResultsHeader( std::ostream& output, const std::vector<Spex::var_name_and_expr>& vars ) const
    {
	std::string s;
	std::string comment = "SPEX results";
	bool continuation = false;
	size_t count = 0;
	for ( std::vector<Spex::var_name_and_expr>::const_iterator var = vars.begin(); var != vars.end(); ++var ) {
	    ++count;
	    if ( s.empty() && continuation ) s += "\",\",";	/* second print statement, signal continuation with "," */
	    else if ( !s.empty() ) s += ",\",\",";		/* between vars. */
	    s += "\"" + var->first + "\"";
	    if ( count > 6 ) {
		output << qnap2_statement( "print(" + s + ")", comment ) << std::endl;
		s.clear();
		count = 0;
		continuation = true;
		comment = "... continued";
	    }
	}
	if ( !s.empty() ) {
	    output << qnap2_statement( "print(" + s + ")", comment ) << std::endl;
	}
    }


    /*
     * Print out the SPEX result variables in chunks as QNAP2 doesn't like BIG print statements.
     */

    void
    QNAP2_Document::printResults( std::ostream& output, const std::vector<Spex::var_name_and_expr>& vars ) const
    {
	std::string s;
	std::string comment = "SPEX results";
	bool continuation = false;
	size_t count = 0;
	for ( std::vector<Spex::var_name_and_expr>::const_iterator var = vars.begin(); var != vars.end(); ++var ) {
	    ++count;
	    if ( s.empty() && continuation ) s += "\",\",";	/* second print statement, signal continuation with "," */
	    else if ( !s.empty() ) s += ",\",\",";		/* between vars. */
	    s += var->first;
	    if ( count > 6 ) {
		output << qnap2_statement( "print(" + s + ")", comment ) << std::endl;
		s.clear();
		count = 0;
		continuation = true;
		comment = "... continued";
	    }
	}
	if ( !s.empty() ) {
	    output << qnap2_statement( "print(" + s + ")", comment ) << std::endl;
	}
    }


    /*
     * Generate for loops;
     */

    void
    QNAP2_Document::for_loop::operator()( const std::string& var ) const
    {
	const std::map<std::string,Spex::ComprehensionInfo>::const_iterator comprehension = Spex::comprehensions().find( var );
	std::string loop;
	std::ostringstream ss;
	if ( comprehension != Spex::comprehensions().end() ) {
	    /* Comprehension */
	    /* FOR i:=1 STEP n UNTIL m DO... */
	    ss << comprehension->second.getInit() << " step " << comprehension->second.getStep() << " until " << comprehension->second.getTest();
	    loop = ss.str();
	} else {
	    /* Array variable */
	    /* FOR i:=1,2,3... DO */
	    const std::map<std::string,LQX::SyntaxTreeNode *>::const_iterator array = Spex::input_variables().find(var);
	    array->second->print(ss);
	    loop = ss.str();
	    /* Get rid of brackets and spaces from array_create(0.5, 1, 1.5) */
	    loop.erase(loop.begin(), loop.begin()+13);	/* "array_create(" */
	    loop.erase(loop.end()-1,loop.end());	/* ")" */
	    loop.erase(std::remove(loop.begin(), loop.end(), ' '), loop.end());	/* Strip blanks */
	}
	std::string name = var;
	std::replace( name.begin(), name.end(), '$', '_'); 		// Make variables acceptable for QNAP2.
	_output << "   " << "for "<< name << ":=" << loop << " do begin" << std::endl;
    }

    /*
     * Sequence through the stations and classes looking for the result variables.
     */

    /* mservice, mbusypct, mcustnb, vcustnb, mresponse, mthruput, custnb */


    void
    QNAP2_Document::getObservations::operator()( const Spex::var_name_and_expr& var ) const
    {
	static const std::map<const BCMP::Model::Result::Type,QNAP2_Document::getObservations::f> key_map = {
	    { BCMP::Model::Result::Type::QUEUE_LENGTH,   &getObservations::get_waiting_time },
	    { BCMP::Model::Result::Type::RESIDENCE_TIME, &getObservations::get_service_time },
	    { BCMP::Model::Result::Type::THROUGHPUT,     &getObservations::get_throughput },
	    { BCMP::Model::Result::Type::UTILIZATION,    &getObservations::get_utilization }
	};

	for ( BCMP::Model::Station::map_t::const_iterator m = stations().begin(); m != stations().end(); ++m ) {

	    /* Check for station results */

	    const BCMP::Model::Result::map_t& station_variables = m->second.resultVariables();
	    for ( BCMP::Model::Result::map_t::const_iterator r = station_variables.begin(); r != station_variables.end(); ++r ) {
		if ( r->second != var.first ) continue;
		const std::map<const BCMP::Model::Result::Type,f>::const_iterator key = key_map.find( r->first );
		if ( key != key_map.end() ) {
		    std::pair<std::string,std::string> expression;
		    expression = (this->*(key->second))( m->first, std::string() );
		    _output << qnap2_statement( var.first + ":=" + expression.first, expression.second ) << std::endl;
		}
	    }

	    /* Check for class results */

	    for ( BCMP::Model::Station::Class::map_t::const_iterator k = m->second.classes().begin(); k != m->second.classes().end(); ++k ) {
		const BCMP::Model::Result::map_t& class_variables = k->second.resultVariables();
		for ( BCMP::Model::Result::map_t::const_iterator r = class_variables.begin(); r != class_variables.end(); ++r ) {
		    if ( r->second != var.first ) continue;
		    const std::map<const BCMP::Model::Result::Type,f>::const_iterator key = key_map.find( r->first );
		    if ( key != key_map.end() ) {
			std::pair<std::string,std::string> expression;
			expression = (this->*(key->second))( m->first, k->first );
			_output << qnap2_statement( var.first + ":=" + expression.first, expression.second ) << std::endl;
		    }
		}
	    }
	}
    }

    std::pair<std::string,std::string>
    QNAP2_Document::getObservations::get_throughput( const std::string& station_name, const std::string& class_name ) const
    {
	std::string result;
	std::string comment;
	const BCMP::Model::Station& station = stations().at(station_name);
	result = "mthruput(" + station_name;
	if ( station.reference() ) {
	    /* Report class results for the customers; the station name is the class name */
	    if ( multiclass() ) result += "," + class_name;
	    result += ")";
	    const std::string think_visits = std::accumulate( stations().begin(), stations().end(), std::string(""), fold_visits( class_name ) );
	    if ( !think_visits.empty() ) result += "/(" + think_visits + ")";
	    comment = "Convert to LQN throughput";
	} else {
	    if ( !class_name.empty() ) result += "," + class_name;
	    result += ")";
	}
	return std::pair<std::string,std::string>(result,comment);
    }

    /*
     * Derive for a multiserver.  QNAP gives odd numbers.
     */
    
    std::pair<std::string,std::string>
    QNAP2_Document::getObservations::get_utilization( const std::string& station_name, const std::string& class_name ) const
    {
	std::string result;
	std::string comment;
	const BCMP::Model::Station& station = stations().at(station_name);
	if ( station.type() == Model::Station::Type::MULTISERVER ) {
	    result = "mthruput";
	} else {
	    result = "mbusypct";
	}
	result += "(" + station_name;
	if ( !class_name.empty() ) result += "," + class_name;
	result += ")";
	if ( station.type() == Model::Station::Type::MULTISERVER ) {
	    result += "*(";
	    if ( !class_name.empty() ) result += to_real(station.classAt(class_name).service_time());
	    else {
		const BCMP::Model::Station::Class::map_t classes = station.classes();
		result += to_real(classes.begin()->second.service_time());
	    }
	    result += ")";
	    comment = "Dervived for multiserver.";
	}
	return std::pair<std::string,std::string>(result,comment);
    }

    /*
     * KEY_SERVICE is mservice + sum_of mresponse
     */

    std::pair<std::string,std::string>
    QNAP2_Document::getObservations::get_service_time( const std::string& station_name, const std::string& class_name ) const
    {
	std::string result;
	std::string comment;
	const BCMP::Model::Station& station = stations().at(station_name);
	if ( station.reference() ) {
	    result =  "mservice(" + station_name;
	    if ( multiclass() ) result += "," + class_name;
	    result += ")";
	    const std::string visits = std::accumulate( stations().begin(), stations().end(), std::string(""), fold_visits( class_name ) );
	    if ( !visits.empty() ) result += "*(" + visits + ")";	// Over all visits
	    const std::string response = std::accumulate( stations().begin(), stations().end(), std::string(""), fold_mresponse( class_name, chains() ) );
	    if ( !response.empty() ) result += "+(" + response +")";
	    comment = "Convert to LQN service time";
	} else {
	    result =  "mresponse(" + station_name;
	    if ( multiclass() && !class_name.empty() ) result += "," + class_name;
	    result += ")";
	    comment = "Convert to LQN service time.";
	}
	return std::pair<std::string,std::string>(result,comment);
    }

    /*
     * KEY_WAITING is mresponse-mservice.
     */

    std::pair<std::string,std::string>
    QNAP2_Document::getObservations::get_waiting_time( const std::string& station_name, const std::string& class_name  ) const
    {
	std::string result;
	std::string comment;
	result = "mresponse(" + station_name + ")-mservice(" + station_name;
	if ( multiclass() && !class_name.empty() ) result += "," + class_name;
	result += ")";
	comment = "Convert to LQN queueing time";
	return std::pair<std::string,std::string>(result,comment);
    }

    void
    QNAP2_Document::end_for::operator()( const std::string& var ) const
    {
	std::string comment = "for " + var;
	std::replace( comment.begin(), comment.end(), '$', '_'); 	// Make variables acceptable for QNAP2.
	_output << qnap2_statement( "end", comment ) << std::endl;
    }

    std::string
    QNAP2_Document::fold_station::operator()( const std::string& s1, const Model::Station::pair_t& s2 ) const
    {
	if ( s2.second.reference() || s2.second.type() == Model::Station::Type::SOURCE ) return s1;
	else if ( s1.empty() ) {
	    return s2.first + _suffix;
	} else {
	    return s1 + "," + s2.first + _suffix;
	}
    }

    std::string
    QNAP2_Document::fold_class::operator()( const std::string& s1, const Model::Station::Class::pair_t& k2 ) const
    {
	if ( _chains.at(k2.first).type() != _type ) return s1;
	else if ( s1.empty() ) {
	    return k2.first;
	} else {
	    return s1 + "," + k2.first;
	}
    }

    std::string
    QNAP2_Document::fold_visits::operator()( const std::string& s1, const Model::Station::pair_t& m2 ) const
    {
	const Model::Station& station = m2.second;
	if ( !station.hasClass(_name) || station.reference() ) return s1;	/* Don't visit self */
	const LQIO::DOM::ExternalVariable * visits = station.classAt(_name).visits();
	if ( LQIO::DOM::ExternalVariable::isDefault(visits,0.) ) return s1;	/* ignore zeros */
	std::string s2 = to_real( visits );
	if ( s1.empty() ) {
	    return s2;
	} else {
	    return s1 + "+" + s2;
	}
    }

    std::string
    QNAP2_Document::fold_mresponse::operator()( const std::string& s1, const Model::Station::pair_t& m2 ) const
    {
	const Model::Station& station = m2.second;
	if ( !station.hasClass(_name) || station.reference() ) return s1;	/* Don't visit self */
	std::string s2 = "mresponse(" + m2.first;
	if ( multiclass() ) {
	    s2 += "," + _name;
	}
	s2 += ")";
	if ( s1.empty() ) {
	    return s2;
	} else {
	    return s1 + "+" + s2;
	}
    }


    /*
     * Output either the name of the variable, or a real
     * constant for QNAP2 (append a . if none found).
     */

    std::string
    QNAP2_Document::to_real( const LQIO::DOM::ExternalVariable* v )
    {
	std::string str;
	if ( v->wasSet() ) {
	    char buf[16];
	    snprintf( buf, 16, "%g", to_double(*v) );
	    str = buf;
	    if ( str.find( '.' ) == std::string::npos ) {
		str += ".";	/* Force real */
	    }
	} else {
	    str = v->getName();
	}
	return str;
    }

    std::string
    QNAP2_Document::to_unsigned( const LQIO::DOM::ExternalVariable* v )
    {
	std::string str;
	if ( v->wasSet() ) {
	    char buf[16];
	    snprintf( buf, 16, "%g", to_double(*v) );
	    str = buf;
	    if ( str.find( '.' ) != std::string::npos ) {
		throw std::domain_error( "Invalid integer" );
	    }
	} else {
	    str = v->getName();
	}
	return str;
    }

    /* static */ std::ostream&
    QNAP2_Document::printKeyword( std::ostream& output, const std::string& s1, const std::string& s2 )
    {
	output << "/" << s1 << "/";
	if ( !s2.empty() ) {
	    output << " " << s2 << ";";
	}
	return output;
    }

    /*
     * Format for QNAP output.  Swap all $ to _. I may have to line
     * wrap as lines are limited to 80 characters (fortran).  If s2 is
     * present, it's a comment.
     */

    /* static */ std::ostream&
    QNAP2_Document::printStatement( std::ostream& output, const std::string& s1, const std::string& s2 )
    {
	bool first = true;
	std::string::const_iterator brk = s1.end();
	for ( std::string::const_iterator begin = s1.begin(); begin != s1.end(); begin = brk ) {
	    bool in_quote = false;

	    if ( s1.end() - begin < 60 ) {
		brk = s1.end();
	    } else {
		/* look for ',' not in quotes. */
		for ( std::string::const_iterator curr = begin; curr - begin < 60 && curr != s1.end(); ++curr ) {
		    if ( *curr == '"' ) in_quote = !in_quote;
		    if ( !in_quote && (*curr == ',' || *curr == '+' || *curr == '*' || *curr == ')' ) ) brk = curr;
		}
	    }

	    /* Take everthing up to brk and make variables variables acceptable for QNAP2. */
	    std::string buffer( brk - begin + (first ? 0 : 1), ' ' );	/* reserve space */
	    std::replace_copy( begin, brk, (first ? buffer.begin(): std::next(buffer.begin())), '$', '_');
//	    std::replace_copy( begin, brk, std::back_inserter<std::string>(buffer.begin()), '$', '_');

	    /* Output the buffer */
	    output << "   ";
	    if ( brk != s1.end() ) {
		output << buffer << std::endl;		/* break the line 	*/
	    } else if ( s2.empty() ) {
		output << buffer << ";";		/* Don't fill 		*/
	    } else {
		output << std::setw(60) << (buffer + ';') << "& " << s2;
	    }
	    first = false;
	}
	return output;
    }
}
