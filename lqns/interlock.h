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
 * $Id: interlock.h 13933 2020-10-15 20:14:58Z greg $
 *
 * ------------------------------------------------------------------------
 */

#if	!defined(INTERLOCK_H)
#define	INTERLOCK_H

#include "dim.h"
#include <set>
#include <deque>

class Interlock;
class InterlockInfo;
class Entry;
class Entity;
class Task;

ostream& operator<<( ostream&, const Interlock& );
ostream& operator<<( ostream&, const InterlockInfo& );

class InterlockInfo 
{
public:
    InterlockInfo( float a = 0.0, float p = 0.0 ) : all(a), ph1(p) {}
    InterlockInfo& operator=( const InterlockInfo& );
    bool operator==( const InterlockInfo& ) const;

    float all;	/* Calls from all phases at root.	*/	
    float ph1;	/* Calls from phase 1 only at root.	*/
};

/* --------------------------- Interlocker. --------------------------- */

class Interlock {

public:
    class Collect {
    public:
	Collect( const Entity * server, std::set<const Entity *>& interlockedTasks )
	    : _server(server),
	      _entryStack(),
	      _interlockedTasks( interlockedTasks )
	    {}

    private:
	Collect( const Collect& ); // = delete;
	Collect& operator=( const Collect& ); // = delete;

    public:
	const Entity * server() const { return _server; }
	bool headOfPath() const { return _entryStack.size() == 0; }
	bool allowPhase2() const { return _entryStack.size() == 1; }		/* Allow from the top-of-path entry only */

	void push_back( const Entry * entry ) { _entryStack.push_back( entry ); }
	void pop_back() { _entryStack.pop_back(); }
	const Entry * back() const { return _entryStack.back(); }
	std::pair<std::set<const Entity *>::const_iterator,bool> insert( const Entity * entity ) { return _interlockedTasks.insert( entity ); }
	bool hasEntry( const Entry * entry ) const { return std::find( _entryStack.begin(), _entryStack.end(), entry ) != _entryStack.end(); }
	
    public:
	const Entity * _server;				/* In */
	std::deque<const Entry *> _entryStack;		/* local */
	std::set<const Entity *>& _interlockedTasks;	/* Out */
    };

public:
    Interlock( const Entity * aServer );
    virtual ~Interlock();

    void initialize();

    ostream& print( ostream& output ) const;
    double interlockedFlow( const Task& viaTask ) const;

    static ostream& printPathTable( ostream& output );

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
    std::set<const Entry *> commonEntries;	/* common source entries	*/
    std::set<const Entity *> allSourceTasks;	/* Phase 1+ sources.		*/
    std::set<const Entity *> ph2SourceTasks;	/* Phase 2+ sources.		*/
    const Entity * myServer;			/* My server.			*/
    unsigned sources;
};

inline ostream& operator<<( ostream& output, const Interlock& self) { return self.print( output ); }

InterlockInfo& operator+=( InterlockInfo&, const InterlockInfo& );
InterlockInfo& operator-=( InterlockInfo&, const InterlockInfo& );
InterlockInfo operator*( const InterlockInfo&, double );
bool operator>( const InterlockInfo&, double  );
#endif
