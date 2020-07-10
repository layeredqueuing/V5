/* -*- c++ -*-
 * model.h	-- Greg Franks
 *
 * $Id: model.h 13675 2020-07-10 15:29:36Z greg $
 */

#ifndef _MODEL_H
#define _MODEL_H

#include "lqn2ps.h"
#include <lqio/input.h>
#include <lqio/filename.h>
#include <lqio/common_io.h>
#include <vector>
#include "layer.h"
#include "point.h"
#include "element.h"

class Entity;
class Group;
class Key;
class Label;
class Model;
class Processor;
class Task;

namespace LQIO {
    namespace DOM {
	class Group;
	class Entity;
    }
}

#if HAVE_REGEX_T
extern void add_group( LQIO::DOM::Group* group );
#endif

class Model
{
    typedef ostream& (*outputFuncPtr)( ostream& );

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
    static const unsigned CLIENT_LEVEL = 0;
    static const unsigned SERVER_LEVEL = 1;
    static const unsigned PROCESSOR_LEVEL = 2;

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

    struct Count {
	Count() : _tasks(0), _processors(0), _entries(0), _activities(0) {}
	Count& operator=( unsigned int value );
	Count& operator+=( const Count& );
	Count& operator()( const Entity * );

	unsigned tasks() const { return _tasks; }
	unsigned processors() const { return _processors; }
	unsigned entries() const { return _entries; }
	unsigned activities() const { return _activities; }
	
    private:
	unsigned _tasks;
	unsigned _processors;
	unsigned _entries;
	unsigned _activities;
    };

protected:
    typedef unsigned (Model::*modelFunc)() const;
    typedef ostream& (Model::*printSXDFunc)( ostream& ) const;

    struct Remap {
	Remap( std::map<unsigned, LQIO::DOM::Entity *>& entities ) : _entities(entities) {}
	void operator()( const Layer& );
	void operator()( const Entity * );
    private:
	std::map<unsigned, LQIO::DOM::Entity *>& _entities;
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

    friend ostream& operator<<( ostream& output, const Model::Stats& self ) { return self.print( output ); }

public:
    Model( LQIO::DOM::Document * document, const string& input_file_name, const string& output_file_name, unsigned int number_of_layers );

    virtual ~Model();
    static bool prepare( const LQIO::DOM::Document * document );
    static unsigned topologicalSort();
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
    ostream& printPostScriptPrologue( ostream& output, const string&, unsigned left=0, unsigned top=0, unsigned right=612, unsigned bottom=792 ) const;
    static ostream& printOverallStatistics( ostream& );

protected:
    virtual bool generate();
    virtual Model& layerize() = 0;
    virtual bool selectSubmodel( const unsigned );

    virtual unsigned totalize();
    unsigned nLayers() const { return _layers.size(); }
    unsigned nTasks() const { return _total.tasks(); }
    unsigned nProcessors() const { return _total.processors(); }
    unsigned nEntries() const { return _total.entries(); }
    unsigned nActivities() const { return _total.activities(); }

    double left() const { return _origin.x(); }
    double right() const { return _origin.x() + _extent.x(); }
    double top() const { return _origin.y() + _extent.y(); }
    double bottom() const { return _origin.y(); }

    virtual Model& sort( compare_func_ptr );
    Model& format();
    virtual Model & justify();
    Model & align();
    Model & alignEntities();
    Model & finalScaleTranslate();
    Model & label();


private:
    static void group_by_processor();
    static void group_by_share();
    void group_by_submodel();
    Model& relayerize( const unsigned );

    Model& operator*=( const double s );
    Model& translateScale( const double s );
    Model& moveBy( const double, const double );
    bool hasOutputFileName() const { return _outputFileName.size() > 0 && _outputFileName != "-"; }

    bool check() const;
#if defined(REP2FLAT)
    Model& expandModel();
    Model& removeReplication();
    Model& returnReplication();
#endif
    Model& rename();
    Model& squishNames();
    Model const& format( Layer& aSubmodel );

    unsigned count( const taskPredicate ) const;
    unsigned count( const callPredicate ) const;
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
    const Model& printSXD( const std::string&, const std::string&, const char *, const printSXDFunc ) const;
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
    ostream& printXML( ostream& output ) const;
    ostream& printLayers( ostream& ) const;

    static const char * get_userid();

public:
    static unsigned forwardingCount;
    static unsigned openArrivalCount;
    static unsigned rendezvousCount[MAX_PHASES+1];
    static unsigned sendNoReplyCount[MAX_PHASES+1];
    static unsigned phaseCount[MAX_PHASES+1];

protected:
    std::vector<Layer> _layers;
    Key * _key;
    Label * _label;

    mutable Point _origin;
    mutable Point _extent;

    Count _total;

private:
    static Model * __model;

    LQIO::DOM::Document * _document;
    const string _inputFileName;
    const string _outputFileName;
    const LQIO::DOM::GetLogin _login;

    unsigned int _modelNumber;
    double _scaling;

    static Stats stats[];
};

/* --------------------- Batched Partition Model ---------------------- */

class Batch_Model : virtual public Model
{
public:
    Batch_Model( LQIO::DOM::Document * document, const string& input_file_name, const string& output_file_name, unsigned int number_of_layers ) :
	Model( document, input_file_name, output_file_name, number_of_layers ) {}

protected:
    virtual Model& layerize();
};

class ProcessorTask_Model : virtual public Model, public Batch_Model
{
public:
    ProcessorTask_Model( LQIO::DOM::Document * document, const string& input_file_name, const string& output_file_name, unsigned int number_of_layers ) :
	Model( document, input_file_name, output_file_name, number_of_layers ),
	Batch_Model( document, input_file_name, output_file_name, number_of_layers ) {}

    virtual Model& layerize() { return Batch_Model::layerize(); }
    virtual Model& justify();

private:
    Model const& justify2( Layer &procLayer, Layer &taskLayer, const double ) const;
};

/* ---------------------- HW/SW Partition Model ----------------------- */

class HWSW_Model : public Model
{
public:
    HWSW_Model( LQIO::DOM::Document * document, const string& input_file_name, const string& output_file_name, unsigned int number_of_layers ) :
	Model( document, input_file_name, output_file_name, number_of_layers ) {}

protected:
    virtual Model& layerize();
};

/* ---------------------- HW/SW Partition Model ----------------------- */

class MOL_Model : public Model
{
public:
    MOL_Model( LQIO::DOM::Document * document, const string& input_file_name, const string& output_file_name, unsigned int number_of_layers ) :
	Model( document, input_file_name, output_file_name, number_of_layers ) {}

protected:
    virtual Model& layerize();
};

/* --------------------------- Group Model ---------------------------- */

class Group_Model : virtual public Model
{
    struct Justify {
	Justify( size_t max_level ) : _x(0), _max_level(max_level) {}
	unsigned int extent() const { return _x; }
	void operator()( Group * );
    private:
	unsigned int _x;
	const size_t _max_level;
    };
    
public:
    Group_Model( LQIO::DOM::Document * document, const string& input_file_name, const string& output_file_name, unsigned int number_of_layers ) :
	Model( document, input_file_name, output_file_name, number_of_layers ) {}

protected:
    virtual Model& justify();
};


class BatchProcessor_Model : virtual public Model, public Batch_Model, public Group_Model
{
public:
    BatchProcessor_Model( LQIO::DOM::Document * document, const string& input_file_name, const string& output_file_name, unsigned int number_of_layers ) :
	Model( document, input_file_name, output_file_name, number_of_layers ),
	Batch_Model( document, input_file_name, output_file_name, number_of_layers ),
	Group_Model( document, input_file_name, output_file_name, number_of_layers ) {}

protected:
    virtual Model& layerize() { return Batch_Model::layerize(); }
    virtual Model& justify() { return Group_Model::justify(); }
};

class BatchGroup_Model : virtual public Model, public Batch_Model, public Group_Model
{
public:
    BatchGroup_Model( LQIO::DOM::Document * document, const string& input_file_name, const string& output_file_name, unsigned int number_of_layers ) :
	Model( document, input_file_name, output_file_name, number_of_layers ),
	Batch_Model( document, input_file_name, output_file_name, number_of_layers ),
	Group_Model( document, input_file_name, output_file_name, number_of_layers ) {}

protected:
    virtual Model& layerize() { return Batch_Model::layerize(); }
    virtual Model& justify() { return Group_Model::justify(); }
};

/* ----------------------- SRVN Partition Model ----------------------- */

class SRVN_Model : virtual public Model, public Batch_Model
{
public:
    SRVN_Model( LQIO::DOM::Document * document, const string& input_file_name, const string& output_file_name, unsigned int number_of_layers ) :
	Model( document, input_file_name, output_file_name, number_of_layers ),
	Batch_Model( document, input_file_name, output_file_name, number_of_layers ) {}

protected:
    virtual bool selectSubmodel( const unsigned );
};

/* -------------------- Squashed Partition Model ---------------------- */

class Squashed_Model : virtual public Model, public Batch_Model
{
    struct Justify {
	void operator()( Group * );
    };
    
public:
    Squashed_Model( LQIO::DOM::Document * document, const string& input_file_name, const string& output_file_name ) :
	Model( document, input_file_name, output_file_name, PROCESSOR_LEVEL ),
	Batch_Model( document, input_file_name, output_file_name, PROCESSOR_LEVEL ) {}

    virtual bool generate();
    virtual Model& justify();
};

inline ostream& operator<<( ostream& output, const Model& self ) { return self.print( output ); }
#endif
