/*
 *  $Id: dom_document.cpp 13727 2020-08-04 14:06:18Z greg $
 *
 *  Created by Martin Mroz on 24/02/09.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#if HAVE_CONFIG_H
#include <config.h>
#endif
#include "dom_document.h"
#include <iostream>
#include <cstring>
#include <cstdlib>
#include <cmath>
#include <algorithm>
#include "glblerr.h"
#if HAVE_LIBEXPAT
#include "expat_document.h"
#endif
#include "srvn_input.h"
#include "srvn_results.h"
#include "srvn_output.h"
#include "srvn_spex.h"
#include "filename.h"
#include "dom_processor.h"

namespace LQIO {
    lqio_params_stats io_vars(VERSION,nullptr);
    
    namespace DOM {
	Document* __document = NULL;
	bool Document::__debugXML = false;
	std::map<const char *, double> Document::__initialValues;
	std::string Document::__input_file_name = "";
	const std::string Document::__comment( "" );
	const char * Document::XComment = "comment";
	const char * Document::XConvergence = "conv_val";			/* Matches schema. 	*/
	const char * Document::XIterationLimit = "it_limit";			/* Matched schema.	*/
	const char * Document::XPrintInterval = "print_int";			/* Matches schema.	*/
	const char * Document::XUnderrelaxationCoefficient = "underrelax_coeff";/* Matches schema.	*/
	const char * Document::XSimulationSeedValue = "seed_value";
	const char * Document::XSimulationNumberOfBlocks = "number_of_blocks";
	const char * Document::XSimulationBlockTime = "block_time";
	const char * Document::XSimulationPrecision = "precision";
	const char * Document::XSimulationWarmUpLoops = "warm_up_loops";
	const char * Document::XSimulationWarmUpTime = "warm_up_time";
	const char * Document::XSpexIterationLimit = "spex_it_limit";
	const char * Document::XSpexUnderrelaxation = "spex_underrelax_coeff";
    
	Document::Document( input_format format ) 
	    : _processors(), _groups(), _tasks(), _entries(), 
	      _entities(), _variables(), _controlVariables(), _nextEntityId(0), 
	      _format(format), _comment2(), 
	      _lqxProgram(""), _lqxProgramLineNumber(0), _parsedLQXProgram(0), _instantiated(false), _pragmas(),
	      _maximumPhase(0), _hasResults(false),
	      _hasRendezvous(NOT_SET), _hasSendNoReply(NOT_SET), _taskHasAndJoin(NOT_SET),		/* Cached valuess */
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
	    __initialValues[XSimulationSeedValue] =         0;
	    __initialValues[XSimulationNumberOfBlocks] =    30.;
	    __initialValues[XSimulationBlockTime] =         50000.;
	    __initialValues[XSimulationWarmUpLoops] =       500.;
	    __initialValues[XSimulationWarmUpTime] =        10000.;
	    __initialValues[XSpexIterationLimit] =          50.;
	    __initialValues[XSpexUnderrelaxation] =         0.9;
	}
    

	Document::~Document() 
	{
	    /* Delete all of the processors (deletes tasks) */
	    std::map<std::string, Processor*>::iterator procIter;
	    for (procIter = _processors.begin(); procIter != _processors.end(); ++procIter) {
		delete(procIter->second);
	    }
      
	    /* Make sure that we only delete entries once */
	    std::map<std::string, Entry*>::iterator entryIter;
	    for (entryIter = _entries.begin(); entryIter != _entries.end(); ++entryIter) {
		delete(entryIter->second);
	    }
      
	    /* Now, delete all of the groups */
	    std::map<std::string, Group*>::iterator groupIter;
	    for (groupIter = _groups.begin(); groupIter != _groups.end(); ++groupIter) {
		delete(groupIter->second);
	    }

	    __document = NULL;
	    __input_file_name = "";

	    LQIO::Spex::clear();
	}
    
	void Document::setModelParameters(const std::string& comment, LQIO::DOM::ExternalVariable* conv_val, LQIO::DOM::ExternalVariable* it_limit, LQIO::DOM::ExternalVariable* print_int, LQIO::DOM::ExternalVariable* underrelax_coeff, const void * element )
	{
	    /* Set up initial model parameters, but only if they were not set using SPEX variables */

	    ExternalVariable * var;		// Querk of map<>[].
	    var = _controlVariables[XComment];
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

	const char * Document::getModelCommentString() const 
	{
	    const char * s;
	    const std::map<const char *, ExternalVariable *>::const_iterator iter = _controlVariables.find(XComment);
	    if ( iter != _controlVariables.end() && iter->second->wasSet() ) {
		if ( iter->second->getString( s ) ) return s;
	    } 
	    return __comment.c_str();
	}

	Document& Document::setModelComment( ExternalVariable * comment )
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
	    return _comment2;
	}

	Document& Document::setExtraComment( const std::string& value )
	{	
	    _comment2 = value;
	    return *this;
	}

	Document& Document::setModelConvergence( ExternalVariable * value )
	{	
	    _controlVariables[XConvergence] = value;
	    return *this;
	}

	Document& Document::setModelIterationLimit( ExternalVariable * value ) 
	{
	    _controlVariables[XIterationLimit] = value;
	    return *this;
	}

	Document& Document::setModelPrintInterval( ExternalVariable * value ) 
	{
	    _controlVariables[XPrintInterval] = value;
	    return *this;
	}

	Document& Document::setModelUnderrelaxationCoefficient( ExternalVariable * value ) 
	{
	    _controlVariables[XUnderrelaxationCoefficient] = value;
	    return *this;
	}

	Document& Document::setSimulationSeedValue( ExternalVariable * value )
	{
	    _controlVariables[XSimulationSeedValue] = value;
	    return *this;
	}

	Document& Document::setSimulationNumberOfBlocks( ExternalVariable * value )
	{
	    _controlVariables[XSimulationNumberOfBlocks] = value;
	    return *this;
	}

	Document& Document::setSimulationBlockTime( ExternalVariable * value )
	{
	    _controlVariables[XSimulationBlockTime] = value;
	    return *this;
	}

	Document& Document::setSimulationPrecision( ExternalVariable * value )
	{
	    _controlVariables[XSimulationPrecision] = value;
	    return *this;
	}

	Document& Document::setSimulationWarmUpLoops( ExternalVariable * value )
	{
	    _controlVariables[XSimulationWarmUpLoops] = value;
	    return *this;
	}

	Document& Document::setSimulationWarmUpTime( ExternalVariable * value )
	{
	    _controlVariables[XSimulationWarmUpTime] = value;
	    return *this;
	}

	Document& Document::setSpexConvergenceIterationLimit( ExternalVariable * spexIterationLimit )
	{
	    _controlVariables[XSpexIterationLimit] = spexIterationLimit;
	    return *this;
	}

	Document& Document::setSpexConvergenceUnderrelaxation( ExternalVariable * spexUnderrelaxation )
	{
	    _controlVariables[XSpexUnderrelaxation] = spexUnderrelaxation;
	    return *this;
	}

	const double Document::getValue( const char * index ) const
	{
	    /* Set to default value if NOT set elsewhere (usually the control program) */
	    double value = __initialValues[index];
	    const std::map<const char *, ExternalVariable *>::const_iterator iter = _controlVariables.find(index);
	    if ( iter != _controlVariables.end() ) {
		const ExternalVariable * var = iter->second;
		if ( var != NULL && var->wasSet() ) {
		    var->getValue(value);
		}
	    }
	    return value;
	}

	const ExternalVariable * Document::get( const char * index ) const
	{
	    const std::map<const char *, ExternalVariable *>::const_iterator iter = _controlVariables.find(index);
	    if ( iter != _controlVariables.end() ) {
		const ExternalVariable * var = iter->second;
		if ( var ) {
		    return iter->second;
		}
	    }
	    return new ConstantExternalVariable( __initialValues[index] );
	}

	void 
	Document::setMVAStatistics( const unsigned int submodels, const unsigned long core, const double step, const double step_squared, const double wait, const double wait_squared, const unsigned int faults )
	{
	    _mvaStatistics.set( submodels, core, step, step_squared, wait, wait_squared, faults );
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
		return NULL;
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
		return NULL;
	    }
	}
    
	const std::string* Document::getTaskNames(unsigned& count) const
	{
	    /* Copy all of the keys */
	    count = _tasks.size();
	    std::string* names = new std::string[_tasks.size()+1];      
	    std::map<std::string, Task*>::const_iterator iter;
	    int i = 0;
      
	    /* Copy in each of the processors names to the list */
	    for (iter = _tasks.begin(); iter != _tasks.end(); ++iter) {
		names[i++] = iter->first;
	    }
      
	    return names;
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
		return NULL;
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
		return NULL;
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
	    for (std::map<std::string, SymbolExternalVariable*>::const_iterator iter = _variables.begin(); iter != _variables.end(); ++iter) {
		SymbolExternalVariable* current = iter->second;
		if (current->wasSet() == false) {
		    return false;
		}
	    }
      
	    return true;
	}
    
	std::vector<std::string> Document::getUndefinedExternalVariables() const
	{
	    /* Returns a list of all undefined external variables as a string */
	    std::vector<std::string> names;
	    for (std::map<std::string, SymbolExternalVariable*>::const_iterator iter = _variables.begin(); iter != _variables.end(); ++iter) {
		SymbolExternalVariable* current = iter->second;
		if (current->wasSet() == false) {
		    names.push_back(iter->first);
		}
	    }
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
	    std::map<std::string, SymbolExternalVariable*>::iterator sym_iter;
	    for (sym_iter = _variables.begin(); sym_iter != _variables.end(); ++sym_iter) {
		SymbolExternalVariable* current = sym_iter->second;
		current->registerInEnvironment(program);
	    }

	    /* Instantiate and initialize all control variables.  Program must exist before we can initialize */
	    SymbolExternalVariable * current = 0;
	    std::map<const char *,double>::const_iterator init_iter;
	    for (init_iter = __initialValues.begin(); init_iter != __initialValues.end(); ++init_iter) {
		const char * name = init_iter->first;		// Create var if necessary

		/* Get value if set */
		double value = init_iter->second;
		ConstantExternalVariable* constant = dynamic_cast<ConstantExternalVariable *>(_controlVariables[name]);
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

	    ConstantExternalVariable* constant = dynamic_cast<ConstantExternalVariable *>(_controlVariables[XComment]);
	    const char * s = __comment.c_str();
	    if ( constant ) {
		constant->getString( s );			// get set value.
	    }
	    current = new SymbolExternalVariable(XComment);
	    current->registerInEnvironment(program);		// This assigns _externalSymbol
	    current->setString( s );				// Set value.
	    _controlVariables[XComment] = current;
	    if ( constant ) delete constant;
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

	const std::string Document::getPragma( const std::string& param ) const
	{
	    return _pragmas.get( param );
	}

	/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- [Result Values] -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

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
	    _resultPlatformInformation = resultPlatformInformation;
	    return *this;
	}

	Document& Document::setResultSolverInformation(const std::string& resultSolverInformation) 
	{ 
	    _hasResults = true;
	    _resultSolverInformation = resultSolverInformation;
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
	    if ( _hasRendezvous == NOT_SET ) {
		_hasRendezvous = (for_each( _tasks.begin(), _tasks.end(), LQIO::DOM::Task::Count( &LQIO::DOM::Phase::hasRendezvous ) ).count() != 0 ? SET_TRUE : SET_FALSE);
	    }
	    return _hasRendezvous == SET_TRUE;
	}

	bool Document::hasSendNoReply() const
	{
	    if ( _hasSendNoReply == NOT_SET ) {
		_hasSendNoReply = (for_each( _tasks.begin(), _tasks.end(), LQIO::DOM::Task::Count( &LQIO::DOM::Phase::hasSendNoReply ) ).count() != 0 ? SET_TRUE : SET_FALSE);
	    }
	    return _hasSendNoReply == SET_TRUE;
	}

	bool Document::hasForwarding() const
	{
	    return std::find_if( _entries.begin(), _entries.end(), LQIO::DOM::DocumentObject::Predicate<LQIO::DOM::Entry>( &LQIO::DOM::Entry::hasForwarding ) ) != _entries.end();
	}

	bool Document::hasNonExponentialPhase() const
	{
	    return for_each( _tasks.begin(), _tasks.end(), LQIO::DOM::Task::Count( &LQIO::DOM::Phase::isNonExponential ) ).count() != 0;
	}

	bool Document::hasDeterministicPhase() const
	{
	    return for_each( _tasks.begin(), _tasks.end(), LQIO::DOM::Task::Count( &LQIO::DOM::Phase::hasDeterministicCalls ) ).count() != 0;
	}

	bool Document::hasMaxServiceTimeExceeded() const
	{
	    return for_each( _tasks.begin(), _tasks.end(), LQIO::DOM::Task::Count( &LQIO::DOM::Phase::hasMaxServiceTimeExceeded ) ).count() != 0;
	}

	bool Document::hasHistogram() const
	{
	    return for_each( _tasks.begin(), _tasks.end(), LQIO::DOM::Task::Count( &LQIO::DOM::Phase::hasHistogram ) ).count() != 0;
	}

	bool Document::entryHasWaitingTimeVariance() const
	{
	    return for_each( _tasks.begin(), _tasks.end(), LQIO::DOM::Task::Count( &LQIO::DOM::Phase::hasResultVarianceWaitingTime ) ).count() != 0;
	}

	bool Document::entryHasDropProbability() const
	{
	    return for_each( _tasks.begin(), _tasks.end(), LQIO::DOM::Task::Count( &LQIO::DOM::Phase::hasResultDropProbability ) ).count() != 0;
	}

	bool Document::entryHasServiceTimeVariance() const
	{
	    return for_each( _tasks.begin(), _tasks.end(), LQIO::DOM::Task::Count( &LQIO::DOM::Phase::hasResultServiceTimeVariance ) ).count() != 0;
	}

	bool Document::processorHasRate() const
	{
	    return std::find_if( _processors.begin(), _processors.end(), LQIO::DOM::DocumentObject::Predicate<LQIO::DOM::Processor>( &LQIO::DOM::Processor::hasRate ) ) != _processors.end();
	}

	bool Document::taskHasAndJoin() const
	{
	    if ( _taskHasAndJoin == NOT_SET ) {
		_taskHasAndJoin = (std::find_if( _tasks.begin(), _tasks.end(), LQIO::DOM::DocumentObject::Predicate<LQIO::DOM::Task>( &LQIO::DOM::Task::hasAndJoinActivityList ) ) != _tasks.end() ? SET_TRUE : SET_FALSE );
	    }
	    return _taskHasAndJoin == SET_TRUE;
	}

	bool Document::taskHasThinkTime() const
	{
	    /* This is a property of tasks only */
	    return std::find_if( _tasks.begin(), _tasks.end(), LQIO::DOM::DocumentObject::Predicate<LQIO::DOM::Task>( &LQIO::DOM::Task::hasThinkTime ) ) != _tasks.end();
	}

	bool Document::hasSemaphoreWait() const
	{
	    return std::find_if( _tasks.begin(), _tasks.end(), &LQIO::DOM::Task::isSemaphoreTask ) != _tasks.end();
	}

	bool Document::hasRWLockWait() const
	{
	    return std::find_if( _tasks.begin(), _tasks.end(), &LQIO::DOM::Task::isRWLockTask ) != _tasks.end();
	}

	bool Document::hasThinkTime() const
	{
	    return for_each( _tasks.begin(), _tasks.end(), LQIO::DOM::Task::Count( &LQIO::DOM::Phase::hasThinkTime ) ).count() != 0;
	}

	bool Document::hasOpenArrivals() const
	{
	    return std::find_if( _entries.begin(), _entries.end(), LQIO::DOM::DocumentObject::Predicate<LQIO::DOM::Entry>( &LQIO::DOM::Entry::hasOpenArrivalRate ) ) != _entries.end();
	}

	bool Document::entryHasThroughputBound() const 
	{
	    return std::find_if( _entries.begin(), _entries.end(), LQIO::DOM::DocumentObject::Predicate<LQIO::DOM::Entry>( &LQIO::DOM::Entry::hasResultsForThroughputBound ) ) != _entries.end();
	}

	bool Document::entryHasOpenWait() const
	{
	    return std::find_if( _entries.begin(), _entries.end(), LQIO::DOM::DocumentObject::Predicate<LQIO::DOM::Entry>( &LQIO::DOM::Entry::hasResultsForOpenWait ) ) != _entries.end();
	}

	/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- [Dom builder ] -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

	ExternalVariable* 
	Document::db_build_parameter_variable(const char* input, bool* isSymbol)
	{
	    if (isSymbol) { *isSymbol = false; }
	    if ( input == NULL ) {
		/* NULL input means a zero */
		return new ConstantExternalVariable(0.0);
	    } else if ( input[0] == '$' ) {
		if (isSymbol) { *isSymbol = true; }
		return getSymbolExternalVariable(input);
	    } else if ( strcmp( input, "@infinity" ) == 0 ) {
		return new ConstantExternalVariable( srvn_get_infinity() );
	    } else {
		double result = 0.0;
		char* endPtr = NULL;
		const char* realEndPtr = input + strlen(input);
		result = strtod(input, &endPtr);

		/* Check if we finished parsing okay */
		if (endPtr != realEndPtr) {
		    std::string err = "<double>: \"";
		    err += input;
		    err += "\"";
		    throw std::invalid_argument( err.c_str() );
		}

		/* Return the resulting value */
		return new ConstantExternalVariable(result);
	    }
	}
	
	void 
	Document::db_check_set_entry(Entry* entry, const std::string& entry_name, Entry::EntryType requisiteType)
	{
	    if ( !entry ) {
		input_error2( ERR_NOT_DEFINED, entry_name.c_str() );
	    } else if ( requisiteType != Entry::ENTRY_NOT_DEFINED && !entry->entryTypeOk( requisiteType ) ) {
		input_error2( ERR_MIXED_ENTRY_TYPES, entry_name.c_str() );
	    }
	}

	/*
	 * Load document.  Based on the file extension, pick the
	 * appropriate loader.  LQNX (XML) input and output are found
	 * in one file.  SRVN input and ouptut are separate.  We don't
	 * try to load the latter if load_results == false
	 */

	/* static */ Document* 
	Document::load(const std::string& input_filename, input_format format, unsigned& errorCode, bool load_results )
	{
	    __input_file_name = input_filename;
            io_vars.reset();                   /* See error.c */

	    errorCode = 0;

	    /* Figure out input file type based on suffix */

	    if ( format == AUTOMATIC_INPUT ) {
		format = getInputFormatFromFilename( input_filename );
	    }

	    /* Create a document to store the product */
	    Document * document = new Document( format );
	
	    /* Read in the model, invoke the builder, and see what happened */

	    bool rc = true;
	    LQIO::Spex::initialize_control_parameters();
	    if ( format == LQN_INPUT ) {
		rc = SRVN::load( *document, input_filename, errorCode, load_results );
	    } else {
#if HAVE_LIBEXPAT
		rc = Expat_Document::load( *document, input_filename, errorCode, load_results );
#else
		rc = false;
#endif
	    }
	    if ( errorCode != 0 ) {
		rc = false;
	    } 

	    /* All went well, so return it */
	    if ( rc ) {
//		I will have to register functions for LQX here.
		LQX::Program * program = document->getLQXProgram();
		if ( program ) {
//		    LQX::Environment * environment = program->getEnvironment();
//	            environment->getMethodTable()->registerMethod(new SolverInterface::Solve(document, &Model::restart, aModel));
		}
		return document;
	    } else {
		delete document;
		return 0;
	    }
	}


	bool
	Document::loadResults(const std::string& directory_name, const std::string& file_name, const std::string& suffix, unsigned& errorCode )
	{
	    if ( getInputFormat() == LQN_INPUT ) {
		LQIO::Filename filename( file_name, "p", directory_name, suffix );
		return LQIO::SRVN::loadResults( filename() );
	    } else if ( getInputFormat() == XML_INPUT ) {
		LQIO::Filename filename( file_name, "lqxo", directory_name, suffix );
#if HAVE_LIBEXPAT
		return Expat_Document::loadResults( *this, filename(), errorCode );
#else
		return false;
#endif
	    } else {
		return false;
	    }
	}

	/* static */ Document::input_format
	Document::getInputFormatFromFilename( const std::string& filename, const input_format default_format )
	{
	    const unsigned long pos = filename.find_last_of( '.' );
	    if ( pos == std::string::npos ) {
		return default_format;
	    } else {
		std::string suffix = filename.substr( pos+1 );
		std::transform(suffix.begin(), suffix.end(), suffix.begin(), ::tolower);
		if ( suffix == "in" || suffix == "lqn" || suffix == "xlqn" || suffix == "txt" || suffix == "spex" ) {
		    return LQN_INPUT;		/* Override */
		} else {
		    return XML_INPUT;
		}
	    }
	}

	/*
	 * Print in input order.
	 */

        std::ostream&
	Document::print( std::ostream& output, const output_format format ) const
	{
	    switch ( format ) {
	    case DEFAULT_OUTPUT:
	    case LQN_OUTPUT: {
		SRVN::Output srvn( *this, _entities );
		srvn.print( output );
		break;
	    }
	    case PARSEABLE_OUTPUT:{
		SRVN::Parseable srvn( *this, _entities );
		srvn.print( output );
		break;		
	    } 
	    case RTF_OUTPUT: {
		SRVN::RTF srvn( *this, _entities );
		srvn.print( output );
		break;
	    }
	    case XML_OUTPUT: {
#if HAVE_LIBEXPAT
		Expat_Document expat( *const_cast<Document *>(this), __input_file_name, false, false );
		expat.serializeDOM( output );
#endif
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
	    for (std::map<std::string, SymbolExternalVariable*>::const_iterator var_p = _variables.begin(); var_p != _variables.end(); ++var_p) {
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

    lqio_params_stats::lqio_params_stats( const char * version, void (*action)(unsigned) ) :
	lq_toolname(),
	lq_version(version),
	lq_command_line(),
	severity_action(action),
	max_error(10),
	error_count(0),
	severity_level(LQIO::NO_ERROR),
	error_messages(global_error_messages)
    {
    }
    
}
