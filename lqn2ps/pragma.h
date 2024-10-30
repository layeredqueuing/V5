/* -*- c++ -*-
 * $HeadURL: http://rads-svn.sce.carleton.ca:8080/svn/lqn/trunk/lqn2ps/pragma.h $
 *
 * Pragma processing.
 *
 * Copyright the Real-Time and Distributed Systems Group,
 * Department of Systems and Computer Engineering,
 * Carleton University, Ottawa, Ontario, Canada. K1S 5B6
 *
 * November, 1994
 *
 * $Id: pragma.h 16335 2023-01-16 19:53:44Z greg $
 *
 * ------------------------------------------------------------------------
 */

#if	!defined(PRAGMA_H)
#define	PRAGMA_H

#include <string>
#include <map>
#include <lqio/dom_pragma.h>
#include <lqio/input.h>
#include "lqn2ps.h"

/* -------------------------------------------------------------------- */

class Pragma {

public:
    typedef void (Pragma::*fptr)(const std::string&);

    enum class BCMP { STANDARD, EXTENDED, LQN };
    enum class ForceInfinite { NONE, FIXED_RATE, MULTISERVERS, ALL };
    
private:
    Pragma();
    virtual ~Pragma() 
	{
	    __cache = nullptr;
	}

    static void set_pragma( const std::pair<std::string,std::string>& p );
    
public:
    static bool forceInfinite( ForceInfinite );
    static Layering layering();
    static bool defaultProcessorScheduling() { assert( __cache != nullptr ); return __cache->_default_processor_scheduling; }
    static scheduling_type processorScheduling() { assert( __cache != nullptr ); return __cache->_processor_scheduling; }
    static bool prune();
    static LQIO::error_severity severityLevel();
    static bool spexHeader();
    static bool defaultTaskScheduling() { assert( __cache != nullptr ); return __cache->_default_task_scheduling; }
    static scheduling_type taskScheduling() { assert( __cache != nullptr ); return __cache->_task_scheduling; }

private:
    void setBCMP(const std::string&);
    void setForceInfinite(const std::string&);
    void setLayering(const std::string&);
    void setProcessorScheduling(const std::string&);
    void setPrune(const std::string&);
    void setSeverityLevel(const std::string&);
    void setSpexHeader(const std::string&);
    void setTaskScheduling(const std::string&);
    
public:
    static void set( const std::map<std::string,std::string>& );
    static std::ostream& usage( std::ostream&  );
    static const std::map<const std::string,const Pragma::fptr>& getPragmas() { return __set_pragma; }
    
private:
    ForceInfinite _force_infinite;
    scheduling_type _processor_scheduling;
    scheduling_type _task_scheduling;
    /* bonus */
    bool _default_processor_scheduling;
    bool _default_task_scheduling;
    
    static Pragma * __cache;
    static std::map<const std::string,const Pragma::fptr> __set_pragma;
};
#endif
