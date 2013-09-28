/* -*- c++ -*-
double  * $HeadURL$
 *
 * Generate MVA program for given layer.
 *
 * Copyright the Real-Time and Distributed Systems Group,
 * Department of Systems and Computer Engineering,
 * Carleton University, Ottawa, Ontario, Canada. K1S 5B6
 *
 * November, 1994
 *
 * $Id$
 *
 * ------------------------------------------------------------------------
 */

#include "dim.h"
#include <lqio/input.h>
#include "cltn.h"
#include "vector.h"

class Entity;
class Task;
class MVASubmodel;

ostream &printInterlock( ostream& output, const Entity& anEntity, const MVASubmodel& );


/* -------------------------------------------------------------------- */
/* Funky Formatting functions for inline with <<.			*/
/* -------------------------------------------------------------------- */

class Generate {
public:
    static void print( const MVASubmodel& );
    static void makefile( const unsigned );

private:
    Generate( const MVASubmodel& );
    ostream& print( ostream& ) const;
    ostream& printClientStation( ostream& output, const Task& aClient ) const;
    ostream& printServerStation( ostream& output, const Entity& aServer ) const;
    ostream& printInterlock( ostream& output, const Entity& aServer ) const;

public:
    static const char * file_name;

private:
    const MVASubmodel& mySubModel;
    const unsigned K;			/* Number of chains */
};
