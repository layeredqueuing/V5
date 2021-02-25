/*
 *  $Id: dom_document.cpp 14488 2021-02-24 22:26:04Z greg $
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
#include "jmva_document.h"
#endif
#include "srvn_input.h"
#include "srvn_results.h"
#include "srvn_output.h"
#include "srvn_spex.h"
#include "filename.h"
#include "dom_processor.h"

extern void ModLangParserTrace(FILE *TraceFILE, char *zTracePrompt);

namespace LQIO {
    lqio_params_stats io_vars(VERSION,nullptr);
    
    namespace DOM {
	Document* __document = nullptr;
	bool Document::__debugXML = false;
	std::map<const char *, double> Document::__initialValues;
	std::string Document::__input_file_name = "";
	const char * Document::XComment = "comment";
	const char * Document::XConvergence = "conv_val";			/* Matches schema. 	*/
	const char * Document::XIterationLimit = "it_limit";			/* Matched schema.	*/
	const char * Document::XPrintInterval = "print_int";			/* Matches schema.	*/
	const char * Document::XUnderrelaxationCoefficient = "underrelax_coeff";/* Matches schema.	*/
	const char * Document::XSpexIterationLimit = "spex_it_limit";
	const char * Document::XSpexUnderrelaxation = "spex_underrelax_coeff";
    
	Document::Document( input_format format ) 
	    : _modelComment(), _documentComment(),
	      _processors(), _groups(), _tasks(), _entries(), 
	      _entities(), _variables(), _controlVariables(), _nextEntityId(0), 
	      _format(format), 
	      _lqxProgram(""), _lqxProgramLineNumber(0), _parsedLQXProgram(0), _instantiated(false), _pragmas(),
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

	    for ( std::map<const char *, ExternalVariable*>::const_iterator var = _controlVariables.begin(); var != _controlVariables.end(); ++var ) {
		delete var->second;
	    }

	    /* BUG_277 Delete External Variables */

	    for ( std::map<std::string, SymbolExternalVariable*>::const_iterator var = _variables.begin(); var != _variables.end(); ++var ) {
		delete var->second;
	    }
	    
	    LQIO::Spex::clear();

	    __document = nullptr;
	    __input_file_name = "";
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

	std::string Document::getModelCommentString() const 
	{
	    const char * s;
	    const std::map<const char *, ExternalVariable *>::const_iterator iter = _controlVariables.find(XComment);
	    if ( iter != _controlVariables.end() && iter->second->wasSet() ) {
		if ( iter->second->getString( s ) ) return std::string(s);
	    } 
	    return _modelComment;
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

	const std::string& Document::getDocumentComment() const 
	{
	    return _documentComment;
	}

	Document& Document::setDocumentComment( const std::string& value )
	{	
	    _documentComment = value;
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
		if ( var != nullptr && var->wasSet() ) {
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
	    /* Returns a list of all undefined external variables as a string */
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
	    const char * s = _modelComment.c_str();
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
	Document::db_build_parameter_variable(const char* input, bool* isSymbol)
	{
	    if (isSymbol) { *isSymbol = false; }
	    if ( input == nullptr ) {
		/* nullptr input means a zero */
		return new ConstantExternalVariable(0.0);
	    } else if ( input[0] == '$' ) {
		if (isSymbol) { *isSymbol = true; }
		return getSymbolExternalVariable(input);
	    } else if ( strcmp( input, "@infinity" ) == 0 ) {
		return new ConstantExternalVariable( srvn_get_infinity() );
	    } else {
		double result = 0.0;
		char* endPtr = nullptr;
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
	Document::db_check_set_entry(Entry* entry, const std::string& entry_name, Entry::Type requisiteType)
	{
	    if ( !entry ) {
		input_error2( ERR_NOT_DEFINED, entry_name.c_str() );
	    } else if ( requisiteType != Entry::Type::NOT_DEFINED && !entry->entryTypeOk( requisiteType ) ) {
		input_error2( ERR_MIXED_ENTRY_TYPES, entry_name.c_str() );
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
	Document::load(const std::string& input_filename, input_format format, unsigned& errorCode, bool load_results )
	{
	    __input_file_name = input_filename;
            io_vars.reset();                   /* See error.c */

	    /* Figure out input file type based on suffix */

	    if ( format == AUTOMATIC_INPUT ) {
		format = getInputFormatFromFilename( input_filename );
	    }

	    /* Create a document to store the product */
	
	    bool rc = true;
	    Document * document = new Document( format );
	    LQIO::Spex::__global_variables = &document->_variables;	/* For SPEX */

	    /* Read in the model, invoke the builder, and see what happened */

	    switch ( format ) {

	    case AUTOMATIC_INPUT:
	    case LQN_INPUT:
		rc = SRVN::load( *document, input_filename, load_results );
		break;

#if HAVE_LIBEXPAT
	    case XML_INPUT:
		rc = Expat_Document::load( *document, input_filename, load_results );
		break;
		
	    case JMVA_INPUT:
		rc = BCMP::JMVA_Document::load( *document, input_filename );
		break;
#else
	    case XML_INPUT:
	    case JMVA_INPUT:
		rc = false;
		break;
#endif

	    case JSON_INPUT:
//		rc = Json_Document::load( *document, input_filename, errorCode, load_results );
		rc = false;
		break;

	    default:
		rc = false;
		break;
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
		return nullptr;
	    }
	}


	bool
	Document::loadResults(const std::string& directory_name, const std::string& file_name, const std::string& suffix, unsigned& errorCode )
	{
	    switch ( getInputFormat() ) {
	    case LQN_INPUT:
		return LQIO::SRVN::loadResults( LQIO::Filename( file_name, "p", directory_name, suffix )() );

	    case XML_INPUT:
#if HAVE_LIBEXPAT
		return Expat_Document::loadResults( *this, LQIO::Filename( file_name, "lqxo", directory_name, suffix )() );
#else
		return false;
#endif
	    default:
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
		} else if ( suffix == "jmva" ) {
		    return JMVA_INPUT;
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
	error_messages()
    {
	error_messages.insert( error_messages.begin(), global_error_messages, global_error_messages+LSTGBLERRMSG+1 );
    }

    void lqio_params_stats::init( const std::string& version, const std::string& toolname, void (*sa)(unsigned), ErrorMessageType * local_error_messages, size_t size )
    {
	lq_version = version;
	lq_toolname = toolname;
	severity_action = sa;
	if ( local_error_messages != nullptr ) {
	    error_messages.insert( error_messages.end(), local_error_messages, local_error_messages+size+1 );
	}
	LQIO::io_vars.max_error = error_messages.size();
    }
}
