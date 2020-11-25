/* -*- c++ -*-
double  * $HeadURL: http://rads-svn.sce.carleton.ca:8080/svn/lqn/trunk-V5/lqns/generate.h $
 *
 * Generate MVA program for given layer.
 *
 * Copyright the Real-Time and Distributed Systems Group,
 * Department of Systems and Computer Engineering,
 * Carleton University, Ottawa, Ontario, Canada. K1S 5B6
 *
 * November, 1994
 *
 * $Id: generate.h 14140 2020-11-25 20:24:15Z greg $
 *
 * ------------------------------------------------------------------------
 */

#include "dim.h"
#include <string>

class Entity;
class Task;
class MVASubmodel;

/* -------------------------------------------------------------------- */
/* Funky Formatting functions for inline with <<.			*/
/* -------------------------------------------------------------------- */

class Generate {

    class ArgsManip {
    public:
	ArgsManip( std::ostream& (*f)(std::ostream&, const unsigned, const unsigned, const unsigned ), const unsigned e, const unsigned k, const unsigned p ) :
	    _f(f), _e(e), _k(k), _p(p) {}

    private:
	std::ostream& (*_f)( std::ostream&, const unsigned, const unsigned, const unsigned );
	const unsigned _e;
	const unsigned _k;
	const unsigned _p;

	friend std::ostream& operator<<(std::ostream & os, const ArgsManip& m ) { return m._f(os,m._e,m._k,m._p); }
    };

    class StnManip {
    public:
	StnManip( std::ostream& (*f)(std::ostream&, const Entity& ), const Entity& s ) : _f(f), _stn(s) {}
    private:
	std::ostream& (*_f)( std::ostream&, const Entity& );
	const Entity& _stn;

	friend std::ostream& operator<<(std::ostream & os, const StnManip& m ) { return m._f(os,m._stn); }
    };
    
public:
    static void print( const MVASubmodel& );
    static void makefile( const unsigned );

private:
    Generate( const MVASubmodel& );
    std::ostream& print( std::ostream& ) const;
    std::ostream& printClientStation( std::ostream& output, const Task& aClient ) const;
    std::ostream& printServerStation( std::ostream& output, const Entity& aServer ) const;
    std::ostream& printInterlock( std::ostream& output, const Entity& aServer ) const;

    static ArgsManip station_args( const unsigned e, const unsigned k, const unsigned p ) { return ArgsManip( &print_station_args, e, k, p ); }
    static ArgsManip overtaking_args( const unsigned e, const unsigned k, const unsigned p ) { return ArgsManip( &print_overtaking_args, e, k, p ); }
    static StnManip station_name( const Entity& entity ) { return StnManip( &print_station_name, entity ); }

    static std::ostream& print_station_name( std::ostream& output, const Entity& anEntity );
    static std::ostream& print_station_args( std::ostream& output, const unsigned e, const unsigned k, const unsigned p );
    static std::ostream& print_overtaking_args( std::ostream& output, const unsigned e, const unsigned k, const unsigned p );

public:
    static std::string file_name;

private:
    const MVASubmodel& _submodel;
    const unsigned K;			/* Number of chains */
};
