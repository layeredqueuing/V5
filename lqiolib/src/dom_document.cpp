/*
 *  $Id$
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
#if HAVE_LIBXERCES_C
#include "xerces_document.h"
#elif HAVE_LIBEXPAT
#include "expat_document.h"
#endif
#include "srvn_input.h"
#include "srvn_results.h"
#include "srvn_output.h"
#include "filename.h"

namespace LQIO {
    const char * input_file_name  = 0;
    const char * output_file_name = 0;

    namespace DOM {
	lqio_params_stats* Document::io_vars = NULL;
	Document* currentDocument = NULL;
	bool Document::__debugXML = false;
    
	Document::Document( lqio_params_stats* ioVars ) 
	    : _conf_95( ConfidenceIntervals( LQIO::ConfidenceIntervals::CONF_95 ) ),
	      _conf_99( ConfidenceIntervals( LQIO::ConfidenceIntervals::CONF_99 ) ),
	      _processors(), _groups(), _tasks(), _entries(), 
	      _entities(), _variables(), _nextEntityId(0), 
	      _comment(), _comment2(), _convergenceValue(0), _iterationLimit(0), _printInterval(0), _underrelaxationCoefficient(0), 
	      _seedValue(0), _numberOfBlocks(0), _blockTime(0), _resultPrecision(0), _warmUpLoops(0), _warmUpTime(0),
	      _xmlDomElement(0),
	      _lqxProgram(""), _lqxProgramLineNumber(0), _parsedLQXProgram(0), _loadedPragmas(), 
	      _maximumPhase(0), _hasResults(false), _hasRendezvous(false), _hasSendNoReply(false), _hasForwarding(false), _hasMaxServiceTime(false), _hasHistogram(false), 
	      _hasSemaphoreWait(false), _hasReaderWait(false), _hasWriterWait(false),
	      _entryHasThinkTime(false), _entryHasNonExponentialPhase(false), _entryHasDeterministicPhase(false), 
	      _entryHasOpenArrivals(false), _entryHasThroughputBound(false), _entryHasOpenWait(false),
	      _entryHasWaitingTimeVariance(false), _entryHasServiceTimeVariance(false), _entryHasDropProbability(false),
	      _taskHasAndJoin(false), _taskHasThinkTime(false),
	      _processorHasRate(false),
	      _resultValid(false),
	      _hasConfidenceIntervals(false),
	      _resultInvocationNumber(0),
	      _resultConvergenceValue(0.0),
	      _resultIterations(0),
	      _resultUserTime(0),
	      _resultSysTime(0),
	      _resultElapsedTime(0)
	{
	    assert( ioVars );			/* Must be set.  See Dom_builder.cpp */
	    io_vars = ioVars;
	    currentDocument = this;
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

	    io_vars = NULL;
	    if ( input_file_name ) {
		input_file_name = 0;
	    }
	    if ( output_file_name ) {
		output_file_name = 0;
	    }
	}
    
	void Document::setModelParameters(const char* comment, LQIO::DOM::ExternalVariable* conv_val, LQIO::DOM::ExternalVariable* it_limit, LQIO::DOM::ExternalVariable* print_int, LQIO::DOM::ExternalVariable* underrelax_coeff, const void * element )
	{
	    /* Set up initial model parameters */
	    _comment = std::string(comment);
	    _convergenceValue = conv_val;
	    _iterationLimit   = it_limit;
	    _printInterval    = print_int;
	    _underrelaxationCoefficient = underrelax_coeff;
	    _xmlDomElement    = element;
	}

	const std::string& Document::getModelComment() const 
	{
	    return _comment;
	}

	const std::string& Document::getExtraComment() const 
	{
	    return _comment2;
	}

	Document& Document::setModelComment( const std::string& value )
	{	
	    _comment = value;
	    return *this;
	}

	Document& Document::setExtraComment( const std::string& value )
	{	
	    _comment2 = value;
	    return *this;
	}

	const double Document::getModelConvergenceValue() const
	{	
	    double value = 0.0;
	    if ( _convergenceValue ) {
		assert(_convergenceValue->getValue(value) == true);
	    } else {
		value = 0.000001;
	    }
	    return value;
	}

	Document& Document::setModelConvergenceValue( ExternalVariable * value )
	{	
	    _convergenceValue = value;
	    return *this;
	}

	const unsigned int Document::getModelIterationLimit() const
	{
	    double value = 0.0;
	    if ( _iterationLimit ) {
		assert(_iterationLimit->getValue(value) == true);
		assert(value - floor(value) == 0);
	    } else {
		value = 50.;
	    }
	    return static_cast<int>(value);
	}

	Document& Document::setModelIterationLimit( ExternalVariable * value ) 
	{
	    _iterationLimit = value;
	    return *this;
	}

	const unsigned int Document::getModelPrintInterval() const
	{
	    double value = 0.0;
	    if ( _printInterval && _printInterval->getValue(value) ) {
		assert(value - floor(value) == 0);
	    }
	    return static_cast<int>(value);
	}

	Document& Document::setModelPrintInterval( ExternalVariable * value ) 
	{
	    _printInterval = value;
	    return *this;
	}

	const double Document::getModelUnderrelaxationCoefficient() const
	{
	    double value = 0.0;
	    if ( _underrelaxationCoefficient ) {
		assert(_underrelaxationCoefficient->getValue(value) == true);
	    } else {
		value = 0.9;
	    }
	    return value;
	}

	Document& Document::setModelUnderrelaxationCoefficient( ExternalVariable * value ) 
	{
	    _underrelaxationCoefficient = value;
	    return *this;
	}

	const long Document::getSimulationSeedValue() const
	{
	    double value = 0.0;
	    if ( _seedValue ) {
		assert(_seedValue->getValue(value) == true);
	    } 
	    return static_cast<long>(value);
	}

	Document& Document::setSimulationSeedValue( ExternalVariable * value )
	{
	    _seedValue = value;
	    return *this;
	}

	const long Document::getSimulationNumberOfBlocks( ) const
	{
	    double value = 0.0;
	    if ( _numberOfBlocks ) {
		assert(_numberOfBlocks->getValue(value) == true);
	    } 
	    return static_cast<long>(value);
	}

	Document& Document::setSimulationNumberOfBlocks( ExternalVariable * value )
	{
	    _numberOfBlocks = value;
	    return *this;
	}

	const double Document::getSimulationBlockTime( ) const
	{
	    double value = 0.0;
	    if ( _blockTime ) {
		assert(_blockTime->getValue(value) == true);
	    } 
	    return value;
	}

	Document& Document::setSimulationBlockTime( ExternalVariable * value )
	{
	    _blockTime = value;
	    return *this;
	}

	const double Document::getSimulationResultPrecision( ) const
	{
	    double value = 0.0;
	    if ( _resultPrecision ) {
		assert(_resultPrecision->getValue(value) == true);
	    } 
	    return value;
	}

	Document& Document::setSimulationResultPrecision( ExternalVariable * value )
	{
	    _resultPrecision = value;
	    return *this;
	}

	const long  Document::getSimulationWarmUpLoops( ) const
	{
	    double value = 0.0;
	    if ( _warmUpLoops ) {
		assert(_warmUpLoops->getValue(value) == true);
	    } 
	    return static_cast<long>(value);
	}

	Document& Document::setSimulationWarmUpLoops( ExternalVariable * value )
	{
	    _warmUpLoops = value;
	    return *this;
	}


	const double Document::getSimulationWarmUpTime( ) const
	{
	    double value = 0.0;
	    if ( _warmUpTime ) {
		assert(_warmUpTime->getValue(value) == true);
	    } 
	    return value;
	}

	Document& Document::setSimulationWarmUpTime( ExternalVariable * value )
	{
	    _warmUpTime = value;
	    return *this;
	}


	void 
	Document::setMVAStatistics( const unsigned int submodels, const unsigned long core, const double step, const double step_squared, const double wait, const double wait_squared, const unsigned int faults )
	{
	    _mvaStatistics.submodels = submodels;
	    _mvaStatistics.core = core;
	    _mvaStatistics.step = step;
	    _mvaStatistics.step_squared = step_squared;
	    _mvaStatistics.wait = wait;
	    _mvaStatistics.wait_squared = wait_squared;
	    _mvaStatistics.faults = faults;
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
	    if(_processors.find(name) != _processors.end()) {
		return const_cast<Document *>(this)->_processors[name];
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
	    std::map<std::string, SymbolExternalVariable*>::const_iterator iter;
	    for (iter = _variables.begin(); iter != _variables.end(); ++iter) {
		SymbolExternalVariable* current = iter->second;
		if (current->wasSet() == false) {
		    return false;
		}
	    }
      
	    return true;
	}
    
	std::string Document::getUndefinedExternalVariables() const
	{
	    /* Returns a list of all undefined external variables as a string */
	    std::string names;
	    for (std::map<std::string, SymbolExternalVariable*>::const_iterator iter = _variables.begin(); iter != _variables.end(); ++iter) {
		SymbolExternalVariable* current = iter->second;
		if (current->wasSet() == false) {
		    if ( names.size() > 0 ) {
			names += ", ";
		    }
		    names += iter->first;
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
	    /* Make sure all of the variables are registered in the program */
	    std::map<std::string, SymbolExternalVariable*>::iterator iter;
	    for (iter = _variables.begin(); iter != _variables.end(); ++iter) {
		SymbolExternalVariable* current = iter->second;
		current->registerInEnvironment(program);
	    }
	}
    
	void Document::addPragma(const std::string& param, const std::string& value )
	{
	    _loadedPragmas[param] = value;
	}
    
	const std::map<std::string,std::string>& Document::getPragmaList() const
	{
	    return _loadedPragmas;
	}

	void Document::clearPragmaList() 
	{
	    _loadedPragmas.clear();
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

	Document& Document::setResultUserTime(clock_t resultUserTime) 
	{ 
	    _hasResults = true;
	    _resultUserTime = resultUserTime;
	    return *this;
	}

	Document& Document::setResultSysTime(clock_t resultSysTime) 
	{ 
	    _hasResults = true;
	    _resultSysTime = resultSysTime;
	    return *this;
	}

	Document& Document::setResultElapsedTime(clock_t resultElapsedTime) 
	{ 
	    _hasResults = true;
	    _resultElapsedTime = resultElapsedTime;
	    return *this;
	}

	bool Document::hasResults() const
	{
	    return _hasResults;
	}

	/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- [Dom builder ] -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

	ExternalVariable* 
	Document::db_build_parameter_variable(const char* input, bool* isSymbol) 
	{
	    if (input && input[0] == '$') {
		if (isSymbol) { *isSymbol = true; }
		return getSymbolExternalVariable(input);
	    } else {
		double result = 0.0;

		/* NULL input means a zero */
		if (input != NULL) {
		    char* endPtr = NULL;
		    const char* realEndPtr = input + strlen(input);
		    result = strtod(input, &endPtr);

		    /* Check if we finished parsing okay */
		    if (endPtr != realEndPtr) {
			printf("WARNING: Not all of `%s' was converted to a double.\n", input);
			printf("WARNING: Problem ignored.\n");
		    }
		}

		/* Return the resulting value */
		if (isSymbol) { *isSymbol = false; }
		return new ConstantExternalVariable(result);
	    }
	}
	
	void 
	Document::db_check_set_entry(Entry* entry, const string& toEntryName, Entry::EntryType requisiteType)
	{
	    if ( !entry ) {
		input_error2( ERR_NOT_DEFINED, toEntryName.c_str() );
	    } else if ( requisiteType != Entry::ENTRY_NOT_DEFINED && !entry->entryTypeOk( requisiteType ) ) {
		input_error2( ERR_MIXED_ENTRY_TYPES, toEntryName.c_str() );
	    }
	}

	Document&
	Document::setCallType( const Call::CallType t ) 
	{
	    if ( t == Call::RENDEZVOUS ) _hasRendezvous = true;
	    if ( t == Call::SEND_NO_REPLY ) _hasSendNoReply = true;
	    if ( t == Call::FORWARD ) _hasForwarding = true;
	    return *this;
	}

	Document& 
	Document::setPhaseType( const phase_type t )
	{
	    if ( t == PHASE_DETERMINISTIC ) _entryHasDeterministicPhase = true;
	    return *this;
	}

	Document* Document::create( lqio_params_stats* io_vars, bool load_results )
	{
#if HAVE_LIBXERCES_C
	    return new Xerces_Document( io_vars );
#elif HAVE_LIBEXPAT
	    return new Expat_Document( io_vars, load_results );		// For XML output.
#else
	    return new Document( io_vars, load_results );		// For XML output.
#endif
	}

	/*
	 * Load document.  Based on the file extension, pick the
	 * appropriate loader.  LQNX (XML) input and output are found
	 * in one file.  SRVN input and ouptut are separate.  We don't
	 * try to load the latter if load_results == false
	 */

	/* static */ Document* 
	Document::load(const string& input_filename, input_format format, const string& output_filename, lqio_params_stats * ioVars, unsigned& errorCode, bool load_results )
	{
	    errorCode = 0;

	    /* Create a document to store the product */
	    Document * document = 0;
	
	    /* Figure out input file type based on suffix */

	    if ( format == AUTOMATIC_INPUT ) {
		const unsigned long pos = input_filename.find_last_of( '.' );
		if ( pos != string::npos ) {
		    string suffix = input_filename.substr( pos+1 );
		    std::transform(suffix.begin(), suffix.end(), suffix.begin(), ::tolower);
		    if ( suffix == "in" || suffix == "lqn" || suffix == "xlqn" || suffix == "txt" ) {
			format = LQN_INPUT;		/* Override */
		    }
		}
	    }

	    /* Read in the model, invoke the builder, and see what happened */

	    if ( format == LQN_INPUT ) {
		document = LoadSRVN( input_filename, output_filename, ioVars,  errorCode );
		if ( document != 0 && load_results ) {
		    LQIO::Filename parse_name( input_filename.c_str(), "p" );
		    if ( parse_name.mtimeCmp( input_filename.c_str() ) < 0 ) {
			cerr << LQIO::DOM::Document::io_vars->lq_toolname << ": input file " << input_filename << " is more recent than " << parse_name() 
			     << " -- results ignored. " << endl;
		    } else {
			loadSRVNResults( parse_name() );
		    }
		}
	    } else {
#if HAVE_LIBXERCES_C
		document = Xerces_Document::LoadLQNX( input_filename, ioVars, errorCode );
		if ( errorCode != 0 ) {
		    delete document;
		    return NULL;
		} 
#elif HAVE_LIBEXPAT
		document = Expat_Document::LoadLQNX( input_filename, output_filename, ioVars, errorCode, load_results );
#endif
	    }

	    /* All went well, so return it */
	    return document;
	}


	/*
	 * Print in input order.
	 */

        std::ostream&
	Document::print( std::ostream& output, const output_format format ) const
	{
	    if ( format == TEXT_OUTPUT ) {
		SRVN::Output srvn( *this, _entities );
		srvn.print( output );
	    } else if ( format == PARSEABLE_OUTPUT ) {
		SRVN::Parseable srvn( *this, _entities );
		srvn.print( output );
	    } else if ( format == RTF_OUTPUT ) {
		SRVN::RTF srvn( *this, _entities );
		srvn.print( output );
	    }

	    return output;
	}


	double
	Document::invert( const double arg ) const
	{
	    return _conf_95.invert( arg );
	}
    }
}
