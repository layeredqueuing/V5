/* -*- c++ -*-
 * lqn2ps.h	-- Greg Franks
 *
 * $Id$
 *
 */

#ifndef _LQN2PS_H
#define _LQN2PS_H

#define EMF_OUTPUT
#define SVG_OUTPUT
#define SXD_OUTPUT
#define TXT_OUTPUT
#define QNAP_OUTPUT
#define PMIF_OUTPUT
/* #define X11_OUTPUT */
#define	TASK_ACTIVITIES		/* Graph type */
#define REP2FLAT		/* Allow expansion */

#if defined(HAVE_CONFIG_H)
#include <config.h>
#endif

#include <iostream>
#include <iomanip>
#include <string>
#include <set>
#include <stdexcept>
#include <lqio/input.h>
#include <lqio/dom_extvar.h>
#if HAVE_REGEX_H
#include <regex.h>
#endif


using namespace std;

namespace LQIO {
    namespace DOM {
	class Document;
	class ActivityList;
	class Activity;
	class Phase;
	class Entity;
	class Processor;
	class Group;
	class Task;    
	class Entry;
	class Call;
    };
};

extern lqio_params_stats io_vars;
extern std::string command_line;

const unsigned int MAX_PHASES	    = 3;	/* Number of Phases.		*/

const double PTS_PER_INCH	    = 72.0;
const double DEFAULT_X_SPACING	    = 9.0;
const double DEFAULT_Y_SPACING	    = 54.0;		/* Works well with xfig	*/
const double FIG_SCALING	    = 50.0/3.0;
const double EEPIC_SCALING	    = 10.0;
const double SVG_SCALING	    = 25.0;
const double SXD_SCALING	    = 2.54/72.0;	/* Points to cm */
const double GD_SCALING		    = 100.0/72.0;	/* Points to screen (100dpi) */
const double EMF_SCALING	    = 20;		/* Points to "twips" (1/20th of a point)  */

extern unsigned int maxStrLen;
extern const unsigned int maxDblLen;

const int min_fontsize 		    = 6;
const int max_fontsize              = 36;
const unsigned int MAX_CONFIDENCE   = 2;
const unsigned N_SEMAPHORE_ENTRIES  = 2;
const unsigned N_RWLOCK_ENTRIES		= 4;
const double EPSILON = 0.000001;

extern string command_line;

typedef enum {
    AGGREGATE_NONE,
    AGGREGATE_SEQUENCES,
    AGGREGATE_ACTIVITIES,
    AGGREGATE_PHASES,
    AGGREGATE_ENTRIES,
    AGGREGATE_THREADS
} aggregation_type;

typedef enum {
    TIMEBENCH_STYLE,
    JLQNDEF_STYLE
} graphical_output_style_type;

typedef enum {
    FORMAT_EEPIC,
#if defined(EMF_OUTPUT)
    FORMAT_EMF,
#endif
    FORMAT_FIG,
#if HAVE_GD_H && HAVE_LIBGD && HAVE_GDIMAGEGIFPTR
    FORMAT_GIF,
#endif
#if HAVE_GD_H && HAVE_LIBGD && HAVE_LIBJPEG 
    FORMAT_JPEG,
#endif
    FORMAT_NULL,
    FORMAT_OUTPUT,
    FORMAT_PARSEABLE,
#if HAVE_GD_H && HAVE_LIBGD && HAVE_LIBPNG
    FORMAT_PNG,
#endif
    FORMAT_POSTSCRIPT,
    FORMAT_PSTEX,
#if defined(QNAP_OUTPUT)
    FORMAT_QNAP,
#endif
    FORMAT_RTF,
    FORMAT_SRVN,
#if defined(SVG_OUTPUT)
    FORMAT_SVG,
#endif
#if defined(SXD_OUTPUT)
    FORMAT_SXD,
#endif
#if defined(TXT_OUTPUT)
    FORMAT_TXT,
#endif
#if defined(X11_OUTPUT)
    FORMAT_X11,
#endif
    FORMAT_XML,
    FORMAT_UNKNOWN
} output_format;

typedef enum {
    LAYERING_BATCH,
    LAYERING_GROUP,
    LAYERING_HWSW,
    LAYERING_SRVN,
    LAYERING_PROCESSOR,
    LAYERING_PROCESSOR_TASK,
    LAYERING_TASK_PROCESSOR,
    LAYERING_SHARE,
    LAYERING_SQUASHED,
    LAYERING_MOL,
    LAYERING_CLIENT
} layering_format;

typedef enum {
    PROCESSOR_NONE,
    PROCESSOR_DEFAULT,
    PROCESSOR_NONINFINITE,
    PROCESSOR_ALL
} processor_format;

typedef enum {
    DEFAULT_JUSTIFY,
    CENTER_JUSTIFY,
    LEFT_JUSTIFY,
    RIGHT_JUSTIFY,
    ALIGN_JUSTIFY,		/* For Nodes		*/
    ABOVE_JUSTIFY		/* For labels on Arcs.	*/
} justification_type;

typedef enum {
    KEY_NONE,
    KEY_TOP_LEFT,
    KEY_TOP_RIGHT,
    KEY_BOTTOM_LEFT,
    KEY_BOTTOM_RIGHT,
    KEY_BELOW_LEFT,
    KEY_ABOVE_LEFT,
    KEY_ON
} key_type;

typedef enum {
    REPLICATION_NOP,
    REPLICATION_REMOVE,
    REPLICATION_EXPAND
} replication_type;

typedef enum {
    FORWARD_SORT,
    REVERSE_SORT,
    TOPILOGICAL_SORT,
    NO_SORT,
    INVALID_SORT
} sort_type;

typedef enum {
    PRAGMA_ANNOTATE,
    PRAGMA_ARROW_SCALING,
    PRAGMA_CLEAR_LABEL_BACKGROUND,
    PRAGMA_EXHAUSTIVE_TOPOLOGICAL_SORT,
    PRAGMA_FLATTEN_SUBMODEL,
    PRAGMA_FORWARDING_DEPTH,
    PRAGMA_GROUP,
    PRAGMA_LAYER_NUMBER,
    PRAGMA_NO_ALIGNMENT_BOX,
    PRAGMA_NO_ASYNC_TOPOLOGICAL_SORT,
    PRAGMA_NO_CV_SQR,
    PRAGMA_NO_PHASE_TYPE,
    PRAGMA_NO_REF_TASK_CONVERSION,
    PRAGMA_QUORUM_REPLY,
    PRAGMA_RENAME,
    PRAGMA_SORT,
    PRAGMA_SQUISH_ENTRY_NAMES,
    PRAGMA_SUBMODEL_CONTENTS,
    PRAGMA_TASKS_ONLY,	
    PRAGMA_XML_SCHEMA
} pragma_type;

typedef enum {
    COLOUR_OFF,
    COLOUR_RESULTS,		/* Default */
    COLOUR_LAYERS,		/* Each layer gets its own colour */
    COLOUR_CLIENTS,		/* Each client chaing gets its own colour */
    COLOUR_SERVER_TYPE,		/* client, server, etc... */
    COLOUR_CHAINS		/* Useful for queueing output only */
} colouring_type;
	
typedef struct 
{
    const char * name;
    const int c;
    const char * arg;
    const char ** opts;
    union {
	int i;
	bool b;
#if HAVE_REGEX_H
	regex_t * r;
#endif
	char * s;
	double f;
    } value;
    const bool result;
    const char * msg;
} option_type;


/*
 * This enumeration must be the same size and in the same order as
 * option Flags::print[]; in lqn2ps.cc
 */

typedef enum
{
    AGGREGATION          ,
    BORDER               ,
    COLOUR               ,
    JLQNDEF		 ,
    FONT_SIZE            ,
    INPUT_FILE_FORMAT	 , 
    HELP                 ,
    JUSTIFICATION        ,
    KEY                  ,
    LAYERING             ,
    MAGNIFICATION        ,
    PRECISION            ,
    OUTPUT_FORMAT        ,
    PROCESSORS           ,
    QUEUEING_MODEL       ,
#if defined(REP2FLAT)
    REPLICATION          ,
#endif
    SUBMODEL             ,
    XX_VERSION           ,
    WARNINGS             ,
    X_SPACING            ,
    Y_SPACING            ,
    PRAGMA               ,
    OPEN_WAIT            ,
    THROUGHPUT_BOUNDS    ,
    CONFIDENCE_INTERVALS ,
    ENTRY_UTILIZATION    ,
    ENTRY_THROUGHPUT     ,
    HISTOGRAMS           ,
    HOLD_TIMES           ,
    INPUT_PARAMETERS     ,
    JOIN_DELAYS          ,
    CHAIN                ,
    LOSS_PROBABILITY     ,
    OUTPUT_FILE          ,
    PROCESS_UTIL         ,
    PROCESS_QUEUEING     ,
    RESULTS              ,
    SERVICE              ,
    TASK_THROUGHPUT      ,
    TASK_UTILIZATION     ,
    VARIANCE             ,
    WAITING              ,
    SERVICE_EXCEEDED     ,
    VERBOSE              ,
    PRINT_AGGREGATE	 ,
    MANUAL		 ,
    RUN_LQX		 ,
    RELOAD_LQX		 ,
    INCLUDE_ONLY         ,
    N_FLAG_VALUES               /* MUST be last! */
} flag_values;

struct Flags
{
    static bool annotate_input;
    static bool async_topological_sort;
    static bool clear_label_background;
    static bool convert_to_reference_task; 
    static bool debug_submodels;
    static bool exhaustive_toplogical_sort;
    static bool flatten_submodel;
    static bool have_results;
    static bool instantiate;
    static bool output_coefficient_of_variation;
    static bool output_phase_type;
    static bool print_alignment_box;
    static bool print_forwarding_by_depth;
    static bool print_layer_number;
    static bool print_submodel_number;
    static bool print_submodels;
    static bool rename_model;
    static bool squish_names;
    static bool surrogates;
    static bool use_colour;
    static bool dump_graphviz;
    static double act_x_spacing;
    static double arrow_scaling;
    static double entry_height;
    static double entry_width;
    static double icon_height;
    static double icon_slope;
    static double icon_width;
    static justification_type activity_justification;
    static justification_type label_justification; 
    static justification_type node_justification;
    static graphical_output_style_type graphical_output_style;
    static option_type print[];
#if HAVE_REGEX_H
    static regex_t * client_tasks;
#endif
    static sort_type sort;
    static unsigned long span;
};

struct Options
{
    static const char * io[];
    static const char * layering[];
    static const char * colouring[];
    static const char * processor[];
    static const char * activity[];
    static const char * justification[];
    static const char * integer[];
    static const char * key[];
    static const char * real[];
    static const char * replication[];
    static const char * string[];
    static const char * sort[];
    static const char * pragma[];
};

static inline unsigned max( const unsigned a1, const unsigned a2 )
{
    return ( a1 > a2 ) ? a1 : a2;
}


static inline double max( const double a1, const double a2 )
{
    return ( a1 > a2 ) ? a1 : a2;
}

static inline unsigned min( const unsigned a1, const unsigned a2 )
{
    return ( a1 < a2 ) ? a1 : a2;
}


static inline double min( const double a1, const double a2 )
{
    return ( a1 < a2 ) ? a1 : a2;
}

/* ------------------------------------------------------------------------ */

class class_error : public exception 
{
public:
    class_error( const string& aStr, const char * file, const unsigned line, const char * anError );
    virtual ~class_error() throw() = 0;
    virtual const char* what() const throw();

private:
    string myMsg;
};


class subclass_responsibility : public class_error 
{
public:
    subclass_responsibility( const string& aStr, const char * file, const unsigned line )
	: class_error( aStr, file, line, "Subclass responsibility." ) {}
    virtual ~subclass_responsibility() throw() {}
};

class not_implemented  : public class_error 
{
public:
    not_implemented( const string& aStr, const char * file, const unsigned line )
	: class_error( aStr, file, line, "Not implemented." ) {}
    virtual ~not_implemented() throw() {}
};


class should_not_implement  : public class_error 
{
public:
    should_not_implement( const string& aStr, const char * file, const unsigned line )
	: class_error( aStr, file, line, "Should not implement." ) {}
    virtual ~should_not_implement() throw() {}
};

class path_error : public exception {
public:
    explicit path_error( const unsigned depth=0 ) : myDepth(depth) {}
    virtual ~path_error() throw() {} 
    virtual const char * what() const throw();
    unsigned depth() const { return myDepth; }

protected:
    string myMsg;
    const unsigned myDepth;
};

/* ------------------------------------------------------------------------ */

class StringManip {
public:
    StringManip( ostream& (*ff)(ostream&, const char * ), const char * aStr )
	: f(ff), myStr(aStr) {}
private:
    ostream& (*f)( ostream&, const char * );
    const char * myStr;

    friend ostream& operator<<(ostream & os, const StringManip& m ) 
	{ return m.f(os,m.myStr); }
};

class StringManip2 {
public:
    StringManip2( ostream& (*ff)(ostream&, const string& ), const string& aStr )
	: f(ff), myStr(aStr) {}
private:
    ostream& (*f)( ostream&, const string& );
    const string& myStr;

    friend ostream& operator<<(ostream & os, const StringManip2& m ) 
	{ return m.f(os,m.myStr); }
};

class StringPlural {
public:
    StringPlural( ostream& (*ff)(ostream&, const string&, const unsigned ), const string& aStr, const unsigned anInt )
	: f(ff), myStr(aStr), myInt(anInt) {}
private:
    ostream& (*f)( ostream&, const string&, const unsigned );
    const string& myStr;
    const unsigned myInt;

    friend ostream& operator<<(ostream & os, const StringPlural& m ) 
	{ return m.f(os,m.myStr,m.myInt); }
};

class IntegerManip {
public:
    IntegerManip( ostream& (*ff)(ostream&, const int ), const int anInt )
	: f(ff), myInt(anInt) {}
private:
    ostream& (*f)( ostream&, const int );
    const int myInt;

    friend ostream& operator<<(ostream & os, const IntegerManip& m ) 
	{ return m.f(os,m.myInt); }
};

class UnsignedManip {
public:
    UnsignedManip( ostream& (*ff)(ostream&, const unsigned int ), const unsigned int anInt )
	: f(ff), myInt(anInt) {}
private:
    ostream& (*f)( ostream&, const unsigned int );
    const unsigned int myInt;

    friend ostream& operator<<(ostream & os, const UnsignedManip& m ) 
	{ return m.f(os,m.myInt); }
};

class BooleanManip {
public:
    BooleanManip( ostream& (*ff)(ostream&, const bool ), const bool aBool )
	: f(ff), myBool(aBool) {}
private:
    ostream& (*f)( ostream&, const bool );
    const bool myBool;

    friend ostream& operator<<(ostream & os, const BooleanManip& m ) 
	{ return m.f(os,m.myBool); }
};

class DoubleManip {
public:
    DoubleManip( ostream& (*ff)(ostream&, const double ), const double aDouble )
	: f(ff), myDouble(aDouble) {}
private:
    ostream& (*f)( ostream&, const double );
    const double myDouble;

    friend ostream& operator<<(ostream & os, const DoubleManip& m ) 
	{ return m.f(os,m.myDouble); }
};

class Integer2Manip {
public:
    Integer2Manip( ostream& (*ff)(ostream&, const int, const int ), const int int1, const int int2 )
	: f(ff), myInt1(int1), myInt2(int2) {}
private:
    ostream& (*f)( ostream&, const int, const int );
    const int myInt1;
    const int myInt2;

    friend ostream& operator<<(ostream & os, const Integer2Manip& m ) 
	{ return m.f(os,m.myInt1,m.myInt2); }
};

class TimeManip {
public:
    TimeManip( ostream& (*ff)(ostream&, const double ), const double aTime )
	: f(ff), myTime(aTime) {}
private:
    ostream& (*f)( ostream&, const double );
    const double myTime;

    friend ostream& operator<<(ostream & os, const TimeManip& m ) 
	{ return m.f(os,m.myTime); }
};

class ExtVarManip {
public:
    ExtVarManip( ostream& (*ff)(ostream&, const LQIO::DOM::ExternalVariable& ), const LQIO::DOM::ExternalVariable& aVar )
	: f(ff), myVar(aVar) {}
private:
    ostream& (*f)( ostream&, const LQIO::DOM::ExternalVariable& );
    const LQIO::DOM::ExternalVariable& myVar;

    friend ostream& operator<<(ostream & os, const ExtVarManip& m ) 
	{ return m.f(os,m.myVar); }
};

/*
 * Return square.  C++ doesn't even have an exponentiation operator, let
 * alone a smart one.
 */

static inline double square( const double x )
{
    return x * x;
}


class Task;
class Entity;

/*
 * Compare to tasks by their name.  Used by the set class to insert items
 */

struct ltTask
{
    bool operator()(const Task * p1, const Task * p2) const;
};

template <class Type> class Vector;

class Activity;
class Entry;
class GenericCall;
class Label;

typedef bool (GenericCall::*callFunc)() const;
typedef bool (Entry::*testFunc)() const;
typedef ostream& (Entry::*entryFunc)( ostream& ) const;
typedef ostream& (Activity::*activityFunc)( ostream& ) const;
typedef ostream& (Entry::*entryCountFunc)( ostream&, int& ) const;
typedef ostream& (Activity::*activityCountFunc)( ostream&, int& ) const;
typedef double (Activity::*aggregateFunc)( const Entry *, const unsigned, const double ) const;
typedef double (GenericCall::*callFunc2)() const;
typedef const Entry& (Entry::*entryLabelFunc)( Label& ) const;

int lqn2ps( int argc, char *argv[] );
void setOutputFormat( const int i );

#if HAVE_REGEX_T
void regexp_check( const int, regex_t * r ) throw( runtime_error );
#endif
double delta( const Vector<double>& values );
bool graphical_output();
bool output_output();
bool input_output();
bool partial_output();
bool processor_output();		/* true if sorting by processor */
bool queueing_output();			/* true if generating queueing network */
bool submodel_output();			/* true if generating a submodel */
bool share_output();			/* true if sorting by processor share */
int set_indent( const unsigned int anInt );
inline double normalized_font_size() { return Flags::print[FONT_SIZE].value.i / Flags::print[MAGNIFICATION].value.f; }
bool process_pragma( const char * );
bool pragma( const string&, const string& );

IntegerManip indent( const int anInt );
IntegerManip temp_indent( const int anInt );
Integer2Manip conf_level( const int, const int );
ExtVarManip instantiate( const LQIO::DOM::ExternalVariable& );
StringPlural plural( const string& s, const unsigned i );
#endif
