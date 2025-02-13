/* -*- c++ -*-
 * $HeadURL: http://rads-svn.sce.carleton.ca:8080/svn/lqn/trunk/lqns/model.h $
 *
 * Layer-ization of model.
 *
 * Copyright the Real-Time and Distributed Systems Group,
 * Department of Systems and Computer Engineering,
 * Carleton University, Ottawa, Ontario, Canada. K1S 5B6
 *
 * November, 1994
 *
 * $Id: model.h 17529 2025-02-12 14:22:13Z greg $
 *
 * ------------------------------------------------------------------------
 */

#ifndef LQNS_MODEL_H
#define	LQNS_MODEL_H

#include <set>
#include <lqio/dom_document.h>
#include <mva/vector.h>
#include "report.h"

class Call;
class Entity;
class Entry;
class Model;
class MVA;
class Processor;
class Server;
class Submodel;
class Task;
class Group;

/* ----------------------- Abstract Superclass. ----------------------- */

class Model {
public:
    typedef bool (Model::*solve_using)();

private:
    template <class Type> struct lt_replica
    {
	bool operator()(const Type * a, const Type * b) const { return a->name() < b->name() || a->getReplicaNumber() < b->getReplicaNumber(); }
    };

    typedef Model * (*create_func)( const LQIO::DOM::Document *, const std::filesystem::path&, const std::filesystem::path&, LQIO::DOM::Document::OutputFormat );
    
protected:
    class SolveSubmodel {
    public:
	SolveSubmodel( Model& model, bool verbose ) : _model(model), _verbose(verbose) {}
	void operator()( Submodel * );
    private:
	Model& _model;
	const bool _verbose;
    };

protected:
    explicit Model( const LQIO::DOM::Document *, const std::filesystem::path&, const std::filesystem::path&, LQIO::DOM::Document::OutputFormat );

private:
    Model( const Model& );
    Model& operator=( const Model& );

public:
    virtual ~Model();

public:
    static LQIO::DOM::Document* load( const std::filesystem::path& inputFileName, const std::filesystem::path& outputFileName );
    static int solve( solve_using, const std::filesystem::path&, const std::filesystem::path&, LQIO::DOM::Document::OutputFormat );
    void recalculateDynamicValues();

private:
    static bool prepare( const LQIO::DOM::Document* document );
    static Model * create( const LQIO::DOM::Document *, const std::filesystem::path&, const std::filesystem::path&, LQIO::DOM::Document::OutputFormat );
    void setModelParameters();

public:
    bool check();
    bool initialize();

    unsigned nSubmodels() const { return _submodels.size(); }
    static unsigned syncSubmodel() { return __sync_submodel; }
    const Vector<Submodel *>& getSubmodels() const { return _submodels; }

    bool compute();
    bool reload();
    bool restart();

    void sanityCheck();

    void insertDOMResults() const;

    std::ostream& printSubmodelWait( std::ostream& output = std::cout ) const;

protected:
    const LQIO::DOM::Document * getDOM() const { return _document; }

    virtual unsigned assignSubmodel() = 0;
    static unsigned topologicalSort();
    virtual void addToSubmodel() = 0;
    virtual void partition() {}		// Partition disjoin chains.
    void initializeSubmodels();
    void reinitializeSubmodels();

    double convergenceValue() const { return _convergence_value; }	/* Cached */
    unsigned iterationLimit() const { return _iteration_limit; }
    double underrelaxation() const;					/* Cached */
    unsigned printInterval() const { return __print_interval; }

    virtual void backPropogate() {}

    virtual double run() = 0;			/* Solve Model.		*/

    void printIntermediate( const double ) const;
	
private:
    bool hasOutputFileName() const { return !_output_file_name.empty() && _output_file_name != "-"; }

    bool generate( unsigned );	/* Create layers.	*/
    static void extend();
    void configure();
    void reorderOutput() const;

    std::ostream& printOvertaking( std::ostream& ) const;

public:
    static LQIO::DOM::Document::InputFormat __input_format;
    static std::set<Processor *,lt_replica<Processor>> __processor;
    static std::set<Group *,lt_replica<Group>> __group;
    static std::set<Task *,lt_replica<Task>> __task;
    static std::set<Entry *,lt_replica<Entry>> __entry;
    static Processor * __think_server;	/* Delay server for think times	*/
    static unsigned __print_interval;	/* for option processing	*/

protected:
    Vector<Submodel *> _submodels;
    bool _converged;			/* True if converged.		*/
    unsigned long _iterations;		/* Number of Model iterations.	*/
    Vector<MVACount> _MVAStats;		/* MVA statistics by level.	*/

    double _convergence_value;		/* Cached */
    unsigned _iteration_limit;		/* Cached */
    double _underrelaxation;		/* Cached */
    
protected:
    static unsigned __sync_submodel;	/* Level of special sync model. */
    
private:
    unsigned long _step_count;		/* Number of solveLayers	*/
    bool _model_initialized;
    const LQIO::DOM::Document * _document;
    const std::filesystem::path _input_file_name;
    const std::filesystem::path _output_file_name;
    const LQIO::DOM::Document::OutputFormat _output_format;

private:
    static const std::map<const LQIO::DOM::Document::OutputFormat,const std::string> __parseable_output;
};


/* --------------------------- Rolia Model. --------------------------- */

class MOL_Model : public Model {
    friend class Model;		/* Allows use of constructor within class Model */

protected:
    MOL_Model( const LQIO::DOM::Document * document, const std::filesystem::path& inputFileName, const std::filesystem::path& outputFileName, LQIO::DOM::Document::OutputFormat outputFormat ) : Model( document, inputFileName, outputFileName, outputFormat ), _HWSubmodel(0) {}
    
    static Model * create( const LQIO::DOM::Document * document, const std::filesystem::path& inputFileName, const std::filesystem::path& outputFileName, LQIO::DOM::Document::OutputFormat outputFormat ) { return new MOL_Model( document, inputFileName, outputFileName, outputFormat ); }

    virtual unsigned assignSubmodel();
    virtual void addToSubmodel();

    virtual double run();

private:
    unsigned _HWSubmodel;
};


/* ----------- HW SW with Back Propogation Partition Model ------------ */
                        
class BackPropogate_MOL_Model : public MOL_Model {
    friend class Model;		/* Allows use of constructor within class Model */

protected:
    BackPropogate_MOL_Model( const LQIO::DOM::Document * document, const std::filesystem::path& inputFileName, const std::filesystem::path& outputFileName, LQIO::DOM::Document::OutputFormat outputFormat ) : MOL_Model( document, inputFileName, outputFileName, outputFormat ) {}

    static Model * create( const LQIO::DOM::Document * document, const std::filesystem::path& inputFileName, const std::filesystem::path& outputFileName, LQIO::DOM::Document::OutputFormat outputFormat ) { return new BackPropogate_MOL_Model( document, inputFileName, outputFileName, outputFormat ); }

    virtual void backPropogate();
};


/* --------------------- Batched Partition Model ---------------------- */
                        
class Batch_Model :  public Model {
    friend class Model;		/* Allows use of constructor within class Model */

protected:
    Batch_Model( const LQIO::DOM::Document * document, const std::filesystem::path& inputFileName, const std::filesystem::path& outputFileName, LQIO::DOM::Document::OutputFormat outputFormat ) : Model( document, inputFileName, outputFileName, outputFormat ) {}

    static Model * create( const LQIO::DOM::Document * document, const std::filesystem::path& inputFileName, const std::filesystem::path& outputFileName, LQIO::DOM::Document::OutputFormat outputFormat ) { return new Batch_Model( document, inputFileName, outputFileName, outputFormat ); }

    virtual unsigned assignSubmodel();
    virtual void addToSubmodel();
    virtual void partition();
    virtual double run();
};


/* ---------- Batched with Back Propogation Partition Model ----------- */
                        
class BackPropogate_Batch_Model : public Batch_Model {
    friend class Model;		/* Allows use of constructor within class Model */

protected:
    BackPropogate_Batch_Model( const LQIO::DOM::Document * document, const std::filesystem::path& inputFileName, const std::filesystem::path& outputFileName, LQIO::DOM::Document::OutputFormat outputFormat ) : Batch_Model( document, inputFileName, outputFileName, outputFormat ) {}

    static Model * create( const LQIO::DOM::Document * document, const std::filesystem::path& inputFileName, const std::filesystem::path& outputFileName, LQIO::DOM::Document::OutputFormat outputFormat ) { return new BackPropogate_Batch_Model( document, inputFileName, outputFileName, outputFormat ); }

    virtual void backPropogate();
};


/* ---------------------- SRVN Partition Model ------------------------ */
                        
class SRVN_Model : public Batch_Model {
    friend class Model;		/* Allows use of constructor within class Model */

protected:
    SRVN_Model( const LQIO::DOM::Document * document, const std::filesystem::path& inputFileName, const std::filesystem::path& outputFileName, LQIO::DOM::Document::OutputFormat outputFormat ) : Batch_Model( document, inputFileName, outputFileName, outputFormat ) {}

    static Model * create( const LQIO::DOM::Document * document, const std::filesystem::path& inputFileName, const std::filesystem::path& outputFileName, LQIO::DOM::Document::OutputFormat outputFormat ) { return new SRVN_Model( document, inputFileName, outputFileName, outputFormat ); }

    virtual unsigned assignSubmodel();
};

/* -------------------- Squashed Partition Model ---------------------- */
                        
class Squashed_Model : public Batch_Model {
    friend class Model;		/* Allows use of constructor within class Model */

protected:
    Squashed_Model( const LQIO::DOM::Document * document, const std::filesystem::path& inputFileName, const std::filesystem::path& outputFileName, LQIO::DOM::Document::OutputFormat outputFormat ) : Batch_Model( document, inputFileName, outputFileName, outputFormat ) {}

    static Model * create( const LQIO::DOM::Document * document, const std::filesystem::path& inputFileName, const std::filesystem::path& outputFileName, LQIO::DOM::Document::OutputFormat outputFormat ) { return new Squashed_Model( document, inputFileName, outputFileName, outputFormat ); }

    virtual unsigned assignSubmodel();
};

/* ---------------------- HwSw Partition Model ------------------------ */
                        
class HwSw_Model : public Batch_Model {
    friend class Model;		/* Allows use of constructor within class Model */

protected:
    HwSw_Model( const LQIO::DOM::Document * document, const std::filesystem::path& inputFileName, const std::filesystem::path& outputFileName, LQIO::DOM::Document::OutputFormat outputFormat ) : Batch_Model( document, inputFileName, outputFileName, outputFormat ) {}

    static Model * create( const LQIO::DOM::Document * document, const std::filesystem::path& inputFileName, const std::filesystem::path& outputFileName, LQIO::DOM::Document::OutputFormat outputFormat ) { return new HwSw_Model( document, inputFileName, outputFileName, outputFormat ); }

    virtual unsigned assignSubmodel();
};
#endif
