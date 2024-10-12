/* -*- c++ -*-
 * $Id: generate.cc 17257 2024-07-11 00:21:27Z greg $
 *
 * Print out model information.  We can also print out the
 * submodels as C++ source.
 *
 * Copyright the Real-Time and Distributed Systems Group,
 * Department of Systems and Computer Engineering,
 * Carleton University, Ottawa, Ontario, Canada. K1S 5B6
 *
 * November, 1994
 *
 * ------------------------------------------------------------------------
 */


#include "lqns.h"
#include <algorithm>
#include <filesystem>
#include <functional>
#include <sstream>
#include <fstream>
#include <cstdlib>
#include <cmath>
#include <getopt.h>
#include <lqio/bcmp_document.h>
#if HAVE_EXPAT_H
#include <lqio/jmva_document.h>
#endif
#include <lqio/qnap2_document.h>
#include <lqio/error.h>
#include <lqio/glblerr.h>
#include <mva/mva.h>
#include <mva/server.h>
#include <mva/vector.h>
#include <mva/multserv.h>
#include "entity.h"
#include "entry.h"
#include "errmsg.h"
#include "generate.h"
#include "flags.h"
#include "model.h"
#include "pragma.h"
#include "submodel.h"
#include "task.h"

/*
 * The directory name where the generated code will go.  By default "debug".
 */

std::string Generate::__directory_name;
Generate::Output Generate::__mode = Generate::Output::NONE;

void
Generate::output( const Vector<Submodel *>& submodels )
{
    typedef void (*fptr)( const Submodel* );
    static std::map<Generate::Output,fptr> f = {
	{ Generate::Output::NONE,   nullptr },
	{ Generate::Output::LIBMVA, &Generate::LibMVA::program },
	{ Generate::Output::JMVA,   &Generate::BCMP_Model::serialize_JMVA },
	{ Generate::Output::QNAP,   &Generate::BCMP_Model::serialize_QNAP },
    };
    
    if ( __mode == Output::NONE ) return;

    try {
	std::filesystem::create_directory( __directory_name );
    }
    catch( const std::filesystem::filesystem_error& e ) {
	runtime_error( LQIO::ERR_CANT_OPEN_DIRECTORY, __directory_name.c_str(), e.what() );
	return;
    }

    if ( __mode == Output::LIBMVA ) {
	Generate::LibMVA::makefile( submodels.size() );	/* We are dumping C source -- make a makefile. */
    }
    std::for_each( submodels.begin(), submodels.end(), f.at(__mode) );
}

/* -------------------------------------------------------------------- */
/*                    Output LibMVA source code.                        */
/* -------------------------------------------------------------------- */

/*
 * Command line options which are output into the generated code.
 */

const std::vector<struct option> Generate::LibMVA::__longopts = {
    { "fraction",	no_argument, nullptr, 'd' },
    { "queue-length",	no_argument, nullptr, 'l' },
    { "customers",	no_argument, nullptr, 'n' },
    { "marginals",	no_argument, nullptr, 'p' },
    { "utilization",	no_argument, nullptr, 'u' },
    { "waiting",	no_argument, nullptr, 'w' },
    { "throughput",	no_argument, nullptr, 'x' },
    { "multiserver",	required_argument, nullptr, 'M' },
    { "exact-mva",	no_argument, nullptr, 'E' },
    { "schweitzer",	no_argument, nullptr, 'S' },
    { "linearizer",	no_argument, nullptr, 'L' },
    { "verbose",	no_argument, nullptr, 'v' },
    { "help",		no_argument, nullptr, 'h' }
};

const std::map<const char, const std::string> Generate::LibMVA::__help = {
    { 'd', "Print the linearizer variable D." },
    { 'l', "Print the the queue length at all the stations." },
    { 'n', "Print the number of customers for all the classes." },
    { 'p', "Print the marginal probabilities." },
    { 'u', "Print the utiliation for all the stations." },
    { 'w', "Print the waiting time for all the stations." },
    { 'x', "Print the throughput for all the classes." },
    { 'M', "Use <arg> for multiservers." },
    { 'E', "Use Exact MVA." },
    { 'S', "Use Schweitzer approximate MVA." },
    { 'L', "Use Linearizer approximage MVA." },
    { 'v', "Verbose - print the input." },
    { 'h', "Print this help." }
};

const std::map<const int,const std::string> Generate::LibMVA::__argument_type = {
    { no_argument,       "no_argument"       },
    { required_argument, "required_argument" },
    { optional_argument, "optional_argument" }
};

/*
 * All the includes needed by the generated code.
 */

const std::vector<std::string> Generate::LibMVA::__includes = {
    "<iostream>",
    "<iomanip>",
    "<map>",
    "<unistd.h>",
    "<getopt.h>",
    "\"mva.h\"",
    "\"open.h\"",
    "\"server.h\"",
    "\"ph2serv.h\"",
    "\"multserv.h\"",
    "\"pop.h\"",
    "\"prob.h\"",
    "\"vector.h\"",
    "\"fpgoop.h\""
};

/*
 * Map the pragma to the class name of the solver.
 */

const std::map<const Pragma::MVA,const std::string> Generate::LibMVA::__solvers = {
    { Pragma::MVA::EXACT, 		"ExactMVA" },
    { Pragma::MVA::FAST, 		"Linearizer2" },
    { Pragma::MVA::LINEARIZER,		"Linearizer" },
    { Pragma::MVA::ONESTEP, 		"OneStepMVA" },
    { Pragma::MVA::ONESTEP_LINEARIZER, 	"OneStepLinearizer" },
    { Pragma::MVA::SCHWEITZER, 		"Schweitzer" }
};

/*
 * Map the pragma enum to our local (which is output into
 * submodel-?.cc) by Generate::LibMVA::print so that we don't have to drag
 * they lqns header files into the generated code.
 */

std::map<const Pragma::Multiserver,std::string> Generate::LibMVA::__multiservers =
{
    { Pragma::Multiserver::DEFAULT,   "" },
    { Pragma::Multiserver::CONWAY,    "" },
    { Pragma::Multiserver::REISER,    "" },
    { Pragma::Multiserver::REISER_PS, "" },
    { Pragma::Multiserver::ROLIA,     "" },
    { Pragma::Multiserver::ROLIA_PS,  "" },
    { Pragma::Multiserver::ZHOU,      "" }
};


/*
 * Map the pragma enum in lqns/pragma.h to the class name of the
 * multiserver (the __type_str is the class name, defined for each
 * server subclass).  The multi-phase version is used if there is more
 * than one phase for an entry at the station.
 */

const std::map<const Pragma::Multiserver,const std::pair<const std::string&,const std::string&> > Generate::LibMVA::__stations =
{
    { Pragma::Multiserver::CONWAY,    { Conway_Multi_Server::__type_str,    Markov_Phased_Conway_Multi_Server::__type_str    } },
    { Pragma::Multiserver::REISER,    { Reiser_Multi_Server::__type_str,    Markov_Phased_Reiser_Multi_Server::__type_str    } },
    { Pragma::Multiserver::REISER_PS, { Reiser_PS_Multi_Server::__type_str, Markov_Phased_Reiser_Multi_Server::__type_str    } },	/* !! */
    { Pragma::Multiserver::ROLIA,     { Rolia_Multi_Server::__type_str,     Markov_Phased_Rolia_Multi_Server::__type_str     } },
    { Pragma::Multiserver::ROLIA_PS,  { Rolia_PS_Multi_Server::__type_str,  Markov_Phased_Rolia_PS_Multi_Server::__type_str  } },
    { Pragma::Multiserver::ZHOU,      { Zhou_Multi_Server::__type_str,      Markov_Phased_Zhou_Multi_Server::__type_str      } }
};

/* ------------------------------------------------------------------------ */

/*
 * Constructor
 */

Generate::LibMVA::LibMVA( const MVASubmodel& submodel )
    : _submodel( submodel ),  K(submodel.nChains())
{
}


/*
 * Make a test program based on the current layer.  Note that solver may overwrite the
 * same file over and over again.  Use -zstep to extract an intermediate model.
 */

/* static */ void
Generate::LibMVA::program( const Submodel * submodel ) 
{
    if ( dynamic_cast<const MVASubmodel *>(submodel) == nullptr ) return;

    std::ostringstream fileName;
    fileName << __directory_name << "/submodel-" << submodel->number() << ".cc";

    std::ofstream output;
    output.open( fileName.str(), std::ios::out );

    if ( !output ) {
	std::cerr << LQIO::io_vars.lq_toolname << ": Cannot open output file " << fileName.str() << " - " << strerror( errno ) << std::endl;
    } else {
	Generate::LibMVA program( *dynamic_cast<const MVASubmodel *>(submodel) );
	program.print( output );
    }

    output.close();
}


/*
 * Make a makefile for the test program(s).
 */

/* static */ void
Generate::LibMVA::makefile( const unsigned nSubmodels )
{
    const std::string makefileName = __directory_name + "/Makefile";
	
    if ( std::filesystem::exists( makefileName ) ) {
	std::cerr << LQIO::io_vars.lq_toolname << ": " << makefileName << " exists.  It was not generated." << std::endl;
	return;
    }
    
    std::ofstream output;
    output.open( makefileName, std::ios::out );

    if ( !output ) {
	std::cerr << LQIO::io_vars.lq_toolname << ": Cannot open output file " << makefileName << " - " << strerror( errno ) << std::endl;
	return;
    }

    const std::string defines = "-g -std=c++17 -Wall -DDEBUG_MVA=1 -DHAVE_CONFIG_H=1 -I$(LQNDIR)/libmva -I$(LQNDIR)/libmva/src/headers/mva";
     
    /* try to find libmva in the directory path */
    std::string path = ".";
    if ( !find_libmva( path ) ) {
	path = "$(HOME)/usr/src";
    }
    output << "LQNDIR=" << path << std::endl
	   << "CXX = g++" << std::endl
	   << "CPPFLAGS = " << defines << std::endl
	   << "OBJS = fpgoop.o multserv.o  mva.o  open.o  ph2serv.o  pop.o  prob.o  server.o" << std::endl
	   << "SRCS = fpgoop.cc multserv.cc mva.cc open.cc ph2serv.cc pop.cc prob.cc server.cc" << std::endl;

    output << std::endl
	   << "all:\t";
    for ( unsigned i = 1; i <= nSubmodels; ++i ) {
	output << "submodel-" << i << " ";
    }
    output << std::endl;

    for ( unsigned i = 1; i <= nSubmodels; ++i ) {
	std::ostringstream fileName;
	fileName << "submodel-" << i;
	output << std::endl
	       << fileName.str() << ":\t$(SRCS) $(OBJS) " << fileName.str() << ".o" << std::endl
	       << "\t$(CXX) $(CXXFLAGS) -I. -o " << fileName.str() << " $(OBJS) " << fileName.str() << ".o -lm" << std::endl;
    }

    output << std::endl
	   << "clean:" << std::endl
	   << "\trm -f $(OBJS) *~";
    for ( unsigned i = 1; i <= nSubmodels; ++i ) {
	std::ostringstream fileName;
	fileName << "submodel-" << i;
	output << " " << fileName.str() << " " << fileName.str() << ".o";
    }
    output << std::endl;

    output << std::endl
	   << "really-clean:\tclean" << std::endl
	   << "\tfor i in $(SRCS) $(INCS); do \\" << std::endl
	   << "\t  if test -L $$i; then \\" << std::endl
	   << "\t    rm $$i; \\" << std::endl
	   << "\t  fi \\" << std::endl
	   << "\tdone" << std::endl;

    output << std::endl
	   << "$(SRCS):" << std::endl
	   << "\t@if test ! -f $@; then ln -s $(LQNDIR)/libmva/src/$@ .; fi" << std::endl;
}

/* ------------------------------------------------------------------------ */

/*
 * Make a test program based on the current layer.  
 */

std::ostream& 
Generate::LibMVA::print( std::ostream& output ) const
{
    /* Output the includes needed */
    
    for ( const auto& s : __includes ) {
	output << "#include " << s << std::endl;
    }
	
    /*
     * Output the options used by the testprogram submodel-<n>.cc
     */

    std::string opts;
    output << std::endl << "const std::vector<struct option> longopts = {" << std::endl;
    for ( std::vector<struct option>::const_iterator o = __longopts.begin(); o != __longopts.end(); ++o  ) {
	opts.append( 1, static_cast<char>(o->val) );
	if ( o->has_arg == required_argument ) opts.append( ":" );
	output << "    { \"" << o->name << "\", " 
	       << __argument_type.at(o->has_arg)
	       << ", nullptr, "
	       << "'" << static_cast<char>(o->val) << "' }";
	if ( std::next( o ) != __longopts.end() ) output << ",";
	output << std::endl;
    }
    output << "};" << std::endl;

    output << std::endl << "const std::map<const char, const std::string> help = {" << std::endl;
    for ( std::map<const char, const std::string>::const_iterator h = __help.begin(); h != __help.end(); ++h ) {
	output << "    { '" << h->first << "', \"" << h->second << "\" }";
	if ( std::next( h ) != __help.end() ) output << ",";
	output << std::endl;
    }
    output << "};" << std::endl;
    
    /* Output the enum of multiservers supported */
       
    output << std::endl << "enum class Multiserver {DEFAULT";
    for ( std::map<const std::string,const Pragma::Multiserver>::const_iterator m = Pragma::__multiserver_pragma.begin(); m != Pragma::__multiserver_pragma.end(); ++m ) {
	std::map<const Pragma::Multiserver,std::string>::iterator s = Generate::LibMVA::__multiservers.find(m->second);
	if ( s == Generate::LibMVA::__multiservers.end() ) continue;
	const std::string& src = m->first;
	std::string& dst = s->second;
	dst.clear();
	std::transform( src.begin(), src.end(), back_inserter(s->second), remap );		/* Map to enum */
	output << ",";
	output << s->second;
    }
    output << "};" << std::endl;
    
    /* Output the map from the pragma name to the enum */
    
    output << std::endl << "const std::map<const std::string,const Multiserver> multiservers = {";
    bool first = true;
    for ( std::map<const std::string,const Pragma::Multiserver>::const_iterator m = Pragma::__multiserver_pragma.begin(); m != Pragma::__multiserver_pragma.end(); ++m ) {
	if ( __multiservers.find(m->second) == __multiservers.end() ) continue;
	if ( !first ) output << ",";
	first = false;
	output << std::endl << "    { \"" << m->first << "\", Multiserver::" << __multiservers.at(m->second) << " }";
    }
    output << std::endl << "};" << std::endl;

    /* The main line. */
    
    output << std::endl << "int main ( int argc, char* argv[] )" << std::endl << "{" << std::endl
	   << "    extern char *optarg;" << std::endl
	   << "    Multiserver multiserver = Multiserver::DEFAULT;" << std::endl
	   << "    MVA::MOL_multiserver_underrelaxation = 0.5;	/* For MOL Multiservers */" << std::endl
	   << "    bool print_input = false;  /* Output input parameters if true */" << std::endl;

    if ( _submodel.nOpenStns() ) {
	output << "    const unsigned n_open_stations\t= " << _submodel.nOpenStns() << ";" << std::endl;
	output << "    Vector<Server *> open_stations(n_open_stations);" << std::endl;
    }
    if ( _submodel.nClosedStns() ) {
	output << "    const unsigned n_stations\t= " << _submodel.nClosedStns() << ";" << std::endl;

	output << "    const unsigned n_chains\t= " << K << ";" << std::endl;
	output << "    Vector<Server *> stations( n_stations);" << std::endl;
	output << "    Population customers( n_chains );" << std::endl;
	output << "    VectorMath<double> think_times( n_chains );" << std::endl;
	output << "    VectorMath<unsigned> priorities( n_chains );" << std::endl;
	if ( _submodel._overlapFactor ) {
	    output << "    Probability *** prOt;\t\t//Overtaking Probabilities" << std::endl << std::endl;
	}
	output << "    MVA::new_solver solver = " << __solvers.at(Pragma::mva()) << "::create;" << std::endl;
    }
    output << std::endl;
	
    /* Argument handling */
    
    output << "    for ( ;; ) {" << std::endl;
    output << "        const int c = getopt_long( argc, argv, \"" << opts << "\", longopts.data(), nullptr );" << std::endl;
    output << "        if ( c == -1 ) break;" << std::endl;
    output << "        switch ( c ) {" << std::endl;
    output << "        case 'd': MVA::debug_D = true; break;" << std::endl;
    output << "        case 'l': MVA::debug_L = true; break;" << std::endl;
    output << "        case 'n': MVA::debug_N = true; break;" << std::endl;
    output << "        case 'p': MVA::debug_P = true; break;" << std::endl;
    output << "        case 'u': MVA::debug_U = true; break;" << std::endl;
    output << "        case 'w': MVA::debug_W = true; break;" << std::endl;
    output << "        case 'x': MVA::debug_X = true; break;" << std::endl;
    output << "        case 'E': solver = ExactMVA::create; break;" << std::endl;
    output << "        case 'S': solver = Schweitzer::create; break;" << std::endl;
    output << "        case 'L': solver = Linearizer::create; break;" << std::endl;
    output << "        case 'v': print_input = true;" << std::endl;
    output << "        case 'M':" << std::endl;
    output << "            if ( optarg != nullptr ) {" << std::endl;
    output << "                const std::map<const std::string,const Multiserver>::const_iterator m = multiservers.find(optarg);" << std::endl;
    output << "                if ( m == multiservers.end() ) {" << std::endl;
    output << "                    std::cerr << \"Invalid argument \\\"\" << optarg << \"\\\" to --multiserver=<arg>\" << std::endl;" << std::endl;
    output << "                    exit( 1 );" << std::endl;
    output << "                }" << std::endl;
    output << "                multiserver = m->second;" << std::endl;
    output << "            }" << std::endl;
    output << "            break;" << std::endl;
    output << "        default:" << std::endl;
    output << "             std::cerr << \"Options:\" << std::endl;" << std::endl;
    output << "             for ( const auto& o : longopts ) {" << std::endl;
    output << "                 const std::string s = o.name + std::string(o.has_arg != no_argument ? \"=<arg>\" : \" \");" << std::endl;
    output << "                 std::cerr << \"-\" << static_cast<char>(o.val) << \", --\" << std::left << std::setw( 20 ) << s << help.at(o.val) << std::endl;" << std::endl;
    output << "             }" << std::endl;
    output << "             exit( 1 );" << std::endl;
    output << "         }" << std::endl;
    output << "    }" << std::endl;
    output << std::endl;

    /* Chains */

    if ( K > 0 ) {
	output << "    /* Chains */" << std::endl << std::endl;

	for ( unsigned i = 1; i <= K; ++i ) {
	    output << "    customers[" << i << "] = " << _submodel.customers(i) << ";" << std::endl;
	    output << "    think_times[" << i << "] = " << _submodel.thinkTime(i) << ";" << std::endl;
	    output << "    priorities[" << i << "]  = " << _submodel.priority(i)  << ";" << std::endl;
	}
	output << std::endl;
    }

    /* Clients */

    output << "    /* Clients */" << std::endl << std::endl;
    std::for_each( _submodel._clients.begin(), _submodel._clients.end(), [&](Task * client){ printClientStation( output, *client ); } );

    /* Servers */
	
    output << std::endl << "    /* Servers */" << std::endl << std::endl;
    std::for_each( _submodel._servers.begin(), _submodel._servers.end(), [&]( Entity * server){ printServerStation( output, *server ); } );

    /* Overlap factor */

    if ( _submodel._overlapFactor ) {
	output << "    /* Overlap Factor */" << std::endl << std::endl;
	output << "    VectorMath<double> * _overlapFactor = new VectorMath<double> [n_chains+1];" << std::endl;
	output << "    for ( unsigned i = 1; i <= n_chains; ++i ) {" << std::endl;
	output << "        _overlapFactor[i].grow( n_chains, 1.0 );" << std::endl;
	output << "    }" << std::endl;
	for ( unsigned i = 1; i <= K; ++i ) {
	    for ( unsigned j = 1; j <= K; ++j ) {
		if ( _submodel._overlapFactor[i][j] ) {
		    output << "    _overlapFactor[" << i << "][" << j << "] = " 
			   << _submodel._overlapFactor[i][j] << ";" << std::endl;
		}
	    }
	}
	output << std::endl;
    }

    /* Symbolic names for array references. */

    output << std::endl << "    /* Station names */" << std::endl << std::endl;
	
    unsigned closedStnNo = 0;
    unsigned openStnNo 	 = 0;

    for ( std::set<Task *>::const_iterator client = _submodel._clients.begin(); client != _submodel._clients.end(); ++client ) {
	++closedStnNo;
	output << "    stations[" << closedStnNo << "]\t= " << station_name( **client ) << ";"  << std::endl;
    }
    for ( std::set<Entity *>::const_iterator server = _submodel._servers.begin(); server != _submodel._servers.end(); ++server ) {
	if ( (*server)->isClosedModelServer() ) {
	    ++closedStnNo;
	    output << "    stations[" << closedStnNo << "]\t= " << station_name( **server ) << ";" << std::endl;
	}
	if ( (*server)->isOpenModelServer() ) {
	    ++openStnNo;
	    output << "    open_stations[" << openStnNo << "]\t= " << station_name( **server ) << ";" << std::endl;
	}
    }
    output << std::endl;

    /* Verbose */
    
    closedStnNo = 0;
    openStnNo   = 0;
    output << "    if ( print_input ) {" << std::endl << "        std::cout << \"Clients:\" << std::endl;" << std::endl;
    for ( std::set<Task *>::const_iterator client = _submodel._clients.begin(); client != _submodel._clients.end(); ++client ) {
	++closedStnNo;
	output << "        std::cout << \"" << closedStnNo << ": " << **client << "\" << std::endl << *" << station_name( **client ) << " << std::endl;" << std::endl;
    }
    output << "        std::cout << std::endl << \"Servers:\" << std::endl;" << std::endl;
    for ( std::set<Entity *>::const_iterator server = _submodel._servers.begin(); server != _submodel._servers.end(); ++server ) {
	output << "        std::cout << \"";
	if ( (*server)->isOpenModelServer() ) {
	    if ( (*server)->isClosedModelServer() ) {
		++closedStnNo;
		output << closedStnNo << ", ";
	    }
	    openStnNo   = 0;
	    output << openStnNo;
	} else {
	    ++closedStnNo;
	    output << closedStnNo;
	}
	output << ": " << **server << "\" << std::endl" << " << *" << station_name( **server ) << " << std::endl;" << std::endl;
    }
    output << "        std::cout << std::endl;" << std::endl << "    }" << std::endl;
    output << std::endl;

    /* Solver */
	
    output << std::endl << "    /* Solution */" << std::endl << std::endl;

    if ( openStnNo > 0 ) {
	output << "    Open open( n_open_stations, open_station );" << std::endl;
	output << "    open.solve();" << std::endl;
	output << "    std::cout << open << std::endl;" << std::endl;
    } 
    if ( closedStnNo > 0 ) {
	if ( MVA::__bounds_limit ) {
	    output << "    " << "MVA::__bounds_limit = " << MVA::__bounds_limit << ";" << std::endl;
	}
	output << "    MVA* model = (*solver)( stations, customers, think_times, priorities";
	if ( _submodel._overlapFactor ) {
	    output << ", _overlapFactor";
	} else {
	    output << ", nullptr";
	}
	output << " );" << std::endl;
	output << "    model->solve();" << std::endl;
	output << "    std::cout << *model << std::endl;" << std::endl;
	output << "    delete model;" << std::endl;

	if ( _submodel._overlapFactor ) {
	    output << std::endl
		   << "    delete [] _overlapFactor;" << std::endl;
	}
    }

    /* Epilogue */
	
    output << "    return 0;" << std::endl << "}" << std::endl << std::endl;
    return output;
}

	

/*
 * Print out station parameters.
 */

std::ostream &
Generate::LibMVA::printClientStation( std::ostream& output, const Task& client ) const
{
    const Server& station = *client.clientStation( _submodel.number() );
    const unsigned P = station.nPhases();
    const unsigned E = station.nEntries();
    const std::string& station_name = std::string( "t_" ) + client.name();

    output << "    /* " << client << " */" << std::endl;

    /* Create the station */
	
    output << "    Server * " << station_name << " = new " << station.typeStr() << '(' << station_args( E, K, P ) << ");" << std::endl;

    for ( unsigned int e = 1; e <= E; ++e ) {
	bool hasService = false;

	for ( unsigned k = 1; k <= K; ++k ) {
	    for ( unsigned p = 1; p <= P; ++p ) {
		if ( station.S( e, k, p ) > 0 && station.V( e, k, p ) > 0 ) {
		    hasService = true;
		    output << "    " << station_name
			   << "->setService(" << station_args( e, k, p ) << "," << station.S( e, k, p ) << ")"
			   << ".setVisits(" << station_args( e, k, p )   << "," << station.V( e, k, p ) << ");";
		}
	    }
	}
	if ( !hasService ) {
	    output << "    /* No service time for station " << station_name << " */" << std::endl;
	}
    }
    output << std::endl;

    return output;
}



/*
 * Print out station parameters.
 */

std::ostream &
Generate::LibMVA::printServerStation( std::ostream& output, const Entity& server ) const
{
    const Server& station = *server.serverStation();
    const unsigned P = station.nPhases();
    const unsigned E = station.nEntries();
    const std::string& station_name = std::string( server.isTask() ? "t_" : "p_" ) + server.name();
    
    output << "    /* " << server << " */" << std::endl;

    /* Create the station */
	
    output << "    Server * " << station_name;
    if ( std::isfinite( station.mu() ) && station.mu() > 1.0 ) {
	output << ";" << std::endl << "    switch ( multiserver ) {" << std::endl;
	for ( std::map<const Pragma::Multiserver,std::string>::const_iterator m = __multiservers.begin(); m != __multiservers.end(); ++m ) {
	    if ( m->first == Pragma::Multiserver::DEFAULT ) {
		output << "    default: " << station_name << " = new " << station.typeStr();
	    } else {
		output << "    case Multiserver::" << m->second << ": " << station_name << " = new ";
		if ( P == 1 ) output << __stations.at(m->first).first;	/* Single phase */
		else output << __stations.at(m->first).second;		/* Two phase */
	    }
	    output << '(' << server.copies() << ',' << station_args( E, K, P ) << "); break;" << std::endl;
	}
	output << "    }" << std::endl;
    } else {
	output << "= new " << station.typeStr() << '(' << station_args( E, K, P ) << ");" << std::endl;
    }

    for ( unsigned int e = 1; e <= E; ++e ) {
	bool hasService = false;
	
	for ( unsigned k = 0; k <= K; ++k ) {
	    for ( unsigned p = 1; p <= P; ++p ) {
		if ( station.S( e, k, p ) == 0. || station.V( e, k, p ) == 0. ) continue;
		hasService = true;
		output << "    " << station_name
		       << "->setService(" << station_args( e, k, p ) << "," << station.S( e, k, p ) << ")"
		       << ".setVisits(" << station_args( e, k, p )   << "," << station.V( e, k, p ) << ")";
		if ( station.hasVariance() ) {
		    output << ".setVariance(" << station_args( e, k, p ) << "," << station.getVariance( e, k, p ) << ")";
		}
		output << ";" << std::endl;
	    }
	    if ( hasService && station.hasVariance() ) {
		output << "    " << station_name << ".setVariance(" << station_args( e, k, 0 ) << "," << station.getVariance( e, k, 0 ) << ");";
	    }
	}
	if ( !hasService ) {
	    output << "    /* No service time for station " << station_name << " */" << std::endl;
	}
		
	/* Overtaking probabilities. */
		
	if ( server.markovOvertaking() ) {
	    Probability *** prOt = station.getPrOt( e );
	    output << "    prOt = " << station_name << "->getPrOt(" << e << ");" << std::endl;
			
	    for( unsigned int k = 1; k <= K; ++k ) {
		for ( unsigned p = 0; p <= Entry::max_phases; ++p ) {	// Total probability!
		    for ( unsigned q = 1; q <= P; ++q ) {
			if ( prOt[k][p][q] == 0.0 ) continue;
			output << "    prOt" << overtaking_args( k, p, q ) << " = " << prOt[k][p][q] << ";" << std::endl;
		    }
		}
	    }
	}
    }

    if ( Pragma::interlock() ) {
	printInterlock( output, server );
    }

    output << std::endl;

    return output;
}



/*
 * Print out interlocking info.
 */

std::ostream &
Generate::LibMVA::printInterlock( std::ostream& output, const Entity& server ) const
{
    for ( std::set<Task *>::const_iterator client = _submodel._clients.begin(); client != _submodel._clients.end(); ++client ) {
	if ( (*client)->throughput() == 0.0 ) continue;
		
	const Probability PrIL = server.prInterlock( **client );
	if ( PrIL == 0.0 ) continue;

	for ( unsigned i = 1; i <= (*client)->clientChains( _submodel.number() ).size(); ++i ) {
	    const unsigned k = (*client)->clientChains( _submodel.number() )[i];
	    if ( server.hasServerChain( k ) ) {
		for ( std::vector<Entry *>::const_iterator entry = server.entries().begin(); entry != server.entries().end(); ++entry ) {
		    output << "    " << station_name( server ) << "->setInterlock("
			   << (*entry)->index() << "," 
			   << k << ","
			   << PrIL << ");" << std::endl;
		}
	    }
	}
    }
    return output;
}

/* ---------------------------------------------------------------------- */

char
Generate::LibMVA::remap( char c )
{
    if ( c == '-' ) return '_';
    else return toupper( c );
}


/*
 * Look for libmva.  Don't bother with root.
 */

bool
Generate::LibMVA::find_libmva( std::string& pathname )
{
    try { 
	if ( std::filesystem::exists( pathname ) ) {	/* Directory exists? 	*/
	    std::string filename = pathname + "/libmva";
	    if ( std::filesystem::exists( filename ) ) return true;	/* libmva found		*/
	    pathname.insert( 0, "../" );
	    return !std::filesystem::equivalent( "/", pathname ) && find_libmva( pathname );	/* try parent 		*/
	}
    }
    catch ( const std::filesystem::filesystem_error& e ) {		/* Ignore 		*/
    }
    return false;							/* Not found		*/
}


/*
 * Manufacture a station name.
 */

/* static */ std::ostream&
Generate::LibMVA::print_station_name( std::ostream& output, const Entity& anEntity )
{
    if ( anEntity.isTask() ) {
	output << "t_";
    } else {
	output << "p_";
    }
    output << anEntity.name();
    return output;
}


/*
 * Manufacture an argument string.
 */

/* static */ std::ostream&
Generate::LibMVA::print_station_args( std::ostream& output, const unsigned e, const unsigned k, const unsigned p )
{
    output << e << "," << k << "," << p;
    return output; 
}

/*
 * Manufacture an array string.
 */

/* static */ std::ostream&
Generate::LibMVA::print_overtaking_args( std::ostream& output, const unsigned e, const unsigned k, const unsigned p )
{
    output << "[" << e << "][" << k << "][" << p << "]";
    return output; 
}

/* -------------------------------------------------------------------- */
/*                        Output BCMP Model                             */
/* -------------------------------------------------------------------- */

/*
 * Construct a BCMP model from the submodel.  This can be solved using
 * a regular MVA solver.
 */

void
Generate::BCMP_Model::serialize_JMVA( const Submodel * submodel )
{
    serialize( submodel, &Generate::JMVA_Document::serialize );
}

void
Generate::BCMP_Model::serialize_QNAP( const Submodel * submodel )
{
    serialize( submodel, &Generate::QNAP_Document::serialize );
}
	       

void
Generate::BCMP_Model::serialize( const Submodel * submodel, serialize_fptr f )
{
    std::map<serialize_fptr, const std::string> extension = {
	{ JMVA_Document::serialize, "jmva" },
	{ QNAP_Document::serialize, "qnap" },
    };
    
    if ( dynamic_cast<const MVASubmodel *>(submodel) == nullptr ) return;

    const std::filesystem::path source_file_name = LQIO::DOM::Document::__input_file_name;	/* We need a copy to preserve over output */
    std::ostringstream file_name;
    file_name << __directory_name << "/submodel-" << submodel->number() << "." << extension.at(f);
    
    std::ofstream output;
    output.open( file_name.str(), std::ios::out );

    if ( !output ) {
	std::cerr << LQIO::io_vars.lq_toolname << ": Cannot open output file " << file_name.str() << " - " << strerror( errno ) << std::endl;
    } else {
	Generate::BCMP_Model bcmp( *dynamic_cast<const MVASubmodel *>(submodel) );
	std::ostringstream comment;
	comment << source_file_name << ", submodel " << submodel->number();
	bcmp.setComment( comment.str() );
	(*f)( output, source_file_name.string(), bcmp._model );
    }

    output.close();
}


void
Generate::JMVA_Document::serialize( std::ostream& output, const std::string& filename, const BCMP::Model& model )
{
#if HAVE_EXPAT_H
    QNIO::JMVA_Document jmva( model );
    jmva.print( output );
#endif
}


void
Generate::QNAP_Document::serialize( std::ostream& output, const std::string& filename, const BCMP::Model& model )
{
    QNIO::QNAP2_Document qnap( model );
    qnap.exportModel( output );
}

/* -------------------------------------------------------------------- */

/*
 * Constructor.  Add the reference station here.  Populate it in contstruct().
 */

Generate::BCMP_Model::BCMP_Model( const MVASubmodel& submodel )
    : _submodel( submodel ), K(submodel.nChains()), _model(), 
      _reference( _model.insertStation( "Reference", BCMP::Model::Station::Type::DELAY, SCHEDULE_DELAY, BCMP::Model::constant( 1. ) ).first->second ),
      _chains()
{
    _reference.setReference( true );
    construct();
}



void
Generate::BCMP_Model::setComment( const std::string& comment )
{
    _model.insertComment( comment );
}


const std::set<Task *>&
Generate::BCMP_Model::clients() const
{
    return _submodel._clients;
}


const std::set<Entity *>&
Generate::BCMP_Model::servers() const
{
    return _submodel._servers;
}


unsigned int 
Generate::BCMP_Model::number() const
{
    return _submodel.number();
}


/*
 * Return the think time computed by the submodel.
 */
 
double
Generate::BCMP_Model::think_time( unsigned int k ) const
{
    return _submodel.thinkTime( k );
}


/*
 * Construct the chains and stations for the model.
 */

void
Generate::BCMP_Model::construct()
{
    if ( std::any_of( servers().begin(), servers().end(), std::mem_fn(&Entity::hasSecondPhase) ) ) { LQIO::runtime_error( ADV_BCMP_NOT_SUPPORTED, number(), "second phases" ); }
    if ( std::any_of( servers().begin(), servers().end(), std::mem_fn(&Entity::hasThreads) ) ) { LQIO::runtime_error( ADV_BCMP_NOT_SUPPORTED, number(), "threads" ); }
    if ( std::any_of( servers().begin(), servers().end(), std::mem_fn(&Entity::isInterlocked) ) ) { LQIO::runtime_error( ADV_BCMP_NOT_SUPPORTED, number(), "interlocking" ); }
    std::for_each( clients().begin(), clients().end(), [&]( const Task * client ){ createChain( *client ); } );
    std::for_each( servers().begin(), servers().end(), [&]( const Entity * server ){ createStation( *server ); } );
}


/*
 * Add chains.  Clients in the submodel (should not have) service times.  So the chain think time for the MVA solver
 * in the submodel is used to set the service time for the reference task for the BCMP model.
 */

void
Generate::BCMP_Model::createChain( const Task& client ) 
{
    const Server& source = *client.clientStation( number() );
    for ( unsigned int k = 1; k <= K; ++k ) {
	if ( !client.hasClientChain( number(), k ) || source.V( k ) == 0. ) continue;
	_chains.emplace( std::make_pair( k, client.name() ) );
	if ( !_model.insertClosedChain( client.name(), BCMP::Model::constant( client.copies() ), BCMP::Model::constant( 0 ) ).second ) {
	    std::cerr << "Generate::BCMP_Model::createChain: client has more than one chain" << std::endl;
	    abort();
	}
	_reference.insertClass( client.name(), BCMP::Model::constant( source.V( k ) ), BCMP::Model::constant( think_time( k ) ) );
    }
}

/* Add stations */
void
Generate::BCMP_Model::createStation( const Entity& server ) 
{
    const Server& source = *server.serverStation();

    BCMP::Model::Station& destination = _model.insertStation( server.name(), stationType( server ), server.scheduling(), BCMP::Model::constant( server.copies() ) ).first->second;
    for ( unsigned k = 0; k <= K; ++k ) {
	if ( !server.hasServerChain(k) || source.S( k ) == 0. || source.V( k ) == 0. ) continue;
	destination.insertClass( _chains.at( k ), BCMP::Model::constant( source.V( k ) ), BCMP::Model::constant( source.S( k ) ) );
    }
}

BCMP::Model::Station::Type
Generate::BCMP_Model::stationType( const Entity& server ) const
{
    if ( server.isInfinite() ) return BCMP::Model::Station::Type::DELAY;
    else if ( server.isMultiServer() ) return BCMP::Model::Station::Type::MULTISERVER;
    else return BCMP::Model::Station::Type::LOAD_INDEPENDENT;
}
