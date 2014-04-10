/* -*- c++ -*-
 * $HeadURL$
 *
 * Everything you wanted to know about an entry, but were afraid to ask.
 *
 * Copyright the Real-Time and Distributed Systems Group,
 * Department of Systems and Computer Engineering,
 * Carleton University, Ottawa, Ontario, Canada. K1S 5B6
 *
 * November, 1994
 * March, 2004
 *
 * $Id$
 *
 * ------------------------------------------------------------------------
 */

#if	!defined(LQNS_CALL_H)
#define LQNS_CALL_H

#include "dim.h"
#include <cstdlib>
#include <lqio/input.h>
#include <lqio/dom_call.h>
#include "stack.h"
#include "prob.h"
    
 
class Entry;
class Call;
class Entry;
class Entity;
class Submodel;
class Phase;
class Path;
class EntryPath;
class InterlockInfo;
class PathNode;
class Server;
class Task;
template <class type> class Stack;
template <class type> class Cltn;
template <class type> class Sequence;

/* ------------------- Arcs between entries are... -------------------- */

class Call {

public:
    typedef enum { RENDEZVOUS_CALL=0x01, SEND_NO_REPLY_CALL=0x02, FORWARDED_CALL=0x04, OVERTAKING_CALL=0x08 } call_type;

    Call( const Phase * fromPhase, const Entry * toEntry );
    virtual ~Call();

private:
    Call( const Call& ) { abort(); }		/* Copying is verbotten */
    Call& operator=( const Call& ) { abort(); return *this; }

public:
    int operator==( const Call& item ) const;
    int operator!=( const Call& item ) const;

    virtual void initWait() = 0;
    void check() const;

    /* Instance variable access */

    double rendezvous() const;
    Call& rendezvous( LQIO::DOM::Call* newCallDOMInfo ) { myCallDOM = newCallDOMInfo; return *this; }
    Call& accumulateRendezvous( const double value )  { abort(); /*myRendezvous += value;*/ return *this; }
    double sendNoReply() const;
    Call& sendNoReply( LQIO::DOM::Call* newCallDOMInfo ) { myCallDOM = newCallDOMInfo; return *this; }
    Call& accumulateSendNoReply( const double value ) { abort(); /*mySendNoReply += value;*/ return *this; }
    double forward() const;
    Call& forward( LQIO::DOM::Call* newCallDOMInfo ) { myCallDOM = newCallDOMInfo; return *this; }
    unsigned fanIn() const;
    unsigned fanOut() const;

    double sumOfCalls() const { return myCallDOM ? myCallDOM->getCallMeanValue() : 0.0; }
	

    /* Queries */
	
    virtual bool isForwardedCall() const { return false; }
    virtual bool isActivityCall() const { return false; }
    virtual bool isProcessorCall() const { return false; }

    bool hasRendezvous() const { return myCallDOM ? (myCallDOM->getCallType() == LQIO::DOM::Call::RENDEZVOUS || myCallDOM->getCallType() == LQIO::DOM::Call::QUASI_RENDEZVOUS) : false; }
    bool hasSendNoReply() const { return myCallDOM ? myCallDOM->getCallType() == LQIO::DOM::Call::SEND_NO_REPLY : false; }
    bool hasForwarding() const { return  myCallDOM ? myCallDOM->getCallType() == LQIO::DOM::Call::FORWARD : false; }
    bool hasOvertaking() const;
    bool hasNoCall() const { return !hasRendezvous() && !hasSendNoReply() && !hasForwarding(); }
    bool hasRendezvousOrNone() const { return !hasSendNoReply() && !hasForwarding(); }
    bool hasSendNoReplyOrNone() const { return !hasRendezvous() && !hasForwarding(); }
    bool hasForwardingOrNone() const { return !hasRendezvous() && !hasSendNoReply(); }

    virtual const Entry * srcEntry() const;
    virtual const char * srcName() const;
    const Phase * srcPhase() const { return source; }
    virtual const Task * srcTask() const;

    const char * dstName() const;
    const Entry * dstEntry() const { return destination; }
    unsigned submodel() const;

    double rendezvousDelay() const;
    double rendezvousDelay( const unsigned k );
    double wait() const { return myWait; }
    double elapsedTime() const;
    double queueingTime() const;
    virtual void insertDOMResults() const;
    double nrFactor( const Submodel& aSubmodel, const unsigned k ) const;

    /* Computation */
	
    double variance() const;
    double CV_sqr() const;
    unsigned followInterlock( Stack<const Entry *>&, const InterlockInfo&, const unsigned ) const;

    /* MVA interface */

    unsigned getChain(); 
    void setChain( const unsigned k, const unsigned p, const double rate );

    void setVisits( const unsigned k, const unsigned p, const double rate );
    virtual void setLambda( const unsigned k, const unsigned p, const double rate );
    void clearWait( const unsigned k, const unsigned p, const double );
    void saveWait( const unsigned k, const unsigned p, const double );
    void saveOpen( const unsigned k, const unsigned p, const double );
	
    /* Proxies */
	
    const Entity * dstTask() const;
    short index() const;
    double serviceTime() const;
	
public:
	
    /* There is now only one, and it has a type built in */
    LQIO::DOM::Call* getCallDOM() const { return myCallDOM; }
	
protected:
    const Phase* source;		/* Calling entry.		*/
    double myWait;			/* Waiting time.		*/

private:
    const Entry* destination;		/* to whom I am referring to	*/	

    /* Input */
    LQIO::DOM::Call* myCallDOM;
	
    unsigned chainNumber;  
};


/* -------------------------------------------------------------------- */

class TaskCall : public Call {
public:
    TaskCall( const Phase *, const Entry * toEntry );

    virtual void initWait();
};


class ForwardedCall : public TaskCall {
public:
    ForwardedCall( const Phase *, const Entry * toEntry, const Call * fwdCall );

    virtual bool isForwardedCall() const { return true; }
    virtual const char * srcName() const;
    virtual void insertDOMResults() const;

private:
    const Call * myFwdCall;		// Original forwarded call
};

class ProcessorCall : public Call {
public:
    ProcessorCall( const Phase *, const Entry * toEntry );

    virtual bool isProcessorCall() const { return true; }
    virtual void initWait();
    virtual void setWait(double newWait);
};


class ActivityCall : public TaskCall {
public:
    ActivityCall( const Phase * fromActivity, const Entry * toEntry );

    virtual bool isActivityCall() const { return true; }
    virtual const Entry * srcEntry() const;
    virtual const char * srcName() const;
    virtual const Task * srcTask() const;
};


class ActivityForwardedCall : public ActivityCall {
public:
    ActivityForwardedCall( const Phase *, const Entry * toEntry );

    virtual bool isForwardedCall() const { return true; }
};


class ActProcCall : public ProcessorCall {
public:
    ActProcCall( const Phase *, const Entry * toEntry );

    virtual bool isActivityCall() const { return true; }
    virtual const Entry * srcEntry() const;
    virtual const char * srcName() const;
    virtual const Task * srcTask() const;
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

/* ------------------------ Exception Handling ------------------------ */

class call_cycle : public path_error 
{
public:
    call_cycle( const Call *, const CallStack& );
    virtual ~call_cycle() throw() {}
};
#endif
