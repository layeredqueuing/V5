/* -*- c++ -*-
 * $Id: xml_input.cpp 16436 2023-02-17 02:04:49Z greg $
 *
 * Read in XML input files.
 *
 * Copyright the Real-Time and Distributed Systems Group,
 * Department of Systems and Computer Engineering,
 * Carleton University, Ottawa, Ontario, Canada. K1S 5B6
 *
 * ------------------------------------------------------------------------
 * December 2003.
 * May 2010.
 * ------------------------------------------------------------------------
 */

#if HAVE_CONFIG_H
#include <config.h>
#endif
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <limits.h>
#if HAVE_STRINGS_H
#include <strings.h>
#endif
#include "common_io.h"
#include "xml_input.h"
#include "dom_document.h"
#include "dom_extvar.h"

namespace XML
{
    void invalid_argument( const std::string& attr, const std::string& arg )
    {
	throw std::invalid_argument( attr + "=" + arg );
    }

    double get_double( const char * attr, const char * val )
    {
	char * end_ptr = nullptr;
	const double value = strtod( val, &end_ptr );
	if ( (!std::isnan(value) && value < 0.) || ( end_ptr && *end_ptr != '\0' ) ) {
	    invalid_argument( attr, val );
	}
	return value;
    }

    long get_long( const char * attr, const char * val )
    {
	char * end_ptr = nullptr;
	const long value = strtol( val, &end_ptr, 10 );
	if ( errno == ERANGE || value < 0 || ( end_ptr && *end_ptr != '\0' ) ) {
	    invalid_argument( attr, val );
	}
	return value;
    }

#if HAVE_EXPAT_H
    void throw_element_error( const std::string& name, const XML_Char ** attributes )
    {
	std::string s = name;
	if ( *attributes != nullptr ) {
	    s += " " + std::string( *attributes );
	    if ( *(attributes+1) != nullptr ) {
		s += "=\"" + std::string(*(attributes+1)) + "\"";
	    }
	}
	throw element_error( s );
    }
    
    const XML_Char *
    getStringAttribute(const XML_Char ** attributes, const XML_Char * attribute, const XML_Char * default_value )
    {
	for ( ; *attributes; attributes += 2 ) {
	    if ( strcasecmp( *attributes, attribute ) == 0 && *(attributes+1) != nullptr ) {
		return *(attributes+1);
	    }
	}
	if ( default_value ) {
	    return default_value;
	} else {
	    throw missing_attribute( attribute );
	}
    }

    const double
    getDoubleAttribute(const XML_Char ** attributes, const XML_Char * attribute, const double default_value )
    {
	for ( ; *attributes; attributes += 2 ) {
	    if ( strcasecmp( *attributes, attribute ) == 0 && *(attributes+1) != nullptr ) {
		return get_double( *attributes, *(attributes+1) );
	    }
	}
	if ( default_value >= 0.0 ) {
	    return default_value;
	} else {
	    throw missing_attribute( attribute );
	}
    }

    const long
    getLongAttribute(const XML_Char ** attributes, const XML_Char * attribute, const long default_value )
    {
	for ( ; *attributes; attributes += 2 ) {
	    if ( strcasecmp( *attributes, attribute ) == 0 ) {
		return get_long( *attributes, *(attributes+1) );
	    }
	}
	if ( default_value >= 0 ) {
	    return default_value;
	} else {
	    throw missing_attribute( attribute );
	}
    }

    const bool
    getBoolAttribute( const XML_Char ** attributes, const XML_Char * attribute, const bool default_value )
    {
	for ( ; *attributes; attributes += 2 ) {
	    if ( strcasecmp( *attributes, attribute ) == 0 && *(attributes+1) != nullptr ) {
		return strcasecmp( *(attributes+1), "yes" ) == 0 || strcasecmp( *(attributes+1), "true" ) == 0 || strcmp( *(attributes+1), "1" ) == 0;
	    }
	}
	return default_value;
    }

    const double
    getTimeAttribute( const XML_Char ** attributes, const XML_Char * attribute )
    {
	for ( ; *attributes; attributes += 2 ) {
	    if ( strcasecmp( *attributes, attribute ) == 0 && *(attributes+1) != nullptr ) {
		unsigned long hrs   = 0;
		unsigned long mins  = 0;
		double secs         = 0;

		sscanf( *(attributes+1), "%ld:%ld:%lf", &hrs, &mins, &secs );
		return hrs * 3600 + mins * 60 + secs;
	    }
	}
	return 0.;
    }
#endif
}
