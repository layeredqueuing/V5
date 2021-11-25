/* -*- c++ -*-
 * $HeadURL: http://rads-svn.sce.carleton.ca:8080/svn/lqn/trunk-V5/qnsolver/pragma.h $
 *
 * Pragma processing.
 *
 * Copyright the Real-Time and Distributed Systems Group,
 * Department of Systems and Computer Engineering,
 * Carleton University, Ottawa, Ontario, Canada. K1S 5B6
 *
 * November, 2021
 *
 * $Id: pragma.h 15125 2021-11-25 03:12:30Z greg $
 *
 * ------------------------------------------------------------------------
 */

#if	!defined(PRAGMA_H)
#define	PRAGMA_H

#include <map>
#include <lqio/dom_pragma.h>
#include "model.h"

/* -------------------------------------------------------------------- */

class Pragma {

public:
    typedef void (Pragma::*fptr)(const std::string&);

private:
    Pragma();
    virtual ~Pragma()
	{
	    __cache = nullptr;
	}

public:
    static Model::Multiserver multiserver()
	{
	    assert( __cache != nullptr );
	    return __cache->_multiserver;
	}

    static Model::Solver solver()
	{
	    assert( __cache != nullptr );
	    return __cache->_solver;
	}


private:
    void setMultiserver(const std::string&);
    void setSolver(const std::string&);

public:
    static void set( const std::map<std::string,std::string>& );
    static std::ostream& usage( std::ostream& );
    static const std::map<const std::string,const Pragma::fptr>& getPragmas() { return __set_pragma; }

private:
    Model::Multiserver _multiserver;
    Model::Solver _solver;

    /* --- */

    static Pragma * __cache;
    static const std::map<const std::string,const Pragma::fptr> __set_pragma;
};
#endif
