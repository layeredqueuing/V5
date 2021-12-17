/* -*- c++ -*-
 * $Id: commandline.h 15220 2021-12-15 15:18:47Z greg $
 *
 * MVA solvers: Exact, Bard-Schweitzer, Linearizer and Linearizer2.
 * Abstract superclass does no operation by itself.
 *
 * Copyright the Real-Time and Distributed Systems Group,
 * Department of Systems and Computer Engineering,
 * Carleton University, Ottawa, Ontario, Canada. K1S 5B6
 *
 * November, 1994
 * ------------------------------------------------------------------------
 */

#if	!defined(LQIO_COMMANDLINE_H)
#define	LQIO_COMMANDLINE_H

#include <string>
#include <getopt.h>

#if defined(HAVE_CONFIG_H)
#include <config.h>
#endif

#if !HAVE_GETOPT_LONG
struct option;
#endif

namespace LQIO {

    class CommandLine
    {
    public:
    CommandLine()
	: _s() {}

	CommandLine& operator=( const std::string& );
	CommandLine& append( int c, const char * optarg );

	inline const char * c_str() { return _s.c_str(); }

    private:
	std::string _s;
    };

}
#endif
