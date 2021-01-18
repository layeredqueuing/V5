/* -*- c++ -*-
 * $Id: qnap2_document.cpp 14379 2021-01-18 14:38:37Z greg $
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
#include "srvn_gram.h"

namespace BCMP {

    std::map<scheduling_type,std::string> QNAP2_Document::__scheduling_str;
    std::map<int,QNAP2_Document::getObservations::f> QNAP2_Document::getObservations::__key_map;	/* Maps srvn_gram.h KEY_XXX to qnap2 function */

    QNAP2_Document::QNAP2_Document( const std::string& input_file_name, const BCMP::Model& model ) :
	_input_file_name(input_file_name), _model(model)
    {
	if ( __scheduling_str.empty() ) {
	    __scheduling_str[SCHEDULE_CUSTOMER] = "infinite";
	    __scheduling_str[SCHEDULE_DELAY]    = "infinite";
	    __scheduling_str[SCHEDULE_FIFO]     = "fifo";
	    __scheduling_str[SCHEDULE_PPR]      = "prior";
	    __scheduling_str[SCHEDULE_PS]       = "ps";

	    getObservations::__key_map[KEY_THROUGHPUT]		    = &getObservations::get_throughput;
	    getObservations::__key_map[KEY_UTILIZATION]		    = &getObservations::get_utilization;
	    getObservations::__key_map[KEY_PROCESSOR_UTILIZATION]   = &getObservations::get_utilization;
	    getObservations::__key_map[KEY_PROCESSOR_WAITING]	    = &getObservations::get_waiting_time;
	    getObservations::__key_map[KEY_SERVICE_TIME]	    = &getObservations::get_service_time;
	    getObservations::__key_map[KEY_WAITING]		    = &getObservations::get_waiting_time;
	}
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
	/* compute terminal service times based on visits */
	const std::string name = std::find_if( stations().begin(), stations().end(), &Model::Station::isCustomer )->first;
	const std::pair<const std::string,const Model::Station::Demand::map_t> terminal( name, _model.computeCustomerDemand( name ) );

	/* 1) Declare all SPEX variables */
	output << qnap2_keyword( "declare" ) << std::endl;
	std::set<const LQIO::DOM::ExternalVariable *> symbol_table;
	const std::string integer_vars 	= std::accumulate( classes().begin(), classes().end(), std::string(""), getVariables( model(), symbol_table ) );
	if ( !integer_vars.empty() )    output << qnap2_statement( "integer " + integer_vars, "SPEX customers vars." ) << std::endl;
	const std::string real_vars    	= std::accumulate( stations().begin(), stations().end(), std::string(""), getVariables( model(), symbol_table ) );
	if ( !real_vars.empty() )       output << qnap2_statement( "real " + real_vars, "SPEX service time vars." ) << std::endl;
	const std::string deferred_vars	= std::accumulate( Spex::inline_expressions().begin(), Spex::inline_expressions().end(), std::string(""), &getDeferredVariables );
	if ( !deferred_vars.empty() )   output << qnap2_statement( "real " + deferred_vars, "SPEX deferred vars." ) << std::endl;
	const std::string result_vars	= std::accumulate( Spex::result_variables().begin(), Spex::result_variables().end(), std::string(""), getResultVariables(symbol_table) );
	if ( !result_vars.empty() )     output << qnap2_statement( "real " + result_vars, "SPEX result vars." ) << std::endl;

	/* 2) Declare all stations */
	output << qnap2_statement( "queue " + std::accumulate( stations().begin(), stations().end(), terminal.first, Model::Station::fold() ), "Station identifiers" ) << std::endl;

	/* 3) Declare the classes */
	if ( !multiclass() ) {
	    output << qnap2_statement( "integer n_users", "Population" ) << std::endl
		   << qnap2_statement( "real think_t", "Think time." ) << std::endl
		   << qnap2_statement( "real " + std::accumulate( stations().begin(), stations().end(), std::string(""), Model::Station::fold( "_t") ), "Station service time" ) << std::endl;
	} else {
	    /* Variables */
	    output << qnap2_statement( "class string name", "Name (for output)" ) << std::endl		// LQN client.
		   << qnap2_statement( "class integer n_users", "Population." ) << std::endl
		   << qnap2_statement( "class real think_t", "Think time." ) << std::endl
		   << qnap2_statement( "class real " + std::accumulate( stations().begin(), stations().end(), std::string(""), Model::Station::fold( "_t") ), "Station service time" ) << std::endl;
	    /* Classes */
	    output << qnap2_statement( "class " + std::accumulate( classes().begin(), classes().end(), std::string(""), Model::Class::fold() ), "Class names" ) << std::endl;
	}

	/* 4) output the statations */
	std::for_each( stations().begin(), stations().end(), printStation( output, model() ) );	// Stations.


	/* Output control stuff if necessary */

	if ( multiclass() || !Spex::observations().empty() ) {
	    output << "&" << std::endl
		   << qnap2_keyword( "control" ) << std::endl;
	}
	if ( multiclass() ) {
	    output << qnap2_statement( "class=all queue", "Compute for all classes" ) << std::endl;	// print for all classes.
	}
	if ( !Spex::observations().empty() ) {
	    output << qnap2_statement( "option=nresult", "Suppress default output" ) << std::endl;
	}
	
	/* 6) Finally, assign the parameters defined in steps 2 & 3 from the constants and variables in the model */

	output << "&" << std::endl
	       << qnap2_keyword( "exec" ) << std::endl
	       << "   begin" << std::endl;

	if ( !Spex::result_variables().empty() && !LQIO::Spex::__no_header ) {
	    const std::string result_vars = std::accumulate( Spex::result_variables().begin(), Spex::result_variables().end(), std::string(""), getResultVariables( std::set<const LQIO::DOM::ExternalVariable *>() ) );
	    output << qnap2_statement( "print(\"" + result_vars + "\")", "SPEX results" ) << std::endl;
	}

	if ( Spex::input_variables().size() > Spex::array_variables().size() ) {	// Only care about scalars
	    output << "&  -- SPEX scalar variables --" << std::endl;
	    std::for_each( Spex::input_variables().begin(), Spex::input_variables().end(), printSPEXScalars( output ) );	/* Scalars */
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
	    const std::string result_vars = std::accumulate( Spex::result_variables().begin(), Spex::result_variables().end(), std::string(""), getResultVariables( std::set<const LQIO::DOM::ExternalVariable *>() ) );
	    output << qnap2_statement( "print("
				       + std::regex_replace( result_vars, std::regex(","), ",\",\"," )	/* For CSV output */
				       + ")", "SPEX results" ) << std::endl;
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
    QNAP2_Document::getVariables::operator()( const std::string& s1, const Model::Class::pair_t& k ) const
    {
	if ( !Model::Class::has_constant_customers( k ) && _symbol_table.insert(k.second.customers()).second == true ) {
	    std::ostringstream ss;
	    ss << s1;
	    if ( !s1.empty() ) ss << ",";
	    ss << *k.second.customers();		/* Will print out name */
	    return ss.str();
	} else {
	    return s1;
	}
    }

    std::string
    QNAP2_Document::getVariables::operator()( const std::string& s1, const Model::Station::pair_t& m ) const
    {
	const Model::Station::Demand::map_t& demands = m.second.demands();
	return std::accumulate( demands.begin(), demands.end(), s1, getVariables( model(), _symbol_table ) );
    }

    /*
     * Collect all variables.  Only add an itemm to the string if it's
     * a variable (the name), and the variable has not been seen
     * before (insert will return true).
     */

    std::string
    QNAP2_Document::getVariables::operator()( const std::string& s1, const Model::Station::Demand::pair_t& demand ) const
    {
	if ( !Model::Station::Demand::has_constant_service_time( demand ) && _symbol_table.insert(demand.second.service_time()).second == true ) {
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
	case Model::Station::CUSTOMER:
	    _output << qnap2_statement( "init=n_users", "Population by class" ) << std::endl
		    << qnap2_statement( "type=" + __scheduling_str.at(SCHEDULE_CUSTOMER) ) << std::endl
		    << qnap2_statement( "service=exp(think_t)" ) << std::endl;
	    break;
	case Model::Station::DELAY:
	    _output << qnap2_statement( "type=" + __scheduling_str.at(SCHEDULE_DELAY) ) << std::endl;
	    break;
	case Model::Station::LOAD_INDEPENDENT:
	    try {
		_output << qnap2_statement( "sched=" + __scheduling_str.at(station.scheduling()) ) << std::endl;
	    }
	    catch ( const std::out_of_range& ) {
		LQIO::solution_error( LQIO::WRN_SCHEDULING_NOT_SUPPORTED,
				      scheduling_label[static_cast<unsigned int>(station.scheduling())].str,
				      "station",
				      m.first.c_str() );
	    }
	    break;
	case Model::Station::MULTISERVER:
	    _output << qnap2_statement( "type=multiple(" + to_unsigned(station.copies()) + ")" ) << std::endl;
	    break;
	default:
	    abort();
	}
	if ( station.type() != Model::Station::CUSTOMER ) {

	    /* Print out service time variables. */

	    _output << qnap2_statement( "service=exp(" + m.first + "_t)" ) << std::endl;

	    const Model::Station::map_t::const_iterator terminal = std::find_if( stations().begin(), stations().end(), &Model::Station::isCustomer );
	    if ( !multiclass() ) {
		_output << qnap2_statement( "transit=" + terminal->first + ",1" ) << std::endl;
	    } else {
		_output << qnap2_statement( "transit(all class)=" + terminal->first + ",1" ) << std::endl;
	    }
	} else {
	    if ( !multiclass() ) {
		_output << qnap2_statement("transit=" + std::accumulate( stations().begin(), stations().end(), std::string(""), printTransit(classes().begin()->first) ), "visits to servers" ) << std::endl;
	    } else {
		for ( Model::Class::map_t::const_iterator k = classes().begin(); k != classes().end(); ++k ) {
		    _output << qnap2_statement("transit(" + k->first + ")=" + std::accumulate( stations().begin(), stations().end(), std::string(""), printTransit(k->first) ), "visits to servers" ) << std::endl;
		}
	    }
	}
    }

    std::string
    QNAP2_Document::printTransit::operator()( const std::string& str, const Model::Station::pair_t& m ) const
    {
	std::ostringstream out;
	const Model::Station& station = m.second;

	out << str;
	if ( station.type() != Model::Station::CUSTOMER && station.hasClass(_name) ) {
	    if ( !str.empty() ) out << ",";
	    out << m.first << "," << *station.demandAt(_name).visits();
	}
	return out.str();
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
	
	const Model::Station::Demand::map_t visits = std::accumulate( stations().begin(), stations().end(), Model::Station::Demand::map_t(), Model::Station::select( &Model::Station::isServer ) );
	const Model::Station::Demand::map_t service_times = std::accumulate( stations().begin(), stations().end(), Model::Station::Demand::map_t(), Model::Station::select( &Model::Station::isCustomer ) );
	Model::Station::Demand::map_t demands = std::accumulate( service_times.begin(), service_times.end(), Model::Station::Demand::map_t(), Model::sum_visits(visits) );

	/*
	 * QNAP Does everything by "transit", so the service time at
	 * the terminal station has to be adjusted by the number of
	 * visits to all other stations.  Let QNAP do it as it's
	 * easier to see the values from the origial file. Force
	 * floating point math.
	 */
	
	for ( Model::Class::map_t::const_iterator k = classes().begin(); k != classes().end(); ++k ) {
	    std::ostringstream think_time;
	    std::ostringstream customers;
	    std::string comment;
	    think_time << *demands.at(k->first).service_time();
	    customers  << *k->second.customers();
	    if ( !k->second.customers()->wasSet() ) {
		comment = "SPEX variable " + k->second.customers()->getName();
	    }
	    std::string think_visits = std::accumulate( stations().begin(), stations().end(), std::string(""), fold_visits( k->first ) );
	    if ( multiclass() ) {
		output << qnap2_statement( k->first + ".name:=\"" + k->first + "\"", "Class (client) name" ) << std::endl;
		output << qnap2_statement( k->first + ".think_t:=(" + think_time.str() + ")/(" + think_visits + ")", "Slice time at client" ) << std::endl;
		output << qnap2_statement( k->first + ".n_users:=" + customers.str(), comment  ) << std::endl;
	    } else {
		output << qnap2_statement( "think_t:=(" + think_time.str() + ")/(" + think_visits + ")", "Slice time at client" ) << std::endl;
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
	for ( Model::Class::map_t::const_iterator k = classes().begin(); k != classes().end(); ++k ) {
	    if ( station.type() == Model::Station::CUSTOMER ) continue;
	    const LQIO::DOM::ExternalVariable * service_time = station.hasClass(k->first) ? station.demandAt(k->first).service_time() : nullptr;
	    std::ostringstream time;
	    std::string comment;
	    if ( LQIO::DOM::ExternalVariable::isDefault(service_time,0.) ) {
		time << "0.000001";		// Qnap doesn't like zero for service time.
		comment = "QNAP does not like zero (0)";
	    } else {
		time << *service_time;		// Might have to strip off $
		if ( !service_time->wasSet() ) {
		    comment = "SPEX variable " + service_time->getName();
		}
	    }
	    if ( multiclass() ) {
		_output << qnap2_statement( k->first + "." + m.first + "_t" + ":=" + time.str(), comment ) << std::endl;
	    } else {
		_output << qnap2_statement(  m.first + "_t" + ":=" + time.str(), comment ) << std::endl;
	    }
	}
    }


    /*
     * Print out all deferred assignment variables (they go inside all loop bodies that
     * arise for array and completion assignements
     */

    void
    QNAP2_Document::printSPEXScalars::operator()( const std::pair<std::string, const LQX::SyntaxTreeNode *>& expr ) const
    {
	if ( !expr.second ) return;		/* Comprehension or array.  I could check array_variables. */
	std::ostringstream ss;
	ss << expr.first << ":=";
	expr.second->print(ss,0);
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
     * Need to search Spex::observations() for var.first (the name).
     * Use the key to find right function.  Next, we use the name of
     * the object to find out if it's a class or a station.  If it's
     * a class, then query "terminal", otherwise query the station.
     */

    void
    QNAP2_Document::getObservations::operator()( const Spex::var_name_and_expr& var ) const
    {
	const Spex::obs_var_tab_t observations = Spex::observations();

	/* try to find the observation, then see if we can handle it here */
	for ( Spex::obs_var_tab_t::const_iterator obs = observations.begin(); obs != observations.end(); ++obs ) {
	    if ( obs->second.getVariableName() != var.first ) continue;	/* !Found the var. */

	    std::pair<std::string,std::string> expression;
	    const std::map<int,f>::const_iterator key = __key_map.find( obs->second.getKey() );
	    if ( key != __key_map.end() ) {
		/* Map up to entity */
		expression = (this->*(key->second))( get_entity_name( obs->second.getKey(), obs->first ) );
	    } else {
		expression.first = "\"N/A\"";
//		throw std::domain_error( "Observation not handled" );
	    }
	    _output << qnap2_statement( var.first + ":=" + expression.first, expression.second ) << std::endl;
	    break;
	}
    }

    /* mservice, mbusypct, mcustnb, vcustnb, mresponse, mthruput, custnb */

    std::pair<std::string,std::string>
    QNAP2_Document::getObservations::get_throughput( const std::string& name ) const
    {
	std::string result;
	std::string comment;
	if ( classes().find( name ) != classes().end() ) {
	    /* Class is a reference task, and name exists even if we have only one class */
	    std::string think_visits = std::accumulate( stations().begin(), stations().end(), std::string(""), fold_visits( name ) );
	    result = "mthruput(terminal";
	    if ( multiclass() ) {
		result += "," + name;
	    }
	    result += ")/(" + think_visits + ")";
	    comment = "Convert to LQN throughput";
	} else if ( stations().find( name ) != stations().end() ) {
	    result = "mthruput(" + name + ")";
	} else {
	    result = "\"N/A\"";
	}
	return std::pair<std::string,std::string>(result,comment);
    }

    std::pair<std::string,std::string>
    QNAP2_Document::getObservations::get_utilization( const std::string& name ) const
    {
	std::string result;
	std::string comment;
	Model::Model::Station::map_t::const_iterator m = stations().find( name );
	if ( m != stations().end() ) {
	    const BCMP::Model::Station& station = m->second;
	    result = "mbusypct(" + name + ")";
	    if ( !LQIO::DOM::Common_IO::is_default_value(station.copies(), 1.0) ) {
		result += "*" + to_unsigned(station.copies());
		comment = "Convert to LQN utilization";
	    }
	} else if ( classes().find( name ) != classes().end() ) {
	    /* must be the terminal? */
	    result = "mbusypct(terminal";
	    if ( multiclass() ) result += "," + name;
	    result += ")";
	}
	return std::pair<std::string,std::string>(result,comment);
    }

    /*
     * KEY_SERVICE is mservice + sum_of mrespone
     */

    std::pair<std::string,std::string>
    QNAP2_Document::getObservations::get_service_time( const std::string& name ) const
    {
	std::string result;
	std::string comment;
	if ( classes().find( name ) != classes().end() ) {
	    result = "mservice(terminal";
	    if ( multiclass() ) result += "," + name;
	    result += ")*(";
	    result = std::accumulate( stations().begin(), stations().end(), result, fold_visits( name ) );
	    result += ")";
	    result = std::accumulate( stations().begin(), stations().end(), result, fold_mresponse( name, classes() ) );
	    comment = "Convert to LQN service time";
	} else if ( stations().find( name ) != stations().end() ) {
	    result = "mservice(" + name + ")";
	    comment = "Station service time only.";
	}
	return std::pair<std::string,std::string>(result,comment);
    }

    /*
     * KEY_WAITING is mresponse-mservice.
     */

    std::pair<std::string,std::string>
    QNAP2_Document::getObservations::get_waiting_time( const std::string& name  ) const
    {
	std::string result;
	std::string comment;
	if ( stations().find( name ) != stations().end() ) {
	    result = "mresponse(" + name + ")-mservice(" + name + ")";	/* No classes */
	    comment = "Convert to LQN queueing time";
	}
	return std::pair<std::string,std::string>(result,comment);
    }

    const std::string&
    QNAP2_Document::getObservations::get_entity_name( int key, const LQIO::DOM::DocumentObject * object )
    {
	const LQIO::DOM::Task * task = nullptr;
	if ( dynamic_cast<const LQIO::DOM::Phase *>(object) ) {
	    task = dynamic_cast<const LQIO::DOM::Phase *>(object)->getSourceEntry()->getTask();
	} else if ( dynamic_cast<const LQIO::DOM::Entry *>(object) ) {
	    task = dynamic_cast<const LQIO::DOM::Entry *>(object)->getTask();
	} else {
	    return object->getName();
	}
	/* handle %pu, %pw, %u, %w */
	if ( key == KEY_PROCESSOR_UTILIZATION || key == KEY_PROCESSOR_WAITING ) {
	    return task->getProcessor()->getName();
	} else {
	    return task->getName();
	}
    }

    void
    QNAP2_Document::end_for::operator()( const std::string& var ) const
    {
	std::string comment = "for " + var;
	std::replace( comment.begin(), comment.end(), '$', '_'); 	// Make variables acceptable for QNAP2.
	_output << qnap2_statement( "end", comment ) << std::endl;
    }

    std::string
    QNAP2_Document::fold_visits::operator()( const std::string& s1, const Model::Station::pair_t& m2 ) const
    {
	const Model::Station& station = m2.second;
	if ( !station.hasClass(_name) || station.type() == Model::Station::CUSTOMER ) return s1;	/* Don't visit self */
	const LQIO::DOM::ExternalVariable * visits = station.demandAt(_name).visits();
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
	if ( !station.hasClass(_name) || station.type() == Model::Station::CUSTOMER ) return s1;	/* Don't visit self */
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
