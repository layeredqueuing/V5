/*
 * $HeadURL$
 *
 * Exported goodies.
 *
 * ------------------------------------------------------------------------
 * $Id$
 * ------------------------------------------------------------------------
 */

#if	!defined(TESTOPEN_H)
#define	TESTOPEN_H

using namespace std;

#include <iostream>
#include <iomanip>
#include "vector.h"
#include "pop.h"

class Server;
class Population;
class Open;

/* TESTOPEN.c */

int main(int argc, char *argv[]);
void special_check( ostream&, const Open&, const unsigned );
void test( Vector<Server *>&, const unsigned );
bool check( ostream&, const Open&, const unsigned );
#endif
