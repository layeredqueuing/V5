/* -*- c++ -*-
 *  $Id: dom_document.h 17587 2025-11-12 20:26:46Z greg $
 *
 *  Created by Martin Mroz on 24/02/09.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#ifndef __LQIO_DOM_DOCUMENT__
#define __LQIO_DOM_DOCUMENT__

#include <map>
#include <string>
#include <filesystem>
#include "dom_pragma.h"
#include "dom_entry.h"
#include "dom_extvar.h"
#include "submodel_info.h"

namespace LQX {
    class Program;
}

extern "C" int srvnlineno;				/* Input line number -- can't use namespace because it's used with C */

namespace LQIO {

    extern struct lqio_params_stats
    {
	lqio_params_stats( const char * version, void (*action)(error_severity) );
	void reset() { error_count = 0; }
	bool anError() const { return error_count > 0; }
	const char * toolname() const { return lq_toolname.c_str(); }
	void init( const std::string& version, const std::string& toolname, void (*sa)(error_severity) );

	std::string lq_toolname;		/* I:Name of tool for messages	  */
	std::string lq_version;			/* I: version number		  */
	std::string lq_command_line;		/* I:Command line		  */
	void (*severity_action)(LQIO::error_severity);	/* I:Severity action		  */

	unsigned max_error;			/* I:Maximum error ID number	  */
	mutable unsigned error_count;		/* IO:Number of errors		  */
	LQIO::error_severity severity_level;	/* I:Messages < severity_level ignored. */
    } io_vars;

    namespace DOM {
	class Activity;
	class ActivityList;
	class Call;
	class Entity;
	class Group;
	class Processor;
	class Task;
	
	class Document {

	public:
	    enum class OutputFormat { DEFAULT, LQN, XML, JABA, JMVA, JSON, RTF, PARSEABLE, QNAP2, TXT };
	    enum class InputFormat  { AUTOMATIC, LQN, XML, JABA, JMVA, JSON, QNAP2 };

	private:
	    Document& operator=( const Document& ) = delete;
	    Document( const Document& ) = delete;

	    /* Constructors and Destructors */

	public:
	    Document( InputFormat );
	    virtual ~Document();

	    InputFormat getInputFormat() const { return _format; }

	    /* STORE: Model Parameter Information */
	    void setModelParameters(const std::string& comment, ExternalVariable* conv_val, ExternalVariable* it_limit, ExternalVariable* print_int, ExternalVariable* underrelax_coeff, const void * arg );

	    /* Handling Entries, Processors, Tasks, Phases and Groups */
	    void addProcessorEntity(Processor* processor);
	    Processor* getProcessorByName(const std::string& name) const;
	    void addTaskEntity(Task* task);
	    Task* getTaskByName(const std::string& name) const;
	    void addEntry(Entry* entry);
	    Entry* getEntryByName(const std::string& name) const;
	    void addGroup(Group* group);
	    Group* getGroupByName(const std::string& name) const;

	    const std::map<std::string,Processor*>& getProcessors() const;	// For output functions.
	    const std::map<std::string,Group*>& getGroups() const;	// For output functions.
	    const std::map<std::string,Task*>& getTasks() const;	// For output functions.
	    const std::map<std::string,Entry*>& getEntries() const;	//
	    const std::vector<Entity *>& getEntities() const;		// For remapping by lqn2ps.

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
	    bool hasLQXProgram() const { return _parsedLQXProgram != nullptr; }
	    void setLQXProgramLineNumber( const unsigned n ) { _lqxProgramLineNumber = n; }
	    const unsigned getLQXProgramLineNumber() const { return _lqxProgramLineNumber; }
	    void registerExternalSymbolsWithProgram(LQX::Program* program);
	    bool instantiated() const { return _instantiated; }
	    void setInstantiated( bool instantiated ) { _instantiated = instantiated; }
	
	    /* LQX Pragma Support */
	    void addPragma(const std::string&,const std::string&);
	    void mergePragmas(const std::map<std::string,std::string>&);
	    const std::map<std::string,std::string>& getPragmaList() const;
	    bool hasPragma(const std::string&) const;
	    const std::string& getPragma( const std::string& ) const;
	    void clearPragmaList();

	    /* Model Parameters */
	    const std::string& getModelName() const { return _modelName; }
	    Document& setModelName( const std::string& name ) { _modelName = name; return *this; }
	    const std::string& getModelDescription() const { return _modelDescription; }
	    Document& setModelDescription();
	    Document& setModelDescription( const std::string& resultDescription );
	    const std::string& getModelComment() const;
	    Document& setModelComment( const std::string& );
	    const ExternalVariable * getModelConvergence() const { return get( XConvergence );	}
	    const double getModelConvergenceValue() const { return getValue( XConvergence ); }
	    Document& setModelConvergence( const ExternalVariable * var ) { return set( XConvergence, var ); }
	    const ExternalVariable * getModelIterationLimit() const { return get( XIterationLimit); }
	    const unsigned int getModelIterationLimitValue() const { return static_cast<unsigned int>(getValue( XIterationLimit )); }
	    Document& setModelIterationLimit( const ExternalVariable * var ) { return set( XIterationLimit, var ); }
	    const ExternalVariable * getModelPrintInterval() const { return get( XPrintInterval); }
	    const unsigned int getModelPrintIntervalValue() const { return static_cast<unsigned int>(getValue( XPrintInterval )); }
	    Document& setModelPrintInterval( const ExternalVariable * var ) { return set( XPrintInterval, var ); }
	    const ExternalVariable * getModelUnderrelaxationCoefficient() const { return get( XUnderrelaxationCoefficient); }
	    const double getModelUnderrelaxationCoefficientValue() const { return getValue( XUnderrelaxationCoefficient ); }
	    Document& setModelUnderrelaxationCoefficient( const ExternalVariable * var ) { return set( XUnderrelaxationCoefficient, var ); }
	    const ExternalVariable * getSpexIterationLimit() const { return get( XSpexIterationLimit ); }
	    const double getSpexIterationLimitValue() const { return getValue( XSpexIterationLimit ); }
	    Document& setSpexIterationLimit( const ExternalVariable * var ) { return set( XSpexIterationLimit, var ); }
	    const ExternalVariable * getSpexUnderrelaxation() const { return get( XSpexUnderrelaxation ); }
	    const double getSpexUnderrelaxationValue() const { return getValue( XSpexUnderrelaxation ); }
	    Document& setSpexUnderrelaxation( const ExternalVariable * var ) { return set( XSpexUnderrelaxation, var ); }
	    const ExternalVariable * getSpexConvergence() const { return get( XSpexConvergence ); }
	    const double getSpexConvergenceValue() const { return getValue( XSpexConvergence ); }
	    Document& setSpexConvergence( const ExternalVariable * var ) { return set( XSpexConvergence, var ); }

	    /* Cached values for formatting */
	    const unsigned getNumberOfProcessors() const { return _processors.size(); }
	    const unsigned getNumberOfTasks() const { return _tasks.size(); }
	    const unsigned getNumberOfEntries() const { return _entries.size(); }
	    const unsigned getNumberOfGroups() const { return _groups.size(); }
	    Document& setMaximumPhase( unsigned p ) { _maximumPhase = p > _maximumPhase ? p : _maximumPhase; return *this; }
	    unsigned getMaximumPhase() const { return _maximumPhase; }

	    /* Results */
	    Document& setMVAStatistics( const unsigned int, const unsigned long, const double, const double, const double, const double, const unsigned int );
	    bool getResultValid() const { return _resultValid; }
	    Document& setResultValid(bool resultValid);
	    unsigned int getResultInvocationNumber() const { return _resultInvocationNumber; }
	    Document& setResultInvocationNumber( const unsigned int resultInvocationNumber ) { _resultInvocationNumber = resultInvocationNumber; return *this; }
	    double getResultConvergenceValue() const { return _resultConvergenceValue; }
	    Document& setResultConvergenceValue(double resultConvergenceValue);
	    unsigned int getResultIterations() const { return _resultIterations; }
	    Document& setResultIterations(unsigned int resultIterations);
	    unsigned int getResultNumberOfBlocks() const { return _resultIterations; }
	    Document& setResultHasConfidenceIntervals( const bool hasConfidenceIntervals ) { _hasConfidenceIntervals = hasConfidenceIntervals; return *this; }
	    Document& setResultHasBottleneckStrength( const bool hasBottleneckStrength ) { _hasBottleneckStrength = hasBottleneckStrength; return *this; }
	    const std::string& getResultPlatformInformation() const { return _resultPlatformInformation; }
	    Document& setResultPlatformInformation();
	    Document& setResultPlatformInformation(const std::string& resultPlatformInformation);
	    const std::string& getResultSolverInformation() const { return _resultSolverInformation; }
	    Document& setResultSolverInformation();
	    Document& setResultSolverInformation(const std::string& resultSolverInformation );
	    const std::string& getResultComment() const;
	    Document& setResultComment( const std::string& );
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
	    static InputFormat getInputFormatFromFilename( const std::filesystem::path&, const InputFormat=InputFormat::AUTOMATIC );
	    static Document* load(const std::filesystem::path&, InputFormat format, unsigned& errorCode, bool load_results );
	    virtual bool loadResults( const std::filesystem::path& directory_name, const std::filesystem::path& file_name, const std::string& suffix, OutputFormat format, unsigned& errorCode );
	    void print( const std::filesystem::path& output_file_name, const std::string& suffix, OutputFormat format, bool rtf_output ) const;
	    void print( const std::filesystem::path& output_file_name, const std::string& suffix, OutputFormat format, bool rtf_output, unsigned int iteration ) const;
	    std::ostream& print( std::ostream& ouptut, const OutputFormat format=OutputFormat::LQN ) const;
	    std::ostream& printExternalVariables( std::ostream& ouptut ) const;

	    /* Semi-private */

	    static void db_check_set_entry(DOM::Entry* entry, DOM::Entry::Type requisiteType = DOM::Entry::Type::NOT_DEFINED );
	    ExternalVariable* db_build_parameter_variable(const std::string& input, bool* isSymbol);
	    static void lqx_parser_trace( FILE * );
	    static std::filesystem::path __input_file_name;

	    static bool __debugXML;
	    static bool __debugJSON;

	private:
	    const double getValue( const std::string& ) const;
	    Document& set( const std::string&, const ExternalVariable * );
	    const ExternalVariable * get( const std::string& ) const;
	    static inline bool wasSet(const std::pair<std::string,SymbolExternalVariable*>& var ) { return var.second->wasSet(); }

	public:
	    /* Names of document attributes */
	    static const char * XComment;
	    static const char * XConvergence;
	    static const char * XIterationLimit;
	    static const char * XPrintInterval;
	    static const char * XSpexConvergence;
	    static const char * XSpexIterationLimit;
	    static const char * XSpexUnderrelaxation;
	    static const char * XUnderrelaxationCoefficient;

	    /* Input/Output extensions */
	    static const std::map<const LQIO::DOM::Document::OutputFormat,const std::filesystem::path> __output_extensions;
	    static const std::map<const std::string,const LQIO::DOM::Document::InputFormat> __extensions_input;

	private:
	    /* Parameter Information */
	    std::string _modelName;
	    std::string _modelDescription;
	    std::string _modelComment;

	    /* List of Objects */

	    std::map<std::string, Processor*> _processors;	/* processor.name -> Processor */
	    std::map<std::string, Group*> _groups;		/* group.name -> Group */
	    std::map<std::string, Task*> _tasks;		/* task.name -> Task */
	    std::map<std::string, Entry*> _entries;		/* entry.name -> Entry */
	    std::vector<Entity*> _entities;			/* Entities by input order */

	    /* We need to make sure all variables named the same point the same */
	    std::map<const std::string, SymbolExternalVariable*> _variables;
	    std::map<const std::string, const ExternalVariable*> _controlVariables;
	    static std::map<const std::string, const double> __initialValues;

	    const InputFormat _format;				/* input format			*/

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

	    /* Solution results from LQNS/LQSim */

	    bool _resultValid;
	    bool _hasConfidenceIntervals;
	    bool _hasBottleneckStrength;
	    unsigned int _resultInvocationNumber;
	    double _resultConvergenceValue;
	    unsigned int _resultIterations;
	    std::string _resultPlatformInformation;
	    std::string _resultSolverInformation;
	    std::string _resultComment;
	    double _resultUserTime;
	    double _resultSysTime;
	    double _resultElapsedTime;
	    long _resultMaxRSS;

	    MVAStatistics _mvaStatistics;
	    static const std::map<const LQIO::DOM::Document::InputFormat,const LQIO::DOM::Document::OutputFormat> __input_to_output_format;
	};

	extern Document* __document;
    }
}
#endif /* __LQIO_DOM_DOCUMENT__ */
