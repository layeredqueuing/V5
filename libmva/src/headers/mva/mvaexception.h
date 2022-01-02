/* -*- c++ -*-
 * Exception handling.
 *
 * Copyright the Real-Time and Distributed Systems Group,
 * Department of Systems and Computer Engineering,
 * Carleton University, Ottawa, Ontario, Canada. K1S 5B6
 *
 * Janunary 2022
 *
 * $Id: mvaexception.h 15322 2022-01-02 15:35:27Z greg $
 *
 * ------------------------------------------------------------------------
 */

#ifndef	LIBMVA_MVAEXCEPTION_H
#define	LIBMVA_MVAEXCEPTION_H

#include <stdexcept>
#include <string>
#include <sstream>

namespace LibMVA {

    class class_error : public std::logic_error 
    {
    public:
	class_error( const std::string& s, const std::string& f, const unsigned l, const std::string& m ) : std::logic_error(construct(s,f,l,m)) {}

    private:
	std::string construct( const std::string& s, const std::string& file, const unsigned line, const std::string& msg )
	    {
		std::ostringstream ss;
		ss << s << ": " << file << " " << line << ": " << msg;
		return ss.str();
	    }
    };


    class subclass_responsibility : public class_error 
    {
    public:
	subclass_responsibility( const std::string& s, const char * f, const unsigned l ) : class_error( s, f, l, "Subclass responsibility." ) {}
    };

    class not_implemented : public class_error 
    {
    public:
	not_implemented( const std::string& s, const char * f, const unsigned l ) : class_error( s, f, l, "Not implemented." ) {}
    };

    class should_not_implement : public class_error 
    {
    public:
	should_not_implement( const std::string& s, const char * f, const unsigned l ) : class_error( s, f, l, "Should not implement." ) {}
    };
}
#endif
