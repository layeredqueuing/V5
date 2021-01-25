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
#include <algorithm>
#include <cstring>
#include <iomanip>
#include <numeric>
#include <sstream>
#include <sstream>
#include <stdexcept>
#include <fcntl.h>
#include <sys/stat.h>
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#if HAVE_SYS_MMAN_H
#include <sys/types.h>
#include <sys/mman.h>
#endif
#include <lqx/SyntaxTree.h>
#include <lqx/Program.h>
#include "common_io.h"
#include "dom_document.h"
#include "dom_extvar.h"
#include "error.h"
#include "glblerr.h"
#include "input.h"
#include "jmva_document.h"
#include "srvn_gram.h"
#include "srvn_spex.h"
#include "xml_input.h"
#include "xml_output.h"


namespace BCMP {
    /* ---------------------------------------------------------------- */
    /* DOM input.                                                       */
    /* ---------------------------------------------------------------- */

    JMVA_Document::JMVA_Document( const std::string& input_file_name ) : _model(), _input_file_name(input_file_name), _parser(nullptr), _stack(),
									 _variables(), _think_time_vars(), _population_vars(), _multiplicity_vars(), _service_time_vars(), _visit_vars()
    {
    }
    
    JMVA_Document::JMVA_Document( const std::string& input_file_name, const BCMP::Model& model ) : _model(model), _input_file_name(input_file_name), _parser(nullptr), _stack(),
												   _variables(), _think_time_vars(), _population_vars(), _multiplicity_vars(), _service_time_vars(), _visit_vars()
    {
    }

    JMVA_Document::~JMVA_Document()
    {
	for ( std::map<std::string, LQIO::DOM::SymbolExternalVariable*>::const_iterator var = _variables.begin(); var != _variables.end(); ++var ) {
	    delete var->second;
	}
	    
	LQIO::Spex::clear();
    }
    
    /* static */ JMVA_Document *
    JMVA_Document::create( const std::string& input_file_name )
    {
	JMVA_Document * document = new JMVA_Document( input_file_name );
	if ( document->parse() ) {
	    return document;
	} else {
	    delete document;
	    return nullptr;
	}
    }

    bool
    JMVA_Document::load( LQIO::DOM::Document& lqn, const std::string& input_filename )
    {
	JMVA_Document * jmva = create( input_filename );
	if ( !jmva ) return false;
	return  jmva->convertToLQN( lqn );
    }


    
    bool
    JMVA_Document::parse()
    {
	struct stat statbuf;
	bool rc = true;
	int input_fd = -1;

	if ( _input_file_name ==  "-" ) {
	    input_fd = fileno( stdin );
	} else if ( ( input_fd = open( _input_file_name.c_str(), O_RDONLY ) ) < 0 ) {
	    std::cerr << LQIO::io_vars.lq_toolname << ": Cannot open input file " << _input_file_name << " - " << strerror( errno ) << std::endl;
	    return false;
	}

	if ( isatty( input_fd ) ) {
	    std::cerr << LQIO::io_vars.lq_toolname << ": Input from terminal is not allowed." << std::endl;
	    return false;
	} else if ( fstat( input_fd, &statbuf ) != 0 ) {
	    std::cerr << LQIO::io_vars.lq_toolname << ": Cannot stat " << _input_file_name << " - " << strerror( errno ) << std::endl;
	    return false;
#if defined(S_ISSOCK)
	} else if ( !S_ISREG(statbuf.st_mode) && !S_ISFIFO(statbuf.st_mode) && !S_ISSOCK(statbuf.st_mode) ) {
#else
	} else if ( !S_ISREG(statbuf.st_mode) && !S_ISFIFO(statbuf.st_mode) ) {
#endif
	    std::cerr << LQIO::io_vars.lq_toolname << ": Input from " << _input_file_name << " is not allowed." << std::endl;
	    return false;
	}

	LQIO::Spex::__global_variables = &_variables;	/* For SPEX */
	_parser = XML_ParserCreateNS(NULL,'/');     /* Gobble header goop */
	if ( !_parser ) {
	    throw std::runtime_error("Could not allocate memory for Expat.");
	}

	XML_SetElementHandler( _parser, start, end );
	XML_SetCdataSectionHandler( _parser, start_cdata, end_cdata );
	XML_SetCharacterDataHandler( _parser, handle_text );
	XML_SetCommentHandler( _parser, handle_comment );
	XML_SetUnknownEncodingHandler( _parser, handle_encoding, static_cast<void *>(this) );
	XML_SetUserData( _parser, static_cast<void *>(this) );

	_stack.push( parse_stack_t("",&JMVA_Document::startDocument) );

#if HAVE_MMAP
	char *buffer;
#endif
	try {
#if HAVE_MMAP
	    buffer = static_cast<char *>(mmap( 0, statbuf.st_size, PROT_READ, MAP_PRIVATE|MAP_FILE, input_fd, 0 ));
	    if ( buffer != MAP_FAILED ) {
		if ( !XML_Parse( _parser, buffer, statbuf.st_size, true ) ) {
		    const char * error = XML_ErrorString(XML_GetErrorCode(_parser));
		    LQIO::input_error( error );
		    rc = false;
		}
	    } else {
		/* Try the old way (for pipes) */
#endif
		const size_t BUFFSIZE = 1024;
		char buffer[BUFFSIZE];
		size_t len = 0;

		do {
		    len = read( input_fd, buffer, BUFFSIZE );
		    if ( static_cast<int>(len) < 0 ) {
			std::cerr << LQIO::io_vars.lq_toolname << ": Read error on " << _input_file_name << " - " << strerror( errno ) << std::endl;
			rc = false;
			break;
		    } else if (!XML_Parse(_parser, buffer, len, len == 0 )) {
			const char * error = XML_ErrorString(XML_GetErrorCode(_parser));
			LQIO::input_error( error );
			rc = false;
			break;
		    }
		} while ( len > 0 );
#if HAVE_MMAP
	    }
#endif
	}
	/* Halt on any error. */
	catch ( const LQIO::element_error& e ) {
	    LQIO::input_error( "Unexpected element <%s> ", e.what() );
	    rc = false;
	}
	catch ( const std::runtime_error& e ) {
	    rc = false;
	}

#if HAVE_MMAP
	if ( buffer != MAP_FAILED ) {
	    munmap( buffer, statbuf.st_size );
	}
#endif
	XML_ParserFree(_parser);
	_parser = 0;
	close( input_fd );
	return rc;
    }

    /*
     * Handlers called from Expat.
     */

    void
    JMVA_Document::start( void *data, const XML_Char *el, const XML_Char **attr )
    {
	JMVA_Document * document = static_cast<JMVA_Document *>(data);
	LQIO_lineno = XML_GetCurrentLineNumber(document->_parser);
	if ( LQIO::DOM::Document::__debugXML ) {
	    std::cerr << std::setw(4) << LQIO_lineno << ": ";
	    for ( unsigned i = 0; i < document->_stack.size(); ++i ) {
		std::cerr << "  ";
	    }
	    std::cerr << "<" << el;
	    for ( const XML_Char ** attributes = attr; *attributes; attributes += 2 ) {
		std::cerr << " " << *attributes << "=\"" << *(attributes+1) << "\"";
	    }
	    std::cerr << ">" << std::endl;
	}
	try {
	    if ( document->_stack.size() > 0 ) {
		parse_stack_t& top = document->_stack.top();
		(document->*top.start)(top.object,el,attr);
	    }
	}
	catch ( const LQIO::duplicate_symbol& e ) {
	    LQIO::input_error2( LQIO::ERR_DUPLICATE_SYMBOL, el, e.what() );
	}
	catch ( const LQIO::missing_attribute & e ) {
	    LQIO::input_error2( LQIO::ERR_MISSING_ATTRIBUTE, el, e.what() );
	}
	catch ( const LQIO::unexpected_attribute & e ) {
	    LQIO::input_error2( LQIO::ERR_UNEXPECTED_ATTRIBUTE, el, e.what() );
	}
	catch ( const LQIO::undefined_symbol & e ) {
	    LQIO::input_error2( LQIO::ERR_NOT_DEFINED, e.what() );
	}
	catch ( const std::domain_error & e ) {
	    LQIO::input_error( "Domain error: %s ", e.what() );
	}
	catch ( const std::invalid_argument & e ) {
	    LQIO::input_error2( LQIO::ERR_INVALID_ARGUMENT, el, e.what() );
	}
    }

    /*
     * Pop elements off the stack until we hit a matching tag.
     */

    void
    JMVA_Document::end( void *data, const XML_Char *el )
    {
	JMVA_Document * document = static_cast<JMVA_Document *>(data);
	bool done = false;
	while ( document->_stack.size() > 0 && !done ) {
	    parse_stack_t& top = document->_stack.top();
	    if ( LQIO::DOM::Document::__debugXML ) {
		std::cerr << std::setw(4) << LQIO_lineno << ": ";
		for ( unsigned i = 1; i < document->_stack.size(); ++i ) {
		    std::cerr << "  ";
		}
		if ( top.element.size() ) {
		    std::cerr << "</" << top.element << ">" << std::endl;
		} else {
		    std::cerr << "empty stack" << std::endl;
		}
	    }
	    done = top == el;
	    if ( top.end ) {		/* Run the end handler if necessary */
		(document->*top.end)(top.object,el);
	    }
	    document->_stack.pop();
	}
    }

    void
    JMVA_Document::start_cdata( void * data )
    {
    }


    void
    JMVA_Document::end_cdata( void * data )
    {
    }


    /*
     * Ignore most text.  However, for an LQX program, concatenate
     * the text.  Since expat gives us text in "chunks", we can't
     * just simply call setLQXProgram.  Rather, we "append" the
     * program to the existing one.
     */

    void
    JMVA_Document::handle_text( void * data, const XML_Char * text, int length )
    {
	JMVA_Document * parser = static_cast<JMVA_Document *>(data);
	if ( parser->_stack.size() == 0 ) return;
	const parse_stack_t& top = parser->_stack.top();
	if ( top.start == &JMVA_Document::startDescription || top.start == &JMVA_Document::startServiceTime || top.start == &JMVA_Document::startVisit ) {
	    parser->_text.append( text, length );
	}
    }

    /*
     * We tack the comment onto the current element.
     */

    void
    JMVA_Document::handle_comment( void * data, const XML_Char * text )
    {
	JMVA_Document * parser = static_cast<JMVA_Document *>(data);
	if ( parser->_stack.size() == 0 ) return;
	const parse_stack_t& top = parser->_stack.top();
	Model::Object * object = top.object.u.o;
	if ( object ) {
	    std::string& comment = const_cast<std::string&>(object->getComment());
	    if ( comment.size() ) {
		comment += "\n";
	    }
	    comment += text;
	}
    }

    int
    JMVA_Document::handle_encoding( void * data, const XML_Char *name, XML_Encoding *info )
    {
	if ( strcasecmp( name, "ascii" ) == 0 ) {
	    /* Initialize the info argument to handle plain old ascii. */
	    for ( unsigned int i = 0; i < 256; ++i ) {
		info->map[i] = i;
	    }
	    info->convert = 0;		/* No need as its all 1 to 1. */
	    info->data = data;		/* The data argument is a pointer to the current document. */
	    return XML_STATUS_OK;
	} else {
	    return XML_STATUS_ERROR;
	}
    }

    bool
    JMVA_Document::parse_stack_t::operator==( const XML_Char * str ) const
    {
	return element == str;
    }

    bool
    JMVA_Document::checkAttributes( const XML_Char * element_name, const XML_Char ** attributes, const std::set<const XML_Char *,JMVA_Document::attribute_table_t>& table ) const
    {
	bool rc = true;
	for ( ; *attributes; attributes += 2 ) {
	    std::set<const XML_Char *>::const_iterator item = table.find(*attributes);
	    if ( item == table.end() ) {
		if ( strncasecmp( *attributes, "http:", 5 ) != 0 ) {                /* Skip these */
		    LQIO::input_error2( LQIO::ERR_UNEXPECTED_ATTRIBUTE, element_name, *attributes );
		    rc = false;
		}
	    }
	}
	return rc;
    }


    void
    JMVA_Document::registerExternalSymbolsWithProgram(LQX::Program* program)
    {
	std::for_each( _variables.begin(),  _variables.end(), register_variable( program ) );
    }

    void
    JMVA_Document::register_variable::operator()( const std::pair<std::string, LQIO::DOM::SymbolExternalVariable*>& symbol ) const
    {
	symbol.second->registerInEnvironment(_lqx);
    }

    /* ---------------------------------------------------------------- */
    /* Parser functions.                                                */
    /* ---------------------------------------------------------------- */

    void
    JMVA_Document::startDocument( Object& object, const XML_Char * element, const XML_Char ** attributes )
    {
	static const std::set<const XML_Char *,JMVA_Document::attribute_table_t> document_table = { Xxml_debug	};
	
	if ( strcasecmp( element, Xmodel ) == 0 ) {
	    checkAttributes( element, attributes, document_table );
	    LQIO::DOM::Document::__debugXML = (LQIO::DOM::Document::__debugXML || XML::getBoolAttribute(attributes,Xxml_debug));
	    _stack.push( parse_stack_t(element,&JMVA_Document::startModel) );
	} else {
	    throw LQIO::element_error( element );
	}
    }

    void
    JMVA_Document::startModel( Object& object, const XML_Char * element, const XML_Char ** attributes )
    {
	static const std::set<const XML_Char *,JMVA_Document::attribute_table_t> solutions_table = { XalgCount, Xiterations, XiterationValue, Xok, XsolutionMethod };

	if ( strcasecmp( element, Xdescription ) == 0 ) {
	    checkAttributes( element, attributes, null_table );
	    _text.clear();						// Reset text buffer.
	    _stack.push( parse_stack_t(element,&JMVA_Document::startDescription,&JMVA_Document::endDescription,Object(&_model)) );
	} else if ( strcasecmp( element, Xparameters ) == 0 ) {
	    checkAttributes( element, attributes, null_table );
	    _stack.push( parse_stack_t(element,&JMVA_Document::startParameters) );
	} else if ( strcasecmp( element, XalgParams) == 0 ) {
	    checkAttributes( element, attributes, null_table );
	    _stack.push( parse_stack_t(element,&JMVA_Document::startAlgParams) );
	} else if ( strcasecmp( element, XwhatIf ) == 0 ) {
	    static const std::set<const XML_Char *,JMVA_Document::attribute_table_t> whatIf_table = { XclassName, XstationName, Xtype, Xvalues };
	    checkAttributes( element, attributes, whatIf_table );
	    createWhatIf( attributes );
	    _stack.push( parse_stack_t(element,&JMVA_Document::startNOP) );
	} else if ( strcasecmp( element, Xsolutions ) == 0 ) {
	    checkAttributes( element, attributes, solutions_table );
	    _stack.push( parse_stack_t(element,&JMVA_Document::startSolutions) );
	} else {
	    throw LQIO::element_error( element );
	} 
    }


    void JMVA_Document::startDescription( Object&, const XML_Char * element, const XML_Char ** attributes )
    {
	throw LQIO::element_error( element );           	/* Should not get here. */
    }

    
    void JMVA_Document::endDescription( Object& object, const XML_Char * element )
    {
	// Through the magic of unions...
	object.u.m->insertComment(_text);
    }

    
    /*
     * <parameters>
     *   <classes number="1">
     *   <stations number="3">
     *   <ReferenceStation number="1">
     * </parameters>
     */
   
    void
    JMVA_Document::startParameters( Object& object, const XML_Char * element, const XML_Char ** attributes )
    {
	static std::set<const XML_Char *,JMVA_Document::attribute_table_t> parameter_table = { Xnumber };
	
	checkAttributes( element, attributes, parameter_table );		// Hoist: All elements the same 
	if ( strcasecmp( element, Xclasses ) == 0 ) {
	    _stack.push( parse_stack_t(element,&JMVA_Document::startClasses) );
	} else if ( strcasecmp( element, Xstations ) == 0 ) {
	    _stack.push( parse_stack_t(element,&JMVA_Document::startStations) );
	} else if ( strcasecmp( element, XReferenceStation ) == 0 ) {
	    _stack.push( parse_stack_t(element,&JMVA_Document::startReferenceStation) );
	} else {
	    throw LQIO::element_error( element );
	}
    }


    /* 
     * <classes number="1">
     *   <closedclass name="c1" population="4"/>
     * </classes>
     */

    void
    JMVA_Document::startClasses( Object& object, const XML_Char * element, const XML_Char ** attributes )
    {
	if ( strcasecmp( element, Xclosedclass) == 0 ) {
	    static const std::set<const XML_Char *,JMVA_Document::attribute_table_t> closedclass_table = { Xname, Xpopulation };
	    checkAttributes( element, attributes, closedclass_table );
	    createClosedClass( attributes );
	    _stack.push( parse_stack_t(element,&JMVA_Document::startNOP) );
	} else if ( strcasecmp( element, Xopenclass) == 0 ) {
	    static const std::set<const XML_Char *,JMVA_Document::attribute_table_t> openclass_table = { Xname };
	    checkAttributes( element, attributes, openclass_table );
	    createOpenClass( attributes );
	    _stack.push( parse_stack_t(element,&JMVA_Document::startNOP) );
	} else {
	    throw LQIO::element_error( element );
	}
    }


    /*
     * <stations number="3">
     *   <delaystation name="Reference">...
     *   <listation name="p2">...
     * </stations>
     */

    void
    JMVA_Document::startStations( Object& object, const XML_Char * element, const XML_Char ** attributes )
    {
	static const std::set<const XML_Char *,JMVA_Document::attribute_table_t> station_table = { Xname, Xservers };
	checkAttributes( element, attributes, station_table );	// Hoist.  Common to all stations.
	if ( strcasecmp( element, Xdelaystation ) == 0 ) {
	    _stack.push( parse_stack_t(element,&JMVA_Document::startStation,Object(createStation( Model::Station::Type::DELAY, attributes )) ) );
	} else if ( strcasecmp( element, Xlistation ) == 0 ) {
	    const LQIO::DOM::ExternalVariable * servers = getVariableAttribute( attributes, Xservers, 1 );
	    if ( LQIO::DOM::ExternalVariable::isDefault( servers, 1.0 ) ) {
		_stack.push( parse_stack_t(element,&JMVA_Document::startStation,Object(createStation( Model::Station::Type::LOAD_INDEPENDENT, attributes )) ) );
	    } else {
		_stack.push( parse_stack_t(element,&JMVA_Document::startStation,Object(createStation( Model::Station::Type::MULTISERVER, attributes )) ) );
	    }
	} else { // multiserver???
	    throw LQIO::element_error( element );
	}
    }

    /* 
     * <listation name="p3">
     *   <servicetimes>
     *     <servicetime customerclass="c1">1</servicetime>
     *   </servicetimes>
     *   <visits>
     *     <visits customerclass="c1">1</visits>
     *   </visits>
     * </listation>
     */
    
    void
    JMVA_Document::startStation( Object& station, const XML_Char * element, const XML_Char ** attributes )
    {
	checkAttributes( element, attributes, null_table );
	if ( strcasecmp( element, Xservicetimes ) == 0 ) {
	    _stack.push( parse_stack_t(element,&JMVA_Document::startServiceTimes,station) );
	} else if ( strcasecmp( element, Xvisits ) == 0 ) {
	    _stack.push( parse_stack_t(element,&JMVA_Document::startVisits,station) );
	} else {
	    throw LQIO::element_error( element );
	}
    }
    
    /* Place holder for handle_text */
    
    const std::set<const XML_Char *,JMVA_Document::attribute_table_t> JMVA_Document::demand_table = { Xcustomerclass };

    void
    JMVA_Document::startServiceTimes( Object& object, const XML_Char * element, const XML_Char ** attributes )
    {
	checkAttributes( element, attributes, demand_table );	// common to visits
	Model::Station * station = object.u.s;
	assert( station != nullptr );
	std::string class_name = XML::getStringAttribute( attributes, Xcustomerclass );
	if ( strcasecmp( element, Xservicetime ) == 0 ) {
	    _text.clear();					// Reset buffer for handle_text.
	    Model::Station::Class::map_t& demands = station->classes();		// Will insert...
	    const std::pair<Model::Station::Class::map_t::iterator,bool> result = demands.emplace( class_name, Model::Station::Class() );
	    _stack.push( parse_stack_t(element,&JMVA_Document::startServiceTime,&JMVA_Document::endServiceTime,Object(&result.first->second) ) );
	} else {
	    throw LQIO::element_error( element );      		/* Should not get here. */
	}
    }
    
    void
    JMVA_Document::startServiceTime( Object& object, const XML_Char * element, const XML_Char ** attributes )
    {
	throw LQIO::element_error( element );           	/* Should not get here. */
    }

    void JMVA_Document::endServiceTime( Object& object, const XML_Char * element ) 
    {
	const LQIO::DOM::ExternalVariable * service_time = getVariable( element, _text.c_str() );
	object.u.d->setServiceTime( service_time );			// Through the magic of unions...
	if ( dynamic_cast<const LQIO::DOM::SymbolExternalVariable *>(service_time) ) _service_time_vars.emplace(object.u.d,service_time->getName());
    }

    void
    JMVA_Document::startVisits( Object& object, const XML_Char * element, const XML_Char ** attributes )
    {
	checkAttributes( element, attributes, demand_table );	// commmon to servicetime.
	Model::Station * station = object.u.s;
	assert( station != nullptr );
	std::string class_name = XML::getStringAttribute( attributes, Xcustomerclass );
	if ( strcasecmp( element, Xvisit ) == 0 ) {
	    _text.clear();					// Reset buffer for handle_text.
	    Model::Station::Class::map_t& demands = station->classes();		// Will insert...
	    const std::pair<Model::Station::Class::map_t::iterator,bool> result = demands.emplace( class_name, Model::Station::Class() );
	    _stack.push( parse_stack_t(element,&JMVA_Document::startVisit,&JMVA_Document::endVisit,Object(&result.first->second) ) );
	} else {
	    throw LQIO::element_error( element );       	/* Should not get here. */
	}
    }

    /* Place holder for handle_text */

    void
    JMVA_Document::startVisit( Object& object, const XML_Char * element, const XML_Char ** attributes )
    {
	throw LQIO::element_error( element );           	/* Should not get here. */
    }


    void JMVA_Document::endVisit( Object& object, const XML_Char * element ) 
    {
	const LQIO::DOM::ExternalVariable * visits = getVariable( element, _text.c_str() );
	object.u.d->setVisits( visits );				// Through the magic of unions...
	if ( dynamic_cast<const LQIO::DOM::SymbolExternalVariable *>(visits) ) _visit_vars.emplace(object.u.d,visits->getName());
    }
    
    /* 
     * <ReferenceStation number="1">
     *   <Class name="c1" refStation="Reference"/>
     * </ReferenceStation>
     */
    
    void
    JMVA_Document::startReferenceStation( Object& object, const XML_Char * element, const XML_Char ** attributes )
    {
	static const std::set<const XML_Char *,JMVA_Document::attribute_table_t> ReferenceStation_table = { Xname, XrefStation };
	if ( strcasecmp( element, XClass ) == 0 ) {
	    checkAttributes( element, attributes, ReferenceStation_table );
	    const std::string refStation = XML::getStringAttribute( attributes, XrefStation );
	    _model.stationAt( refStation ).setType(Model::Station::Type::CUSTOMER);
	    _stack.push( parse_stack_t(element,&JMVA_Document::startNOP,object) );
	} else {
	    throw LQIO::element_error( element );
	}
    }
    

    /* 
     * <algParams>
     *   <algType maxSamples="10000" name="MVA" tolerance="1.0E-7"/>
     *   <compareAlgs value="false"/>
     * </algParams>
     */

    void
    JMVA_Document::startAlgParams( Object& object, const XML_Char * element, const XML_Char ** attributes )
    {
	if ( strcasecmp( element, XalgType ) == 0 ) {
	    _stack.push( parse_stack_t(element,&JMVA_Document::startNOP,object) );
	} else if ( strcasecmp( element, XcompareAlgs ) == 0 ) {
	    _stack.push( parse_stack_t(element,&JMVA_Document::startNOP,object) );
	} else {
	    throw LQIO::element_error( element );
	}
     }

    /*
     * <solutions algCount="1" iteration="0" iterationValue="1.0" ok="true" solutionMethod="analytical whatif">
     *   <algorithm iterations="0" name="MVA">
     *     <stationresults station="terminals">
     *       <classresults customerclass="c1">
     *         <measure meanValue="0.8683417085427135" measureType="Number of Customers" successful="true"/>
     *         <measure meanValue="0.8683417085427135" measureType="Throughput" successful="true"/>
     *         <measure meanValue="1.0" measureType="Residence time" successful="true"/>
     *         <measure meanValue="0.8683417085427135" measureType="Utilization" successful="true"/>
     *       </classresults>
    */

    void
    JMVA_Document::startSolutions( Object& object, const XML_Char * element, const XML_Char ** attributes )
    {
	if ( strcasecmp( element, Xalgorithm ) == 0 ) {
	    static const std::set<const XML_Char *,JMVA_Document::attribute_table_t> table = { XalgCount, Xiterations, XiterationValue, Xok, XsolutionMethod };
	    checkAttributes( element, attributes, table );
	    _stack.push( parse_stack_t(element,&JMVA_Document::startAlgorithm,object) );
	} else {
	    throw LQIO::element_error( element );
	}
    }

    const std::set<const XML_Char *,JMVA_Document::attribute_table_t> JMVA_Document::class_results_table = { Xcustomerclass };
    
    void
    JMVA_Document::startAlgorithm( Object& object, const XML_Char * element, const XML_Char ** attributes )
    {
	if ( strcasecmp( element, Xstationresults ) == 0 ) {
	    static const std::set<const XML_Char *,JMVA_Document::attribute_table_t> station_results_table = { Xstation };
	    checkAttributes( element, attributes, station_results_table );
	    const std::string name = XML::getStringAttribute(attributes,Xname);
	    _stack.push( parse_stack_t(element,&JMVA_Document::startStationResults,Object(&_model.stationAt(name))) );
	} else if ( strcasecmp( element, Xclassresults ) == 0 ) {
	    checkAttributes( element, attributes, class_results_table );
	    const std::string name = XML::getStringAttribute(attributes,Xname);
	    _stack.push( parse_stack_t(element,&JMVA_Document::startClassResults,Object(&_model.chainAt(name))) );
	} else {
	    throw LQIO::element_error( element );
	}
    }
    
    void
    JMVA_Document::startStationResults( Object& object, const XML_Char * element, const XML_Char ** attributes )
    {
	if ( strcasecmp( element, Xclassresults ) == 0 ) {
	    checkAttributes( element, attributes, class_results_table );
	    const std::string name = XML::getStringAttribute(attributes,Xname);
	    const Object::MK mk(object.u.s,&_model.chainAt(name));
	    _stack.push( parse_stack_t(element,&JMVA_Document::startClassResults,Object(&mk) ) );
	} else {
	    throw LQIO::element_error( element );
	}
    }

    /*
     * Generate SPEX result vars here.
     */
    
    void
    JMVA_Document::startClassResults( Object& object, const XML_Char * element, const XML_Char ** attributes )
    {
	if ( strcasecmp( element, Xmeasure ) == 0 ) {
	    const std::set<const XML_Char *,JMVA_Document::attribute_table_t> measure_table = { XmeanValue, XmeasureType, Xsuccessful };
	    checkAttributes( element, attributes, measure_table );
	    createResults( object, attributes );
	    _stack.push( parse_stack_t(element,&JMVA_Document::startNOP,object) );
	} else {
	    throw LQIO::element_error( element );
	}
    }

    /* 
     */
    
    void
    JMVA_Document::startNOP( Object& object, const XML_Char * element, const XML_Char ** attributes )
    {
	throw LQIO::element_error( element );             /* Should not get here. */
    }

    
    /* 
     * Return either a constant or a variable by converting attribute.  If the attribute is NOT found
     * use the default (if present), or throw missing_attribute.
     */
	
    const LQIO::DOM::ExternalVariable *
    JMVA_Document::getVariableAttribute( const XML_Char **attributes, const XML_Char * attribute, double default_value )
    {
	for ( ; *attributes; attributes += 2 ) {
	    if ( strcasecmp( *attributes, attribute ) == 0 ) return getVariable( attribute, *(attributes+1) );
	}
	if ( default_value >= 0.0 ) {
	    return new LQIO::DOM::ConstantExternalVariable( default_value );
	} else {
	    throw LQIO::missing_attribute( attribute );
	}
    }

    const LQIO::DOM::ExternalVariable *
    JMVA_Document::getVariable( const XML_Char *attribute, const XML_Char *value ) 
    {
	if ( value[0] == '$' ) {
	    const std::map<std::string,LQIO::DOM::SymbolExternalVariable*>::const_iterator var = _variables.find(value);
	    if ( var != _variables.end() ) return var->second;
	    std::pair<const std::map<std::string,LQIO::DOM::SymbolExternalVariable*>::const_iterator,bool> result = _variables.emplace( value, new LQIO::DOM::SymbolExternalVariable(value) );
	    return result.first->second;
	} else {
	    char* endPtr = nullptr;
	    const char* realEndPtr = value + strlen(value);
	    const double real = strtod(value, &endPtr);
	    if ( endPtr != realEndPtr ) throw std::invalid_argument(value);
	    return new LQIO::DOM::ConstantExternalVariable(real);
	}
    }

    void
    JMVA_Document::createClosedClass( const XML_Char ** attributes )
    {
	std::string name = XML::getStringAttribute( attributes, Xname );
	const LQIO::DOM::ExternalVariable * population = getVariableAttribute( attributes, Xpopulation );
	const LQIO::DOM::ExternalVariable * think_time = getVariableAttribute( attributes, Xthinktime, 0.0 );
	std::pair<Model::Chain::map_t::iterator,bool> result = _model.insertChain( name, Model::Chain::CLOSED, population, think_time );
	if ( !result.second ) std::runtime_error( "Duplicate class" );
	if ( dynamic_cast<const LQIO::DOM::SymbolExternalVariable *>(population) ) _population_vars.emplace(&result.first->second,population->getName());
	if ( dynamic_cast<const LQIO::DOM::SymbolExternalVariable *>(think_time) ) _think_time_vars.emplace(&result.first->second,think_time->getName());
    }

    
    void
    JMVA_Document::createOpenClass( const XML_Char ** attributes )
    {
//	_document.insertClass( XML::getStringAttribute( Xname, attributes ),
//			       Model::Chain::CLOSED, 
//			       XML::getLognAttribue( Xpopulation, attributes );
//			       XML::getDoubleAttribute( Xthinktime, attributes, 0.0 ) );
    }

    
    Model::Station *
    JMVA_Document::createStation( Model::Station::Type type, const XML_Char ** attributes )
    {
	scheduling_type scheduling = SCHEDULE_DELAY;
	switch ( type ) {
	case Model::Station::Type::LOAD_INDEPENDENT: scheduling = SCHEDULE_PS; break;
	case Model::Station::Type::MULTISERVER: scheduling = SCHEDULE_PS; break;
	default: break;
	}
	const std::string name = XML::getStringAttribute( attributes, Xname );
	const LQIO::DOM::ExternalVariable * multiplicity = getVariableAttribute( attributes, Xservers, 1 );
	const std::pair<Model::Station::map_t::iterator,bool> result = stations().emplace( name, Model::Station( type, scheduling, multiplicity ) );
	if ( !result.second ) std::runtime_error( "Duplicate station" );
	Model::Station * station = &result.first->second;
	if ( dynamic_cast<const LQIO::DOM::SymbolExternalVariable *>(multiplicity) ) _multiplicity_vars.emplace(station,multiplicity->getName());
	return station;						
    }

    /* 
     * <whatIf type="Customer Numbers" values="1.0;1.5;2.0"/>  <!-- all classes by ratio -->
     * <whatIf className="c1" type="Population Mix" values="0.16666666666666666;0.3333333333333333;0.5;0.6666666666666666;0.8333333333333333"/>
     * <whatIf className="c1" type="Customer Numbers" values="4.0;5.0;6.0;7.0;8.0"/>
     * <whatIf className="c1" stationName="p1" type="Service Demands" values="0.4;0.44000000000000006;0.4800000000000001;0.5200000000000001;0.56;0.6;0.64;0.68;0.72;0.76;0.8"/>
     * <whatIf stationName="p1" type="Service Demands" values="1.0;1.1;1.2;1.3;1.4;1.5;1.6;1.7;1.8;1.9;2.0"/>  <!-- all classes by ratio -->
     * values are comprehensions, but enumerated.
     *
     * If only a className XOR a station name is present, then apply
     * proportinately to all classes or stations, otherwise assign
     * values.  We can be a bit more flexible.The values are a list
     * which can be converted to a comprehension.  The latter is
     * better for QNAP2 output.
     *
     * type can be "Customer Numbers" or "Service Demands" (or
     * "Population Mix" - ratio between two classes)
     */
	
    void
    JMVA_Document::createWhatIf( const XML_Char ** attributes )
    {
	const std::string type = XML::getStringAttribute( attributes, Xtype );
	const std::string className = XML::getStringAttribute( attributes, XclassName, "" );		/* className and/or stationName */
	const std::string stationName = XML::getStringAttribute( attributes, XstationName, "" );	/* className and/or stationName */
	const Comprehension comprehension( XML::getStringAttribute( attributes, Xvalues ) );
	
	std::string name;
	LQIO::DOM::SymbolExternalVariable * x = nullptr;

	if ( type == "Customer Numbers" ) {
	    Model::Chain& k = chains().at(className);
	    /* Get a variable... $N1,$N2,... */
	    std::map<const Model::Chain *,std::string>::iterator var = _population_vars.find(&k);	/* Look for class 		*/
	    if ( var != _population_vars.end() ) {							/* Var is defined for class	*/ 
		x = _variables.at(var->second);								/* So use it			*/
		name = x->getName();
	    } else {
		name = "$N" + std::to_string(_population_vars.size() + 1);				/* Need to create one 		*/
		x = new LQIO::DOM::SymbolExternalVariable( name );
		_population_vars.emplace( &k, name );
		_variables.emplace( name, x );								/* Save it.			*/
	    }
	    k.setCustomers( x );								/* swap constanst for variable in class */

	} else if ( type == "Service Demands" ) {
	    Model::Station& m = stations().at(stationName);
	    if ( !className.empty() ) {
		Model::Station::Class& d = m.classes().at(className);
		/* Get a variable... $S1,$S2,... */
		std::map<const Model::Station::Class *,std::string>::iterator var = _service_time_vars.find( &d );
		if ( var != _service_time_vars.end() ) {
		    x = _variables.at(var->second);
		    name = x->getName();
		} else {
		    name = "$S" + std::to_string(_service_time_vars.size() + 1);
		    x = new LQIO::DOM::SymbolExternalVariable( name );
		    _service_time_vars.emplace( &d, name );
		    _variables.emplace( name, x );
		}
		d.setServiceTime( x );
	    } else {
		abort();
	    }

	} else if ( type == "Number of Servers" ) {
	    Model::Station& m = stations().at(stationName);
	    /* Get a variable... $S1,$S2,... */
	    std::map<const Model::Station *,std::string>::iterator var = _multiplicity_vars.find( &m );
	    if ( var != _multiplicity_vars.end() ) {
		x = _variables.at(var->second);
		name = x->getName();
	    } else {
		name = "$M" + std::to_string(_multiplicity_vars.size() + 1);
		x = new LQIO::DOM::SymbolExternalVariable( name );
		_multiplicity_vars.emplace( &m, name );
		_variables.emplace( name, x );
	    }
	    m.setCopies( x );
	} else {
	    abort();
	}

	/* create a spex array comprehension for evaluation by LQX */
	spex_array_comprehension( name.c_str(), comprehension.begin(), comprehension.end(), comprehension.stride() );
    }

    void
    JMVA_Document::createResults( const Object& object, const XML_Char ** attributes )
    {
	static const std::map<const std::string,int> table = { {XNumber_of_Customers, 0}, {XThroughput, 1}, {XResidence_time, 2}, {XUtilization, 3} };
	assert ( object.getDiscriminator() == Object::type::PAIR );
	const Model::Station * m = object.getMK()->first;
	const Model::Chain   * k = object.getMK()->second;
	const std::string result = XML::getStringAttribute(attributes,XmeasureType);

	switch ( table.at(result) ) {
	case 1:		/* Queue length */
	    break;
	case 2:
	    break;
	case 3:
	    break;
	case 4:
	    break;
	}
    }
    
    void
    JMVA_Document::Comprehension::convert( const std::string& s )
    {
	double previous = 0;
	char * endptr = nullptr;
	for ( const char *p = s.data(); *p != '\0'; p = endptr ) {
	    if ( *p == ';' ) ++p;
	    double value = strtod( p, &endptr );
	    if ( *endptr != '\0' && *endptr != ';' ) throw std::invalid_argument( s );
	    _end = value;			/* always take the last */
	    if ( _begin < 0 ) {
		_begin = value;
	    } else {
		_stride = value - previous;
	    }
	    previous = value;
	}
    }
    
}

namespace BCMP {

    std::ostream&
    JMVA_Document::print( std::ostream& output ) const
    {
	std::for_each( stations().begin(), stations().end(), BCMP::Model::pad_demand( chains() ) );	/* JMVA want's zeros */
	
	XML::set_indent(0);
	output << "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>" << std::endl
	       << "<!-- " << LQIO::DOM::Common_IO::svn_id() << " -->" << std::endl;
	if ( LQIO::io_vars.lq_command_line.size() > 0 ) {
	    output << "<!-- " << LQIO::io_vars.lq_command_line << " -->" << std::endl;
	}

	output << XML::start_element( Xmodel )
	       << XML::attribute( "xmlns:xsi", std::string("http://www.w3.org/2001/XMLSchema-instance") )
	       << XML::attribute( "xsi:noNamespaceSchemaLocation", std::string("JMTmodel.xsd") )
	       << ">" << std::endl;
	      
	if ( !_model.comment().empty() ) {
	    output << XML::start_element( Xdescription ) << ">" << std::endl
		   << XML::cdata( _model.comment() ) << std::endl
		   << XML::end_element( Xdescription ) << ">" << std::endl;
	}

 	output << XML::start_element( Xparameters ) << ">" << std::endl;

	output << XML::start_element( Xclasses ) << XML::attribute( Xnumber, static_cast<unsigned int>(chains().size()) ) << ">" << std::endl;
	std::for_each( chains().begin(), chains().end(), printClass( output ) );
	output << XML::end_element( Xclasses ) << std::endl;

	output << XML::start_element( Xstations ) << XML::attribute( Xnumber, static_cast<unsigned int>(stations().size()) ) << ">" << std::endl;
	std::for_each( stations().begin(), stations().end(), printStation( output ) );
	output << XML::end_element( Xstations ) << std::endl;

	output << XML::start_element( XReferenceStation ) << XML::attribute( Xnumber, static_cast<unsigned int>(chains().size()) ) << ">" << std::endl;
	std::for_each( chains().begin(), chains().end(), printReference( output, stations() ) );
	output << XML::end_element( XReferenceStation ) << std::endl;

	output << XML::end_element( Xparameters ) << std::endl;
	output << XML::start_element( XalgParams ) << ">" << std::endl
	       << XML::simple_element( XalgType ) << XML::attribute( "maxSamples", 10000U ) << XML::attribute( Xname, std::string("MVA") ) << XML::attribute( "tolerance", 1.0E-7 ) << "/>" << std::endl
	       << XML::simple_element( XcompareAlgs ) << XML::attribute( Xvalue, false ) << "/>" << std::endl
	       << XML::end_element( XalgParams ) << std::endl;

	/* SPEX */
	/* Insert WhatIf for statements for arrays and completions. */
	/* 	<whatIf className="c1" stationName="p2" type="Service Demands" values="1.0;1.1;1.2;1.3;1.4;1.5;1.6;1.7;1.8;1.9;2.0"/> */

	if ( !Spex::array_variables().empty() ) {
	    output << "<!-- SPEX arrays and completions -->" << std::endl;
	    std::for_each( Spex::array_variables().begin(), Spex::array_variables().end(), what_if( output, _model ) );
	}

	/* SPEX */
	/* Insert a results section, but only to output the variables */
	
	if ( !Spex::result_variables().empty() ) {
	    std::map<const std::string,std::multimap<const std::string,const std::string> > results;
	    /* Store in a map<station,multimap<class,measure>>, then output by station and class. */
	    std::for_each( Spex::result_variables().begin(), Spex::result_variables().end(), getObservations( results, model() ) );
	    output << XML::start_element( Xsolutions ) << XML::attribute( Xok, Xfalse ) << ">" << std::endl;
	    output << XML::start_element( Xiterations ) << ">" << std::endl;
	    /* Output by station, then class */
	    output << XML::end_element( Xiterations ) << std::endl;
	    output << XML::end_element( Xsolutions ) << std::endl;
	}
	output << XML::end_element( Xmodel ) << std::endl;
	return output;
    }

    /*
     * Print out the special "reference" station.
     */
    
    void
    JMVA_Document::printStation::operator()( const Model::Station::pair_t& m ) const
    {
	const BCMP::Model::Station& station = m.second;
	std::string element;
	switch ( station.type() ) {
	case Model::Station::Type::CUSTOMER:
	case Model::Station::Type::DELAY:
	    element = Xdelaystation;
	    break;
	case Model::Station::Type::MULTISERVER:
	case Model::Station::Type::LOAD_INDEPENDENT:
	    element = Xlistation;
	    break;
	case Model::Station::Type::NOT_DEFINED:
	    throw std::range_error( "JMVA_Document::printStation::operator(): Undefined station type." );
	}
	
	_output << XML::start_element( element ) << XML::attribute( Xname, m.first );
	if ( station.copies() != nullptr ) _output << XML::attribute( Xservers, *station.copies() );
	_output << ">" << std::endl;
	_output << XML::start_element( Xservicetimes ) << ">" << std::endl;
	std::for_each( station.classes().begin(), station.classes().end(), printService( _output ) );
	_output << XML::end_element( Xservicetimes ) << std::endl;
	_output << XML::start_element( Xvisits ) << ">" << std::endl;
	std::for_each( station.classes().begin(), station.classes().end(), printVisits( _output ) );
	_output << XML::end_element( Xvisits ) << std::endl;
	_output << XML::end_element( element ) << std::endl;
    }

    void
    JMVA_Document::printClass::operator()( const BCMP::Model::Chain::pair_t& k ) const
    {
	if ( k.second.isInClosedModel() ) {
	    _output << XML::simple_element( "closedclass" )
		    << XML::attribute( "name", k.first )
		    << XML::attribute( "population", *k.second.customers() )
		    << "/>" << std::endl;
	}
	if ( k.second.isInOpenModel() ) {
	}
    }
    
    void
    JMVA_Document::printReference::operator()( const BCMP::Model::Chain::pair_t& k ) const
    {
	BCMP::Model::Station::map_t::const_iterator m = std::find_if( _stations.begin(), _stations.end(), &Model::Station::isCustomer );
	if ( m != _stations.end() ) {
	    _output << XML::simple_element( XClass )
		    << XML::attribute( Xname, k.first )
		    << XML::attribute( XrefStation, m->first )
		    << "/>" << std::endl;
	}
    }


    void
    JMVA_Document::printService::operator()( const BCMP::Model::Station::Class::pair_t& d ) const
    {
	std::ostringstream service_time;
	if ( d.second.service_time() ) {
	    service_time << *d.second.service_time();
	} else {
	    service_time << 0;
	}
	_output << XML::inline_element( Xservicetime, Xcustomerclass, d.first, service_time.str() ) << std::endl;
    }

    void
    JMVA_Document::printVisits::operator()( const BCMP::Model::Station::Class::pair_t& d ) const
    {
	std::ostringstream visits;
	if ( d.second.visits() ) {
	    visits << *d.second.visits();
	} else {
	    visits << 0;
	}
	_output << XML::inline_element( Xvisit, Xcustomerclass, d.first, visits.str() ) << std::endl;
    }

    /*
     * Generate for WhatIf.  
     * <whatIf className="c1" type="Customer Numbers" values="4.0;5.0;6.0;7.0;8.0"/>
     * <whatIf className="c1" stationName="p1" type="Service Demands" values="0.4;0.44000000000000006;0.4800000000000001;0.5200000000000001;0.56;0.6;0.64;0.68;0.72;0.76;0.8"/>
     */

    void
    JMVA_Document::what_if::operator()( const std::string& var ) const
    {
	_output << XML::simple_element( XwhatIf );
	std::ostringstream ss;
	std::string array;
	BCMP::Model::Chain::map_t::const_iterator k = std::find_if( chains().begin(), chains().end(), what_if::has_customers( var ) );
	if ( k != chains().end() ) {
	    _output << XML::attribute( XclassName, k->first )
		    << XML::attribute( Xtype, "Customer Numbers" );
	} else {
	    BCMP::Model::Station::map_t::const_iterator m = std::find_if( stations().begin(), stations().end(), what_if::has_var( var ) );
	    if ( m != stations().end() ) {
		const LQIO::DOM::ExternalVariable * copies = m->second.copies();
		if ( copies != nullptr && !copies->wasSet() && copies->getName() == var ) {
		    _output << XML::attribute( XstationName, m->first )
			    << XML::attribute( Xtype, "Number of Servers" );
		} else {
		    const BCMP::Model::Station::Class::map_t& classes = m->second.classes();
		    const BCMP::Model::Station::Class::map_t::const_iterator d = std::find_if( classes.begin(), classes.end(), what_if::has_service_time( var ) );
		    _output << XML::attribute( XstationName, m->first )
			    << XML::attribute( XclassName, d->first )
			    << XML::attribute( Xtype, "Service Demands" );
		}
	    }
	}

	const std::map<std::string,Spex::ComprehensionInfo>::const_iterator comprehension = Spex::comprehensions().find( var );
	if ( comprehension != Spex::comprehensions().end() ) {
	    /* Simple, run the comprehension directly */
	    for ( double value = comprehension->second.getInit(); value <= comprehension->second.getTest(); value += comprehension->second.getStep() ) {
		if ( value != comprehension->second.getInit() ) ss << ";";
		ss << value;
	    }
	    array = ss.str();
	} else {
	    /* Little harder, the values are encoded in the varible's LQX statement directly */
	    const std::map<std::string,LQX::SyntaxTreeNode *>::const_iterator lqx = Spex::input_variables().find(var);
	    lqx->second->print(ss);		/* So print out the LQX */
	    array = ss.str();
	    /* Get rid of brackets and spaces from array_create(0.5, 1, 1.5) */
	    array.erase(array.begin(), array.begin()+13);		/* "array_create(" */
	    array.erase(std::prev(array.end()),array.end());		/* ")" */
	    std::replace(array.begin(), array.end(), ',', ';' );	/* Xerces wants ';', not ',' */
	    array.erase(std::remove(array.begin(), array.end(), ' '), array.end());	/* Strip blanks */
	}
	_output << XML::attribute( Xvalues, array );
	_output << "/>  <!--" << var << "-->" << std::endl;
    }

    /* Return true if this class has the variable */
    
    bool
    JMVA_Document::what_if::has_customers::operator()( const Model::Chain::pair_t& k ) const
    {
	const LQIO::DOM::ExternalVariable * var = k.second.customers();
	return var != nullptr && !var->wasSet() && var->getName() == _var;
    }

    bool
    JMVA_Document::what_if::has_var::operator()( const Model::Station::pair_t& m ) const
    {
	const LQIO::DOM::ExternalVariable * var = m.second.copies();
	if ( var != nullptr && !var->wasSet() && var->getName() == _var ) return true;
	const BCMP::Model::Station::Class::map_t& classes = m.second.classes();
	return std::any_of( classes.begin(), classes.end(), what_if::has_service_time( _var ) );
    }

    bool
    JMVA_Document::what_if::has_service_time::operator()( const Model::Station::Class::pair_t& d ) const
    {
	const LQIO::DOM::ExternalVariable * var = d.second.service_time();
	return var != nullptr && !var->wasSet() && var->getName() == _var;
    }

    /*
     * Need to search Spex::observations() for var.first (the name).
     * Use the key to find right function.  Next, we use the name of
     * the object to find out if it's a class or a station.  If it's a
     * class, then query the reference station, otherwise query the
     * station.
     */

    void
    JMVA_Document::getObservations::operator()( const Spex::var_name_and_expr& var ) const
    {
	static const std::map<int,JMVA_Document::getObservations::f> key_map = {
	    { KEY_THROUGHPUT,	    	&getObservations::get_throughput },
#if 0
	    { KEY_UTILIZATION,	    	&getObservations::get_utilization },
	    { KEY_PROCESSOR_UTILIZATION,&getObservations::get_utilization },
	    { KEY_PROCESSOR_WAITING,    &getObservations::get_waiting_time },
	    { KEY_SERVICE_TIME,	    	&getObservations::get_service_time },
	    { KEY_WAITING,		&getObservations::get_waiting_time }
#endif
	};	/* Maps srvn_gram.h KEY_XXX to qnap2 function */

	const Spex::obs_var_tab_t observations = Spex::observations();

	/* try to find the observation, then see if we can handle it here */
	for ( Spex::obs_var_tab_t::const_iterator obs = observations.begin(); obs != observations.end(); ++obs ) {
	    if ( obs->second.getVariableName() != var.first ) continue;	/* !Found the var. */

	    std::pair<std::string,std::string> expression;
	    const std::map<int,f>::const_iterator key = key_map.find( obs->second.getKey() );
	    if ( key != key_map.end() ) {
		/* Map up to entity */
	    } else {
		expression.first = "\"N/A\"";
//		throw std::domain_error( "Observation not handled" );
	    }
//	    _output << qnap2_statement( var.first + ":=" + expression.first, expression.second ) << std::endl;
	    break;
	}
    }

    /* mservice, mbusypct, mcustnb, vcustnb, mresponse, mthruput, custnb */

    std::string
    JMVA_Document::getObservations::get_throughput( const std::string& name ) const
    {
	std::string result;
	if ( chains().find( name ) != chains().end() ) {
	    /* terminals,name, throughput */
	    /* Class is a reference task, and name exists even if we have only one class */
	} else if ( stations().find( name ) != stations().end() ) {
	    /* name,class?*/
	} else {
	}
	return result;
    }
    
}

namespace BCMP {
    using namespace LQIO;

    bool JMVA_Document::convertToLQN( DOM::Document& document ) const
    {
	return _model.convertToLQN( document );
    }
}


namespace BCMP {
    const std::set<const XML_Char *,JMVA_Document::attribute_table_t> JMVA_Document::algParams_table = { XmaxSamples, Xname, Xtolerance };
    const std::set<const XML_Char *,JMVA_Document::attribute_table_t> JMVA_Document::compareAlgs_table = { XmeanValue, XmeasureType, Xsuccessful };
    const std::set<const XML_Char *,JMVA_Document::attribute_table_t> JMVA_Document::null_table = {};
    
    const XML_Char * JMVA_Document::XClass		= "Class";
    const XML_Char * JMVA_Document::XReferenceStation	= "ReferenceStation";
    const XML_Char * JMVA_Document::XalgParams		= "algParams";
    const XML_Char * JMVA_Document::XalgType		= "algType";
    const XML_Char * JMVA_Document::Xclasses		= "classes";
    const XML_Char * JMVA_Document::Xclosedclass	= "closedclass";
    const XML_Char * JMVA_Document::XcompareAlgs	= "compareAlgs";
    const XML_Char * JMVA_Document::Xcustomerclass	= "customerclass";
    const XML_Char * JMVA_Document::Xdelaystation	= "delaystation";
    const XML_Char * JMVA_Document::Xdescription	= "description";
    const XML_Char * JMVA_Document::Xlistation		= "listation";
    const XML_Char * JMVA_Document::XmaxSamples		= "maxSamples";
    const XML_Char * JMVA_Document::Xmodel		= "model";
    const XML_Char * JMVA_Document::Xmultiplicity	= "multiplicity";
    const XML_Char * JMVA_Document::Xname		= "name";
    const XML_Char * JMVA_Document::Xnumber		= "number";
    const XML_Char * JMVA_Document::Xopenclass		= "openclass";
    const XML_Char * JMVA_Document::Xparameters		= "parameters";
    const XML_Char * JMVA_Document::Xpopulation		= "population";
    const XML_Char * JMVA_Document::XrefStation		= "refStation";
    const XML_Char * JMVA_Document::Xservers		= "servers";
    const XML_Char * JMVA_Document::Xservicetime	= "servicetime";
    const XML_Char * JMVA_Document::Xservicetimes	= "servicetimes";
    const XML_Char * JMVA_Document::Xstation		= "station";
    const XML_Char * JMVA_Document::Xstations		= "stations";
    const XML_Char * JMVA_Document::Xthinktime 		= "thinktime";
    const XML_Char * JMVA_Document::Xtolerance		= "tolerance";
    const XML_Char * JMVA_Document::Xvalue		= "value";
    const XML_Char * JMVA_Document::Xvisit		= "visit";
    const XML_Char * JMVA_Document::Xvisits		= "visits";
    const XML_Char * JMVA_Document::XwhatIf		= "whatIf";
    const XML_Char * JMVA_Document::Xxml_debug 		= "xml-debug";

    const XML_Char * JMVA_Document::Xalgorithm 		= "algorithm";
    const XML_Char * JMVA_Document::Xclassresults	= "classresults";
    const XML_Char * JMVA_Document::XmeanValue		= "meanValue";
    const XML_Char * JMVA_Document::Xmeasure		= "measure";
    const XML_Char * JMVA_Document::XmeasureType        = "measureType";
    const XML_Char * JMVA_Document::Xsolutions 		= "solutions";
    const XML_Char * JMVA_Document::Xstationresults	= "stationresults";
    const XML_Char * JMVA_Document::Xsuccessful		= "successful";

    const XML_Char * JMVA_Document::XclassName		= "className";
    const XML_Char * JMVA_Document::XstationName	= "stationName";
    const XML_Char * JMVA_Document::Xtype		= "type";
    const XML_Char * JMVA_Document::Xvalues		= "values";

    const XML_Char * JMVA_Document::XalgCount 		= "algCount"; 
    const XML_Char * JMVA_Document::Xiterations		= "iterations";
    const XML_Char * JMVA_Document::XiterationValue	= "iterationValue";
    const XML_Char * JMVA_Document::Xok			= "ok";
    const XML_Char * JMVA_Document::Xfalse		= "false";
    const XML_Char * JMVA_Document::XsolutionMethod   	= "solutionMethod";

    const XML_Char * JMVA_Document::XNumber_of_Customers= "Number of Customers";
    const XML_Char * JMVA_Document::XThroughput         = "Throughput";
    const XML_Char * JMVA_Document::XResidence_time     = "Residence time";
    const XML_Char * JMVA_Document::XUtilization        = "Utilization";
    
}
