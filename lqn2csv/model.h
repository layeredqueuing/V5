/* -*- c++ -*-
 * $HeadURL: http://rads-svn.sce.carleton.ca:8080/svn/lqn/trunk/lqn2csv/model.h $
 *
 * Dimensions common to everything, plus some funky inline functions.
 *
 * Copyright the Real-Time and Distributed Systems Group,
 * Department of Systems and Computer Engineering,
 * Carleton University, Ottawa, Ontario, Canada. K1S 5B6
 *
 * October, 2021
 *
 * $Id: model.h 17560 2025-11-03 22:45:15Z greg $
 *
 * ------------------------------------------------------------------------
 */

#if !defined MODEL_H
#define MODEL_H
#include <map>
#include <filesystem>
#include <cmath>
#include <string>

#include <lqio/dom_document.h>

namespace LQIO {
    namespace DOM {
	class Activity;
	class Call;
	class Document;
	class DocumentObject;
	class Entry;
	class Phase;
	class Task;
    }
}

namespace Model {
    class Value;
}

namespace Model {
    enum class Mode { PATHNAME, FILENAME_STRIP, DIRECTORY_STRIP, DIRECTORY };

    class Object {
    public:
	enum class Type
	{
	    ACTIVITY,
	    ACTIVITY_CALL,
	    DOCUMENT,
	    ENTRY,
	    ENTITY,
	    JOIN,
	    PHASE,
	    PHASE_CALL,
	    PROCESSOR,
	    TASK
	};

	static const std::map<const Object::Type,const std::pair<const std::string,const std::string>> __object_type;
    };

    class Value
    {
    public:
	enum class Type { DOUBLE, STRING, PATHNAME };

	Value( double value ) : _type(Type::DOUBLE) { _u._double = value; }
	Value( const char * string ) : _type(Type::STRING) { _u._string = string; }
	Value( const std::filesystem::path& path );

	virtual ~Value() {}
	
	Value& operator=( const Value& src );
	bool operator!=( const Value& dst ) const;
	
	bool isnan() const { return _type == Type::DOUBLE && std::isnan( _u._double ); }

	const Type _type;
	union {
	    double _double;
	    const char * _string;
	} _u;
    };

    class Result {
    public:
	typedef double (LQIO::DOM::DocumentObject::*fptr)() const;
	typedef const std::string& (LQIO::DOM::Document::*sfptr)() const;
	
	enum class Type
	{
	    NONE,
	    ACTIVITY_DEMAND,
	    ACTIVITY_PR_RQST_LOST, 
	    ACTIVITY_PR_SVC_EXCD, 
	    ACTIVITY_PROCESSOR_WAITING,
	    ACTIVITY_REQUEST_RATE,
	    ACTIVITY_SERVICE,
	    ACTIVITY_THROUGHPUT,
	    ACTIVITY_VARIANCE,
	    ACTIVITY_WAITING,
	    COMMENT,
	    ENTRY_THROUGHPUT,
	    ENTRY_UTILIZATION, 
	    HOLD_TIMES, 
	    JOIN_DELAYS, 
	    MARGINAL_PROBABILITIES,
	    MVA_STEPS, 
	    OPEN_ARRIVAL_WAIT,
	    OPEN_ARRIVAL_RATE,
	    PHASE_DEMAND,
	    PHASE_PROCESSOR_WAITING,
	    PHASE_PR_RQST_LOST, 
	    PHASE_PR_SVC_EXCD, 
	    PHASE_REQUEST_RATE,
	    PHASE_SERVICE,
	    PHASE_UTILIZATION,
	    PHASE_VARIANCE,
	    PHASE_WAITING,
	    PROCESSOR_MULTIPLICITY,
	    PROCESSOR_UTILIZATION,
	    SOLVER_VERSION,
	    TASK_MULTIPLICITY,
	    TASK_THROUGHPUT,
	    TASK_UTILIZATION,
	    TASK_THINK_TIME,
	    THROUGHPUT_BOUND
	};



	struct result_fields {
	    Object::Type type;
	    const std::string name;
	    const std::string abbreviation;
	    fptr f;
	};
	
	typedef std::pair<std::string,Model::Result::Type> result_t;
	typedef const std::string& (*get_field)( const std::pair<std::string,Model::Result::Type>& result_t );

    private:
//	Result( const Result& ) = delete;
//	Result& operator=( const Result& ) = delete;

    public:
	Result( const LQIO::DOM::Document& dom ) : _dom(dom) {}
	std::vector<Model::Value> operator()( const std::vector<Model::Value>&, const std::pair<std::string,Model::Result::Type>& ) const;
	static std::string ObjectName( const std::string&, const std::pair<std::string,Model::Result::Type>& );
	static std::string TypeName( const std::string&, const std::pair<std::string,Model::Result::Type>& );
	static bool equal( Type, Type );
	static bool isIndependentVariable( Type );
	static bool isDependentVariable( Type );
	static void insertHeader( std::vector<std::vector<Model::Value>>& data, const std::string&, const std::vector<Model::Result::result_t>& results, Result::get_field f );

	static const std::string& getObjectType( const std::pair<std::string,Model::Result::Type>& );
	static const std::string& getObjectName( const std::pair<std::string,Model::Result::Type>& );
	static const std::string& getTypeName( const std::pair<std::string,Model::Result::Type>& );

    private:
	const LQIO::DOM::Document& dom() const { return _dom; }
	const LQIO::DOM::DocumentObject * findObject( const std::string& name, Model::Object::Type type ) const;
	const LQIO::DOM::Phase * findPhase( const LQIO::DOM::Entry *, const std::string& ) const;
	const LQIO::DOM::Call * findCall( const LQIO::DOM::Phase *, const std::string& ) const;
	const LQIO::DOM::Activity * findActivity( const LQIO::DOM::Task *, const std::string& ) const;

	static bool throughput( Type type ) { return type == Type::ACTIVITY_THROUGHPUT || type == Type::ENTRY_THROUGHPUT || type == Type::TASK_THROUGHPUT || type == Type::THROUGHPUT_BOUND; }
	static bool utilization( Type type ) { return type == Type::TASK_UTILIZATION || type == Type::ENTRY_UTILIZATION || type == Type::PHASE_UTILIZATION || type == Type::PROCESSOR_UTILIZATION; }
	static bool service_time( Type type ) { return type == Type::ACTIVITY_SERVICE || type == Type::PHASE_SERVICE; }
	static bool waiting( Type type ) { return type == Type::ACTIVITY_WAITING || type == Type::PHASE_WAITING || type == Type::PHASE_PROCESSOR_WAITING || type == Type::OPEN_ARRIVAL_WAIT; }
	static bool variance( Type type ) { return type == Type::PHASE_VARIANCE || type == Type::ACTIVITY_VARIANCE; }

    public:
	static const std::map<Type,result_fields> __results;
	static const std::map<Type,sfptr> __document_results;

    private:
	const LQIO::DOM::Document& _dom;
    };

    class Process {
    public:
	Process( std::vector<std::vector<Model::Value>>& data, const std::vector<Model::Result::result_t>& results, size_t limit, Mode mode, const std::pair<size_t,double>& x_index ) : _data(data), _results(results), _limit(limit), _mode(mode), _x_index(x_index), _model_no(0) {}

	void operator()( const std::filesystem::path& path );

    private:
	size_t x_index() const { return _x_index.first; }
	Model::Value x_value() const { return _x_index.second; }
	void set_x_value( const Model::Value& value ) { _x_index.second = value; }

    private:
	std::vector<std::vector<Model::Value>>& _data;
	const std::vector<Model::Result::result_t>& _results;
	const size_t _limit;
	Mode _mode;
	std::pair<size_t,Model::Value> _x_index;	/* For splot output */
	unsigned int _model_no;				/* Record number */
    };

    class InsertHeader {
    public:
	InsertHeader( std::vector<Model::Value>& row, Result::get_field f ) : _row(row), _f(f) {}
	void operator()( const std::pair<std::string,Model::Result::Type>& result ) { _row.emplace_back( (*_f)( result ) ); }

    private:
	std::vector<Model::Value>& _row;
	const Result::get_field _f;
    };

}

std::ostream& operator<<( std::ostream&, const Model::Value& );
#endif
