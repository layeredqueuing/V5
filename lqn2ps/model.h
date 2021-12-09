/* -*- c++ -*-
 * model.h	-- Greg Franks
 *
 * $Id: model.h 15184 2021-12-09 20:22:28Z greg $
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
	class ExternalVariable;
    }
}

class Model
{
    typedef std::ostream& (*outputFuncPtr)( std::ostream& );
    typedef Model * (*create_func)( LQIO::DOM::Document * document, const std::string& input_file_name, const std::string& output_file_name, unsigned int number_of_layers );


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
public:
    static const unsigned CLIENT_LEVEL = 0;
    static const unsigned SERVER_LEVEL = 1;
    static const unsigned PROCESSOR_LEVEL = 2;

public:
    /* default values */
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
    typedef std::ostream& (Model::*printSXDFunc)( std::ostream& ) const;

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
	std::ostream& operator<<( std::ostream& output ) const { return print( output ); }

	Stats & accumulate( double value, const std::string& );
	Stats & accumulate( const Model *, const std::string& );
	Stats & accumulate( const modelFunc func ) { f = func; return *this; }
	Stats & name( const std::string& aName ) { _name = aName; return *this; }
	double sum() const { return x; }
	std::ostream& print( std::ostream& ) const;

    private:
	std::string _name;
	unsigned n;
	double x;
	double x_sqr;
	double log_x;
	double one_x;
	double min;
	double max;
	std::string min_filename;
	std::string max_filename;
	modelFunc f;
    };

    friend std::ostream& operator<<( std::ostream& output, const Model::Stats& self ) { return self.print( output ); }

public:
    Model( LQIO::DOM::Document * document, const std::string& input_file_name, const std::string& output_file_name, unsigned int number_of_layers );

    virtual ~Model();
    static bool prepare( const LQIO::DOM::Document * document );
#if BUG_270
    static bool prune();
#endif
    static unsigned topologicalSort();
    static void add_group( const std::string& );

private:
    Model( const Model& );		/* Copying is verbotten */
    Model& operator=( const Model& );

public:
    const LQIO::DOM::Document * getDOM() const { return _document; }

    Model& setModelNumber( unsigned int n ) { _modelNumber = n; return *this; }

    static void create( const std::string& inputFileName,  const std::string& output_file_name, const std::string& parse_file_name, int model_no );
    bool load( const char * );
    bool process();
    bool store();
    bool reload();
    static double scaling() { return __model->_scaling; }

    Model& accumulateStatistics( const std::string& fileName );

    std::ostream& print( std::ostream& ) const;
    std::ostream& printStatistics( std::ostream&, const char * = 0 ) const;
    std::ostream& printSummary( std::ostream& ) const;
#if defined(SXD_OUTPUT)
    Model const& printSXD( const char * ) const;
#endif

    static std::ostream& printEEPICprologue( std::ostream& output );
    static std::ostream& printEEPICepilogue( std::ostream& output );
    std::ostream& printPostScriptPrologue( std::ostream& output, const std::string&, unsigned left=0, unsigned top=0, unsigned right=612, unsigned bottom=792 ) const;
    static std::ostream& printOverallStatistics( std::ostream& );

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
    Model& expand();
    Model& removeReplication();
    Model& returnReplication();
#endif
    Model& rename();
    Model& squish();
    Model const& format( Layer& aSubmodel );

    unsigned count( const taskPredicate ) const;
    unsigned count( const callPredicate ) const;
    unsigned nMultiServers() const;
    unsigned nInfiniteServers() const;

    std::string getExtension();
    Model const& accumulateTaskStats( const std::string& ) const;	/* Does not count ref. tasks. */
    Model const& accumulateEntryStats( const std::string& ) const;	/* Does not count ref. tasks. */
    std::map<unsigned, LQIO::DOM::Entity *>& remapEntities() const;

#if JMVA_OUTPUT || QNAP2_OUTPUT
    std::ostream& printBCMP( std::ostream& output ) const;
#endif
    std::ostream& printEEPIC( std::ostream& output ) const;
#if EMF_OUTPUT
    std::ostream& printEMF( std::ostream& output ) const;
#endif
    std::ostream& printFIG( std::ostream& output ) const;
#if HAVE_LIBGD
    std::ostream& printGD( std::ostream& output, outputFuncPtr func ) const;
#if HAVE_GDIMAGEGIFPTR
    std::ostream& printGIF( std::ostream& output ) const;
#endif
#if HAVE_LIBJPEG
    std::ostream& printJPG( std::ostream& output ) const;
#endif
#if HAVE_LIBPNG
    std::ostream& printPNG( std::ostream& output ) const;
#endif
#endif
    std::ostream& printPostScript( std::ostream& output ) const;
#if defined(SVG_OUTPUT)
    std::ostream& printSVG( std::ostream& output ) const;
#endif
#if defined(SXD_OUTPUT)
    const Model& printSXD( const std::string&, const std::string&, const char *, const printSXDFunc ) const;
    std::ostream& printSXD( std::ostream& output ) const;
    std::ostream& printSXDMeta( std::ostream& output ) const;
    std::ostream& printSXDMimeType( std::ostream& output ) const;
    std::ostream& printSXDSettings( std::ostream& output ) const;
    std::ostream& printSXDStyles( std::ostream& output ) const;
    std::ostream& printSXDManifest( std::ostream& output ) const;
#endif
#if defined(TXT_OUTPUT)
    std::ostream& printTXT( std::ostream& output ) const;
#endif
#if defined(X11_OUTPUT)
    std::ostream& printX11( std::ostream& output ) const;
#endif
    std::ostream& printInput( std::ostream& output ) const;
    std::ostream& printOutput( std::ostream& output ) const;
    std::ostream& printNOP( std::ostream& output ) const { return output; }
    std::ostream& printParseable( std::ostream& output ) const;
    std::ostream& printRTF( std::ostream& output ) const;
    std::ostream& printJSON( std::ostream& output ) const;
    std::ostream& printLQX( std::ostream& output ) const;
    std::ostream& printXML( std::ostream& output ) const;

    std::ostream& printLayers( std::ostream& ) const;

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
    static Stats stats[];

    LQIO::DOM::Document * _document;
    const std::string _inputFileName;
    const std::string _outputFileName;
    const LQIO::DOM::GetLogin _login;

    unsigned int _modelNumber;
    double _scaling;

public:
#if BUG_270
    static std::vector<Entity *> __zombies;	/* transmorgrify	*/
#endif
};

/* --------------------- Batched Partition Model ---------------------- */

class Batch_Model : virtual public Model
{
protected:
    Batch_Model( LQIO::DOM::Document * document, const std::string& input_file_name, const std::string& output_file_name, unsigned int number_of_layers ) :
	Model( document, input_file_name, output_file_name, number_of_layers ) {}

public:
    static Model * create( LQIO::DOM::Document * document, const std::string& input_file_name, const std::string& output_file_name, unsigned int number_of_layers )
	{ return new Batch_Model( document, input_file_name, output_file_name, number_of_layers ); }

protected:
    virtual Model& layerize();
};

class ProcessorTask_Model : virtual public Model, public Batch_Model
{
private:
    ProcessorTask_Model( LQIO::DOM::Document * document, const std::string& input_file_name, const std::string& output_file_name, unsigned int number_of_layers ) :
	Model( document, input_file_name, output_file_name, number_of_layers ),
	Batch_Model( document, input_file_name, output_file_name, number_of_layers ) {}

public:
    static Model * create( LQIO::DOM::Document * document, const std::string& input_file_name, const std::string& output_file_name, unsigned int number_of_layers )
	{ return new ProcessorTask_Model( document, input_file_name, output_file_name, number_of_layers ); }

    virtual Model& layerize() { return Batch_Model::layerize(); }
    virtual Model& justify();

private:
    Model const& justify2( Layer &procLayer, Layer &taskLayer, const double ) const;
};

/* ---------------------- HW/SW Partition Model ----------------------- */

class HWSW_Model : public Model
{
private:
    HWSW_Model( LQIO::DOM::Document * document, const std::string& input_file_name, const std::string& output_file_name, unsigned int number_of_layers ) :
	Model( document, input_file_name, output_file_name, number_of_layers ) {}

public:
    static Model * create( LQIO::DOM::Document * document, const std::string& input_file_name, const std::string& output_file_name, unsigned int number_of_layers )
	{ return new HWSW_Model( document, input_file_name, output_file_name, number_of_layers ); }


protected:
    virtual Model& layerize();
};

/* ---------------------- HW/SW Partition Model ----------------------- */

class MOL_Model : public Model
{
private:
    MOL_Model( LQIO::DOM::Document * document, const std::string& input_file_name, const std::string& output_file_name, unsigned int number_of_layers ) :
	Model( document, input_file_name, output_file_name, number_of_layers ) {}

public:
    static Model * create( LQIO::DOM::Document * document, const std::string& input_file_name, const std::string& output_file_name, unsigned int number_of_layers )
	{ return new MOL_Model( document, input_file_name, output_file_name, number_of_layers ); }

protected:
    virtual Model& layerize();
};

/* --------------------------- Group Model ---------------------------- */

/* abstract */ class Group_Model : virtual public Model
{
    struct Justify {
	Justify( size_t max_level ) : _x(0), _max_level(max_level) {}
	unsigned int extent() const { return _x; }
	void operator()( Group * );
    private:
	unsigned int _x;
	const size_t _max_level;
    };
    
protected:
    Group_Model( LQIO::DOM::Document * document, const std::string& input_file_name, const std::string& output_file_name, unsigned int number_of_layers ) :
	Model( document, input_file_name, output_file_name, number_of_layers ) {}

protected:
    virtual Model& justify();
};


class BatchProcessor_Model : virtual public Model, public Batch_Model, public Group_Model
{
private:
    BatchProcessor_Model( LQIO::DOM::Document * document, const std::string& input_file_name, const std::string& output_file_name, unsigned int number_of_layers ) :
	Model( document, input_file_name, output_file_name, number_of_layers ),
	Batch_Model( document, input_file_name, output_file_name, number_of_layers ),
	Group_Model( document, input_file_name, output_file_name, number_of_layers ) {}

public:
    static Model * create( LQIO::DOM::Document * document, const std::string& input_file_name, const std::string& output_file_name, unsigned int number_of_layers )
	{ return new BatchProcessor_Model( document, input_file_name, output_file_name, number_of_layers ); }

protected:
    virtual Model& layerize() { return Batch_Model::layerize(); }
    virtual Model& justify() { return Group_Model::justify(); }
};

class BatchGroup_Model : virtual public Model, public Batch_Model, public Group_Model
{
private:
    BatchGroup_Model( LQIO::DOM::Document * document, const std::string& input_file_name, const std::string& output_file_name, unsigned int number_of_layers ) :
	Model( document, input_file_name, output_file_name, number_of_layers ),
	Batch_Model( document, input_file_name, output_file_name, number_of_layers ),
	Group_Model( document, input_file_name, output_file_name, number_of_layers ) {}

public:
    static Model * create( LQIO::DOM::Document * document, const std::string& input_file_name, const std::string& output_file_name, unsigned int number_of_layers )
	{ return new BatchGroup_Model( document, input_file_name, output_file_name, number_of_layers ); }

protected:
    virtual Model& layerize() { return Batch_Model::layerize(); }
    virtual Model& justify() { return Group_Model::justify(); }
};

/* ----------------------- SRVN Partition Model ----------------------- */

class SRVN_Model : virtual public Model, public Batch_Model
{
private:
    SRVN_Model( LQIO::DOM::Document * document, const std::string& input_file_name, const std::string& output_file_name, unsigned int number_of_layers ) :
	Model( document, input_file_name, output_file_name, number_of_layers ),
	Batch_Model( document, input_file_name, output_file_name, number_of_layers ) {}

public:
    static Model * create( LQIO::DOM::Document * document, const std::string& input_file_name, const std::string& output_file_name, unsigned int number_of_layers )
	{ return new SRVN_Model( document, input_file_name, output_file_name, number_of_layers ); }

protected:
    virtual bool selectSubmodel( const unsigned );
};

/* -------------------- Squashed Partition Model ---------------------- */

class Squashed_Model : virtual public Model, public Batch_Model
{
    struct Justify {
	void operator()( Group * );
    };
    
private:
    Squashed_Model( LQIO::DOM::Document * document, const std::string& input_file_name, const std::string& output_file_name ) :
	Model( document, input_file_name, output_file_name, PROCESSOR_LEVEL ),
	Batch_Model( document, input_file_name, output_file_name, PROCESSOR_LEVEL ) {}

public:
    static Model * create( LQIO::DOM::Document * document, const std::string& input_file_name, const std::string& output_file_name, unsigned int number_of_layers )
	{ return new Squashed_Model( document, input_file_name, output_file_name ); }

    virtual bool generate();
    virtual Model& justify();
};

inline std::ostream& operator<<( std::ostream& output, const Model& self ) { return self.print( output ); }
#endif
