/* -*- C++ -*-
 * help.h	-- Greg Franks
 *
 * $Id: option.h 15060 2021-10-09 23:28:42Z greg $
 */

#ifndef _OPTION_H
#define _OPTION_H

#include <map>
#include <vector>
#include "help.h"

namespace Options
{
    class Option {
    public:
	typedef void (*optionFunc)( const std::string& );

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
//	Debug( const Debug& ) = delete;
	Debug& operator=( const Debug& ) = delete;

    public:
	Debug( optionFunc func, Help::help_fptr help ) : Option(func, false, help) {}
	static void initialize();

	static void exec( const int ix, const std::string& );
	static std::vector<char *> __options;
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
	static void all( const std::string& ); 
	static void all2( const std::string& );
//	static void activities( const std::string& ) { _bits[ACTIVITIES] = true; }
//	static void calls( const std::string& ) { _bits[CALLS] = true; }
	static void forks( const std::string& ) { _bits[FORKS] = true; }
	static void interlock( const std::string& ) { _bits[INTERLOCK] = true; }
//	static void joins( const std::string& ) { _bits[JOINS] = true; }
	static void layers( const std::string& ) { _bits[LAYERS] = true; }
	static void mva( const std::string& );
	static void variance( const std::string& ) { _bits[VARIANCE] = true; }
	static void overtaking( const std::string& );
#if HAVE_LIBGSL
	static void quorum( const std::string& ) { _bits[QUORUM] = true; }
#endif
	static void xml( const std::string& );
	static void lqx( const std::string& );

    private:
	static std::vector<bool> _bits;
    };

    class Trace : public Option 
    {
    public:
	Trace( optionFunc func, bool hasArg, Help::help_fptr help ) : Option( func, hasArg, help ) {}

	static void initialize();
	static void exec( const int ix, const std::string& );
	static std::vector<char *> __options;
	static std::map<const std::string, const Trace> __table;

    private:
	static void activities( const std::string& ); 
	static void convergence( const std::string& );
	static void delta_wait( const std::string& );
	static void forks( const std::string& );
	static void idle_time( const std::string& );
	static void interlock( const std::string& );
	static void intermediate( const std::string& );
	static void joins( const std::string& );
	static void mva( const std::string& );
	static void overtaking( const std::string& );
	static void quorum( const std::string& );
	static void replication( const std::string& );
	static void throughput( const std::string& );
	static void variance( const std::string& );
	static void virtual_entry( const std::string& );
	static void wait( const std::string& );
    };


    class Special : public Option 
    {
    public:
	Special() : Option() {}
	Special( optionFunc func, bool hasArg, Help::help_fptr help ) : Option( func, hasArg, help ) {}	

	static void initialize();
	static void exec( const int ix, const std::string& );
	static std::vector<char *> __options;
	static std::map<const std::string, const Special> __table;

    public:
	static void iteration_limit( const std::string& ); 
	static void print_interval( const std::string& );
	static void overtaking( const std::string& );
	static void convergence_value( const std::string& );
	static void underrelaxation( const std::string& );

    private:
	static void single_step( const std::string& );
	static void generate_queueing_model( const std::string& );
	static void mol_ms_underrelaxation( const std::string& );
	static void make_man( const std::string& );
	static void make_tex( const std::string& );
	static void min_steps( const std::string& );
	static void ignore_overhanging_threads( const std::string& );
	static void full_reinitialize( const std::string& );
    };
}
#endif
