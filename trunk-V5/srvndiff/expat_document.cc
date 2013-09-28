/* -*- c++ -*-
 * $Id$
 *
 * Read in XML input files.
 *
 * Copyright the Real-Time and Distributed Systems Group,
 * Department of Systems and Computer Engineering,
 * Carleton University, Ottawa, Ontario, Canada. K1S 5B6
 *
 * ------------------------------------------------------------------------
 * May 2010.
 * ------------------------------------------------------------------------
 */

#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdexcept>
#include <fstream>
#include <iomanip>
#include <string>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <cassert>
#include <cmath>
#include <fcntl.h>
#if HAVE_SYS_ERRNO_H
#include <sys/errno.h>
#endif
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#if HAVE_STRINGS_H
#include <strings.h>
#endif

#include "expat_document.h"
#include "srvn_results.h"
#include "error.h"
#include "srvndiff.h"

extern char * lq_toolname;

namespace LQIO {
    namespace DOM {
        using namespace std;

        bool Expat_Document::__debugXML = false;

        const XML_Char * Expat_Document::XNil = "";
        const XML_Char * Expat_Document::XMLSchema_instance = "http://www.w3.org/2001/XMLSchema-instance";

        const XML_Char * Expat_Document::XDETERMINISTIC =                       "DETERMINISTIC";
        const XML_Char * Expat_Document::XGRAPH =                               "GRAPH";
        const XML_Char * Expat_Document::XNONE =                                "NONE";
        const XML_Char * Expat_Document::XPH1PH2 =                              "PH1PH2";
        const XML_Char * Expat_Document::Xactivity =                            "activity";
        const XML_Char * Expat_Document::Xasynch_call =                         "asynch-call";
        const XML_Char * Expat_Document::Xbound_to_entry =                      "bound-to-entry";
        const XML_Char * Expat_Document::Xcall_order =                          "call-order";
        const XML_Char * Expat_Document::Xcalls_mean =                          "calls-mean";
        const XML_Char * Expat_Document::Xcap =                                 "cap";
        const XML_Char * Expat_Document::Xcomment =                             "comment";
        const XML_Char * Expat_Document::Xconv_val =                            "conv_val";
        const XML_Char * Expat_Document::Xconv_val_result =                     "conv-val";
        const XML_Char * Expat_Document::Xcore =                                "core";
        const XML_Char * Expat_Document::Xcount =                               "count";
        const XML_Char * Expat_Document::Xdest =                                "dest";
        const XML_Char * Expat_Document::Xelapsed_time =                        "elapsed-time";
        const XML_Char * Expat_Document::Xend =                                 "end";
        const XML_Char * Expat_Document::Xentry =                               "entry";
        const XML_Char * Expat_Document::Xentry_phase_activities =              "entry-phase-activities";
        const XML_Char * Expat_Document::Xfanin =                               "fan-in";
        const XML_Char * Expat_Document::Xfanout =                              "fan-out";
        const XML_Char * Expat_Document::Xfaults =                              "faults";
        const XML_Char * Expat_Document::Xforwarding =                          "forwarding";
        const XML_Char * Expat_Document::Xgroup =                               "group";
        const XML_Char * Expat_Document::Xhistogram =                           "service-time-distribution";
        const XML_Char * Expat_Document::Xhistogram_bin =                       "histogram-bin";
        const XML_Char * Expat_Document::Xhost_demand_cvsq =                    "host-demand-cvsq";
        const XML_Char * Expat_Document::Xhost_demand_mean =                    "host-demand-mean";
        const XML_Char * Expat_Document::Xhost_max_phase_service_time =         "host-max-phase-service-time";
        const XML_Char * Expat_Document::Xit_limit =                            "it_limit";
        const XML_Char * Expat_Document::Xiterations =                          "iterations";
        const XML_Char * Expat_Document::Xjoin_variance =                       "join-variance";
        const XML_Char * Expat_Document::Xjoin_waiting =                        "join-waiting";
        const XML_Char * Expat_Document::Xloss_probability =                    "loss-probability";
        const XML_Char * Expat_Document::Xlqn_model =                           "lqn-model";
        const XML_Char * Expat_Document::Xlqx =                                 "lqx";
        const XML_Char * Expat_Document::Xmax =                                 "max";
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
        const XML_Char * Expat_Document::Xphase =                               "phase";
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
        const XML_Char * Expat_Document::Xprocessor =                           "processor";
        const XML_Char * Expat_Document::Xquantum =                             "quantum";
        const XML_Char * Expat_Document::Xqueue_length =                        "queue-length";
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
        const XML_Char * Expat_Document::Xresult_processor =                    "result-processor";
        const XML_Char * Expat_Document::Xresult_task =                         "result-task";
        const XML_Char * Expat_Document::Xrwlock =                              "rwlock";
        const XML_Char * Expat_Document::Xrwlock_reader_holding =               "rwlock-reader-holding";
        const XML_Char * Expat_Document::Xrwlock_reader_holding_variance =      "rwlock-reader-holding-variance";
        const XML_Char * Expat_Document::Xrwlock_reader_utilization =           "rwlock-reader_utilization";
        const XML_Char * Expat_Document::Xrwlock_reader_waiting =               "rwlock-reader-waiting";
        const XML_Char * Expat_Document::Xrwlock_reader_waiting_variance =      "rwlock-reader-waiting-variance";
        const XML_Char * Expat_Document::Xrwlock_writer_holding =               "rwlock-writer-holding";
        const XML_Char * Expat_Document::Xrwlock_writer_holding_variance =      "rwlock-writer-holding-variance";
        const XML_Char * Expat_Document::Xrwlock_writer_utilization =           "rwlock-writer_utilization";
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
        const XML_Char * Expat_Document::Xspeed_factor =                        "speed-factor";
        const XML_Char * Expat_Document::Xsquared_coeff_variation =             "squared-coeff-variation";
        const XML_Char * Expat_Document::Xstep =                                "step";
        const XML_Char * Expat_Document::Xstep_squared =                        "step-squared";
        const XML_Char * Expat_Document::Xsubmodels =                           "submodels";
        const XML_Char * Expat_Document::Xsynch_call =                          "synch-call";
        const XML_Char * Expat_Document::Xsystem_cpu_time =                     "system-cpu-time";
        const XML_Char * Expat_Document::Xtask =                                "task";
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

	std::map<const XML_Char *,ActivityList::ActivityListType,Expat_Document::attribute_table_t> Expat_Document::precedence_table;
	std::map<Expat_Document::result_fptr,Expat_Document::result_fptr,Expat_Document::confidence_result_table_t> Expat_Document::confidence_result_table;

	/* Ugly hack required here simply for sorting... C++ won't let me cast a function pointer to a caddr_t, so... */

	bool Expat_Document::confidence_result_table_t::operator()( result_fptr f1, result_fptr f2 ) const
	{
	    typedef union {
		result_fptr f;
		unsigned char * a[sizeof(result_fptr)];
	    } hack;

	    hack h1, h2;

	    h1.f = f1;
	    h2.f = f2;
	    
	    return memcmp( h1.a, h2.a, sizeof(result_fptr) ) < 0;
	}

	/* ---------------------------------------------------------------- */
        /* DOM input.                                                       */
	/* ---------------------------------------------------------------- */

	Expat_Document *
	Expat_Document::LoadLQNX( const char* filename, unsigned& errorCode )
	{
	    Expat_Document * document = new Expat_Document();
	    if ( !document->parse( filename ) ) {
		delete document;
		return 0;
	    }
	    return document;
	}


	Expat_Document::Expat_Document()
	    : _parser(),
	      _conf_95( ConfidenceIntervals( LQIO::ConfidenceIntervals::CONF_95 ) ),
	      _conf_99( ConfidenceIntervals( LQIO::ConfidenceIntervals::CONF_99 ) )
	{
	    _parser = XML_ParserCreateNS(NULL,'/');	/* Gobble header goop */
	    if ( !_parser ) {
		throw runtime_error("");
	    }

	    if ( precedence_table.size() == 0 ) {
		init_tables();
	    }

	    XML_SetElementHandler( _parser, start, end );
//	    XML_SetCDataSectionHandler( _parser, start_cdata, end_cdata );
	    XML_SetCharacterDataHandler( _parser, handle_text );
            XML_SetUnknownEncodingHandler( _parser, handle_encoding, static_cast<void *>(this) );
	    XML_SetUserData( _parser, static_cast<void *>(this) );
	    _stack.push( parse_stack_t(XNil,&Expat_Document::startModel) );
	}


	Expat_Document::~Expat_Document()
	{
	    XML_ParserFree(_parser);
	}


	/*  Parse the XML file, catching any XML exceptions that might propogate
	    out of it. */

	bool
	Expat_Document::parse( const char * filename )
	{
	    int input_fd = -1;

	    if ( strcmp( filename, "-" ) == 0 ) {
		input_fd = fileno( stdin );
		input_file_name = lq_toolname;
            } else if ( ( input_fd = open( filename, O_RDONLY ) ) < 0 ) {
                std::cerr << lq_toolname << ": Cannot open input file " << filename << " - " << strerror( errno ) << std::endl;
                return false;
	    } 

	    bool rc = true;
	    if ( isatty( input_fd  ) ) {
		std::cerr << lq_toolname << ": Input from terminal is not allowed." << std::endl;
		rc = false;
	    } else {
		input_file_name = filename;
	    }

	    const size_t BUFFSIZE = 1024;
	    char buffer[BUFFSIZE];		    /* Create a document to store the product */
	    size_t len = 0;

	    try {
		do { 
		    len = read( input_fd, buffer, BUFFSIZE );
		    if ( static_cast<int>(len) < 0 ) {
			std::cerr << lq_toolname << ": Read error on " << input_file_name << " - " << strerror( errno ) << std::endl;
			rc = false;
			break;
		    } else if (!XML_Parse(_parser, buffer, len, len != BUFFSIZE)) {
			input_error( XML_ErrorString(XML_GetErrorCode(_parser)) );
			rc = false;
			break;
		    }
		} while ( len == BUFFSIZE );
	    }
	    catch ( LQIO::element_error& e ) {
		input_error( "Unexpected element <%s> ", e.what() );
		rc = false;
	    }
	    catch ( LQIO::missing_attribute& e ) {
		rc = false;
	    }
	    catch ( std::invalid_argument& e ) {
		rc = false;
	    }

	    close( input_fd );
	    return rc;
	}

	void
	Expat_Document::input_error( const char * fmt, ... ) const
	{
	    va_list args;
	    va_start( args, fmt );
	    verrprintf( stdout, RUNTIME_ERROR, input_file_name,  XML_GetCurrentLineNumber(_parser), 0, fmt, args );
	    va_end( args );
	}

	void
	Expat_Document::start( void *data, const XML_Char *el, const XML_Char **attr )
	{
	    Expat_Document * document = static_cast<Expat_Document *>(data);
	    const parse_stack_t& top = document->_stack.top();
	    if ( __debugXML ) {
		for ( unsigned i = 0; i < document->_stack.size(); ++i ) {
		    cerr << "  ";
		}
		cerr << "<" << el;
		for ( const XML_Char ** attributes = attr; *attributes; attributes += 2 ) {
		    cerr << " " << *attributes << "=\"" << *(attributes+1) << "\"";
		}
		cerr << ">" << endl;
	    }
	    try {
		(document->*top.start_func)(top.object.c_str(),el,attr);
	    }
	    catch ( LQIO::missing_attribute& e ) {
		document->input_error( "Missing attribute \"%s\" for element <%s>", e.what(), el );
		throw;
	    }
	    catch ( std::invalid_argument& e ) {
		document->input_error( "Invalid argument \"%s\" to attribute for element <%s>", e.what(), el );
		throw;
	    }
	}

	/*
	 * Pop elements off the stack until we hit a matching tag.
	 */

	void
	Expat_Document::end( void *data, const XML_Char *el )
	{
	    Expat_Document * document = static_cast<Expat_Document *>(data);
	    bool done = false;
	    while ( document->_stack.size() > 0 && !done ) {
		parse_stack_t& top = document->_stack.top();
		if ( __debugXML ) {
		    for ( unsigned i = 1; i < document->_stack.size(); ++i ) {
			cerr << "  ";
		    }
		    if ( top.element.size() ) {
			cerr << "</" << top.element << ">" << endl;
		    } else {
			cerr << "empty stack" << endl;
		    }
		}
		done = (document->_stack.size() == 1) || (top == el);
		if ( top.end_func ) {
		    (document->*top.end_func)(top.object.c_str(),el);
		}
		document->_stack.pop();
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


	void
	Expat_Document::handle_text( void * data, const XML_Char * text, int length ) 
	{
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
	Expat_Document::startModel( const DocumentObject * object, const XML_Char * element, const XML_Char ** attributes )
	{
	    if ( strcasecmp( element, Xlqn_model ) == 0 ) {
		__debugXML = (__debugXML || getBoolAttribute(attributes,Xxml_debug));
		_stack.push( parse_stack_t(element,&Expat_Document::startModelType) );
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
	Expat_Document::startModelType( const DocumentObject * object, const XML_Char * element, const XML_Char ** attributes )
	{
	    if ( strcasecmp( element, Xsolver_parameters) == 0 ) {
		_stack.push( parse_stack_t(element,&Expat_Document::startResultGeneral) );

	    } else if ( strcasecmp( element, Xprocessor) == 0 ) {
		_stack.push( parse_stack_t(element,&Expat_Document::startProcessorType,getStringAttribute( attributes, Xname )) );

	    } else if ( strcasecmp( element, Xlqx ) == 0 ) {
		_stack.push( parse_stack_t(element,&Expat_Document::startLQX,object) );

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
	Expat_Document::startResultGeneral( const DocumentObject * object, const XML_Char * element, const XML_Char ** attributes )
	{
	    if ( strcasecmp( element, Xresult_general ) == 0 ) {
		long iterations = 50;
		getLongAttribute(attributes,Xiterations);
		if ( iterations > 1 ) {
		    _conf_95.set_t_value( iterations );
		}
		set_general( getBoolAttribute(attributes,Xvalid),
			     getDoubleAttribute(attributes,Xconv_val_result),
			     iterations,
			     0,			// Processors -- obsolete
			     1 );			// Phases -- obsolete?
		_stack.push( parse_stack_t(element,&Expat_Document::startMVAInfo,object) );

	    } else if ( strcasecmp( element, Xpragma ) == 0 ) {
		_stack.push( parse_stack_t(element,&Expat_Document::startNOP) );

	    } else {
		throw element_error( element );

	    }
	}


	void
	Expat_Document::startMVAInfo( const DocumentObject *, const XML_Char * element, const XML_Char ** attributes )
	{
	    if ( strcasecmp( element, Xmva_info ) == 0 ) {
		_stack.push( parse_stack_t(element,&Expat_Document::startNOP) );
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
	Expat_Document::startProcessorType( const DocumentObject * processor, const XML_Char * element, const XML_Char ** attributes )
	{
	    if ( strcasecmp( element, Xresult_processor) == 0 ) {
		handleProcessorResults( processor, 0, 0, attributes );
		_stack.push( parse_stack_t(element,&Expat_Document::startOutputResultType,processor,XNil,XNil,&Expat_Document::handleProcessorResults) );

	    } else if ( strcasecmp( element, Xtask) == 0 ) {
		_stack.push( parse_stack_t(element,&Expat_Document::startTaskType,getStringAttribute( attributes, Xname ) ));

	    } else if ( strcasecmp( element, Xgroup ) == 0 ) {
		_stack.push( parse_stack_t(element,&Expat_Document::startGroupType,getStringAttribute( attributes, Xname ) ));

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
	Expat_Document::startGroupType( const DocumentObject * group, const XML_Char * element, const XML_Char ** attributes )
	{
	    if ( strcasecmp( element, Xresult_group ) == 0 ) {
		handleGroupResults( group, 0, 0, attributes );
		_stack.push( parse_stack_t(element,&Expat_Document::startOutputResultType,group,XNil,XNil,&Expat_Document::handleGroupResults) );

	    } else if ( strcasecmp( element, Xtask ) == 0 ) {
		_stack.push( parse_stack_t(element,&Expat_Document::startTaskType, getStringAttribute( attributes, Xname ) ) );

	    } else {
		throw element_error( element );
	    }
	}

	/*
	  <xsd:complexType name="TaskType">
	  <xsd:sequence>
	  <xsd:element name="result-task" type="OutputResultType" minOccurs="0" maxOccurs="unbounded"/>
	  <xsd:element name="entry" type="EntryType" maxOccurs="unbounded"/>
	  <xsd:element name="service" type="ServiceType" minOccurs="0" maxOccurs="unbounded"/>
	  <xsd:element name="task-activities" type="TaskActivityGraph" minOccurs="0"/>
	  </xsd:sequence>
	  </xsd:complexType>
	*/

	void
	Expat_Document::startTaskType( const DocumentObject * task, const XML_Char * element, const XML_Char ** attributes )
	{
	    if ( strcasecmp( element, Xresult_task) == 0 ) {
		handleTaskResults( task, 0, 0, attributes );
		_stack.push( parse_stack_t(element,&Expat_Document::startOutputResultType,task,XNil,XNil,&Expat_Document::handleTaskResults) );

	    } else if ( strcasecmp( element, Xentry) == 0 ) {
		_stack.push( parse_stack_t(element,&Expat_Document::startEntryType,getStringAttribute( attributes, Xname ) ) );

	    } else if ( strcasecmp( element, Xservice_time_distribution ) == 0 ) {
		_stack.push( parse_stack_t(element,&Expat_Document::startOutputDistributionType, task) );

	    } else if ( strcasecmp( element, Xservice ) == 0 
			|| strcasecmp( element, Xfanin ) == 0 
			|| strcasecmp( element, Xfanout ) == 0 ) {
		_stack.push( parse_stack_t(element,&Expat_Document::startNOP) );

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
	Expat_Document::startEntryType( const DocumentObject * entry, const XML_Char * element, const XML_Char ** attributes )
	{
	    if ( strcasecmp( element, Xresult_entry) == 0 ) {
		handleEntryResults( entry, 0, 0, attributes );
		_stack.push( parse_stack_t(element,&Expat_Document::startOutputResultType,entry,XNil,XNil,&Expat_Document::handleEntryResults) );

	    } else if ( strcasecmp( element, Xservice_time_distribution) == 0 ) {
		_stack.push( parse_stack_t(element,&Expat_Document::startOutputDistributionType,entry) );

	    } else if ( strcasecmp( element, Xforwarding ) == 0 ) {
		_stack.push( parse_stack_t(element,&Expat_Document::startEntryMakingCallType,entry) );

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
	Expat_Document::startPhaseActivities( const DocumentObject * entry, const XML_Char * element, const XML_Char ** attributes )
	{
	    if ( strcasecmp( element, Xactivity) == 0 ) {
		_stack.push( parse_stack_t(element,&Expat_Document::startActivityDefBase,entry,getStringAttribute( attributes, Xphase ),XNil,&Expat_Document::handlePhaseResults) );

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
	Expat_Document::startActivityDefBase( const DocumentObject * task, const XML_Char * element, const XML_Char ** attributes )
	{
	    const parse_stack_t& top = _stack.top();
	    const DocumentObject * activity = top.extra.c_str();
	    if ( strcasecmp( element, Xresult_activity ) == 0 ) {
		if ( top.result ) {
		    (this->*top.result)( task, activity, 0, attributes );		/* task may be entry, in which case activity is phase.  result function figures this out. */
		}
		_stack.push( parse_stack_t(element,&Expat_Document::startOutputResultType,task,activity,XNil,top.result) );

	    } else if ( strcasecmp( element, Xsynch_call ) == 0 ) {
		const XML_Char * dest = getStringAttribute( attributes, Xdest );
		if ( top.result == &Expat_Document::handlePhaseResults ) {
		    _stack.push( parse_stack_t(element,&Expat_Document::startActivityMakingCallType,task,activity,dest,&Expat_Document::handlePhaseRNVCallResults) );
		} else {
		    _stack.push( parse_stack_t(element,&Expat_Document::startActivityMakingCallType,task,activity,dest,&Expat_Document::handleActivityRNVCallResults) );
		} 

	    } else if ( strcasecmp( element, Xasynch_call ) == 0 ) {
		const XML_Char * dest = getStringAttribute( attributes, Xdest );
		if ( top.result == &Expat_Document::handlePhaseResults ) {
		    _stack.push( parse_stack_t(element,&Expat_Document::startActivityMakingCallType,task,activity,dest,&Expat_Document::handlePhaseSNRCallResults) );
		} else {
		    _stack.push( parse_stack_t(element,&Expat_Document::startActivityMakingCallType,task,activity,dest,&Expat_Document::handleActivitySNRCallResults) );
		} 

	    } else if ( strcasecmp( element, Xservice_time_distribution ) == 0 ) {
		_stack.push( parse_stack_t(element,&Expat_Document::startOutputDistributionType,task,activity) );
		
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
	Expat_Document::startActivityMakingCallType( const DocumentObject * call, const XML_Char * element, const XML_Char ** attributes )
	{
	    if ( strcasecmp( element, Xresult_call ) == 0 ) {
		const parse_stack_t& top = _stack.top();
		if ( top.result ) {
		    const DocumentObject * task = top.object.c_str();
		    const DocumentObject * activity = top.extra.c_str();
		    const XML_Char * dest = top.dest.c_str();
		    (this->*top.result)( task, activity, dest, attributes );		/* task may be entry, in which case activity is phase.  result function figures this out. */
		    _stack.push( parse_stack_t(element,&Expat_Document::startOutputResultType,call,activity,dest,top.result) );
		} else {
		    _stack.push( parse_stack_t(element,&Expat_Document::startOutputResultType,call) );
		}
	    } else {
		throw element_error( element );
	    }
	}

	void
	Expat_Document::startEntryMakingCallType( const DocumentObject * call, const XML_Char * element, const XML_Char ** attributes )
	{
	    if ( strcasecmp( element, Xresult_call ) == 0 ) {
		const parse_stack_t& top = _stack.top();
		_stack.push( parse_stack_t(element,&Expat_Document::startOutputResultType,call,XNil,XNil,top.result) );
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
	Expat_Document::startTaskActivityGraph( const DocumentObject * task, const XML_Char * element, const XML_Char ** attributes )
	{
	    if ( strcasecmp( element, Xactivity ) == 0 ) {
		_stack.push( parse_stack_t( element, &Expat_Document::startActivityDefBase, task, getStringAttribute(attributes,Xname), XNil, &Expat_Document::handleActivityResults) );
	    } else if ( strcasecmp( element, Xprecedence ) == 0 ) {
		_stack.push( parse_stack_t(element,&Expat_Document::startPrecedenceType,task) );
	    } else if ( strcasecmp( element, Xreply_entry ) == 0 ) {
		_stack.push( parse_stack_t(element,&Expat_Document::startReplyActivity,task) );
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
	Expat_Document::startPrecedenceType( const DocumentObject * task, const XML_Char * element, const XML_Char ** attributes )
	{
	    std::map<const XML_Char *,ActivityList::ActivityListType>::const_iterator item = precedence_table.find(element);
	    if ( item != precedence_table.end() ) {
		switch ( item->second ) {
		case ActivityList::OR_JOIN_ACTIVITY_LIST:
		case ActivityList::JOIN_ACTIVITY_LIST:
		    _stack.push( parse_stack_t(element,&Expat_Document::startActivityListType) );
		    break;

		case ActivityList::AND_JOIN_ACTIVITY_LIST:
		    _stack.push( parse_stack_t(element,&Expat_Document::startActivityListType,task,XNil,XNil,0,&Expat_Document::endActivityListType) );
		    _stack.top().data = static_cast<void *>(new join_info_t);
		    break;

		case ActivityList::OR_FORK_ACTIVITY_LIST:
		case ActivityList::FORK_ACTIVITY_LIST:
		case ActivityList::AND_FORK_ACTIVITY_LIST:
		case ActivityList::REPEAT_ACTIVITY_LIST:
		    _stack.push( parse_stack_t(element,&Expat_Document::startActivityListType) );
		    break;
		}
	    } else {
		throw element_error( element );
	    }
	}


	/*
	 * If results are present, top of stack will have mean and variance.  Form the list as activities are found.
	 */
		   
	void
	Expat_Document::startActivityListType( const DocumentObject * activity_list, const XML_Char * element, const XML_Char ** attributes )
	{
	    parse_stack_t& top = _stack.top();
	    if ( strcasecmp( element, Xresult_join_delay ) == 0 ) {
		handleJoinResults( activity_list, 0, 0, attributes );
		_stack.push( parse_stack_t(element,&Expat_Document::startJoinResultType,activity_list,top.extra.c_str(),top.dest.c_str()) );
		_stack.top().data = top.data;

	    } else if ( strcasecmp( element, Xservice_time_distribution ) == 0 ) {
		_stack.push( parse_stack_t(element,&Expat_Document::startOutputDistributionType, activity_list) );
		
	    } else if ( strcasecmp( element, Xactivity ) == 0 ) {
		if ( top.extra.size() == 0 ) {
		    top.extra = getStringAttribute( attributes, Xname );		// Set to first activity
		} else {
		    top.dest  = getStringAttribute( attributes, Xname );		// Set to last activity
		}
		_stack.push( parse_stack_t(element,&Expat_Document::startNOP) );
	    } else {
		throw element_error( element );
	    }
	}

	void
	Expat_Document::endActivityListType( const DocumentObject * task, const XML_Char * element )
	{
	    const parse_stack_t& top = _stack.top();
	    unsigned first_activity = find_or_add_activity( task, top.extra.c_str() );
	    unsigned last_activity  = find_or_add_activity( task, top.dest.c_str() );
	    join_info_t * data   = static_cast<join_info_t *>(top.data);
	    if ( first_activity && last_activity && data ) {
		join_tab[pass][first_activity][last_activity].mean           = data->mean;
		join_tab[pass][first_activity][last_activity].variance       = data->variance;
		join_tab[pass][first_activity][last_activity].mean_conf      = data->mean_conf;
		join_tab[pass][first_activity][last_activity].variance_conf  = data->variance_conf;
	    }
	    if ( data ) {
		delete data;
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
	Expat_Document::startReplyActivity( const DocumentObject * entry, const XML_Char * element, const XML_Char ** attributes )
	{
	    if ( strcasecmp( element, Xreply_activity ) == 0 ) {
		_stack.push( parse_stack_t(element,&Expat_Document::startNOP) );
	    } else {
		throw element_error( element );
	    }
	}


	void
	Expat_Document::startOutputResultType( const DocumentObject * object, const XML_Char * element, const XML_Char ** attributes )
	{
	    if ( strcasecmp( element, Xresult_conf_95 ) == 0 ) {
		confidence_intervals_present[pass] = true;

		/* Determine which confidence interval function to run based on the function used to extract the primary results */

		const parse_stack_t& top = _stack.top();
		const DocumentObject * extra = top.extra.c_str();
		const DocumentObject * dest  = top.dest.c_str();
		if ( top.result ) {
		    std::map<result_fptr,result_fptr,confidence_result_table_t>::const_iterator conf_func = confidence_result_table.find( top.result );
		    if ( conf_func != confidence_result_table.end() ) {
			result_fptr result = conf_func->second;
			(this->*result)( object, extra, dest, attributes );
		    }
		}
		_stack.push( parse_stack_t(element,&Expat_Document::startNOP) );

	    } else if ( strcasecmp( element, Xresult_conf_99 ) == 0 ) {
		_stack.push( parse_stack_t(element,&Expat_Document::startNOP) );
		

	    } else {
		throw element_error( element );

	    }
	}


	void
	Expat_Document::startJoinResultType( const DocumentObject * task, const XML_Char * element, const XML_Char ** attributes )
	{
	    if ( strcasecmp( element, Xresult_conf_95 ) == 0 ) {
		handleJoinConfResults( task, 0, 0, attributes );
		_stack.push( parse_stack_t(element,&Expat_Document::startNOP) );
	    } else if ( strcasecmp( element, Xresult_conf_99 ) == 0 ) {
		_stack.push( parse_stack_t(element,&Expat_Document::startNOP) );
	    } else {
		throw element_error( element );
	    }
	}


	void
	Expat_Document::startOutputDistributionType( const DocumentObject * object, const XML_Char * element, const XML_Char ** attributes )
	{
	    if ( strcasecmp( element, Xhistogram_bin ) == 0 || strcasecmp( element, Xunderflow_bin ) == 0 || strcasecmp( element, Xoverflow_bin ) == 0 ) {
		_stack.push( parse_stack_t(element,&Expat_Document::startNOP) );
	    } else {
		throw element_error( element );
	    }
	}


	void
	Expat_Document::startLQX( const DocumentObject * object, const XML_Char * element, const XML_Char ** attributes )
	{
	    throw element_error( element );		/* Should not get here. */
	}

	void
	Expat_Document::startNOP( const DocumentObject * object, const XML_Char * element, const XML_Char ** attributes )
	{
	    throw element_error( element );		/* Should not get here. */
	}

	/* ------------------------------------------------------------------------ */
	/* Functions used to extract results.  Erroneous or superfluous attributes  */
	/* are ignored.								    */
	/* ------------------------------------------------------------------------ */

	void
	Expat_Document::handleProcessorResults( const DocumentObject * processor, const DocumentObject * extra, const DocumentObject * dest, const XML_Char ** attributes )
	{
	    const unsigned int p = find_or_add_processor( processor );
	    if ( !p ) return;
	    
	    processor_tab[pass][p].utilization = getDoubleAttribute( attributes, Xutilization, 0.0 );
	    processor_tab[pass][p].has_results = true;
	}

	void
	Expat_Document::handleGroupResults( const DocumentObject * group, const DocumentObject * extra, const DocumentObject * dest, const XML_Char ** attributes )
	{
	    const unsigned int p = find_or_add_group( group );
	    if ( !p ) return;
	    
	    group_tab[pass][p].utilization = getDoubleAttribute( attributes, Xutilization, 0.0 );
	    group_tab[pass][p].has_results = true;
	}

	void
	Expat_Document::handleTaskResults( const DocumentObject * task, const DocumentObject * extra, const DocumentObject * dest, const XML_Char ** attributes )
	{
	    const unsigned int t = find_or_add_task( task );
	    if ( !t ) return;

	    task_tab[pass][t].throughput = getDoubleAttribute( attributes, Xthroughput, 0.0 );
	    task_tab[pass][t].utilization = getDoubleAttribute( attributes, Xutilization, 0.0 );
	    task_tab[pass][t].semaphore_waiting = getDoubleAttribute( attributes, Xsemaphore_waiting, 0.0 );
	    task_tab[pass][t].semaphore_utilization = getDoubleAttribute( attributes, Xsemaphore_utilization, 0.0 );
	    task_tab[pass][t].rwlock_reader_waiting = getDoubleAttribute( attributes, Xrwlock_reader_waiting, 0.0 );
	    task_tab[pass][t].rwlock_reader_holding = getDoubleAttribute( attributes, Xrwlock_reader_holding, 0.0 );
	    task_tab[pass][t].rwlock_reader_utilization = getDoubleAttribute( attributes, Xrwlock_reader_utilization, 0.0 );
	    task_tab[pass][t].rwlock_writer_waiting = getDoubleAttribute( attributes, Xrwlock_writer_waiting, 0.0 );
	    task_tab[pass][t].rwlock_writer_holding = getDoubleAttribute( attributes, Xrwlock_writer_holding, 0.0 );
	    task_tab[pass][t].rwlock_writer_utilization = getDoubleAttribute( attributes, Xrwlock_writer_utilization, 0.0 );
	    task_tab[pass][t].has_results = true;
	}

	void
	Expat_Document::handleEntryResults( const DocumentObject * entry, const DocumentObject * extra, const DocumentObject * dest, const XML_Char ** attributes )
	{
	    const unsigned int e = find_or_add_entry( entry );
	    if ( !e ) return;

	    const double wait_time = getDoubleAttribute( attributes, Xopen_wait_time, 0.0 );
	    if ( wait_time > 0.0 ) {
		entry_tab[pass][e].open_waiting = wait_time;
		entry_tab[pass][e].open_arrivals = true;
	    }

	    const double throughput = getDoubleAttribute( attributes, Xthroughput, 0.0 );
	    entry_tab[pass][e].throughput = throughput;
	    
	    /* For case where we have activities... results specified at entry level */

	    for ( unsigned int p = 0; p < 2; ++p ) {
		activity_info& ph = entry_tab[pass][e].phase[p];
		const double s = getDoubleAttribute( attributes, XphaseP_service_time[p], 0.0 );
		if ( s ) {
		    ph.service = s;
		    ph.variance = getDoubleAttribute( attributes, XphaseP_service_time_variance[p], 0.0 );
		}
	    }
	}

	void 
	Expat_Document::handlePhaseResults( const DocumentObject * entry, const DocumentObject * phase, const DocumentObject * dest, const XML_Char ** attributes )
	{
	    const unsigned int e = find_or_add_entry( entry );
	    const unsigned long int p = strtol( phase, 0, 10 );
	    if ( !e || !p || p > MAX_PHASES ) return;		/* Bogus entry. */
	    set_max_phase( p );

	    activity_info& ph = entry_tab[pass][e].phase[p-1];
	    ph.service           = getDoubleAttribute( attributes, Xservice_time, 0.0 );
	    ph.variance          = getDoubleAttribute( attributes, Xservice_time_variance, 0.0 );
	    ph.processor_waiting = getDoubleAttribute( attributes, Xproc_waiting, 0.0 );
	    ph.utilization       = getDoubleAttribute( attributes, Xutilization, 0.0 );
	}

	void
	Expat_Document::handleActivityResults( const DocumentObject * task, const DocumentObject * activity, const DocumentObject * dest, const XML_Char ** attributes )
	{
	    const unsigned int a = find_or_add_activity( task, activity );
	    if ( !a ) return;

	    activity_info& act = activity_tab[pass][a];
	    act.service           = getDoubleAttribute( attributes, Xservice_time, 0.0 );
	    act.variance          = getDoubleAttribute( attributes, Xservice_time_variance, 0.0 );
	    act.processor_waiting = getDoubleAttribute( attributes, Xproc_waiting, 0.0 );
	}

	void
	Expat_Document::handleEntryFwdCallResults( const DocumentObject * entry, const DocumentObject * phase, const DocumentObject * dest, const XML_Char ** attributes )
	{
	    const unsigned int e = find_or_add_entry( entry );
	    const unsigned int d = find_or_add_entry( dest );
	    if ( !e || !d ) return;		/* Bogus entry. */

	    call_info & call = entry_tab[pass][e].phase[0].to[d];
	    call.waiting  = getDoubleAttribute( attributes, Xwaiting, 0.0 );
	    call.wait_var = getDoubleAttribute( attributes, Xwaiting_variance, 0.0 );
	}


	void
	Expat_Document::handlePhaseRNVCallResults( const DocumentObject * entry, const DocumentObject * phase, const DocumentObject * dest, const XML_Char ** attributes )
	{
	    const unsigned int e = find_or_add_entry( entry );
	    const unsigned int p = strtol( phase, 0, 10 );
	    const unsigned int d = find_or_add_entry( dest );
	    if ( !e || !p || p > MAX_PHASES || !d ) return;		/* Bogus entry. */
	    set_max_phase( p );

	    call_info & call = entry_tab[pass][e].phase[p-1].to[d];
	    call.waiting  = getDoubleAttribute( attributes, Xwaiting, 0.0 );
	    call.wait_var = getDoubleAttribute( attributes, Xwaiting_variance, 0.0 );
	}

	void
	Expat_Document::handleActivityRNVCallResults( const DocumentObject * task, const DocumentObject * activity, const DocumentObject * dest, const XML_Char ** attributes )
	{
	    const unsigned int a = find_or_add_activity( task, activity );
	    const unsigned int d = find_or_add_entry( dest );
	    if ( !a || !d ) return;

	    call_info & call = activity_tab[pass][a].to[d];
	    call.waiting  = getDoubleAttribute( attributes, Xwaiting, 0.0 );
	    call.wait_var = getDoubleAttribute( attributes, Xwaiting_variance, 0.0 );
	}

	void
	Expat_Document::handlePhaseSNRCallResults( const DocumentObject * entry, const DocumentObject * phase, const DocumentObject * dest, const XML_Char ** attributes )
	{
	    const unsigned int e = find_or_add_entry( entry );
	    const unsigned int p = strtol( phase, 0, 10 );
	    const unsigned int d = find_or_add_entry( dest );
	    if ( !e || !p || p > MAX_PHASES || !d ) return;		/* Bogus entry. */
	    set_max_phase( p );

	    call_info & call = entry_tab[pass][e].phase[p-1].to[d];
	    call.snr_waiting      = getDoubleAttribute( attributes, Xwaiting, 0.0 );
	    call.snr_wait_var     = getDoubleAttribute( attributes, Xwaiting_variance, 0.0 );
	    call.loss_probability = getDoubleAttribute( attributes, Xloss_probability, 0.0 );
	}

	void
	Expat_Document::handleActivitySNRCallResults( const DocumentObject * task, const DocumentObject * activity, const DocumentObject * dest, const XML_Char ** attributes )
	{
	    const unsigned int a = find_or_add_activity( task, activity );
	    const unsigned int d = find_or_add_entry( dest );
	    if ( !a || !d ) return;

	    call_info & call = activity_tab[pass][a].to[d];
	    call.snr_waiting      = getDoubleAttribute( attributes, Xwaiting, 0.0 );
	    call.snr_wait_var     = getDoubleAttribute( attributes, Xwaiting_variance, 0.0 );
	    call.loss_probability = getDoubleAttribute( attributes, Xloss_probability, 0.0 );
	}

	/* 
	 * This one is a bit tricky because we have the attributes before we have the list of activities.
	 */

	void
	Expat_Document::handleJoinResults( const DocumentObject * join_list, const DocumentObject * extra, const DocumentObject * dest, const XML_Char ** attributes )
	{
	    join_info_t * data = static_cast<join_info_t *>(_stack.top().data);
	    if ( data ) {
		data->mean     = getDoubleAttribute( attributes, Xjoin_waiting );
		data->variance = getDoubleAttribute( attributes, Xjoin_variance, 0.0 );
	    }
	}

	void
	Expat_Document::handleProcessorConfResults( const DocumentObject * processor, const DocumentObject * extra, const DocumentObject * dest, const XML_Char ** attributes )
	{
	    const unsigned int p = find_or_add_processor( processor );
	    if ( !p ) return;
	    
	    processor_tab[pass][p].utilization_conf = getDoubleAttribute( attributes, Xutilization, 0.0 );
	}

	void
	Expat_Document::handleGroupConfResults( const DocumentObject * group, const DocumentObject * extra, const DocumentObject * dest, const XML_Char ** attributes )
	{
	    const unsigned int p = find_or_add_group( group );
	    if ( !p ) return;
	    
	    group_tab[pass][p].utilization_conf = getDoubleAttribute( attributes, Xutilization, 0.0 );
	}

	void
	Expat_Document::handleTaskConfResults( const DocumentObject * task, const DocumentObject * extra, const DocumentObject * dest, const XML_Char ** attributes )
	{
	    const unsigned int t = find_or_add_task( task );
	    if ( !t ) return;

	    task_tab[pass][t].throughput_conf  = getDoubleAttribute( attributes, Xthroughput, 0.0 );
	    task_tab[pass][t].utilization_conf = getDoubleAttribute( attributes, Xutilization, 0.0 );
	    task_tab[pass][t].semaphore_waiting_conf     = getDoubleAttribute( attributes, Xsemaphore_waiting, 0.0 );
	    task_tab[pass][t].semaphore_utilization_conf = getDoubleAttribute( attributes, Xsemaphore_utilization, 0.0 );
	    task_tab[pass][t].rwlock_reader_waiting_conf = getDoubleAttribute( attributes, Xrwlock_reader_waiting, 0.0 );
	    task_tab[pass][t].rwlock_reader_holding_conf = getDoubleAttribute( attributes, Xrwlock_reader_holding, 0.0 );
	    task_tab[pass][t].rwlock_reader_utilization_conf = getDoubleAttribute( attributes, Xrwlock_reader_utilization, 0.0 );
	    task_tab[pass][t].rwlock_writer_waiting_conf = getDoubleAttribute( attributes, Xrwlock_writer_waiting, 0.0 );
	    task_tab[pass][t].rwlock_writer_holding_conf = getDoubleAttribute( attributes, Xrwlock_writer_holding, 0.0 );
	    task_tab[pass][t].rwlock_writer_utilization_conf = getDoubleAttribute( attributes, Xrwlock_writer_utilization, 0.0 );

	    task_tab[pass][t].has_results = true;
	}

	void
	Expat_Document::handleEntryConfResults( const DocumentObject * entry, const DocumentObject * extra, const DocumentObject * dest, const XML_Char ** attributes )
	{
	    const unsigned int e = find_or_add_entry( entry );
	    if ( !e ) return;

	    const double wait_time_conf = getDoubleAttribute( attributes, Xopen_wait_time, 0.0 );
	    if ( wait_time_conf > 0.0 ) {
		entry_tab[pass][e].open_wait_conf = wait_time_conf;
	    }
	}

	void 
	Expat_Document::handlePhaseConfResults( const DocumentObject * entry, const DocumentObject * phase, const DocumentObject * dest, const XML_Char ** attributes )
	{
	    const unsigned int e = find_or_add_entry( entry );
	    const unsigned int p = strtol( phase, 0, 10 );
	    if ( !e || !p || p > MAX_PHASES ) return;		/* Bogus entry. */

	    activity_info& ph = entry_tab[pass][e].phase[p-1];
	    ph.serv_conf	      = getDoubleAttribute( attributes, Xservice_time, 0.0 );
	    ph.var_conf		      = getDoubleAttribute( attributes, Xservice_time_variance, 0.0 );
	    ph.processor_waiting_conf = getDoubleAttribute( attributes, Xproc_waiting, 0.0 );
	}

	void
	Expat_Document::handleActivityConfResults( const DocumentObject * task, const DocumentObject * activity, const DocumentObject * dest, const XML_Char ** attributes )
	{
	    const unsigned int a = find_or_add_activity( task, activity );
	    if ( !a ) return;

	    activity_info& act = activity_tab[pass][a];
	    act.serv_conf	       = getDoubleAttribute( attributes, Xservice_time, 0.0 );
	    act.var_conf	       = getDoubleAttribute( attributes, Xservice_time_variance, 0.0 );
	    act.processor_waiting_conf = getDoubleAttribute( attributes, Xproc_waiting, 0.0 );
	}

	void
	Expat_Document::handleEntryFwdCallConfResults( const DocumentObject * entry, const DocumentObject * phase, const DocumentObject * dest, const XML_Char ** attributes )
	{
	    const unsigned int e = find_or_add_entry( entry );
	    const unsigned int d = find_or_add_entry( dest );
	    if ( !e || !d ) return;		/* Bogus entry. */

	    call_info & call = entry_tab[pass][e].phase[0].to[d];
	    call.wait_conf     = getDoubleAttribute( attributes, Xwaiting, 0.0 );
	    call.wait_var_conf = getDoubleAttribute( attributes, Xwaiting_variance, 0.0 );
	}


	void
	Expat_Document::handlePhaseRNVCallConfResults( const DocumentObject * entry, const DocumentObject * phase, const DocumentObject * dest, const XML_Char ** attributes )
	{
	    const unsigned int e = find_or_add_entry( entry );
	    const unsigned int p = strtol( phase, 0, 10 );
	    const unsigned int d = find_or_add_entry( dest );
	    if ( !e || !p || p > MAX_PHASES || !d ) return;		/* Bogus entry. */

	    call_info & call = entry_tab[pass][e].phase[p-1].to[d];
	    call.wait_conf     = getDoubleAttribute( attributes, Xwaiting, 0.0 );
	    call.wait_var_conf = getDoubleAttribute( attributes, Xwaiting_variance, 0.0 );
	}

	void
	Expat_Document::handleActivityRNVCallConfResults( const DocumentObject * task, const DocumentObject * activity, const DocumentObject * dest, const XML_Char ** attributes )
	{
	    const unsigned int a = find_or_add_activity( task, activity );
	    const unsigned int d = find_or_add_entry( dest );
	    if ( !a || !d ) return;

	    call_info & call = activity_tab[pass][a].to[d];
	    call.wait_conf     = getDoubleAttribute( attributes, Xwaiting, 0.0 );
	    call.wait_var_conf = getDoubleAttribute( attributes, Xwaiting_variance, 0.0 );
	}

	void
	Expat_Document::handlePhaseSNRCallConfResults( const DocumentObject * entry, const DocumentObject * phase, const DocumentObject * dest, const XML_Char ** attributes )
	{
	    const unsigned int e = find_or_add_entry( entry );
	    const unsigned int p = strtol( phase, 0, 10 );
	    const unsigned int d = find_or_add_entry( dest );
	    if ( !e || !p || p  > MAX_PHASES || !d ) return;		/* Bogus entry. */

	    call_info & call = entry_tab[pass][e].phase[p-1].to[d];
	    call.snr_wait_conf     = getDoubleAttribute( attributes, Xwaiting, 0.0 );
	    call.snr_wait_var_conf = getDoubleAttribute( attributes, Xwaiting_variance, 0.0 );
	    call.loss_prob_conf    = getDoubleAttribute( attributes, Xloss_probability, 0.0 );
	}

	void
	Expat_Document::handleActivitySNRCallConfResults( const DocumentObject * task, const DocumentObject * activity, const DocumentObject * dest, const XML_Char ** attributes )
	{
	    const unsigned int a = find_or_add_activity( task, activity );
	    const unsigned int d = find_or_add_entry( dest );
	    if ( !a || !d ) return;

	    call_info & call = activity_tab[pass][a].to[d];
	    call.snr_wait_conf     = getDoubleAttribute( attributes, Xwaiting, 0.0 );
	    call.snr_wait_var_conf = getDoubleAttribute( attributes, Xwaiting_variance, 0.0 );
	    call.loss_prob_conf    = getDoubleAttribute( attributes, Xloss_probability, 0.0 );
	}

	void
	Expat_Document::handleJoinConfResults( const DocumentObject * task, const DocumentObject *, const DocumentObject *, const XML_Char ** attributes )
	{
	    join_info_t * data = static_cast<join_info_t *>(_stack.top().data);
	    if ( data ) {
		data->mean_conf     = getDoubleAttribute( attributes, Xjoin_waiting );
		data->variance_conf = getDoubleAttribute( attributes, Xjoin_variance, 0.0 );
	    }
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
		    char * end_ptr = 0;
		    const double value = strtod( *(attributes+1), &end_ptr );
		    if ( value < 0 || ( end_ptr && *end_ptr != '\0' ) ) throw std::invalid_argument( *(attributes+1) );
		    return value;
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
		    const double value = strtod( *(attributes+1), &end_ptr );		// Xerces can output integers as floats...
		    if ( value < 0 || fmod( value, 1.0 ) != 0.0 ) throw std::invalid_argument( *(attributes+1) );
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
		    double hrs   = 0.0;
		    double mins  = 0.0;
		    double secs  = 0.0;

		    sscanf( *(attributes+1), "%lf:%lf:%lf", &hrs, &mins, &secs );
		    return hrs * 3600.0 + mins * 60.0 + secs;
		}
	    }
	    return 0;
	}

	const unsigned int
	Expat_Document::getEnumerationAttribute( const XML_Char ** attributes, const XML_Char * attribute, const XML_Char ** enum_strings, const unsigned int default_value ) const
	{
	    for ( ; *attributes; attributes += 2 ) {
		if ( strcasecmp( *attributes, attribute ) == 0 ) {
		    for ( unsigned int i = 0; enum_strings[i]; ++i ) {
			if ( strcasecmp( enum_strings[i], *(attributes+1) ) == 0 ) return i;
		    }
		}
	    }
	    return default_value;
	}

	bool
	Expat_Document::parse_stack_t::operator==( const XML_Char * str ) const
	{
	    return element == str;
	}


	/*
	 * Results for most of the elements of an lqn-model are of a common type in the schema.  This table is used to
	 * invoke the appropriate function.  The function is implemented in dom objects that support the result.
	 */

	void
	Expat_Document::init_tables()
	{
            precedence_table[Xpre] =       ActivityList::JOIN_ACTIVITY_LIST;
            precedence_table[Xpre_or] =    ActivityList::OR_JOIN_ACTIVITY_LIST;
            precedence_table[Xpre_and] =   ActivityList::AND_JOIN_ACTIVITY_LIST;
            precedence_table[Xpost] =      ActivityList::FORK_ACTIVITY_LIST;
            precedence_table[Xpost_or] =   ActivityList::OR_FORK_ACTIVITY_LIST;
            precedence_table[Xpost_and] =  ActivityList::AND_FORK_ACTIVITY_LIST;
            precedence_table[Xpost_loop] = ActivityList::REPEAT_ACTIVITY_LIST;

            confidence_result_table[&Expat_Document::handleProcessorResults]        = &Expat_Document::handleProcessorConfResults;
            confidence_result_table[&Expat_Document::handleGroupResults]            = &Expat_Document::handleGroupConfResults;
            confidence_result_table[&Expat_Document::handleTaskResults]             = &Expat_Document::handleTaskConfResults;
            confidence_result_table[&Expat_Document::handleEntryResults]            = &Expat_Document::handleEntryConfResults;
            confidence_result_table[&Expat_Document::handlePhaseResults]            = &Expat_Document::handlePhaseConfResults;
            confidence_result_table[&Expat_Document::handleActivityResults]         = &Expat_Document::handleActivityConfResults;
            confidence_result_table[&Expat_Document::handleEntryFwdCallResults]     = &Expat_Document::handleEntryFwdCallConfResults;
            confidence_result_table[&Expat_Document::handlePhaseRNVCallResults]     = &Expat_Document::handlePhaseRNVCallConfResults;
            confidence_result_table[&Expat_Document::handleActivityRNVCallResults]  = &Expat_Document::handleActivityRNVCallConfResults;
            confidence_result_table[&Expat_Document::handlePhaseSNRCallResults]     = &Expat_Document::handlePhaseSNRCallConfResults;
            confidence_result_table[&Expat_Document::handleActivitySNRCallResults]  = &Expat_Document::handleActivitySNRCallConfResults;
            confidence_result_table[&Expat_Document::handleJoinResults]             = &Expat_Document::handleJoinConfResults;

	}
    }
}
