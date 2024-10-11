/*  -*- c++ -*-
 * $Id: filename.cpp 17358 2024-10-11 11:15:09Z greg $
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
#include <cstdio>
#include <cstring>
#include <iostream>
#include <sstream>
#include <sys/stat.h>
#include <sys/types.h>
#include "filename.h"
#include "glblerr.h"

namespace LQIO {

    Filename::Filename( const std::filesystem::path& base, const std::string& extension, const std::filesystem::path& directory, const std::string& suffix )
    {
	generate( directory, base, suffix, extension );
    }


    Filename::Filename( const Filename& filename )
    {
	_path = filename._path;
    }


    const std::filesystem::path&
    Filename::generate( const std::filesystem::path& directory, const std::filesystem::path& path, const std::string& extension, const std::string& suffix )
    {
	/* prepend directory */
	if ( !directory.empty() ) {
	    _path = directory / path;
	} else {
	    _path = path;
	}

	if ( !suffix.empty() ) {
	    _path.replace_extension();
	    _path += suffix;	/* Usually the iteration count */
	}

	_path.replace_extension( extension );	/* Put in the new extension */

	return _path;
    }



    Filename&
    Filename::operator=( const Filename& filename )
    {
	if ( this != &filename ) {
	    _path = filename._path;
	}
	return *this;
    }


    Filename&
    Filename::operator=( const std::string& path )
    {
	_path = path;
	return *this;
    }


    Filename&
    Filename::operator=( const std::filesystem::path& path )
    {
	_path = path;
	return *this;
    }


    Filename&
    Filename::operator<<( const std::string& s )
    {
	_path += s;
	return *this;
    }


    Filename&
    Filename::operator<<( const unsigned n )
    {
	std::stringstream ss;
	ss << std::setfill( '0' ) << std::setw(3) << n;
	_path += ss.str();
	return *this;
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
	    throw std::invalid_argument( "Cannot stat: " + str() );
	} else {
	    return src.st_mtime - dst.st_mtime;
	}
    }


    /*
     * Create a backup file if necessary
     */

    void
    Filename::backup( const std::filesystem::path& filename )
    {
	if ( std::filesystem::is_regular_file( filename ) ) {
	    std::filesystem::path backup = filename;
	    backup += "~";
	    std::filesystem::rename( filename, backup );
	}
    }

    /*
     * Create a directory (if needed)
     */

    std::filesystem::path
    Filename::createDirectory( const std::filesystem::path& path, bool lqx_output ) 
    {
	if ( std::filesystem::is_directory( path ) ) {
	    return path;
	}

	std::filesystem::path directory_name;
	if ( lqx_output ) {
	    /* We need to create a directory to store output. */
	    directory_name = LQIO::Filename( path, "d" )();		/* Get the base file name */

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
