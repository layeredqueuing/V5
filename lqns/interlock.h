/* -*- c++ -*-
 * $HeadURL: http://rads-svn.sce.carleton.ca:8080/svn/lqn/trunk-V5/lqns/interlock.h $
 *
 * Layer-ization of model.
 *
 * Copyright the Real-Time and Distributed Systems Group,
 * Department of Systems and Computer Engineering,
 * Carleton University, Ottawa, Ontario, Canada. K1S 5B6
 *
 * November, 1994
 *
 * $Id: interlock.h 14140 2020-11-25 20:24:15Z greg $
 *
 * ------------------------------------------------------------------------
 */

#if	!defined(INTERLOCK_H)
#define	INTERLOCK_H

#include "dim.h"
#include <set>
#include <deque>

class Entry;
class Entity;
class Task;

struct InterlockInfo 
{
    InterlockInfo( float a = 0.0, float p = 0.0 ) : all(a), ph1(p) {}
    InterlockInfo& operator=( const InterlockInfo& );
    bool operator==( const InterlockInfo& ) const;
    InterlockInfo& reset() { all = 0.; ph1 = 0.; return *this; }

    float all;	/* Calls from all phases at root.	*/	
    float ph1;	/* Calls from phase 1 only at root.	*/
};

/* --------------------------- Interlocker. --------------------------- */

class Interlock {

public:
    class CollectTasks {
    public:
	CollectTasks( const Entity& server, std::set<const Entity *>& interlockedTasks )
	    : _server(server),
	      _entryStack(),
	      _interlockedTasks( interlockedTasks )
	    {}

    private:
	CollectTasks( const CollectTasks& ); // = delete;
	CollectTasks& operator=( const CollectTasks& ); // = delete;

    public:
	const Entity * server() const { return &_server; }
	bool headOfPath() const { return _entryStack.size() == 0; }
	bool prune() const { return _entryStack.size() > 1; }		/* Allow from the top-of-path entry only */

	void push_back( const Entry * entry ) { _entryStack.push_back( entry ); }
	void pop_back() { _entryStack.pop_back(); }
	const Entry * back() const { return _entryStack.back(); }
	std::pair<std::set<const Entity *>::const_iterator,bool> insert( const Entity * entity ) { return _interlockedTasks.insert( entity ); }
	bool has_entry( const Entry * entry ) const;
	
    private:
	const Entity& _server;				/* In */
	std::deque<const Entry *> _entryStack;		/* local */
	std::set<const Entity *>& _interlockedTasks;	/* Out */
    };

    class CollectTable {
    public:
	CollectTable() : _entryStack(), _phase2(false), _calls(1.0,1.0) {}
	CollectTable( const CollectTable& src, bool phase2 ) : _entryStack(src._entryStack), _phase2(phase2), _calls(src._calls) {}
	CollectTable( const CollectTable& src, const InterlockInfo& calls ) : _entryStack(src._entryStack), _phase2(src._phase2), _calls(calls)
	    {
		if ( _phase2 ) _calls.ph1 = 0.0;
	    }

    private:
	CollectTable( const CollectTable& ); // = delete
	CollectTable& operator=( const CollectTable& ); // = delete

    public:
	bool prune() const { return _entryStack.size() > 1 && _phase2; }
	InterlockInfo& calls() { return _calls; }
	const InterlockInfo& calls() const { return _calls; }
	
	void push_back( const Entry * entry ) { _entryStack.push_back( entry ); }
	void pop_back() { _entryStack.pop_back(); }
	const Entry * front() const { return _entryStack.front(); }
	const Entry * back() const { return _entryStack.back(); }
	bool has_entry( const Entry * entry ) const;

    private:
	std::deque<const Entry *> _entryStack;		/* local */
	bool _phase2;					/* local */
	InterlockInfo _calls;				/* out */
    };

public:
    Interlock( const Entity& aServer );
    virtual ~Interlock();

    void initialize();

    std::ostream& print( std::ostream& output ) const;
    double interlockedFlow( const Task& viaTask ) const;
    const std::set<const Entry *>& commonEntries() const { return _commonEntries; }

    static std::ostream& printPathTable( std::ostream& output );

private:
    Interlock( const Interlock& );
    Interlock& operator=( const Interlock& );

private:
    void findInterlock();
    void pruneInterlock();
    void findSources();
    void findParentEntries( const Entry&, const Entry& );

    bool isBranchPoint( const Entry& srcX, const Entry& entryA, const Entry& srcY, const Entry& entryB ) const;
    bool getInterlockedTasks( const int headOfPath, const Entry *, std::set<const Entity *>& interlockedTasks ) const; 
    unsigned countSources( const std::set<const Entity *>& );

private:
    std::set<const Entry *> _commonEntries;	/* common source entries	*/
    std::set<const Entity *> _allSourceTasks;	/* Phase 1+ sources.		*/
    std::set<const Entity *> _ph2SourceTasks;	/* Phase 2+ sources.		*/
    const Entity& _server;			/* My server.			*/
    unsigned _sources;
};

inline std::ostream& operator<<( std::ostream& output, const Interlock& self) { return self.print( output ); }

InterlockInfo& operator+=( InterlockInfo&, const InterlockInfo& );
InterlockInfo& operator-=( InterlockInfo&, const InterlockInfo& );
InterlockInfo operator*( const InterlockInfo&, double );

std::ostream& operator<<( std::ostream&, const Interlock& );
std::ostream& operator<<( std::ostream&, const InterlockInfo& );
bool operator>( const InterlockInfo&, double  );
#endif
