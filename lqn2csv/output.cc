/*  -*- c++ -*-
 * $Id: lqn2csv.cc 17557 2025-10-31 13:23:31Z greg $
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

#include "config.h"
#include <iomanip>
#include "output.h"
#include "lqn2csv.h"
#include "model.h"

/* Output an item, separated by commas. */

std::ostream&
Output::rows( std::ostream& output, const std::vector<std::vector<Model::Value>>& data ) 
{
    std::for_each( data.begin(), data.end(), [&]( const std::vector<Model::Value>& row ) {
	unsigned int column = 0;
	std::for_each( row.begin(), row.end(), [&]( const Model::Value& item ) {
	    if ( column > 0 ) {
		if ( width == 0 ) {
		    output << ",";
		} else {
		    output << " ";
		    output.width( width-1 );
		}
	    } else if ( width > 0 && item._type == Model::Value::Type::PATHNAME ) {
		output.width( path_width );
	    } 
	    output << item;
	    column += 1;
	} );
	output << std::endl;
    } );
    output << std::endl;
    return output;
}


std::ostream& 
Output::columns( std::ostream& output, const std::vector<std::vector<Model::Value>>& data, size_t columns )
{
    for ( size_t column = 0; column < columns; ++column ) {
	std::for_each( data.begin(), data.end(), [&]( const std::vector<Model::Value>& row ) {
	    if ( row.size() == 0 ) return;
	    if ( width == 0 ) {
		output << ",";
	    } else {
		output << " ";
		output.width( width - 1 );
	    }
	    output << row.at(column);
	} );
	output << std::endl;
    }
    output << std::endl;
    return output;
}


std::ostream& operator<<( std::ostream& output, const Model::Value& item )
{
    if ( item._type == Model::Value::Type::DOUBLE && !std::isnan( item._u._double ) ) {
	output << std::right << item._u._double;
    } else if ( (item._type == Model::Value::Type::STRING || item._type == Model::Value::Type::PATHNAME) && item._u._string != nullptr ) {
	std::string s;	// concatenate if necessary
	if ( width == 0 ) s = "\"";
	s += item._u._string;
	if ( width == 0 ) s += "\"";
	output << std::left << s;
    } else {
	output << std::right << "NULL";
    }
    return output;
}
