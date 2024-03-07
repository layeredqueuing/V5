/*  -*- c++ -*-
 * $Id: filename.cpp 17081 2024-03-01 22:09:31Z greg $
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
#include <filesystem>
#include <iostream>
#include <cstdio>
#include <cstring>
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

    bool
    Filename::isRegularFile( const std::string& fileName )
    {
	return std::filesystem::is_regular_file( fileName );
    }


    /*
     * Return non-zero if fileName is a directory, -1 if the file cannot
     * be accessed using stat(1), and zero otherwise.
     */

    bool
    Filename::isDirectory( const std::string& fileName )
    {
	return std::filesystem::is_directory( fileName );
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
	if ( isRegularFile( filename ) ) {
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
	    try {
		std::filesystem::create_directory( directory_name );
	    }
	    catch( const std::filesystem::filesystem_error& e ) {
		runtime_error( LQIO::ERR_CANT_OPEN_DIRECTORY, directory_name.c_str(), e.what() );
	    }
	}
	return directory_name;
    }

    
}
