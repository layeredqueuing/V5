/************************************************************************/
/* Copyright the Real-Time and Distributed Systems Group,		*/
/* Department of Systems and Computer Engineering,			*/
/* Carleton University, Ottawa, Ontario, Canada. K1S 5B6		*/
/* 									*/
/* 2010.								*/
/************************************************************************/

/*
 * $Id$
 *
 * This class is used to hide the methods used to output to the Xerces DOM.
 */

#ifndef __CONFIDENCE_INTERVALS_H
#define __CONFIDENCE_INTERVALS_H

#include <string>
namespace LQIO {

    class ConfidenceIntervals {
    public:
	typedef enum { CONF_95, CONF_99 } confidence_level_t;

	static double invert( const double value, const unsigned blocks, const confidence_level_t level );
	static double get_t_value( const unsigned blocks, const confidence_level_t level );

	ConfidenceIntervals( const confidence_level_t level = CONF_95, const unsigned blocks = 0 );

	ConfidenceIntervals& set_blocks( const unsigned blocks );
	ConfidenceIntervals& set_level( const confidence_level_t );
	double operator()( double value ) const;
	double invert( double value ) const;
	bool is_set() const { return _t_value != 0; }
	
    private:
	static double t_values[2][34];
	double _t_value;
	confidence_level_t _level;
	unsigned int _blocks;
	bool _is_set;
    };
}

#endif /* __CONFIDENCE_INTERVALS_H */
