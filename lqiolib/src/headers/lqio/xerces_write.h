/************************************************************************/
/* Copyright the Real-Time and Distributed Systems Group,		*/
/* Department of Systems and Computer Engineering,			*/
/* Carleton University, Ottawa, Ontario, Canada. K1S 5B6		*/
/* 									*/
/* 2009.								*/
/************************************************************************/

/*
 * $Id: xerces_write.h 11963 2014-04-10 14:36:42Z greg $
 *
 * This class is used to hide the methods used to output to the Xerces DOM.
 */

#ifndef __XERCES_RESULT_H
#define __XERCES_RESULT_H

#include <string>
#include <ostream>
#include <xercesc/dom/DOM.hpp>
#include <xercesc/util/XercesDefs.hpp>		// For XMLCh type.
#include "confidence_intervals.h"

XERCES_CPP_NAMESPACE_USE


namespace LQIO {

    bool serializeDOM(std::ostream&, bool instantiate=false);
    int doesDOMcontainResults();

    class XercesWrite {
    private:
	XercesWrite( const XercesWrite& );
	XercesWrite& operator=( const XercesWrite& );
	
    public:
	XercesWrite();
	XercesWrite( DOMNode * current_node, const XMLCh * element_name );
	virtual ~XercesWrite() {}

	virtual XercesWrite& operator()( const XMLCh * attribute_name, double );
	XercesWrite& operator()( const XMLCh * attribute_name, const char * );
	XercesWrite& operator()( const XMLCh * attribute_name, const std::string& );
	XercesWrite& operator()( const XMLCh * attribute_name, unsigned int );
	XercesWrite& insert_time( const XMLCh * attribute_name, unsigned int );
	DOMElement * getElement() const { return _element; }

	void insertElement( DOMElement *, const XMLCh * element_name );

    private:
	DOMElement * _element;
    };

    class XercesWriteConfidence: public XercesWrite, private ConfidenceIntervals {
    public:
	XercesWriteConfidence( DOMNode * current_node, const XMLCh * element_name, const ConfidenceIntervals& conf );
	virtual ~XercesWriteConfidence() {}

	virtual XercesWriteConfidence& operator()( const XMLCh * attribute_name, double );

    private:
	const ConfidenceIntervals& _conf;
    };
}

#endif /* DOM_PRINT_ERROR_HANDLER_HPP */
