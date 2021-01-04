/* -*- C++ -*-
 * help.h	-- Greg Franks
 *
 * $Id: option.h 14319 2021-01-02 04:11:00Z greg $
 */

#ifndef _OPTION_H
#define _OPTION_H

#include <config.h>
#include "dim.h"
#include <map>
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
	optionFunc func() { return _func; }

    private:
	optionFunc _func;
	bool _hasArg;
	Help::help_fptr _help;
    };

    class Debug : public Option 
    {
    public:
	Debug() : Option() {}
	Debug( optionFunc func, Help::help_fptr help ) :  Option(func, false, help) {}
	
	static void initialize();
	static void exec( const int ix, const char * );
	static const char ** __options;
	static std::map<const char *, Debug, lt_str> __table;

//	static bool activities() { return _activities; }
//	static bool calls() { return _calls; }
	static bool forks() { return _forks; }
	static bool interlock() { return _interlock; }
//	static bool joins() { return _joins; }
	static bool layers() { return _layers; };
	static bool variance() { return _variance; }
#if HAVE_LIBGSL
	static bool quorum() { return _quorum; };
#endif

    private:
	static void all( const char * ); 
	static void all2( const char * );
//	static void activities( const char * ) { _activities = true; }
//	static void calls( const char * ) { _calls = true; }
	static void forks( const char * ) { _forks = true; }
	static void interlock( const char * ) { _interlock = true; }
//	static void joins( const char * ) { _joins = true; }
	static void layers( const char * ) { _layers = true; }
	static void mva( const char * );
	static void variance( const char * ) { _variance = true; }
	static void overtaking( const char * );
#if HAVE_LIBGSL
	static void quorum( const char * ) { _quorum = true; }
#endif
	static void xml( const char * );
	static void lqx( const char * );

    private:
//	static bool _activities;
//	static bool _calls;
	static bool _forks;
	static bool _interlock;
//	static bool _joins;
	static bool _layers;			/* -d: Print out debug info.		*/
	static bool _variance;
#if HAVE_LIBGSL
	static bool _quorum;			/* print out local etc.			*/
#endif
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
