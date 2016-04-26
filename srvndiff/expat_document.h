/* -*- C++ -*-
 *  $Id: expat_document.h 11963 2014-04-10 14:36:42Z greg $
 *
 *  Created by Greg Franks.
 */

#ifndef __LQIO_EXPAT_DOCUMENT__
#define __LQIO_EXPAT_DOCUMENT__

#include <string>
#include <stack>
#include <map>
#include <stdexcept>
#include <cstdarg>
#include <iostream>
#include "confidence_intervals.h"
#if HAVE_STRINGS_H
#include <strings.h>
#endif
#include <expat.h>

#include "error.h"

namespace LQIO {
    namespace DOM {
	class Expat_Document;
/* -- hacks for miminizing changes to header file (!) */

	typedef XML_Char DocumentObject;
	typedef XML_Char AndJoinActivityList;

	struct ActivityList {
	    /* Descriminator for the list type */
	    typedef enum ActivityListType {
		JOIN_ACTIVITY_LIST = 1,
		FORK_ACTIVITY_LIST,
		AND_FORK_ACTIVITY_LIST,
		AND_JOIN_ACTIVITY_LIST,
		OR_FORK_ACTIVITY_LIST,
		OR_JOIN_ACTIVITY_LIST,
		REPEAT_ACTIVITY_LIST
	    } ActivityListType;
	};
	struct Call {
	    /* Different types of calls */
	    typedef enum CallType {
		NULL_CALL,
		SEND_NO_REPLY,
		RENDEZVOUS,
		FORWARD,
	    } CallType;
	};

	class Expat_Document {

	private:
	    struct parse_stack_t;
	    typedef void (Expat_Document::*start_fptr)( const DocumentObject *, const XML_Char *, const XML_Char ** );
	    typedef void (Expat_Document::*end_fptr)( const DocumentObject *, const XML_Char * );
	    typedef void (Expat_Document::*result_fptr)( const DocumentObject *, const DocumentObject *, const DocumentObject *, const XML_Char ** );

	    struct parse_stack_t
	    {
		parse_stack_t(const XML_Char * e, start_fptr f, const DocumentObject * o=Expat_Document::XNil, const DocumentObject * x=Expat_Document::XNil, const DocumentObject * d=Expat_Document::XNil, result_fptr r=0, end_fptr y=0 )
		    : element(e), object(o), extra(x), dest(d), data(0), start_func(f), end_func(y), result(r) {}
		bool operator==( const XML_Char * ) const;

		const std::basic_string<XML_Char> element;
		std::basic_string<XML_Char> object;
		std::basic_string<XML_Char> extra;
		std::basic_string<XML_Char> dest;
		void * data;
		start_fptr start_func;
		end_fptr end_func;
		result_fptr result;
	    };

	    struct precedence_table_t
	    {
		bool operator()( const XML_Char * s1, const XML_Char * s2 ) const { return strcasecmp( s1, s2 ) < 0; }
	    };

	    struct call_type_table_t {
		const XML_Char * element;
		const XML_Char * attribute;
	    };

	    struct attribute_table_t
	    {
		bool operator()( const XML_Char * s1, const XML_Char * s2 ) const { return strcasecmp( s1, s2 ) < 0; }
	    };

	    struct confidence_result_table_t
	    {
		bool operator()( result_fptr f1, result_fptr f2 ) const;
	    };

	public:
	    static bool load( const char * filename );		// Factory.
	    virtual ~Expat_Document();

	private:
	    explicit Expat_Document();
	    Expat_Document( const Expat_Document& );
	    Expat_Document& operator=( const Expat_Document& );

	    static void start( void *data, const XML_Char *el, const XML_Char **attr );
	    static void end( void *data, const XML_Char *el );
	    static void start_cdata( void *data );
	    static void end_cdata( void *data );
	    static void handle_text( void * data, const XML_Char * text, int length );
	    static int handle_encoding( void * data, const XML_Char *name, XML_Encoding *info );

	private:
	    bool parse( const char * );
	    void input_error( const char * fmt, ... ) const;

	    /* Element handlers called from start() above. */
	    void startModel( const DocumentObject *, const XML_Char * element, const XML_Char ** attributes );
	    void startModelType( const DocumentObject *, const XML_Char * element, const XML_Char ** attributes );
	    void startResultGeneral( const DocumentObject *, const XML_Char * element, const XML_Char ** attributes );
	    void startMVAInfo( const DocumentObject *, const XML_Char * element, const XML_Char ** attributes );
	    void startProcessorType( const DocumentObject *, const XML_Char * element, const XML_Char ** attributes );
	    void startGroupType( const DocumentObject *, const XML_Char * element, const XML_Char ** attributes );
	    void startTaskType( const DocumentObject *, const XML_Char * element, const XML_Char ** attributes );
	    void startEntryType( const DocumentObject *, const XML_Char * element, const XML_Char ** attributes );
	    void startPhaseActivities( const DocumentObject *, const XML_Char * element, const XML_Char ** attributes );
	    void startActivityDefBase( const DocumentObject *, const XML_Char * element, const XML_Char ** attributes );
	    void startActivityMakingCallType( const DocumentObject * activity, const XML_Char * element, const XML_Char ** attributes );
	    void startEntryMakingCallType( const DocumentObject * activity, const XML_Char * element, const XML_Char ** attributes );
	    void startTaskActivityGraph( const DocumentObject * task, const XML_Char * element, const XML_Char ** attributes );
	    void startPrecedenceType( const DocumentObject * task, const XML_Char * element, const XML_Char ** attributes );
	    void startActivityListType( const DocumentObject * task, const XML_Char * element, const XML_Char ** attributes );
	    void endActivityListType( const DocumentObject * task, const XML_Char * element );
	    void startReplyActivity( const DocumentObject * task, const XML_Char * element, const XML_Char ** attributes );
	    void startOutputResultType( const DocumentObject * activity, const XML_Char * element, const XML_Char ** attributes );
	    void startJoinResultType( const DocumentObject * activity, const XML_Char * element, const XML_Char ** attributes );
	    void startOutputDistributionType( const DocumentObject * activity, const XML_Char * element, const XML_Char ** attributes );
	    void startLQX( const DocumentObject * activity, const XML_Char * element, const XML_Char ** attributes );
	    void startNOP( const DocumentObject * activity, const XML_Char * element, const XML_Char ** attributes );

	    void handleProcessorResults( const DocumentObject *, const DocumentObject *, const DocumentObject *, const XML_Char ** attributes );
	    void handleGroupResults( const DocumentObject *, const DocumentObject *, const DocumentObject *, const XML_Char ** attributes );
	    void handleTaskResults( const DocumentObject *, const DocumentObject *, const DocumentObject *, const XML_Char ** attributes );
	    void handleEntryResults( const DocumentObject *, const DocumentObject *, const DocumentObject *, const XML_Char ** attributes );
	    void handlePhaseResults( const DocumentObject *, const DocumentObject *, const DocumentObject *, const XML_Char ** attributes );
	    void handleActivityResults( const DocumentObject *, const DocumentObject *, const DocumentObject *, const XML_Char ** attributes );
	    void handleEntryFwdCallResults( const DocumentObject *, const DocumentObject *, const DocumentObject *, const XML_Char ** attributes );
	    void handlePhaseRNVCallResults( const DocumentObject *, const DocumentObject *, const DocumentObject *, const XML_Char ** attributes );
	    void handleActivityRNVCallResults( const DocumentObject *, const DocumentObject *, const DocumentObject *, const XML_Char ** attributes );
	    void handlePhaseSNRCallResults( const DocumentObject *, const DocumentObject *, const DocumentObject *, const XML_Char ** attributes );
	    void handleActivitySNRCallResults( const DocumentObject *, const DocumentObject *, const DocumentObject *, const XML_Char ** attributes );
	    void handleJoinResults( const DocumentObject *, const DocumentObject *, const DocumentObject *, const XML_Char ** attributes );

	    void handleConfidenceResults( const DocumentObject *, const DocumentObject *, const DocumentObject *, const XML_Char ** attributes );

	    void handleProcessorConfResults( const DocumentObject *, const DocumentObject *, const DocumentObject *, const XML_Char ** attributes );
	    void handleGroupConfResults( const DocumentObject *, const DocumentObject *, const DocumentObject *, const XML_Char ** attributes );
	    void handleTaskConfResults( const DocumentObject *, const DocumentObject *, const DocumentObject *, const XML_Char ** attributes );
	    void handleEntryConfResults( const DocumentObject *, const DocumentObject *, const DocumentObject *, const XML_Char ** attributes );
	    void handlePhaseConfResults( const DocumentObject *, const DocumentObject *, const DocumentObject *, const XML_Char ** attributes );
	    void handleActivityConfResults( const DocumentObject *, const DocumentObject *, const DocumentObject *, const XML_Char ** attributes );
	    void handleEntryFwdCallConfResults( const DocumentObject *, const DocumentObject *, const DocumentObject *, const XML_Char ** attributes );
	    void handlePhaseRNVCallConfResults( const DocumentObject *, const DocumentObject *, const DocumentObject *, const XML_Char ** attributes );
	    void handleActivityRNVCallConfResults( const DocumentObject *, const DocumentObject *, const DocumentObject *, const XML_Char ** attributes );
	    void handlePhaseSNRCallConfResults( const DocumentObject *, const DocumentObject *, const DocumentObject *, const XML_Char ** attributes );
	    void handleActivitySNRCallConfResults( const DocumentObject *, const DocumentObject *, const DocumentObject *, const XML_Char ** attributes );
	    void handleJoinConfResults( const DocumentObject *, const DocumentObject *, const DocumentObject *, const XML_Char ** attributes );

	    const XML_Char * getStringAttribute( const XML_Char ** attributes, const XML_Char * Xcomment, const XML_Char * default_value=0 ) const;
	    const double getDoubleAttribute( const XML_Char ** attributes, const XML_Char * Xconv_val, const double default_value=-1.0 ) const;
	    const long getLongAttribute( const XML_Char ** attributes, const XML_Char * Xprint_int, const long default_value=-1 ) const;
	    const bool getBoolAttribute( const XML_Char ** attributes, const XML_Char * Xprint_int, const bool default_value=false ) const;
	    const double getTimeAttribute( const XML_Char ** attributes, const XML_Char * Xprint_int ) const;
	    const unsigned int getEnumerationAttribute( const XML_Char ** attributes, const XML_Char * Xprint_int, const XML_Char **, const unsigned int ) const;

	    static void init_tables();

	public:
	    static bool __debugXML;

	private:
	    XML_Parser _parser;
	    std::stack<parse_stack_t> _stack;
	    ConfidenceIntervals _conf_95;
	    const ConfidenceIntervals _conf_99;

	private:
	    static std::map<const XML_Char *,ActivityList::ActivityListType,attribute_table_t> precedence_table;
	    static std::map<result_fptr,result_fptr,confidence_result_table_t> confidence_result_table;

	    static const XML_Char *XNil;

	    static const XML_Char * XMLSchema_instance;

	    static const XML_Char *XDETERMINISTIC;
	    static const XML_Char *XGRAPH;
	    static const XML_Char *XNONE;
	    static const XML_Char *XPH1PH2;
	    static const XML_Char *Xactivity;
	    static const XML_Char *Xasynch_call;
	    static const XML_Char *Xbound_to_entry;
	    static const XML_Char *Xcall_order;
	    static const XML_Char *Xcalls_mean;
	    static const XML_Char *Xcap;
	    static const XML_Char *Xcomment;
	    static const XML_Char *Xconv_val;
	    static const XML_Char *Xconv_val_result;
	    static const XML_Char *Xcore;
	    static const XML_Char *Xcount;
	    static const XML_Char *Xdest;
	    static const XML_Char *Xelapsed_time;
	    static const XML_Char *Xend;
	    static const XML_Char *Xentry;
	    static const XML_Char *Xentry_phase_activities;
	    static const XML_Char *Xfanin;
	    static const XML_Char *Xfanout;
	    static const XML_Char *Xfaults;
	    static const XML_Char *Xforwarding;
	    static const XML_Char *Xgroup;
	    static const XML_Char *Xhistogram;
	    static const XML_Char *Xhost_demand_cvsq;
	    static const XML_Char *Xhost_demand_mean;
	    static const XML_Char *Xhost_max_phase_service_time;
	    static const XML_Char *Xit_limit;
	    static const XML_Char *Xiterations;
	    static const XML_Char *Xjoin_variance;
	    static const XML_Char *Xjoin_waiting;
	    static const XML_Char *Xloss_probability;
	    static const XML_Char *Xlqn_model;
	    static const XML_Char *Xlqx;
	    static const XML_Char *Xmax;
	    static const XML_Char *Xmax_service_time;
	    static const XML_Char *Xmin;
	    static const XML_Char *Xmultiplicity;
	    static const XML_Char *Xmva_info;
	    static const XML_Char *Xname;
	    static const XML_Char *Xnumber_bins;
	    static const XML_Char *Xopen_arrival_rate;
	    static const XML_Char *Xopen_wait_time;
	    static const XML_Char *Xparam;
	    static const XML_Char *XphaseP_proc_waiting[];
	    static const XML_Char *XphaseP_service_time[];
	    static const XML_Char *XphaseP_service_time_variance[];
	    static const XML_Char *XphaseP_utilization[];
	    static const XML_Char *Xphase;
	    static const XML_Char *Xplatform_info;
	    static const XML_Char *Xpost;
	    static const XML_Char *Xpost_and;
	    static const XML_Char *Xpost_loop;
	    static const XML_Char *Xpost_or;
	    static const XML_Char *Xpragma;
	    static const XML_Char *Xpre;
	    static const XML_Char *Xpre_and;
	    static const XML_Char *Xpre_or;
	    static const XML_Char *Xprecedence;
	    static const XML_Char *Xprint_int;
	    static const XML_Char *Xpriority;
	    static const XML_Char *Xprob;
	    static const XML_Char *Xprob_exceed_max_service_time;
	    static const XML_Char *Xproc_utilization;
	    static const XML_Char *Xproc_waiting;
	    static const XML_Char *Xprocessor;
	    static const XML_Char *Xquantum;
	    static const XML_Char *Xqueue_length;
	    static const XML_Char *Xquorum;
	    static const XML_Char *Xreplication;
	    static const XML_Char *Xreply_activity;
	    static const XML_Char *Xreply_entry;
	    static const XML_Char *Xresult_activity;
	    static const XML_Char *Xresult_call;
	    static const XML_Char *Xresult_conf_95;
	    static const XML_Char *Xresult_conf_99;
	    static const XML_Char *Xresult_entry;
	    static const XML_Char *Xresult_general;
	    static const XML_Char *Xresult_group;
	    static const XML_Char *Xresult_join_delay;
	    static const XML_Char *Xresult_processor;
	    static const XML_Char *Xresult_task;
	    static const XML_Char *Xr_lock;
	    static const XML_Char *Xr_unlock;
	    static const XML_Char *Xrwlock;
	    static const XML_Char *Xrwlock_reader_waiting;
	    static const XML_Char *Xrwlock_reader_waiting_variance;
	    static const XML_Char *Xrwlock_reader_holding;
	    static const XML_Char *Xrwlock_reader_holding_variance;
	    static const XML_Char *Xrwlock_reader_utilization;
	    static const XML_Char *Xrwlock_writer_waiting;
	    static const XML_Char *Xrwlock_writer_waiting_variance;
	    static const XML_Char *Xrwlock_writer_holding;
	    static const XML_Char *Xrwlock_writer_holding_variance;
	    static const XML_Char *Xrwlock_writer_utilization;
	    static const XML_Char *Xscheduling;
	    static const XML_Char *Xsemaphore;
	    static const XML_Char *Xsemaphore_waiting;
	    static const XML_Char *Xsemaphore_waiting_variance;
	    static const XML_Char *Xsemaphore_utilization;
	    static const XML_Char *Xservice;
	    static const XML_Char *Xservice_time;
	    static const XML_Char *Xservice_time_distribution;
	    static const XML_Char *Xservice_time_variance;
	    static const XML_Char *Xshare;
	    static const XML_Char *Xsignal;
	    static const XML_Char *Xsolver_info;
	    static const XML_Char *Xsolver_parameters;
	    static const XML_Char *Xspeed_factor;
	    static const XML_Char *Xsquared_coeff_variation;
	    static const XML_Char *Xstep;
	    static const XML_Char *Xstep_squared;
	    static const XML_Char *Xsubmodels;
	    static const XML_Char *Xsynch_call;
	    static const XML_Char *Xsystem_cpu_time;
	    static const XML_Char *Xtask;
	    static const XML_Char *Xtask_activities;
	    static const XML_Char *Xthink_time;
	    static const XML_Char *Xthroughput;
	    static const XML_Char *Xthroughput_bound;
	    static const XML_Char *Xtype;
	    static const XML_Char *Xunderrelax_coeff;
	    static const XML_Char *Xuser_cpu_time;
	    static const XML_Char *Xutilization;
	    static const XML_Char *Xvalid;
	    static const XML_Char *Xvalue;
	    static const XML_Char *Xwait;
	    static const XML_Char *Xwait_squared;
	    static const XML_Char *Xwaiting;
	    static const XML_Char *Xwaiting_variance;
	    static const XML_Char *Xw_lock;
	    static const XML_Char *Xw_unlock;
	    static const XML_Char *Xxml_debug;
            static const XML_Char *Xhistogram_bin;
            static const XML_Char *Xoverflow_bin;
            static const XML_Char *Xresult_activity_distribution;
            static const XML_Char *Xresult_forwarding;
            static const XML_Char *Xunderflow_bin;
	};
    }
}
#endif /* */
