/* -*- c++ -*-
 * $Id: generate.cc 14823 2021-06-15 18:07:36Z greg $
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
#if HAVE_SYS_UTSNAME_H
#include <sys/utsname.h>
#endif
#include <lqio/error.h>
#include <lqio/glblerr.h>
#include <mva/mva.h>
#include <mva/open.h>
#include <mva/server.h>
#include "entity.h"
#include "entry.h"
#include "generate.h"
#include "flags.h"
#include "model.h"
#include "pragma.h"
#include "processor.h"
#include "submodel.h"
#include "task.h"

static const char * myIncludes[] = {
    "\"mva.h\"",
    "\"open.h\"",
    "\"server.h\"",
    "\"ph2serv.h\"",
    "\"multserv.h\"",
    "\"pop.h\"",
    "\"prob.h\"",
    "\"vector.h\"",
    "\"fpgoop.h\"",
    0
};

static const std::map<const Pragma::MVA,const std::string> solvers = {
    { Pragma::MVA::EXACT, 		"ExactMVA" },
    { Pragma::MVA::FAST, 		"Linearizer2" },
    { Pragma::MVA::LINEARIZER,		"Linearizer" },
    { Pragma::MVA::ONESTEP, 		"OneStepMVA" },
    { Pragma::MVA::ONESTEP_LINEARIZER, 	"OneStepLinearizer" },
    { Pragma::MVA::SCHWEITZER, 		"Schweitzer" }
};

std::string Generate::file_name;


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
    for ( int i = 0; myIncludes[i]; ++i ) {
	output << "#include " << myIncludes[i] << std::endl;
    }
	
    output << std::endl << "int main ( int, char *[] )" << std::endl << "{" << std::endl;

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
	output << "    Probability *** prOt;\t\t//Overtaking Probabilities" << std::endl << std::endl;
    }
	
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
	printClientStation( output, *(*client) );
    }

    /* Servers */
	
    output << std::endl << "    /* Servers */" << std::endl << std::endl;

    for ( std::set<Entity *>::const_iterator server = _submodel._servers.begin(); server != _submodel._servers.end(); ++server ) {
	printServerStation( output, *(*server) );
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

    output << "    cout << \"Clients:\" << std::endl;" << std::endl;
    for ( std::set<Task *>::const_iterator client = _submodel._clients.begin(); client != _submodel._clients.end(); ++client ) {
	++closedStnNo;
	output << "    station[" << closedStnNo << "]\t= " << station_name( *(*client) ) << ";";
 	output << "\tcout << \"" << closedStnNo << ": " << *(*client) << "\" << std::endl;" << std::endl;
    }
    output << "    cout << std::endl << \"Servers:\" << std::endl;" << std::endl;
    for ( std::set<Entity *>::const_iterator server = _submodel._servers.begin(); server != _submodel._servers.end(); ++server ) {
	if ( (*server)->isInClosedModel() ) {
	    ++closedStnNo;
	    output << "    station[" << closedStnNo << "]\t= " << station_name( *(*server) ) << ";";
	    output << "\tcout << \"" << closedStnNo << ": " << *(*server) << "\" << std::endl;" << std::endl;
	}
	if ( (*server)->isInOpenModel() ) {
	    ++openStnNo;
	    output << "    open_station[" << openStnNo << "]\t= " << station_name( *(*server) ) << ";" << std::endl;
	}
    }
    output << "    cout << std::endl;" << std::endl;
    output << std::endl;

    /* Solver */
	
    output << std::endl << "    /* Solution */" << std::endl << std::endl;

    if ( openStnNo > 0 ) {
	output << "    Open open( n_open_stations, open_station );" << std::endl;
	output << "    open.solve();" << std::endl;
	output << "    cout << open << std::endl;" << std::endl;
    } 
    if ( closedStnNo > 0 ) {
	if ( MVA::__bounds_limit ) {
	    output << "    " << "MVA::boundsLimit = " << MVA::__bounds_limit << ";" << std::endl;
	}
	output << "    " << solvers.at(Pragma::mva());
	output << " model( station, customers, thinkTime, priority";
	if ( _submodel._overlapFactor ) {
	    output << ", _overlapFactor";
	}
	output << " );" << std::endl;
	output << "    model.solve();" << std::endl;
	output << "    cout << model << std::endl;" << std::endl;


	if ( _submodel._overlapFactor ) {
	    output << std::endl
		   << "    delete [] _overlapFactor;" << std::endl;
	}
    }

    /* Epilogue */
	
    output << "    return 0;" << std::endl << "}" << std::endl;

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
Generate::printServerStation( std::ostream& output, const Entity& aServer ) const
{
    const Server * const aStation = aServer.serverStation();
    const unsigned P = aServer.maxPhase();
    const unsigned E = aServer.nEntries();

    output << "    /* " << aServer << " */" << std::endl;

    /* Create the station */
	
    output << "    Server * " << station_name( aServer ) << " = new " << aStation->typeStr() << '(';
    if ( aServer.isMultiServer() ) {
	output << aServer.copies() << ',';
    }
    output << station_args( E, K, P ) << ");" << std::endl;

    for ( std::vector<Entry *>::const_iterator entry = aServer.entries().begin(); entry != aServer.entries().end(); ++entry ) {
	const unsigned e = (*entry)->index();
	bool hasService = false;
	
	for ( unsigned k = 0; k <= K; ++k ) {
	    for ( unsigned p = 1; p <= P; ++p ) {
		if ( aStation->S( e, k, p ) > 0 ) {
		    hasService = true;
		    output << "    " << station_name( aServer ) << "->setService("  
			   << station_args( e, k, p )
			   << "," << aStation->S( e, k, p ) << ")";
		    if ( aServer.hasVariance()
			 && ( aServer.isTask()
			      || ( aServer.isProcessor() && Pragma::defaultProcessorScheduling() )) ) {
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
		   << station_name( aServer ) << " */" << std::endl;
	}

		
	/* Overtaking probabilities. */
		
	if ( aServer.markovOvertaking() ) {
	    Probability *** prOt = aStation->getPrOt( e );

	    output << "    prOt = " << station_name( aServer ) << "->getPrOt("
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
	printInterlock( output, aServer );
    }

    output << std::endl;

    return output;
}



/*
 * Print out interlocking info.
 */

std::ostream &
Generate::printInterlock( std::ostream& output, const Entity& aServer ) const
{
    for ( std::set<Task *>::const_iterator client = _submodel._clients.begin(); client != _submodel._clients.end(); ++client ) {
	if ( (*client)->throughput() == 0.0 ) continue;
		
	const Probability PrIL = aServer.prInterlock( *(*client) );
	if ( PrIL == 0.0 ) continue;

	for ( unsigned i = 1; i <= (*client)->clientChains( _submodel.number() ).size(); ++i ) {
	    const unsigned k = (*client)->clientChains( _submodel.number() )[i];
	    if ( aServer.hasServerChain( k ) ) {
		for ( std::vector<Entry *>::const_iterator entry = aServer.entries().begin(); entry != aServer.entries().end(); ++entry ) {
		    output << "    " << station_name( aServer ) << "->setInterlock("
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

    fileName << file_name << "-" << aSubModel.number() << ".cc";

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
    std::string fileName = file_name;
    fileName += ".mk";
	
    std::ofstream output;
    output.open( fileName.c_str(), std::ios::out );

    if ( !output ) {
	std::cerr << LQIO::io_vars.lq_toolname << ": Cannot open output file " << fileName << " - " << strerror( errno ) << std::endl;
	return;
    }

    std::string defines = "-DTESTMVA -DHAVE_BOOL -DHAVE_STD=1 -DHAVE_NAMESPACES=1";
#if HAVE_IEEEFP_H
    defines += " -DHAVE_IEEEFP_H=1";
#endif

    output << "LQNDIR=$(HOME)/Interlock_merge_LQN/lqn/interlock-merge" << std::endl
	   << "CC = gcc" << std::endl
	   << "CXX = g++" << std::endl
	   << "CFLAGS = -g" << std::endl
	   << "CXXFLAGS = -g" << std::endl
	   << "CPPFLAGS = " << defines << " -I. -I$(LQNDIR)" << std::endl
	   << "OBJS = dim.o fpgoop.o multserv.o  mva.o  open.o  ph2serv.o  pop.o  prob.o  server.o vector.o" << std::endl
	   << "SRCS = dim.cc fpgoop.cc multserv.cc mva.cc open.cc ph2serv.cc pop.cc prob.cc server.cc vector.cc" << std::endl
	   << "INCS = dim.h fpgoop.h multserv.h mva.h open.h ph2serv.h pop.h prob.h server.h vector.h" << std::endl;

    output << std::endl
	   << "all:\t";
    for ( unsigned i = 1; i <= nSubmodels; ++i ) {
	output << file_name << "-" << i << " ";
    }
    output << std::endl;

    for ( unsigned i = 1; i <= nSubmodels; ++i ) {
	std::ostringstream fileName;
	fileName << file_name << "-" << i;
	output << std::endl
	       << fileName.str() << ":\t$(INCS) $(SRCS) $(OBJS) " << fileName.str() << ".o" << std::endl
	       << "\t$(CXX) $(CXXFLAGS) -I. -o " << fileName.str() << " $(OBJS) " << fileName.str() << ".o -lm" << std::endl;
    }

    output << std::endl
	   << "clean:" << std::endl
	   << "\trm -f $(OBJS) *~";
    for ( unsigned i = 1; i <= nSubmodels; ++i ) {
	std::ostringstream fileName;
	fileName << file_name << "-" << i;
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
	   << "\t@if test ! -f $@; then ln -s $(LQNDIR)/lqns/$@ .; fi" << std::endl;

    output << std::endl
	   << "$(INCS):" << std::endl
	   << "\t@if test ! -f $@; then ln -s $(LQNDIR)/lqns/$@ .; fi" << std::endl;

    output << std::endl
	   << "dim.cc:\tvector.cc vector.h server.h" << std::endl;
    
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


