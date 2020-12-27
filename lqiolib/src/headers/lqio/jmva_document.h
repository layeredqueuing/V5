/* -*- C++ -*-
 *  $Id: expat_document.h 13717 2020-08-03 00:04:28Z greg $
 *
 *  Created by Martin Mroz on 24/02/09.
 */

#ifndef __LQIO_JMVA_DOCUMENT__
#define __LQIO_JMVA_DOCUMENT__

#include <expat.h>
#include <stack>
#include "bcmp_document.h"

namespace BCMP {
    class JMVA_Document;
    typedef void (JMVA_Document::*start_fptr)( JMVA::BCMP_Object *, const XML_Char *, const XML_Char ** );
    typedef void (JMVA_Document::*end_fptr)( JMVA::BCMP_Object *, const XML_Char * );

    class JMVA_Document {

	struct parse_stack_t
	{
	    parse_stack_t(const XML_Char * e, start_fptr sh, JMVA::BCMP_Object * o ) : element(e), start(sh), end(nullptr), object(o) {}
	    parse_stack_t(const XML_Char * e, start_fptr sh, end_fptr eh, JMVA::BCMP_Object * o ) : element(e), start(sh), end(eh), object(o) {}
	    bool operator==( const XML_Char * ) const;

	    const std::basic_string<XML_Char> element;
	    start_fptr start;
	    end_fptr end;
	    JMVA::BCMP_Object * object;
	};

	struct attribute_table_t
	{
	    bool operator()( const XML_Char * s1, const XML_Char * s2 ) const { return strcasecmp( s1, s2 ) < 0; }
	};
	
    private:
	JMVA_Document( const std::string& input_file_name );
	
	bool checkAttributes( const XML_Char * element, const XML_Char ** attributes, std::set<const XML_Char *,JMVA_Document::attribute_table_t>& table ) const;

	void startDocument( JMVA::BCMP_Object *, const XML_Char * element, const XML_Char ** attributes );
	void startModel( JMVA::BCMP_Object *, const XML_Char * element, const XML_Char ** attributes );
	void startParameters( JMVA::BCMP_Object *, const XML_Char * element, const XML_Char ** attributes );
	void startClasses( JMVA::BCMP_Object *, const XML_Char * element, const XML_Char ** attributes );
	void startClass( JMVA::BCMP_Object *, const XML_Char * element, const XML_Char ** attributes );
	void startStations( JMVA::BCMP_Object *, const XML_Char * element, const XML_Char ** attributes );
	
	static void start( void *data, const XML_Char *el, const XML_Char **attr );
	static void end( void *data, const XML_Char *el );
	static void start_cdata( void *data );
	static void end_cdata( void *data );
	static void handle_text( void * data, const XML_Char * text, int length );
	static void handle_comment( void * data, const XML_Char * text );
	static int handle_encoding( void * data, const XML_Char *name, XML_Encoding *info );

    public:
	virtual ~JMVA_Document() {}
	static JMVA_Document * create( const std::string& input_file_name );
	
    private:
	bool parse();
	static void init_tables();
	
    private:
	JMVA * _document;
	const std::string _input_file_name;
	XML_Parser _parser;
	std::stack<parse_stack_t> _stack;

	static std::set<const XML_Char *,attribute_table_t> class_table;
	static std::set<const XML_Char *,attribute_table_t> document_table;
	static std::set<const XML_Char *,attribute_table_t> null_table;
	static std::set<const XML_Char *,attribute_table_t> parameter_table;
	static std::set<const XML_Char *,attribute_table_t> station_table;
	
	static const XML_Char * XReferenceStation;
	static const XML_Char * XalgParams;
	static const XML_Char * Xclasses;
	static const XML_Char * Xclosedclass;
	static const XML_Char * Xcustomerclass;
	static const XML_Char * Xdelaystation;
	static const XML_Char * Xlistation;
	static const XML_Char * Xmodel;
	static const XML_Char * Xname;
	static const XML_Char * Xnumber;
	static const XML_Char * Xopenclass;
	static const XML_Char * Xparameters;
	static const XML_Char * Xservicetimes;
	static const XML_Char * Xstations;
	static const XML_Char * Xvisits;
	static const XML_Char * Xxml_debug;
    };
}
#endif
