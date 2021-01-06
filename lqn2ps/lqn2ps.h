/* -*- c++ -*-
 * lqn2ps.h	-- Greg Franks
 *
 * $Id: lqn2ps.h 14337 2021-01-05 11:32:10Z greg $
 *
 */

#ifndef _LQN2PS_H
#define _LQN2PS_H

#define EMF_OUTPUT	1
#define JMVA_OUTPUT	1
#define QNAP2_OUTPUT	1
#define SVG_OUTPUT	1
#define SXD_OUTPUT	1
#define TXT_OUTPUT	1
/* #define X11_OUTPUT */
#define REP2FLAT	1	/* Allow expansion */
#define BUG_270		1	/* Prune Null servers */

#if defined(HAVE_CONFIG_H)
#include <config.h>
#endif

#include <iostream>
#include <iomanip>
#include <string>
#include <stdexcept>
#include <deque>
#include <regex>
#include <lqio/dom_extvar.h>
#if defined(HAVE_VALUES_H)
#include <values.h>
#endif
#if defined(HAVE_FLOAT_H)
#include <float.h>
#endif

#if !defined(MAXDOUBLE)
#define MAXDOUBLE FLT_MAX
#endif

namespace LQIO {
    namespace DOM {
	class Activity;
	class ActivityList;
	class Call;
	class Document;
	class DocumentObject;
	class Entity;
	class Entry;
	class Group;
	class Phase;
	class Pragma;
	class Processor;
	class Task;    
    };
};

extern std::string command_line;
bool process_special( const char * p, LQIO::DOM::Pragma& );
bool special( const std::string& parameter, const std::string& value, LQIO::DOM::Pragma& );

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

extern std::string command_line;

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
#if JMVA_OUTPUT
    FORMAT_JMVA,
#endif
#if HAVE_GD_H && HAVE_LIBGD && HAVE_LIBJPEG 
    FORMAT_JPEG,
#endif
    FORMAT_LQX,
    FORMAT_NULL,
    FORMAT_OUTPUT,
#if QNAP2_OUTPUT
    FORMAT_QNAP2,
#endif
    FORMAT_PARSEABLE,
#if HAVE_GD_H && HAVE_LIBGD && HAVE_LIBPNG
    FORMAT_PNG,
#endif
    FORMAT_POSTSCRIPT,
    FORMAT_PSTEX,
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
    LAYERING_MOL,
    LAYERING_PROCESSOR,
    LAYERING_PROCESSOR_TASK,
    LAYERING_SHARE,
    LAYERING_SQUASHED,
    LAYERING_SRVN,
    LAYERING_TASK_PROCESSOR
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
    REPLICATION_EXPAND,
    REPLICATION_RETURN
} replication_type;

typedef enum {
    FORWARD_SORT,
    REVERSE_SORT,
    TOPILOGICAL_SORT,
    NO_SORT,
    INVALID_SORT
} sort_type;

typedef enum {
    SPECIAL_ANNOTATE,
    SPECIAL_ARROW_SCALING,
    SPECIAL_BCMP,
    SPECIAL_CLEAR_LABEL_BACKGROUND,
    SPECIAL_EXHAUSTIVE_TOPOLOGICAL_SORT,
    SPECIAL_FLATTEN_SUBMODEL,
    SPECIAL_FORWARDING_DEPTH,
    SPECIAL_GROUP,
    SPECIAL_LAYER_NUMBER,
    SPECIAL_NO_ALIGNMENT_BOX,
    SPECIAL_NO_ASYNC_TOPOLOGICAL_SORT,
    SPECIAL_NO_CV_SQR,
    SPECIAL_NO_PHASE_TYPE,
    SPECIAL_NO_REF_TASK_CONVERSION,
    SPECIAL_PRUNE,
    SPECIAL_PROCESSOR_SCHEDULING,
    SPECIAL_QUORUM_REPLY,
    SPECIAL_RENAME,
    SPECIAL_SORT,
    SPECIAL_SQUISH_ENTRY_NAMES,
    SPECIAL_SPEX_HEADER,
    SPECIAL_SUBMODEL_CONTENTS,
    SPECIAL_TASKS_ONLY,	
    SPECIAL_TASK_SCHEDULING
} special_type;

typedef enum {
    COLOUR_OFF,
    COLOUR_RESULTS,		/* Default */
    COLOUR_LAYERS,		/* Each layer gets its own colour */
    COLOUR_CLIENTS,		/* Each client chaing gets its own colour */
    COLOUR_SERVER_TYPE,		/* client, server, etc... */
    COLOUR_CHAINS,		/* Useful for queueing output only */
    COLOUR_DIFFERENCES		/* Results are differences */
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
	std::regex * r;
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
    GNUPLOT		 ,
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
    SPECIAL               ,
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
    PROCESSOR_UTILIZATION,
    PROCESSOR_QUEUEING   ,
    RESULTS              ,
    SERVICE              ,
    TASK_THROUGHPUT      ,
    TASK_UTILIZATION     ,
    VARIANCE             ,
    WAITING              ,
    SERVICE_EXCEEDED     ,
    MODEL_COMMENT        ,
    SOLVER_INFO		 ,
    SUMMARY              ,
    IGNORE_ERRORS	 ,
    PRINT_AGGREGATE	 ,
    RUN_LQX		 ,
    RELOAD_LQX		 ,
    INCLUDE_ONLY         ,
    N_FLAG_VALUES               /* MUST be last! */
} flag_values;

struct Flags
{
    static bool annotate_input;
    static bool async_topological_sort;
    static bool bcmp_model;
    static bool clear_label_background;
    static bool convert_to_reference_task; 
    static bool debug;
    static bool debug_submodels;
    static bool dump_graphviz;
    static bool exhaustive_toplogical_sort;
    static bool flatten_submodel;
    static bool have_results;
    static bool instantiate;
    static bool output_coefficient_of_variation;
    static bool output_phase_type;
    static bool print_alignment_box;
    static bool print_comment;
    static bool print_forwarding_by_depth;
    static bool print_layer_number;
    static bool print_submodel_number;
    static bool print_submodels;
    static bool prune;			// BUG 270.
    static bool rename_model;
    static bool squish_names;
    static bool surrogates;
    static bool use_colour;
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
    static std::regex * client_tasks;
    static sort_type sort;
    static unsigned long span;
    static const unsigned int size;
};

class Options
{
private:
    Options();
    Options( const Options& );
    Options& operator=( const Options& );

public:
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
    static const char * special[];

    static size_t find_if( const char**, const std::string& );
};

/* ------------------------------------------------------------------------ */

class class_error : public std::logic_error
{
public:
    class_error( const std::string& method, const char * file, const unsigned line, const std::string& error );
    virtual ~class_error() throw() = 0;
private:
    static std::string message( const std::string& method, const char * file, const unsigned line, const std::string& );
};


class subclass_responsibility : public class_error 
{
public:
    subclass_responsibility( const std::string& method, const char * file, const unsigned line )
	: class_error( method, file, line, "Subclass responsibility." ) {}
    virtual ~subclass_responsibility() throw() {}
};

class not_implemented  : public class_error 
{
public:
    not_implemented( const std::string& method, const char * file, const unsigned line )
	: class_error( method, file, line, "Not implemented." ) {}
    virtual ~not_implemented() throw() {}
};


class should_not_implement  : public class_error 
{
public:
    should_not_implement( const std::string& method, const char * file, const unsigned line )
	: class_error( method, file, line, "Should not implement." ) {}
    virtual ~should_not_implement() throw() {}
};

class path_error : public std::exception {
public:
    explicit path_error( const size_t depth=0 ) : _depth(depth) {}
    virtual ~path_error() throw() {} 
    virtual const char * what() const throw();
    size_t depth() const { return _depth; }

protected:
    std::string myMsg;
    const size_t _depth;
};

/* ------------------------------------------------------------------------ */

class StringManip {
public:
    StringManip( std::ostream& (*ff)(std::ostream&, const char * ), const char * aStr )
	: f(ff), myStr(aStr) {}
private:
    std::ostream& (*f)( std::ostream&, const char * );
    const char * myStr;

    friend std::ostream& operator<<(std::ostream & os, const StringManip& m ) 
	{ return m.f(os,m.myStr); }
};

class StringManip2 {
public:
    StringManip2( std::ostream& (*ff)(std::ostream&, const std::string& ), const std::string& aStr )
	: f(ff), myStr(aStr) {}
private:
    std::ostream& (*f)( std::ostream&, const std::string& );
    const std::string& myStr;

    friend std::ostream& operator<<(std::ostream & os, const StringManip2& m ) 
	{ return m.f(os,m.myStr); }
};

class StringPlural {
public:
    StringPlural( std::ostream& (*ff)(std::ostream&, const std::string&, const unsigned ), const std::string& aStr, const unsigned anInt )
	: f(ff), myStr(aStr), myInt(anInt) {}
private:
    std::ostream& (*f)( std::ostream&, const std::string&, const unsigned );
    const std::string& myStr;
    const unsigned myInt;

    friend std::ostream& operator<<(std::ostream & os, const StringPlural& m ) 
	{ return m.f(os,m.myStr,m.myInt); }
};

class IntegerManip {
public:
    IntegerManip( std::ostream& (*ff)(std::ostream&, const int ), const int anInt )
	: f(ff), myInt(anInt) {}
private:
    std::ostream& (*f)( std::ostream&, const int );
    const int myInt;

    friend std::ostream& operator<<(std::ostream & os, const IntegerManip& m ) 
	{ return m.f(os,m.myInt); }
};

class UnsignedManip {
public:
    UnsignedManip( std::ostream& (*ff)(std::ostream&, const unsigned int ), const unsigned int anInt )
	: f(ff), myInt(anInt) {}
private:
    std::ostream& (*f)( std::ostream&, const unsigned int );
    const unsigned int myInt;

    friend std::ostream& operator<<(std::ostream & os, const UnsignedManip& m ) 
	{ return m.f(os,m.myInt); }
};

class BooleanManip {
public:
    BooleanManip( std::ostream& (*ff)(std::ostream&, const bool ), const bool aBool )
	: f(ff), myBool(aBool) {}
private:
    std::ostream& (*f)( std::ostream&, const bool );
    const bool myBool;

    friend std::ostream& operator<<(std::ostream & os, const BooleanManip& m ) 
	{ return m.f(os,m.myBool); }
};

class DoubleManip {
public:
    DoubleManip( std::ostream& (*ff)(std::ostream&, const double ), const double aDouble )
	: f(ff), myDouble(aDouble) {}
private:
    std::ostream& (*f)( std::ostream&, const double );
    const double myDouble;

    friend std::ostream& operator<<(std::ostream & os, const DoubleManip& m ) 
	{ return m.f(os,m.myDouble); }
};

class Integer2Manip {
public:
    Integer2Manip( std::ostream& (*ff)(std::ostream&, const int, const int ), const int int1, const int int2 )
	: f(ff), myInt1(int1), myInt2(int2) {}
private:
    std::ostream& (*f)( std::ostream&, const int, const int );
    const int myInt1;
    const int myInt2;

    friend std::ostream& operator<<(std::ostream & os, const Integer2Manip& m ) 
	{ return m.f(os,m.myInt1,m.myInt2); }
};

class TimeManip {
public:
    TimeManip( std::ostream& (*ff)(std::ostream&, const double ), const double aTime )
	: f(ff), myTime(aTime) {}
private:
    std::ostream& (*f)( std::ostream&, const double );
    const double myTime;

    friend std::ostream& operator<<(std::ostream & os, const TimeManip& m ) 
	{ return m.f(os,m.myTime); }
};

class ExtVarManip {
public:
    ExtVarManip( std::ostream& (*ff)(std::ostream&, const LQIO::DOM::ExternalVariable& ), const LQIO::DOM::ExternalVariable& aVar )
	: f(ff), myVar(aVar) {}
private:
    std::ostream& (*f)( std::ostream&, const LQIO::DOM::ExternalVariable& );
    const LQIO::DOM::ExternalVariable& myVar;

    friend std::ostream& operator<<(std::ostream & os, const ExtVarManip& m ) 
	{ return m.f(os,m.myVar); }
};


/*
 * Return square.  C++ doesn't even have an exponentiation operator, let
 * alone a smart one.
 */

template <typename Type> inline Type square( Type a ) { return a * a; }

class Task;
class Entity;
class Activity;
class Entry;
class GenericCall;
class Label;
class Phase;

typedef bool (GenericCall::*callPredicate)() const;
typedef bool (Task::*taskPredicate)() const;
typedef std::ostream& (Entry::*entryFunc)( std::ostream& ) const;
typedef std::ostream& (Activity::*activityFunc)( std::ostream& ) const;
typedef std::ostream& (Entry::*entryCountFunc)( std::ostream&, int& ) const;
typedef std::ostream& (Activity::*activityCountFunc)( std::ostream&, int& ) const;
typedef double (Activity::*aggregateFunc)( Entry *, const unsigned, const double );
typedef double (GenericCall::*callPredicate2)() const;
typedef Entry& (Entry::*entryLabelFunc)( Label& );
typedef double (LQIO::DOM::DocumentObject::*get_function)() const;
typedef LQIO::DOM::DocumentObject& (LQIO::DOM::DocumentObject::*set_function)( const double );


int lqn2ps( int argc, char *argv[] );
void setOutputFormat( const int i );

bool graphical_output();
bool output_output();
bool input_output();
bool partial_output();
bool processor_output();		/* true if sorting by processor */
bool queueing_output();			/* true if generating queueing network */
bool submodel_output();			/* true if generating a submodel */
bool difference_output();		/* true if print differences */
bool share_output();			/* true if sorting by processor share */
int set_indent( int anInt );
inline double normalized_font_size() { return Flags::print[FONT_SIZE].value.i / Flags::print[MAGNIFICATION].value.f; }

IntegerManip indent( const int anInt );				/* See main.cc */
IntegerManip temp_indent( const int anInt );			/* See main.cc */
Integer2Manip conf_level( const int, const int );		/* See main.cc */
StringPlural plural( const std::string& s, const unsigned i );	/* See main.cc */
DoubleManip opt_pct( const double aDouble );			/* See main.cc */

/* ------------------------------------------------------------------------ */

#if defined(REP2FLAT)
    void update_mean( LQIO::DOM::DocumentObject *, set_function, const LQIO::DOM::DocumentObject *, get_function, unsigned );
    void update_variance( LQIO::DOM::DocumentObject *, set_function, const LQIO::DOM::DocumentObject *, get_function );
#endif


template <class Type> struct Exec
{
    typedef Type& (Type::*funcPtr)();
    Exec<Type>( funcPtr f ) : _f(f) {};
    void operator()( Type * object ) const { (object->*_f)(); }
    void operator()( Type& object ) const { (object.*_f)(); }
private:
    funcPtr _f;
};

template <class Type> struct ConstExec
{
    typedef const Type& (Type::*funcPtr)() const;
    ConstExec<Type>( const funcPtr f ) : _f(f) {};
    void operator()( const Type * object ) const { (object->*_f)(); }
    void operator()( const Type& object ) const { (object.*_f)(); }
private:
    const funcPtr _f;
};
    
template <class Type1, class Type2> struct Exec1
{
    typedef Type1& (Type1::*funcPtr)( Type2 x );
    Exec1<Type1,Type2>( funcPtr f, Type2 x ) : _f(f), _x(x) {}
    void operator()( Type1 * object ) const { (object->*_f)( _x ); }
    void operator()( Type1& object ) const { (object.*_f)( _x ); }
private:
    funcPtr _f;
    Type2 _x;
};

template <class Type1, class Type2> struct ConstExec1
{
    typedef const Type1& (Type1::*funcPtr)( Type2 x ) const;
    ConstExec1<Type1,Type2>( const funcPtr f, Type2 x ) : _f(f), _x(x) {}
    void operator()( Type1 * object ) const { (object->*_f)( _x ); }
    void operator()( Type1& object ) const { (object.*_f)( _x ); }
private:
    const funcPtr _f;
    Type2 _x;
};

template <class Type1, class Type2, class Type3> struct Exec2
{
    typedef Type1& (Type1::*funcPtr)( Type2 x, Type3 y );
    Exec2<Type1,Type2,Type3>( funcPtr f, Type2 x, Type3 y ) : _f(f), _x(x), _y(y) {}
    void operator()( Type1 * object ) const { (object->*_f)( _x, _y ); }
    void operator()( Type1& object ) const { (object.*_f)( _x, _y ); }
private:
    funcPtr _f;
    Type2 _x;
    Type3 _y;
};


template <class Type1, class Type2, class Type3> struct ConstExec2
{
    typedef const Type1& (Type1::*funcPtr)( Type2 x, Type3 y ) const;
    ConstExec2<Type1,Type2,Type3>( const funcPtr f, Type2 x, Type3 y ) : _f(f), _x(x), _y(y) {}
    void operator()( const Type1 * object ) const { (object->*_f)( _x, _y ); }
    void operator()( const Type1& object ) const { (object.*_f)( _x, _y ); }
private:
    const funcPtr _f;
    Type2 _x;
    Type3 _y;
};


template <class Type1, class Type2, class Type3, class Type4> struct ConstExec3
{
    typedef const Type1& (Type1::*funcPtr)( Type2 x, Type3 y, Type4 z ) const;
    ConstExec3<Type1,Type2,Type3,Type4>( const funcPtr f, Type2 x, Type3 y, Type4 z ) : _f(f), _x(x), _y(y), _z(z) {}
    void operator()( const Type1 * object ) const { (object->*_f)( _x, _y, _z ); }
    void operator()( const Type1& object ) const { (object.*_f)( _x, _y, _z ); }
private:
    const funcPtr _f;
    Type2 _x;
    Type3 _y;
    Type4 _z;
};


template <class Type1, class Type2, class Type3> struct ExecX
{
    typedef Type1& (Type1::*funcPtr)( Type3 x );
    ExecX<Type1,Type2,Type3>( funcPtr f, Type3 x ) : _f(f), _x(x) {}
    void operator()( const Type2& object ) const { (object.second->*_f)( _x ); }
private:
    funcPtr _f;
    Type3 _x;
};

template <class Type1, class Type2, class Type3> struct ConstExecX
{
    typedef const Type1& (Type1::*funcPtr)( Type3 x ) const;
    ConstExecX<Type1,Type2,Type3>( const funcPtr f, Type3 x ) : _f(f), _x(x) {}
    void operator()( const Type2& object ) const { (object.second->*_f)( _x ); }
private:
    const funcPtr _f;
    Type3 _x;
};

template <class Type> struct ExecXY
{
    typedef Type& (Type::*funcPtrXY)( double x, double y );
    ExecXY<Type>( funcPtrXY f, double x, double y ) : _f(f), _x(x), _y(y) {};
    void operator()( Type * object ) const { (object->*_f)( _x, _y ); }
    void operator()( Type& object ) const { (object.*_f)( _x, _y ); }
private:
    funcPtrXY _f;
    double _x;
    double _y;
};

template <class Type> struct Predicate
{
    typedef bool (Type::*predicate)() const;
    Predicate<Type>( const predicate p ) : _p(p) {};
    bool operator()( const Type * object ) const { return (object->*_p)(); }
    bool operator()( const Type& object ) const { return (object.*_p)(); }
private:
    const predicate _p;
};

template <class Type1, class Type2> struct Predicate1
{
    typedef bool (Type1::*predicate)(Type2) const;
    Predicate1<Type1,Type2>( const predicate p, Type2 v ) : _p(p), _v(v) {};
    bool operator()( const Type1 * object ) const { return (object->*_p)(_v); }
    bool operator()( const Type1& object ) const { return (object.*_p)(_v); }
private:
    const predicate _p;
    Type2 _v;
};

template <class Type> struct AndPredicate
{
    typedef bool (Type::*predicate)() const;
    AndPredicate<Type>( const predicate p ) : _p(p), _rc(true) {};
    void operator()( const Type * object ) { _rc = (object->*_p)() && _rc; }
    void operator()( const Type& object ) { _rc = (object.*_p)() && _rc; }
    bool result() const { return _rc; }
private:
    const predicate _p;
    bool _rc;
};

template <class Type1, class Type2> struct Count
{
    typedef unsigned (Type1::*funcPtr)(const Type2) const;
    Count<Type1,Type2>( funcPtr f, const Type2 p ) : _f(f), _p(p), _count(0) {}
    void operator()( const Type1 * object ) { _count += (object->*_f)(_p); }
    void operator()( const Type1& object ) { _count += (object.*_f)(_p); }
    unsigned int count() const { return _count; }
private:
    funcPtr _f;
    const Type2 _p;
    unsigned int _count;
};

template <class Type1, class Type2> struct Sum
{
    typedef Type2 (Type1::*funcPtr)() const;
    Sum<Type1,Type2>( funcPtr f ) : _f(f), _sum(0) {}
    void operator()( const Type1 * object ) { _sum += (object->*_f)(); }
    void operator()( const Type1& object ) { _sum += (object.*_f)(); }
    Type2 sum() const { return _sum; }
private:
    funcPtr _f;
    Type2 _sum;
};
	
template <class Type1> struct Sum<Type1,LQIO::DOM::ExternalVariable>
{
    typedef const LQIO::DOM::ExternalVariable& (Type1::*funcPtr)() const;
    Sum<Type1,LQIO::DOM::ExternalVariable>( funcPtr f ) : _f(f), _sum(0) {}
    void operator()( const Type1 * object ) { _sum += LQIO::DOM::to_double((object->*_f)()); }
    void operator()( const Type1& object ) { _sum += LQIO::DOM::to_double((object.*_f)()); }
    double sum() const { return _sum; }
private:
    funcPtr _f;
    double _sum;
};
	
template <class Type1> struct SumP
{
    typedef const LQIO::DOM::ExternalVariable& (Type1::*funcPtr)( unsigned int ) const;
    SumP<Type1>( funcPtr f, unsigned int p ) : _f(f), _p(p), _sum(0) {}
    void operator()( const Type1 * object ) { _sum += LQIO::DOM::to_double((object->*_f)(_p)); }
    void operator()( const Type1& object ) { _sum += LQIO::DOM::to_double((object.*_f)(_p)); }
    double sum() const { return _sum; }
private:
    funcPtr _f;
    unsigned int _p;
    double _sum;
};
	
template <class Type> struct EQ
{
    EQ<Type>( const Type * const a ) : _a(a) {}
    bool operator()( const Type * const b ) const { return _a == b; }
private:
    const Type * const _a;
};

template <class Type> struct EQStr
{
    EQStr( const std::string & s ) : _s(s) {}
    bool operator()(const Type * e1 ) const { return e1->name() == _s; }
private:
    const std::string & _s;
};

template <class Type> struct LT
{
    bool operator()(const Type * a, const Type * b) const { return a->name() < b->name(); }
};
#endif
