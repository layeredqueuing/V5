/* -*- c++ -*- */
/************************************************************************/
/* Copyright the Real-Time and Distributed Systems Group,		*/
/* Department of Systems and Computer Engineering,			*/
/* Carleton University, Ottawa, Ontario, Canada. K1S 5B6		*/
/* 									*/
/* 2004.								*/
/************************************************************************/

/*
 * $Id$
 */

#ifndef __XERECES_COMMON_H
#define __XERECES_COMMON_H

#include <xercesc/dom/DOMErrorHandler.hpp>
#include <iostream>
#include <xercesc/util/XMLString.hpp>
#include <xercesc/util/XercesDefs.hpp>
#include <xercesc/dom/DOMError.hpp>
#include <xercesc/sax/ErrorHandler.hpp>
#include <cstdio>
#include <stdexcept>
#include <sys/time.h>
#include "error.h"

XERCES_CPP_NAMESPACE_USE

namespace LQIO {

    /***************************************************
	 DOM Tree Error Reporter
    ***************************************************/

    class DOMTreeErrorReporter : public ErrorHandler
    {
    public:
	DOMTreeErrorReporter() : fSawErrors(false) {}

	~DOMTreeErrorReporter() {}

	// -----------------------------------------------------------------------
	//  Implementation of the error handler interface
	// -----------------------------------------------------------------------
	void warning(const SAXParseException& toCatch);
	void error(const SAXParseException& toCatch);
	void fatalError(const SAXParseException& toCatch);
	void resetErrors();

	//  Getter method
	bool getSawErrors() const;

	// -----------------------------------------------------------------------
	//  fSawErrors
	//      This is set if we get any errors, and is queryable via a getter
	//      method. Its used by the main code to suppress output if there are
	//      errors.
	// -----------------------------------------------------------------------
	bool    fSawErrors;

    private:
	void errprintf( const severity_t, const SAXParseException& toCatch );
    };

    inline bool DOMTreeErrorReporter::getSawErrors() const
    {
	return fSawErrors;
    }

    class XStr
    {
    public:
	XStr(const char* const toTranscode)
	    {
		fUnicodeForm = XMLString::transcode(toTranscode);
	    }

	~XStr()
	    {
		XMLString::release(&fUnicodeForm);
	    }

	const XMLCh* unicodeForm() const
	    {
		return fUnicodeForm;
	    }

    private:
	XMLCh *fUnicodeForm;
    };

    class StrX
    {
    public :
	StrX(const XMLCh* const toTranscode) : fLocalForm(XMLString::transcode(toTranscode)) {}
	~StrX() { XMLString::release(&fLocalForm); }

	const bool operator!() const;
	const double asDouble() const throw( std::invalid_argument );
	const double optDouble() const throw( std::invalid_argument );
	const long asLong() const throw( std::invalid_argument );
	const long optLong() const throw( std::invalid_argument );
	const bool asBool() const throw( std::invalid_argument );
	const bool optBool() const;
	const clock_t optTime() const;
	const char * asCStr() const throw( std::invalid_argument );
	const char * optCStr() const;
	const char* localForm() const { return fLocalForm; }

    private :
//	StrX( const StrX& );
	StrX& operator=(const StrX& );

	char * fLocalForm;
    };

#define X(str) XStr(str).unicodeForm()
#define Xt(str) XMLString::transcode(str)
#define Xr(str) XMLString::release(str)
#define DB(str) if (xmlInputDebug == true){cout << str;}

    /* Since all the iostream stuff is in a namespace, you have to
       declare the functions you need to use into the global namespace. */

    using std::cout;
    using std::cerr;
    using std::flush;
    using std::endl;

    inline std::ostream& operator<<(std::ostream& target, const StrX& toDump)
    {
	target << toDump.localForm();
	return target;
    }

    extern DOMDocument *inputFileDOM;

    extern bool xmlInputDebug;
    extern int getEnumerationFromElement( const XMLCh * attrubute, const char **, int default_value ) ;
};
	
#endif /* __XERECES_COMMON_H */
