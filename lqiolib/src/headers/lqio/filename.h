/* -*- c++ -*-
 * $Id: filename.h 13717 2020-08-03 00:04:28Z greg $
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

#if	!defined(SRVNIOLIB_FILENAME_H)
#define	SRVNIOLIB_FILENAME_H

#if defined(HAVE_CONFIG_H)
#include <config.h>
#endif

#include <string>
#include <stdexcept>

/* Autconf botches inline sometimes. */

namespace LQIO {

    class Filename
    {
    public:
	Filename() {};
	Filename( const std::string& base, const std::string& extension = "", const std::string& directory = "", const std::string& suffix = "");
	Filename( const Filename& );
	Filename& operator=( const Filename& );
	Filename& operator=( const std::string& );

	const std::string& operator()() const;
	Filename& operator<<( const char * );
	Filename& operator<<( const unsigned );

	const std::string& generate( const std::string& base, const std::string& extension, const std::string& directory = "", const std::string& suffix = "" );
	Filename& backup() { Filename::backup( (*this)() ); return *this; }

	int mtimeCmp( const std::string& fileName );

	unsigned rfind( const std::string& s ) const;
	unsigned find( const std::string& s ) const;
	Filename& insert( unsigned pos, const char * s );

	static int isRegularFile( const std::string& fileName );
	static int isRegularFile( int fileno );
	static int isDirectory( const std::string& fileName );
	static int isWriteableFile( int fileno );
	static void backup( const std::string& filename );

    private:
	std::string _filename;
    };
}
#endif
