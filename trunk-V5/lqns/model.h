/* -*- c++ -*-
 * $HeadURL$
 *
 * Layer-ization of model.
 *
 * Copyright the Real-Time and Distributed Systems Group,
 * Department of Systems and Computer Engineering,
 * Carleton University, Ottawa, Ontario, Canada. K1S 5B6
 *
 * November, 1994
 *
 * $Id$
 *
 * ------------------------------------------------------------------------
 */

#if	!defined(LAYERIZE_H)
#define	LAYERIZE_H

#include "dim.h"
#include "cltn.h"
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
namespace LQIO {
    namespace DOM {
	class Document;
    }
}

//ostream& operator<<( ostream&, const Model& );

/* ----------------------- Abstract Superclass. ----------------------- */

class Model {
    friend class SolverReport;

public:
    static LQIO::DOM::Document* load( const string& inputFileName, const string& outputFileName );
    static Model * createModel( const LQIO::DOM::Document *, const string&, const string&, bool check_model = true );
    static bool prepare( const LQIO::DOM::Document* document );
    static void recalculateDynamicValues( const LQIO::DOM::Document* document );
    static void setModelParameters( const LQIO::DOM::Document* doc );
    static void dispose();

public:
    virtual ~Model();

protected:
    explicit Model( const LQIO::DOM::Document *, const string&, const string& );

private:
    Model( const Model& );
    Model& operator=( const Model& );

public:
    Model& reinitialize();
    unsigned generate( set<Task *,ltTask>::const_iterator& );	/* Create layers.	*/

    void InitializeModel();

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
    virtual unsigned assignSubmodel( set<Task *,ltTask>::const_iterator& ) = 0;
    static unsigned topologicalSort( set<Task *,ltTask>::const_iterator& );
    virtual void addToSubmodel() = 0;
    unsigned pruneLayers();
    void initialize();
    void initClients();
    void reinitClients();
    double relaxation() const;
    virtual void backPropogate() {}

    virtual double run() = 0;			/* Solve Model.		*/
    Model& solve( Sequence<Submodel *>& );
    Model& solveSubmodel( Submodel * );

    void printIntermediate( const double ) const;
	
private:
    static void resetCounts();
    static bool checkModel();
    static void extendModel();
    static void initProcessors();
    void configure();
    void setInitialized() { _model_initialized = true; }

    unsigned nSubmodels() const { return _submodels.size(); }
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

protected:
    Cltn<Submodel *> _submodels;
    bool _converged;			/* True if converged.		*/
    unsigned long _iterations;		/* Number of Model iterations.	*/
    unsigned _max_depth;		/* Deepest extent in model.	*/
    Vector<MVACount> MVAStats;		/* MVA statistics by level.	*/

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

    virtual unsigned assignSubmodel( set<Task *,ltTask>::const_iterator& );
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

    virtual unsigned assignSubmodel( set<Task *,ltTask>::const_iterator& );
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

    virtual unsigned assignSubmodel( set<Task *,ltTask>::const_iterator& );
};

/* -------------------- Squashed Partition Model ---------------------- */
                        
class Squashed_Model : public Batch_Model {
    friend class Model;		/* Allows use of constructor within class Model */

protected:
    Squashed_Model( const LQIO::DOM::Document * document, const string& inputFileName, const string& outputFileName ) 
	: Batch_Model( document, inputFileName, outputFileName )
	{}

    virtual unsigned assignSubmodel( set<Task *,ltTask>::const_iterator& );
};

/* ---------------------- HwSw Partition Model ------------------------ */
                        
class HwSw_Model : public Batch_Model {
    friend class Model;		/* Allows use of constructor within class Model */

protected:
    HwSw_Model( const LQIO::DOM::Document * document, const string& inputFileName, const string& outputFileName ) 
	: Batch_Model( document, inputFileName, outputFileName )
	{}

    virtual unsigned assignSubmodel( set<Task *,ltTask>::const_iterator& );
};
#endif
