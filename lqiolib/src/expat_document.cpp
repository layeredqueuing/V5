/* -*- c++ -*-
 * $Id: expat_document.cpp 13729 2020-08-04 20:20:16Z greg $
 *
 * Read in XML input files.
 *
 * Copyright the Real-Time and Distributed Systems Group,
 * Department of Systems and Computer Engineering,
 * Carleton University, Ottawa, Ontario, Canada. K1S 5B6
 *
 * ------------------------------------------------------------------------
 * December 2003.
 * May 2010.
 * ------------------------------------------------------------------------
 */

#if HAVE_CONFIG_H
#include <config.h>
#endif

#include "expat_document.h"
#include <fstream>
#include <iomanip>
#include <algorithm>
#include <string>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <vector>
#include <cassert>
#include <errno.h>
#include <cmath>
#include <fcntl.h>
#include <sys/stat.h>
#if HAVE_SYS_MMAN_H
#include <sys/types.h>
#include <sys/mman.h>
#endif
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#if HAVE_STRINGS_H
#include <strings.h>
#endif

#include <lqx/SyntaxTree.h>
#include "dom_object.h"
#include "dom_histogram.h"
#include "dom_task.h"
#include "dom_entry.h"
#include "dom_phase.h"
#include "dom_call.h"
#include "dom_activity.h"
#include "dom_actlist.h"
#include "srvn_results.h"
#include "input.h"
#include "error.h"
#include "glblerr.h"
#include "filename.h"
#include "dom_phase.h"
#include "dom_task.h"
#include "dom_actlist.h"
#include "dom_histogram.h"
#include "srvn_spex.h"

extern "C" {
#include "srvn_gram.h"
    int LQIO_parse_string( int start_token, const char * s );
}

namespace LQIO {
    namespace DOM {

        int Expat_Document::__indent = 0;

        /* ---------------------------------------------------------------- */
        /* DOM input.                                                       */
        /* ---------------------------------------------------------------- */

        Expat_Document::Expat_Document( Document& document, const std::string& input_file_name, bool createObjects, bool loadResults )
            : _document( document ), _parser(), _input_file_name(input_file_name), _createObjects(createObjects), _loadResults(loadResults), _stack(),
	      _has_spex(false), _spex_parameters(), _spex_results(), _spex_convergence(), _spex_observation()
        {
	    init_tables();
        }


        Expat_Document::~Expat_Document()
        {
        }

	bool
        Expat_Document::load( Document& document, const std::string& input_filename, unsigned& errorCode, const bool load_results )
        {
	    Expat_Document input( document, input_filename, true, load_results );

	    if ( !input.parse() ) {
		return false;
	    } else {
		const std::string& program_text = document.getLQXProgramText();
		if ( program_text.size() ) {
		    LQX::Program* program;
		    /* If we have an LQX program, then we need to compute */
		    if ( Spex::__parameter_list != NULL ) {
			LQIO::solution_error( LQIO::ERR_LQX_SPEX, input_filename.c_str() );
			return false;
		    } else if ( (program = LQX::Program::loadFromText(input_filename.c_str(), document.getLQXProgramLineNumber(), program_text.c_str())) == NULL ) {
			LQIO::solution_error( LQIO::ERR_LQX_COMPILATION, input_filename.c_str() );
			return false;
		    } else {
			document.setLQXProgram( program );
		    }
		} else {
		    /*
		     * Parameters, Results and Convergence call the SRVN parser.  For XML, these variables are set
		     * so we need to set up the program after the model is loaded.
		     */

		    spex_set_program( Spex::__parameter_list, Spex::__result_list, Spex::__convergence_list );
		}
		return true;
            }
        }

        /*
         * Load results (only) from filename.  The input DOM must be present (and match iff LQNX input).
         */

        /* static */ bool
        Expat_Document::loadResults( Document& document, const std::string& input_file_name, unsigned& errorCode )
        {
	    Expat_Document input( document, input_file_name, false, true );
	    return input.parse();
        }


        /*
         * Parse the XML file, catching any XML exceptions that might
         * propogate out of it.
         */

        bool
        Expat_Document::parse()
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

            _stack.push( parse_stack_t("",&Expat_Document::startModel,0) );

#if HAVE_MMAP
	    char *buffer;
#endif
	    try {
#if HAVE_MMAP
		buffer = static_cast<char *>(mmap( 0, statbuf.st_size, PROT_READ, MAP_PRIVATE|MAP_FILE, input_fd, 0 ));
		if ( buffer != MAP_FAILED ) {
		    if ( !XML_Parse( _parser, buffer, statbuf.st_size, true ) ) {
			const char * error = XML_ErrorString(XML_GetErrorCode(_parser));
			input_error( error );
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
			    input_error( error );
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
		input_error( "Unexpected element <%s> ", e.what() );
		rc = false;
	    }
	    catch ( const LQIO::missing_attribute & e ) {
		rc = false;
	    }
	    catch ( const LQIO::unexpected_attribute & e ) {
		rc = false;
	    }
	    catch ( const std::invalid_argument & e ) {
		rc = false;
	    }
	    catch ( const LQIO::undefined_symbol & e ) {
		rc = false;
	    }
	    catch ( const std::domain_error & e ) {
		input_error( "Domain error: %s ", e.what() );
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

        void
        Expat_Document::input_error( const char * fmt, ... ) const
        {
            va_list args;
            va_start( args, fmt );
            verrprintf( stderr, RUNTIME_ERROR, _input_file_name.c_str(),  XML_GetCurrentLineNumber(_parser), 0, fmt, args );
            va_end( args );
        }

        /*
         * Handlers called from Expat.
         */

        void
        Expat_Document::start( void *data, const XML_Char *el, const XML_Char **attr )
        {
            Expat_Document * document = static_cast<Expat_Document *>(data);
	    LQIO_lineno = XML_GetCurrentLineNumber(document->_parser);
            if ( Document::__debugXML ) {
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
		    const parse_stack_t& top = document->_stack.top();
		    (document->*top.start)(top.object,el,attr);
		}
            }
	    catch ( const duplicate_symbol& e ) {
		LQIO::input_error2( LQIO::ERR_DUPLICATE_SYMBOL, el, e.what() );
	    }
            catch ( const LQIO::missing_attribute & e ) {
		LQIO::input_error2( LQIO::ERR_MISSING_ATTRIBUTE, el, e.what() );
            }
            catch ( const LQIO::unexpected_attribute & e ) {
		LQIO::input_error2( LQIO::ERR_UNEXPECTED_ATTRIBUTE, el, e.what() );
            }
            catch ( const std::invalid_argument & e ) {
		LQIO::input_error2( LQIO::ERR_INVALID_ARGUMENT, el, e.what() );
            }
            catch ( const LQIO::undefined_symbol & e ) {
		LQIO::input_error2( LQIO::ERR_NOT_DEFINED, e.what() );
            }
        }

        /*
         * Pop elements off the stack until we hit a matching tag.
         */

        void
        Expat_Document::end( void *data, const XML_Char *el )
        {
            Expat_Document * document = static_cast<Expat_Document *>(data);
            DocumentObject * extra_object = 0;
            bool done = false;
            while ( document->_stack.size() > 0 && !done ) {
                parse_stack_t& top = document->_stack.top();
                if ( top.extra_object ) {
                    extra_object = top.extra_object;
                }
                if ( Document::__debugXML ) {
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
            if ( document->_stack.size() > 0 && extra_object ) {
                document->_stack.top().extra_object = extra_object;             // needed for act_connect.
            }
            return;
        }

        void
        Expat_Document::start_cdata( void * data )
        {
        }


        void
        Expat_Document::end_cdata( void * data )
        {
        }


        /*
         * Ignore most text.  However, for an LQX program, concatenate
         * the text.  Since expat gives us text in "chunks", we can't
         * just simply call setLQXProgram.  Rather, we "append" the
         * program to the existing one.
         */

        void
        Expat_Document::handle_text( void * data, const XML_Char * text, int length )
        {
            Expat_Document * parser = static_cast<Expat_Document *>(data);
            if ( parser->_stack.size() == 0 ) return;
            const parse_stack_t& top = parser->_stack.top();
            if ( top.start == &Expat_Document::startLQX ) {
		std::string& program = const_cast<std::string &>(parser->_document.getLQXProgramText());
                program.append( text, length );
            } else if ( top.start == &Expat_Document::startSPEXParameters ) {
		parser->_spex_parameters.append( text, length );
	    } else if ( top.start == &Expat_Document::startSPEXResults ) {
		parser->_spex_results.append( text, length );
	    } else if ( top.start == &Expat_Document::startSPEXConvergence ) {
		parser->_spex_convergence.append( text, length );
	    }
        }

	/*
	 * We tack the comment onto the current element.
	 */

	void
	Expat_Document::handle_comment( void * data, const XML_Char * text )
	{
            Expat_Document * parser = static_cast<Expat_Document *>(data);
            if ( parser->_stack.size() == 0 ) return;
            const parse_stack_t& top = parser->_stack.top();
	    DocumentObject * object = top.object;
	    if ( object ) {
		std::string& comment = const_cast<std::string&>(object->getComment());
		if ( comment.size() ) {
		    comment += "\n";
		}
		comment += text;
	    }
	}

        int
        Expat_Document::handle_encoding( void * data, const XML_Char *name, XML_Encoding *info )
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

        /* ---------------------------------------------------------------- */
        /* Parser functions.                                                */
        /* ---------------------------------------------------------------- */

        void
        Expat_Document::startModel( DocumentObject * object, const XML_Char * element, const XML_Char ** attributes )
        {
	    checkAttributes( element, attributes, model_table );

            if ( strcasecmp( element, Xlqn_model ) == 0 ) {
                Document::__debugXML = (Document::__debugXML || getBoolAttribute(attributes,Xxml_debug));
                _stack.push( parse_stack_t(element,&Expat_Document::startModelType,0) );
            } else {
                throw element_error( element );
            }
        }

        /*
          <xsd:complexType name="LqnModelType">
          <xsd:sequence>
          <xsd:element name="run-control" minOccurs="0">
          <xsd:element name="plot-control" minOccurs="0">
          <xsd:element name="solver-parameters" minOccurs="1" maxOccurs="1">
          <xsd:element name="processor" type="ProcessorType" maxOccurs="unbounded"/>
          <xsd:element name="slot" type="SlotType" minOccurs="0" maxOccurs="unbounded"/>
          <xsd:element name="lqx" type="xsd:string" minOccurs="0" maxOccurs="1"/>
          </xsd:sequence>
        */

        void
        Expat_Document::startModelType( DocumentObject * object, const XML_Char * element, const XML_Char ** attributes )
        {
            if ( strcasecmp( element, Xsolver_parameters) == 0 ) {
                handleModel( object, attributes );
                _stack.push( parse_stack_t(element,&Expat_Document::startResultGeneral,0) );

            } else if ( strcasecmp( element, Xprocessor) == 0 ) {
                Processor * processor = handleProcessor( object, attributes );
                _stack.push( parse_stack_t(element,&Expat_Document::startProcessorType,processor) );

            } else if ( strcasecmp( element, Xlqx ) == 0 ) {
                _document.setLQXProgramLineNumber(XML_GetCurrentLineNumber(_parser));
                _stack.push( parse_stack_t(element,&Expat_Document::startLQX,object) );

            } else if ( strcasecmp( element, Xspex_parameters ) == 0 ) {
                _document.setLQXProgramLineNumber(XML_GetCurrentLineNumber(_parser));
                _stack.push( parse_stack_t(element,&Expat_Document::startSPEXParameters,&Expat_Document::endSPEXParameters,object) );

            } else if ( strcasecmp( element, Xspex_results ) == 0 ) {
                _document.setLQXProgramLineNumber(XML_GetCurrentLineNumber(_parser));
                _stack.push( parse_stack_t(element,&Expat_Document::startSPEXResults,&Expat_Document::endSPEXResults,object) );

            } else {
                throw element_error( element );
            }
        }

        /*
          <xsd:element name="result-general" minOccurs="0" maxOccurs="1">
          <xsd:complexType>
          <xsd:sequence>
          <xsd:element name="mva-info" minOccurs="0" maxOccurs="1">
          </xsd:sequence>
          </xsd:complexType>
          </xsd:element>
        */

        void
        Expat_Document::startResultGeneral( DocumentObject * object, const XML_Char * element, const XML_Char ** attributes )
        {
            if ( strcasecmp( element, Xresult_general ) == 0 ) {
		_document.setInstantiated( true );		/* Set true even if we aren't loading results */
                if ( _loadResults ) {
                    const long iterations = getLongAttribute(attributes,Xiterations);
		    _document.setResultSolverInformation( getStringAttribute(attributes,Xsolver_info,"") );
                    _document.setResultValid( getBoolAttribute(attributes,Xvalid) );
                    _document.setResultConvergenceValue( getDoubleAttribute(attributes,Xconv_val_result) );
                    _document.setResultIterations( iterations );
                    _document.setResultElapsedTime( getTimeAttribute(attributes, Xelapsed_time) );
                    _document.setResultSysTime( getTimeAttribute(attributes,Xsystem_cpu_time) );
                    _document.setResultUserTime( getTimeAttribute(attributes,Xuser_cpu_time) );
		    _document.setResultMaxRSS( getLongAttribute(attributes,Xmax_rss,0) );
                    _document.setResultPlatformInformation( getStringAttribute(attributes,Xplatform_info) );
                    if ( 1 < iterations && iterations <= 30 ) {
                        const_cast<ConfidenceIntervals *>(&_conf_95)->set_blocks( iterations );
                    }
                }
                _stack.push( parse_stack_t(element,&Expat_Document::startMVAInfo,object) );

            } else if ( strcasecmp( element, Xpragma ) == 0 ) {
                const XML_Char * parameter = getStringAttribute(attributes,Xparam);
                _document.addPragma(parameter,getStringAttribute(attributes,Xvalue,""));
                _stack.push( parse_stack_t(element,&Expat_Document::startNOP,0) );

	    } else if ( strcasecmp( element, Xresult_observation ) == 0 ) {
		handleSPEXObservation( object, attributes );
                _stack.push( parse_stack_t(element,&Expat_Document::startNOP,&Expat_Document::endSPEXObservationType,object) );
		
            } else {
                throw element_error( element );
            }
        }


        void
        Expat_Document::startMVAInfo( DocumentObject * object, const XML_Char * element, const XML_Char ** attributes )
        {
            if ( strcasecmp( element, Xmva_info ) == 0 ) {
                if ( _loadResults ) {
                    _document.setMVAStatistics( getLongAttribute(attributes,Xsubmodels),
						getLongAttribute(attributes,Xcore),
						getDoubleAttribute(attributes,Xstep),
						getDoubleAttribute(attributes,Xstep_squared),
						getDoubleAttribute(attributes,Xwait),
						getDoubleAttribute(attributes,Xwait_squared),
						getLongAttribute(attributes,Xfaults) );
                }
                _stack.push( parse_stack_t(element,&Expat_Document::startNOP,0) );
/* !!! SPEX document observations */
            } else {
                throw element_error( element );
            }
        }

        /*
          <xsd:complexType name="ProcessorType">
          <xsd:annotation>
          <xsd:documentation>Processors run tasks.</xsd:documentation>
          </xsd:annotation>
          <xsd:sequence>
          <xsd:element name="result-processor" type="OutputResultType" minOccurs="0" maxOccurs="unbounded"/>
          <xsd:choice>
          <xsd:element name="group" type="GroupType" maxOccurs="unbounded"/>
          <xsd:element name="task" type="TaskType" maxOccurs="unbounded"/>
          </xsd:choice>
          </xsd:sequence>
          </xsd:complexType>
        */

        void
        Expat_Document::startProcessorType( DocumentObject * processor, const XML_Char * element, const XML_Char ** attributes )
        {
            if ( strcasecmp( element, Xresult_processor) == 0 ) {
		try {
		    handleResults( processor, attributes );
		}
		catch ( const unexpected_attribute& e ) {
		    LQIO::input_error2( LQIO::ERR_UNEXPECTED_ATTRIBUTE, element, *attributes );
		}
                _stack.push( parse_stack_t(element,&Expat_Document::startOutputResultType,processor) );

	    } else if ( strcasecmp( element, Xresult_observation ) == 0 ) {
		try {
		    handleSPEXObservation( processor, attributes );
		}
		catch ( const unexpected_attribute& e ) {
		    LQIO::input_error2( LQIO::ERR_UNEXPECTED_ATTRIBUTE, element, *attributes );
		}
                _stack.push( parse_stack_t(element,&Expat_Document::startSPEXObservationType,&Expat_Document::endSPEXObservationType,processor) );

            } else if ( strcasecmp( element, Xtask) == 0 ) {
                Task * task = handleTask( processor, attributes );
                _stack.push( parse_stack_t(element,&Expat_Document::startTaskType,task) );

            } else if ( strcasecmp( element, Xgroup ) == 0 ) {
                DocumentObject * group = handleGroup( processor, attributes );
                _stack.push( parse_stack_t(element,&Expat_Document::startGroupType,group) );

            } else {
                throw element_error( element );
            }
        }

        /*
          <xsd:complexType name="GroupType">
          <xsd:sequence>
          <xsd:element name="result-group" type="OutputResultType" minOccurs="0" maxOccurs="unbounded"/>
          <xsd:element name="task" type="TaskType" maxOccurs="unbounded"/>
          </xsd:sequence>
          </xsd:complexType>
        */

        void
        Expat_Document::startGroupType( DocumentObject * group, const XML_Char * element, const XML_Char ** attributes )
        {
            if ( strcasecmp( element, Xresult_group ) == 0 ) {
		try {
		    handleResults( group, attributes );
		}
		catch ( const unexpected_attribute& e ) {
		    LQIO::input_error2( LQIO::ERR_UNEXPECTED_ATTRIBUTE, element, *attributes );
		}
                _stack.push( parse_stack_t(element,&Expat_Document::startOutputResultType,group) );

	    } else if ( strcasecmp( element, Xresult_observation ) == 0 ) {
		try {
		    handleSPEXObservation( group, attributes );
		}
		catch ( const unexpected_attribute& e ) {
		    LQIO::input_error2( LQIO::ERR_UNEXPECTED_ATTRIBUTE, element, *attributes );
		}
                _stack.push( parse_stack_t(element,&Expat_Document::startSPEXObservationType,&Expat_Document::endSPEXObservationType,group) );

            } else if ( strcasecmp( element, Xtask ) == 0 ) {
                Task * task = handleTask( group, attributes );
                _stack.push( parse_stack_t(element,&Expat_Document::startTaskType,task) );

            } else {
                throw element_error( element );
            }
        }

        /*
          <xsd:complexType name="TaskType">
          <xsd:sequence>
          <xsd:element name="result-task" type="OutputResultType" minOccurs="0" maxOccurs="unbounded"/>
          <xsd:element name="service-time-distribution" type="OutputEntryDistributionType" minOccurs="0" maxOccurs="unbounded"/>
          <xsd:element name="decision" type="DecisionType" minOccurs="0" maxOccurs="unbounded"/>
          <xsd:element name="entry" type="EntryType" maxOccurs="unbounded"/>
          <xsd:element name="service" type="ServiceType" minOccurs="0" maxOccurs="unbounded"/>
          <xsd:element name="task-activities" type="TaskActivityGraph" minOccurs="0"/>
          </xsd:sequence>
          </xsd:complexType>
        */

        void
        Expat_Document::startTaskType( DocumentObject * task, const XML_Char * element, const XML_Char ** attributes )
        {
            if ( strcasecmp( element, Xresult_task) == 0 ) {
		try {
		    handleResults( task, attributes );
		}
		catch ( const unexpected_attribute& e ) {
		    LQIO::input_error2( LQIO::ERR_UNEXPECTED_ATTRIBUTE, element, *attributes );
		}
                _stack.push( parse_stack_t(element,&Expat_Document::startOutputResultType,task) );

	    } else if ( strcasecmp( element, Xresult_observation ) == 0 ) {
		try {
		    handleSPEXObservation( task, attributes );
		}
		catch ( const unexpected_attribute& e ) {
		    LQIO::input_error2( LQIO::ERR_UNEXPECTED_ATTRIBUTE, element, *attributes );
		}
                _stack.push( parse_stack_t(element,&Expat_Document::startSPEXObservationType,&Expat_Document::endSPEXObservationType,task) );

            } else if ( strcasecmp( element, Xfanin ) == 0 ) {
                handleFanIn( task, attributes );
                _stack.push( parse_stack_t(element,&Expat_Document::startNOP,0) );

            } else if ( strcasecmp( element, Xfanout ) == 0 ) {
                handleFanOut( task, attributes );
                _stack.push( parse_stack_t(element,&Expat_Document::startNOP,0) );

            } else if ( strcasecmp( element, Xentry) == 0 ) {
                Entry * entry = handleEntry( task, attributes );
                _stack.push( parse_stack_t(element,&Expat_Document::startEntryType,entry) );

            } else if ( strcasecmp( element, Xservice_time_distribution ) == 0 ) {
                Histogram * histogram = handleHistogram( task, attributes );
                _stack.push( parse_stack_t(element,&Expat_Document::startOutputDistributionType,histogram) );

            } else if ( strcasecmp( element, Xservice ) == 0 ) {
                _stack.push( parse_stack_t(element,&Expat_Document::startNOP,0) );              // Not implemented.

            } else if ( strcasecmp( element, Xtask_activities ) == 0 ) {
                _stack.push( parse_stack_t(element,&Expat_Document::startTaskActivityGraph,task) );

            } else {
                throw element_error( element );
            }
        }


        /*
          <xsd:complexType name="EntryType">
          <xsd:sequence>
          <xsd:element name="result-entry" type="OutputResultType" minOccurs="0" maxOccurs="unbounded"/>
          <xsd:element name="service-time-distribution" type="OutputEntryDistributionType" minOccurs="0" maxOccurs="unbounded"/>
          <xsd:element name="forwarding" type="EntryMakingCallType" minOccurs="0" maxOccurs="unbounded"/>
          <xsd:choice>
          <xsd:element name="entry-activity-graph" type="EntryActivityGraph" minOccurs="0"/>
          <xsd:element name="entry-phase-activities" type="PhaseActivities" minOccurs="0">
          <xsd:unique name="UniquePhaseNumber">
          </xsd:choice>
          </xsd:sequence>
        */

        void
        Expat_Document::startEntryType( DocumentObject * entry, const XML_Char * element, const XML_Char ** attributes )
        {
            if ( strcasecmp( element, Xresult_entry) == 0 ) {
		try {
		    handleResults( entry, attributes );
		}
		catch ( const unexpected_attribute& e ) {
		    LQIO::input_error2( LQIO::ERR_UNEXPECTED_ATTRIBUTE, element, *attributes );
		}
                _stack.push( parse_stack_t(element,&Expat_Document::startOutputResultType,entry) );

	    } else if ( strcasecmp( element, Xresult_observation ) == 0 ) {
		try {
		    handleSPEXObservation( entry, attributes );
		}
		catch ( const unexpected_attribute& e ) {
		    LQIO::input_error2( LQIO::ERR_UNEXPECTED_ATTRIBUTE, element, *attributes );
		}
                _stack.push( parse_stack_t(element,&Expat_Document::startSPEXObservationType,&Expat_Document::endSPEXObservationType,entry) );

            } else if ( strcasecmp( element, Xservice_time_distribution ) == 0 ) {
                Histogram * histogram = handleHistogram( entry, attributes );
                _stack.push( parse_stack_t(element,&Expat_Document::startOutputDistributionType,histogram) );

            } else if ( strcasecmp( element, Xforwarding ) == 0 ) {
		Call * call = handleEntryCall( entry, attributes );
                _stack.push( parse_stack_t(element,&Expat_Document::startEntryMakingCallType,call) );

            } else if ( strcasecmp( element, Xentry_phase_activities ) == 0 ) {
                _stack.push( parse_stack_t(element,&Expat_Document::startPhaseActivities,entry) );

            } else {
                throw element_error( element );
            }
        }


        /*
          <xsd:complexType name="PhaseActivities">
          <xsd:sequence>
          <xsd:element name="activity" type="ActivityDefBase" maxOccurs="3"/>
          </xsd:sequence>
          </xsd:complexType>
        */

        void
        Expat_Document::startPhaseActivities( DocumentObject * entry, const XML_Char * element, const XML_Char ** attributes )
        {
            if ( strcasecmp( element, Xactivity) == 0 ) {
                Phase * phase = handlePhaseActivity( entry, attributes );
                _stack.push( parse_stack_t(element,&Expat_Document::startActivityDefBase,phase) );

            } else {
                throw element_error( element );

            }
        }


        /*
          <xsd:complexType name="ActivityDefBase">
          <xsd:sequence>
          <xsd:element name="service-time-distribution" type="OutputDistributionType" minOccurs="0" maxOccurs="unbounded"/>
          <xsd:element name="result-activity" type="OutputResultType" minOccurs="0" maxOccurs="unbounded"/>
          <xsd:choice>
          <xsd:group ref="Call-List-Group"/>
          <xsd:group ref="Activity-CallGroup" maxOccurs="unbounded"/>
          </xsd:choice>
          </xsd:sequence>
        */

        void
        Expat_Document::startActivityDefBase( DocumentObject * activity, const XML_Char * element, const XML_Char ** attributes )
        {
            Call * call = 0;
            if ( strcasecmp( element, Xresult_activity ) == 0 ) {
		try {
		    handleResults( activity, attributes );
		}
		catch ( const unexpected_attribute& e ) {
		    LQIO::input_error2( LQIO::ERR_UNEXPECTED_ATTRIBUTE, element, *attributes );
		}
                _stack.push( parse_stack_t(element,&Expat_Document::startOutputResultType,activity) );

	    } else if ( strcasecmp( element, Xresult_observation ) == 0 ) {
		try {
		    handleSPEXObservation( activity, attributes );
		}
		catch ( const unexpected_attribute& e ) {
		    LQIO::input_error2( LQIO::ERR_UNEXPECTED_ATTRIBUTE, element, *attributes );
		}
                _stack.push( parse_stack_t(element,&Expat_Document::startSPEXObservationType,&Expat_Document::endSPEXObservationType,activity) );

            } else if ( strcasecmp( element, Xservice_time_distribution ) == 0 ) {
                Histogram * histogram = handleHistogram( activity, attributes );
                _stack.push( parse_stack_t(element,&Expat_Document::startOutputDistributionType,histogram) );

            } else if ( strcasecmp( element, Xsynch_call ) == 0 ) {
                if ( dynamic_cast<Activity *>(activity) ) {
                    call = handleActivityCall( activity, attributes, Call::RENDEZVOUS );
                } else {
                    call = handlePhaseCall( activity, attributes, Call::RENDEZVOUS );              // Phase
                }
                _stack.push( parse_stack_t(element,&Expat_Document::startActivityMakingCallType,call) );

            } else if ( strcasecmp( element, Xasynch_call ) == 0 ) {
                if ( dynamic_cast<Activity *>(activity) ) {
                    call = handleActivityCall( activity, attributes, Call::SEND_NO_REPLY );
                } else {
                    call = handlePhaseCall( activity, attributes, Call::SEND_NO_REPLY );
                }
                _stack.push( parse_stack_t(element,&Expat_Document::startActivityMakingCallType,call) );

            } else {
                throw element_error( element );
            }
        }

        /*
          <xsd:complexType name="MakingCallType">
          <xsd:sequence>
          <xsd:element name="result-call" type="OutputResultType" minOccurs="0" maxOccurs="unbounded"/>
          </xsd:sequence>
        */

        void
        Expat_Document::startActivityMakingCallType( DocumentObject * call, const XML_Char * element, const XML_Char ** attributes )
        {
            if ( strcasecmp( element, Xresult_call ) == 0 ) {
                if ( call ) {
		    try {
			handleResults( call, attributes );
		    }
		    catch ( const unexpected_attribute& e ) {
			LQIO::input_error2( LQIO::ERR_UNEXPECTED_ATTRIBUTE, element, *attributes );
		    }
                }
                _stack.push( parse_stack_t(element,&Expat_Document::startOutputResultType,call) );

	    } else if ( strcasecmp( element, Xresult_observation ) == 0 ) {
		try {
		    handleSPEXObservation( call, attributes );
		}
		catch ( const unexpected_attribute& e ) {
		    LQIO::input_error2( LQIO::ERR_UNEXPECTED_ATTRIBUTE, element, *attributes );
		}
                _stack.push( parse_stack_t(element,&Expat_Document::startSPEXObservationType,&Expat_Document::endSPEXObservationType,call) );


	    } else if ( strcasecmp( element, Xresult_observation ) == 0 ) {
		try {
		    handleSPEXObservation( call, attributes );
		}
		catch ( const unexpected_attribute& e ) {
		    LQIO::input_error2( LQIO::ERR_UNEXPECTED_ATTRIBUTE, element, *attributes );
		}
                _stack.push( parse_stack_t(element,&Expat_Document::startSPEXObservationType,&Expat_Document::endSPEXObservationType,call) );

	    } else if ( strcasecmp( element, Xqueue_length_distribution ) == 0 ) {
		Histogram * histogram = handleQueueLengthDistribution( call, attributes );
		_stack.push( parse_stack_t(element,&Expat_Document::startOutputDistributionType,histogram) );

            } else {
                throw element_error( element );
            }
        }


        void
        Expat_Document::startEntryMakingCallType( DocumentObject * call, const XML_Char * element, const XML_Char ** attributes )
        {
            if ( strcasecmp( element, Xresult_call ) == 0 ) {
                if ( call ) {
		    try {
			handleResults( call, attributes );
		    }
		    catch ( const unexpected_attribute& e ) {
			LQIO::input_error2( LQIO::ERR_UNEXPECTED_ATTRIBUTE, element, *attributes );
		    }
                }
                _stack.push( parse_stack_t(element,&Expat_Document::startOutputResultType,call) );

	    } else if ( strcasecmp( element, Xresult_observation ) == 0 ) {
		try {
		    handleSPEXObservation( call, attributes );
		}
		catch ( const unexpected_attribute& e ) {
		    LQIO::input_error2( LQIO::ERR_UNEXPECTED_ATTRIBUTE, element, *attributes );
		}
                _stack.push( parse_stack_t(element,&Expat_Document::startSPEXObservationType,&Expat_Document::endSPEXObservationType,call) );

	    } else if ( strcasecmp( element, Xqueue_length_distribution ) == 0 ) {
		Histogram * histogram = handleQueueLengthDistribution( call, attributes );
		_stack.push( parse_stack_t(element,&Expat_Document::startOutputDistributionType,histogram) );

            } else {
                throw element_error( element );
            }
        }


        /*
          <xsd:complexType name="TaskActivityGraph">
          <xsd:complexContent>
          <xsd:element name="activity" type="ActivityDefType" maxOccurs="unbounded"/>
          <xsd:element name="precedence" type="PrecedenceType" minOccurs="0" maxOccurs="unbounded"/>
          <xsd:sequence>
          <xsd:element name="reply-entry" minOccurs="0" maxOccurs="unbounded">
          </xsd:sequence>
          </xsd:complexContent>
          </xsd:complexType>
        */

        void
        Expat_Document::startTaskActivityGraph( DocumentObject * task, const XML_Char * element, const XML_Char ** attributes )
        {
            if ( strcasecmp( element, Xactivity ) == 0 ) {
                Activity * activity = handleTaskActivity( task, attributes );
                _stack.push( parse_stack_t(element,&Expat_Document::startActivityDefBase,activity) );
            } else if ( strcasecmp( element, Xprecedence ) == 0 ) {
                _stack.push( parse_stack_t(element,&Expat_Document::startPrecedenceType,task) );
            } else if ( strcasecmp( element, Xreply_entry ) == 0 ) {
                const XML_Char * entry_name = getStringAttribute( attributes, Xname );
                Entry * entry = _document.getEntryByName( entry_name );
                if ( !entry ) {
                } else {
                    _stack.push( parse_stack_t(element,&Expat_Document::startReplyActivity,entry,task) );
                }
            } else {
                throw element_error( element );
            }
        }


        /*
          <xsd:complexType name="PrecedenceType">
          <xsd:sequence>
          <xsd:choice>
          <xsd:element name="pre" type="SingleActivityListType"/>
          <xsd:element name="pre-OR" type="ActivityListType"/>
          <xsd:element name="pre-AND" type="AndJoinListType"/>
          </xsd:choice>
          <xsd:choice>
          <xsd:element name="post" type="SingleActivityListType" minOccurs="0"/>
          <xsd:element name="post-OR" type="OrListType" minOccurs="0"/>
          <xsd:element name="post-AND" type="ActivityListType" minOccurs="0"/>
          <xsd:element name="post-LOOP" type="ActivityLoopListType" minOccurs="0"/>
          </xsd:choice>
          </xsd:sequence>
          </xsd:complexType>
        */

        void
        Expat_Document::startPrecedenceType( DocumentObject * task, const XML_Char * element, const XML_Char ** attributes )
        {
            ActivityList * pre_list = 0;
            ActivityList * post_list = 0;
            std::map<const XML_Char *,ActivityList::ActivityListType>::const_iterator item = precedence_table.find(element);
            if ( item != precedence_table.end() ) {
                switch ( item->second ) {
                case ActivityList::OR_JOIN_ACTIVITY_LIST:
                case ActivityList::JOIN_ACTIVITY_LIST:
                case ActivityList::AND_JOIN_ACTIVITY_LIST:
                    if ( item->second == ActivityList::AND_JOIN_ACTIVITY_LIST ) {
                        pre_list = new AndJoinActivityList( &_document, dynamic_cast<Task *>(task),
							    getOptionalAttribute(attributes,Xquorum) );
                    } else {
                        pre_list = new ActivityList( &_document, dynamic_cast<Task *>(task), item->second );
                    }
                    post_list = dynamic_cast<ActivityList *>(_stack.top().extra_object);
                    if ( post_list ) {
                        pre_list->setNext(post_list);
                        post_list->setPrevious(pre_list);
                        _stack.top().extra_object = 0;
                        _stack.push( parse_stack_t(element,&Expat_Document::startActivityListType,pre_list) );
                    } else {
                        _stack.push( parse_stack_t(element,&Expat_Document::startActivityListType,pre_list,pre_list) );
                    }
                    break;

                case ActivityList::OR_FORK_ACTIVITY_LIST:
                case ActivityList::FORK_ACTIVITY_LIST:
                case ActivityList::AND_FORK_ACTIVITY_LIST:
                case ActivityList::REPEAT_ACTIVITY_LIST:
                    post_list = new ActivityList( &_document, dynamic_cast<Task *>(task), item->second );
                    pre_list = dynamic_cast<ActivityList *>(_stack.top().extra_object);
                    if ( pre_list ) {
                        pre_list->setNext(post_list);
                        post_list->setPrevious(pre_list);
                        _stack.top().extra_object = 0;
                        _stack.push( parse_stack_t(element,&Expat_Document::startActivityListType,post_list) );
                    } else {
                        _stack.push( parse_stack_t(element,&Expat_Document::startActivityListType,post_list,post_list) );
                    }
                    if ( item->second == ActivityList::REPEAT_ACTIVITY_LIST )  {
                        /* List end is an attribute */
                        const XML_Char * activity_name = getStringAttribute(attributes,Xend);
                        if ( activity_name ) {
                            Activity* activity = dynamic_cast<Task *>(task)->getActivity(activity_name);
                            activity->inputFrom(post_list);
                            post_list->add(activity,NULL);              /* Special count case */
                        }
                    }
                    break;
                }
            } else {
                throw element_error( element );
            }
        }


        void
        Expat_Document::startActivityListType( DocumentObject * activity_list, const XML_Char * element, const XML_Char ** attributes )
        {
            if ( strcasecmp( element, Xactivity ) == 0 ) {
                handleActivityList( dynamic_cast<ActivityList *>(activity_list), attributes );
                _stack.push( parse_stack_t(element,&Expat_Document::startNOP,0) );

            } else if ( strcasecmp( element, Xservice_time_distribution ) == 0 && dynamic_cast<ActivityList *>(activity_list)->getListType() == ActivityList::AND_JOIN_ACTIVITY_LIST ) {
                Histogram * histogram = handleHistogram( activity_list, attributes );
		_stack.push( parse_stack_t(element,&Expat_Document::startOutputDistributionType,histogram) );

            } else if ( strcasecmp( element, Xresult_join_delay ) == 0 && dynamic_cast<ActivityList *>(activity_list)->getListType() == ActivityList::AND_JOIN_ACTIVITY_LIST ) {
                handleJoinResults( dynamic_cast<AndJoinActivityList *>(activity_list), attributes );
                _stack.push( parse_stack_t(element,&Expat_Document::startJoinResultType,activity_list) );

            } else {
                throw element_error( element );
            }
        }

        /*
          <xsd:sequence>
          <xsd:element name="reply-entry" minOccurs="0" maxOccurs="unbounded">
          <xsd:complexType>
          <xsd:group ref="ReplyActivity"/>
          <xsd:attribute name="name" type="xsd:string" use="required"/>
          </xsd:complexType>
          </xsd:element>
          </xsd:sequence>
        */

        void
        Expat_Document::startReplyActivity( DocumentObject * entry, const XML_Char * element, const XML_Char ** attributes )
        {
            if ( strcasecmp( element, Xreply_activity ) == 0 ) {
                const XML_Char * activity_name = getStringAttribute(attributes,Xname);
                if ( activity_name ) {
                    const Task * task = dynamic_cast<Task *>(_stack.top().extra_object);                // entry may not have task.
                    assert( task != 0 );
                    Activity * activity = task->getActivity( activity_name );
                    if ( !activity ) {
                        input_error2( ERR_NOT_DEFINED, activity_name );
                    } else {
                        activity->getReplyList().push_back(dynamic_cast<Entry *>(entry));
                    }
                }
                _stack.push( parse_stack_t(element,&Expat_Document::startNOP,0) );
            } else {
                throw element_error( element );
            }
        }


        void
        Expat_Document::startOutputResultType( DocumentObject * object, const XML_Char * element, const XML_Char ** attributes )
        {
            if ( strcasecmp( element, Xresult_conf_95 ) == 0 ) {
                handleResults95( object, attributes );
                _stack.push( parse_stack_t(element,&Expat_Document::startNOP,0) );
            } else if ( strcasecmp( element, Xresult_conf_99 ) == 0 ) {
                _stack.push( parse_stack_t(element,&Expat_Document::startNOP,0) );
            } else {
                throw element_error( element );
            }
        }


        void
        Expat_Document::startJoinResultType( DocumentObject * object, const XML_Char * element, const XML_Char ** attributes )
        {
            if ( strcasecmp( element, Xresult_conf_95 ) == 0 ) {
                handleJoinResults95( dynamic_cast<LQIO::DOM::AndJoinActivityList*>(object), attributes );
                _stack.push( parse_stack_t(element,&Expat_Document::startNOP,0) );
            } else if ( strcasecmp( element, Xresult_conf_99 ) == 0 ) {
                _stack.push( parse_stack_t(element,&Expat_Document::startNOP,0) );
            } else {
                throw element_error( element );
            }
        }


        void
        Expat_Document::startOutputDistributionType( DocumentObject * object, const XML_Char * element, const XML_Char ** attributes )
        {
            if ( strcasecmp( element, Xhistogram_bin ) == 0 || strcasecmp( element, Xunderflow_bin ) == 0 || strcasecmp( element, Xoverflow_bin ) == 0 ) {
                handleHistogramBin( object, element, attributes );
                _stack.push( parse_stack_t(element,&Expat_Document::startNOP,0) );
            } else {
                throw element_error( element );
            }
        }

	void
	Expat_Document::startSPEXObservationType(  DocumentObject * object, const XML_Char * element, const XML_Char ** attributes )
	{
	    if ( strcasecmp( element, Xresult_conf_95 ) == 0 ) {
		handleSPEXObservation( object, attributes, 95 );
                _stack.push( parse_stack_t(element,&Expat_Document::startNOP,0) );

            } else if ( strcasecmp( element, Xresult_conf_99 ) == 0 ) {
		handleSPEXObservation( object, attributes, 99 );
                _stack.push( parse_stack_t(element,&Expat_Document::startNOP,0) );

	    } else {
		throw element_error( element );
	    }
	}

	/*
	 * Handler called when an </element> is found.  Things like processors and tasks may have 
	 * spex observation variables gathered up, and since the confidence variables are in a separate 
	 * element from the regular variables, they cannot be handled until everything in <element> is
	 * done.
	 */

	
	void
	Expat_Document::endSPEXObservationType( DocumentObject * object, const XML_Char * element )
	{
	    if ( strcasecmp( element, Xresult_observation ) == 0 ) {
		for ( std::set<LQIO::Spex::ObservationInfo>::const_iterator observation = _spex_observation.begin(); observation != _spex_observation.end(); ++observation ) {
		    if ( object == NULL ) {
			LQIO::spex.observation( *observation );
		    } else if ( dynamic_cast<Processor *>(object)
			 || dynamic_cast<Group *>(object)
			 || dynamic_cast<Task *>(object)
			 || dynamic_cast<Entry *>(object) ) {
			LQIO::spex.observation( object, *observation );
		    } else if ( dynamic_cast<LQIO::DOM::Activity *>(object) ) {
			/* Handle before phase because an activity is a phase */
			LQIO::DOM::Activity * activity = dynamic_cast<Activity *>(object);
			const LQIO::DOM::Task * task = activity->getTask();
			LQIO::spex.observation( task, activity, *observation );
		    } else if ( dynamic_cast<LQIO::DOM::Phase *>(object) ) {
			/* Map up to entry... Spex oddity */
			const Phase * phase = dynamic_cast<Phase *>(object);
			const Entry * entry = phase->getSourceEntry();
			LQIO::spex.observation( entry, *observation );
		    } else if ( dynamic_cast<LQIO::DOM::Call *>(object) ) {
			/* Even more complicated becauses I need to get to the entry to find the call in LQX */
			const LQIO::DOM::Call * call = dynamic_cast<const LQIO::DOM::Call *>(object);
			const DocumentObject * source = call->getSourceObject();
			const LQIO::DOM::Entry * destination = call->getDestinationEntry();
			if ( dynamic_cast<const LQIO::DOM::Activity *>(source) ) {
			    const LQIO::DOM::Activity * activity = dynamic_cast<const Activity *>(source);
			    const LQIO::DOM::Task * task = activity->getTask();
			    LQIO::spex.observation( task, activity, destination, *observation );
			} else if ( dynamic_cast<const LQIO::DOM::Phase *>(source) ) {
			    const LQIO::DOM::Phase * phase = dynamic_cast<const Phase *>(source);
			    const LQIO::DOM::Entry * entry = phase->getSourceEntry();
			    LQIO::spex.observation( entry, get_phase( phase ), destination, *observation );
			} else {
			    abort(); /* forwarding */
			}
		    }
		}
		_spex_observation.clear();		/* Reset for another object */
	    }
	}

        void
        Expat_Document::startLQX( DocumentObject * object, const XML_Char * element, const XML_Char ** attributes )
        {
            throw element_error( element );             /* Should not get here. */
        }

	void Expat_Document::startSPEXParameters( DocumentObject * object, const XML_Char * element, const XML_Char ** attributes )
        {
            throw element_error( element );             /* Should not get here. */
        }

	void Expat_Document::endSPEXParameters( DocumentObject * object, const XML_Char * element )
        {
	    LQIO_parse_string( SPEX_PARAMETER, _spex_parameters.c_str() );
        }

	void Expat_Document::startSPEXResults( DocumentObject * object, const XML_Char * element, const XML_Char ** attributes )
        {
            throw element_error( element );             /* Should not get here. */
        }

	void Expat_Document::endSPEXResults( DocumentObject * object, const XML_Char * element )
        {
	    LQIO_parse_string( SPEX_RESULT, _spex_results.c_str() );
        }

	void Expat_Document::startSPEXConvergence( DocumentObject * object, const XML_Char * element, const XML_Char ** attributes )
        {
            throw element_error( element );             /* Should not get here. */
        }

	void Expat_Document::endSPEXConvergence( DocumentObject * object, const XML_Char * element )
        {
	    LQIO_parse_string( SPEX_CONVERGENCE, _spex_convergence.c_str() );
        }

        void
        Expat_Document::startNOP( DocumentObject * object, const XML_Char * element, const XML_Char ** attributes )
        {
            throw element_error( element );             /* Should not get here. */
        }

        /* ------------------------------------------------------------------------ */

        DocumentObject *
        Expat_Document::handleModel( DocumentObject * object, const XML_Char ** attributes )
        {
	    checkAttributes( Xlqn_model, attributes, parameter_table );

            if ( _createObjects ) {
                _document.setModelParameters( getStringAttribute(attributes,Xcomment),
					      getVariableAttribute(attributes,Xconv_val,"0.00001"),
					      getVariableAttribute(attributes,Xit_limit,"50"),
					      getVariableAttribute(attributes,Xprint_int,"0"),
					      getVariableAttribute(attributes,Xunderrelax_coeff,"0.9"),
					      0 );
            }
            return object;
        }


        /*
          <xsd:attribute name="multiplicity" type="SrvnNonNegativeInteger" default="1"/>
          <xsd:attribute name="speed-factor" type="SrvnFloat" default="1"/>
          <xsd:attribute name="scheduling" type="SchedulingType" default="fcfs"/>
          <xsd:attribute name="replication" type="xsd:nonNegativeInteger" default="1"/>
          <xsd:attribute name="quantum" type="SrvnFloat" default="0"/>
          <xsd:attribute name="name" type="xsd:string" use="required"/>
        */

        Processor *
        Expat_Document::handleProcessor( DocumentObject * object, const XML_Char ** attributes )
        {
	    checkAttributes( Xprocessor, attributes, processor_table );

            const XML_Char * processor_name = getStringAttribute(attributes,Xname);
            Processor * processor = _document.getProcessorByName( getStringAttribute(attributes,Xname) );
            if ( _createObjects ) {
                if ( processor ) {
                    throw duplicate_symbol( processor_name );
                }
                const scheduling_type scheduling_flag = getSchedulingAttribute( attributes, SCHEDULE_FIFO );
                processor = new Processor( &_document, processor_name,
					   scheduling_flag,
					   getOptionalAttribute(attributes,Xmultiplicity),
					   getOptionalAttribute(attributes,Xreplication) );
                _document.addProcessorEntity( processor );

		processor->setRate( getVariableAttribute(attributes,Xspeed_factor,"1.") );

		LQIO::DOM::ExternalVariable * quantum = getOptionalAttribute(attributes,Xquantum);
		if ( !is_default_value( quantum, 0. ) ) {
                    if ( scheduling_flag == SCHEDULE_FIFO
                         || scheduling_flag == SCHEDULE_HOL
                         || scheduling_flag == SCHEDULE_PPR
                         || scheduling_flag == SCHEDULE_RAND ) {
                        input_error2( LQIO::WRN_QUANTUM_SCHEDULING, processor_name, scheduling_label[scheduling_flag].str );
                    } else {
                        processor->setQuantum( quantum );
                    }
                } else if ( scheduling_flag == SCHEDULE_CFS ) {
                    input_error2( LQIO::ERR_NO_QUANTUM_SCHEDULING, processor_name, scheduling_label[scheduling_flag].str );
                }


            } else if ( !processor) {
                throw undefined_symbol( processor_name );
            }

            return processor;
        }



        /*
          <xsd:attribute name="name" type="xsd:string" use="required"/>
          <xsd:attribute name="cap" type="xsd:boolean" use="optional" default="false"/>
          <xsd:attribute name="share" type="xsd:double" use="required"/>
        */

        DocumentObject *
        Expat_Document::handleGroup( DocumentObject * processor, const XML_Char ** attributes )
        {
	    checkAttributes( Xgroup, attributes, group_table );

	    const XML_Char * group_name = getStringAttribute(attributes,Xname);
	    Group* group = _document.getGroupByName( group_name );
	    if ( dynamic_cast<Processor *>(processor)->getSchedulingType() != SCHEDULE_CFS ) {
		LQIO::input_error2( LQIO::WRN_NON_CFS_PROCESSOR, group_name, processor->getName().c_str() );
		return processor;
	    } else if ( _createObjects ) {
		if ( group ) {
		    throw duplicate_symbol( group_name );
		} else {
		    group = new Group( &_document, group_name,
				       dynamic_cast<Processor *>(processor),
				       getVariableAttribute( attributes, Xshare ),
				       getBoolAttribute( attributes, Xcap) );
		    _document.addGroup(group);
		    dynamic_cast<Processor *>(processor)->addGroup(group);
		}
            } else if ( !group ) {
                throw undefined_symbol( group_name );
            }
            return group;
        }



        /*
          <xsd:attribute name="name" type="xsd:string" use="required"/>
          <xsd:attribute name="multiplicity" type="SrvnNonNegativeInteger" default="1"/>
          <xsd:attribute name="replication" type="xsd:nonNegativeInteger" default="1"/>
          <xsd:attribute name="scheduling" type="TaskSchedulingType" default="fcfs"/>
          <xsd:attribute name="think-time" type="SrvnFloat" default="0"/>
          <xsd:attribute name="priority" type="xsd:nonNegativeInteger" default="0"/>
          <xsd:attribute name="queue-length" type="xsd:nonNegativeInteger" default="0"/>
          <xsd:attribute name="activity-graph" type="TaskOptionType"/>
        */

        Task *
        Expat_Document::handleTask( DocumentObject * object, const XML_Char ** attributes )
        {
	    checkAttributes( Xtask, attributes, task_table );
            const XML_Char * task_name = getStringAttribute(attributes,Xname);
            Task * task = _document.getTaskByName( task_name );


            if ( _createObjects ) {
                const scheduling_type sched_type = getSchedulingAttribute( attributes, SCHEDULE_FIFO );
                if ( task ) {
                    throw duplicate_symbol( task_name );
                }

                Processor * processor = dynamic_cast<Processor *>(object);
                Group * group = 0;
                if ( !processor ) {
                    group = dynamic_cast<Group *>(object);
                    if ( !group ) {
                        return 0;
                    } else {
                        processor = const_cast<Processor *>(group->getProcessor());
                    }
                }

                std::vector<Entry *> entries;           /* Add list later */
                const XML_Char * tokens = getStringAttribute( attributes, Xinitially, "" );

                if ( sched_type == SCHEDULE_SEMAPHORE ) {
                    task = new SemaphoreTask( &_document, task_name, entries, processor,
                                              getOptionalAttribute(attributes,Xqueue_length),
					      getOptionalAttribute(attributes,Xpriority),
                                              getOptionalAttribute(attributes,Xmultiplicity),
                                              getOptionalAttribute(attributes,Xreplication),
                                              group );
                    if ( strcasecmp( tokens, "0" ) == 0 ) {
                        dynamic_cast<SemaphoreTask *>(task)->setInitialState(SemaphoreTask::INITIALLY_EMPTY);
                    }
                    /* Otherwise, aliased to multiplicity */

                } else if ( sched_type == SCHEDULE_RWLOCK ){
                    task = new RWLockTask( &_document, task_name, entries, processor,
                                           getOptionalAttribute(attributes,Xqueue_length),
					   getOptionalAttribute(attributes,Xpriority),
                                           getOptionalAttribute(attributes,Xmultiplicity),
                                           getOptionalAttribute(attributes,Xreplication),
                                           group );

                    if ( strlen( tokens ) > 0 ) {
                        input_error( "Unexpected attribute <%s> ", Xinitially );
                    }
                } else {
                    task = new Task( &_document, task_name, sched_type, entries, processor,
                                     getOptionalAttribute(attributes,Xqueue_length),
				     getOptionalAttribute(attributes,Xpriority),
                                     getOptionalAttribute(attributes,Xmultiplicity),
                                     getOptionalAttribute(attributes,Xreplication),
                                     group );
                    if ( strlen( tokens ) > 0 ) {
                        input_error( "Unexpected attribute <%s> ", Xinitially );
                    }
                }

		LQIO::DOM::ExternalVariable * think_time = getOptionalAttribute( attributes, Xthink_time );
		if ( !is_default_value( think_time, 0. ) ) {
                    if ( sched_type == SCHEDULE_CUSTOMER ) {
                        task->setThinkTime( think_time );
		    } else {
                        LQIO::input_error2( LQIO::ERR_NON_REF_THINK_TIME, task_name );
                    }
                }

                /* Link in the entity information */
                _document.addTaskEntity(task);
                processor->addTask(task);
                if ( group ) group->addTask(task);

            } else if ( !task ) {
                throw undefined_symbol( task_name );
            }

            return task;
        }


        /*
	  <xsd:attribute name="source" type="xsd:string" use="required"/>
	  <xsd:attribute name="value" type="xsd:nonNegativeInteger" use="required"/>
        */

        void
        Expat_Document::handleFanIn( DocumentObject * object, const XML_Char ** attributes )
        {
            const XML_Char * source = NULL;
            const XML_Char * value = NULL;
            for ( ; *attributes; attributes += 2 ) {
                if ( strcasecmp( *attributes, Xsource ) == 0 && !source ) {
                    source = *(attributes+1);
                } else if ( strcasecmp( *attributes, Xvalue ) == 0 && !value ) {
                    value = *(attributes+1);
                } else {
		    LQIO::input_error2( LQIO::ERR_UNEXPECTED_ATTRIBUTE, Xfanin, *attributes );
                }
            }
            if ( !source ) {
                throw missing_attribute( Xsource );
            } else if ( !value ) {
                throw missing_attribute( Xvalue );
            } else {
                dynamic_cast<LQIO::DOM::Task *>(object)->setFanIn( source, _document.db_build_parameter_variable(value,NULL) );
            }
        }

        /*
	  <xsd:attribute name="dest" type="xsd:string" use="required"/>
	  <xsd:attribute name="value" type="xsd:nonNegativeInteger" use="required"/>
        */

        void
        Expat_Document::handleFanOut( DocumentObject * object, const XML_Char ** attributes )
        {
            const XML_Char * destination = NULL;
            const XML_Char * value = NULL;
            for ( ; *attributes; attributes += 2 ) {
                if ( strcasecmp( *attributes, Xdest ) == 0 && !destination ) {
                    destination = *(attributes+1);
                } else if ( strcasecmp( *attributes, Xvalue ) == 0 && !value ) {
                    value = *(attributes+1);
                } else {
		    LQIO::input_error2( LQIO::ERR_UNEXPECTED_ATTRIBUTE, Xfanout, *attributes );
                }
            }
            if ( !destination ) {
                throw missing_attribute( Xdest );
            } else if ( !value ) {
                throw missing_attribute( Xvalue );
            } else {
                dynamic_cast<LQIO::DOM::Task *>(object)->setFanOut( destination, _document.db_build_parameter_variable(value,NULL) );
            }
        }

        /*
          <xsd:attribute name="type" use="required">
          <xsd:simpleType>
          <xsd:enumeration value="PH1PH2"/>
          <xsd:enumeration value="GRAPH"/>
          <xsd:enumeration value="NONE"/>
          </xsd:simpleType>
          </xsd:attribute>
          <xsd:attribute name="name" type="xsd:string" use="required"/>
          <xsd:attribute name="open-arrival-rate" type="SrvnFloat"/>
          <xsd:attribute name="priority" type="xsd:int"/>
          <xsd:attribute name="semaphore" type="SemaphoreType"/>
        */

        Entry *
        Expat_Document::handleEntry( DocumentObject * task, const XML_Char ** attributes )
        {
	    checkAttributes( Xentry, attributes, entry_table );
            const XML_Char * entry_name = getStringAttribute(attributes,Xname);

            Entry * entry = _document.getEntryByName( entry_name );
            if ( _createObjects ) {

                if ( !entry ) {
                    entry = new Entry( &_document, entry_name );
                    _document.addEntry(entry);            /* Add to global table */
		} else if ( entry->getEntryType() != LQIO::DOM::Entry::ENTRY_NOT_DEFINED ) {
		    throw duplicate_symbol( entry_name );
                }

		const XML_Char * type = getStringAttribute(attributes,Xtype,"");
		if ( strcasecmp( type, XNONE ) == 0 ) {
		    entry->setEntryType( Entry::ENTRY_ACTIVITY_NOT_DEFINED );
		} else if ( strcasecmp( type, XPH1PH2 ) == 0 ) {
		    entry->setEntryType( Entry::ENTRY_STANDARD_NOT_DEFINED );
		}

		entry->setEntryPriority( getOptionalAttribute(attributes,Xpriority) );
		entry->setOpenArrivalRate( getOptionalAttribute(attributes,Xopen_arrival_rate ) );

                const XML_Char * semaphore = getStringAttribute(attributes,Xsemaphore,"");
                if ( strlen(semaphore) > 0 ) {
                    if (strcasecmp(semaphore,Xsignal) == 0 ) {
                        entry->setSemaphoreFlag(SEMAPHORE_SIGNAL);
                    } else if (strcasecmp(semaphore,Xwait) == 0 )  {
                        entry->setSemaphoreFlag(SEMAPHORE_WAIT);
                    } else {
                        internal_error( __FILE__, __LINE__, "handleEntries: <entry name=\"%s\" sempahore=\"%s\">", entry_name, semaphore );
                    }
                }

                const XML_Char * rwlock = getStringAttribute(attributes,Xrwlock,"");
                if ( strlen(rwlock) > 0 ) {
                    if (strcasecmp(rwlock,Xr_unlock) == 0 ) {
                        entry->setRWLockFlag(RWLOCK_R_UNLOCK);
                    } else if (strcasecmp(rwlock,Xr_lock) == 0 ) {
                        entry->setRWLockFlag(RWLOCK_R_LOCK);
                    } else if (strcasecmp(rwlock,Xw_unlock) == 0 ) {
                        entry->setRWLockFlag(RWLOCK_W_UNLOCK);
                    } else if (strcasecmp(rwlock,Xw_lock) == 0 ) {
                        entry->setRWLockFlag(RWLOCK_W_LOCK);
                    } else {
                        internal_error( __FILE__, __LINE__, "handleEntries: <entry name=\"%s\" rwlock=\"%s\">", entry_name, rwlock );
                    }
                }

                const std::vector<Entry *>& entries = dynamic_cast<Task *>(task)->getEntryList();
                const_cast<std::vector<Entry *>*>(&entries)->push_back( entry );        /* Add to task. */

            } else if ( !entry ) {
                throw undefined_symbol( entry_name );
            }

            return entry;
        }


        Phase *
        Expat_Document::handlePhaseActivity( DocumentObject * entry, const XML_Char ** attributes )
        {
            Phase* phase = 0;
            const long p = getLongAttribute(attributes,Xphase);
            if ( p < 1 || 3 < p ) {
                throw std::domain_error( "phase" );
            } else {
                phase = dynamic_cast<Entry *>(entry)->getPhase(p);
                if (!phase) internal_error( __FILE__, __LINE__, "missing phase." );
            }

            if ( _createObjects ) {
                phase->setName( getStringAttribute(attributes,Xname) );
                _document.db_check_set_entry(dynamic_cast<Entry *>(entry), entry->getName(), DOM::Entry::ENTRY_STANDARD);
            }

            handleActivity( phase, attributes );

            return phase;
        }


        Activity *
        Expat_Document::handleTaskActivity( DocumentObject * task, const XML_Char ** attributes )
        {
            const XML_Char * activity_name = getStringAttribute(attributes,Xname);
            Activity * activity = dynamic_cast<Task *>(task)->getActivity(activity_name, _createObjects);
            if ( activity ) {
		if ( _createObjects && activity->isSpecified() ) {
		    throw duplicate_symbol( activity_name );
		}
                activity->setIsSpecified(true);

                const XML_Char * first_entry = getStringAttribute(attributes,Xbound_to_entry,"");
                if ( strlen(first_entry) > 0 ) {
                    Entry* entry = _document.getEntryByName(first_entry);
                    _document.db_check_set_entry(entry, first_entry, Entry::ENTRY_ACTIVITY);
                    entry->setStartActivity(activity);
                }

                handleActivity( activity, attributes );
            } else {
                throw undefined_symbol( activity_name );
            }

            return activity;
        }


        void
        Expat_Document::handleActivity( Phase * phase, const XML_Char ** attributes )
        {
	    checkAttributes( Xactivity, attributes, activity_table );
            if ( _createObjects ) {
		phase->setServiceTime( getVariableAttribute(attributes,Xhost_demand_mean,"0.0" ) );
		phase->setCoeffOfVariationSquared( getOptionalAttribute(attributes,Xhost_demand_cvsq) );
		phase->setThinkTime( getOptionalAttribute(attributes,Xthink_time) );
                const double max_service = getDoubleAttribute(attributes,Xmax_service_time,0.0);
                if ( max_service > 0 ) {
                    findOrAddHistogram( phase, LQIO::DOM::Histogram::CONTINUOUS, 0, max_service, max_service );
                }
                const XML_Char * call_order = getStringAttribute(attributes,Xcall_order,"");
                if ( strlen(call_order) > 0 ) {
                    phase->setPhaseTypeFlag(strcasecmp(XDETERMINISTIC, call_order) == 0 ? PHASE_DETERMINISTIC : PHASE_STOCHASTIC);
                }
            }
        }



        void
        Expat_Document::handleActivityList( ActivityList * activity_list, const XML_Char ** attributes )
        {
            const XML_Char * activity_name = getStringAttribute( attributes, Xname );
            const Task * task = activity_list->getTask();
            if ( !task ) internal_error( __FILE__, __LINE__, "missing task." );

            Activity * activity = task->getActivity( activity_name );
            if ( !activity ) {
                throw undefined_symbol( activity_name );

            } else if ( _createObjects ) {
                switch ( activity_list->getListType() ) {
                case ActivityList::AND_JOIN_ACTIVITY_LIST:
                case ActivityList::OR_JOIN_ACTIVITY_LIST:
                case ActivityList::JOIN_ACTIVITY_LIST:
                    activity_list->add( activity );
                    activity->outputTo( activity_list );
                    break;

                case ActivityList::OR_FORK_ACTIVITY_LIST:
                    activity_list->add( activity, getVariableAttribute( attributes, Xprob ) );
                    activity->inputFrom( activity_list );
                    break;

                case ActivityList::FORK_ACTIVITY_LIST:
                case ActivityList::AND_FORK_ACTIVITY_LIST:
                    activity_list->add( activity );
                    activity->inputFrom( activity_list );
                    break;

                case ActivityList::REPEAT_ACTIVITY_LIST:
                    activity_list->add( activity, getVariableAttribute( attributes, Xcount ) );
                    activity->inputFrom( activity_list );
                    break;
                }
            }
        }

        /*
          <xsd:attribute name="dest" type="xsd:string" use="required"/>
          <xsd:attribute name="calls-mean" type="SrvnFloat" use="required"/>
        */

        Call *
        Expat_Document::handlePhaseCall( DocumentObject * phase, const XML_Char ** attributes, const Call::CallType call_type )
        {
	    checkAttributes( call_type == DOM::Call::RENDEZVOUS ? Xsynch_call : Xasynch_call, attributes, call_table );

            const XML_Char * dest_entry_name = getStringAttribute(attributes,Xdest);
	    Phase * from_phase = dynamic_cast<Phase *>(phase);
            const Entry * from_entry = from_phase->getSourceEntry();
            Entry* to_entry = _document.getEntryByName(dest_entry_name);

            if ( !to_entry ) {
                if ( _createObjects ) {
                    to_entry = new Entry( &_document, dest_entry_name );         // This differs here.. entry may not exist, so create it anyway then test later. assert(to_entry != NULL);
                    _document.addEntry(to_entry);         /* Add to global table */
                } else {
                    throw undefined_symbol( dest_entry_name );
                }
            }

            Call * call = from_phase->getCallToTarget(to_entry);

            if ( _createObjects ) {
                /* Make sure that this is a standard entry */
                if ( !from_entry ) internal_error( __FILE__, __LINE__, "missing from entry" );
                _document.db_check_set_entry(const_cast<Entry *>(from_entry), from_entry->getName(), Entry::ENTRY_STANDARD);
                _document.db_check_set_entry(to_entry, dest_entry_name, Entry::ENTRY_NOT_DEFINED);

                /* Push all the times */

		LQIO::DOM::ExternalVariable* calls = getVariableAttribute(attributes,Xcalls_mean);

                /* Check the existence */
                if (call == NULL) {
                    call = new Call( &_document, call_type, dynamic_cast<Phase *>(phase), to_entry, calls );
		    std::string name = phase->getName();
		    name += '_';
		    name += to_entry->getName();
		    call->setName(name);
		    dynamic_cast<Phase *>(phase)->addCall(call);
                } else {
                    if (call->getCallType() != Call::NULL_CALL) {
                        input_error2( WRN_MULTIPLE_SPECIFICATION );
                    }

                    /* Set the new call type and the new mean */
                    call->setCallType(call_type);
                    call->setCallMean(calls);
                }
            }

            return call;
        }



        /*
          <xsd:attribute name="dest" type="xsd:string" use="required"/>
          <xsd:attribute name="calls-mean" type="SrvnFloat" use="required"/>
        */

        Call *
        Expat_Document::handleActivityCall( DocumentObject * activity, const XML_Char ** attributes, const Call::CallType call_type )
        {
	    checkAttributes( call_type == DOM::Call::RENDEZVOUS ? Xsynch_call : Xasynch_call, attributes, call_table );

            const XML_Char * dest_entry_name = getStringAttribute(attributes,Xdest);

            /* Obtain the entry that we will be adding the phase times to */
            Entry* to_entry = _document.getEntryByName(dest_entry_name);
            if ( !to_entry ) {
                if ( _createObjects ) {
                    to_entry = new Entry( &_document, dest_entry_name );         // This differs here.. entry may not exist, so create it anyway then test later. assert(to_entry != NULL);
                    _document.addEntry(to_entry);         /* Add to global table */
                } else {
                    throw undefined_symbol( dest_entry_name );
                }
            }

            Call * call = dynamic_cast<Activity *>(activity)->getCallToTarget( to_entry );
            if ( _createObjects ) {
                /* Push all the times */

                if ( !call ) {
                    call = new Call( &_document, call_type, dynamic_cast<Activity *>(activity), to_entry, getVariableAttribute(attributes,Xcalls_mean) );
		    std::string name = activity->getName();
		    name += '_';
		    name += to_entry->getName();
		    call->setName(name);
                    dynamic_cast<Activity *>(activity)->addCall(call);
                } else if (call->getCallType() != Call::NULL_CALL) {
                    LQIO::input_error2( LQIO::WRN_MULTIPLE_SPECIFICATION );
                }
            } else if ( !call ) {
                throw undefined_symbol( "call" );
            }

            return call;
        }


        Call *
        Expat_Document::handleEntryCall( DocumentObject * entry, const XML_Char ** attributes )
        {
            const XML_Char * dest_entry_name = getStringAttribute(attributes,Xdest);

            /* Obtain the entry that we will be adding the phase times to */
            Entry* to_entry = _document.getEntryByName(dest_entry_name);
            if ( !to_entry ) {
                if ( _createObjects ) {
                    to_entry = new Entry( &_document, dest_entry_name );         // This differs here.. entry may not exist, so create it anyway then test later. assert(to_entry != NULL);
                    _document.addEntry(to_entry);         /* Add to global table */
                } else {
                    throw std::runtime_error( dest_entry_name );
                }
            }

	    Entry * from_entry = dynamic_cast<Entry *>(entry);
	    Call * call = from_entry->getForwardingToTarget(to_entry);
            if ( _createObjects ) {
                if ( call == NULL ) {
                    call = new Call( &_document, from_entry, to_entry, getVariableAttribute(attributes,Xprob) );
		    std::string name = from_entry->getName();
		    name += '_';
		    name += to_entry->getName();
		    call->setName(name);
                    from_entry->addForwardingCall(call);
                } else if (call->getCallType() != Call::NULL_CALL) {
                    LQIO::input_error2( LQIO::WRN_MULTIPLE_SPECIFICATION );
                }
            }

            return call;
        }


        Histogram *
        Expat_Document::handleHistogram( DocumentObject * object, const XML_Char ** attributes )
        {
	    checkAttributes( Xhistogram_bin, attributes, histogram_table );

	    /* Handle entries specially */
	    unsigned int phase = getLongAttribute( attributes, Xphase, 0 );		/* Default, other it will throw up. */
	    if ( phase ) {
		if ( !dynamic_cast<Entry *>(object)) {
		    LQIO::input_error2( LQIO::ERR_UNEXPECTED_ATTRIBUTE, Xhistogram_bin, Xphase );
		    return NULL;
		} else {
		    return findOrAddHistogram( object, phase, Histogram::CONTINUOUS,	/* Special version for entries. */
					       getLongAttribute(attributes, Xnumber_bins, 10),
					       getDoubleAttribute(attributes, Xmin),
					       getDoubleAttribute(attributes, Xmax));
		}
	    } else {
		return findOrAddHistogram( object, Histogram::CONTINUOUS,
					   getLongAttribute(attributes, Xnumber_bins, 10),
					   getDoubleAttribute(attributes, Xmin),
					   getDoubleAttribute(attributes, Xmax));
	    }
        }

        Histogram *
        Expat_Document::handleQueueLengthDistribution( DocumentObject * object, const XML_Char ** attributes )
        {
	    checkAttributes( Xhistogram_bin, attributes, histogram_table );

            return findOrAddHistogram( object, Histogram::DISCRETE,
				       getLongAttribute(attributes, Xnumber_bins,0),	/* default values (for petrisrvn) */
                                       getDoubleAttribute(attributes, Xmin,0),
                                       getDoubleAttribute(attributes, Xmax,0) );
        }

        void
        Expat_Document::handleHistogramBin( DocumentObject * object, const XML_Char * element, const XML_Char ** attributes )
        {
            Histogram * histogram = dynamic_cast<Histogram *>(object);
            if ( histogram && _loadResults ) {
                const unsigned int index = histogram->getBinIndex(getDoubleAttribute(attributes,Xbegin));
                const double mean = getDoubleAttribute(attributes,Xprob);
                const double variance = invert( getDoubleAttribute(attributes,Xconf_95,0.0) );
                histogram->setBinMeanVariance( index, mean, variance );
            }
        }

        /*
         * This function is table driven.  The approrpiate function is called for whichever attribute appears.
         * If the target element is does not expect the results, the superclass Document_Object method handles
         * the call instead.  Zero's are stored so that the 'hasResults' is set inadvertently.
         */

        void
        Expat_Document::handleResults( DocumentObject * object, const XML_Char ** attributes )
        {
            for ( ; *attributes; attributes += 2 ) {
                std::map<const XML_Char *,result_table_t>::const_iterator item = result_table.find(*attributes);
                if ( item != result_table.end() ) {
                    set_result_fptr func = item->second.mean;
		    const double value = get_double( *attributes, *(attributes+1) );
		    if ( func && _loadResults && value > 0. ) {
                        (object->*func)( value );
                    }
                } else {
		    throw unexpected_attribute( *attributes );
                }
            }
        }

        void
        Expat_Document::handleResults95( DocumentObject * object, const XML_Char ** attributes )
        {
            for ( ; *attributes; attributes += 2 ) {
                std::map<const XML_Char *,result_table_t>::const_iterator item = result_table.find(*attributes);
                if ( item != result_table.end() ) {
                    set_result_fptr func = item->second.variance;
		    const double value = get_double( *attributes, *(attributes+1) );
		    if ( func && _loadResults && value > 0. ) {
                        (object->*func)( invert( value ) );      /* Save result as variance */
                    }
                }
            }
        }

        void
        Expat_Document::handleJoinResults( AndJoinActivityList * join_list, const XML_Char ** attributes )
        {
            if ( _loadResults ) {
                join_list->setResultJoinDelay( getDoubleAttribute(attributes,Xjoin_waiting,0.0) );
                join_list->setResultVarianceJoinDelay( getDoubleAttribute(attributes,Xjoin_variance,0.0) );
            }
        }


        void
        Expat_Document::handleJoinResults95( AndJoinActivityList * join_list, const XML_Char ** attributes )
        {
            if ( _loadResults ) {
                join_list->setResultJoinDelayVariance( invert(getDoubleAttribute(attributes,Xjoin_waiting,0.0)) );
                join_list->setResultVarianceJoinDelayVariance( invert(getDoubleAttribute(attributes,Xjoin_variance,0.0)) );
            }
        }

	/*
	 * Stick all observations into _spex_observation in case we need add 95% confidence intervals.
	 * Observation handling is done by the end handler endSPEXObservation();
	 */
	
	void
 	Expat_Document::handleSPEXObservation( DocumentObject * object, const XML_Char ** attributes, unsigned int conf_level )
	{
            for ( ; *attributes; attributes += 2 ) {
                std::map<const XML_Char *,observation_table_t>::const_iterator item = observation_table.find(*attributes);
                if ( item != observation_table.end() ) {
		    const int key = item->second.key;
		    /* Find the phase for the observation */
		    int p = 0;
		    if ( dynamic_cast<LQIO::DOM::Phase *>( object ) != NULL && dynamic_cast<LQIO::DOM::Activity *>( object ) == NULL ) {
			p = get_phase( dynamic_cast<LQIO::DOM::Phase *>( object ) );
		    } else if ( dynamic_cast<LQIO::DOM::Call *>( object ) != NULL ) {
			const LQIO::DOM::Call * call = dynamic_cast<const LQIO::DOM::Call *>(object);
			const DocumentObject * source = call->getSourceObject();
			if ( dynamic_cast<const LQIO::DOM::Phase *>(source) && dynamic_cast<LQIO::DOM::Activity *>( object ) == NULL ) {
			    p = get_phase( dynamic_cast<const LQIO::DOM::Phase *>(source) );
			}
		    } else {
			p = item->second.phase;
		    }
		    /* Find or create the observation and set necessary fields */
		    std::pair<std::set<LQIO::Spex::ObservationInfo>::iterator,bool> item = _spex_observation.insert( LQIO::Spex::ObservationInfo( key, p ) );
		    LQIO::Spex::ObservationInfo& obs = const_cast<LQIO::Spex::ObservationInfo&>(*(item.first));
		    try {
			if ( conf_level == 0 ) {
			    obs.setVariableName( *(attributes+1) );
			} else {
			    obs.setConfLevel(conf_level).setConfVariableName( *(attributes+1) );
			}
		    }
		    catch ( const std::invalid_argument& e ) {
			Common_IO::invalid_argument( *attributes, *(attributes+1) );
		    }

                } else {
		    throw unexpected_attribute( *attributes );
                }
            }
	}

        Histogram *
        Expat_Document::findOrAddHistogram( DocumentObject * object, Histogram::histogram_t type, unsigned int n_bins, double min, double max )
        {
            Histogram * histogram = 0;
            if ( _createObjects ) {
                if ( object->hasHistogram() ) throw duplicate_symbol( object->getName() );

                histogram = new Histogram( &_document, type, n_bins, min, max );
                object->setHistogram( histogram );
            } else if ( !object->hasHistogram() ) {
		throw std::runtime_error( object->getName() );
	    } else {
		histogram = const_cast<Histogram *>(object->getHistogram());
            }
            return histogram;
        }


        Histogram *
        Expat_Document::findOrAddHistogram( DocumentObject * object, unsigned int phase, Histogram::histogram_t type, unsigned int n_bins, double min, double max )
        {
            Histogram * histogram = 0;
            if ( _createObjects ) {
                if ( object->hasHistogramForPhase( phase ) ) throw duplicate_symbol( object->getName() );

                histogram = new Histogram( &_document, type, n_bins, min, max );
                object->setHistogramForPhase( phase, histogram );
            } else if ( !object->hasHistogramForPhase( phase ) ) {
		throw std::runtime_error( object->getName() );
	    } else {
		histogram = const_cast<Histogram *>(object->getHistogramForPhase( phase ));
            }
            return histogram;
	}

	/* 
	 * Ensure all attributes for element_name are found in the table.
	 */
	
        bool
        Expat_Document::checkAttributes( const XML_Char * element_name, const XML_Char ** attributes, std::set<const XML_Char *,Expat_Document::attribute_table_t>& table ) const
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

	/* 
	 * Return either a constant or a variable by converting attribute.  If the attribute is NOT found
	 * use the default (if present), or throw missing_attribute.
	 */
	
	LQIO::DOM::ExternalVariable *
	Expat_Document::getVariableAttribute( const XML_Char **attributes, const XML_Char * attribute, const XML_Char * default_value ) const
	{
	    const XML_Char * s = default_value;
            for ( ; *attributes; attributes += 2 ) {
                if ( strcasecmp( *attributes, attribute ) == 0 ) {
                    s = *(attributes+1);
                }
            }
	    if ( !s ) {
                throw missing_attribute( attribute );
	    } else {
		return _document.db_build_parameter_variable( s, NULL );
	    }
	}

	/* 
	 * Return either a constant or a variable by converting attribute.  If the attribute is NOT found
	 * use the default (if present), or throw missing_attribute.
	 */
	
	LQIO::DOM::ExternalVariable *
	Expat_Document::getOptionalAttribute( const XML_Char **attributes, const XML_Char * attribute ) const
	{
            for ( ; *attributes; attributes += 2 ) {
                if ( strcasecmp( *attributes, attribute ) == 0 ) {
		    return _document.db_build_parameter_variable( *(attributes+1), NULL );
		}
	    }
	    return NULL;
	}

        const XML_Char *
        Expat_Document::getStringAttribute(const XML_Char ** attributes, const XML_Char * attribute, const XML_Char * default_value ) const
        {
            for ( ; *attributes; attributes += 2 ) {
                if ( strcasecmp( *attributes, attribute ) == 0 ) {
                    return *(attributes+1);
                }
            }
            if ( default_value ) {
                return default_value;
            } else {
                throw missing_attribute( attribute );
            }
        }

        const double
        Expat_Document::getDoubleAttribute(const XML_Char ** attributes, const XML_Char * attribute, const double default_value ) const
        {
            for ( ; *attributes; attributes += 2 ) {
                if ( strcasecmp( *attributes, attribute ) == 0 ) {
                    return get_double( *attributes, *(attributes+1) );
                }
            }
            if ( default_value >= 0.0 ) {
                return default_value;
            } else {
                throw missing_attribute( attribute );
            }
        }

        const long
        Expat_Document::getLongAttribute(const XML_Char ** attributes, const XML_Char * attribute, const long default_value ) const
        {
            for ( ; *attributes; attributes += 2 ) {
                if ( strcasecmp( *attributes, attribute ) == 0 ) {
                    char * end_ptr = 0;
                    const double value = strtod( *(attributes+1), &end_ptr );
                    if ( value < 0. || rint(value) != value || ( end_ptr && *end_ptr != '\0' ) ) {
			invalid_argument( *attributes, *(attributes+1) );
		    }
                    return static_cast<long>(value);
                }
            }
            if ( default_value >= 0 ) {
                return default_value;
            } else {
                throw missing_attribute( attribute );
            }
        }

        const bool
        Expat_Document::getBoolAttribute( const XML_Char ** attributes, const XML_Char * attribute, const bool default_value ) const
        {
            for ( ; *attributes; attributes += 2 ) {
                if ( strcasecmp( *attributes, attribute ) == 0 ) {
                    return strcasecmp( *(attributes+1), "yes" ) == 0 || strcasecmp( *(attributes+1), "true" ) == 0 || strcmp( *(attributes+1), "1" ) == 0;
                }
            }
            return default_value;
        }

        const double
        Expat_Document::getTimeAttribute( const XML_Char ** attributes, const XML_Char * attribute ) const
        {
            for ( ; *attributes; attributes += 2 ) {
                if ( strcasecmp( *attributes, attribute ) == 0 ) {
                    unsigned long hrs   = 0;
                    unsigned long mins  = 0;
                    double secs         = 0;

                    sscanf( *(attributes+1), "%ld:%ld:%lf", &hrs, &mins, &secs );
                    return hrs * 3600 + mins * 60 + secs;
                }
            }
            return 0.;
        }

        const scheduling_type
        Expat_Document::getSchedulingAttribute( const XML_Char ** attributes, const scheduling_type default_value ) const
        {
            for ( ; *attributes; attributes += 2 ) {
                if ( strcasecmp( *attributes, Xscheduling ) == 0 ) {
		    std::map<const char *, scheduling_type>::const_iterator i = scheduling_table.find( *(attributes+1) );
		    if ( i == scheduling_table.end() ) {
			invalid_argument( *attributes, *(attributes+1) );
		    } else {
			return i->second;
                    }
                }
            }
            return default_value;
        }

	double Expat_Document::get_double( const char * attr, const char * val ) const
	{
	    char * end_ptr = 0;
	    const double value = strtod( val, &end_ptr );
	    if ( value < 0 || ( end_ptr && *end_ptr != '\0' ) ) {
		invalid_argument( attr, val );
	    }
	    return value;
	}

        /* ---------------------------------------------------------------- */
        /* DOM serialization 						    */
        /* ---------------------------------------------------------------- */

	class ExportObservation {
	public:
	    ExportObservation( std::ostream& output, unsigned int conf_level=0 ) : _output( output ), _conf_level(conf_level) {}
	    void operator()( const std::pair<const DocumentObject *,const Spex::ObservationInfo>& obs ) const { print( obs.second ); }
	    void operator()( const Spex::ObservationInfo& obs ) const { print( obs ); }

	private:
	    void print( const Spex::ObservationInfo& obs ) const
		{
		    const char * attribute = LQIO::DOM::Expat_Document::__key_lqx_function_map.at(obs.getKey());
		    if ( _conf_level && obs.getConfLevel() == _conf_level ) {
			_output << LQIO::DOM::Expat_Document::attribute( attribute, obs.getConfVariableName() );
		    } else {
			_output << LQIO::DOM::Expat_Document::attribute( attribute, obs.getVariableName() );
		    }
		}

	    std::ostream& _output;
	    const unsigned _conf_level;
	};

	class HasConfidenceObservation {
	public:
	    HasConfidenceObservation( unsigned int conf_level ) : _conf_level(conf_level) {}
	    bool operator()( const std::pair<const DocumentObject *,const Spex::ObservationInfo>& obs ) const { return obs.second.getConfLevel() == _conf_level; }
	private:
	    const unsigned int _conf_level;
	};
	
        void
        Expat_Document::serializeDOM( std::ostream& output ) const
        {
            if ( _document.hasConfidenceIntervals() ) {
                const_cast<ConfidenceIntervals *>(&_conf_95)->set_blocks( _document.getResultNumberOfBlocks() );
                const_cast<ConfidenceIntervals *>(&_conf_99)->set_blocks( _document.getResultNumberOfBlocks() );
            }
	    /* Output SPEX if present, otherwise output LQX (also if present) */
	    const_cast<Expat_Document *>(this)->_has_spex = Spex::numberOfInputVariables() > 0;
	    
            exportHeader( output );
	    if ( hasSPEX() && !_document.instantiated() ) {
		exportSPEXParameters( output );
	    }
            exportGeneral( output );

	    /* Export in model input order */

	    const std::map<unsigned,Entity *>& entities = _document.getEntities();
	    for_each( entities.begin(), entities.end(), ExportProcessor( output, *this ) );

            if ( !_document.instantiated() ) {
		if ( hasSPEX() ) {
		    exportSPEXResults( output );
		    exportSPEXConvergence( output );
		} else {
		    exportLQX( output );            // If translating, do this, otherwise, don't
		}
            }

            exportFooter( output );
	}

        void
        Expat_Document::exportHeader( std::ostream& output ) const
        {
            Filename base_name( _input_file_name );

            output << "<?xml version=\"1.0\"?>" << std::endl
		   << "<!-- " << Common_IO::svn_id() << " -->" << std::endl;

	    if ( LQIO::io_vars.lq_command_line.size() > 0 ) {
		output << comment( LQIO::io_vars.lq_command_line );
	    }
            output << start_element( Xlqn_model ) << attribute( Xname, base_name() )
                   << " description=\"" << LQIO::io_vars.lq_toolname << " " << io_vars.lq_version << " solution for model from: " << _input_file_name << ".\""
                   << " xmlns:xsi=\"" << XMLSchema_instance << "\" xsi:noNamespaceSchemaLocation=\"";

            const char * p = getenv( "LQN_SCHEMA_DIR" );
	    std::string schema_path;
            if ( p != 0 ) {
                schema_path = p;
                schema_path += "/";
            } else {
#if defined(__CYGWIN__)
                FILE *pPipe = popen( "cygpath -w /usr/local/share/lqns/", "rt" );
                if( pPipe != NULL ) {
                    char   psBuffer[512];
                    fgets( psBuffer, 512, pPipe );
                    pclose( pPipe );
                    psBuffer[strlen(psBuffer)-1]='\0';
                    schema_path = (string) psBuffer;
                }

#elif defined(WINNT)
                schema_path = "file:///C:/Program Files/LQN Solvers/";
#else
                schema_path = "/usr/local/share/lqns/";
#endif
            }
            schema_path += "lqn.xsd";

            output << schema_path;
            output << "\">" << std::endl;
        }

	void
	Expat_Document::exportSPEXParameters( std::ostream& output ) const
	{
	    const int precision = output.precision(10);
	    output << start_element( Xspex_parameters ) << ">" /* <![CDATA[" */ << std::endl;
	    const std::map<std::string,LQX::SyntaxTreeNode *>& input_variables = Spex::get_input_variables();
	    LQX::SyntaxTreeNode::setVariablePrefix( "$" );
	    for_each( input_variables.begin(), input_variables.end(), Spex::PrintInputVariable( output ) );
	    output /* << "]]>" << std::endl */ << end_element( Xspex_parameters ) << std::endl;
	    output.precision(precision);
	}
    
        /*
          <result-general conv-val="0" elapsed-time="0.000000:0.000000:0.000000" iterations="2" platform-info="Gregs-MacBook.local Darwin 10.3.0" solver-info="Gregs-MacBook.local Darwin 10.3.0" system-cpu-time="0.000000:0.000000:0.000000" user-cpu-time="0.000000:0.000000:0.000000" valid="YES"/>
        */

        void
        Expat_Document::exportGeneral( std::ostream& output ) const
        {
	    const std::vector<Spex::ObservationInfo> doc_vars = Spex::get_document_variables();
	    const bool complex_element = hasResults() || _document.hasPragmas() || doc_vars.size() > 0;

            output << start_element( Xsolver_parameters, complex_element )
                   << attribute( Xcomment, *_document.getModelComment() )
                   << attribute( Xconv_val, *_document.getModelConvergence() )
                   << attribute( Xit_limit, *_document.getModelIterationLimit() )
                   << attribute( Xunderrelax_coeff, *_document.getModelUnderrelaxationCoefficient() )
                   << attribute( Xprint_int, *_document.getModelPrintInterval() );
            if ( complex_element ) {
                output << ">" << std::endl;
		if ( doc_vars.size() > 0 ) {
                    output << simple_element( Xresult_observation );
		    for_each( doc_vars.begin(), doc_vars.end(), ExportObservation( output ) );
		    output << "/>" << std::endl;
		}
                if ( hasResults() ) {
		    const MVAStatistics& mva_info = _document.getResultMVAStatistics();
                    const bool has_mva_info = mva_info.getNumberOfSubmodels() > 0;
                    output << start_element( Xresult_general, has_mva_info )
			   << attribute( Xsolver_info, _document.getResultSolverInformation() )
                           << attribute( Xvalid, _document.getResultValid() ? "YES" : "NO" )
                           << attribute( Xconv_val_result, _document.getResultConvergenceValue() )
                           << attribute( Xiterations, _document.getResultIterations() )
                           << attribute( Xplatform_info, _document.getResultPlatformInformation() )
                           << time_attribute( Xuser_cpu_time, _document.getResultUserTime() )
                           << time_attribute( Xsystem_cpu_time, _document.getResultSysTime() )
                           << time_attribute( Xelapsed_time, _document.getResultElapsedTime() );
		    if ( _document.getResultMaxRSS() > 0 ) {
			output << attribute( Xmax_rss, _document.getResultMaxRSS() );
		    }
                    if ( has_mva_info ) {
                        output << ">" << std::endl;
                        output << simple_element( Xmva_info )
                               << attribute( Xsubmodels, mva_info.getNumberOfSubmodels() )
                               << attribute( Xcore, static_cast<double>(mva_info.getNumberOfCore()) )
                               << attribute( Xstep, mva_info.getNumberOfStep() )
                               << attribute( Xstep_squared, mva_info.getNumberOfStepSquared() )
                               << attribute( Xwait, mva_info.getNumberOfWait() )
                               << attribute( Xwait_squared, mva_info.getNumberOfWaitSquared() )
                               << attribute( Xfaults, mva_info.getNumberOfFaults() )
                               << "/>" << std::endl;
                    }
                    output << end_element( Xresult_general, has_mva_info ) << std::endl;
                }
                if ( _document.hasPragmas() ) {
                    const std::map<std::string,std::string>& pragmas = _document.getPragmaList();
                    for ( std::map<std::string,std::string>::const_iterator next_pragma = pragmas.begin(); next_pragma != pragmas.end(); ++next_pragma ) {
                        output << start_element( Xpragma, false )
                               << attribute( Xparam, next_pragma->first )
                               << attribute( Xvalue, next_pragma->second );
                        output << end_element( Xpragma, false ) << std::endl;
                    }
                }
            }
            output << end_element( Xsolver_parameters, complex_element ) << std::endl;
        }


        /*
         * <processor name="P1">
         * <xsd:attribute name="multiplicity" type="xsd:string" default="1"/>
         * <xsd:attribute name="speed-factor" type="xsd:decimal" default="1"/>
         * <xsd:attribute name="scheduling" type="SchedulingType" default="fcfs"/>
         * <xsd:attribute name="replication" type="xsd:string" default="1"/>
         * <xsd:attribute name="quantum" type="xsd:decimal"/>
         */

        void
        Expat_Document::exportProcessor( std::ostream& output, const Processor & processor ) const
        {
            const std::set<Task*>& task_list = processor.getTaskList();
            if ( !task_list.size() ) return;

            const scheduling_type scheduling = processor.getSchedulingType();
            output << start_element( Xprocessor )
                   << attribute( Xname, processor.getName() );
	    if ( processor.isInfinite() ) {
		output << attribute( Xscheduling, scheduling_label[SCHEDULE_DELAY].XML );        // see labels.cpp
		/* All other attributes don't matter for a delay server */
	    } else {
		output << attribute( Xscheduling, scheduling_label[scheduling].XML );            // see labels.cpp
		if ( processor.isMultiserver() ) {
		    output << attribute( Xmultiplicity, *processor.getCopies() );
		}
		if ( processor.hasRate() ) {
		    output << attribute( Xspeed_factor, *processor.getRate() );
		}
		if ( processor.hasQuantumScheduling() ) {
		    if ( processor.hasQuantum() ) {
			output << attribute( Xquantum, *processor.getQuantum() );
		    } else {
			output << attribute( Xquantum, 0. );
		    }
		}
	    }
            if ( processor.hasReplicas() ) {
                output << attribute( Xreplication, *processor.getReplicas() );
            }
            output << ">" << std::endl;

	    if ( processor.getComment().size() > 0 ) {
		output << comment( processor.getComment() );
	    }

	    if ( hasSPEX() && !_document.instantiated() ) {
		exportObservation( output, &processor );
	    }
            if ( hasResults() ) {
                if ( _document.hasConfidenceIntervals() ) {
                    output << start_element( Xresult_processor ) << attribute( Xutilization, processor.getResultUtilization() ) << ">" << std::endl;
                    output << simple_element( Xresult_conf_95 )  << attribute( Xutilization, _conf_95( processor.getResultUtilizationVariance() ) ) << "/>" << std::endl;
                    output << simple_element( Xresult_conf_99 )  << attribute( Xutilization, _conf_99( processor.getResultUtilizationVariance() ) ) << "/>" << std::endl;
                    output << end_element( Xresult_processor ) << std::endl;
                } else {
                    output << simple_element( Xresult_processor ) << attribute( Xutilization, processor.getResultUtilization() ) << "/>" << std::endl;
                }
            }

            const std::set<Group*>& group_list = processor.getGroupList();
	    for_each( group_list.begin(), group_list.end(), ExportGroup( output, *this ) );

	    for_each( task_list.begin(), task_list.end(), ExportTask( output, *this, true ) );

            output << end_element( Xprocessor ) << std::endl;
        }


        void
        Expat_Document::exportGroup( std::ostream& output, const Group & group ) const
        {
            output << start_element( Xgroup ) << attribute( Xname, group.getName() )
                   << attribute( Xshare, *group.getGroupShare() )
                   << attribute( Xcap, group.getCap() )
                   << ">" << std::endl;

	    if ( hasSPEX() && !_document.instantiated() ) {
		exportObservation( output, &group );
	    }
            if ( hasResults() ) {
                if ( _document.hasConfidenceIntervals() ) {
                    output << start_element( Xresult_group ) << attribute( Xutilization, group.getResultUtilization() ) << ">" << std::endl;
                    output << simple_element( Xresult_conf_95 )  << attribute( Xutilization, _conf_95( group.getResultUtilizationVariance() ) ) << "/>" << std::endl;
                    output << simple_element( Xresult_conf_99 )  << attribute( Xutilization, _conf_99( group.getResultUtilizationVariance() ) ) << "/>" << std::endl;
                    output << end_element( Xresult_group ) << std::endl;
                } else {
                    output << simple_element( Xresult_group ) << attribute( Xutilization, group.getResultUtilization() ) << "/>" << std::endl;
                }
            }


            const std::set<Task*>& task_list = group.getTaskList();
	    for_each( task_list.begin(), task_list.end(), ExportTask( output, *this ) );

            output << end_element( Xgroup ) << std::endl;
        }


        /*
         * <task name="UIF" multiplicity="10" scheduling="ref">
         *    <xsd:attribute name="multiplicity" type="xsd:string" default="1"/>
         *    <xsd:attribute name="replication" type="xsd:string" default="1"/>
         *    <xsd:attribute name="scheduling" type="TaskSchedulingType" default="n"/>
         *    <xsd:attribute name="think-time" type="xsd:string" default="0"/>
         *    <xsd:attribute name="priority" type="xsd:int"/>
         *    <xsd:attribute name="activity-graph" type="TaskOptionType"/>
         *    <!--OptionType to be defined Yes|NO-->
         */

        void
        Expat_Document::exportTask( std::ostream& output, const Task & task ) const
        {
            output << start_element( Xtask ) << attribute( Xname, task.getName() );
	    if ( task.isInfinite() ) {
		output << attribute( Xscheduling, scheduling_label[SCHEDULE_DELAY].XML );            // see lqio/labels.c
	    } else { 
		output << attribute( Xscheduling, scheduling_label[task.getSchedulingType()].XML );            // see lqio/labels.c
		if ( task.isMultiserver() ) {
		    output << attribute( Xmultiplicity, *task.getCopies() );
		}
	    }

            if ( task.getSchedulingType() == SCHEDULE_CUSTOMER && task.hasThinkTime() ) {
                output << attribute( Xthink_time, *task.getThinkTime() );
            }
            if ( task.hasPriority() ) {
                output << attribute( Xpriority, *task.getPriority() );
            }
            if ( task.hasQueueLength() ) {
                output << attribute( Xqueue_length, *task.getQueueLength() );
            }
            if ( task.hasReplicas() ) {
                output << attribute( Xreplication, *task.getReplicas() );
            }
            if ( task.getSchedulingType() == SCHEDULE_SEMAPHORE && dynamic_cast<const SemaphoreTask&>(task).getInitialState() == SemaphoreTask::INITIALLY_EMPTY ) {
                output << attribute( Xinitially, 0.0 );
            }

            output << ">" << std::endl;

	    if ( task.getComment().size() > 0 ) {
		output << comment( task.getComment() );
	    }

	    if ( hasSPEX() && !_document.instantiated() ) {
		exportObservation( output, &task );
	    }
            if ( hasResults() ) {
                if ( task.hasHistogram() ) {
                    exportHistogram( output, *task.getHistogram() );
                }

                const bool has_confidence = _document.hasConfidenceIntervals();
                output << start_element( Xresult_task, has_confidence )
                       << attribute( Xthroughput, task.getResultThroughput() )
                       << attribute( Xutilization, task.getResultUtilization() )
                       << task_phase_results( task, XphaseP_utilization, &Task::getResultPhasePUtilization )
                       << attribute( Xproc_utilization, task.getResultProcessorUtilization() );
		if ( _document.hasBottleneckStrength() ) {
		    output << attribute( Xbottleneck_strength, task.getResultBottleneckStrength() );
		}

                if ( dynamic_cast<const SemaphoreTask *>(&task) ) {
                    output << attribute( Xsemaphore_waiting, task.getResultHoldingTime() )
                           << attribute( Xsemaphore_waiting_variance, task.getResultVarianceHoldingTime() )
                           << attribute( Xsemaphore_utilization, task.getResultHoldingUtilization() );
                } else if ( dynamic_cast<const RWLockTask *>(&task) ) {
                    output << attribute( Xrwlock_reader_waiting, task.getResultReaderBlockedTime() )
                           << attribute( Xrwlock_reader_waiting_variance, task.getResultVarianceReaderBlockedTime() )
                           << attribute( Xrwlock_reader_holding, task.getResultReaderHoldingTime() )
                           << attribute( Xrwlock_reader_holding_variance, task.getResultVarianceReaderHoldingTime() )
                           << attribute( Xrwlock_reader_utilization, task.getResultReaderHoldingUtilization() )
                           << attribute( Xrwlock_writer_waiting, task.getResultWriterBlockedTime() )
                           << attribute( Xrwlock_writer_waiting_variance, task.getResultVarianceWriterBlockedTime() )
                           << attribute( Xrwlock_writer_holding, task.getResultWriterHoldingTime() )
                           << attribute( Xrwlock_writer_holding_variance, task.getResultVarianceWriterHoldingTime() )
                           << attribute( Xrwlock_writer_utilization, task.getResultWriterHoldingUtilization() );
                }

                if ( has_confidence ) {
                    output << ">" << std::endl;
                    output << simple_element( Xresult_conf_95 )
                           << attribute( Xthroughput, _conf_95( task.getResultThroughputVariance() ) )
                           << attribute( Xutilization, _conf_95( task.getResultUtilizationVariance() ) )
                           << task_phase_results( task, XphaseP_utilization, &Task::getResultPhasePUtilizationVariance, &_conf_95 )
                           << attribute( Xproc_utilization, _conf_95(task.getResultProcessorUtilizationVariance() ) );

                    if ( dynamic_cast<const SemaphoreTask *>(&task) ) {
                        output << attribute( Xsemaphore_waiting, _conf_95(task.getResultHoldingTimeVariance()) )
                               << attribute( Xsemaphore_waiting_variance, _conf_95(task.getResultVarianceHoldingTimeVariance()) )
                               << attribute( Xsemaphore_utilization, _conf_95(task.getResultHoldingUtilizationVariance()) );
                    } else if ( dynamic_cast<const RWLockTask *>(&task) ) {
                        output << attribute( Xrwlock_reader_waiting, _conf_95(task.getResultReaderBlockedTime()) )
                               << attribute( Xrwlock_reader_waiting_variance, _conf_95(task.getResultVarianceReaderBlockedTime()) )
                               << attribute( Xrwlock_reader_holding, _conf_95(task.getResultReaderHoldingTime()) )
                               << attribute( Xrwlock_reader_holding_variance, _conf_95(task.getResultVarianceReaderHoldingTime()) )
                               << attribute( Xrwlock_reader_utilization, _conf_95(task.getResultReaderHoldingUtilization()) )
                               << attribute( Xrwlock_writer_waiting, _conf_95(task.getResultWriterBlockedTime()) )
                               << attribute( Xrwlock_writer_waiting_variance, _conf_95(task.getResultVarianceWriterBlockedTime()) )
                               << attribute( Xrwlock_writer_holding, _conf_95(task.getResultWriterHoldingTime()) )
                               << attribute( Xrwlock_writer_holding_variance, _conf_95(task.getResultVarianceWriterHoldingTime()) )
                               << attribute( Xrwlock_writer_utilization, _conf_95(task.getResultWriterHoldingUtilization()) );
                    }

                    output << "/>" << std::endl;

                    output << simple_element( Xresult_conf_99 )
                           << attribute( Xthroughput, _conf_99( task.getResultThroughputVariance() ) )
                           << attribute( Xutilization, _conf_99( task.getResultUtilizationVariance() ) )
                           << task_phase_results( task, XphaseP_utilization, &Task::getResultPhasePUtilizationVariance, &_conf_99 )
                           << attribute( Xproc_utilization, _conf_99(task.getResultProcessorUtilizationVariance() ) );
                    if ( dynamic_cast<const SemaphoreTask *>(&task) ) {
                        output << attribute( Xsemaphore_waiting, _conf_99(task.getResultHoldingTimeVariance()) )
                               << attribute( Xsemaphore_waiting_variance, _conf_99(task.getResultVarianceHoldingTimeVariance()) )
                               << attribute( Xsemaphore_utilization, _conf_99(task.getResultHoldingUtilizationVariance()) );
                    } else if ( dynamic_cast<const RWLockTask *>(&task) ) {
                        output << attribute( Xrwlock_reader_waiting, _conf_99(task.getResultReaderBlockedTime()) )
                               << attribute( Xrwlock_reader_waiting_variance, _conf_99(task.getResultVarianceReaderBlockedTime()) )
                               << attribute( Xrwlock_reader_holding, _conf_99(task.getResultReaderHoldingTime()) )
                               << attribute( Xrwlock_reader_holding_variance, _conf_99(task.getResultVarianceReaderHoldingTime()) )
                               << attribute( Xrwlock_reader_utilization, _conf_99(task.getResultReaderHoldingUtilization()) )
                               << attribute( Xrwlock_writer_waiting, _conf_99(task.getResultWriterBlockedTime()) )
                               << attribute( Xrwlock_writer_waiting_variance, _conf_99(task.getResultVarianceWriterBlockedTime()) )
                               << attribute( Xrwlock_writer_holding, _conf_99(task.getResultWriterHoldingTime()) )
                               << attribute( Xrwlock_writer_holding_variance, _conf_99(task.getResultVarianceWriterHoldingTime()) )
                               << attribute( Xrwlock_writer_utilization, _conf_99(task.getResultWriterHoldingUtilization()) );
                    }

                    output << "/>" << std::endl;
                }

                output << end_element( Xresult_task, has_confidence ) << std::endl;
            }


            for ( std::map<const std::string, ExternalVariable *>::const_iterator next_fanin = task.getFanIns().begin(); next_fanin != task.getFanIns().end(); ++next_fanin ) {
                const std::string& src = next_fanin->first;
                const ExternalVariable * value = next_fanin->second;
                output << simple_element( Xfanin ) << attribute( Xsource, src )
                       << attribute( Xvalue, *value )
                       << "/>" << std::endl;
            }

            for ( std::map<const std::string, ExternalVariable *>::const_iterator next_fanout = task.getFanOuts().begin(); next_fanout != task.getFanOuts().end(); ++next_fanout ) {
                const std::string dst = next_fanout->first;
                const ExternalVariable * value = next_fanout->second;
                output << simple_element( Xfanout ) << attribute( Xdest, dst )
                       << attribute( Xvalue, *value )
                       << "/>" << std::endl;
            }

            const std::vector<Entry *>& entries = task.getEntryList();
	    for_each( entries.begin(), entries.end(), ExportEntry( output, *this ) );

            const std::map<std::string,DOM::Activity*>& activities = task.getActivities();
            if ( activities.size() > 0 ) {

                /* The activities */

                output << start_element( Xtask_activities ) << ">" << std::endl;
                for_each( activities.begin(), activities.end(), ExportActivity( output, *this ) );

                /* Precedence connections */

                const std::set<ActivityList*>& precedences = task.getActivityLists();
                for_each ( precedences.begin(), precedences.end(), ExportPrecedence( output, *this ) );

                /* Finally handle the list of replies. We find all of the reply entries for the activities, then swap the order. */
                std::map<const Entry *,std::vector<const Activity *> > entry_reply_list;
                for ( std::map<std::string,DOM::Activity*>::const_iterator next_activity = activities.begin(); next_activity != activities.end(); ++next_activity ) {
                    const Activity * activity = next_activity->second;

                    const std::vector<DOM::Entry*>& entry_list = activity->getReplyList();
                    for ( std::vector<DOM::Entry *>::const_iterator next_entry = entry_list.begin(); next_entry != entry_list.end(); ++next_entry ) {
                        std::vector<const Activity *>& activity_list = entry_reply_list[*next_entry];
                        activity_list.push_back( activity );
                    }
                }

                for ( std::map<const Entry *,std::vector<const Activity *> >::const_iterator next_entry = entry_reply_list.begin(); next_entry != entry_reply_list.end(); ++next_entry ) {
                    const Entry * entry = next_entry->first;
                    output << start_element( Xreply_entry ) << attribute( Xname, entry->getName() ) << ">" << std::endl;
                    const std::vector<const Activity *>& activity_list = next_entry->second;
                    for ( std::vector<const Activity *>::const_iterator next_activity = activity_list.begin(); next_activity != activity_list.end(); ++next_activity ) {
                        output << simple_element( Xreply_activity ) << attribute( Xname, (*next_activity)->getName() ) << "/>" << std::endl;
                    }
                    output << end_element( Xreply_entry ) << std::endl;
                }

                output << end_element( Xtask_activities ) << std::endl;
            }

            output << end_element( Xtask ) << std::endl;
        }


        /*
         * <entry name="user">
         *   <xsd:attribute name="name" type="xsd:string" use="required"/>
         *   <xsd:attribute name="type" use="required">  // can be PH1PH2, GRAPH (not supported), NONE (activity graph)
         *   <xsd:attribute name="open-arrival-rate" type="xsd:string"/>
         *   <xsd:attribute name="priority" type="xsd:int"/>
         *
         *   <entry-phase-activities>
         *     <activity name="foo" phase="N">
         *       <xsd:attribute name="name" type="xsd:string" use="required"/>
         *       <xsd:attribute name="bound-to-entry" type="xsd:string"/>
         *       <xsd:attribute name="host-demand-mean" type="xsd:decimal" use="required"/>
         *       <xsd:attribute name="host-demand-cvsq" type="xsd:decimal"/>
         *       <xsd:attribute name="think-time" type="xsd:decimal"/>
         *       <xsd:attribute name="max-service-time" type="xsd:decimal"/>
         *       <xsd:attribute name="call-order" type="CallOrderType"/>
         *
         */

        void
        Expat_Document::exportEntry( std::ostream& output, const Entry& entry ) const
        {
            const bool complex_element = entry.getStartActivity() == 0 		/* Phase1/2 type entry */
                || entry.getForwarding().size() > 0
		|| (entry.getStartActivity() && entry.hasHistogram())           /* Activity entry with a histogram */
                || hasResults();
            output << start_element( Xentry, complex_element )
                   << attribute( Xname, entry.getName() );

            if ( entry.isStandardEntry() ) {
                output << attribute( Xtype, XPH1PH2 );
            } else {
                output << attribute( Xtype, XNONE );
            }

            /* Parameters for an entry. */

            if ( entry.getOpenArrivalRate() ) {
                output << attribute( Xopen_arrival_rate, *entry.getOpenArrivalRate() );
            }
            if ( entry.getEntryPriority() ) {
                output << attribute( Xpriority, *entry.getEntryPriority() );
            }

            switch ( entry.getSemaphoreFlag() ) {
            case SEMAPHORE_SIGNAL: output << attribute( Xsemaphore, Xsignal ); break;
            case SEMAPHORE_WAIT:   output << attribute( Xsemaphore, Xwait ); break;
	    default: break;
            }

            switch ( entry.getRWLockFlag() ) {
            case RWLOCK_R_UNLOCK: output << attribute( Xrwlock, Xr_unlock ); break;
            case RWLOCK_R_LOCK:   output << attribute( Xrwlock, Xr_lock ); break;
            case RWLOCK_W_UNLOCK: output << attribute( Xrwlock, Xw_unlock ); break;
            case RWLOCK_W_LOCK:   output << attribute( Xrwlock, Xw_lock ); break;
	    default: break;
            }

            if ( complex_element ) {
                output << ">" << std::endl;

		if ( entry.getComment().size() > 0 ) {
		    output << comment( entry.getComment() );
		}

		if ( hasSPEX() && !_document.instantiated() ) {
		    exportObservation( output, &entry );
		}
                if ( hasResults() ) {
		    if ( entry.getStartActivity() && entry.hasHistogram() ) {
			for ( unsigned p = 1; p <= Phase::MAX_PHASE; ++p ) {
			    if ( entry.hasHistogramForPhase(p) ) {
				exportHistogram( output, *entry.getHistogramForPhase(p), p );
			    }
			}
		    }

                    const bool has_confidence = _document.hasConfidenceIntervals();

                    output << start_element( Xresult_entry, has_confidence )
                           << attribute( Xutilization, entry.getResultUtilization() )
                           << attribute( Xthroughput, entry.getResultThroughput() )
                           << attribute( Xsquared_coeff_variation, entry.getResultSquaredCoeffVariation() )
                           << attribute( Xproc_utilization, entry.getResultProcessorUtilization() );
		    if ( entry.hasResultsForThroughputBound() ) {
			output << attribute( Xthroughput_bound, entry.getResultThroughputBound() );
		    }
                    if ( entry.hasResultsForOpenWait() ) {
                        output << attribute( Xopen_wait_time, entry.getResultWaitingTime() );
                    }

                    /* Results for activity entries. */
                    if ( entry.getStartActivity() != 0 ) {
                        output << entry_phase_results( entry, XphaseP_service_time, &Entry::getResultPhasePServiceTime )
                               << entry_phase_results( entry, XphaseP_service_time_variance, &Entry::getResultPhasePVarianceServiceTime )
                               << entry_phase_results( entry, XphaseP_proc_waiting, &Entry::getResultPhasePProcessorWaiting )
                               << entry_phase_results( entry, XphaseP_utilization, &Entry::getResultPhasePUtilization );
                    }

                    if ( has_confidence ) {
                        output << ">" << std::endl;

                        output << simple_element( Xresult_conf_95 )
                               << attribute( Xutilization, _conf_95( entry.getResultUtilizationVariance() ) )
                               << attribute( Xthroughput, _conf_95( entry.getResultThroughputVariance() ) )
                               << attribute( Xsquared_coeff_variation, _conf_95( entry.getResultSquaredCoeffVariationVariance() ) )
                               << attribute( Xproc_utilization, _conf_95( entry.getResultProcessorUtilizationVariance() ) );
                        if ( entry.hasResultsForOpenWait() ) {
                            output << attribute( Xopen_wait_time, _conf_95( entry.getResultWaitingTimeVariance() ) );
                        }

                        /* Results for activity entries. */
                        if ( !entry.isStandardEntry() ) {
                            output << entry_phase_results( entry, XphaseP_service_time, &Entry::getResultPhasePServiceTimeVariance, &_conf_95 )
                                   << entry_phase_results( entry, XphaseP_service_time_variance, &Entry::getResultPhasePVarianceServiceTimeVariance, &_conf_95 )
                                   << entry_phase_results( entry, XphaseP_proc_waiting, &Entry::getResultPhasePProcessorWaitingVariance, &_conf_95 )
                                   << entry_phase_results( entry, XphaseP_utilization, &Entry::getResultPhasePUtilizationVariance, &_conf_95 );
                        }
                        output << "/>" << std::endl;

                        output << simple_element( Xresult_conf_99 )
                               << attribute( Xutilization, _conf_99( entry.getResultUtilizationVariance() ) )
                               << attribute( Xthroughput, _conf_99( entry.getResultThroughputVariance() ) )
                               << attribute( Xsquared_coeff_variation, _conf_99( entry.getResultSquaredCoeffVariationVariance() ) )
                               << attribute( Xproc_utilization, _conf_99( entry.getResultProcessorUtilizationVariance() ) );
                        if ( entry.hasResultsForOpenWait() ) {
                            output << attribute( Xopen_wait_time, _conf_99( entry.getResultWaitingTimeVariance() ) );
                        }

                        /* Results for activity entries. */
                        if ( !entry.isStandardEntry() ) {
                            output << entry_phase_results( entry, XphaseP_service_time, &Entry::getResultPhasePServiceTimeVariance, &_conf_99 )
                                   << entry_phase_results( entry, XphaseP_service_time_variance, &Entry::getResultPhasePVarianceServiceTimeVariance, &_conf_99 )
                                   << entry_phase_results( entry, XphaseP_proc_waiting, &Entry::getResultPhasePProcessorWaitingVariance, &_conf_99 )
                                   << entry_phase_results( entry, XphaseP_utilization, &Entry::getResultPhasePUtilizationVariance, &_conf_99 );
                        }
                        output << "/>" << std::endl;
                    }

                    output << end_element( Xresult_entry, has_confidence ) << std::endl;
                }

                const std::vector<Call*>& forwarding = entry.getForwarding();
                for_each( forwarding.begin(), forwarding.end(), ExportCall( output, *this ) );

                if ( entry.isStandardEntry() ) {
                    output << start_element( Xentry_phase_activities ) << ">" << std::endl;

                    const std::map<unsigned, Phase*>& phases = entry.getPhaseList();
                    for_each ( phases.begin(), phases.end(), ExportPhase( output, *this ) );
                    output << end_element( Xentry_phase_activities ) << std::endl;
                }
            } /* if ( complex_element ) */

            output << end_element( Xentry, complex_element ) << std::endl;

	    if ( !complex_element && entry.getComment().size() > 0 ) {
		output << comment( entry.getComment() );
	    }
        }


        /*
         * <activity name="user_ph1" ...>
         *   <xsd:attribute name="name" type="xsd:string" use="required"/>
         *   <xsd:attribute name="bound-to-entry" type="xsd:string"/>  // N/A for phases
         *   <xsd:attribute name="host-demand-mean" type="xsd:decimal" use="required"/>
         *   <xsd:attribute name="host-demand-cvsq" type="xsd:decimal"/>
         *   <xsd:attribute name="think-time" type="xsd:decimal"/>
         *   <xsd:attribute name="max-service-time" type="xsd:decimal"/>
         *   <xsd:attribute name="call-order" type="CallOrderType"/>
         *   <xsd:attribute name="phase">
         *      <synch-call dest="e1" calls-mean="5"/>
         *      <asynch-call dest="e2" calls-mean="20"/>
         * </activity>
         */

        void
        Expat_Document::exportActivity( std::ostream& output, const Phase &phase, const unsigned p ) const
        {
            const std::vector<Call*>& calls = phase.getCalls();
            const bool complex_element = calls.size() > 0 || hasResults() || phase.hasHistogram() || hasSPEX();
            output << start_element( Xactivity, complex_element )
                   << attribute( Xname, phase.getName() );

            const Activity * activity = dynamic_cast<const Activity *>(&phase);
            if ( activity ) {
                if ( activity->isStartActivity() ) {
                    output << attribute ( Xbound_to_entry, activity->getSourceEntry()->getName() );
                }
            } else {
                output << attribute( Xphase, p );
            }

            if ( phase.getServiceTime() ) {
                output << attribute( Xhost_demand_mean, *phase.getServiceTime() );
            }
            if ( phase.isNonExponential() ) {
                output << attribute( Xhost_demand_cvsq, *phase.getCoeffOfVariationSquared() );
            }
            if ( phase.hasThinkTime() ) {
                output << attribute( Xthink_time,  *phase.getThinkTime() );
            }
            if ( phase.hasMaxServiceTimeExceeded() ) {
                output << attribute( Xmax_service_time, phase.getMaxServiceTime() );
            }
            if ( phase.hasDeterministicCalls() ) {
                output << attribute( Xcall_order, XDETERMINISTIC );
            }

            if ( complex_element ) {
                output << ">" << std::endl;

		if ( phase.getComment().size() > 0 ) {
		    output << comment( phase.getComment() );
		}

		if ( hasSPEX() && !_document.instantiated() ) {
		    exportObservation( output, &phase );
		}
                if ( hasResults() ) {
		    const bool has_variance = phase.getResultVarianceServiceTime() > 0.0;
                    if ( phase.hasHistogram() ) {
                        exportHistogram( output, *phase.getHistogram() );
                    }

                    const bool has_confidence = _document.hasConfidenceIntervals();

                    output << start_element( Xresult_activity, has_confidence )
                           << attribute( Xproc_waiting, phase.getResultProcessorWaiting() )
                           << attribute( Xservice_time, phase.getResultServiceTime() )
			   << attribute( Xutilization, phase.getResultUtilization() );

		    if ( has_variance ) {
			output << attribute( Xservice_time_variance, phase.getResultVarianceServiceTime() );	// optional attribute.
		    }
		    if ( dynamic_cast<const Activity *>(&phase) ) {
			output << attribute( Xthroughput, phase.getResultThroughput() )
			       << attribute( Xproc_utilization, phase.getResultProcessorUtilization() );
		    }
                    if ( phase.hasMaxServiceTimeExceeded() ) {
                        output << attribute( Xprob_exceed_max_service_time, phase.getResultMaxServiceTimeExceeded() );
                    }

                    if ( has_confidence ) {
                        output << ">" << std::endl;
                        output << simple_element( Xresult_conf_95 )
                               << attribute( Xproc_waiting, _conf_95( phase.getResultProcessorWaitingVariance() ) )
                               << attribute( Xservice_time, _conf_95( phase.getResultServiceTimeVariance() ) )
                               << attribute( Xutilization, _conf_95( phase.getResultUtilizationVariance() ) );
			if ( has_variance ) {
			    output << attribute( Xservice_time_variance, _conf_95( phase.getResultVarianceServiceTimeVariance() ) );
			}
			if ( dynamic_cast<const Activity *>(&phase) ) {
			    output << attribute( Xthroughput, _conf_95( phase.getResultThroughputVariance() ) )
				   << attribute( Xproc_utilization, _conf_95( phase.getResultProcessorUtilizationVariance() ) );
			}
                        if ( phase.hasMaxServiceTimeExceeded() ) {
                            output << attribute( Xprob_exceed_max_service_time, _conf_95( phase.getResultMaxServiceTimeExceededVariance() ) );
                        }
                        output << "/>" << std::endl;

                        output << simple_element( Xresult_conf_99 )
                               << attribute( Xproc_waiting, _conf_99( phase.getResultProcessorWaitingVariance() ) )
                               << attribute( Xservice_time, _conf_99( phase.getResultServiceTimeVariance() ) )
                               << attribute( Xutilization, _conf_99( phase.getResultUtilizationVariance() ) );
			if ( has_variance ) {
			    output << attribute( Xservice_time_variance, _conf_99( phase.getResultVarianceServiceTimeVariance() ) );
			}
			if ( dynamic_cast<const Activity *>(&phase) ) {
			    output << attribute( Xthroughput, _conf_99( phase.getResultThroughputVariance() ) )
				   << attribute( Xproc_utilization, _conf_95( phase.getResultProcessorUtilizationVariance() ) );
			}
                        if ( phase.hasMaxServiceTimeExceeded() ) {
                            output << attribute( Xprob_exceed_max_service_time, _conf_99( phase.getResultMaxServiceTimeExceededVariance() ) );
                        }
                        output << "/>" << std::endl;
                    }

                    output << end_element( Xresult_activity, has_confidence ) << std::endl;
                }

		for_each( calls.begin(), calls.end(), ExportCall( output, *this ) );
            } /* if ( complex_element ) */

            output << end_element( Xactivity, complex_element ) << std::endl;

	    if ( !complex_element && phase.getComment().size() > 0 ) {
		output << comment( phase.getComment() );
	    }
        }



        const XML_Char * Expat_Document::precedence_type_table[ActivityList::REPEAT_ACTIVITY_LIST+1];

        void
        Expat_Document::ExportPrecedence::operator()( const ActivityList * activity_list ) const {
	    /* look for the 'pre' side.  Do the post side based on the pre-side */
	    if ( activity_list->isForkList() ) return;
	    _output << _self.start_element( Xprecedence ) << ">" << std::endl;
	    _self.exportPrecedence( _output, *activity_list );
	    if ( activity_list->getNext() ) {
		_self.exportPrecedence( _output, *(activity_list->getNext()) );
	    }
	    _output << _self.end_element( Xprecedence ) << std::endl;
	}

        void
        Expat_Document::exportPrecedence( std::ostream& output, const ActivityList& activity_list ) const
        {
            output << start_element( precedence_type_table[activity_list.getListType()] );
            const AndJoinActivityList * join_list = dynamic_cast<const AndJoinActivityList *>(&activity_list);
            if ( join_list && hasResults() ) {
                output  << ">" << std::endl;
                if ( join_list->hasHistogram() ) {
                    exportHistogram( output, *join_list->getHistogram() );
                }
                bool has_confidence = _document.hasConfidenceIntervals();

                output << start_element( Xresult_join_delay, has_confidence )
                       << attribute( Xjoin_waiting, join_list->getResultJoinDelay() )
                       << attribute( Xjoin_variance, join_list->getResultVarianceJoinDelay() );
                if ( has_confidence ) {
                    output << ">" << std::endl;
                    output << simple_element( Xresult_conf_95 )
                           << attribute( Xjoin_waiting, _conf_95( join_list->getResultJoinDelayVariance() ) )
                           << attribute( Xjoin_variance, _conf_95( join_list->getResultVarianceJoinDelayVariance() ) )
                           << "/>" << std::endl;
                    output << simple_element( Xresult_conf_99 )
                           << attribute( Xjoin_waiting, _conf_99( join_list->getResultJoinDelayVariance() ) )
                           << attribute( Xjoin_variance, _conf_99( join_list->getResultVarianceJoinDelayVariance() ) )
                           << "/>" << std::endl;
                }
                output << end_element( Xresult_join_delay, has_confidence ) << std::endl;
            } else if ( activity_list.getListType() == ActivityList::REPEAT_ACTIVITY_LIST ) {
                const std::vector<const Activity*>& list = activity_list.getList();
                for ( std::vector<const Activity*>::const_iterator next_activity = list.begin(); next_activity != list.end(); ++next_activity ) {
                    const Activity * activity = *next_activity;
                    if ( activity_list.getParameter( activity ) == NULL ) {
                        output << attribute( Xend, activity->getName() );
                    }
                }
                output  << ">" << std::endl;
            } else {
                output  << ">" << std::endl;
            }

            const std::vector<const Activity*>& list = activity_list.getList();
            for ( std::vector<const Activity*>::const_iterator next_activity = list.begin(); next_activity != list.end(); ++next_activity ) {
                const Activity * activity = *next_activity;
                const ExternalVariable * value = NULL;

                switch ( activity_list.getListType() ) {
                case ActivityList::REPEAT_ACTIVITY_LIST:
                case ActivityList::OR_FORK_ACTIVITY_LIST:
                    value = activity_list.getParameter( activity );
                    if ( !value ) continue;             /* usually the end list value for loops */
                    break;
		default: break;
                }

                output << simple_element( Xactivity )
                       << attribute( Xname, activity->getName() );

                switch ( activity_list.getListType() ) {
                case ActivityList::OR_FORK_ACTIVITY_LIST:
                    output << attribute( Xprob, *value );
                    break;

                case ActivityList::REPEAT_ACTIVITY_LIST:
                    output << attribute( Xcount, *value );
                    break;
		default: break;
                }
                output << "/>" << std::endl;
            }
            output << end_element( precedence_type_table[activity_list.getListType()] ) << std::endl;
        }


        /*
         * <synch-call dest="e1" calls-mean="5"/>
         * <synch-call dest="e2" calls-mean="20"/>
         */

        Expat_Document::call_type_table_t Expat_Document::call_type_table[] = {
            { 0, 0 },
            { Xasynch_call, Xcalls_mean },
            { Xsynch_call,  Xcalls_mean },
            { Xforwarding,  Xprob }
        };

        void
        Expat_Document::exportCall( std::ostream& output, const Call & call ) const
        {
            const bool complex_type = hasResults() || call.hasHistogram();
            output << start_element( call_type_table[call.getCallType()].element, complex_type )
                   << attribute( Xdest, call.getDestinationEntry()->getName() );
            if ( call.getCallMean() ) {
                output << attribute( call_type_table[call.getCallType()].attribute, *call.getCallMean() );
            }

            if ( complex_type ) {
                output << ">" << std::endl;

		if ( hasSPEX() && !_document.instantiated() ) {
		    exportObservation( output, &call );
		}
		if ( hasResults() ) {
		    const bool has_confidence = _document.hasConfidenceIntervals();
		    output << start_element( Xresult_call, has_confidence )
			   << attribute( Xwaiting, call.getResultWaitingTime() );
		    if ( call.hasResultVarianceWaitingTime() ) {
			output << attribute( Xwaiting_variance, call.getResultVarianceWaitingTime() );
		    }
		    if ( call.hasResultDropProbability() ) {
			output << attribute( Xloss_probability, call.getResultDropProbability() );
		    }

		    if ( has_confidence ) {
			output << ">" << std::endl;
			output << simple_element( Xresult_conf_95 )
			       << attribute( Xwaiting, _conf_95( call.getResultWaitingTimeVariance() ) );
			if ( call.hasResultVarianceWaitingTime() ) {
			    output << attribute( Xwaiting_variance, _conf_95( call.getResultVarianceWaitingTimeVariance() ) );
			}
			if ( call.hasResultDropProbability() ) {
			    output << attribute( Xloss_probability, _conf_95( call.getResultDropProbabilityVariance() ) );
			}
			output << "/>" << std::endl;
			output << simple_element( Xresult_conf_99 )
			       << attribute( Xwaiting, _conf_99( call.getResultWaitingTimeVariance() ) );
			if ( call.hasResultVarianceWaitingTime() ) {
			    output << attribute( Xwaiting_variance, _conf_99( call.getResultVarianceWaitingTimeVariance() ) );
			}
			if ( call.hasResultDropProbability() ) {
			    output << attribute( Xloss_probability, _conf_99( call.getResultDropProbabilityVariance() ) );
			}
			output << "/>" << std::endl;
		    }
		    output << end_element( Xresult_call, has_confidence ) << std::endl;
		}

		if ( call.hasHistogram() ) {
                    exportHistogram( output, *call.getHistogram() );
		}
            }

            output << end_element( call_type_table[call.getCallType()].element, complex_type ) << std::endl;
        }



        void
        Expat_Document::exportHistogram( std::ostream& output, const Histogram& histogram, const unsigned phase ) const
        {
            if ( histogram.getBins() == 0 ) return;
	    const bool complex_type = histogram.hasResults();
	    const XML_Char * element_name = histogram.getHistogramType() == Histogram::CONTINUOUS ? Xservice_time_distribution : Xqueue_length_distribution;

            output << start_element( element_name, complex_type )
                   << attribute( Xnumber_bins, histogram.getBins() )
                   << attribute( Xmin, histogram.getMin() )
                   << attribute( Xmax, histogram.getMax() );
	    if ( phase > 0 ) {
		output << attribute( Xphase, phase );
	    }
	    if ( complex_type ) {
		output << ">" << std::endl;
		for ( unsigned int i = 0; i < histogram.getBins() + 2; ++i ) {
		    const XML_Char * bin_name;
		    if ( i == 0 ) {
			if ( histogram.getBinBegin(i) == histogram.getBinEnd(i) ) continue;         /* Nothing to see here. */
			bin_name = Xunderflow_bin;
		    } else if ( i == histogram.getOverflowIndex() ) {
			if ( histogram.getBinMean(i) == 0 ) break;		/* No point.. */
			bin_name = Xoverflow_bin;
		    } else {
			bin_name = Xhistogram_bin;
		    }
		    output << start_element( bin_name, false )
			   << attribute( Xbegin, histogram.getBinBegin(i) )
			   << attribute( Xend,   histogram.getBinEnd(i)  )
			   << attribute( Xprob,  histogram.getBinMean(i) );
		    const double variance = histogram.getBinVariance(i);
		    if ( variance > 0 && _document.hasConfidenceIntervals() ) {
			output << attribute( Xconf_95, _conf_95( variance ) )
			       << attribute( Xconf_99, _conf_99( variance ) );
		    }
		    output << end_element( bin_name, false ) << std::endl;
		}
	    }

            output << end_element( element_name, complex_type ) << std::endl;
        }


        void
	Expat_Document::exportObservation( std::ostream& output, const DocumentObject * object ) const
	{
	    std::pair<Spex::obs_var_tab_t::const_iterator, Spex::obs_var_tab_t::const_iterator> range = Spex::get_observations().equal_range( object );
    	    if ( range.first != range.second ) {
		const bool has_95 = find_if( range.first, range.second, HasConfidenceObservation( 95 ) ) != range.second;
		const bool has_99 = find_if( range.first, range.second, HasConfidenceObservation( 99 ) ) != range.second;
		const bool complex_type = has_95 || has_99;
		output << start_element( Xresult_observation, complex_type );
		for_each( range.first, range.second, ExportObservation( output ) );
		if ( complex_type ) {
		    output << ">" << std::endl;
		    if ( has_95 ) {
			output << start_element( Xconf_95, false );
			for_each( range.first, range.second, ExportObservation( output, 95 ) );
			output << "/>" << std::endl;
		    }
		    if ( has_99 ) {
			output << start_element( Xconf_99, false );
			for_each( range.first, range.second, ExportObservation( output, 99 ) );
			output << "/>" << std::endl;
		    }
		}
		output << end_element( Xresult_observation, complex_type ) << std::endl;
	    }
	}
	
        void
        Expat_Document::exportLQX( std::ostream& output ) const
        {
	    LQX::Program * program = _document.getLQXProgram();
	    const std::string& program_text = _document.getLQXProgramText();
            if ( program_text.size() > 0 || program != NULL ) {
		const int precision = output.precision(10);
                output << start_element( Xlqx ) << "><![CDATA[" << std::endl;
		if ( program_text.size() > 0 ) {
		    output << program_text;
		} else {
		    program->print( output );
		}
		output << "]]>" << std::endl << end_element( Xlqx ) << std::endl;
		output.precision(precision);
	    }
        }


	void
	Expat_Document::exportSPEXResults( std::ostream& output ) const
	{
	    const std::vector<Spex::var_name_and_expr>& results = Spex::get_result_variables();
	    if ( results.size() > 0 ) {
		const int precision = output.precision(10);
		output << start_element( Xspex_results ) << ">" /* <![CDATA[" */ << std::endl;
		LQX::SyntaxTreeNode::setVariablePrefix( "$" );
		for_each( results.begin(), results.end(), Spex::PrintResultVariable( output ) );
		output /* << "]]>" << std::endl */ << end_element( Xspex_results ) << std::endl;
		output.precision(precision);
	    }
	}
    
	void
	Expat_Document::exportSPEXConvergence( std::ostream& output ) const
	{
	}
    
        void
        Expat_Document::exportFooter( std::ostream& output ) const
        {
            output << end_element( Xlqn_model ) << std::endl;
        }

        bool
        Expat_Document::parse_stack_t::operator==( const XML_Char * str ) const
        {
            return element == str;
        }

        /*
         * Print out results of the form phase1-utilization="value"* ...
         * If the ConfidenceInvervals object is present, its
         * operator()() function is used.
         */

        /* static */ std::ostream&
        Expat_Document::printEntryPhaseResults( std::ostream& output, const Entry & entry, const XML_Char ** attributes, const doubleEntryFunc func, const ConfidenceIntervals * conf )
        {
            for ( unsigned p = 1; p <= Phase::MAX_PHASE; ++p ) {
                if ( !entry.hasResultsForPhase(p) ) continue;
                const double value = (entry.*func)(p);
                if ( value > 0.0 ) {
                    output << attribute( attributes[p-1], conf ? (*conf)(value) : value );
                }
            }
            return output;
        }


        /*
         * Print out results of the form phase1-utilization="value"* ...
         * If the ConfidenceInvervals object is present, its
         * operator()() function is used.
         */

        /* static */ std::ostream&
        Expat_Document::printTaskPhaseResults( std::ostream& output, const Task & task, const XML_Char ** attributes, const doubleTaskFunc func, const ConfidenceIntervals * conf )
        {
            for ( unsigned p = 1; p <= task.getResultPhaseCount(); ++p ) {
                const double value = (task.*func)(p);
                if ( value > 0.0 ) {
                    output << attribute( attributes[p-1], conf ? (*conf)(value) : value );
                }
            }
            return output;
        }


        /*
         * Results for most of the elements of an lqn-model are of a common type in the schema.  This table is used to
         * invoke the appropriate function.  The function is implemented in dom objects that support the result.
         */

        void
        Expat_Document::init_tables()
        {
            if ( result_table.size() != 0 ) return;		/* Done already */

	    Common_IO::init_tables();

            activity_table.insert(Xphase);
            activity_table.insert(Xname);
            activity_table.insert(Xbound_to_entry);
            activity_table.insert(Xhost_demand_mean);
            activity_table.insert(Xhost_demand_cvsq);
            activity_table.insert(Xthink_time);
            activity_table.insert(Xcall_order);
            activity_table.insert(Xmax_service_time);

            call_table.insert(Xdest);
            call_table.insert(Xcalls_mean);
            call_table.insert(Xprob);

            entry_table.insert(Xname);
            entry_table.insert(Xtype);
            entry_table.insert(Xpriority);
            entry_table.insert(Xopen_arrival_rate);
            entry_table.insert(Xsemaphore);
            entry_table.insert(Xrwlock);

	    escape_table['&']  = "&amp;";
	    escape_table['\''] = "&apos;";
	    escape_table['>']  = "&gt;";
	    escape_table['<']  = "&lt;";
	    escape_table['"']  = "&qout;";

            group_table.insert(Xname);
            group_table.insert(Xcap);
            group_table.insert(Xshare);

            histogram_table.insert(Xmin);
            histogram_table.insert(Xmax);
//          histogram_table.insert(Xbin_size);
            histogram_table.insert(Xnumber_bins);
            histogram_table.insert(Xphase);
//          histogram_table.insert(Xmean);
//          histogram_table.insert(Xstd_dev);
//          histogram_table.insert(Xskew);
//          histogram_table.insert(Xkurtosis);

            model_table.insert("description");
            model_table.insert("lqncore-schema-version");
            model_table.insert("lqn-schema-version");
            model_table.insert(Xname);
            model_table.insert(Xxml_debug);

            observation_table[Xelapsed_time] =                   observation_table_t( KEY_ELAPSED_TIME );
            observation_table[Xiterations] =                     observation_table_t( KEY_ITERATIONS );
            observation_table[Xphase] =                          observation_table_t();
            observation_table[XphaseP_proc_waiting[0]] =         observation_table_t( KEY_PROCESSOR_WAITING, 1 );
            observation_table[XphaseP_proc_waiting[1]] =         observation_table_t( KEY_PROCESSOR_WAITING, 2 );
            observation_table[XphaseP_proc_waiting[2]] =         observation_table_t( KEY_PROCESSOR_WAITING, 3 );
            observation_table[XphaseP_service_time[0]] =         observation_table_t( KEY_SERVICE_TIME, 1 );
            observation_table[XphaseP_service_time[1]] =         observation_table_t( KEY_SERVICE_TIME, 2 );
            observation_table[XphaseP_service_time[2]] =         observation_table_t( KEY_SERVICE_TIME, 3 );
            observation_table[XphaseP_service_time_variance[0]]= observation_table_t( KEY_VARIANCE, 1 );
            observation_table[XphaseP_service_time_variance[1]]= observation_table_t( KEY_VARIANCE, 2 );
            observation_table[XphaseP_service_time_variance[2]]= observation_table_t( KEY_VARIANCE, 3 );
            observation_table[XphaseP_utilization[0]] =          observation_table_t( KEY_UTILIZATION, 1 );
            observation_table[XphaseP_utilization[1]] =          observation_table_t( KEY_UTILIZATION, 2 );
            observation_table[XphaseP_utilization[2]] =          observation_table_t( KEY_UTILIZATION, 3 );
            observation_table[Xproc_utilization] =        	 observation_table_t( KEY_PROCESSOR_UTILIZATION );
            observation_table[Xproc_waiting] =            	 observation_table_t( KEY_PROCESSOR_WAITING );
            observation_table[Xservice_time] =            	 observation_table_t( KEY_SERVICE_TIME );
            observation_table[Xservice_time_variance] =          observation_table_t( KEY_VARIANCE );
            observation_table[Xsystem_cpu_time] =                observation_table_t( KEY_SYSTEM_TIME );
            observation_table[Xthroughput] =                     observation_table_t( KEY_THROUGHPUT );
            observation_table[Xthroughput_bound] =               observation_table_t( KEY_THROUGHPUT_BOUND );
            observation_table[Xuser_cpu_time] =                  observation_table_t( KEY_USER_TIME );
            observation_table[Xutilization] =                    observation_table_t( KEY_UTILIZATION );
            observation_table[Xwaiting] =                 	 observation_table_t( KEY_WAITING );
            observation_table[Xwaiting_variance] =        	 observation_table_t( KEY_WAITING_VARIANCE );

            parameter_table.insert(Xcomment);
            parameter_table.insert(Xconv_val);
            parameter_table.insert(Xit_limit);
            parameter_table.insert(Xprint_int);
            parameter_table.insert(Xunderrelax_coeff);

            processor_table.insert(Xname);
            processor_table.insert(Xscheduling);
            processor_table.insert(Xquantum);
            processor_table.insert(Xmultiplicity);
            processor_table.insert(Xreplication);
            processor_table.insert(Xspeed_factor);

            precedence_table[Xpre] =       ActivityList::JOIN_ACTIVITY_LIST;
            precedence_table[Xpre_or] =    ActivityList::OR_JOIN_ACTIVITY_LIST;
            precedence_table[Xpre_and] =   ActivityList::AND_JOIN_ACTIVITY_LIST;
            precedence_table[Xpost] =      ActivityList::FORK_ACTIVITY_LIST;
            precedence_table[Xpost_or] =   ActivityList::OR_FORK_ACTIVITY_LIST;
            precedence_table[Xpost_and] =  ActivityList::AND_FORK_ACTIVITY_LIST;
            precedence_table[Xpost_loop] = ActivityList::REPEAT_ACTIVITY_LIST;

            precedence_type_table[ActivityList::JOIN_ACTIVITY_LIST] =     Xpre;
            precedence_type_table[ActivityList::OR_JOIN_ACTIVITY_LIST] =  Xpre_or;
            precedence_type_table[ActivityList::AND_JOIN_ACTIVITY_LIST] = Xpre_and;
            precedence_type_table[ActivityList::FORK_ACTIVITY_LIST] =     Xpost;
            precedence_type_table[ActivityList::OR_FORK_ACTIVITY_LIST] =  Xpost_or;
            precedence_type_table[ActivityList::AND_FORK_ACTIVITY_LIST] = Xpost_and;
            precedence_type_table[ActivityList::REPEAT_ACTIVITY_LIST] =   Xpost_loop;

	    result_table[Xbottleneck_strength] =	    result_table_t( &DocumentObject::setResultBottleneckStrength,        0 );
            result_table[Xjoin_variance] =                  result_table_t( &DocumentObject::setResultVarianceJoinDelay,         &DocumentObject::setResultVarianceJoinDelayVariance );
            result_table[Xjoin_waiting] =                   result_table_t( &DocumentObject::setResultJoinDelay,                 &DocumentObject::setResultJoinDelayVariance );
            result_table[Xloss_probability] =               result_table_t( &DocumentObject::setResultDropProbability,           &DocumentObject::setResultDropProbabilityVariance );
            result_table[Xopen_wait_time] =                 result_table_t( &DocumentObject::setResultWaitingTime,               &DocumentObject::setResultWaitingTimeVariance );
            result_table[XphaseP_proc_waiting[0]] =         result_table_t( &DocumentObject::setResultPhase1ProcessorWaiting,    &DocumentObject::setResultPhase1ProcessorWaitingVariance );
            result_table[XphaseP_proc_waiting[1]] =         result_table_t( &DocumentObject::setResultPhase2ProcessorWaiting,    &DocumentObject::setResultPhase2ProcessorWaitingVariance );
            result_table[XphaseP_proc_waiting[2]] =         result_table_t( &DocumentObject::setResultPhase3ProcessorWaiting,    &DocumentObject::setResultPhase3ProcessorWaitingVariance );
            result_table[XphaseP_service_time[0]] =         result_table_t( &DocumentObject::setResultPhase1ServiceTime,         &DocumentObject::setResultPhase1ServiceTimeVariance );
            result_table[XphaseP_service_time[1]] =         result_table_t( &DocumentObject::setResultPhase2ServiceTime,         &DocumentObject::setResultPhase2ServiceTimeVariance );
            result_table[XphaseP_service_time[2]] =         result_table_t( &DocumentObject::setResultPhase3ServiceTime,         &DocumentObject::setResultPhase3ServiceTimeVariance );
            result_table[XphaseP_service_time_variance[0]]= result_table_t( &DocumentObject::setResultPhase1VarianceServiceTime, &DocumentObject::setResultPhase1VarianceServiceTimeVariance );
            result_table[XphaseP_service_time_variance[1]]= result_table_t( &DocumentObject::setResultPhase2VarianceServiceTime, &DocumentObject::setResultPhase2VarianceServiceTimeVariance );
            result_table[XphaseP_service_time_variance[2]]= result_table_t( &DocumentObject::setResultPhase3VarianceServiceTime, &DocumentObject::setResultPhase3VarianceServiceTimeVariance );
            result_table[XphaseP_utilization[0]] =          result_table_t( &DocumentObject::setResultPhase1Utilization,         &DocumentObject::setResultPhase1UtilizationVariance );
            result_table[XphaseP_utilization[1]] =          result_table_t( &DocumentObject::setResultPhase2Utilization,         &DocumentObject::setResultPhase2UtilizationVariance );
            result_table[XphaseP_utilization[2]] =          result_table_t( &DocumentObject::setResultPhase3Utilization,         &DocumentObject::setResultPhase3UtilizationVariance );
            result_table[Xprob_exceed_max_service_time] =   result_table_t( 0, 0 );
            result_table[Xproc_utilization] =               result_table_t( &DocumentObject::setResultProcessorUtilization,      &DocumentObject::setResultProcessorUtilizationVariance );
            result_table[Xproc_waiting] =                   result_table_t( &DocumentObject::setResultProcessorWaiting,          &DocumentObject::setResultProcessorWaitingVariance );
            result_table[Xrwlock_reader_holding] =          result_table_t( &DocumentObject::setResultReaderHoldingTime,         &DocumentObject::setResultReaderHoldingTimeVariance );
            result_table[Xrwlock_reader_holding_variance] = result_table_t( &DocumentObject::setResultVarianceReaderHoldingTime, &DocumentObject::setResultVarianceReaderHoldingTimeVariance );
            result_table[Xrwlock_reader_utilization] =      result_table_t( &DocumentObject::setResultReaderHoldingUtilization,  &DocumentObject::setResultReaderHoldingUtilizationVariance );
            result_table[Xrwlock_reader_waiting] =          result_table_t( &DocumentObject::setResultReaderBlockedTime,         &DocumentObject::setResultReaderBlockedTimeVariance );
            result_table[Xrwlock_reader_waiting_variance] = result_table_t( &DocumentObject::setResultVarianceReaderBlockedTime, &DocumentObject::setResultVarianceReaderBlockedTimeVariance );
            result_table[Xrwlock_writer_holding] =          result_table_t( &DocumentObject::setResultWriterHoldingTime,         &DocumentObject::setResultWriterHoldingTimeVariance );
            result_table[Xrwlock_writer_holding_variance] = result_table_t( &DocumentObject::setResultVarianceWriterHoldingTime, &DocumentObject::setResultVarianceWriterHoldingTimeVariance );
            result_table[Xrwlock_writer_utilization] =      result_table_t( &DocumentObject::setResultWriterHoldingUtilization,  &DocumentObject::setResultWriterHoldingUtilizationVariance );
            result_table[Xrwlock_writer_waiting] =          result_table_t( &DocumentObject::setResultWriterBlockedTime,         &DocumentObject::setResultWriterBlockedTimeVariance );
            result_table[Xrwlock_writer_waiting_variance] = result_table_t( &DocumentObject::setResultVarianceWriterBlockedTime, &DocumentObject::setResultVarianceWriterBlockedTimeVariance );
            result_table[Xsemaphore_utilization] =          result_table_t( &DocumentObject::setResultHoldingUtilization,        &DocumentObject::setResultHoldingUtilizationVariance );
            result_table[Xsemaphore_waiting] =              result_table_t( &DocumentObject::setResultHoldingTime,               &DocumentObject::setResultHoldingTimeVariance );
            result_table[Xsemaphore_waiting_variance] =     result_table_t( &DocumentObject::setResultVarianceHoldingTime,       &DocumentObject::setResultVarianceHoldingTimeVariance );
            result_table[Xservice_time] =                   result_table_t( &DocumentObject::setResultServiceTime,               &DocumentObject::setResultServiceTimeVariance );
            result_table[Xservice_time_variance] =          result_table_t( &DocumentObject::setResultVarianceServiceTime,       &DocumentObject::setResultVarianceServiceTimeVariance );
            result_table[Xsquared_coeff_variation] =        result_table_t( &DocumentObject::setResultSquaredCoeffVariation,     &DocumentObject::setResultSquaredCoeffVariationVariance );
            result_table[Xthroughput] =                     result_table_t( &DocumentObject::setResultThroughput,                &DocumentObject::setResultThroughputVariance );
            result_table[Xthroughput_bound] =               result_table_t( &DocumentObject::setResultThroughputBound,           0 );
            result_table[Xutilization] =                    result_table_t( &DocumentObject::setResultUtilization,               &DocumentObject::setResultUtilizationVariance );
            result_table[Xwaiting] =                        result_table_t( &DocumentObject::setResultWaitingTime,               &DocumentObject::setResultWaitingTimeVariance );
            result_table[Xwaiting_variance] =               result_table_t( &DocumentObject::setResultVarianceWaitingTime,       &DocumentObject::setResultVarianceWaitingTimeVariance );

            task_table.insert(Xname);
            task_table.insert(Xscheduling);
            task_table.insert(Xinitially);
            task_table.insert(Xqueue_length);
            task_table.insert(Xpriority);
            task_table.insert(Xthink_time);
            task_table.insert(Xmultiplicity);
            task_table.insert(Xreplication);
            task_table.insert(Xactivity_graph);                 // ignored.

	    __key_lqx_function_map[KEY_ELAPSED_TIME]		= Xelapsed_time;
	    __key_lqx_function_map[KEY_ITERATIONS]		= Xiterations;
	    __key_lqx_function_map[KEY_PROCESSOR_UTILIZATION]	= Xproc_utilization;
	    __key_lqx_function_map[KEY_PROCESSOR_WAITING]	= Xproc_waiting;
	    __key_lqx_function_map[KEY_SERVICE_TIME]		= Xservice_time;
	    __key_lqx_function_map[KEY_SYSTEM_TIME]		= Xsystem_cpu_time;
	    __key_lqx_function_map[KEY_THROUGHPUT]		= Xthroughput;
	    __key_lqx_function_map[KEY_THROUGHPUT_BOUND]	= Xthroughput_bound;
	    __key_lqx_function_map[KEY_USER_TIME]		= Xuser_cpu_time;
	    __key_lqx_function_map[KEY_UTILIZATION]		= Xutilization;
	    __key_lqx_function_map[KEY_VARIANCE]		= Xservice_time_variance;
	    __key_lqx_function_map[KEY_WAITING]			= Xwaiting;
	    __key_lqx_function_map[KEY_WAITING_VARIANCE]	= Xwaiting_variance;
        };

        std::ostream&
        Expat_Document::printIndent( std::ostream& output, const int i )
        {
            if ( i < 0 ) {
                if ( __indent + i < 0 ) {
                    __indent = 0;
                } else {
                    __indent += i;
                }
            }
            if ( __indent != 0 ) {
                output << std::setw( __indent * 3 ) << " ";
            }
            if ( i > 0 ) {
                __indent += i;
            }
            return output;
        }


        std::ostream&
        Expat_Document::printStartElement( std::ostream& output, const XML_Char * element, const bool complex_element )
        {
            output << indent( complex_element ? 1 : 0  ) << "<" << element;
            return output;
        }

        std::ostream&
        Expat_Document::printEndElement( std::ostream& output, const XML_Char * element, const bool complex_element )
        {
            if ( complex_element ) {
                output << indent( -1 ) << "</" << element << ">";
            } else {
                output << "/>";
            }
            return output;
        }


        /*
	 * Print out an attribtue stored as a string.  If we encounter
	 * an &, or any other character in escape_table, we have to
	 * escape it.
	 */

	std::ostream&
        Expat_Document::printAttribute( std::ostream& output, const XML_Char * attribute, const XML_Char * value )
        {
            output << " " << attribute << "=\"";
	    for ( ; *value != 0; ++value ) {
		std::map<const XML_Char,const XML_Char *>::const_iterator item = escape_table.find(*value);
		if ( item == escape_table.end() ) {
		    output << *value;
		} else if ( item->first != '&' ) {
		    output << item->second;
		} else {
		    unsigned int i;
		    bool valid = false;
		    for ( i = 1; isalnum( value[i] ); ++i );
		    valid = ( value[i] == ';' );
		    if ( !valid && value[1] == '#' ) {
			for ( i = 2; isxdigit( value[i] ); ++i );
			valid = ( value[i] == ';' );
		    }
		    if ( valid ) {
			output << *value;	/* Found valid escape... ignore processing here */
		    } else {
			output << item->second;	/* No match in table so must escape ampresand */
		    }
		}
	    }
	    output << "\"";
            return output;
        }

        std::ostream&
        Expat_Document::printAttribute( std::ostream& output, const XML_Char * attribute, const double value )
        {
            output << " " << attribute << "=\"" <<  value << "\"";
            return output;
        }

        std::ostream&
        Expat_Document::printAttribute( std::ostream& output, const XML_Char * attribute, const ExternalVariable& var )
        {
            output << " " << attribute << "=\"" << var << "\"";
            return output;
        }

	std::ostream&
	Expat_Document::printComment( std::ostream& output, const std::string& s )
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

        std::ostream&
        Expat_Document::printTime( std::ostream& output, const XML_Char * attribute, const double time )
        {
            output << " " << attribute << "=\"" << LQIO::DOM::CPUTime::print( time ) <<  "\"";
            return output;
        }
    }
}

namespace LQIO {
    namespace DOM {

        /* ---------------------------------------------------------------- */
        /* Data.							    */
        /* ---------------------------------------------------------------- */

        const XML_Char * Expat_Document::XMLSchema_instance = "http://www.w3.org/2001/XMLSchema-instance";

        const XML_Char * Expat_Document::XDETERMINISTIC =                       "DETERMINISTIC";
        const XML_Char * Expat_Document::XGRAPH =                               "GRAPH";
        const XML_Char * Expat_Document::XNONE =                                "NONE";
        const XML_Char * Expat_Document::XPH1PH2 =                              "PH1PH2";
        const XML_Char * Expat_Document::Xactivity =                            LQIO::DOM::Activity::__typeName;
        const XML_Char * Expat_Document::Xactivity_graph =                      "activity-graph";
        const XML_Char * Expat_Document::Xasynch_call =                         "asynch-call";
        const XML_Char * Expat_Document::Xbegin =                               "begin";
        const XML_Char * Expat_Document::Xbound_to_entry =                      "bound-to-entry";
	const XML_Char * Expat_Document::Xbottleneck_strength =			"bottleneck-strength";
        const XML_Char * Expat_Document::Xcall_order =                          "call-order";
        const XML_Char * Expat_Document::Xcalls_mean =                          "calls-mean";
        const XML_Char * Expat_Document::Xcap =                                 "cap";
        const XML_Char * Expat_Document::Xcomment =                             "comment";
        const XML_Char * Expat_Document::Xconf_95 =                             "conf-95";
        const XML_Char * Expat_Document::Xconf_99 =                             "conf-99";
        const XML_Char * Expat_Document::Xconv_val =                            "conv_val";
        const XML_Char * Expat_Document::Xconv_val_result =                     "conv-val";
        const XML_Char * Expat_Document::Xcore =                                "core";
        const XML_Char * Expat_Document::Xcount =                               "count";
        const XML_Char * Expat_Document::Xdest =                                "dest";
        const XML_Char * Expat_Document::Xelapsed_time =                        "elapsed-time";
        const XML_Char * Expat_Document::Xend =                                 "end";
        const XML_Char * Expat_Document::Xentry =                               LQIO::DOM::Entry::__typeName;
        const XML_Char * Expat_Document::Xentry_phase_activities =              "entry-phase-activities";
        const XML_Char * Expat_Document::Xfanin =                               "fan-in";
        const XML_Char * Expat_Document::Xfanout =                              "fan-out";
        const XML_Char * Expat_Document::Xfaults =                              "faults";
        const XML_Char * Expat_Document::Xforwarding =                          "forwarding";
        const XML_Char * Expat_Document::Xgroup =                               "group";
        const XML_Char * Expat_Document::Xhistogram_bin =                       "histogram-bin";
        const XML_Char * Expat_Document::Xhost_demand_cvsq =                    "host-demand-cvsq";
        const XML_Char * Expat_Document::Xhost_demand_mean =                    "host-demand-mean";
        const XML_Char * Expat_Document::Xinitially =                           "initially";
        const XML_Char * Expat_Document::Xit_limit =                            "it_limit";
        const XML_Char * Expat_Document::Xiterations =                          "iterations";
        const XML_Char * Expat_Document::Xjoin_variance =                       "join-variance";
        const XML_Char * Expat_Document::Xjoin_waiting =                        "join-waiting";
        const XML_Char * Expat_Document::Xloss_probability =                    "loss-probability";
        const XML_Char * Expat_Document::Xlqn_model =                           "lqn-model";
        const XML_Char * Expat_Document::Xlqx =                                 "lqx";
        const XML_Char * Expat_Document::Xmax =                                 "max";
	const XML_Char * Expat_Document::Xmax_rss = 				"max-rss";
        const XML_Char * Expat_Document::Xmax_service_time =                    "max-service-time";
        const XML_Char * Expat_Document::Xmin =                                 "min";
        const XML_Char * Expat_Document::Xmultiplicity =                        "multiplicity";
        const XML_Char * Expat_Document::Xmva_info =                            "mva-info";
        const XML_Char * Expat_Document::Xname =                                "name";
        const XML_Char * Expat_Document::Xnumber_bins =                         "number-bins";
        const XML_Char * Expat_Document::Xopen_arrival_rate =                   "open-arrival-rate";
        const XML_Char * Expat_Document::Xopen_wait_time =                      "open-wait-time";
        const XML_Char * Expat_Document::Xoverflow_bin =                        "overflow-bin";
        const XML_Char * Expat_Document::Xparam =                               "param";
        const XML_Char * Expat_Document::Xphase =                               LQIO::DOM::Phase::__typeName;
        const XML_Char * Expat_Document::XphaseP_proc_waiting[] =             { "phase1-proc-waiting", "phase2-proc-waiting", "phase3-proc-waiting" };
        const XML_Char * Expat_Document::XphaseP_service_time[] =             { "phase1-service-time", "phase2-service-time", "phase3-service-time" };
        const XML_Char * Expat_Document::XphaseP_service_time_variance[] =    { "phase1-service-time-variance", "phase2-service-time-variance", "phase3-service-time-variance" };
        const XML_Char * Expat_Document::XphaseP_utilization[] =              { "phase1-utilization", "phase2-utilization", "phase3-utilization" };
        const XML_Char * Expat_Document::Xplatform_info =                       "platform-info";
        const XML_Char * Expat_Document::Xpost =                                "post";
        const XML_Char * Expat_Document::Xpost_and =                            "post-AND";
        const XML_Char * Expat_Document::Xpost_loop =                           "post-LOOP";
        const XML_Char * Expat_Document::Xpost_or =                             "post-OR";
        const XML_Char * Expat_Document::Xpragma =                              "pragma";
        const XML_Char * Expat_Document::Xpre =                                 "pre";
        const XML_Char * Expat_Document::Xpre_and =                             "pre-AND";
        const XML_Char * Expat_Document::Xpre_or =                              "pre-OR";
        const XML_Char * Expat_Document::Xprecedence =                          "precedence";
        const XML_Char * Expat_Document::Xprint_int =                           "print_int";
        const XML_Char * Expat_Document::Xpriority =                            "priority";
        const XML_Char * Expat_Document::Xprob =                                "prob";
        const XML_Char * Expat_Document::Xprob_exceed_max_service_time =        "prob-exceed-max-service-time";
        const XML_Char * Expat_Document::Xproc_utilization =                    "proc-utilization";
        const XML_Char * Expat_Document::Xproc_waiting =                        "proc-waiting";
        const XML_Char * Expat_Document::Xprocessor =                           LQIO::DOM::Processor::__typeName;
        const XML_Char * Expat_Document::Xquantum =                             "quantum";
        const XML_Char * Expat_Document::Xqueue_length =                        "queue-length";
        const XML_Char * Expat_Document::Xqueue_length_distribution =           "queue-length-distribution";
        const XML_Char * Expat_Document::Xquorum =                              "quorum";
        const XML_Char * Expat_Document::Xr_lock =                              "r-lock";
        const XML_Char * Expat_Document::Xr_unlock =                            "r-unlock";
        const XML_Char * Expat_Document::Xreplication =                         "replication";
        const XML_Char * Expat_Document::Xreply_activity =                      "reply-activity";
        const XML_Char * Expat_Document::Xreply_entry =                         "reply-entry";
        const XML_Char * Expat_Document::Xresult_activity =                     "result-activity";
        const XML_Char * Expat_Document::Xresult_activity_distribution =        "result_activity_distribution";
        const XML_Char * Expat_Document::Xresult_call =                         "result-call";
        const XML_Char * Expat_Document::Xresult_conf_95 =                      "result-conf-95";
        const XML_Char * Expat_Document::Xresult_conf_99 =                      "result-conf-99";
        const XML_Char * Expat_Document::Xresult_entry =                        "result-entry";
        const XML_Char * Expat_Document::Xresult_forwarding =                   "result-forwarding";
        const XML_Char * Expat_Document::Xresult_general =                      "result-general";
        const XML_Char * Expat_Document::Xresult_group =                        "result-group";
        const XML_Char * Expat_Document::Xresult_join_delay =                   "result-join-delay";
	const XML_Char * Expat_Document::Xresult_observation =                  "result-observe";
        const XML_Char * Expat_Document::Xresult_processor =                    "result-processor";
        const XML_Char * Expat_Document::Xresult_task =                         "result-task";
        const XML_Char * Expat_Document::Xrwlock =                              "rwlock";
        const XML_Char * Expat_Document::Xrwlock_reader_holding =               "rwlock-reader-holding";
        const XML_Char * Expat_Document::Xrwlock_reader_holding_variance =      "rwlock-reader-holding-variance";
        const XML_Char * Expat_Document::Xrwlock_reader_utilization =           "rwlock-reader-utilization";
        const XML_Char * Expat_Document::Xrwlock_reader_waiting =               "rwlock-reader-waiting";
        const XML_Char * Expat_Document::Xrwlock_reader_waiting_variance =      "rwlock-reader-waiting-variance";
        const XML_Char * Expat_Document::Xrwlock_writer_holding =               "rwlock-writer-holding";
        const XML_Char * Expat_Document::Xrwlock_writer_holding_variance =      "rwlock-writer-holding-variance";
        const XML_Char * Expat_Document::Xrwlock_writer_utilization =           "rwlock-writer-utilization";
        const XML_Char * Expat_Document::Xrwlock_writer_waiting =               "rwlock-writer-waiting";
        const XML_Char * Expat_Document::Xrwlock_writer_waiting_variance =      "rwlock-writer-waiting-variance";
        const XML_Char * Expat_Document::Xscheduling =                          "scheduling";
        const XML_Char * Expat_Document::Xsemaphore =                           "semaphore";
        const XML_Char * Expat_Document::Xsemaphore_waiting =                   "semaphore-waiting";
        const XML_Char * Expat_Document::Xsemaphore_waiting_variance =          "semaphore-waiting-variance";
        const XML_Char * Expat_Document::Xsemaphore_utilization =               "semaphore-utilization";
        const XML_Char * Expat_Document::Xservice =                             "service";
        const XML_Char * Expat_Document::Xservice_time =                        "service-time";
        const XML_Char * Expat_Document::Xservice_time_distribution =           "service-time-distribution";
        const XML_Char * Expat_Document::Xservice_time_variance =               "service-time-variance";
        const XML_Char * Expat_Document::Xshare =                               "share";
        const XML_Char * Expat_Document::Xsignal =                              "signal";
        const XML_Char * Expat_Document::Xsolver_info =                         "solver-info";
        const XML_Char * Expat_Document::Xsolver_parameters =                   "solver-params";
        const XML_Char * Expat_Document::Xsource =                              "source";
        const XML_Char * Expat_Document::Xspeed_factor =                        "speed-factor";
	const XML_Char * Expat_Document::Xspex_parameters =			"parameters";
	const XML_Char * Expat_Document::Xspex_results =			"results-collection";
        const XML_Char * Expat_Document::Xsquared_coeff_variation =             "squared-coeff-variation";
        const XML_Char * Expat_Document::Xstep =                                "step";
        const XML_Char * Expat_Document::Xstep_squared =                        "step-squared";
        const XML_Char * Expat_Document::Xsubmodels =                           "submodels";
        const XML_Char * Expat_Document::Xsynch_call =                          "synch-call";
        const XML_Char * Expat_Document::Xsystem_cpu_time =                     "system-cpu-time";
        const XML_Char * Expat_Document::Xtask =                                LQIO::DOM::Task::__typeName;
        const XML_Char * Expat_Document::Xtask_activities =                     "task-activities";
        const XML_Char * Expat_Document::Xthink_time =                          "think-time";
        const XML_Char * Expat_Document::Xthroughput =                          "throughput";
        const XML_Char * Expat_Document::Xthroughput_bound =                    "throughput-bound";
        const XML_Char * Expat_Document::Xtype =                                "type";
        const XML_Char * Expat_Document::Xunderflow_bin =                       "underflow-bin";
        const XML_Char * Expat_Document::Xunderrelax_coeff =                    "underrelax_coeff";
        const XML_Char * Expat_Document::Xuser_cpu_time =                       "user-cpu-time";
        const XML_Char * Expat_Document::Xutilization =                         "utilization";
        const XML_Char * Expat_Document::Xvalid =                               "valid";
        const XML_Char * Expat_Document::Xvalue =                               "value";
        const XML_Char * Expat_Document::Xw_lock =                              "w-lock";
        const XML_Char * Expat_Document::Xw_unlock =                            "w-unlock";
        const XML_Char * Expat_Document::Xwait =                                "wait";
        const XML_Char * Expat_Document::Xwait_squared =                        "wait-squared";
        const XML_Char * Expat_Document::Xwaiting =                             "waiting";
        const XML_Char * Expat_Document::Xwaiting_variance =                    "waiting-variance";
        const XML_Char * Expat_Document::Xxml_debug =                           "xml-debug";

	std::map<const XML_Char, const XML_Char *> Expat_Document::escape_table;
        std::map<const XML_Char *,ActivityList::ActivityListType,Expat_Document::attribute_table_t> Expat_Document::precedence_table;
        std::map<const XML_Char *,Expat_Document::result_table_t,Expat_Document::result_table_t>  Expat_Document::result_table;
        std::map<const XML_Char *,Expat_Document::observation_table_t,Expat_Document::observation_table_t>  Expat_Document::observation_table;	/* SPEX */
        std::set<const XML_Char *,Expat_Document::attribute_table_t> Expat_Document::activity_table;
        std::set<const XML_Char *,Expat_Document::attribute_table_t> Expat_Document::call_table;
        std::set<const XML_Char *,Expat_Document::attribute_table_t> Expat_Document::entry_table;

        std::set<const XML_Char *,Expat_Document::attribute_table_t> Expat_Document::group_table;
        std::set<const XML_Char *,Expat_Document::attribute_table_t> Expat_Document::histogram_table;
        std::set<const XML_Char *,Expat_Document::attribute_table_t> Expat_Document::model_table;
        std::set<const XML_Char *,Expat_Document::attribute_table_t> Expat_Document::parameter_table;
        std::set<const XML_Char *,Expat_Document::attribute_table_t> Expat_Document::processor_table;
        std::set<const XML_Char *,Expat_Document::attribute_table_t> Expat_Document::task_table;
	std::map<int,const char *> Expat_Document::__key_lqx_function_map;	/* Maps srvn_gram.h KEY_XXX to XML attribute name */
    }
}
