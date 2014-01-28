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
 * December 2003.
 * May 2010.
 * ------------------------------------------------------------------------
 */

#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdexcept>
#include <fstream>
#include <iomanip>
#include "expat_document.h"
#include <string>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <vector>
#include <cassert>
#include <cmath>
#include <fcntl.h>
#include <sys/stat.h>
#if HAVE_SYS_MMAN_H
#include <sys/types.h>
#include <sys/mman.h>
#endif
#if HAVE_SYS_ERRNO_H
#include <sys/errno.h>
#endif
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#if HAVE_STRINGS_H
#include <strings.h>
#endif

#include "dom_object.h"
#include "srvn_results.h"
#include "input.h"
#include "error.h"
#include "glblerr.h"
#include "filename.h"
#include "dom_phase.h"
#include "dom_task.h"
#include "dom_actlist.h"
#include "dom_histogram.h"

namespace LQIO {
    namespace DOM {
        using namespace std;

        bool Expat_Document::__instantiate = false;             // Instantiate external variables iff true.
        int Expat_Document::__indent = 0;
        const XML_Char * Expat_Document::XMLSchema_instance = "http://www.w3.org/2001/XMLSchema-instance";

        const XML_Char * Expat_Document::XDETERMINISTIC =                       "DETERMINISTIC";
        const XML_Char * Expat_Document::XGRAPH =                               "GRAPH";
        const XML_Char * Expat_Document::XNONE =                                "NONE";
        const XML_Char * Expat_Document::XPH1PH2 =                              "PH1PH2";
        const XML_Char * Expat_Document::Xactivity =                            "activity";
        const XML_Char * Expat_Document::Xactivity_graph =                      "activity-graph";
        const XML_Char * Expat_Document::Xasynch_call =                         "asynch-call";
        const XML_Char * Expat_Document::Xbegin =                               "begin";
        const XML_Char * Expat_Document::Xbound_to_entry =                      "bound-to-entry";
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
        const XML_Char * Expat_Document::Xentry =                               "entry";
        const XML_Char * Expat_Document::Xentry_phase_activities =              "entry-phase-activities";
        const XML_Char * Expat_Document::Xfanin =                               "fan-in";
        const XML_Char * Expat_Document::Xfanout =                              "fan-out";
        const XML_Char * Expat_Document::Xfaults =                              "faults";
        const XML_Char * Expat_Document::Xforwarding =                          "forwarding";
        const XML_Char * Expat_Document::Xgroup =                               "group";
        const XML_Char * Expat_Document::Xhistogram_bin =                       "histogram-bin";
        const XML_Char * Expat_Document::Xhost_demand_cvsq =                    "host-demand-cvsq";
        const XML_Char * Expat_Document::Xhost_demand_mean =                    "host-demand-mean";
        const XML_Char * Expat_Document::Xhost_max_phase_service_time =         "host-max-phase-service-time";
        const XML_Char * Expat_Document::Xinitially =                           "initially";
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
        const XML_Char * Expat_Document::Xsource =                              "source";
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

        std::set<const XML_Char *,Expat_Document::attribute_table_t> Expat_Document::model_table;
        std::set<const XML_Char *,Expat_Document::attribute_table_t> Expat_Document::parameter_table;
        std::set<const XML_Char *,Expat_Document::attribute_table_t> Expat_Document::processor_table;
        std::set<const XML_Char *,Expat_Document::attribute_table_t> Expat_Document::group_table;
        std::set<const XML_Char *,Expat_Document::attribute_table_t> Expat_Document::task_table;
        std::set<const XML_Char *,Expat_Document::attribute_table_t> Expat_Document::entry_table;
        std::set<const XML_Char *,Expat_Document::attribute_table_t> Expat_Document::activity_table;
	std::map<const XML_Char, const XML_Char *> Expat_Document::escape_table;
        std::map<const XML_Char *,ActivityList::ActivityListType,Expat_Document::attribute_table_t> Expat_Document::precedence_table;
        std::set<const XML_Char *,Expat_Document::attribute_table_t> Expat_Document::call_table;
        std::set<const XML_Char *,Expat_Document::attribute_table_t> Expat_Document::histogram_table;
        std::map<const XML_Char *,Expat_Document::result_table_t,Expat_Document::result_table_t>  Expat_Document::result_table;

        /* ---------------------------------------------------------------- */
        /* DOM input.                                                       */
        /* ---------------------------------------------------------------- */

        Expat_Document::Expat_Document( lqio_params_stats* ioVars, const bool load_results )
            : Document( ioVars ), _parser(), _XMLDOMPresent(false), _createObjects(true), _loadResults(load_results), _resultsLoaded(false)
        {
	    init_tables();

            io_vars->error_count = 0;                   /* See error.c */
            io_vars->anError = false;
        }


        Expat_Document::~Expat_Document()
        {
        }


        void
        Expat_Document::initialize()
        {
            _parser = XML_ParserCreateNS(NULL,'/');     /* Gobble header goop */
            if ( !_parser ) {
                throw runtime_error("");
            }

            XML_SetElementHandler( _parser, start, end );
            XML_SetCdataSectionHandler( _parser, start_cdata, end_cdata );
            XML_SetCharacterDataHandler( _parser, handle_text );
	    XML_SetCommentHandler( _parser, handle_comment );
            XML_SetUnknownEncodingHandler( _parser, handle_encoding, static_cast<void *>(this) );
            XML_SetUserData( _parser, static_cast<void *>(this) );

            _stack.push( parse_stack_t("",&Expat_Document::startModel,0) );
        }



        Document *
        Expat_Document::LoadLQNX( const std::string& input_filename, const std::string& output_filename, lqio_params_stats* ioVars, unsigned& errorCode, const bool load_results )
        {
            input_file_name = input_filename.c_str();
            output_file_name = output_filename.c_str();
	    Expat_Document * document = new Expat_Document( ioVars, load_results );

	    if ( !document ) return 0;

	    if ( !document->parse( input_filename ) ) {
		delete document;
		document = 0;
	    } else {
		const std::string& program_text = document->getLQXProgramText();
		if ( program_text.size() ) {
		    /* If we have an LQX program, then we need to compute */
		    LQX::Program* program = LQX::Program::loadFromText(input_filename.c_str(), document->getLQXProgramLineNumber(), program_text.c_str());
		    if (program == NULL) {
			LQIO::solution_error( LQIO::ERR_LQX_COMPILATION, input_filename.c_str() );
		    }
		    document->setLQXProgram( program );
		}
	    }

            return document;
        }

        /*
         * Load results (only) from filename.  The input DOM must be present (and match iff LQNX input).
         */

        bool
        Expat_Document::loadResults( const char * filename, unsigned& errorCode )
        {
            errorCode = 0;
	    _createObjects = false;			/* Don't make new objects. 	*/
	    _loadResults = true;			/* (default for this is false)	*/
	    _resultsLoaded = parse( filename );
	    return _resultsLoaded;
        }


        /*
         * Parse the XML file, catching any XML exceptions that might
         * propogate out of it.
         */

        bool
        Expat_Document::parse( const std::string& file_name )
        {
	    struct stat statbuf;
	    bool rc = true;
            int input_fd = -1;

            if ( file_name ==  "-" ) {
                input_fd = fileno( stdin );
                input_file_name = io_vars->lq_toolname;
            } else if ( ( input_fd = open( file_name.c_str(), O_RDONLY ) ) < 0 ) {
                std::cerr << io_vars->lq_toolname << ": Cannot open input file " << file_name << " - " << strerror( errno ) << std::endl;
                return false;
            }

            if ( isatty( input_fd ) ) {
                std::cerr << io_vars->lq_toolname << ": Input from terminal is not allowed." << std::endl;
		return false;
	    } else if ( fstat( input_fd, &statbuf ) != 0 ) {
                std::cerr << io_vars->lq_toolname << ": Cannot stat " << input_file_name << " - " << strerror( errno ) << std::endl;
		return false;
#if defined(S_ISSOCK)
	    } else if ( !S_ISREG(statbuf.st_mode) && !S_ISFIFO(statbuf.st_mode) && !S_ISSOCK(statbuf.st_mode) ) {
#else
	    } else if ( !S_ISREG(statbuf.st_mode) && !S_ISFIFO(statbuf.st_mode) ) {
#endif
                std::cerr << io_vars->lq_toolname << ": Input from " << input_file_name << " is not allowed." << std::endl;
		return false;
            } 

            initialize();		/* Creates parser */

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
			    std::cerr << Document::io_vars->lq_toolname << ": Read error on " << input_file_name << " - " << strerror( errno ) << std::endl;
			    rc = false;
			    break;
			} else if (!XML_Parse(_parser, buffer, len, len != BUFFSIZE)) {
			    const char * error = XML_ErrorString(XML_GetErrorCode(_parser));
			    input_error( error );
			    rc = false;
			    break;
			}
		    } while ( len == BUFFSIZE );
#if HAVE_MMAP
		}
#endif
	    }
	    /* Halt on any error. */
	    catch ( LQIO::element_error& e ) {
		input_error( "Unexpected element <%s> ", e.what() );
		rc = false;
	    }
	    catch ( LQIO::missing_attribute & e ) {
		rc = false;
	    }
	    catch ( LQIO::unexpected_attribute & e ) {
		rc = false;
	    }
	    catch ( std::invalid_argument & e ) {
		rc = false;
	    }
	    catch ( LQIO::undefined_symbol & e ) {
		rc = false;
	    }
	    catch ( domain_error & e ) {
		input_error( "Domain error: %s ", e.what() );
		rc = false;
	    }

#if HAVE_MMAP
	    if ( buffer != MAP_FAILED ) {
		munmap( buffer, statbuf.st_size );
	    }
#endif
            XML_ParserFree(_parser);
            close( input_fd );
            return rc;
        }

        void
        Expat_Document::input_error( const char * fmt, ... ) const
        {
            va_list args;
            va_start( args, fmt );
            verrprintf( stderr, RUNTIME_ERROR, input_file_name,  XML_GetCurrentLineNumber(_parser), 0, fmt, args );
            va_end( args );
        }

        /*
         * Handlers called from Expat.
         */

        void
        Expat_Document::start( void *data, const XML_Char *el, const XML_Char **attr )
        {
            Expat_Document * document = static_cast<Expat_Document *>(data);
            document->_XMLDOMPresent = true;
	    LQIO_line_number = XML_GetCurrentLineNumber(document->_parser);
            const parse_stack_t& top = document->_stack.top();
            if ( __debugXML ) {
		cerr << setw(4) << LQIO_line_number << ": ";
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
                if ( !top.func ) internal_error( __FILE__, __LINE__, "empty stack." );
                (document->*top.func)(top.object,el,attr);
            }
            catch ( LQIO::missing_attribute & e ) {
                document->input_error( "Missing attribute \"%s\" for element <%s>", e.what(), el );
                throw;
            }
            catch ( LQIO::unexpected_attribute & e ) {
                document->input_error( "Unexpected attribute \"%s\" for element <%s>", e.what(), el );
                throw;
            }
            catch ( std::invalid_argument & e ) {
                document->input_error( "Invalid argument \"%s\" to attribute for element <%s>", e.what(), el );
                throw;
            }
            catch ( LQIO::undefined_symbol & e ) {
                input_error2( ERR_NOT_DEFINED, e.what() );
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
            DocumentObject * extra_object = 0;
            bool done = false;
            while ( document->_stack.size() > 0 && !done ) {
                parse_stack_t& top = document->_stack.top();
                if ( top.extra_object ) {
                    extra_object = top.extra_object;
                }
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
            Expat_Document * document = static_cast<Expat_Document *>(data);
            if ( document->_stack.size() == 0 ) return;
            const parse_stack_t& top = document->_stack.top();
            if ( top.func == &Expat_Document::startLQX ) {
                string& program = const_cast<string &>(document->getLQXProgramText());
                program.append( text, length );
            }
        }

	/* 
	 * At some point, we might tack the comment onto the current element.
	 * For now, just toss it.
	 */

	void
	Expat_Document::handle_comment( void * data, const XML_Char * text )
	{
            Expat_Document * document = static_cast<Expat_Document *>(data);
            if ( document->_stack.size() == 0 ) return;
            const parse_stack_t& top = document->_stack.top();
	    DocumentObject * object = top.object;

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
            checkAttributes( attributes, model_table );
            if ( strcasecmp( element, Xlqn_model ) == 0 ) {
                __debugXML = (__debugXML || getBoolAttribute(attributes,Xxml_debug));
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
                setLQXProgramLineNumber(XML_GetCurrentLineNumber(_parser));
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
        Expat_Document::startResultGeneral( DocumentObject * object, const XML_Char * element, const XML_Char ** attributes )
        {
            if ( strcasecmp( element, Xresult_general ) == 0 ) {
                if ( _loadResults ) {
                    const long iterations = getLongAttribute(attributes,Xiterations);
                    setResultValid( getBoolAttribute(attributes,Xvalid) );
                    setResultConvergenceValue( getDoubleAttribute(attributes,Xconv_val_result) );
                    setResultIterations( iterations );
                    setResultElapsedTime( getTimeAttribute(attributes, Xelapsed_time) );
                    setResultSysTime( getTimeAttribute(attributes,Xsystem_cpu_time) );
                    setResultUserTime( getTimeAttribute(attributes,Xuser_cpu_time) );
                    setResultPlatformInformation( getStringAttribute(attributes,Xplatform_info) );
                    if ( 1 < iterations && iterations <= 30 ) {
                        const_cast<ConfidenceIntervals *>(&_conf_95)->set_blocks( iterations );
                    }
                }
                _stack.push( parse_stack_t(element,&Expat_Document::startMVAInfo,object) );
            } else if ( strcasecmp( element, Xpragma ) == 0 ) {
                const XML_Char * parameter = getStringAttribute(attributes,Xparam);
                addPragma(parameter,getStringAttribute(attributes,Xvalue,""));
                _stack.push( parse_stack_t(element,&Expat_Document::startNOP,0) );
            } else {
                throw element_error( element );
            }
        }


        void
        Expat_Document::startMVAInfo( DocumentObject *, const XML_Char * element, const XML_Char ** attributes )
        {
            if ( strcasecmp( element, Xmva_info ) == 0 ) {
                if ( _loadResults ) {
                    setMVAStatistics( getLongAttribute(attributes,Xsubmodels),
                                      getLongAttribute(attributes,Xcore),
                                      getDoubleAttribute(attributes,Xstep),
                                      getDoubleAttribute(attributes,Xstep_squared),
                                      getDoubleAttribute(attributes,Xwait),
                                      getDoubleAttribute(attributes,Xwait_squared),
                                      getLongAttribute(attributes,Xfaults) );
                }
                _stack.push( parse_stack_t(element,&Expat_Document::startNOP,0) );
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
                handleResults( processor, attributes );
                _stack.push( parse_stack_t(element,&Expat_Document::startOutputResultType,processor) );

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
                handleResults( group, attributes );
                _stack.push( parse_stack_t(element,&Expat_Document::startOutputResultType,group) );

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
                handleResults( task, attributes );
                _stack.push( parse_stack_t(element,&Expat_Document::startOutputResultType,task) );

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
                handleResults( entry, attributes );
                _stack.push( parse_stack_t(element,&Expat_Document::startOutputResultType,entry) );

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
                handleResults( activity, attributes );
                _stack.push( parse_stack_t(element,&Expat_Document::startOutputResultType,activity) );

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
                    handleResults( call, attributes );
                }
                _stack.push( parse_stack_t(element,&Expat_Document::startOutputResultType,call) );
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
                    handleResults( call, attributes );
                }
                _stack.push( parse_stack_t(element,&Expat_Document::startOutputResultType,call) );
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
                Entry * entry = getEntryByName( entry_name );
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
                        pre_list = new AndJoinActivityList( this, dynamic_cast<Task *>(task), item->second, 0,
							    db_build_parameter_variable(getStringAttribute(attributes,Xquorum,"0"),NULL) );
                    } else {
                        pre_list = new ActivityList( this, dynamic_cast<Task *>(task), item->second, 0 );
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
                    post_list = new ActivityList( this, dynamic_cast<Task *>(task), item->second, 0 );
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
        Expat_Document::startLQX( DocumentObject * object, const XML_Char * element, const XML_Char ** attributes )
        {
            throw element_error( element );             /* Should not get here. */
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
            checkAttributes( attributes, parameter_table );
            if ( _createObjects ) {
                setModelParameters( getStringAttribute(attributes,Xcomment),
                                    db_build_parameter_variable(getStringAttribute(attributes,Xconv_val,"0.00001"),NULL),
				    db_build_parameter_variable(getStringAttribute(attributes,Xit_limit,"50"),NULL),
				    db_build_parameter_variable(getStringAttribute(attributes,Xprint_int,"0"),NULL),
				    db_build_parameter_variable(getStringAttribute(attributes,Xunderrelax_coeff,"0.9"),NULL),
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
            checkAttributes( attributes, processor_table );

            const XML_Char * processor_name = getStringAttribute(attributes,Xname);
            Processor * processor = getProcessorByName( getStringAttribute(attributes,Xname) );
            if ( _createObjects ) {
                if ( processor ) {
                    throw duplicate_symbol( processor_name );
                }
                const scheduling_type scheduling_flag = static_cast<scheduling_type>(getEnumerationAttribute( attributes, Xscheduling, schedulingTypeXMLString, SCHEDULE_FIFO ));
                processor = new Processor(this, processor_name,
                                          scheduling_flag,
                                          db_build_parameter_variable(getStringAttribute(attributes,Xmultiplicity,"1"),NULL),
                                          getLongAttribute(attributes,Xreplication,1),
                                          0 );
                addProcessorEntity( processor );
                io_vars->n_processors += 1;

                const XML_Char * quantum = getStringAttribute(attributes,Xquantum,"");
                if ( strlen(quantum) > 0 ) {
                    if ( scheduling_flag == SCHEDULE_FIFO
                         || scheduling_flag == SCHEDULE_HOL
                         || scheduling_flag == SCHEDULE_PPR
                         || scheduling_flag == SCHEDULE_RAND ) {
                        input_error2( LQIO::WRN_QUANTUM_SCHEDULING, processor_name, scheduling_type_str[scheduling_flag] );
                    } else {
                        processor->setQuantum( db_build_parameter_variable(quantum,NULL) );
                    }
                } else if ( scheduling_flag == SCHEDULE_CFS ) {
                    input_error2( LQIO::ERR_NO_QUANTUM_SCHEDULING, processor_name, scheduling_type_str[scheduling_flag] );
                }

                const XML_Char * rate = getStringAttribute(attributes,Xspeed_factor,"");
                if ( strlen( rate ) > 0 ) {
                    processor->setRate( db_build_parameter_variable( rate, NULL ) );
                } else {
                    processor->setRate( new ConstantExternalVariable( 1. ) );
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
            checkAttributes( attributes, group_table );

            const XML_Char * group_name = getStringAttribute(attributes,Xname);
            Group* group = getGroupByName( group_name );
	    if ( dynamic_cast<Processor *>(processor)->getSchedulingType() != SCHEDULE_CFS ) {
		LQIO::input_error2( LQIO::WRN_NON_CFS_PROCESSOR, group_name, processor->getName().c_str() );
		return processor;
            } else if ( _createObjects ) {
                if ( group ) {
                    throw duplicate_symbol( group_name );
		} else {
		    group  = new Group(this, group_name,
				       dynamic_cast<Processor *>(processor),
				       db_build_parameter_variable( getStringAttribute( attributes, Xshare ), NULL ),
				       getBoolAttribute( attributes, Xcap),
				       0 );
		    addGroup(group);
		    dynamic_cast<Processor *>(processor)->addGroup(group);

		    io_vars->n_groups += 1;
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
            checkAttributes( attributes, task_table );
            const XML_Char * task_name = getStringAttribute(attributes,Xname);
            Task * task = getTaskByName( task_name );


            if ( _createObjects ) {
                const scheduling_type sched_type = static_cast<scheduling_type>(getEnumerationAttribute( attributes, Xscheduling, schedulingTypeXMLString, SCHEDULE_FIFO ));
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
                    task = new SemaphoreTask( this, task_name, entries,
                                              db_build_parameter_variable(getStringAttribute(attributes,Xqueue_length,"0"),NULL),
                                              processor,
                                              getLongAttribute(attributes,Xpriority,0),
                                              db_build_parameter_variable(getStringAttribute(attributes,Xmultiplicity,"1"),NULL),
                                              getLongAttribute(attributes,Xreplication,1),
                                              group,
                                              0 );
                    if ( strcasecmp( tokens, "0" ) == 0 ) {
                        dynamic_cast<SemaphoreTask *>(task)->setInitialState(SemaphoreTask::INITIALLY_EMPTY);
                    }
                    /* Otherwise, aliased to multiplicity */

                } else if ( sched_type == SCHEDULE_RWLOCK ){
                    task = new RWLockTask( this, task_name, entries,
                                           db_build_parameter_variable(getStringAttribute(attributes,Xqueue_length,"0"),NULL),
                                           processor,
                                           getLongAttribute(attributes,Xpriority,0),
                                           db_build_parameter_variable(getStringAttribute(attributes,Xmultiplicity,"1"),NULL),
                                           getLongAttribute(attributes,Xreplication,1),
                                           group,
                                           0 );

                    if ( strlen( tokens ) > 0 ) {
                        input_error( "Unexpected attribute <%s> ", Xinitially );
                    }
                } else {
                    task = new Task( this, task_name, sched_type, entries,
                                     db_build_parameter_variable(getStringAttribute(attributes,Xqueue_length,"0"),NULL),
                                     processor,
                                     getLongAttribute(attributes,Xpriority,0),
                                     db_build_parameter_variable(getStringAttribute(attributes,Xmultiplicity,"1"),NULL),
                                     getLongAttribute(attributes,Xreplication,1),
                                     group,
                                     0 );
                    if ( strlen( tokens ) > 0 ) {
                        input_error( "Unexpected attribute <%s> ", Xinitially );
                    }
                }

		char * end_ptr = 0;
                const XML_Char * think_time = getStringAttribute(attributes,Xthink_time,"");
		if ( strtod( think_time, &end_ptr ) != 0.0 || (end_ptr != 0 && *end_ptr != 0) ) {
                    if ( sched_type == SCHEDULE_CUSTOMER ) {
                        task->setThinkTime( db_build_parameter_variable(think_time,NULL));
		    } else {
                        LQIO::input_error2( LQIO::ERR_NON_REF_THINK_TIME, task_name );
                    }
                }

                /* Link in the entity information */
                addTaskEntity(task);
                processor->addTask(task);
                if ( group ) group->addTask(task);

                io_vars->n_tasks += 1;

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
            const XML_Char * source = 0;
            unsigned int value = 0;
            for ( ; *attributes; attributes += 2 ) {
                if ( strcasecmp( *attributes, Xsource ) == 0 && !source ) {
                    source = *(attributes+1);
                } else if ( strcasecmp( *attributes, Xvalue ) == 0 && !value ) {
                    value = atoi( *(attributes+1) );
                } else {
                    throw unexpected_attribute( *attributes );
                }
            }
            if ( !source ) {
                throw missing_attribute( Xsource );
            } else if ( !value ) {
                throw missing_attribute( Xvalue );
            } else {
                dynamic_cast<LQIO::DOM::Task *>(object)->setFanIn( source, value );
            }
        }

        /*
	  <xsd:attribute name="dest" type="xsd:string" use="required"/>
	  <xsd:attribute name="value" type="xsd:nonNegativeInteger" use="required"/>
        */

        void
        Expat_Document::handleFanOut( DocumentObject * object, const XML_Char ** attributes )
        {
            const XML_Char * destination = 0;
            unsigned int value = 0;
            for ( ; *attributes; attributes += 2 ) {
                if ( strcasecmp( *attributes, Xdest ) == 0 && !destination ) {
                    destination = *(attributes+1);
                } else if ( strcasecmp( *attributes, Xvalue ) == 0 && !value ) {
                    value = atoi( *(attributes+1) );
                } else {
                    throw unexpected_attribute( *attributes );
                }
            }
            if ( !destination ) {
                throw missing_attribute( Xdest );
            } else if ( !value ) {
                throw missing_attribute( Xvalue );
            } else {
                dynamic_cast<LQIO::DOM::Task *>(object)->setFanOut( destination, value );
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
            checkAttributes( attributes, entry_table );
            const XML_Char * entry_name = getStringAttribute(attributes,Xname);

            Entry * entry = getEntryByName( entry_name );
            if ( _createObjects ) {

                if ( !entry ) {
                    entry = new Entry(this, entry_name, 0);
                    addEntry(entry);            /* Add to global table */
                }

		const XML_Char * type = getStringAttribute(attributes,Xtype,"");
		if ( strcasecmp( type, XNONE ) == 0 ) {
		    entry->setEntryType( Entry::ENTRY_ACTIVITY_NOT_DEFINED );
		} else if ( strcasecmp( type, XPH1PH2 ) == 0 ) {
		    entry->setEntryType( Entry::ENTRY_STANDARD_NOT_DEFINED );
		}

                const XML_Char * priority = getStringAttribute(attributes,Xpriority,"");
                if ( strlen(priority) > 0 ) {
                    bool isSymbol = false;
                    entry->setEntryPriority(db_build_parameter_variable(priority, &isSymbol));
                }

                const XML_Char * open_arrivals = getStringAttribute(attributes,Xopen_arrival_rate,"");
                if ( strlen(open_arrivals) > 0 ) {
                    bool isSymbol = false;
                    entry->setOpenArrivalRate(db_build_parameter_variable(open_arrivals, &isSymbol));
                }

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

                Document::io_vars->n_entries += 1;

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
                throw domain_error( "phase" );
            } else {
                phase = dynamic_cast<Entry *>(entry)->getPhase(p);
                if (!phase) internal_error( __FILE__, __LINE__, "missing phase." );
            }

            if ( _createObjects ) {
                phase->setName( getStringAttribute(attributes,Xname) );
                db_check_set_entry(dynamic_cast<Entry *>(entry), entry->getName(), DOM::Entry::ENTRY_STANDARD);
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
                activity->setName( activity_name );
                activity->setIsSpecified(true);

                const XML_Char * first_entry = getStringAttribute(attributes,Xbound_to_entry,"");
                if ( strlen(first_entry) > 0 ) {
                    Entry* entry = getEntryByName(first_entry);
                    db_check_set_entry(entry, first_entry, Entry::ENTRY_ACTIVITY);
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
            checkAttributes( attributes, activity_table );
            if ( _createObjects ) {

                const XML_Char * demand = getStringAttribute(attributes,Xhost_demand_mean,"");
                if ( strlen(demand) > 0 ) {
                    phase->setServiceTime(db_build_parameter_variable(demand, NULL));
                } /* !!! Perhaps check demand instead !!! */
                const XML_Char * cvsq = getStringAttribute(attributes,Xhost_demand_cvsq,"");
                if ( strlen(cvsq) > 0 ) {
                    phase->setCoeffOfVariationSquared(db_build_parameter_variable(cvsq, NULL));
                }
                const XML_Char * think = getStringAttribute(attributes,Xthink_time,"");
                if ( strlen(think) > 0 ) {
                    phase->setThinkTime(db_build_parameter_variable(think, NULL));
                }
                const double max_service =  getDoubleAttribute(attributes,Xmax_service_time,0.0);
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
                    activity_list->add( activity, db_build_parameter_variable( getStringAttribute( attributes, Xprob ), NULL) );
                    activity->inputFrom( activity_list );
                    break;

                case ActivityList::FORK_ACTIVITY_LIST:
                case ActivityList::AND_FORK_ACTIVITY_LIST:
                    activity_list->add( activity );
                    activity->inputFrom( activity_list );
                    break;

                case ActivityList::REPEAT_ACTIVITY_LIST:
                    activity_list->add( activity, db_build_parameter_variable( getStringAttribute( attributes, Xcount ), NULL) );
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
            checkAttributes( attributes, call_table );
            unsigned from_phase = 0;

            const XML_Char * dest_entry_name = getStringAttribute(attributes,Xdest);
            const Entry * from_entry = dynamic_cast<Phase *>(phase)->getSourceEntry();
            Entry* to_entry = getEntryByName(dest_entry_name);

            if ( !to_entry ) {
                if ( _createObjects ) {
                    to_entry = new Entry(this, dest_entry_name, 0);         // This differs here.. entry may not exist, so create it anyway then test later. assert(to_entry != NULL);
                    addEntry(to_entry);         /* Add to global table */
                } else {
                    throw undefined_symbol( dest_entry_name );
                }
            }


            const std::map<unsigned, Phase*>& phases = from_entry->getPhaseList();
            for ( std::map<unsigned, Phase*>::const_iterator p = phases.begin(); from_phase == 0 && p != phases.end(); ++p ) {
                if ( p->second == phase ) from_phase = p->first;
            }
            if ( !from_phase ) internal_error( __FILE__, __LINE__, "missing from_phase" );

            Call * call = from_entry->getCallToTarget(to_entry, from_phase );

            if ( _createObjects ) {
                const XML_Char * calls = getStringAttribute(attributes,Xcalls_mean);

                /* Make sure that this is a standard entry */
                if ( !from_entry ) internal_error( __FILE__, __LINE__, "missing from entry" );
                db_check_set_entry(const_cast<Entry *>(from_entry), from_entry->getName(), Entry::ENTRY_STANDARD);
                db_check_set_entry(to_entry, dest_entry_name, Entry::ENTRY_NOT_DEFINED);

                /* Push all the times */

                bool isSymbol = false;
                ExternalVariable* ev_calls = db_build_parameter_variable(calls, &isSymbol);

                /* Check the existence */
                if (call == NULL) {
                    call = new Call(this, call_type, dynamic_cast<Phase *>(phase), to_entry, from_phase, ev_calls, 0 );
		    string name = phase->getName();
		    name += '_';
		    name += to_entry->getName();
		    call->setName(name);
                    const_cast<Entry *>(from_entry)->appendOriginatingCall(call);
                } else {
                    if (call->getCallType() != Call::NULL_CALL) {
                        input_error2( WRN_MULTIPLE_SPECIFICATION );
                    }

                    /* Set the new call type and the new mean */
                    call->setCallType(call_type);
                    call->setCallMean(ev_calls);
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
            checkAttributes( attributes, call_table );

            const XML_Char * dest_entry_name = getStringAttribute(attributes,Xdest);
            const XML_Char * calls = getStringAttribute(attributes,Xcalls_mean);

            /* Obtain the entry that we will be adding the phase times to */
            Entry* to_entry = getEntryByName(dest_entry_name);
            if ( !to_entry ) {
                if ( _createObjects ) {
                    to_entry = new Entry(this, dest_entry_name, 0);         // This differs here.. entry may not exist, so create it anyway then test later. assert(to_entry != NULL);
                    addEntry(to_entry);         /* Add to global table */
                } else {
                    throw undefined_symbol( dest_entry_name );
                }
            }

            Call * call = dynamic_cast<Activity *>(activity)->getCallToTarget( to_entry );
            if ( _createObjects ) {
                /* Push all the times */

                if ( !call ) {
                    bool isSymbol = false;
                    ExternalVariable* ev_calls = db_build_parameter_variable(calls, &isSymbol);

                    call = new Call(this, call_type, dynamic_cast<Activity *>(activity), to_entry, 0, ev_calls, 0 );
		    string name = activity->getName();
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
            const XML_Char * prob = getStringAttribute(attributes,Xprob);

            /* Obtain the entry that we will be adding the phase times to */
            Entry* to_entry = getEntryByName(dest_entry_name);
            if ( !to_entry ) {
                if ( _createObjects ) {
                    to_entry = new Entry(this, dest_entry_name, 0);         // This differs here.. entry may not exist, so create it anyway then test later. assert(to_entry != NULL);
                    addEntry(to_entry);         /* Add to global table */
                } else {
                    throw runtime_error( dest_entry_name );
                }
            }

            Entry * from_entry = dynamic_cast<Entry *>(entry);
            Call * call = from_entry->getForwardingToTarget(to_entry);
            if ( _createObjects ) {
                if ( call == NULL ) {
                    bool isSymbol = false;
                    call = new Call(this, from_entry, to_entry, db_build_parameter_variable(prob, &isSymbol), 0 );
		    string name = from_entry->getName();
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
            checkAttributes( attributes, histogram_table );
	    /* Handle entries specially */
	    unsigned int phase = getLongAttribute( attributes, Xphase, 0 );		/* Default, other it will throw up. */
	    if ( phase ) {
		if ( !dynamic_cast<Entry *>(object)) {
                    throw unexpected_attribute( Xphase );
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
            checkAttributes( attributes, histogram_table );
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
                    char * end_ptr = 0;
		    const double value = strtod( *(attributes+1), &end_ptr );
                    if ( value < 0 || ( end_ptr && *end_ptr != '\0' ) ) throw std::invalid_argument( *(attributes+1) );
                    if ( func && _loadResults && value > 0. ) {
                        (object->*func)( value );
                    }
                } else {
                    input_error( "Unexpected attribute <%s> ", *attributes );
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
                    char * end_ptr = 0;
		    const double value = strtod( *(attributes+1), &end_ptr );
                    if ( value < 0 || ( end_ptr && *end_ptr != '\0' ) ) throw std::invalid_argument( *(attributes+1) );
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
                join_list->setResultJoinDelay( invert(getDoubleAttribute(attributes,Xjoin_waiting,0.0)) );
                join_list->setResultVarianceJoinDelay( invert(getDoubleAttribute(attributes,Xjoin_variance,0.0)) );
            }
        }


        Histogram *
        Expat_Document::findOrAddHistogram( DocumentObject * object, Histogram::histogram_t type, unsigned int n_bins, double min, double max )
        {
            Histogram * histogram = 0;
            if ( _createObjects ) {
                if ( object->hasHistogram() ) throw duplicate_symbol( object->getName() );

                histogram = new Histogram(this, type, n_bins, min, max, 0 );
                object->setHistogram( histogram );
            } else {
                if ( !object->hasHistogram() ) {
                    throw runtime_error( object->getName() );
                } else {
                    histogram = const_cast<Histogram *>(object->getHistogram());
                }
            }
            return histogram;
        }


        Histogram *
        Expat_Document::findOrAddHistogram( DocumentObject * object, unsigned int phase, Histogram::histogram_t type, unsigned int n_bins, double min, double max )
        {
            Histogram * histogram = 0;
            if ( _createObjects ) {
                if ( object->hasHistogramForPhase( phase ) ) throw duplicate_symbol( object->getName() );

                histogram = new Histogram(this, type, n_bins, min, max, 0 );
                object->setHistogramForPhase( phase, histogram );
            } else {
                if ( !object->hasHistogramForPhase( phase ) ) {
                    throw runtime_error( object->getName() );
                } else {
                    histogram = const_cast<Histogram *>(object->getHistogramForPhase( phase ));
                }
            }
            return histogram;
	}

        bool
        Expat_Document::checkAttributes( const XML_Char ** attributes, std::set<const XML_Char *,Expat_Document::attribute_table_t>& table ) const
        {
            for ( ; *attributes; attributes += 2 ) {
                std::set<const XML_Char *>::const_iterator item = table.find(*attributes);
                if ( item == table.end() ) {
                    if ( strncasecmp( *attributes, "http:", 5 ) == 0 ) continue;                /* Skip these */
                    else throw unexpected_attribute( *attributes );
                }
            }
            return true;
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
                    const double value = strtod( *(attributes+1), &end_ptr );
                    if ( value < 0. || rint(value) != value || ( end_ptr && *end_ptr != '\0' ) ) throw std::invalid_argument( *(attributes+1) );
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

        const clock_t
        Expat_Document::getTimeAttribute( const XML_Char ** attributes, const XML_Char * attribute ) const
        {
            for ( ; *attributes; attributes += 2 ) {
                if ( strcasecmp( *attributes, attribute ) == 0 ) {
                    unsigned long hrs   = 0;
                    unsigned long mins  = 0;
                    unsigned long secs  = 0;

                    sscanf( *(attributes+1), "%ld:%ld:%ld", &hrs, &mins, &secs );
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
                    throw std::invalid_argument( *(attributes+1) );
                }
            }
            return default_value;
        }

        /* ---------------------------------------------------------------- */
        /* DOM serialization - write results to XERCES then save.           */
        /* ---------------------------------------------------------------- */

        bool
        Expat_Document::isXMLDOMPresent() const
        {
            return _XMLDOMPresent;
        }


        /* Export the DOM. */
        void
        Expat_Document::serializeDOM( const char * file_name, bool instantiate ) const
        {
            ofstream output;
            output.open( file_name, ios::out );
            if ( !output ) {
                cerr << io_vars->lq_toolname << ": Cannot open output file " << file_name << " - " << strerror( errno ) << endl;
            } else {
		serializeDOM( output, instantiate );
	    }
        }

        void
        Expat_Document::serializeDOM( ostream& output, bool instantiate ) const
        {
            __instantiate = instantiate;

            if ( getResultNumberOfBlocks() > 1 ) {
                const_cast<ConfidenceIntervals *>(&_conf_95)->set_blocks( getResultNumberOfBlocks() );
                const_cast<ConfidenceIntervals *>(&_conf_99)->set_blocks( getResultNumberOfBlocks() );
            }

            exportHeader( output );
            exportParameters( output );

	    /* Export in model input order */

            for ( std::map<unsigned, Entity *>::const_iterator entityIter = _entities.begin(); entityIter != _entities.end(); ++entityIter) {
		if ( dynamic_cast<Processor *>(entityIter->second) ) {
		    exportProcessor( output, *dynamic_cast<Processor *>(entityIter->second) );
		}
            }

            if ( !instantiate ) {
                exportLQX( output );            // If translating, do this, otherwise, don't
            }

            exportFooter( output );
	}

        void
        Expat_Document::exportHeader( ostream& output ) const
        {
            Filename base_name( input_file_name );

            output << "<?xml version=\"1.0\"?>" << endl;

	    if ( io_vars->lq_command_line && strlen( io_vars->lq_command_line ) > 0 ) {
		output << comment( io_vars->lq_command_line );
	    }
            output << start_element( Xlqn_model ) << attribute( Xname, base_name() )
                   << " description=\"" << io_vars->lq_toolname << " " << io_vars->lq_version << " solution for model from: " << input_file_name << ".\""
                   << " xmlns:xsi=\"" << XMLSchema_instance << "\" xsi:noNamespaceSchemaLocation=\"";

            const char * p = getenv( "LQN_SCHEMA_DIR" );
            string schema_path;
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
            output << "\">" << endl;
        }

        /*
          <result-general conv-val="0" elapsed-time="0.000000:0.000000:0.000000" iterations="2" platform-info="Gregs-MacBook.local Darwin 10.3.0" solver-info="Gregs-MacBook.local Darwin 10.3.0" system-cpu-time="0.000000:0.000000:0.000000" user-cpu-time="0.000000:0.000000:0.000000" valid="YES"/>
        */

        void
        Expat_Document::exportParameters( ostream& output ) const
        {
            const bool complex_element = hasResults() || hasPragmas();
            output << start_element( Xsolver_parameters, complex_element )
                   << attribute( Xcomment, getModelComment() )
                   << attribute( Xconv_val, getModelConvergenceValue() )
                   << attribute( Xit_limit, getModelIterationLimit() )
                   << attribute( Xunderrelax_coeff, getModelUnderrelaxationCoefficient() )
                   << attribute( Xprint_int, getModelPrintInterval() );
            if ( complex_element ) {
                output << ">" << endl;
                if ( hasResults() ) {
                    const bool has_mva_info = _mvaStatistics.submodels > 0;
                    output << start_element( Xresult_general, has_mva_info )
                           << attribute( Xvalid, getResultValid() ? "YES" : "NO" )
                           << attribute( Xconv_val_result, getResultConvergenceValue() )
                           << attribute( Xiterations, getResultIterations() )
                           << attribute( Xplatform_info, getResultPlatformInformation() )
                           << time_attribute( Xuser_cpu_time, getResultUserTime() )
                           << time_attribute( Xsystem_cpu_time, getResultSysTime() )
                           << time_attribute( Xelapsed_time, getResultElapsedTime() );
                    if ( has_mva_info ) {
                        output << ">" << endl;
                        output << simple_element( Xmva_info )
                               << attribute( Xsubmodels, _mvaStatistics.submodels )
                               << attribute( Xcore, static_cast<double>(_mvaStatistics.core) )
                               << attribute( Xstep, _mvaStatistics.step )
                               << attribute( Xstep_squared, _mvaStatistics.step_squared )
                               << attribute( Xwait, _mvaStatistics.wait )
                               << attribute( Xwait_squared, _mvaStatistics.wait_squared )
                               << attribute( Xfaults, _mvaStatistics.faults )
                               << "/>" << endl;
                    }
                    output << end_element( Xresult_general, has_mva_info ) << endl;
                }
                if ( hasPragmas() ) {
                    const std::map<std::string,std::string>& pragmas = getPragmaList();
                    for ( std::map<std::string,std::string>::const_iterator next_pragma = pragmas.begin(); next_pragma != pragmas.end(); ++next_pragma ) {
                        output << start_element( Xpragma, false )
                               << attribute( Xparam, next_pragma->first )
                               << attribute( Xvalue, next_pragma->second );
                        output << end_element( Xpragma, false ) << endl;
                    }
                }
            }
            output << end_element( Xsolver_parameters, complex_element ) << endl;
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
        Expat_Document::exportProcessor( ostream& output, const Processor & processor ) const
        {
            const std::vector<Task*>& task_list = processor.getTaskList();
            if ( !task_list.size() ) return;

            const scheduling_type scheduling = processor.getSchedulingType();
            output << start_element( Xprocessor )
                   << attribute( Xname, processor.getName() )
                   << attribute( Xscheduling, schedulingTypeXMLString[scheduling] );            // see lqio/labels.c

            if ( processor.isMultiserver() ) {
                output << attribute( Xmultiplicity, *processor.getCopies() );
            }
            if ( processor.hasRate() ) {
                output << attribute( Xspeed_factor, *processor.getRate() );
            }
            if ( processor.getReplicas() > 1 ) {
                output << attribute( Xreplication, processor.getReplicas() );
            }
            if ( (scheduling == SCHEDULE_PS
                  || scheduling == SCHEDULE_PS_HOL
                  || scheduling == SCHEDULE_PS_PPR
                  || scheduling == SCHEDULE_CFS)
                 && processor.hasQuantum() ) {
                output << attribute( Xquantum, *processor.getQuantum() );
            }
            output << ">" << endl;

            if ( hasResults() ) {
                if ( getResultNumberOfBlocks() > 1 ) {
                    output << start_element( Xresult_processor ) << attribute( Xutilization, processor.getResultUtilization() ) << ">" << endl;
                    output << simple_element( Xresult_conf_95 )  << attribute( Xutilization, _conf_95( processor.getResultUtilizationVariance() ) ) << "/>" << endl;
                    output << simple_element( Xresult_conf_99 )  << attribute( Xutilization, _conf_99( processor.getResultUtilizationVariance() ) ) << "/>" << endl;
                    output << end_element( Xresult_processor ) << endl;
                } else {
                    output << simple_element( Xresult_processor ) << attribute( Xutilization, processor.getResultUtilization() ) << "/>" << endl;
                }
            }

            const std::vector<Group*>& group_list = processor.getGroupList();
            std::vector<Group*>::const_iterator next_group;
            for (next_group = group_list.begin(); next_group != group_list.end(); ++next_group) {
                exportGroup( output, **next_group );
            }

            for ( std::vector<Task*>::const_iterator  next_task = task_list.begin(); next_task != task_list.end(); ++next_task) {
                if ( !(*next_task)->getGroup() ) {
                    exportTask( output, **next_task );
                }
            }

            output << end_element( Xprocessor ) << endl;
        }


        void
        Expat_Document::exportGroup( ostream& output, const Group & group ) const
        {
            output << start_element( Xgroup ) << attribute( Xname, group.getName() )
                   << attribute( Xshare, *group.getGroupShare() )
                   << attribute( Xcap, group.getCap() )
                   << ">" << endl;

            if ( hasResults() ) {
                if ( getResultNumberOfBlocks() > 1 ) {
                    output << start_element( Xresult_group ) << attribute( Xutilization, group.getResultUtilization() ) << ">" << endl;
                    output << simple_element( Xresult_conf_95 )  << attribute( Xutilization, _conf_95( group.getResultUtilizationVariance() ) ) << "/>" << endl;
                    output << simple_element( Xresult_conf_99 )  << attribute( Xutilization, _conf_99( group.getResultUtilizationVariance() ) ) << "/>" << endl;
                    output << end_element( Xresult_group ) << endl;
                } else {
                    output << simple_element( Xresult_group ) << attribute( Xutilization, group.getResultUtilization() ) << "/>" << endl;
                }
            }


            const std::vector<Task*>& task_list = group.getTaskList();
            for ( std::vector<Task*>::const_iterator next_task = task_list.begin(); next_task != task_list.end(); ++next_task ) {
                exportTask( output, **next_task );
            }

            output << end_element( Xgroup ) << endl;
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
        Expat_Document::exportTask( ostream& output, const Task & task ) const
        {

            const scheduling_type scheduling = task.getSchedulingType();
            output << start_element( Xtask ) << attribute( Xname, task.getName() )
                   << attribute( Xscheduling, schedulingTypeXMLString[scheduling] );            // see lqio/labels.c

            if ( task.isMultiserver() ) {
                output << attribute( Xmultiplicity, *task.getCopies() );
            }
            if ( task.getSchedulingType() == SCHEDULE_CUSTOMER && task.hasThinkTime() ) {
                output << attribute( Xthink_time, *task.getThinkTime() );
            }
            if ( task.getPriority() ) {
                output << attribute( Xpriority, task.getPriority() );
            }
            if ( task.hasQueueLength() ) {
                output << attribute( Xqueue_length, *task.getQueueLength() );
            }
            if ( task.getReplicas() > 1 ) {
                output << attribute( Xreplication, task.getReplicas() );
            }
            if ( task.getSchedulingType() == SCHEDULE_SEMAPHORE && dynamic_cast<const SemaphoreTask&>(task).getInitialState() == SemaphoreTask::INITIALLY_EMPTY ) {
                output << attribute( Xinitially, 0.0 );
            }

            output << ">" << endl;

            if ( hasResults() ) {
                if ( task.hasHistogram() ) {
                    exportHistogram( output, *task.getHistogram() );
                }

                const bool has_confidence = getResultNumberOfBlocks() > 1;
                output << start_element( Xresult_task, has_confidence )
                       << attribute( Xthroughput, task.getResultThroughput() )
                       << attribute( Xutilization, task.getResultUtilization() )
                       << task_phase_results( task, XphaseP_utilization, &Task::getResultPhasePUtilization )
                       << attribute( Xproc_utilization, task.getResultProcessorUtilization() );

                if ( dynamic_cast<const SemaphoreTask *>(&task) ) {
                    output << attribute( Xsemaphore_waiting, task.getResultHoldingTime() )
                           << attribute( Xsemaphore_waiting_variance, task.getResultVarianceHoldingTime() )
                           << attribute( Xsemaphore_utilization, task.getResultHoldingUtilization() );
                }

                if ( dynamic_cast<const RWLockTask *>(&task) ) {

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
                    output << ">" << endl;
                    output << simple_element( Xresult_conf_95 )
                           << attribute( Xthroughput, _conf_95( task.getResultThroughputVariance() ) )
                           << attribute( Xutilization, _conf_95( task.getResultUtilizationVariance() ) )
                           << task_phase_results( task, XphaseP_utilization, &Task::getResultPhasePUtilizationVariance, &_conf_95 )
                           << attribute( Xproc_utilization, _conf_95(task.getResultProcessorUtilizationVariance() ) );
                    if ( dynamic_cast<const SemaphoreTask *>(&task) ) {
                        output << attribute( Xsemaphore_waiting, _conf_95(task.getResultHoldingTimeVariance()) )
                               << attribute( Xsemaphore_waiting_variance, _conf_95(task.getResultVarianceHoldingTimeVariance()) )
                               << attribute( Xsemaphore_utilization, _conf_95(task.getResultHoldingUtilizationVariance()) );
                    }
                    if ( dynamic_cast<const RWLockTask *>(&task) ) {
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

                    output << "/>" << endl;

                    output << simple_element( Xresult_conf_99 )
                           << attribute( Xthroughput, _conf_99( task.getResultThroughputVariance() ) )
                           << attribute( Xutilization, _conf_99( task.getResultUtilization() ) )
                           << task_phase_results( task, XphaseP_utilization, &Task::getResultPhasePUtilizationVariance, &_conf_99 )
                           << attribute( Xproc_utilization, _conf_99(task.getResultProcessorUtilizationVariance() ) );
                    if ( dynamic_cast<const SemaphoreTask *>(&task) ) {
                        output << attribute( Xsemaphore_waiting, _conf_99(task.getResultHoldingTimeVariance()) )
                               << attribute( Xsemaphore_waiting_variance, _conf_99(task.getResultVarianceHoldingTimeVariance()) )
                               << attribute( Xsemaphore_utilization, _conf_99(task.getResultHoldingUtilizationVariance()) );
                    }
                    if ( dynamic_cast<const RWLockTask *>(&task) ) {
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

                    output << "/>" << endl;
                }

                output << end_element( Xresult_task, has_confidence ) << endl;
            }


            for ( std::map<const std::string, unsigned int>::const_iterator next_fanin = task.getFanIns().begin(); next_fanin != task.getFanIns().end(); ++next_fanin ) {
                const std::string& src = next_fanin->first;
                const unsigned int value = next_fanin->second;
                output << simple_element( Xfanin ) << attribute( Xsource, src )
                       << attribute( Xvalue, value )
                       << "/>" << endl;
            }

            for ( std::map<const std::string, unsigned int>::const_iterator next_fanout = task.getFanOuts().begin(); next_fanout != task.getFanOuts().end(); ++next_fanout ) {
                const std::string dst = next_fanout->first;
                const unsigned int value = next_fanout->second;
                output << simple_element( Xfanout ) << attribute( Xdest, dst )
                       << attribute( Xvalue, value )
                       << "/>" << endl;
            }

            const std::vector<Entry *>& entries = task.getEntryList();
            for ( std::vector<DOM::Entry *>::const_iterator next_entry = entries.begin(); next_entry != entries.end(); ++next_entry ) {
                exportEntry( output, **next_entry );
            }

            const std::map<std::string,DOM::Activity*>& activities = task.getActivities();
            if ( activities.size() > 0 ) {

                /* The activities */

                output << start_element( Xtask_activities ) << ">" << endl;
                for ( std::map<std::string,DOM::Activity*>::const_iterator next_activity = activities.begin(); next_activity != activities.end(); ++next_activity ) {
                    exportActivity( output, *(next_activity->second), 0 );
                }

                /* Precedence connections */

                const std::set<ActivityList*>& precedences = task.getActivityLists();
                for ( std::set<ActivityList*>::const_iterator next_precedence = precedences.begin(); next_precedence != precedences.end(); ++next_precedence ) {
                    /* look for the 'pre' side.  Do the post side based on the pre-side */
                    const ActivityList * activity_list = *next_precedence;
                    switch ( activity_list->getListType() ) {
                    case ActivityList::JOIN_ACTIVITY_LIST:
                    case ActivityList::AND_JOIN_ACTIVITY_LIST:
                    case ActivityList::OR_JOIN_ACTIVITY_LIST:
                        output << start_element( Xprecedence ) << ">" << endl;
                        exportPrecedence( output, *activity_list );
                        if ( activity_list->getNext() ) {
                            exportPrecedence( output, *(activity_list->getNext()) );
                        }
                        output << end_element( Xprecedence ) << endl;
                        break;
		    default:
			break;
                    }
                }

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
                    output << start_element( Xreply_entry ) << attribute( Xname, entry->getName() ) << ">" << endl;
                    const std::vector<const Activity *>& activity_list = next_entry->second;
                    for ( std::vector<const Activity *>::const_iterator next_activity = activity_list.begin(); next_activity != activity_list.end(); ++next_activity ) {
                        output << simple_element( Xreply_activity ) << attribute( Xname, (*next_activity)->getName() ) << "/>" << endl;
                    }
                    output << end_element( Xreply_entry ) << endl;
                }

                output << end_element( Xtask_activities ) << endl;
            }

            output << end_element( Xtask ) << endl;
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
        Expat_Document::exportEntry( ostream& output, const Entry& entry ) const
        {
            const bool complex_element = entry.getStartActivity() == 0  /* Phase1/2 type entry */
                || entry.getForwarding().size() > 0
		|| (entry.getStartActivity() && entry.hasHistogram())           /* Activity entry with a histogram */
                || hasResults();
            output << start_element( Xentry, complex_element )
                   << attribute( Xname, entry.getName() );

            if ( entry.getMaximumPhase() > 0 ) {
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
                output << ">" << endl;

                if ( hasResults() ) {
		    if ( entry.getStartActivity() && entry.hasHistogram() ) {
			for ( unsigned p = 1; p <= Phase::MAX_PHASE; ++p ) {
			    if ( entry.hasHistogramForPhase(p) ) {
				exportHistogram( output, *entry.getHistogramForPhase(p), p );
			    }
			}
		    }

                    const bool has_confidence = getResultNumberOfBlocks() > 1;

                    output << start_element( Xresult_entry, has_confidence )
                           << attribute( Xutilization, entry.getResultUtilization() )
                           << attribute( Xthroughput, entry.getResultThroughput() )
                           << attribute( Xsquared_coeff_variation, entry.getResultSquaredCoeffVariation() )
                           << attribute( Xproc_utilization, entry.getResultProcessorUtilization() );
		    if ( entry.hasResultsForThroughputBound() ) {
			output << attribute( Xthroughput_bound, entry.getResultThroughputBound() );
		    }
                    if ( entry.hasResultsForOpenWait() ) {
                        output << attribute( Xopen_wait_time, entry.getResultOpenWaitTime() );
                    }

                    /* Results for activity entries. */
                    if ( entry.getStartActivity() != 0 ) {
                        output << entry_phase_results( entry, XphaseP_service_time, &Entry::getResultPhasePServiceTime )
                               << entry_phase_results( entry, XphaseP_service_time_variance, &Entry::getResultPhasePVarianceServiceTime )
                               << entry_phase_results( entry, XphaseP_proc_waiting, &Entry::getResultPhasePProcessorWaiting )
                               << entry_phase_results( entry, XphaseP_utilization, &Entry::getResultPhasePUtilization );
                    }

                    if ( has_confidence ) {
                        output << ">" << endl;

                        output << simple_element( Xresult_conf_95 )
                               << attribute( Xutilization, _conf_95( entry.getResultUtilizationVariance() ) )
                               << attribute( Xthroughput, _conf_95( entry.getResultThroughputVariance() ) )
                               << attribute( Xsquared_coeff_variation, _conf_95( entry.getResultSquaredCoeffVariationVariance() ) )
                               << attribute( Xproc_utilization, _conf_95( entry.getResultProcessorUtilizationVariance() ) );
                        if ( entry.hasResultsForOpenWait() ) {
                            output << attribute( Xopen_wait_time, _conf_95( entry.getResultOpenWaitTimeVariance() ) );
                        }

                        /* Results for activity entries. */
                        if ( entry.getMaximumPhase() == 0 ) {
                            output << entry_phase_results( entry, XphaseP_service_time, &Entry::getResultPhasePServiceTimeVariance, &_conf_95 )
                                   << entry_phase_results( entry, XphaseP_service_time_variance, &Entry::getResultPhasePVarianceServiceTimeVariance, &_conf_95 )
                                   << entry_phase_results( entry, XphaseP_proc_waiting, &Entry::getResultPhasePProcessorWaitingVariance, &_conf_95 )
                                   << entry_phase_results( entry, XphaseP_utilization, &Entry::getResultPhasePUtilizationVariance, &_conf_95 );
                        }
                        output << "/>" << endl;

                        output << simple_element( Xresult_conf_99 )
                               << attribute( Xutilization, _conf_99( entry.getResultUtilizationVariance() ) )
                               << attribute( Xthroughput, _conf_99( entry.getResultThroughputVariance() ) )
                               << attribute( Xsquared_coeff_variation, _conf_99( entry.getResultSquaredCoeffVariationVariance() ) )
                               << attribute( Xproc_utilization, _conf_99( entry.getResultProcessorUtilizationVariance() ) );
                        if ( entry.hasResultsForOpenWait() ) {
                            output << attribute( Xopen_wait_time, _conf_99( entry.getResultOpenWaitTimeVariance() ) );
                        }

                        /* Results for activity entries. */
                        if ( entry.getMaximumPhase() == 0 ) {
                            output << entry_phase_results( entry, XphaseP_service_time, &Entry::getResultPhasePServiceTimeVariance, &_conf_99 )
                                   << entry_phase_results( entry, XphaseP_service_time_variance, &Entry::getResultPhasePVarianceServiceTimeVariance, &_conf_99 )
                                   << entry_phase_results( entry, XphaseP_proc_waiting, &Entry::getResultPhasePProcessorWaitingVariance, &_conf_99 )
                                   << entry_phase_results( entry, XphaseP_utilization, &Entry::getResultPhasePUtilizationVariance, &_conf_99 );
                        }
                        output << "/>" << endl;
                    }

                    output << end_element( Xresult_entry, has_confidence ) << endl;
                }

                const std::vector<Call*>& forwarding = entry.getForwarding();
                for ( std::vector<Call*>::const_iterator next_call = forwarding.begin(); next_call != forwarding.end(); ++next_call ) {
                    exportCall( output, **next_call );
                }

                if ( entry.getStartActivity() == NULL && entry.getMaximumPhase() > 0 ) {
                    output << start_element( Xentry_phase_activities ) << ">" << endl;

                    const std::map<unsigned, Phase*>& phases = entry.getPhaseList();
                    for ( std::map<unsigned, Phase*>::const_iterator next_phase = phases.begin(); next_phase != phases.end(); ++next_phase ) {
                        const Phase * phase = next_phase->second;
                        if ( phase->isNotNull() ) {
                            exportActivity( output, *phase, next_phase->first );
                        }
                    }
                    output << end_element( Xentry_phase_activities ) << endl;
                }
            }   /* Complex element */

            output << end_element( Xentry, complex_element ) << endl;
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
            const bool complex_element = calls.size() > 0 || hasResults() || phase.hasHistogram();
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
                output << attribute( Xmax_service_time, phase.getHistogram()->getMax() );
            }
            if ( phase.hasDeterministicCalls() ) {
                output << attribute( Xcall_order, XDETERMINISTIC );
            }

            if ( complex_element ) {
                output << ">" << endl;
                if ( hasResults() ) {
		    const bool has_variance = phase.getResultVarianceServiceTime() > 0.0;
                    if ( phase.hasHistogram() ) {
                        exportHistogram( output, *phase.getHistogram() );
                    }

                    const bool has_confidence = getResultNumberOfBlocks() > 1;

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
                        output << ">" << endl;
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
                        output << "/>" << endl;

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
                        output << "/>" << endl;
                    }

                    output << end_element( Xresult_activity, has_confidence ) << endl;
                }

                for (std::vector<Call*>::const_iterator next_call = calls.begin(); next_call != calls.end(); ++next_call) {
                    exportCall( output, **next_call );
                }
            }

            output << end_element( Xactivity, complex_element ) << endl;
        }



        const XML_Char * Expat_Document::precedence_type_table[ActivityList::REPEAT_ACTIVITY_LIST+1];

        void
        Expat_Document::exportPrecedence( ostream& output, const ActivityList& activity_list ) const
        {
            output << start_element( precedence_type_table[activity_list.getListType()] );
            const AndJoinActivityList * join_list = dynamic_cast<const AndJoinActivityList *>(&activity_list);
            if ( join_list && hasResults() ) {
                output  << ">" << endl;
                if ( join_list->hasHistogram() ) {
                    exportHistogram( output, *join_list->getHistogram() );
                }
                bool has_confidence = getResultNumberOfBlocks() > 1;

                output << start_element( Xresult_join_delay, has_confidence )
                       << attribute( Xjoin_waiting, join_list->getResultJoinDelay() )
                       << attribute( Xjoin_variance, join_list->getResultVarianceJoinDelay() );
                if ( has_confidence ) {
                    output << ">" << endl;
                    output << simple_element( Xresult_conf_95 )
                           << attribute( Xjoin_waiting, _conf_95( join_list->getResultJoinDelayVariance() ) )
                           << attribute( Xjoin_variance, _conf_95( join_list->getResultVarianceJoinDelayVariance() ) )
                           << "/>" << endl;
                    output << simple_element( Xresult_conf_99 )
                           << attribute( Xjoin_waiting, _conf_99( join_list->getResultJoinDelayVariance() ) )
                           << attribute( Xjoin_variance, _conf_99( join_list->getResultVarianceJoinDelayVariance() ) )
                           << "/>" << endl;
                }
                output << end_element( Xresult_join_delay, has_confidence ) << endl;
            } else if ( activity_list.getListType() == ActivityList::REPEAT_ACTIVITY_LIST ) {
                const std::vector<const Activity*>& list = activity_list.getList();
                for ( std::vector<const Activity*>::const_iterator next_activity = list.begin(); next_activity != list.end(); ++next_activity ) {
                    const Activity * activity = *next_activity;
                    if ( activity_list.getParameter( activity ) == NULL ) {
                        output << attribute( Xend, activity->getName() );
                    }
                }
                output  << ">" << endl;
            } else {
                output  << ">" << endl;
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
                output << "/>" << endl;
            }
            output << end_element( precedence_type_table[activity_list.getListType()] ) << endl;
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
        Expat_Document::exportCall( ostream& output, const Call & call ) const
        {
            const bool complex_type = hasResults() || call.hasHistogram();
            output << start_element( call_type_table[call.getCallType()].element, complex_type )
                   << attribute( Xdest, call.getDestinationEntry()->getName() );
            if ( call.getCallMean() ) {
                output << attribute( call_type_table[call.getCallType()].attribute, *call.getCallMean() );
            }

            if ( complex_type ) {
                output << ">" << endl;

		if ( hasResults() ) {
		    const bool has_confidence = getResultNumberOfBlocks() > 1;
		    output << start_element( Xresult_call, has_confidence )
			   << attribute( Xwaiting, call.getResultWaitingTime() );
		    if ( call.hasResultVarianceWaitingTime() ) {
			output << attribute( Xwaiting_variance, call.getResultVarianceWaitingTime() );
		    }
		    if ( call.hasResultDropProbability() ) {
			output << attribute( Xloss_probability, call.getResultDropProbability() );
		    }

		    if ( has_confidence ) {
			output << ">" << endl;
			output << simple_element( Xresult_conf_95 )
			       << attribute( Xwaiting, _conf_95( call.getResultWaitingTimeVariance() ) );
			if ( call.hasResultVarianceWaitingTime() ) {
			    output << attribute( Xwaiting_variance, _conf_95( call.getResultVarianceWaitingTimeVariance() ) );
			}
			if ( call.hasResultDropProbability() ) {
			    output << attribute( Xloss_probability, _conf_95( call.getResultDropProbabilityVariance() ) );
			}
			output << "/>" << endl;
			output << simple_element( Xresult_conf_99 )
			       << attribute( Xwaiting, _conf_99( call.getResultWaitingTimeVariance() ) );
			if ( call.hasResultVarianceWaitingTime() ) {
			    output << attribute( Xwaiting_variance, _conf_99( call.getResultVarianceWaitingTimeVariance() ) );
			}
			if ( call.hasResultDropProbability() ) {
			    output << attribute( Xloss_probability, _conf_99( call.getResultDropProbabilityVariance() ) );
			}
			output << "/>" << endl;
		    }
		    output << end_element( Xresult_call, has_confidence ) << endl;
		}

		if ( call.hasHistogram() ) {
                    exportHistogram( output, *call.getHistogram() );
		}
            }

            output << end_element( call_type_table[call.getCallType()].element, complex_type ) << endl;
        }



        void
        Expat_Document::exportHistogram( ostream& output, const Histogram& histogram, const unsigned phase ) const
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
		output << ">" << endl;
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
		    if ( variance > 0 && getResultNumberOfBlocks() > 1 ) {
			output << attribute( Xconf_95, _conf_95( variance ) )
			       << attribute( Xconf_99, _conf_99( variance ) );
		    }
		    output << end_element( bin_name, false ) << endl;
		}
	    }

            output << end_element( element_name, complex_type ) << endl;
        }


        void
        Expat_Document::exportLQX( ostream& output ) const
        {
            const std::string& program = getLQXProgramText();
            if ( program.size() ) {
                output << start_element( Xlqx ) << "><![CDATA[" << endl;
                output << program;
                output << "]]>" << endl << end_element( Xlqx ) << endl;
            }
        }


        void
        Expat_Document::exportFooter( ostream& output ) const
        {
            output << end_element( Xlqn_model ) << endl;
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

        /* static */ ostream&
        Expat_Document::printEntryPhaseResults( ostream& output, const Entry & entry, const XML_Char ** attributes, const doubleEntryFunc func, const ConfidenceIntervals * conf )
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

        /* static */ ostream&
        Expat_Document::printTaskPhaseResults( ostream& output, const Task & task, const XML_Char ** attributes, const doubleTaskFunc func, const ConfidenceIntervals * conf )
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
	    
	    escape_table['&']  = "&amp;";
	    escape_table['\''] = "&apos;";
	    escape_table['>']  = "&gt;";
	    escape_table['<']  = "&lt;";
	    escape_table['"']  = "&qout";

            model_table.insert("description");
            model_table.insert("lqncore-schema-version");
            model_table.insert("lqn-schema-version");
            model_table.insert(Xname);
            model_table.insert(Xxml_debug);

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

            group_table.insert(Xname);
            group_table.insert(Xcap);
            group_table.insert(Xshare);

            task_table.insert(Xname);
            task_table.insert(Xscheduling);
            task_table.insert(Xinitially);
            task_table.insert(Xqueue_length);
            task_table.insert(Xpriority);
            task_table.insert(Xthink_time);
            task_table.insert(Xmultiplicity);
            task_table.insert(Xreplication);
            task_table.insert(Xactivity_graph);                 // ignored.

            entry_table.insert(Xname);
            entry_table.insert(Xtype);
            entry_table.insert(Xpriority);
            entry_table.insert(Xopen_arrival_rate);
            entry_table.insert(Xsemaphore);
            entry_table.insert(Xrwlock);

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

            histogram_table.insert(Xmin);
            histogram_table.insert(Xmax);
//          histogram_table.insert(Xbin_size);
            histogram_table.insert(Xnumber_bins);
            histogram_table.insert(Xphase);
//          histogram_table.insert(Xmean);
//          histogram_table.insert(Xstd_dev);
//          histogram_table.insert(Xskew);
//          histogram_table.insert(Xkurtosis);

            result_table[Xjoin_variance] =                  result_table_t( &DocumentObject::setResultVarianceJoinDelay,         &DocumentObject::setResultVarianceJoinDelayVariance );
            result_table[Xjoin_waiting] =                   result_table_t( &DocumentObject::setResultJoinDelay,                 &DocumentObject::setResultJoinDelayVariance );
            result_table[Xloss_probability] =               result_table_t( &DocumentObject::setResultDropProbability,           &DocumentObject::setResultDropProbabilityVariance );
            result_table[Xopen_wait_time] =                 result_table_t( &DocumentObject::setResultOpenWaitTime,              &DocumentObject::setResultOpenWaitTimeVariance );
            result_table[XphaseP_proc_waiting[0]] =         result_table_t( &DocumentObject::setResultPhase1ProcessorWaiting,    &DocumentObject::setResultPhase1ProcessorWaitingVariance );
            result_table[XphaseP_service_time[0]] =         result_table_t( &DocumentObject::setResultPhase1ServiceTime,         &DocumentObject::setResultPhase1ServiceTimeVariance );
            result_table[XphaseP_service_time_variance[0]]= result_table_t( &DocumentObject::setResultPhase1VarianceServiceTime, &DocumentObject::setResultPhase1VarianceServiceTimeVariance );
            result_table[XphaseP_utilization[0]] =          result_table_t( &DocumentObject::setResultPhase1Utilization,         &DocumentObject::setResultPhase1UtilizationVariance );
            result_table[XphaseP_proc_waiting[1]] =         result_table_t( &DocumentObject::setResultPhase2ProcessorWaiting,    &DocumentObject::setResultPhase2ProcessorWaitingVariance );
            result_table[XphaseP_service_time[1]] =         result_table_t( &DocumentObject::setResultPhase2ServiceTime,         &DocumentObject::setResultPhase2ServiceTimeVariance );
            result_table[XphaseP_service_time_variance[1]]= result_table_t( &DocumentObject::setResultPhase2VarianceServiceTime, &DocumentObject::setResultPhase2VarianceServiceTimeVariance );
            result_table[XphaseP_utilization[1]] =          result_table_t( &DocumentObject::setResultPhase2Utilization,         &DocumentObject::setResultPhase2UtilizationVariance );
            result_table[XphaseP_proc_waiting[2]] =         result_table_t( &DocumentObject::setResultPhase3ProcessorWaiting,    &DocumentObject::setResultPhase3ProcessorWaitingVariance );
            result_table[XphaseP_service_time[2]] =         result_table_t( &DocumentObject::setResultPhase3ServiceTime,         &DocumentObject::setResultPhase3ServiceTimeVariance );
            result_table[XphaseP_service_time_variance[2]]= result_table_t( &DocumentObject::setResultPhase3VarianceServiceTime, &DocumentObject::setResultPhase3VarianceServiceTimeVariance );
            result_table[XphaseP_utilization[2]] =          result_table_t( &DocumentObject::setResultPhase3Utilization,         &DocumentObject::setResultPhase3UtilizationVariance );
            result_table[Xproc_utilization] =               result_table_t( &DocumentObject::setResultProcessorUtilization,      &DocumentObject::setResultProcessorUtilizationVariance );
            result_table[Xproc_waiting] =                   result_table_t( &DocumentObject::setResultProcessorWaiting,          &DocumentObject::setResultProcessorWaitingVariance );
            result_table[Xprob_exceed_max_service_time] =   result_table_t( 0, 0 );
            result_table[Xsemaphore_waiting] =              result_table_t( &DocumentObject::setResultHoldingTime,               &DocumentObject::setResultHoldingTimeVariance );
            result_table[Xsemaphore_waiting_variance] =     result_table_t( &DocumentObject::setResultVarianceHoldingTime,       &DocumentObject::setResultVarianceHoldingTimeVariance );
            result_table[Xsemaphore_utilization] =          result_table_t( &DocumentObject::setResultHoldingUtilization,        &DocumentObject::setResultHoldingUtilizationVariance );

            result_table[Xrwlock_reader_waiting] =          result_table_t( &DocumentObject::setResultReaderBlockedTime,         &DocumentObject::setResultReaderBlockedTimeVariance );
            result_table[Xrwlock_reader_waiting_variance] = result_table_t( &DocumentObject::setResultVarianceReaderBlockedTime, &DocumentObject::setResultVarianceReaderBlockedTimeVariance );
            result_table[Xrwlock_reader_holding] =          result_table_t( &DocumentObject::setResultReaderHoldingTime,         &DocumentObject::setResultReaderHoldingTimeVariance );
            result_table[Xrwlock_reader_holding_variance] = result_table_t( &DocumentObject::setResultVarianceReaderHoldingTime, &DocumentObject::setResultVarianceReaderHoldingTimeVariance );
            result_table[Xrwlock_reader_utilization] =      result_table_t( &DocumentObject::setResultReaderHoldingUtilization,  &DocumentObject::setResultReaderHoldingUtilizationVariance );
            result_table[Xrwlock_writer_waiting] =          result_table_t( &DocumentObject::setResultWriterBlockedTime,         &DocumentObject::setResultWriterBlockedTimeVariance );
            result_table[Xrwlock_writer_waiting_variance] = result_table_t( &DocumentObject::setResultVarianceWriterBlockedTime, &DocumentObject::setResultVarianceWriterBlockedTimeVariance );
            result_table[Xrwlock_writer_holding] =          result_table_t( &DocumentObject::setResultWriterHoldingTime,         &DocumentObject::setResultWriterHoldingTimeVariance );
            result_table[Xrwlock_writer_holding_variance] = result_table_t( &DocumentObject::setResultVarianceWriterHoldingTime, &DocumentObject::setResultVarianceWriterHoldingTimeVariance );
            result_table[Xrwlock_writer_utilization] =      result_table_t( &DocumentObject::setResultWriterHoldingUtilization,  &DocumentObject::setResultWriterHoldingUtilizationVariance );

            result_table[Xservice_time] =                   result_table_t( &DocumentObject::setResultServiceTime,               &DocumentObject::setResultServiceTimeVariance );
            result_table[Xservice_time_variance] =          result_table_t( &DocumentObject::setResultVarianceServiceTime,       &DocumentObject::setResultVarianceServiceTimeVariance );
            result_table[Xsquared_coeff_variation] =        result_table_t( &DocumentObject::setResultSquaredCoeffVariation,     &DocumentObject::setResultSquaredCoeffVariationVariance );
            result_table[Xthroughput] =                     result_table_t( &DocumentObject::setResultThroughput,                &DocumentObject::setResultThroughputVariance );
            result_table[Xthroughput_bound] =               result_table_t( &DocumentObject::setResultThroughputBound,           0 );
            result_table[Xutilization] =                    result_table_t( &DocumentObject::setResultUtilization,               &DocumentObject::setResultUtilizationVariance );
            result_table[Xwaiting] =                        result_table_t( &DocumentObject::setResultWaitingTime,               &DocumentObject::setResultWaitingTimeVariance );
            result_table[Xwaiting_variance] =               result_table_t( &DocumentObject::setResultVarianceWaitingTime,       &DocumentObject::setResultVarianceWaitingTimeVariance );

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
        };

        ostream&
        Expat_Document::printIndent( ostream& output, const int i )
        {
            if ( i < 0 ) {
                if ( __indent + i < 0 ) {
                    __indent = 0;
                } else {
                    __indent += i;
                }
            }
            if ( __indent != 0 ) {
                output << setw( __indent * 3 ) << " ";
            }
            if ( i > 0 ) {
                __indent += i;
            }
            return output;
        }


        ostream&
        Expat_Document::printStartElement( ostream& output, const XML_Char * element, const bool complex_element )
        {
            output << indent( complex_element ? 1 : 0  ) << "<" << element;
            return output;
        }

        ostream&
        Expat_Document::printEndElement( ostream& output, const XML_Char * element, const bool complex_element )
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

	ostream&
        Expat_Document::printAttribute( ostream& output, const XML_Char * attribute, const XML_Char * value )
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

        ostream&
        Expat_Document::printAttribute( ostream& output, const XML_Char * attribute, const double value )
        {
            output << " " << attribute << "=\"" <<  value << "\"";
            return output;
        }

        ostream&
        Expat_Document::printAttribute( ostream& output, const XML_Char * attribute, const ExternalVariable& value )
        {
            output << " " << attribute << "=\"";
            if ( __instantiate ) {
                output << to_double( value );
            } else {
                output << value;
            }
            output << "\"";
            return output;
        }

	ostream& 
	Expat_Document::printComment( std::ostream& output, const string& s )
	{
	    output << "<!-- ";
	    for ( const char * p = io_vars->lq_command_line; *p; ++p ) {	/* Handle comments */
		if ( *p != '-' || *(p+1) != '-' ) {
		    output << *p;						/* Strip '--' strings */
		}
	    }
	    output << " -->" << endl;
	    return output;
	}

        ostream&
        Expat_Document::printTime( ostream& output, const XML_Char * attribute, const clock_t time )
        {
#if defined(HAVE_SYS_TIME_H)
#if defined(CLK_TCK)
	    const double dtime = static_cast<double>(time) / static_cast<double>(CLK_TCK);
#else
	    const double dtime = static_cast<double>(time) / static_cast<double>(sysconf(_SC_CLK_TCK));
#endif
	    const double csecs = fmod( dtime * 100.0, 100.0 );
#else
	    const double dtime = time;
	    const double csecs = 0.0;
#endif
            const double secs  = fmod( floor( dtime ), 60.0 );
            const double mins  = fmod( floor( dtime / 60.0 ), 60.0 );
            const double hrs   = floor( dtime / 3600.0 );
            const ios_base::fmtflags flags = output.setf( ios::dec|ios::fixed, ios::basefield|ios::fixed );
            const int precision = output.precision(0);
            output.setf( ios::right, ios::adjustfield );

            output << " " << attribute << "=\"";
            char fill = output.fill('0');
            output << setw(2) << hrs
                   << ':' << setw(2) << mins
                   << ':' << setw(2) << secs
                   << '.' << setw(2) << csecs
                   <<  "\"";

            output.flags(flags);
            output.precision(precision);
            output.fill(fill);
            return output;
        }
    }
}
