/*	-*- c++ -*-
 * $Id$
 *
 * Read in XML input files.
 *
 * Copyright the Real-Time and Distributed Systems Group,
 * Department of Systems and Computer Engineering,
 * Carleton University, Ottawa, Ontario, Canada. K1S 5B6
 *
 * December 2003.
 *
 * ------------------------------------------------------------------------
 */

#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdexcept>
#include <string>
#include <cstdlib>
#include <fstream>
#include <cmath>
#if HAVE_SYS_RESOURCE_H
#include <sys/resource.h>
#endif
#if defined(HAVE_UNISTD_H)
#include <unistd.h>
#endif


#include <xercesc/dom/DOM.hpp>
#include <xercesc/util/PlatformUtils.hpp>

#include <assert.h>

#include "xerces_common.h"
#include "xerces_write.h"


namespace LQIO {

    /*****************************************************************************/
    /* Everything below has something to do with INSERTING results into the DOM  */
    /*****************************************************************************/

    XercesWrite::XercesWrite()
	: _element(0)
    {
    }

    XercesWrite::XercesWrite( DOMNode * current_node, const XMLCh * element_name )
	: _element(0)
    {
	DOMElement * current_element = dynamic_cast<DOMElement *>(current_node);
	if ( !current_element ) return;		// No element, so don't bother.
	
	/* Look for the first child that matches */

	DOMNodeList *childNodes = current_element->getChildNodes();
	for ( unsigned x = 0; x < childNodes->getLength();x ++ ) {
	    _element = dynamic_cast<DOMElement *>(childNodes->item(x));
	    if ( !_element ) continue;
	    if ( XMLString::compareIStringASCII(_element->getTagName(), element_name) == 0) break;
	}

	if ( !_element ) {
	    insertElement( current_element, element_name );
	}
    } 

    XercesWrite& XercesWrite::operator()( const XMLCh * attribute_name, double arg )
    {
	char buf[32];
	snprintf( buf, 32, "%.7g", arg );
	_element->setAttribute( attribute_name, X(buf) );
	return *this;
    }

    XercesWrite& XercesWrite::operator()( const XMLCh * attribute_name, const char * arg )
    {
	_element->setAttribute( attribute_name, X(arg) );
	return *this;
    }

    XercesWrite& XercesWrite::operator()( const XMLCh * attribute_name, const std::string& arg )
    {
	_element->setAttribute( attribute_name, X(arg.c_str()) );
	return *this;
    }

    XercesWrite& XercesWrite::operator()( const XMLCh * attribute_name, unsigned int arg )
    {
	XMLCh buf[32];
	XMLString::binToText(arg,buf,32,10);
	_element->setAttribute( attribute_name, buf );
	return *this;
    }

    XercesWrite& XercesWrite::insert_time( const XMLCh * attribute_name, unsigned int time )
    {
#if HAVE_SYS_TIME_H
#if defined(CLK_TCK)
	const double dtime = time / static_cast<double>(CLK_TCK);
#else
	const double dtime = time / static_cast<double>(sysconf(_SC_CLK_TCK));
#endif
#else
	const double dtime = time;
#endif

	const double secs  = fmod( floor( dtime ), 60.0 );
	const double mins  = fmod( floor( dtime / 60.0 ), 60.0 );
	const double hrs   = floor( dtime / 3600.0 );
	const double msec  = fmod( dtime * 1000., 1000 );
	char buf[32];
	snprintf( buf, 32, "%02.0f:%02.0f:%02.0f.%03.0f", hrs, mins, secs, msec );
	_element->setAttribute( attribute_name, X(buf) );
	return *this;
    }


    void
    XercesWrite::insertElement( DOMElement * current_element, const XMLCh * element_name ) 
    {
	DOMDocument * document = current_element->getOwnerDocument();
	_element = document->createElement(element_name);

//	DOMNode *spaceNode = current_element->getPreviousSibling()->cloneNode(true);	// This is a newline.  Go figure.
	const XMLCh * spaces = current_element->getPreviousSibling()->getNodeValue();	// This is a newline and spaces.
	unsigned int n_spaces = XMLString::stringLen(spaces);
	XMLCh * spaces2 = new XMLCh[n_spaces+4];		// Tack on three more spaces to make the output look purdie.
	XMLString::moveChars(spaces2,spaces,n_spaces+1);	// Don't forget the null.
	if ( n_spaces >= 3 ) {
	    spaces2[n_spaces+3] = spaces[n_spaces-0];	// Null
	    spaces2[n_spaces+2] = spaces[n_spaces-1];
	    spaces2[n_spaces+1] = spaces[n_spaces-2];
	    spaces2[n_spaces+0] = spaces[n_spaces-3];
	}
	if (current_element->hasChildNodes()) {
	    current_element->insertBefore(_element, current_element->getFirstChild());
	    current_element->insertBefore(inputFileDOM->createTextNode(spaces2), current_element->getFirstChild());
	} else {
	    current_element->appendChild(inputFileDOM->createTextNode(spaces2));
	    current_element->appendChild(dynamic_cast<DOMNode *>(_element));
	    current_element->appendChild(inputFileDOM->createTextNode(spaces));
	}
	delete [] spaces2;
    }
 

    XercesWriteConfidence::XercesWriteConfidence(DOMNode * current_node, const XMLCh * element_name, const ConfidenceIntervals& conf )
	: XercesWrite( current_node, element_name ), _conf(conf)
    {
    }

    XercesWriteConfidence& XercesWriteConfidence::operator()( const XMLCh * attribute_name, double arg )
    {
	XercesWrite::operator()( attribute_name, _conf(arg) );
	return *this;
    }
}
