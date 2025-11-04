/* -*- c++ -*-
 * $HeadURL: http://rads-svn.sce.carleton.ca:8080/svn/lqn/trunk/lqn2csv/lqn2csv.h $
 *
 * Dimensions common to everything, plus some funky inline functions.
 *
 * Copyright the Real-Time and Distributed Systems Group,
 * Department of Systems and Computer Engineering,
 * Carleton University, Ottawa, Ontario, Canada. K1S 5B6
 *
 * October, 2021
 *
 * $Id: lqn2csv.h 17560 2025-11-03 22:45:15Z greg $
 *
 * ------------------------------------------------------------------------
 */

#if !defined(LQN2CSV_H)
#define LQN2CSV_H

#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <string>

extern std::string toolname;
extern int precision;
extern size_t width;
extern size_t path_width;
#endif
