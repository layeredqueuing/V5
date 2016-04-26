/* -*- c++ -*-
 * model.h	-- Greg Franks
 *
 * $Id: model.h 12079 2014-07-07 12:17:26Z greg $
 */

#ifndef _MODEL_H
#define _MODEL_H

#include "lqn2ps.h"
#include <lqio/input.h>
#include <lqio/filename.h>
#include "vector.h"
#include "layer.h"
#include "point.h"

template <class Type> class Stack;
template <class Type> class Sequence;
class Entity;
class Group;
class Key;
class Label;
class Model;
class Processor;
class Task;
class GroupByShareDefault;

namespace LQIO {
    namespace DOM {
	class Group;
	class Entity;
    }
}

#if HAVE_REGEX_T
extern void add_group( LQIO::DOM::Group* group );
#endif

typedef ostream& (Entity::*entityFunc)( ostream& output ) const;
typedef ostream& (Task::*taskFunc)( ostream& output ) const;
typedef ostream& (Processor::*procFunc)( ostream& output ) const;
typedef ostream& (Task::*taskCallFunc)( ostream& output, callFunc aFunc ) const;
#if defined(__SUNPRO_CC)
typedef unsigned (Model::*modelFunc)() const;
typedef ostream& (Model::*printSXDFunc)( ostream& ) const;
#endif

class Model
{
    friend bool pragma( const string&, const string& );
    friend void set_general( int v, double c, int i, int pr, int ph );
    friend void add_elapsed_time( const char * value );
    friend void add_system_time( const char * value );
    friend void add_user_time( const char * value );
    friend void add_solver_info( const char * value );
    typedef ostream& (*outputFuncPtr)( ostream& );
    typedef bool (Task::*boolTaskFunc)() const;
    
    /* Statistics collected.  Output is ordered by the order here. */

    typedef enum { TOTAL_LAYERS, TOTAL_PROCESSORS, 
		   TOTAL_TASKS, TOTAL_INFINITE_SERVERS, TOTAL_MULTISERVERS, 
		   TOTAL_ENTRIES, 
		   TASKS_PER_LAYER, 
		   ENTRIES_PER_TASK, 
		   PHASES_PER_ENTRY, SERVICE_TIME_PER_PHASE, 
		   RNVS_PER_ENTRY, RNV_RATE_PER_CALL,				/* IN */
		   SNRS_PER_ENTRY, SNR_RATE_PER_CALL,
		   OPEN_ARRIVALS_PER_ENTRY, OPEN_ARRIVAL_RATE_PER_ENTRY,
		   FORWARDING_PER_ENTRY, FORWARDING_PROBABILITY_PER_CALL,	/* OUT */
		   N_STATS } MODEL_STATS;
    typedef enum { REAL_TIME, USER_TIME, SYS_TIME } TIME_VALUES;

public:
    /* default values */
    static unsigned iteration_limit;
    static unsigned print_interval;
    static double convergence_value;
    static double underrelaxation;

    static int maxModelNumber;

    static bool deterministicPhasesPresent;
    static bool maxServiceTimePresent;
    static bool nonExponentialPhasesPresent;
    static bool thinkTimePresent;
    static bool boundsPresent;
    static bool histogramPresent;
    static bool serviceExceededPresent;
    static bool variancePresent;

private:
    static const char * XMLSchema_instance;

protected:
#if !defined(__SUNPRO_CC)
    typedef unsigned (Model::*modelFunc)() const;
    typedef ostream& (Model::*printSXDFunc)( ostream& ) const;
#endif
    static const unsigned CLIENT_LEVEL = 1;
    static const unsigned SERVER_LEVEL = 2;
    static const unsigned PROCESSOR_LEVEL = 3;

    class LayerSequence
    {
    public:
	LayerSequence( const Vector2<Layer> & layers  );
	Entity * operator()() const;
	unsigned size() const;
	const LayerSequence& rewind() const;

    private:
	const Vector2<Layer> & myLayers;
	mutable unsigned layerIndex;
	mutable unsigned entityIndex;
    };

    class Stats
    {
    public:
	Stats();
	ostream& operator<<( ostream& output ) const { return print( output ); }

	Stats & accumulate( double value, const string& );
	Stats & accumulate( const Model *, const string& );
	Stats & accumulate( const modelFunc aFunc ) { myFunc = aFunc; return *this; }
	Stats & name( const string& aName ) { myName = aName; return *this; }
	double sum() const { return x; }
	ostream& print( ostream& ) const;

    private:
	string myName;
	unsigned n;
	double x;
	double x_sqr;
	double log_x;
	double one_x;
	double min;
	double max;
	string min_filename;
	string max_filename;
	modelFunc myFunc;
    };

    class Aggregate
    {
    public:
	Aggregate() {}

	void operator()( Task * task  );
    };

    friend ostream& operator<<( ostream& output, const Model::Stats& self ) { return self.print( output ); }

public:
    Model( LQIO::DOM::Document * document, const string& input_file_name, const string& output_file_name );

    virtual ~Model();
    bool prepare();
    static void free();
#if HAVE_REGEX_T
    static void add_group( const string& );
#endif

private:
    Model( const Model& );		/* Copying is verbotten */
    Model& operator=( const Model& );
    
public:
    const LQIO::DOM::Document * getDOM() const { return _document; }

    Model& setModelNumber( unsigned int n ) { _modelNumber = n; return *this; }

    bool load( const char * );
    bool process();
    bool store();
    bool reload();
    static void aggregate( LQIO::DOM::Document& );
    static double scaling() { return __model->_scaling; }

    Model& accumulateStatistics( const string& fileName );

    ostream& print( ostream& ) const;
    ostream& printStatistics( ostream&, const char * = 0 ) const;
    ostream& printSummary( ostream& ) const;
#if defined(SXD_OUTPUT)
    Model const& printSXD( const char * ) const;
#endif

    static ostream& printEEPICprologue( ostream& output );
    static ostream& printEEPICepilogue( ostream& output );
    static ostream& printPostScriptPrologue( ostream& output, const string&, unsigned left=0, unsigned top=0, unsigned right=612, unsigned bottom=792 );
    static ostream& printOverallStatistics( ostream& );

protected:
    virtual bool generate();
    unsigned topologicalSort();
    virtual Model& layerize() = 0;
    virtual bool selectSubmodel( const unsigned );

    virtual unsigned totalize();
    unsigned nLayers() const { return _numberOfLayers; }
    unsigned nTasks() const { return _taskCount; }
    unsigned nProcessors() const { return _processorCount; }
    unsigned nEntries() const { return _entryCount; }

    double left() const { return myOrigin.x(); }
    double right() const { return myOrigin.x() + myExtent.x(); }
    double top() const { return myOrigin.y() + myExtent.y(); }
    double bottom() const { return myOrigin.y(); }

    virtual Model const& sort( compare_func_ptr ) const;
    Model const& format() const;
    virtual Model const& justify() const;
    Model const& align() const;
    Model const& alignEntities() const;
    Model const& finalScaleTranslate() const;
    Model const& label() const;


private:
    static void group_by_processor();
    static void group_by_share();
    void group_by_submodel();
    Model& relayerize( const unsigned );

    const Model& operator*=( const double s ) const;
    const Model& translateScale( const double s ) const;
    const Model& moveBy( const double, const double ) const;
    bool hasOutputFileName() const { return _outputFileName.size() > 0 && _outputFileName != "-"; }

    bool check();
#if defined(REP2FLAT)
    Model& expandModel();
    Model& removeReplication();
#endif
    Model& rename();
    Model& squishNames();
    Model const& format( Layer& aSubmodel ) const;

    unsigned count( const boolTaskFunc ) const;
    unsigned count( const callFunc ) const;
    unsigned nMultiServers() const;
    unsigned nInfiniteServers() const;

    string getExtension();
    Model const& accumulateTaskStats( const string& ) const;	/* Does not count ref. tasks. */
    Model const& accumulateEntryStats( const string& ) const;	/* Does not count ref. tasks. */
    map<unsigned, LQIO::DOM::Entity *>& remapEntities() const;

    ostream& printEEPIC( ostream& output ) const;
#if defined(EMF_OUTPUT)
    ostream& printEMF( ostream& output ) const;
#endif
    ostream& printFIG( ostream& output ) const;
#if HAVE_LIBGD
    ostream& printGD( ostream& output, outputFuncPtr func ) const;
#endif
    ostream& printPostScript( ostream& output ) const;
#if defined(SVG_OUTPUT)
    ostream& printSVG( ostream& output ) const;
#endif
#if defined(SXD_OUTPUT)
    const Model& printSXD( const char *, const char *, const char *, const printSXDFunc ) const;
    ostream& printSXD( ostream& output ) const;
    ostream& printSXDMeta( ostream& output ) const;
    ostream& printSXDMimeType( ostream& output ) const;
    ostream& printSXDSettings( ostream& output ) const;
    ostream& printSXDStyles( ostream& output ) const;
    ostream& printSXDManifest( ostream& output ) const;
#endif
#if defined(TXT_OUTPUT)
    ostream& printTXT( ostream& output ) const;
#endif
#if defined(X11_OUTPUT)
    ostream& printX11( ostream& output ) const;
#endif
    ostream& printInput( ostream& output ) const;
    ostream& printOutput( ostream& output ) const;
    ostream& printParseable( ostream& output ) const;
    ostream& printRTF( ostream& output ) const;
#if defined(PMIF_OUTPUT)
    ostream& printPMIF( ostream& output ) const;
#endif
    void printXML( ostream& output ) const;

    ostream& printLayers( ostream& ) const;

    static const char * get_userid();

public:
    static unsigned forwardingCount;
    static unsigned openArrivalCount;
    static unsigned rendezvousCount[MAX_PHASES+1];
    static unsigned sendNoReplyCount[MAX_PHASES+1];
    static unsigned phaseCount[MAX_PHASES+1];
    static Cltn<Group *> group;

protected:
    Vector2<Layer> layers;
    Key * myKey;

    mutable Point myOrigin;
    mutable Point myExtent;

    unsigned _taskCount;
    unsigned _processorCount;
    unsigned _entryCount;

private:
    static Model * __model;

    LQIO::DOM::Document * _document;
    const string _inputFileName;
    const string _outputFileName;

    unsigned int _numberOfLayers;
    unsigned int _modelNumber;
    double _scaling;

    static Stats stats[];
};

/* --------------------- Batched Partition Model ---------------------- */

class Batch_Model : virtual public Model
{
public:
    Batch_Model( LQIO::DOM::Document * document, const string& input_file_name, const string& output_file_name ) : Model( document, input_file_name, output_file_name ) {}

protected:
    virtual Model& layerize();
};

class ProcessorTask_Model : virtual public Model, public Batch_Model 
{
public:
    ProcessorTask_Model( LQIO::DOM::Document * document, const string& input_file_name, const string& output_file_name ) : Model( document, input_file_name, output_file_name ), Batch_Model( document, input_file_name, output_file_name ) {}

    virtual Model& layerize() { return Batch_Model::layerize(); }
    virtual Model const& justify() const;

private:
    Model const& justify2( Layer &procLayer, Layer &taskLayer, const double ) const;
};

/* ---------------------- HW/SW Partition Model ----------------------- */

class HWSW_Model : public Model
{
public:
    HWSW_Model( LQIO::DOM::Document * document, const string& input_file_name, const string& output_file_name ) : Model( document, input_file_name, output_file_name ) {}

protected:
    virtual Model& layerize();
};

/* ---------------------- HW/SW Partition Model ----------------------- */

class MOL_Model : public Model
{
public:
    MOL_Model( LQIO::DOM::Document * document, const string& input_file_name, const string& output_file_name ) : Model( document, input_file_name, output_file_name ) {}

protected:
    virtual Model& layerize();
};

/* --------------------------- Group Model ---------------------------- */

class Group_Model : virtual public Model
{
public:
    Group_Model( LQIO::DOM::Document * document, const string& input_file_name, const string& output_file_name ) : Model( document, input_file_name, output_file_name ) {}

protected:
    virtual Model const& justify() const;
};


class BatchProcessor_Model : virtual public Model, public Batch_Model, public Group_Model
{
public:
    BatchProcessor_Model( LQIO::DOM::Document * document, const string& input_file_name, const string& output_file_name ) : Model( document, input_file_name, output_file_name ), Batch_Model( document, input_file_name, output_file_name ), Group_Model( document, input_file_name, output_file_name ) {}

protected:
    virtual Model& layerize() { return Batch_Model::layerize(); }
    virtual Model const& justify() const { return Group_Model::justify(); }
};

class BatchGroup_Model : virtual public Model, public Batch_Model, public Group_Model
{
public:
    BatchGroup_Model( LQIO::DOM::Document * document, const string& input_file_name, const string& output_file_name ) : Model( document, input_file_name, output_file_name ), Batch_Model( document, input_file_name, output_file_name ), Group_Model( document, input_file_name, output_file_name ) {}

protected:
    virtual Model& layerize() { return Batch_Model::layerize(); }
    virtual Model const& justify() const { return Group_Model::justify(); }
};

/* ----------------------- SRVN Partition Model ----------------------- */

class SRVN_Model : virtual public Model, public Batch_Model
{
public:
    SRVN_Model( LQIO::DOM::Document * document, const string& input_file_name, const string& output_file_name ) : Model( document, input_file_name, output_file_name ), Batch_Model( document, input_file_name, output_file_name ) {}

protected:
    virtual bool selectSubmodel( const unsigned );
};

/* -------------------- Squashed Partition Model ---------------------- */

class Squashed_Model : virtual public Model, public Batch_Model
{
public:
    Squashed_Model( LQIO::DOM::Document * document, const string& input_file_name, const string& output_file_name ) : Model( document, input_file_name, output_file_name ), Batch_Model( document, input_file_name, output_file_name ) {}

    virtual bool generate();
    virtual Model const& justify() const;
};

/* ------------------ Strict Client Partition Model ------------------- */

class StrictClient_Model : virtual public Model, public Batch_Model {
    friend class Model;

public:
    StrictClient_Model( LQIO::DOM::Document * document, const string& input_file_name, const string& output_file_name ) : Model( document, input_file_name, output_file_name ), Batch_Model( document, input_file_name, output_file_name ) {}

};


class CommentManip {
public:
    CommentManip( ostream& (*ff)(ostream&, const char *, const string& ), const char * aPrefix, const string& aStr )
	: f(ff), myPrefix(aPrefix), myStr(aStr) {}
private:
    ostream& (*f)( ostream&, const char *, const string& );
    const char * myPrefix;
    const string& myStr;

    friend ostream& operator<<(ostream & os, const CommentManip& m ) 
	{ return m.f(os,m.myPrefix,m.myStr); }
};

inline ostream& operator<<( ostream& output, const Model& self ) { return self.print( output ); }
#endif
