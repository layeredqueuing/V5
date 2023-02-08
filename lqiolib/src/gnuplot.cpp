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
#include <sstream>
#include <string>
#include <lqx/SyntaxTree.h>
#include "gnuplot.h"

using namespace LQIO;

void GnuPlot::insert_header( std::vector<LQX::SyntaxTreeNode *>* program, const std::string& comment, const std::vector<std::pair<const std::string,LQX::SyntaxTreeNode *>>& variables )
{
    program->push_back( print_node( "#!/opt/local/bin/gnuplot" ) );
    program->push_back( print_node( std::string( "# $" ) + "Id" + "$" ) );	// Fake out SVN.
    if ( !comment.empty() ) {
	program->push_back( print_node( std::string( "# " ) + comment ) );
    }
    std::vector<LQX::SyntaxTreeNode *>* arguments = new std::vector<LQX::SyntaxTreeNode *>();
    arguments->push_back( new LQX::ConstantValueExpression( " " ) );
    arguments->push_back( new LQX::ConstantValueExpression( "# " ) );
	
    for ( std::vector<std::pair<const std::string,LQX::SyntaxTreeNode *>>::const_iterator var = variables.begin(); var != variables.end(); ++var ) {
	if ( !var->first.empty() )  {
	    arguments->push_back( new LQX::ConstantValueExpression( var->first ) );	/* Variable name */
	} else if ( var->second != nullptr ) {
	    std::ostringstream ss;
	    ss << "\"" << *var->second << "\"";
	    arguments->push_back( new LQX::ConstantValueExpression( ss.str() ) );	/* expression */
	} else {
	    arguments->push_back( new LQX::ConstantValueExpression( std::string( "\"\"" ) ) );
	}
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


/*
 * Gnuplot palette
 */

const std::set<std::string> GnuPlot::colour_names = {
    "white",
    "black",
    "dark-grey",
    "red",
    "web-green",
    "web-blue",
    "dark-magenta",
    "dark-cyan",
    "dark-orange",
    "dark-yellow",
    "royalblue",
    "goldenrod",
    "dark-spring-green",
    "purple",
    "steelblue",
    "dark-red",
    "dark-chartreuse",
    "orchid",
    "aquamarine",
    "brown",
    "yellow",
    "turquoise",
    "grey0",
    "grey10",
    "grey20",
    "grey30",
    "grey40",
    "grey50",
    "grey60",
    "grey70",
    "grey",
    "grey80",
    "grey90",
    "grey100",
    "light-red",
    "light-green",
    "light-blue",
    "light-magenta",
    "light-cyan",
    "light-goldenrod",
    "light-pink",
    "light-turquoise",
    "gold",
    "green",
    "dark-green",
    "spring-green",
    "forest-green",
    "sea-green",
    "blue",
    "dark-blue",
    "midnight-blue",
    "navy",
    "medium-blue",
    "skyblue",
    "cyan",
    "magenta",
    "dark-turquoise",
    "dark-pink",
    "coral",
    "light-coral",
    "orange-red",
    "salmon",
    "dark-salmon",
    "khaki",
    "dark-khaki",
    "dark-goldenrod",
    "beige",
    "olive",
    "orange",
    "violet",
    "dark-violet",
    "plum",
    "dark-plum",
    "dark-olivegreen",
    "orangered4",
    "brown4",
    "sienna4",
    "orchid4",
    "mediumpurple3",
    "slateblue1",
    "yellow4",
    "sienna1",
    "tan1",
    "sandybrown",
    "light-salmon",
    "pink",
    "khaki1",
    "lemonchiffon",
    "bisque",
    "honeydew",
    "slategrey",
    "seagreen",
    "antiquewhite",
    "chartreuse",
    "greenyellow",
    "gray",
    "light-gray",
    "light-grey",
    "dark-gray",
    "slategray",
    "gray0",
    "gray10",
    "gray20",
    "gray30",
    "gray40",
    "gray50",
    "gray60",
    "gray70",
    "gray80",
    "gray90",
    "gray100"
};    
