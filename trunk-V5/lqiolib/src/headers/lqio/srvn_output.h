/* -*- c++ -*- */
/************************************************************************/
/* Copyright the Real-Time and Distributed Systems Group,		*/
/* Department of Systems and Computer Engineering,			*/
/* Carleton University, Ottawa, Ontario, Canada. K1S 5B6		*/
/* 									*/
/* 2009.								*/
/************************************************************************/

/*
 * $Id$
 *
 * This class is used to hide the methods used to output to the Xerces DOM.
 */

#ifndef __SRVN_OUTPUT_H
#define __SRVN_OUTPUT_H

#include <vector>
#include <map>
#include <ostream>
#include <cassert>
#include <sys/time.h>
#include "dom_phase.h"
#include "dom_call.h"
#include "input.h"
#include "confidence_intervals.h"

namespace LQIO {
    namespace DOM {
	class Call;
	class Document;
	class Entity;
	class Entry;
	class ExternalVariable;
	class Group;
	class Phase;
	class Processor;
	class Task;
	class ActivityList;
    }

    using namespace std;

    namespace SRVN {

	typedef const DOM::ExternalVariable* (DOM::Activity::*activityFunc)() const;
	typedef double (DOM::Activity::*doubleActivityFunc)() const;
	typedef bool (DOM::Activity::*boolActivityFunc)() const;

	typedef const DOM::ExternalVariable* (DOM::Call::*callFunc)() const;
	typedef bool (DOM::Call::*boolCallFunc)() const;

	typedef const DOM::ExternalVariable* (DOM::Phase::*phaseFunc)() const;
	typedef double (DOM::Phase::*doublePhaseFunc)() const;
	typedef double (DOM::Entry::*doubleEntryPhaseFunc)( const unsigned ) const;
	typedef const DOM::Histogram* (DOM::Entry::*histogramEntryFunc)( const unsigned ) const;
	typedef phase_type (DOM::Phase::*phaseTypeFunc)() const;
//	typedef void (CallInput::*voidCallFunc)( const DOM::Call& ) const;

	typedef const DOM::ExternalVariable* (DOM::Entry::*entryFunc)() const;

	typedef double (DOM::Task::*doubleTaskFunc)( const unsigned ) const;

	class CallManip {
	public:
	    CallManip( ostream& (*f)(ostream&, const DOM::Call& ), const DOM::Call& c ) : _f(f), _c(c) {}
	private:
	    ostream& (*_f)( ostream&, const DOM::Call& );
	    const DOM::Call& _c;
	    friend ostream& operator<<(ostream & os, const CallManip& m ) { return m._f(os,m._c); }
	};

	class ConfidenceManip {
	public:
	    ConfidenceManip( ostream& (*f)(ostream&, const unsigned, const ConfidenceIntervals::confidence_level_t ), const unsigned s, const ConfidenceIntervals::confidence_level_t l  ) : _f(f), _s(s), _l(l) {}
	private:
	    ostream& (*_f)( ostream&, const unsigned, const ConfidenceIntervals::confidence_level_t );
	    const unsigned _s;
	    const ConfidenceIntervals::confidence_level_t _l;
	    friend ostream& operator<<(ostream & os, const ConfidenceManip& m ) { return m._f(os,m._s,m._l); }
	};

	class DoubleManip {
	public:
	    DoubleManip( ostream& (*f)(ostream&, const double ), const double a ) : _f(f), _a(a) {}
	private:
	    ostream& (*_f)( ostream&, const double );
	    const double _a;
	    friend ostream& operator<<(ostream & os, const DoubleManip& m ) { return m._f(os,m._a); }
	};

	class DoubleDoubleManip {
	public:
	    DoubleDoubleManip( ostream& (*f)(ostream&, const double, const double ), const double a, const double b ) : _f(f), _a(a), _b(b) {}
	private:
	    ostream& (*_f)( ostream&, const double, const double );
	    const double _a;
	    const double _b;
	    friend ostream& operator<<(ostream & os, const DoubleDoubleManip& m ) { return m._f(os,m._a,m._b); }
	};

	class EntityManip {
	public:
	    EntityManip( ostream& (*f)(ostream&, const DOM::Entity& ), const DOM::Entity& e ) : _f(f), _e(e) {}
	private:
	    ostream& (*_f)( ostream&, const DOM::Entity& );
	    const DOM::Entity& _e;
	    friend ostream& operator<<(ostream & os, const EntityManip& m ) { return m._f(os,m._e); }
	};

	class EntityNameManip {
	public:
	    EntityNameManip( ostream& (*f)(ostream&, const DOM::Entity&, bool& ), const DOM::Entity& e, bool& b ) : _f(f), _e(e), _b(b) {}
	private:
	    ostream& (*_f)( ostream&, const DOM::Entity&, bool& );
	    const DOM::Entity& _e;
	    bool& _b;
	    friend ostream& operator<<(ostream & os, const EntityNameManip& m ) { return m._f(os,m._e,m._b); }
	};

	class EntryManip {
	public:
	    EntryManip( ostream& (*f)(ostream&, const DOM::Entry&, const entryFunc ), const DOM::Entry & e, const entryFunc p ) : _f(f), _e(e), _p(p) {}
	private:
	    ostream& (*_f)( ostream&, const DOM::Entry&, const entryFunc );
	    const DOM::Entry & _e;
	    const entryFunc _p;
	    friend ostream& operator<<(ostream & os, const EntryManip& m ) { return m._f(os,m._e,m._p); }
	};

	class EntryNameManip {
	public:
	    EntryNameManip( ostream& (*f)(ostream&, const DOM::Entry& ), const DOM::Entry& e ) : _f(f), _e(e) {}
	private:
	    ostream& (*_f)( ostream&, const DOM::Entry& );
	    const DOM::Entry& _e;
	    friend ostream& operator<<(ostream & os, const EntryNameManip& m ) { return m._f(os,m._e); }
	};

	class PhaseManip {
	public:
	    PhaseManip( ostream& (*f)(ostream&, const DOM::Entry&, const phaseFunc ), const DOM::Entry & e, const phaseFunc p ) : _f(f), _e(e), _p(p) {}
	private:
	    ostream& (*_f)( ostream&, const DOM::Entry&, const phaseFunc );
	    const DOM::Entry & _e;
	    const phaseFunc _p;
	    friend ostream& operator<<(ostream & os, const PhaseManip& m ) { return m._f(os,m._e,m._p); }
	};

	class PhaseTypeManip {
	public:
	    PhaseTypeManip( ostream& (*f)(ostream&, const DOM::Entry&, const phaseTypeFunc ), const DOM::Entry & e, const phaseTypeFunc p ) : _f(f), _e(e), _p(p) {}
	private:
	    ostream& (*_f)( ostream&, const DOM::Entry&, const phaseTypeFunc );
	    const DOM::Entry & _e;
	    const phaseTypeFunc _p;
	    friend ostream& operator<<(ostream & os, const PhaseTypeManip& m ) { return m._f(os,m._e,m._p); }
	};

	class ProcessorManip {
	public:
	    ProcessorManip( ostream& (*f)(ostream&, const DOM::Processor& ), const DOM::Processor& p ) : _f(f), _p(p) {}
	private:
	    ostream& (*_f)( ostream&, const DOM::Processor& );
	    const DOM::Processor& _p;
	    friend ostream& operator<<(ostream & os, const ProcessorManip& m ) { return m._f(os,m._p); }
	};

	class ResultsManip {
	public:
	    ResultsManip( ostream& (*f)(ostream&, const DOM::Entry&, const doublePhaseFunc, const doubleEntryPhaseFunc, const bool ), const DOM::Entry & e, doublePhaseFunc p, doubleEntryPhaseFunc q=0, const bool pad=false ) : _f(f), _e(e), _p(p), _q(q), _pad(pad) {}
	private:
	    ostream& (*_f)( ostream&, const DOM::Entry&, const doublePhaseFunc, const doubleEntryPhaseFunc, const bool );
	    const DOM::Entry & _e;
	    const doublePhaseFunc _p;
	    const doubleEntryPhaseFunc _q;
	    const bool _pad;
	    friend ostream& operator<<(ostream & os, const ResultsManip& m ) { return m._f(os,m._e,m._p,m._q,m._pad); }
	};

	class ResultsConfidenceManip {
	public:
	    ResultsConfidenceManip( ostream& (*f)(ostream&, const DOM::Entry&, const doublePhaseFunc, const doubleEntryPhaseFunc, const ConfidenceIntervals*, const bool ), const DOM::Entry & e, const doublePhaseFunc p, const doubleEntryPhaseFunc q, const ConfidenceIntervals* c, const bool pad=false ) : _f(f), _e(e), _p(p), _q(q), _c(c), _pad(pad) {}
	private:
	    ostream& (*_f)( ostream&, const DOM::Entry&, const doublePhaseFunc, const doubleEntryPhaseFunc, const ConfidenceIntervals*, const bool );
	    const DOM::Entry & _e;
	    const doublePhaseFunc _p;
	    const doubleEntryPhaseFunc _q;
	    const ConfidenceIntervals* _c;
	    const bool _pad;
	    friend ostream& operator<<(ostream & os, const ResultsConfidenceManip& m ) { return m._f(os,m._e,m._p,m._q,m._c,m._pad); }
	};

	class StringManip {
	public:
	    StringManip( ostream& (*f)(ostream&, const char * ), const char * s ) : _f(f), _s(s) {}
	private:
	    ostream& (*_f)( ostream&, const char * );
	    const char * _s;
	    friend ostream& operator<<(ostream & os, const StringManip& m ) { return m._f(os,m._s); }
	};

	class TaskManip {
	public:
	    TaskManip( ostream& (*f)(ostream&, const DOM::Task& ), const DOM::Task& t ) : _f(f), _t(t) {}
	private:
	    ostream& (*_f)( ostream&, const DOM::Task& );
	    const DOM::Task& _t;
	    friend ostream& operator<<(ostream & os, const TaskManip& m ) { return m._f(os,m._t); }
	};

	class TaskResultsManip {
	public:
	    TaskResultsManip( ostream& (*f)(ostream&, const DOM::Task&, const doubleTaskFunc, const bool ), const DOM::Task & t, const doubleTaskFunc p, const bool pad=false ) : _f(f), _t(t), _p(p), _pad(pad) {}
	private:
	    ostream& (*_f)( ostream&, const DOM::Task&, const doubleTaskFunc, const bool );
	    const DOM::Task & _t;
	    const doubleTaskFunc _p;
	    const bool _pad;
	    friend ostream& operator<<(ostream & os, const TaskResultsManip& m ) { return m._f(os,m._t,m._p,m._pad); }
	};

	class TaskResultsConfidenceManip {
	public:
	    TaskResultsConfidenceManip( ostream& (*f)(ostream&, const DOM::Task&, const doubleTaskFunc, const ConfidenceIntervals*, const bool ), const DOM::Task & t, const doubleTaskFunc p, const ConfidenceIntervals* c, const bool pad=false ) : _f(f), _t(t), _p(p), _c(c), _pad(pad) {}
	private:
	    ostream& (*_f)( ostream&, const DOM::Task&, const doubleTaskFunc, const ConfidenceIntervals*, const bool );
	    const DOM::Task & _t;
	    const doubleTaskFunc _p;
	    const ConfidenceIntervals* _c;
	    const bool _pad;
	    friend ostream& operator<<(ostream & os, const TaskResultsConfidenceManip& m ) { return m._f(os,m._t,m._p,m._c,m._pad); }
	};

	class TimeManip {
	public:
	    TimeManip( ostream& (*f)(ostream&, const clock_t ), const clock_t t ) : _f(f), _t(t) {}
	private:
	    ostream& (*_f)( ostream&, const clock_t );
	    const clock_t _t;
	    friend ostream& operator<<(ostream & os, const TimeManip& m ) { return m._f(os,m._t); }
	};

	class UnsignedManip {
	public:
	    UnsignedManip( ostream& (*f)(ostream&, const unsigned int ), const unsigned int n ) : _f(f), _n(n) {}
	private:
	    ostream& (*_f)( ostream&, const unsigned int );
	    const unsigned int _n;
	    friend ostream& operator<<(ostream & os, const UnsignedManip& m ) { return m._f(os,m._n); }
	};

	class ForPhase {
	public:
	    ForPhase();
	    const DOM::Call*& operator[](const unsigned ix) { assert( ix && ix <= DOM::Phase::MAX_PHASE ); return ia[ix-1]; }
	    const DOM::Call* operator[](const unsigned ix) const { assert( ix && ix <= DOM::Phase::MAX_PHASE ); return ia[ix-1]; }
		
	    ForPhase& setMaxPhase( const unsigned mp ) { _maxPhase = mp; return *this; }
	    const unsigned getMaxPhase() const { return _maxPhase; }
	    ForPhase& setType( const DOM::Call::CallType type ) { _type = type; return *this; }
	    const DOM::Call::CallType getType() const { return _type; }

	private:
	    const DOM::Call * ia[DOM::Phase::MAX_PHASE];
	    unsigned _maxPhase;
	    DOM::Call::CallType _type;
	};

	/* ------------------------------------------------------------------------ */

	class Output {
	public:
	    Output( const DOM::Document&, const map<unsigned, DOM::Entity *>& entities, bool print_confidence_intervals=true, bool print_variances=true, bool print_histograms=true );
	    virtual ~Output();
	    virtual ostream& print( ostream& output ) const;

	protected:
	    virtual ostream& printPreamble( ostream& output ) const;
	    virtual ostream& printInput( ostream& output ) const;
	    virtual ostream& printResults( ostream& output ) const;
	    virtual ostream& printPostamble( ostream& output ) const { return output; }

	private:
	    Output( const Output& );
	    Output& operator=( const Output& );

	protected:
	    const DOM::Document& getDOM() const { return _document; }
	    static ostream& newline( ostream& output );
	    static ostream& textbf( ostream& output );
	    static ostream& textrm( ostream& output );
	    static ostream& textit( ostream& output );

	protected:
	    const DOM::Document& _document;
	    const map<unsigned, DOM::Entity *>& _entities;

	private:
	    const bool _print_variances;
	    const bool _print_histograms;

	public:
	    static UnsignedManip phase_header( const unsigned n );
	    static StringManip hold_header( const char * s );
	    static StringManip rwlock_header( const char * s );
	    static StringManip call_header( const char * s );

	private:
	    static StringManip task_header( const char * s );
	    static StringManip entry_header( const char * s );
	    static StringManip activity_header( const char * s );

	    static ostream& callHeader( ostream& output, const char * s );
	    static ostream& taskHeader( ostream& output, const char * s );
	    static ostream& entryHeader( ostream& output, const char * s );
	    static ostream& activityHeader( ostream& output, const char * s );
	    static ostream& phaseHeader( ostream& output, unsigned int n );
	    static ostream& holdHeader( ostream& output, const char * s );
	    static ostream& rwlockHeader( ostream& output, const char * s );
	};

	class Parseable : public Output {

	public:
	    Parseable( const DOM::Document&, const map<unsigned, DOM::Entity *>& entities, bool print_confidence_intervals=true );
	    virtual ~Parseable();
	    virtual ostream& print( ostream& output ) const;

	private:
	    Parseable( const Parseable& );
	    Parseable& operator=( const Parseable& );

	protected:
	    virtual ostream& printPreamble( ostream& output ) const;
	    virtual ostream& printResults( ostream& output ) const;
	};

	class RTF : public Output {
	public:
	    RTF( const DOM::Document&, const map<unsigned, DOM::Entity *>& entities, bool print_confidence_intervals=true );
	    virtual ~RTF();

	private:
	    RTF( const RTF& );
	    RTF& operator=( const RTF& );
	    
	protected:
	    virtual ostream& printPreamble( ostream& output ) const;
	    virtual ostream& printPostamble( ostream& output ) const;
	};

	class Input {
	public:
	    Input( const DOM::Document&, const map<unsigned, DOM::Entity *>& entities, bool  );
	    virtual ~Input();

	private:
	    Input( const Input& );
	    Input& operator=( const Input& );

	public:
	    virtual ostream& print( ostream& output ) const;

	private:
	    ostream& printGeneral( ostream& output ) const;
	    static bool is_processor( const pair<unsigned, DOM::Entity *>& );
	    static bool is_task( const pair<unsigned, DOM::Entity *>& );

	protected:
	    const DOM::Document& _document;
	    const map<unsigned, DOM::Entity *>& _entities;

	private:
	    const bool _annotate;
	};

	/* ------------------------------------------------------------------------ */

	class ObjectOutput {
	    friend class Output;
	    friend class Parseable;
	    friend class RTF;

	public:
	    ObjectOutput( ostream& output ) : _output(output) {}
	    virtual ~ObjectOutput() {}

	protected:
	    ObjectOutput& operator=( const ObjectOutput& );

	    static ostream& phaseResults( ostream& output, const DOM::Entry& e, const doublePhaseFunc f, const doubleEntryPhaseFunc g, const bool pad );
	    static ostream& phaseResultsConfidence( ostream& output, const DOM::Entry & e, const doublePhaseFunc f, const doubleEntryPhaseFunc g, const ConfidenceIntervals * conf, const bool pad );

	protected:
	    static EntityNameManip entity_name( const DOM::Entity& anEntity, bool& );
	    static EntryNameManip entry_name( const DOM::Entry& );
	    static UnsignedManip activity_separator( const unsigned int offset );
	    static ConfidenceManip conf_level( const unsigned int, const ConfidenceIntervals::confidence_level_t );
	    static TimeManip print_time( const clock_t t );
	    static DoubleManip colour( const double );

	    static ostream& taskPhaseResults( ostream& output, const DOM::Task & task, const doubleTaskFunc func, const bool pad );
	    static ostream& task3PhaseResults( ostream& output, const DOM::Task & task, const doubleTaskFunc func, const bool pad );
	    static ostream& taskPhaseResultsConfidence( ostream& output, const DOM::Task & task, const doubleTaskFunc func, const ConfidenceIntervals * conf, const bool pad );
	    static ostream& task3PhaseResultsConfidence( ostream& output, const DOM::Task & task, const doubleTaskFunc func, const ConfidenceIntervals * conf, const bool pad );

	protected:
	    static ostream& printEntityName( ostream& output, const DOM::Entity& entity, bool& print );	/* Print name of parent if print == true */
	    static ostream& printEntryName( ostream& output, const DOM::Entry& entry );
	    static ostream& activitySeparator( ostream& output, const unsigned n );
	    static ostream& confLevel( ostream& output, const unsigned int fill, const ConfidenceIntervals::confidence_level_t level );
	    static ostream& entryInfo( ostream& output, const DOM::Entry& e, const entryFunc f );
	    static ostream& phaseInfo( ostream& output, const DOM::Entry& e, const phaseFunc f );
	    static ostream& phaseTypeInfo( ostream& output, const DOM::Entry& e, const phaseTypeFunc f );
	    static ostream& entryHistogramInfo( ostream& output, const DOM::Entry& e, const histogramEntryFunc f );

	    static ostream& newline( ostream& output ) { if ( __rtf ) { output << "\\"; } output << endl; return output; }
	    static ostream& textrm( ostream& output ) { if ( __rtf ) { output << "\\f0 "; } return output; }
	    static ostream& textbf( ostream& output ) { if ( __rtf ) { output << "\\f1 "; } return output; }
	    static ostream& textit( ostream& output ) { if ( __rtf ) { output << "\\f2 "; } return output; }
	    static ostream& activityEOF( ostream& output ) { if ( __parseable ) { output << "-1 "; } return output; }

	private:
	    static ostream& printTime( ostream& output, const clock_t dtime );
	    static ostream& textcolour( ostream& output, const double );

	public:
	    static const unsigned int __maxStrLen = 16;		/* Global for print functions */
	    static const unsigned int __maxDblLen = 12;

	protected:
	    ostream& _output;

	    static bool __task_has_think_time;
	    static bool __task_has_group;
	    static unsigned int __maxPhase;
	    static ConfidenceIntervals * __conf95;
	    static ConfidenceIntervals * __conf99;
	    static bool __parseable;
	    static bool __rtf;
	    static bool __coloured;
	};


	class ObjectInput {
	    friend class Input;

	public:
	    ObjectInput( ostream& output ) : _output(output) {}
	    virtual ~ObjectInput() {}
	    
	    static CallManip number_of_calls( const DOM::Call& call ) { return CallManip( &ObjectInput::printNumberOfCalls, call ); }
	    static CallManip call_type( const DOM::Call& call ) { return CallManip( &ObjectInput::printCallType, call ); }

	private:
	    ObjectInput& operator=( const ObjectInput& );

	protected:
	    static ostream& printNumberOfCalls( ostream& output, const DOM::Call& );
	    static ostream& printCallType( ostream& output, const DOM::Call& );

	protected:
	    ostream& _output;

	    static unsigned int __maxInpLen;			/* for lqn2lqn formatting */
	    static unsigned int __maxEntLen;			/* Entry name */
	};

	/* ------------------------------------------------------------------------ */

	class DocumentOutput : public ObjectOutput {
	public:
	    DocumentOutput( ostream& output ) : ObjectOutput(output) {}
	    virtual void operator()( const DOM::Document& ) const;
	};

	/* ------------------------------------------------------------------------ */

	class EntityOutput : public ObjectOutput {
	public:
	    EntityOutput( ostream& output ) : ObjectOutput(output) {}
	    virtual void operator()( const pair<unsigned, DOM::Entity *>& ) const = 0;

	protected:
	    virtual void printParameters( const DOM::Entity& ) const;
	};


	class EntityInput : public ObjectInput {
	public:
	    EntityInput( ostream& output ) : ObjectInput( output ) {}
	    virtual void operator()( const pair<unsigned, DOM::Entity *>& ) const = 0;

	    static ostream& print( ostream& output, const DOM::Entity* e );

	protected:
	    static EntityManip copies_of( const DOM::Entity & anEntity );
	    static EntityManip replicas_of( const DOM::Entity & anEntity );

	private:
	    static ostream& printCopies( ostream& output, const DOM::Entity& e );
	    static ostream& printReplicas( ostream& output, const DOM::Entity& e );
	};

	/* ------------------------------------------------------------------------ */

	class ProcessorOutput : public EntityOutput  {
	protected:
	    typedef void (ProcessorOutput::*voidProcessorFunc)( const DOM::Processor& ) const;

	    static ResultsManip processor_queueing_time( const DOM::Entry& e );
	    static ResultsConfidenceManip processor_queueing_time_confidence( const DOM::Entry& e, const ConfidenceIntervals * c );

	    static ostream& printScheduling( ostream& output, const DOM::Processor& p );

	public:
	    ProcessorOutput( ostream& output, voidProcessorFunc f ) : EntityOutput(output), _func(f) {}
	    void operator()( const pair<unsigned,DOM::Entity *>& ) const;

	    void printParameters( const DOM::Processor& ) const;
	    void printUtilizationAndWaiting( const DOM::Processor& ) const;
	    void printUtilizationAndWaiting( const DOM::Processor&, const vector<DOM::Task*>& tasks ) const;

	private:
	    const voidProcessorFunc _func;
	};


	class ProcessorInput : public EntityInput  {
	protected:
	    typedef void (ProcessorInput::*voidProcessorFunc)( const DOM::Processor& ) const;

	    static ProcessorManip scheduling_of( const DOM::Processor& p ) { return SRVN::ProcessorManip( &SRVN::ProcessorInput::printScheduling, p ); }
	    static ProcessorManip rate_of( const DOM::Processor& p ) { return SRVN::ProcessorManip( &SRVN::ProcessorInput::printRate, p ); }

	    static ostream& printRate( ostream& output, const DOM::Processor& p );
	    static ostream& printScheduling( ostream& output, const DOM::Processor& p );

	public:
	    ProcessorInput( ostream& output, voidProcessorFunc f ) : EntityInput(output), _func(f) {}
	    void operator()( const pair<unsigned, DOM::Entity *>& ) const;

	    void print( const DOM::Processor& ) const;

	private:
	    const voidProcessorFunc _func;
	};

	/* ------------------------------------------------------------------------ */

	class GroupOutput : public ObjectOutput  {
	protected:
	    typedef void (GroupOutput::*voidGroupFunc)( const LQIO::DOM::Group& ) const;

	public:
	    GroupOutput( ostream& output, voidGroupFunc f ) : ObjectOutput(output), _func(f) {}
	    void operator()( const pair<const string,LQIO::DOM::Group*>& ) const;

	    void printParameters( const LQIO::DOM::Group& ) const;

	private:
	    voidGroupFunc _func;
	};

	class GroupInput : public ObjectInput  {
	protected:
	    typedef void (GroupInput::*voidGroupFunc)( const LQIO::DOM::Group& ) const;

	public:
	    GroupInput( ostream& output, voidGroupFunc f ) : ObjectInput(output), _func(f) {}
	    void operator()( const pair<const string,LQIO::DOM::Group*>& ) const;

	    void printParameters( const LQIO::DOM::Group& ) const;
	    void print( const DOM::Group& ) const;

	private:
	    voidGroupFunc _func;
	};

	/* ------------------------------------------------------------------------ */

	class TaskOutput : public EntityOutput {
	protected:
	    typedef void (TaskOutput::*voidTaskFunc)( const DOM::Task& ) const;

	private:
	    static ResultsManip entry_utilization( const DOM::Entry& e );
	    static TaskResultsManip task_utilization( const DOM::Task& t );
	    static ResultsConfidenceManip entry_utilization_confidence( const DOM::Entry& e, const ConfidenceIntervals * c );
	    static TaskResultsConfidenceManip task_utilization_confidence( const DOM::Task& t, const ConfidenceIntervals * c );
	    static TaskResultsManip holding_time( const DOM::Task& t );
	    static TaskResultsConfidenceManip holding_time_confidence( const DOM::Task& t, const ConfidenceIntervals * c );
	    static TaskResultsManip holding_time_variance( const DOM::Task& t );
	    static TaskResultsConfidenceManip holding_time_variance_confidence( const DOM::Task& t, const ConfidenceIntervals * c );

	public:
	    TaskOutput( ostream& output, voidTaskFunc f ) : EntityOutput(output), _func(f) {}
	    void operator()( const pair<unsigned, DOM::Entity *>& ) const;

	    void printParameters( const DOM::Task& ) const;
	    void printScheduling( const DOM::Task& ) const;
	    void printThroughputAndUtilization( const DOM::Task& ) const;
	    void printJoinDelay( const DOM::Task& ) const;
	    void printHoldTime( const DOM::Task& ) const;
	    void printRWLOCKHoldTime( const DOM::Task& ) const;

	private:
	    const voidTaskFunc _func;
	};


	class TaskInput : public EntityInput {
	protected:
	    typedef void (TaskInput::*voidTaskFunc)( const DOM::Task& ) const;

	protected:
	    static TaskManip scheduling_of( const DOM::Task & t )  { return SRVN::TaskManip( &SRVN::TaskInput::printScheduling, t ); }
	    static TaskManip copies_of( const DOM::Task & t )      { return SRVN::TaskManip( &SRVN::TaskInput::printCopies, t ); }
	    static TaskManip entries_of( const DOM::Task& t )      { return SRVN::TaskManip( &SRVN::TaskInput::printEntryList, t ); }
	    static TaskManip group_of( const DOM::Task& t )        { return SRVN::TaskManip( &SRVN::TaskInput::printGroup, t ); }
	    static TaskManip priority_of( const DOM::Task& t )     { return SRVN::TaskManip( &SRVN::TaskInput::printPriority, t ); }
	    static TaskManip queue_length_of( const DOM::Task& t ) { return SRVN::TaskManip( &SRVN::TaskInput::printQueueLength, t ); }
	    static TaskManip think_time_of( const DOM::Task & t )  { return SRVN::TaskManip( &SRVN::TaskInput::printThinkTime, t ); }

	private:
	    static ostream& printCopies( ostream& output, const DOM::Task& t );
	    static ostream& printEntryList( ostream& output,  const DOM::Task& t );
	    static ostream& printGroup( ostream& output,  const DOM::Task& t );
	    static ostream& printPriority( ostream& output,  const DOM::Task& t );
	    static ostream& printQueueLength( ostream& output,  const DOM::Task& t );
	    static ostream& printScheduling( ostream& output, const DOM::Task& t );
	    static ostream& printThinkTime( ostream& output,  const DOM::Task & t );

	public:
	    TaskInput( ostream& output, voidTaskFunc f ) : EntityInput(output), _func(f) {}
	    void operator()( const pair<unsigned, DOM::Entity *>& ) const;

	    void print( const DOM::Task& ) const;
	    void printEntryInput( const DOM::Task& ) const;
	    void printActivityInput( const DOM::Task& ) const;

	private:
	    const voidTaskFunc _func;
	};

	/* ------------------------------------------------------------------------ */

	class EntryOutput : public ObjectOutput {
	protected:
	    typedef void (EntryOutput::*voidEntryFunc)( const DOM::Entry&, const DOM::Entity&, bool& ) const;
	    typedef void (EntryOutput::*voidActivityFunc)( const DOM::Activity& ) const;
	    typedef bool (EntryOutput::*testActivityFunc)( const DOM::Activity& ) const;

	    static PhaseManip coefficient_of_variation( const DOM::Entry& e );
	    static ResultsManip max_service_time( const DOM::Entry& e );
	    static EntryManip open_arrivals( const DOM::Entry& e );
	    static PhaseTypeManip phase_type( const DOM::Entry& e );
	    static PhaseManip service_demand( const DOM::Entry& e );
	    static PhaseManip think_time( const DOM::Entry& e );

	public:
	    EntryOutput( ostream& output, const voidEntryFunc ef, const voidActivityFunc af=0, const testActivityFunc tf=0 ) : ObjectOutput(output), _entryFunc(ef), _activityFunc(af), _testFunc(tf) {}
	    virtual void operator()( const pair<unsigned, DOM::Entity *>& ) const;

	    /* Input values */

	    void printEntryCoefficientOfVariation( const DOM::Entry &entry, const DOM::Entity &entity, bool& print ) const;
	    void printEntryDemand( const DOM::Entry &entry, const DOM::Entity &entity, bool& print ) const;
	    void printEntryHistory( const DOM::Entry &entry, const DOM::Entity &entity, bool& print ) const;
	    void printEntryMaxServiceTime( const DOM::Entry &entry, const DOM::Entity &entity, bool& print ) const;
	    void printEntryPhaseType( const DOM::Entry &entry, const DOM::Entity &entity, bool& print ) const;
	    void printEntryThinkTime( const DOM::Entry &entry, const DOM::Entity &entity, bool& print ) const;
	    void printOpenArrivals( const DOM::Entry &entry, const DOM::Entity &entity, bool& print ) const;
	    void printForwarding( const DOM::Entry &entry, const DOM::Entity &entity, bool& print ) const;

	    void printActivityCoefficientOfVariation( const DOM::Activity &activity ) const;
	    void printActivityDemand( const DOM::Activity &activity ) const;
	    void printActivityMaxServiceTime( const DOM::Activity &activity ) const;
	    void printActivityPhaseType( const DOM::Activity &activity ) const;
	    void printActivityThinkTime( const DOM::Activity &activity ) const;

	    /* Results */

	    void printEntryThroughputBounds( const DOM::Entry &entry, const DOM::Entity &entity, bool& print ) const;
	    void printEntryServiceTime( const DOM::Entry &entry, const DOM::Entity &entity, bool& print ) const;
	    void printEntryMaxServiceTimeExceeded( const DOM::Entry &entry, const DOM::Entity &entity, bool& print ) const;
	    void printEntryVarianceServiceTime( const DOM::Entry &entry, const DOM::Entity &entity, bool& print ) const;
	    void printOpenQueueWait( const DOM::Entry &entry, const DOM::Entity &entity, bool& print ) const;

	    bool testActivityMaxServiceTimeExceeded( const DOM::Activity &activity ) const;
	    void printActivityServiceTime( const DOM::Activity &activity ) const;
	    void printActivityMaxServiceTimeExceeded( const DOM::Activity &activity ) const;
	    void printActivityVarianceServiceTime( const DOM::Activity &activity ) const;

	protected:
	    static ResultsManip service_time( const DOM::Entry& e );
	    static ResultsManip service_time_exceeded( const DOM::Entry& e );
	    static ResultsConfidenceManip service_time_confidence( const DOM::Entry& e, const ConfidenceIntervals * c );
	    static ResultsConfidenceManip service_time_exceeded_confidence( const DOM::Entry& e, const ConfidenceIntervals * c );
	    static ResultsManip variance_service_time( const DOM::Entry& e );
	    static ResultsConfidenceManip variance_service_time_confidence( const DOM::Entry& e, const ConfidenceIntervals * c );

	private:
	    void printActivity( const DOM::Activity &activity, const activityFunc func ) const;
	    void activityResults( const DOM::Activity &activity, const doubleActivityFunc mean, const doubleActivityFunc var ) const;
	    bool testForActivityResults( const DOM::Activity &activity, boolActivityFunc tf ) const;

	private:
	    const voidEntryFunc _entryFunc;
	    const voidActivityFunc _activityFunc;
	    const testActivityFunc _testFunc;
	};

	class EntryInput : public ObjectInput {
	protected:
	    typedef void (EntryInput::*voidEntryFunc)( const DOM::Entry& ) const;

	public:
	    EntryInput( ostream& output, const voidEntryFunc ef ) : ObjectInput(output), _entryFunc(ef) {}
	    virtual void operator()( const DOM::Entry * ) const;

	    void print( const DOM::Entry& e ) const;
	    void printForwarding( const DOM::Entry& e ) const;
	    void printCalls( const DOM::Entry& e ) const;

	private:
	    const voidEntryFunc _entryFunc;
	};

	class PhaseInput : public ObjectInput {
	protected:
	    typedef void (PhaseInput::*voidPhaseFunc)( const LQIO::DOM::Phase& ) const;

	    PhaseInput( ostream& output ) : ObjectInput( output ), _func(0) {}

	public:
	    PhaseInput( ostream& output, voidPhaseFunc f ) : ObjectInput(output), _func(f) {}
	    virtual void operator()( const pair<unsigned,DOM::Phase *>& ) const;

	    void printCoefficientOfVariation( const DOM::Phase& p ) const;
	    void printMaxServiceTimeExceeded( const DOM::Phase& ) const;
	    void printPhaseFlag( const DOM::Phase& ) const; 
	    void printServiceTime( const DOM::Phase& ) const;
	    void printThinkTime( const DOM::Phase& ) const;


	private:
	    const voidPhaseFunc _func;
	};

	class ActivityInput : public PhaseInput {
	protected:
	    typedef void (ActivityInput::*voidActivityFunc)( const LQIO::DOM::Activity& ) const;

	public:
	    ActivityInput( ostream& output, voidActivityFunc f ) : PhaseInput(output), _func(f) {}
	    virtual void operator()( const pair<string,DOM::Activity *>& ) const;

	    void print( const DOM::Activity& a ) const;
	    void printCalls( const DOM::Activity& a ) const;

	private:
	    const voidActivityFunc _func;
	};

	class ActivityListInput : public ObjectInput {
	protected:
	    typedef void (ActivityListInput::*voidActivityListFunc)( const LQIO::DOM::ActivityList& ) const;

	public:
	    ActivityListInput( ostream& output, voidActivityListFunc f, const unsigned int n ) : ObjectInput(output), _func(f), _size(n), _count(0) {}
	    virtual void operator()( const DOM::ActivityList * ) const;

	    void print( const DOM::ActivityList& a ) const;

	private:
	    void printPreList( const DOM::ActivityList& precedence ) const;
	    void printPostList( const DOM::ActivityList& precedence ) const;

	private:
	    const voidActivityListFunc _func;
	    const unsigned int _size;
	    unsigned int _count;
	};

	class CallOutput : public ObjectOutput  {

	protected:
	    typedef void (CallOutput::*voidCallFunc)( const DOM::Call * call ) const;
	    typedef void (CallOutput::*callConfFPtr)( const DOM::Call * call, const ConfidenceIntervals* ) const;

	    class CallResultsManip {
	    public:
		CallResultsManip( ostream& (*f)(ostream&, const CallOutput&, const ForPhase&, const callConfFPtr, const ConfidenceIntervals* ), const CallOutput & c, const ForPhase& p, const callConfFPtr x, const ConfidenceIntervals* l=0 ) : _f(f), _c(c), _p(p), _x(x), _conf(l) {}
	    private:
		ostream& (*_f)( ostream&, const CallOutput&, const ForPhase&, const callConfFPtr, const ConfidenceIntervals* );
		const CallOutput& _c; 
		const ForPhase& _p;
		const callConfFPtr _x;
		const ConfidenceIntervals* _conf;
		friend ostream& operator<<(ostream & os, const CallResultsManip& m ) { return m._f(os,m._c,m._p,m._x,m._conf); }
	    };

	    static CallResultsManip print_calls( const CallOutput&, const ForPhase&c, const callConfFPtr f, const ConfidenceIntervals* l=0 );

	public:
	    CallOutput( ostream& output, const boolCallFunc t, const callConfFPtr m, const callConfFPtr v=0 ) : ObjectOutput(output), _testFunc(t), _meanFunc(m), _confFunc(v) {}
	    virtual void operator()( const pair<unsigned, DOM::Entity *>& ) const;

	public:
	    void printCallRate( const DOM::Call * call, const ConfidenceIntervals* ) const;
	    void printCallWaiting( const DOM::Call * call, const ConfidenceIntervals* ) const;
	    void printCallWaitingConfidence( const DOM::Call * call, const ConfidenceIntervals* ) const;
	    void printCallVarianceWaiting( const DOM::Call * call, const ConfidenceIntervals* ) const;
	    void printCallVarianceWaitingConfidence( const DOM::Call * call, const ConfidenceIntervals* ) const;
	    void printDropProbability( const DOM::Call * call, const ConfidenceIntervals* ) const;
	    void printDropProbabilityConfidence( const DOM::Call * call, const ConfidenceIntervals* conf ) const;

	private:
	    static ostream& printCalls( ostream&, const CallOutput&c, const ForPhase& p, const callConfFPtr f, const ConfidenceIntervals* l );

	private:
	    const boolCallFunc _testFunc;
	    const callConfFPtr _meanFunc;
	    const callConfFPtr _confFunc;
	};

	/* ------------------------------------------------------------------------ */

	class HistogramOutput : public ObjectOutput {
	protected:
	    typedef void (HistogramOutput::*histogramFunc)( const DOM::Histogram& ) const;

	public:
	    HistogramOutput( ostream& output, const histogramFunc func ) : ObjectOutput(output), _func(func) {}
	    virtual void operator()( const pair<unsigned, DOM::Entity *>& ) const;

	public:
	    void printHistogram( const DOM::Histogram& ) const;

	private:
	    const histogramFunc _func;
	};
    }
}

inline std::ostream& operator<<( std::ostream& output, const LQIO::SRVN::Output& self ) { return self.print( output ); }
inline std::ostream& operator<<( std::ostream& output, const LQIO::SRVN::Parseable& self ) { return self.print( output ); }
#endif /* __SRVN_OUTPUT_H */
