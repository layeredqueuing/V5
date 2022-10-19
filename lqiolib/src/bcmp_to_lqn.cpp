/* -*- c++ -*-
 * $Id: bcmp_to_lqn.cpp 15956 2022-10-07 13:54:20Z greg $
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
#include <sstream>
#include <algorithm>
#include <lqx/SyntaxTree.h>
#include "dom_document.h"
#include "dom_processor.h"
#include "dom_task.h"
#include "dom_entry.h"
#include "dom_call.h"
#include "bcmp_to_lqn.h"
#include "srvn_spex.h"

using namespace LQIO;

bool
DOM::BCMP_to_LQN::convert()
{
    _lqn.addPragma( LQIO::DOM::Pragma::_bcmp_, LQIO::DOM::Pragma::_true_ );
    _lqn.addPragma( LQIO::DOM::Pragma::_prune_, LQIO::DOM::Pragma::_true_ );
    _lqn.addPragma( LQIO::DOM::Pragma::_mva_, LQIO::DOM::Pragma::_exact_ );
    _lqn.setExtraComment( "*** Manually change task-processors representing processors to processors. **" );
    try {
	std::for_each( chains().begin(), chains().end(), createLQNTaskProcessor( *this ) );
	std::for_each( stations().begin(), stations().end(), createLQNTaskProcessor( *this ) );
	std::for_each( chains().begin(), chains().end(), connectClassToStation( *this ) );
    }
    catch ( const std::runtime_error& ) {
	return false;
    }
    return true;
}


/*
 * Create LQN Reference tasks from chains.
 */

void
DOM::BCMP_to_LQN::createLQNTaskProcessor::operator()( const BCMP::Model::Chain::pair_t& k ) 
{
    /* Create the processor */
    std::string name = k.first;
    std::replace( name.begin(), name.end(), ' ', '_' );
    DOM::Processor * processor = new DOM::Processor( &lqn(), name, SCHEDULE_DELAY );
    lqn().addProcessorEntity(processor);

    /* Create an entry */
    std::vector<DOM::Entry *> entries;
    DOM::Entry * entry = new DOM::Entry( &lqn(), name );
    LQIO::DOM::Document::db_check_set_entry(entry, LQIO::DOM::Entry::Type::STANDARD);
    lqn().addEntry( entry );
    entries.push_back( entry );
    std::pair<entry_type::iterator,bool> item = client_entries().emplace( k.first, entry );
    assert( item.second );

    /* Add service time */
    const BCMP::Model::Station::map_t::const_iterator terminal = std::find_if( stations().begin(), stations().end(), &BCMP::Model::Station::isCustomer );
    if ( terminal == stations().end() ) throw std::runtime_error( "No Customer." );
    const BCMP::Model::Station::Class& clasx = terminal->second.classAt(k.first);
//    phase->setServiceTimeValue(k.second.think_time());
    DOM::Phase* phase = entry->getPhase(1);
    phase->setName(k.first);
    phase->setServiceTime(getExternalVariable(clasx.service_time()));

    DOM::Task * task = new DOM::Task( &lqn(), name, SCHEDULE_CUSTOMER, entries, processor );
    task->setCopies(getExternalVariable(k.second.customers()));
    processor->addTask(task);
    lqn().addTaskEntity(task);
}

/*
 * Create LQN servers tasks from stations.  Recall: The processor is the server in the
 * BCMP model, so that task in the LQN model is only a placeholder.
 */

void
DOM::BCMP_to_LQN::createLQNTaskProcessor::operator()( const BCMP::Model::Station::pair_t& m ) 
{
    if ( m.second.reference() ) return;
    
    /* Always a PS type processor */
    std::string name = m.first;
    std::replace( name.begin(), name.end(), ' ', '_' );
    DOM::Processor * processor = new DOM::Processor( &lqn(), name, SCHEDULE_PS );
    lqn().addProcessorEntity(processor);
    processor->setCopies(getExternalVariable(m.second.copies()));

    /* Create the entries */
    std::vector<DOM::Entry *> entries;
    unsigned int e = 1;			/* Used to create name_<n> where n is the class. */
    for ( BCMP::Model::Chain::map_t::const_iterator k = chains().begin(); k != chains().end(); ++k, ++e ) {
	if ( !m.second.hasClass( k->first ) ) continue;
	std::ostringstream entry_name;
	entry_name << name << "_" << e;	/* Station name, index for class */
	DOM::Entry * entry = new DOM::Entry( &lqn(), entry_name.str() );
	LQIO::DOM::Document::db_check_set_entry(entry, LQIO::DOM::Entry::Type::STANDARD);
	lqn().addEntry( entry );
	server_entries().emplace( m.first + k->first, entry );

	/* Add service time */
	DOM::Phase* phase = entry->getPhase(1);
	phase->setName( entry_name.str() );
	const BCMP::Model::Station::Class& clasx = m.second.classAt( k->first );
	phase->setServiceTime(getExternalVariable(clasx.service_time()));
	entries.push_back( entry );
    }

    /* Create the task */
    scheduling_type scheduling = m.second.type() == BCMP::Model::Station::Type::DELAY ? SCHEDULE_DELAY : SCHEDULE_FIFO;
    DOM::Task * task = new DOM::Task( &lqn(), name, scheduling, entries, processor );
    task->setCopies(getExternalVariable(m.second.copies()));
    processor->addTask(task);
    lqn().addTaskEntity(task);
}


/*
 * All chains and stations are represented by tasks with one entry
 * per class.  Create a class for any non-zero visit.
 */

void
DOM::BCMP_to_LQN::connectClassToStation::operator()( const BCMP::Model::Chain::pair_t& k )
{
    for ( BCMP::Model::Station::map_t::const_iterator m = stations().begin(); m != stations().end(); ++m ) {
	std::string key = m->first + k.first;
	entry_type::iterator server_iter = server_entries().find(key);
	if ( server_iter == server_entries().end() ) continue;	/* No call from class k to station m */

	/* Create a call from k(k) to m(k). */
	DOM::Entry * client_entry = client_entries().find(k.first)->second;
	DOM::Entry * server_entry = server_iter->second;

	assert(client_entry->getMaximumPhase() == 1);
	LQIO::DOM::Phase * client_phase = client_entry->getPhase(1);
	LQIO::DOM::Call* call = new LQIO::DOM::Call(&lqn(), LQIO::DOM::Call::Type::RENDEZVOUS, client_phase, server_entry );
	const BCMP::Model::Station::Class& clasx = m->second.classAt(k.first);
	call->setCallMean(getExternalVariable(clasx.visits()));
	client_phase->addCall(call);
    }
}


LQIO::DOM::ExternalVariable *
DOM::BCMP_to_LQN::getExternalVariable( const LQX::SyntaxTreeNode * expression ) 
{
    LQIO::DOM::ExternalVariable * variable = nullptr;
    if ( dynamic_cast<const LQX::ConstantValueExpression *>(expression) ) {

	/* It's a constant. */

	LQX::SymbolAutoRef value = const_cast<LQX::SyntaxTreeNode *>(expression)->invoke(nullptr);
	assert( value->getType() == LQX::Symbol::SYM_DOUBLE );
//	    std::cerr << "*** Value is " << value->getDoubleValue() << " ***" << std::endl;
	variable = new LQIO::DOM::ConstantExternalVariable( value->getDoubleValue() );

    } else if ( dynamic_cast<const LQX::VariableExpression *>( expression ) ) {

	/* It's a variable */
	    
	std::ostringstream name;
	expression->print( name );
//	    std::cerr << "*** Value is " << name.str() << " ***" << std::endl;
	variable = LQIO::DOM::__document->db_build_parameter_variable( name.str().c_str(), nullptr );

    } else {

	/* It's an expression.  create a temporary variable, then assign the expression  */
    }

    return variable;
}
