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
 * $Id: interlock.h 13676 2020-07-10 15:46:20Z greg $
 *
 * ------------------------------------------------------------------------
 */

#if	!defined(INTERLOCK_H)
#define	INTERLOCK_H

#include "dim.h"
#include <set>
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
private:
    template <typename Type> static std::set<Type>
    intersection( const std::set<Type>& set1, const std::set<Type>& set2 )
	{
	    std::set<Type> result;
	    typename std::set<Type>::const_iterator item;
	    for ( item = set1.begin(); item != set1.end(); ++item ) {
		if ( set2.find( *item ) != set2.end() )
		    result.insert( *item );
	    }
	    return result;
	}

    template <typename Type> static std::set<Type>
    difference( const std::set<Type>& minuend, const std::set<Type>& subtrahend )
	{
	    std::set<Type> difference;
	    typename std::set<Type>::const_iterator item;
	    for ( item = minuend.begin(); item != minuend.end(); ++item ) {
		if ( subtrahend.find( *item ) == subtrahend.end() ) {
		    difference.insert( *item );
		}
	    }
	    return difference;
	}


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
