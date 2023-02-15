/* -*- C++ -*-
 *  $Id: expat_document.h 16421 2023-02-14 02:45:49Z greg $
 *
 *  Created by Martin Mroz on 24/02/09.
 */

#ifndef __LQIO_EXPAT_DOCUMENT__
#define __LQIO_EXPAT_DOCUMENT__

#include <string>
#include <stack>
#include <stdexcept>
#include <cstdarg>
#include <iostream>
#if HAVE_STRINGS_H
#include <strings.h>
#endif
#include <expat.h>
#include "common_io.h"
#include "dom_document.h"
#include "dom_histogram.h"
#include "dom_task.h"
#include "dom_processor.h"
#include "dom_actlist.h"
#include "srvn_spex.h"

namespace LQIO {
    namespace DOM {
	class Expat_Document;
	class ExportObservation;
	
	typedef void (Expat_Document::*start_fptr)( DocumentObject *, const XML_Char *, const XML_Char ** );
	typedef void (Expat_Document::*end_fptr)( DocumentObject *, const XML_Char * );
	typedef DocumentObject& (DocumentObject::*set_result_fptr)( const double );

	class Expat_Document : private Common_IO {

	private:
	    typedef double (DOM::Entry::*doubleEntryFunc)( const unsigned ) const;
	    typedef double (DOM::Task::*doubleTaskFunc)( const unsigned ) const;

	    struct parse_stack_t
	    {
		parse_stack_t(const XML_Char * e, start_fptr sh, DocumentObject * o, DocumentObject * r=nullptr) : element(e), start(sh), end(nullptr), object(o), extra_object(r) {}
		parse_stack_t(const XML_Char * e, start_fptr sh, end_fptr eh, DocumentObject * o ) : element(e), start(sh), end(eh), object(o), extra_object(nullptr) {}
		bool operator==( const XML_Char * ) const;

		const std::basic_string<XML_Char> element;
		start_fptr start;
		end_fptr end;
		DocumentObject * object;
		DocumentObject * extra_object;
	    };

	    struct result_table_t
	    {
		result_table_t( set_result_fptr m=0, set_result_fptr v=0 ) : mean(m), variance(v) {}
		bool operator()( const XML_Char * s1, const XML_Char * s2 ) const { return strcasecmp( s1, s2 ) < 0; }
		set_result_fptr mean;
		set_result_fptr variance;
	    };

	    /*+ SPEX */
	    struct observation_table_t
	    {
		observation_table_t() : key(0), phase(0) {}
		observation_table_t( int k, int p=0 ) : key(k), phase(p) {}
		bool operator()( const char * s1, const char * s2 ) const { return strcasecmp( s1, s2 ) < 0; }
		const int key;
		const int phase;
	    };
	    /*- SPEX */
	    
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

	    class XMLCharManip {
	    public:
		XMLCharManip( std::ostream& (*f)(std::ostream&, const XML_Char *, const XML_Char * ), const XML_Char * a, const XML_Char * v=0 ) : _f(f), _a(a), _v(v) {}
	    private:
		std::ostream& (*_f)( std::ostream&, const XML_Char *, const XML_Char * );
		const XML_Char * _a;
		const XML_Char * _v;

		friend std::ostream& operator<<(std::ostream & os, const XMLCharManip& m ) { return m._f(os,m._a,m._v); }
	    };

	    class XMLCharBoolManip {
	    public:
		XMLCharBoolManip( std::ostream& (*f)(std::ostream&, const XML_Char *, const bool ), const XML_Char * a, const bool b ) : _f(f), _a(a), _b(b) {}
	    private:
		std::ostream& (*_f)( std::ostream&, const XML_Char *, const bool );
		const XML_Char * _a;
		const bool _b;

		friend std::ostream& operator<<(std::ostream & os, const XMLCharBoolManip& m ) { return m._f(os,m._a,m._b); }
	    };

	    class XMLCharDoubleManip {
	    public:
		XMLCharDoubleManip( std::ostream& (*f)(std::ostream&, const XML_Char *, const double ), const XML_Char * a, const double v ) : _f(f), _a(a), _v(v) {}
	    private:
		std::ostream& (*_f)( std::ostream&, const XML_Char *, const double );
		const XML_Char * _a;
		const double _v;

		friend std::ostream& operator<<(std::ostream & os, const XMLCharDoubleManip& m ) { return m._f(os,m._a,m._v); }
	    };

	    class EntryResultsManip {
	    public:
		EntryResultsManip( std::ostream& (*f)(std::ostream&, const DOM::Entry&, const XML_Char **, const doubleEntryFunc, const ConfidenceIntervals * ),
				   const Entry & e, const XML_Char ** a, const doubleEntryFunc p, const ConfidenceIntervals * c=0 ) : _f(f), _e(e), _a(a), _p(p), _c(c) {}
	    private:
		std::ostream& (*_f)( std::ostream&, const DOM::Entry&, const XML_Char **, const doubleEntryFunc, const ConfidenceIntervals * );
		const DOM::Entry & _e;		/* entry */
		const XML_Char ** _a;		/* attributes */
		const doubleEntryFunc _p;	/* phase */
		const ConfidenceIntervals * _c;	/* optional confidence level */
		friend std::ostream& operator<<(std::ostream & os, const EntryResultsManip& m ) { return m._f(os,m._e,m._a,m._p,m._c); }
	    };

	    class TaskResultsManip {
	    public:
		TaskResultsManip( std::ostream& (*f)(std::ostream&, const DOM::Task&, const XML_Char **, const doubleTaskFunc, const ConfidenceIntervals * ),
				  const Task & t, const XML_Char ** a, const doubleTaskFunc p, const ConfidenceIntervals * c=0 ) : _f(f), _t(t), _a(a), _p(p), _c(c) {}
	    private:
		std::ostream& (*_f)( std::ostream&, const DOM::Task&, const XML_Char **, const doubleTaskFunc, const ConfidenceIntervals * );
		const DOM::Task & _t;		/* task */
		const XML_Char ** _a;		/* attributes */
		const doubleTaskFunc _p;	/* phase */
		const ConfidenceIntervals * _c;	/* optional confidence level */
		friend std::ostream& operator<<(std::ostream & os, const TaskResultsManip& m ) { return m._f(os,m._t,m._a,m._p,m._c); }
	    };

	    struct ExportProcessor {
		ExportProcessor( std::ostream& output, const Expat_Document& self ) : _output( output ), _self( self ) {}
		void operator()( const std::pair<unsigned, Entity *>& e ) const { Processor * p = dynamic_cast<Processor *>(e.second); if ( p ) _self.exportProcessor( _output, *p ); }
	    private:
		std::ostream& _output;
		const Expat_Document& _self;
	    };

	    struct ExportGroup {
		ExportGroup( std::ostream& output, const Expat_Document& self ) : _output( output ), _self( self ) {}
		void operator()( const Group * g ) const { _self.exportGroup( _output, *g ); }
	    private:
		std::ostream& _output;
		const Expat_Document& _self;
	    };

	    struct ExportTask {
		ExportTask( std::ostream& output, const Expat_Document& self, bool not_in_group=false ) : _output( output ), _self( self ), _not_in_group(not_in_group) {}
		void operator()( const Task * t ) const { if ( !_not_in_group || !t->getGroup() ) _self.exportTask( _output, *t ); }
	    private:
		std::ostream& _output;
		const Expat_Document& _self;
		const bool _not_in_group;
	    };

	    struct ExportEntry {
		ExportEntry( std::ostream& output, const Expat_Document& self ) : _output( output ), _self( self ) {}
		void operator()( const Entry * e ) const { _self.exportEntry( _output, *e ); }
	    private:
		std::ostream& _output;
		const Expat_Document& _self;
	    };

	    struct ExportPhase {
		ExportPhase( std::ostream& output, const Expat_Document& self ) : _output( output ), _self( self ) {}
		void operator()( const std::pair<unsigned,Phase *>& p ) const { const Phase * phase = p.second; if ( phase->isPresent() ) _self.exportActivity( _output, *phase, p.first ); }
	    private:
		std::ostream& _output;
		const Expat_Document& _self;
	    };

	    struct ExportCall {
		ExportCall( std::ostream& output, const Expat_Document& self ) : _output( output ), _self( self ) {}
		void operator()( const Call * c ) const { _self.exportCall( _output, *c ); }
	    private:
		std::ostream& _output;
		const Expat_Document& _self;
	    };

	    struct ExportActivity {
		ExportActivity( std::ostream& output, const Expat_Document& self ) : _output( output ), _self( self ) {}
		void operator()( const std::pair<std::string,Activity *>& a ) const { _self.exportActivity( _output, *a.second ); }
	    private:
		std::ostream& _output;
		const Expat_Document& _self;
	    };

	    struct ExportPrecedence {
		ExportPrecedence( std::ostream& output, const Expat_Document& self ) : _output( output ), _self( self ) {}
		void operator()( const ActivityList *) const;
	    private:
		std::ostream& _output;
		const Expat_Document& _self;
	    };

	private:
	    friend class LQIO::DOM::Document;
	    friend class LQIO::DOM::ExportObservation;

	    Expat_Document( Document&, const std::string&, bool=true, bool=false );

	public:
	    virtual ~Expat_Document();

	    bool hasResults() const { return _document.hasResults(); }
	    bool hasSPEX() const { return _has_spex; /* And not outputing LQX */ }

	    void serializeDOM( std::ostream& output ) const;
	    
	    static bool load( Document&, const std::string&, const bool load_results );		// Factory.
	    static bool loadResults( Document&, const std::string& );

	private:
	    Expat_Document( const Expat_Document& ) = delete;
	    Expat_Document& operator=( const Expat_Document& ) = delete;

	    static void start( void *data, const XML_Char *el, const XML_Char **attr );
	    static void end( void *data, const XML_Char *el );
	    static void start_cdata( void *data );
	    static void end_cdata( void *data );
	    static void handle_text( void * data, const XML_Char * text, int length );
	    static void handle_comment( void * data, const XML_Char * text );
	    static int handle_encoding( void * data, const XML_Char *name, XML_Encoding *info );

	private:
	    bool parse();
	    void input_error( const char * fmt, ... ) const;

	    /* Element handlers called from start() above. */
	    void startModel( DocumentObject *, const XML_Char * element, const XML_Char ** attributes );
	    void startModelType( DocumentObject *, const XML_Char * element, const XML_Char ** attributes );
	    void startResultGeneral( DocumentObject *, const XML_Char * element, const XML_Char ** attributes );
	    void startMVAInfo( DocumentObject *, const XML_Char * element, const XML_Char ** attributes );

	    void startProcessorType( DocumentObject *, const XML_Char * element, const XML_Char ** attributes );
	    void startGroupType( DocumentObject *, const XML_Char * element, const XML_Char ** attributes );
	    void startTaskType( DocumentObject *, const XML_Char * element, const XML_Char ** attributes );
	    void startEntryType( DocumentObject *, const XML_Char * element, const XML_Char ** attributes );
	    void startPhaseActivities( DocumentObject *, const XML_Char * element, const XML_Char ** attributes );
	    void startActivityDefBase( DocumentObject *, const XML_Char * element, const XML_Char ** attributes );
	    void startActivityMakingCallType( DocumentObject * activity, const XML_Char * element, const XML_Char ** attributes );
	    void startEntryMakingCallType( DocumentObject * activity, const XML_Char * element, const XML_Char ** attributes );
	    void startTaskActivityGraph( DocumentObject * task, const XML_Char * element, const XML_Char ** attributes );
	    void startPrecedenceType( DocumentObject * task, const XML_Char * element, const XML_Char ** attributes );
	    void startActivityListType( DocumentObject * task, const XML_Char * element, const XML_Char ** attributes );
	    void startReplyActivity( DocumentObject * task, const XML_Char * element, const XML_Char ** attributes );
	    void startOutputResultType( DocumentObject * document, const XML_Char * element, const XML_Char ** attributes );
/*+ SPEX */
	    void startSPEXObservationType( DocumentObject * object, const XML_Char * element, const XML_Char ** attributes );
	    void endSPEXObservationType( DocumentObject * object, const XML_Char * elelemt );
/*- SPEX */
	    void startJoinResultType( DocumentObject * activity, const XML_Char * element, const XML_Char ** attributes );
	    void startOutputDistributionType( DocumentObject * activity, const XML_Char * element, const XML_Char ** attributes );
	    void startMarginalQueueProbabilities( DocumentObject * entity, const XML_Char * element, const XML_Char ** attributes );
	    void endMarginalQueueProbabilities( DocumentObject * entity, const XML_Char * element );
	    void startLQX( DocumentObject * activity, const XML_Char * element, const XML_Char ** attributes );
/*+ SPEX */
	    void startSPEXParameters( DocumentObject * document, const XML_Char * element, const XML_Char ** attributes );
	    void endSPEXParameters( DocumentObject * document, const XML_Char * element );
	    void startSPEXResults( DocumentObject * document, const XML_Char * element, const XML_Char ** attributes );
	    void endSPEXResults( DocumentObject * document, const XML_Char * element );
	    void startSPEXConvergence( DocumentObject * document, const XML_Char * element, const XML_Char ** attributes );
	    void endSPEXConvergence( DocumentObject * document, const XML_Char * element );
/*- SPEX */
	    void startNOP( DocumentObject * document, const XML_Char * element, const XML_Char ** attributes );

	    /* Methods invoked from element handlers */
	    DocumentObject * handleModel( DocumentObject * object, const XML_Char ** attributes );

	    Processor * handleProcessor( DocumentObject * object, const XML_Char ** attributes );
	    DocumentObject * handleGroup( DocumentObject * object, const XML_Char ** attributes );
	    Task * handleTask( DocumentObject * object, const XML_Char ** attributes );
	    void handleFanIn( DocumentObject * object, const XML_Char ** attributes );
	    void handleFanOut( DocumentObject * object, const XML_Char ** attributes );
	    Entry * handleEntry( DocumentObject * object, const XML_Char ** attributes );
	    Phase * handlePhaseActivity( DocumentObject * object, const XML_Char ** attributes );
	    Activity * handleTaskActivity( DocumentObject * object, const XML_Char ** attributes );
	    void handleActivity( Phase * phase, const XML_Char ** attributes );
	    void handleActivityList( ActivityList * activity_list, const XML_Char ** attributes );
	    Call * handleActivityCall( DocumentObject *, const XML_Char ** attributes, const Call::Type call_type );
	    Call * handleEntryCall( DocumentObject *, const XML_Char ** attributes );
	    Call * handlePhaseCall( DocumentObject *, const XML_Char ** attributes, const Call::Type call_type );
	 // DecisionPath * handleDecisionPath( DocumentObject * object, const XML_Char ** attributes );
	    Histogram * handleHistogram( DocumentObject * object, const XML_Char ** attributes );

	    Histogram * handleQueueLengthDistribution( DocumentObject * object, const XML_Char ** attributes );
	    void handleHistogramBin( DocumentObject * object, const XML_Char * element, const XML_Char ** attributes );
	    void handleResults( DocumentObject *, const XML_Char ** attributes );
	    void handleResults95( DocumentObject *, const XML_Char ** attributes );
	    void handleJoinResults( AndJoinActivityList *, const XML_Char ** attributes );
	    void handleJoinResults95( AndJoinActivityList *, const XML_Char ** attributes );
	    void handleMarginalQueueProbabilities( Entity *, const XML_Char ** attributes );
	    void handleSPEXObservation( DocumentObject *, const XML_Char ** attributes, unsigned int = 0 );
	    void handleSPEXObservation95( DocumentObject *, const XML_Char ** attributes );
	    Histogram * findOrAddHistogram( DocumentObject * object, Histogram::Type type, unsigned int n_bins, double min, double max );
	    Histogram * findOrAddHistogram( DocumentObject * object, unsigned int phase, Histogram::Type type, unsigned int n_bins, double min, double max );

	    bool checkAttributes( const XML_Char * element, const XML_Char ** attributes, const std::set<const XML_Char *,Expat_Document::attribute_table_t>& table ) const;

	    LQIO::DOM::ExternalVariable * getOptionalAttribute( const XML_Char ** attributes, const XML_Char * argument ) const;
	    LQIO::DOM::ExternalVariable * getVariableAttribute( const XML_Char ** attributes, const XML_Char * argument, const XML_Char * default_value=nullptr ) const;
	    static const scheduling_type getSchedulingAttribute( const XML_Char ** attributes, const scheduling_type );

	    void exportHeader( std::ostream& output ) const;
	    void exportSPEXParameters( std::ostream& output ) const;
	    void exportGeneral( std::ostream& output ) const;
	    void exportProcessor( std::ostream& output, const Processor & ) const;
	    void exportGroup( std::ostream& output, const Group & ) const;
	    void exportTask( std::ostream& output,const Task & ) const;
	    void exportEntry( std::ostream& output, const Entry & ) const;
	    void exportActivity( std::ostream& output, const Phase &, const unsigned=0 ) const;
	    void exportPrecedence( std::ostream& output, const ActivityList& activity_list ) const;
	    void exportCall( std::ostream& output, const Call & ) const;
	    void exportHistogram( std::ostream& output, const Histogram& histogram, const unsigned phase=0 ) const;
	    void exportObservation( std::ostream& output, const DocumentObject * object ) const;
	    void exportLQX( std::ostream& output ) const;
	    void exportSPEXResults( std::ostream& output ) const;
	    void exportSPEXConvergence( std::ostream& output ) const;
	    void exportFooter( std::ostream& output ) const;
	    static void init_tables();
	    static std::ostream& printEntryPhaseResults( std::ostream& output, const Entry & entry, const XML_Char ** attributes, const doubleEntryFunc func, const ConfidenceIntervals * );
	    static std::ostream& printTaskPhaseResults( std::ostream& output, const Task & task, const XML_Char ** attributes, const doubleTaskFunc func, const ConfidenceIntervals * );
	    static EntryResultsManip entry_phase_results( const Entry& t, const XML_Char ** l, doubleEntryFunc f, const ConfidenceIntervals * c=0 ) { return EntryResultsManip( &printEntryPhaseResults, t, l, f, c ); }
	    static TaskResultsManip task_phase_results( const Task& t, const XML_Char ** l, doubleTaskFunc f, const ConfidenceIntervals * c=0 ) { return TaskResultsManip( &printTaskPhaseResults, t, l, f, c ); }

	public:
	    static bool __instantiate;

	private:
	    Document& _document;
	    XML_Parser _parser;
	    const std::string& _input_file_name;
	    bool _createObjects;
	    bool _loadResults;
	    std::stack<parse_stack_t> _stack;
	    std::string _text;			/* Place for text sections. */

	    /*+ SPEX */
	    bool _has_spex;			/* True if SPEX present AND not outputting LQX */
	    std::set<LQIO::Spex::ObservationInfo,LQIO::Spex::ObservationInfo> _spex_observation;
	    /*- SPEX */

	private:
	    static const std::set<const XML_Char *,attribute_table_t> call_table;
	    static const std::set<const XML_Char *,Expat_Document::attribute_table_t> histogram_table;
	    static const std::map<const XML_Char *,const result_table_t,result_table_t>  result_table;
	    static const std::map<const XML_Char *,const observation_table_t,observation_table_t>  observation_table;	/* SPEX */
	    static const std::map<const int,const char *> __key_lqx_function_map;			/* Maps srvn_gram.h KEY_XXX to SPEX attribute name */
	    static const std::map<const Call::Type,const call_type_table_t> call_type_table;

	    static const XML_Char * XMLSchema_instance;

	    static const XML_Char *XDETERMINISTIC;
	    static const XML_Char *XGRAPH;
	    static const XML_Char *XNONE;
	    static const XML_Char *XPH1PH2;
	    static const XML_Char *Xactivity;
	    static const XML_Char *Xactivity_graph;
	    static const XML_Char *Xasynch_call;
	    static const XML_Char *Xbegin;
	    static const XML_Char *Xbottleneck_strength;
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
	    static const XML_Char *Xhost_demand_cvsq;
	    static const XML_Char *Xhost_demand_mean;
	    static const XML_Char *Xinitially;
	    static const XML_Char *Xit_limit;
	    static const XML_Char *Xiterations;
	    static const XML_Char *Xjoin_variance;
	    static const XML_Char *Xjoin_waiting;
	    static const XML_Char *Xloss_probability;
	    static const XML_Char *Xlqn_model;
	    static const XML_Char *Xlqx;
	    static const XML_Char *Xmarginal_queue_probabilities;
	    static const XML_Char *Xmax;
	    static const XML_Char *Xmax_rss;
	    static const XML_Char *Xmax_service_time;
	    static const XML_Char *Xmin;
	    static const XML_Char *Xmultiplicity;
	    static const XML_Char *Xmva_info;
	    static const XML_Char *Xname;
	    static const XML_Char *Xnumber_bins;
	    static const XML_Char *Xopen_arrival_rate;
	    static const XML_Char *Xopen_wait_time;
	    static const XML_Char *Xparam;
	    static const XML_Char *Xphase;
	    static const XML_Char *XphaseP_proc_waiting[];
	    static const XML_Char *XphaseP_service_time[];
	    static const XML_Char *XphaseP_service_time_variance[];
	    static const XML_Char *XphaseP_utilization[];
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
	    static const XML_Char *Xqueue_length_distribution;
	    static const XML_Char *Xquorum;
	    static const XML_Char *Xr_lock;
	    static const XML_Char *Xr_unlock;
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
	    static const XML_Char *Xresult_observation;
	    static const XML_Char *Xresult_processor;
	    static const XML_Char *Xresult_task;
	    static const XML_Char *Xrwlock;
	    static const XML_Char *Xrwlock_reader_holding;
	    static const XML_Char *Xrwlock_reader_holding_variance;
	    static const XML_Char *Xrwlock_reader_utilization;
	    static const XML_Char *Xrwlock_reader_waiting;
	    static const XML_Char *Xrwlock_reader_waiting_variance;
	    static const XML_Char *Xrwlock_writer_holding;
	    static const XML_Char *Xrwlock_writer_holding_variance;
	    static const XML_Char *Xrwlock_writer_utilization;
	    static const XML_Char *Xrwlock_writer_waiting;
	    static const XML_Char *Xrwlock_writer_waiting_variance;
	    static const XML_Char *Xscheduling;
	    static const XML_Char *Xsemaphore;
	    static const XML_Char *Xsemaphore_utilization;
	    static const XML_Char *Xsemaphore_waiting;
	    static const XML_Char *Xsemaphore_waiting_variance;
	    static const XML_Char *Xservice;
	    static const XML_Char *Xservice_time;
	    static const XML_Char *Xservice_time_distribution;
	    static const XML_Char *Xservice_time_variance;
	    static const XML_Char *Xshare;
	    static const XML_Char *Xsignal;
	    static const XML_Char *Xsize;
	    static const XML_Char *Xsolver_info;
	    static const XML_Char *Xsolver_parameters;
	    static const XML_Char *Xsource;
	    static const XML_Char *Xspeed_factor;
	    static const XML_Char *Xspex_convergence;
	    static const XML_Char *Xspex_parameters;
	    static const XML_Char *Xspex_results;
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
	    static const XML_Char *Xw_lock;
	    static const XML_Char *Xw_unlock;
	    static const XML_Char *Xwait;
	    static const XML_Char *Xwait_squared;
	    static const XML_Char *Xwaiting;
	    static const XML_Char *Xwaiting_variance;
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
