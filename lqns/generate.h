/* -*- c++ -*-
 * Generate MVA program for given layer.
 *
 * Copyright the Real-Time and Distributed Systems Group,
 * Department of Systems and Computer Engineering,
 * Carleton University, Ottawa, Ontario, Canada. K1S 5B6
 *
 * November, 1994
 *
 * $Id: generate.h 15659 2022-06-09 01:05:42Z greg $
 *
 * ------------------------------------------------------------------------
 */

class Entity;
class Task;
class MVASubmodel;

#include <map>
#include <lqio/dom_phase.h>
#include "pragma.h"

/* -------------------------------------------------------------------- */
/* Funky Formatting functions for inline with <<.			*/
/* -------------------------------------------------------------------- */

class Generate {

    enum class Multiserver { DEFAULT, CONWAY, REISER, REISER_PS, ROLIA, ROLIA_PS, ZHOU };

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

    static bool find_libmva( std::string& pathname );
    
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
    static std::string __directory_name;
    
private:
    const MVASubmodel& _submodel;
    const unsigned K;			/* Number of chains */
    static const std::vector<std::string> __includes;
    static const std::vector<struct option> __longopts;
    static const std::map<const char, const std::string> __help;
    static const std::map<const int,const std::string> __argument_type;
    static const std::map<const Pragma::MVA,const std::string> __solvers;
    static const std::map<const Pragma::Multiserver,const Generate::Multiserver> __multiservers;
    static const std::map<const Pragma::Multiserver,const std::pair<const std::string&,const std::string&> > __stations;
};
