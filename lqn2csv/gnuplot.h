/* -*- c++ -*-
 * Gnuplot code.
 *
 * Copyright the Real-Time and Distributed Systems Group,
 * Department of Systems and Computer Engineering,
 * Carleton University, Ottawa, Ontario, Canada. K1S 5B6
 *
 * October, 2021
 *
 * $Id: model.h 15037 2021-10-04 16:35:47Z greg $
 *
 * ------------------------------------------------------------------------
 */

#if !defined GNUPLOT_H
#define GNUPLOT_H
#include <ostream>
#include <map>
#include "model.h"

namespace Model {
    class GnuPlot {
    public:
	GnuPlot( std::ostream&, const std::string&, const std::vector<Model::Result::result_t>& );

	void preamble();
	void plot();
	bool splot_output() const { return _splot_x_index.first != 0; }
	const std::pair<size_t,double>& getSplotXIndex() const { return  _splot_x_index; }

    private:
	std::ostream& _output;
	const std::string _output_file_name;
	const std::vector<Model::Result::result_t>& _results;
	Model::Result::result_t _x1_axis;	/* None if no independent variables	*/
	Model::Result::result_t _x2_axis;
	std::map<const std::string,Model::Result::Type> _y1_axis;
	std::map<const std::string,Model::Result::Type> _y2_axis;
	size_t _x1_index;			/* Position in _results of x1 variable	*/
	size_t _x2_index;			/* Position in _results of x2 variable	*/
	std::map<std::string,size_t> _y1_index;
	std::map<std::string,size_t> _y2_index;
	std::pair<size_t,double> _splot_x_index;
    };
}
#endif
