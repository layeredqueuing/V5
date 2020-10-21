/* -*- c++ -*-
 * $Id: generate.cc 13954 2020-10-19 17:22:27Z greg $
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


#include "dim.h"
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
#include "generate.h"
#include "entity.h"
#include "task.h"
#include "processor.h"
#include "entry.h"
#include "model.h"
#include "submodel.h"
#include "server.h"
#include "mva.h"
#include "open.h"
#include "lqns.h"
#include "pragma.h"

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

static const char * solvers[] = {
    "Linearizer",
    "ExactMVA",
    "Schweitzer",
    "Linearizer",
    "Linearizer2"
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

ostream& 
Generate::print( ostream& output ) const
{
    for ( int i = 0; myIncludes[i]; ++i ) {
	output << "#include " << myIncludes[i] << endl;
    }
	
    output << endl << "int main ( int, char *[] )" << endl << "{" << endl;

    if ( _submodel.n_openStns() ) {
	output << "    const unsigned n_open_stations\t= " << _submodel.n_openStns() << ";" << endl;
	output << "    Vector<Server *> open_station(n_open_stations);" << endl;
    }
    if ( _submodel.n_closedStns() ) {
	output << "    const unsigned n_stations\t= " << _submodel.n_closedStns() << ";" << endl;

	output << "    const unsigned n_chains\t= " << K << ";" << endl;
	output << "    Vector<Server *> station( n_stations);" << endl;
	output << "    Population customers( n_chains );" << endl;
	output << "    VectorMath<double> thinkTime( n_chains );" << endl;
	output << "    VectorMath<unsigned> priority( n_chains );" << endl;
	output << "    Probability *** prOt;\t\t//Overtaking Probabilities" << endl << endl;
    }
	
    /* Chains */

    if ( K > 0 ) {
	output << "    /* Chains */" << endl << endl;

	for ( unsigned i = 1; i <= K; ++i ) {
	    output << "    customers[" << i << "] = " << _submodel.customers(i) << ";" << endl;
	    output << "    thinkTime[" << i << "] = " << _submodel.thinkTime(i) << ";" << endl;
	    output << "    priority[" << i << "]  = " << _submodel.priority(i)  << ";" << endl;
	}
	output << endl;
    }

    /* Clients */

    output << "    /* Clients */" << endl << endl;

    for ( std::set<Task *>::const_iterator client = _submodel._clients.begin(); client != _submodel._clients.end(); ++client ) {
	printClientStation( output, *(*client) );
    }

    /* Servers */
	
    output << endl << "    /* Servers */" << endl << endl;

    for ( std::set<Entity *>::const_iterator server = _submodel._servers.begin(); server != _submodel._servers.end(); ++server ) {
	printServerStation( output, *(*server) );
    }

    /* Overlap factor */

    if ( _submodel._overlapFactor ) {
	output << "    /* Overlap Factor */" << endl << endl;
	output << "    VectorMath<double> * _overlapFactor = new VectorMath<double> [n_chains+1];" << endl;
	output << "    for ( unsigned i = 1; i <= n_chains; ++i ) {" << endl;
	output << "        _overlapFactor[i].grow( n_chains, 1.0 );" << endl;
	output << "    }" << endl;
	for ( unsigned i = 1; i <= K; ++i ) {
	    for ( unsigned j = 1; j <= K; ++j ) {
		if ( _submodel._overlapFactor[i][j] ) {
		    output << "    _overlapFactor[" << i << "][" << j << "] = " 
			   << _submodel._overlapFactor[i][j] << ";" << endl;
		}
	    }
	}
	output << endl;
    }

    /* Symbolic names for array references. */

    output << endl << "    /* Station names */" << endl << endl;
	
    unsigned closedStnNo = 0;
    unsigned openStnNo 	 = 0;

    output << "    cout << \"Clients:\" << endl;" << endl;
    for ( std::set<Task *>::const_iterator client = _submodel._clients.begin(); client != _submodel._clients.end(); ++client ) {
	++closedStnNo;
	output << "    station[" << closedStnNo << "]\t= " << station_name( *(*client) ) << ";";
 	output << "\tcout << \"" << closedStnNo << ": " << *(*client) << "\" << endl;" << endl;
    }
    output << "    cout << endl << \"Servers:\" << endl;" << endl;
    for ( std::set<Entity *>::const_iterator server = _submodel._servers.begin(); server != _submodel._servers.end(); ++server ) {
	if ( (*server)->isInClosedModel() ) {
	    ++closedStnNo;
	    output << "    station[" << closedStnNo << "]\t= " << station_name( *(*server) ) << ";";
	    output << "\tcout << \"" << closedStnNo << ": " << *(*server) << "\" << endl;" << endl;
	}
	if ( (*server)->isInOpenModel() ) {
	    ++openStnNo;
	    output << "    open_station[" << openStnNo << "]\t= " << station_name( *(*server) ) << ";" << endl;
	}
    }
    output << "    cout << endl;" << endl;
    output << endl;

    /* Solver */
	
    output << endl << "    /* Solution */" << endl << endl;

    if ( openStnNo > 0 ) {
	output << "    Open open( n_open_stations, open_station );" << endl;
	output << "    open.solve();" << endl;
	output << "    cout << open << endl;" << endl;
    } 
    if ( closedStnNo > 0 ) {
	if ( MVA::__bounds_limit ) {
	    output << "    " << "MVA::boundsLimit = " << MVA::__bounds_limit << ";" << endl;
	}
	output << "    " << solvers[Pragma::mva()];
	output << " model( station, customers, thinkTime, priority";
	if ( _submodel._overlapFactor ) {
	    output << ", _overlapFactor";
	}
	output << " );" << endl;
	output << "    model.solve();" << endl;
	output << "    cout << model << endl;" << endl;


	if ( _submodel._overlapFactor ) {
	    output << endl
		   << "    delete [] _overlapFactor;" << endl;
	}
    }

    /* Epilogue */
	
    output << "    return 0;" << endl << "}" << endl;

    return output;
}


/*
 * Print out station parameters.
 */

ostream &
Generate::printClientStation( ostream& output, const Task& aClient ) const
{
    const Server * const aStation = aClient.clientStation(_submodel.number());
    const unsigned P = aClient.maxPhase();
    const unsigned E = aClient.nEntries();

    output << "    /* " << aClient << " */" << endl;

    /* Create the station */
	
    output << "    Server * " << station_name( aClient ) << " = new " << aStation->typeStr() 
	   << '(' << station_args( E, K, P ) << ");" << endl;

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
		    output << ";" << endl;
		}
	    }
	}
	if ( !hasService ) {
	    output << "    /* No service time for station "
		   << station_name( aClient ) << " */" << endl;
	}
    }
    output << endl;

    return output;
}



/*
 * Print out station parameters.
 */

ostream &
Generate::printServerStation( ostream& output, const Entity& aServer ) const
{
    const Server * const aStation = aServer.serverStation();
    const unsigned P = aServer.maxPhase();
    const unsigned E = aServer.nEntries();

    output << "    /* " << aServer << " */" << endl;

    /* Create the station */
	
    output << "    Server * " << station_name( aServer ) << " = new " << aStation->typeStr() << '(';
    if ( aServer.isMultiServer() ) {
	output << aServer.copies() << ',';
    }
    output << station_args( E, K, P ) << ");" << endl;

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
		    output << ";" << endl;
		}
	    }
	}
	if ( !hasService ) {
	    output << "    /* No service time for station "
		   << station_name( aServer ) << " */" << endl;
	}

		
	/* Overtaking probabilities. */
		
	if ( aServer.markovOvertaking() ) {
	    Probability *** prOt = aStation->getPrOt( e );

	    output << "    prOt = " << station_name( aServer ) << "->getPrOt("
		   << e << ");" << endl;
			
	    for( unsigned int k = 1; k <= K; ++k ) {
		for ( unsigned p = 0; p <= Entry::max_phases; ++p ) {	// Total probability!
		    for ( unsigned q = 1; q <= P; ++q ) {
			if ( prOt[k][p][q] == 0.0 ) continue;
			output << "    prOt" << overtaking_args( k, p, q )
			       << " = " << prOt[k][p][q] << ";" << endl;
		    }
		}
	    }
	}

    }

    if ( Pragma::interlock() ) {
	printInterlock( output, aServer );
    }

    output << endl;

    return output;
}



/*
 * Print out interlocking info.
 */

ostream &
Generate::printInterlock( ostream& output, const Entity& aServer ) const
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
			   << PrIL << ");" << endl;
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
    ostringstream fileName;

    fileName << file_name << "-" << aSubModel.number() << ".cc";

    ofstream output;
    output.open( fileName.str().c_str(), ios::out );

    if ( !output ) {
	cerr << LQIO::io_vars.lq_toolname << ": Cannot open output file " << fileName.str() << " - " << strerror( errno ) << endl;
    } else {
	Generate aCModel( aSubModel );
	aCModel.print( output );
    }

    output.close();
}



void
Generate::makefile( const unsigned nSubmodels )
{
    string fileName = file_name;
    fileName += ".mk";
	
    ofstream output;
    output.open( fileName.c_str(), ios::out );

    if ( !output ) {
	cerr << LQIO::io_vars.lq_toolname << ": Cannot open output file " << fileName << " - " << strerror( errno ) << endl;
	return;
    }

    string defines = "-DTESTMVA -DHAVE_BOOL -DHAVE_STD=1 -DHAVE_NAMESPACES=1";
#if HAVE_IEEEFP_H
    defines += " -DHAVE_IEEEFP_H=1";
#endif

    output << "LQNDIR=$(HOME)/Interlock_merge_LQN/lqn/interlock-merge" << endl
	   << "CC = gcc" << endl
	   << "CXX = g++" << endl
	   << "CFLAGS = -g" << endl
	   << "CXXFLAGS = -g" << endl
	   << "CPPFLAGS = " << defines << " -I. -I$(LQNDIR)" << endl
	   << "OBJS = dim.o fpgoop.o multserv.o  mva.o  open.o  ph2serv.o  pop.o  prob.o  server.o vector.o" << endl
	   << "SRCS = dim.cc fpgoop.cc multserv.cc mva.cc open.cc ph2serv.cc pop.cc prob.cc server.cc vector.cc" << endl
	   << "INCS = dim.h fpgoop.h multserv.h mva.h open.h ph2serv.h pop.h prob.h server.h vector.h" << endl;

    output << endl
	   << "all:\t";
    for ( unsigned i = 1; i <= nSubmodels; ++i ) {
	output << file_name << "-" << i << " ";
    }
    output << endl;

    for ( unsigned i = 1; i <= nSubmodels; ++i ) {
	ostringstream fileName;
	fileName << file_name << "-" << i;
	output << endl
	       << fileName.str() << ":\t$(INCS) $(SRCS) $(OBJS) " << fileName.str() << ".o" << endl
	       << "\t$(CXX) $(CXXFLAGS) -I. -o " << fileName.str() << " $(OBJS) " << fileName.str() << ".o -lm" << endl;
    }

    output << endl
	   << "clean:" << endl
	   << "\trm -f $(OBJS) *~";
    for ( unsigned i = 1; i <= nSubmodels; ++i ) {
	ostringstream fileName;
	fileName << file_name << "-" << i;
	output << " " << fileName.str() << " " << fileName.str() << ".o";
    }
    output << endl;

    output << endl
	   << "really-clean:\tclean" << endl
	   << "\tfor i in $(SRCS) $(INCS); do \\" << endl
	   << "\t  if test -L $$i; then \\" << endl
	   << "\t    rm $$i; \\" << endl
	   << "\t  fi \\" << endl
	   << "\tdone" << endl;

    output << endl
	   << "$(SRCS):" << endl
	   << "\t@if test ! -f $@; then ln -s $(LQNDIR)/lqns/$@ .; fi" << endl;

    output << endl
	   << "$(INCS):" << endl
	   << "\t@if test ! -f $@; then ln -s $(LQNDIR)/lqns/$@ .; fi" << endl;

    output << endl
	   << "dim.cc:\tvector.cc vector.h server.h" << endl;
    
}

/* ---------------------------------------------------------------------- */

/*
 * Manufacture a station name.
 */

/* static */ ostream&
Generate::print_station_name( ostream& output, const Entity& anEntity )
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
Generate::print_station_args( ostream& output, const unsigned e, const unsigned k, const unsigned p )
{
    output << e << "," << k << "," << p;
    return output; 
}

/* static */ std::ostream&
Generate::print_overtaking_args( ostream& output, const unsigned e, const unsigned k, const unsigned p )
{
    output << "[" << e << "][" << k << "][" << p << "]";
    return output; 
}


