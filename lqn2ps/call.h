/* -*- c++ -*-
 * $Source$
 *
 * Everything you wanted to know about an entry, but were afraid to ask.
 *
 * Copyright the Real-Time and Distributed Systems Group,
 * Department of Systems and Computer Engineering,
 * Carleton University, Ottawa, Ontario, Canada. K1S 5B6
 *
 * November, 1994
 * May 2010
 *
 * ------------------------------------------------------------------------
 * $Id: call.h 11963 2014-04-10 14:36:42Z greg $
 * ------------------------------------------------------------------------
 */

#if	!defined(CALL_H)
#define CALL_H

#include "lqn2ps.h"
#include "arc.h"
#include "cltn.h"
#include "stack.h"
#include <lqio/dom_extvar.h>

class Entity;
class Entry;
class Entry;
class GenericCall;
class Label;
class LabelCallManip;
class OpenArrivalSource;
class Phase;
class Point;
class Processor;
class SRVNCallManip;
class Task;
template <class type> class Cltn;
template <class type> class Sequence;


/*
 * Overloaded operators.
 */

ostream& operator<<( ostream&, const GenericCall& );

class GenericCall
{
public:
    GenericCall();
    virtual ~GenericCall();

    virtual const LQIO::DOM::Call * getDOM( const unsigned p ) const { return 0; }

    virtual const string& srcName() const = 0;
    virtual const string& dstName() const = 0;
    virtual unsigned srcLevel() const;
    virtual unsigned dstLevel() const = 0;
    virtual const Entity * srcTask() const = 0;
    virtual double srcIndex() const;
    virtual double dstIndex() const = 0;

    virtual unsigned fanIn() const = 0;
    virtual unsigned fanOut() const = 0;

    virtual bool hasRendezvous() const { return false; }
    virtual bool hasSendNoReply() const { return false; }
    virtual bool hasForwarding() const { return false; }
    bool hasNoCall() const { return !hasRendezvous() && !hasSendNoReply() && !hasForwarding(); }
    bool hasAnyCall() const { return hasRendezvous() || hasSendNoReply() || hasForwarding(); }
    bool hasRendezvousOrNone() const { return !hasSendNoReply() && !hasForwarding(); }
    bool hasSendNoReplyOrNone() const { return !hasRendezvous() && !hasForwarding(); }
    bool hasForwardingOrNone() const { return !hasRendezvous() && !hasSendNoReply(); }

    bool hasForwardingLevel() const;
    bool hasAncestorLevel() const { return srcLevel() < dstLevel(); }
    virtual bool hasRendezvousVariance() const { return false; }
    virtual bool hasSendNoReplyVariance() const { return false; }
    virtual bool hasDropProbability() const { return false; }
    virtual bool hasInfiniteWait() const { return false; }
    virtual bool isSelected() const { return true; }
    virtual bool isPseudoCall() const { return false; }
    virtual bool isLoopBack() const { return false; }
    virtual bool isProcessorCall() const { return false; }

    virtual double sumOfRendezvous() const = 0;			/* Sum over all phases. */
    virtual double sumOfSendNoReply() const = 0;		/* Sum over all phases.	*/

    virtual const GenericCall& setChain( const unsigned ) const = 0;

    virtual Graphic::colour_type colour() const = 0;
    GenericCall& linestyle( Graphic::linestyle_type aLinestyle ) { myArc->linestyle( aLinestyle ); return *this; }
    
    Point& pointAt( const unsigned i ) const { return (*myArc)[i]; }

    virtual GenericCall& moveDst( const Point& aPoint );
    virtual GenericCall& moveSrc( const Point& aPoint );
    virtual GenericCall& moveSrcBy( const double, const double );

    GenericCall& moveSecond( const Point& aPoint );
    GenericCall& movePenultimate( const Point& aPoint );
    virtual GenericCall& scaleBy( const double, const double );
    virtual GenericCall& translateY( const double );
    virtual GenericCall& depth( const unsigned );
    virtual GenericCall& label() = 0;

    virtual ostream& draw( ostream& ) const;
    virtual ostream& print( ostream& ) const = 0;

protected:
    Label * myLabel;
    Arc * myArc;
};

/* ------------------- Arcs between entries are... -------------------- */

class Call : public GenericCall
{
    static bool hasVariance;

private:
    Call( const Call& aCall );
    Call& operator=( const Call& );

protected:
    typedef SRVNCallManip (* print_func_ptr)( const Call& );

public:
    Call();
    Call( const Entry * toEntry, const unsigned );
    virtual ~Call();
    static int compareDst( const void *, const void * );
    static int compareSrc( const void *, const void * );
    static void reset();
    virtual void check() = 0;

    int operator==( const Call& item ) const;
    int operator!=( const Call& item ) const { return !(*this == item); }
    Call& merge( const Call& src, const double );
    Call& merge( const int p, const Call& src, const double );
    
    /* Instance Variable access */


    virtual const LQIO::DOM::Call * getDOM( const unsigned p ) const;
    const LQIO::DOM::Call * getDOMFwd() const;
    Call& rendezvous( const unsigned p, const LQIO::DOM::Call * value );
    virtual double sumOfRendezvous() const;
    const LQIO::DOM::ExternalVariable & rendezvous( const unsigned p = 1 ) const;
    virtual double sumOfSendNoReply() const;
    Call& sendNoReply( const unsigned p, const LQIO::DOM::Call * value );
    const LQIO::DOM::ExternalVariable & sendNoReply( const unsigned p = 1 ) const;
    Call& forward( const LQIO::DOM::Call * value );
    virtual const LQIO::DOM::ExternalVariable & forward() const;
    virtual Call * addForwardingCall( Entry * toEntry, const double ) const = 0;
    virtual phase_type phaseTypeFlag( const unsigned p ) const = 0;
    virtual unsigned fanIn() const;
    virtual unsigned fanOut() const;

    bool hasWaiting() const;
    Call& waiting( const unsigned p, const double value );
    double waiting( const unsigned p ) const;
    Call& waitVariance( const unsigned p, const double value );
    double waitVariance( const unsigned p ) const;
    bool hasDropProbability( const unsigned p ) const;
    Call& dropProbability( const unsigned p, const double value );
    double dropProbability( const unsigned p ) const;

    virtual Call * proxy() const { return 0; }
    virtual Call& proxy( Call * aCall ) { return *this; }

    /* Queries */
	
    virtual unsigned maxPhase() const = 0;
    const string & dstName() const;
    virtual double dstIndex() const;
    virtual unsigned dstLevel() const;

    bool hasRendezvousForPhase( const unsigned ) const;
    bool hasSendNoReplyForPhase( const unsigned ) const;
    virtual bool hasRendezvous() const;
    virtual bool hasSendNoReply() const;
    virtual bool hasForwarding() const;
    virtual bool hasRendezvousVariance() const { return hasRendezvous() && hasVariance; }
    virtual bool hasSendNoReplyVariance() const { return hasSendNoReply() && hasVariance; }
    virtual bool hasDropProbability() const;
    virtual bool hasInfiniteWait() const;
    virtual bool isSelected() const;
    virtual bool isLoopBack() const;

    /* Proxies */
	
    const Entry * dstEntry() const;
    const Task * dstTask() const;
    double variance() const;
    phase_type phaseTypeFlag() const;
    const LQIO::DOM::ExternalVariable & serviceTime() const;
    double executionTime() const;
    double Cv_sqr() const;

    /* Other */

    double srcVisits( const unsigned, const unsigned, const unsigned = 0 ) const;
    Call& aggregatePhases();

    virtual Graphic::colour_type colour() const;

    virtual Call& moveDst( const Point& aPoint );
    virtual Call& label();
    virtual ostream& print( ostream& ) const;

protected:
    Call& setArcType();
    virtual ostream& printSRVNLine( ostream& output, char code, print_func_ptr func ) const;

private:
    /* Input */
	
    const Entry* destination;		/* to whom I am referring to	*/	
    Vector2<const LQIO::DOM::Call *> myRendezvous;	/* rendezvous.		*/
    Vector2<const LQIO::DOM::Call *> mySendNoReply;	/* send no reply.	*/
    const LQIO::DOM::Call * myForwarding;	/* Forwarding probability.	*/
};

class EntryCall : public Call 
{
public:
    EntryCall( const Entry * fromEntry, const Entry * toEntry );
    virtual ~EntryCall();
    virtual void check();

    const Entry * srcEntry() const { return source; }
    virtual const string & srcName() const;
    virtual const Entity * srcTask() const;
    virtual double srcIndex() const;
    virtual const EntryCall& setChain( const unsigned ) const;

    virtual unsigned maxPhase() const;
    virtual phase_type phaseTypeFlag( const unsigned p ) const;

    virtual Call * addForwardingCall( Entry * toEntry, const double ) const;

    virtual Graphic::colour_type colour() const;

private:
    const Entry* source;		/* Calling entry.		*/
};


class ProxyEntryCall : public EntryCall 
{
public:
    ProxyEntryCall( const Entry * fromEntry, const Entry * toEntry );

    virtual bool isPseudoCall() const { return true; }

    virtual Call * proxy() const { return myProxy; }
    virtual Call& proxy( Call * aCall ) { myProxy = aCall; return *this; }

private:
    Call * myProxy;			/* if myForwarding > 0		*/
};

class PseudoEntryCall : public EntryCall 
{
public:
    PseudoEntryCall( const Entry * fromEntry, const Entry * toEntry );

    virtual bool isPseudoCall() const { return true; }
    virtual bool hasRendezvous() const { return true; }
};

class ActivityCall : public Call 
{
public:
    ActivityCall( const Activity * fromActivity, const Entry * toEntry );
    virtual ~ActivityCall();
    virtual void check();

    const Activity * srcActivity() const { return source; }
    virtual const string & srcName() const;
    virtual const Entity * srcTask() const;
    virtual double srcIndex() const;
    virtual const ActivityCall& setChain( const unsigned ) const;

    virtual unsigned maxPhase() const { return 1; }
    virtual phase_type phaseTypeFlag( const unsigned ) const;

    virtual Call * addForwardingCall( Entry * toEntry, const double ) const;

    virtual Graphic::colour_type colour() const;

protected:
    virtual ostream& printSRVNLine( ostream& output, char code, print_func_ptr func ) const;

private:
    const Activity* source;		/* Calling entry.		*/
};

class ProxyActivityCall : public ActivityCall 
{
public:
    ProxyActivityCall( const Activity * fromActivity, const Entry * toEntry );

    virtual bool isPseudoCall() const { return true; }

    virtual Call * proxy() const { return myProxy; }
    virtual Call& proxy( Call * aCall ) { myProxy = aCall; return *this; }

private:
    Call * myProxy;			/* if myForwarding > 0		*/
};

class Reply : public ActivityCall
{
public:
    Reply( const Activity * fromActivity, const Entry * toEntry );
    virtual ~Reply();

    virtual Graphic::colour_type colour() const;
};

/* ----------------- Calls to processors from tasks. ------------------ */

class EntityCall : public GenericCall {
public:
    EntityCall( const Task * fromTask ) : GenericCall(), mySrcTask(fromTask) {}

    virtual const string & srcName() const;
    virtual const Entity * srcTask() const;

    virtual double serviceTime( const unsigned ) const = 0;

protected:
    const Task * mySrcTask;
};


/* ----------------- Calls to processors from tasks. ------------------ */

class TaskCall : public EntityCall 
{
public:
    TaskCall( const Task * fromTask, const Task * toTask );
    virtual ~TaskCall();

    int operator==( const TaskCall& item ) const;
    int operator!=( const TaskCall& item ) const { return !(*this == item); }

    const Task * dstTask() const { return myDstTask; }
    virtual const string & dstName() const;
    virtual unsigned dstLevel() const;
    virtual double dstIndex() const;

    virtual const LQIO::DOM::ExternalVariable& rendezvous() const { return myRendezvous; }
    TaskCall& rendezvous( const LQIO::DOM::ConstantExternalVariable& );
    virtual double sumOfRendezvous() const;
    virtual const LQIO::DOM::ExternalVariable& sendNoReply() const { return mySendNoReply; }
    TaskCall& sendNoReply( const LQIO::DOM::ConstantExternalVariable& );
    virtual double sumOfSendNoReply() const;
    virtual const LQIO::DOM::ExternalVariable & forward() const { return myForwarding; }
    TaskCall& taskForward( const LQIO::DOM::ConstantExternalVariable& );
    virtual unsigned fanIn() const;
    virtual unsigned fanOut() const;
    virtual double serviceTime( const unsigned ) const;

    virtual bool hasRendezvous() const;
    virtual bool hasSendNoReply() const;
    virtual bool hasForwarding() const;
    virtual bool isSelected() const;

    virtual const TaskCall& setChain( const unsigned ) const;

    virtual Graphic::colour_type colour() const;

    virtual bool isLoopBack() const;

    virtual TaskCall& moveSrc( const Point& aPoint );
    virtual TaskCall& moveSrcBy( const double, const double );
    virtual TaskCall& label();
    virtual ostream& print( ostream& output ) const { return output; }

private:
    const Task * myDstTask;
    LQIO::DOM::ConstantExternalVariable myRendezvous;
    LQIO::DOM::ConstantExternalVariable mySendNoReply;
    LQIO::DOM::ConstantExternalVariable myForwarding;
};


class ProxyTaskCall : public TaskCall 
{
public:
    ProxyTaskCall( const Task * fromTask, const Task * toTask );

    virtual bool isPseudoCall() const { return true; }
    ProxyTaskCall& rendezvous( const unsigned int p, double value );
};

class PseudoTaskCall : public TaskCall 
{
public:
    PseudoTaskCall( const Task * fromTask, const Task * toTask );

    virtual bool isPseudoCall() const { return true; }
    bool hasRendezvous() const { return true; }
};

/* ----------------- Calls to processors from tasks. ------------------ */

class ProcessorCall : public EntityCall {
public:
    ProcessorCall( const Task * fromTask, const Processor * toProcessor );
    virtual ~ProcessorCall();

    int operator==( const ProcessorCall& item ) const;
    int operator!=( const ProcessorCall& item ) const { return !(*this == item); }

    const Processor * dstProcessor() const { return myProcessor; }
    ProcessorCall& setDstProcessor( const Processor * aProcessor ) { myProcessor = aProcessor; return *this; }
    virtual const string & dstName() const;
    virtual double dstIndex() const;
    virtual unsigned dstLevel() const;

    virtual const LQIO::DOM::ExternalVariable & rendezvous() const;
    virtual double sumOfRendezvous() const { return 0.0; }
    virtual double sumOfSendNoReply() const { return 0.0; }
    virtual const LQIO::DOM::ExternalVariable & sendNoReply() const;
    virtual const LQIO::DOM::ExternalVariable & forward() const;
    virtual unsigned fanIn() const;
    virtual unsigned fanOut() const;
    virtual double serviceTime( const unsigned ) const;

    virtual bool isSelected() const;
    virtual bool isProcessorCall() const { return true; }

    virtual const ProcessorCall& setChain( const unsigned ) const;

    virtual Graphic::colour_type colour() const;

    virtual ProcessorCall& moveSrc( const Point& aPoint );
    virtual ProcessorCall& moveSrcBy( const double, const double );
    virtual ProcessorCall& moveDst( const Point& aPoint );
    virtual ProcessorCall& label();
    virtual ostream& print( ostream& output ) const { return output; }

private:
    const Processor * myProcessor;
};


class PseudoProcessorCall : public ProcessorCall 
{
public:
    PseudoProcessorCall( const Task * fromTask, const Processor * toProcessor );

    virtual bool isPseudoCall() const { return true; }
    virtual bool hasRendezvous() const { return true; }
};

/* ----------- Calls to tasks from Open Arrival Sources . ------------- */

class OpenArrival : public GenericCall {
public:
    OpenArrival( const OpenArrivalSource *, const Entry * );
    virtual ~OpenArrival();

    int operator==( const OpenArrival& item ) const;
    int operator!=( const OpenArrival& item ) const { return !(*this == item); }

    virtual const string & srcName() const;
    virtual const Entity * srcTask() const;
    virtual const string & dstName() const;
    const Task * dstTask() const;
    virtual double srcIndex() const;
    virtual double dstIndex() const;
    virtual unsigned dstLevel() const;
    virtual unsigned fanIn() const;
    virtual unsigned fanOut() const;

    virtual double sumOfRendezvous() const { return 0.0; }
    virtual double sumOfSendNoReply() const;
    virtual const LQIO::DOM::ExternalVariable & rendezvous() const;
    virtual const LQIO::DOM::ExternalVariable & sendNoReply() const;
    virtual const LQIO::DOM::ExternalVariable & forward() const;

    const LQIO::DOM::ExternalVariable& openArrivalRate() const;
    double openWait() const;

    virtual const OpenArrival& setChain( const unsigned ) const;

    virtual Graphic::colour_type colour() const;

    virtual OpenArrival& moveDst( const Point& aPoint );
    virtual OpenArrival& label();
    virtual ostream& print( ostream& output ) const { return output; }

private:
    const OpenArrivalSource * source;
    const Entry * destination;
};


/* -------------- Special class to handle call stacks. ---------------- */

class CallStack : public Stack<const Call *>
{
public:
    CallStack( const unsigned size = 0 ) : Stack<const Call *>( size ) {}

    unsigned find( const Call *, const bool ) const;
    unsigned size() const;
    unsigned size2() const;
};

class SRVNCallManip {
public:
    SRVNCallManip( ostream& (*ff)(ostream&, const Call & ), const Call & aCall  )
	: f(ff), myCall(aCall) {}
private:
    ostream& (*f)( ostream&, const Call & );
    const Call & myCall;

    friend ostream& operator<<(ostream & os, const SRVNCallManip& m )
	{ return m.f(os,m.myCall); }
};


class TaskCallManip {
public:
    TaskCallManip( ostream& (*ff)(ostream&, const TaskCall & ), const TaskCall & aCall  )
	: f(ff), myCall(aCall) {}
private:
    ostream& (*f)( ostream&, const TaskCall & );
    const TaskCall & myCall;

    friend ostream& operator<<(ostream & os, const TaskCallManip& m )
	{ return m.f(os,m.myCall); }
};


class EntityCallManip {
public:
    EntityCallManip( ostream& (*ff)(ostream&, const EntityCall & ), const EntityCall & aCall  )
	: f(ff), myCall(aCall) {}
private:
    ostream& (*f)( ostream&, const EntityCall & );
    const EntityCall & myCall;

    friend ostream& operator<<(ostream & os, const EntityCallManip& m )
	{ return m.f(os,m.myCall); }
};


class call_cycle : public path_error 
{
public:
    call_cycle( const Call *, const CallStack& );
};

SRVNCallManip print_rendezvous( const Call& aCall );
SRVNCallManip print_sendnoreply( const Call& aCall );
SRVNCallManip print_forwarding( const Call& aCall );
LabelCallManip print_wait( const Call& aCall );
#if defined(QNAP_OUTPUT)
EntityCallManip qnap_visits( const EntityCall& aCall );
#endif
#endif

