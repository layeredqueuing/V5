/*  -*- c++ -*-
 * $Id: filename.cpp 16117 2022-11-17 17:58:24Z greg $
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

#include <config.h>
#include <iostream>
#include <cstdio>
#include <cstring>
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <sys/stat.h>
#include <sys/types.h>
#include "filename.h"
#include "glblerr.h"

namespace LQIO {

    Filename::Filename( const std::string& base, const std::string& extension, const std::string& directory, const std::string& suffix )
    {
	generate( base, extension, directory, suffix );
    }


    Filename::Filename( const Filename& aFilename )
    {
	_filename = aFilename._filename;
    }


    const std::string&
    Filename::generate( const std::string& base, const std::string& extension, const std::string& directory, const std::string& suffix )
    {
	/* prepend directory */

	const size_t dir = base.rfind( '/' );		/* Locate directory	*/

	/*
	 * special cases:
	 *   No extension:         eg 'foo'     --> foo.out.
	 *   'dot' file:           eg '.foo'    --> .foo.out
	 *  // input ending in .out: eg 'foo.out' --> foo.out.out
	 */
		
	size_t dot = base.rfind( '.' );
	if ( (dir != std::string::npos && dot != std::string::npos && dot <= dir + 1) || dot == 0 ) {
	    dot = std::string::npos;		/* Dot is in a directory name - ignore, or base is a 'dot' file */
	} 
//	if ( directory.size() == 0 && dot != std::string::npos && base.substr( dot + 1 ) == extension ) {
//	    dot = std::string::npos;		/* Extension of base is the same as extension */
//	}

	/* Stick in base file name (and directory part if directory.size() == 0 ) */

	if ( dir == std::string::npos || directory.size() == 0  ) {
	    _filename = base.substr( 0, dot );
	} else {
	    _filename = base.substr( dir + 1, dot - (dir + 1) );
	}

	/* Prepend directory */

	if ( directory.size() > 0 ) {
	    _filename.insert( 0, "/" );
	    _filename.insert( 0, directory );
	} 

	_filename += suffix;	/* Usually the iteration count */

	if ( extension.size() ) {
	    _filename += '.';
	    _filename += extension;
	}

	return _filename;
    }



    Filename&
    Filename::operator=( const Filename& aFilename )
    {
	if ( this != &aFilename ) {
	    _filename = aFilename._filename;
	}
	return *this;
    }


    Filename&
    Filename::operator=( const std::string& aFilename )
    {
	_filename = aFilename;
	return *this;
    }


    /* 
     * Return the filename.
     */

    const std::string& 
    Filename::operator()() const
    {
	return _filename;
    }


    Filename&
    Filename::operator<<( const char * aStr )
    {
	_filename += aStr;
	return *this;
    }


    Filename&
    Filename::operator<<( const unsigned n )
    {
	const size_t size = 8;
	char buf[size];
	snprintf( buf, size, "%03d", n );

	_filename += buf;
	return *this;
    }


    /*
     * Return non-zero if fileName is a regular file, -1 if the file cannot
     * be accessed using stat(1), and zero if the file is a directory or
     * other wierd item, such as /dev/null.
     */

    int
    Filename::isRegularFile( const std::string& fileName )
    {
	struct stat statbuf;

	if ( stat( fileName.c_str(), &statbuf ) != 0 ) {
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
    Filename::isDirectory( const std::string& fileName )
    {
	struct stat statbuf;

	if ( stat( fileName.c_str(), &statbuf ) != 0 ) {
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
    Filename::mtimeCmp( const std::string& filename )
    {
	struct stat dst;
	struct stat src;

	if ( stat( filename.c_str(), &dst ) < 0 ) {
	    throw std::invalid_argument( "Cannot stat: " + filename );
	} else if ( stat( (*this)().c_str(), &src ) < 0 ) {
	    throw std::invalid_argument( "Cannot stat: " + (*this)() );
	} else {
	    return src.st_mtime - dst.st_mtime;
	}
    }


    unsigned 
    Filename::rfind( const std::string& s ) const
    {
	return _filename.rfind( s );
    }

    unsigned 
    Filename::find( const std::string& s ) const
    {
	return _filename.find( s );
    }

    Filename& 
    Filename::insert( unsigned pos, const char * s )
    {
	_filename.insert( pos, s );
	return *this;
    }

    /*
     * Create a backup file if necessary
     */

    void
    Filename::backup( const std::string& filename )
    {
	if ( isRegularFile( filename ) > 0 ) {
	    std::string backup = filename;
	    backup += "~";
	    rename( filename.c_str(), backup.c_str() );
	}
    }

    /*
     * Create a directory (if needed)
     */

    std::string
    Filename::createDirectory( const std::string& file_name, bool lqx_output ) 
    {
	std::string directory_name;
	if ( isDirectory( file_name ) > 0 ) {
	    directory_name = file_name;
	} else if ( lqx_output ) {
	    /* We need to create a directory to store output. */
	    directory_name = LQIO::Filename( file_name, "d" )();		/* Get the base file name */
	}

	if ( !directory_name.empty() ) {
	    int rc = access( directory_name.c_str(), R_OK|W_OK|X_OK );
	    if ( rc < 0 ) {
		if ( errno == ENOENT ) {
#if defined(__WINNT__)
		    rc = mkdir( directory_name.c_str() );
#else
		    rc = mkdir( directory_name.c_str(), S_IRUSR|S_IWUSR|S_IXUSR|S_IRGRP|S_IWGRP|S_IXGRP|S_IWOTH|S_IROTH|S_IXOTH );
#endif
		}
		if ( rc < 0 ) {
		    runtime_error( LQIO::ERR_CANT_OPEN_DIRECTORY, directory_name.c_str(), strerror( errno ) );
		}
	    }
	}
	return directory_name;
    }

    
}
