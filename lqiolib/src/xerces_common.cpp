/*  -*- c++ -*-
 * $Id: xerces_common.cpp 11963 2014-04-10 14:36:42Z greg $
 *
 * Copyright the Real-Time and Distributed Systems Group,
 * Department of Systems and Computer Engineering,
 * Carleton University, Ottawa, Ontario, Canada. K1S 5B6
 *
 * 2004
 */

#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdexcept>

#include <xercesc/util/PlatformUtils.hpp>

#include <xercesc/dom/DOM.hpp>
#include <xercesc/dom/DOMImplementation.hpp>
#include <xercesc/dom/DOMImplementationLS.hpp>

#include <xercesc/framework/MemBufFormatTarget.hpp>
#include <xercesc/parsers/XercesDOMParser.hpp>
#include <xercesc/util/XMLUni.hpp>
#include <xercesc/util/IOException.hpp>
#include <xercesc/sax/SAXParseException.hpp>

#include <string>
#include <cstdlib>
#include <cstdarg>
#include <cmath>
#include <fstream>
#include <map>
#include <vector>
#include <ctype.h>
#include <errno.h>
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#include "error.h"
#include "glblerr.h"
#include "xerces_common.h"

namespace LQIO {

    /* Reference for DOMDocument so that xmlresults.cc can see the DOM */
    DOMDocument *inputFileDOM = NULL;



    /* Prototypes for internal use */



    /**********************************************
    Implementation of the DOM Tree Error Reporter
    *********************************************/

    void DOMTreeErrorReporter::warning(const SAXParseException& toCatch )
    {
	errprintf( WARNING_ONLY, toCatch );
    }

    void DOMTreeErrorReporter::error(const SAXParseException& toCatch)
    {
	fSawErrors = true;
	errprintf( RUNTIME_ERROR, toCatch );
    }

    void DOMTreeErrorReporter::fatalError(const SAXParseException& toCatch)
    {
	fSawErrors = true;
	errprintf( RUNTIME_ERROR, toCatch );
    }

    void DOMTreeErrorReporter::resetErrors()
    {
	fSawErrors = false;
    }

    void DOMTreeErrorReporter::errprintf( const severity_t severity, const SAXParseException& toCatch )
    {
	StrX message(toCatch.getMessage());
	if ( strncmp( message.asCStr(), "An exception occurred", 21 ) == 0 ) {
	    throw std::runtime_error( message.asCStr() );
	}
	StrX file_name(toCatch.getSystemId());
	verrprintf( stderr, severity, file_name.asCStr(), toCatch.getLineNumber(), toCatch.getColumnNumber(),
		    message.asCStr(), 0 );
    }


    bool isDOMpresent(void)
    {
	return inputFileDOM != NULL;
    }


/* Function that starts off the whole reading in of the XML file */

    bool serializeDOM(std::ostream& output, bool)
    {
	XMLCh tempStr[100];
	XMLString::transcode("LS", tempStr, 99);
	DOMImplementationLS *impl = dynamic_cast<DOMImplementationLS *>(DOMImplementationRegistry::getDOMImplementation(tempStr));
	DOMLSSerializer   *theSerializer = impl->createLSSerializer();
	DOMLSOutput       *theOutputDesc = impl->createLSOutput();

	bool rc = false;

	try
	{
	    MemBufFormatTarget *myFormTarget = new MemBufFormatTarget();

	    /* do the serialization through DOMLSSerializer::write(); */

	    theOutputDesc->setByteStream(myFormTarget);
	    theSerializer->write(inputFileDOM, theOutputDesc);

	    const XMLByte * data = myFormTarget->getRawBuffer();
	    XMLSize_t len = myFormTarget->getLen();

	    output.write( reinterpret_cast<const char *>(data), len );

	    delete myFormTarget;
	    rc = true;
	}
	catch (IOException& e ) {
	    cerr << StrX(e.getMessage());
	    cerr << "  Code is " << e.getCode();

	    if ( errno != 0 ) {
		cerr << ": " << strerror( errno );
	    }
	}
	catch (XMLException& e)
	{
	    cerr << "An error occurred during creation of output transcoder. Msg is:"
		 << endl
		 << StrX(e.getMessage());
	    if ( errno != 0 ) {
		cerr << ": " << strerror( errno );
	    }
	    cerr << endl;
	    rc = false;
	}
	delete theSerializer;
	return rc;
    }

    const double
    StrX::asDouble() const throw( std::invalid_argument )
    {
	if ( !(*this) ) {
	    throw std::invalid_argument( "" );
	}
	return optDouble();
    }

    const double
    StrX::optDouble() const throw( std::invalid_argument )
    {
	if ( !(*this) ) return 0.0;
	char * endPtr;
	double value = strtod( fLocalForm, &endPtr );
	if ( value < 0. ) {
	    throw std::invalid_argument( fLocalForm );
	} else if ( endPtr && *endPtr != '\0' ) {
	    throw std::invalid_argument( fLocalForm );
	}
	return value;
    }


    const long
    StrX::asLong() const throw( std::invalid_argument )
    {
	if ( !(*this) ) {
	    throw std::invalid_argument( "" );
	}
	return optLong();
    }

    const long
    StrX::optLong() const throw( std::invalid_argument )
    {
	if ( !(*this) ) return 0;
	char * endPtr;
	long value = strtol( fLocalForm, &endPtr, 10 );
	if ( value < 0. ) {
	    throw std::invalid_argument( fLocalForm );
	} else if ( endPtr && *endPtr != '\0' ) {
	    throw std::invalid_argument( fLocalForm );
	} 
	return value;
    }

    const bool
    StrX::asBool() const throw( std::invalid_argument )
    {
	if ( !(*this) ) {
	    throw std::invalid_argument( "" );
	}
	return optBool();
    }

    const bool
    StrX::optBool() const
    {
	if ( !(*this) ) return false;
	return strcasecmp( fLocalForm, "true" ) == 0
	    || strcasecmp( fLocalForm, "yes" ) == 0 
	    || strcmp( fLocalForm, "1" ) == 0;
    }

    const clock_t
    StrX::optTime() const
    {
	if ( !(*this) ) return 0.0;

	unsigned long hrs   = 0.0;
	unsigned long mins  = 0.0;
	unsigned long secs  = 0.0;
    
	sscanf( fLocalForm, "%ld:%ld:%ld", &hrs, &mins, &secs );

#if defined(HAVE_SYS_TIME_H)
#if defined(CLK_TCK)
	return (hrs * 3600 + mins * 60 + secs) * CLK_TCK;
#else
	return (hrs * 3600 + mins * 60 + secs) * sysconf(_SC_CLK_TCK);
#endif
#else
	return hrs * 3600 + mins * 60 + secs;
#endif
    }

    const bool
    StrX::operator!() const
    {
	return !fLocalForm || *fLocalForm == '\0';
    }
	
    const char * 
    StrX::asCStr() const throw( std::invalid_argument )
    {
	if ( !fLocalForm ) {
	    throw std::invalid_argument( "" );
	}
	return fLocalForm;
    }

    const char * 
    StrX::optCStr() const
    {
	if ( !fLocalForm ) {
	    throw "missing attribute";
	} else if ( *fLocalForm == 0 ) {
	    return 0;
	}
	return fLocalForm;
    }

    int
    getEnumerationFromElement( const XMLCh * attribute_value, const char ** enum_strings, int default_value ) 
    {
	for ( int i = 0; enum_strings[i]; ++i ) {
	    if ( XMLString::compareIString( X(enum_strings[i]), attribute_value ) == 0 ) return i; 
	}
	return default_value;
    }

    unsigned numberOfElements(const char *elementName)
    {
	return(inputFileDOM->getElementsByTagName(X(elementName))->getLength());
    }

    int doesDOMcontainResults(void)
    {
	if (isDOMpresent())
	{
	    return ((numberOfElements("result-processor") > 0) || (numberOfElements("result-task") > 0) ||
		    (numberOfElements("result-entry") > 0) || (numberOfElements("result-activity") > 0) ||
		    (numberOfElements("result-join-delay") > 0) || (numberOfElements("result-forwarding") > 0) ||
		    (numberOfElements("result-activity-distribution") > 0) || (numberOfElements("result-call") > 0));
	}
	return(0);
    }

    void
    missing_attribute( const char * attr, const DOMNode * node )
    {
	input_error2( ERR_MISSING_ATTRIBUTE, StrX( node->getNodeName() ).asCStr(), attr );
    }

};
