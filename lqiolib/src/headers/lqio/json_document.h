/* -*- C++ -*-
 *  $Id: json_document.h 15836 2022-08-15 21:18:20Z greg $
 *
 *  Created by Greg Franks.
 */

#ifndef __LQIO_JSON_DOCUMENT__
#define __LQIO_JSON_DOCUMENT__

#include <cmath>
#include "picojson.h"
#include "dom_document.h"
#include "dom_actlist.h"
#include "dom_call.h"
#include "common_io.h"
#include "input.h"
#include "srvn_spex.h"

namespace LQIO {
    namespace DOM {
	class ActivityList;
	class Call;
	class DocumentObject;
	class Entity;
	class Entry;
	class ExternalVariable;
	class Group;
	class Histogram;
	class Phase;
	class Processor;
	class Task;

	class JSON_Document : private Common_IO {
	public:
	    class ImportEntry;
	    class ImportActivity;
	    class ImportResult;

	    enum class dom_type { DOM_NULL, BOOLEAN, CLOCK, DOUBLE, DOM_EXTVAR, LONG, UNSIGNED, STRING, DOM_ACTIVITY, DOM_GROUP, DOM_PROCESSOR, JSON_OBJECT };
	    typedef void (DOM::Task::*fan_in_out_fptr)( const std::string&, const ExternalVariable* );

	private:
	    friend class LQIO::DOM::Document;
	  
	    JSON_Document( Document& document, const std::string&, bool create_objects=true, bool load_results=false );

	public:
	    static bool load( Document&, const std::string& filename, unsigned int & errorCode, const bool load_results  );		// Factory.
	    static bool loadResults( Document& document, const std::string& filename );


	    /* ---------------------------------------------------------------- */

	public:
	    ~JSON_Document();

	    bool hasResults() const { return _document.hasResults(); }

	    void serializeDOM( std::ostream& ) const;

	    Document& getDocument() const { return _document; }

	private:
	    JSON_Document( const JSON_Document& ) = delete;
	    JSON_Document& operator=( const JSON_Document& ) = delete;

	    bool parse();

	    /* Prototypes for internally used functions */

	    void input_error( const char * fmt, ... ) const;

	    void handleModel();
	    void handleComment( DocumentObject *, const picojson::value& );
	    void handleParameters( DocumentObject *, const picojson::value& );
	    void handlePragma( DocumentObject *, const picojson::value& );
	    void handleGeneral( DocumentObject *, const picojson::value& );
	    void handleProcessor( DocumentObject *, const picojson::value& );
	    void handleGroup( DocumentObject *, const picojson::value& );
	    void handleTask( DocumentObject *, const picojson::value& );
	    void handleFanInOut( DocumentObject *, const picojson::value&, fan_in_out_fptr );
	    void handleEntry( DocumentObject *, const picojson::value& );
	    void handlePhase( DocumentObject *, const picojson::value& );
	    void handleTaskActivity( DocumentObject *, const picojson::value& );
	    void handleActivity( DocumentObject *, const picojson::value& );
	    void handlePrecedence( DocumentObject *, const picojson::value& );
	    void handleCall( DocumentObject *, const picojson::value&, Call::Type call_type );
	    void handleReplyList( DocumentObject *, const picojson::value& );
	    void handleResults( DocumentObject *, const picojson::value& );
	    void handleGeneralResult( DocumentObject *, const picojson::value& );
	    void handleGeneralObservation( DocumentObject *, const picojson::value& );
	    void handleMvaInfo( Document *, const picojson::value& );
	    void handleObservation( DocumentObject *, const picojson::value& );
	    void handleConvergence( DocumentObject *, const picojson::value& );
	    void handleResult( DocumentObject *, const picojson::value& );
	    void handleHistogram( DocumentObject *, const picojson::value& );
	    void handleHistogramBin( DocumentObject *, unsigned int, const picojson::value& );
	    void handleMaxServiceTime( DocumentObject *, const picojson::value& );
	  
	    void handleResults( DocumentObject *, const std::map<std::string, picojson::value>& );

	    void connectEntry( Entry *, Task *, const std::string& name );

	    static bool has_attribute( const char *, const std::map<std::string, picojson::value>& );
	    static const std::string get_string_attribute( const char *, const std::map<std::string, picojson::value>& );
	    static double get_double_attribute( const char *, const std::map<std::string, picojson::value>& );
	    static long get_long_attribute( const char *, const std::map<std::string, picojson::value>& );
	    static const picojson::value& get_value_attribute( const char *, const std::map<std::string, picojson::value>& );
	    static scheduling_type get_scheduling_attribute( const std::map<std::string, picojson::value>&, const scheduling_type );
	    static unsigned int get_unsigned_int( const picojson::value& );
	    static unsigned int get_opt_phase( const picojson::value::object& );
	    ExternalVariable * get_external_variable( const picojson::value& ) const;

	    static bool has_activities( const std::pair<std::string,Task *>& tp );
	    static bool has_synch_call( const Call * call ) { return call->hasRendezvous(); }
	    static bool has_asynch_call( const Call * call ) { return call->hasSendNoReply(); }

	    /* ---------------------------------------------------------------- */
	    /* Importing from picojson						*/
	    /* ---------------------------------------------------------------- */

	public:
	    typedef Document& (Document::*doc_bool_fptr)( bool );
	    typedef Document& (Document::*doc_double_fptr)( double );
	    typedef Document& (Document::*doc_extvar_fptr)( const ExternalVariable * );
	    typedef Document& (Document::*doc_long_fptr)( long );
	    typedef Document& (Document::*doc_string_fptr)( const std::string& );
	    typedef Document& (Document::*doc_unsigned_fptr)( unsigned );
	    typedef void (Call::*call_extvar_fptr)( const ExternalVariable * );
	    typedef void (DocumentObject::*obj_string_fptr)( const std::string& );
	    typedef void (Entity::*entity_double_fptr)( double );
	    typedef void (Entity::*entity_unsigned_fptr)( unsigned );
	    typedef void (Entry::*entry_activity_fptr)( Activity * );
	    typedef void (Entry::*entry_extvar_fptr)( const ExternalVariable * );
	    typedef void (Group::*group_bool_fptr)( bool );
	    typedef void (Group::*group_extvar_fptr)( const ExternalVariable * );
	    typedef void (Group::*group_proc_fptr)( Processor * );
	    typedef void (Phase::*phase_extvar_fptr)( const ExternalVariable * );
	    typedef void (Phase::*phase_type_fptr)( Phase::Type );
	    typedef void (Processor::*proc_extvar_fptr)( const ExternalVariable * );
	    typedef void (Task::*task_extvar_fptr)( const ExternalVariable * );
	    typedef void (Task::*task_unsigned_fptr)( unsigned );
	    typedef void (Task::*task_group_fptr)( Group * );
	    typedef void (Task::*task_proc_fptr)( Processor * );
	    typedef void (JSON_Document::*set_object_fptr)( DocumentObject*, const picojson::value& );
	    typedef void (JSON_Document::*set_docobj_fptr)( Document*, const picojson::value& );
	    typedef void (JSON_Document::*call_type_fptr)( DocumentObject*, const picojson::value&, Call::Type );
	    typedef void (JSON_Document::*task_fan_in_out_fptr)( DocumentObject*, const picojson::value&, fan_in_out_fptr );
	    typedef DocumentObject& (DocumentObject::*set_result_fptr)( const double );
	    typedef DocumentObject& (DocumentObject::*set_result_ph_fptr)( unsigned int, const double );

	    union set_fptr {
                set_fptr( void * _v_ ) : v(_v_) {}
                set_fptr( call_extvar_fptr _ca_e_ ) : ca_e(_ca_e_) {}
                set_fptr( call_type_fptr _ca_t_ ) : ca_t(_ca_t_) {}
                set_fptr( doc_bool_fptr _db_ ) : db(_db_) {}
                set_fptr( doc_double_fptr _dd_ ) : dd(_dd_) {}
		set_fptr( doc_long_fptr _dl_ ) : dl(_dl_) {}
                set_fptr( doc_extvar_fptr _de_ ) : de(_de_) {}
                set_fptr( doc_string_fptr _ds_ ) : ds(_ds_) {}
                set_fptr( doc_unsigned_fptr _du_ ) : du(_du_) {}
                set_fptr( entity_double_fptr _et_d_ ) : et_d(_et_d_) {}
                set_fptr( entity_unsigned_fptr _et_u_ ) : et_u(_et_u_) {}
                set_fptr( entry_activity_fptr _en_a_ ) : en_a(_en_a_) {}
                set_fptr( entry_extvar_fptr _en_e_ ) : en_e(_en_e_) {}
                set_fptr( group_bool_fptr _gr_b_ ) : gr_b(_gr_b_) {}
                set_fptr( group_extvar_fptr _gr_e_ ) : gr_e(_gr_e_) {}
                set_fptr( group_proc_fptr _gr_p_ ) : gr_p(_gr_p_) {}
                set_fptr( obj_string_fptr _os_ ) : os(_os_) {}
                set_fptr( phase_extvar_fptr _ph_e_ ) : ph_e(_ph_e_) {}
                set_fptr( phase_type_fptr _ph_t_ ) : ph_t(_ph_t_) {}
                set_fptr( proc_extvar_fptr _pr_e_ ) : pr_e(_pr_e_) {}
                set_fptr( set_docobj_fptr _doc_ ) : doc(_doc_) {}
                set_fptr( set_object_fptr _o_ ) : o(_o_) {}
                set_fptr( set_result_fptr _re_d_ ) : re_d(_re_d_) {}
                set_fptr( set_result_ph_fptr _rp_d_ ) : rp_d(_rp_d_) {}
                set_fptr( task_extvar_fptr _ta_e_ ) : ta_e(_ta_e_) {}
                set_fptr( task_fan_in_out_fptr _ta_f_ ) : ta_f(_ta_f_) {}
                set_fptr( task_group_fptr _ta_g_ ) : ta_g(_ta_g_) {}
                set_fptr( task_proc_fptr _ta_p_ ) : ta_p(_ta_p_) {}
                set_fptr( task_unsigned_fptr _ta_u_ ) : ta_u(_ta_u_) {}
		set_fptr() : v(nullptr) {}
                void * v;
                call_extvar_fptr ca_e;
                call_type_fptr ca_t;
                doc_bool_fptr db;
                doc_double_fptr dd;
		doc_long_fptr dl;
                doc_extvar_fptr de;
                doc_string_fptr ds;
                doc_unsigned_fptr du;
                entity_double_fptr et_d;
                entity_unsigned_fptr et_u;
                entry_activity_fptr en_a;
                entry_extvar_fptr en_e;
                group_bool_fptr gr_b;
                group_extvar_fptr gr_e;
                group_proc_fptr gr_p;
                obj_string_fptr os;
                phase_extvar_fptr ph_e;
                phase_type_fptr ph_t;
                proc_extvar_fptr pr_e;
                set_docobj_fptr doc;
                set_object_fptr o;
                set_result_fptr re_d;
                set_result_ph_fptr rp_d;
                task_extvar_fptr ta_e;
                task_fan_in_out_fptr ta_f;
                task_group_fptr ta_g;
                task_proc_fptr ta_p;
                task_unsigned_fptr ta_u;
	    };

	    class Import {
		friend JSON_Document::JSON_Document( Document&, const std::string&, bool, bool );

	    private:
		class AttributeManip {
		public:
		    AttributeManip( std::ostream& (*f)(std::ostream&, const std::string&, const picojson::value& ), const std::string& a, const picojson::value& v ) : _f(f), _a(a), _v(v) {}
		private:
		    std::ostream& (*_f)( std::ostream&, const std::string&, const picojson::value& );
		    const std::string& _a;
		    const picojson::value& _v;

		    friend std::ostream& operator<<(std::ostream & os, const AttributeManip& m ) { return m._f(os,m._a,m._v); }
		};

	    public:
                Import( call_extvar_fptr f )    : _t(dom_type::DOM_EXTVAR),    _f(f) {}
                Import( call_type_fptr f )      : _t(dom_type::JSON_OBJECT),   _f(f) {}
                Import( doc_bool_fptr f )       : _t(dom_type::BOOLEAN),       _f(f) {}
                Import( doc_double_fptr f )     : _t(dom_type::DOUBLE),        _f(f) {}
                Import( doc_long_fptr f )       : _t(dom_type::LONG),          _f(f) {}
                Import( doc_extvar_fptr f )     : _t(dom_type::DOM_EXTVAR),    _f(f) {}
                Import( doc_string_fptr f )     : _t(dom_type::STRING),        _f(f) {}
                Import( doc_unsigned_fptr f )   : _t(dom_type::UNSIGNED),      _f(f) {}
                Import( entity_double_fptr f )  : _t(dom_type::DOUBLE),        _f(f) {}
                Import( entity_unsigned_fptr f ): _t(dom_type::UNSIGNED),      _f(f) {}
                Import( entry_activity_fptr f ) : _t(dom_type::DOM_ACTIVITY),  _f(f) {}
                Import( entry_extvar_fptr f )   : _t(dom_type::DOM_EXTVAR),    _f(f) {}
                Import( group_bool_fptr f )     : _t(dom_type::BOOLEAN),       _f(f) {}
                Import( group_extvar_fptr f )   : _t(dom_type::DOM_EXTVAR),    _f(f) {}
                Import( group_proc_fptr f )     : _t(dom_type::DOM_PROCESSOR), _f(f) {}
                Import( obj_string_fptr f )     : _t(dom_type::STRING),        _f(f) {}
                Import( phase_extvar_fptr f )   : _t(dom_type::DOM_EXTVAR),    _f(f) {}
                Import( phase_type_fptr f )     : _t(dom_type::BOOLEAN),       _f(f) {}
                Import( proc_extvar_fptr f )    : _t(dom_type::DOM_EXTVAR),    _f(f) {}
                Import( set_docobj_fptr f )     : _t(dom_type::JSON_OBJECT),   _f(f) {}
                Import( set_object_fptr f )     : _t(dom_type::JSON_OBJECT),   _f(f) {}
                Import( set_result_fptr f )     : _t(dom_type::DOUBLE),        _f(f) {}
                Import( task_extvar_fptr f )    : _t(dom_type::DOM_EXTVAR),    _f(f) {}
                Import( task_group_fptr f )     : _t(dom_type::DOM_GROUP),     _f(f) {}
                Import( task_proc_fptr f )      : _t(dom_type::DOM_PROCESSOR), _f(f) {}
                Import( task_unsigned_fptr f )  : _t(dom_type::UNSIGNED),      _f(f) {}
                Import( task_fan_in_out_fptr f ): _t(dom_type::JSON_OBJECT),   _f(f) {}
		Import( dom_type t, const doc_double_fptr f ) : _t(t), _f(f) {}
		Import() : _t(dom_type::DOM_NULL),  _f() { }
		virtual ~Import() {}

		dom_type getType() const { return _t; }
		set_fptr getFptr() const { return _f; }

	    public:
		static IntegerManip indent( int i ) { return IntegerManip( &printIndent, i ); }
		static AttributeManip begin_attribute( const std::string& s, const picojson::value& v ) { return AttributeManip( beginAttribute, s, v ); }
		static AttributeManip end_attribute( const std::string& s, const picojson::value& v ) { return AttributeManip( endAttribute, s, v ); }

		static std::ostream& beginAttribute( std::ostream&, const std::string&, const picojson::value& );
		static std::ostream& beginAttribute( std::ostream&, const picojson::value& );
		static std::ostream& endAttribute( std::ostream&, const picojson::value& );
		static std::ostream& endAttribute( std::ostream&, const std::string&, const picojson::value& );
		static std::ostream& printIndent( std::ostream&, int );

	    protected:
		static int __indent;

	    private:
		const dom_type _t;
		const set_fptr _f;
	    };

	    class ImportModel : public Import
	    {
	    public:
		ImportModel() : Import() {}
		ImportModel( set_object_fptr f ) : Import( f ) {}
		bool operator()( const char * s1, const char * s2 ) const { return strcasecmp( s1, s2 ) < 0; }
		void operator()( const std::string& attribute, JSON_Document& document, const picojson::value& ) const;
	    };

	    class ImportGeneral : public Import
	    {
	    public:
		ImportGeneral() : Import() {}
		ImportGeneral( doc_string_fptr f ) : Import( f ) {}
		ImportGeneral( doc_extvar_fptr f ) : Import( f ) {}
		ImportGeneral( set_object_fptr f ) : Import( f ) {}

		bool operator()( const char * s1, const char * s2 ) const { return strcasecmp( s1, s2 ) < 0; }
		void operator()( const std::string& attribute, JSON_Document&, Document& document, const picojson::value& ) const;
	    };

	    class ImportProcessor : public Import
	    {
	    public:
		ImportProcessor() : Import() {}
		ImportProcessor( entity_double_fptr f )   : Import( f ) {}
		ImportProcessor( entity_unsigned_fptr f ) : Import( f ) {}
		ImportProcessor( obj_string_fptr f )	  : Import( f ) {}
		ImportProcessor( proc_extvar_fptr f )     : Import( f ) {}
		ImportProcessor( set_object_fptr f )	  : Import( f ) {}

		bool operator()( const char * s1, const char * s2 ) const { return strcasecmp( s1, s2 ) < 0; }
		void operator()( const std::string& attribute, JSON_Document&, Processor&, const picojson::value& ) const;
	    };

	    class ImportGroup : public Import
	    {
	    public:
		ImportGroup() : Import() {}
                ImportGroup( group_bool_fptr f )        : Import( f ) {}
                ImportGroup( group_extvar_fptr f )      : Import( f ) {}
                ImportGroup( group_proc_fptr f )        : Import( f ) {}
                ImportGroup( obj_string_fptr f )        : Import( f ) {}
                ImportGroup( set_object_fptr f )        : Import( f ) {}

		bool operator()( const char * s1, const char * s2 ) const { return strcasecmp( s1, s2 ) < 0; }
		void operator()( const std::string& attribute, JSON_Document&, Group&, const picojson::value& ) const;
	    };

	    class ImportTask : public Import
	    {
	    public:
		ImportTask() : Import(), _g(0) {}
		ImportTask( entity_double_fptr f )     	: Import( f ), _g(0) {}
		ImportTask( obj_string_fptr f )   	: Import( f ), _g(0) {}
		ImportTask( set_object_fptr f )      	: Import( f ), _g(0) {}		/* For entry */
		ImportTask( task_extvar_fptr f )     	: Import( f ), _g(0) {}
		ImportTask( task_group_fptr f )		: Import( f ), _g(0) {}
		ImportTask( task_proc_fptr f )		: Import( f ), _g(0) {}
		ImportTask( task_unsigned_fptr f ) 	: Import( f ), _g(0) {}
		ImportTask( task_fan_in_out_fptr f, fan_in_out_fptr g ) : Import( f ), _g(g) {}

		bool operator()( const char * s1, const char * s2 ) const { return strcasecmp( s1, s2 ) < 0; }
		void operator()( const std::string& attribute, JSON_Document&, Task&, const picojson::value& ) const;

		fan_in_out_fptr getGptr() const { return _g; }

	    private:
		fan_in_out_fptr _g;
	    };

	    class ImportEntry : public Import
	    {
	    public:
		ImportEntry() : Import(), _call_type(Call::Type::NULL_CALL) {}
		ImportEntry( entity_double_fptr f )   	  : Import( f ), _call_type(Call::Type::NULL_CALL) {}
		ImportEntry( entity_unsigned_fptr f ) 	  : Import( f ), _call_type(Call::Type::NULL_CALL) {}
		ImportEntry( entry_activity_fptr f )  	  : Import( f ), _call_type(Call::Type::NULL_CALL) {}
		ImportEntry( entry_extvar_fptr f )    	  : Import( f ), _call_type(Call::Type::NULL_CALL) {}
		ImportEntry( obj_string_fptr f )   	  : Import( f ) {}
		ImportEntry( phase_extvar_fptr f )    	  : Import( f ), _call_type(Call::Type::NULL_CALL) {}
		ImportEntry( phase_type_fptr f )      	  : Import( f ), _call_type(Call::Type::NULL_CALL) {}
		ImportEntry( set_object_fptr f )      	  : Import( f ), _call_type(Call::Type::NULL_CALL) {}		/* For results */
		ImportEntry( call_type_fptr f, Call::Type t ) : Import( f ), _call_type(t) {}

		bool operator()( const char * s1, const char * s2 ) const { return strcasecmp( s1, s2 ) < 0; }
		void operator()( const std::string& attribute, bool, JSON_Document&, Entry&, const picojson::value& ) const;	/* Entry */
		void operator()( const std::string& attribute, JSON_Document&, Phase&, const picojson::value& ) const;		/* Phase */

		Call::Type getCallType() const { return _call_type; }

	    private:
		Call::Type _call_type;
	    };

	    class ImportPhase : public Import
	    {
	    public:
		ImportPhase() : Import(), _call_type(Call::Type::NULL_CALL) {}
		ImportPhase( obj_string_fptr f )   	  : Import( f ) {}
		ImportPhase( phase_extvar_fptr f ) 	  : Import( f ), _call_type(Call::Type::NULL_CALL) {}
		ImportPhase( phase_type_fptr f )   	  : Import( f ), _call_type(Call::Type::NULL_CALL) {}
		ImportPhase( set_object_fptr f )   	  : Import( f ), _call_type(Call::Type::NULL_CALL) {}		/* For reply list */
		ImportPhase( call_type_fptr f, Call::Type t ) : Import( f ), _call_type(t) {}

		bool operator()( const char * s1, const char * s2 ) const { return strcasecmp( s1, s2 ) < 0; }
		void operator()( const std::string& attribute, JSON_Document&, Phase&, const picojson::value& ) const;

		Call::Type getCallType() const { return _call_type; }

	    private:
		Call::Type _call_type;
	    };

	    class ImportCall : public Import
	    {
	    public:
		ImportCall() : Import() {}
		ImportCall( call_extvar_fptr f ) : Import( f ) {}
		ImportCall( set_object_fptr f )  : Import( f ) {}
		
		bool operator()( const char * s1, const char * s2 ) const { return strcasecmp( s1, s2 ) < 0; }
		void operator()( const std::string& attribute, JSON_Document&, Call&, const picojson::value& ) const;
		void operator()( const std::string& attribute, JSON_Document&, Entry&, const picojson::value& ) const;
	    };
		
	    class ImportTaskActivity : public Import
	    {
	    public:
		ImportTaskActivity() : Import() {}
                ImportTaskActivity( set_object_fptr f ) : Import( f ) {}

		bool operator()( const char * s1, const char * s2 ) const { return strcasecmp( s1, s2 ) < 0; }
		void operator()( const std::string& attribute, JSON_Document&, Task *, const picojson::value& ) const;
	    };

	    class ImportActivity : public Import
	    {
	    public:
		ImportActivity() : Import(), _call_type(Call::Type::NULL_CALL) {}
		ImportActivity( obj_string_fptr f )   	  : Import( f ) {}
		ImportActivity( phase_extvar_fptr f ) 	  : Import( f ), _call_type(Call::Type::NULL_CALL) {}
		ImportActivity( phase_type_fptr f )   	  : Import( f ), _call_type(Call::Type::NULL_CALL) {}
		ImportActivity( set_object_fptr f )   	  : Import( f ), _call_type(Call::Type::NULL_CALL) {}		/* For reply list */
		ImportActivity( call_type_fptr f, Call::Type t ) : Import( f ), _call_type(t) {}

		bool operator()( const char * s1, const char * s2 ) const { return strcasecmp( s1, s2 ) < 0; }
		void operator()( const std::string& attribute, JSON_Document&, Activity&, const picojson::value& ) const;

		Call::Type getCallType() const { return _call_type; }

	    private:
		Call::Type _call_type;
	    };

	    class ImportPrecedence : public Import
	    {
	    public:
		ImportPrecedence() : Import() {}
		ImportPrecedence( ActivityList::Type t ) : Import(), _precedence_type(t) {}

		bool operator()( const char * s1, const char * s2 ) const { return strcasecmp( s1, s2 ) < 0; }
		void operator()( const std::string& attribute, JSON_Document&, ActivityList&, const picojson::value& ) const;

		ActivityList::ActivityList::Type precedence_type() const { return _precedence_type; }

	    private:
		ActivityList::Type _precedence_type;
	    };

	    class ImportGeneralObservation : public Import
	    {
	    public:
		ImportGeneralObservation() : Import(), _key(0) {}
		ImportGeneralObservation( int key ) : Import(), _key(key) {}

		bool operator()( const char * s1, const char * s2 ) const { return strcasecmp( s1, s2 ) < 0; }
		void operator()( const std::string& attribute, JSON_Document&, Document& document, const picojson::value& ) const;
		int getKey() const { return _key; }

	    private:
		int _key;
	    };

	    class ImportGeneralResult : public Import
	    {
	    public:
		ImportGeneralResult() : Import() {}
		ImportGeneralResult( doc_bool_fptr f )     : Import( f ) {}
		ImportGeneralResult( doc_double_fptr f )   : Import( f ) {}
		ImportGeneralResult( doc_extvar_fptr f )   : Import( f ) {}
		ImportGeneralResult( doc_long_fptr f )     : Import( f ) {}
		ImportGeneralResult( doc_string_fptr f )   : Import( f ) {}
		ImportGeneralResult( doc_unsigned_fptr f ) : Import( f ) {}
		ImportGeneralResult( set_docobj_fptr f )   : Import( f ) {}		/* For mva info */
		ImportGeneralResult( dom_type t, const doc_double_fptr f ) : Import(t,f) {}

		bool operator()( const char * s1, const char * s2 ) const { return strcasecmp( s1, s2 ) < 0; }
		void operator()( const std::string& attribute, JSON_Document&, Document& document, const picojson::value& ) const;
	    };

	    class ImportObservation : public Import
	    {
	    public:
		ImportObservation() : Import(), _key(0) {}
		ImportObservation( int key ) : Import(), _key(key) {}

		bool operator()( const char * s1, const char * s2 ) const { return strcasecmp( s1, s2 ) < 0; }
		void operator()( const std::string& attribute, JSON_Document&, DocumentObject&, unsigned int, const picojson::value& ) const;
		void operator()( const std::string& attribute, JSON_Document&, Entry&, unsigned int, Entry&, const picojson::value& ) const;
		int getKey() const { return _key; }

	    private:
		void addObservation( DocumentObject * object, unsigned int phase, int key, const char * var1, unsigned int conf = 0, const char * var2 = 0 ) const;

	    private:
		int _key;
	    };
	  
	    class ImportResult : public Import
	    {
	    public:
		ImportResult() : Import(), _g(0), _f2(0), _g2(0) {}
		ImportResult( set_result_fptr f, set_result_fptr g=nullptr, set_result_ph_fptr f2=nullptr, set_result_ph_fptr g2=nullptr ) : Import(f), _g(g), _f2(f2), _g2(g2) {}

		bool operator()( const char * s1, const char * s2 ) const { return strcasecmp( s1, s2 ) < 0; }
		void operator()( const std::string& attribute, JSON_Document&, DocumentObject&, const picojson::value& ) const;

		set_result_fptr getGptr() const { return _g; }
		set_result_ph_fptr getF2ptr() const { return _f2; }
		set_result_ph_fptr getG2ptr() const { return _g2; }

	    private:
		set_result_fptr _g;
		set_result_ph_fptr _f2;
		set_result_ph_fptr _g2;
	    };

	    class ImportHistogram : public Import
	    {
	    public:
		ImportHistogram() : Import() {}
		ImportHistogram( set_object_fptr f ) : Import(f) {}

		bool operator()( const char * s1, const char * s2 ) const { return strcasecmp( s1, s2 ) < 0; }
		void operator()( const std::string& attribute, JSON_Document&, Histogram&, const picojson::value& ) const;
	    };

	    static const std::map<const char*,const ImportModel,JSON_Document::ImportModel> model_table;
	    static const std::map<const char*,const ImportGeneral,JSON_Document::ImportGeneral> general_table;
	    static const std::map<const char*,const ImportProcessor,JSON_Document::ImportProcessor> processor_table;
	    static const std::map<const char*,const ImportGroup,JSON_Document::ImportGroup> group_table;
	    static const std::map<const char*,const ImportTask,JSON_Document::ImportTask> task_table;
	    static const std::map<const char*,const ImportEntry,JSON_Document::ImportEntry> entry_table;
	    static const std::map<const char*,const ImportPhase,JSON_Document::ImportPhase> phase_table;
	    static const std::map<const char*,const ImportCall,JSON_Document::ImportCall> call_table;
	    static const std::map<const char*,const ImportTaskActivity,JSON_Document::ImportTaskActivity> task_activity_table;
	    static const std::map<const char*,const ImportActivity,JSON_Document::ImportActivity> activity_table;
	    static const std::map<const char*,const ImportPrecedence,JSON_Document::ImportPrecedence> precedence_table;
	    static const std::map<const char*,const ImportGeneralObservation,JSON_Document::ImportGeneralObservation>  general_observation_table;
	    static const std::map<const char*,const ImportObservation,JSON_Document::ImportObservation> observation_table;
	    static const std::map<const char*,const ImportGeneralResult,JSON_Document::ImportGeneralResult> general_result_table;
	    static const std::map<const char*,const ImportResult,JSON_Document::ImportResult> result_table;
	    static const std::map<const char*,const ImportHistogram,JSON_Document::ImportHistogram> histogram_table;

	    static const std::map<const ActivityList::Type,const std::string>  precedence_type_table;
	    static const std::map<const Call::Type,const std::string> call_type_table;

	private:
	    class Export {
		friend JSON_Document::JSON_Document( Document&, const std::string&, bool, bool );

		class ResultManip {
		public:
		    ResultManip( std::ostream& (*f)(std::ostream&, const double, const LQIO::ConfidenceIntervals&, const double ), const double mean, const LQIO::ConfidenceIntervals& conf_95, const double variance ) : _f(f), _mean(mean), _conf_95(conf_95), _variance(variance) {}
		private:
		    std::ostream& (*_f)( std::ostream&, double, const LQIO::ConfidenceIntervals&, double );
		    const double _mean;
		    const LQIO::ConfidenceIntervals& _conf_95;
		    const double _variance;

		    friend std::ostream& operator<<(std::ostream & os, const ResultManip& m ) { return m._f(os,m._mean,m._conf_95,m._variance); }
		};

		class ObservationManip {
		public:
		    ObservationManip( std::ostream& (*f)(std::ostream&, const Spex::ObservationInfo& ), const Spex::ObservationInfo& obs ) : _f(f), _obs(obs) {}
		private:
		    std::ostream& (*_f)( std::ostream&, const Spex::ObservationInfo& );
		    const Spex::ObservationInfo& _obs;

		    friend std::ostream& operator<<(std::ostream & os, const ObservationManip& m ) { return m._f(os,m._obs); }
		};

		class SeparatorManip {
		public:
		    SeparatorManip( std::ostream& (*f)(std::ostream&, const Export& ), const Export& self ) : _f(f), _self(self) {}
		    private:
		    std::ostream& (*_f)( std::ostream&, const Export& );
		    const Export& _self;

		    friend std::ostream& operator<<(std::ostream & os, const SeparatorManip& m ) { return m._f(os,m._self); }
		};

	    protected:
		struct CollectPhase {
		    CollectPhase( std::vector<Spex::ObservationInfo>& vars, unsigned int phase ) : _vars(vars), _phase(phase) { _vars.clear(); }	// Reset
		    void operator()( const std::pair<const DocumentObject *,const Spex::ObservationInfo>& p ) {
			const Spex::ObservationInfo& obs = p.second;
			if ( obs.getPhase() == _phase ) {
			    _vars.push_back(obs);
			}
		    }
		private:
		    std::vector<Spex::ObservationInfo>& _vars;
		    const unsigned int _phase;
		};

	    private:
		struct CollectKey {
		    CollectKey( std::map<unsigned int,const Spex::ObservationInfo>& vars, int key ) : _vars(vars), _key(key) { _vars.clear(); }	// Reset
		    void operator()( const std::pair<const DocumentObject *,const Spex::ObservationInfo>& p ) {
			const Spex::ObservationInfo& obs = p.second;
			if ( obs.getKey() == _key ) {
			    _vars.insert(std::pair<unsigned int,const Spex::ObservationInfo>(obs.getPhase(),obs));
			}
		    }
		private:
		    std::map<unsigned int,const Spex::ObservationInfo>& _vars;
		    const int _key;
		};

		class BooleanManip {
		public:
		    BooleanManip( std::ostream& (*f)(std::ostream&, const bool ), const bool b ) : _f(f), _b(b) {}
		private:
		    std::ostream& (*_f)( std::ostream&, const bool );
		    const bool _b;

		    friend std::ostream& operator<<(std::ostream & os, const BooleanManip& m ) { return m._f(os,m._b); }
		};

		class ExtvarManip {
		public:
		    ExtvarManip( std::ostream& (*f)(std::ostream&, const ExternalVariable& ), const ExternalVariable& v ) : _f(f), _v(v) {}
		private:
		    std::ostream& (*_f)( std::ostream&, const ExternalVariable& );
		    const ExternalVariable& _v;

		    friend std::ostream& operator<<(std::ostream & os, const ExtvarManip& m ) { return m._f(os,m._v); }
		};

		class StringExtvarManip {
		public:
		    StringExtvarManip( std::ostream& (*f)(std::ostream&, const std::string&, const ExternalVariable& ), const std::string& a, const ExternalVariable& v ) : _f(f), _a(a), _v(v) {}
		private:
		    std::ostream& (*_f)( std::ostream&, const std::string&, const ExternalVariable& );
		    const std::string& _a;
		    const ExternalVariable& _v;

		    friend std::ostream& operator<<(std::ostream & os, const StringExtvarManip& m ) { return m._f(os,m._a,m._v); }
		};

		class StringObservationManip {
		public:
		    StringObservationManip( std::ostream& (*f)(std::ostream&, const std::string&, const Spex::ObservationInfo& ), const std::string& a, const Spex::ObservationInfo& v ) : _f(f), _a(a), _v(v) {}
		private:
		    std::ostream& (*_f)( std::ostream&, const std::string&, const Spex::ObservationInfo& );
		    const std::string& _a;
		    const Spex::ObservationInfo& _v;

		    friend std::ostream& operator<<(std::ostream & os, const StringObservationManip& m ) { return m._f(os,m._a,m._v); }
		};

		class StringResultManip {
		public:
		    StringResultManip( std::ostream& (*f)(std::ostream&, const std::string&, const double, const LQIO::ConfidenceIntervals&, const double ), const std::string& s, const double mean, const LQIO::ConfidenceIntervals& conf_95, const double variance ) : _f(f), _s(s), _mean(mean), _conf_95(conf_95), _variance(variance) {}
		private:
		    std::ostream& (*_f)( std::ostream&, const std::string&, double, const LQIO::ConfidenceIntervals&, double );
		    const std::string& _s;
		    const double _mean;
		    const LQIO::ConfidenceIntervals& _conf_95;
		    const double _variance;

		    friend std::ostream& operator<<(std::ostream & os, const StringResultManip& m ) { return m._f(os,m._s,m._mean,m._conf_95,m._variance); }
		};

	    public:
		Export( std::ostream& output, const LQIO::ConfidenceIntervals& conf_95, unsigned count = 0 ) : _output(output), _conf_95(conf_95), _count(count) {}

	    public:
		static StringBooleanManip begin_array( const std::string& name, bool inline_array=false ) { return StringBooleanManip( &printArrayBegin, name, inline_array ); }
		static StringBooleanManip next_begin_array( const std::string& name, bool inline_array=false ) { return StringBooleanManip( &printNextArrayBegin, name, inline_array ); }
		static BooleanManip end_array( bool inline_array=false) { return BooleanManip( &printArrayEnd, inline_array ); }
		static StringManip begin_object( const std::string& name ) { return StringManip( &printObjectBegin, name ); }
		static StringManip next_begin_object( const std::string& name ) { return StringManip( &printNextObjectBegin, name ); }
		static SimpleManip begin_object() { return SimpleManip( &printObjectBegin ); }
		static SimpleManip end_object() { return SimpleManip( &printObjectEnd ); }
		static SimpleManip indent() { return SimpleManip( &printIndent ); }
		SeparatorManip separator() const { return SeparatorManip( &printSeparator, *this ); }
		static ExtvarManip print_value( const ExternalVariable& value ) { return ExtvarManip( &printValue, value ); }
		static ObservationManip print_value( const Spex::ObservationInfo& observation ) { return ObservationManip( &printValue, observation ); }
		static ResultManip print_value( const double mean, const LQIO::ConfidenceIntervals& conf_95, const double variance ) { return ResultManip( &printValue, mean, conf_95, variance ); }
		static StringDoubleManip attribute( const std::string& name, double value ) { return StringDoubleManip( &printAttribute, name, value ); }
		static StringExtvarManip attribute( const std::string& name, const ExternalVariable& value ) { return StringExtvarManip( &printAttribute, name, value ); }
		static StringObservationManip attribute( const std::string& name, const Spex::ObservationInfo& value ) { return StringObservationManip( &printAttribute, name, value ); }
		static StringResultManip attribute( const std::string& name, const double mean, const LQIO::ConfidenceIntervals& conf_95, const double variance ) { return StringResultManip( &printAttribute, name, mean, conf_95, variance ); }
		static StringStringManip attribute( const std::string& name, const std::string& value ) { return StringStringManip( &printAttribute, name, value ); }
		static StringBooleanManip next_attribute( const std::string& name, bool value ) { return StringBooleanManip( &printNextAttribute, name, value ); }
		static StringDoubleManip next_attribute( const std::string& name, double value ) { return StringDoubleManip( &printNextAttribute, name, value ); }
		static StringLongManip next_attribute( const std::string& name, long value ) { return StringLongManip( &printNextAttribute, name, value ); }
		static StringExtvarManip next_attribute( const std::string& name, const ExternalVariable& value ) { return StringExtvarManip( &printNextAttribute, name, value ); }
		static StringResultManip next_attribute( const std::string& name, const double mean, const LQIO::ConfidenceIntervals& conf_95, const double variance ) { return StringResultManip( &printNextAttribute, name, mean, conf_95, variance ); }
		static StringStringManip next_attribute( const std::string& name, const std::string& value ) { return StringStringManip( &printNextAttribute, name, value ); }
		static StringDoubleManip time_attribute( const std::string& name, const double value ) { return StringDoubleManip( &printTimeAttribute, name, value ); }
		static StringManip escape_string( const std::string& string ) { return StringManip( &escapeString, string ); }

	    private:
		static std::ostream& printIndent( std::ostream& );
		static std::ostream& printSeparator( std::ostream&, const Export& );
		static std::ostream& printArrayBegin( std::ostream&, const std::string&, bool );
		static std::ostream& printNextArrayBegin( std::ostream&, const std::string&, bool );
		static std::ostream& printArrayEnd( std::ostream&, bool );
		static std::ostream& printObjectBegin( std::ostream&, const std::string& );
		static std::ostream& printNextObjectBegin( std::ostream&, const std::string& );
		static std::ostream& printObjectBegin( std::ostream& );
		static std::ostream& printObjectEnd( std::ostream& );
		static std::ostream& printValue( std::ostream&, const ExternalVariable& );
		static std::ostream& printValue( std::ostream&, const Spex::ObservationInfo& );
		static std::ostream& printValue( std::ostream&, const double, const LQIO::ConfidenceIntervals& conf_95, const double );
		static std::ostream& printAttribute( std::ostream&, const std::string&, const ExternalVariable& );
		static std::ostream& printAttribute( std::ostream&, const std::string&, const Spex::ObservationInfo& );
		static std::ostream& printAttribute( std::ostream&, const std::string&, const double );
		static std::ostream& printAttribute( std::ostream&, const std::string&, const double, const LQIO::ConfidenceIntervals& conf_95, const double );
		static std::ostream& printAttribute( std::ostream&, const std::string&, const long );
		static std::ostream& printAttribute( std::ostream&, const std::string&, const std::string& );
		static std::ostream& printNextAttribute( std::ostream&, const std::string&, const ExternalVariable& );
		static std::ostream& printNextAttribute( std::ostream&, const std::string&, const bool );
		static std::ostream& printNextAttribute( std::ostream&, const std::string&, const double );
		static std::ostream& printNextAttribute( std::ostream&, const std::string&, const double, const LQIO::ConfidenceIntervals& conf_95, const double );
		static std::ostream& printNextAttribute( std::ostream&, const std::string&, const long );
		static std::ostream& printNextAttribute( std::ostream&, const std::string&, const std::string& );
		static std::ostream& printTimeAttribute( std::ostream&, const std::string&, const double );
		static std::ostream& escapeString( std::ostream&, const std::string& );

	    protected:
		std::ostream& _output;
		const LQIO::ConfidenceIntervals& _conf_95;

	    private:
		mutable unsigned int _count;
		static int __indent;
	    };

	    class Model : public Export {
	    public:
		Model( std::ostream& output, const LQIO::ConfidenceIntervals& conf_95 ) : Export( output, conf_95 ) {}

		void print( const Document& ) const;
	    };

	    class ExportPragma : public Export {
	    public:
		ExportPragma( std::ostream& output, const LQIO::ConfidenceIntervals& conf_95 ) : Export( output, conf_95 ) {}

		void operator()( const std::pair<std::string,std::string>& ) const;

	    private:
	    };

	    class ExportParameters : public Export {
	    public:
		ExportParameters( std::ostream& output, const LQIO::ConfidenceIntervals& conf_95 ) : Export( output, conf_95 ) {}

		void operator()( const std::pair<std::string,LQX::SyntaxTreeNode *>& var ) const;
	    };

	    class ExportGeneral : public Export {
	    public:
		ExportGeneral( std::ostream& output, const LQIO::ConfidenceIntervals& conf_95 ) : Export( output, conf_95 ) {}

		void print( const Document& ) const;
	    };

	    class ExportProcessor : public Export {
	    public:
		ExportProcessor( std::ostream& output, const LQIO::ConfidenceIntervals& conf_95 ) : Export( output, conf_95 ) {}
		void operator()( const std::pair<std::string,Processor *>& processor ) const { print( *(processor.second) ); }

	    private:
		void print( const Processor& ) const;
	    };

	    class ExportGroup : public Export {
		typedef void (ExportGroup::*void_fptr)( const Group& ) const;

	    public:
		ExportGroup( std::ostream& output, const LQIO::ConfidenceIntervals& conf_95 ) : Export( output, conf_95 ) {}
		void operator()( const Group * group ) const { print( *group ); }

		void print( const Group& ) const;
	    };

	    class ExportTask : public Export {
	    public:
		ExportTask( std::ostream& output, const LQIO::ConfidenceIntervals& conf_95 ) : Export( output, conf_95 ) {}
		void operator()( const std::pair<std::string,Task *>& task ) const { print( *(task.second) ); }
		void operator()( const Task * task ) const { print( *task ); }

	    private:
		void print( const Task& ) const;
	    };

	    class ExportHistogram;

	    class ExportEntry : public Export {
                typedef const ExternalVariable * (Phase::*extvar_fptr )() const;
                typedef double (Call::*double_call_fptr )() const;
                typedef double (Entry::*double_entry_fptr )( unsigned int ) const;
                typedef double (Phase::*double_phase_fptr )() const;
                typedef Phase::Type (Phase::*phtype_fptr )() const;
                typedef void (ExportEntry::*void_fptr)( const Entry& ) const;

		class EntryResultsManip {
		public:
		    EntryResultsManip( std::ostream& (*f)(std::ostream&, const LQIO::DOM::Entry&, const char *, const double_entry_fptr, const LQIO::ConfidenceIntervals&, const double_entry_fptr ),
				       const LQIO::DOM::Entry & e, const char * a, const double_entry_fptr m, const LQIO::ConfidenceIntervals& c, const double_entry_fptr v ) : _f(f), _e(e), _a(a), _m(m), _c(c), _v(v) {}
		private:
		    std::ostream& (*_f)( std::ostream&, const LQIO::DOM::Entry&, const char *, const double_entry_fptr, const LQIO::ConfidenceIntervals&, const double_entry_fptr );
		    const LQIO::DOM::Entry & _e;	/* entry */
		    const char * _a;			/* attribute */
		    const double_entry_fptr _m;		/* phase */
		    const LQIO::ConfidenceIntervals& _c;/* confidence intervals */
		    const double_entry_fptr _v;		/* phase */
		    friend std::ostream& operator<<(std::ostream & os, const EntryResultsManip& m ) { return m._f(os,m._e,m._a,m._m,m._c,m._v); }
		};

	    public:
		ExportEntry( std::ostream& output, const LQIO::ConfidenceIntervals& conf_95 ) : Export( output, conf_95 ) {}
		void operator()( const Entry * entry ) const { print( *entry ); }

		void print( const Entry& ) const;

	    protected:
		static EntryResultsManip entry_phase_results( const LQIO::DOM::Entry& e, const char * a, const double_entry_fptr m, const LQIO::ConfidenceIntervals& c, const double_entry_fptr v=NULL ) { return EntryResultsManip( &ExportEntry::printEntryPhaseResults, e, a, m, c, v ); }
		static std::ostream& printEntryPhaseResults( std::ostream& output, const LQIO::DOM::Entry & entry, const char * attribute, const double_entry_fptr mean, const LQIO::ConfidenceIntervals& conf_95, const double_entry_fptr variance );

	    private:
		void printPhase( const Entry& entry, const char * name, extvar_fptr ) const;
		void printPhase( const Entry& entry, const char * name, phtype_fptr ) const;
		void printPhase( const Entry& entry, const char * name, double_entry_fptr, const ConfidenceIntervals&, double_entry_fptr ) const;
		void printPhase( const Entry& entry, const char * name, const ConfidenceIntervals *, double_phase_fptr ) const;
		void printPhase( const Entry& entry, const char * name, const ExportHistogram& ) const;
		
		void printResult( const Entry& ) const;
	    };

	    class ExportPhaseActivity : public Export
	    {
	    protected:
		ExportPhaseActivity( std::ostream& output, const LQIO::ConfidenceIntervals& conf_95 ) : Export( output, conf_95 ) {}

	    protected:
		void printParameters( const Phase& ) const;
		void printResults( const Phase& ) const;
	    };

	    class ExportPhase : public ExportPhaseActivity {
	    public:
		ExportPhase( std::ostream& output, const LQIO::ConfidenceIntervals& conf_95 ) : ExportPhaseActivity( output, conf_95 ) {}
		void operator()( const std::pair<unsigned int,Phase *>& phase ) const { print( phase.first, *(phase.second) ); }

	    private:
		void print( unsigned int, const Phase& ) const;
	    };

	    class ExportCall : public Export {
 	    public:
                typedef double (Call::*double_call_fptr )() const;

	    public:
		ExportCall( std::ostream& output, Call::Type type, const LQIO::ConfidenceIntervals& conf_95 ) : Export( output, conf_95 ), _type(type) {}
		void operator()( const Call * call ) const;

	    private:
		void print( const char * attribute, const Call& call, double_call_fptr mean, double_call_fptr variance ) const;

	    private:
		const Call::Type _type;
	    };

	    class ExportActivity : public ExportPhaseActivity {
	    public:
		ExportActivity( std::ostream& output, const LQIO::ConfidenceIntervals& conf_95 ) : ExportPhaseActivity( output, conf_95 ) {}
		void operator()( const std::pair<std::string,Activity *>& activity ) const { print( *(activity.second) ); }

	    private:
		void print( const Activity& ) const;
	    };

	    class ExportPrecedence : public Export {
		typedef void (ExportPrecedence::*void_fptr)( const ActivityList& ) const;

	    public:
		ExportPrecedence( std::ostream& output, const LQIO::ConfidenceIntervals& conf_95 ) : Export( output, conf_95 ) {}
		void operator()( ActivityList * list ) const { print( *list ); }

	    private:
		void print( const ActivityList& ) const;
	    };

	    class ExportHistogram : public Export {
	    public:
		ExportHistogram( std::ostream& output, const LQIO::ConfidenceIntervals& conf_95 ) : Export( output, conf_95 ) {}
		void operator()( Histogram& ) const;

		void print( const Histogram& ) const;
	    };


	    class ExportFanInOut : public Export {
		typedef void (ExportFanInOut::*void_fptr)( const std::pair<const std::string, const ExternalVariable *>& ) const;

	    public:
		ExportFanInOut( std::ostream& output, const LQIO::ConfidenceIntervals& conf_95 ) : Export( output, conf_95 ) {}
		void operator()( const std::pair<const std::string, const ExternalVariable *>& fan_in_out ) const { print( fan_in_out ); }

	    private:
		void print( const std::pair<const std::string, const ExternalVariable *>& ) const;
	    };

	    class ExportInputVariables : public Export {
	    public:
		ExportInputVariables( std::ostream& output, const LQIO::ConfidenceIntervals& conf_95 ) : Export( output, conf_95 ) {}

		void operator()( const Spex::var_name_and_expr& ) const;
	    };

	    class ExportObservation : public Export {
	    public:
		ExportObservation( std::ostream& output, const LQIO::ConfidenceIntervals& conf_95 ) : Export( output, conf_95 ) {}
		void operator()( const std::pair<const DocumentObject *,const Spex::ObservationInfo>& obs ) const { print( obs.second ); }
		void operator()( const Spex::ObservationInfo& obs ) const { print( obs ); }

	    private:
		void print( const Spex::ObservationInfo& obs ) const;
	    };
	
	    class ExportResults : public Export {
	    public:
		ExportResults( std::ostream& output, const LQIO::ConfidenceIntervals& conf_95, unsigned int count ) : Export( output, conf_95, count ) {}

		void operator()( const Spex::var_name_and_expr& ) const;
	    };

	    /* ------------------------------------------------------------------------ */
	private:
	    picojson::value _dom;
	    Document& _document;
	    const std::string& _input_file_name;
	    bool _createObjects;
	    bool _loadResults;
	    static const std::map<const int,const char *> __key_lqx_function_map;			/* Maps srvn_gram.h KEY_XXX to lqx function name */

            static const char * Xactivity;
            static const char * Xand_fork;
            static const char * Xand_join;
            static const char * Xasynch_call;
            static const char * Xbegin;
	    static const char * Xbottleneck_strength;
            static const char * Xcap;
            static const char * Xcoeff_of_var_sq;
            static const char * Xcomment;
            static const char * Xconf_95;
            static const char * Xconf_99;
	    static const char * Xconvergence;
            static const char * Xconv_val;
            static const char * Xconv_val_result;
            static const char * Xcore;
	    static const char * Xdestination;
            static const char * Xdeterministic;
	    static const char * Xdrop_probability;
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
	    static const char * Xmarginal_queue_probabilities;
            static const char * Xmax;
	    static const char * Xmax_rss;
            static const char * Xmax_service_time;
	    static const char * Xmean;
	    static const char * Xmean_calls;
            static const char * Xmin;
            static const char * Xmultiplicity;
            static const char * Xmva_info;
            static const char * Xname;
            static const char * Xnumber_bins;
	    static const char * Xobserve;
            static const char * Xopen_arrival_rate;
            static const char * Xopen_wait_time;
            static const char * Xor_fork;
            static const char * Xor_join;
            static const char * Xoverflow_bin;
	    static const char * Xparameters;
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
            static const char * Xresults;
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
	    static const char * Xtotal;
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
