/*
 * $HeadURL$
 *
 * Exported goodies.
 *
 * ------------------------------------------------------------------------
 * $Id: testmva.h 11969 2014-04-11 21:19:54Z greg $
 * ------------------------------------------------------------------------
 */

#if	!defined(TESTMVA_H)
#define	TESTMVA_H

#include <iostream>
#include <iomanip>
#include "vector.h"
#include "pop.h"

using namespace std;

class Server;
class Population;
class MVA;

#define	MAX_N1	9
#define	MAX_N2	2

typedef enum {EXACT_SOLVER, LINEARIZER_SOLVER, LINEARIZER2_SOLVER, BARD_SCHWEITZER_SOLVER} solverId;

#define	EXACT_SOLVER_BIT		0x1
#define	LINEARIZER_SOLVER_BIT		0x2
#define	LINEARIZER2_SOLVER_BIT		0x4
#define	BARD_SCHWEITZER_SOLVER_BIT	0x8

/* linearizer.c */

int main(int argc, char *argv[]);
void special_check( ostream&, const MVA&, const unsigned );
void test( PopVector&, Vector<Server *>&, VectorMath<double>&, VectorMath<unsigned>&, const unsigned );
bool check( const int, const MVA&, const unsigned );

#endif
