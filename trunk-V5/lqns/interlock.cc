/* -*- c++ -*-
 * $Id$
 *
 * Call-chain/interlock finder.
 *
 * Copyright the Real-Time and Distributed Systems Group,
 * Department of Systems and Computer Engineering,
 * Carleton University, Ottawa, Ontario, Canada. K1S 5B6
 *
 * April 2003
 *
 * ------------------------------------------------------------------------
 */

#include "interlock.h"
#include "task.h"
#include "entry.h"
#include "lqns.h"
#include "cltn.h"
#include "stack.h"
#include "option.h"

ostream&
operator<<( ostream& output, const Interlock& self )
{
    return self.print( output );
}

/* -------------------------------------------------------------------- */
/* Funky Formatting functions for inline with <<.			*/
/* -------------------------------------------------------------------- */

class StringNManip {
public:
    StringNManip( ostream& (*ff)(ostream&, const char *, const unsigned ), const char * s, const unsigned n ) : f(ff), _s(s), _n(n) {}
private:
    ostream& (*f)( ostream&, const char *, const unsigned );
    const char * _s;
    const unsigned int _n;

    friend ostream& operator<<(ostream & os, const StringNManip& m ) { return m.f(os,m._s,m._n); }
};

StringNManip trunc( const char *, const unsigned );

/************************************************************************/
/*                     Throughput Interlock Model.                      */
/************************************************************************/

/*
 * Generate the interlock table.  It is impossible to interlock on
 * infinite servers since all calls (by definition) go to unique
 * instances.
 */

Interlock::Interlock( const Entity * aServer ) 
    : myServer(aServer), 
      sources(0)
{
    initialize();
}



Interlock::~Interlock()
{
    commonEntry.clearContents();
    allSourceTasks.clearContents();
    ph2SourceTasks.clearContents();
}


void
Interlock::initialize()
{
    if ( !myServer->isInfinite() ) {
	findInterlock();
	pruneInterlock();
	findSources();
    }
}


/*
 * This function searches for interlock paths among all the callers
 * to `aTask'.
 */

void
Interlock::findInterlock()
{
    if ( Options::Debug::interlock() ) {
	cout << "Interlock for server: " << myServer->name() << endl;
    }

    Sequence<Entry *> nextEntry( myServer->entries() );
    const Entry * anEntry;

    /* Locate all callers to myServer */

    Cltn<Task *> myClients;
    myServer->clients( myClients );
    Sequence<Task *> nextAClient( myClients );
    Sequence<Task *> nextCClient( myClients );
    const Task * clientA;
    const Task * clientC;

    while ( clientA = nextAClient() ) {
	Sequence<Entry *> nextA( clientA->entries() );
	Entry * entryA;

	if ( !clientA->isUsed() ) continue;		/* Ignore this task - not used. */

	while ( clientC = nextCClient() ) {
	    if ( clientA == clientC || !clientC->isUsed() ) continue;

	    Sequence<Entry *> nextC( clientC->entries() );
	    Entry * entryC;

	    while ( entryA = nextA() ) {
		while ( entryC = nextC() ) {
		    bool foundAB = false;
		    bool foundCD = false;

		    /* Check that both entries call me. */

		    while ( anEntry = nextEntry() ) {
			if ( entryA->isInterlocked( anEntry ) ) foundAB = true;
			if ( entryC->isInterlocked( anEntry ) ) foundCD = true;
		    }
		    if ( foundAB && foundCD ) {
			findParentEntries( *entryA, *entryC );
		    }
		}
	    }
	}
    }
}




/*
 * Locate all common parent entries to A and C.
 */

void
Interlock::findParentEntries( const Entry& srcA, const Entry& srcC )
{
    const Entry& entryA = srcA;
    const Entry& entryC = srcC;
    const unsigned a = entryA.entryId();
    const unsigned c = entryC.entryId();

    if ( Options::Debug::interlock() ) {
	cout << "  Common parents for entries " << srcA.name() << " and " << srcC.name() << ": ";
    }

    /* Figure 6 in interlock paper. */

    for ( set<Task *,ltTask>::const_iterator aTaskIter = task.begin(); aTaskIter != task.end(); ++aTaskIter ) {
	const Task * aTask = *aTaskIter;
	if ( !aTask ) continue;

	Sequence<Entry *> nextX( aTask->entries() );
	Sequence<Entry *> nextY( aTask->entries() );

	const Entry *srcX;
	const Entry *srcY;

	/* x calls a, a calls b; y calls c, c calls d */

	while ( srcX = nextX() ) {
	    while ( srcY = nextY() ) {
		if ( srcX->_interlock[a].all > 0.0 && srcY->_interlock[c].all > 0.0 ) {

		    /* Prune here (branch point?) */

		    if ( Options::Debug::interlock() ) {
			cout << srcX->name();
		    }

		    if ( isBranchPoint( *srcX, entryA, *srcY, entryC ) ) {
			commonEntry += srcX;
			if ( Options::Debug::interlock() ) {
			    cout << "* ";
			}
		    } else if ( Options::Debug::interlock() ) {
			cout << " ";
		    }
		}
	    }
	}
    }

    if ( Options::Debug::interlock() ) {
	cout << endl;
    }
}



/*
 * Given a set of entries that are branch points, find interlocked flow.
 * Interlock probability is affected by the number of threads of viaTask.
 * This method is called each time we generate an MVA model.
 */

double
Interlock::interlockedFlow( const Task& viaTask ) const
{
    if ( sources == 0 ) return 0.0;

    Sequence<const Entry *> nextE( commonEntry );
    const Entry * srcE;

    /* Find all flow from the common parent list to viaTask. */

    double sum = 0.0;
    while ( srcE = nextE() ) {
	Sequence<Entry *> nextA( viaTask.entries() );
	const Entry * dstA;

	while ( dstA = nextA() ) {
	    const Entity * srcTask = srcE->owner();
	    const unsigned a = dstA->entryId();

	    if ( srcE->_interlock[a].all > 0.0 && allSourceTasks.find( srcTask ) ) {
		sum += srcE->throughput() * srcE->_interlock[a].all;
	    }
	    const double ph2 = srcE->_interlock[a].all - srcE->_interlock[a].ph1;
	    if ( ph2 > 0.0 && ph2SourceTasks.find( srcTask ) ) {
		sum += srcE->throughput() * ph2;
	    }
	    if ( flags.trace_interlock ) {
		cout << "  Interlock E=" << srcE->name() << " A=" << dstA->name() 
		     << " Throughput=" << srcE->throughput() 
		     << ", interlock[" << a << "]={" << srcE->_interlock[a].all << "," << ph2
		     << "}, sum=" << sum << endl;
	    }
	}
    }

    if ( flags.trace_interlock ) {
	cout << "Interlock sum=" << sum << ", viaTask: " << viaTask.throughput() 
	     << ", threads=" << viaTask.concurrentThreads()  << ", sources=" << sources << endl;
    }
    return min( sum, viaTask.throughput() ) / (viaTask.throughput() * viaTask.concurrentThreads() * sources );
}



/*
 * Go through the interlock list and remove entries from parents 
 * See prune above.
 */

void
Interlock::pruneInterlock()
{
    /* For all tasks in common entry... subtract off their common entries */

    for ( unsigned i = commonEntry.size(); i > 0; --i ) {

	const Entity * anEntity = commonEntry[i]->owner();
	if ( !anEntity->interlock ) continue;

	Sequence<const Entry *> nextEntry( anEntity->interlock->commonEntry );
	const Entry * anEntry;

	while ( anEntry = nextEntry() ) {
	    const unsigned j = commonEntry.find( anEntry );
	    if ( j > 0 ) {
		if ( i > j ) {
		    i -= 1;
		}
		commonEntry -= anEntry;
	    }
	}
    }
}



/*
 * Find all sources for flow along interlock paths by descending down the
 * call paths to ``myServer'' or some other terminus.  Only count those
 * paths which actually go to ``myServer''.
 */

void
Interlock::findSources()
{
    Cltn<const Entity *> interlockedTasks;
    const Entity * aTask;

    /* Look for all parent tasks */

    Stack<const Entry *> entryStack( io_vars.n_tasks + 2 );
    Sequence<const Entry *> nextEntry( commonEntry );
    const Entry * anEntry;
    while ( anEntry = nextEntry() ) {
	aTask = anEntry->owner();

	/* Add tasks corresponding to branch point entries */

	allSourceTasks += aTask;

	if ( anEntry->maxPhase() > 1 ) {
	    ph2SourceTasks += aTask;
	}

	/* Locate all tasks on interlocked paths. */

	anEntry->getInterlockedTasks( entryStack, myServer, interlockedTasks );
    }

    /*
     * Prune out tasks in sourceTasks that are also in
     * interlockedTasks for allSrcTasks.  Ph2Tasks is the opposite
     * of what was prunded out as phase 2 sources are independent.
     */

#ifdef	DEBUG_INTERLOCK
    if ( Options::Debug::interlock() ) {
	Sequence<const Entity *> nextSrcTask( allSourceTasks );
	Sequence<const Entity *> nextIntTask( interlockedTasks );

	cout <<         "    All Sourcing Tasks: ";
	while ( aTask = nextSrcTask() ) {
	    cout << aTask->name() << " ";
	}
	cout << endl << "    Interlocked Tasks:  ";
	while ( aTask = nextIntTask() ) {
	    cout << aTask->name() << " ";
	}
	cout << endl;
    }
#endif

    allSourceTasks -= interlockedTasks;
    ph2SourceTasks &= interlockedTasks;

#ifdef	DEBUG_INTERLOCK
    if ( Options::Debug::interlock() ) {
	Sequence<const Entity *> nextSrcTask( allSourceTasks );
	Sequence<const Entity *> nextPh2Task( ph2SourceTasks );

	cout <<         "    Common Parent Tasks (all): ";
	while ( aTask = nextSrcTask() ) {
	    cout << aTask->name() << " ";
	}
	cout << endl << "    Common Parent Tasks (Ph2): ";
	while ( aTask = nextPh2Task() ) {
	    cout << aTask->name() << " ";
	}
	cout << endl;
    }
#endif

    /* Now count up the instances on each task in sourceTasks */
    /* And add on all other sources */

    sources = countSources( interlockedTasks );

    /* Useful trivia. */

    if ( Options::Debug::interlock() ) {
	Sequence<const Entity *> nextIntTask( interlockedTasks );
	Sequence<const Entity *> nextSrcTask( allSourceTasks );

	cout << "    Interlocked Tasks: ";
	while ( aTask = nextIntTask() ) {
	    cout << aTask->name() << " ";
	}
	cout << endl << "    Sourcing Tasks:    ";
	while ( aTask = nextSrcTask() ) {
	    cout << aTask->name() << " ";
	}
	cout << endl << endl;
    }
}



/*
 * Return 1 if X and Y follow paths to different tasks on the
 * correct call path.
 */

bool
Interlock::isBranchPoint( const Entry& srcX, const Entry& entryA, const Entry& srcY, const Entry& entryB ) const
{

    /*
     * I have to ensure that a call to myself (which is not feasible...)
     */

    if ( srcX.owner() == entryA.owner()
	 && srcY.owner() == entryB.owner()
	 && entryA.owner() == entryB.owner() ) return false;	// Multiserver client

    /*
     * Quick check -- send interlock?
     */

    if ( srcX == entryA || srcY == entryB ) return true;

#ifdef	NOTDEF
    if ( Options::Debug::interlock() ) {
	cout << "  isBranchPoint: " << srcX.name() << (char *)(srcX.isProcessorEntry() ? "*, " : ", ")
	     << entryA.name() << (char *)(entryA.isProcessorEntry() ? "*, " : ", ")
	     << srcY.name() << (char *)(srcY.isProcessorEntry() ? "*, " : ", ")
	     << entryB.name() << (char *)(entryB.isProcessorEntry() ? "*, " : ", ")
	     << endl;
    }
#endif

    /*
     * Sequence over all calls from X and Y and see if they go to
     * different tasks.  Only consider those entries which ultimately
     * go to a and b respectively
     */

    CallInfo nextX( &srcX, Call::RENDEZVOUS_CALL );
    CallInfo nextY( &srcY, Call::RENDEZVOUS_CALL );

    const CallInfoItem * yxe;
    const CallInfoItem * yyf;
    const unsigned a = entryA.entryId();
    const unsigned b = entryB.entryId();

    while ( yxe = nextX() ) {
	const Entry * dstE = yxe->dstEntry();

	if ( dstE->_interlock[a].all == 0.0 ) continue;

	while ( yyf = nextY() ) {
	    const Entry * dstF = yyf->dstEntry();

	    if ( dstF->_interlock[b].all > 0.0 && dstE->owner() != dstF->owner() ) return 1;
	}
    }

    return false;
}



/*
 * This procedure is used to locate all of the other sources that are along the
 * call paths.  Tasks found cannot be in sources or in paths.
 */

unsigned
Interlock::countSources( const Cltn<const Entity *>& paths )
{
    const Entity * aTask;

    /*
     * Sequence over arriving arcs and add task to sources
     * provided that it is not in paths.
     */

    Sequence<const Entity *> nextPathTask( paths );

    while ( aTask = nextPathTask() ) {

	Sequence<Entry *> nextEntry( aTask->entries() );
	const Entry * anEntry;

	while ( anEntry = nextEntry() ) {

	    Sequence<Call *> nextCall( anEntry->callerList() );
	    const Call * parent;

	    while ( parent = nextCall() ) {
		if ( !parent->hasRendezvous() ) continue;

		aTask = parent->srcTask();
		if ( !paths.find( aTask ) ) {
		    allSourceTasks += aTask;
		}
	    }
	}
    }

    /*
     * Now count up all the copies of sourcing tasks.  Note that we
     * have to use the special population() call to account
     * properly for infinite servers.
     */

    Sequence<const Entity *> nextSrcTask( allSourceTasks );

    unsigned n = 0;
    while ( aTask = nextSrcTask() ) {
	n += static_cast<unsigned>(aTask->population());
    }

    /*
     * Add in phase 2 sources of tasks that are on the interlock
     * path.  These tasks count as additional sources because phase
     * 2+ flow is (quasi) independent of phase 1 flow.  Ignore all
     * tasks that are immediate parents of the interlockee (mva
     * interlock attends to this case).
     */

    Sequence<Entry *> nextDstEntry( myServer->entries() );
    while ( aTask = nextPathTask() ) {

	Sequence<Entry *> nextSrcEntry( aTask->entries() );
	const Entry * srcEntry;

	while ( srcEntry = nextSrcEntry() ) {

	    const Entry * dstEntry;

	    while ( dstEntry = nextDstEntry() ) {
		const unsigned e = dstEntry->entryId();
		const double ph2 = srcEntry->_interlock[e].all - srcEntry->_interlock[e].ph1;
		if ( ph2 > 0.0 ) {
		    n += 1;
		    goto nextTask;	/* Any hit counts... */
		}
	    }
	}

    nextTask: ;
    }

    /* All done! */

    return n;
}



/*
 * Print path information.
 */

ostream&
Interlock::print( ostream& output ) const
{
    output << myServer->name() << ": Sources=" << sources << ", entries: " ;

    Sequence<const Entry *> nextEntry( commonEntry );
    const Entry * anEntry;

    while ( anEntry = nextEntry() ) {
	output << anEntry->name() << " ";
    }

    return output;
}

/*
 * Print out path table.
 */

ostream&
Interlock::printPathTable( ostream& output )
{
    set<Entry *,ltEntry>::const_iterator srcEntry;
    set<Entry *,ltEntry>::const_iterator dstEntry;

    output << "src\\dst ";
    unsigned i;
    unsigned j;
    for ( i = 1, srcEntry = entry.begin(); srcEntry != entry.end(); ++srcEntry, ++i ) {
	const Entry * anEntry = *srcEntry;
	output << trunc( anEntry->name(), 8 );
	if ( i % 8 == 0 ) {
	    output << ' ';
	}
    }
    output << endl;

    for ( i = 0, srcEntry = entry.begin(); srcEntry != entry.end(); ++srcEntry, ++i ) {
	const Entry * src = *srcEntry;

	if ( i % 9 == 0 ) {
	    output << "--------";
	    for ( j = 1; j <= entry.size(); ++j ) {
		output << "--------";
		if ( j % 8 == 0 ) {
		    output << '+';
		}
	    }
	    output << endl;
	}

	output << trunc( src->name(), 8 );
	for ( j = 1, dstEntry = entry.begin(); dstEntry != entry.end(); ++dstEntry, ++j ) {
	    const Entry * dst = *dstEntry;
	    output << setw(8) << src->_interlock[dst->entryId()].all;
	    if ( j % 8 == 0 ) {
		output << '|';
	    }
	}
	output << endl;
    }
    output << endl;

    return output;
}

bool 
InterlockInfo::operator==( const InterlockInfo& arg ) const
{
    return all == arg.all && ph1 == arg.ph1;
}

ostream&
operator<<( ostream& output, const InterlockInfo& self )
{
    output << self.all << self.ph1 << endl;
    return output;
}


InterlockInfo&
InterlockInfo::operator=( const InterlockInfo& anEntry )
{
    if ( this == &anEntry ) return *this;

    ph1 = anEntry.ph1;
    all = anEntry.all;

    return *this;
}



InterlockInfo&
operator+=( InterlockInfo& arg1, const InterlockInfo& arg2 )
{
    arg1.ph1 += arg2.ph1;
    arg1.all += arg2.all;

    return arg1;
}



InterlockInfo&
operator-=( InterlockInfo& arg1, const InterlockInfo& arg2 )
{
    arg1.ph1 -= arg2.ph1;
    arg1.all -= arg2.all;

    return arg1;
}


InterlockInfo
operator*( const InterlockInfo& multiplicand, double multiplier )
{
    InterlockInfo product;

    product.all = multiplicand.all * multiplier;
    product.ph1 = multiplicand.ph1 * multiplier;

    return product;
}

static ostream& trunc_str( ostream& output, const char * s, const unsigned n ) 
{
    if ( strlen( s ) > n ) {
	output.write( s, n );
    } else {
	output << setw(n) << setfill( ' ' ) << s;
    }
    return output;
}

StringNManip trunc( const char * s, const unsigned n ) { return StringNManip( trunc_str, s, n ); }
