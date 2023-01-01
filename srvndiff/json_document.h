/* -*- C++ -*-
 *  $Id: json_document.h 16230 2023-01-01 15:01:53Z greg $
 *
 *  Created by Greg Franks.
 */

#ifndef __LQIO_JSON_DOCUMENT__
#define __LQIO_JSON_DOCUMENT__

#include <cmath>
#include "../lqiolib/src/headers/lqio/picojson.h"
#include <stdexcept>
#include <time.h>
#include "error.h"
#include "confidence_intervals.h"

namespace LQIO {
    namespace DOM {

	class Json_Document {
	    typedef enum call_type { RENDEZVOUS, SEND_NO_REPLY, FORWARD } call_type;
	    typedef void (Json_Document::*set_object_fptr)( const picojson::value& );
	    typedef void (*call_save_fptr)( unsigned int activity, unsigned int destination, const picojson::value& result );
	    typedef void (*phs_save_fptr)( unsigned int entry, unsigned int phase, const picojson::value& result );
	    typedef void (*ent_save_fptr)( unsigned int entry, unsigned int phase, unsigned int destination, const picojson::value& result );


	private:
	    class AttributeManip 
	    {
	    public:
		AttributeManip( std::ostream& (*f)(std::ostream&, const std::string&, const picojson::value& ), const std::string& a, const picojson::value& v ) : _f(f), _a(a), _v(v) {}
	    private:
		std::ostream& (*_f)( std::ostream&, const std::string&, const picojson::value& );
		const std::string& _a;
		const picojson::value& _v;

		friend std::ostream& operator<<(std::ostream & os, const AttributeManip& m ) { return m._f(os,m._a,m._v); }
	    };

	    class IntegerManip {
	    public:
		IntegerManip( std::ostream& (*f)(std::ostream&, const int ), const int i ) : _f(f), _i(i) {}
	    private:
		std::ostream& (*_f)( std::ostream&, const int );
		const int _i;

		friend std::ostream& operator<<(std::ostream & os, const IntegerManip& m ) { return m._f(os,m._i); }
	    };

	    class SimpleManip {
	    public:
		SimpleManip( std::ostream& (*f)(std::ostream& ) ) : _f(f) {}
	    private:
		std::ostream& (*_f)( std::ostream& );

		friend std::ostream& operator<<(std::ostream & os, const SimpleManip& m ) { return m._f(os); }
	    };

	    class StringManip {
	    public:
		StringManip( std::ostream& (*f)(std::ostream&, const std::string& ), const std::string& s ) : _f(f), _s(s) {}
	    private:
		std::ostream& (*_f)( std::ostream&, const std::string& );
		const std::string& _s;

		friend std::ostream& operator<<(std::ostream & os, const StringManip& m ) { return m._f(os,m._s); }
	    };

	    class ImportModel
	    {
	    public:
		struct Match {
		    Match( const std::string& s ) : _s(s) {}
		    bool operator()( const std::pair<const char*,ImportModel>& p ) const;
		private:
		    const std::string& _s;
		};
		
	    public:
		ImportModel() : _f(0), _min_match(0) {}
		ImportModel( unsigned int min_match, set_object_fptr f ) : _f(f), _min_match(min_match) {}

		bool operator()( const char * s1, const char * s2 ) const { return strcasecmp( s1, s2 ) < 0; }
		bool operator()( const std::string&, Json_Document& document, const picojson::value& ) const;

		set_object_fptr getFptr() const { return _f; }

	    private:
		set_object_fptr _f;
		unsigned int _min_match;		/* Minimum string length that must match */
	    };

	public:
	    Json_Document();
	    static bool load( const std::string& filename );		// Factory.

	    /* ---------------------------------------------------------------- */

	public:
	    ~Json_Document();

	private:
	    Json_Document( const Json_Document& );
	    Json_Document& operator=( const Json_Document& );

	    static void init_tables();

	    bool parse( const std::string& file_name );

	    /* Prototypes for internally used functions */

	    void input_error( const char * fmt, ... ) const;

	    void handleModel();
	    void handleGeneral( const picojson::value& );
	    void handleProcessor( const picojson::value& );
	    void handleGroup( const picojson::value& );
	    void handleTask( const picojson::value& );
	    void handleEntry( const picojson::value& );
	    void handlePhase( const unsigned e, const picojson::value& value );
	    void handleTaskActivity( const picojson::value& );
	    void handleActivity( const std::string&, const picojson::value& );
	    void handleEntryCalls( const unsigned e, call_type t, const picojson::value& value );
	    void handlePhaseCalls( unsigned int e, unsigned int p, call_type t, const picojson::value& value ) const;
	    void handleActivityCalls( const unsigned int a, call_type t, const picojson::value& value ) const;
	    void handlePrecedence( const std::string&, const picojson::value& );

	    bool get_results( const picojson::value::object& results, const char * attribute, double& mean, double& conf_95 ) const;

	    void handleProcessorResults( unsigned int p, const picojson::value::object& ) const;
	    void handleGroupResults( unsigned int g, const picojson::value::object& results ) const;
	    void handleTaskResults( unsigned int t, const picojson::value::object& ) const;
	    void handleEntryResults( unsigned int e, const picojson::value::object& ) const;
	    void handlePhaseResults( unsigned int e, unsigned int p, const picojson::value::object& ) const;
	    void handlePhaseResults( unsigned int e, const picojson::value::array& results, phs_save_fptr f ) const;
	    void handleActivityResults( unsigned int a, const picojson::value::object& results ) const;
	    void handlePrecedenceResults( unsigned int first_activity, unsigned int last_activity, const picojson::value::object& results ) const;

	    void handleEntryCallResults( unsigned int e, unsigned int d, const picojson::value& value, call_save_fptr f ) const { (*f)( e, d, value ); }
	    void handlePhaseCallResults( unsigned int e, unsigned int p, unsigned int d, const picojson::value& value, ent_save_fptr f ) const { (*f)( e, p, d, value ); }
	    void handleActivityCallResults( unsigned int a, unsigned int d, const picojson::value& value, call_save_fptr f ) const { (*f)( a, d, value ); }

	    static const std::string get_string_attribute( const char *, const std::map<std::string, picojson::value>& );
	    static const double get_double_attribute( const char *, const std::map<std::string, picojson::value>&, double=-1.0 );
	    static const long get_long_attribute( const char *, const std::map<std::string, picojson::value>& );
	    static const bool get_bool_attribute( const char *, const std::map<std::string, picojson::value>& );
	    static const picojson::value::object& get_object_attribute( const char * name, const std::map<std::string, picojson::value>& obj );
	    static const picojson::value::array& get_array_attribute( const char * name, const std::map<std::string, picojson::value>& obj );
	    static const double get_time_attribute( const char *, const std::map<std::string, picojson::value>& );
	    static const picojson::value& get_value_attribute( const char *, const std::map<std::string, picojson::value>& );

	    static void save_entry_fwd_wait( unsigned int entry, unsigned int destination, const picojson::value& value );
	    static void save_entry_fwd_wait_var( unsigned int entry, unsigned int destination, const picojson::value& value );
	    static void save_entry_proc_waiting( unsigned int entry, unsigned int phase, const picojson::value& value );
	    static void save_entry_service( unsigned int entry, unsigned int phase, const picojson::value& value );
	    static void save_entry_utilization( unsigned int entry, unsigned int phase, const picojson::value& value );
	    static void save_entry_variance( unsigned int e, unsigned int p, const picojson::value& value );
	    static void save_phase_rnv_wait( unsigned int entry, unsigned int phase, unsigned int destination, const picojson::value& value );
	    static void save_phase_rnv_wait_var( unsigned int entry, unsigned int phase, unsigned int destination, const picojson::value& value );
	    static void save_phase_snr_wait( unsigned int entry, unsigned int phase, unsigned int destination, const picojson::value& value );
	    static void save_phase_snr_wait_var( unsigned int entry, unsigned int phase, unsigned int destination, const picojson::value& value );
	    static void save_activity_rnv_wait( unsigned int activity, unsigned int destination, const picojson::value& value );
	    static void save_activity_rnv_wait_var( unsigned int activity, unsigned int destination, const picojson::value& value );
	    static void save_activity_snr_wait( unsigned int activity, unsigned int destination, const picojson::value& value );
	    static void save_activity_snr_wait_var( unsigned int activity, unsigned int destination, const picojson::value& value );

	    static IntegerManip indent( int i ) { return IntegerManip( &printIndent, i ); }
	    static AttributeManip begin_attribute( const std::string& s, const picojson::value& v ) { return AttributeManip( beginAttribute, s, v ); }
	    static AttributeManip end_attribute( const std::string& s, const picojson::value& v ) { return AttributeManip( endAttribute, s, v ); }
	    static AttributeManip attribute( const std::string& s, const picojson::value& v ) { return AttributeManip( printAttribute, s, v ); }
	    static StringManip begin_object( const std::string& name ) { return StringManip( &printObjectBegin, name ); }
	    static SimpleManip end_object() { return SimpleManip( &printObjectEnd ); }

	    static std::ostream& beginAttribute( std::ostream&, const std::string&, const picojson::value& );
	    static std::ostream& endAttribute( std::ostream&, const std::string&, const picojson::value& );
	    static std::ostream& printAttribute( std::ostream&, const std::string&, const picojson::value& );
	    static std::ostream& printIndent( std::ostream&, int );
	    static std::ostream& printObjectBegin( std::ostream&, const std::string& );
	    static std::ostream& printObjectEnd( std::ostream& );

	public:
	    static bool __debugJSON;

	private:
	    picojson::value _dom;
	    ConfidenceIntervals _conf_95;
	    static int __indent;

	    static std::map<const char*,Json_Document::ImportModel,Json_Document::ImportModel>  model_table;

            static const char * Xactivity;
            static const char * Xand_fork;
            static const char * Xand_join;
            static const char * Xasynch_call;
            static const char * Xbegin;
            static const char * Xcap;
            static const char * Xcoeff_of_var_sq;
            static const char * Xcomment;
            static const char * Xconf_95;
            static const char * Xconf_99;
            static const char * Xconv_val;
            static const char * Xconv_val_result;
            static const char * Xcore;
            static const char * Xdeterministic;
            static const char * Xelapsed_time;
            static const char * Xend;
            static const char * Xentry;
            static const char * Xfanin;
            static const char * Xfanout;
            static const char * Xfaults;
            static const char * Xforwarding;
            static const char * Xgeneral;
            static const char * Xgroup;
            static const char * Xhistogram;
            static const char * Xhistogram_bin;
            static const char * Xinitially;
            static const char * Xit_limit;
            static const char * Xiterations;
            static const char * Xjoin_variance;
            static const char * Xjoin_waiting;
            static const char * Xloop;
            static const char * Xlqx;
            static const char * Xmax;
            static const char * Xmax_service_time;
            static const char * Xmean;
            static const char * Xmean_calls;
            static const char * Xmin;
            static const char * Xmultiplicity;
            static const char * Xmva_info;
            static const char * Xname;
            static const char * Xnumber_bins;
            static const char * Xopen_arrival_rate;
            static const char * Xopen_wait_time;
            static const char * Xor_fork;
            static const char * Xor_join;
            static const char * Xoverflow_bin;
	    static const char * Xphase;
            static const char * Xphase_type_flag;
            static const char * Xphase_utilization;
            static const char * Xplatform_info;
            static const char * Xpost;
            static const char * Xpragma;
            static const char * Xpre;
            static const char * Xprecedence;
            static const char * Xprint_int;
            static const char * Xpriority;
            static const char * Xprob;
            static const char * Xprob_exceed_max;
            static const char * Xproc_utilization;
            static const char * Xproc_waiting;
            static const char * Xprocessor;
            static const char * Xquantum;
            static const char * Xqueue_length;
            static const char * Xquorum;
            static const char * Xr_lock;
            static const char * Xr_unlock;
            static const char * Xreplication;
            static const char * Xreply_to;
            static const char * Xresult95;
            static const char * Xresult;
            static const char * Xrwlock;
            static const char * Xrwlock_reader_holding;
            static const char * Xrwlock_reader_holding_variance;
            static const char * Xrwlock_reader_utilization;
            static const char * Xrwlock_reader_waiting;
            static const char * Xrwlock_reader_waiting_variance;
            static const char * Xrwlock_writer_holding;
            static const char * Xrwlock_writer_holding_variance;
            static const char * Xrwlock_writer_utilization;
            static const char * Xrwlock_writer_waiting;
            static const char * Xrwlock_writer_waiting_variance;
            static const char * Xscheduling;
            static const char * Xsemaphore;
            static const char * Xsemaphore_utilization;
            static const char * Xsemaphore_waiting;
            static const char * Xsemaphore_waiting_variance;
            static const char * Xservice_time;
            static const char * Xservice_time_variance;
            static const char * Xservice_type;
            static const char * Xshare;
            static const char * Xsignal;
	    static const char * Xsolver_info;
            static const char * Xspeed_factor;
            static const char * Xsquared_coeff_variation;
            static const char * Xstart_activity;
            static const char * Xstep;
            static const char * Xstep_squared;
            static const char * Xsubmodels;
            static const char * Xsynch_call;
            static const char * Xsystem_cpu_time;
            static const char * Xtask;
            static const char * Xthink_time;
            static const char * Xthroughput;
            static const char * Xthroughput_bound;
            static const char * Xunderflow_bin;
            static const char * Xunderrelax_coeff;
            static const char * Xuser_cpu_time;
            static const char * Xutilization;
            static const char * Xvalid;
            static const char * Xw_lock;
            static const char * Xw_unlock;
            static const char * Xwait;
            static const char * Xwait_squared;
            static const char * Xwaiting;
            static const char * Xwaiting_variance;
	};

    }
}
#endif /* */
