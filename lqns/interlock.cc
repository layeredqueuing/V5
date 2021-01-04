/* -*- c++ -*-
 * $Id: interlock.cc 14319 2021-01-02 04:11:00Z greg $
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

#include <algorithm>
#include <cmath>
#include <algorithm>
#include <numeric>
#include "lqns.h"
#include "interlock.h"
#include "task.h"
#include "entry.h"
#include "option.h"
#include "model.h"

bool Interlock::CollectTasks::has_entry(const Entry * entry ) const { return std::find( _entryStack.begin(), _entryStack.end(), entry ) != _entryStack.end(); }
bool Interlock::CollectTable::has_entry(const Entry * entry ) const { return std::find( _entryStack.begin(), _entryStack.end(), entry ) != _entryStack.end(); }

/* -------------------------------------------------------------------- */
/* Funky Formatting functions for inline with <<.			*/
/* -------------------------------------------------------------------- */

class StringNManip {
public:
    StringNManip( std::ostream& (*ff)(std::ostream&, const std::string&, const unsigned ), const std::string& s, const unsigned n ) : f(ff), _s(s), _n(n) {}
private:
    std::ostream& (*f)( std::ostream&, const std::string&, const unsigned );
    const std::string& _s;
    const unsigned int _n;

    friend std::ostream& operator<<(std::ostream & os, const StringNManip& m ) { return m.f(os,m._s,m._n); }
};

StringNManip trunc( const std::string&, const unsigned );

/************************************************************************/
/*                     Throughput Interlock Model.                      */
/************************************************************************/

/*
 * Generate the interlock table.  It is impossible to interlock on
 * infinite servers since all calls (by definition) go to unique
 * instances.
 */

Interlock::Interlock( const Entity& aServer ) 
    : _commonEntries(),
      _allSourceTasks(),
      _ph2SourceTasks(),
      _server(aServer), 
      _sources(0)
{
}



Interlock::~Interlock()
{
    _commonEntries.clear();
    _allSourceTasks.clear();
    _ph2SourceTasks.clear();
}


void
Interlock::initialize()
{
    if ( !_server.isInfinite() ) {
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
	std::cout << "Interlock for server: " << _server.name() << std::endl;
    }

    /* Locate all callers to myServer */

    std::set<Task *> clients;
    _server.getClients( clients );

    for ( std::set<Task *>::const_iterator clientA = clients.begin(); clientA != clients.end(); ++clientA ) {
	if ( !(*clientA)->isUsed() ) continue;		/* Ignore this task - not used. */

	for ( std::set<Task *>::const_iterator clientC = clients.begin(); clientC != clients.end(); ++clientC ) {
	    if ( (*clientA) == (*clientC) || !(*clientC)->isUsed() ) continue;

	    for ( std::vector<Entry *>::const_iterator entryA = (*clientA)->entries().begin(); entryA != (*clientA)->entries().end(); ++entryA ) {
		for ( std::vector<Entry *>::const_iterator entryC = (*clientC)->entries().begin(); entryC != (*clientC)->entries().end(); ++entryC ) {
		    bool foundAB = false;
		    bool foundCD = false;

		    /* Check that both entries call me. */

		    for ( std::vector<Entry *>::const_iterator entry = _server.entries().begin(); entry != _server.entries().end(); ++entry ) {
			if ( (*entryA)->isInterlocked( (*entry) ) ) foundAB = true;
			if ( (*entryC)->isInterlocked( (*entry) ) ) foundCD = true;
		    }
		    if ( foundAB && foundCD ) {
			findParentEntries( *(*entryA), *(*entryC) );
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
	std::cout << "  Common parents for entries " << srcA.name() << " and " << srcC.name() << ": ";
    }

    /* Figure 6 in interlock paper. */

    for ( std::set<Task *>::const_iterator aTask = Model::__task.begin(); aTask != Model::__task.end(); ++aTask ) {

	/* x calls a, a calls b; y calls c, c calls d */

	for ( std::vector<Entry *>::const_iterator srcX = (*aTask)->entries().begin(); srcX != (*aTask)->entries().end(); ++srcX ) {
	    for ( std::vector<Entry *>::const_iterator srcY = (*aTask)->entries().begin(); srcY != (*aTask)->entries().end(); ++srcY ) {
		if ( (*srcX)->_interlock[a].all > 0.0 && (*srcY)->_interlock[c].all > 0.0 ) {

		    /* Prune here (branch point?) */

		    if ( Options::Debug::interlock() ) {
			std::cout << (*srcX)->name();
		    }

		    if ( isBranchPoint( *(*srcX), entryA, *(*srcY), entryC ) ) {
			_commonEntries.insert(*srcX);
			if ( Options::Debug::interlock() ) {
			    std::cout << "* ";
			}
		    } else if ( Options::Debug::interlock() ) {
			std::cout << " ";
		    }
		}
	    }
	}
    }

    if ( Options::Debug::interlock() ) {
	std::cout << std::endl;
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
    if ( _sources == 0 ) return 0.0;

    /* Find all flow from the common parent list to viaTask. */

    double sum = 0.0;
    for ( std::set<const Entry *>::const_iterator srcE = _commonEntries.begin(); srcE != _commonEntries.end(); ++srcE ) {
	for ( std::vector<Entry *>::const_iterator dstA = viaTask.entries().begin(); dstA != viaTask.entries().end(); ++dstA ) {
	    const Entity * srcTask = (*srcE)->owner();
	    const unsigned a = (*dstA)->entryId();

	    if ( (*srcE)->_interlock[a].all > 0.0 && (*srcE)->maxPhase() == 1 && _allSourceTasks.find( srcTask ) != _allSourceTasks.end() ) {
		sum += (*srcE)->throughput() * (*srcE)->_interlock[a].all;
	    } else if ((*srcE)->_interlock[a].ph1 > 0.0 && (*srcE)->maxPhase() > 1 && _allSourceTasks.find( srcTask )  != _allSourceTasks.end() ){
		sum += (*srcE)->throughput() * (*srcE)->_interlock[a].ph1;	
	    }
		
	    const double ph2 = (*srcE)->_interlock[a].all - (*srcE)->_interlock[a].ph1;
	    if ( ph2 > 0.0 && _ph2SourceTasks.find( srcTask ) != _ph2SourceTasks.end() ) {
		sum += (*srcE)->throughput() * ph2;
	    }
	    if ( flags.trace_interlock ) {
		std::cout << "  Interlock E=" << (*srcE)->name() << " A=" << (*dstA)->name() 
		     << " Throughput=" << (*srcE)->throughput() 
		     << ", interlock[" << a << "]={" << (*srcE)->_interlock[a].all << "," << ph2
		     << "}, sum=" << sum << std::endl;
	    }
	}
    }

    if ( flags.trace_interlock ) {
	std::cout << "Interlock sum=" << sum << ", viaTask: " << viaTask.throughput() 
	     << ", threads=" << viaTask.concurrentThreads()  << ", sources=" << _sources << std::endl;
    }
    return std::min( sum, viaTask.throughput() ) / (viaTask.throughput() * viaTask.concurrentThreads() * _sources );
}



/*
 * Go through the interlock list and remove entries from parents 
 * See prune above.
 */

void
Interlock::pruneInterlock()
{
    /* For all tasks in common entry... subtract off their common entries */

    std::set<const Entry *> prune;
    for ( std::set<const Entry *>::const_iterator i = _commonEntries.begin(); i != _commonEntries.end(); ++i ) {
	const Entity * dst = (*i)->owner();
	const std::set<const Entry *>& dst_entries = dst->commonEntries();
	for ( std::set<const Entry *>::const_iterator entry = dst_entries.begin(); entry != dst_entries.end(); ++entry ) {
	    _commonEntries.erase( *entry );		// Nop if not found
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
    std::set<const Entity *> interlockedTasks;

    /* Look for all parent tasks */

    for ( std::set<const Entry *>::const_iterator entry = _commonEntries.begin(); entry != _commonEntries.end(); ++entry ) {
	const Entity * aTask = (*entry)->owner();

	/* Add tasks corresponding to branch point entries */

	_allSourceTasks.insert( aTask );

	if ( (*entry)->maxPhase() > 1 ) {
	    _ph2SourceTasks.insert( aTask );
	}

	/* Locate all tasks on interlocked paths. */

	CollectTasks data( _server, interlockedTasks );
	(*entry)->getInterlockedTasks( data );
    }

    /*
     * Prune out tasks in sourceTasks that are also in
     * interlockedTasks for allSrcTasks.  Ph2Tasks is the opposite
     * of what was prunded out as phase 2 sources are independent.
     */

#ifdef	DEBUG_INTERLOCK
    if ( Options::Debug::interlock() ) {
	std::cout <<         "    All Sourcing Tasks: ";
	for ( std::set<const Entity *>::const_iterator task = _allSourceTasks.begin(); task != _allSourceTasks.end(); ++task ) {
	    std::cout << (*task)->name() << " ";
	}
	std::cout << std::endl << "    Interlocked Tasks:  ";
	for ( std::set<const Entity *>::const_iterator task = interlockedTasks.begin(); task != interlockedTasks.end(); ++task ) {
	    std::cout << (*task)->name() << " ";
	}
	std::cout << std::endl;
    }
#endif

    std::set<const Entity *> difference;
    std::set_difference( _allSourceTasks.begin(), _allSourceTasks.end(),
			 interlockedTasks.begin(), interlockedTasks.end(),
			 std::inserter( difference, difference.end() ) );
    _allSourceTasks = difference;

    std::set<const Entity *> intersection;
    std::set_intersection( _ph2SourceTasks.begin(), _ph2SourceTasks.end(),
			   interlockedTasks.begin(), interlockedTasks.end(),
			   std::inserter( intersection, intersection.end() ) );
    _ph2SourceTasks = intersection;

#ifdef	DEBUG_INTERLOCK
    if ( Options::Debug::interlock() ) {
	std::cout <<         "    Common Parent Tasks (all): ";
	for ( std::set<const Entity *>::const_iterator task = _allSourceTasks.begin(); task != _allSourceTasks.end(); ++task ) {
	    std::cout << (*task)->name() << " ";
	}
	std::cout << std::endl << "    Common Parent Tasks (Ph2): ";
	for ( std::set<const Entity *>::const_iterator task = _ph2SourceTasks.begin(); task != _ph2SourceTasks.end(); ++task ) {
	    std::cout << (*task)->name() << " ";
	}
	std::cout << std::endl;
    }
#endif

    /* Now count up the instances on each task in sourceTasks */
    /* And add on all other sources */

    _sources = countSources( interlockedTasks );

    /* Useful trivia. */

    if ( Options::Debug::interlock() ) {
	for ( std::set<const Entity *>::const_iterator task = interlockedTasks.begin(); task != interlockedTasks.end(); ++task ) {
	    std::cout << (*task)->name() << " ";
	}
	std::cout << std::endl << "    Sourcing Tasks:    ";
	for ( std::set<const Entity *>::const_iterator task = _allSourceTasks.begin(); task != _allSourceTasks.end(); ++task ) {
	    std::cout << (*task)->name() << " ";
	}
	std::cout << std::endl << std::endl;
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
	std::cout << "  isBranchPoint: " << srcX.name() << (char *)(srcX.isProcessorEntry() ? "*, " : ", ")
	     << entryA.name() << (char *)(entryA.isProcessorEntry() ? "*, " : ", ")
	     << srcY.name() << (char *)(srcY.isProcessorEntry() ? "*, " : ", ")
	     << entryB.name() << (char *)(entryB.isProcessorEntry() ? "*, " : ", ")
	     << std::endl;
    }
#endif

    /*
     * Sequence over all calls from X and Y and see if they go to
     * different tasks.  Only consider those entries which ultimately
     * go to a and b respectively
     */

    CallInfo nextX( srcX, Call::RENDEZVOUS_CALL );
    CallInfo nextY( srcY, Call::RENDEZVOUS_CALL );

    const unsigned a = entryA.entryId();
    const unsigned b = entryB.entryId();

    for ( std::vector<CallInfoItem>::const_iterator yxe = nextX.begin(); yxe != nextX.end(); ++yxe ) {
	const Entry * dstE = yxe->dstEntry();

	if ( dstE->_interlock[a].all == 0.0 ) continue;

	for ( std::vector<CallInfoItem>::const_iterator yyf = nextY.begin(); yyf != nextY.end(); ++yyf ) {
	    const Entry * dstF = yyf->dstEntry();

	    if ( dstF->_interlock[b].all > 0.0 && dstE->owner() != dstF->owner() ) return true;
	}
    }

    return false;
}



/*
 * This procedure is used to locate all of the other sources that are along the
 * call paths.  Tasks found cannot be in sources or in paths.
 */

unsigned
Interlock::countSources( const std::set<const Entity *>& interlockedTasks )
{
    /*
     * Sequence over arriving arcs and add task to sources
     * provided that it is not in paths.
     */

    for ( std::set<const Entity *>::const_iterator task = interlockedTasks.begin(); task != interlockedTasks.end(); ++task ) {
	const std::vector<Entry *>& entries = (*task)->entries();
	for ( std::vector<Entry *>::const_iterator entry = entries.begin(); entry != entries.end(); ++entry ) {
	    const std::set<Call *>& callerList = (*entry)->callerList();
	    for ( std::set<Call *>::const_iterator call = callerList.begin(); call != callerList.end(); ++call ) {
		if ( !(*call)->hasRendezvous() ) continue;

		const Task * parentTask = (*call)->srcTask();
		if ( interlockedTasks.find( parentTask ) == interlockedTasks.end() ) {
		    _allSourceTasks.insert( parentTask );
		}
	    }
	}
    }

    /*
     * Now count up all the copies of sourcing tasks.  Note that we
     * have to use the special population() call to account
     * properly for infinite servers.
     */

    unsigned n = std::accumulate( _allSourceTasks.begin(), _allSourceTasks.end(), 0., add_using<const Entity>( &Entity::population ) );

    /*
     * Add in phase 2 sources of tasks that are on the interlock
     * path.  These tasks count as additional sources because phase
     * 2+ flow is (quasi) independent of phase 1 flow.  Ignore all
     * tasks that are immediate parents of the interlockee (mva
     * interlock attends to this case).
     */

    const std::vector<Entry *>& dst_entries = _server.entries();
    for ( std::set<const Entity *>::const_iterator task = interlockedTasks.begin(); task != interlockedTasks.end(); ++task ) {
	const std::vector<Entry *>& src_entries = (*task)->entries();
	for ( std::vector<Entry *>::const_iterator srcEntry = src_entries.begin(); srcEntry != src_entries.end(); ++srcEntry ) {
	    for ( std::vector<Entry *>::const_iterator dstEntry = dst_entries.begin(); dstEntry != dst_entries.end(); ++dstEntry ) {
		const unsigned e = (*dstEntry)->entryId();
		const double ph2 = (*srcEntry)->_interlock[e].all - (*srcEntry)->_interlock[e].ph1;
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

std::ostream&
Interlock::print( std::ostream& output ) const
{
    output << _server.name() << ": Sources=" << _sources << ", entries: " ;

    for ( std::set<const Entry *>::const_iterator srcE = _commonEntries.begin(); srcE != _commonEntries.end(); ++srcE ) {
	output << (*srcE)->name() << " ";
    }

    return output;
}

/*
 * Print out path table.
 */

std::ostream&
Interlock::printPathTable( std::ostream& output )
{
    std::set<Entry *>::const_iterator srcEntry;
    std::set<Entry *>::const_iterator dstEntry;
    static const unsigned int columns = 5;

    std::ios_base::fmtflags oldFlags = output.setf( std::ios::left, std::ios::adjustfield );
    output << "src\\dst   ";
    unsigned i;
    unsigned j;
    for ( i = 1, srcEntry = Model::__entry.begin(); srcEntry != Model::__entry.end(); ++srcEntry, ++i ) {
	const Entry * anEntry = *srcEntry;
	output << trunc( anEntry->name(), 10 );
	if ( i % columns == 0 ) {
	    output << ' ';
	}
    }
    output << std::endl;

    for ( i = 0, srcEntry = Model::__entry.begin(); srcEntry != Model::__entry.end(); ++srcEntry, ++i ) {
	const Entry * src = *srcEntry;

	if ( i % columns == 0 ) {
	    output << "----------";
	    for ( j = 1; j <= Model::__entry.size(); ++j ) {
		output << "----------";
		if ( j % columns == 0 ) {
		    output << '+';
		}
	    }
	    output << std::endl;
	}

	output << trunc( src->name(), 10 );
	for ( j = 1, dstEntry = Model::__entry.begin(); dstEntry != Model::__entry.end(); ++dstEntry, ++j ) {
	    output.setf( std::ios::right, std::ios::adjustfield );
	    output << std::setw(4) << src->_interlock[(*dstEntry)->entryId()].all << ",";
	    output.setf( std::ios::left, std::ios::adjustfield );
	    output << std::setw(4) << src->_interlock[(*dstEntry)->entryId()].ph1 << " ";
	    if ( j % columns == 0 ) {
		output << '|';
	    }
	}
	output << std::endl;
    }
    output.flags(oldFlags);
    output << std::endl;

    return output;
}

bool 
InterlockInfo::operator==( const InterlockInfo& arg ) const
{
    return all == arg.all && ph1 == arg.ph1;
}

std::ostream&
operator<<( std::ostream& output, const InterlockInfo& self )
{
    output << self.all << self.ph1 << std::endl;
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

bool 
operator>( const InterlockInfo& lhs, double rhs )
{
    return ((lhs.all>rhs) || (lhs.ph1>rhs));
}

static std::ostream& trunc_str( std::ostream& output, const std::string& s, const unsigned n ) 
{
    if ( s.size() > n ) {
	output.write( s.c_str(), n );
    } else {
	std::ios_base::fmtflags oldFlags = output.setf( std::ios::left, std::ios::adjustfield );
	output << std::setw(n) << std::setfill( ' ' ) << s;
	output.flags(oldFlags);
    }
    return output;
}

StringNManip trunc( const std::string& s, const unsigned n ) { return StringNManip( trunc_str, s, n ); }
