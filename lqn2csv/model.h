/* -*- c++ -*-
 * $HeadURL: http://rads-svn.sce.carleton.ca:8080/svn/lqn/trunk-V5/lqn2csv/model.h $
 *
 * Dimensions common to everything, plus some funky inline functions.
 *
 * Copyright the Real-Time and Distributed Systems Group,
 * Department of Systems and Computer Engineering,
 * Carleton University, Ottawa, Ontario, Canada. K1S 5B6
 *
 * October, 2021
 *
 * $Id: model.h 17335 2024-10-04 18:00:09Z greg $
 *
 * ------------------------------------------------------------------------
 */

#if !defined MODEL_H
#define MODEL_H
#include <map>
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

    class Output;
    std::ostream& operator<<( std::ostream&, const Model::Output& );

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

    class Output
    {
	friend std::ostream& operator<<( std::ostream&, const Model::Output& );

    public:
	enum class Type { DOUBLE, STRING };

	Output( double value ) : _type(Type::DOUBLE) { _u._double = value; }
	Output( const char * string ) : _type(Type::STRING) { _u._string = string; }

	Output& operator=( const Output& src );
	bool operator!=( const Output& dst ) const;
	
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
	std::vector<Model::Output> operator()( const std::vector<Model::Output>&, const std::pair<std::string,Model::Result::Type>& ) const;
	static std::string ObjectName( const std::string&, const std::pair<std::string,Model::Result::Type>& );
	static std::string TypeName( const std::string&, const std::pair<std::string,Model::Result::Type>& );
	static bool equal( Type, Type );
	static bool isIndependentVariable( Type );
	static bool isDependentVariable( Type );
	static void printHeader( std::ostream&, const std::string&, const std::vector<Model::Result::result_t>& results, Result::get_field f, size_t width );

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
	Process( std::ostream& output, const std::vector<Model::Result::result_t>& results, size_t limit, size_t header_column_width, Mode mode, const std::pair<size_t,double>& x_index ) : _output(output), _results(results), _limit(limit), _header_column_width(header_column_width), _mode(mode), _x_index(x_index), _i(0) {}

	void operator()( const std::string& filename );

    private:
	size_t x_index() const { return _x_index.first; }
	Model::Output x_value() const { return _x_index.second; }
	void set_x_value( const Model::Output& value ) { _x_index.second = value; }
	void print_filename( const std::string& ) const;

    private:
	std::ostream& _output;
	const std::vector<Model::Result::result_t>& _results;
	const size_t _limit;
	const size_t _header_column_width;
	const Mode _mode;
	std::pair<size_t,Model::Output> _x_index;	/* For splot output */
	unsigned int _i;			/* Record number */
    };

    class PrintHeader {
    public:
	PrintHeader( std::ostream& output, Result::get_field f ) : _output(output), _f(f) {}
	void operator()( const std::pair<std::string,Model::Result::Type>& ) const;

    private:
	std::ostream& _output;
	const Result::get_field _f;
    };

    class PrintLine {
    public:
	PrintLine( std::ostream& output ) : _output(output) {}
	void operator()( const Model::Output& ) const;

    private:
	std::ostream& _output;
    };
}
#endif
