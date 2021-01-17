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
#include <map>
#include <iomanip>
#include "xml_output.h"
#include "dom_extvar.h"
#include "common_io.h"

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

    static std::ostream& printComment( std::ostream& output, const std::string& s, const std::string& )
    {
	    output << indent(0) << "<!-- ";
	    for ( std::string::const_iterator p = s.begin(); p != s.end() ; ++p ) {	/* Handle comments */
		if ( *p != '-' || *(p+1) != '-' ) {
		    output << *p;						/* Strip '--' strings */
		}
	    }
	    output << " -->" << std::endl;
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

    static std::ostream& printInlineElement( std::ostream& output, const std::string& e, const std::string& a, const std::string& v, const std::string& t )
    {
	output << indent( 0 ) << "<" << e << attribute( a, v )  << ">" << t << "</" << e << ">";
	return output;
    }
    
    static std::ostream& printAttribute( std::ostream& output, const std::string& a, const std::string& value )
    {
	static std::map<const char,const char *> escape_table;

	if ( escape_table.empty() ) {
	    escape_table['&']  = "&amp;";
	    escape_table['\''] = "&apos;";
	    escape_table['>']  = "&gt;";
	    escape_table['<']  = "&lt;";
	    escape_table['"']  = "&qout;";
	}

	output << " " << a << "=\"";
	for ( const char * v = value.c_str(); *v != 0; ++v ) {
	    std::map<const char,const char *>::const_iterator item = escape_table.find(*v);
	    if ( item == escape_table.end() ) {
		output << *v;
	    } else if ( item->first != '&' ) {
		output << item->second;
	    } else {
		unsigned int i;
		bool valid = false;
		for ( i = 1; isalnum( v[i] ); ++i );
		valid = ( v[i] == ';' );
		if ( !valid && v[1] == '#' ) {
		    for ( i = 2; isxdigit( v[i] ); ++i );
		    valid = ( v[i] == ';' );
		}
		if ( valid ) {
		    output << *v;	/* Found valid escape... ignore processing here */
		} else {
		    output << item->second;	/* No match in table so must escape ampresand */
		}
	    }
	}
	output << "\"";
	return output;
    }
    
    static std::ostream& printAttribute( std::ostream& output, const std::string& a, const LQIO::DOM::ExternalVariable& v )
    {
	output << " " << a << "=\"" << v << "\"";
	return output;
    }
    
    static std::ostream& printAttribute( std::ostream& output, const std::string&  a, unsigned int v )
    {
	output << " " << a << "=\"" << v << "\"";
	return output;
    }

    static std::ostream& printAttribute( std::ostream& output, const std::string&  a, double v )
    {
	output << " " << a << "=\"" << v << "\"";
	return output;
    }

    static std::ostream& printAttribute( std::ostream& output, const std::string& a, const char * v )
    {
	return printAttribute( output, a, std::string(v) );
    }

    static std::ostream& printAttribute( std::ostream& output, const std::string&  a, bool v )
    {
	output << " " << a << "=\"" << (v ? "true" : "false") << "\"";
	return output;
    }

    static std::ostream& printTime( std::ostream& output, const std::string& a, const double v )
    {
	output << " " << a << "=\"" << LQIO::DOM::CPUTime::print( v ) <<  "\"";
	return output;
    }

    static std::ostream& printCData( std::ostream& output, const std::string& s, const std::string& )
    {
	output << "><![CDATA[" << s << "]]>";
	return output;
    }
    
    IntegerManip indent( int i ) { return IntegerManip( &doIndent, i ); }
    IntegerManip temp_indent( int i ) { return IntegerManip( &doTempIndent, i ); }
    BooleanManip start_element( const std::string& e, bool b ) { return BooleanManip( &printStartElement, e, b ); }
    BooleanManip end_element( const std::string& e, bool b ) { return BooleanManip( &printEndElement, e, b ); }
    BooleanManip simple_element( const std::string& e ) { return BooleanManip( &printStartElement, e, false ); }
    String2Manip inline_element( const std::string& e, const std::string& a, const std::string& v, const std::string& t ) { return String2Manip( &printInlineElement, e, a, v, t ); }
    StringManip attribute( const std::string& a, const std::string& v ) { return StringManip( &printAttribute, a, v ); }
    CharPtrManip attribute( const std::string& a, const char * v ) { return CharPtrManip( &printAttribute, a, v ); }
    DoubleManip attribute( const std::string& a, double v ) { return DoubleManip( &printAttribute, a, v ); }
    UnsignedManip attribute( const std::string& a, unsigned v ) { return UnsignedManip( &printAttribute, a, v ); }
    BooleanManip attribute( const std::string& a, bool v ) { return BooleanManip( &printAttribute, a, v ); }
    ExternalVariableManip attribute( const std::string& a, const LQIO::DOM::ExternalVariable& v ) { return ExternalVariableManip( &printAttribute, a, v ); }
    DoubleManip time_attribute( const std::string& a, const double v ) { return DoubleManip( &printTime, a, v ); }
    StringManip comment( const std::string& s ) { return StringManip( &printComment, s, std::string("") ); }
    StringManip cdata( const std::string& s ) { return StringManip( &printCData, s, std::string("") ); }
}
