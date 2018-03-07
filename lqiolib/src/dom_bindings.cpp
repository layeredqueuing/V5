/*
 *  $Id: dom_bindings.cpp 13201 2018-03-05 23:45:30Z greg $
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

#include "dom_document.h"
#include "confidence_intervals.h"

#include <sstream>
#include <cstring>
namespace LQIO {

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- [Object] */
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */

    class LQXDocumentObject : public LQX::LanguageObject {
    protected:
	typedef double (DOM::DocumentObject::*get_result_fptr)() const;

	struct result_table_t
	{
	    result_table_t( get_result_fptr m=0, get_result_fptr v=0 ) : mean(m), variance(v) {}
	    bool operator()( const char * s1, const char * s2 ) const { return strcasecmp( s1, s2 ) < 0; }
	    LQX::SymbolAutoRef operator()( DOM::DocumentObject& domObject ) const { return LQX::Symbol::encodeDouble( (domObject.*mean)() ); }
	    get_result_fptr mean;
	    get_result_fptr variance;
	};

    public:
	LQXDocumentObject( uint32_t kLQXobject, DOM::DocumentObject * domObject ) : LQX::LanguageObject(kLQXobject), _domObject(domObject)
	    {
	    }

	virtual LQX::SymbolAutoRef getPropertyNamed(LQX::Environment* env, const std::string& name)
	    {
		std::map<const char *,result_table_t,result_table_t>::const_iterator attribute =  __attributeTable.find( name.c_str() );
		if ( attribute != __attributeTable.end() ) {
		    try {
			return attribute->second( *_domObject );
		    }
		    catch ( LQIO::should_implement e ) {
		    }
		}

		/* Anything we don't handle may be handled by our superclass */
		return this->LanguageObject::getPropertyNamed(env, name);
	    }

        DOM::DocumentObject* getDOMObject() const { return _domObject; }


	static void initializeTables() 
	    {
		if ( __attributeTable.size() != 0 ) return;

		__attributeTable["loss_probability"]               = result_table_t( &DOM::DocumentObject::getResultDropProbability,            &DOM::DocumentObject::getResultDropProbabilityVariance );
		__attributeTable["open_wait_time"]                 = result_table_t( &DOM::DocumentObject::getResultOpenWaitTime,               &DOM::DocumentObject::getResultOpenWaitTimeVariance );
		__attributeTable["phase1_proc_waiting"]            = result_table_t( &DOM::DocumentObject::getResultPhase1ProcessorWaiting,     &DOM::DocumentObject::getResultPhase1ProcessorWaitingVariance );
		__attributeTable["phase1_service_time"]            = result_table_t( &DOM::DocumentObject::getResultPhase1ServiceTime,          &DOM::DocumentObject::getResultPhase1ServiceTimeVariance );
		__attributeTable["phase1_service_time_variance"]   = result_table_t( &DOM::DocumentObject::getResultPhase1VarianceServiceTime,  &DOM::DocumentObject::getResultPhase1VarianceServiceTimeVariance );
		__attributeTable["phase1_utilization"]             = result_table_t( &DOM::DocumentObject::getResultPhase1Utilization,          &DOM::DocumentObject::getResultPhase1UtilizationVariance );
		__attributeTable["phase2_proc_waiting"]            = result_table_t( &DOM::DocumentObject::getResultPhase2ProcessorWaiting,     &DOM::DocumentObject::getResultPhase2ProcessorWaitingVariance );
		__attributeTable["phase2_service_time"]            = result_table_t( &DOM::DocumentObject::getResultPhase2ServiceTime,          &DOM::DocumentObject::getResultPhase2ServiceTimeVariance );
		__attributeTable["phase2_service_time_variance"]   = result_table_t( &DOM::DocumentObject::getResultPhase2VarianceServiceTime,  &DOM::DocumentObject::getResultPhase2VarianceServiceTimeVariance );
		__attributeTable["phase2_utilization"]             = result_table_t( &DOM::DocumentObject::getResultPhase2Utilization,          &DOM::DocumentObject::getResultPhase2UtilizationVariance );
		__attributeTable["phase3_proc_waiting"]            = result_table_t( &DOM::DocumentObject::getResultPhase3ProcessorWaiting,     &DOM::DocumentObject::getResultPhase3ProcessorWaitingVariance );
		__attributeTable["phase3_service_time"]            = result_table_t( &DOM::DocumentObject::getResultPhase3ServiceTime,          &DOM::DocumentObject::getResultPhase3ServiceTimeVariance );
		__attributeTable["phase3_service_time_variance"]   = result_table_t( &DOM::DocumentObject::getResultPhase3VarianceServiceTime,  &DOM::DocumentObject::getResultPhase3VarianceServiceTimeVariance );
		__attributeTable["phase3_utilization"]             = result_table_t( &DOM::DocumentObject::getResultPhase3Utilization,          &DOM::DocumentObject::getResultPhase3UtilizationVariance );
		__attributeTable["proc_utilization"]               = result_table_t( &DOM::DocumentObject::getResultProcessorUtilization,       &DOM::DocumentObject::getResultProcessorUtilizationVariance );
		__attributeTable["proc_waiting"]                   = result_table_t( &DOM::DocumentObject::getResultProcessorWaiting,           &DOM::DocumentObject::getResultProcessorWaitingVariance );
		__attributeTable["rwlock_reader_holding"]          = result_table_t( &DOM::DocumentObject::getResultReaderHoldingTime,          &DOM::DocumentObject::getResultReaderHoldingTimeVariance );
		__attributeTable["rwlock_reader_holding_variance"] = result_table_t( &DOM::DocumentObject::getResultVarianceReaderHoldingTime,  &DOM::DocumentObject::getResultVarianceReaderHoldingTimeVariance );
		__attributeTable["rwlock_reader_utilization"]      = result_table_t( &DOM::DocumentObject::getResultReaderHoldingUtilization,   &DOM::DocumentObject::getResultReaderHoldingUtilizationVariance );
		__attributeTable["rwlock_reader_waiting"]          = result_table_t( &DOM::DocumentObject::getResultReaderBlockedTime,          &DOM::DocumentObject::getResultReaderBlockedTimeVariance );
		__attributeTable["rwlock_reader_waiting_variance"] = result_table_t( &DOM::DocumentObject::getResultVarianceReaderBlockedTime,  &DOM::DocumentObject::getResultVarianceReaderBlockedTimeVariance );
		__attributeTable["rwlock_writer_holding"]          = result_table_t( &DOM::DocumentObject::getResultWriterHoldingTime,          &DOM::DocumentObject::getResultWriterHoldingTimeVariance );
		__attributeTable["rwlock_writer_holding_variance"] = result_table_t( &DOM::DocumentObject::getResultVarianceWriterHoldingTime,  &DOM::DocumentObject::getResultVarianceWriterHoldingTimeVariance );
		__attributeTable["rwlock_writer_utilization"]      = result_table_t( &DOM::DocumentObject::getResultWriterHoldingUtilization,   &DOM::DocumentObject::getResultWriterHoldingUtilizationVariance );
		__attributeTable["rwlock_writer_waiting"]          = result_table_t( &DOM::DocumentObject::getResultWriterBlockedTime,          &DOM::DocumentObject::getResultWriterBlockedTimeVariance );
		__attributeTable["rwlock_writer_waiting_variance"] = result_table_t( &DOM::DocumentObject::getResultVarianceWriterBlockedTime,  &DOM::DocumentObject::getResultVarianceWriterBlockedTimeVariance );
		__attributeTable["semaphore_utilization"]          = result_table_t( &DOM::DocumentObject::getResultHoldingUtilization,         &DOM::DocumentObject::getResultHoldingUtilizationVariance );
		__attributeTable["semaphore_waiting"]              = result_table_t( &DOM::DocumentObject::getResultHoldingTime,                &DOM::DocumentObject::getResultHoldingTimeVariance );
		__attributeTable["semaphore_waiting_variance"]     = result_table_t( &DOM::DocumentObject::getResultVarianceHoldingTime,        &DOM::DocumentObject::getResultVarianceHoldingTimeVariance );
		__attributeTable["service_time"]                   = result_table_t( &DOM::DocumentObject::getResultServiceTime,                &DOM::DocumentObject::getResultServiceTimeVariance );
		__attributeTable["service_time_variance"]          = result_table_t( &DOM::DocumentObject::getResultVarianceServiceTime,        &DOM::DocumentObject::getResultVarianceServiceTimeVariance );
		__attributeTable["squared_coeff_variation"]        = result_table_t( &DOM::DocumentObject::getResultSquaredCoeffVariation,      0 );
		__attributeTable["throughput"]                     = result_table_t( &DOM::DocumentObject::getResultThroughput,                 &DOM::DocumentObject::getResultThroughputVariance );
		__attributeTable["throughput_bound"]               = result_table_t( &DOM::DocumentObject::getResultThroughputBound,            0 );
		__attributeTable["utilization"]                    = result_table_t( &DOM::DocumentObject::getResultUtilization,                &DOM::DocumentObject::getResultUtilizationVariance );
		__attributeTable["waiting"]                        = result_table_t( &DOM::DocumentObject::getResultWaitingTime,                &DOM::DocumentObject::getResultWaitingTimeVariance );
		__attributeTable["waiting_variance"]               = result_table_t( &DOM::DocumentObject::getResultVarianceWaitingTime,        &DOM::DocumentObject::getResultVarianceWaitingTimeVariance );
	    }

    protected:
        DOM::DocumentObject * _domObject;
        static std::map<const char *,result_table_t,result_table_t> __attributeTable;

    };

    std::map<const char *,LQXDocumentObject::result_table_t,LQXDocumentObject::result_table_t> LQXDocumentObject::__attributeTable;

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- [Processor] */
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
#pragma mark -

    class LQXProcessor : public LQXDocumentObject {
    public:

        const static uint32_t kLQXProcessorObjectTypeId = 10+1;

        /* Designated Initializers */
        LQXProcessor(DOM::Processor* proc) : LQXDocumentObject(kLQXProcessorObjectTypeId,proc)
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

        virtual std::string description()
            {
                /* Return a description of the task */
                std::stringstream ss;
                ss << "Processor(" << getDOMProcessor()->getName() << ")";
		return ss.str();
	    }

	virtual std::string hashableString()
	    {
		/* Return the hashable string */
		std::stringstream ss;
		ss << "!!__external__(Processor)&&n=" << getDOMProcessor()->getName();
		return ss.str();
	    }

	virtual std::string getTypeName()
	    {
		return "Processor";
	    }

        DOM::Processor* getDOMProcessor() const { return dynamic_cast<DOM::Processor*>(_domObject); }


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
	LQXGetProcessor(DOM::Document* doc) : _document(doc), _symbolCache() {}
	virtual ~LQXGetProcessor() {}

	/* Basic information for the method itself */
	virtual std::string getName() const { return "processor"; }
	virtual const char* getParameterInfo() const { return "s"; }
	virtual std::string getHelp() const { return "Returns the processor associated with a name."; }

	/* Invocation of the method from the language */
	virtual LQX::SymbolAutoRef invoke(LQX::Environment* env,
					  std::vector<LQX::SymbolAutoRef >& args) {

	    /* Decode the name of the task and look it up in cache */
	    const char* procName = decodeString(args, 0);
	    if (_symbolCache.find(procName) != _symbolCache.end()) {
		return _symbolCache[procName];
	    }

	    /* Obtain the task reference  */
	    DOM::Processor* proc = _document->getProcessorByName(procName);

	    /* There was no task given */
	    if (proc == NULL) {
		printf("warning: No processor specified with name %s\n", procName);
		return LQX::Symbol::encodeNull();
	    }

	    /* Return an encapsulated reference to the task */
	    LQXProcessor* procObject = new LQXProcessor(proc);
	    _symbolCache[procName] = LQX::Symbol::encodeObject(procObject, false);
	    return _symbolCache[procName];
	}

    private:
	DOM::Document* _document;
	std::map<std::string,LQX::SymbolAutoRef> _symbolCache;
    };

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- [Task] */
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */

    class LQXTask : public LQXDocumentObject
    {
    public:

	const static uint32_t kLQXTaskObjectTypeId = 10+0;

	/* Designated Initializers */
        LQXTask(DOM::Task* task) : LQXDocumentObject(kLQXTaskObjectTypeId,task)
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

	virtual std::string description()
	    {
		/* Return a description of the task */
		std::string s = getTypeName();
		s += "(";
		s += getDOMTask()->getName();
		s += ")";
		return s;
	    }

	virtual std::string hashableString()
	    {
		/* Return the hashable string */
		std::stringstream ss;
		ss << "!!__external__(Task)&&n=" << getDOMTask()->getName();
		return ss.str();
	    }

	virtual std::string getTypeName()
	    {
		return "Task";
	    }

        DOM::Task* getDOMTask() const { return dynamic_cast<DOM::Task*>(_domObject); }

    private:
    };

    class LQXGetTask : public LQX::Method {
    public:
	LQXGetTask(DOM::Document* doc) : _document(doc), _symbolCache() {}
	virtual ~LQXGetTask() {}

	/* Basic information for the method itself */
	virtual std::string getName() const { return "task"; }
	virtual const char* getParameterInfo() const { return "s"; }
	virtual std::string getHelp() const { return "Returns the task associated with a name."; }

	/* Invocation of the method from the language */
	virtual LQX::SymbolAutoRef invoke(LQX::Environment* env,
					  std::vector<LQX::SymbolAutoRef >& args) {

	    /* Decode the name of the task and look it up in cache */
	    const char* taskName = decodeString(args, 0);
	    if (_symbolCache.find(taskName) != _symbolCache.end()) {
		return _symbolCache[taskName];
	    }

	    /* Obtain the task reference  */
	    DOM::Task* task = _document->getTaskByName(taskName);

	    /* There was no task given */
	    if (task == NULL) {
		printf("warning: No task specified with name %s\n", taskName);
		return LQX::Symbol::encodeNull();
	    }

	    /* Return an encapsulated reference to the task */
	    LQXTask* taskObject = new LQXTask(task);
	    _symbolCache[taskName] = LQX::Symbol::encodeObject(taskObject, false);
	    return _symbolCache[taskName];
	}

    private:
	DOM::Document* _document;
	std::map<std::string,LQX::SymbolAutoRef> _symbolCache;
    };

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
#pragma mark -

    class LQXEntry : public LQXDocumentObject {
    public:

	const static uint32_t kLQXEntryObjectTypeId = 10+2;

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

	virtual std::string description()
	    {
		/* Return a description of the task */
		std::stringstream ss;
		ss << "Entry(" << getDOMEntry()->getName() << ")";
		return ss.str();
	    }

	virtual std::string hashableString()
	    {
		/* Return the hashable string */
		std::stringstream ss;
		ss << "!!__external__(Entry)&&n=" << getDOMEntry()->getName();
		return ss.str();
	    }

	virtual std::string getTypeName()
	    {
		return "Entry";
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

        DOM::Entry* getDOMEntry() const { return dynamic_cast<DOM::Entry*>(_domObject); }

    };

    class LQXGetEntry : public LQX::Method {
    public:
	LQXGetEntry(DOM::Document* doc) : _document(doc), _symbolCache() {}
	virtual ~LQXGetEntry() {}

	/* Basic information for the method itself */
	virtual std::string getName() const { return "entry"; }
	virtual const char* getParameterInfo() const { return "s"; }
	virtual std::string getHelp() const { return "Returns the entry with the given name."; }

	/* Invocation of the method from the language */
	virtual LQX::SymbolAutoRef invoke(LQX::Environment* env,
					  std::vector<LQX::SymbolAutoRef >& args) {

	    /* Decode the name of the task and look it up in cache */
	    const char* entryName = decodeString(args, 0);
	    if (_symbolCache.find(entryName) != _symbolCache.end()) {
		return _symbolCache[entryName];
	    }

	    /* Obtain the task reference  */
	    DOM::Entry* entry = _document->getEntryByName(entryName);

	    /* There was no task given */
	    if (entry == NULL) {
		printf("warning: No entry specified with name %s\n", entryName);
		return LQX::Symbol::encodeNull();
	    }

	    /* Return an encapsulated reference to the task */
	    LQXEntry* entryObject = new LQXEntry(entry);
	    _symbolCache[entryName] = LQX::Symbol::encodeObject(entryObject, false);
	    return _symbolCache[entryName];
	}

    private:
	DOM::Document* _document;
	std::map<std::string,LQX::SymbolAutoRef> _symbolCache;
    };

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
#pragma mark -

    class LQXPhase : public LQXDocumentObject {
    public:

	const static uint32_t kLQXPhaseObjectTypeId = 10+2;

	/* Designated Initializers */
	LQXPhase(DOM::Phase* phase) : LQXDocumentObject(kLQXPhaseObjectTypeId,phase)
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

	virtual std::string description()
	    {
		/* Return a description of the task */
		std::stringstream ss;
		ss << "Phase(n)";
		return ss.str();
	    }

	virtual std::string hashableString()
	    {
		/* Return the hashable string */
		std::stringstream ss;
		ss << "!!__external__(Phase)&&n=" << (void *)getDOMPhase();
		return ss.str();
	    }

	virtual std::string getTypeName()
	    {
		return "Phase";
	    }

        DOM::Phase* getDOMPhase() const { return dynamic_cast<DOM::Phase*>(_domObject); }

    };

// [MM] These should be cached.
    class LQXGetPhase : public LQX::Method {
    public:
	LQXGetPhase() {}
	virtual ~LQXGetPhase() {}

	/* Basic information for the method itself */
	virtual std::string getName() const { return "phase"; }
	virtual const char* getParameterInfo() const { return "od"; }
	virtual std::string getHelp() const { return "Returns the given phase of the given entry."; }

	/* Invocation of the method from the language */
	virtual LQX::SymbolAutoRef invoke(LQX::Environment* env,
					  std::vector<LQX::SymbolAutoRef >& args) {

	    /* Decode the arguments to the given method */
	    LQX::LanguageObject* lo = decodeObject(args, 0);
	    double phase = decodeDouble(args, 1);
	    LQXEntry* entry = dynamic_cast<LQXEntry *>(lo);

	    /* Make sure that what we have is an entry */
	    if (!entry) {
		printf("warning: Argument 1 to phase(object,double) is not an entry.\n");
		return LQX::Symbol::encodeNull();
	    }

	    /* Obtain the phase for the entry */
	    DOM::Entry* domEntry = entry->getDOMEntry();
	    DOM::Phase* domPhase = domEntry->getPhase((unsigned)phase);
	    return LQX::Symbol::encodeObject(new LQXPhase(domPhase), false);
	}
    };

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
#pragma mark -

    class LQXActivity : public LQXDocumentObject {
    public:

	const static uint32_t kLQXActivityObjectTypeId = 10+4;

	/* Designated Initializers */
	LQXActivity(DOM::Activity* act) : LQXDocumentObject(kLQXActivityObjectTypeId,act)
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

	virtual std::string description()
	    {
		/* Return a description of the task */
		std::stringstream ss;
		ss << "Activity(" << getDOMActivity()->getName() << ")";
		return ss.str();
	    }

	virtual std::string hashableString()
	    {
		/* Return the hashable string */
		std::stringstream ss;
		ss << "!!__external__(Activity)&&n=" << (void *)getDOMActivity();
		return ss.str();
	    }

	virtual std::string getTypeName()
	    {
		return "Activity";
	    }

        DOM::Activity* getDOMActivity() const { return dynamic_cast<DOM::Activity*>(_domObject); }
    };

// [MM] These should be cached.
    class LQXGetActivity : public LQX::Method {
    public:
	LQXGetActivity() {}
	virtual ~LQXGetActivity() {}

	/* Basic information for the method itself */
	virtual std::string getName() const { return "activity"; }
	virtual const char* getParameterInfo() const { return "os"; }
	virtual std::string getHelp() const { return "Returns the activity with the given name."; }

	/* Invocation of the method from the language */
	virtual LQX::SymbolAutoRef invoke(LQX::Environment* env,
					  std::vector<LQX::SymbolAutoRef >& args) {

	    /* Decode the arguments to the given method */
	    LQX::LanguageObject* lo = decodeObject(args, 0);
	    const char* actName = decodeString(args, 1);
	    LQXTask* task = dynamic_cast<LQXTask *>(lo);

	    /* Make sure that what we have is an entry */
	    if (!task) {
		printf("warning: Argument 1 to activity(object,string) is not a task.\n");
		return LQX::Symbol::encodeNull();
	    }

	    /* Obtain the phase for the entry */
	    DOM::Task* domTask = task->getDOMTask();
	    DOM::Activity* domAct = domTask->getActivity(actName,false);

	    /* Make sure we got one */
	    if (domAct == NULL) {
		printf("warning: No activity named %s for task %s.\n",
		       domTask->getName().c_str(), domAct->getName().c_str());
		return LQX::Symbol::encodeNull();
	    }

	    return LQX::Symbol::encodeObject(new LQXActivity(domAct), false);
	}
    };

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
#pragma mark -

    class LQXCall : public LQXDocumentObject {
    public:

	const static uint32_t kLQXCallObjectTypeId = 10+3;

	/* Designated Initializers */
	LQXCall(DOM::Call* call) : LQXDocumentObject(kLQXCallObjectTypeId,call)
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

	virtual std::string description()
	    {
		/* Return a description of the task */
		std::stringstream ss;
		ss << "Call(" << const_cast<DOM::Entry*>(getDOMCall()->getSourceEntry())->getName() << "->"
		   << const_cast<DOM::Entry*>(getDOMCall()->getDestinationEntry())->getName() << ")";
		return ss.str();
	    }

	virtual std::string hashableString()
	    {
		/* Return the hashable string */
		std::stringstream ss;
		ss << "!!__external__(Call)&&n=" << (void *)getDOMCall();
		return ss.str();
	    }

	virtual std::string getTypeName()
	    {
		return "Call";
	    }

        DOM::Call* getDOMCall() const { return dynamic_cast<DOM::Call*>(_domObject); }
    };


// [MM] These should be cached.
    class LQXGetCall : public LQX::Method {
    public:
	LQXGetCall() {}
	virtual ~LQXGetCall() {}

	/* Basic information for the method itself */
	virtual std::string getName() const { return "call"; }
	virtual const char* getParameterInfo() const { return "os"; }
	virtual std::string getHelp() const { return "Returns the given call from entry to dest."; }

	/* Invocation of the method from the language */
	virtual LQX::SymbolAutoRef invoke(LQX::Environment* env,
					  std::vector<LQX::SymbolAutoRef >& args) {

	    /* Decode the arguments to the given method */
	    LQX::LanguageObject* lo = decodeObject(args, 0);
	    const char* destEntry = decodeString(args, 1);
	    LQXPhase* phase = dynamic_cast<LQXPhase *>(lo);

	    DOM::Phase * domPhase = 0;
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
		    return LQX::Symbol::encodeObject(new LQXCall((DOM::Call*)call), false);
		}
	    }

	    printf("warning: No call found for phase %s with destination entry %s.\n", lo->description().c_str(), destEntry);
	    return LQX::Symbol::encodeNull();
	}
    };

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
#pragma mark -

    class LQXDocument : public LQX::LanguageObject {
    protected:
	typedef double (DOM::Document::*get_double_fptr)() const;
	typedef unsigned int (DOM::Document::*get_unsigned_fptr)() const;
	typedef bool (DOM::Document::*get_bool_fptr)() const;
	typedef clock_t (DOM::Document::*get_clock_fptr)() const;

	struct result_table_t
	{
	private:
	    typedef enum { IS_NULL, IS_DOUBLE, IS_BOOL, IS_UNSIGNED, IS_CLOCK } result_t;

	public:
	    result_table_t() : _t(IS_NULL) {}
	    result_table_t( get_double_fptr f ) : _t(IS_DOUBLE) { fptr.r_double = f; }
	    result_table_t( get_unsigned_fptr f ) : _t(IS_UNSIGNED) { fptr.r_unsigned = f; }
	    result_table_t( get_bool_fptr f ) : _t(IS_BOOL) { fptr.r_bool = f; }
	    result_table_t( get_clock_fptr f ) : _t(IS_CLOCK) { fptr.r_clock = f; }

	    bool operator()( const char * s1, const char * s2 ) const { return strcasecmp( s1, s2 ) < 0; }
	    LQX::SymbolAutoRef operator()( LQIO::DOM::Document& document ) const 	
		{
		    switch ( _t ) {
		    case IS_DOUBLE: return LQX::Symbol::encodeDouble( (document.*fptr.r_double)() );
		    case IS_UNSIGNED: return LQX::Symbol::encodeDouble( static_cast<double>((document.*fptr.r_unsigned)()) );
		    case IS_CLOCK: return LQX::Symbol::encodeDouble( static_cast<double>((document.*fptr.r_clock)()) );
		    case IS_BOOL: return LQX::Symbol::encodeBoolean( (document.*fptr.r_bool)() );
		    case IS_NULL: return LQX::Symbol::encodeNull();
		    }
		}

	    union {
		get_double_fptr r_double;
		get_unsigned_fptr r_unsigned;
		get_bool_fptr r_bool;
		get_clock_fptr r_clock;
	    } fptr;
	    result_t _t;
	};

    public:

	const static uint32_t kLQXDocumentObjectTypeId = 10+5;

	/* Designated Initializers */
	LQXDocument(DOM::Document * doc) : LQX::LanguageObject(kLQXDocumentObjectTypeId), _document(doc)
	    {
	    }

	virtual ~LQXDocument()
	    {
	    }

	static void initializeTables() 
	    {
		if ( __attributeTable.size() != 0 ) return;

		__attributeTable["iterations"]	    = result_table_t( &DOM::Document::getResultIterations );
		__attributeTable["user_cpu_time"]   = result_table_t( &DOM::Document::getResultUserTime );
		__attributeTable["system_cpu_time"] = result_table_t( &DOM::Document::getResultSysTime );
		__attributeTable["elapsed_time"]    = result_table_t( &DOM::Document::getResultElapsedTime );
		__attributeTable["valid"]	    = result_table_t( &DOM::Document::getResultValid );
		__attributeTable["invocation"]	    = result_table_t( &DOM::Document::getResultInvocationNumber );
		__attributeTable["waits"]	    = result_table_t( &DOM::Document::getResultMVAWait );
	    }

	/* Comparison and Operators */
	virtual bool isEqualTo(const LQX::LanguageObject* other) const
	    {
		const LQXDocument* document = dynamic_cast<const LQXDocument *>(other);
		return document && document->_document == _document;	/* Return a comparison of the types */
	    }

	virtual std::string description()
	    {
		/* Return a description of the task */
		std::stringstream ss;
		ss << "Document(" << ")";
		return ss.str();
	    }

	virtual std::string hashableString()
	    {
		/* Return the hashable string */
		std::stringstream ss;
		ss << "!!__external__(Document)&&n=";
		return ss.str();
	    }

	virtual std::string getTypeName()
	    {
		return "Document";
	    }

	virtual LQX::SymbolAutoRef getPropertyNamed(LQX::Environment* env, const std::string& name)
	    {
		/* All the valid properties of documents */
		std::map<const char *,result_table_t,result_table_t>::const_iterator attribute =  __attributeTable.find( name.c_str() );
		if ( attribute != __attributeTable.end() ) {
		    return attribute->second( *_document );
		}

		/* Anything we don't handle may be handled by our superclass */
		return this->LanguageObject::getPropertyNamed(env, name);
	    }

    private:
	DOM::Document * _document;
	static std::map<const char *,result_table_t,result_table_t> __attributeTable;
    };

    std::map<const char *,LQXDocument::result_table_t,LQXDocument::result_table_t> LQXDocument::__attributeTable;

    class LQXGetDocument : public LQX::Method {
    public:
	LQXGetDocument(DOM::Document* doc) : _document(doc), _symbolCache() {}
	virtual ~LQXGetDocument() {}

	/* Basic information for the method itself */
	virtual std::string getName() const { return "document"; }
	virtual const char* getParameterInfo() const { return ""; }
	virtual std::string getHelp() const { return "Returns the document."; }

	/* Invocation of the method from the language */
	virtual LQX::SymbolAutoRef invoke(LQX::Environment* env,
					  std::vector<LQX::SymbolAutoRef >& args) {

	    /* Return an encapsulated reference to the task */
	    LQXDocument* docObject = new LQXDocument(_document);
	    const char* docName = "lqn-model";
	    _symbolCache[docName] = LQX::Symbol::encodeObject(docObject, false);
	    return _symbolCache[docName];
	}

    private:
	DOM::Document* _document;
	std::map<std::string,LQX::SymbolAutoRef> _symbolCache;
    };

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
#pragma mark -
    class LQXConfidenceInterval : public LQXDocumentObject {
    public:

	const static uint32_t kLQXConfidenceIntervalObjectTypeId = 10+6;

	/* Designated Initializers */
	LQXConfidenceInterval(DOM::DocumentObject * doc, const double conf_val ) : LQXDocumentObject(kLQXConfidenceIntervalObjectTypeId,doc), _conf_int()
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

	virtual std::string description()
	    {
		/* Return a description of the task */
		std::stringstream ss;
		ss << "ConfidenceInterval(" << ")";
		return ss.str();
	    }

	virtual std::string hashableString()
	    {
		/* Return the hashable string */
		std::stringstream ss;
		ss << "!!__external__(ConfidenceInterval)&&n=";
		return ss.str();
	    }

	virtual std::string getTypeName()
	    {
		return "ConfidenceInterval";
	    }

	virtual LQX::SymbolAutoRef getPropertyNamed(LQX::Environment* env, const std::string& name)
	    {
		/* All the valid properties of document objects */
		std::map<const char *,result_table_t,result_table_t>::const_iterator attribute =  __attributeTable.find( name.c_str() );
		if ( attribute != __attributeTable.end() ) {
		    try {
			get_result_fptr variance = attribute->second.variance;
			const double value = _conf_int((getDOMObject()->*variance)());

			return LQX::Symbol::encodeDouble( value );
		    }
		    catch ( LQIO::should_implement e ) {
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
	virtual LQX::SymbolAutoRef invoke(LQX::Environment* env,
					  std::vector<LQX::SymbolAutoRef >& args) {

	    /* Return an encapsulated reference to the task */
	    LQXDocumentObject* lo = dynamic_cast<LQXDocumentObject *>(decodeObject(args, 0));
	    double conf_val = decodeDouble(args, 1);

	    DOM::DocumentObject * domObject = lo->getDOMObject();
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

    void RegisterBindings(LQX::Environment* env, DOM::Document* document)
    {
	LQXDocumentObject::initializeTables();
	LQXDocument::initializeTables();

	LQX::MethodTable* mt = env->getMethodTable();
	mt->registerMethod(new LQXGetTask(document));
	mt->registerMethod(new LQXGetProcessor(document));
	mt->registerMethod(new LQXGetEntry(document));
	mt->registerMethod(new LQXGetPhase());
	mt->registerMethod(new LQXGetCall());
	mt->registerMethod(new LQXGetActivity());
	mt->registerMethod(new LQXGetDocument(document));
	mt->registerMethod(new LQXGetConfidenceInterval());
    }

}
