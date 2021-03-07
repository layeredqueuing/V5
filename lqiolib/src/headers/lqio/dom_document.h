/* -*- c++ -*-
 *  $Id: dom_document.h 14523 2021-03-06 22:53:02Z greg $
 *
 *  Created by Martin Mroz on 24/02/09.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#ifndef __LQIO_DOM_DOCUMENT__
#define __LQIO_DOM_DOCUMENT__

#include <sys/time.h>
#include <map>
#include "dom_processor.h"
#include "dom_group.h"
#include "dom_task.h"
#include "dom_entry.h"
#include "dom_activity.h"
#include "dom_actlist.h"
#include "dom_call.h"
#include "dom_extvar.h"
#include "dom_pragma.h"
#include "submodel_info.h"

namespace LQX {
    class Program;
}

namespace LQIO {
    namespace DOM {
	class Document {

	public:
	    typedef enum { DEFAULT_OUTPUT, LQN_OUTPUT, XML_OUTPUT, JSON_OUTPUT, RTF_OUTPUT, PARSEABLE_OUTPUT } output_format;
	    typedef enum { AUTOMATIC_INPUT, LQN_INPUT, XML_INPUT, JSON_INPUT, JMVA_INPUT } input_format;

	private:
	    enum class cached { SET_FALSE, SET_TRUE, NOT_SET };

	private:
	    Document& operator=( const Document& );
	    Document( const Document& );

	    /* Constructors and Destructors */

	public:
	    Document( input_format );
	    virtual ~Document();

	    input_format getInputFormat() const { return _format; }

	    /* STORE: Model Parameter Information */
	    void setModelParameters(const std::string& comment, ExternalVariable* conv_val, ExternalVariable* it_limit, ExternalVariable* print_int, ExternalVariable* underrelax_coeff, const void * arg );
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
	    bool instantiated() const { return _instantiated; }
	    void setInstantiated( bool instantiated ) { _instantiated = instantiated; }
	    
	    /* LQX Pragma Support */
	    void addPragma(const std::string&,const std::string&);
	    void mergePragmas(const std::map<std::string,std::string>&);
	    const std::map<std::string,std::string>& getPragmaList() const;
	    bool hasPragmas() const { return getPragmaList().size() > 0; }
	    const std::string getPragma( const std::string& ) const;
	    void clearPragmaList();

	    /* Model Parameters */
	    const std::string& getExtraComment() const;
	    Document& setExtraComment( const std::string& );
	    const ExternalVariable * getModelComment() const { return get( XComment ); }
	    std::string getModelCommentString() const;
	    Document& setModelComment( ExternalVariable * );
	    Document& setModelCommentString( const std::string& );
	    const ExternalVariable * getModelConvergence() const { return get( XConvergence );	}
	    const double getModelConvergenceValue() const { return getValue( XConvergence ); }
	    Document& setModelConvergence( ExternalVariable * );
	    const ExternalVariable * getModelIterationLimit() const { return get( XIterationLimit); }
	    const unsigned int getModelIterationLimitValue() const { return static_cast<unsigned int>(getValue( XIterationLimit)); }
	    Document& setModelIterationLimit( ExternalVariable * );
	    const ExternalVariable * getModelPrintInterval() const { return get( XPrintInterval); }
	    const unsigned int getModelPrintIntervalValue() const { return static_cast<unsigned int>(getValue( XPrintInterval)); };
	    Document& setModelPrintInterval( ExternalVariable * );
	    const ExternalVariable * getModelUnderrelaxationCoefficient() const { return get( XUnderrelaxationCoefficient); }
	    const double getModelUnderrelaxationCoefficientValue() const { return getValue( XUnderrelaxationCoefficient); }
	    Document& setModelUnderrelaxationCoefficient( ExternalVariable * );
	    const ExternalVariable * getSpexConvergenceIterationLimit() const { return get( XSpexIterationLimit ); }
	    const double getSpexConvergenceIterationLimitValue() const { return getValue( XSpexIterationLimit ); }
	    Document& setSpexConvergenceIterationLimit( ExternalVariable * );
	    const ExternalVariable * getSpexConvergenceUnderrelaxation() const { return get( XSpexUnderrelaxation ); }
	    const double getSpexConvergenceUnderrelaxationValue() const { return getValue( XSpexUnderrelaxation ); }
	    Document& setSpexConvergenceUnderrelaxation( ExternalVariable * );

	    /* Cached values for formatting */
	    const unsigned getNumberOfProcessors() const { return _processors.size(); }
	    const unsigned getNumberOfTasks() const { return _tasks.size(); }
	    const unsigned getNumberOfEntries() const { return _entries.size(); }
	    const unsigned getNumberOfGroups() const { return _groups.size(); }
	    Document& setMaximumPhase( unsigned p ) { _maximumPhase = p > _maximumPhase ? p : _maximumPhase; return *this; }
	    unsigned getMaximumPhase() const { return _maximumPhase; }

	    /* Results */
	    unsigned int getResultInvocationNumber() const { return _resultInvocationNumber; }
	    Document& setResultInvocationNumber( const unsigned int resultInvocationNumber ) { _resultInvocationNumber = resultInvocationNumber; return *this; }
	    double getResultConvergenceValue() const { return _resultConvergenceValue; }
	    Document& setResultConvergenceValue(double resultConvergenceValue);
	    bool getResultValid() const { return _resultValid; }
	    Document& setResultValid(bool resultValid);
	    unsigned int getResultIterations() const { return _resultIterations; }
	    Document& setResultIterations(unsigned int resultIterations);
	    unsigned int getResultNumberOfBlocks() const { return _resultIterations; }
	    Document& setResultHasConfidenceIntervals( const bool hasConfidenceIntervals ) { _hasConfidenceIntervals = hasConfidenceIntervals; return *this; }
	    Document& setResultHasBottleneckStrength( const bool hasBottleneckStrength ) { _hasBottleneckStrength = hasBottleneckStrength; return *this; }
	    const std::string& getResultPlatformInformation() const { return _resultPlatformInformation; }
	    Document& setResultPlatformInformation(const std::string& resultPlatformInformation);
	    const std::string& getResultSolverInformation() const { return _resultSolverInformation; }
	    Document& setResultSolverInformation(const std::string& resultSolverInformation);
	    double getResultElapsedTime() const { return _resultElapsedTime; }
	    Document& setResultElapsedTime(double resultElapsedTime);
	    double getResultSysTime() const { return _resultSysTime; }
	    Document& setResultSysTime(double resultSysTime);
	    double getResultUserTime() const { return _resultUserTime; }
	    Document& setResultUserTime(double resultUserTime);
	    long getResultMaxRSS() const { return _resultMaxRSS; }
	    Document& setResultMaxRSS( long resultMaxRSS );
	    

	    const MVAStatistics& getResultMVAStatistics() const { return _mvaStatistics; }
	    double getResultMVAStep() const { return _mvaStatistics.getNumberOfStep(); }
	    double getResultMVAWait() const { return _mvaStatistics.getNumberOfWait(); }

	    /* Queries */

	    virtual bool hasResults() const;
	    bool hasRendezvous() const;
	    bool hasSendNoReply() const;
	    bool hasForwarding() const;
	    bool hasNonExponentialPhase() const;
	    bool hasDeterministicPhase() const;
	    bool hasMaxServiceTimeExceeded() const;
	    bool hasHistogram() const;
	    bool hasConfidenceIntervals() const { return _hasConfidenceIntervals; }
	    bool hasBottleneckStrength() const { return _hasBottleneckStrength; }
	    bool hasSemaphoreWait() const;
	    bool hasRWLockWait() const;
	    bool hasThinkTime() const;
	    bool hasOpenArrivals() const;
	    bool processorHasRate() const;
	    bool taskHasAndJoin() const;
	    bool taskHasThinkTime() const;

	    bool entryHasThroughputBound() const;
	    bool entryHasOpenWait() const;
	    bool entryHasWaitingTimeVariance() const;
	    bool entryHasDropProbability() const;
	    bool entryHasServiceTimeVariance() const;


	    /* I/O */
	    static input_format getInputFormatFromFilename( const std::string&, const input_format=AUTOMATIC_INPUT );
	    static Document* load(const std::string&, input_format format, unsigned& errorCode, bool load_results );
	    virtual bool loadResults( const std::string&, const std::string&, const std::string&, unsigned& errorCode );
	    std::ostream& print( std::ostream& ouptut, const output_format format=LQN_OUTPUT ) const;
	    std::ostream& printExternalVariables( std::ostream& ouptut ) const;

	    /* Semi-private */

	    static void db_check_set_entry(DOM::Entry* entry, const std::string& toEntryName, DOM::Entry::Type requisiteType = DOM::Entry::Type::NOT_DEFINED );
	    DOM::ExternalVariable* db_build_parameter_variable(const char* input, bool* isSymbol);
	    static void lqx_parser_trace( FILE * );
	    static std::string __input_file_name;

	    static bool __debugXML;
	    static bool __debugJSON;

	private:
	    const double getValue( const char * ) const;
	    const ExternalVariable * get( const char * ) const;
	    static inline bool wasSet(const std::pair<std::string,SymbolExternalVariable*>& var ) { return var.second->wasSet(); }
	    struct notSet {
		notSet(std::vector<std::string>& list) : _list(list) {}
		void operator()( const std::pair<std::string,SymbolExternalVariable*>& var ) { if (!var.second->wasSet()) _list.push_back(var.first); }
	    private:
		std::vector<std::string>& _list;
	    };

	public:
	    /* Names of document attributes */
	    static const char * XComment;
	    static const char * XConvergence;
	    static const char * XIterationLimit;
	    static const char * XPrintInterval;
	    static const char * XUnderrelaxationCoefficient;
	    static const char * XSpexIterationLimit;
	    static const char * XSpexUnderrelaxation;

	private:
	    /* Parameter Information */
	    std::string _extraComment;

	    /* List of Objects */

	    std::map<std::string, Processor*> _processors;    	/* processor.name -> Processor */
	    std::map<std::string, Group*> _groups;            	/* group.name -> Group */
	    std::map<std::string, Task*> _tasks;              	/* task.name -> Task */
	    std::map<std::string, Entry*> _entries;           	/* entry.name -> Entry */
	    std::map<unsigned, Entity*> _entities;            	/* entity.id -> Entity */

	    /* We need to make sure all variables named the same point the same */
	    std::map<std::string, SymbolExternalVariable*> _variables;
	    std::map<const char *, ExternalVariable*> _controlVariables;
	    static std::map<const char *, double> __initialValues;

	    unsigned _nextEntityId;                           	/* for sorting, see _entities 	*/
	    const input_format _format;				/* input format 		*/

	    /* The stored LQX program, if any */
	    std::string _lqxProgram;
	    unsigned _lqxProgramLineNumber;
	    LQX::Program * _parsedLQXProgram;
	    bool _instantiated;

	    /* Pragmas loaded by the scanner */
	    Pragma _pragmas;

	    /* Various flags used by printing */

	    unsigned  _maximumPhase;
	    bool _hasResults;

	    /* Cached values */

	    mutable cached _hasRendezvous;
	    mutable cached _hasSendNoReply;
	    mutable cached _taskHasAndJoin;

	    /* Solution results from LQNS/LQSim */

	    bool _resultValid;
	    bool _hasConfidenceIntervals;
	    bool _hasBottleneckStrength;
	    unsigned int _resultInvocationNumber;
	    double _resultConvergenceValue;
	    unsigned int _resultIterations;
	    std::string _resultPlatformInformation;
	    std::string _resultSolverInformation;
	    double _resultUserTime;
	    double _resultSysTime;
	    double _resultElapsedTime;
	    long _resultMaxRSS;

	    MVAStatistics _mvaStatistics;
	};

	extern Document* __document;
    }
}
#endif /* __LQIO_DOM_DOCUMENT__ */
