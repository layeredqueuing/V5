/* -*- c++ -*-
 * $HeadURL: http://rads-svn.sce.carleton.ca:8080/svn/lqn/trunk-V5/lqns/model.h $
 *
 * Layer-ization of model.
 *
 * Copyright the Real-Time and Distributed Systems Group,
 * Department of Systems and Computer Engineering,
 * Carleton University, Ottawa, Ontario, Canada. K1S 5B6
 *
 * November, 1994
 *
 * $Id: model.h 15051 2021-10-07 23:56:46Z greg $
 *
 * ------------------------------------------------------------------------
 */

#if	!defined(LQNS_MODEL_H)
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
private:
    template <class Type> struct lt_replica
    {
	bool operator()(const Type * a, const Type * b) const { return a->name() < b->name() || a->getReplicaNumber() < b->getReplicaNumber(); }
    };
    
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
    explicit Model( const LQIO::DOM::Document *, const std::string&, const std::string& );

private:
    Model( const Model& );
    Model& operator=( const Model& );

public:
    virtual ~Model();

public:
    static LQIO::DOM::Document* load( const std::string& inputFileName, const std::string& outputFileName );
    static bool prepare( const LQIO::DOM::Document* document );
    static void recalculateDynamicValues( const LQIO::DOM::Document* document );
    static void setModelParameters( const LQIO::DOM::Document* doc );
    static Model * create( const LQIO::DOM::Document *, const std::string&, const std::string& );

public:
    bool check();
    bool initialize();

    unsigned nSubmodels() const { return _submodels.size(); }
    static unsigned syncSubmodel() { return __sync_submodel; }
    const Vector<Submodel *>& getSubmodels() const { return _submodels; }

    bool solve();
    bool reload();
    bool restart();

    void sanityCheck();

    void insertDOMResults() const;

    std::ostream& printSubmodelWait( std::ostream& output = std::cout ) const;

protected:
    virtual unsigned assignSubmodel() = 0;
    static unsigned topologicalSort();
    virtual void addToSubmodel() = 0;
    virtual void optimize() {}
    void initStations();
    void reinitStations();

    double relaxation() const;
    virtual void backPropogate() {}

    virtual double run() = 0;			/* Solve Model.		*/

    void printIntermediate( const double ) const;
	
private:
    bool generate( unsigned );	/* Create layers.	*/
    static void extend();
    void configure();

    bool hasOutputFileName() const { return _output_file_name.size() > 0 && _output_file_name != "-"; }
    std::string createDirectory() const;

    std::ostream& printOvertaking( std::ostream& ) const;

public:
    static double __convergence_value;
    static unsigned __iteration_limit;
    static double __underrelaxation;
    static unsigned __print_interval;
    static LQIO::DOM::Document::InputFormat input_format;
    static std::set<Processor *,lt_replica<Processor>> __processor;
    static std::set<Group *,lt_replica<Group>> __group;
    static std::set<Task *,lt_replica<Task>> __task;
    static std::set<Entry *,lt_replica<Entry>> __entry;
    static Processor * __think_server;	/* Delay server for think times	*/

protected:
    static unsigned __sync_submodel;	/* Level of special sync model. */
    
protected:
    Vector<Submodel *> _submodels;
    bool _converged;			/* True if converged.		*/
    unsigned long _iterations;		/* Number of Model iterations.	*/
    Vector<MVACount> _MVAStats;		/* MVA statistics by level.	*/

private:
    unsigned long _step_count;		/* Number of solveLayers	*/
    bool _model_initialized;
    const LQIO::DOM::Document * _document;
    const std::string _input_file_name;
    const std::string _output_file_name;
};


/* --------------------------- Rolia Model. --------------------------- */

class MOL_Model : public Model {
    friend class Model;		/* Allows use of constructor within class Model */

protected:
    MOL_Model( const LQIO::DOM::Document * document, const std::string& inputFileName, const std::string& outputFileName ) : Model( document, inputFileName, outputFileName ), _HWSubmodel(0) {}

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
    BackPropogate_MOL_Model( const LQIO::DOM::Document * document, const std::string& inputFileName, const std::string& outputFileName ) : MOL_Model( document, inputFileName, outputFileName ) {}

    virtual void backPropogate();
};


/* --------------------- Batched Partition Model ---------------------- */
                        
class Batch_Model :  public Model {
    friend class Model;		/* Allows use of constructor within class Model */

protected:
    Batch_Model( const LQIO::DOM::Document * document, const std::string& inputFileName, const std::string& outputFileName ) : Model( document, inputFileName, outputFileName ) {}

    virtual unsigned assignSubmodel();
    virtual void addToSubmodel();
    virtual void optimize();
    virtual double run();
};


/* ---------- Batched with Back Propogation Partition Model ----------- */
                        
class BackPropogate_Batch_Model : public Batch_Model {
    friend class Model;		/* Allows use of constructor within class Model */

protected:
    BackPropogate_Batch_Model( const LQIO::DOM::Document * document, const std::string& inputFileName, const std::string& outputFileName ) : Batch_Model( document, inputFileName, outputFileName ) {}
    virtual void backPropogate();
};


/* ---------------------- SRVN Partition Model ------------------------ */
                        
class SRVN_Model : public Batch_Model {
    friend class Model;		/* Allows use of constructor within class Model */

protected:
    SRVN_Model( const LQIO::DOM::Document * document, const std::string& inputFileName, const std::string& outputFileName ) 
	: Batch_Model( document, inputFileName, outputFileName )
	{}

    virtual unsigned assignSubmodel();
};

/* -------------------- Squashed Partition Model ---------------------- */
                        
class Squashed_Model : public Batch_Model {
    friend class Model;		/* Allows use of constructor within class Model */

protected:
    Squashed_Model( const LQIO::DOM::Document * document, const std::string& inputFileName, const std::string& outputFileName ) 
	: Batch_Model( document, inputFileName, outputFileName )
	{}

    virtual unsigned assignSubmodel();
};

/* ---------------------- HwSw Partition Model ------------------------ */
                        
class HwSw_Model : public Batch_Model {
    friend class Model;		/* Allows use of constructor within class Model */

protected:
    HwSw_Model( const LQIO::DOM::Document * document, const std::string& inputFileName, const std::string& outputFileName ) 
	: Batch_Model( document, inputFileName, outputFileName )
	{}

    virtual unsigned assignSubmodel();
};
#endif
