/* -*- c++ -*-
 * $Id$
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

class GenerateArgsManip {
public:
    GenerateArgsManip( ostream& (*ff)(ostream&, const unsigned, const unsigned, const unsigned ), const unsigned ee, const unsigned kk, const unsigned pp ) :
	f(ff), e(ee), k(kk), p(pp) {}

private:
    ostream& (*f)( ostream&, const unsigned, const unsigned, const unsigned );
    const unsigned e;
    const unsigned k;
    const unsigned p;

    friend ostream& operator<<(ostream & os, const GenerateArgsManip& m ) { return m.f(os,m.e,m.k,m.p); }
};

static GenerateArgsManip station_args( const unsigned, const unsigned, const unsigned );
static GenerateArgsManip overtaking_args( const unsigned, const unsigned, const unsigned );

class GenerateStnManip {
public:
    GenerateStnManip( ostream& (*ff)(ostream&, const Entity& ), const Entity& s ) : f(ff), stn(s) {}
private:
    ostream& (*f)( ostream&, const Entity& );
    const Entity& stn;

    friend ostream& operator<<(ostream & os, const GenerateStnManip& m ) { return m.f(os,m.stn); }
};

static GenerateStnManip station_name( const Entity& ); /*  */

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

const char * Generate::file_name = 0;


/*
 * Constructor
 */

Generate::Generate( const MVASubmodel& aSubModel )
    : mySubModel( aSubModel ),  K(aSubModel.nChains())
{
}



/*
 * Make a test program based on the current layer.  Note that solver may overwrite the
 * same file over and over again.  Use -zstep to extract an intermediate model.
 */

ostream& 
Generate::print( ostream& output ) const
{
    Sequence<Task *> nextClient(mySubModel.clients);
    Sequence<Entity *> nextServer(mySubModel.servers);
    const Task * aClient;

    for ( int i = 0; myIncludes[i]; ++i ) {
	output << "#include " << myIncludes[i] << endl;
    }
	
    output << endl << "int main ( int, char *[] )" << endl << "{" << endl;

    if ( mySubModel.n_openStns() ) {
	output << "    const unsigned n_open_stations\t= " << mySubModel.n_openStns() << ";" << endl;
	output << "    Vector<Server *> open_station(n_open_stations);" << endl;
    }
    if ( mySubModel.n_closedStns() ) {
	output << "    const unsigned n_stations\t= " << mySubModel.n_closedStns() << ";" << endl;

	output << "    const unsigned n_chains\t= " << K << ";" << endl;
	output << "    Vector<Server *> station( n_stations);" << endl;
	output << "    PopVector customers( n_chains );" << endl;
	output << "    VectorMath<double> thinkTime( n_chains );" << endl;
	output << "    VectorMath<unsigned> priority( n_chains );" << endl;
	output << "    Probability *** prOt;\t\t//Overtaking Probabilities" << endl << endl;
    }
	
    /* Chains */

    if ( K > 0 ) {
	output << "    /* Chains */" << endl << endl;

	for ( unsigned i = 1; i <= K; ++i ) {
	    output << "    customers[" << i << "] = " << mySubModel.customers(i) << ";" << endl;
	    output << "    thinkTime[" << i << "] = " << mySubModel.thinkTime(i) << ";" << endl;
	    output << "    priority[" << i << "]  = " << mySubModel.priority(i)  << ";" << endl;
	}
	output << endl;
    }

    /* Clients */

    output << "    /* Clients */" << endl << endl;

    while ( aClient = nextClient() ) {
	printClientStation( output, *aClient );
    }

    /* Servers */
	
    output << endl << "    /* Servers */" << endl << endl;

    const Entity * aServer;
    while ( aServer = nextServer() ) {
	printServerStation( output, *aServer );
    }

    /* Overlap factor */

    if ( mySubModel.overlapFactor ) {
	output << "    /* Overlap Factor */" << endl << endl;
	output << "    VectorMath<double> * overlapFactor = new VectorMath<double> [n_chains+1];" << endl;
	output << "    for ( unsigned i = 1; i <= n_chains; ++i ) {" << endl;
	output << "        overlapFactor[i].grow( n_chains, 1.0 );" << endl;
	output << "    }" << endl;
	for ( unsigned i = 1; i <= K; ++i ) {
	    for ( unsigned j = 1; j <= K; ++j ) {
		if ( mySubModel.overlapFactor[i][j] ) {
		    output << "    overlapFactor[" << i << "][" << j << "] = " 
			   << mySubModel.overlapFactor[i][j] << ";" << endl;
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
    while ( aClient = nextClient() ) {
	++closedStnNo;
	output << "    station[" << closedStnNo << "]\t= " << station_name( *aClient ) << ";";
 	output << "\tcout << \"" << closedStnNo << ": " << *aClient << "\" << endl;" << endl;
    }
    output << "    cout << endl << \"Servers:\" << endl;" << endl;
    while ( aServer = nextServer() ) {
	if ( aServer->isInClosedModel() ) {
	    ++closedStnNo;
	    output << "    station[" << closedStnNo << "]\t= " << station_name( *aServer ) << ";";
	    output << "\tcout << \"" << closedStnNo << ": " << *aServer << "\" << endl;" << endl;
	}
	if ( aServer->isInOpenModel() ) {
	    ++openStnNo;
	    output << "    open_station[" << openStnNo << "]\t= " << station_name( *aServer ) << ";" << endl;
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
	if ( MVA::boundsLimit ) {
	    output << "    " << "MVA::boundsLimit = " << MVA::boundsLimit << ";" << endl;
	}
	output << "    " << solvers[pragma.getMVA()];
	output << " model( station, customers, thinkTime, priority";
	if ( mySubModel.overlapFactor ) {
	    output << ", overlapFactor";
	}
	output << " );" << endl;
	output << "    model.solve();" << endl;
	output << "    cout << model << endl;" << endl;


	if ( mySubModel.overlapFactor ) {
	    output << endl
		   << "    delete [] overlapFactor;" << endl;
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
    const Server * const aStation = aClient.clientStation(mySubModel.number());
    const unsigned P = aClient.maxPhase();
    const unsigned E = aClient.nEntries();

    output << "    /* " << aClient << " */" << endl;

    /* Create the station */
	
    output << "    Server * " << station_name( aClient ) << " = new " << aStation->typeStr() 
	   << '(' << station_args( E, K, P ) << ");" << endl;

    Sequence<Entry *> nextEntry(aClient.entries());
    const Entry * anEntry;

    while ( anEntry = nextEntry() ) {
	const unsigned e = anEntry->index();
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

    Sequence<Entry *> nextEntry(aServer.entries());
    const Entry * anEntry;

    while ( anEntry = nextEntry() ) {
	const unsigned e = anEntry->index();
	bool hasService = false;
	unsigned k;
		
	for ( k = 0; k <= K; ++k ) {
	    for ( unsigned p = 1; p <= P; ++p ) {
		if ( aStation->S( e, k, p ) > 0 ) {
		    hasService = true;
		    output << "    " << station_name( aServer ) << "->setService("  
			   << station_args( e, k, p )
			   << "," << aStation->S( e, k, p ) << ")";
		    if ( aServer.hasVariance()
			 && ( aServer.isTask()
			      || ( aServer.isProcessor() && pragma.getProcessor() == DEFAULT_PROCESSOR)) ) {
			output << ".setVariance("
			       << station_args( e, k, p )
			       << "," << anEntry->variance( p ) << ")";
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
			
	    for( k = 1; k <= K; ++k ) {
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

    if ( pragma.getInterlock() == THROUGHPUT_INTERLOCK ) {
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
    Task * aClient;
    Sequence<Entry *> nextEntry( aServer.entries() );
    Sequence<Task *> nextClient( mySubModel.clients );

    while( aClient = nextClient() ) {
	if ( aClient->throughput() == 0.0 ) continue;
		
	const Probability PrIL = aServer.prInterlock( *aClient );
	if ( PrIL == 0.0 ) continue;

	for ( unsigned i = 1; i <= aClient->clientChains( mySubModel.number() ).size(); ++i ) {
	    const unsigned k = aClient->clientChains( mySubModel.number() )[i];
	    if ( aServer.hasServerChain( k ) ) {
		const Entry * anEntry;
		while ( anEntry = nextEntry() ) {
		    output << "    " << station_name( aServer ) << "->setInterlock("
			   << anEntry->index() << "," 
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
	cerr << io_vars.lq_toolname << ": Cannot open output file " << fileName << " - " << strerror( errno ) << endl;
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
	cerr << io_vars.lq_toolname << ": Cannot open output file " << fileName << " - " << strerror( errno ) << endl;
	return;
    }

    string defines = "-DTESTMVA -DHAVE_BOOL -DHAVE_STD=1 -DHAVE_NAMESPACES=1";
#if HAVE_IEEEFP_H
    defines += " -DHAVE_IEEEFP_H=1";
#endif

    output << "LQNDIR=$(HOME)/usr/src" << endl
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

static ostream&
print_station_name( ostream& output, const Entity& anEntity )
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

static ostream&
print_station_args( ostream& output, const unsigned e, const unsigned k, const unsigned p )
{
    output << e << "," << k << "," << p;
    return output; 
}

static ostream&
print_overtaking_args( ostream& output, const unsigned e, const unsigned k, const unsigned p )
{
    output << "[" << e << "][" << k << "][" << p << "]";
    return output; 
}

static GenerateArgsManip
station_args( const unsigned e, const unsigned k, const unsigned p )
{
    return GenerateArgsManip( &print_station_args, e, k, p );
}

static GenerateArgsManip
overtaking_args( const unsigned e, const unsigned k, const unsigned p )
{
    return GenerateArgsManip( &print_overtaking_args, e, k, p );
}

static GenerateStnManip
station_name( const Entity& anEntity )
{
    return GenerateStnManip( &print_station_name, anEntity );
}
