/* -*- c++ -*-
 * $Id: expat_document.cpp 11510 2013-09-03 20:41:37Z greg $
 *
 * Read in XML input files.
 *
 * Copyright the Real-Time and Distributed Systems Group,
 * Department of Systems and Computer Engineering,
 * Carleton University, Ottawa, Ontario, Canada. K1S 5B6
 *
 * ------------------------------------------------------------------------
 * September 2013
 * ------------------------------------------------------------------------
 */

#if HAVE_CONFIG_H
#include <config.h>
#endif
#include <cstring>
#include "common_io.h"
#include "dom_extvar.h"

namespace LQIO {
    namespace DOM {

	bool
	Common_IO::Compare::operator()( const char * s1, const char * s2 ) const 
	{
	    return strcasecmp( s1, s2 ) < 0;
	}

	std::map<const char *, scheduling_type,Common_IO::Compare> Common_IO::scheduling_table;

	Common_IO::Common_IO() 
	    : _conf_95( ConfidenceIntervals( LQIO::ConfidenceIntervals::CONF_95 ) ),
	      _conf_99( ConfidenceIntervals( LQIO::ConfidenceIntervals::CONF_99 ) )
	{
	}

	void
	Common_IO::init_tables() 
	{
            if ( scheduling_table.size() != 0 ) return;		/* Done already */

	    scheduling_table[schedulingTypeXMLString[SCHEDULE_CUSTOMER]] =    SCHEDULE_CUSTOMER;
	    scheduling_table[schedulingTypeXMLString[SCHEDULE_DELAY]] =	      SCHEDULE_DELAY;  
	    scheduling_table[schedulingTypeXMLString[SCHEDULE_FIFO]] = 	      SCHEDULE_FIFO;   
	    scheduling_table[schedulingTypeXMLString[SCHEDULE_HOL]] =  	      SCHEDULE_HOL;    
	    scheduling_table[schedulingTypeXMLString[SCHEDULE_PPR]] =  	      SCHEDULE_PPR;    
	    scheduling_table[schedulingTypeXMLString[SCHEDULE_RAND]] = 	      SCHEDULE_RAND;   
	    scheduling_table[schedulingTypeXMLString[SCHEDULE_PS]] =   	      SCHEDULE_PS;     
	    scheduling_table[schedulingTypeXMLString[SCHEDULE_PS_HOL]] =      SCHEDULE_PS_HOL; 
	    scheduling_table[schedulingTypeXMLString[SCHEDULE_PS_PPR]] =      SCHEDULE_PS_PPR; 
	    scheduling_table[schedulingTypeXMLString[SCHEDULE_POLL]] = 	      SCHEDULE_POLL;   
	    scheduling_table[schedulingTypeXMLString[SCHEDULE_BURST]] =	      SCHEDULE_BURST;  
	    scheduling_table[schedulingTypeXMLString[SCHEDULE_UNIFORM]] =     SCHEDULE_UNIFORM;
	    scheduling_table[schedulingTypeXMLString[SCHEDULE_SEMAPHORE]] =   SCHEDULE_SEMAPHORE;    	 
	    scheduling_table[schedulingTypeXMLString[SCHEDULE_CFS]] =	      SCHEDULE_CFS;
	    scheduling_table[schedulingTypeXMLString[SCHEDULE_RWLOCK]] =      SCHEDULE_RWLOCK;
	}

	/* static */ void
	Common_IO::invalid_argument( const std::string& attr, const std::string& arg )
	{
	    std::string err = attr;
	    err += "\"=\"";
	    err += arg;
	    err += "\"";
	    throw std::invalid_argument( err.c_str() );
	}


	double
	Common_IO::invert( const double arg ) const
	{
	    return _conf_95.invert( arg );
	}

	/* 
	 * This function is used to ignore default values in the
	 * input.  
	 */
	
	bool
	Common_IO::is_default_value( const LQIO::DOM::ExternalVariable * var, double default_value ) 
	{
	    double value = 0.0;
	    return var == NULL || (var->wasSet() && var->getValue(value) && value == default_value);
	}

	ForPhase::ForPhase()
	    : _maxPhase(DOM::Phase::MAX_PHASE), _type(DOM::Call::NULL_CALL)
	{
	    for ( unsigned p = 0; p < DOM::Phase::MAX_PHASE; ++p ) {
		ia[p] = 0;
	    }
	}

    }
}
