/* -*- c++ -*-
 * $Id: expat_document.cpp 13764 2020-08-17 19:50:05Z greg $
 *
 * Read in XML input files.
 *
 * Copyright the Real-Time and Distributed Systems Group,
 * Department of Systems and Computer Engineering,
 * Carleton University, Ottawa, Ontario, Canada. K1S 5B6
 *
 * ------------------------------------------------------------------------
 * December 2020.
 * ------------------------------------------------------------------------
 */

#if HAVE_CONFIG_H
#include <config.h>
#endif
#include <iomanip>
#include "xml_output.h"

namespace XML {
    static int current_indent = 0;

    int set_indent( int indent )
    {
	int old_indent = current_indent;
	current_indent = indent;
	return old_indent;
    }


    static std::ostream& doIndent( std::ostream& output, int indent )
    {
	if ( indent < 0 ) {
	    if ( current_indent + indent < 0 ) {
		current_indent = 0;
	    } else {
		current_indent += indent;
	    }
	}
	if ( current_indent != 0 ) {
	    output << std::setw( current_indent * 3 ) << " ";
	}
	if ( indent > 0 ) {
	    current_indent += indent;
	}
	return output;
    }

    static std::ostream& doTempIndent( std::ostream& output, int indent )
    {
	output << std::setw( (current_indent + indent) * 3 ) << " ";
	return output;
    }

    static std::ostream& printStartElement( std::ostream& output, const std::string& element, bool complex_element )
    {
	output << indent( complex_element ? 1 : 0  ) << "<" << element;
	return output;
    }

    static std::ostream& printEndElement( std::ostream& output, const std::string& element, bool complex_element )
    {
	if ( complex_element ) {
	    output << indent( -1 ) << "</" << element << ">";
	} else {
	    output << "/>";
	}
	return output;
    }

    static std::ostream& printInlineElement( std::ostream& output, const std::string& e, const std::string& a, const std::string& v, double d )
    {
	output << indent( 0 ) << "<" << e << attribute( a, v )  << ">" << d << "</" << e << ">";
	return output;
    }
    
    static std::ostream& printAttribute( std::ostream& output, const std::string& a, const std::string& v )
    {
	output << " " << a << "=\"" << v << "\"";
	return output;
    }
    
    static std::ostream& printAttribute( std::ostream& output, const std::string&  a, double v )
    {
	output << " " << a << "=\"" << v << "\"";
	return output;
    }
    
    static std::ostream& printAttribute( std::ostream& output, const std::string&  a, unsigned int v )
    {
	output << " " << a << "=\"" << v << "\"";
	return output;
    }

    IntegerManip indent( int i ) { return IntegerManip( &doIndent, i ); }
    IntegerManip temp_indent( int i ) { return IntegerManip( &doTempIndent, i ); }
    BooleanManip start_element( const std::string& e, bool b ) { return BooleanManip( &printStartElement, e, b ); }
    BooleanManip end_element( const std::string& e, bool b ) { return BooleanManip( &printEndElement, e, b ); }
    BooleanManip simple_element( const std::string& e ) { return BooleanManip( &printStartElement, e, false ); }
    InlineElementManip inline_element( const std::string& e, const std::string& a, const std::string& v, double d ) { return InlineElementManip( &printInlineElement, e, a, v, d ); }
    StringManip attribute( const std::string& a, const std::string& v ) { return StringManip( &printAttribute, a, v ); }
    DoubleManip attribute( const std::string&a, double v ) { return DoubleManip( &printAttribute, a, v ); }
    UnsignedManip attribute( const std::string&a, unsigned v ) { return UnsignedManip( &printAttribute, a, v ); }
}
