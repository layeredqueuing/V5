#include <cstdlib>
#include <unistd.h>
#include <getopt.h>
#include <libgen.h>
#include <string>
#include "mva.h"
#include "open.h"
#include "server.h"
#include "ph2serv.h"
#include "multserv.h"
#include "pop.h"
#include "prob.h"
#include "vector.h"
#include "fpgoop.h"

const struct option longopts[] =
    /* name */ /* has arg */ /*flag */ /* val */
{
    { "customers",	 required_argument, 0, 'c' },
    { "multiplicity",	 required_argument, 0, 'm' },
    { "debug",           no_argument,       0, 'd' },
    { "all",		 no_argument,	    0, 'a' },
    { "bard", 		 no_argument,       0, 'b' },
    { "schweitzer",	 no_argument,       0, 's' },
    { "exact-mva",       no_argument,       0, 'e' },
    { "linearizer",      no_argument,       0, 'l' },
    { "help",            no_argument,       0, 'h' },
    { 0, 0, 0, 0 }
};

const char opts[]	= "c:m:dabselh";
const char * opthelp[]  = {
    /* "customers",       */    "Set the number of customers in chain 1 to ARG.",
    /* "multiplicity",	  */	"Set the multiplicity of the multiserver to ARG.",
    /* "debug",           */    "Enable debug code.",
    /* "bard"		  */    "Test using Bard-Schweitzer solver.",
    /* "schweitzer"	  */    "Test using Bard-Schweitzer solver.",
    /* "exact-mva",       */    "Test using Exact MVA solver.",
    /* "linearizer",      */    "Test using Generic Linearizer.",
    /* "help",            */    "Show this.",
    0
};

bool use_schweitzer = false;
bool use_linearizer = false;
bool use_exact_mva  = false;
std::string progname;

extern char *optarg;
extern int optind;
extern int optopt;
extern int opterr;
extern int optreset;
static void usage();

int main ( int argc, char *argv[] )
{
    unsigned J		    	= 2;
    unsigned n_customers	= 4;
    const unsigned n_stations	= 2;
    const unsigned n_chains	= 1;
    char * endptr = 0;

    progname = basename( argv[0] );

    for ( ;; ) {
	const int c = getopt_long( argc, argv, opts, longopts, NULL );
	if ( c == EOF ) break;

	switch( c ) {
	case 'c':
	    n_customers = strtol( optarg, &endptr, 10 );
	    if ( n_customers == 0 || *endptr != '\0' ) {
		cerr << progname << " --customers=" << optarg << " is invalid." << endl;
	    }
	    break;

	case 'm':
	    J = strtol( optarg, &endptr, 10 );
	    if ( J == 0 || *endptr != '\0' ) {
		cerr << progname << " --customers=" << optarg << " is invalid." << endl;
	    }
	    break;

	case 'a':
	    use_schweitzer = true;
	    use_linearizer = true;
	    use_exact_mva  = true;
	    break;

	case 's':
	case 'b':
	    use_schweitzer = true;
	    break;

	case 'l':
	    use_linearizer = true;
	    break;

	case 'e':
	    use_exact_mva  = true;
	    break;

	case 'd':
#if DEBUG_MVA
	    MVA::debug_P = true;
	    MVA::debug_U = true;
	    MVA::debug_W = true;
	    Conway_Multi_Server::debug_XE = true;
#endif
	    break;

	default:
	    usage();
	    exit( 1 );
	}
    }

    if ( !use_schweitzer && !use_linearizer && !use_exact_mva ) {
	use_exact_mva  = true;
    }

    Vector<Server *> station( n_stations);
    PopVector customers( n_chains );
    VectorMath<double> thinkTime( n_chains );
    VectorMath<unsigned> priority( n_chains );
    Probability *** prOt;		//Overtaking Probabilities

    /* Chains */

    customers[1] = n_customers;
    thinkTime[1] = 0;
    priority[1]  = 0;

    /* Clients */

    /* c0       ref(4)    1     c0       0   c0  */
    Client t_c0(1,1,1);
    t_c0.setService(1,1,1,1).setVisits(1,1,1,1);


    /* Servers */

    /* t0       mult(2)   1     p0       0   e0 (Reiser_Multi_Server) */
    Server * t_t0 = 0;
    if ( progname == "reiser" ) {
	t_t0 = new Reiser_Multi_Server(J,1,1,1);
    } else {
	t_t0 = new Conway_Multi_Server(J,1,1,1);
    }
    t_t0->setService(1,1,1,1).setVisits(1,1,1,1);

    /* Station names */

    cout << "Clients:" << endl;
    station[1]	= &t_c0; cout << "1: c0       ref(" << customers[1] << ")    1     c0       0   c0 " << endl;
    cout << endl << "Servers:" << endl;
    station[2]	= t_t0;	cout << "2: t0       mult(" << J << ")   1     p0       0   e0 (" << t_t0->typeStr() << ")" << endl;
    cout << endl;


    /* Solution */

    MVA::boundsLimit = 8;
    if ( use_exact_mva ) {
	cout << endl << "-- Exact MVA --" << endl;
	ExactMVA model( station, customers, thinkTime, priority );
	model.solve();
	cout << model << endl;
    }
    if ( use_schweitzer ) {
	cout << endl << "-- Schweitzer --" << endl;
	Schweitzer model( station, customers, thinkTime, priority );
	model.solve();
	cout << model << endl;
    }
    if ( use_linearizer ) {
	cout << endl << "-- Linearizer --" << endl;
	Linearizer model( station, customers, thinkTime, priority );
	model.solve();
	cout << model << endl;
    }
    return 0;
}

static void
usage() 
{
    cerr << "Usage: " << progname;
    cerr << " [option]" << endl << endl;
    cerr << "Options" << endl;
    const char ** p = opthelp;
    for ( const struct option *o = longopts; (o->name || o->val) && *p; ++o, ++p ) {
	string s;
	if ( o->name ) {
	    s = "--";
	    s += o->name;
	    switch ( o->val ) {
	    }
	} else {
	    s = " ";
	}
	if ( isascii(o->val) && isgraph(o->val) ) {
	    cerr << " -" << static_cast<char>(o->val) << ", ";
	} else {
	    cerr << "     ";
	}
	cerr.setf( ios::left, ios::adjustfield );
	cerr << setw(24) << s << *p << endl;
    }
}
