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
#include "dom_document.h"
#include "dom_processor.h"
#include "dom_task.h"
#include "dom_entry.h"
#include "bcmp_to_lqn.h"

using namespace LQIO;

bool
DOM::BCMP_to_LQN::convert() const
{
	std::for_each( classes().begin(), classes().end(), createLQNTaskProcessor( _lqn ) );
	std::for_each( stations().begin(), stations().end(), createLQNTaskProcessor( _lqn ) );

	// for all classes create tasks/entries/processor.
	// for all stations create tasks/entries/processors.
	// Create calls.
	return false;
}


void
DOM::BCMP_to_LQN::createLQNTaskProcessor::operator()( const BCMP::Model::Class::pair_t& k )
{
    DOM::Processor * processor = new DOM::Processor( &_lqn, k.first, SCHEDULE_DELAY );
    _lqn.addProcessorEntity(processor);
}

void
DOM::BCMP_to_LQN::createLQNTaskProcessor::operator()( const BCMP::Model::Station::pair_t& m )
{
    DOM::Processor * processor = new DOM::Processor( &_lqn, m.first, m.second.scheduling() );
    _lqn.addProcessorEntity(processor);
}
