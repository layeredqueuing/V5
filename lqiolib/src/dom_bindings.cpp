/*
 *  $Id: dom_bindings.cpp 15209 2021-12-13 16:20:37Z greg $
 *
 *  Created by Martin Mroz on 16/04/09.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#include <cstdio>
#include <lqx/Environment.h>
#include <lqx/SymbolTable.h>
#include <lqx/MethodTable.h>
#include <lqx/LanguageObject.h>
#include <lqx/Array.h>
#include <lqx/RuntimeException.h>

#include "dom_document.h"
#include "confidence_intervals.h"

#include <sstream>
#include <cstring>
namespace LQIO {

    const char * __lqx_elapsed_time             = "elapsed_time";
    const char * __lqx_exceeded_time            = "pr_time_exceeded";
    const char * __lqx_iterations               = "iterations";
    const char * __lqx_pr_exceeded              = "pr_exceeded";
    const char * __lqx_processor_utilization    = "proc_utilization";
    const char * __lqx_processor_waiting        = "proc_waiting";
    const char * __lqx_service_time             = "service_time";
    const char * __lqx_system_time              = "system_cpu_time";
    const char * __lqx_throughput               = "throughput";
    const char * __lqx_throughput_bound         = "throughput_bound";
    const char * __lqx_user_time                = "user_cpu_time";
    const char * __lqx_utilization              = "utilization";
    const char * __lqx_variance                 = "service_time_variance";
    const char * __lqx_waiting                  = "waiting";
    const char * __lqx_waiting_variance         = "waiting_variance";


/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- [Object] */
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */

    class LQXDocumentObject : public LQX::LanguageObject {
    protected:
	typedef double (DOM::DocumentObject::*get_result_fptr)() const;

	struct attribute_table_t
	{
	    attribute_table_t( get_result_fptr m=nullptr, get_result_fptr v=nullptr ) : mean(m), variance(v) {}
	    LQX::SymbolAutoRef operator()( const DOM::DocumentObject& domObject ) const { return LQX::Symbol::encodeDouble( (domObject.*mean)() ); }
	    const get_result_fptr mean;
	    const get_result_fptr variance;
	};

    public:
	LQXDocumentObject( uint32_t kLQXobject, const DOM::DocumentObject * domObject ) : LQX::LanguageObject(kLQXobject), _domObject(domObject)
	    {
	    }

	virtual LQX::SymbolAutoRef getPropertyNamed(LQX::Environment* env, const std::string& name)
	    {
		std::map<const std::string,attribute_table_t>::const_iterator attribute =  __attributeTable.find( name.c_str() );
		if ( attribute != __attributeTable.end() ) {
		    try {
			if ( _domObject ) {
			    return attribute->second( *_domObject );
			}
		    }
		    catch ( const LQIO::should_implement& e ) {
		    }
		}

		/* Anything we don't handle may be handled by our superclass */
		return this->LanguageObject::getPropertyNamed(env, name);
	    }

        const DOM::DocumentObject* getDOMObject() const { return _domObject; }

    protected:
        const DOM::DocumentObject * _domObject;
        static const std::map<const std::string,attribute_table_t> __attributeTable;

    };

    const std::map<const std::string,LQXDocumentObject::attribute_table_t> LQXDocumentObject::__attributeTable =
    {
	{ "loss_probability",               attribute_table_t( &DOM::DocumentObject::getResultDropProbability,            &DOM::DocumentObject::getResultDropProbabilityVariance ) },
	{ "phase1_pr_time_exceeded",        attribute_table_t( &DOM::DocumentObject::getResultPhase1MaxServiceTimeExceeded, nullptr ) },
	{ "phase1_proc_waiting",            attribute_table_t( &DOM::DocumentObject::getResultPhase1ProcessorWaiting,     &DOM::DocumentObject::getResultPhase1ProcessorWaitingVariance ) },
	{ "phase1_service_time",            attribute_table_t( &DOM::DocumentObject::getResultPhase1ServiceTime,          &DOM::DocumentObject::getResultPhase1ServiceTimeVariance ) },
	{ "phase1_service_time_variance",   attribute_table_t( &DOM::DocumentObject::getResultPhase1VarianceServiceTime,  &DOM::DocumentObject::getResultPhase1VarianceServiceTimeVariance ) },
	{ "phase1_utilization",             attribute_table_t( &DOM::DocumentObject::getResultPhase1Utilization,          &DOM::DocumentObject::getResultPhase1UtilizationVariance ) },
	{ "phase2_proc_waiting",            attribute_table_t( &DOM::DocumentObject::getResultPhase2ProcessorWaiting,     &DOM::DocumentObject::getResultPhase2ProcessorWaitingVariance ) },
	{ "phase2_service_time",            attribute_table_t( &DOM::DocumentObject::getResultPhase2ServiceTime,          &DOM::DocumentObject::getResultPhase2ServiceTimeVariance ) },
	{ "phase2_service_time_variance",   attribute_table_t( &DOM::DocumentObject::getResultPhase2VarianceServiceTime,  &DOM::DocumentObject::getResultPhase2VarianceServiceTimeVariance ) },
	{ "phase2_utilization",             attribute_table_t( &DOM::DocumentObject::getResultPhase2Utilization,          &DOM::DocumentObject::getResultPhase2UtilizationVariance ) },
	{ "phase3_pr_time_exceeded",        attribute_table_t( &DOM::DocumentObject::getResultPhase2MaxServiceTimeExceeded, nullptr ) },
	{ "phase3_pr_time_exceeded",        attribute_table_t( &DOM::DocumentObject::getResultPhase3MaxServiceTimeExceeded, nullptr ) },
	{ "phase3_proc_waiting",            attribute_table_t( &DOM::DocumentObject::getResultPhase3ProcessorWaiting,     &DOM::DocumentObject::getResultPhase3ProcessorWaitingVariance ) },
	{ "phase3_service_time",            attribute_table_t( &DOM::DocumentObject::getResultPhase3ServiceTime,          &DOM::DocumentObject::getResultPhase3ServiceTimeVariance ) },
	{ "phase3_service_time_variance",   attribute_table_t( &DOM::DocumentObject::getResultPhase3VarianceServiceTime,  &DOM::DocumentObject::getResultPhase3VarianceServiceTimeVariance ) },
	{ "phase3_utilization",             attribute_table_t( &DOM::DocumentObject::getResultPhase3Utilization,          &DOM::DocumentObject::getResultPhase3UtilizationVariance ) },
	{ "rwlock_reader_holding",          attribute_table_t( &DOM::DocumentObject::getResultReaderHoldingTime,          &DOM::DocumentObject::getResultReaderHoldingTimeVariance ) },
	{ "rwlock_reader_holding_variance", attribute_table_t( &DOM::DocumentObject::getResultVarianceReaderHoldingTime,  &DOM::DocumentObject::getResultVarianceReaderHoldingTimeVariance ) },
	{ "rwlock_reader_utilization",      attribute_table_t( &DOM::DocumentObject::getResultReaderHoldingUtilization,   &DOM::DocumentObject::getResultReaderHoldingUtilizationVariance ) },
	{ "rwlock_reader_waiting",          attribute_table_t( &DOM::DocumentObject::getResultReaderBlockedTime,          &DOM::DocumentObject::getResultReaderBlockedTimeVariance ) },
	{ "rwlock_reader_waiting_variance", attribute_table_t( &DOM::DocumentObject::getResultVarianceReaderBlockedTime,  &DOM::DocumentObject::getResultVarianceReaderBlockedTimeVariance ) },
	{ "rwlock_writer_holding",          attribute_table_t( &DOM::DocumentObject::getResultWriterHoldingTime,          &DOM::DocumentObject::getResultWriterHoldingTimeVariance ) },
	{ "rwlock_writer_holding_variance", attribute_table_t( &DOM::DocumentObject::getResultVarianceWriterHoldingTime,  &DOM::DocumentObject::getResultVarianceWriterHoldingTimeVariance ) },
	{ "rwlock_writer_utilization",      attribute_table_t( &DOM::DocumentObject::getResultWriterHoldingUtilization,   &DOM::DocumentObject::getResultWriterHoldingUtilizationVariance ) },
	{ "rwlock_writer_waiting",          attribute_table_t( &DOM::DocumentObject::getResultWriterBlockedTime,          &DOM::DocumentObject::getResultWriterBlockedTimeVariance ) },
	{ "rwlock_writer_waiting_variance", attribute_table_t( &DOM::DocumentObject::getResultVarianceWriterBlockedTime,  &DOM::DocumentObject::getResultVarianceWriterBlockedTimeVariance ) },
	{ "semaphore_utilization",          attribute_table_t( &DOM::DocumentObject::getResultHoldingUtilization,         &DOM::DocumentObject::getResultHoldingUtilizationVariance ) },
	{ "semaphore_waiting",              attribute_table_t( &DOM::DocumentObject::getResultHoldingTime,                &DOM::DocumentObject::getResultHoldingTimeVariance ) },
	{ "semaphore_waiting_variance",     attribute_table_t( &DOM::DocumentObject::getResultVarianceHoldingTime,        &DOM::DocumentObject::getResultVarianceHoldingTimeVariance ) },
	{ "squared_coeff_variation",        attribute_table_t( &DOM::DocumentObject::getResultSquaredCoeffVariation,      nullptr ) },
	{ __lqx_pr_exceeded,                attribute_table_t( &DOM::DocumentObject::getResultMaxServiceTimeExceeded,     nullptr ) },
	{ __lqx_processor_utilization,	    attribute_table_t( &DOM::DocumentObject::getResultProcessorUtilization,       &DOM::DocumentObject::getResultProcessorUtilizationVariance ) },
	{ __lqx_processor_waiting,          attribute_table_t( &DOM::DocumentObject::getResultProcessorWaiting,           &DOM::DocumentObject::getResultProcessorWaitingVariance ) },
	{ __lqx_service_time,               attribute_table_t( &DOM::DocumentObject::getResultServiceTime,                &DOM::DocumentObject::getResultServiceTimeVariance ) },
	{ __lqx_variance,                   attribute_table_t( &DOM::DocumentObject::getResultVarianceServiceTime,        &DOM::DocumentObject::getResultVarianceServiceTimeVariance ) },
	{ __lqx_throughput,                 attribute_table_t( &DOM::DocumentObject::getResultThroughput,                 &DOM::DocumentObject::getResultThroughputVariance ) },
	{ __lqx_throughput_bound,           attribute_table_t( &DOM::DocumentObject::getResultThroughputBound,            nullptr ) },
	{ __lqx_utilization,                attribute_table_t( &DOM::DocumentObject::getResultUtilization,                &DOM::DocumentObject::getResultUtilizationVariance ) },
	{ __lqx_waiting,                    attribute_table_t( &DOM::DocumentObject::getResultWaitingTime,                &DOM::DocumentObject::getResultWaitingTimeVariance ) },
	{ __lqx_waiting_variance,           attribute_table_t( &DOM::DocumentObject::getResultVarianceWaitingTime,        &DOM::DocumentObject::getResultVarianceWaitingTimeVariance ) }
    };

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- [Processor] */
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
#pragma mark -

    class LQXProcessor : public LQXDocumentObject {
    public:

        const static uint32_t kLQXProcessorObjectTypeId = 10+1;

        /* Designated Initializers */
        LQXProcessor(const DOM::Processor* proc) : LQXDocumentObject(kLQXProcessorObjectTypeId,proc)
            {
            }

        virtual ~LQXProcessor()
            {
            }

        /* Comparison and Operators */
        virtual bool isEqualTo(const LQX::LanguageObject* other) const
            {
                const LQXProcessor* processor = dynamic_cast<const LQXProcessor *>(other);
                return processor && processor->getDOMProcessor() == getDOMProcessor();  /* Return a comparison of the types */
            }

        virtual std::string description() const
            {
                /* Return a description of the task */
                std::stringstream ss;
                ss << getTypeName() << "(" << getDOMProcessor()->getName() << ")";
		return ss.str();
	    }

	virtual std::string getTypeName() const
	    {
		return DOM::Processor::__typeName;
	    }

        const DOM::Processor* getDOMProcessor() const { return dynamic_cast<const DOM::Processor*>(_domObject); }


	virtual LQX::SymbolAutoRef getPropertyNamed(LQX::Environment* env, const std::string& name)
	    {
		/* All the valid properties of tasks */
		if (name == "utilization") {
		    return LQX::Symbol::encodeDouble(getDOMProcessor()->getResultUtilization());
		}

		/* Anything we don't handle may be handled by our superclass */
		return this->LanguageObject::getPropertyNamed(env, name);
	    }
    };

    class LQXGetProcessor : public LQX::Method {
    public:
	LQXGetProcessor(const DOM::Document* doc) : _document(doc), _symbolCache() {}
	virtual ~LQXGetProcessor() {}

	/* Basic information for the method itself */
	virtual std::string getName() const { return DOM::Processor::__typeName; }
	virtual const char* getParameterInfo() const { return "s"; }
	virtual std::string getHelp() const { return "Returns the processor associated with a name."; }

	/* Invocation of the method from the language */
	virtual LQX::SymbolAutoRef invoke(LQX::Environment* env, std::vector<LQX::SymbolAutoRef >& args) {

	    /* Decode the name of the processor and look it up in cache */
	    const char* procName = decodeString(args, 0);
	    if (_symbolCache.find(procName) != _symbolCache.end()) {
		return _symbolCache[procName];
	    }

	    /* Obtain the processor reference  */
	    DOM::Processor* proc = _document->getProcessorByName(procName);

	    /* There was no processor given */
	    if (proc == nullptr) {
		throw LQX::RuntimeException( "No processor specified with name ", procName );
		return LQX::Symbol::encodeNull();
	    }

	    /* Return an encapsulated reference to the processor */
	    LQXProcessor* procObject = new LQXProcessor(proc);
	    _symbolCache[procName] = LQX::Symbol::encodeObject(procObject, false);
	    return _symbolCache[procName];
	}

    private:
	const DOM::Document* _document;
	std::map<std::string,LQX::SymbolAutoRef> _symbolCache;
    };

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- [Group] */
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
#pragma mark -

    class LQXGroup : public LQXDocumentObject {
    public:

        const static uint32_t kLQXGroupObjectTypeId = 10+2;

        /* Designated Initializers */
        LQXGroup(const DOM::Group* group) : LQXDocumentObject(kLQXGroupObjectTypeId,group)
            {
            }

        virtual ~LQXGroup()
            {
            }

        /* Comparison and Operators */
        virtual bool isEqualTo(const LQX::LanguageObject* other) const
            {
                const LQXGroup* group = dynamic_cast<const LQXGroup *>(other);
                return group && group->getDOMGroup() == getDOMGroup();  /* Return a comparison of the types */
            }

        virtual std::string description() const
            {
                /* Return a description of the group */
                std::stringstream ss;
                ss << getTypeName() << "(" << getDOMGroup()->getName() << ")";
		return ss.str();
	    }

	virtual std::string getTypeName() const
	    {
		return DOM::Group::__typeName;
	    }

        const DOM::Group* getDOMGroup() const { return dynamic_cast<const DOM::Group*>(_domObject); }


	virtual LQX::SymbolAutoRef getPropertyNamed(LQX::Environment* env, const std::string& name)
	    {
		/* All the valid properties of groups */
		if (name == "utilization") {
		    return LQX::Symbol::encodeDouble(getDOMGroup()->getResultUtilization());
		}

		/* Anything we don't handle may be handled by our superclass */
		return this->LanguageObject::getPropertyNamed(env, name);
	    }
    };

    class LQXGetGroup : public LQX::Method {
    public:
	LQXGetGroup(const DOM::Document* doc) : _document(doc), _symbolCache() {}
	virtual ~LQXGetGroup() {}

	/* Basic information for the method itself */
	virtual std::string getName() const { return DOM::Group::__typeName; }
	virtual const char* getParameterInfo() const { return "s"; }
	virtual std::string getHelp() const { return "Returns the group associated with a name."; }

	/* Invocation of the method from the language */
	virtual LQX::SymbolAutoRef invoke(LQX::Environment* env, std::vector<LQX::SymbolAutoRef >& args) {

	    /* Decode the name of the group and look it up in cache */
	    const char* groupName = decodeString(args, 0);
	    if (_symbolCache.find(groupName) != _symbolCache.end()) {
		return _symbolCache[groupName];
	    }

	    /* Obtain the group reference  */
	    DOM::Group* group = _document->getGroupByName(groupName);

	    /* There was no group given */
	    if (group == nullptr) {
		throw LQX::RuntimeException("No group specified with name `%s'.", groupName);
		return LQX::Symbol::encodeNull();
	    }

	    /* Return an encapsulated reference to the group */
	    LQXGroup* groupObject = new LQXGroup(group);
	    _symbolCache[groupName] = LQX::Symbol::encodeObject(groupObject, false);
	    return _symbolCache[groupName];
	}

    private:
	const DOM::Document* _document;
	std::map<std::string,LQX::SymbolAutoRef> _symbolCache;
    };

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- [Task] */
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */

    class LQXTask : public LQXDocumentObject
    {
    public:

	const static uint32_t kLQXTaskObjectTypeId = 10+3;

	/* Designated Initializers */
        LQXTask(const DOM::Task* task) : LQXDocumentObject(kLQXTaskObjectTypeId,task)
            {
            }

	virtual ~LQXTask()
	    {
	    }

	/* Comparison and Operators */
	virtual bool isEqualTo(const LQX::LanguageObject* other) const
	    {
		const LQXTask* task = dynamic_cast<const LQXTask *>(other);
		return task && task->getDOMTask() == getDOMTask();	/* Return a comparison of the types */
	    }

	virtual std::string description() const
	    {
		/* Return a description of the task */
                std::stringstream ss;
                ss << getTypeName() << "(" << getDOMTask()->getName() << ")";
		return ss.str();
	    }

	virtual std::string getTypeName() const
	    {
		return DOM::Task::__typeName;
	    }

        const DOM::Task* getDOMTask() const { return dynamic_cast<const DOM::Task*>(_domObject); }

    private:
    };

    class LQXGetTask : public LQX::Method {
    public:
	LQXGetTask(const DOM::Document* doc) : _document(doc), _symbolCache() {}
	virtual ~LQXGetTask() {}

	/* Basic information for the method itself */
	virtual std::string getName() const { return DOM::Task::__typeName; }
	virtual const char* getParameterInfo() const { return "s"; }
	virtual std::string getHelp() const { return "Returns the task associated with a name."; }

	/* Invocation of the method from the language */
	virtual LQX::SymbolAutoRef invoke(LQX::Environment* env, std::vector<LQX::SymbolAutoRef >& args) {

	    /* Decode the name of the task and look it up in cache */
	    const char* taskName = decodeString(args, 0);
	    if (_symbolCache.find(taskName) != _symbolCache.end()) {
		return _symbolCache[taskName];
	    }

	    /* Obtain the task reference  */
	    DOM::Task* task = _document->getTaskByName(taskName);

	    /* There was no task given */
	    if (task == nullptr) {
		throw LQX::RuntimeException("No task specified with name `%s'.", taskName);
		return LQX::Symbol::encodeNull();
	    }

	    /* Return an encapsulated reference to the task */
	    LQXTask* taskObject = new LQXTask(task);
	    _symbolCache[taskName] = LQX::Symbol::encodeObject(taskObject, false);
	    return _symbolCache[taskName];
	}

    private:
	const DOM::Document* _document;
	std::map<std::string,LQX::SymbolAutoRef> _symbolCache;
    };

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- [Entry] */
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
#pragma mark -

    class LQXEntry : public LQXDocumentObject {
    public:

	const static uint32_t kLQXEntryObjectTypeId = 10+4;

	/* Designated Initializers */
        LQXEntry(DOM::Entry* entry) : LQXDocumentObject(kLQXEntryObjectTypeId,entry)
	    {
	    }

	virtual ~LQXEntry()
	    {
	    }

	/* Comparison and Operators */
	virtual bool isEqualTo(const LQX::LanguageObject* other) const
	    {
		const LQXEntry* entry = dynamic_cast<const LQXEntry *>(other);
		return entry && entry->getDOMEntry() == getDOMEntry();
	    }

	virtual std::string description() const
	    {
		/* Return a description of the task */
		std::stringstream ss;
		ss << getTypeName() << "(" << getDOMEntry()->getName() << ")";
		return ss.str();
	    }

	virtual std::string getTypeName() const
	    {
		return DOM::Entry::__typeName;
	    }

	virtual LQX::SymbolAutoRef getPropertyNamed(LQX::Environment* env, const std::string& name)
	    {
		if (name == "has_phase_1") {
		    return LQX::Symbol::encodeBoolean(getDOMEntry()->hasResultsForPhase(1));
		} else if (name == "has_phase_2") {
		    return LQX::Symbol::encodeBoolean(getDOMEntry()->hasResultsForPhase(2));
		} else if (name == "has_phase_3") {
		    return LQX::Symbol::encodeBoolean(getDOMEntry()->hasResultsForPhase(3));
		} else if (name == "has_open_wait_time") {
		    return LQX::Symbol::encodeBoolean(getDOMEntry()->hasResultsForOpenWait());
		}
		/* Anything we don't handle may be handled by our superclass */
		return this->LQXDocumentObject::getPropertyNamed(env, name);
	    }

        const DOM::Entry* getDOMEntry() const { return dynamic_cast<const DOM::Entry*>(_domObject); }

    };

    class LQXGetEntry : public LQX::Method {
    public:
	LQXGetEntry(const DOM::Document* doc) : _document(doc), _symbolCache() {}
	virtual ~LQXGetEntry() {}

	/* Basic information for the method itself */
	virtual std::string getName() const { return DOM::Entry::__typeName; }
	virtual const char* getParameterInfo() const { return "s"; }
	virtual std::string getHelp() const { return "Returns the entry with the given name."; }

	/* Invocation of the method from the language */
	virtual LQX::SymbolAutoRef invoke(LQX::Environment* env, std::vector<LQX::SymbolAutoRef >& args) {

	    /* Decode the name of the entry and look it up in cache */
	    const char* entryName = decodeString(args, 0);
	    if (_symbolCache.find(entryName) != _symbolCache.end()) {
		return _symbolCache[entryName];
	    }

	    /* Obtain the entry reference  */
	    DOM::Entry* entry = _document->getEntryByName(entryName);

	    /* There was no entry given */
	    if (entry == nullptr) {
		throw LQX::RuntimeException("No entry specified with name `%s'.", entryName);
		return LQX::Symbol::encodeNull();
	    }

	    /* Return an encapsulated reference to the entry */
	    LQXEntry* entryObject = new LQXEntry(entry);
	    _symbolCache[entryName] = LQX::Symbol::encodeObject(entryObject, false);
	    return _symbolCache[entryName];
	}

    private:
	const DOM::Document* _document;
	std::map<std::string,LQX::SymbolAutoRef> _symbolCache;
    };

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- [Phase] */
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
#pragma mark -

    class LQXPhase : public LQXDocumentObject {
    public:

	const static uint32_t kLQXPhaseObjectTypeId = 10+5;

	/* Designated Initializers */
	LQXPhase(const DOM::Phase* phase) : LQXDocumentObject(kLQXPhaseObjectTypeId,phase)
	    {
	    }

	virtual ~LQXPhase()
	    {
	    }

	/* Comparison and Operators */
	virtual bool isEqualTo(const LQX::LanguageObject* other) const
	    {
		const LQXPhase* phase = dynamic_cast<const LQXPhase *>(other);
		return phase && phase->getDOMPhase() == getDOMPhase();
	    }

	virtual std::string description() const
	    {
		/* Return a description of the phase */
		std::stringstream ss;
		ss << getTypeName() << "(n)";
		return ss.str();
	    }

	virtual std::string getTypeName() const
	    {
		return DOM::Phase::__typeName;
	    }

        const DOM::Phase* getDOMPhase() const { return dynamic_cast<const DOM::Phase*>(_domObject); }

    };

// [MM] These should be cached.
    class LQXGetPhase : public LQX::Method {
    public:
	LQXGetPhase() {}
	virtual ~LQXGetPhase() {}

	/* Basic information for the method itself */
	virtual std::string getName() const { return DOM::Phase::__typeName; }
	virtual const char* getParameterInfo() const { return "od"; }
	virtual std::string getHelp() const { return "Returns the given phase of the given entry."; }

	/* Invocation of the method from the language */
	virtual LQX::SymbolAutoRef invoke(LQX::Environment* env, std::vector<LQX::SymbolAutoRef >& args) {

	    /* Decode the arguments to the given method */
	    LQX::LanguageObject* lo = decodeObject(args, 0);
	    const unsigned int phase = static_cast<unsigned int>(decodeDouble(args, 1));
	    LQXEntry* entry = dynamic_cast<LQXEntry *>(lo);

	    /* Make sure that what we have is an entry */
	    if (!entry) {
		throw LQX::RuntimeException("Argument 1 to phase(object,double) is not an entry.");
		return LQX::Symbol::encodeNull();
	    }

	    /* Obtain the phase for the entry */
	    const DOM::Entry* domEntry = entry->getDOMEntry();
	    const DOM::Phase* domPhase = domEntry->getPhase(phase);
	    return LQX::Symbol::encodeObject(new LQXPhase(domPhase), false);
	}
    };

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- [Activity] */
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
#pragma mark -

    class LQXActivity : public LQXDocumentObject {
    public:

	const static uint32_t kLQXActivityObjectTypeId = 10+6;

	/* Designated Initializers */
	LQXActivity(const DOM::Activity* act) : LQXDocumentObject(kLQXActivityObjectTypeId,act)
	    {
	    }

	virtual ~LQXActivity()
	    {
	    }

	/* Comparison and Operators */
	virtual bool isEqualTo(const LQX::LanguageObject* other) const
	    {
		const LQXActivity* act = dynamic_cast<const LQXActivity *>(other);
		return act && act->getDOMActivity() == getDOMActivity();
	    }

	virtual std::string description() const
	    {
		/* Return a description of the task */
		std::stringstream ss;
		ss << getTypeName() << "(" << getDOMActivity()->getName() << ")";
		return ss.str();
	    }

	virtual std::string getTypeName() const
	    {
		return DOM::Activity::__typeName;
	    }

        const DOM::Activity* getDOMActivity() const { return dynamic_cast<const DOM::Activity*>(_domObject); }
    };

// [MM] These should be cached.
    class LQXGetActivity : public LQX::Method {
    public:
	LQXGetActivity() {}
	virtual ~LQXGetActivity() {}

	/* Basic information for the method itself */
	virtual std::string getName() const { return DOM::Activity::__typeName; }
	virtual const char* getParameterInfo() const { return "os"; }
	virtual std::string getHelp() const { return "Returns the activity with the given name."; }

	/* Invocation of the method from the language */
	virtual LQX::SymbolAutoRef invoke(LQX::Environment* env, std::vector<LQX::SymbolAutoRef >& args) {

	    /* Decode the arguments to the given method */
	    LQX::LanguageObject* lo = decodeObject(args, 0);
	    const char* actName = decodeString(args, 1);
	    LQXTask* task = dynamic_cast<LQXTask *>(lo);

	    /* Make sure that what we have is a task */
	    if (!task) {
		throw LQX::RuntimeException("Argument 1 to activity(object,string) is not a task.");
		return LQX::Symbol::encodeNull();
	    }

	    /* Obtain the activity for the task */
	    const DOM::Task* domTask = task->getDOMTask();
	    const DOM::Activity* domAct = domTask->getActivity(actName);

	    /* Make sure we got one */
	    if (domAct == nullptr) {
		throw LQX::RuntimeException("No activity named `%s' for task `%s'.",
					    actName, domTask->getName().c_str());
		return LQX::Symbol::encodeNull();
	    }

	    return LQX::Symbol::encodeObject(new LQXActivity(domAct), false);
	}
    };

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- [Call] */
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
#pragma mark -

    class LQXCall : public LQXDocumentObject {
    public:

	const static uint32_t kLQXCallObjectTypeId = 10+7;

	/* Designated Initializers */
	LQXCall(const DOM::Call* call) : LQXDocumentObject(kLQXCallObjectTypeId,call)
	    {
	    }

	virtual ~LQXCall()
	    {
	    }

	/* Comparison and Operators */
	virtual bool isEqualTo(const LQX::LanguageObject* other) const
	    {
		const LQXCall* call = dynamic_cast<const LQXCall *>(other);
		return call && call->getDOMCall() == getDOMCall();
	    }

	virtual std::string description() const
	    {
		/* Return a description of the task */
		std::stringstream ss;
		ss << getTypeName() << "(" << getDOMCall()->getSourceObject()->getName() << "->"
		   << getDOMCall()->getDestinationEntry()->getName() << ")";
		return ss.str();
	    }

	virtual std::string getTypeName() const
	    {
		return DOM::Call::__typeName;
	    }

        const DOM::Call* getDOMCall() const { return dynamic_cast<const DOM::Call*>(_domObject); }
    };


// [MM] These should be cached.
    class LQXGetCall : public LQX::Method {
    public:
	LQXGetCall() {}
	virtual ~LQXGetCall() {}

	/* Basic information for the method itself */
	virtual std::string getName() const { return DOM::Call::__typeName; }
	virtual const char* getParameterInfo() const { return "os"; }
	virtual std::string getHelp() const { return "Returns the given call from entry to dest."; }

	/* Invocation of the method from the language */
	virtual LQX::SymbolAutoRef invoke(LQX::Environment* env, std::vector<LQX::SymbolAutoRef >& args) {

	    /* Decode the arguments to the given method */
	    LQX::LanguageObject* lo = decodeObject(args, 0);
	    const char* destEntry = decodeString(args, 1);
	    LQXPhase* phase = dynamic_cast<LQXPhase *>(lo);

	    const DOM::Phase * domPhase = nullptr;
	    if ( phase ) {
		/* Obtain the phase for the entry */
		domPhase = phase->getDOMPhase();
	    } else {
		LQXActivity * activity = dynamic_cast<LQXActivity *>(lo);
		if ( activity ) {
		    domPhase = activity->getDOMActivity();
		} else {
		    throw LQX::RuntimeException("Argument 1 to call(object,string) is not a phase.");
		    return LQX::Symbol::encodeNull();
		}
	    }

	    const std::vector<DOM::Call*>& calls = domPhase->getCalls();
	    std::vector<DOM::Call*>::const_iterator iter;
	    for (iter = calls.begin(); iter != calls.end(); ++iter) {
		const DOM::Call* call = *iter;
		if (call->getDestinationEntry()->getName() == destEntry) {
		    return LQX::Symbol::encodeObject(new LQXCall(call), false);
		}
	    }

	    throw LQX::RuntimeException("No call found for phase `%s' with destination entry `%s'.", lo->description().c_str(), destEntry);
	    return LQX::Symbol::encodeNull();
	}
    };

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- [Forwarding] */
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
#pragma mark -

    class LQXForward : public LQXDocumentObject {
    public:

	const static uint32_t kLQXForwardObjectTypeId = 10+8;

	/* Designated Initializers */
	LQXForward(const DOM::Call* call) : LQXDocumentObject(kLQXForwardObjectTypeId,call)
	    {
	    }

	virtual ~LQXForward()
	    {
	    }

	/* Comparison and Operators */
	virtual bool isEqualTo(const LQX::LanguageObject* other) const
	    {
		const LQXForward* fwd = dynamic_cast<const LQXForward *>(other);
		return fwd && fwd->getDOMCall() == getDOMCall();
	    }

	virtual std::string description() const
	    {
		/* Return a description of the task */
		std::stringstream ss;
		ss << getTypeName() << "(" << getDOMCall()->getSourceObject()->getName() << "->"
		   << getDOMCall()->getDestinationEntry()->getName() << ")";
		return ss.str();
	    }

	virtual std::string getTypeName() const
	    {
		return DOM::Call::__typeName;
	    }

        const DOM::Call* getDOMCall() const { return dynamic_cast<const DOM::Call*>(_domObject); }
    };


// [MM] These should be cached.
    class LQXGetForward : public LQX::Method {
    public:
	LQXGetForward() {}
	virtual ~LQXGetForward() {}

	/* Basic information for the method itself */
	virtual std::string getName() const { return "forward"; }
	virtual const char* getParameterInfo() const { return "os"; }
	virtual std::string getHelp() const { return "Returns the given forwarding call from entry to dest."; }

	/* Invocation of the method from the language */
	virtual LQX::SymbolAutoRef invoke(LQX::Environment* env, std::vector<LQX::SymbolAutoRef >& args) {

	    /* Decode the arguments to the given method */
	    LQX::LanguageObject* lo = decodeObject(args, 0);
	    const char* destEntry = decodeString(args, 1);
	    LQXEntry* entry = dynamic_cast<LQXEntry *>(lo);

	    if ( entry == nullptr ) {
		throw LQX::RuntimeException("Argument 1 to forward(object,string) is not a entry.");
		return LQX::Symbol::encodeNull();
	    } 
	    const DOM::Entry * domEntry = entry->getDOMEntry();

	    const std::vector<DOM::Call*>& calls = domEntry->getForwarding();
	    std::vector<DOM::Call*>::const_iterator iter;
	    for (iter = calls.begin(); iter != calls.end(); ++iter) {
		const DOM::Call* call = *iter;
		if (call->getDestinationEntry()->getName() == destEntry) {
		    return LQX::Symbol::encodeObject(new LQXForward(call), false);
		}
	    }

	    throw LQX::RuntimeException("No call found for entry `%s' with destination entry `%s'.", lo->description().c_str(), destEntry);
	    return LQX::Symbol::encodeNull();
	}
    };

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- [Doucment] */
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
#pragma mark -

    class LQXDocument : public LQX::LanguageObject {
    protected:
	typedef double (DOM::Document::*get_double_fptr)() const;
	typedef unsigned int (DOM::Document::*get_unsigned_fptr)() const;
	typedef bool (DOM::Document::*get_bool_fptr)() const;
	typedef double (DOM::Document::*get_clock_fptr)() const;
	typedef const DOM::ExternalVariable * (DOM::Document::*get_extvar_fptr)() const;

	struct attribute_table_t
	{
	private:
	    enum class result_is { EMPTY, DOUBLE, BOOL, UNSIGNED, CLOCK, EXTVAR };

	public:
	    attribute_table_t() : _t(result_is::EMPTY) {}
	    attribute_table_t( get_double_fptr f )   : _t(result_is::DOUBLE)   { fptr.r_double = f; }
	    attribute_table_t( get_unsigned_fptr f ) : _t(result_is::UNSIGNED) { fptr.r_unsigned = f; }
	    attribute_table_t( get_bool_fptr f )     : _t(result_is::BOOL)     { fptr.r_bool = f; }
	    attribute_table_t( get_extvar_fptr f )   : _t(result_is::EXTVAR)   { fptr.r_extvar = f; }

	    bool operator()( const char * s1, const char * s2 ) const { return strcasecmp( s1, s2 ) < 0; }
	    LQX::SymbolAutoRef operator()( const DOM::Document& document ) const
		{
		    switch ( _t ) {
		    case result_is::DOUBLE:   return LQX::Symbol::encodeDouble( (document.*fptr.r_double)() );
		    case result_is::UNSIGNED: return LQX::Symbol::encodeDouble( static_cast<double>((document.*fptr.r_unsigned)()) );
		    case result_is::CLOCK:    return LQX::Symbol::encodeDouble( static_cast<double>((document.*fptr.r_clock)()) );
		    case result_is::BOOL:     return LQX::Symbol::encodeBoolean( (document.*fptr.r_bool)() );
		    case result_is::EXTVAR: {
			/* This is sneaky... If the external variable is writable, then we can set it using it's document property.
			 * Otherwise, it is read-only */
			const DOM::ExternalVariable * var = (document.*fptr.r_extvar)();
			const DOM::SymbolExternalVariable * sym = dynamic_cast<const DOM::SymbolExternalVariable *>(var);
			if ( sym ) {
			    return sym->_externalSymbol;
			} else if ( var && var->wasSet() ) {
			    switch ( var->getType() ) {
			    case DOM::ExternalVariable::Type::DOUBLE: return LQX::Symbol::encodeDouble(to_double(*var));
			    case DOM::ExternalVariable::Type::STRING: return LQX::Symbol::encodeString(to_string(*var));
			    default: break;
			    }
			}
			/* Fall through to default if not set */
		    }
		    default: return LQX::Symbol::encodeNull();
		}
	    }

	    union {
		get_double_fptr r_double;
		get_unsigned_fptr r_unsigned;
		get_bool_fptr r_bool;
		get_clock_fptr r_clock;
		get_extvar_fptr r_extvar;
	    } fptr;
	    result_is _t;
	};

    public:

	const static uint32_t kLQXDocumentObjectTypeId = 10+10;

	/* Designated Initializers */
	LQXDocument(const DOM::Document * doc) : LQX::LanguageObject(kLQXDocumentObjectTypeId), _document(doc)
	    {
	    }

	virtual ~LQXDocument()
	    {
	    }


	/* Comparison and Operators */
	virtual bool isEqualTo(const LQX::LanguageObject* other) const
	    {
		const LQXDocument* document = dynamic_cast<const LQXDocument *>(other);
		return document && document->_document == _document;	/* Return a comparison of the types */
	    }

	virtual std::string description() const
	    {
		/* Return a description of the task */
		std::stringstream ss;
		ss << getTypeName() << "()";
		return ss.str();
	    }

	virtual std::string getTypeName() const
	    {
		return "Document";
	    }

	virtual LQX::SymbolAutoRef getPropertyNamed(LQX::Environment* env, const std::string& name)
	    {
		/* All the valid properties of documents */
		std::map<const std::string,attribute_table_t>::const_iterator attribute =  __attributeTable.find( name.c_str() );
		if ( attribute != __attributeTable.end() ) {
		    return attribute->second( *_document );
		}

		/* Anything we don't handle may be handled by our superclass */
		return this->LanguageObject::getPropertyNamed(env, name);
	    }

    private:
	const DOM::Document * _document;
	static const std::map<const std::string,attribute_table_t> __attributeTable;
    };

    const std::map<const std::string,LQXDocument::attribute_table_t> LQXDocument::__attributeTable =
    {
	{ __lqx_iterations,				attribute_table_t( &DOM::Document::getResultIterations ) },
	{ __lqx_user_time,      			attribute_table_t( &DOM::Document::getResultUserTime ) },
	{ __lqx_system_time, 				attribute_table_t( &DOM::Document::getResultSysTime ) },
	{ __lqx_elapsed_time,				attribute_table_t( &DOM::Document::getResultElapsedTime ) },
	{ "valid",	    				attribute_table_t( &DOM::Document::getResultValid ) },
	{ "invocation",	    				attribute_table_t( &DOM::Document::getResultInvocationNumber ) },
	{ "waits",				    	attribute_table_t( &DOM::Document::getResultMVAWait ) },
	{ "steps",				    	attribute_table_t( &DOM::Document::getResultMVAStep ) },
	{ DOM::Document::XComment,		    	attribute_table_t( &DOM::Document::getModelComment ) },
	{ DOM::Document::XConvergence,		    	attribute_table_t( &DOM::Document::getModelConvergence ) },
	{ DOM::Document::XIterationLimit,	    	attribute_table_t( &DOM::Document::getModelIterationLimit ) },
	{ DOM::Document::XUnderrelaxationCoefficient,	attribute_table_t( &DOM::Document::getModelUnderrelaxationCoefficient ) },
	{ DOM::Document::XPrintInterval,		attribute_table_t( &DOM::Document::getModelPrintInterval ) },
	{ DOM::Document::XSpexIterationLimit,		attribute_table_t( &DOM::Document::getSpexConvergenceIterationLimit ) },
	{ DOM::Document::XSpexUnderrelaxation,		attribute_table_t( &DOM::Document::getSpexConvergenceUnderrelaxation ) }
    };

    class LQXGetDocument : public LQX::Method {
    public:
	LQXGetDocument(const DOM::Document* doc) : _document(doc), _symbolCache() {}
	virtual ~LQXGetDocument() {}

	/* Basic information for the method itself */
	virtual std::string getName() const { return "document"; }
	virtual const char* getParameterInfo() const { return ""; }
	virtual std::string getHelp() const { return "Returns the document."; }

	/* Invocation of the method from the language */
	virtual LQX::SymbolAutoRef invoke(LQX::Environment* env, std::vector<LQX::SymbolAutoRef >& args) {

	    /* Return an encapsulated reference to the document */
	    const char* docName = "lqn-model";
	    if (_symbolCache.find(docName) != _symbolCache.end()) {
		return _symbolCache[docName];
	    }
	    LQXDocument* docObject = new LQXDocument(_document);
	    _symbolCache[docName] = LQX::Symbol::encodeObject(docObject, false);
	    return _symbolCache[docName];
	}

    private:
	const DOM::Document* _document;
	std::map<std::string,LQX::SymbolAutoRef> _symbolCache;
    };

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- [Pragma] */
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
#pragma mark -
    class LQXPragma : public LQXDocumentObject {
    public:

        const static uint32_t kLQXPragmaObjectTypeId = 10+11;

        /* Designated Initializers */
        LQXPragma(const std::string& value) : LQXDocumentObject(kLQXPragmaObjectTypeId,nullptr), _value(value)
            {
            }

        virtual ~LQXPragma()
            {
            }

        /* Comparison and Operators */
        virtual bool isEqualTo(const LQX::LanguageObject* other) const
            {
                const LQXPragma* pragma = dynamic_cast<const LQXPragma *>(other);
                return pragma && pragma->getDOMPragma() == getDOMPragma();  /* Return a comparison of the types */
            }

        virtual std::string description() const
            {
                /* Return a description of the pragma */
                std::stringstream ss;
                ss << getTypeName() << "(" << getDOMPragma() << ")";
		return ss.str();
	    }

	virtual std::string getTypeName() const
	    {
		return "pramga";
	    }

        const std::string& getDOMPragma() const { return _value; }


	virtual LQX::SymbolAutoRef getPropertyNamed(LQX::Environment* env, const std::string& name)
	    {
		/* All the valid properties of pragmas */
		if (name == "value") {
		    return LQX::Symbol::encodeString(_value.c_str());
		}

		/* Anything we don't handle may be handled by our superclass */
		return this->LanguageObject::getPropertyNamed(env, name);
	    }

    private:
	const std::string _value;
    };

    /*
     * A litle different than the others... Returns a string, not an object
     */

    class LQXGetPragma : public LQX::Method {
    public:
	LQXGetPragma( const DOM::Document* doc) : _document(doc), _symbolCache() {}
	virtual ~LQXGetPragma() {}

	/* Basic information for the method itself */
	virtual std::string getName() const { return "pragma"; }
	virtual const char* getParameterInfo() const { return "s"; }
	virtual std::string getHelp() const { return "Returns the value associated with the pragma"; }

	/* Invocation of the method from the language */
	virtual LQX::SymbolAutoRef invoke(LQX::Environment* env, std::vector<LQX::SymbolAutoRef >& args) {

	    /* Decode the arguments to the given method */
	    const std::string pragmaName = decodeString(args, 0);
	    if (_symbolCache.find(pragmaName) != _symbolCache.end()) {
		return _symbolCache[pragmaName];
	    }

	    const std::string value = _document->getPragma( pragmaName );
	    if ( value.empty() ) {
		return LQX::Symbol::encodeNull();	/* NOP */
	    }
	    LQXPragma* pragmaObject = new LQXPragma(value);
	    _symbolCache[pragmaName] = LQX::Symbol::encodeObject(pragmaObject, false);
	    return _symbolCache[pragmaName];
	}

    private:
	const DOM::Document* _document;
	std::map<std::string,LQX::SymbolAutoRef> _symbolCache;
    };

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
#pragma mark -
    class LQXConfidenceInterval : public LQXDocumentObject {
    public:

	const static uint32_t kLQXConfidenceIntervalObjectTypeId = 10+11;

	/* Designated Initializers */
	LQXConfidenceInterval(const DOM::DocumentObject * doc, const double conf_val ) : LQXDocumentObject(kLQXConfidenceIntervalObjectTypeId,doc), _conf_int()
	    {
		const unsigned long n_blocks = getDOMObject()->getDocument()->getResultNumberOfBlocks();
		if ( n_blocks > 2 ) {
		    _conf_int.set_blocks( n_blocks );
		    if ( conf_val == 95 ) {
			_conf_int.set_level(ConfidenceIntervals::CONF_95);
		    } else if ( conf_val == 99 ) {
			_conf_int.set_level(ConfidenceIntervals::CONF_99);
		    } else {
		    }
		}
	    }

	virtual ~LQXConfidenceInterval()
	    {
	    }

	/* Comparison and Operators */
	virtual bool isEqualTo(const LQX::LanguageObject* other) const
	    {
		const LQXConfidenceInterval* document = dynamic_cast<const LQXConfidenceInterval *>(other);
		return document && document->getDOMObject() == getDOMObject();	/* Return a comparison of the types */
	    }

	virtual std::string description() const
	    {
		/* Return a description of the task */
		std::stringstream ss;
		ss << "ConfidenceInterval(" << ")";
		return ss.str();
	    }

	virtual std::string getTypeName() const
	    {
		return "ConfidenceInterval";
	    }

	virtual LQX::SymbolAutoRef getPropertyNamed(LQX::Environment* env, const std::string& name)
	    {
		/* All the valid properties of document objects */
		std::map<const std::string,attribute_table_t>::const_iterator attribute =  __attributeTable.find( name );
		if ( attribute != __attributeTable.end() ) {
		    try {
			get_result_fptr variance = attribute->second.variance;
			const double value = _conf_int((getDOMObject()->*variance)());

			return LQX::Symbol::encodeDouble( value );
		    }
		    catch ( const LQIO::should_implement& e ) {
		    }
		}
		/* Anything we don't handle may be handled by our superclass */
		return this->LanguageObject::getPropertyNamed(env, name);
	    }

    private:
	ConfidenceIntervals _conf_int;
    };

    class LQXGetConfidenceInterval : public LQX::Method {
    public:
	LQXGetConfidenceInterval() : _symbolCache() {}
	virtual ~LQXGetConfidenceInterval() {}

	/* Basic information for the method itself */
	virtual std::string getName() const { return "conf_int"; }
	virtual const char* getParameterInfo() const { return "od"; }
	virtual std::string getHelp() const { return "Returns the confidence interval."; }

	/* Invocation of the method from the language */
	virtual LQX::SymbolAutoRef invoke(LQX::Environment* env, std::vector<LQX::SymbolAutoRef >& args) {

	    /* Return an encapsulated reference to the task */
	    LQXDocumentObject* lo = dynamic_cast<LQXDocumentObject *>(decodeObject(args, 0));
	    double conf_val = decodeDouble(args, 1);

	    const DOM::DocumentObject * domObject = lo->getDOMObject();
	    LQXConfidenceInterval* docObject = new LQXConfidenceInterval(domObject,conf_val);
	    const char* objectName = domObject->getName().c_str();
	    _symbolCache[objectName] = LQX::Symbol::encodeObject(docObject, false);
	    return _symbolCache[objectName];
	}

    private:
	std::map<std::string,LQX::SymbolAutoRef> _symbolCache;
    };

}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
#pragma mark -

namespace LQIO {

    void RegisterBindings(LQX::Environment* env, const DOM::Document* document)
    {
	LQX::MethodTable* mt = env->getMethodTable();
	mt->registerMethod(new LQXGetTask(document));
	mt->registerMethod(new LQXGetProcessor(document));
	mt->registerMethod(new LQXGetGroup(document));
	mt->registerMethod(new LQXGetEntry(document));
	mt->registerMethod(new LQXGetPhase());
	mt->registerMethod(new LQXGetCall());
	mt->registerMethod(new LQXGetForward());
	mt->registerMethod(new LQXGetActivity());
	mt->registerMethod(new LQXGetDocument(document));
	mt->registerMethod(new LQXGetConfidenceInterval());
	mt->registerMethod(new LQXGetPragma(document));
    }

}
