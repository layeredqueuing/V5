/* -*- C++ -*-
 *  $Id: expat_document.h 13717 2020-08-03 00:04:28Z greg $
 *
 *  Created by Martin Mroz on 24/02/09.
 */

#ifndef __LQIO__XML_INPUT__
#define __LQIO__XML_INPUT__

#include <expat.h>
#include "input.h"

namespace XML {
    void invalid_argument( const std::string& attr, const std::string& arg );
    double get_double( const char *, const char * );

    const XML_Char * getStringAttribute( const XML_Char ** attributes, const XML_Char * Xcomment, const XML_Char * default_value=nullptr );
    const bool getBoolAttribute( const XML_Char ** attributes, const XML_Char * Xprint_int, const bool default_value=false );
    const double getDoubleAttribute( const XML_Char ** attributes, const XML_Char * Xconv_val, const double default_value=-1.0 );
    const double getTimeAttribute( const XML_Char ** attributes, const XML_Char * Xprint_int );
    const long getLongAttribute( const XML_Char ** attributes, const XML_Char * Xprint_int, const long default_value=-1 );
}
#endif
