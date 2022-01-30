#include <iomanip>
#include <iostream>
#include <getopt.h>
#include <map>
#include "mva.h"
#include "open.h"
#include "server.h"
#include "ph2serv.h"
#include "multserv.h"
#include "pop.h"
#include "prob.h"
#include "vector.h"
#include "fpgoop.h"

struct option longopts[] = {
    { "debug",		no_argument, nullptr, 'd' },
    { "exact",		no_argument, nullptr, 'e' },
    { "help",		no_argument, nullptr, 'h' },
    { "linearizer",	no_argument, nullptr, 'l' },
    { "schweitzer",	no_argument, nullptr, 's' },
    { "verbose",	no_argument, nullptr, 'v' },
    { nullptr,		0,	     nullptr, '\0' }
};

enum class solver_t { EXACT_MVA, LINEARIZER, SCHWEITZER };

const std::map<const solver_t, MVA::new_solver> solvers = {
    { solver_t::EXACT_MVA,  ExactMVA::create },
    { solver_t::SCHWEITZER, Schweitzer::create },
    { solver_t::LINEARIZER, Linearizer::create }
};


int main ( int argc, char * argv[] )
{
    MVA::debug_N = false;
    const unsigned n_stations	= 4;
    const unsigned n_chains	= 3;
    Vector<Server *> station( n_stations);
    Population customers( n_chains );
    VectorMath<double> thinkTime( n_chains );
    VectorMath<unsigned> priority( n_chains );
    Probability *** prOt;		//Overtaking Probabilities
    solver_t using_solver = solver_t::EXACT_MVA;
    bool verbose = false;

    for ( ;; ) {
	const int c = getopt_long( argc, argv, "dehlsv", longopts, nullptr );
	if ( c == EOF ) break;

	switch ( c ) {
	case 'd':	MVA::debug_N = true;	break;
	case 'e':	using_solver = solver_t::EXACT_MVA; break;
	case 'l':	using_solver = solver_t::LINEARIZER; break;
	case 's':	using_solver = solver_t::SCHWEITZER; break;
	case 'v':	verbose = true; break;
	default:
	    std::cerr << "testcust: unknown option " << static_cast<char>(c) << std::endl;
	    /* Fall through */
	case 'h':
	    std::cerr << "Usage: testcust [option]" << std::endl << std::endl
		      << "Options:" << std::endl
                      << " -d, --debug          Enable debug_N." << std::endl
                      << " -e, --exact-mva      Use Exact MVA. (default)" <<std::endl
                      << " -h, --help           Print this message." << std::endl
                      << " -l, --linearizer     Use Linearizer approximate MVA." <<std::endl
                      << " -s, --schweitzer     Use Bard-Schweitzer approximate MVA." <<std::endl
                      << " -v, --verbose        Verbose output (default is throughput only)." << std::endl;
	    exit( 1 );
	}
    }

    /* Chains */

    customers[1] = 1;
    thinkTime[1] = 0;
    priority[1]  = 0;
    customers[2] = 1;
    thinkTime[2] = 0;
    priority[2]  = 0;
    customers[3] = 1;
    thinkTime[3] = 0;
    priority[3]  = 0;

    /* Clients */

    /* Class1.1   --              ref(2)    Class1.1   0   ,Class1.1 */
    station[1] = new Client(1,3,1);
    station[1]->setService(1,1,1,1).setVisits(1,1,1,1);
    /* No service time for station t_Class1 */

    /* Class2.1   --              ref(2)    Class2.1   0   ,Class2.1 */
    station[2] = new Client(1,3,1);
    station[2]->setService(1,2,1,1).setVisits(1,2,1,1);
    /* No service time for station station[2] */

    /* Class3.1   --              ref(2)    Class3.1   0   ,Class3.1 */
    station[3] = new Client(1,3,1);
    station[3]->setService(1,3,1,1).setVisits(1,3,1,1);
    /* No service time for station station[3] */


    /* Station2.1 HVFCFS_Server   serv      Station2.1 0   ,Station2_1.1,Station2_2.1,Station2_3.1 */
    station[4] = new HVFCFS_Server(3,3,1);
    station[4]->setService(1,1,1,1).setVariance(1,1,1,1).setVisits(1,1,1,1);
    station[4]->setService(2,2,1,1).setVariance(2,2,1,1).setVisits(2,2,1,1);
    station[4]->setService(3,3,1,1).setVariance(3,3,1,1).setVisits(3,3,1,1);

    /* Station names */

    if ( verbose ) {
	std::cout << "Clients:" << std::endl;
	std::cout << "1: Class1.1   --              ref(2)    Class1.1   0   ,Class1.1" << std::endl;
	std::cout << "2: Class2.1   --              ref(2)    Class2.1   0   ,Class2.1" << std::endl;
	std::cout << "3: Class3.1   --              ref(2)    Class3.1   0   ,Class3.1" << std::endl;
	std::cout << std::endl << "Servers:" << std::endl;
	std::cout << "4: Station2.1 HVFCFS_Server   serv      Station2.1 0   ,Station2_1.1,Station2_2.1,Station2_3.1" << std::endl;
	std::cout << std::endl;
    }

    /* Solution */

    MVA::__bounds_limit = 8;

    for ( unsigned int N1 = 0; N1 <= 1; ++N1 ) {
	for ( unsigned int N2 = 0; N2 <= 1; ++N2 ) {
	    for ( unsigned int N3 = 0; N3 <= 1; ++N3 ) {
		customers[1] = N1;
		customers[2] = N2;
		customers[3] = N3;

		if ( verbose ) {
		    std::cout << "---- " << customers << " ----" << std::endl;
		}
#if 1
		const MVA::new_solver solver = solvers.at(using_solver);
		MVA * model = (*solver)( station, customers, thinkTime, priority, nullptr );
		model->solve();
		if ( verbose ) {
		    std::cout << *model;
		} else {
		    model->printX( std::cout );
		}
		delete model;
#else
		ExactMVA model( station, customers, thinkTime, priority );
		model.solve();
		std::cout << model << std::endl << std::endl;
#endif
		if ( verbose ) {
		    std::cout << std::endl;
		}
	    }
	}
    }

    return 0;
}
