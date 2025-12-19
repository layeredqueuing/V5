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
#include <cstring>
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
	size_t j = 0;
	std::for_each( data.begin(), data.end(), [&]( const std::vector<Model::Value>& row ) {
	    if ( row.size() == 0 ) return;
	    if ( j > 0 ) {
		if ( width == 0 ) {
		    output << ",";
		} else {
		    output << " ";
		    output.width( width - 1 );
		}
	    } else if ( width > 0 ) {
		output.width( width );
	    }
	    output << row.at(column);
	    j += 1;
	} );
	output << std::endl;
    }
    output << std::endl;
    return output;
}


std::ostream& operator<<( std::ostream& output, const Model::Value& item )
{
    switch ( item._type ) {
    case Model::Value::Type::DOUBLE:
	if ( !std::isnan( item._u._double ) ) {
	    output << std::right << item._u._double;
	} else {
	    output << std::right << "NULL";
	}
	break;
    case Model::Value::Type::STRING:
	if ( item._u._string == nullptr || strlen( item._u._string ) == 0 ) {
	    output << "";
	} else if ( width == 0 ) {	// quote CSV output
	    output << "\"" << item._u._string << "\""; 
	} else {
	    output << std::left << item._u._string;
	}
	break;
    case Model::Value::Type::PATHNAME:
	if ( item._u._pathname != nullptr ) {
	    output << std::left << *item._u._pathname;
	} else {
	    output << "";
	}
	break;
    }
    return output;
}
