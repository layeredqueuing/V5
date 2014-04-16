/* -*- c++ -*-
 *  $Id$
 *
 *  Created by Martin Mroz on 24/02/09.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#ifndef __LQIO_DOM_DOCUMENT__
#define __LQIO_DOM_DOCUMENT__

#include <sys/time.h>
#include <map>
#include "dom_entity.h"
#include "dom_task.h"
#include "dom_processor.h"
#include "dom_entry.h"
#include "dom_activity.h"
#include "dom_actlist.h"
#include "dom_call.h"
#include "dom_extvar.h"
#include "dom_phase.h"
#include "dom_group.h"

struct lqio_params_stats;

namespace LQX {
    class Program;
}

namespace LQIO {
    namespace DOM {
	
	class Document {

	public:
	    typedef enum { TEXT_OUTPUT, RTF_OUTPUT, PARSEABLE_OUTPUT } output_format;
	    typedef enum { AUTOMATIC_INPUT, LQN_INPUT, XML_INPUT } input_format;

	private:
	    Document& operator=( const Document& );
	    Document( const Document& );
	    
	    /* Constructors and Destructors */

	public:
	    Document( lqio_params_stats*, input_format );
	    virtual ~Document();
      
	    const void * getXMLDOMElement() const { return _xmlDomElement; }
	    input_format getInputFormat() const { return _format; }

	    /* STORE: Model Parameter Information */
	    void setModelParameters(const char *comment, ExternalVariable* conv_val, ExternalVariable* it_limit, ExternalVariable* print_int, ExternalVariable* underrelax_coeff, const void * arg );
	    void setMVAStatistics( const unsigned int, const unsigned long, const double, const double, const double, const double, const unsigned int );
	    
	    /* Obtaining identifiers for entities */
	    unsigned getNextEntityId();
      
	    /* Handling Entries, Processors, Tasks, Phases and Groups */
	    void addProcessorEntity(Processor* processor);
	    Processor* getProcessorByName(const std::string& name) const;
	    void addTaskEntity(Task* task);
	    Task* getTaskByName(const std::string& name) const;
	    const std::string* getTaskNames(unsigned& count) const;
	    void addEntry(Entry* entry);
	    Entry* getEntryByName(const std::string& name) const;
	    void addGroup(Group* group);
	    Group* getGroupByName(const std::string& name) const;

	    const std::map<std::string,Processor*>& getProcessors() const;	// For output functions.
	    const std::map<std::string,Group*>& getGroups() const;	// For output functions.
	    const std::map<std::string,Task*>& getTasks() const;	// For output functions.
	    const std::map<std::string,Entry*>& getEntries() const;	//
	    const std::map<unsigned,Entity *>& getEntities() const;	// For remapping by lqn2ps.

	    void clearAllMaps();		/* For rep2flat. */
      
	    /* Managing External Variables (NON-const) */
	    SymbolExternalVariable* getSymbolExternalVariable(const std::string& name);
	    bool hasSymbolExternalVariable(const std::string& name) const;
	    unsigned getSymbolExternalVariableCount() const;
	    bool areAllExternalVariablesAssigned() const;
	    std::vector<std::string> getUndefinedExternalVariables() const;
      
	    /* Setting the LQX Program Code */
	    void setLQXProgramText(const std::string& program);
	    void setLQXProgramText(const char *);
	    void setLQXProgram( LQX::Program * program ) { _parsedLQXProgram = program; }
	    const std::string& getLQXProgramText() const;
	    LQX::Program * getLQXProgram() const;
	    bool hasLQXProgram() const { return _parsedLQXProgram != 0; }
	    void setLQXProgramLineNumber( const unsigned n ) { _lqxProgramLineNumber = n; }
	    const unsigned getLQXProgramLineNumber() const { return _lqxProgramLineNumber; }
	    void registerExternalSymbolsWithProgram(LQX::Program* program);
      
	    /* LQX Pragma Support */
	    void addPragma(const std::string&,const std::string&);
	    const std::map<std::string,std::string>& getPragmaList() const;
	    bool hasPragmas() const { return getPragmaList().size() > 0; }
	    void clearPragmaList();

	    /* Model Parameters */
	    const std::string& getModelComment() const;
	    const std::string& getExtraComment() const;
	    const double getModelConvergenceValue() const;
	    const unsigned int getModelIterationLimit() const;
	    const unsigned int getModelPrintInterval() const;
	    const double getModelUnderrelaxationCoefficient() const;
	    Document& setModelComment( const std::string& );
	    Document& setExtraComment( const std::string& );		/* Does not show up in input file. */
	    Document& setModelConvergenceValue( ExternalVariable * );
	    Document& setModelIterationLimit( ExternalVariable * );
	    Document& setModelPrintInterval( ExternalVariable * );
	    Document& setModelUnderrelaxationCoefficient( ExternalVariable * );
	    const long getSimulationSeedValue() const;
	    const long getSimulationNumberOfBlocks() const; 
	    const double getSimulationBlockTime() const;
	    const double getSimulationResultPrecision() const;
	    const double getSimulationWarmUpTime() const;
	    const long getSimulationWarmUpLoops() const;    
	    Document& setSimulationSeedValue( ExternalVariable * );
	    Document& setSimulationNumberOfBlocks( ExternalVariable * ); 
	    Document& setSimulationBlockTime( ExternalVariable * );      
	    Document& setSimulationResultPrecision( ExternalVariable * );
	    Document& setSimulationWarmUpLoops( ExternalVariable * );    
	    Document& setSimulationWarmUpTime( ExternalVariable * );    
	    const double getSpexConvergenceIterationLimit() const;
	    const double getSpexConvergenceUnderrelaxation() const;
	    Document& setSpexConvergenceIterationLimit( ExternalVariable * );
	    Document& setSpexConvergenceUnderrelaxation( ExternalVariable * );

      
	    /* Cached values for formatting */
	    const unsigned getNumberOfProcessors() const { return _processors.size(); }
	    const unsigned getNumberOfTasks() const { return _tasks.size(); }
	    const unsigned getNumberOfEntries() const { return _entries.size(); }
	    const unsigned getNumberOfGroups() const { return _groups.size(); }
	    Document& setMaximumPhase( unsigned p ) { _maximumPhase = max( _maximumPhase, p ); return *this; }
	    unsigned getMaximumPhase() const { return _maximumPhase; }
	    Document& setCallType( Call::CallType t );
	    bool hasRendezvous() const { return _hasRendezvous; }
	    bool hasSendNoReply() const { return _hasSendNoReply; }
	    bool hasForwarding() const { return _hasForwarding; }
	    Document& setHasMaxServiceTime( const bool hasMaxServiceTime ) { _hasMaxServiceTime = hasMaxServiceTime; return *this; }
	    bool hasMaxServiceTime() const { return _hasMaxServiceTime; }
	    Document& setHasHistogram( const bool hasHistogram ) { _hasHistogram = hasHistogram; return *this; }
	    bool hasHistogram() const { return _hasHistogram; }
	    Document& setHasSemaphoreWait( const bool hasSemaphoreWait ) { _hasSemaphoreWait = hasSemaphoreWait; return *this; }
	    bool hasSemaphoreWait() const { return _hasSemaphoreWait; }

	    Document& setHasReaderWait( const bool hasReaderWait ) { _hasReaderWait = hasReaderWait; return *this; }
	    bool hasReaderWait() const { return _hasReaderWait; }
	    Document& setHasWriterWait( const bool hasWriterWait ) { _hasWriterWait = hasWriterWait; return *this; }
	    bool hasWriterWait() const { return _hasWriterWait; }

	    Document& setEntryHasThinkTime( const bool entryHasThinkTime ) { _entryHasThinkTime = entryHasThinkTime; return *this; }
	    bool entryHasThinkTime() const { return _entryHasThinkTime; }
	    Document& setEntryHasNonExponentialPhase( const bool entryHasNonExponentialPhase ) { _entryHasNonExponentialPhase = entryHasNonExponentialPhase; return *this; }
	    bool entryHasNonExponentialPhase() const { return _entryHasNonExponentialPhase; }
	    Document& setPhaseType( const phase_type );
	    bool entryHasDeterministicPhase() const { return _entryHasDeterministicPhase; }
	    Document& setEntryHasOpenArrivals( const bool entryHasOpenArrivals ) { _entryHasOpenArrivals = entryHasOpenArrivals; return *this; }
	    bool entryHasOpenArrivals() const { return _entryHasOpenArrivals; }
	    Document& setEntryHasThroughputBound( const bool entryHasThroughputBound ) { _entryHasThroughputBound = entryHasThroughputBound; return *this; }
	    bool entryHasThroughputBound() const { return _entryHasThroughputBound; }
	    Document& setEntryHasOpenWait( const bool entryHasOpenWait ) { _entryHasOpenWait = entryHasOpenWait; return *this; }
	    bool entryHasOpenWait() const { return _entryHasOpenWait; }
	    Document& setEntryHasWaitingTimeVariance( const bool entryHasWaitingTimeVariance ) { _entryHasWaitingTimeVariance = entryHasWaitingTimeVariance; return *this; }
	    bool entryHasWaitingTimeVariance() const { return _entryHasWaitingTimeVariance; }
	    Document& setEntryHasDropProbability( bool entryHasDropProbability ) { _entryHasDropProbability = entryHasDropProbability; return *this; }
	    bool entryHasDropProbability() const { return _entryHasDropProbability; }
	    Document& setEntryHasServiceTimeVariance( const bool entryHasServiceTimeVariance ) { _entryHasServiceTimeVariance = entryHasServiceTimeVariance; return *this; }
	    bool entryHasServiceTimeVariance() const { return _entryHasServiceTimeVariance; }
	    Document& setTaskHasAndJoin( const bool taskHasAndJoin ) { _taskHasAndJoin = taskHasAndJoin; return *this; }
	    bool taskHasAndJoin() const { return _taskHasAndJoin; }
	    Document& setTaskHasThinkTime( const bool taskHasThinkTime ) { _taskHasThinkTime = taskHasThinkTime; return *this; }
	    bool taskHasThinkTime() const { return _taskHasThinkTime; }
	    Document& setProcessorHasRate( const bool processorHasRate ) { _processorHasRate = processorHasRate; return *this; }
	    bool processorHasRate() const { return _processorHasRate; }

	    /* Results */
	    unsigned int getResultInvocationNumber() const { return _resultInvocationNumber; }
	    Document& setResultInvocationNumber( const unsigned int resultInvocationNumber ) { _resultInvocationNumber = resultInvocationNumber; return *this; }
	    double getResultConvergenceValue() const { return _resultConvergenceValue; }
	    Document& setResultConvergenceValue(double resultConvergenceValue);
	    bool getResultValid() const { return _resultValid; }
	    Document& setResultValid(bool resultValid);
	    unsigned int getResultIterations() const { return _resultIterations; }
	    Document& setResultIterations(unsigned int resultIterations);
	    unsigned int getResultNumberOfBlocks() const { return _hasConfidenceIntervals ? _resultIterations : 0; }
	    Document& setResultHasConfidenceIntervals( const bool hasConfidenceIntervals ) { _hasConfidenceIntervals = hasConfidenceIntervals; return *this; }
	    Document& setResultHasBottleneckStrength( const bool hasBottleneckStrength ) { _hasBottleneckStrength = hasBottleneckStrength; return *this; }
	    const std::string& getResultPlatformInformation() const { return _resultPlatformInformation; }
	    Document& setResultPlatformInformation(const std::string& resultPlatformInformation);
	    const std::string& getResultSolverInformation() const { return _resultSolverInformation; }
	    Document& setResultSolverInformation(const std::string& resultSolverInformation);
	    clock_t getResultElapsedTime() const { return _resultElapsedTime; }
	    Document& setResultElapsedTime(clock_t resultElapsedTime);
	    clock_t getResultSysTime() const { return _resultSysTime; }
	    Document& setResultSysTime(clock_t resultSysTime);
	    clock_t getResultUserTime() const { return _resultUserTime; }
	    Document& setResultUserTime(clock_t resultUserTime);

	    unsigned int getResultMVASubmodels() const { return _mvaStatistics.submodels; }
	    unsigned long getResultMVACore() const { return _mvaStatistics.core; }
	    double getResultMVAStep() const { return _mvaStatistics.step; }
	    double getResultMVAStepSquared() const { return _mvaStatistics.step_squared; }
	    double getResultMVAWait() const { return _mvaStatistics.wait; }
	    double getResultMVAWaitSquared() const { return _mvaStatistics.wait_squared; }
	    unsigned int getResultMVAFaults() const { return _mvaStatistics.faults; }

	    /* Queries */
	    
	    virtual bool hasResults() const;
	    bool hasConfidenceIntervals() const { return _hasConfidenceIntervals; }
	    bool hasBottleneckStrength() const { return _hasBottleneckStrength; }
	    virtual bool isXMLDOMPresent() const;

	    /* I/O */
	    void serializeDOM( std::ostream& output, bool instantiate=false ) const;
	    static Document* load(const std::string&, input_format format, const std::string&, lqio_params_stats*, unsigned& errorCode, bool load_results );
	    virtual bool loadResults( const std::string&, const std::string&, const std::string&, unsigned& errorCode );
	    std::ostream& print( std::ostream& ouptut, const output_format format=TEXT_OUTPUT ) const;
	    std::ostream& printExternalVariables( std::ostream& ouptut ) const;

	    /* Semi-private */

	    static void db_check_set_entry(DOM::Entry* entry, const std::string& toEntryName, DOM::Entry::EntryType requisiteType = DOM::Entry::ENTRY_NOT_DEFINED );
	    DOM::ExternalVariable* db_build_parameter_variable(const char* input, bool* isSymbol) throw( std::invalid_argument );
	    static lqio_params_stats* io_vars;
	    static bool __debugXML;
      
	private:
	    unsigned max( const unsigned i, const unsigned j ) { return i > j ? i : j; }
	    
	private:
	    /* List of Processors */

	    std::map<std::string, Processor*> _processors;    	/* processor.name -> Processor */
	    std::map<std::string, Group*> _groups;            	/* group.name -> Group */
	    std::map<std::string, Task*> _tasks;              	/* task.name -> Task */
	    std::map<std::string, Entry*> _entries;           	/* entry.name -> Entry */
	    std::map<unsigned, Entity*> _entities;            	/* entity.id -> Entity */

	    /* We need to make sure all variables named the same point the same */
	    std::map<std::string, SymbolExternalVariable*> _variables;
      
	    unsigned _nextEntityId;                           	/* for sorting, see _entities 	*/
	    const input_format _format;				/* input format 		*/
      
	    /* Parameter Information */
	    std::string _comment;
	    std::string _comment2;
	    ExternalVariable * _convergenceValue;
	    ExternalVariable * _iterationLimit;
	    ExternalVariable * _printInterval;
	    ExternalVariable * _underrelaxationCoefficient;
	    ExternalVariable * _seedValue;
	    ExternalVariable * _numberOfBlocks;
	    ExternalVariable * _blockTime;
	    ExternalVariable * _resultPrecision;
	    ExternalVariable * _warmUpLoops;
	    ExternalVariable * _warmUpTime;
	    ExternalVariable * _spexIterationLimit;
	    ExternalVariable * _spexUnderrelaxation;

	    double _defaultConvergenceValue;

	    const void * _xmlDomElement;	/* Xerces object */

	    /* The stored LQX program, if any */
	    std::string _lqxProgram;
	    unsigned _lqxProgramLineNumber;
	    LQX::Program * _parsedLQXProgram;
      
	    /* Pragmas loaded by the scanner */
	    std::map<std::string,std::string> _loadedPragmas;

	    /* Various flags used by printing */

	    unsigned  _maximumPhase;
	    bool _hasResults;
	    bool _hasRendezvous;
	    bool _hasSendNoReply;
	    bool _hasForwarding;
	    bool _hasMaxServiceTime;
	    bool _hasHistogram;
	    bool _hasSemaphoreWait;
	    bool _hasReaderWait;
	    bool _hasWriterWait;
	    bool _entryHasThinkTime;
	    bool _entryHasNonExponentialPhase;
	    bool _entryHasDeterministicPhase;
	    bool _entryHasOpenArrivals;
	    bool _entryHasThroughputBound;
	    bool _entryHasOpenWait;	
	    bool _entryHasWaitingTimeVariance;
	    bool _entryHasServiceTimeVariance;
	    bool _entryHasDropProbability;
	    bool _taskHasAndJoin;
	    bool _taskHasThinkTime;
	    bool _processorHasRate;
	    
	    /* Solution results from LQNS/LQSim */

	    bool _resultValid;
	    bool _hasConfidenceIntervals;
	    bool _hasBottleneckStrength;
	    unsigned int _resultInvocationNumber;
	    double _resultConvergenceValue;
	    unsigned int _resultIterations;
	    std::string _resultPlatformInformation;
	    std::string _resultSolverInformation;
	    clock_t _resultUserTime;
	    clock_t _resultSysTime;
	    clock_t _resultElapsedTime;

	    struct MVAStatistics {
		MVAStatistics() : submodels(0), core(0), step(0.0), step_squared(0.0), wait(0.0), wait_squared(0.0), faults(0) {}
		unsigned int submodels;
		unsigned long core;
		double step;
		double step_squared;
		double wait;
		double wait_squared;
		unsigned int faults;
	    } _mvaStatistics;

	};

	extern Document* currentDocument;
    }
    
    extern const char* input_file_name;
    extern const char* output_file_name;
}
#endif /* __LQIO_DOM_DOCUMENT__ */
