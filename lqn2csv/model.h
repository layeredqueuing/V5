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
 * $Id: model.h 15037 2021-10-04 16:35:47Z greg $
 *
 * ------------------------------------------------------------------------
 */

#if !defined MODEL_H
#include <map>

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

    class Object {
    public:
	enum class Type
	{
	    ACTIVITY,
	    CALL,
	    ENTRY,
	    JOIN,
	    PHASE,
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
	    ACTIVITY_SERVICE,
	    ACTIVITY_THROUGHPUT,
	    ACTIVITY_VARIANCE,
	    ACTIVITY_WAITING,
	    ENTRY_THROUGHPUT,
	    ENTRY_UTILIZATION, 
	    HOLD_TIMES, 
	    JOIN_DELAYS, 
	    LOSS_PROBABILITY, 
	    OPEN_WAIT, 
	    PHASE_UTILIZATION,
	    PROCESSOR_UTILIZATION,
	    PROCESSOR_WAITING,
	    SERVICE_EXCEEDED, 
	    SERVICE_TIME,
	    TASK_THROUGHPUT,
	    TASK_UTILIZATION,
	    THROUGHPUT_BOUND,
	    VARIANCE, 
	    WAITING_TIME
	};

	struct result_fields {
	    Object::Type type;
	    const std::string name;
	    fptr f;
	};
	
	typedef std::pair<std::string,Model::Result::Type> result_t;

    private:
//	Result( const Result& ) = delete;
//	Result& operator=( const Result& ) = delete;

    public:
	Result( const LQIO::DOM::Document& dom ) : _dom(dom) {}
	std::string operator()( const std::string&, const std::pair<std::string,Model::Result::Type>& ) const;
	static std::string ObjectType( const std::string&, const std::pair<std::string,Model::Result::Type>& );
	static std::string ObjectName( const std::string&, const std::pair<std::string,Model::Result::Type>& );
	static std::string TypeName( const std::string&, const std::pair<std::string,Model::Result::Type>& );
	static bool equal( Type, Type );

    private:
	const LQIO::DOM::Document& dom() const { return _dom; }
	const LQIO::DOM::DocumentObject * findObject( const std::string& name, Model::Result::Type type ) const;
	const LQIO::DOM::Phase * findPhase( const LQIO::DOM::Entry *, const std::string& ) const;
	const LQIO::DOM::Call * findCall( const LQIO::DOM::Phase *, const std::string& ) const;
	const LQIO::DOM::Activity * findActivity( const LQIO::DOM::Task *, const std::string& ) const;

	static bool throughput( Type type ) { return type == Type::ACTIVITY_THROUGHPUT || type == Type::ENTRY_THROUGHPUT || type == Type::TASK_THROUGHPUT || type == Type::THROUGHPUT_BOUND; }
	static bool utilization( Type type ) { return type == Type::TASK_UTILIZATION || type == Type::ENTRY_UTILIZATION || type == Type::PHASE_UTILIZATION || type == Type::PROCESSOR_UTILIZATION; }
	static bool service_time( Type type ) { return type == Type::ACTIVITY_SERVICE || type == Type::SERVICE_TIME; }
	static bool waiting( Type type ) { return type == Type::ACTIVITY_WAITING || type == Type::PROCESSOR_WAITING || type == Type::WAITING_TIME || type == Type::OPEN_WAIT; }
	static bool variance( Type type ) { return type == Type::VARIANCE || type == Type::ACTIVITY_VARIANCE; }

    public:
	static const std::map<Type,result_fields> __results;

    private:
	const LQIO::DOM::Document& _dom;
    };

    class Process {
    public:
	Process( std::ostream& output, const std::vector<Model::Result::result_t>& results ) : _output(output), _results( results ), _i(1) {}

	void operator()( const char * filename ) const;
	
    private:
	std::ostream& _output;
	const std::vector<Model::Result::result_t>& _results;
	mutable unsigned int _i;
    };
	
}
#endif
