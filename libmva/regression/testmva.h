/*
 * $HeadURL$
 *
 * Exported goodies.
 *
 * ------------------------------------------------------------------------
 * $Id: testmva.h 14307 2020-12-31 15:54:48Z greg $
 * ------------------------------------------------------------------------
 */

#if	!defined(TESTMVA_H)
#define	TESTMVA_H

#include <iostream>
#include <iomanip>
#include <mva/vector.h>
#include <mva/pop.h>
#include <mva/server.h>
#include <mva/mva.h>

using namespace std;

class Server;
class Population;
class MVA;

#define	MAX_N1	9
#define	MAX_N2	2

typedef enum {EXACT_SOLVER, LINEARIZER_SOLVER, LINEARIZER2_SOLVER, BARD_SCHWEITZER_SOLVER } solverId;

#define	EXACT_SOLVER_BIT		0x01
#define	LINEARIZER_SOLVER_BIT		0x02
#define	LINEARIZER2_SOLVER_BIT		0x04
#define	BARD_SCHWEITZER_SOLVER_BIT	0x08

/* linearizer.c */

int main(int argc, char *argv[]);
void special_check( ostream&, const MVA&, const unsigned );
void test( Population&, Vector<Server *>&, VectorMath<double>&, VectorMath<unsigned>&, const unsigned );
bool check( const int, const MVA&, const unsigned );

#endif
