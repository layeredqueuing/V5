/* -*- C++ -*-
 *  $Id: xml_input.h 15868 2022-09-20 08:57:33Z greg $
 *
 *  Created by Martin Mroz on 24/02/09.
 */

#ifndef __LQIO__XML_INPUT__
#define __LQIO__XML_INPUT__

#include <string>
#if HAVE_EXPAT_H
#include <expat.h>
#endif

namespace XML {
    void invalid_argument( const std::string& attr, const std::string& arg );
    double get_double( const char *, const char * );
    long get_long( const char *, const char * );

    class element_error : public std::runtime_error
    {
    public:
	explicit element_error( const std::string& s ) : runtime_error( s ) {}
    };


    class missing_attribute : public std::runtime_error
    {
    public:
	explicit missing_attribute( const std::string& s ) : runtime_error( s ) {}
    };


    class unexpected_attribute : public std::runtime_error
    {
    public:
	explicit unexpected_attribute( const std::string& s ) : runtime_error( s ) {}
    };


#if HAVE_EXPAT_H
    void throw_element_error( const std::string&, const XML_Char ** attributes );
    const XML_Char * getStringAttribute( const XML_Char ** attributes, const XML_Char * Xcomment, const XML_Char * default_value=nullptr );
    const bool getBoolAttribute( const XML_Char ** attributes, const XML_Char * Xprint_int, const bool default_value=false );
    const double getDoubleAttribute( const XML_Char ** attributes, const XML_Char * Xconv_val, const double default_value=-1.0 );
    const double getTimeAttribute( const XML_Char ** attributes, const XML_Char * Xprint_int );
    const long getLongAttribute( const XML_Char ** attributes, const XML_Char * Xprint_int, const long default_value=-1 );
#endif
}
#endif
