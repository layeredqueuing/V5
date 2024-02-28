/* -*- C++ -*-
 * option.h	-- Greg Franks
 *
 * $Id: option.h 17067 2024-02-27 18:45:36Z greg $
 */

#ifndef _OPTION_H
#define _OPTION_H

#include <map>
#include <vector>
#include <lqio/dom_pragma.h>
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
	enum { ACTIVITIES=0, CALLS=1, FORKS=2, INTERLOCK=3, JOINS=4, REPLICATION=5, VARIANCE=6, QUORUM=7 };
//	Debug( const Debug& ) = delete;
	Debug& operator=( const Debug& ) = delete;

    public:
	Debug( optionFunc func, Help::help_fptr help ) : Option(func, false, help) {}
	static void initialize();

	static void exec( const int ix, const std::string& );
	static std::vector<char *> __options;
	static const std::map<const std::string, const Debug> __table;

//	static bool activities() { return __bits[ACTIVITIES]; }
//	static bool calls() { return __bits[CALLS]; }
	static bool forks() { return __bits[FORKS]; }
	static bool interlock() { return __bits[INTERLOCK]; }
//	static bool joins() { return __bits[JOINS]; }
	static bool replication() { return __bits[REPLICATION]; }
	static bool submodels( unsigned long submodel = 0 );
	static bool variance() { return __bits[VARIANCE]; }
#if HAVE_LIBGSL
	static bool quorum() { return __bits[QUORUM]; };
#endif

    private:
//	static void activities( const std::string& ) { __bits[ACTIVITIES] = true; }
//	static void calls( const std::string& ) { __bits[CALLS] = true; }
	static void forks( const std::string& s ) { __bits[FORKS] = LQIO::DOM::Pragma::isTrue( s ); }
	static void interlock( const std::string& s ) { __bits[INTERLOCK] = LQIO::DOM::Pragma::isTrue( s ); }
//	static void joins( const std::string& ) { __bits[JOINS] = LQIO::DOM::Pragma::isTrue( s ); }
	static void mva( const std::string& );
	static void replication( const std::string& s) { __bits[REPLICATION] = LQIO::DOM::Pragma::isTrue( s ); }
    public:
	static void submodels( const std::string& s );
    private:
	static void variance( const std::string& s ) { __bits[VARIANCE] = LQIO::DOM::Pragma::isTrue( s ); }
	static void overtaking( const std::string& );
#if HAVE_LIBGSL
	static void quorum( const std::string& ) { __bits[QUORUM] = LQIO::DOM::Pragma::isTrue( s ); }
#endif
	static void xml( const std::string& );
	static void lqx( const std::string& );

    private:
	static std::vector<bool> __bits;
	static unsigned long __submodels;
    };

    class Trace : public Option 
    {
    public:
	Trace( optionFunc func, bool hasArg, Help::help_fptr help ) : Option( func, hasArg, help ) {}

	static void initialize();
	static void exec( const int ix, const std::string& );
	static std::vector<char *> __options;
	static std::map<const std::string, const Trace> __table;
	static bool delta_wait( unsigned long submodel = 0 );
	static bool mva( unsigned long submodel = 0 );
	static bool verbose() { return __verbose; }

    private:
	static void activities( const std::string& ); 
	static void convergence( const std::string& );
	static void delta_wait( const std::string& );
	static void forks( const std::string& );
	static void idle_time( const std::string& );
	static void interlock( const std::string& );
	static void intermediate( const std::string& );
	static void joins( const std::string& );
    public:
	static void mva( const std::string& );		/* Used by main */
    private:
	static void overtaking( const std::string& );
	static void quorum( const std::string& );
	static void replication( const std::string& );
	static void throughput( const std::string& );
	static void variance( const std::string& );
    public:
	static void verbose( const std::string& );
    private:
	static void virtual_entry( const std::string& );
	static void wait( const std::string& );

    private:
	static unsigned long __delta_wait;
	static unsigned long __mva;			/* Print out MVA solutions.		*/
	static bool __verbose;
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
	static void print_interval( const std::string& );
	static void overtaking( const std::string& );

    private:
	static void single_step( const std::string& );
	static void generate_queueing_model( const std::string& );
	static void generate_jmva_output( const std::string& );
	static void make_man( const std::string& );
	static void make_tex( const std::string& );
	static void min_steps( const std::string& );
#if HAVE_LIBGSL
	static void ignore_overhanging_threads( const std::string& );
#endif
	static void full_reinitialize( const std::string& );
    };
}
#endif
