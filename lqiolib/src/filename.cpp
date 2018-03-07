/*  -*- c++ -*-
 * $Id: filename.cpp 13200 2018-03-05 22:48:55Z greg $
 *
 * File name generation.
 *
 * Copyright the Real-Time and Distributed Systems Group,
 * Department of Systems and Computer Engineering,
 * Carleton University, Ottawa, Ontario, Canada. K1S 5B6
 *
 * 2004
 *
 * ------------------------------------------------------------------------
 */

#include "filename.h"
#include <iostream>
#include <cstdio>
#include <cstring>
#if HAVE_MALLOC_H
#include <malloc.h>
#endif
#include <sys/stat.h>

namespace LQIO {

    Filename::Filename( const char * base, const char * extension, const char * directory, const char * suffix )
    {
	generate( base, extension, directory, suffix );
    }


    Filename::Filename( const Filename& aFilename )
    {
	aString = aFilename.aString;
    }


    const char *
    Filename::generate( const char * base, const char * extension, const char * directory, const char * suffix )
    {
	const char * p = strrchr( base, '/' );		/* Locate directory	*/
	const char * q;
	const bool dir_present = directory && *directory;

	if ( dir_present ) {
	    aString = directory;
	    aString +=  '/';
	} else {
	    aString = "";
	}

	if ( !p ) {
	    p = base;
	} else if ( !dir_present ) {
	    p += 1;
	    for ( q = base; q < p; ++q ) {
		aString += *q;
	    }
	}

	q = strrchr( p, '.' );

	/*
	 * Check for special cases:
	 *   No extension:         eg 'foo'     --> foo.out.
	 *   'dot' file:           eg '.foo'    --> .foo.out
	 *   input ending in .out: eg 'foo.out' --> foo.out.out
	 */
		
	if ( !q || p == q ) {
	    q = p + strlen( p );		/* Move to end of string */
	} 

	for ( ; p < q; ++p ) {
	    aString += *p;
	}
		
	if ( suffix && *suffix ) {	/* Usually the iteration count */
	    aString += suffix;
	}

	if ( extension && *extension ) {
	    aString += '.';
	    aString += extension;
	}

	return (*this)();
    }



    Filename&
    Filename::operator=( const Filename& aFilename )
    {
	if ( this != &aFilename ) {
	    aString = aFilename.aString;
	}
	return *this;
    }


    Filename&
    Filename::operator=( const char * aFilename )
    {
	aString = aFilename;
	return *this;
    }


    /* 
     * Return the filename.
     */

    const char * 
    Filename::operator()() const
    {
	return aString.c_str();
    }


    Filename&
    Filename::operator<<( const char * aStr )
    {
	aString += aStr;
	return *this;
    }


    Filename&
    Filename::operator<<( const unsigned n )
    {
	char buf[8];
	sprintf( buf, "%03d", n );

	aString += buf;
	return *this;
    }


    /*
     * Return non-zero if fileName is a regular file, -1 if the file cannot
     * be accessed using stat(1), and zero if the file is a directory or
     * other wierd item, such as /dev/null.
     */

    int
    Filename::isRegularFile( const char * fileName )
    {
	struct stat statbuf;

	if ( stat( fileName, &statbuf ) != 0 ) {
	    return -1;
	} else {
	    return S_ISREG(statbuf.st_mode);
	}
    }


    int
    Filename::isRegularFile( int fileno )
    {
	struct stat statbuf;

	if ( fstat( fileno, &statbuf ) != 0 ) {
	    return -1;
	} else {
	    return S_ISREG(statbuf.st_mode);
	}
    }


    /*
     * Return non-zero if fileName is a directory, -1 if the file cannot
     * be accessed using stat(1), and zero otherwise.
     */

    int
    Filename::isDirectory( const char * fileName )
    {
	struct stat statbuf;

	if ( stat( fileName, &statbuf ) != 0 ) {
	    return -1;
	} else {
	    return S_ISDIR(statbuf.st_mode);
	}
    }


    /*
     * Return non-zero if fileName is a regular file, -1 if the file cannot
     * be accessed using stat(1), and zero if the file is a directory or
     * other wierd item, such as /dev/null.
     */

    int
    Filename::isWriteableFile( int fileno )
    {
	struct stat statbuf;

	if ( fstat( fileno, &statbuf ) != 0 ) {
	    return -1;
	} else {
#if defined(S_ISSOCK)
	    return S_ISREG(statbuf.st_mode) || S_ISFIFO(statbuf.st_mode) || S_ISSOCK(statbuf.st_mode);  
#else
	    return S_ISREG(statbuf.st_mode) || S_ISFIFO(statbuf.st_mode);
#endif
	}
    }



    /*
     * Return 1 if the receiver is "older" than filename, 0 if not, and -1
     * if either file returns error from stat(2).
     */

    int
    Filename::mtimeCmp( const char * filename ) 
    {
	struct stat dst;
	struct stat src;

	if ( stat( filename, &dst ) < 0 ) {
	    std::string err = "Cannot stat: ";
	    err += filename;
	    throw std::invalid_argument( err.c_str() );
	} else if ( stat( (*this)(), &src ) < 0 ) {
	    std::string err = "Cannot stat: ";
	    err += filename;
	    throw std::invalid_argument( err.c_str() );
	} else {
	    return src.st_mtime - dst.st_mtime;
	}
    }


    unsigned 
    Filename::rfind( const string& s ) const
    {
	return aString.rfind( s );
    }

    unsigned 
    Filename::find( const string& s ) const
    {
	return aString.find( s );
    }

    Filename& 
    Filename::insert( unsigned pos, const char * s )
    {
	aString.insert( pos, s );
	return *this;
    }

    /*
     * Create a backup file if necessary
     */

    void
    Filename::backup( const char * filename )
    {
	if ( isRegularFile( filename ) > 0 ) {
	    string backup = filename;
	    backup += "~";
	    rename( filename, backup.c_str() );
	}
    }
};
