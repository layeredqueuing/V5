/*  -*- c++ -*-
 * $HeadURL: http://rads-svn.sce.carleton.ca:8080/svn/lqn/trunk-V5/lqns/overtake.h $ -- Greg Franks
 *
 * $Id: overtake.h 14319 2021-01-02 04:11:00Z greg $
 */

#ifndef _OVERTAKE_H
#define _OVERTAKE_H

#include "dim.h"
#include <mva/vector.h>
#include <mva/prob.h>
#include "slice.h"

class Entry;
class Task;
class Entity;

class Overtaking
{
private:

    /*
     * Information needed for the markov phased server.
     */

    class ijInfo {
    public:
	ijInfo();

	void initialize( const Task * srcTask, const Entity * dstTask );
	
	double rendezvous() const { return _rendezvous.sum(); }
	double rendezvous( const unsigned p ) const { return _rendezvous[p]; }

    private:
	VectorMath<double> _rendezvous;
    };

public:
    class PrintHelper {
    public:
	PrintHelper( std::ostream& output ) : _output( output ) {}
	void operator()( const Entry& entA, const Entry& entB, const Entry& entC, const Entry& entD, const unsigned j, const Probability pr[] ) const;

    private:
	std::ostream& _output;
    };

public:
    Overtaking( const Task* client, const Entity* server );

    void compute( const PrintHelper * = 0 );
    std::ostream& print( std::ostream& );
	
private:
    void clearOvertakingStates(const unsigned max_phases, Probability PrOTState[MAX_PHASES+1][MAX_PHASES+1][2]);
    void computeOvertaking( const Entry& entA, const Entry& entB, const Entry& entC, const Entry& entD,
			    double y_aj[MAX_PHASES+1], Slice_Info ab_info[], const PrintHelper * ) const;

    std::ostream& printSlice( std::ostream& output, const Entry& src, const Entry& dst, const Slice_Info phase_info[] ) const;
	
    std::ostream& printStart( std::ostream& output,
			 const Entry& entA, const Entry& entB, const Entry& entC, const Entry& entD,
			 const Probability pr[] ) const;

protected:
    const Task * _client;
    const Entity* _server;
	
private:
    ijInfo ij_info;		/* Call information from client to server */
    Probability PrOtState[MAX_PHASES+1][MAX_PHASES+1][MAX_PHASES+1][2];
};

inline std::ostream& operator<<( std::ostream& output, Overtaking& self ) { return self.print( output ); }

#endif
