/* -*- c++ -*-
 * $Id: jmva_document.cpp 15429 2022-02-04 23:04:05Z greg $
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

#define BUG_343 1
#define BUG_344 1

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
#include "bcmp_bindings.h"
#include "common_io.h"
#include "dom_document.h"
#include "dom_extvar.h"
#include "error.h"
#include "filename.h"
#include "glblerr.h"
#include "input.h"
#include "jmva_document.h"
#include "srvn_gram.h"
#include "srvn_spex.h"
#include "xml_input.h"
#include "xml_output.h"


namespace BCMP {

    /*
     * BSD complains about the implicit copy constructor.
     */
  
    JMVA_Document::Object::Object( const Object& o ) : _discriminator(o._discriminator)
    {
        switch ( _discriminator ) {
	case type::CLASS:   u.k = o.u.k; break;
	case type::DEMAND:  u.d = o.u.d; break;
	case type::MODEL:   u.m = o.u.m; break;
	case type::OBJECT:  u.o = o.u.o; break;
	case type::PAIR:    u.mk = o.u.mk; break;
	case type::STATION: u.s = o.u.s; break;
	case type::VOID:    u.v = nullptr; break;
	    /* NO default to catch new discriminators through warning */
	}
    }
  
    /* ---------------------------------------------------------------- */
    /* DOM input.                                                       */
    /* ---------------------------------------------------------------- */

    JMVA_Document::JMVA_Document( const std::string& input_file_name ) :
	_model(), _input_file_name(input_file_name), _parser(nullptr), _stack(),
	_pragmas(), _lqx_program_text(), _lqx_program_line_number(0), _lqx_program(nullptr), _spex_program(nullptr), _variables(), 
	_think_time_vars(), _population_vars(), _arrival_rate_vars(),
	_multiplicity_vars(), _service_time_vars(), _visit_vars(),
	_plot_population_mix(false), _x1(), _x2()
    {
	LQIO::DOM::Document::__input_file_name = input_file_name;
    }

    JMVA_Document::JMVA_Document( const std::string& input_file_name, const BCMP::Model& model ) :
	_model(model), _input_file_name(input_file_name), _parser(nullptr), _stack(),
	_pragmas(), _lqx_program_text(), _lqx_program_line_number(0), _lqx_program(nullptr), _spex_program(nullptr), _variables(), 
	_think_time_vars(), _population_vars(), _arrival_rate_vars(),
	_multiplicity_vars(), _service_time_vars(), _visit_vars(),
	_plot_population_mix(false), _x1(), _x2()
    {
	LQIO::DOM::Document::__input_file_name = input_file_name;
    }

    JMVA_Document::~JMVA_Document()
    {
	for ( std::map<std::string, LQIO::DOM::SymbolExternalVariable*>::const_iterator var = _variables.begin(); var != _variables.end(); ++var ) {
	    delete var->second;
	}
	
	LQIO::Spex::clear();
	LQIO::DOM::Document::__input_file_name.clear();
    }

    /*
     * Load the document 
     */
    
    bool
    JMVA_Document::load()
    {
	if ( !parse() ) return false;

	/* LQX present? */
	const std::string& program_text = getLQXProgramText();
	if ( !program_text.empty() ) {
	    LQX::Program* program = nullptr;
	    program = LQX::Program::loadFromText(_input_file_name.c_str(), getLQXProgramLineNumber(), program_text.c_str());
	    setLQXProgram( program );
	}
	return true;
    }


    /* 
     * For loading a JMVA document into an LQN document.  There is a
     * memory leak here, but the external variables are shared between
     * the jmva document and the dom.
     */
    
    bool
    JMVA_Document::load( LQIO::DOM::Document& lqn, const std::string& input_file_name )
    {
	JMVA_Document * jmva = new JMVA_Document( input_file_name );
	if ( !jmva->parse() ) return false;
	return  jmva->convertToLQN( lqn );
    }


    bool
    JMVA_Document::parse()
    {
	struct stat statbuf;
	bool rc = true;
	int input_fd = -1;

	if ( !Filename::isFileName( _input_file_name ) ) {
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
	    LQIO::input_error( "Runtime error: %s ", e.what() );
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
	catch ( const std::out_of_range& e ) {
	    LQIO::input_error( "Undefined variable." );
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
	} else if ( top.start == &JMVA_Document::startLQX ) {
	    std::string& program = const_cast<std::string &>(parser->getLQXProgramText());
	    program.append( text, length );
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
	if ( top.object.isObject() ) {
	    Model::Object * object = top.object.getObject();
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
    JMVA_Document::mergePragmas(const std::map<std::string,std::string>& list)
    {
	_pragmas.merge( list );
    }
    
    void
    JMVA_Document::registerExternalSymbolsWithProgram(LQX::Program* program)
    {
	std::for_each( _variables.begin(), _variables.end(), register_variable( program ) );
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
	static const std::set<const XML_Char *,JMVA_Document::attribute_table_t> document_table = { Xxml_debug, Xjaba };
	
	if ( strcasecmp( element, Xmodel ) == 0 ) {
	    checkAttributes( element, attributes, document_table );
	    LQIO::DOM::Document::__debugXML = (LQIO::DOM::Document::__debugXML || XML::getBoolAttribute(attributes,Xxml_debug));
	    _stack.push( parse_stack_t(element,&JMVA_Document::startModel,&JMVA_Document::endModel,object) );
	} else {
	    throw LQIO::element_error( element );
	}
    }

    void
    JMVA_Document::startModel( Object& object, const XML_Char * element, const XML_Char ** attributes )
    {
	static const std::set<const XML_Char *,JMVA_Document::attribute_table_t> solutions_table = { XalgCount, Xiteration, XiterationValue, Xok, XsolutionMethod, XResultVariables };

	if ( strcasecmp( element, Xpragma ) == 0 ) {
	    _pragmas.insert( XML::getStringAttribute(attributes,Xparam), XML::getStringAttribute(attributes,Xvalue,"") );
	    _stack.push( parse_stack_t(element,&JMVA_Document::startNOP) );
	} else if ( strcasecmp( element, Xdescription ) == 0 ) {
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
	    setResultVariables( XML::getStringAttribute( attributes, XResultVariables, "" ) );
	    _stack.push( parse_stack_t(element,&JMVA_Document::startSolutions) );
	} else if ( strcasecmp( element, XLQX ) == 0 ) {
	    setLQXProgramLineNumber(XML_GetCurrentLineNumber(_parser));
	    _stack.push( parse_stack_t(element,&JMVA_Document::startLQX) );
	} else {
	    throw LQIO::element_error( element );
	}
    }


    /*
     * If I have a Whatif, but no results, create them
     */

    void
    JMVA_Document::endModel( Object& object, const XML_Char * element )
    {
//	if ( _variables.empty() || !LQIO::Spex::__result_variables.empty() || _plot_population_mix ) return;
	if ( !LQIO::Spex::__result_variables.empty() ) return;

	for (std::map<std::string,LQIO::DOM::SymbolExternalVariable*>::const_iterator var = _variables.begin(); var != _variables.end(); ++var ) {
	    appendResultVariable( var->first );
	}
	/* For all stations... create name_X, name_Q, name_R and name_U */
	for ( Model::Station::map_t::const_iterator m = stations().begin(); m != stations().end(); ++m ) {
	    static const std::map<const std::string,const Model::Result::Type> result = {
		{"$Q", Model::Result::Type::QUEUE_LENGTH},
		{"$X", Model::Result::Type::THROUGHPUT},
		{"$R", Model::Result::Type::RESIDENCE_TIME},
		{"$U", Model::Result::Type::UTILIZATION}
	    };

	    std::for_each( result.begin(), result.end(), create_result( *this, m ) );

	}
    }


    void JMVA_Document::startDescription( Object&, const XML_Char * element, const XML_Char ** attributes )
    {
	throw LQIO::element_error( element );           	/* Should not get here. */
    }


    void JMVA_Document::endDescription( Object& object, const XML_Char * element )
    {
	// Through the magic of unions...
	object.getModel()->insertComment(LQIO::rtrim(LQIO::ltrim(_text)));
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
	    static const std::set<const XML_Char *,JMVA_Document::attribute_table_t> closedclass_table = { Xname, Xpopulation, Xthinktime };
	    checkAttributes( element, attributes, closedclass_table );
	    createClosedChain( attributes );
	    _stack.push( parse_stack_t(element,&JMVA_Document::startNOP) );
	} else if ( strcasecmp( element, Xopenclass) == 0 ) {
	    static const std::set<const XML_Char *,JMVA_Document::attribute_table_t> openclass_table = { Xname, Xrate };
	    checkAttributes( element, attributes, openclass_table );
	    createOpenChain( attributes );
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
	    if ( !LQIO::DOM::ExternalVariable::isDefault( getVariableAttribute( attributes, Xservers, 1 ), 1.0 ) ) {
		solution_error( LQIO::ERR_INVALID_PARAMETER, Xservers, Xlistation, XML::getStringAttribute( attributes, Xname ), "Not equal to 1" );
	    }
	    _stack.push( parse_stack_t(element,&JMVA_Document::startStation,Object(createStation( Model::Station::Type::LOAD_INDEPENDENT, attributes )) ) );
	} else if ( strcasecmp( element, Xldstation ) == 0 ) {
	    _stack.push( parse_stack_t(element,&JMVA_Document::startStation,Object(createStation( Model::Station::Type::MULTISERVER, attributes )) ) );
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
	Model::Station * station = object.getStation();
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
	object.getDemand()->setServiceTime( service_time );			// Through the magic of unions...
	if ( dynamic_cast<const LQIO::DOM::SymbolExternalVariable *>(service_time) ) _service_time_vars.emplace(object.getDemand(),service_time->getName());
    }

    void
    JMVA_Document::startVisits( Object& object, const XML_Char * element, const XML_Char ** attributes )
    {
	checkAttributes( element, attributes, demand_table );	// commmon to servicetime.
	Model::Station * station = object.getStation();
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
	object.getDemand()->setVisits( visits );				// Through the magic of unions...
	if ( dynamic_cast<const LQIO::DOM::SymbolExternalVariable *>(visits) ) _visit_vars.emplace(object.getDemand(),visits->getName());
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
	    if ( refStation != XArrivalProcess ) {
		_model.stationAt( refStation ).setReference(true);
	    }
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
	    static const std::set<const XML_Char *,JMVA_Document::attribute_table_t> table = { Xiterations, Xname };
	    checkAttributes( element, attributes, table );
	    _stack.push( parse_stack_t(element,&JMVA_Document::startAlgorithm,object) );
	} else {
	    throw LQIO::element_error( element );
	}
    }

    void
    JMVA_Document::startAlgorithm( Object& object, const XML_Char * element, const XML_Char ** attributes )
    {
	if ( strcasecmp( element, Xstationresults ) == 0 ) {
	    static const std::set<const XML_Char *,JMVA_Document::attribute_table_t> station_results_table = {
		Xstation
	    };
	    
	    checkAttributes( element, attributes, station_results_table );
	    const std::string name = XML::getStringAttribute(attributes,Xstation);
	    Object mk_object(Object::MK(&_model.stationAt(name),nullptr));
	    _stack.push( parse_stack_t(element,&JMVA_Document::startStationResults,mk_object) );
	} else if ( strcasecmp( element, Xnormconst ) == 0 ) {
	} else {
	    throw LQIO::element_error( element );
	}
    }

    const std::set<const XML_Char *,JMVA_Document::attribute_table_t> JMVA_Document::measure_table = {
	XmeanValue,
	XmeasureType,
	Xsuccessful
    };

    void
    JMVA_Document::startStationResults( Object& object, const XML_Char * element, const XML_Char ** attributes )
    {
	const std::set<const XML_Char *,JMVA_Document::attribute_table_t> class_results_table = {
	    Xcustomerclass,
	};

	Object::MK& mk = object.getMK();
	if ( strcasecmp( element, Xclassresults ) == 0 ) {
	    checkAttributes( element, attributes, class_results_table );
	    const std::string name = XML::getStringAttribute(attributes,Xcustomerclass);
	    const BCMP::Model::Station * m = mk.first;
	    mk.second = &m->classAt(name);
	    _stack.push( parse_stack_t(element,&JMVA_Document::startClassResults,object) );
	} else if ( strcasecmp( element, Xmeasure ) == 0 ) {
	    checkAttributes( element, attributes, measure_table );
	    mk.second = nullptr;
	    createMeasure( object, attributes );
	    _stack.push( parse_stack_t(element,&JMVA_Document::startNOP,object) );
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
	    checkAttributes( element, attributes, measure_table );
	    createMeasure( object, attributes );
	    _stack.push( parse_stack_t(element,&JMVA_Document::startNOP,object) );
	} else {
	    throw LQIO::element_error( element );
	}
    }


    /*
     */

    void
    JMVA_Document::startLQX( Object& object, const XML_Char * element, const XML_Char ** attributes )
    {
	throw LQIO::element_error( element );             /* Should not get here. */
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
    JMVA_Document::createClosedChain( const XML_Char ** attributes )
    {
	std::string name = XML::getStringAttribute( attributes, Xname );
	const LQIO::DOM::ExternalVariable * population = getVariableAttribute( attributes, Xpopulation );
	const LQIO::DOM::ExternalVariable * think_time = getVariableAttribute( attributes, Xthinktime, 0.0 );
	std::pair<Model::Chain::map_t::iterator,bool> result = _model.insertClosedChain( name, population, think_time );
	if ( !result.second ) std::runtime_error( "Duplicate class" );
	if ( dynamic_cast<const LQIO::DOM::SymbolExternalVariable *>(population) ) _population_vars.emplace(&result.first->second,population->getName());
	if ( dynamic_cast<const LQIO::DOM::SymbolExternalVariable *>(think_time) ) _think_time_vars.emplace(&result.first->second,think_time->getName());
    }


    void
    JMVA_Document::createOpenChain( const XML_Char ** attributes )
    {
	std::string name = XML::getStringAttribute( attributes, Xname );
	const LQIO::DOM::ExternalVariable * arrival_rate = getVariableAttribute( attributes, Xrate );
	std::pair<Model::Chain::map_t::iterator,bool> result = _model.insertOpenChain( name, arrival_rate );
	if ( !result.second ) std::runtime_error( "Duplicate class" );
	if ( dynamic_cast<const LQIO::DOM::SymbolExternalVariable *>(arrival_rate) ) _arrival_rate_vars.emplace(&result.first->second,arrival_rate->getName());
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
	const std::pair<Model::Station::map_t::iterator,bool> result = _model.insertStation( name, Model::Station( type, scheduling, multiplicity ) );
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
     * values are generators, but enumerated.
     *
     * If only a className XOR a station name is present, then apply
     * proportinately to all classes or stations, otherwise assign
     * values.  We can be a bit more flexible.  The values are a list
     * which can be converted to a generator.  The latter is better
     * for QNAP2 output.
     *
     * type can be "Customer Numbers" or "Service Demands" (or
     * "Population Mix" - ratio between two classes)
     */
	
    const std::map<const std::string,JMVA_Document::setIndependentVariable> JMVA_Document::independent_var_table = {
	{ XArrival_Rates, 	&JMVA_Document::setArrivalRate },
	{ XCustomer_Numbers, 	&JMVA_Document::setCustomers },
	{ XNumber_of_Servers, 	&JMVA_Document::setMultiplicity },
	{ XPopulation_Mix,	&JMVA_Document::setPopulationMix },
	{ XService_Demands, 	&JMVA_Document::setDemand }
    };

    void
    JMVA_Document::createWhatIf( const XML_Char ** attributes )
    {
	const std::string className = XML::getStringAttribute( attributes, XclassName, "" );		/* className and/or stationName */
	const std::string stationName = XML::getStringAttribute( attributes, XstationName, "" );	/* className and/or stationName */

	const std::string x_label = XML::getStringAttribute( attributes, Xtype );
	std::map<const std::string,JMVA_Document::setIndependentVariable>::const_iterator f = independent_var_table.find( x_label );
	if ( f == independent_var_table.end() ) throw std::runtime_error( "JMVA_Document::createWhatIf" );

	const std::string x_var = (this->*(f->second))( stationName, className );

	const Generator generator( XML::getStringAttribute( attributes, Xvalues ) );
	LQX::SyntaxTreeNode * statement = nullptr;
	if ( generator.begin() == generator.end() ) {
	    /* One item = scalar */
	    statement = static_cast<LQX::SyntaxTreeNode *>(spex_assignment_statement( x_var.c_str(), new LQX::ConstantValueExpression( generator.begin() ), true ));
	    LQIO::Spex::__input_variables[x_var] = statement;	/* Save for output */
	} else {
	    /* Stride present, so it's a... */
	    statement = static_cast<LQX::SyntaxTreeNode *>( spex_array_comprehension( "_i", 0., generator.count(), 1.0 ) );
	    LQX::SyntaxTreeNode * assignment_expr = new LQX::AssignmentStatementNode( new LQX::VariableExpression( &x_var.c_str()[1], false ),
										      new LQX::MathExpression( LQX::MathExpression::MULTIPLY,
													       new LQX::VariableExpression( "i", false ),
													       new LQX::ConstantValueExpression( generator.stride() ) ) );
	    LQIO::Spex::__deferred_assignment.insert( LQIO::Spex::__deferred_assignment.begin(), assignment_expr );
	    LQIO::Spex::__input_variables[x_var] = assignment_expr;	/* Save for output */
//	    statement = static_cast<LQX::SyntaxTreeNode *>( spex_array_comprehension( x_var.c_str(), generator.begin(), generator.end(), generator.stride() ) );
//	} else {
//	    /* it's a string of values */
	}

	/* Add the loop to the program */
	_spex_program = static_cast<expr_list *> (spex_list( _spex_program, statement ));

	/* If this is the first WhatIf, then set the first x variable for gnuplot */
	if ( _x1.empty() ) {
	    _x1.set( x_var, x_label, generator.end() );
	} else if ( _x2.empty() ) {
	    _x2.set( x_var, x_label, generator.end() );
	}
    }



    /*
     * Set the result variables.  They can be either at the Station or at the Class within the station.
     *
     *	<measure meanValue="1.0" measureType="Number of Customers" successful="true"/>
     *  <measure meanValue="1.0" measureType="Throughput" successful="true"/>
     *	<measure meanValue="1.0" measureType="Residence time" successful="true"/>
     *  <measure meanValue="1.0" measureType="Utilization" successful="true"/>
     */

    void
    JMVA_Document::createMeasure( Object& object, const XML_Char ** attributes )
    {
	static const std::map<const std::string,const Model::Result::Type> measure_table = {
	    {XNumber_of_Customers,  Model::Result::Type::QUEUE_LENGTH},
	    {XThroughput, 	    Model::Result::Type::THROUGHPUT},
	    {XResidence_Time, 	    Model::Result::Type::RESIDENCE_TIME},
	    {XUtilization, 	    Model::Result::Type::UTILIZATION}
	};

	std::string value = XML::getStringAttribute( attributes, XmeanValue );
	if ( !value.empty() && value[0] == '$' ) {
	    const Model::Station * m = const_cast<Model::Station*>(object.getMK().first);
	    const Model::Station::Class * k = const_cast<Model::Station::Class *>(object.getMK().second);

	    createObservation( value, measure_table.at(XML::getStringAttribute( attributes, XmeasureType ) ), m, k );
	}
    }

    std::string
    JMVA_Document::setArrivalRate( const std::string& stationName, const std::string& className )
    {
	Model::Chain& k = chains().at(className);
	LQIO::DOM::SymbolExternalVariable * x = nullptr;
	std::string name;
	std::map<const Model::Chain *,std::string>::iterator var = _arrival_rate_vars.find( &k );		/* chain, var	*/
	if ( var != _arrival_rate_vars.end() ) {
	    x = _variables.at(var->second);
	    name = x->getName();
	} else {
	    name = "$A" + std::to_string(_arrival_rate_vars.size() + 1);
	    x = new LQIO::DOM::SymbolExternalVariable( name );
	    _arrival_rate_vars.emplace( &k, name );
	    _variables.emplace( name, x );
	}
	k.setArrivalRate( x );
	return name;
    }


    std::string
    JMVA_Document::setCustomers( const std::string& stationName, const std::string& className )
    {
	Model::Chain& k = chains().at(className);
	LQIO::DOM::SymbolExternalVariable * x = nullptr;
	std::string name;
	/* Get a variable... $N1,$N2,... */
	std::map<const Model::Chain *,std::string>::iterator var = _population_vars.find(&k);	/* Look for class 		*/
	if ( var != _population_vars.end() ) {							/* Var is defined for class	*/
	    x = _variables.at(var->second);							/* So use it			*/
	    name = x->getName();
	} else {
	    name = "$N" + std::to_string(_population_vars.size() + 1);				/* Need to create one 		*/
	    x = new LQIO::DOM::SymbolExternalVariable( name );
	    _population_vars.emplace( &k, name );
	    _variables.emplace( name, x );							/* Save it.			*/
	}
	k.setCustomers( x );								/* swap constanst for variable in class */
	return name;
    }

    std::string
    JMVA_Document::setDemand( const std::string& stationName, const std::string& className )
    {
	Model::Station& m = stations().at(stationName);
	LQIO::DOM::SymbolExternalVariable * x = nullptr;
	std::string name;
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
	return name;
    }

    std::string
    JMVA_Document::setMultiplicity( const std::string& stationName, const std::string& className )
    {
	Model::Station& m = stations().at(stationName);
	LQIO::DOM::SymbolExternalVariable * x = nullptr;
	std::string name;
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
	return name;
    }

    /*
     * A little more complicated as there are two (or more?) classes.
     * The station is ignored.  Only two classes are allowed.  The
     * Beta parameter determines the fraction of customers in
     * className, starting with 1 up to className.customers-1.
     */

    std::string
    JMVA_Document::setPopulationMix( const std::string& stationName, const std::string& className )
    {
 	if ( chains().size() != 2 ) throw std::runtime_error( "JMVA_Document::setPopulationMix" );
	setPlotPopulationMix( true );
	
	const Model::Chain::map_t::iterator i = chains().begin();
	const Model::Chain::map_t::iterator j = std::next(i);
	const std::string beta = "$Beta";					/* Local variable	*/

	LQX::SyntaxTreeNode * assignment_expr;

	/*
	 * Two new variables are needed, n1, for class 1, which is $N
	 * times class 1 customers, and n2, which is (1-$N) times
	 * class 2 customers.  Both need to be rounded to the next
	 * highest integer.  Return the "beta" value to the caller.
	 */
	
	const Model::Chain::map_t::iterator k1 = i->first == className ? i : j;
	const Model::Chain::map_t::iterator k2 = i->first == className ? j : i;
	const double k1_customers = to_double( *k1->second.customers() );	/* Get original (constant) values	*/
	const double k2_customers = to_double( *k2->second.customers() );	/* Get original (constant) values	*/

	const std::string class1_population = "$N_" + k1->first;
	const std::string x_name = "_N_" + k1->first;
	_x1.set( x_name, x_name, k1_customers );

	LQIO::DOM::SymbolExternalVariable * n1 = new LQIO::DOM::SymbolExternalVariable( class1_population );
	_population_vars.emplace( &k1->second, class1_population );
	_variables.emplace( class1_population, n1 );		/* allows Spex to change customers in class1...	*/
	k1->second.setCustomers( n1 );				/* ... so swap constanst for variable in class.	*/
	_spex_program = static_cast<expr_list *>(spex_list( _spex_program,
							    new LQX::AssignmentStatementNode( new LQX::VariableExpression( x_name, false ),
											      new LQX::ConstantValueExpression( k1_customers ) ) ) );
	expr_list * function_args = new expr_list;
	function_args->push_back( new LQX::MathExpression( LQX::MathExpression::MULTIPLY,
							   new LQX::VariableExpression( &beta[1], false ),
							   new LQX::VariableExpression( x_name, false ) ) );
	assignment_expr = new LQX::AssignmentStatementNode( new LQX::VariableExpression( &class1_population[1], false ), new LQX::MethodInvocationExpression( "round", function_args ) );
	LQIO::Spex::__deferred_assignment.push_back( assignment_expr );
	LQIO::Spex::__input_variables[class1_population] = assignment_expr;

	const std::string class2_population = "$N_" + k2->first;
	const std::string y_name = "_N_" + k2->first;
	_x2.set( y_name, y_name, k2_customers );
	
	LQIO::DOM::SymbolExternalVariable * n2 = new LQIO::DOM::SymbolExternalVariable( class2_population );
	_population_vars.emplace( &k2->second, class2_population );
	_variables.emplace( class2_population, n2 );		/* allows Spex to change customers in class1...	*/
	k2->second.setCustomers( n2 );				/* ... so swap constanst for variable in class.	*/
	_spex_program = static_cast<expr_list *>(spex_list( _spex_program,
							    new LQX::AssignmentStatementNode( new LQX::VariableExpression( y_name, false ),
											      new LQX::ConstantValueExpression( k2_customers ) ) ) );
	function_args = new expr_list;
	function_args->push_back( new LQX::MathExpression( LQX::MathExpression::MULTIPLY,
							   new LQX::MathExpression( LQX::MathExpression::SUBTRACT,  new LQX::ConstantValueExpression( 1. ), new LQX::VariableExpression( &beta[1], false ) ),
							   new LQX::VariableExpression( y_name, false ) ) );
	assignment_expr = new LQX::AssignmentStatementNode( new LQX::VariableExpression( &class2_population[1], false ), new LQX::MethodInvocationExpression( "round", function_args ) );
	LQIO::Spex::__deferred_assignment.push_back( assignment_expr );
	LQIO::Spex::__input_variables[class2_population] = assignment_expr;
	return beta;
    }

    /*
     * Save the result variables that control the output.  If none are
     * present (like if reading a model produced from JMVA, then
     * auto-create them all).
     */

    void
    JMVA_Document::setResultVariables( const std::string& attr )
    {
	std::string s = attr;
	if ( s.empty() ) return;

	/* Tokeninze the input string on ';' */
	size_t pos = 0;
	while ((pos = s.find(";")) != std::string::npos) {
	    std::string variable = s.substr(0, pos);
	    s.erase(0, pos + 1);
	    appendResultVariable( variable );
	}
	if ( !s.empty() ) {
	    appendResultVariable( s );
	}
    }


    /*
     * Create result and observation.
     */

    void
    JMVA_Document::create_result::operator()( const std::pair<const std::string,const Model::Result::Type>& r ) const
    {
	std::string name;
	name = r.first + "_" + _m->first;	// Don't forget leading $!
	std::replace( name.begin(), name.end(), ' ', '_' );			/* Remove spaces from names */
	_self.appendResultVariable( name );
	_self.createObservation( name, r.second, &_m->second, nullptr );	/* Station results only */
    }


    LQX::SyntaxTreeNode *
    JMVA_Document::createObservation( const std::string& name, Model::Result::Type type, const Model::Station * m, const Model::Station::Class * k )
    {
	static const std::map<const BCMP::Model::Result::Type,const char * const> lqx_function = {
	    { BCMP::Model::Result::Type::RESIDENCE_TIME, BCMP::__lqx_residence_time },
	    { BCMP::Model::Result::Type::THROUGHPUT, BCMP::__lqx_throughput },
	    { BCMP::Model::Result::Type::UTILIZATION, BCMP::__lqx_utilization },
	    { BCMP::Model::Result::Type::QUEUE_LENGTH, BCMP::__lqx_queue_length }
	};

	/* Will need station/class names -- search map for m */
	BCMP::Model::Station::map_t::const_iterator mi = model().findStation(m);
	if ( mi == model().stations().end() ) return nullptr;

	/* Get the station object */
	LQX::MethodInvocationExpression * object = new LQX::MethodInvocationExpression( m->getTypeName(), new LQX::ConstantValueExpression( mi->first ), nullptr );
	
	if ( k == nullptr ) {
	    /* No class, so return station function to extract result */
	    const_cast<Model::Station *>(m)->insertResultVariable( type, name );
	    
	} else {
	    /* Get the class object for m the station */
	    BCMP::Model::Station::Class::map_t::const_iterator mk = m->findClass(k);
	    if ( mk == m->classes().end() ) return nullptr;
	    object = new LQX::MethodInvocationExpression( k->getTypeName(), object, new LQX::ConstantValueExpression( mk->first ), 0 );
	    const_cast<BCMP::Model::Station::Class *>(k)->insertResultVariable( type, name );

	}
	LQIO::Spex::__observation_variables[name] = new LQX::AssignmentStatementNode( getObservationVariable( name ),	/* Strip $ for LQX variable name */
										      new LQX::ObjectPropertyReadNode( object, lqx_function.at(type) ) );
	return object;
    }

    LQX::SyntaxTreeNode *
    JMVA_Document::createObservation( const std::string& name, Model::Result::Type type, const std::string& clasx )
    {
	static const std::map<const BCMP::Model::Result::Type,const char * const> lqx_function = {
	    { BCMP::Model::Result::Type::QUEUE_LENGTH,	 BCMP::__lqx_queue_length },
	    { BCMP::Model::Result::Type::RESIDENCE_TIME, BCMP::__lqx_residence_time },
	    { BCMP::Model::Result::Type::RESPONSE_TIME,  BCMP::__lqx_response_time },
	    { BCMP::Model::Result::Type::THROUGHPUT,     BCMP::__lqx_throughput },
            { BCMP::Model::Result::Type::UTILIZATION,    BCMP::__lqx_utilization }
	};

	/* Get the model object */
	LQX::MethodInvocationExpression * object = new LQX::MethodInvocationExpression( "chain", new LQX::ConstantValueExpression(clasx), nullptr );
	LQIO::Spex::__observation_variables[name] = new LQX::AssignmentStatementNode( getObservationVariable( name ),	/* Strip $ for LQX variable name */
										      new LQX::ObjectPropertyReadNode( object, lqx_function.at(type) ) );
	return object;
    }


    /*
     * Convert a string of the form "v_1;v_2;...;v_n" to v_1, v_n, n.
     * The assumption is that all of the values "v" are monotonically
     * increasing and evenly distributed with a difference of 
     * (v_n - v_1)/(n-1) between pairs.
     */
    
    void
    JMVA_Document::Generator::convert( const std::string& s )
    {
	char * endptr = nullptr;
	for ( const char *p = s.data(); *p != '\0'; p = endptr ) {
	    if ( *p == ';' ) ++p;
	    const double value = strtod( p, &endptr );
	    if ( (*endptr != '\0' && *endptr != ';') || (value < _end) || (_count != 0 && value == _end) ) throw std::invalid_argument( s );
	    _end = value;			/* always take the last */
	    if ( p == s.data() ) {
		_begin = _end;
	    } else {
		_count += 1;
	    }
	}
    }

    std::vector<std::string>
    JMVA_Document::getUndefinedExternalVariables() const
    {
	/* Returns a list of all undefined external variables as a string */
	std::vector<std::string> names;
	std::for_each( _variables.begin(), _variables.end(), notSet(names) );
	return names;
    }

    /*
     * Strip the leading $ from name as all of the result variables are assigned inside the whatIf for loop
     */

    void
    JMVA_Document::appendResultVariable( const std::string& name )
    {
	if ( name.empty() ) return;
	LQIO::Spex::__result_variables.emplace_back( LQIO::Spex::var_name_and_expr( name, getObservationVariable( name ) ) );
    }

    /*
     * Get an observation variable (which should not be an input variable).
     */
    
    LQX::VariableExpression * JMVA_Document::getObservationVariable( const std::string& name ) const
    {
	LQX::VariableExpression * var = nullptr;
	if ( std::isdigit( name[1] ) ) {
	    std::string local = name;
	    local[0] = '_';					  	/* Swap $ to _ */
	    var = new LQX::VariableExpression( local, false );
	} else {
	    var = new LQX::VariableExpression( &name[1], false );	/* Strip $ */
	}
	return var;
    }
	

    /*
     * Plot throughput/response time for system (and bounds)
     */

    void
    JMVA_Document::plot( Model::Result::Type type, const std::string& arg )
    {
	LQIO::Spex::__observation_variables.clear();	/* Get rid of them all. */
	LQIO::Spex::__result_variables.clear();		/* Get rid of them all. */
	_model.clearAllResultVariables();		/* Get rid of them all.	*/
	_gnuplot.push_back( LQIO::Spex::print_node( "set title \"" + _model.comment() + "\"" ) );
	_gnuplot.push_back( LQIO::Spex::print_node( "#set output \"" + LQIO::Filename( _input_file_name, "svg", "", "" )() + "\"" ) );
	_gnuplot.push_back( LQIO::Spex::print_node( "#set terminal svg" ) );

	std::ostringstream plot;		// Plot command collected here.
	plot << "plot ";

	if ( type == Model::Result::Type::THROUGHPUT && plotPopulationMix() ) {
	    plot_population_mix_vs_throughput( plot );
	} else if ( type == Model::Result::Type::UTILIZATION && plotPopulationMix() ) {
	    plot_population_mix_vs_utilization( plot );
	} else if ( arg.empty() ) {
	    plot_chain( plot, type );
	} else if ( chains().find( arg ) != chains().end() ) {
	    plot_class( plot, type, arg );		/* If arg is a class, plot all stations */
	} else if ( stations().find( arg ) != stations().end() ) {
	    plot_station( plot, type, arg );		/* If arg is a station, plot all classes */
	} else {
	    throw std::invalid_argument( arg );
	}

	/* Append the plot command to the program (plot has to be near the end) */
	_gnuplot.push_back( LQIO::Spex::print_node( "set datafile separator \",\"" ) );		/* Use CSV. */
	_gnuplot.push_back( LQIO::Spex::print_node( plot.str() ) );
    }


    /*
     * for all stations plot class arg.
     */

    std::ostream&
    JMVA_Document::plot_class( std::ostream& plot, Model::Result::Type type, const std::string& arg )
    {
	appendResultVariable( _x1.var );
	_gnuplot.push_back( LQIO::Spex::print_node( "set xlabel \"" + _x1.label + "\"" ) );			// X axis
	_gnuplot.push_back( LQIO::Spex::print_node( "set ylabel \"" + y_label_table.at(type) + "\"" ) );	// Y1 axis
	_gnuplot.push_back( LQIO::Spex::print_node( "set key title \"Class " + arg + "\"" ) );
	_gnuplot.push_back( LQIO::Spex::print_node( "set key top left box" ) );

	const size_t x = 1;		/* GNUPLOT starts from 1, not 0 */
	size_t y = x;
	
	for ( Model::Station::map_t::const_iterator m = stations().begin(); m != stations().end(); ++m ) {
	    if ( m->second.reference() || !m->second.hasClass(arg) ) continue;

	    if ( y > 1 ) plot << ", ";

	    /* Create observation, var name is class name. */
	    y += 1;
	    const std::string y_var = "$" + m->first;
	    createObservation( y_var, type, &m->second, &m->second.classAt(arg) );
	    appendResultVariable( y_var );

	    /* Append plot command to plot */
	    std::string title = m->first;
	    plot << "\"$DATA\" using " << x << ":" << y << " with linespoints"
		 << " title \"" << title << "\"";
	
	}

	return plot;
    }

    /*
     * for station arg plot all classes
     */

    std::ostream&
    JMVA_Document::plot_station( std::ostream& plot, Model::Result::Type type, const std::string& arg )
    {
	appendResultVariable( _x1.var );
	_gnuplot.push_back( LQIO::Spex::print_node( "set xlabel \"" + _x1.label + "\"" ) );			// X axis
	_gnuplot.push_back( LQIO::Spex::print_node( "set ylabel \"" + y_label_table.at(type) + "\"" ) );	// Y1 axis
	_gnuplot.push_back( LQIO::Spex::print_node( "set key title \"Station " + arg + "\"" ) );
	_gnuplot.push_back( LQIO::Spex::print_node( "set key top left box" ) );

	const size_t x = 1;		/* GNUPLOT starts from 1, not 0 */
	size_t y = x;
	
	const Model::Station& station = stations().at( arg );
	for ( Model::Station::Class::map_t::const_iterator k = station.classes().begin(); k != station.classes().end(); ++k ) {
	    if ( k != station.classes().begin() ) plot << ", ";

	    /* Create observation, var name is class name. */
	    y += 1;
	    const std::string y_var = "$" + k->first;
	    createObservation( y_var, type, &station, &k->second );
	    appendResultVariable( y_var );

	    /* Append plot command to plot */
	    std::string title = k->first;
	    plot << "\"$DATA\" using " << x << ":" << y << " with linespoints"
		 << " title \"" << title << "\"";
	
	}

	return plot;
    }


    std::ostream&
    JMVA_Document::plot_chain( std::ostream& plot, Model::Result::Type type )
    {
	static const std::map<const Model::Result::Type, const std::string> y_labels = {
	    {Model::Result::Type::THROUGHPUT,     XThroughput },
	    {Model::Result::Type::RESPONSE_TIME,  XResponse_Time },
	};

	appendResultVariable( _x1.var );
	_gnuplot.push_back( LQIO::Spex::print_node( "set xlabel \"" + _x1.label + "\"" ) );		// X axis
	_gnuplot.push_back( LQIO::Spex::print_node( "set ylabel \"" + y_labels.at(type) + "\"" ) );	// Y1 axis
	if ( type == Model::Result::Type::THROUGHPUT ) {
	    _gnuplot.push_back( LQIO::Spex::print_node( "set key bottom right" ) );
	    _gnuplot.push_back( LQIO::Spex::print_node( "set key box" ) );
	}

	/* Create observation variables (Y axis).  X axis will be WhatIf. */

	size_t n_labels = 0;
	double y_max = 0.;

	const size_t x = 1;		/* GNUPLOT starts from 1, not 0 */
	size_t y = x;

	for ( Model::Chain::map_t::const_iterator k = chains().begin(); k != chains().end(); ++k ) {
	    Model::Bound bounds( *k, stations() );

	    if ( k != chains().begin() ) plot << ", ";

	    /* Create observation, var name is class name. */
	    const std::string y_var = "$" + k->first;
	    createObservation( y_var, type, k->first );
	    appendResultVariable( y_var );

	    /* Append plot command to plot */
	    y += 1;
	    std::string title;
	    if ( chains().size() > 1 ) {
		title = k->first + " ";
	    }
	    title += "MVA";
	    plot << "\"$DATA\" using " << x << ":" << y << " with linespoints"
		 << " title \"" << title << "\"";

	    /* Now plot the bounds. */
	    if ( type == Model::Result::Type::THROUGHPUT || type == Model::Result::Type::RESPONSE_TIME ) {
		std::ostringstream label1;
		std::ostringstream label2;
		std::string title1;
		std::string title2;
		const double nStar = (bounds.D_sum() + bounds.Z()) / bounds.D_max();
		double bound1 = 0.0;

		if ( chains().size() > 1 ) {
		    title1 = k->first + " ";
		    title2 = k->first + " ";
		}
		switch ( type ) {
		case Model::Result::Type::THROUGHPUT:
		    bound1 = 1. / bounds.D_max();
		    title1 += "1/Dmax";
		    title2 += "1/(Dsum+Z)";
		    break;
		case Model::Result::Type::RESPONSE_TIME:
		    bound1 = bounds.D_sum();
		    title1 += "Dsum";
		    title2 += "N*Dmax-Z";
		    break;
		default:
		    break;
		}
		
		n_labels += 1;
		label1 << "set label " << n_labels << " \"" << bound1 << "\" at " << 0.2 << "," << bound1 * 1.02 << "," << 0. << " left";
		_gnuplot.push_back( LQIO::Spex::print_node( label1.str() ) );

		n_labels += 1;
		label2 << "set label " << n_labels << " \"N*=" << nStar << "\" at " << nStar << "," << bound1 * 1.02 << "," << 0. << " right";
		_gnuplot.push_back( LQIO::Spex::print_node( label2.str() ) );

		plot << ", " << bound1 << " with lines title \"" << title1 << "\"";
		switch ( type ) {
		case Model::Result::Type::THROUGHPUT:
		    plot << ", x/(" << bounds.D_sum() << "+" << bounds.Z() << ") with lines title \"" << title2 << "\"";
		    y_max = std::max( y_max, 1./bounds.D_max() );
		    break;
		case Model::Result::Type::RESPONSE_TIME:
		    plot << ", x*" << bounds.D_max() << "-" << bounds.Z() << " with lines title \"" << title2 << "\"";
		    y_max = std::max( y_max, _x1.max*bounds.D_max()-bounds.Z() );
		    break;
		default:
		    break;
		}
	    }
	}
	std::ostringstream yrange;
	yrange << "set yrange [" << 0 << ":" << y_max * 1.10 << "]";
	_gnuplot.push_back( LQIO::Spex::print_node( yrange.str() ) );

	return plot;
    }

    /*
     * Plot the results of a population mix.  X-axis is class 1,
     * Y-axis is class 2.  I should possibly label a few points, but
     * they might have to computed by gnuplot.
     */

    std::ostream&
    JMVA_Document::plot_population_mix_vs_throughput( std::ostream& plot )
    {
	const Model::Chain::map_t::iterator x = chains().begin();
	const Model::Chain::map_t::iterator y = std::next(x);

	std::string x_var = "$" + x->first;
	std::string y_var = "$" + y->first;
	createObservation( x_var, Model::Result::Type::THROUGHPUT, x->first );
	createObservation( y_var, Model::Result::Type::THROUGHPUT, y->first );

	std::ostringstream x_cust;
	std::ostringstream y_cust;
	x_cust << *x->second.customers();
	y_cust << *y->second.customers();
	appendResultVariable( x_cust.str() );
	appendResultVariable( y_cust.str() );
	appendResultVariable( x_var );
	appendResultVariable( y_var );
	
	_gnuplot.push_back( LQIO::Spex::print_node( "set xlabel \"" + x->first + " Throughput\"" ) );	// X axis
	_gnuplot.push_back( LQIO::Spex::print_node( "set ylabel \"" + y->first + " Throughput\"" ) );	// Y1 axis
	_gnuplot.push_back( LQIO::Spex::print_node( "set key bottom left" ) );
	_gnuplot.push_back( LQIO::Spex::print_node( "set key box" ) );

	plot << "\"$DATA\" using 3:4 with linespoints title \"MVA\"";

	/* Compute bound for each station */

	double x_max = 0;
	double y_max = 0;
	for ( Model::Station::map_t::const_iterator m = stations().begin(); m != stations().end(); ++m ) {
	    if (     m->second.type() != Model::Station::Type::LOAD_INDEPENDENT
		  && m->second.type() != Model::Station::Type::MULTISERVER ) continue;

	    double D_x = Model::Bound::D( m->second, *x );		/* Adjusted for multiservers	*/
	    double D_y = Model::Bound::D( m->second, *y );
	    if ( D_x == 0. && D_y == 0. ) continue;

	    x_max = std::max( x_max, D_x );
	    y_max = std::max( y_max, D_y );
	    if ( D_y == 0. ) {
		plot << ", 1/" << D_x << ",t";
	    } else {
		plot << ", t,(1-t*" << D_x << ")/" << D_y;;
	    }
	    plot << " with lines title \"" << m->first << " Bound\"";
	}

	/* Set range (if possible), otherwise punt */
	if ( x_max > 0 && y_max > 0 ) {
	    const double x_pos = 1.0/x_max;
	    const double y_pos = 1.0/y_max;
	    _gnuplot.push_back( LQIO::Spex::print_node( "set parametric" ) );
	    _gnuplot.push_back( LQIO::Spex::print_node( "set xrange [0:" + std::to_string(x_pos*1.05) + "]" ) );
	    _gnuplot.push_back( LQIO::Spex::print_node( "set trange [0:" + std::to_string(x_pos) + "]" ) );
	    _gnuplot.push_back( LQIO::Spex::print_node( "set yrange [0:" + std::to_string(y_pos*1.05) + "]" ) );

	    std::ostringstream label_1, label_2;
	    label_1 << ")\" at " << x_pos * 0.01 << "," << y_pos << " left";
	    label_2 << ",0)\" at " << x_pos << "," << y_pos * 0.01 << " right";
	    _gnuplot.push_back( new LQX::FilePrintStatementNode( LQIO::Spex::make_list( new LQX::ConstantValueExpression("set label \"(0,"),
											new LQX::VariableExpression( _x1.var, false ),
											new LQX::ConstantValueExpression(label_1.str()), nullptr ), true, false ) );
	    _gnuplot.push_back( new LQX::FilePrintStatementNode( LQIO::Spex::make_list( new LQX::ConstantValueExpression("set label \"("),
											new LQX::VariableExpression( _x2.var, false ),
											new LQX::ConstantValueExpression(label_2.str()), nullptr ), true, false ) );
	}

	return plot;
    }


    /*
     * Plot the utilization versus the population mix.
     */
    
    std::ostream&
    JMVA_Document::plot_population_mix_vs_utilization( std::ostream& plot )
    {
	appendResultVariable( _x1.var );
	appendResultVariable( _x2.var );

	/* Find utilization for all stations */

	_gnuplot.push_back( LQIO::Spex::print_node( "set xlabel \""  + _x1.label + "\"" ) );			// X axis
//	_gnuplot.push_back( LQIO::Spex::print_node( "set x2label \"" + _x2.label + "\"" ) );			// X axis
	_gnuplot.push_back( LQIO::Spex::print_node( "set ylabel \""  + y_label_table.at(Model::Result::Type::UTILIZATION) + "\"" ) );	// Y1 axis
	_gnuplot.push_back( LQIO::Spex::print_node( "set key title \"Station\"" ) );
//	_gnuplot.push_back( LQIO::Spex::print_node( "set key top left box" ) );

	const size_t x = 1;		/* GNUPLOT starts from 1, not 0 */
	size_t y = x + 1;		/* Skip "mirror" x */
	
	for ( Model::Station::map_t::const_iterator m = stations().begin(); m != stations().end(); ++m ) {
	    if (     m->second.type() != Model::Station::Type::LOAD_INDEPENDENT
		  && m->second.type() != Model::Station::Type::MULTISERVER ) continue;

	    if ( y > 2 ) plot << ", ";

	    /* Create observation, var name is class name. */
	    y += 1;
	    const std::string y_var = "$" + m->first;
	    createObservation( y_var, Model::Result::Type::UTILIZATION, &m->second, nullptr );
	    appendResultVariable( y_var );

	    /* Append plot command to plot */
	    plot << "\"$DATA\" using " << x << ":" << y << " with linespoints" << " title \"" << m->first << "\"";
	}

	return plot;
    }
}

namespace BCMP {

    /* ---------------------------------------------------------------- */
    /* Output.								*/
    /* ---------------------------------------------------------------- */

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

	if ( !_pragmas.empty() ) {
	    const std::map<std::string,std::string>& pragmas = _pragmas.getList();
	    for ( std::map<std::string,std::string>::const_iterator next_pragma = pragmas.begin(); next_pragma != pragmas.end(); ++next_pragma ) {
		output << XML::start_element( Xpragma, false )
		       << XML::attribute( Xparam, next_pragma->first )
		       << XML::attribute( Xvalue, next_pragma->second )
		       << XML::end_element( Xpragma, false ) << std::endl;
	    }
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
	       << XML::simple_element( XalgType ) << XML::attribute( "maxSamples", 10000U ) << XML::attribute( Xname, std::string("MVA") ) << XML::attribute( "tolerance", 1.0E-7 ) << XML::end_element( XalgType, false ) << std::endl
	       << XML::simple_element( XcompareAlgs ) << XML::attribute( Xvalue, false ) << XML::end_element( XcompareAlgs, false )  << std::endl
	       << XML::end_element( XalgParams ) << std::endl;

	/* SPEX */
	/* Insert WhatIf for statements for arrays and completions. */
	/* 	<whatIf className="c1" stationName="p2" type="Service Demands" values="1.0;1.1;1.2;1.3;1.4;1.5;1.6;1.7;1.8;1.9;2.0"/> */

	if ( !Spex::input_variables().empty() ) {
	    output << "   <!-- SPEX input variables -->" << std::endl;
	    What_If what_if( output, _model );
	    std::for_each( Spex::scalar_variables().begin(), Spex::scalar_variables().end(), what_if );		/* Do scalars in order	*/
	    std::for_each( Spex::array_variables().begin(),  Spex::array_variables().end(),  what_if );		/* Do arrays in order	*/
	}

	/* SPEX */
	/* Insert a results section, but only to output the variables */
	
	if ( !Spex::result_variables().empty() ) {
	    output << "   <!-- SPEX results -->" << std::endl;
	    /* Store in a map<station,pair<string,map<class,string>>, then output by station and class. */
	    output << XML::start_element( Xsolutions ) << XML::attribute( Xok, Xfalse );
	    const std::string result_vars = std::accumulate( Spex::result_variables().begin(), Spex::result_variables().end(), std::string(""), &fold );
	    if ( !result_vars.empty() ) output << XML::attribute( XResultVariables, result_vars );
	    output << ">" << std::endl;
	    output << XML::start_element( Xalgorithm ) << XML::attribute( Xiterations, static_cast<unsigned int>(0) ) << ">" << std::endl;
	    /* Output by station, then class */
	    for ( BCMP::Model::Station::map_t::const_iterator m = stations().begin(); m != stations().end(); ++m ) {
		output << XML::start_element( Xstationresults ) << XML::attribute( Xstation, m->first ) << ">" << std::endl;
		const BCMP::Model::Station& station = m->second;
		const BCMP::Model::Result::map_t& station_variables = station.resultVariables();
		std::for_each( station_variables.begin(), station_variables.end(), printMeasure( output ) );
		for ( BCMP::Model::Station::Class::map_t::const_iterator k = station.classes().begin(); k != station.classes().end(); ++k ) {
		    output << XML::start_element( Xclassresults ) << XML::attribute( Xcustomerclass, k->first ) << ">" << std::endl;
		    const BCMP::Model::Station::Class& clasx = k->second;
		    const BCMP::Model::Result::map_t& class_variables = clasx.resultVariables();
		    std::for_each( class_variables.begin(), class_variables.end(), printMeasure( output ) );
		    output << XML::end_element( Xclassresults ) << std::endl;
		}
		output << XML::end_element( Xstationresults ) << std::endl;
	    }
	    output << XML::end_element( Xalgorithm ) << std::endl;
	    output << XML::end_element( Xsolutions ) << std::endl;
	}
	output << XML::end_element( Xmodel ) << std::endl;
	return output;
    }


    /*
     * Insert SPEX result variables.  The <measure> element is hijacked by putting the result variable
     * into the meanValue for the measure.
     */
    
    void
    JMVA_Document::printMeasure::operator()( const Model::Result::pair_t& r ) const
    {
	static const std::map<const BCMP::Model::Result::Type,const char * const> attribute = {
	    { BCMP::Model::Result::Type::QUEUE_LENGTH, XNumberOfCustomers },
	    { BCMP::Model::Result::Type::RESIDENCE_TIME, XResidenceTime },
	    { BCMP::Model::Result::Type::THROUGHPUT, XThroughput },
	    { BCMP::Model::Result::Type::UTILIZATION, XUtilization }
	};
	_output << XML::simple_element( Xmeasure )
		<< XML::attribute( XmeasureType, attribute.at(r.first) )
		<< XML::attribute( XmeanValue, r.second )
		<< XML::end_element( Xmeasure, false ) << std::endl;
    }

    /*
     * Print out the special "reference" station.
     */

    void
    JMVA_Document::printStation::operator()( const Model::Station::pair_t& m ) const
    {
	const BCMP::Model::Station& station = m.second;
	static const std::map<Model::Station::Type,const char * const> type = {
	    { Model::Station::Type::DELAY, Xdelaystation },
	    { Model::Station::Type::MULTISERVER, Xldstation },
	    { Model::Station::Type::LOAD_INDEPENDENT, Xlistation }
	};
	const char * const element = type.at(station.type());

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
	if ( k.second.isClosed() ) {
	    _output << XML::simple_element( Xclosedclass )
		    << XML::attribute( Xname, k.first )
		    << XML::attribute( Xpopulation, *k.second.customers() )
		    << XML::end_element( Xclosedclass, false ) << std::endl;
	} else if ( k.second.isOpen() ) {
	    _output << XML::simple_element( Xopenclass )
		    << XML::attribute( Xname, k.first )
		    << XML::attribute( Xrate, *k.second.arrival_rate() )
		    << XML::end_element( Xopenclass ) << std::endl;
	} else {
	    throw std::range_error( "JMVA_Document::printClass::operator(): Undefined class." );
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
		    << XML::end_element( XClass, false ) << std::endl;
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
    JMVA_Document::What_If::operator()( const std::string& var ) const
    {
	const std::map<std::string,LQX::SyntaxTreeNode *>::const_iterator y = Spex::input_variables().find(var);
	this->operator()( *y );
    }

    void
    JMVA_Document::What_If::operator()( const std::pair<std::string,LQX::SyntaxTreeNode *>& var ) const
    {
	/* Find the variable */
	BCMP::Model::Station::map_t::const_iterator m;
	BCMP::Model::Chain::map_t::const_iterator k;
	if ( (k = std::find_if( chains().begin(), chains().end(), What_If::has_customers( var.first ) )) != chains().end() ) {
	    _output << XML::simple_element( XwhatIf )
		    << XML::attribute( XclassName, k->first )
		    << XML::attribute( Xtype, XCustomer_Numbers );
	} else if ( (k = std::find_if( chains().begin(), chains().end(), What_If::has_customers( var.first ) )) != chains().end() ) {
	    _output << XML::simple_element( XwhatIf )
		    << XML::attribute( XclassName, k->first )
		    << XML::attribute( Xtype, XArrival_Rates );
	} else if ( (m = std::find_if( stations().begin(), stations().end(), What_If::has_copies( var.first ) )) != stations().end() ) {
	    _output << XML::simple_element( XwhatIf )
		    << XML::attribute( XstationName, m->first )
		    << XML::attribute( Xtype, XNumber_of_Servers );
	} else if ( (m = std::find_if( stations().begin(), stations().end(), What_If::has_var( var.first ) )) != stations().end() ) {
	    const BCMP::Model::Station::Class::map_t& classes = m->second.classes();
	    BCMP::Model::Station::Class::map_t::const_iterator d;
	    _output << XML::simple_element( XwhatIf );
	    if ( (d = std::find_if( classes.begin(), classes.end(), What_If::has_service_time( var.first ) )) != classes.end() ) {
		_output << XML::attribute( XstationName, m->first )
			<< XML::attribute( XclassName, d->first )
			<< XML::attribute( Xtype, XService_Demands );
	    } else if ( (d = std::find_if( classes.begin(), classes.end(), What_If::has_visits( var.first ) )) != classes.end() ) {
		_output << XML::attribute( XstationName, m->first )
			<< XML::attribute( XclassName, d->first )
			<< XML::attribute( Xtype, "Visits" );
	    } else {
		abort();
	    }
	} else {
	    /* Var not found! */
	    _output << "<!-- Var not found: " << var.first << " -->" << std::endl;
	    return;
	}

	/* Print it out */
	
	std::ostringstream ss;
	std::string values;
	std::map<std::string,Spex::ComprehensionInfo>::const_iterator comprehension;
	std::vector<std::string>::const_iterator array;
	if ( (comprehension = Spex::comprehensions().find( var.first )) != Spex::comprehensions().end() ) {
	    /* Simple, run the comprehension directly */
	    for ( double value = comprehension->second.getInit(); value <= comprehension->second.getTest(); value += comprehension->second.getStep() ) {
		if ( value != comprehension->second.getInit() ) ss << ";";
		ss << value;
	    }
	    values = ss.str();
	} else if ( (array = std::find( Spex::array_variables().begin(), Spex::array_variables().end(), var.first )) != Spex::array_variables().end() ) {
	    /* Little harder, the values are encoded in the varible's LQX statement directly */
	    var.second->print(ss);		/* So print out the LQX */
	    values = ss.str();
	    /* Get rid of brackets and spaces from array_create(0.5, 1, 1.5) */
	    values.erase(values.begin(), values.begin()+13);		/* "array_create(" */
	    values.erase(std::prev(values.end()),values.end());		/* ")" */
	    std::replace(values.begin(), values.end(), ',', ';' );	/* Xerces wants ';', not ',' */
	    values.erase(std::remove(values.begin(), values.end(), ' '), values.end());	/* Strip blanks */
	} else {
	    var.second->print(ss);		/* So print out the LQX */
	    values = ss.str();
	}
	_output << XML::attribute( Xvalues, values );
	_output << "/>  <!--" << var.first << "-->" << std::endl;
    }

    /* Return true if this class has the variable */

    bool
    JMVA_Document::What_If::has_customers::operator()( const Model::Chain::pair_t& k ) const
    {
	if ( !k.second.isClosed() ) return false;
	const LQIO::DOM::ExternalVariable * var = k.second.customers();
	return var != nullptr && !var->wasSet() && var->getName() == _var;
    }

    bool
    JMVA_Document::What_If::has_arrival_rate::operator()( const Model::Chain::pair_t& k ) const
    {
	if ( !k.second.isOpen() ) return false;
	const LQIO::DOM::ExternalVariable * var = k.second.arrival_rate();
	return var != nullptr && !var->wasSet() && var->getName() == _var;
    }

    bool
    JMVA_Document::What_If::has_copies::operator()( const Model::Station::pair_t& m ) const
    {
	const LQIO::DOM::ExternalVariable * var = m.second.copies();
	return var != nullptr && !var->wasSet() && var->getName() == _var;
    }

    /* Search for the variable in all classes */

    bool
    JMVA_Document::What_If::has_var::operator()( const Model::Station::pair_t& m ) const
    {
	const BCMP::Model::Station::Class::map_t& classes = m.second.classes();
	return std::any_of( classes.begin(), classes.end(), What_If::has_service_time( _var ) )
	    || std::any_of( classes.begin(), classes.end(), What_If::has_visits( _var ) );
    }

    bool
    JMVA_Document::What_If::has_service_time::operator()( const Model::Station::Class::pair_t& d ) const
    {
	const LQIO::DOM::ExternalVariable * var = d.second.service_time();
	return var != nullptr && !var->wasSet() && var->getName() == _var;
    }

    bool
    JMVA_Document::What_If::has_visits::operator()( const Model::Station::Class::pair_t& d ) const
    {
	const LQIO::DOM::ExternalVariable * var = d.second.visits();
	return var != nullptr && !var->wasSet() && var->getName() == _var;
    }

    std::string
    JMVA_Document::fold( const std::string& s1, const Spex::var_name_and_expr& v2 )
    {
	if ( !s1.empty() ) {
	    return s1 + ";" + v2.first;
	} else {
	    return v2.first;
	}
    }
}

namespace BCMP {
    using namespace LQIO;

    bool JMVA_Document::convertToLQN( DOM::Document& document ) const
    {
	if ( !_model.convertToLQN( document ) ) return false;
	LQIO::Spex::__result_variables.clear();		/* Get rid of them all. */
	/* Add SPEX */
	return true;
    }
}


namespace BCMP {
    /* Tables for input parsing */
    const std::set<const XML_Char *,JMVA_Document::attribute_table_t> JMVA_Document::algParams_table = { XmaxSamples, Xname, Xtolerance };
    const std::set<const XML_Char *,JMVA_Document::attribute_table_t> JMVA_Document::compareAlgs_table = { XmeanValue, XmeasureType, Xsuccessful };
    const std::set<const XML_Char *,JMVA_Document::attribute_table_t> JMVA_Document::null_table = {};

    /* Table for y label when plotting */
    const std::map<const Model::Result::Type, const std::string> JMVA_Document::y_label_table = {
	{Model::Result::Type::QUEUE_LENGTH,   JMVA_Document::XNumber_of_Customers },
	{Model::Result::Type::RESIDENCE_TIME, JMVA_Document::XResidence_Time },
	{Model::Result::Type::THROUGHPUT,     JMVA_Document::XThroughput },
	{Model::Result::Type::UTILIZATION,    JMVA_Document::XUtilization }
    };

    /* Schema element/attribute names */
    const XML_Char * JMVA_Document::XArrivalProcess	= "Arrival Process";
    const XML_Char * JMVA_Document::XClass		= "Class";
    const XML_Char * JMVA_Document::XReferenceStation	= "ReferenceStation";
    const XML_Char * JMVA_Document::XalgParams		= "algParams";
    const XML_Char * JMVA_Document::XalgType		= "algType";
    const XML_Char * JMVA_Document::Xclass		= "class";
    const XML_Char * JMVA_Document::Xclasses		= "classes";
    const XML_Char * JMVA_Document::Xclosedclass	= "closedclass";
    const XML_Char * JMVA_Document::XcompareAlgs	= "compareAlgs";
    const XML_Char * JMVA_Document::Xcustomerclass	= "customerclass";
    const XML_Char * JMVA_Document::Xdelaystation	= "delaystation";
    const XML_Char * JMVA_Document::Xdescription	= "description";
    const XML_Char * JMVA_Document::Xjaba		= "jaba";
    const XML_Char * JMVA_Document::Xlistation		= "listation";
    const XML_Char * JMVA_Document::XLQX		= "lqx";
    const XML_Char * JMVA_Document::Xldstation		= "ldstation";
    const XML_Char * JMVA_Document::XmaxSamples		= "maxSamples";
    const XML_Char * JMVA_Document::Xmodel		= "model";
    const XML_Char * JMVA_Document::Xmultiplicity	= "multiplicity";
    const XML_Char * JMVA_Document::Xname		= "name";
    const XML_Char * JMVA_Document::Xnumber		= "number";
    const XML_Char * JMVA_Document::Xopenclass		= "openclass";
    const XML_Char * JMVA_Document::Xparam		= "param";
    const XML_Char * JMVA_Document::Xparameters		= "parameters";
    const XML_Char * JMVA_Document::Xpopulation		= "population";
    const XML_Char * JMVA_Document::Xpragma		= "pragma";
    const XML_Char * JMVA_Document::Xrate		= "rate";
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
    const XML_Char * JMVA_Document::Xiteration		= "iteration";
    const XML_Char * JMVA_Document::Xiterations		= "iterations";
    const XML_Char * JMVA_Document::XiterationValue	= "iterationValue";
    const XML_Char * JMVA_Document::Xok			= "ok";
    const XML_Char * JMVA_Document::Xfalse		= "false";
    const XML_Char * JMVA_Document::XsolutionMethod   	= "solutionMethod";

    const XML_Char * JMVA_Document::XArrival_Rates	= "Arrival Rates";
    const XML_Char * JMVA_Document::XCustomer_Numbers 	= "Customer Numbers";
    const XML_Char * JMVA_Document::XNumberOfCustomers	= "NumberOfCustomers";
    const XML_Char * JMVA_Document::XNumber_of_Customers= "Number of Customers";
    const XML_Char * JMVA_Document::XNumber_of_Servers	= "Number of Servers";
    const XML_Char * JMVA_Document::XPopulation_Mix     = "Population Mix";
    const XML_Char * JMVA_Document::XResidenceTime      = "ResidenceTime";
    const XML_Char * JMVA_Document::XResidence_Time     = "Residence time";
    const XML_Char * JMVA_Document::XResponse_Time      = "Response Time";
    const XML_Char * JMVA_Document::XResultVariables	= "ResultVariables";
    const XML_Char * JMVA_Document::XService_Demands	= "Service Demands";
    const XML_Char * JMVA_Document::XThroughput         = "Throughput";
    const XML_Char * JMVA_Document::XUtilization        = "Utilization";
    const XML_Char * JMVA_Document::Xnormconst          = "normconst";
}
