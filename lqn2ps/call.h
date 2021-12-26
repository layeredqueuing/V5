/* -*- c++ -*-
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
 * $Id: call.h 15262 2021-12-26 18:55:49Z greg $
 * ------------------------------------------------------------------------
 */

#if	!defined(CALL_H)
#define CALL_H

#include "lqn2ps.h"
#include <vector>
#include <deque>
#include <lqio/bcmp_document.h>
#include <lqio/dom_call.h>
#include <lqio/dom_extvar.h>
#include <lqio/srvn_spex.h>
#include "arc.h"

class Entity;
class Entry;
class Label;
class LabelCallManip;
class OpenArrivalSource;
class Point;
class Processor;
class Task;
class SRVNCallManip;
class CallStack;

/*
 * Overloaded operators.
 */

class GenericCall
{
public:
    struct PredicateAndSelected
    {
	PredicateAndSelected( const callPredicate p ) : _p(p) {};
	bool operator()( const GenericCall* call ) const { return call->isSelected() && (!_p || (call->*_p)()); }
    private:
	const callPredicate _p;
    };

private:
    GenericCall& operator=( const GenericCall& src );

protected:
    GenericCall( const GenericCall& src );
    
public:
    GenericCall();
    virtual ~GenericCall();
    virtual GenericCall * clone() const = 0;

    virtual const LQIO::DOM::Call * getDOM( const unsigned p ) const { return nullptr; }

    virtual const std::string& srcName() const = 0;
    virtual const std::string& dstName() const = 0;
    virtual unsigned srcLevel() const;
    virtual unsigned dstLevel() const = 0;
    virtual const Task * srcTask() const = 0;
    virtual double srcIndex() const;
    virtual double dstIndex() const = 0;

    virtual unsigned fanIn() const = 0;
    virtual unsigned fanOut() const = 0;

    virtual LQIO::DOM::Call::Type callType() const { return LQIO::DOM::Call::Type::NULL_CALL; }
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

    virtual const LQIO::DOM::ExternalVariable * sumOfRendezvous() const = 0;			/* Sum over all phases. */
    virtual double sumOfSendNoReply() const = 0;		/* Sum over all phases.	*/

    virtual GenericCall& setChain( const unsigned ) = 0;

    virtual Graphic::Colour colour() const = 0;
    GenericCall& linestyle( Graphic::LineStyle linestyle ) { _arc->linestyle( linestyle ); return *this; }
    
    unsigned int nPoints() const { return _arc->nPoints(); }
    Point& pointAt( const unsigned i ) const { return _arc->pointAt(i); }

    virtual GenericCall& moveDst( const Point& aPoint );
    virtual GenericCall& moveSrc( const Point& aPoint );
    virtual GenericCall& moveSrcBy( const double, const double );

    GenericCall& moveSecond( const Point& aPoint );
    GenericCall& movePenultimate( const Point& aPoint );
    virtual GenericCall& scaleBy( const double, const double );
    virtual GenericCall& translateY( const double );
    virtual GenericCall& depth( const unsigned );
    virtual GenericCall& label() = 0;

    const GenericCall& draw( std::ostream& ) const;
    virtual std::ostream& print( std::ostream& ) const = 0;
    std::ostream& dump( std::ostream& ) const;

    static bool compareDst( const GenericCall *, const GenericCall * );
    static bool compareSrc( const GenericCall *, const GenericCall * );

protected:
    virtual void dump() const = 0;

protected:
    Label * _label;
    Arc * _arc;
};

inline std::ostream& operator<<( std::ostream& output, const GenericCall& self ) { self.draw( output ); return output; }

/* ------------------- Arcs between entries are... -------------------- */

class Call : public GenericCall
{
    friend class Entry;
    friend class ProcessorCall;
    
public:
    class cycle_error : public std::runtime_error
    {
    public:
	cycle_error( const CallStack& );
	size_t depth() const { return _depth; }
    private:
	static std::string fold( const std::string& s1, const Call * c2 );
	const size_t _depth;
    };

    struct PredicateAndEntry
    {
	PredicateAndEntry( const Entry * e, const callPredicate p ) : _e(e), _p(p) {};
	bool operator()( const Call* call ) const { return call->dstEntry() == _e && (!_p || (call->*_p)()); }
    private:
	const Entry * _e;
	const callPredicate _p;
    };

    struct PredicateAndTask
    {
	PredicateAndTask( const Task * t, const callPredicate p ) : _t(t), _p(p) {};
	bool operator()( const Call* call ) const { return call->dstTask() == _t && (!_p || (call->*_p)()); }
    private:
	const Task * _t;
	const callPredicate _p;
    };


private:
    Call& operator=( const Call& );

protected:
    Call( const Call& aCall );

protected:
    typedef SRVNCallManip (* print_func_ptr)( const Call& );

public:
    Call();
    Call( const Entry * toEntry, const unsigned );
    virtual ~Call();
    
    static void reset();
    virtual bool check() const = 0;
    bool checkReplication() const;

    int operator==( const Call& item ) const;
    int operator!=( const Call& item ) const { return !(*this == item); }
    Call& merge( Phase& phase, const Call& src, const double );
    Call& merge( Phase& phase, const unsigned int p, const Call& src, const double );
    
    /* Instance Variable access */

    virtual const LQIO::DOM::Call * getDOM( const unsigned p ) const;
    const LQIO::DOM::Call * getFwdDOM() const { return _forwarding; }
    virtual const LQIO::DOM::ExternalVariable * sumOfRendezvous() const;
    Call& rendezvous( const unsigned p, const double value );
    Call& rendezvous( const unsigned p, const LQIO::DOM::Call * value );
    const LQIO::DOM::ExternalVariable & rendezvous( const unsigned p = 1 ) const;
    virtual double sumOfSendNoReply() const;
    Call& sendNoReply( const unsigned p, const double value );
    Call& sendNoReply( const unsigned p, const LQIO::DOM::Call * value );
    const LQIO::DOM::ExternalVariable & sendNoReply( const unsigned p = 1 ) const;
    Call& forward( const LQIO::DOM::Call * value );
    virtual const LQIO::DOM::ExternalVariable & forward() const;
    virtual Call * addForwardingCall( Entry * toEntry, const double ) = 0;
    virtual LQIO::DOM::Phase::Type phaseTypeFlag( const unsigned p ) const = 0;
    virtual unsigned fanIn() const;
    virtual unsigned fanOut() const;

    bool equalType( const Call& ) const;
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
    const std::string & dstName() const;
    virtual double dstIndex() const;
    virtual unsigned dstLevel() const;
    size_t numberOfPhases() const { return _calls.size(); }
    virtual LQIO::DOM::Call::Type callType() const { return _callType; }

    bool hasRendezvousForPhase( const unsigned ) const;
    bool hasSendNoReplyForPhase( const unsigned ) const;
    virtual bool hasRendezvous() const { return _callType == LQIO::DOM::Call::Type::RENDEZVOUS; }
    virtual bool hasSendNoReply() const { return _callType == LQIO::DOM::Call::Type::SEND_NO_REPLY; }
    virtual bool hasForwarding() const { return _forwarding != nullptr; }
    virtual bool hasRendezvousVariance() const { return hasRendezvous() && __hasVariance; }
    virtual bool hasSendNoReplyVariance() const { return hasSendNoReply() && __hasVariance; }
    virtual bool hasDropProbability() const;
    virtual bool hasInfiniteWait() const;
    virtual bool isSelected() const;
    virtual bool isLoopBack() const;

    /* Proxies */
	
    const Entry * dstEntry() const;
    const Task * dstTask() const;

    /* Other */

    double srcVisits( const unsigned, const unsigned, const unsigned = 0 ) const;
    Call& aggregatePhases( LQIO::DOM::Phase& );
#if defined(BUG_270)
    Call& updateRateFrom( const Call& client, const Call& server );
#endif
    
    virtual Graphic::Colour colour() const;

    virtual Call& moveDst( const Point& aPoint );
    virtual Call& label();
    virtual std::ostream& print( std::ostream& ) const;

#if defined(REP2FLAT)
    virtual Call& expand( const Entry& );
    virtual Call& replicateCall( std::vector<Call *>&, Call ** );
#endif

protected:
    Call& setArcType();
    virtual std::ostream& printSRVNLine( std::ostream& output, char code, print_func_ptr func ) const;
    virtual void dump() const;

private:
    static const LQIO::DOM::ExternalVariable * sum_of_calls( const LQIO::DOM::ExternalVariable *, const LQIO::DOM::Call * );
    
private:
    /* Input */
	
    const Entry* _destination;			/* to whom I am referring to	*/	
    LQIO::DOM::Call::Type _callType;
    std::vector<const LQIO::DOM::Call *> _calls;/* RNV, SNR, FWD (by phase)	*/
    const LQIO::DOM::Call * _forwarding;
    static bool __hasVariance;
};

class EntryCall : public Call 
{
private:
    EntryCall( const EntryCall& );
    
public:
    EntryCall( const Entry * fromEntry, const Entry * toEntry );
    virtual ~EntryCall();

    virtual EntryCall * clone() const { return new EntryCall( *this ); }

    virtual bool check() const;

    EntryCall& setSrcEntry( const Entry * entry ) { _source = entry; return *this; }
    const Entry * srcEntry() const { return _source; }
    virtual const std::string & srcName() const;
    virtual const Task * srcTask() const;
    virtual double srcIndex() const;
    virtual EntryCall& setChain( const unsigned );

    virtual unsigned maxPhase() const;
    virtual LQIO::DOM::Phase::Type phaseTypeFlag( const unsigned p ) const;

    virtual Call * addForwardingCall( Entry * toEntry, const double );

    virtual Graphic::Colour colour() const;

private:
    const Entry* _source;		/* Calling entry.		*/
};

class ProxyEntryCall : public EntryCall 
{
public:
    ProxyEntryCall( const Entry * fromEntry, const Entry * toEntry );

    virtual bool isPseudoCall() const { return true; }

    virtual Call * proxy() const { return myProxy; }
    virtual Call& proxy( Call * aCall ) { myProxy = aCall; return *this; }

private:
    Call * myProxy;			/* if _forwarding > 0		*/
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
private:
    ActivityCall( const ActivityCall& );

public:
    ActivityCall( const Activity * fromActivity, const Entry * toEntry );
    virtual ~ActivityCall();
    virtual ActivityCall * clone() const { return new ActivityCall(*this); }

    virtual bool check() const;

    const Activity * srcActivity() const { return _source; }
    virtual const std::string & srcName() const;
    virtual const Task * srcTask() const;
    virtual double srcIndex() const;
    virtual ActivityCall& setChain( const unsigned );

    virtual unsigned maxPhase() const { return 1; }
    virtual LQIO::DOM::Phase::Type phaseTypeFlag( const unsigned ) const;

    virtual Call * addForwardingCall( Entry * toEntry, const double );

    virtual Graphic::Colour colour() const;

protected:
    virtual std::ostream& printSRVNLine( std::ostream& output, char code, print_func_ptr func ) const;

private:
    const Activity* _source;		/* Calling entry.		*/
};

class ProxyActivityCall : public ActivityCall 
{
public:
    ProxyActivityCall( const Activity * fromActivity, const Entry * toEntry );

    virtual bool isPseudoCall() const { return true; }

    virtual Call * proxy() const { return myProxy; }
    virtual Call& proxy( Call * aCall ) { myProxy = aCall; return *this; }

private:
    Call * myProxy;			/* if _forwarding > 0		*/
};

class Reply : public ActivityCall
{
public:
    Reply( const Activity * fromActivity, const Entry * toEntry );
    virtual ~Reply();

    virtual Graphic::Colour colour() const;
    virtual bool isSelected() const { return true; }
};

/* ----------------- Calls to processors from tasks. ------------------ */

class EntityCall : public GenericCall {
private:
    EntityCall( const EntityCall& );
    
public:
    EntityCall( const Task * fromTask, const Entity * toEntity ) : GenericCall(), _srcTask(fromTask), _dstEntity(toEntity) {}

    virtual const std::string & srcName() const;
    EntityCall& setSrcTask( const Task * task ) { _srcTask = task; return *this; }
    virtual const Task * srcTask() const { return _srcTask; }

    EntityCall& setDstEntity( const Entity * entity ) { _dstEntity = entity; return *this; }
    const Entity * dstEntity() const { return _dstEntity; }
    virtual const std::string & dstName() const;
    virtual unsigned dstLevel() const;
    virtual double dstIndex() const;

#if defined(BUG_270)
    virtual EntityCall& updateRateFrom( const Call& );
#endif
protected:
    const Task * _srcTask;
    const Entity * _dstEntity;
};

/* ----------------- Calls to processors from tasks. ------------------ */

class TaskCall : public EntityCall 
{
private:
    TaskCall( const TaskCall& );
    
public:
    TaskCall( const Task * fromTask, const Task * toTask );
    virtual ~TaskCall();
    virtual TaskCall * clone() const { return new TaskCall( *this ); }

    int operator==( const TaskCall& item ) const;
    int operator!=( const TaskCall& item ) const { return !(*this == item); }

    virtual const LQIO::DOM::ExternalVariable& rendezvous() const { return _rendezvous; }
    TaskCall& rendezvous( const LQIO::DOM::ConstantExternalVariable& );
    virtual const LQIO::DOM::ExternalVariable * sumOfRendezvous() const;
    virtual const LQIO::DOM::ExternalVariable& sendNoReply() const { return _sendNoReply; }
    TaskCall& sendNoReply( const LQIO::DOM::ConstantExternalVariable& );
    virtual double sumOfSendNoReply() const;
    virtual const LQIO::DOM::ExternalVariable& forward() const { return _forwarding; }
    TaskCall& taskForward( const LQIO::DOM::ConstantExternalVariable& );
    virtual unsigned fanIn() const;
    virtual unsigned fanOut() const;

    virtual bool hasRendezvous() const;
    virtual bool hasSendNoReply() const;
    virtual bool hasForwarding() const;
    virtual bool isSelected() const;

    virtual TaskCall& setChain( const unsigned );

    virtual Graphic::Colour colour() const;

    virtual bool isLoopBack() const;

    virtual TaskCall& moveSrc( const Point& aPoint );
    virtual TaskCall& moveSrcBy( const double, const double );
    virtual TaskCall& label();
    virtual std::ostream& print( std::ostream& output ) const { return output; }

protected:
    virtual void dump() const;

private:
    LQIO::DOM::ConstantExternalVariable _rendezvous;
    LQIO::DOM::ConstantExternalVariable _sendNoReply;
    LQIO::DOM::ConstantExternalVariable _forwarding;
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
private:
    ProcessorCall( const ProcessorCall& );
    
public:
    ProcessorCall( const Task * fromTask, const Processor * toProcessor );
    virtual ~ProcessorCall();
    virtual ProcessorCall * clone() const {  return new ProcessorCall( *this ); }

    int operator==( const ProcessorCall& item ) const;
    int operator!=( const ProcessorCall& item ) const { return !(*this == item); }

    ProcessorCall& setSrcEntry( const Entry * entry ) { _source = entry; return *this; }
    const Entry * srcEntry() const { return _source; }

    virtual bool hasRendezvous() const { return _callType != LQIO::DOM::Call::Type::SEND_NO_REPLY; }	/* Default is also rendezvous */
    virtual bool hasSendNoReply() const { return _callType == LQIO::DOM::Call::Type::SEND_NO_REPLY; }
    virtual LQIO::DOM::Call::Type callType() const { return _callType; }

    ProcessorCall& rendezvous( const LQIO::DOM::ExternalVariable * value );
    virtual const LQIO::DOM::ExternalVariable& rendezvous() const;
    virtual const LQIO::DOM::ExternalVariable * sumOfRendezvous() const;
    virtual const LQIO::DOM::ExternalVariable& sendNoReply() const;
    virtual double sumOfSendNoReply() const;
    virtual const LQIO::DOM::ExternalVariable& forward() const;
    virtual unsigned fanIn() const;
    virtual unsigned fanOut() const;
    const LQIO::DOM::ExternalVariable * visits() const { return _demand.visits(); }
    const LQIO::DOM::ExternalVariable * service_time() const { return _demand.service_time(); }
    void setServiceTime( const LQIO::DOM::ExternalVariable * service_time ) { _demand.setServiceTime( service_time ); }
#if defined(BUG_270)
    virtual ProcessorCall& updateRateFrom( const Call& );
#endif

    virtual bool isSelected() const;
    virtual bool isProcessorCall() const { return true; }

    virtual ProcessorCall& setChain( const unsigned );

    virtual Graphic::Colour colour() const;

    virtual ProcessorCall& moveSrc( const Point& aPoint );
    virtual ProcessorCall& moveSrcBy( const double, const double );
    virtual ProcessorCall& moveDst( const Point& aPoint );
    virtual ProcessorCall& label();
    virtual std::ostream& print( std::ostream& output ) const { return output; }

private:
    void moveLabel();
    static double sum_of_extvar( double, const LQIO::DOM::ConstantExternalVariable& );

protected:
    virtual void dump() const;

private:
    LQIO::DOM::Call::Type _callType;		/* Union discriminator		*/
    BCMP::Model::Station::Class _demand;
    const Entry * _source;			/* not null if a clone.		*/
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
private:
    OpenArrival( const OpenArrival& );
    
public:
    OpenArrival( const OpenArrivalSource *, const Entry * );
    virtual ~OpenArrival();

    OpenArrival * clone() const { return new OpenArrival( *this ); }

    int operator==( const OpenArrival& item ) const;
    int operator!=( const OpenArrival& item ) const { return !(*this == item); }

    virtual const std::string & srcName() const;
    virtual const Task * srcTask() const;
    virtual const std::string & dstName() const;
    const Task * dstTask() const;
    virtual double srcIndex() const;
    virtual double dstIndex() const;
    virtual unsigned dstLevel() const;
    virtual unsigned fanIn() const;
    virtual unsigned fanOut() const;

    virtual const LQIO::DOM::ExternalVariable * sumOfRendezvous() const { return nullptr; }
    virtual double sumOfSendNoReply() const;
    virtual const LQIO::DOM::ExternalVariable & rendezvous() const;
    virtual const LQIO::DOM::ExternalVariable & sendNoReply() const;
    virtual const LQIO::DOM::ExternalVariable & forward() const;

    const LQIO::DOM::ExternalVariable& openArrivalRate() const;
    double openWait() const;

    virtual OpenArrival& setChain( const unsigned );

    virtual Graphic::Colour colour() const;

    virtual OpenArrival& moveDst( const Point& aPoint );
    virtual OpenArrival& label();
    virtual std::ostream& print( std::ostream& output ) const { return output; }

protected:
    virtual void dump() const;

private:
    const OpenArrivalSource * _source;
    const Entry * _destination;
};

/* -------------- Special class to handle call stacks. ---------------- */

class CallStack : public std::deque<const Call *>
{
public:
    std::deque<const Call *>::const_iterator find( const Call *, const bool );
    size_t size() const;
};

class SRVNCallManip {
public:
    SRVNCallManip( std::ostream& (*ff)(std::ostream&, const Call & ), const Call & aCall  )
	: f(ff), myCall(aCall) {}
private:
    std::ostream& (*f)( std::ostream&, const Call & );
    const Call & myCall;

    friend std::ostream& operator<<(std::ostream & os, const SRVNCallManip& m )
	{ return m.f(os,m.myCall); }
};


class TaskCallManip {
public:
    TaskCallManip( std::ostream& (*ff)(std::ostream&, const TaskCall & ), const TaskCall & aCall  )
	: f(ff), myCall(aCall) {}
private:
    std::ostream& (*f)( std::ostream&, const TaskCall & );
    const TaskCall & myCall;

    friend std::ostream& operator<<(std::ostream & os, const TaskCallManip& m )
	{ return m.f(os,m.myCall); }
};


class EntityCallManip {
public:
    EntityCallManip( std::ostream& (*ff)(std::ostream&, const EntityCall & ), const EntityCall & aCall  )
	: f(ff), myCall(aCall) {}
private:
    std::ostream& (*f)( std::ostream&, const EntityCall & );
    const EntityCall & myCall;

    friend std::ostream& operator<<(std::ostream & os, const EntityCallManip& m )
	{ return m.f(os,m.myCall); }
};

SRVNCallManip print_rendezvous( const Call& aCall );
SRVNCallManip print_sendnoreply( const Call& aCall );
SRVNCallManip print_forwarding( const Call& aCall );
LabelCallManip print_wait( const Call& aCall );
#endif
