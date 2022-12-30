/*
 *  $Id: dom_document.cpp 16212 2022-12-30 20:39:19Z greg $
 *
 *  Created by Martin Mroz on 24/02/09.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#if HAVE_CONFIG_H
#include <config.h>
#endif
#include "dom_document.h"
#include <fstream>
#include <cstring>
#include <cstdlib>
#include <cmath>
#include <algorithm>
#include <numeric>
#if HAVE_SYS_UTSNAME_H
#include <sys/utsname.h>
#endif
#include "glblerr.h"
#if HAVE_LIBEXPAT
#include "expat_document.h"
#include "jmva_document.h"
#endif
#include "dom_activity.h"
#include "dom_actlist.h"
#include "dom_call.h"
#include "dom_entry.h"
#include "dom_group.h"
#include "dom_processor.h"
#include "dom_task.h"
#include "filename.h"
#include "json_document.h"
#include "srvn_input.h"
#include "srvn_output.h"
#include "srvn_results.h"
#include "srvn_spex.h"

extern void ModLangParserTrace(FILE *TraceFILE, char *zTracePrompt);

namespace LQIO {
    lqio_params_stats io_vars(VERSION,nullptr);

    namespace DOM {
	Document* __document = nullptr;
	bool Document::__debugXML = false;
	bool Document::__debugJSON = false;
	std::map<const char *, double> Document::__initialValues;
	std::string Document::__input_file_name = "";
	const char * Document::XComment = "comment";
	const char * Document::XConvergence = "conv_val";			/* Matches schema. 	*/
	const char * Document::XIterationLimit = "it_limit";			/* Matched schema.	*/
	const char * Document::XPrintInterval = "print_int";			/* Matches schema.	*/
	const char * Document::XUnderrelaxationCoefficient = "underrelax_coeff";/* Matches schema.	*/
	const char * Document::XSpexIterationLimit = "spex_it_limit";
	const char * Document::XSpexUnderrelaxation = "spex_underrelax_coeff";

	const std::map<const LQIO::DOM::Document::OutputFormat,const std::string> Document::__output_extensions = {
	    { OutputFormat::XML,	"lqxo" },
	    { OutputFormat::JSON,	"lqjo" },
	    { OutputFormat::PARSEABLE,	"p" },
	    { OutputFormat::QNAP2,	"qnap" }
	};
	const std::map<const LQIO::DOM::Document::InputFormat,const LQIO::DOM::Document::OutputFormat> Document::__input_to_output_format = {
	    { InputFormat::XML,		OutputFormat::XML },
	    { InputFormat::JSON,	OutputFormat::JSON },
	    { InputFormat::LQN,		OutputFormat::XML },
	    { InputFormat::JMVA,	OutputFormat::JMVA },
	    { InputFormat::QNAP2,	OutputFormat::QNAP2 }
	};
	const std::map<const std::string,const LQIO::DOM::Document::InputFormat> Document::__extensions_input = {
	    { "in",			InputFormat::LQN },
	    { "jmva",			InputFormat::JMVA },
	    { "json",			InputFormat::JSON },
	    { "lqj",			InputFormat::JSON },
	    { "lqjo",			InputFormat::JSON },
	    { "lqn",			InputFormat::LQN },
	    { "lqnj",			InputFormat::JSON },
	    { "lqnx",			InputFormat::XML },
	    { "lqx",			InputFormat::XML },
	    { "lqxo",			InputFormat::XML },
	    { "qnp",			InputFormat::QNAP2 },
	    { "qnap",			InputFormat::QNAP2 },
	    { "qnap2",			InputFormat::QNAP2 },
	    { "spex",			InputFormat::LQN },
	    { "txt",			InputFormat::LQN },
	    { "xlqn",			InputFormat::LQN },
	    { "xml",			InputFormat::XML }
	};
	


	Document::Document( InputFormat format )
	    : _extraComment(),
	      _processors(), _groups(), _tasks(), _entries(),
	      _entities(), _variables(), _controlVariables(), _nextEntityId(0),
	      _format(format),
	      _lqxProgram(""), _lqxProgramLineNumber(0), _parsedLQXProgram(nullptr), _instantiated(false), _pragmas(),
	      _maximumPhase(0), _hasResults(false),
	      _hasRendezvous(cached::NOT_SET), _hasSendNoReply(cached::NOT_SET), _taskHasAndJoin(cached::NOT_SET),		/* Cached valuess */
	      _resultValid(false), _hasConfidenceIntervals(false), _hasBottleneckStrength(false),
	      _resultInvocationNumber(0),
	      _resultConvergenceValue(0.0),
	      _resultIterations(0),
	      _resultUserTime(0),
	      _resultSysTime(0),
	      _resultElapsedTime(0),
	      _resultMaxRSS(0)
	{
	    __document = this;

	    __initialValues[XConvergence] =                 0.00001;
	    __initialValues[XIterationLimit] =              50.;
	    __initialValues[XPrintInterval] =               10.;
	    __initialValues[XUnderrelaxationCoefficient] =  0.9;
	    __initialValues[XSpexIterationLimit] =          50.;
	    __initialValues[XSpexUnderrelaxation] =         0.9;
	}


	Document::~Document()
	{
	    /* Delete all of the processors (deletes tasks) */

	    for ( std::map<std::string, Processor*>::iterator proc = _processors.begin(); proc != _processors.end(); ++proc ) {
		delete proc->second;
	    }

	    /* Make sure that we only delete entries once */
	    for ( std::map<std::string, Entry*>::iterator entry = _entries.begin(); entry != _entries.end(); ++entry ) {
		delete entry->second;
	    }

	    /* Now, delete all of the groups */

	    for ( std::map<std::string, Group*>::iterator group = _groups.begin(); group != _groups.end(); ++group ) {
		delete group->second;
	    }

	    /* BUG_277 Delete External Variables */

	    for ( std::map<const std::string, SymbolExternalVariable*>::const_iterator var = _variables.begin(); var != _variables.end(); ++var ) {
		delete var->second;
	    }

	    /* Now delete the control variables */
	    for ( std::map<const std::string, const ExternalVariable*>::const_iterator var = _controlVariables.begin(); var != _controlVariables.end(); ++var ) {
		delete var->second;
	    }

	    LQIO::Spex::clear();

	    __document = nullptr;
	    __input_file_name = "";
	}

	void Document::setModelParameters(const std::string& comment, LQIO::DOM::ExternalVariable* conv_val, LQIO::DOM::ExternalVariable* it_limit, LQIO::DOM::ExternalVariable* print_int, LQIO::DOM::ExternalVariable* underrelax_coeff, const void * element )
	{
	    /* Set up initial model parameters, but only if they were not set using SPEX variables */

	    const ExternalVariable * var = _controlVariables[XComment];
	    if ( !var ) {
		_controlVariables[XComment] = new ConstantExternalVariable( comment.c_str() );
	    }
	    var = _controlVariables[XConvergence];
	    if ( !var && conv_val ) {
		_controlVariables[XConvergence] = conv_val;
	    }
	    var = _controlVariables[XIterationLimit];
	    if ( !var && it_limit ) {
		_controlVariables[XIterationLimit] = it_limit;
	    }
	    var = _controlVariables[XPrintInterval];
	    if ( !var && print_int ) {
		_controlVariables[XPrintInterval] = print_int;
	    }
	    var = _controlVariables[XUnderrelaxationCoefficient];
	    if ( !var && underrelax_coeff ) {
		_controlVariables[XUnderrelaxationCoefficient] = underrelax_coeff;
	    }
	}

	std::string Document::getModelCommentString() const
	{
	    const char * s;
	    const std::map<const std::string, const ExternalVariable *>::const_iterator iter = _controlVariables.find(XComment);
	    if ( iter != _controlVariables.end() && iter->second != nullptr && iter->second->wasSet() ) {
		if ( iter->second->getString( s ) ) return std::string(s);
	    }
	    return std::string("");
	}

	Document& Document::setModelComment( const ExternalVariable * comment )
	{
	    _controlVariables[XComment] = comment;
	    return *this;
	}

	Document& Document::setModelCommentString( const std::string& comment )
	{
	    _controlVariables[XComment] = new ConstantExternalVariable( comment.c_str() );
	    return *this;
	}

	const std::string& Document::getExtraComment() const
	{
	    return _extraComment;
	}

	Document& Document::setExtraComment( const std::string& value )
	{
	    _extraComment = value;
	    return *this;
	}

	Document& Document::setModelConvergence( const ExternalVariable * value )
	{
	    _controlVariables[XConvergence] = value;
	    return *this;
	}

	Document& Document::setModelIterationLimit( const ExternalVariable * value )
	{
	    _controlVariables[XIterationLimit] = value;
	    return *this;
	}

	Document& Document::setModelPrintInterval( const ExternalVariable * value )
	{
	    _controlVariables[XPrintInterval] = value;
	    return *this;
	}

	Document& Document::setModelUnderrelaxationCoefficient( const ExternalVariable * value )
	{
	    _controlVariables[XUnderrelaxationCoefficient] = value;
	    return *this;
	}

	Document& Document::setSpexConvergenceIterationLimit( const ExternalVariable * spexIterationLimit )
	{
	    _controlVariables[XSpexIterationLimit] = spexIterationLimit;
	    return *this;
	}

	Document& Document::setSpexConvergenceUnderrelaxation( const ExternalVariable * spexUnderrelaxation )
	{
	    _controlVariables[XSpexUnderrelaxation] = spexUnderrelaxation;
	    return *this;
	}

	const double Document::getValue( const char * index ) const
	{
	    /* Set to default value if NOT set elsewhere (usually the control program) */
	    double value = __initialValues[index];
	    const std::map<const std::string, const ExternalVariable *>::const_iterator iter = _controlVariables.find(index);
	    if ( iter != _controlVariables.end() ) {
		const ExternalVariable * var = iter->second;
		if ( var != nullptr && var->wasSet() ) {
		    var->getValue(value);
		}
	    }
	    return value;
	}

	const ExternalVariable * Document::get( const char * index ) const
	{
	    const std::map<const std::string, const ExternalVariable *>::const_iterator iter = _controlVariables.find(index);
	    if ( iter != _controlVariables.end() ) {
		const ExternalVariable * var = iter->second;
		if ( var ) {
		    return iter->second;
		}
	    }
	    return new ConstantExternalVariable( __initialValues[index] );
	}

	unsigned Document::getNextEntityId()
	{
	    /* Obtain the next valid identifier */
	    return _nextEntityId++;
	}

	void Document::addProcessorEntity(Processor* processor)
	{
	    /* Map in the processor entity */
	    _entities[processor->getId()] = processor;
	    _processors[processor->getName()] = processor;
	}

	Processor* Document::getProcessorByName(const std::string& name) const
	{
	    /* Return the processor by name */
	    std::map<std::string, Processor*>::const_iterator processor = _processors.find(name);
	    if( processor != _processors.end()) {
		return processor->second;
	    } else {
		return nullptr;
	    }
	}

	const std::map<std::string,Processor*>& Document::getProcessors() const
	{
	    /* Return the pointer */
	    return _processors;
	}

	void Document::addTaskEntity(Task* task)
	{
	    /* Map in the task entity */
	    _entities[task->getId()] = task;
	    _tasks[task->getName()] = task;
	}

	Task* Document::getTaskByName(const std::string& name) const
	{
	    /* Return the task by name */
	    std::map<std::string,Task*>::const_iterator task = _tasks.find(name);
	    if (task != _tasks.end()) {
		return task->second;
	    } else {
		return nullptr;
	    }
	}

	const std::map<std::string,Task*>& Document::getTasks() const
	{
	    /* Return the pointer */
	    return _tasks;
	}

	void Document::addEntry(Entry* entry)
	{
	    /* Store the entry in the table */
	    _entries[entry->getName()] = entry;
	}

	Entry* Document::getEntryByName(const std::string& name) const
	{
	    std::map<std::string, Entry*>::const_iterator entry = _entries.find(name);
	    /* Return the named entry */
	    if ( entry == _entries.end()) {
		return nullptr;
	    } else {
		return entry->second;
	    }
	}

	const std::map<std::string,Entry*>& Document::getEntries() const
	{
	    /* Return the pointer */
	    return _entries;
	}

	void Document::addGroup(Group* group)
	{
	    /* Store the group in the map regarless of existence */
	    _groups[group->getName()] = group;
	}

	Group* Document::getGroupByName(const std::string& name) const
	{
	    std::map<std::string,Group*>::const_iterator group = _groups.find(name);
	    /* Attempt to find the group with the given name */
	    if ( group != _groups.end()) {
		return group->second;
	    } else {
		return nullptr;
	    }
	}

	const std::map<std::string,Group*>& Document::getGroups() const
	{
	    /* Return the pointer */
	    return _groups;
	}

	const std::map<unsigned,Entity*>& Document::getEntities() const
	{
	    /* Return the pointer */
	    return _entities;
	}

	void Document::clearAllMaps()
	{
	    _processors.clear();
	    _tasks.clear();
	    _entries.clear();
	    _groups.clear();
	    _entities.clear();
	}

	SymbolExternalVariable* Document::getSymbolExternalVariable(const std::string& name)
	{
	    /* Link the variable into the list */
	    if (_variables.find(name) != _variables.end()) {
		return _variables[name];
	    } else {
		SymbolExternalVariable* variable = new SymbolExternalVariable(name);
		_variables[name] = variable;
		LQIO::Spex::__global_variables.insert(name);		/* For SPEX */
		return variable;
	    }
	}

	bool Document::hasSymbolExternalVariable(const std::string& name) const
	{
	    return _variables.find(name) != _variables.end();
	}

	unsigned Document::getSymbolExternalVariableCount() const
	{
	    /* Return the number of variables */
	    return _variables.size();
	}

	bool Document::areAllExternalVariablesAssigned() const
	{
	    /* Check to make sure all external variables were set */
	    return std::all_of( _variables.begin(), _variables.end(), &wasSet );
	}

	std::vector<std::string> Document::getUndefinedExternalVariables() const
	{
	    std::vector<std::string> names;
	    std::for_each( _variables.begin(), _variables.end(), notSet(names) );
	    return names;
	}


	void Document::setLQXProgramText(const std::string& program)
	{
	    /* Copies the LQX program body */
	    _lqxProgram = program;
	}

	void Document::setLQXProgramText(const char * program)
	{
	    /* Copies the LQX program body */
	    _lqxProgram = program;
	}

	const std::string& Document::getLQXProgramText() const
	{
	    /* Return a reference to the program text */
	    return _lqxProgram;
	}

	LQX::Program * Document::getLQXProgram() const
	{
	    return _parsedLQXProgram;
	}

	void Document::registerExternalSymbolsWithProgram(LQX::Program* program)
	{
	    if ( _instantiated ) return;			// Done already.
	    _instantiated = true;
	    if ( _variables.size() == 0 && !program ) return;	// NOP.

	    /* Make sure all of the variables are registered in the program */
	    std::map<const std::string, SymbolExternalVariable*>::iterator sym_iter;
	    for (sym_iter = _variables.begin(); sym_iter != _variables.end(); ++sym_iter) {
		sym_iter->second->registerInEnvironment(program);
	    }

	    /* Instantiate and initialize all control variables.  Program must exist before we can initialize */
	    SymbolExternalVariable * current = 0;
	    std::map<const char *,double>::const_iterator init_iter;
	    for (init_iter = __initialValues.begin(); init_iter != __initialValues.end(); ++init_iter) {
		const char * name = init_iter->first;		// Create var if necessary

		/* Get value if set */
		double value = init_iter->second;
		const ConstantExternalVariable* constant = dynamic_cast<const ConstantExternalVariable *>(_controlVariables[name]);
		if ( constant ) {
		    constant->getValue( value );
		    delete constant;
		}
		/* Now make it a variable */
		current = new SymbolExternalVariable(name);
		current->registerInEnvironment(program);	// This assigns _externalSymbol
		current->set( value );				// Now set it to the default value
		_controlVariables[name] = current;
	    }

	    const ConstantExternalVariable* constant = dynamic_cast<const ConstantExternalVariable *>(_controlVariables[XComment]);
	    const char * s = nullptr;
	    if ( constant ) {
		constant->getString( s );			// get set value.
		current = new SymbolExternalVariable(XComment);
		current->registerInEnvironment(program);		// This assigns _externalSymbol
		current->setString( s );				// Set value.
		_controlVariables[XComment] = current;
		delete constant;
	    }
	}

	void Document::addPragma(const std::string& param, const std::string& value )
	{
	    _pragmas.insert( param, value );
	}

	void Document::mergePragmas(const std::map<std::string,std::string>& list )
	{
	    _pragmas.merge( list );
	}

	const std::map<std::string,std::string>& Document::getPragmaList() const
	{
	    return _pragmas.getList();
	}

	void Document::clearPragmaList()
	{
	    _pragmas.clear();
	}

	const std::string& Document::getPragma( const std::string& param ) const
	{
	    return _pragmas.get( param );
	}

	bool Document::hasPragma( const std::string& param ) const
	{
	    return _pragmas.have( param );
	}

	/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- [Result Values] -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

	Document&
	Document::setMVAStatistics( const unsigned int submodels, const unsigned long core, const double step, const double step_squared, const double wait, const double wait_squared, const unsigned int faults )
	{
	    _mvaStatistics.set( submodels, core, step, step_squared, wait, wait_squared, faults );
	    return *this;
	}

	Document& Document::setResultValid(bool resultValid)
	{
	    _hasResults = true;
	    _resultValid = resultValid;
	    return *this;
	}

	Document& Document::setResultConvergenceValue(double resultConvergenceValue)
	{
	    _hasResults = true;
	    _resultConvergenceValue = resultConvergenceValue;
	    return *this;
	}

	Document& Document::setResultIterations(unsigned int resultIterations)
	{
	    _hasResults = true;
	    _resultIterations = resultIterations;
	    return *this;
	}

	Document& Document::setResultPlatformInformation(const std::string& resultPlatformInformation)
	{
	    _hasResults = true;
	    if ( !resultPlatformInformation.empty() ) {
		_resultPlatformInformation = resultPlatformInformation;
	    } else {
#if HAVE_UNAME
		struct utsname uu;		/* Get system triva. */
		uname( &uu );
		_resultPlatformInformation = std::string(uu.nodename) + " " + uu.sysname + " " + uu.release;
#endif
	    }
	    return *this;
	}

	Document& Document::setResultSolverInformation(const std::string& resultSolverInformation)
	{
	    _hasResults = true;
	    if ( !resultSolverInformation.empty() ) {
		_resultSolverInformation = resultSolverInformation;
	    } else {
		_resultSolverInformation = LQIO::io_vars.lq_toolname + " " + LQIO::io_vars.lq_version;
	    }
	    return *this;
	}

	Document& Document::setResultUserTime(double resultUserTime)
	{
	    _hasResults = true;
	    _resultUserTime = resultUserTime;
	    return *this;
	}

	Document& Document::setResultSysTime(double resultSysTime)
	{
	    _hasResults = true;
	    _resultSysTime = resultSysTime;
	    return *this;
	}

	Document& Document::setResultElapsedTime(double resultElapsedTime)
	{
	    _hasResults = true;
	    _resultElapsedTime = resultElapsedTime;
	    return *this;
	}

	Document& Document::setResultMaxRSS( long resultMaxRSS )
	{
	    if ( resultMaxRSS > 0 ) {		/* Only set if > 0 */
		_hasResults = true;
		_resultMaxRSS = resultMaxRSS;
	    }
	    return *this;
	}


	bool Document::hasResults() const
	{
	    return _hasResults;
	}

	bool Document::hasRendezvous() const
	{
	    /* This is a property of phases and activities, so count_if can't be used here */
	    if ( _hasRendezvous == cached::NOT_SET ) {
		_hasRendezvous = std::any_of( _tasks.begin(), _tasks.end(), LQIO::DOM::Task::any_of( &LQIO::DOM::Phase::hasRendezvous ) ) ? cached::SET_TRUE : cached::SET_FALSE;
	    }
	    return _hasRendezvous == cached::SET_TRUE;
	}

	bool Document::hasSendNoReply() const
	{
	    if ( _hasSendNoReply == cached::NOT_SET ) {
		_hasSendNoReply = std::any_of( _tasks.begin(), _tasks.end(), LQIO::DOM::Task::any_of( &LQIO::DOM::Phase::hasSendNoReply ) ) ? cached::SET_TRUE : cached::SET_FALSE;
	    }
	    return _hasSendNoReply == cached::SET_TRUE;
	}

	bool Document::hasForwarding() const
	{
	    return std::any_of( _entries.begin(), _entries.end(), LQIO::DOM::DocumentObject::Predicate<LQIO::DOM::Entry>( &LQIO::DOM::Entry::hasForwarding ) );
	}

	bool Document::hasNonExponentialPhase() const
	{
	    return std::any_of( _tasks.begin(), _tasks.end(), LQIO::DOM::Task::any_of( &LQIO::DOM::Phase::isNonExponential ) );
	}

	bool Document::hasDeterministicPhase() const
	{
	    return std::any_of( _tasks.begin(), _tasks.end(), LQIO::DOM::Task::any_of( &LQIO::DOM::Phase::hasDeterministicCalls ) );
	}

	bool Document::hasMaxServiceTimeExceeded() const
	{
	    return std::any_of( _tasks.begin(), _tasks.end(), LQIO::DOM::Task::any_of( &LQIO::DOM::Phase::hasMaxServiceTimeExceeded ) );
	}

	bool Document::hasHistogram() const
	{
	    return std::any_of( _tasks.begin(), _tasks.end(), LQIO::DOM::Task::any_of( &LQIO::DOM::Phase::hasHistogram ) );
	}

	bool Document::entryHasWaitingTimeVariance() const
	{
	    return std::any_of( _tasks.begin(), _tasks.end(), LQIO::DOM::Task::any_of( &LQIO::DOM::Phase::hasResultVarianceWaitingTime ) );
	}

	bool Document::entryHasDropProbability() const
	{
	    return std::any_of( _tasks.begin(), _tasks.end(), LQIO::DOM::Task::any_of( &LQIO::DOM::Phase::hasResultDropProbability ) );
	}

	bool Document::entryHasServiceTimeVariance() const
	{
	    return std::any_of( _tasks.begin(), _tasks.end(), LQIO::DOM::Task::any_of( &LQIO::DOM::Phase::hasResultServiceTimeVariance ) );
	}

	bool Document::processorHasRate() const
	{
	    return std::any_of( _processors.begin(), _processors.end(), LQIO::DOM::DocumentObject::Predicate<LQIO::DOM::Processor>( &LQIO::DOM::Processor::hasRate ) );
	}

	bool Document::taskHasAndJoin() const
	{
	    if ( _taskHasAndJoin == cached::NOT_SET ) {
		_taskHasAndJoin = std::any_of( _tasks.begin(), _tasks.end(), LQIO::DOM::DocumentObject::Predicate<LQIO::DOM::Task>( &LQIO::DOM::Task::hasAndJoinActivityList ) ) ? cached::SET_TRUE : cached::SET_FALSE;
	    }
	    return _taskHasAndJoin == cached::SET_TRUE;
	}

	bool Document::taskHasThinkTime() const
	{
	    /* This is a property of tasks only */
	    return std::any_of( _tasks.begin(), _tasks.end(), LQIO::DOM::DocumentObject::Predicate<LQIO::DOM::Task>( &LQIO::DOM::Task::hasThinkTime ) );
	}

	bool Document::hasSemaphoreWait() const
	{
	    return std::any_of( _tasks.begin(), _tasks.end(), &LQIO::DOM::Task::isSemaphoreTask );
	}

	bool Document::hasRWLockWait() const
	{
	    return std::any_of( _tasks.begin(), _tasks.end(), &LQIO::DOM::Task::isRWLockTask );
	}

	bool Document::hasThinkTime() const
	{
	    return std::any_of( _tasks.begin(), _tasks.end(), LQIO::DOM::Task::any_of( &LQIO::DOM::Phase::hasThinkTime ) );
	}

	bool Document::hasOpenArrivals() const
	{
	    return std::any_of( _entries.begin(), _entries.end(), LQIO::DOM::DocumentObject::Predicate<LQIO::DOM::Entry>( &LQIO::DOM::Entry::hasOpenArrivalRate ) );
	}

	bool Document::entryHasThroughputBound() const
	{
	    return std::any_of( _entries.begin(), _entries.end(), LQIO::DOM::DocumentObject::Predicate<LQIO::DOM::Entry>( &LQIO::DOM::Entry::hasResultsForThroughputBound ) );
	}

	bool Document::entryHasOpenWait() const
	{
	    return std::any_of( _entries.begin(), _entries.end(), LQIO::DOM::DocumentObject::Predicate<LQIO::DOM::Entry>( &LQIO::DOM::Entry::hasResultsForOpenWait ) );
	}

	/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- [Dom builder ] -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

	ExternalVariable*
	Document::db_build_parameter_variable(const std::string& input, bool* isSymbol)
	{
	    if (isSymbol) { *isSymbol = false; }
	    if ( input.empty() ) {
		/* nullptr input means a zero */
		return new ConstantExternalVariable(0.0);
	    } else if ( input[0] == '$' ) {
		if (isSymbol) { *isSymbol = true; }
		return getSymbolExternalVariable(input);
	    } else if ( input == "@infinity" ) {
		return new ConstantExternalVariable( std::numeric_limits<double>::infinity() );
	    } else {
		double result = 0.0;
		char* endPtr = nullptr;
		const char* realEndPtr = input.c_str() + input.size();
		result = strtod(input.c_str(), &endPtr);

		/* Check if we finished parsing okay */
		if (endPtr != realEndPtr) {
		    throw std::invalid_argument( "<double>: \"" + std::string(input) + "\"" );
		}

		/* Return the resulting value */
		return new ConstantExternalVariable(result);
	    }
	}

	void
	Document::db_check_set_entry(Entry* entry, Entry::Type requisiteType)
	{
	    if ( requisiteType != Entry::Type::NOT_DEFINED && !entry->entryTypeOk( requisiteType ) ) {
		entry->input_error( ERR_MIXED_ENTRY_TYPES );
	    }
	}

	void
	Document::lqx_parser_trace( FILE * file )
	{
	    static char prompt[] = "lqx:";		/* Work around C++11 warnings */
	    ModLangParserTrace( file, prompt );
	}

	/*
	 * Load document.  Based on the file extension, pick the
	 * appropriate loader.  LQNX (XML) input and output are found
	 * in one file.  SRVN input and ouptut are separate.  We don't
	 * try to load the latter if load_results == false
	 */

	/* static */ Document*
	Document::load(const std::string& input_filename, InputFormat format, unsigned& errorCode, bool load_results )
	{
	    __input_file_name = input_filename;
            io_vars.reset();                   /* See error.c */
	    
	    /* Figure out input file type based on suffix */

	    if ( format == InputFormat::AUTOMATIC ) {
		format = getInputFormatFromFilename( input_filename );
	    }

	    /* Create a document to store the product */

	    bool rc = true;
	    Document * document = new Document( format );

	    /* Read in the model, invoke the builder, and see what happened */

	    switch ( format ) {

	    case InputFormat::AUTOMATIC:
	    case InputFormat::LQN:
		rc = SRVN::load( *document, input_filename, load_results );
		break;

#if HAVE_LIBEXPAT
	    case InputFormat::XML:
		rc = Expat_Document::load( *document, input_filename, load_results );
		break;

	    case InputFormat::JMVA:
		rc = QNIO::JMVA_Document::load( *document, input_filename );
		break;
#else
	    case InputFormat::XML:
	    case InputFormat::JMVA:
		rc = false;
		break;
#endif

	    case InputFormat::JSON:
		rc = JSON_Document::load( *document, input_filename, errorCode, load_results );
		break;

	    default:
		rc = false;
		break;
	    }

	    /* All went well, so return it */

	    if ( rc ) {
		return document;

	    } else {
		delete document;
		return nullptr;
	    }
	}


	/* 
	 * Just load the results.
	 */
	
	bool
	Document::loadResults(const std::string& directory_name, const std::string& file_name, const std::string& extension, OutputFormat output_format, unsigned& errorCode )
	{
	    if ( output_format == OutputFormat::DEFAULT && (getInputFormat() != InputFormat::LQN || getLQXProgram() != nullptr) ) {
		output_format = __input_to_output_format.at( getInputFormat() );
	    } 

	    switch ( output_format ) {
	    case OutputFormat::DEFAULT:
	    case OutputFormat::LQN:
		return LQIO::SRVN::loadResults( LQIO::Filename( file_name, "p", directory_name, extension )() );

	    case OutputFormat::XML:
#if HAVE_LIBEXPAT
		return Expat_Document::loadResults( *this, LQIO::Filename( file_name, "lqxo", directory_name, extension )() );
#else
		return false;
#endif
	    case OutputFormat::JSON:
		return JSON_Document::loadResults( *this, LQIO::Filename( file_name, "lqxo", directory_name, extension )() );
		return false;

	    default:
		return false;
	    }
	}

	/*
	 * Figure out the input file from the extension.  If in doubt, punt to XML.
	 */
	 
	/* static */ Document::InputFormat
	Document::getInputFormatFromFilename( const std::string& filename, const InputFormat default_format )
	{
	    const unsigned long pos = filename.find_last_of( '.' );
	    if ( pos == std::string::npos ) {
		return default_format;
	    }
	    
	    std::string suffix = filename.substr( pos+1 );
	    std::transform(suffix.begin(), suffix.end(), suffix.begin(), ::tolower);
	    const std::map<const std::string,const LQIO::DOM::Document::InputFormat>::const_iterator ext = __extensions_input.find( suffix );
	    if ( ext != __extensions_input.end() ) {
		return ext->second;
	    } else {
		return InputFormat::XML;
	    }
	}

	/*
	 * All the gobbly gook to print out the model.
	 */

	void
	Document::print( const std::string& output_file_name, const std::string& suffix, OutputFormat output_format, bool rtf_output ) const
	{
	    const bool lqx_output = getResultInvocationNumber() > 0;
	    const std::string directory_name = LQIO::Filename::createDirectory( Filename::Filename::isFileName( output_file_name ) ? output_file_name : __input_file_name, lqx_output );

	    /* Set output format from input, or if LQN and LQX then force to XML. */
    
	    if ( output_format == OutputFormat::DEFAULT && (getInputFormat() != InputFormat::LQN || getLQXProgram() != nullptr) ) {
		output_format = __input_to_output_format.at( getInputFormat() );
	    }

	    /* override is true for '-p -o filename.out when filename.in' == '-p filename.in' */

	    bool override = false;
	    if ( Filename::isFileName( output_file_name ) && LQIO::Filename::isRegularFile( output_file_name ) > 0 ) {
		LQIO::Filename filename( __input_file_name, rtf_output ? "rtf" : "out" );
		override = filename() == output_file_name;
	    }

	    if ( override || ((!Filename::isFileName( output_file_name ) || !directory_name.empty()) && Filename::isFileName( __input_file_name ) )) {

		/* Case for "-o <dir>" or "[-o xxx.out] -{p,j,x}" when xxx.out == xxx.in if -o xxx.out is present */

		std::ofstream output;

		/* Parseable output. {.p, .lqxo, .lqjo} */

		const std::map<const LQIO::DOM::Document::OutputFormat,const std::string>::const_iterator extension =  __output_extensions.find( output_format );
		if ( extension != __output_extensions.end() ) {
		    LQIO::Filename filename( __input_file_name, extension->second, directory_name, suffix );
		    filename.backup();
		    output.open( filename(), std::ios::out );
		    if ( !output ) {
			runtime_error( LQIO::ERR_CANT_OPEN_FILE, filename().c_str(), strerror( errno ) );
		    } else {
			print( output, extension->first );
			output.close();
		    }
		}

		/* Regular output */

		if ( !LQIO::DOM::__document->hasPragma( LQIO::DOM::Pragma::_default_output_ ) || LQIO::DOM::Pragma::isTrue(LQIO::DOM::__document->getPragma( LQIO::DOM::Pragma::_default_output_ )) ) {
		    LQIO::Filename filename( __input_file_name, rtf_output ? "rtf" : "out", directory_name, suffix );

		    output.open( filename(), std::ios::out );
		    if ( !output ) {
			runtime_error( LQIO::ERR_CANT_OPEN_FILE, filename().c_str(), strerror( errno ) );
		    } else if ( rtf_output ) {
			print( output, LQIO::DOM::Document::OutputFormat::RTF );
		    } else {
			print( output );
		    }
		    output.close();
		}
		
	    } else if ( Filename::isFileName( output_file_name ) ) {

		/* case for -o xxx, do not map filename. */

		LQIO::Filename::backup( output_file_name );
		std::ofstream output;

		output.open( output_file_name, std::ios::out );
		if ( !output ) {
		    runtime_error( LQIO::ERR_CANT_OPEN_FILE, output_file_name.c_str(), strerror( errno ) );
		} else if ( __output_extensions.find( output_format ) != __output_extensions.end() ) {
		    print( output, output_format );
		} else if ( rtf_output ) {
		    print( output, LQIO::DOM::Document::OutputFormat::RTF );
		} else {
		    print( output );
		}
		output.close();

	    } else {

		/* case for pipeines: ... | lqns ... */

		const std::map<const LQIO::DOM::Document::OutputFormat,const std::string>::const_iterator extension =  __output_extensions.find( output_format );
		if ( extension != __output_extensions.end() ) {
		    print( std::cout, extension->first );
		} else if ( rtf_output ) {
		    print( std::cout, LQIO::DOM::Document::OutputFormat::RTF );
		} else {
		    print( std::cout );
		}

	    }
	}



	/*
	 * Print intermediate output (just generate {.p,lqxo,lqjo} or {.out,.rtf} files, not both).
	 */

	void
	Document::print( const std::string& output_file_name, const std::string& suffix, OutputFormat output_format, bool rtf_output, unsigned int iteration ) const
	{
	    const bool lqx_output = getResultInvocationNumber() > 0;
	    const std::string directory_name = LQIO::Filename::createDirectory( Filename::isFileName( output_file_name ) ? output_file_name : __input_file_name, lqx_output );
	    const std::map<const LQIO::DOM::Document::OutputFormat,const std::string>::const_iterator format_iterator = __output_extensions.find( output_format );

	    LQIO::Filename filename;
	    if ( Filename::isFileName( output_file_name ) && directory_name.empty() ) {
		/* If the output file name exists and it is not a directory, just use that. */
		filename = output_file_name;

	    } else if ( Filename::isFileName( __input_file_name ) ) {
		/* Otherwise, construct one from the input file name */
		std::string extension;
		if ( format_iterator != __output_extensions.end() ) {
		    extension = format_iterator->second;
		} else if ( rtf_output ) {
		    extension = "rtf";
		} else {
		    extension = "out";
		}
		filename.generate( __input_file_name, extension, directory_name, lqx_output ? suffix : std::string("") );

	    } else {
		/* Don't output to stdout. */
		return;
	    }

	    /* Make filename look like an emacs autosave file. */
	    filename << "~" << iteration << "~";

	    std::ofstream output;
	    output.open( filename(), std::ios::out );

	    if ( !output ) return;			/* Ignore errors */

	    if ( format_iterator != LQIO::DOM::Document::__output_extensions.end() ) {
		print( output, format_iterator->first );
	    } else if ( rtf_output ) {
		print( output, LQIO::DOM::Document::OutputFormat::RTF );
	    } else {
		print( output );
	    }
	    output.close();
	}



	/*
	 * Print in input order.
	 */

        std::ostream&
	Document::print( std::ostream& output, const OutputFormat format ) const
	{
	    switch ( format ) {
	    case OutputFormat::DEFAULT:
	    case OutputFormat::LQN: {
		SRVN::Output srvn( *this, _entities );
		srvn.print( output );
		break;
	    }
	    case OutputFormat::PARSEABLE:{
		SRVN::Parseable srvn( *this, _entities );
		srvn.print( output );
		break;
	    }
	    case OutputFormat::RTF: {
		SRVN::RTF srvn( *this, _entities );
		srvn.print( output );
		break;
	    }
	    case OutputFormat::XML: {
#if HAVE_LIBEXPAT
		Expat_Document expat( *const_cast<Document *>(this), __input_file_name, false, false );
		expat.serializeDOM( output );
#endif
		break;
	    }
	    case OutputFormat::JSON: {
		JSON_Document json( *const_cast<Document *>(this), __input_file_name, false, false );
		json.serializeDOM( output );
		break;
	    }
	    default:
		abort();
	    }

	    return output;
	}


	std::ostream& Document::printExternalVariables( std::ostream& output ) const
	{
	    /* Check to make sure all external variables were set */
	    for (std::map<const std::string, SymbolExternalVariable*>::const_iterator var_p = _variables.begin(); var_p != _variables.end(); ++var_p) {
		if ( var_p != _variables.begin() ) {
		    output << ", ";
		}

		const SymbolExternalVariable* var = var_p->second;
		output << var->getName();

		if ( var->wasSet() ) {
		    output << " = " << *var;
		}
	    }
	    return output;
	}
    }
    
    lqio_params_stats::lqio_params_stats( const char * version, void (*action)(error_severity) ) :
	lq_toolname(),
	lq_version(version),
	lq_command_line(),
	severity_action(action),
	max_error(10),
	error_count(0),
	severity_level(LQIO::error_severity::ALL)
    {
    }

    void lqio_params_stats::init( const std::string& version, const std::string& toolname, void (*sa)(error_severity) )
    {
	lq_version = version;
	lq_toolname = toolname;
	severity_action = sa;
    }
}
