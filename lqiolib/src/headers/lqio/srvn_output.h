/* -*- c++ -*- */
/************************************************************************/
/* Copyright the Real-Time and Distributed Systems Group,		*/
/* Department of Systems and Computer Engineering,			*/
/* Carleton University, Ottawa, Ontario, Canada. K1S 5B6		*/
/* 									*/
/* 2009.								*/
/************************************************************************/

/*
 * $Id: srvn_output.h 13806 2020-08-27 18:05:17Z greg $
 *
 * This class is used to hide the methods used to output to the Xerces DOM.
 */

#ifndef __SRVN_OUTPUT_H
#define __SRVN_OUTPUT_H

#include <vector>
#include <set>
#include <map>
#include <ostream>
#include <cassert>
#include <sys/time.h>
#include "dom_processor.h"
#include "dom_task.h"
#include "dom_phase.h"
#include "dom_entry.h"
#include "dom_call.h"
#include "input.h"
#include "common_io.h"
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

    namespace SRVN {

	typedef const DOM::ExternalVariable* (DOM::Activity::*activityFunc)() const;
	typedef double (DOM::Activity::*doubleActivityFunc)() const;
	typedef bool (DOM::Activity::*boolActivityFunc)() const;

	typedef const DOM::ExternalVariable* (DOM::Call::*callFunc)() const;

	typedef const DOM::ExternalVariable* (DOM::Phase::*phaseFunc)() const;
	typedef double (DOM::Phase::*doublePhaseFunc)() const;
	typedef double (DOM::Call::*doubleCallFunc)() const;
	typedef double (DOM::Entry::*doubleEntryPhaseFunc)( const unsigned ) const;
	typedef const DOM::Histogram* (DOM::Entry::*histogramEntryFunc)( const unsigned ) const;
	typedef phase_type (DOM::Phase::*phaseTypeFunc)() const;

	typedef const DOM::ExternalVariable* (DOM::Entry::*entryFunc)() const;

	typedef double (DOM::Task::*doubleTaskFunc)( const unsigned ) const;

	class CallManip {
	public:
	    CallManip( std::ostream& (*f)(std::ostream&, const DOM::Call* ), const DOM::Call* c ) : _f(f), _c(c) {}
	private:
	    std::ostream& (*_f)( std::ostream&, const DOM::Call* );
	    const DOM::Call* _c;
	    friend std::ostream& operator<<(std::ostream & os, const CallManip& m ) { return m._f(os,m._c); }
	};

	class ConfidenceManip {
	public:
	    ConfidenceManip( std::ostream& (*f)(std::ostream&, const unsigned, const ConfidenceIntervals::confidence_level_t ), const unsigned s, const ConfidenceIntervals::confidence_level_t l  ) : _f(f), _s(s), _l(l) {}
	private:
	    std::ostream& (*_f)( std::ostream&, const unsigned, const ConfidenceIntervals::confidence_level_t );
	    const unsigned _s;
	    const ConfidenceIntervals::confidence_level_t _l;
	    friend std::ostream& operator<<(std::ostream & os, const ConfidenceManip& m ) { return m._f(os,m._s,m._l); }
	};

	class DoubleDoubleManip {
	public:
	    DoubleDoubleManip( std::ostream& (*f)(std::ostream&, const double, const double ), const double a, const double b ) : _f(f), _a(a), _b(b) {}
	private:
	    std::ostream& (*_f)( std::ostream&, const double, const double );
	    const double _a;
	    const double _b;
	    friend std::ostream& operator<<(std::ostream & os, const DoubleDoubleManip& m ) { return m._f(os,m._a,m._b); }
	};

	class EntityManip {
	public:
	    EntityManip( std::ostream& (*f)(std::ostream&, const DOM::Entity& ), const DOM::Entity& e ) : _f(f), _e(e) {}
	private:
	    std::ostream& (*_f)( std::ostream&, const DOM::Entity& );
	    const DOM::Entity& _e;
	    friend std::ostream& operator<<(std::ostream & os, const EntityManip& m ) { return m._f(os,m._e); }
	};

	class EntityNameManip {
	public:
	    EntityNameManip( std::ostream& (*f)(std::ostream&, const DOM::Entity&, bool& ), const DOM::Entity& e, bool& b ) : _f(f), _e(e), _b(b) {}
	private:
	    std::ostream& (*_f)( std::ostream&, const DOM::Entity&, bool& );
	    const DOM::Entity& _e;
	    bool& _b;
	    friend std::ostream& operator<<(std::ostream & os, const EntityNameManip& m ) { return m._f(os,m._e,m._b); }
	};

	class EntryManip {
	public:
	    EntryManip( std::ostream& (*f)(std::ostream&, const DOM::Entry&, const entryFunc ), const DOM::Entry & e, const entryFunc p ) : _f(f), _e(e), _p(p) {}
	private:
	    std::ostream& (*_f)( std::ostream&, const DOM::Entry&, const entryFunc );
	    const DOM::Entry & _e;
	    const entryFunc _p;
	    friend std::ostream& operator<<(std::ostream & os, const EntryManip& m ) { return m._f(os,m._e,m._p); }
	};

	class EntryNameManip {
	public:
	    EntryNameManip( std::ostream& (*f)(std::ostream&, const DOM::Entry& ), const DOM::Entry& e ) : _f(f), _e(e) {}
	private:
	    std::ostream& (*_f)( std::ostream&, const DOM::Entry& );
	    const DOM::Entry& _e;
	    friend std::ostream& operator<<(std::ostream & os, const EntryNameManip& m ) { return m._f(os,m._e); }
	};

	class PhaseManip {
	public:
	    PhaseManip( std::ostream& (*f)(std::ostream&, const DOM::Entry&, const phaseFunc ), const DOM::Entry & e, const phaseFunc p ) : _f(f), _e(e), _p(p) {}
	private:
	    std::ostream& (*_f)( std::ostream&, const DOM::Entry&, const phaseFunc );
	    const DOM::Entry & _e;
	    const phaseFunc _p;
	    friend std::ostream& operator<<(std::ostream & os, const PhaseManip& m ) { return m._f(os,m._e,m._p); }
	};

	class PhaseTypeManip {
	public:
	    PhaseTypeManip( std::ostream& (*f)(std::ostream&, const DOM::Entry&, const phaseTypeFunc ), const DOM::Entry & e, const phaseTypeFunc p ) : _f(f), _e(e), _p(p) {}
	private:
	    std::ostream& (*_f)( std::ostream&, const DOM::Entry&, const phaseTypeFunc );
	    const DOM::Entry & _e;
	    const phaseTypeFunc _p;
	    friend std::ostream& operator<<(std::ostream & os, const PhaseTypeManip& m ) { return m._f(os,m._e,m._p); }
	};

	class ProcessorManip {
	public:
	    ProcessorManip( std::ostream& (*f)(std::ostream&, const DOM::Processor& ), const DOM::Processor& p ) : _f(f), _p(p) {}
	private:
	    std::ostream& (*_f)( std::ostream&, const DOM::Processor& );
	    const DOM::Processor& _p;
	    friend std::ostream& operator<<(std::ostream & os, const ProcessorManip& m ) { return m._f(os,m._p); }
	};

	class ResultsManip {
	public:
	    ResultsManip( std::ostream& (*f)(std::ostream&, const DOM::Entry&, const doublePhaseFunc, const doubleEntryPhaseFunc, const bool ), const DOM::Entry & e, doublePhaseFunc p, doubleEntryPhaseFunc q=0, const bool pad=false ) : _f(f), _e(e), _p(p), _q(q), _pad(pad) {}
	private:
	    std::ostream& (*_f)( std::ostream&, const DOM::Entry&, const doublePhaseFunc, const doubleEntryPhaseFunc, const bool );
	    const DOM::Entry & _e;
	    const doublePhaseFunc _p;
	    const doubleEntryPhaseFunc _q;
	    const bool _pad;
	    friend std::ostream& operator<<(std::ostream & os, const ResultsManip& m ) { return m._f(os,m._e,m._p,m._q,m._pad); }
	};

	class ResultsConfidenceManip {
	public:
	    ResultsConfidenceManip( std::ostream& (*f)(std::ostream&, const DOM::Entry&, const doublePhaseFunc, const doubleEntryPhaseFunc, const ConfidenceIntervals*, const bool ), const DOM::Entry & e, const doublePhaseFunc p, const doubleEntryPhaseFunc q, const ConfidenceIntervals* c, const bool pad=false ) : _f(f), _e(e), _p(p), _q(q), _c(c), _pad(pad) {}
	private:
	    std::ostream& (*_f)( std::ostream&, const DOM::Entry&, const doublePhaseFunc, const doubleEntryPhaseFunc, const ConfidenceIntervals*, const bool );
	    const DOM::Entry & _e;
	    const doublePhaseFunc _p;
	    const doubleEntryPhaseFunc _q;
	    const ConfidenceIntervals* _c;
	    const bool _pad;
	    friend std::ostream& operator<<(std::ostream & os, const ResultsConfidenceManip& m ) { return m._f(os,m._e,m._p,m._q,m._c,m._pad); }
	};

	class TaskManip {
	public:
	    TaskManip( std::ostream& (*f)(std::ostream&, const DOM::Task& ), const DOM::Task& t ) : _f(f), _t(t) {}
	private:
	    std::ostream& (*_f)( std::ostream&, const DOM::Task& );
	    const DOM::Task& _t;
	    friend std::ostream& operator<<(std::ostream & os, const TaskManip& m ) { return m._f(os,m._t); }
	};

	class TaskResultsManip {
	public:
	    TaskResultsManip( std::ostream& (*f)(std::ostream&, const DOM::Task&, const doubleTaskFunc, const bool ), const DOM::Task & t, const doubleTaskFunc p, const bool pad=false ) : _f(f), _t(t), _p(p), _pad(pad) {}
	private:
	    std::ostream& (*_f)( std::ostream&, const DOM::Task&, const doubleTaskFunc, const bool );
	    const DOM::Task & _t;
	    const doubleTaskFunc _p;
	    const bool _pad;
	    friend std::ostream& operator<<(std::ostream & os, const TaskResultsManip& m ) { return m._f(os,m._t,m._p,m._pad); }
	};

	class TaskResultsConfidenceManip {
	public:
	    TaskResultsConfidenceManip( std::ostream& (*f)(std::ostream&, const DOM::Task&, const doubleTaskFunc, const ConfidenceIntervals*, const bool ), const DOM::Task & t, const doubleTaskFunc p, const ConfidenceIntervals* c, const bool pad=false ) : _f(f), _t(t), _p(p), _c(c), _pad(pad) {}
	private:
	    std::ostream& (*_f)( std::ostream&, const DOM::Task&, const doubleTaskFunc, const ConfidenceIntervals*, const bool );
	    const DOM::Task & _t;
	    const doubleTaskFunc _p;
	    const ConfidenceIntervals* _c;
	    const bool _pad;
	    friend std::ostream& operator<<(std::ostream & os, const TaskResultsConfidenceManip& m ) { return m._f(os,m._t,m._p,m._c,m._pad); }
	};

	class UnsignedManip {
	public:
	    UnsignedManip( std::ostream& (*f)(std::ostream&, const unsigned int ), const unsigned int n ) : _f(f), _n(n) {}
	private:
	    std::ostream& (*_f)( std::ostream&, const unsigned int );
	    const unsigned int _n;
	    friend std::ostream& operator<<(std::ostream & os, const UnsignedManip& m ) { return m._f(os,m._n); }
	};

	class IsSetAndGTManip {
	public:
	    IsSetAndGTManip( std::ostream& (*f)(std::ostream&, const std::string&, const DOM::ExternalVariable *, double ), const std::string& s, const DOM::ExternalVariable * v, double d ) : _f(f), _s(s), _v(v), _d(d) {}
	private:
	    std::ostream& (*_f)( std::ostream&, const std::string&, const DOM::ExternalVariable *, double );
	    const std::string& _s;
	    const DOM::ExternalVariable * _v;
	    double _d;
	    friend std::ostream& operator<<(std::ostream & os, const IsSetAndGTManip& m ) { return m._f(os,m._s,m._v,m._d); }
	};

	class ExtvarPrintingManip {
	public:
	    ExtvarPrintingManip( std::ostream& (*f)(std::ostream&, const DOM::ExternalVariable *, double ), const DOM::ExternalVariable * v, double d ) : _f(f), _v(v), _d(d) {}
	private:
	    std::ostream& (*_f)( std::ostream&, const DOM::ExternalVariable *, double );
	    const DOM::ExternalVariable * _v;
	    double _d;
	    friend std::ostream& operator<<(std::ostream & os, const ExtvarPrintingManip& m ) { return m._f(os,m._v,m._d); }
	};

	class Predicate {
	public:
	    typedef bool (DOM::Entry::*test)() const;
	    Predicate( test f ) : _f(f) {}

	    bool operator()( const std::pair<unsigned int,DOM::Entity *>& ) const;

	private:
	    const test _f;
	};

	/* ------------------------------------------------------------------------ */

	class Output {
	public:
	    Output( const DOM::Document&, const std::map<unsigned, DOM::Entity *>& entities, bool print_confidence_intervals=true, bool print_variances=true, bool print_histograms=true );
	    virtual ~Output();
	    virtual std::ostream& print( std::ostream& output ) const;

	protected:
	    virtual std::ostream& printPreamble( std::ostream& output ) const;
	    virtual std::ostream& printInput( std::ostream& output ) const;
	    virtual std::ostream& printResults( std::ostream& output ) const;
	    virtual std::ostream& printPostamble( std::ostream& output ) const { return output; }

	private:
	    Output( const Output& );
	    Output& operator=( const Output& );

	protected:
	    const DOM::Document& getDOM() const { return _document; }
	    static std::ostream& newline( std::ostream& output );
	    static std::ostream& textbf( std::ostream& output );
	    static std::ostream& textrm( std::ostream& output );
	    static std::ostream& textit( std::ostream& output );

	protected:
	    const DOM::Document& _document;
	    const std::map<unsigned, DOM::Entity *>& _entities;
	    const bool _print_variances;

	private:
	    const bool _print_histograms;

	public:
	    static UnsignedManip phase_header( const unsigned n )	  { return SRVN::UnsignedManip( &SRVN::Output::phaseHeader, n ); }
	    static DOM::StringManip hold_header( const std::string& s )	  { return DOM::StringManip( &SRVN::Output::holdHeader, s ); }
	    static DOM::StringManip rwlock_header( const std::string& s ) { return DOM::StringManip( &SRVN::Output::rwlockHeader, s ); }
	    static DOM::StringManip call_header( const std::string& s )   { return DOM::StringManip( &SRVN::Output::callHeader, s ); }

	private:
	    static DOM::StringManip task_header( const std::string& s )     { return DOM::StringManip( &SRVN::Output::taskHeader, s ); }    
	    static DOM::StringManip entry_header( const std::string& s )    { return DOM::StringManip( &SRVN::Output::entryHeader, s ); }   
	    static DOM::StringManip activity_header( const std::string& s ) { return DOM::StringManip( &SRVN::Output::activityHeader, s ); }

	    static std::ostream& callHeader( std::ostream& output, const std::string& s );
	    static std::ostream& taskHeader( std::ostream& output, const std::string& s );
	    static std::ostream& entryHeader( std::ostream& output, const std::string& s );
	    static std::ostream& activityHeader( std::ostream& output, const std::string& s );
	    static std::ostream& phaseHeader( std::ostream& output, unsigned int n );
	    static std::ostream& holdHeader( std::ostream& output, const std::string& s );
	    static std::ostream& rwlockHeader( std::ostream& output, const std::string& s );
	};

	class Parseable : public Output {

	public:
	    Parseable( const DOM::Document&, const std::map<unsigned, DOM::Entity *>& entities, bool print_confidence_intervals=true );
	    virtual ~Parseable();
	    virtual std::ostream& print( std::ostream& output ) const;

	private:
	    Parseable( const Parseable& );
	    Parseable& operator=( const Parseable& );

	protected:
	    virtual std::ostream& printPreamble( std::ostream& output ) const;
	    virtual std::ostream& printResults( std::ostream& output ) const;
	    static DOM::StringManip print_comment( const std::string& s )  { return DOM::StringManip( &SRVN::Parseable::printComment, s ); }

	private:
	    static std::ostream& printComment( std::ostream& output, const std::string& s );
	};

	class RTF : public Output {
	public:
	    RTF( const DOM::Document&, const std::map<unsigned, DOM::Entity *>& entities, bool print_confidence_intervals=true );
	    virtual ~RTF();

	private:
	    RTF( const RTF& );
	    RTF& operator=( const RTF& );
	    
	protected:
	    virtual std::ostream& printPreamble( std::ostream& output ) const;
	    virtual std::ostream& printPostamble( std::ostream& output ) const;
	};

	class Input {
	public:
	    Input( const DOM::Document&, const std::map<unsigned, DOM::Entity *>& entities, bool annotate );
	    virtual ~Input();

	private:
	    Input( const Input& );
	    Input& operator=( const Input& );

	public:
	    virtual std::ostream& print( std::ostream& output ) const;
	    static IsSetAndGTManip is_double_and_gt( const std::string& s, const DOM::ExternalVariable * v, double d )  { return IsSetAndGTManip( &SRVN::Input::doubleAndGreaterThan, s, v, d ); }
	    static IsSetAndGTManip is_integer_and_gt( const std::string& s, const DOM::ExternalVariable * v, double d ) { return IsSetAndGTManip( &SRVN::Input::integerAndGreaterThan, s, v, d ); }
	    static ExtvarPrintingManip print_integer_parameter( const DOM::ExternalVariable* v, double d ) { return ExtvarPrintingManip( &SRVN::Input::printIntegerExtvarParameter, v, d ); }
	    static ExtvarPrintingManip print_double_parameter( const DOM::ExternalVariable* v, double d )  { return ExtvarPrintingManip( &SRVN::Input::printDoubleExtvarParameter, v, d ); }
	    
	private:
	    std::ostream& printHeader( std::ostream& output ) const;
	    std::ostream& printGeneral( std::ostream& output ) const;
	    static bool is_processor( const std::pair<unsigned, DOM::Entity *>& );
	    static bool is_task( const std::pair<unsigned, DOM::Entity *>& );
	    static std::ostream& doubleAndGreaterThan( std::ostream& output, const std::string& s, const DOM::ExternalVariable * v, double d );
	    static std::ostream& integerAndGreaterThan( std::ostream& output, const std::string& s, const DOM::ExternalVariable * v, double d );
	    static std::ostream& printIntegerExtvarParameter( std::ostream& output, const DOM::ExternalVariable * v, double d );
	    static std::ostream& printDoubleExtvarParameter( std::ostream& output, const DOM::ExternalVariable * v, double d );

	protected:
	    const DOM::Document& _document;
	    const std::map<unsigned, DOM::Entity *>& _entities;

	private:
	    const bool _annotate;
	};

	/* ------------------------------------------------------------------------ */

	class ObjectOutput {
	    friend class Output;
	    friend class Parseable;
	    friend class RTF;

	public:
	    ObjectOutput( std::ostream& output ) : _output(output) {}
	    virtual ~ObjectOutput() {}

	protected:
	    ObjectOutput& operator=( const ObjectOutput& );

	    static std::ostream& phaseResults( std::ostream& output, const DOM::Entry& e, const doublePhaseFunc f, const doubleEntryPhaseFunc g, const bool pad );
	    static std::ostream& phaseResultsConfidence( std::ostream& output, const DOM::Entry & e, const doublePhaseFunc f, const doubleEntryPhaseFunc g, const ConfidenceIntervals * conf, const bool pad );

	protected:
	    static EntityNameManip entity_name( const DOM::Entity& e, bool& b ) { return SRVN::EntityNameManip( &SRVN::ObjectOutput::printEntityName, e, b ); }
	    static EntryNameManip entry_name( const DOM::Entry& e ) { return SRVN::EntryNameManip( &SRVN::ObjectOutput::printEntryName, e ); }
	    static UnsignedManip activity_separator( const unsigned int n ) { return SRVN::UnsignedManip( &SRVN::ObjectOutput::activitySeparator, n ); }
	    static ConfidenceManip conf_level( const unsigned int n, const ConfidenceIntervals::confidence_level_t l ) { return SRVN::ConfidenceManip( &SRVN::ObjectOutput::confLevel,n,l); }
	    static LQIO::DOM::DoubleManip colour( const double c ) { return LQIO::DOM::DoubleManip( &SRVN::ObjectOutput::textcolour, c ); }

	    static std::ostream& taskPhaseResults( std::ostream& output, const DOM::Task & task, const doubleTaskFunc func, const bool pad );
	    static std::ostream& task3PhaseResults( std::ostream& output, const DOM::Task & task, const doubleTaskFunc func, const bool pad );
	    static std::ostream& taskPhaseResultsConfidence( std::ostream& output, const DOM::Task & task, const doubleTaskFunc func, const ConfidenceIntervals * conf, const bool pad );
	    static std::ostream& task3PhaseResultsConfidence( std::ostream& output, const DOM::Task & task, const doubleTaskFunc func, const ConfidenceIntervals * conf, const bool pad );

	protected:
	    static std::ostream& printEntityName( std::ostream& output, const DOM::Entity& entity, bool& print );	/* Print name of parent if print == true */
	    static std::ostream& printEntryName( std::ostream& output, const DOM::Entry& entry );
	    static std::ostream& activitySeparator( std::ostream& output, const unsigned n );
	    static std::ostream& confLevel( std::ostream& output, const unsigned int fill, const ConfidenceIntervals::confidence_level_t level );
	    static std::ostream& entryInfo( std::ostream& output, const DOM::Entry& e, const entryFunc f );
	    static std::ostream& phaseInfo( std::ostream& output, const DOM::Entry& e, const phaseFunc f );
	    static std::ostream& phaseTypeInfo( std::ostream& output, const DOM::Entry& e, const phaseTypeFunc f );
	    static std::ostream& entryHistogramInfo( std::ostream& output, const DOM::Entry& e, const histogramEntryFunc f );

	    static std::ostream& newline( std::ostream& output ) { if ( __rtf ) { output << "\\"; } output << std::endl; return output; }
	    static std::ostream& textrm( std::ostream& output ) { if ( __rtf ) { output << "\\f0 "; } return output; }
	    static std::ostream& textbf( std::ostream& output ) { if ( __rtf ) { output << "\\f1 "; } return output; }
	    static std::ostream& textit( std::ostream& output ) { if ( __rtf ) { output << "\\f2 "; } return output; }
	    static std::ostream& activityEOF( std::ostream& output ) { if ( __parseable ) { output << "-1 "; } return output; }

	private:
	    static std::ostream& textcolour( std::ostream& output, const double );

	public:
	    static const unsigned int __maxStrLen = 16;		/* Global for print functions */
	    static const unsigned int __maxDblLen = 12;

	protected:
	    std::ostream& _output;

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
	    ObjectInput( std::ostream& output ) : _output(output) {}
	    virtual ~ObjectInput() {}
	    
	    static CallManip number_of_calls( const DOM::Call* call ) { return CallManip( &ObjectInput::printNumberOfCalls, call ); }
	    static CallManip call_type( const DOM::Call* call ) { return CallManip( &ObjectInput::printCallType, call ); }

	private:
	    ObjectInput& operator=( const ObjectInput& );

	protected:
	    static std::ostream& printNumberOfCalls( std::ostream& output, const DOM::Call* );
	    static std::ostream& printCallType( std::ostream& output, const DOM::Call* );
	    static std::ostream& printObservationVariables( std::ostream& output, const DOM::DocumentObject& );
	    void printReplyList( const std::vector<DOM::Entry*>& replies ) const;

	protected:
	    std::ostream& _output;

	    static unsigned int __maxInpLen;			/* for lqn2lqn formatting */
	    static unsigned int __maxEntLen;			/* Entry name */
	};

	/* ------------------------------------------------------------------------ */

	class DocumentOutput : public ObjectOutput {
	public:
	    DocumentOutput( std::ostream& output ) : ObjectOutput(output) {}
	    virtual void operator()( const DOM::Document& ) const;
	};

	/* ------------------------------------------------------------------------ */

	class EntityOutput : public ObjectOutput {
	public:
	    EntityOutput( std::ostream& output ) : ObjectOutput(output) {}

	protected:
	    void printCommonParameters( const DOM::Entity& ) const;
	};


	class EntityInput : public ObjectInput {
	public:
	    EntityInput( std::ostream& output ) : ObjectInput( output ) {}
	    virtual void operator()( const std::pair<unsigned, DOM::Entity *>& ) const = 0;

	    static std::ostream& print( std::ostream& output, const DOM::Entity* e );
	};

	/* ------------------------------------------------------------------------ */

	class ProcessorOutput : public EntityOutput  {
	protected:
	    typedef void (ProcessorOutput::*voidProcessorFunc)( const DOM::Processor& ) const;

	    static ResultsManip processor_queueing_time( const DOM::Entry& e ) { return SRVN::ResultsManip( &SRVN::ObjectOutput::phaseResults, e, &DOM::Phase::getResultProcessorWaiting, &DOM::Phase::getResultZero ); }
	    static ResultsConfidenceManip processor_queueing_time_confidence( const DOM::Entry& e, const ConfidenceIntervals * c ) { return SRVN::ResultsConfidenceManip( &SRVN::ObjectOutput::phaseResultsConfidence, e, &DOM::Phase::getResultProcessorWaitingVariance, &DOM::Phase::getResultZero, c, true ); }

	    static std::ostream& printScheduling( std::ostream& output, const DOM::Processor& p );

	public:
	    ProcessorOutput( std::ostream& output, voidProcessorFunc f ) : EntityOutput(output), _func(f) {}
	    void operator()( const std::pair<unsigned,DOM::Entity *>& ) const;

	    void printParameters( const DOM::Processor& ) const;
	    void printUtilizationAndWaiting( const DOM::Processor& ) const;
	    void printUtilizationAndWaiting( const DOM::Processor&, const std::set<DOM::Task*>& tasks ) const;

	private:
	    const voidProcessorFunc _func;
	};


	class ProcessorInput : public EntityInput  {
	protected:
	    typedef void (ProcessorInput::*voidProcessorFunc)( const DOM::Processor& ) const;

	    static ProcessorManip copies_of( const DOM::Processor & p )    { return ProcessorManip( &ProcessorInput::printCopies, p ); }  
	    static ProcessorManip rate_of( const DOM::Processor& p ) 	   { return ProcessorManip( &ProcessorInput::printRate, p ); }
	    static ProcessorManip replicas_of( const DOM::Processor& p )   { return ProcessorManip( &ProcessorInput::printReplicas, p ); }
	    static ProcessorManip scheduling_of( const DOM::Processor& p ) { return ProcessorManip( &ProcessorInput::printScheduling, p ); }

	    static std::ostream& printCopies( std::ostream& output, const DOM::Processor& p );
	    static std::ostream& printRate( std::ostream& output, const DOM::Processor& p );
	    static std::ostream& printReplicas( std::ostream& output, const DOM::Processor& p );
	    static std::ostream& printScheduling( std::ostream& output, const DOM::Processor& p );

	public:
	    ProcessorInput( std::ostream& output, voidProcessorFunc f ) : EntityInput(output), _func(f) {}
	    void operator()( const std::pair<unsigned, DOM::Entity *>& ) const;

	    void print( const DOM::Processor& ) const;

	private:
	    const voidProcessorFunc _func;
	};

	/* ------------------------------------------------------------------------ */

	class GroupOutput : public ObjectOutput  {
	protected:
	    typedef void (GroupOutput::*voidGroupFunc)( const LQIO::DOM::Group& ) const;

	public:
	    GroupOutput( std::ostream& output, voidGroupFunc f ) : ObjectOutput(output), _func(f) {}
	    void operator()( const std::pair<const std::string,LQIO::DOM::Group*>& ) const;

	    void printParameters( const LQIO::DOM::Group& ) const;

	private:
	    voidGroupFunc _func;
	};

	class GroupInput : public ObjectInput  {
	protected:
	    typedef void (GroupInput::*voidGroupFunc)( const LQIO::DOM::Group& ) const;

	public:
	    GroupInput( std::ostream& output, voidGroupFunc f ) : ObjectInput(output), _func(f) {}
	    void operator()( const std::pair<const std::string,LQIO::DOM::Group*>& ) const;

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
	    static ResultsManip entry_utilization( const DOM::Entry& e )   { return ResultsManip( &ObjectOutput::phaseResults, e, &DOM::Phase::getResultUtilization, &DOM::Entry::getResultPhasePUtilization, true ); }
	    static TaskResultsManip task_utilization( const DOM::Task& t ) { return TaskResultsManip( &ObjectOutput::taskPhaseResults, t, &DOM::Task::getResultPhasePUtilization, true ); }			     
	    static ResultsConfidenceManip entry_utilization_confidence( const DOM::Entry& e, const ConfidenceIntervals * c )   { return ResultsConfidenceManip( &ObjectOutput::phaseResultsConfidence, e, &DOM::Phase::getResultUtilizationVariance, &DOM::Entry::getResultPhasePUtilizationVariance, c, true ); }
	    static TaskResultsConfidenceManip task_utilization_confidence( const DOM::Task& t, const ConfidenceIntervals * c ) { return TaskResultsConfidenceManip( &ObjectOutput::taskPhaseResultsConfidence, t, &DOM::Task::getResultPhasePUtilizationVariance, c, true ); }				       

	    static TaskResultsManip holding_time( const DOM::Task& t );
	    static TaskResultsConfidenceManip holding_time_confidence( const DOM::Task& t, const ConfidenceIntervals * c );
	    static TaskResultsManip holding_time_variance( const DOM::Task& t );
	    static TaskResultsConfidenceManip holding_time_variance_confidence( const DOM::Task& t, const ConfidenceIntervals * c );

	public:
	    TaskOutput( std::ostream& output, voidTaskFunc f ) : EntityOutput(output), _func(f) {}
	    void operator()( const std::pair<unsigned, DOM::Entity *>& ) const;

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
	    static TaskManip copies_of( const DOM::Task & t )      { return TaskManip( &TaskInput::printCopies, t ); }
	    static TaskManip entries_of( const DOM::Task& t )      { return TaskManip( &TaskInput::printEntryList, t ); }
	    static TaskManip group_of( const DOM::Task& t )        { return TaskManip( &TaskInput::printGroup, t ); }
	    static TaskManip priority_of( const DOM::Task& t )     { return TaskManip( &TaskInput::printPriority, t ); }
	    static TaskManip queue_length_of( const DOM::Task& t ) { return TaskManip( &TaskInput::printQueueLength, t ); }
	    static TaskManip replicas_of( const DOM::Task & t )    { return TaskManip( &TaskInput::printReplicas, t ); }
	    static TaskManip scheduling_of( const DOM::Task & t )  { return TaskManip( &TaskInput::printScheduling, t ); }
	    static TaskManip think_time_of( const DOM::Task & t )  { return TaskManip( &TaskInput::printThinkTime, t ); }

	private:
	    static std::ostream& printCopies( std::ostream& output, const DOM::Task& t );
	    static std::ostream& printEntryList( std::ostream& output,  const DOM::Task& t );
	    static std::ostream& printGroup( std::ostream& output,  const DOM::Task& t );
	    static std::ostream& printPriority( std::ostream& output,  const DOM::Task& t );
	    static std::ostream& printQueueLength( std::ostream& output,  const DOM::Task& t );
	    static std::ostream& printReplicas( std::ostream& output, const DOM::Task& t );
	    static std::ostream& printScheduling( std::ostream& output, const DOM::Task& t );
	    static std::ostream& printThinkTime( std::ostream& output,  const DOM::Task & t );

	public:
	    TaskInput( std::ostream& output, voidTaskFunc f ) : EntityInput(output), _func(f) {}
	    void operator()( const std::pair<unsigned, DOM::Entity *>& ) const;

	    void print( const DOM::Task& ) const;
	    void printEntryInput( const DOM::Task& ) const;
	    void printActivityInput( const DOM::Task& ) const;

	private:
	    const voidTaskFunc _func;
	};

	/* ------------------------------------------------------------------------ */

	class EntryOutput : public ObjectOutput {
	    public:
	    class CountForwarding {
	    public:
		CountForwarding() : _count(0) {}
		virtual void operator()( const std::pair<unsigned, DOM::Entity *>& );
		
		unsigned int getCount() const { return _count; }
	    private:
		unsigned int _count;
	    };
	    
	protected:
	    typedef void (EntryOutput::*voidEntryFunc)( const DOM::Entry&, const DOM::Entity&, bool& ) const;
	    typedef void (EntryOutput::*voidActivityFunc)( const DOM::Activity& ) const;
	    typedef bool (EntryOutput::*testActivityFunc)( const DOM::Activity& ) const;

	    static PhaseManip coefficient_of_variation( const DOM::Entry& e ) { return PhaseManip( &ObjectOutput::phaseInfo, e, &DOM::Phase::getCoeffOfVariationSquared ); }
	    static ResultsManip max_service_time( const DOM::Entry& e )	      { return ResultsManip( &ObjectOutput::phaseResults, e, &DOM::Phase::getMaxServiceTime ); }	  
	    static EntryManip open_arrivals( const DOM::Entry& e )	      { return EntryManip( &ObjectOutput::entryInfo, e, &DOM::Entry::getOpenArrivalRate ); }	  
	    static PhaseTypeManip phase_type( const DOM::Entry& e )	      { return PhaseTypeManip( &ObjectOutput::phaseTypeInfo, e, &DOM::Phase::getPhaseTypeFlag ); }  
	    static PhaseManip service_demand( const DOM::Entry& e )	      { return PhaseManip( &ObjectOutput::phaseInfo, e, &DOM::Phase::getServiceTime ); }		  
	    static PhaseManip think_time( const DOM::Entry& e )		      { return PhaseManip( &ObjectOutput::phaseInfo, e, &DOM::Phase::getThinkTime ); }		  

	public:
	    EntryOutput( std::ostream& output, const voidEntryFunc ef, const voidActivityFunc af=0, const testActivityFunc tf=0 ) : ObjectOutput(output), _entryFunc(ef), _activityFunc(af), _testFunc(tf) {}
	    virtual void operator()( const std::pair<unsigned, DOM::Entity *>& ) const;

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
	    void printForwardingWaiting( const DOM::Entry &entry, const DOM::Entity &entity, bool& print ) const;
	    void printForwardingVarianceWaiting( const DOM::Entry &entry, const DOM::Entity &entity, bool& print ) const;

	    bool nullActivityTest( const DOM::Activity &activity ) const { return false; }	/* Ignore activities (forwarding) 	*/
	    void nullActivityFunc( const DOM::Activity &activity ) const { }			/* Force separator (forwarding) 	*/
	    bool testActivityMaxServiceTimeExceeded( const DOM::Activity &activity ) const;
	    void printActivityServiceTime( const DOM::Activity &activity ) const;
	    void printActivityMaxServiceTimeExceeded( const DOM::Activity &activity ) const;
	    void printActivityVarianceServiceTime( const DOM::Activity &activity ) const;

	protected:
	    static ResultsManip service_time( const DOM::Entry& e )          { return ResultsManip( &ObjectOutput::phaseResults, e, &DOM::Phase::getResultServiceTime, &DOM::Entry::getResultPhasePServiceTime, false ); }
	    static ResultsManip service_time_exceeded( const DOM::Entry& e ) { return ResultsManip( &ObjectOutput::phaseResults, e, &DOM::Phase::getResultMaxServiceTimeExceeded, 0 ); }							
	    static ResultsManip variance_service_time( const DOM::Entry& e ) { return ResultsManip( &ObjectOutput::phaseResults, e, &DOM::Phase::getResultVarianceServiceTime, &DOM::Entry::getResultPhasePVarianceServiceTime, true ); }

	    static ResultsConfidenceManip service_time_confidence( const DOM::Entry& e, const ConfidenceIntervals * c )           { return ResultsConfidenceManip( &ObjectOutput::phaseResultsConfidence, e, &DOM::Phase::getResultServiceTimeVariance, &DOM::Entry::getResultPhasePServiceTimeVariance, c); }		    
	    static ResultsConfidenceManip service_time_exceeded_confidence( const DOM::Entry& e, const ConfidenceIntervals * c )  { return ResultsConfidenceManip( &ObjectOutput::phaseResultsConfidence, e, &DOM::Phase::getResultMaxServiceTimeExceededVariance, 0, c); }						    
	    static ResultsConfidenceManip variance_service_time_confidence( const DOM::Entry& e, const ConfidenceIntervals * c )  { return ResultsConfidenceManip( &ObjectOutput::phaseResultsConfidence, e, &DOM::Phase::getResultVarianceServiceTimeVariance, &DOM::Entry::getResultPhasePVarianceServiceTimeVariance, c); }

	private:
	    void printActivity( const DOM::Activity &activity, const activityFunc func ) const;
	    void activityResults( const DOM::Activity &activity, const doubleActivityFunc mean, const doubleActivityFunc var ) const;
	    bool testForActivityResults( const DOM::Activity &activity, boolActivityFunc tf ) const;
	    void commonPrintForwarding( const DOM::Entry &entry, const DOM::Entity &entity, bool& print, doubleCallFunc, doubleCallFunc ) const;

	private:
	    const voidEntryFunc _entryFunc;
	    const voidActivityFunc _activityFunc;
	    const testActivityFunc _testFunc;
	};

	class EntryInput : public ObjectInput {
	protected:
	    typedef void (EntryInput::*voidEntryFunc)( const DOM::Entry& ) const;

	public:
	    EntryInput( std::ostream& output, const voidEntryFunc ef ) : ObjectInput(output), _entryFunc(ef) {}
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

	    PhaseInput( std::ostream& output ) : ObjectInput( output ), _func(0) {}

	public:
	    PhaseInput( std::ostream& output, voidPhaseFunc f ) : ObjectInput(output), _func(f), _p(1) {}
	    virtual void operator()( const std::pair<unsigned,DOM::Phase *>& ) const;

	    void printCoefficientOfVariation( const DOM::Phase& ) const;
	    void printMaxServiceTimeExceeded( const DOM::Phase& ) const;
	    void printPhaseFlag( const DOM::Phase& ) const; 
	    void printServiceTime( const DOM::Phase& ) const;
	    void printThinkTime( const DOM::Phase& ) const;


	private:
	    const voidPhaseFunc _func;
	    mutable unsigned int _p;
	};

	class ActivityInput : public PhaseInput {
	protected:
	    typedef void (ActivityInput::*voidActivityFunc)( const LQIO::DOM::Activity& ) const;

	public:
	    ActivityInput( std::ostream& output, voidActivityFunc f ) : PhaseInput(output), _func(f) {}
	    virtual void operator()( const std::pair<std::string,DOM::Activity *>& ) const;

	    void print( const DOM::Activity& a ) const;

	private:
	    const voidActivityFunc _func;
	};

	class ActivityListInput : public ObjectInput {
	protected:
	    typedef void (ActivityListInput::*voidActivityListFunc)( const LQIO::DOM::ActivityList& ) const;

	public:

	    ActivityListInput( std::ostream& output, voidActivityListFunc f, const unsigned int n ) : ObjectInput(output), _func(f), _size(n), _count(0), _pending_reply_activity(nullptr) {}
	    virtual void operator()( const DOM::ActivityList * ) const;

	    void print( const DOM::ActivityList& a ) const;
	    const DOM::Activity * getPendingReplyActivity() const { return _pending_reply_activity; }

	private:
	    void printPreList( const DOM::ActivityList& precedence ) const;
	    void printPostList( const DOM::ActivityList& precedence ) const;

	private:
	    const voidActivityListFunc _func;
	    const unsigned int _size;
	    mutable unsigned int _count;
	    mutable const DOM::Activity * _pending_reply_activity;	/* In case we have to tack on an extra list item */
	};

	class CallOutput : public ObjectOutput  {

	protected:
	    typedef void (CallOutput::*callConfFPtr)( const DOM::Call * call, const ConfidenceIntervals* ) const;

	    class CallResultsManip {
	    public:
		CallResultsManip( std::ostream& (*f)(std::ostream&, const CallOutput&, const DOM::ForPhase&, const callConfFPtr, const ConfidenceIntervals* ), const CallOutput & c, const DOM::ForPhase& p, const callConfFPtr x, const ConfidenceIntervals* l=0 ) : _f(f), _c(c), _p(p), _x(x), _conf(l) {}
	    private:
		std::ostream& (*_f)( std::ostream&, const CallOutput&, const DOM::ForPhase&, const callConfFPtr, const ConfidenceIntervals* );
		const CallOutput& _c; 
		const DOM::ForPhase& _p;
		const callConfFPtr _x;
		const ConfidenceIntervals* _conf;
		friend std::ostream& operator<<(std::ostream & os, const CallResultsManip& m ) { return m._f(os,m._c,m._p,m._x,m._conf); }
	    };

	    static CallResultsManip print_calls( const CallOutput& c, const DOM::ForPhase& p, const callConfFPtr f, const ConfidenceIntervals* l=0 ) { return CallResultsManip( &CallOutput::printCalls, c, p, f, l ); }

	public:
	    CallOutput( std::ostream& output, const DOM::Call::boolCallFunc t, const callConfFPtr m=NULL, const callConfFPtr v=NULL ) : ObjectOutput(output), _testFunc(t), _meanFunc(m), _confFunc(v), _count(0) {}
	    virtual void operator()( const std::pair<unsigned, DOM::Entity *>& ) const;

	public:
	    unsigned int getCount() const { return _count; }
	    void printCallRate( const DOM::Call * call, const ConfidenceIntervals* ) const;
	    void printCallWaiting( const DOM::Call * call, const ConfidenceIntervals* ) const;
	    void printCallWaitingConfidence( const DOM::Call * call, const ConfidenceIntervals* ) const;
	    void printCallVarianceWaiting( const DOM::Call * call, const ConfidenceIntervals* ) const;
	    void printCallVarianceWaitingConfidence( const DOM::Call * call, const ConfidenceIntervals* ) const;
	    void printDropProbability( const DOM::Call * call, const ConfidenceIntervals* ) const;
	    void printDropProbabilityConfidence( const DOM::Call * call, const ConfidenceIntervals* conf ) const;

	private:
	    static std::ostream& printCalls( std::ostream&, const CallOutput&c, const DOM::ForPhase& p, const callConfFPtr f, const ConfidenceIntervals* l );

	private:
	    const DOM::Call::boolCallFunc _testFunc;
	    const callConfFPtr _meanFunc;
	    const callConfFPtr _confFunc;
	    mutable unsigned int _count;
	};

	class ActivityCallInput : public ObjectInput {
	protected:
	    typedef void (ActivityCallInput::*voidCallFunc)( const LQIO::DOM::Call * ) const;

	public:
	    ActivityCallInput( std::ostream& output, voidCallFunc f ) : ObjectInput(output), _func(f) {}
	    virtual void operator()( const DOM::Call* call ) const { (this->*_func)( call ); }

	    void print( const DOM::Call* call ) const;

	private:
	    const voidCallFunc _func;
	};
	
	/* ------------------------------------------------------------------------ */

	class HistogramOutput : public ObjectOutput {
	protected:
	    typedef void (HistogramOutput::*histogramFunc)( const DOM::Histogram& ) const;

	public:
	    HistogramOutput( std::ostream& output, const histogramFunc func ) : ObjectOutput(output), _func(func) {}
	    virtual void operator()( const std::pair<unsigned, DOM::Entity *>& ) const;

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
