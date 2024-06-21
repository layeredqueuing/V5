/* -*- c++ -*- */
/************************************************************************/
/* Copyright the Real-Time and Distributed Systems Group,		*/
/* Department of Systems and Computer Engineering,			*/
/* Carleton University, Ottawa, Ontario, Canada. K1S 5B6		*/
/* 									*/
/* 2012.								*/
/************************************************************************/

/*
 * $Id: srvn_spex.h 15960 2022-10-11 16:12:41Z greg $
 */

#ifndef __LQIO_GNUPLOT_H__
#define __LQIO_GNUPLOT_H__

#include <vector>
#include <set>

namespace LQX {
    class SyntaxTreeNode;
}

namespace LQIO {
    namespace GnuPlot {
	enum class Format { NONE, TERMINAL, EMF, EPS, FIG, GIF, LATEX, PDF, PNG, SVG };
	    
	void insert_header( std::vector<LQX::SyntaxTreeNode *>* program, const std::string& comment, const std::vector<std::pair<const std::string,LQX::SyntaxTreeNode *>>& variables );
	LQX::SyntaxTreeNode * print_node( const std::string& );
	LQX::SyntaxTreeNode * print_node( LQX::SyntaxTreeNode *, ... );

	extern const std::set<std::string> colour_names;
	extern const std::map<const LQIO::GnuPlot::Format,const std::string> output_suffix;
    };
}
#endif /* __LQIO_GNUPLOT_H__ */
