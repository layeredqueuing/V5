/* -*- c++ -*-
 * lqn2ps.h	-- Greg Franks
 *
 * $Id: lqn2ps.h 16963 2024-01-28 02:29:06Z greg $
 * ------------------------------------------------------------------------
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
#define BUG_270_DEBUG	1	/* Prune Null servers */
#define BUG_299		0	/* Divide (y) by fan-out (load balance) */

#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>
#include <iomanip>
#include <string>
#include <stdexcept>
#include <regex>
#include <lqio/dom_document.h>
#include <lqio/dom_extvar.h>
#include "option.h"

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

const unsigned int MAX_PHASES	    = 3;		/* Number of Phases.		*/

const double PTS_PER_INCH	    = 72.0;
const double DEFAULT_X_SPACING	    = 9.0;
const double DEFAULT_Y_SPACING	    = 45.0;		/* Works well with xfig	*/
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
const unsigned N_RWLOCK_ENTRIES	    = 4;
const double EPSILON = 0.000001;

extern std::string command_line;

enum class Output_Style {
    TIMEBENCH,
    JLQNDEF
};

/*
 * This enumeration must be the same size and in the same order as
 * option Flags::print[]; in lqn2ps.cc
 */

typedef enum
{
    AGGREGATION          ,
    BORDER               ,
    COLOUR               ,
    DIFFMODE		 ,
    FONT_SIZE            ,
    INPUT_FORMAT	 ,
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
    SPECIAL              ,
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
    MODEL_DESCRIPTION    ,
    SOLVER_INFO		 ,
    SUMMARY              ,
    IGNORE_ERRORS	 ,
    PRINT_AGGREGATE	 ,
    RUN_LQX		 ,
    RELOAD_LQX		 ,
    OUTPUT_LQX		 ,
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
    static bool print_spex;
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
    static Justification activity_justification;
    static Justification label_justification;
    static Justification node_justification;
    static Output_Style graphical_output_style;
    static std::vector<Options::Type> print;
    static std::regex * client_tasks;
    static Sorting sort;
    static unsigned long span;
    static const unsigned int size;

public:
    static bool ignore_errors() { return print[IGNORE_ERRORS].opts.value.b; }
    static bool set_ignore_errors( bool b ) { print[IGNORE_ERRORS].opts.value.b = b; return b; }
    static bool print_input_parameters() { return Flags::print[INPUT_PARAMETERS].opts.value.b; }
    static bool set_print_input_parameters( bool b ) { Flags::print[INPUT_PARAMETERS].opts.value.b = b; return b; }
    static bool print_results() { return print[RESULTS].opts.value.b; }
    static bool set_print_results( bool b ) { print[RESULTS].opts.value.b = b; return b; }
    static bool reload_lqx() { return print[RELOAD_LQX].opts.value.b; }
    static bool set_reload_lqx( bool b ) { print[RELOAD_LQX].opts.value.b = b; return b; }
    static bool run_lqx() { return print[RUN_LQX].opts.value.b; }
    static bool set_run_lqx( bool b ) { print[RUN_LQX].opts.value.b = b; return b; }

    static double border() { return Flags::print[BORDER].opts.value.d; }
    static double set_border( double d ) { Flags::print[BORDER].opts.value.d = d; return d; }
    static double magnification() { return Flags::print[MAGNIFICATION].opts.value.d; }
    static double set_magnification( double d ) { Flags::print[MAGNIFICATION].opts.value.d = d; return d; }
    static double x_spacing() { return Flags::print[X_SPACING].opts.value.d; }
    static double set_x_spacing( double d ) { return Flags::print[X_SPACING].opts.value.d = d; return d; }
    static double y_spacing() { return Flags::print[Y_SPACING].opts.value.d; }
    static double set_y_spacing( double d ) { return Flags::print[Y_SPACING].opts.value.d = d; return d; }

    static unsigned int chain() { return Flags::print[CHAIN].opts.value.i; }
    static unsigned int set_chain( unsigned int i ) { Flags::print[CHAIN].opts.value.i = i; return i; }
    static unsigned int font_size() { return Flags::print[FONT_SIZE].opts.value.i; }
    static unsigned int set_font_size( int i ) { Flags::print[FONT_SIZE].opts.value.i = i; return i; }
    static unsigned int precision() { return Flags::print[PRECISION].opts.value.i; }
    static unsigned int queueing_model() { return Flags::print[QUEUEING_MODEL].opts.value.i; }
    static unsigned int set_queueing_model( unsigned int i ) { Flags::print[QUEUEING_MODEL].opts.value.i = i; return i; }
    static unsigned int set_precision( unsigned int i ) { Flags::print[PRECISION].opts.value.i = i; return i; }
    static unsigned int submodel() { return Flags::print[SUBMODEL].opts.value.i; }
    static unsigned int set_submodel( unsigned int i ) { Flags::print[SUBMODEL].opts.value.i = i; return i; }

    static std::regex* include_only() { return Flags::print[INCLUDE_ONLY].opts.value.m; }
    static std::regex* set_include_only( std::regex* m ) { Flags::print[INCLUDE_ONLY].opts.value.m = m; return m; }
    
    static Aggregate aggregation() { return print[AGGREGATION].opts.value.a; }
    static Aggregate set_aggregation( Aggregate a ) { print[AGGREGATION].opts.value.a = a; return a; }
    static Colouring colouring() { return print[COLOUR].opts.value.c; }
    static Colouring set_colouring( Colouring c ) { print[COLOUR].opts.value.c = c; return c; }
    static File_Format input_format() { return print[INPUT_FORMAT].opts.value.f; }
    static File_Format output_format() { return print[OUTPUT_FORMAT].opts.value.f; }
    static File_Format set_input_format( File_Format f ) { print[INPUT_FORMAT].opts.value.f = f; return f; }
    static File_Format set_output_format( File_Format f ) { print[OUTPUT_FORMAT].opts.value.f = f; return f; }
    static Key_Position key_position() { return print[KEY].opts.value.k; }
    static Key_Position set_key_position( Key_Position k ) { print[KEY].opts.value.k = k; return k; }
    static Layering layering() { return print[LAYERING].opts.value.l; }
    static Layering set_layering( Layering l ) { print[LAYERING].opts.value.l = l; return l;}
    static Processors processors() { return print[PROCESSORS].opts.value.p; }
    static Processors set_processors( Processors p ) { print[PROCESSORS].opts.value.p = p; return p; }
    static Replication replication() { return print[REPLICATION].opts.value.r; }
    static Replication set_replication( Replication r ) { return print[REPLICATION].opts.value.r = r; return r; }
    //    static Special
    //    static Sorting
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
template <typename Type> inline void Delete( Type x ) { delete x; }

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
void setOutputFormat( const File_Format );

bool graphical_output();
bool output_output();
bool input_output();			/* LQN, JSON, or XML output	*/
bool bcmp_output();			/* JMVA or QNAP2 model output	*/
bool partial_output();
bool processor_output();		/* true if sorting by processor */
bool queueing_output();			/* true if generating queueing network */
bool submodel_output();			/* true if generating a submodel */
bool difference_output();		/* true if print differences */
bool share_output();			/* true if sorting by processor share */
int set_indent( int anInt );
inline double normalized_font_size() { return Flags::font_size() / Flags::magnification(); }

IntegerManip indent( const int anInt );				/* See main.cc */
IntegerManip temp_indent( const int anInt );			/* See main.cc */
Integer2Manip conf_level( const int, const int );		/* See main.cc */
StringPlural plural( const std::string& s, const unsigned i );	/* See main.cc */

/* ------------------------------------------------------------------------ */

#if REP2FLAT
    void update_mean( LQIO::DOM::DocumentObject *, set_function, const LQIO::DOM::DocumentObject *, get_function, unsigned );
    void update_variance( LQIO::DOM::DocumentObject *, set_function, const LQIO::DOM::DocumentObject *, get_function );
#endif


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


template <class Type1, class Type2, class Type3> struct ExecX
{
    typedef Type1& (Type1::*funcPtr)( Type3 x );
    ExecX<Type1,Type2,Type3>( funcPtr f, Type3 x ) : _f(f), _x(x) {}
    void operator()( const Type2& object ) const { (object.second->*_f)( _x ); }
private:
    funcPtr _f;
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

template <class Type> struct Select
{
    typedef bool (Type::*predicate)() const;
    Select<Type>( const predicate p ) : _p(p) {};
    std::vector<Type*> operator()( const std::vector<Type*>& in, const Type* object ) const { if ( (object->*_p)() ) { std::vector<Type*> out(in); out.push_back(object);  return out; } else { return in;} }
    std::vector<Type*> operator()( const std::vector<Type*>& in, const Type& object ) const { if ( (object.*_p)()  ) { std::vector<Type*> out(in); out.push_back(&object); return out; } else { return in;} }
private:
    const predicate _p;
};


template <class Type1, class Type2> struct Collect
{
    typedef const Type1& (Type2::*function)() const;
    Collect<Type1,Type2>( const function f ) : _f(f) {};
    std::vector<Type1> operator()( const std::vector<Type1>& in, const Type2* object ) const { std::vector<Type1> out(in); out.push_back((object->*_f)()); return out; }
    std::vector<Type1> operator()( const std::vector<Type1>& in, const Type2& object ) const { std::vector<Type1> out(in); out.push_back((object.*_f)());  return out; }
private:
    const function _f;
};

inline std::string fold( const std::string& s1, const std::string& s2 ) { return s1 + "," + s2; }

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
