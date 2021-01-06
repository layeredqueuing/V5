/* -*- c++ -*-
 * $Id: expat_document.cpp 13764 2020-08-17 19:50:05Z greg $
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
#include "dom_document.h"
#include "dom_processor.h"
#include "dom_task.h"
#include "dom_entry.h"
#include "dom_call.h"
#include "bcmp_to_lqn.h"

using namespace LQIO;

bool
DOM::BCMP_to_LQN::convert()
{
    _lqn.addPragma( LQIO::DOM::Pragma::_bcmp_, LQIO::DOM::Pragma::_true_ );
    _lqn.addPragma( LQIO::DOM::Pragma::_prune_, LQIO::DOM::Pragma::_true_ );
    _lqn.setDocumentComment( "*** Manually change task-processors representing processors to processors. **" );
    try {
	std::for_each( classes().begin(), classes().end(), createLQNTaskProcessor( *this) );
	std::for_each( stations().begin(), stations().end(), createLQNTaskProcessor( *this ) );
	std::for_each( classes().begin(), classes().end(), connectClassToStation( *this ) );
    }
    catch ( const std::runtime_error& ) {
	return false;
    }
    return true;
}


/*
 * Create LQN Reference tasks from classes.
 */

void
DOM::BCMP_to_LQN::createLQNTaskProcessor::operator()( const BCMP::Model::Class::pair_t& k ) 
{
    /* Create the processor */
    DOM::Processor * processor = new DOM::Processor( &lqn(), k.first, SCHEDULE_DELAY );
    lqn().addProcessorEntity(processor);

    /* Create an entry */
    DOM::Entry * entry = new DOM::Entry( &lqn(), k.first );
    std::vector<DOM::Entry *> entries;
    entries.push_back( entry );
    LQIO::DOM::Document::db_check_set_entry(entry, entry->getName(), LQIO::DOM::Entry::ENTRY_STANDARD);
    client_entries().insert( entry_item( k.first, entry ) );

    /* Add service time */
    const BCMP::Model::Station::map_t::const_iterator terminal = std::find_if( stations().begin(), stations().end(), &BCMP::Model::Station::isCustomer );
    if ( terminal == stations().end() ) throw std::runtime_error( "No Customer." );
    const BCMP::Model::Station::Demand& demand = terminal->second.demandAt(k.first);
//    phase->setServiceTimeValue(k.second.think_time());
    DOM::Phase* phase = entry->getPhase(1);
    phase->setName(k.first);
    phase->setServiceTime(const_cast<DOM::ExternalVariable *>(demand.service_time()));

    DOM::Task * task = new DOM::Task( &lqn(), k.first, SCHEDULE_CUSTOMER, entries, processor );
    task->setCopies(const_cast<DOM::ExternalVariable *>(k.second.customers()));
    processor->addTask(task);
    lqn().addTaskEntity(task);
}

/*
 * Create LQN servers tasks from stations.
 */

void
DOM::BCMP_to_LQN::createLQNTaskProcessor::operator()( const BCMP::Model::Station::pair_t& m ) 
{
    if ( m.second.type() == BCMP::Model::Station::CUSTOMER ) return;
    
    /* Always a PS type processor */
    DOM::Processor * processor = new DOM::Processor( &lqn(), m.first, SCHEDULE_PS );
    lqn().addProcessorEntity(processor);

    /* Create the entries */
    std::vector<DOM::Entry *> entries;
    unsigned int e = 1;			/* Used to create name_<n> where n is the class. */
    for ( BCMP::Model::Class::map_t::const_iterator k = classes().begin(); k != classes().end(); ++k, ++e ) {
	if ( !m.second.hasClass( k->first ) ) continue;
	std::ostringstream name;
	name << m.first << "_" << e;	/* Station name, index for class */
	DOM::Entry * entry = new DOM::Entry( &lqn(), name.str() );
	LQIO::DOM::Document::db_check_set_entry(entry, entry->getName(), LQIO::DOM::Entry::ENTRY_STANDARD);
	server_entries().insert( entry_item(m.first + k->first, entry) );

	/* Add service time */
	DOM::Phase* phase = entry->getPhase(1);
	phase->setName(name.str());
	const BCMP::Model::Station::Demand& demand = m.second.demandAt(k->first);
	phase->setServiceTime(const_cast<DOM::ExternalVariable *>((demand.service_time())));
	entries.push_back(entry);
    }

    /* Create the task */
    scheduling_type scheduling = m.second.type() == BCMP::Model::Station::DELAY ? SCHEDULE_DELAY : SCHEDULE_FIFO;
    DOM::Task * task = new DOM::Task( &lqn(), m.first, scheduling, entries, processor );
    task->setCopies(const_cast<DOM::ExternalVariable *>(m.second.copies()));
    processor->addTask(task);
    lqn().addTaskEntity(task);
}


/*
 * All classes and stations are represented by tasks with one entry
 * per class.  Create a class for any non-zero visit.
 */

void
DOM::BCMP_to_LQN::connectClassToStation::operator()( const BCMP::Model::Class::pair_t& k )
{
    for ( BCMP::Model::Station::map_t::const_iterator m = stations().begin(); m != stations().end(); ++m ) {
	std::string key = m->first + k.first;
	entry_type::iterator server_iter = server_entries().find(key);
	if ( server_iter == server_entries().end() ) continue;	/* No call from class k to station m */

	/* Create a call from k(k) to m(k). */
	DOM::Entry * client_entry = client_entries().find(k.first)->second;
	DOM::Entry * server_entry = server_iter->second;

	LQIO::DOM::Phase * client_phase = client_entry->getPhase(1);
	LQIO::DOM::Call* call = new LQIO::DOM::Call(&lqn(), LQIO::DOM::Call::RENDEZVOUS, client_phase, server_entry );
	const BCMP::Model::Station::Demand& demand = m->second.demandAt(k.first);
	call->setCallMean( const_cast<DOM::ExternalVariable *>(demand.visits()) );
	client_phase->addCall(call);
    }
}
