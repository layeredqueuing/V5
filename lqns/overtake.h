/*  -*- c++ -*-
 * $HeadURL: svn://192.168.2.10/lqn/trunk-V5/lqns/overtake.h $ -- Greg Franks
 *
 * $Id: overtake.h 11963 2014-04-10 14:36:42Z greg $
 */

#ifndef _OVERTAKE_H
#define _OVERTAKE_H

#include "dim.h"
#include <lqio/input.h>
#include "vector.h"
#include "cltn.h"
#include "prob.h"
#include "slice.h"

class Entry;
class Task;
class Entity;
class Submodel;

class Overtaking
{
private:

    /*
     * Information needed for the markov phased server.
     */

    class ijInfo {
    public:
	ijInfo() { configure(); }

	void initialize( const Task * srcTask, const Entity * dstTask );
	
	double rendezvous() const { return myRendezvous.sum(); }
	double rendezvous( const unsigned p ) const { return myRendezvous[p]; }

    private:
	void configure();

    private:
	VectorMath<double> myRendezvous;
    };

public:
    class PrintHelper {
    public:
	PrintHelper( ostream& output ) : _output( output ) {}
	void operator()( const Entry& entA, const Entry& entB, const Entry& entC, const Entry& entD, const unsigned j, const Probability pr[] ) const;

    private:
	ostream& _output;
    };

public:
    Overtaking( const Task* client, const Entity* server );

    void compute( const PrintHelper * = 0 );
    ostream& print( ostream& );
	
private:
    void clearOvertakingStates(const unsigned max_phases, Probability PrOTState[MAX_PHASES+1][MAX_PHASES+1][2]);
    void computeOvertaking( const Entry& entA, const Entry& entB, const Entry& entC, const Entry& entD,
			    double y_aj[MAX_PHASES+1], Slice_Info ab_info[], const PrintHelper * ) const;

    ostream& printSlice( ostream& output, const Entry& src, const Entry& dst, const Slice_Info phase_info[] ) const;
	
    ostream& printStart( ostream& output,
			 const Entry& entA, const Entry& entB, const Entry& entC, const Entry& entD,
			 const Probability pr[] ) const;

protected:
    const Task * _client;
    const Entity* _server;
	
private:
    ijInfo ij_info;		/* Call information from client to server */
    Probability PrOtState[MAX_PHASES+1][MAX_PHASES+1][MAX_PHASES+1][2];
};

inline ostream& operator<<( ostream& output, Overtaking& self ) { return self.print( output ); }

#endif
