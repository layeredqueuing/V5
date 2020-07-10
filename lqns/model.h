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
 * $Id: model.h 13676 2020-07-10 15:46:20Z greg $
 *
 * ------------------------------------------------------------------------
 */

#if	!defined(LAYERIZE_H)
#define	LAYERIZE_H

#include "dim.h"
#include <set>
#include "vector.h"
#include "entity.h"
#include "group.h"
#include "report.h"
#include <lqio/dom_document.h>

class Call;
class Entity;
class Entry;
class Model;
class MVA;
class Processor;
class Server;
class Submodel;
class Group;

/* ----------------------- Abstract Superclass. ----------------------- */

class Model {
protected:

    class SolveSubmodel {
    public:
	SolveSubmodel( Model& self, bool verbose ) : _self(self), _verbose(verbose) {}

	void operator()( Submodel * );

    private:
	Model& _self;
	const bool _verbose;
    };

    /*
     * Compare to tasks by their name.  Used by the set class to insert items
     */

    template <class Type> struct LT
    {
	bool operator()(const Type * a, const Type * b) const { return a->name() < b->name(); }
    };

public:
    static LQIO::DOM::Document* load( const string& inputFileName, const string& outputFileName );
    static Model * createModel( const LQIO::DOM::Document *, const string&, const string&, bool check_model = true );
    static bool prepare( const LQIO::DOM::Document* document );
    static void recalculateDynamicValues( const LQIO::DOM::Document* document );
    static void setModelParameters( const LQIO::DOM::Document* doc );

public:
    virtual ~Model();

protected:
    explicit Model( const LQIO::DOM::Document *, const string&, const string& );

private:
    Model( const Model& );
    Model& operator=( const Model& );

public:
    Model& reinitialize();
    unsigned generate();	/* Create layers.	*/
    unsigned nSubmodels() const { return _submodels.size(); }
    bool initializeModel();

    void updateWait( Entity * ) const;

    unsigned syncModelNumber() const { return sync_submodel; }

    bool solve();
    bool reload();
    bool restart();

    void sanityCheck();

    void insertDOMResults() const;

    ostream& printLayers( ostream& ) const;
    ostream& printSubmodelWait( ostream& output = cout ) const;

protected:
    virtual unsigned assignSubmodel() = 0;
    static unsigned topologicalSort();
    virtual void addToSubmodel() = 0;
    void initialize();
    void initClients();
    void reinitClients();
    double relaxation() const;
    virtual void backPropogate() {}

    virtual double run() = 0;			/* Solve Model.		*/

    void printIntermediate( const double ) const;
	
private:
    static bool checkModel();
    static void extendModel();
    static void initProcessors();
    void configure();
    void setInitialized() { _model_initialized = true; }

    bool hasOutputFileName() const { return _output_file_name.size() > 0 && _output_file_name != "-"; }
    string createDirectory() const;

    ostream& printOvertaking( ostream& ) const;

public:
    static unsigned sync_submodel;	/* Level of special sync model. */
    static Processor * thinkServer;

    static double convergence_value;
    static unsigned iteration_limit;
    static double underrelaxation;
    static unsigned print_interval;
    static LQIO::DOM::Document::input_format input_format;
    static std::set<Processor *, LT<Entity> > __processor;
    static std::set<Group *, LT<Group> > __group;
    static std::set<Task *,LT<Entity> > __task;
    static std::set<Entry *,LT<Entry> > __entry;
    
protected:
    Vector<Submodel *> _submodels;
    bool _converged;			/* True if converged.		*/
    unsigned long _iterations;		/* Number of Model iterations.	*/
    Vector<MVACount> _MVAStats;		/* MVA statistics by level.	*/

private:
    unsigned long _step_count;		/* Number of solveLayers	*/
    bool _model_initialized;
    const LQIO::DOM::Document * _document;
    string _input_file_name;
    string _output_file_name;
};


/* --------------------------- Rolia Model. --------------------------- */

class MOL_Model : public Model {
    friend class Model;		/* Allows use of constructor within class Model */

protected:
    MOL_Model( const LQIO::DOM::Document * document, const string& inputFileName, const string& outputFileName ) : Model( document, inputFileName, outputFileName ), HWSubmodel(0) {}

    virtual unsigned assignSubmodel();
    virtual void addToSubmodel();

    virtual double run();

protected:
    unsigned HWSubmodel;
};


/* ----------- HW SW with Back Propogation Partition Model ------------ */
                        
class BackPropogate_MOL_Model : public MOL_Model {
    friend class Model;		/* Allows use of constructor within class Model */

protected:
    BackPropogate_MOL_Model( const LQIO::DOM::Document * document, const string& inputFileName, const string& outputFileName ) : MOL_Model( document, inputFileName, outputFileName ) {}

    virtual void backPropogate();
};


/* --------------------- Batched Partition Model ---------------------- */
                        
class Batch_Model :  public Model {
    friend class Model;		/* Allows use of constructor within class Model */

protected:
    Batch_Model( const LQIO::DOM::Document * document, const string& inputFileName, const string& outputFileName ) : Model( document, inputFileName, outputFileName ) {}

    virtual unsigned assignSubmodel();
    virtual void addToSubmodel();
    virtual double run();
};


/* ---------- Batched with Back Propogation Partition Model ----------- */
                        
class BackPropogate_Batch_Model : public Batch_Model {
    friend class Model;		/* Allows use of constructor within class Model */

protected:
    BackPropogate_Batch_Model( const LQIO::DOM::Document * document, const string& inputFileName, const string& outputFileName ) : Batch_Model( document, inputFileName, outputFileName ) {}
    virtual void backPropogate();
};


/* ---------------------- SRVN Partition Model ------------------------ */
                        
class SRVN_Model : public Batch_Model {
    friend class Model;		/* Allows use of constructor within class Model */

protected:
    SRVN_Model( const LQIO::DOM::Document * document, const string& inputFileName, const string& outputFileName ) 
	: Batch_Model( document, inputFileName, outputFileName )
	{}

    virtual unsigned assignSubmodel();
};

/* -------------------- Squashed Partition Model ---------------------- */
                        
class Squashed_Model : public Batch_Model {
    friend class Model;		/* Allows use of constructor within class Model */

protected:
    Squashed_Model( const LQIO::DOM::Document * document, const string& inputFileName, const string& outputFileName ) 
	: Batch_Model( document, inputFileName, outputFileName )
	{}

    virtual unsigned assignSubmodel();
};

/* ---------------------- HwSw Partition Model ------------------------ */
                        
class HwSw_Model : public Batch_Model {
    friend class Model;		/* Allows use of constructor within class Model */

protected:
    HwSw_Model( const LQIO::DOM::Document * document, const string& inputFileName, const string& outputFileName ) 
	: Batch_Model( document, inputFileName, outputFileName )
	{}

    virtual unsigned assignSubmodel();
};
#endif
