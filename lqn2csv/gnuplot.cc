/*  -*- c++ -*-
 * $Id: model.cc 15037 2021-10-04 16:35:47Z greg $
 *
 * Command line processing.
 *
 * Copyright the Real-Time and Distributed Systems Group,
 * Department of Systems and Computer Engineering,
 * Carleton University, Ottawa, Ontario, Canada. K1S 5B6
 *
 * October, 2021
 *
 * ------------------------------------------------------------------------
 */

#include <vector>
#include <iostream>
#include "gnuplot.h"
#include "lqn2csv.h"

Model::GnuPlot::GnuPlot( std::ostream& output, const std::string& output_file_name, const std::vector<Model::Result::result_t>& results ) :
    _output(output),
    _output_file_name(output_file_name),
    _results(results),
    _x1_axis("",Result::Type::NONE),	   /* None if no independent variables     */
    _x2_axis("",Result::Type::NONE),
    _y1_axis(),
    _y2_axis(),
    _x1_index(1),			   /* Position in _results of x1 variable  */
    _x2_index(2),			   /* Position in _results of x2 variable  */
    _splot_x_index(0,0.)
{
}


void
Model::GnuPlot::preamble()
{
    std::string title;

    _output << "#!/opt/local/bin/gnuplot" << std::endl;
    _output << "set datafile separator \",\"" << std::endl;		/* Use CSV. */
    if ( !_output_file_name.empty() ) {
#if defined(__WINNT__)
	const size_t i = _output_file_name.find_last_of( "\\" );
#else
	const size_t i = _output_file_name.find_last_of( "/" );
#endif
	if ( i != std::string::npos ) {
	    title = _output_file_name.substr( i + 1 );
	} else {
	    title = _output_file_name;
	}
	const size_t j = title.find_last_of( "." );
	if ( j != std::string::npos ) {
	    title.erase( j );
	}
	_output << "#set output \"" <<  title << ".svg" << std::endl;
	_output << "#set terminal svg" << std::endl;
    }

    /* Go through all results and see if we have no more than two matching types. */
    for ( std::vector<Model::Result::result_t>::const_iterator result = _results.begin(); result != _results.end(); ++result ) {
	/* Handle any dependent variables */
	if ( Result::isIndependentVariable( result->second ) ) {
	    if ( _x1_axis.second == Result::Type::NONE ) {
		_x1_axis = *result;
		_x1_index = result - _results.begin() + 2;
	    } else if ( _x2_axis.second == Result::Type::NONE ) {
		_x2_axis = *result;
		_x2_index = result - _results.begin() + 2;
		_output << "set x2label \"" << Model::Result::__results.at(result->second).name << "\"" << std::endl;
		_output << "set x2tics" << std::endl;
	    } else {
		std::cerr << toolname << ": Too many independent variables to plot starting with " << result->first << std::endl;
		exit( 1 );
	    }
	} else if ( _y1_axis.empty() ) {
	    _y1_axis.emplace( *result );
	    if ( !title.empty() ) title.push_back( ' ' );
	    title += Model::Result::__results.at(result->second).name;
	} else if ( !Model::Result::equal( _y1_axis.begin()->second, result->second ) && _y2_axis.empty() ) {
	    _y2_axis.emplace( *result );
	    // set y2label...
	    if ( !title.empty() ) title.push_back( ' ' );
	    title += Model::Result::__results.at(result->second).name;
	} else if ( !Model::Result::equal( _y1_axis.begin()->second, result->second ) && !Model::Result::equal( _y2_axis.begin()->second, result->second ) ) {
	    std::cerr << toolname << ": Too many dependent variables to plot starting with " << result->first << std::endl;
	    exit( 1 );
	}
    }
    if ( _x2_axis.second != Result::Type::NONE ) {
	_splot_x_index.first = _x1_index;
    }

    _output << "set title \"" + title + "\"" << std::endl;
    _output << "$DATA << EOF" << std::endl;
}



void
Model::GnuPlot::plot()
{
    _output << "EOF" << std::endl;
    if ( _y1_axis.empty() ) return;		/* Nothing to plot */

    _output << "set xlabel \"" << Model::Result::__results.at(_x1_axis.second).name << "\"" << std::endl;
    if ( splot_output() ) {
	_output << "set ylabel \"" << Model::Result::__results.at(_x2_axis.second).name << "\"" << std::endl
		<< "set zlabel \"" << Model::Result::__results.at(_y1_axis.begin()->second).name << "\"" << std::endl;
    } else {
	_output << "set ylabel \"" << Model::Result::__results.at(_y1_axis.begin()->second).name << "\"" << std::endl;
    }
    if ( !_y2_axis.empty() ) {
	_output << "set ylabel \"" << Model::Result::__results.at(_y2_axis.begin()->second).name << "\"" << std::endl;
	_output << "set y2tics" << std::endl;
    }

    bool first_line = true;
    for ( std::vector<Model::Result::result_t>::const_iterator result = _results.begin(); result != _results.end(); ++result ) {
	if ( *result == _x1_axis || *result == _x2_axis ) continue;
	if ( first_line ) {
	    if ( splot_output() ) {
		_output << "splot ";
	    } else {
		_output << "plot ";
	    }
	    first_line = false;
	} else {
	    _output << ",\\" << std::endl << "     ";
	}
	_output << "$DATA using " << _x1_index << ":";
	if ( splot_output() ) {
	    _output << _x2_index << ":";
	}
	_output << (result - _results.begin()) + 2 << " with linespoints";

	/* Search y1 and y2 maps */
	if ( _y2_axis.find( result->first ) != _y2_axis.end() ) {
	    _output << " axis x1y2";
	}
	_output << " title \"" << Model::Object::__object_type.at(Model::Result::__results.at(result->second).type)
	       << " " << result->first << " " << Model::Result::__results.at(result->second).name << "\"";
    }
    _output << std::endl;
}
