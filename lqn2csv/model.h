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
 * $Id: model.h 15586 2022-05-21 11:30:51Z greg $
 *
 * ------------------------------------------------------------------------
 */

#if !defined MODEL_H
#define MODEL_H
#include <map>
#include <string>

//#include <lqio/dom_object.h>

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

    enum class Mode { FILENAME, DIRECTORY };

    class Object {
    public:
	enum class Type
	{
	    ACTIVITY,
	    ACTIVITY_CALL,
	    ENTRY,
	    JOIN,
	    PHASE,
	    PHASE_CALL,
	    PROCESSOR,
	    TASK
	};

	static const std::map<Object::Type,const std::string> __object_type;
    };
    
    class Result {
    public:
	typedef double (LQIO::DOM::DocumentObject::*fptr)() const;

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
	    ENTRY_THROUGHPUT,
	    ENTRY_UTILIZATION, 
	    HOLD_TIMES, 
	    JOIN_DELAYS, 
	    OPEN_WAIT, 
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
	    TASK_MULTIPLICITY,
	    TASK_THROUGHPUT,
	    TASK_UTILIZATION,
	    THROUGHPUT_BOUND
	};



	struct result_fields {
	    Object::Type type;
	    const std::string name;
	    const std::string abbreviation;
	    fptr f;
	};
	
	typedef std::pair<std::string,Model::Result::Type> result_t;

    private:
//	Result( const Result& ) = delete;
//	Result& operator=( const Result& ) = delete;

    public:
	Result( const LQIO::DOM::Document& dom ) : _dom(dom) {}
	std::vector<double> operator()( const std::vector<double>&, const std::pair<std::string,Model::Result::Type>& ) const;
	static std::string ObjectType( const std::string&, const std::pair<std::string,Model::Result::Type>& );
	static std::string ObjectName( const std::string&, const std::pair<std::string,Model::Result::Type>& );
	static std::string TypeName( const std::string&, const std::pair<std::string,Model::Result::Type>& );
	static bool equal( Type, Type );
	static bool isIndependentVariable( Type );
	static bool isDependentVariable( Type );

    private:
	const LQIO::DOM::Document& dom() const { return _dom; }
	const LQIO::DOM::DocumentObject * findObject( const std::string& name, Model::Object::Type type ) const;
	const LQIO::DOM::Phase * findPhase( const LQIO::DOM::Entry *, const std::string& ) const;
	const LQIO::DOM::Call * findCall( const LQIO::DOM::Phase *, const std::string& ) const;
	const LQIO::DOM::Activity * findActivity( const LQIO::DOM::Task *, const std::string& ) const;

	static bool throughput( Type type ) { return type == Type::ACTIVITY_THROUGHPUT || type == Type::ENTRY_THROUGHPUT || type == Type::TASK_THROUGHPUT || type == Type::THROUGHPUT_BOUND; }
	static bool utilization( Type type ) { return type == Type::TASK_UTILIZATION || type == Type::ENTRY_UTILIZATION || type == Type::PHASE_UTILIZATION || type == Type::PROCESSOR_UTILIZATION; }
	static bool service_time( Type type ) { return type == Type::ACTIVITY_SERVICE || type == Type::PHASE_SERVICE; }
	static bool waiting( Type type ) { return type == Type::ACTIVITY_WAITING || type == Type::PHASE_WAITING || type == Type::PHASE_PROCESSOR_WAITING || type == Type::OPEN_WAIT; }
	static bool variance( Type type ) { return type == Type::PHASE_VARIANCE || type == Type::ACTIVITY_VARIANCE; }

    public:
	static const std::map<Type,result_fields> __results;

    private:
	const LQIO::DOM::Document& _dom;
    };

    class Process {
    public:
	Process( std::ostream& output, const std::vector<Model::Result::result_t>& results, size_t limit, Mode mode, const std::pair<size_t,double>& x_index ) : _output(output), _results(results), _limit(limit), _mode(mode), _x_index(x_index), _i(0) {}

	void operator()( const std::string& filename );

    private:
	size_t x_index() const { return _x_index.first; }
	size_t x_value() const { return _x_index.second; }
	void set_x_value( double value ) { _x_index.second = value; }

    private:
	std::ostream& _output;
	const std::vector<Model::Result::result_t>& _results;
	const size_t _limit;
	const Mode _mode;
	std::pair<size_t,double> _x_index;	/* For splot output */
	unsigned int _i;			/* Record number */
    };

    class Print {
    public:
	Print( std::ostream& output, Mode mode, const std::string& filename ) : _output(output), _mode(mode), _filename(filename), _first(true) {}
	
	void operator()( double );

    private:
	std::ostream& _output;
	const Mode _mode;
	const std::string& _filename;
	bool _first;
    };
}
#endif
