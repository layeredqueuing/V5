/* -*- c++ -*-
 * $Id: generate.cc 15660 2022-06-09 01:09:12Z greg $
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
#include <sstream>
#include <fstream>
#include <cstdlib>
#include <cmath>
#include <errno.h>
#include <getopt.h>
#if HAVE_SYS_UTSNAME_H
#include <sys/utsname.h>
#endif
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <lqio/error.h>
#include <lqio/glblerr.h>
#include <mva/mva.h>
#include <mva/open.h>
#include <mva/server.h>
#include <mva/multserv.h>
#include <mva/ph2serv.h>
#include "entity.h"
#include "entry.h"
#include "generate.h"
#include "flags.h"
#include "model.h"
#include "pragma.h"
#include "processor.h"
#include "submodel.h"
#include "task.h"

const std::vector<std::string> Generate::__includes = {
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

const std::vector<struct option> Generate::__longopts = {
    { "fraction", no_argument, nullptr, 'd' },
    { "queue-length", no_argument, nullptr, 'l' },
    { "customers", no_argument, nullptr, 'n' },
    { "marginals", no_argument, nullptr, 'p' },
    { "utilization", no_argument, nullptr, 'u' },
    { "waiting", no_argument, nullptr, 'w' },
    { "throughput", no_argument, nullptr, 'x' },
    { "multiserver", required_argument, nullptr, 'M' },
    { "exact-mva",  no_argument, nullptr, 'E' },
    { "schweitzer", no_argument, nullptr, 'S' },
    { "linearizer", no_argument, nullptr, 'L' },
    { "help",	 no_argument, nullptr, 'h' }
};

const std::map<const char, const std::string> Generate::__help = {
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
    { 'h', "Print this help." }
};

const std::map<const int,const std::string> Generate::__argument_type = {
    { no_argument,       "no_argument"       },
    { required_argument, "required_argument" },
    { optional_argument, "optional_argument" }
};

const std::map<const Pragma::MVA,const std::string> Generate::__solvers = {
    { Pragma::MVA::EXACT, 		"ExactMVA" },
    { Pragma::MVA::FAST, 		"Linearizer2" },
    { Pragma::MVA::LINEARIZER,		"Linearizer" },
    { Pragma::MVA::ONESTEP, 		"OneStepMVA" },
    { Pragma::MVA::ONESTEP_LINEARIZER, 	"OneStepLinearizer" },
    { Pragma::MVA::SCHWEITZER, 		"Schweitzer" }
};

const std::map<const Pragma::Multiserver,const Generate::Multiserver> Generate::__multiservers =
{
    { Pragma::Multiserver::DEFAULT, Multiserver::DEFAULT },
    { Pragma::Multiserver::CONWAY,  Multiserver::CONWAY  },
    { Pragma::Multiserver::REISER,  Multiserver::REISER  },
    { Pragma::Multiserver::ROLIA,   Multiserver::ROLIA   },
    { Pragma::Multiserver::ZHOU,    Multiserver::ZHOU    }
};


const std::map<const Pragma::Multiserver,const std::pair<const std::string&,const std::string&> > Generate::__stations =
{
    { Pragma::Multiserver::CONWAY,  { Conway_Multi_Server::__type_str, Markov_Phased_Conway_Multi_Server::__type_str } },
    { Pragma::Multiserver::REISER,  { Reiser_Multi_Server::__type_str, Markov_Phased_Reiser_Multi_Server::__type_str } },
    { Pragma::Multiserver::ROLIA,   { Rolia_Multi_Server::__type_str,  Markov_Phased_Rolia_Multi_Server::__type_str  } },
    { Pragma::Multiserver::ZHOU,    { Zhou_Multi_Server::__type_str,   Markov_Phased_Zhou_Multi_Server::__type_str   } }
};

std::string Generate::__directory_name;


/*
 * Constructor
 */

Generate::Generate( const MVASubmodel& aSubModel )
    : _submodel( aSubModel ),  K(aSubModel.nChains())
{
}



/*
 * Make a test program based on the current layer.  Note that solver may overwrite the
 * same file over and over again.  Use -zstep to extract an intermediate model.
 */

std::ostream& 
Generate::print( std::ostream& output ) const
{
    for ( const auto& s : __includes ) {
	output << "#include " << s << std::endl;
    }
	
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
    
    output << std::endl << "enum class Multiserver {null";
    for ( std::map<const std::string,const Pragma::Multiserver>::const_iterator m = Pragma::__multiserver_pragma.begin(); m != Pragma::__multiserver_pragma.end(); ++m ) {
	if ( Generate::__multiservers.find(m->second) == Generate::__multiservers.end() ) continue;
	output << ",";
	output << m->first;
    }
    output << "};" << std::endl;
    
    output << std::endl << "const std::map<const std::string,const Multiserver> multiservers = {";
    bool first = true;
    for ( std::map<const std::string,const Pragma::Multiserver>::const_iterator m = Pragma::__multiserver_pragma.begin(); m != Pragma::__multiserver_pragma.end(); ++m ) {
	if ( Generate::__multiservers.find(m->second) == Generate::__multiservers.end() ) continue;
	if ( !first ) output << ",";
	first = false;
	output << std::endl << "    { \"" << m->first << "\", Multiserver::" << m->first << " }";
    }
    output << std::endl << "};" << std::endl;

//    output << std::endl << "static Server * new_Multi_Server( Multiserver multiserver, unsigned int copies, unsigned int e, unsigned k, unsigned int p );" << std::endl;
    
    output << std::endl << "int main ( int argc, char* argv[] )" << std::endl << "{" << std::endl
	   << "    extern char *optarg;" << std::endl
	   << "    Multiserver multiserver = Multiserver::null;" << std::endl;

    if ( _submodel.n_openStns() ) {
	output << "    const unsigned n_open_stations\t= " << _submodel.n_openStns() << ";" << std::endl;
	output << "    Vector<Server *> open_station(n_open_stations);" << std::endl;
    }
    if ( _submodel.n_closedStns() ) {
	output << "    const unsigned n_stations\t= " << _submodel.n_closedStns() << ";" << std::endl;

	output << "    const unsigned n_chains\t= " << K << ";" << std::endl;
	output << "    Vector<Server *> station( n_stations);" << std::endl;
	output << "    Population customers( n_chains );" << std::endl;
	output << "    VectorMath<double> thinkTime( n_chains );" << std::endl;
	output << "    VectorMath<unsigned> priority( n_chains );" << std::endl;
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
    output << "        case 'M':" << std::endl;
    output << "            if ( optarg != nullptr ) {" << std::endl;
    output << "                const std::map<const std::string,const Multiserver>::const_iterator m = multiservers.find(optarg);" << std::endl;
    output << "                if ( m != multiservers.end() ) multiserver = m->second;" << std::endl;
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
	    output << "    thinkTime[" << i << "] = " << _submodel.thinkTime(i) << ";" << std::endl;
	    output << "    priority[" << i << "]  = " << _submodel.priority(i)  << ";" << std::endl;
	}
	output << std::endl;
    }

    /* Clients */

    output << "    /* Clients */" << std::endl << std::endl;

    for ( std::set<Task *>::const_iterator client = _submodel._clients.begin(); client != _submodel._clients.end(); ++client ) {
	printClientStation( output, **client );
    }

    /* Servers */
	
    output << std::endl << "    /* Servers */" << std::endl << std::endl;

    for ( std::set<Entity *>::const_iterator server = _submodel._servers.begin(); server != _submodel._servers.end(); ++server ) {
	printServerStation( output, **server );
    }

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

    output << "    std::cout << \"Clients:\" << std::endl;" << std::endl;
    for ( std::set<Task *>::const_iterator client = _submodel._clients.begin(); client != _submodel._clients.end(); ++client ) {
	++closedStnNo;
	output << "    station[" << closedStnNo << "]\t= " << station_name( **client ) << ";";
 	output << "\tstd::cout << \"" << closedStnNo << ": " << **client << "\" << std::endl;" << std::endl;
    }
    output << "    std::cout << std::endl << \"Servers:\" << std::endl;" << std::endl;
    for ( std::set<Entity *>::const_iterator server = _submodel._servers.begin(); server != _submodel._servers.end(); ++server ) {
	if ( (*server)->isClosedModelServer() ) {
	    ++closedStnNo;
	    output << "    station[" << closedStnNo << "]\t= " << station_name( **server ) << ";";
	    output << "\tstd::cout << \"" << closedStnNo << ": " << **server << "\" << std::endl;" << std::endl;
	}
	if ( (*server)->isOpenModelServer() ) {
	    ++openStnNo;
	    output << "    open_station[" << openStnNo << "]\t= " << station_name( **server ) << ";" << std::endl;
	}
    }
    output << "    std::cout << std::endl;" << std::endl;
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
	output << "    MVA* model = (*solver)( station, customers, thinkTime, priority";
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
Generate::printClientStation( std::ostream& output, const Task& aClient ) const
{
    const Server * const aStation = aClient.clientStation(_submodel.number());
    const unsigned P = aClient.maxPhase();
    const unsigned E = aClient.nEntries();

    output << "    /* " << aClient << " */" << std::endl;

    /* Create the station */
	
    output << "    Server * " << station_name( aClient ) << " = new " << aStation->typeStr() 
	   << '(' << station_args( E, K, P ) << ");" << std::endl;

    for ( std::vector<Entry *>::const_iterator entry = aClient.entries().begin(); entry != aClient.entries().end(); ++entry ) {
	const unsigned e = (*entry)->index();
	bool hasService = false;

	for ( unsigned k = 1; k <= K; ++k ) {
	    for ( unsigned p = 1; p <= P; ++p ) {
		if ( aStation->S( e, k, p ) > 0 ) {
		    hasService = true;
		    output << "    " << station_name( aClient ) << "->setService("  
			   << station_args( e, k, p ) << "," 
			   << aStation->S( e, k, p ) << ")";
		    for ( unsigned q = 1; q <= Entry::max_phases; ++q ) {
			if ( aStation->V( e, k, q ) == 0 ) continue;
			output << ".setVisits(" 
			       << station_args( e, k, q ) 
			       << "," << aStation->V( e, k, q ) << ")";
		    }
		    output << ";" << std::endl;
		}
	    }
	}
	if ( !hasService ) {
	    output << "    /* No service time for station "
		   << station_name( aClient ) << " */" << std::endl;
	}
    }
    output << std::endl;

    return output;
}



/*
 * Print out station parameters.
 */

std::ostream &
Generate::printServerStation( std::ostream& output, const Entity& server ) const
{
    const Server * const aStation = server.serverStation();
    const unsigned P = server.maxPhase();
    const unsigned E = server.nEntries();

    output << "    /* " << server << " */" << std::endl;

    /* Create the station */
	
    output << "    Server * " << station_name( server );
    if ( server.isMultiServer() ) {
	output << ";" << std::endl << "    switch ( multiserver ) {" << std::endl
	       << "    default: " << station_name( server ) << " = new " << aStation->typeStr() << '(' << server.copies() << ',' << station_args( E, K, P ) << "); break;" << std::endl;
	for ( std::map<const std::string,const Pragma::Multiserver>::const_iterator m = Pragma::__multiserver_pragma.begin(); m != Pragma::__multiserver_pragma.end(); ++m ) {
	    if ( Generate::__multiservers.find(m->second) == Generate::__multiservers.end() ) continue;
	    output << "    case Multiserver::" << m->first << ": " << station_name( server ) << " = new ";
	    if ( P == 1 ) output << __stations.at(m->second).first;	/* Single phase */
	    else output << __stations.at(m->second).second;		/* Two phase */
	    output << '(' << server.copies() << ',' << station_args( E, K, P ) << "); break;" << std::endl;
	}
	output << "    }" << std::endl;
    } else {
	output << "= new " << aStation->typeStr() << '(' << station_args( E, K, P ) << ");" << std::endl;
    }

    for ( std::vector<Entry *>::const_iterator entry = server.entries().begin(); entry != server.entries().end(); ++entry ) {
	const unsigned e = (*entry)->index();
	bool hasService = false;
	
	for ( unsigned k = 0; k <= K; ++k ) {
	    for ( unsigned p = 1; p <= P; ++p ) {
		if ( aStation->S( e, k, p ) > 0 ) {
		    hasService = true;
		    output << "    " << station_name( server ) << "->setService("  
			   << station_args( e, k, p )
			   << "," << aStation->S( e, k, p ) << ")";
		    if ( server.hasVariance()
			 && ( server.isTask()
			      || ( server.isProcessor() && Pragma::defaultProcessorScheduling() )) ) {
			output << ".setVariance("
			       << station_args( e, k, p )
			       << "," << (*entry)->varianceForPhase(p) << ")";
		    }
		    for ( unsigned q = 1; q <= Entry::max_phases; ++q ) {
			if ( aStation->V( e, k, q ) == 0 ) continue;
			output << ".setVisits(" 
			       << station_args( e, k, q )
			       << "," << aStation->V( e, k, q ) << ")";
		    }
		    output << ";" << std::endl;
		}
	    }
	}
	if ( !hasService ) {
	    output << "    /* No service time for station "
		   << station_name( server ) << " */" << std::endl;
	}

		
	/* Overtaking probabilities. */
		
	if ( server.markovOvertaking() ) {
	    Probability *** prOt = aStation->getPrOt( e );

	    output << "    prOt = " << station_name( server ) << "->getPrOt("
		   << e << ");" << std::endl;
			
	    for( unsigned int k = 1; k <= K; ++k ) {
		for ( unsigned p = 0; p <= Entry::max_phases; ++p ) {	// Total probability!
		    for ( unsigned q = 1; q <= P; ++q ) {
			if ( prOt[k][p][q] == 0.0 ) continue;
			output << "    prOt" << overtaking_args( k, p, q )
			       << " = " << prOt[k][p][q] << ";" << std::endl;
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
Generate::printInterlock( std::ostream& output, const Entity& server ) const
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


/*
 * Make a test program based on the current layer.  Note that solver may overwrite the
 * same file over and over again.  Use -zstep to extract an intermediate model.
 */

void
Generate::print( const MVASubmodel& aSubModel ) 
{
    std::ostringstream fileName;
    fileName << __directory_name << "/submodel-" << aSubModel.number() << ".cc";

    std::ofstream output;
    output.open( fileName.str().c_str(), std::ios::out );

    if ( !output ) {
	std::cerr << LQIO::io_vars.lq_toolname << ": Cannot open output file " << fileName.str() << " - " << strerror( errno ) << std::endl;
    } else {
	Generate aCModel( aSubModel );
	aCModel.print( output );
    }

    output.close();
}



void
Generate::makefile( const unsigned nSubmodels )
{
    const std::string makefileName = __directory_name + "/Makefile";
	
    std::ofstream output;
    output.open( makefileName.c_str(), std::ios::out );

    if ( !output ) {
	std::cerr << LQIO::io_vars.lq_toolname << ": Cannot open output file " << makefileName << " - " << strerror( errno ) << std::endl;
	return;
    }

    const std::string defines = "-g -std=c++11 -Wall -DDEBUG_MVA=1 -DHAVE_CONFIG_H=1 -I$(LQNDIR)/libmva -I$(LQNDIR)/libmva/src/headers/mva";
     
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


/*
 * Look for libmva
 */

bool
Generate::find_libmva( std::string& pathname )
{
    if ( access( pathname.c_str(), F_OK ) == 0 ) {			/* Directory exists? 	*/
	std::string filename = pathname + "/libmva";
	pathname.insert( 0, "../" );
	if ( access( filename.c_str(), F_OK ) == 0 ) return true;	/* libmva found		*/
	return find_libmva( pathname );					/* try parent 		*/
    } 
    return false;							/* Not found		*/
}

/* ---------------------------------------------------------------------- */

/*
 * Manufacture a station name.
 */

/* static */ std::ostream&
Generate::print_station_name( std::ostream& output, const Entity& anEntity )
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
Generate::print_station_args( std::ostream& output, const unsigned e, const unsigned k, const unsigned p )
{
    output << e << "," << k << "," << p;
    return output; 
}

/* static */ std::ostream&
Generate::print_overtaking_args( std::ostream& output, const unsigned e, const unsigned k, const unsigned p )
{
    output << "[" << e << "][" << k << "][" << p << "]";
    return output; 
}


