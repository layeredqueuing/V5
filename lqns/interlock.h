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
 * $Id: interlock.h 11963 2014-04-10 14:36:42Z greg $
 *
 * ------------------------------------------------------------------------
 */

#if	!defined(INTERLOCK_H)
#define	INTERLOCK_H

#include "dim.h"
#include "cltn.h"

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
    bool getInterlockedTasks( const int headOfPath, const Entry *, Cltn<const Entity *>& interlockedTasks ) const; 
    unsigned countSources( const Cltn<const Entity *>& );
	
private:
    Cltn<const Entry *> commonEntry;		/* common source entries	*/
    Cltn<const Entity *> allSourceTasks;	/* Phase 1+ sources.		*/
    Cltn<const Entity *> ph2SourceTasks;	/* Phase 2+ sources.		*/
    const Entity * myServer;			/* My server.			*/
    unsigned sources;
};


InterlockInfo& operator+=( InterlockInfo&, const InterlockInfo& );
InterlockInfo& operator-=( InterlockInfo&, const InterlockInfo& );
InterlockInfo operator*( const InterlockInfo&, double );
#endif
