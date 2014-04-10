#include "mva.h"
#include "open.h"
#include "server.h"
#include "ph2serv.h"
#include "multserv.h"
#include "pop.h"
#include "prob.h"
#include "vector.h"
#include "fpgoop.h"

int main ( int, char *[] )
{
    const unsigned n_stations	= 2;
    const unsigned n_chains	= 1;
    Vector<Server *> station( n_stations);
    PopVector customers( n_chains );
    VectorMath<double> thinkTime( n_chains );
    VectorMath<unsigned> priority( n_chains );
    Probability *** prOt;		//Overtaking Probabilities

    /* Chains */

    customers[1] = 4;
    thinkTime[1] = 0;
    priority[1]  = 0;

    /* Clients */

    /* c0       ref(4)    1     c0       0   c0  */
    Server * t_c0 = new Client(1,1,1);
    t_c0->setService(1,1,1,1).setVisits(1,1,1,1);


    /* Servers */

    /* t0       mult(2)   1     p0       0   e0 (Reiser_Multi_Server) */
    Server * t_t0 = new FCFS_Server(1,1,1);
    t_t0->setService(1,1,1,1).setVisits(1,1,1,1);

    /* Station names */

    cout << "Clients:" << endl;
    station[1]	= t_c0;	cout << "1: c0       ref(4)    1     c0       0   c0 " << endl;
    cout << endl << "Servers:" << endl;
    station[2]	= t_t0;	cout << "2: t0       mult(2)   1     p0       0   e0 (Reiser_Multi_Server)" << endl;
    cout << endl;


    /* Solution */

    MVA::boundsLimit = 8;
//    Linearizer model( station, customers, thinkTime, priority );
    ExactMVA model( station, customers, thinkTime, priority );
    model.solve();
    cout << model << endl;
    return 0;
}
