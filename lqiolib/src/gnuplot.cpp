/*
 *  $Id: srvn_spex.cpp 15965 2022-10-12 20:56:50Z greg $
 *
 *  Created by Greg Franks on 2012/05/03.
 *  Copyright 2012 __MyCompanyName__. All rights reserved.
 *
 * Note: Be careful with static casts because of lists and nodes.
 */

#if HAVE_CONFIG_H
#include <config.h>
#endif
#include <cstdarg>
#include <vector>
#include <string>
#include <lqx/SyntaxTree.h>
#include "gnuplot.h"

namespace LQIO {

    void GnuPlot::insert_header( std::vector<LQX::SyntaxTreeNode *>* program, const std::string& comment, const std::vector<std::pair<const std::string,LQX::SyntaxTreeNode *>>& variables )
    {
	program->push_back( print_node( "#!/opt/local/bin/gnuplot" ) );
	if ( !comment.empty() ) {
	    program->push_back( print_node( std::string( "#" ) + comment ) );
	}
	std::vector<LQX::SyntaxTreeNode *>* arguments = new std::vector<LQX::SyntaxTreeNode *>();
	arguments->push_back( new LQX::ConstantValueExpression( " " ) );
	arguments->push_back( new LQX::ConstantValueExpression( "# " ) );
	
	for ( std::vector<std::pair<const std::string,LQX::SyntaxTreeNode *>>::const_iterator var = variables.begin(); var != variables.end(); ++var ) {
	    arguments->push_back( new LQX::ConstantValueExpression( var->first ) );	/* Variable name */
	}
	program->push_back( new LQX::FilePrintStatementNode( arguments, true, true ) );	/* Print out a comment with the values that will follow */
	program->push_back( print_node( "$DATA << EOF" ) );				/* Append newline.  Don't space */
    }
    
    LQX::SyntaxTreeNode * GnuPlot::print_node( const std::string& s )
    {
	std::vector<LQX::SyntaxTreeNode *>* parameters = new std::vector<LQX::SyntaxTreeNode *>();
	parameters->push_back( new LQX::ConstantValueExpression( s ) );
	return new LQX::FilePrintStatementNode( parameters, true, false );
    }

    LQX::SyntaxTreeNode * GnuPlot::print_node( LQX::SyntaxTreeNode * arg1, ... )
    {
	std::vector<LQX::SyntaxTreeNode *>* parameters = new std::vector<LQX::SyntaxTreeNode *>();
	va_list arguments;		       // A place to store the list of arguments

	va_start ( arguments, arg1 );		// Initializing arguments to store all values after num
	for ( LQX::SyntaxTreeNode* arg = arg1; arg != nullptr; arg = va_arg( arguments, LQX::SyntaxTreeNode* ) ) {
	    parameters->push_back( arg );
	}
	va_end( arguments );
	return new LQX::FilePrintStatementNode( parameters, true, false );
    }
}
