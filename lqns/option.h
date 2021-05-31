/* -*- C++ -*-
 * help.h	-- Greg Franks
 *
 * $Id: option.h 14710 2021-05-27 22:58:01Z greg $
 */

#ifndef _OPTION_H
#define _OPTION_H

#include <config.h>
#include "dim.h"
#include <map>
#include <vector>
#include "help.h"

namespace Options
{
    class Option {
    public:
	typedef void (*optionFunc)( const char * );

	Option() : _func(0), _hasArg(false), _help(0) {}
	Option( optionFunc func, bool hasArg, Help::help_fptr help ) : _func(func), _hasArg(hasArg), _help(help) {}

	Help::help_fptr help() const { return _help; }
	bool hasArg() const { return _hasArg; }
	optionFunc func() const { return _func; }

    private:
	optionFunc _func;
	bool _hasArg;
	Help::help_fptr _help;
    };

    class Debug : public Option 
    {
    private:
	enum { ACTIVITIES=0, CALLS=1, FORKS=2, INTERLOCK=3, JOINS=4, LAYERS=5, VARIANCE=6, QUORUM=7 };
	
    public:
	Debug() : Option() {}
	Debug( optionFunc func, Help::help_fptr help ) :  Option(func, false, help) {}
	
	static void initialize();
	static void exec( const int ix, const char * );
	static const char ** __options;
	static const std::map<const std::string, const Debug> __table;

//	static bool activities() { return _bits[ACTIVITIES]; }
//	static bool calls() { return _bits[CALLS]; }
	static bool forks() { return _bits[FORKS]; }
	static bool interlock() { return _bits[INTERLOCK]; }
//	static bool joins() { return _bits[JOINS]; }
	static bool layers() { return _bits[LAYERS]; };
	static bool variance() { return _bits[VARIANCE]; }
#if HAVE_LIBGSL
	static bool quorum() { return _bits[QUORUM]; };
#endif

    private:
	static void all( const char * ); 
	static void all2( const char * );
//	static void activities( const char * ) { _bits[ACTIVITIES] = true; }
//	static void calls( const char * ) { _bits[CALLS] = true; }
	static void forks( const char * ) { _bits[FORKS] = true; }
	static void interlock( const char * ) { _bits[INTERLOCK] = true; }
//	static void joins( const char * ) { _bits[JOINS] = true; }
	static void layers( const char * ) { _bits[LAYERS] = true; }
	static void mva( const char * );
	static void variance( const char * ) { _bits[VARIANCE] = true; }
	static void overtaking( const char * );
#if HAVE_LIBGSL
	static void quorum( const char * ) { _bits[QUORUM] = true; }
#endif
	static void xml( const char * );
	static void lqx( const char * );

    private:
	static std::vector<bool> _bits;
    };

    class Trace : public Option 
    {
    public:
	Trace() : Option() {}
	Trace( optionFunc func, bool hasArg, Help::help_fptr help ) : Option( func, hasArg, help ) {}
	
	static void initialize();
	static void exec( const int ix, const char * );
	static const char ** __options;
	static std::map<const char *, Trace, lt_str> __table;

    private:
	static void activities( const char * ); 
	static void convergence( const char * );
	static void delta_wait( const char * );
	static void forks( const char * );
	static void idle_time( const char * );
	static void interlock( const char * );
	static void intermediate( const char * );
	static void joins( const char * );
	static void mva( const char * );
	static void overtaking( const char * );
	static void quorum( const char * );
	static void replication( const char * );
	static void throughput( const char * );
	static void variance( const char * );
	static void virtual_entry( const char * );
	static void wait( const char * );
    };


    class Special : public Option 
    {
    public:
	Special() : Option() {}
	Special( optionFunc func, bool hasArg, Help::help_fptr help ) : Option( func, hasArg, help ) {}
	
	static void initialize();
	static void exec( const int ix, const char * );
	static const char ** __options;
	static std::map<const char *, Special, lt_str> __table;

    private:
	static void iteration_limit( const char * ); 
	static void print_interval( const char * );
	static void overtaking( const char * );
	static void convergence_value( const char * );
	static void single_step( const char * );
	static void underrelaxation( const char * );
	static void generate_queueing_model( const char * );
	static void mol_ms_underrelaxation( const char * );
	static void make_man( const char * );
	static void make_tex( const char * );
	static void min_steps( const char * );
	static void ignore_overhanging_threads( const char * );
	static void full_reinitialize( const char * );
    };
}
#endif
