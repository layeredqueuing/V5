/* -*- c++ -*-
 * $Id: xerces_document.cpp 11963 2014-04-10 14:36:42Z greg $
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

#include <stdexcept>
#include <iomanip>
#include "xerces_document.h"
#include "xerces_common.h"
#include "xerces_write.h"
#include "dom_histogram.h"

#include <xercesc/util/PlatformUtils.hpp>

#include <xercesc/dom/DOMImplementation.hpp>
#include <xercesc/dom/DOMImplementationLS.hpp>

#include <xercesc/parsers/XercesDOMParser.hpp>
#include <xercesc/framework/StdOutFormatTarget.hpp>
#include <xercesc/framework/LocalFileFormatTarget.hpp>
#include <xercesc/sax/SAXParseException.hpp>

#include <string>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <map>
#include <vector>
#include <ctype.h>
#include <cstdarg>

#include <assert.h>

#include "input.h"
#include "error.h"
#include "glblerr.h"
#include "xerces_common.h"
#include "dom_phase.h"
#include "dom_task.h"
#include "confidence_intervals.h"

namespace LQIO {
    namespace DOM {
	using namespace std;

#define PARSER_LQN_VERSION 	1.0   /* version of lqn.xsd this parser supports */
#define PARSER_LQNCORE_VERSION 	1.1   /* version of lqn-core.xsd this parser supports */

	/* Element and attribute names... they are pre-computed in the constructor. */

	XMLCh * Xerces_Document::XPH1PH2 = 0;
	XMLCh * Xerces_Document::Xactivity = 0;
	XMLCh * Xerces_Document::Xasynch_call = 0;
	XMLCh * Xerces_Document::Xbegin = 0;
	XMLCh * Xerces_Document::Xbound_to_entry = 0;
	XMLCh * Xerces_Document::Xcall_order = 0;
	XMLCh * Xerces_Document::Xcalls_mean = 0;
	XMLCh * Xerces_Document::Xcap = 0;
	XMLCh * Xerces_Document::Xcomment = 0;
	XMLCh * Xerces_Document::Xconf_95 = 0;
	XMLCh * Xerces_Document::Xconf_99 = 0;
	XMLCh * Xerces_Document::Xconv_val = 0;
	XMLCh * Xerces_Document::Xconv_val_result = 0;
	XMLCh * Xerces_Document::Xcore = 0;
	XMLCh * Xerces_Document::Xcount = 0;
	XMLCh * Xerces_Document::Xdest = 0;
	XMLCh * Xerces_Document::Xdeterministic = 0;
	XMLCh * Xerces_Document::Xdrop_probability = 0;
	XMLCh * Xerces_Document::Xelapsed_time = 0;
	XMLCh * Xerces_Document::Xend = 0;
	XMLCh * Xerces_Document::Xentry = 0;
	XMLCh * Xerces_Document::Xentry_phase_activities = 0;
	XMLCh * Xerces_Document::Xfanin = 0;
	XMLCh * Xerces_Document::Xfanout = 0;
	XMLCh * Xerces_Document::Xfaults = 0;
	XMLCh * Xerces_Document::Xforwarding = 0;
	XMLCh * Xerces_Document::Xgroup = 0;
	XMLCh * Xerces_Document::Xhistogram_bin = 0;
	XMLCh * Xerces_Document::Xhost_demand_cvsq = 0;
	XMLCh * Xerces_Document::Xhost_demand_mean = 0;
	XMLCh * Xerces_Document::Xmax_service_time = 0;
	XMLCh * Xerces_Document::Xinitially = 0;
	XMLCh * Xerces_Document::Xit_limit = 0;
	XMLCh * Xerces_Document::Xiterations = 0;
	XMLCh * Xerces_Document::Xjoin_variance = 0;
	XMLCh * Xerces_Document::Xjoin_waiting = 0;
	XMLCh * Xerces_Document::Xlqn_model = 0;
	XMLCh * Xerces_Document::Xlqx = 0;
	XMLCh * Xerces_Document::Xmax = 0;
	XMLCh * Xerces_Document::Xmin = 0;
	XMLCh * Xerces_Document::Xmultiplicity = 0;
	XMLCh * Xerces_Document::Xmva_info = 0;
	XMLCh * Xerces_Document::Xname = 0;
	XMLCh * Xerces_Document::Xnone = 0;
	XMLCh * Xerces_Document::Xnumber_bins = 0;
	XMLCh * Xerces_Document::Xopen_arrival_rate = 0;
	XMLCh * Xerces_Document::Xopen_wait_time = 0;
	XMLCh * Xerces_Document::Xoverflow_bin = 0;
	XMLCh * Xerces_Document::Xparam = 0;
	XMLCh * Xerces_Document::Xphase = 0;
	XMLCh * Xerces_Document::Xphase_proc_waiting[DOM::Phase::MAX_PHASE] = { 0, 0, 0 };
	XMLCh * Xerces_Document::Xphase_service_time[DOM::Phase::MAX_PHASE] = { 0, 0, 0 };
	XMLCh * Xerces_Document::Xphase_service_time_variance[DOM::Phase::MAX_PHASE] = { 0, 0, 0 };
	XMLCh * Xerces_Document::Xphase_utilization[DOM::Phase::MAX_PHASE] = { 0, 0, 0 };
	XMLCh * Xerces_Document::Xphase_waiting[DOM::Phase::MAX_PHASE] = { 0, 0, 0 };
	XMLCh * Xerces_Document::Xphase_waiting_variance[DOM::Phase::MAX_PHASE] = { 0, 0, 0 };
	XMLCh * Xerces_Document::Xplatform_info = 0;
	XMLCh * Xerces_Document::Xpragma = 0;
	XMLCh * Xerces_Document::Xprecedence = 0;
	XMLCh * Xerces_Document::Xprint_int = 0;
	XMLCh * Xerces_Document::Xpriority = 0;
	XMLCh * Xerces_Document::Xprob = 0;
	XMLCh * Xerces_Document::Xproc_utilization = 0;
	XMLCh * Xerces_Document::Xproc_waiting = 0;
	XMLCh * Xerces_Document::Xprocessor = 0;
	XMLCh * Xerces_Document::Xquantum = 0;
	XMLCh * Xerces_Document::Xqueue_length = 0;
	XMLCh * Xerces_Document::Xquorum = 0;
	XMLCh * Xerces_Document::Xreplication = 0;
	XMLCh * Xerces_Document::Xreply_activity = 0;
	XMLCh * Xerces_Document::Xreply_entry = 0;
	XMLCh * Xerces_Document::Xresult_activity = 0;
	XMLCh * Xerces_Document::Xresult_call = 0;
	XMLCh * Xerces_Document::Xresult_conf_95 = 0;
	XMLCh * Xerces_Document::Xresult_conf_99 = 0;
	XMLCh * Xerces_Document::Xresult_entry = 0;
	XMLCh * Xerces_Document::Xresult_forwarding = 0;
	XMLCh * Xerces_Document::Xresult_general = 0;
	XMLCh * Xerces_Document::Xresult_group = 0;
	XMLCh * Xerces_Document::Xresult_join_delay = 0;
	XMLCh * Xerces_Document::Xresult_processor = 0;
	XMLCh * Xerces_Document::Xresult_task = 0;
	XMLCh * Xerces_Document::Xr_lock =	0;
	XMLCh * Xerces_Document::Xr_unlock = 0;
	XMLCh * Xerces_Document::Xrwlock =0;
	XMLCh * Xerces_Document::Xrwlock_reader_waiting =0;
	XMLCh * Xerces_Document::Xrwlock_reader_waiting_variance = 0;
	XMLCh * Xerces_Document::Xrwlock_reader_holding =	0;
	XMLCh * Xerces_Document::Xrwlock_reader_holding_variance =	0;
	XMLCh * Xerces_Document::Xrwlock_reader_utilization =0;
	XMLCh * Xerces_Document::Xrwlock_writer_waiting =	0;
	XMLCh * Xerces_Document::Xrwlock_writer_waiting_variance =0;
	XMLCh * Xerces_Document::Xrwlock_writer_holding =	0;
	XMLCh * Xerces_Document::Xrwlock_writer_holding_variance =	0;
	XMLCh * Xerces_Document::Xrwlock_writer_utilization =	0;
 	XMLCh * Xerces_Document::Xscheduling = 0;
	XMLCh * Xerces_Document::Xsemaphore = 0;
	XMLCh * Xerces_Document::Xsemaphore_waiting = 0;
	XMLCh * Xerces_Document::Xsemaphore_waiting_variance = 0;
	XMLCh * Xerces_Document::Xsemaphore_utilization = 0;
	XMLCh * Xerces_Document::Xservice = 0;
	XMLCh * Xerces_Document::Xservice_time = 0;
	XMLCh * Xerces_Document::Xservice_time_distribution = 0;
	XMLCh * Xerces_Document::Xservice_time_variance = 0;
	XMLCh * Xerces_Document::Xshare = 0;
	XMLCh * Xerces_Document::Xsignal = 0;
	XMLCh * Xerces_Document::Xsolver_info = 0;
	XMLCh * Xerces_Document::Xsolver_parameters = 0;
	XMLCh * Xerces_Document::Xsource = 0;
	XMLCh * Xerces_Document::Xspeed_factor = 0;
	XMLCh * Xerces_Document::Xsquared_coeff_variation = 0;
	XMLCh * Xerces_Document::Xstep = 0;
	XMLCh * Xerces_Document::Xstep_squared = 0;
	XMLCh * Xerces_Document::Xsubmodels = 0;
	XMLCh * Xerces_Document::Xsynch_call = 0;
	XMLCh * Xerces_Document::Xsystem_cpu_time = 0;
	XMLCh * Xerces_Document::Xtask = 0;
	XMLCh * Xerces_Document::Xtask_activities = 0;
	XMLCh * Xerces_Document::Xthink_time = 0;
	XMLCh * Xerces_Document::Xthroughput = 0;
	XMLCh * Xerces_Document::Xthroughput_bound = 0;
	XMLCh * Xerces_Document::Xtype = 0;
	XMLCh * Xerces_Document::Xunderflow_bin = 0;
	XMLCh * Xerces_Document::Xunderrelax_coeff = 0;
	XMLCh * Xerces_Document::Xuser_cpu_time = 0;
	XMLCh * Xerces_Document::Xutilization = 0;
	XMLCh * Xerces_Document::Xvalid = 0;
	XMLCh * Xerces_Document::Xvalue = 0;
	XMLCh * Xerces_Document::Xwait = 0;
	XMLCh * Xerces_Document::Xwait_squared = 0;
	XMLCh * Xerces_Document::Xwaiting = 0;
	XMLCh * Xerces_Document::Xwaiting_variance = 0;
	XMLCh * Xerces_Document::Xw_lock =	0;
	XMLCh * Xerces_Document::Xw_unlock = 0;
	XMLCh * Xerces_Document::Xxml_debug = 0;

	std::map<XMLCh *,ActivityList::ActivityListType,Xerces_Document::ltXMLCh> Xerces_Document::Xpre_post;
	unsigned Xerces_Document::__indent = 0;
        Xerces_Document * Xerces_Document::__xercesDOM = 0;

	bool
	Xerces_Document::load( Document& document, const std::string& filename, lqio_params_stats* ioVars, unsigned& errorCode )
	{
	    try {
		LQIO_lineno = 0;		/* Suppress line numbers */

		__xercesDOM = new Xerces_Document( document );
		if ( __xercesDOM->parse( filename.c_str() ) ) {
		    __xercesDOM->handleModel( inputFileDOM->getDocumentElement() );
		    const std::string& program_text = document.getLQXProgramText();
		    if ( program_text.size() ) {
			/* If we have an LQX program, then we need to compute */
			LQX::Program* program = LQX::Program::loadFromText(filename.c_str(), document.getLQXProgramLineNumber(), program_text.c_str());
			if (program == NULL) {
			    LQIO::solution_error( LQIO::ERR_LQX_COMPILATION, filename.c_str() );
			} 
			document.setLQXProgram( program );
		    }
		} else {
		    Document::io_vars->anError = true;
		}
	    }
	    catch (const XMLException& e) {
		LQIO::solution_error( LQIO::ERR_PARSE_ERROR, StrX(e.getMessage()).asCStr() );
	    }
	    catch (const DOMException& e) {
		const unsigned int maxChars = 2047;
		XMLCh errText[maxChars + 1];

		if (DOMImplementation::loadDOMExceptionMsg(e.code, errText, maxChars)) {
		    solution_error( ERR_PARSE_ERROR, StrX(errText).asCStr() );
		} else {
		    char errmsg[64];
		    sprintf( errmsg, "DOMException code is: %d", e.code );
		    solution_error( ERR_PARSE_ERROR, errmsg );
		}
	    }
	    catch (const SAXException& e) {
		LQIO::solution_error( LQIO::ERR_PARSE_ERROR, StrX(e.getMessage()).asCStr() );
	    }
	    catch ( const element_error & e ) {
		LQIO::solution_error( LQIO::ERR_PARSE_ERROR, e.what() );
	    }
	    catch ( const LQIO::missing_attribute & e ) {
		LQIO::solution_error( LQIO::ERR_PARSE_ERROR, e.what() );
	    }
	    catch ( const std::runtime_error & e ) {
		LQIO::solution_error( LQIO::ERR_PARSE_ERROR, e.what() );
	    }
	    catch (...) {
		Document::io_vars->anError = true;
		throw;
	    }
	    return Document::io_vars->anError == false;
	}


	Xerces_Document::Xerces_Document( Document& document )
	    : _document( document ), _parser(0), errReporter(0)
	{
	    assert (inputFileDOM == NULL);		/* Singleton */

	    /* Initialize the XML4C2 system */
	    try {
		XMLPlatformUtils::Initialize();
	    }

	    catch(const XMLException &toCatch) {
		cerr << "Error during Xerces-c Initialization:   "
		     << StrX(toCatch.getMessage()) << endl;
		exit(1);
	    }

	    /* Initialize common tag strings */
	
	    initialize_strings();

	    /* Create our parser, then attach an error handler to the parser.
	       The parser will call back to methods of the ErrorHandler if it
	       discovers errors during the course of parsing the XML document. */

	    _parser = new XercesDOMParser;
	    _parser->setValidationScheme(XercesDOMParser::Val_Always);
	    _parser->setDoNamespaces(true);
	    _parser->setDoSchema(true);
	    _parser->setValidationSchemaFullChecking(true);

	    errReporter = new DOMTreeErrorReporter();
	    _parser->setErrorHandler(errReporter);
	}


	Xerces_Document::~Xerces_Document()
	{
	    /* And call the termination method */

	    XMLPlatformUtils::Terminate();

	    errReporter = NULL;
	    _parser = NULL;
	    __xercesDOM = NULL;

	    inputFileDOM = NULL;
	}

	/* ---------------------------------------------------------------- */
        /* DOM input.                                                       */
	/* ---------------------------------------------------------------- */

	/*  Parse the XML file, catching any XML exceptions that might propogate
	    out of it. */

	bool
	Xerces_Document::parse( const char * filename )
	{
	    _parser->parse(filename);

	    /* If the parse was successful, output the document data from the DOM tree */

	    if ( errReporter->getSawErrors() ) return false;

	    inputFileDOM = _parser->getDocument();
	    _root = inputFileDOM->getDocumentElement();
	    const XMLCh *rootElementName = _root->getTagName();

	    /* Do quick sanity check to see if the XML file is a lqn-model */

	    // input_error("ERROR - this does not appear to be a proper LQN model file.");
	    if (rootElementName == NULL || XMLString::compareIStringASCII(rootElementName, Xlqn_model) != 0 ) return false;

	    StrX debug( _root->getAttribute(Xxml_debug) );
	    Document::__debugXML |= debug.optBool();

	    return true;
	}

	/*
	 * <lqn-model>
	 *   <solver-params...>
	 *   <processor...>
	 *   <lqx...>
	 * </lqn-model>
	 */

	void
	Xerces_Document::handleModel( const DOMElement * lqnModel )
	{
	    if ( Document::__debugXML ) cerr << start_element( lqnModel ) << endl;

	    DOMNodeList * lqnModelList = lqnModel->getChildNodes();
	    unsigned state = 0;
	    for ( unsigned x = 0; x < lqnModelList->getLength(); x++ ) {
		const DOMElement *child_element = dynamic_cast<DOMElement *>(lqnModelList->item(x));
		if ( !child_element ) continue;		/* Ignore other stuff. */
		if ( state == 0 && XMLString::compareIStringASCII(child_element->getTagName(), Xsolver_parameters) == 0 ) {
		    handleModelParameters( child_element );
		    state = 1;
		} else if ( (state == 1 || state == 2) && XMLString::compareIStringASCII(child_element->getTagName(), Xprocessor) == 0 ) {
		    handleProcessor( child_element );
		    state = 2;
		} else if ( state == 2 && XMLString::compareIStringASCII(child_element->getTagName(), Xlqx) == 0 ) {
		    handleCalls( lqnModel );	/* All processors read - go back and add calls */
		    handleLQX( child_element );
		    state = 3;
		} else {
		    throw element_error( StrX(child_element->getTagName()).asCStr() );
		}
	    }
	    if ( state < 2 ) {
		internal_error( __FILE__, __LINE__, "handleModel" );
	    } else if ( state == 2 ) {
		handleCalls( lqnModel );	/* All processors read, but no LQX - go back and add calls */
	    }

	    if ( Document::__debugXML ) cerr << end_element( lqnModel ) << endl;

	}

	void
	Xerces_Document::handleModelParameters( const DOMElement * param_element )
	{
	    assert( param_element );	// Dynamic cast failed?
	    _document.setModelParameters(StrX(param_element->getAttribute(Xcomment)).asCStr(),
					 _document.db_build_parameter_variable(StrX(param_element->getAttribute(Xconv_val)).asCStr(),NULL),
					 _document.db_build_parameter_variable(StrX(param_element->getAttribute(Xit_limit)).asCStr(),NULL),
					 _document.db_build_parameter_variable(StrX(param_element->getAttribute(Xprint_int)).asCStr(),NULL),
					 _document.db_build_parameter_variable(StrX(param_element->getAttribute(Xunderrelax_coeff)).asCStr(),NULL),
					 param_element);

	    /* handle pragmas and results */

	    const DOMNodeList *nodeList 	= param_element->getChildNodes();
	    const unsigned int nodeListLength	= nodeList->getLength();

	    for ( unsigned x = 0; x < nodeListLength; x++ ) {
		const DOMElement * child_element = dynamic_cast<DOMElement *>(nodeList->item(x));
		if ( !child_element ) continue;
		if ( Document::__debugXML ) cerr << simple_element( child_element ) << endl;

		if ( XMLString::compareIStringASCII(child_element->getTagName(), Xpragma) == 0 ) {
		    /* Get the pragmas if there are any */

		    StrX parameter(child_element->getAttribute(Xparam));
		    if ( !parameter ) {
			throw missing_attribute( StrX(Xpragma).asCStr() );
			continue;
		    }
		    _document.addPragma(parameter.asCStr(),StrX(child_element->getAttribute(Xvalue)).asCStr());

		} else if ( XMLString::compareIStringASCII(child_element->getTagName(), Xresult_general) == 0 ) {
		    /* Get the results if there are any */

		    double iterations = StrX(child_element->getAttribute(Xiterations)).optLong();
		    _document.setResultValid( StrX(child_element->getAttribute(Xvalid)).optBool() );
		    _document.setResultConvergenceValue( StrX(child_element->getAttribute(Xconv_val_result)).optDouble() );
		    _document.setResultIterations( iterations );
		    _document.setResultElapsedTime( StrX(child_element->getAttribute(Xelapsed_time)).optTime() );
		    _document.setResultSysTime( StrX(child_element->getAttribute(Xsystem_cpu_time)).optTime() );
		    _document.setResultUserTime( StrX(child_element->getAttribute(Xuser_cpu_time)).optTime() );	
		    StrX platform_info(child_element->getAttribute(Xplatform_info));
		    if ( platform_info.optCStr() ) _document.setResultPlatformInformation( platform_info.asCStr() );
		    if ( 1 < iterations && iterations <= 30 ) {
			const_cast<ConfidenceIntervals *>(&_conf_95)->set_blocks( iterations );
		    }

		    const DOMNodeList *nodeList2       = child_element->getChildNodes();
		    const unsigned int nodeList2Length = nodeList2->getLength();

		    for ( unsigned int y = 0; y < nodeList2Length; ++y ) {
			const DOMElement * stats_element = dynamic_cast<DOMElement *>(nodeList2->item(y));
			if ( !stats_element ) continue;
			if ( XMLString::compareIStringASCII(child_element->getTagName(), Xmva_info) == 0 ) {
			    _document.setMVAStatistics( StrX(stats_element->getAttribute(Xsubmodels)).optLong(),
							StrX(stats_element->getAttribute(Xcore)).optLong(),
							StrX(stats_element->getAttribute(Xstep)).optDouble(),
							StrX(stats_element->getAttribute(Xstep_squared)).optDouble(),
							StrX(stats_element->getAttribute(Xwait)).optDouble(),
							StrX(stats_element->getAttribute(Xwait_squared)).optDouble(),
							StrX(stats_element->getAttribute(Xfaults)).optLong() );
			}
		    }
		}
	    }
	}


	/*
	 * <processor name="p0" scheduling="inf">
	 *   <task>
	 */

	void
	Xerces_Document::handleProcessor( const DOMElement *processor_element )
	{
	    if ( Document::__debugXML ) cerr << start_element( processor_element ) << endl;

	    const StrX processorName(processor_element->getAttribute(Xname));
	    if ( !processorName ) missing_attribute( StrX(processor_element->getTagName()).asCStr() );

	    const scheduling_type scheduling_flag = static_cast<scheduling_type>(getEnumerationFromElement( processor_element->getAttribute(Xscheduling), schedulingTypeXMLString, SCHEDULE_FIFO ));
	    Processor *  processor = new Processor( &_document, processorName.asCStr(),
						    scheduling_flag,
						    _document.db_build_parameter_variable(StrX(processor_element->getAttribute(Xmultiplicity)).asCStr(),NULL),
						    StrX(processor_element->getAttribute(Xreplication)).asLong(),
						    processor_element );

	    const StrX quantum(processor_element->getAttribute(Xquantum));
	    const char * quantum_str = quantum.optCStr();
	    if ( quantum_str && strcmp( quantum_str, "0" ) != 0 ) {		/* Check for default from schema and ignore. */
		if ( scheduling_flag == SCHEDULE_FIFO
		     || scheduling_flag == SCHEDULE_HOL
		     || scheduling_flag == SCHEDULE_PPR
		     || scheduling_flag == SCHEDULE_RAND ) {
		    input_error2( LQIO::WRN_QUANTUM_SCHEDULING, processorName.asCStr(), scheduling_type_str[scheduling_flag] );
		} else {
		    processor->setQuantum( _document.db_build_parameter_variable(quantum.asCStr(),NULL) );
		}
	    } else if ( scheduling_flag == SCHEDULE_CFS ) {
		input_error2( LQIO::ERR_NO_QUANTUM_SCHEDULING, processorName.asCStr(), scheduling_type_str[scheduling_flag] );
	    }

	    const StrX rate(processor_element->getAttribute(Xspeed_factor));
	    if ( rate.optCStr() ) {
		processor->setRate( _document.db_build_parameter_variable( rate.asCStr(), NULL ) );
	    } else {
		processor->setRate( new ConstantExternalVariable( 1. ) );
	    }

	    _document.addProcessorEntity( processor );
	    Document::io_vars->n_processors += 1;
	
	    /* handle groups, tasks, or results */

	    const DOMNodeList *nodeList 	= processor_element->getChildNodes();
	    const unsigned int nodeListLength	= nodeList->getLength();
	    int state = 0;	/* 1 == tasks only, 2 == groups only. */
	    int task_count = 0;

	    for ( unsigned x = 0; x < nodeListLength; x++ ) {
		const DOMElement * child_element = dynamic_cast<DOMElement *>(nodeList->item(x));
		if ( !child_element ) continue;

		if ( (state == 0 || state == 1) && XMLString::compareIStringASCII(child_element->getTagName(), Xtask) == 0 ) {
		    task_count += handleTask( child_element,processorName.asCStr(),0 );
		    state = 1;

		} else if ((state == 0 || state == 2 ) && XMLString::compareIStringASCII(child_element->getTagName(), Xgroup) == 0 ) {
		    state = 2;
		    task_count += handleGroup( child_element, processorName.asCStr() );

		} else if (state == 0 && XMLString::compareIStringASCII(child_element->getTagName(), Xresult_processor) == 0 ) {
		    if ( Document::__debugXML ) cerr << simple_element( child_element ) << endl;

		    processor->setResultUtilization( StrX(child_element->getAttribute(Xutilization)).optDouble() );

		    DOMNodeList * result_list = child_element->getElementsByTagName(Xresult_conf_95);
		    const DOMElement * result95_element = dynamic_cast<DOMElement *>(result_list->item(0));
		    if ( result95_element ) {
			processor->setResultUtilizationVariance( invert(StrX(result95_element->getAttribute(Xutilization)).optDouble()) );
		    }

		} else {
		    throw element_error( StrX(child_element->getTagName()).asCStr() );

		}
	    }

	    if ( task_count == 0 ) {
		input_error2( WRN_NO_TASKS_DEFINED_FOR_PROCESSOR, processorName.asCStr() );
	    }

	    if ( Document::__debugXML ) cerr << end_element( processor_element ) << endl;
	}



	/*
	 *  <processor name="server" scheduling="cfs" quantum="0.1">
	 *    <group name="g1" share="0.7">
	 *       <task name="s0" scheduling="fcfs">
	 */

	int
	Xerces_Document::handleGroup(const DOMElement *group_element, const char *processorName)
	{
	    if ( Document::__debugXML ) cerr << start_element( group_element ) << endl;

	    StrX groupName(group_element->getAttribute(Xname));
	    if ( !groupName ) throw missing_attribute( StrX(group_element->getTagName()).asCStr() );

	    Processor* processor = _document.getProcessorByName(processorName);
	    if (processor == NULL) {
		return 0;
	    } 
	    
	    /* Check if this group exists yet */
	    if (_document.getGroupByName(groupName.asCStr()) != NULL) {
		LQIO::input_error2( LQIO::ERR_DUPLICATE_SYMBOL, "Group", groupName.asCStr() );
		return 0;
	    }

	    Group* group = 0;
	    if ( processor->getSchedulingType() != SCHEDULE_CFS ) {
		/* Ignore group and carry on. */
		LQIO::input_error2( LQIO::WRN_NON_CFS_PROCESSOR, groupName.asCStr(), processor->getName().c_str() );
	    } else {
		/* Store the group inside of the Document */
		group = new Group(&_document,
				  groupName.asCStr(),
				  processor,
				  _document.db_build_parameter_variable( StrX(group_element->getAttribute(Xshare)).asCStr(), NULL ),
				  StrX(group_element->getAttribute(Xcap)).asBool(),
				  group_element );
		_document.addGroup(group);
		processor->addGroup(group);

		Document::io_vars->n_groups += 1;
	    }

	    const DOMNodeList *nodeList = group_element->getChildNodes();
	    const unsigned int  nodeListLength = nodeList->getLength();

	    int task_count = 0;
	    for ( unsigned x = 0; x < nodeListLength; x++ ) {
		const DOMElement * child_element = dynamic_cast<DOMElement *>(nodeList->item(x));
		if ( !child_element ) continue;
		if ( XMLString::compareIStringASCII(child_element->getTagName(), Xtask) == 0 ) {
		    task_count += handleTask( child_element, processorName, groupName.asCStr() );

		} else if ( XMLString::compareIStringASCII(child_element->getTagName(), Xresult_group) == 0 ) {
		    if ( Document::__debugXML ) cerr << simple_element( child_element ) << endl;
		    if ( !group ) continue;

		    group->setResultUtilization( StrX(child_element->getAttribute(Xutilization)).optDouble() );

		    DOMNodeList * result_list = child_element->getElementsByTagName(Xresult_conf_95);
		    const DOMElement * result95_element = dynamic_cast<DOMElement *>(result_list->item(0));
		    if ( result95_element ) {
			group->setResultUtilizationVariance( invert(StrX(result95_element->getAttribute(Xutilization)).optDouble()) );
		    }

		} else {
		    throw element_error( StrX(child_element->getTagName()).asCStr() );

		}
	    }

	    if ( Document::__debugXML ) cerr << end_element( group_element ) << endl;

	    return task_count;
	}



	/*
	 * <task name="s0" scheduling="fcfs">
	 *   <entry name="s0" type="PH1PH2">
	 */

	int
	Xerces_Document::handleTask( const DOMElement *task_element, const char *processorName, const char *groupName )
	{
	    if ( !task_element ) {
		internal_error( __FILE__, __LINE__, "handleTasks" );
		return 0;
	    }
	    if ( Document::__debugXML ) cerr << start_element( task_element ) << endl;

	    StrX taskName(task_element->getAttribute(Xname));
	    if ( !taskName ) throw missing_attribute( StrX(Xtask).asCStr() );

	    std::vector<Entry *> entries;
	    getEntryList(task_element, entries);
	    if (entries.size()== 0) {
		input_error2( ERR_NO_ENTRIES_DEFINED_FOR_TASK, taskName.asCStr());
		return 0;
	    }

	    Processor* processor = _document.getProcessorByName(processorName);
	    if ( !processor ) internal_error( __FILE__, __LINE__, "handleTasks" );		// We should have a processor.

	    /* Ditto for the group, if specified */
	    Group * group = NULL;
	    if ( groupName ) {
		group = _document.getGroupByName(groupName);
	    }

	    Task * task = 0;
	    const scheduling_type sched_type = static_cast<scheduling_type>(getEnumerationFromElement(task_element->getAttribute(Xscheduling),schedulingTypeXMLString,SCHEDULE_FIFO));
	    const StrX tokens = task_element->getAttribute(Xinitially);

	    if ( sched_type == SCHEDULE_SEMAPHORE ) {
		task = new SemaphoreTask( &_document, taskName.asCStr(), entries, processor,
					  _document.db_build_parameter_variable( StrX(task_element->getAttribute(Xqueue_length)).asCStr(), NULL ),
					  StrX(task_element->getAttribute(Xpriority)).asLong(),
					  _document.db_build_parameter_variable(StrX(task_element->getAttribute(Xmultiplicity)).asCStr(),NULL),
					  StrX(task_element->getAttribute(Xreplication)).asLong(),
					  group,						/* For Group */
					  task_element );
		if ( tokens.optCStr() != 0 && tokens.asLong() == 0 ) {
		    dynamic_cast<SemaphoreTask *>(task)->setInitialState(SemaphoreTask::INITIALLY_EMPTY);
		}

	    }else if(sched_type == SCHEDULE_RWLOCK){
		task = new RWLockTask( &_document, taskName.asCStr(), entries, processor,
				       _document.db_build_parameter_variable(StrX(task_element->getAttribute(Xqueue_length)).asCStr(),NULL),
				       StrX(task_element->getAttribute(Xpriority)).asLong(),
				       _document.db_build_parameter_variable(StrX(task_element->getAttribute(Xmultiplicity)).asCStr(),NULL),
				       StrX(task_element->getAttribute(Xreplication)).asLong(),
				       group,						/* For Group */
				       task_element );	
		if ( tokens.optCStr() != 0 ) {
		    input_error( "Unexpected attribute <%s> ", Xinitially );
		}
	    }else {
		
		task = new Task( &_document, taskName.asCStr(), sched_type, entries, processor,
				 _document.db_build_parameter_variable(StrX(task_element->getAttribute(Xqueue_length)).asCStr(),NULL),
				 StrX(task_element->getAttribute(Xpriority)).asLong(),
				 _document.db_build_parameter_variable(StrX(task_element->getAttribute(Xmultiplicity)).asCStr(),NULL),
				 StrX(task_element->getAttribute(Xreplication)).asLong(),
				 group,								/* For Group */
				 task_element );

		if ( tokens.optCStr() != 0 ) {
		    input_error( "Unexpected attribute <%s> ", Xinitially );
		}
	    }


	    const StrX think_time(task_element->getAttribute(Xthink_time));
	    const char * think_time_str = think_time.optCStr();
	    char * end_ptr = 0;
	    if ( think_time.optDouble() != 0.0 ) {	/* Ignore schema default value */
		if ( sched_type == SCHEDULE_CUSTOMER ) {
		    task->setThinkTime( _document.db_build_parameter_variable(think_time_str,NULL) );
		} else {
		    LQIO::input_error2( LQIO::ERR_NON_REF_THINK_TIME, taskName.asCStr() );
		}
	    }

	    /* Link in the entity information */
	    _document.addTaskEntity(task);
	    processor->addTask(task);
	    if ( group ) group->addTask(task);

	    Document::io_vars->n_tasks += 1;

	    const DOMNodeList *nodeList	    	= task_element->getChildNodes();
	    const unsigned int nodeListLength	= nodeList->getLength();

	    for ( unsigned x = 0; x < nodeListLength; x++ ) {
		const DOMElement * child_element = dynamic_cast<DOMElement *>(nodeList->item(x));
		if ( !child_element ) continue;

		if ( XMLString::compareIStringASCII(child_element->getTagName(), Xentry ) == 0 ) {
		    handleEntry( child_element );

		} else if ( XMLString::compareIStringASCII(child_element->getTagName(), Xfanin ) == 0 ) {
		    handleFanIn( task, child_element );

		} else if ( XMLString::compareIStringASCII(child_element->getTagName(), Xfanout ) == 0 ) {
		    handleFanOut( task, child_element);

		} else if ( XMLString::compareIStringASCII(child_element->getTagName(), Xtask_activities ) == 0 ) {
		    handleTaskActivities( task, child_element );

		} else if ( XMLString::compareIStringASCII(child_element->getTagName(), Xservice_time_distribution ) == 0 ) {
		    handleHistogram( task, child_element );

		} else if ( XMLString::compareIStringASCII(child_element->getTagName(), Xresult_task ) == 0 ) {
		    if ( Document::__debugXML ) cerr << simple_element( child_element ) << endl;

		    task->setResultThroughput( StrX(child_element->getAttribute(Xthroughput)).optDouble() )
			.setResultUtilization( StrX(child_element->getAttribute(Xutilization)).optDouble() )
			.setResultProcessorUtilization( StrX(child_element->getAttribute(Xproc_utilization)).optDouble() );

		    task->setResultPhase1Utilization( StrX(child_element->getAttribute(Xphase_utilization[0])).optDouble() )
			.setResultPhase2Utilization( StrX(child_element->getAttribute(Xphase_utilization[1])).optDouble() )
			.setResultPhase3Utilization( StrX(child_element->getAttribute(Xphase_utilization[2])).optDouble() );

		    if ( sched_type == SCHEDULE_SEMAPHORE ) {
			SemaphoreTask * semaphore = dynamic_cast<SemaphoreTask *>(task);
			semaphore->setResultHoldingTime( StrX(child_element->getAttribute(Xsemaphore_waiting)).optDouble() )
			    .setResultVarianceHoldingTime( StrX(child_element->getAttribute(Xsemaphore_waiting_variance)).optDouble() )
			    .setResultHoldingUtilization( StrX(child_element->getAttribute(Xsemaphore_utilization)).optDouble() );
		    }

		    if ( sched_type == SCHEDULE_RWLOCK ) {
			RWLockTask * rwlock = dynamic_cast<RWLockTask *>(task);
			rwlock->setResultReaderBlockedTime ( StrX(child_element->getAttribute(Xrwlock_reader_waiting)).optDouble() )
			    .setResultVarianceReaderBlockedTime ( StrX(child_element->getAttribute(Xrwlock_reader_waiting_variance)).optDouble() )
			    .setResultReaderHoldingTime ( StrX(child_element->getAttribute(Xrwlock_reader_holding)).optDouble() )
			    .setResultVarianceReaderHoldingTime ( StrX(child_element->getAttribute(Xrwlock_reader_holding_variance)).optDouble() )
			    .setResultReaderHoldingUtilization ( StrX(child_element->getAttribute(Xrwlock_reader_utilization)).optDouble() )
			    .setResultWriterBlockedTime ( StrX(child_element->getAttribute(Xrwlock_writer_waiting)).optDouble() )
			    .setResultVarianceWriterBlockedTime ( StrX(child_element->getAttribute(Xrwlock_writer_waiting_variance)).optDouble() )
			    .setResultWriterHoldingTime(StrX ( child_element->getAttribute(Xrwlock_writer_holding)).optDouble() )
			    .setResultVarianceWriterHoldingTime(  StrX(child_element->getAttribute(Xrwlock_writer_holding_variance)).optDouble() )
			    .setResultWriterHoldingUtilization ( StrX(child_element->getAttribute(Xrwlock_writer_utilization)).optDouble() );
		    }

		    DOMNodeList * result_list = child_element->getElementsByTagName(Xresult_conf_95);
		    const DOMElement * result95_element = dynamic_cast<DOMElement *>(result_list->item(0));
		    if ( result95_element ) {
			task->setResultThroughputVariance( invert(StrX(result95_element->getAttribute(Xthroughput)).optDouble()) )
			    .setResultUtilizationVariance( invert(StrX(result95_element->getAttribute(Xutilization)).optDouble()) )
			    .setResultProcessorUtilizationVariance( invert(StrX(result95_element->getAttribute(Xproc_utilization)).optDouble()) );

			task->setResultPhase1Utilization( invert(StrX(child_element->getAttribute(Xphase_utilization[0])).optDouble() ))
			    .setResultPhase2Utilization( invert(StrX(child_element->getAttribute(Xphase_utilization[1])).optDouble() ))
			    .setResultPhase3Utilization( invert(StrX(child_element->getAttribute(Xphase_utilization[2])).optDouble() ));

			if ( sched_type == SCHEDULE_SEMAPHORE ) {
			    SemaphoreTask * semaphore = dynamic_cast<SemaphoreTask *>(task);
			    semaphore->setResultHoldingTimeVariance( invert(StrX(child_element->getAttribute(Xsemaphore_waiting)).optDouble() ) )
				.setResultVarianceHoldingTimeVariance( invert(StrX(child_element->getAttribute(Xsemaphore_waiting_variance)).optDouble() ) )
				.setResultHoldingUtilizationVariance( invert(StrX(child_element->getAttribute(Xsemaphore_utilization)).optDouble() ) );
			}

			if ( sched_type == SCHEDULE_RWLOCK ) {
			    RWLockTask * rwlock = dynamic_cast<RWLockTask *>(task);
			    rwlock->setResultReaderBlockedTimeVariance( invert ( StrX(child_element->getAttribute(Xrwlock_reader_waiting)).optDouble() ) )
				.setResultVarianceReaderBlockedTimeVariance( invert ( StrX(child_element->getAttribute(Xrwlock_reader_waiting_variance)).optDouble() ) )
				.setResultReaderHoldingTimeVariance( invert ( StrX(child_element->getAttribute(Xrwlock_reader_holding)).optDouble() ) )
				.setResultVarianceReaderHoldingTimeVariance( invert ( StrX(child_element->getAttribute(Xrwlock_reader_holding_variance)).optDouble() ) )
				.setResultReaderHoldingUtilizationVariance( invert ( StrX(child_element->getAttribute(Xrwlock_reader_utilization)).optDouble() ) )
				.setResultWriterBlockedTimeVariance( invert ( StrX(child_element->getAttribute(Xrwlock_writer_waiting)).optDouble() ) )
				.setResultVarianceWriterBlockedTimeVariance( invert ( StrX(child_element->getAttribute(Xrwlock_writer_waiting_variance)).optDouble() ) )
				.setResultWriterHoldingTimeVariance( invert(StrX ( child_element->getAttribute(Xrwlock_writer_holding)).optDouble() ) )
				.setResultVarianceWriterHoldingTimeVariance( invert(  StrX(child_element->getAttribute(Xrwlock_writer_holding_variance)).optDouble() ) )
				.setResultWriterHoldingUtilizationVariance( invert ( StrX(child_element->getAttribute(Xrwlock_writer_utilization)).optDouble() ) );
			}

		    }
		} else if ( XMLString::compareIStringASCII(child_element->getTagName(), Xservice ) == 0 ) {
		    /* nop */

		} else {
		    throw element_error( StrX(child_element->getTagName()).asCStr() );
		}
	    }

	    if ( Document::__debugXML ) cerr << end_element( task_element ) << endl;

	    return 1;
	}

	void
	Xerces_Document::handleFanIn( Task * curTask, const DOMElement *fanin_element )
	{
	    curTask->setFanIn( StrX(fanin_element->getAttribute(Xsource)).asCStr(),
			       StrX(fanin_element->getAttribute(Xvalue)).asLong() );
	}

	void
	Xerces_Document::handleFanOut( Task * curTask, const DOMElement *fanout_element )
	{
	    curTask->setFanOut( StrX(fanout_element->getAttribute(Xdest)).asCStr(),
				StrX(fanout_element->getAttribute(Xvalue)).asLong() );
	}

	/* extract entrys from the task Node */

	void
	Xerces_Document::getEntryList(const DOMElement *taskNode, std::vector<Entry*>& entries )
	{
	    DOMNodeList * entry_list = taskNode->getElementsByTagName(Xentry);
	    const unsigned int entryListLength = entry_list->getLength();

	    for ( unsigned x = 0; x < entryListLength; x++ ) {
		const DOMElement * curEntry = dynamic_cast<DOMElement *>(entry_list->item(x));
		StrX entryName(curEntry->getAttribute(Xname));
		if ( !entryName ) throw missing_attribute( StrX(Xentry).asCStr() );
		Entry * entry = new Entry(&_document, entryName.asCStr(), curEntry);
		_document.addEntry(entry);
		entries.push_back( entry );
	    }
	}



	/*
	 * <entry name="s0" type="PH1PH2">
	 *   <entry-phase-activities>
	 */

	void
	Xerces_Document::handleEntry( const DOMElement * entry_element )
	{
	    if ( Document::__debugXML ) cerr << start_element( entry_element ) << endl;

	    StrX entryName(entry_element->getAttribute(Xname));
	    if ( !entryName ) throw  missing_attribute( StrX(Xentry).asCStr() );

	    Entry * entry = _document.getEntryByName(entryName.asCStr());

	    /* Get the remaining entry parameters */

	    const XMLCh * entryType = entry_element->getAttribute(Xtype);
	    if ( *entryType != 0 ) {
		if ( XMLString::compareIStringASCII(entryType,Xnone) == 0 ) {
		    entry->setEntryType( Entry::ENTRY_ACTIVITY_NOT_DEFINED );
		} else if ( XMLString::compareIStringASCII(entryType,XPH1PH2) == 0 ) {
		    entry->setEntryType( Entry::ENTRY_STANDARD_NOT_DEFINED );
		}
	    }

	    StrX priority(entry_element->getAttribute(Xpriority));
	    if ( priority.optCStr() ) {
		bool isSymbol = false;
		entry->setEntryPriority(_document.db_build_parameter_variable(priority.optCStr(), &isSymbol));
	    }
	    StrX openArrivals(entry_element->getAttribute(Xopen_arrival_rate));
	    if ( openArrivals.optCStr() ) {
		bool isSymbol = false;
		entry->setOpenArrivalRate(_document.db_build_parameter_variable(openArrivals.optCStr(), &isSymbol));
	    }
	    const XMLCh * semaphore = entry_element->getAttribute(Xsemaphore);
	    if ( *semaphore != 0 ) {
		if (XMLString::compareIStringASCII(semaphore,Xsignal) == 0 ) {
		    entry->setSemaphoreFlag(SEMAPHORE_SIGNAL);
		} else if (XMLString::compareIStringASCII(semaphore,Xwait) == 0 )  {
		    entry->setSemaphoreFlag(SEMAPHORE_WAIT);
		} else {
		    StrX attr( semaphore );
		    internal_error( __FILE__, __LINE__, "handleEntries: <entry name=\"%s\" sempahore=\"%s\">", entryName.asCStr(), attr.asCStr() );
		}
	    }


	    const XMLCh * rwlock = entry_element->getAttribute(Xrwlock);
	    if ( *rwlock != 0 ) {
		if (XMLString::compareIStringASCII(rwlock,Xr_lock) == 0 ) {
		    entry->setRWLockFlag(RWLOCK_R_LOCK);

		} else if (XMLString::compareIStringASCII(rwlock,Xr_unlock) == 0 )  {
		    entry->setRWLockFlag(RWLOCK_R_UNLOCK);

		} else if (XMLString::compareIStringASCII(rwlock,Xw_unlock) == 0 )  {
		    entry->setRWLockFlag(RWLOCK_W_LOCK);

		} else if (XMLString::compareIStringASCII(rwlock,Xw_unlock) == 0 )  {
		    entry->setRWLockFlag(RWLOCK_W_UNLOCK);

		} else {
		    StrX attr( rwlock );
		    internal_error( __FILE__, __LINE__, "handleEntries: <entry name=\"%s\" rwlock=\"%s\">", entryName.asCStr(), attr.asCStr() );
		}
	    }


	    Document::io_vars->n_entries += 1;

	    /* Now do its children. */

	    const DOMNodeList *nodeList	    	= entry_element->getChildNodes();
	    const unsigned int nodeListLength	= nodeList->getLength();

	    for ( unsigned y = 0; y < nodeListLength; y++ ) {
		DOMElement * child_element = dynamic_cast<DOMElement *>(nodeList->item(y));
		if ( !child_element ) continue;

		if ( XMLString::compareIStringASCII(child_element->getTagName(), Xentry_phase_activities ) == 0 ) {
		    _document.db_check_set_entry(entry, entryName.asCStr(), Entry::ENTRY_STANDARD);
		    handleEntryActivities( entry, child_element );

		} else if ( XMLString::compareIStringASCII(child_element->getTagName(), Xservice_time_distribution ) == 0 ) {
		    handleHistogram( entry, child_element );

		} else if ( XMLString::compareIStringASCII(child_element->getTagName(), Xresult_entry ) == 0 ) {
		    const unsigned max_p = entry->getEntryType() == Entry::ENTRY_STANDARD ? 0 : entry->getMaximumPhase();
		    
		    entry->setResultProcessorUtilization( StrX(child_element->getAttribute(Xproc_utilization)).optDouble() )
			.setResultSquaredCoeffVariation( StrX(child_element->getAttribute(Xsquared_coeff_variation)).optDouble() )
			.setResultThroughput( StrX(child_element->getAttribute(Xthroughput)).optDouble() )
			.setResultThroughputBound( StrX(child_element->getAttribute(Xthroughput_bound)).optDouble() )
			.setResultUtilization( StrX(child_element->getAttribute(Xutilization)).optDouble() )
			.setResultOpenWaitTime(  StrX(child_element->getAttribute(Xopen_wait_time)).optDouble() );

		    for ( unsigned p = 0; p < max_p; ++p ) {
			entry->setResultPhasePProcessorWaiting( p+1, StrX(child_element->getAttribute(Xphase_proc_waiting[p])).optDouble() )
			    .setResultPhasePServiceTime( p+1, StrX(child_element->getAttribute(Xphase_service_time[p])).optDouble() )
			    .setResultPhasePVarianceServiceTime( p+1, StrX(child_element->getAttribute(Xphase_service_time_variance[p])).optDouble() )
			    .setResultPhasePUtilization( p+1, StrX(child_element->getAttribute(Xphase_utilization[p])).optDouble() );
		    }

		    DOMNodeList * result_list = child_element->getElementsByTagName(Xresult_conf_95);
		    const DOMElement * result95_element = dynamic_cast<DOMElement *>(result_list->item(0));
		    if ( result95_element ) {
			entry->setResultProcessorUtilizationVariance( invert( StrX(result95_element->getAttribute(Xproc_utilization)).optDouble() ))
			    .setResultSquaredCoeffVariationVariance( invert( StrX(result95_element->getAttribute(Xsquared_coeff_variation)).optDouble() ))
			    .setResultThroughputVariance( invert( StrX(result95_element->getAttribute(Xthroughput)).optDouble() ))
			    .setResultUtilizationVariance( invert( StrX(result95_element->getAttribute(Xutilization)).optDouble() ))
			    .setResultOpenWaitTimeVariance(  invert( StrX(result95_element->getAttribute(Xopen_wait_time)).optDouble() ));
		    }

		    for ( unsigned p = 0; p < max_p; ++p ) {
			entry->setResultPhasePProcessorWaitingVariance( p+1, invert( StrX(result95_element->getAttribute(Xphase_proc_waiting[p])).optDouble() ))
			    .setResultPhasePServiceTimeVariance( p+1, invert( StrX(result95_element->getAttribute(Xphase_service_time[p])).optDouble() ))
			    .setResultPhasePVarianceServiceTimeVariance( p+1, invert( StrX(result95_element->getAttribute(Xphase_service_time_variance[p])).optDouble() ))
			    .setResultPhasePUtilizationVariance( p+1, invert( StrX(result95_element->getAttribute(Xphase_utilization[p])).optDouble() ));
		    }

		} else if ( XMLString::compareIStringASCII(child_element->getTagName(), Xforwarding ) != 0 ) {
		    throw element_error( StrX(child_element->getTagName()).asCStr() );

		}

	    }

	    if ( Document::__debugXML ) cerr << end_element( entry_element ) << endl;
	}



	/*
	 * <activity name="c1_ph1" phase="1" host-demand-mean="0.1">
	 */

	void
	Xerces_Document::handleEntryActivities(Entry * entry, const DOMElement *activity_list_element )
	{
	    if ( Document::__debugXML ) cerr << start_element( activity_list_element ) << endl;

	    const DOMNodeList *activityList = activity_list_element->getChildNodes();
	    for ( unsigned x=0;x<activityList->getLength();x++) {
		const DOMElement * activity_element = dynamic_cast<DOMElement *>(activityList->item(x));
		if (!activity_element) continue;

		StrX p(activity_element->getAttribute(Xphase));
		if ( !p ) throw missing_attribute( StrX(Xactivity).asCStr() );
		StrX activityName( activity_element->getAttribute(Xname));
		Phase* phase = entry->getPhase(p.asLong());
		if (!phase) internal_error( __FILE__, __LINE__, "handleEntryActivities" );
		if ( activityName.optCStr() ) phase->setName( activityName.asCStr() );
		if ( Document::__debugXML ) cerr << start_element2( activity_element ) << " phase=\"" << p.asLong() << "\">" << endl;

		phase->setXMLDOMElement( activity_element );

		/* handle attributes */

		StrX demand = activity_element->getAttribute(Xhost_demand_mean);
		if ( demand.optCStr() ) {
		    bool isSymbol = false;
		    phase->setServiceTime(_document.db_build_parameter_variable(demand.asCStr(), &isSymbol));
		}
		StrX cvsq = activity_element->getAttribute(Xhost_demand_cvsq);
		if ( cvsq.optCStr() ) {
		    phase->setCoeffOfVariationSquared(_document.db_build_parameter_variable(cvsq.asCStr(), NULL));
		}
		StrX think = activity_element->getAttribute(Xthink_time);
		if ( think.optCStr() ) {
		    phase->setThinkTime(_document.db_build_parameter_variable(think.asCStr(),NULL));
		}
 		StrX max_time = activity_element->getAttribute(Xmax_service_time);
 		if ( max_time.optCStr() ) {
 		    bool isSymbol = false;
		    phase->setHistogram(new Histogram( &_document, Histogram::CONTINUOUS, 2, max_time.asLong(), max_time.asLong(), 0 ));
		}
		const XMLCh * call_order = activity_element->getAttribute(Xcall_order);
		if ( *call_order != 0 ) {
		    phase->setPhaseTypeFlag(XMLString::compareIStringASCII(Xdeterministic, call_order) == 0 ? PHASE_DETERMINISTIC : PHASE_STOCHASTIC);
		}

		/* Now do its children. */

		const DOMNodeList *nodeList	    	= activity_element->getChildNodes();
		const unsigned int nodeListLength	= nodeList->getLength();

		for ( unsigned y = 0; y < nodeListLength; y++ ) {
		    const DOMElement * child_element = dynamic_cast<DOMElement *>(nodeList->item(y));
		    if ( !child_element ) continue;
		    if ( Document::__debugXML ) cerr << simple_element( child_element ) << endl;

		    if ( XMLString::compareIStringASCII(child_element->getTagName(), Xservice_time_distribution ) == 0 ) {
			handleHistogram( phase, child_element );

		    } else if ( XMLString::compareIStringASCII(child_element->getTagName(), Xresult_activity ) == 0 ) {
			phase->setResultProcessorWaiting( StrX(child_element->getAttribute(Xproc_waiting)).optDouble() )
			    .setResultServiceTime( StrX(child_element->getAttribute(Xservice_time)).optDouble() )
			    .setResultVarianceServiceTime( StrX(child_element->getAttribute(Xservice_time_variance)).optDouble() )
			    .setResultUtilization( StrX(child_element->getAttribute(Xutilization)).optDouble() );

			DOMNodeList * result_list = child_element->getElementsByTagName(Xresult_conf_95);
			const DOMElement * result95_element = dynamic_cast<DOMElement *>(result_list->item(0));
			if ( result95_element ) {
			    phase->setResultProcessorWaitingVariance( invert(StrX(result95_element->getAttribute(Xproc_waiting)).optDouble() ))
				.setResultServiceTimeVariance( invert(StrX(result95_element->getAttribute(Xservice_time)).optDouble() ))
				.setResultVarianceServiceTimeVariance( invert(StrX(result95_element->getAttribute(Xservice_time_variance)).optDouble() ))
				.setResultUtilizationVariance( invert(StrX(result95_element->getAttribute(Xutilization)).optDouble() ));
			}
		    } else if ( XMLString::compareIStringASCII(child_element->getTagName(), Xsynch_call ) != 0 
				&& XMLString::compareIStringASCII(child_element->getTagName(), Xasynch_call ) != 0 ) {
			throw element_error( StrX(child_element->getTagName()).asCStr() );
		    }
		}
		if ( Document::__debugXML ) cerr << end_element( activity_element ) << endl;
	    }

	    if ( Document::__debugXML ) cerr << end_element( activity_list_element ) << endl;
	}


	/* This handles task activities */

	void
	Xerces_Document::handleTaskActivities(Task * curTask, const DOMElement *activity_list_element)
	{
	    if ( Document::__debugXML ) cerr << start_element( activity_list_element ) << endl;

	    DOMNodeList *activityList	 = activity_list_element->getChildNodes();

	    for ( unsigned x = 0; x < activityList->getLength(); x++ ) {
		const DOMElement *child_element = dynamic_cast<DOMElement *>(activityList->item(x));
		if ( !child_element ) continue;

		if ( XMLString::compareIStringASCII(child_element->getTagName(), Xactivity ) == 0 ) {
		    handleActivity( curTask, child_element );

		} else if ( XMLString::compareIStringASCII(child_element->getTagName(), Xprecedence ) == 0 ) {
		    handlePrecedence(curTask,child_element);

		} else if ( XMLString::compareIStringASCII(child_element->getTagName(), Xreply_entry ) == 0 ) {
		    /* Find all activities that generate a reply */
		    if ( Document::__debugXML ) cerr << start_element( child_element ) << endl;

		    DOMNodeList *repActList = child_element->getElementsByTagName(Xreply_activity);
		    StrX entryName(child_element->getAttribute(Xname));
		    for ( unsigned y = 0; y < repActList->getLength(); y++ ) {
			const DOMElement * reply_list_element = dynamic_cast<DOMElement *>(repActList->item(y));
			if ( Document::__debugXML ) cerr << simple_element( reply_list_element ) << endl;
			
			const StrX activity_name(reply_list_element->getAttribute(Xname));
			if ( !activity_name ) continue;

			Entry* entry = _document.getEntryByName(entryName.asCStr());

			/* Now we need to find an entry for the given name */
			if ( entry->getTask() != curTask ) {
			    LQIO::input_error2( LQIO::ERR_WRONG_TASK_FOR_ENTRY, entryName.asCStr(), curTask->getName().c_str() );
			} else {
			    Activity * activity = curTask->getActivity(activity_name.optCStr());
			    activity->getReplyList().push_back(entry);
			}
		    }
		    if ( Document::__debugXML ) cerr << end_element( child_element ) << endl;
		} else {
		    throw element_error( StrX(child_element->getTagName()).asCStr() );
		}
	    }

	    if ( Document::__debugXML ) cerr << end_element( activity_list_element ) << endl;
	}



	void
	Xerces_Document::handleActivity(Task * curTask, const DOMElement *activity_element)
	{
	    if ( Document::__debugXML ) cerr << start_element( activity_element ) << endl;

	    StrX activityName( activity_element->getAttribute(Xname));
	    if ( !activityName ) throw missing_attribute( StrX(Xactivity).asCStr() );

	    Activity* activity = curTask->getActivity(activityName.asCStr(), true);
	    activity->setIsSpecified(true);

	    const StrX firstEntry(activity_element->getAttribute(Xbound_to_entry));
	    if ( firstEntry.optCStr() ) {
		Entry* entry = _document.getEntryByName(firstEntry.asCStr());
		_document.db_check_set_entry(entry, firstEntry.asCStr(), Entry::ENTRY_ACTIVITY);
		entry->setStartActivity(activity);
	    }

	    StrX demand = activity_element->getAttribute(Xhost_demand_mean);
	    if ( demand.optCStr() ) {
		activity->setServiceTime(_document.db_build_parameter_variable(demand.asCStr(), NULL));
	    }
	    StrX cvsq = activity_element->getAttribute(Xhost_demand_cvsq);
	    if ( cvsq.optCStr() ) {
		activity->setCoeffOfVariationSquared(_document.db_build_parameter_variable(cvsq.asCStr(), NULL));
	    }
	    StrX think = activity_element->getAttribute(Xthink_time);
	    if ( think.optCStr() ) {
		activity->setThinkTime(_document.db_build_parameter_variable(think.asCStr(), NULL));
	    }

//		    handleActivity(curTask, activityName.asCStr(), activity_element, "max-service-time", &set_max_activity_service_time);
	    activity->setXMLDOMElement( activity_element );

	    const XMLCh * call_order = activity_element->getAttribute(Xcall_order);
	    if ( *call_order != 0 ) {
		activity->setPhaseTypeFlag(XMLString::compareIStringASCII(Xdeterministic, call_order) == 0 ? PHASE_DETERMINISTIC : PHASE_STOCHASTIC);
	    }

	    /* Now do its children. */

	    const DOMNodeList *nodeList	    	= activity_element->getChildNodes();
	    const unsigned int nodeListLength	= nodeList->getLength();

	    for ( unsigned x = 0; x < nodeListLength; x++ ) {
		const DOMElement * child_element = dynamic_cast<DOMElement *>(nodeList->item(x));
		if ( !child_element ) continue;
		if ( Document::__debugXML ) cerr << simple_element( child_element ) << endl;

		if ( XMLString::compareIStringASCII(child_element->getTagName(), Xservice_time_distribution ) == 0 ) {
		    handleHistogram( activity, child_element );

		} else if ( XMLString::compareIStringASCII(child_element->getTagName(), Xresult_activity ) == 0 ) {
		    activity->setResultProcessorWaiting( StrX(child_element->getAttribute(Xproc_waiting)).optDouble() );
		    activity->setResultProcessorUtilization( StrX(child_element->getAttribute(Xproc_utilization)).optDouble() )
			.setResultThroughput( StrX(child_element->getAttribute(Xthroughput)).optDouble() )
			.setResultSquaredCoeffVariation( StrX(child_element->getAttribute(Xsquared_coeff_variation)).optDouble() )
			.setResultServiceTime( StrX(child_element->getAttribute(Xservice_time)).optDouble() )
			.setResultVarianceServiceTime( StrX(child_element->getAttribute(Xservice_time_variance)).optDouble() )
			.setResultUtilization( StrX(child_element->getAttribute(Xutilization)).optDouble() );

		    DOMNodeList * result_list = child_element->getElementsByTagName(Xresult_conf_95);
		    const DOMElement * result95_element = dynamic_cast<DOMElement *>(result_list->item(0));
		    if ( result95_element ) {
			activity->setResultProcessorWaitingVariance( invert(StrX(result95_element->getAttribute(Xproc_waiting)).optDouble() ));
			activity->setResultProcessorUtilizationVariance( invert(StrX(result95_element->getAttribute(Xproc_utilization)).optDouble() ))
			    .setResultThroughputVariance( invert(StrX(result95_element->getAttribute(Xthroughput)).optDouble() ))
//			.setResultCVSquaredVariance( invert(StrX(result95_element->getAttribute(Xsquared_coeff_variation)).optDouble() ))
			    .setResultServiceTimeVariance( invert(StrX(result95_element->getAttribute(Xservice_time)).optDouble() ))
			    .setResultVarianceServiceTimeVariance( invert(StrX(result95_element->getAttribute(Xservice_time_variance)).optDouble() ))
			    .setResultUtilizationVariance( invert(StrX(result95_element->getAttribute(Xutilization)).optDouble() ));
		    }
		    result_list;
		} else if ( XMLString::compareIStringASCII(child_element->getTagName(), Xsynch_call ) != 0 
			    && XMLString::compareIStringASCII(child_element->getTagName(), Xasynch_call ) != 0 ) {
		    throw element_error( StrX(child_element->getTagName()).asCStr() );
		}
	    }
	    if ( Document::__debugXML ) cerr << end_element( activity_element ) << endl;
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
	Xerces_Document::handlePrecedence(Task * curTask, const DOMElement *precedence_element )
	{
	    if ( Document::__debugXML ) cerr << start_element( precedence_element ) << endl;

	    /* Go do all the precedence elements which wire up the various activities */

	    DOMNodeList * precedence_list = precedence_element->getChildNodes();	/* <pre></pre><post></post> */
	    const unsigned int length = precedence_list->getLength();

	    ActivityList * post_list = NULL;
	    ActivityList * pre_list = NULL;
	    for ( unsigned x = 0; x < length; x++ ) {
		const DOMElement * pre_post_element = dynamic_cast<DOMElement *>(precedence_list->item(x));
		if ( !pre_post_element ) continue;
		const XMLCh * tag_name = pre_post_element->getTagName();
		std::map<XMLCh *,ActivityList::ActivityListType>::const_iterator item = Xpre_post.find(const_cast<XMLCh *>(tag_name));
		if ( item == Xpre_post.end() ) throw element_error( StrX(tag_name).asCStr() );

		const DOMElement * result_element = 0;

		switch ( item->second ) {
		case ActivityList::AND_JOIN_ACTIVITY_LIST:
		    if ( pre_list ) internal_error( __FILE__, __LINE__, "Duplicate pre." );
		    pre_list = new AndJoinActivityList( &_document, curTask, 
							_document.db_build_parameter_variable(StrX(precedence_element->getAttribute(Xquorum)).optCStr(),NULL ),
							pre_post_element );
		    handleActivityList( pre_list, curTask, pre_post_element, item->second );
		    break;

		case ActivityList::OR_JOIN_ACTIVITY_LIST:
		case ActivityList::JOIN_ACTIVITY_LIST:
		    if ( pre_list ) internal_error( __FILE__, __LINE__, "Duplicate pre." );
		    pre_list = new ActivityList( &_document, curTask, item->second, pre_post_element );
		    handleActivityList( pre_list, curTask, pre_post_element, item->second );
		    break;

		case ActivityList::OR_FORK_ACTIVITY_LIST:
		case ActivityList::FORK_ACTIVITY_LIST:
		case ActivityList::AND_FORK_ACTIVITY_LIST:
		    if ( post_list ) internal_error( __FILE__, __LINE__, "Duplicate post." );
		    post_list = new ActivityList( &_document, curTask, item->second, pre_post_element );
		    handleActivityList( post_list, curTask, pre_post_element, item->second );
		    break;

		case ActivityList::REPEAT_ACTIVITY_LIST:
		    if ( post_list ) internal_error( __FILE__, __LINE__, "Duplicate post." );
		    post_list = new ActivityList( &_document, curTask, item->second, pre_post_element );
		    handleActivityList( post_list, curTask, pre_post_element, item->second );
		    handleListEnd( post_list, curTask, pre_post_element ); 
		    break;
		}
	    }

	    /* Connect the activities */

	    if (pre_list != NULL) {
		if (pre_list != NULL) {
		    pre_list->setNext(post_list);
		}
		if (post_list != NULL) {
		    post_list->setPrevious(pre_list);
		}
	    } else {
		internal_error( __FILE__, __LINE__, "handleActivitySequence" );
	    }

	    if ( Document::__debugXML ) cerr << end_element( precedence_element ) << endl;
	}


	void
	Xerces_Document::handleActivityList( ActivityList * activity_list, Task * domTask, const DOMElement *precedence_element, const ActivityList::ActivityListType type )
	{
	    if ( Document::__debugXML ) cerr << start_element( precedence_element ) << endl;

	    const DOMNodeList * child_list = precedence_element->getChildNodes();
	    const unsigned int len = child_list->getLength();

	    for ( unsigned x = 0; x < len; ++x ) {
		const DOMElement * child_element = dynamic_cast<DOMElement *>(child_list->item(x));
		if ( !child_element ) continue;
		if ( Document::__debugXML ) cerr << simple_element( child_element ) << endl;

		const XMLCh * tag_name = child_element->getTagName();
		if ( XMLString::compareIStringASCII(tag_name, Xservice_time_distribution ) == 0 ) {
		    handleHistogram( activity_list, child_element );

		} else if ( XMLString::compareIStringASCII(tag_name, Xresult_join_delay ) == 0 ) {
		    AndJoinActivityList * andjoin_list = dynamic_cast<AndJoinActivityList *>(activity_list);
		    andjoin_list->setResultJoinDelay( StrX(child_element->getAttribute(Xjoin_waiting)).optDouble() );
		    andjoin_list->setResultVarianceJoinDelay( StrX(child_element->getAttribute(Xjoin_variance)).optDouble() );

		    const DOMElement * result95_element = dynamic_cast<DOMElement *>(child_element->getElementsByTagName(Xresult_conf_95)->item(0));
		    if ( result95_element ) {
			andjoin_list->setResultJoinDelayVariance( invert(StrX(result95_element->getAttribute(Xjoin_waiting)).optDouble() ));
			andjoin_list->setResultVarianceJoinDelayVariance( invert(StrX(result95_element->getAttribute(Xjoin_variance)).optDouble() ));
		    }
		} else if ( XMLString::compareIStringASCII(tag_name, Xactivity ) == 0 ) {
		    StrX activity_name(child_element->getAttribute(Xname));
		    if ( !activity_name ) throw missing_attribute( StrX(Xactivity).asCStr() );

		    Activity* activity = domTask->getActivity(activity_name.asCStr());
		    if ( !activity ) {
			input_error2( ERR_NOT_DEFINED, activity_name.asCStr() );
		    } else {
			switch ( type ) {
			case ActivityList::AND_JOIN_ACTIVITY_LIST:
			case ActivityList::OR_JOIN_ACTIVITY_LIST:
			case ActivityList::JOIN_ACTIVITY_LIST:
			    activity_list->add(activity);
			    activity->outputTo(activity_list);
			    break;

			case ActivityList::FORK_ACTIVITY_LIST:
			case ActivityList::AND_FORK_ACTIVITY_LIST:
			    activity_list->add(activity);
			    activity->inputFrom(activity_list);
			    break;

			case ActivityList::OR_FORK_ACTIVITY_LIST: 
			{
			    StrX prob(child_element->getAttribute(Xprob));
			    activity_list->add(activity,_document.db_build_parameter_variable(prob.asCStr(),NULL));
			    activity->inputFrom(activity_list);
			}
			break;
			case ActivityList::REPEAT_ACTIVITY_LIST:
			{
			    StrX count(child_element->getAttribute(Xcount));
			    activity_list->add(activity,_document.db_build_parameter_variable(count.asCStr(),NULL));
			    activity->inputFrom(activity_list);
			}
			break;
			}
		    }
		} else {
		    throw element_error( StrX(tag_name).asCStr() );
		}
	    }
	    if ( Document::__debugXML ) cerr << end_element( precedence_element ) << endl;
	}

	void
	Xerces_Document::handleListEnd( ActivityList * activity_list, Task * domTask, const DOMElement *precedence_element )
	{
	    StrX activity_name(precedence_element->getAttribute(Xend));
	    if ( activity_name.optCStr() ) {
		Activity* activity = domTask->getActivity(activity_name.asCStr());
		activity->inputFrom(activity_list);
		activity_list->add(activity,NULL);		/* Special count case */
	    }
	}

	/* ------------------------------------------------------------------------ */

//                <activity name="client_ph1" phase="1" host-demand-mean="1">
//                   <synch-call dest="server" calls-mean="1"/>
//                </activity>
	void
	Xerces_Document::handleCalls(const DOMElement *model)
	{

	    /* extract calls from all entry - entry-activities */

	    DOMNodeList * taskList = model->getElementsByTagName(Xtask);

	    for ( unsigned x = 0; x < taskList->getLength(); x++ ) {
		const DOMElement *task_element = dynamic_cast<DOMElement *>(taskList->item(x));
		if ( !task_element ) continue;

		StrX taskName( task_element->getAttribute(Xname) );
		Task * task= _document.getTaskByName(taskName.asCStr());

		DOMNodeList * entryList = task_element->getChildNodes();

		for ( unsigned y = 0; y < entryList->getLength(); ++y ) {
		    const DOMElement *child_element = dynamic_cast<DOMElement *>(entryList->item(y));
		    if ( !child_element ) continue;

		    if ( XMLString::compareIStringASCII( Xentry, child_element->getTagName()) == 0
			 && XMLString::compareIStringASCII( XPH1PH2, child_element->getAttribute(Xtype) ) == 0 ) {
			handleCallsForEntry( child_element );
			handleForwardingForEntry( child_element );
		    } else if ( XMLString::compareIStringASCII( Xtask_activities, child_element->getTagName()) == 0 ) {
			handleCallsForActivities( task, child_element );
		    }
		}
	    }
	}

	void
	Xerces_Document::handleCallsForEntry( const DOMElement * entryElement )
	{
	    DOMNodeList *activityList = entryElement->getElementsByTagName(Xactivity);
	    const StrX entry_name( entryElement->getAttribute(Xname));
	    /* extract calls from all task-activities */

	    Entry* entry = _document.getEntryByName(entry_name.asCStr());
	    if ( !entry ) internal_error( __FILE__, __LINE__, "handleCallsForEntry" );

	    for ( unsigned x = 0; x < activityList->getLength(); x++ ) {
		unsigned int phase = 0;
		const DOMElement * activityElement = dynamic_cast<DOMElement *>(activityList->item(x));
		if ( !activityElement ) continue;
		if ( !XMLString::textToBin( activityElement->getAttribute(Xphase), phase ) ) continue;
		
		DOMNodeList * callList = activityElement->getChildNodes();
		if ( Document::__debugXML ) cerr << "  <!-- Entry name=\"" << entry_name.asCStr() << "\" phase=\"" << phase << "\" -->" << endl;

		for ( unsigned y = 0; y < callList->getLength(); y++ ) {
		    const DOMElement * callElement = dynamic_cast<DOMElement *>(callList->item(y));
		    if ( !callElement ) continue;

		    const XMLCh * tag_name = callElement->getTagName();
	
		    if ( XMLString::compareIStringASCII(tag_name, Xsynch_call) == 0 ) {
			handleEntryCall( entry, phase, Call::RENDEZVOUS, callElement );
		    } else if ( XMLString::compareIStringASCII(tag_name, Xasynch_call) == 0 ) {
			handleEntryCall( entry, phase, Call::SEND_NO_REPLY, callElement );
		    } else {
		    }
		}
	    }
	}

	void
	Xerces_Document:: handleCallsForActivities( const Task * task, const DOMElement * taskActivitiesElement )
	{
	    DOMNodeList *activityList = taskActivitiesElement->getElementsByTagName(Xactivity);

	    /* extract calls from all task-activities */

	    for ( unsigned x = 0; x < activityList->getLength(); x++ ) {
		const DOMElement * activity_element = dynamic_cast<DOMElement *>(activityList->item(x));
		if ( !activity_element ) continue;
		StrX activityName( activity_element->getAttribute(Xname));
		Activity* activity = task->getActivity(activityName.asCStr());

		DOMNodeList * callList = activity_element->getChildNodes();
		for ( unsigned y = 0; y < callList->getLength(); y++ ) {
		    const DOMElement * callElement = dynamic_cast<DOMElement *>(callList->item(y));
		    if ( !callElement ) continue;

		    const XMLCh * tag_name = callElement->getTagName();

		    if ( XMLString::compareIStringASCII(tag_name, Xsynch_call) == 0 ) {
			handleActivityCall( activity, Call::RENDEZVOUS, callElement );
		    } else if ( XMLString::compareIStringASCII(tag_name, Xasynch_call) == 0 ) {
			handleActivityCall( activity, Call::SEND_NO_REPLY, callElement );
		    } else {
		    }
		}
	    }
	}


	void
	Xerces_Document::handleForwardingForEntry( const DOMElement * entry_element )
	{
	    /* extract all forwarding calls */

	    DOMNodeList * forwardList = entry_element->getElementsByTagName(Xforwarding);
	    StrX entryName( entry_element->getAttribute(Xname));
	    for ( unsigned y = 0 ; y < forwardList->getLength(); y++ ) {
		const DOMElement *forward_element = dynamic_cast<DOMElement *>(forwardList->item(y));
		StrX dstEntryName(forward_element->getAttribute(Xdest));
		if ( Document::__debugXML ) cerr << "          <" << StrX(forward_element->getTagName()).asCStr() << " dest=\"" << dstEntryName.asCStr() << "\">" << endl;
		if ( !dstEntryName ) throw missing_attribute( StrX(Xforwarding).asCStr() );

		StrX prob(forward_element->getAttribute(Xprob));
		if ( !prob ) throw missing_attribute( StrX(Xforwarding).asCStr() );
		Entry* to_entry = _document.getEntryByName(dstEntryName.asCStr());
		Entry* from_entry = _document.getEntryByName(entryName.asCStr());
		Call * call = from_entry->getForwardingToTarget(to_entry);

		if ( call == NULL ) {
		    bool isSymbol = false;
		    Call* call = new Call( &_document, from_entry, to_entry,
					   _document.db_build_parameter_variable(prob.asCStr(), &isSymbol),
					   forward_element );
		    from_entry->addForwardingCall(call);
		} else {
		    if (call->getCallType() != Call::NULL_CALL) {
			LQIO::input_error2( LQIO::WRN_MULTIPLE_SPECIFICATION );
		    }
		}

		if ( Document::__debugXML ) cerr << "          </" << StrX(forward_element->getTagName()).asCStr() << "\">" << endl;
	    }
	}


	/*
	 * <synch-call dest="s2" calls-mean="1"/>
	 */

	void

	Xerces_Document::handleEntryCall( Entry * from_entry, int from_phase, const Call::CallType call_type, const DOMElement * call_element )
	{
	    StrX dstEntryName(call_element->getAttribute(Xdest));
	    if ( Document::__debugXML ) cerr << "          <" << StrX(call_element->getTagName()).asCStr() << " dest=\"" << dstEntryName.asCStr() << "\">" << endl;
	    if ( !dstEntryName ) throw missing_attribute( StrX(call_element->getTagName()).asCStr() );

	    StrX calls(call_element->getAttribute(Xcalls_mean));
	    if ( !calls ) throw missing_attribute( StrX(call_element->getTagName()).asCStr() );

	    /* Obtain the entry that we will be adding the phase times to */
	    Entry* to_entry = _document.getEntryByName(dstEntryName.asCStr());

	    assert(to_entry != NULL);

	    /* Make sure that this is a standard entry */
	    _document.db_check_set_entry(from_entry, from_entry->getName().c_str(), Entry::ENTRY_STANDARD);
	    _document.db_check_set_entry(to_entry, dstEntryName.asCStr());

	    /* Push all the times */

	    Phase* phase = from_entry->getPhase(from_phase);
	    bool isSymbol = false;
	    ExternalVariable* ev_calls = _document.db_build_parameter_variable(calls.asCStr(), &isSymbol);

	    Call* call = from_entry->getCallToTarget(to_entry, from_phase);

	    /* Check the existence */
	    if (call == NULL) {
		call = new Call(&_document, call_type, phase, to_entry, ev_calls, call_element );
		phase->addCall( call );
	    } else {
		if (call->getCallType() != Call::NULL_CALL) {
		    LQIO::input_error2( LQIO::WRN_MULTIPLE_SPECIFICATION );
		}

		/* Set the new call type and the new mean */
		call->setCallType(call_type);
		call->setCallMean(ev_calls);
	    }

	    /* Get child nodes. */

	    const DOMNodeList * nodeList = call_element->getChildNodes();
	    const unsigned int nodeListLength = nodeList->getLength();
	    for ( unsigned x = 0; x < nodeListLength; x++ ) {
		const DOMElement * child_element = dynamic_cast<DOMElement *>(nodeList->item(x));
		if ( !child_element ) continue;

		if ( XMLString::compareIStringASCII(child_element->getTagName(), Xresult_call) == 0 ) {
		    call->setResultWaitingTime( StrX(child_element->getAttribute(Xwaiting)).optDouble() )
			.setResultVarianceWaitingTime( StrX(child_element->getAttribute(Xwaiting_variance)).optDouble() )
			.setResultDropProbability( StrX(child_element->getAttribute(Xdrop_probability)).optDouble() );

		    DOMNodeList * result_list = child_element->getElementsByTagName(Xresult_conf_95);
		    const DOMElement * result95_element = dynamic_cast<DOMElement *>(result_list->item(0));
		    if ( result95_element ) {
			call->setResultWaitingTimeVariance( invert(StrX(result95_element->getAttribute(Xwaiting)).optDouble() ))
			    .setResultVarianceWaitingTimeVariance( invert(StrX(result95_element->getAttribute(Xwaiting_variance)).optDouble() ))
			    .setResultDropProbabilityVariance( invert(StrX(result95_element->getAttribute(Xdrop_probability)).optDouble() ));
		    }
		}
	    }
	    if ( Document::__debugXML ) cerr << "          </" << StrX(call_element->getTagName()).asCStr() << "\">" << endl;
	}



	void
	Xerces_Document::handleActivityCall( Activity * from_activity, const Call::CallType call_type, const DOMElement * call_element )
	{
	    StrX dstEntryName(call_element->getAttribute(Xdest));
	    if ( Document::__debugXML ) cerr << "          <" << StrX(call_element->getTagName()).asCStr() << " dest=\"" << dstEntryName.asCStr() << "\">" << endl;
	    if ( !dstEntryName ) throw missing_attribute( StrX(call_element->getTagName()).asCStr() );

	    StrX calls(call_element->getAttribute(Xcalls_mean));
	    if ( !calls ) throw missing_attribute( StrX(call_element->getTagName()).asCStr() );

	    /* Obtain the entry that we will be adding the phase times to */
	    Entry* to_entry = _document.getEntryByName(dstEntryName.asCStr());
	    assert(to_entry != NULL);

	    /* Push all the times */

	    ExternalVariable* ev_calls = _document.db_build_parameter_variable(calls.asCStr(), NULL);

	    Call* call = new Call(&_document, call_type, from_activity, to_entry, ev_calls, call_element );
	    from_activity->addCall(call);

	    /* Get child nodes. */

	    const DOMNodeList * nodeList = call_element->getChildNodes();
	    const unsigned int nodeListLength = nodeList->getLength();
	    for ( unsigned x = 0; x < nodeListLength; x++ ) {
		const DOMElement * child_element = dynamic_cast<DOMElement *>(nodeList->item(x));
		if ( !child_element ) continue;
		if ( XMLString::compareIStringASCII(child_element->getTagName(), Xresult_call) == 0 ) {
		    call->setResultWaitingTime( StrX(child_element->getAttribute(Xwaiting)).optDouble() );
		    call->setResultVarianceWaitingTime( StrX(child_element->getAttribute(Xwaiting_variance)).optDouble() );
		    call->setResultDropProbability( StrX(child_element->getAttribute(Xdrop_probability)).optDouble() );

		    DOMNodeList * result_list = child_element->getElementsByTagName(Xresult_conf_95);
		    const DOMElement * result95_element = dynamic_cast<DOMElement *>(result_list->item(0));
		    if ( result95_element ) {
			call->setResultWaitingTimeVariance( invert(StrX(result95_element->getAttribute(Xwaiting)).optDouble() ));
			call->setResultVarianceWaitingTimeVariance( invert(StrX(result95_element->getAttribute(Xwaiting_variance)).optDouble() ));
			call->setResultDropProbabilityVariance( invert(StrX(result95_element->getAttribute(Xdrop_probability)).optDouble() ));
		    }
		}
	    }
	    if ( Document::__debugXML ) cerr << "          </" << StrX(call_element->getTagName()).asCStr() << "\">" << endl;
	}

	void
	Xerces_Document::handleLQX(const DOMElement *lqx_element)
	{
	    /* This is the element containing the LQX code */

	    if ( Document::__debugXML ) cerr << start_element( lqx_element ) << endl;
	
	    const XMLCh* body = dynamic_cast<DOMText*>(lqx_element->getFirstChild())->getData();

	    /* Now, invoke the callout */
	    _document.setLQXProgramText(Xt(body));

   	    if ( Document::__debugXML ) cerr << end_element( lqx_element ) << endl;
	}

	void
	Xerces_Document::handleHistogram( DocumentObject * object, const DOMElement * histogram_element )
	{
	    StrX min( histogram_element->getAttribute(Xmin) );
	    StrX max( histogram_element->getAttribute(Xmax) );
	    StrX n_bins( histogram_element->getAttribute(Xnumber_bins) );
	    StrX phase( histogram_element->getAttribute(Xphase) );
	    Histogram * histogram = 0;
	    if ( phase.optLong() ) {		/* Entry histogram by phase */
		if ( !dynamic_cast<Entry *>(object)) {
		    input_error( "Unexpected attribute <%s> ", Xphase );
		} else {
		    histogram = new Histogram(&_document, Histogram::CONTINUOUS, n_bins.asLong(), min.asDouble(), max.asDouble(), histogram_element );
		    object->setHistogramForPhase( phase.optLong(), histogram );
		}
	    } else {
		histogram = new Histogram(&_document, Histogram::CONTINUOUS, n_bins.asLong(), min.asDouble(), max.asDouble(), histogram_element );
		object->setHistogram( histogram );

	    }
	    if ( histogram ) {
		DOMNodeList * histogram_bins = histogram_element->getElementsByTagName(Xhistogram_bin);
		handleHistogramBins( histogram, histogram_bins );
		handleHistogramBins( histogram, histogram_element->getElementsByTagName(Xunderflow_bin) );
		handleHistogramBins( histogram, histogram_element->getElementsByTagName(Xoverflow_bin) );
	    }
	}


        void
	Xerces_Document::handleHistogramBins( Histogram * histogram, const DOMNodeList * histogram_bins )
	{
	    for ( unsigned x = 0; x < histogram_bins->getLength(); x++ ) {
		const DOMElement * histogram_bin_element = dynamic_cast<DOMElement *>(histogram_bins->item(x));
		if ( !histogram_bin_element ) continue;
		StrX index( histogram_bin_element->getAttribute(Xbegin) );
		StrX mean( histogram_bin_element->getAttribute(Xprob) );
		StrX conf_95( histogram_bin_element->getAttribute(Xconf_95) );
		
		histogram->setBinMeanVariance( histogram->getBinIndex( index.asDouble() ), mean.asDouble(), conf_95.optCStr() ? invert( conf_95.asDouble() ) : 0.0 );
	    }
	}

	/* ---------------------------------------------------------------- */
	/* DOM serialization - write results to XERCES then save.	    */
	/* ---------------------------------------------------------------- */

        void Xerces_Document::serializeDOM( std::ostream& output, bool instantiate ) 
	{
	    if ( __xercesDOM ) {
		__xercesDOM->serializeDOM2( output, instantiate );
	    } else {
		throw runtime_error( "XML output failed: no DOM." );
	    }
	}


	/* store the results in the XML DOM, then serialize that. */
	void Xerces_Document::serializeDOM2( ostream& output, bool instantiate ) const
	{
	    clearExistingDOMResults();

	    if ( _document.getResultNumberOfBlocks() > 1 ) {
		const_cast<ConfidenceIntervals *>(&_conf_95)->set_blocks( _document.getResultNumberOfBlocks() );
		const_cast<ConfidenceIntervals *>(&_conf_99)->set_blocks( _document.getResultNumberOfBlocks() );
	    }

	    insertDocumentResults();

	    const std::map<std::string,LQIO::DOM::Processor *>& processors = _document.getProcessors();
	    for ( std::map<std::string, Processor*>::const_iterator procIter = processors.begin(); procIter != processors.end(); ++procIter) {
		insertProcessorResults( procIter->second );
	    }

	    LQIO::serializeDOM(output, instantiate);
	}

	
	void
	Xerces_Document::insertDocumentResults() const
	{
	    DOMElement * document_element = (DOMElement *)(_document.getXMLDOMElement());
	    if ( _document.hasPragmas() ) {
		const std::map<std::string,std::string>& pragmas = _document.getPragmaList();
		for ( std::map<std::string,std::string>::const_iterator next_pragma = pragmas.begin(); next_pragma != pragmas.end(); ++next_pragma ) {
		    XercesWrite pragma_element( document_element, Xpragma );
		    pragma_element( Xparam, next_pragma->first );
		    pragma_element( Xvalue, next_pragma->second );
		}
	    }

	    XercesWrite result_element( document_element, Xresult_general );
	    result_element( Xconv_val_result, _document.getResultConvergenceValue() );
	    result_element( Xvalid,           _document.getResultValid() ? "YES" : "NO");
	    result_element( Xiterations,      _document.getResultIterations() );
	    result_element( Xplatform_info,   _document.getResultPlatformInformation() );
	    result_element( Xsolver_info,     _document.getResultSolverInformation() );
	    result_element.insert_time( Xuser_cpu_time,   _document.getResultUserTime() );
	    result_element.insert_time( Xsystem_cpu_time, _document.getResultSysTime() );
	    result_element.insert_time( Xelapsed_time,    _document.getResultElapsedTime() );
	    
	    if ( _document.getResultMVASubmodels() ) {
		XercesWrite mva_element( result_element.getElement(), Xmva_info );
		mva_element( Xsubmodels, _document.getResultMVASubmodels() );
		mva_element( Xcore, static_cast<double>(_document.getResultMVACore()) );
		mva_element( Xstep, _document.getResultMVAStep() );
		mva_element( Xstep_squared, _document.getResultMVAStepSquared() );
		mva_element( Xwait, _document.getResultMVAWait() );
		mva_element( Xwait_squared, _document.getResultMVAWaitSquared() );
		mva_element( Xfaults, _document.getResultMVAFaults() );
	    }
	}


	void
	Xerces_Document::insertProcessorResults( const Processor * processor ) const
	{
	    XercesWrite result_element( (DOMElement *)(processor->getXMLDOMElement()), Xresult_processor );
	    result_element( Xutilization, processor->getResultUtilization());
	    if ( _document.getResultNumberOfBlocks() > 1 ) {
		/* Order is important */
		XercesWriteConfidence conf_99_element(result_element.getElement(), Xresult_conf_99, _conf_95 );
		XercesWriteConfidence conf_95_element(result_element.getElement(), Xresult_conf_95, _conf_99 );
		conf_99_element( Xutilization, processor->getResultUtilizationVariance());
		conf_95_element( Xutilization, processor->getResultUtilizationVariance());
	    }

	    /* Now do my groups and tasks */

	    const std::set<Group*>& groupList = processor->getGroupList();
	    for (std::set<Group*>::const_iterator groupIter = groupList.begin(); groupIter != groupList.end(); ++groupIter) {
		insertGroupResults( *groupIter );
	    }

	    const std::set<Task*>& taskList = processor->getTaskList();
	    for ( std::set<Task*>::const_iterator  taskIter = taskList.begin(); taskIter != taskList.end(); ++taskIter) {
		insertTaskResults( *taskIter );
	    }
	}


	void
	Xerces_Document::insertGroupResults( const Group * group ) const
	{
	    XercesWrite result_element( (DOMElement *)(group->getXMLDOMElement()), Xresult_group );

	    result_element(Xutilization, group->getResultUtilization());

	    if ( _document.getResultNumberOfBlocks() > 1 ) {
		/* Order is important */
		XercesWriteConfidence conf_99_element(result_element.getElement(), Xresult_conf_99, _conf_99 );
		XercesWriteConfidence conf_95_element(result_element.getElement(), Xresult_conf_95, _conf_95 );
		conf_99_element(Xutilization, group->getResultUtilizationVariance());
		conf_95_element(Xutilization, group->getResultUtilizationVariance());
	    }
	}


	void
	Xerces_Document::insertTaskResults( const Task * task ) const
	{
	    if ( task->hasHistogram() ) {
		insertHistogramResults( task->getHistogram() );
	    }

	    XercesWrite result_element( (DOMElement *)(task->getXMLDOMElement()), Xresult_task );
	    /* Store the actual data into the DOM for phase utilizations */
	    result_element(Xthroughput, task->getResultThroughput());
	    for (unsigned p = 0; p < task->getResultPhaseCount(); ++p) {
		result_element(Xphase_utilization[p], task->getResultPhasePUtilization(p+1));
	    }
	    result_element(Xutilization, task->getResultUtilization());
	    result_element(Xproc_utilization, task->getResultProcessorUtilization());

	    if ( dynamic_cast<const SemaphoreTask *>(task) ) {
		const SemaphoreTask * semaphore = dynamic_cast<const SemaphoreTask *>(task);
		result_element(Xsemaphore_waiting, semaphore->getResultHoldingTime());
		result_element(Xsemaphore_waiting_variance, semaphore->getResultVarianceHoldingTime());
		result_element(Xsemaphore_utilization, semaphore->getResultHoldingUtilization());
	    }

	    if ( dynamic_cast<const RWLockTask *>(task) ) {

		const RWLockTask * rwlock = dynamic_cast<const RWLockTask *>(task);
		result_element( Xrwlock_reader_waiting, rwlock->getResultReaderBlockedTime() );
		result_element( Xrwlock_reader_waiting_variance, rwlock->getResultVarianceReaderBlockedTime() );
		result_element( Xrwlock_reader_holding, rwlock->getResultReaderHoldingTime() );
		result_element( Xrwlock_reader_holding_variance, rwlock->getResultVarianceReaderHoldingTime() );
		result_element( Xrwlock_reader_utilization, rwlock->getResultReaderHoldingUtilization() );
		result_element( Xrwlock_writer_waiting, rwlock->getResultWriterBlockedTime() );
		result_element( Xrwlock_writer_waiting_variance, rwlock->getResultVarianceWriterBlockedTime() );
		result_element( Xrwlock_writer_holding, rwlock->getResultWriterHoldingTime() );
		result_element( Xrwlock_writer_holding_variance, rwlock->getResultVarianceWriterHoldingTime() );
		result_element( Xrwlock_writer_utilization, rwlock->getResultWriterHoldingUtilization() );
	    }

	    if ( _document.getResultNumberOfBlocks() > 1 ) {
		/* Order is important */
		XercesWriteConfidence conf_99_element(result_element.getElement(), Xresult_conf_99, _conf_99 );
		XercesWriteConfidence conf_95_element(result_element.getElement(), Xresult_conf_95, _conf_95 );
		conf_99_element(Xthroughput, task->getResultThroughputVariance());
		conf_95_element(Xthroughput, task->getResultThroughputVariance());
		conf_99_element(Xutilization, task->getResultUtilizationVariance());
		conf_95_element(Xutilization, task->getResultUtilizationVariance());
		conf_99_element(Xproc_utilization, task->getResultProcessorUtilizationVariance());
		conf_95_element(Xproc_utilization, task->getResultProcessorUtilizationVariance());
		for (unsigned p = 0; p < task->getResultPhaseCount(); ++p) {
		    conf_99_element(Xphase_utilization[p], task->getResultPhasePUtilizationVariance(p+1));
		    conf_95_element(Xphase_utilization[p], task->getResultPhasePUtilizationVariance(p+1));
		}

		if ( dynamic_cast<const SemaphoreTask *>(task) ) {
		    const SemaphoreTask * semaphore = dynamic_cast<const SemaphoreTask *>(task);
		    conf_95_element(Xsemaphore_waiting, semaphore->getResultHoldingTimeVariance());
		    conf_95_element(Xsemaphore_waiting_variance, semaphore->getResultVarianceHoldingTimeVariance());
		    conf_95_element(Xsemaphore_utilization, semaphore->getResultHoldingUtilizationVariance());
		    conf_99_element(Xsemaphore_waiting, semaphore->getResultHoldingTimeVariance());
		    conf_99_element(Xsemaphore_waiting_variance, semaphore->getResultVarianceHoldingTimeVariance());
		    conf_99_element(Xsemaphore_utilization, semaphore->getResultHoldingUtilizationVariance());
		}

		if ( dynamic_cast<const RWLockTask *>(task) ) {
		    const RWLockTask * rwlock = dynamic_cast<const RWLockTask *>(task);

		    conf_95_element( Xrwlock_reader_waiting, rwlock->getResultReaderBlockedTime() );
		    conf_95_element( Xrwlock_reader_waiting_variance, rwlock->getResultVarianceReaderBlockedTime() );
		    conf_95_element( Xrwlock_reader_holding, rwlock->getResultReaderHoldingTime() );
		    conf_95_element( Xrwlock_reader_holding_variance, rwlock->getResultVarianceReaderHoldingTime() );
		    conf_95_element( Xrwlock_reader_utilization, rwlock->getResultReaderHoldingUtilization() );
		    conf_95_element( Xrwlock_writer_waiting, rwlock->getResultWriterBlockedTime() );
		    conf_95_element( Xrwlock_writer_waiting_variance, rwlock->getResultVarianceWriterBlockedTime() );
		    conf_95_element( Xrwlock_writer_holding, rwlock->getResultWriterHoldingTime() );
		    conf_95_element( Xrwlock_writer_holding_variance, rwlock->getResultVarianceWriterHoldingTime() );
		    conf_95_element( Xrwlock_writer_utilization, rwlock->getResultWriterHoldingUtilization() );

		    conf_99_element( Xrwlock_reader_waiting, rwlock->getResultReaderBlockedTime() );
		    conf_99_element( Xrwlock_reader_waiting_variance, rwlock->getResultVarianceReaderBlockedTime() );
		    conf_99_element( Xrwlock_reader_holding, rwlock->getResultReaderHoldingTime() );
		    conf_99_element( Xrwlock_reader_holding_variance, rwlock->getResultVarianceReaderHoldingTime() );
		    conf_99_element( Xrwlock_reader_utilization, rwlock->getResultReaderHoldingUtilization() );
		    conf_99_element( Xrwlock_writer_waiting, rwlock->getResultWriterBlockedTime() );
		    conf_99_element( Xrwlock_writer_waiting_variance, rwlock->getResultVarianceWriterBlockedTime() );
		    conf_99_element( Xrwlock_writer_holding, rwlock->getResultWriterHoldingTime() );
		    conf_99_element( Xrwlock_writer_holding_variance, rwlock->getResultVarianceWriterHoldingTime() );
		    conf_99_element( Xrwlock_writer_utilization, rwlock->getResultWriterHoldingUtilization() );
		}

	    }
	    const std::vector<Entry*>& entries = task->getEntryList();
	    for ( std::vector<Entry *>::const_iterator nextEntry = entries.begin(); nextEntry != entries.end(); ++nextEntry ) {
		insertEntryResults(*nextEntry);
	    }

	    const std::map<std::string,Activity*>& activities = task->getActivities();
	    for ( std::map<std::string,Activity*>::const_iterator nextActivity = activities.begin(); nextActivity != activities.end(); ++nextActivity ) {
		insertActivityResults(nextActivity->second);
	    }

	    const std::set<ActivityList*>& precedences = task->getActivityLists();
	    for ( std::set<ActivityList*>::const_iterator nextPrecedence = precedences.begin(); nextPrecedence != precedences.end(); ++nextPrecedence ) {
		const AndJoinActivityList * join_list = dynamic_cast<const AndJoinActivityList *>(*nextPrecedence);
		if ( join_list ) {
		    insertAndJoinActivityListResults(join_list);
		}
	    }
	}

	void
	Xerces_Document::insertEntryResults( const Entry * entry ) const
	{
	    XercesWrite result_element((DOMElement *)(entry->getXMLDOMElement()),Xresult_entry);

	    /* Actually write the values into the element */
	    result_element(Xthroughput, entry->getResultThroughput());
	    if ( entry->hasResultsForThroughputBound() ) {
		result_element(Xthroughput_bound, entry->getResultThroughputBound());
	    }
	    result_element(Xutilization, entry->getResultUtilization());
	    result_element(Xproc_utilization, entry->getResultProcessorUtilization());
	    result_element(Xsquared_coeff_variation, entry->getResultSquaredCoeffVariation());

	    /* Write out the Phase Results */
	    for ( unsigned p = 1; p <= Phase::MAX_PHASE; ++p ) {
		if (entry->hasResultsForPhase(p)) {
		    result_element(Xphase_service_time[p-1], entry->getResultPhasePServiceTime(p));
		    result_element(Xphase_service_time_variance[p-1], entry->getResultPhasePVarianceServiceTime(p));
		    result_element(Xphase_proc_waiting[p-1], entry->getResultPhasePProcessorWaiting(p));
		}
	    }

	    /* Write out the open wait time */
	    if (entry->hasResultsForOpenWait()) {
		result_element(Xopen_wait_time, entry->getResultOpenWaitTime());
	    }

	    if ( _document.getResultNumberOfBlocks() > 1 ) {
		/* Order is important */
		XercesWriteConfidence conf_99_element(result_element.getElement(), Xresult_conf_99, _conf_99);
		XercesWriteConfidence conf_95_element(result_element.getElement(), Xresult_conf_95, _conf_95 );
		/* Actually write the values into the element */
		conf_99_element(Xthroughput, entry->getResultThroughputVariance());
		conf_95_element(Xthroughput, entry->getResultThroughputVariance());
		conf_95_element(Xutilization, entry->getResultUtilizationVariance());
		conf_99_element(Xutilization, entry->getResultUtilizationVariance());
		conf_95_element(Xproc_utilization, entry->getResultProcessorUtilizationVariance());
		conf_99_element(Xproc_utilization, entry->getResultProcessorUtilizationVariance());

		/* Write out the Phase Results */
		for ( unsigned p = 1; p <= Phase::MAX_PHASE; ++p ) {
		    if (entry->hasResultsForPhase(p)) {
			conf_99_element(Xphase_service_time[p-1], entry->getResultPhasePServiceTimeVariance(p));
			conf_95_element(Xphase_service_time[p-1], entry->getResultPhasePServiceTimeVariance(p));
			conf_99_element(Xphase_service_time_variance[p-1], entry->getResultPhasePVarianceServiceTimeVariance(p));
			conf_95_element(Xphase_service_time_variance[p-1], entry->getResultPhasePVarianceServiceTimeVariance(p));
			conf_99_element(Xphase_proc_waiting[p-1], entry->getResultPhasePProcessorWaitingVariance(p));
			conf_95_element(Xphase_proc_waiting[p-1], entry->getResultPhasePProcessorWaitingVariance(p));
		    }
		}

		/* Write out the open wait time */
		if (entry->hasResultsForOpenWait()) {
		    conf_99_element(Xopen_wait_time, entry->getResultOpenWaitTimeVariance());
		    conf_95_element(Xopen_wait_time, entry->getResultOpenWaitTimeVariance());
		}
	    }

	    const std::map<unsigned, Phase*>& phases = entry->getPhaseList();
	    for ( std::map<unsigned, Phase*>::const_iterator iter = phases.begin(); iter != phases.end(); ++iter) {
		insertPhaseResults( iter->second );
	    }

	    const std::vector<Call *>& forwarding = entry->getForwarding();
	    for ( std::vector<Call *>::const_iterator forwardingIter = forwarding.begin(); forwardingIter != forwarding.end(); ++forwardingIter) {
		insertCallResults(*forwardingIter);
	    }

	}

	void
	Xerces_Document::insertPhaseResults( const Phase* phase ) const
	{
	    const void * element = phase->getXMLDOMElement();
	    if ( !element ) return;

	    if ( phase->hasHistogram() ) {
		insertHistogramResults( phase->getHistogram() );
	    }
	    XercesWrite result_element((DOMNode *)element, Xresult_activity);

	    result_element(Xservice_time, phase->getResultServiceTime() );
	    result_element(Xservice_time_variance, phase->getResultVarianceServiceTime() );
	    result_element(Xutilization, phase->getResultUtilization() );
	    result_element(Xproc_waiting, phase->getResultProcessorWaiting() );
	    if ( _document.getResultNumberOfBlocks() > 1 ) {
		/* Order is important */
		XercesWriteConfidence conf_99_element(result_element.getElement(), Xresult_conf_99, _conf_99 );
		XercesWriteConfidence conf_95_element(result_element.getElement(), Xresult_conf_95, _conf_95 );
		conf_99_element(Xservice_time, phase->getResultServiceTimeVariance() );
		conf_95_element(Xservice_time, phase->getResultServiceTimeVariance() );
		conf_99_element(Xservice_time_variance, phase->getResultVarianceServiceTimeVariance() );
		conf_95_element(Xservice_time_variance, phase->getResultVarianceServiceTimeVariance() );
		conf_99_element(Xutilization, phase->getResultUtilizationVariance() );
		conf_95_element(Xutilization, phase->getResultUtilizationVariance() );
		conf_99_element(Xproc_waiting, phase->getResultProcessorWaitingVariance() );
		conf_95_element(Xproc_waiting, phase->getResultProcessorWaitingVariance() );
	    }

	    const std::vector<Call*>& calls = phase->getCalls();
	    for (std::vector<Call*>::const_iterator callIter = calls.begin(); callIter != calls.end(); ++callIter) {
		insertCallResults(*callIter);
	    }

	}


	void
	Xerces_Document::insertActivityResults( const Activity * activity ) const
	{
	    const void * element = activity->getXMLDOMElement();
	    if ( !element ) return;

	    insertPhaseResults( activity );             /* Do stuff common to phase. */

	    XercesWrite result_element( (DOMNode *)element, Xresult_activity );

	    result_element(Xsquared_coeff_variation, activity->getResultSquaredCoeffVariation());
	    result_element(Xthroughput, activity->getResultThroughput());
	    result_element(Xproc_utilization, activity->getResultProcessorUtilization());

	    if ( _document.getResultNumberOfBlocks() > 1 ) {
		/* Order is important */
		XercesWriteConfidence conf_99_element(result_element.getElement(), Xresult_conf_99, _conf_99 );
		XercesWriteConfidence conf_95_element(result_element.getElement(), Xresult_conf_95, _conf_95 );
		conf_99_element(Xthroughput, activity->getResultThroughputVariance());
		conf_95_element(Xthroughput, activity->getResultThroughputVariance());
		conf_99_element(Xproc_utilization, activity->getResultProcessorUtilizationVariance());
		conf_95_element(Xproc_utilization, activity->getResultProcessorUtilizationVariance());
	    }
	}

	void
	Xerces_Document::insertCallResults( const Call * call ) const
	{
	    XercesWrite result_element( (DOMElement *)(call->getXMLDOMElement()), Xresult_call );	// BUG_455 forward elements can use result-call.
	    result_element(Xwaiting, call->getResultWaitingTime());
	    if ( call->hasResultVarianceWaitingTime() ) {
		result_element(Xwaiting_variance, call->getResultVarianceWaitingTime());
	    }
	    if ( _document.getResultNumberOfBlocks() > 1 ) {
		/* Order is important */
		XercesWriteConfidence conf_99_element(result_element.getElement(), Xresult_conf_99, _conf_99 );
		XercesWriteConfidence conf_95_element(result_element.getElement(), Xresult_conf_95, _conf_95 );
		conf_99_element(Xwaiting, call->getResultWaitingTimeVariance());
		conf_95_element(Xwaiting, call->getResultWaitingTimeVariance());
		if ( call->hasResultVarianceWaitingTime() ) {
		    conf_99_element(Xwaiting_variance, call->getResultVarianceWaitingTimeVariance());
		    conf_95_element(Xwaiting_variance, call->getResultVarianceWaitingTimeVariance());
		}
	    }
	}


	void
	Xerces_Document::insertAndJoinActivityListResults( const AndJoinActivityList * activity_list ) const
	{
	    const void * element = activity_list->getXMLDOMElement();
	    if ( !element ) return;

	    XercesWrite result_element( (DOMNode *)element, Xresult_join_delay );
	    result_element(Xjoin_waiting, activity_list->getResultJoinDelay());
	    if ( activity_list->hasResultVarianceJoinDelay() ) {
		result_element(Xjoin_variance, activity_list->getResultVarianceJoinDelay());
	    }

	    if ( _document.getResultNumberOfBlocks() > 1 ) {
		XercesWriteConfidence conf_99_element(result_element.getElement(), Xresult_conf_99, _conf_99 );
		XercesWriteConfidence conf_95_element(result_element.getElement(), Xresult_conf_95, _conf_95 );
		conf_99_element(Xjoin_waiting, activity_list->getResultJoinDelayVariance());
		conf_95_element(Xjoin_waiting, activity_list->getResultJoinDelayVariance());
		if ( activity_list->hasResultVarianceJoinDelay() ) {
		    conf_99_element(Xjoin_variance, activity_list->getResultVarianceJoinDelayVariance());
		    conf_95_element(Xjoin_variance, activity_list->getResultVarianceJoinDelayVariance());
		}
	    }

	}

        void
	Xerces_Document::insertHistogramResults( const Histogram * histogram ) const
	{
	    const void * element = histogram->getXMLDOMElement();
	    if ( !element ) return;

	    XercesWrite bin_element;
	    for ( unsigned int i = 0; i < histogram->getBins() + 2; ++i ) {
		if ( i == 0 ) {
		    if ( histogram->getBinBegin(i) == histogram->getBinEnd(i) ) continue;		/* Nothing to see here. */
		    bin_element.insertElement( (DOMElement *)element, Xunderflow_bin );
		} else if ( i == histogram->getOverflowIndex() ) {
		    bin_element.insertElement( (DOMElement *)element, Xoverflow_bin );
		} else {
		    bin_element.insertElement( (DOMElement *)element, Xhistogram_bin );
		}
		bin_element( Xbegin, histogram->getBinBegin(i) );
		bin_element( Xend,   histogram->getBinEnd(i) );
		bin_element( Xprob,  histogram->getBinMean(i) );
		const double variance = histogram->getBinVariance(i);
		if ( variance > 0 && _document.getResultNumberOfBlocks() > 1 ) {
		    bin_element( Xconf_95, _conf_95( variance ) );
		    bin_element( Xconf_99, _conf_99( variance ) );
		}
	    }
	}

	bool
	Xerces_Document::compareVersionNumber(DOMDocument *doc, const char *attribName, double supportedVersionNum)
	{
	    StrX version(_root->getAttribute(X(attribName)));
	
	    return version.asDouble() <= supportedVersionNum;
	}

	bool
	Xerces_Document::isXMLDOMPresent() const
	{
	    return inputFileDOM != 0;
	}

	void
	Xerces_Document::initialize_strings()
	{
            XPH1PH2 =                           Xt("PH1PH2");
            Xactivity =                         Xt("activity");
            Xactivity =                         Xt("activity");
            Xasynch_call =                      Xt("asynch-call");
            Xbegin =                            Xt("begin");
            Xbound_to_entry =                   Xt("bound-to-entry");
            Xcall_order =                       Xt("call-order");
            Xcalls_mean =                       Xt("calls-mean");
            Xcap =                              Xt("cap");
            Xcomment =                          Xt("comment");
            Xconf_95 =                          Xt("conf-95");
            Xconf_99 =                          Xt("conf-99");
            Xconv_val =                         Xt("conv_val");
            Xconv_val_result =                  Xt("conv-val");
            Xcore =                             Xt("core");
            Xcount =                            Xt("count");
            Xdest =                             Xt("dest");
            Xdeterministic =                    Xt("deterministic");
            Xdrop_probability =                 Xt("drop-probability");
            Xelapsed_time =                     Xt("elapsed-time");
            Xend =                              Xt("end");
            Xentry =                            Xt("entry");
            Xentry =                            Xt("entry");
            Xentry_phase_activities =           Xt("entry-phase-activities");
            Xfanin =                            Xt("fan-in");
            Xfanout =                           Xt("fan-out");
            Xfaults =                           Xt("faults");
            Xforwarding =                       Xt("forwarding");
            Xgroup =                            Xt("group");
            Xhistogram_bin =                    Xt("histogram-bin");
            Xhost_demand_cvsq =                 Xt("host-demand-cvsq");
            Xhost_demand_mean =                 Xt("host-demand-mean");
            Xinitially =                        Xt("initially");
            Xit_limit =                         Xt("it_limit");
            Xiterations =                       Xt("iterations");
            Xjoin_variance =                    Xt("join-variance");
            Xjoin_waiting =                     Xt("join-waiting");
            Xlqn_model =                        Xt("lqn-model");
            Xlqx =                              Xt("lqx");
            Xmax =                              Xt("max");
            Xmax_service_time =                 Xt("max-service-time");
            Xmin =                              Xt("min");
            Xmultiplicity =                     Xt("multiplicity");
            Xmva_info =                         Xt("mva-info");
            Xname =                             Xt("name");
            Xnone =                             Xt("none");
            Xnumber_bins =                      Xt("number-bins");
            Xopen_arrival_rate =                Xt("open-arrival-rate");
            Xopen_wait_time =                   Xt("open-wait-time");
            Xoverflow_bin =                     Xt("overflow-bin");
            Xparam =                            Xt("param");
            Xphase =                            Xt("phase");
            Xphase_proc_waiting[0] =            Xt("phase1-proc-waiting"); 
            Xphase_proc_waiting[1] =            Xt("phase2-proc-waiting"); 
            Xphase_proc_waiting[2] =            Xt("phase3-proc-waiting"); 
            Xphase_service_time[0] =            Xt("phase1-service-time");          /* We're going to change X */
            Xphase_service_time[1] =            Xt("phase2-service-time");          /* We're going to change X */
            Xphase_service_time[2] =            Xt("phase3-service-time");          /* We're going to change X */
            Xphase_service_time_variance[0]=    Xt("phase1-service-time-variance");
            Xphase_service_time_variance[1]=    Xt("phase2-service-time-variance");
            Xphase_service_time_variance[2]=    Xt("phase3-service-time-variance");
            Xphase_utilization[0] =             Xt("phase1-utilization");
            Xphase_utilization[1] =             Xt("phase2-utilization");
            Xphase_utilization[2] =             Xt("phase3-utilization");
            Xplatform_info =                    Xt("platform-info");
            Xpragma =                           Xt("pragma");
            Xprecedence =                       Xt("precedence");
            Xprint_int =                        Xt("print_int");
            Xpriority =                         Xt("priority");
            Xprob =                             Xt("prob");
            Xproc_utilization =                 Xt("proc-utilization");
            Xproc_waiting =                     Xt("proc-waiting");
            Xprocessor =                        Xt("processor");
            Xquantum =                          Xt("quantum");
            Xqueue_length =                     Xt("queue-length");
            Xquorum =                           Xt("quorum");
            Xr_lock =                           Xt("r-lock");
            Xr_lock =                           Xt("r-lock");
            Xr_unlock =                         Xt("r-unlock");
            Xr_unlock =                         Xt("r-unlock");
            Xreplication =                      Xt("replication");
            Xreply_activity =                   Xt("reply-activity");
            Xreply_entry =                      Xt("reply-entry");
            Xresult_activity =                  Xt("result-activity");
            Xresult_call =                      Xt("result-call");
            Xresult_conf_95 =                   Xt("result-conf-95");
            Xresult_conf_99 =                   Xt("result-conf-99");
            Xresult_entry =                     Xt("result-entry");
            Xresult_forwarding =                Xt("result-forwarding");
            Xresult_general =                   Xt("result-general");
            Xresult_group =                     Xt("result-group");
            Xresult_join_delay =                Xt("result-join-delay");
            Xresult_processor =                 Xt("result-processor");
            Xresult_task =                      Xt("result-task");
            Xrwlock =                           Xt("rwlock");
            Xrwlock_reader_holding =            Xt("rwlock-reader-holding");
            Xrwlock_reader_holding_variance =   Xt("rwlock-reader-holding-variance");
            Xrwlock_reader_utilization =        Xt("rwlock-reader_utilization");
            Xrwlock_reader_waiting =            Xt("rwlock-reader-waiting");
            Xrwlock_reader_waiting_variance =   Xt("rwlock-reader-waiting-variance");
            Xrwlock_writer_holding =            Xt("rwlock-writer-holding");
            Xrwlock_writer_holding_variance =   Xt("rwlock-writer-holding-variance");
            Xrwlock_writer_utilization =        Xt("rwlock-writer_utilization");
            Xrwlock_writer_waiting =            Xt("rwlock-writer-waiting");
            Xrwlock_writer_waiting_variance =   Xt("rwlock-writer-waiting-variance");
            Xscheduling =                       Xt("scheduling");
            Xsemaphore =                        Xt("semaphore");
            Xsemaphore_utilization =            Xt("semaphore-utilization");
            Xsemaphore_waiting =                Xt("semaphore-waiting");
            Xsemaphore_waiting_variance =       Xt("semaphore-waiting-variance");
            Xservice =                          Xt("service");
            Xservice_time =                     Xt("service-time");
            Xservice_time_distribution =        Xt("service-time-distribution");
            Xservice_time_variance =            Xt("service-time-variance");
            Xshare =                            Xt("share");
            Xsignal =                           Xt("signal");
            Xsolver_info =                      Xt("solver-info");
            Xsolver_parameters =                Xt("solver-params");
            Xsource =                           Xt("source" );
            Xspeed_factor =                     Xt("speed-factor");
            Xsquared_coeff_variation =          Xt("squared-coeff-variation");
            Xstep =                             Xt("step");
            Xstep_squared =                     Xt("step-squared");
            Xsubmodels =                        Xt("submodels");
            Xsynch_call =                       Xt("synch-call");
            Xsystem_cpu_time =                  Xt("system-cpu-time");
            Xtask =                             Xt("task");
            Xtask =                             Xt("task");
            Xtask_activities =                  Xt("task-activities");
            Xtask_activities =                  Xt("task-activities");
            Xthink_time =                       Xt("think-time");
            Xthroughput =                       Xt("throughput");
            Xthroughput_bound =                 Xt("throughput-bound");
            Xtype =                             Xt("type");
            Xunderflow_bin =                    Xt("underflow-bin");
            Xunderrelax_coeff =                 Xt("underrelax_coeff");
            Xuser_cpu_time =                    Xt("user-cpu-time");
            Xutilization =                      Xt("utilization");
            Xvalid =                            Xt("valid");
            Xvalue =                            Xt("value");
            Xwait =                             Xt("wait");
            Xwait_squared =                     Xt("wait-squared");
            Xwaiting =                          Xt("waiting");
            Xwaiting_variance =                 Xt("waiting-variance");
            Xxml_debug =                        Xt("xml-debug");

	    Xpre_post[Xt("pre")] = 	    ActivityList::JOIN_ACTIVITY_LIST;
	    Xpre_post[Xt("pre-OR")] =	    ActivityList::OR_JOIN_ACTIVITY_LIST;
	    Xpre_post[Xt("pre-AND")] =      ActivityList::AND_JOIN_ACTIVITY_LIST;
	    Xpre_post[Xt("post")] =	    ActivityList::FORK_ACTIVITY_LIST;
	    Xpre_post[Xt("post-OR")] =	    ActivityList::OR_FORK_ACTIVITY_LIST;
	    Xpre_post[Xt("post-AND")] =     ActivityList::AND_FORK_ACTIVITY_LIST;
	    Xpre_post[Xt("post-LOOP")] =    ActivityList::REPEAT_ACTIVITY_LIST;
        }

	void
	Xerces_Document::delete_strings()
	{
	    Xr(&XPH1PH2);
	    Xr(&Xactivity);
	    Xr(&Xactivity);
	    Xr(&Xasynch_call);
	    Xr(&Xbound_to_entry);
	    Xr(&Xbegin);
	    Xr(&Xconf_95);
	    Xr(&Xconf_99);
	    Xr(&Xcall_order);
	    Xr(&Xcalls_mean);
	    Xr(&Xcap);
	    Xr(&Xcomment);
	    Xr(&Xconv_val);
	    Xr(&Xconv_val_result);
	    Xr(&Xcore);
	    Xr(&Xcount);
	    Xr(&Xdest);
	    Xr(&Xdeterministic);
	    Xr(&Xdrop_probability);
	    Xr(&Xelapsed_time);
	    Xr(&Xend);
	    Xr(&Xentry);
	    Xr(&Xentry);
	    Xr(&Xentry_phase_activities);
	    Xr(&Xfanin);
	    Xr(&Xfanout);
	    Xr(&Xfaults);
	    Xr(&Xforwarding);
	    Xr(&Xgroup);
	    Xr(&Xhost_demand_cvsq);
	    Xr(&Xhost_demand_mean);
	    Xr(&Xmax_service_time);
	    Xr(&Xit_limit);
	    Xr(&Xiterations);
	    Xr(&Xjoin_variance);
	    Xr(&Xjoin_waiting);
	    Xr(&Xlqn_model);
	    Xr(&Xlqx);
	    Xr(&Xmax);
	    Xr(&Xmin);
	    Xr(&Xmultiplicity);
	    Xr(&Xmva_info);
	    Xr(&Xname);
	    Xr(&Xnone);
	    Xr(&Xnumber_bins);
	    Xr(&Xopen_arrival_rate);
	    Xr(&Xopen_wait_time);
	    Xr(&Xparam);
	    Xr(&Xphase);
	    Xr(&Xphase_proc_waiting[0]);
	    Xr(&Xphase_proc_waiting[1]);
	    Xr(&Xphase_proc_waiting[2]);
	    Xr(&Xphase_service_time[0]);
	    Xr(&Xphase_service_time[1]);
	    Xr(&Xphase_service_time[2]);
	    Xr(&Xphase_service_time_variance[0]);
	    Xr(&Xphase_service_time_variance[1]);
	    Xr(&Xphase_service_time_variance[2]);
	    Xr(&Xphase_utilization[0]);
	    Xr(&Xphase_utilization[1]);
	    Xr(&Xphase_utilization[2]);
	    Xr(&Xphase_waiting[0]);
	    Xr(&Xphase_waiting[1]);
	    Xr(&Xphase_waiting[2]);
	    Xr(&Xphase_waiting_variance[0]);
	    Xr(&Xphase_waiting_variance[1]);
	    Xr(&Xphase_waiting_variance[2]);
	    Xr(&Xplatform_info);
	    Xr(&Xpragma);
	    Xr(&Xprecedence);
	    Xr(&Xprint_int);
	    Xr(&Xpriority);
	    Xr(&Xprob);
	    Xr(&Xproc_utilization);
	    Xr(&Xproc_waiting);
	    Xr(&Xprocessor);
	    Xr(&Xquantum);
	    Xr(&Xqueue_length);
	    Xr(&Xquorum);
	    Xr(&Xreplication);
	    Xr(&Xreply_activity);
	    Xr(&Xreply_entry);
	    Xr(&Xresult_activity);
	    Xr(&Xresult_call);
	    Xr(&Xresult_conf_95);
	    Xr(&Xresult_conf_99);
	    Xr(&Xresult_entry);
	    Xr(&Xresult_general);
	    Xr(&Xresult_group);
	    Xr(&Xresult_join_delay);
	    Xr(&Xresult_processor);
	    Xr(&Xresult_task);
	    Xr(&Xr_lock);
	    Xr(&Xr_unlock);
	    Xr(&Xrwlock);
	    Xr(&Xrwlock_reader_waiting);
	    Xr(&Xrwlock_reader_waiting_variance);
	    Xr(&Xrwlock_reader_holding);
	    Xr(&Xrwlock_reader_holding_variance);
	    Xr(&Xrwlock_reader_utilization);
	    Xr(&Xrwlock_writer_waiting);
	    Xr(&Xrwlock_writer_waiting_variance);
	    Xr(&Xrwlock_writer_holding);
	    Xr(&Xrwlock_writer_holding_variance);
	    Xr(&Xrwlock_writer_utilization);
	    Xr(&Xscheduling);
	    Xr(&Xsemaphore);
	    Xr(&Xservice);
	    Xr(&Xservice_time);
	    Xr(&Xservice_time_distribution);
	    Xr(&Xservice_time_variance);
	    Xr(&Xshare);
	    Xr(&Xsignal);
	    Xr(&Xsolver_info);
	    Xr(&Xsolver_parameters);
	    Xr(&Xspeed_factor);
	    Xr(&Xsquared_coeff_variation);
	    Xr(&Xstep);
	    Xr(&Xstep_squared);
	    Xr(&Xsubmodels);
	    Xr(&Xsynch_call);
	    Xr(&Xsystem_cpu_time);
	    Xr(&Xtask);
	    Xr(&Xtask);
	    Xr(&Xtask_activities);
	    Xr(&Xtask_activities);
	    Xr(&Xthink_time);
	    Xr(&Xthroughput);
	    Xr(&Xthroughput_bound);
	    Xr(&Xtype);
	    Xr(&Xunderrelax_coeff);
	    Xr(&Xuser_cpu_time);
	    Xr(&Xutilization);
	    Xr(&Xvalid);
	    Xr(&Xvalue);
	    Xr(&Xwait);
	    Xr(&Xwait_squared);
	    Xr(&Xwaiting);
	    Xr(&Xwaiting_variance);
	    Xr(&Xw_lock);
	    Xr(&Xw_unlock);
	    Xr(&Xxml_debug);
            Xr(&Xhistogram_bin);
            Xr(&Xoverflow_bin);
            Xr(&Xresult_forwarding);
            Xr(&Xunderflow_bin);

	    std::map<XMLCh *,ActivityList::ActivityListType>::iterator item;
	    for ( item = Xpre_post.begin(); item != Xpre_post.end(); ++item ) {
		XMLCh * x = item->first;
		Xr( &x );
	    }
	}

	/* We clear out any old results that may be in the XML file before doing any insertion to the DOM */

	void 
	Xerces_Document::clearResultList(DOMNodeList *resultList)
	{
	    for (unsigned x=0;x<resultList->getLength();x++) {
		DOMNode * parentNode = (resultList->item(x))->getParentNode();
		parentNode->removeChild(resultList->item(x)->getPreviousSibling());
		parentNode->removeChild(resultList->item(x));
		x--;
	    }   
	}

	void 
	Xerces_Document::clearExistingDOMResults()
	{
	    clearResultList(inputFileDOM->getElementsByTagName(Xmva_info));
//	    clearResultList(inputFileDOM->getElementsByTagName(Xmva_pramga));
	    clearResultList(inputFileDOM->getElementsByTagName(Xresult_general));
	    clearResultList(inputFileDOM->getElementsByTagName(Xresult_processor));
	    clearResultList(inputFileDOM->getElementsByTagName(Xresult_task));
	    clearResultList(inputFileDOM->getElementsByTagName(Xresult_entry));
	    clearResultList(inputFileDOM->getElementsByTagName(Xresult_activity));
	    clearResultList(inputFileDOM->getElementsByTagName(Xresult_join_delay));
	    clearResultList(inputFileDOM->getElementsByTagName(Xresult_forwarding));
	    clearResultList(inputFileDOM->getElementsByTagName(Xresult_call));
	    clearResultList(inputFileDOM->getElementsByTagName(Xresult_conf_95));
	    clearResultList(inputFileDOM->getElementsByTagName(Xresult_conf_99));
	    clearResultList(inputFileDOM->getElementsByTagName(Xhistogram_bin));
	    clearResultList(inputFileDOM->getElementsByTagName(Xoverflow_bin));
	    clearResultList(inputFileDOM->getElementsByTagName(Xunderflow_bin));
	}

	ostream&
	Xerces_Document::printStartElement( ostream& output, const DOMElement *element )
	{
	    printStartElement2( output, element );
	    output  << ">";
	    return output;
	}

	ostream&
	Xerces_Document::printSimpleElement( ostream& output, const DOMElement *element )
	{
	    printStartElement2( output, element );
	    output  << "/>";
	    if ( __indent > 0 ) {
		__indent -= 1;
	    }
	    return output;
	}

	ostream&
	Xerces_Document::printStartElement2( ostream& output, const DOMElement *element )
	{
	    if ( __indent != 0 ) {
		output << setw( __indent * 3 ) << " ";
	    }
	    __indent += 1;
	    output << "<" << StrX(element->getTagName()).asCStr();
	    StrX name( element->getAttribute(Xname) );
	    if ( name.optCStr() ) {
		output << " name=\"" << name.asCStr() << "\"";
	    }
	    return output;
	}

	ostream&
	Xerces_Document::printEndElement( ostream& output, const DOMElement *element )
	{
	    if ( __indent > 0 ) {
		__indent -= 1;
		if ( __indent > 0 ) {
		    output << setw( __indent * 3 ) << " ";
		}
	    }
	    output << "</" << StrX(element->getTagName()).asCStr() << ">";
	    return output;
	}

    }
}
