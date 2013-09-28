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

    customers[1] = 10;
    thinkTime[1] = 0;
    priority[1]  = 0;

    /* Clients */

    /* client   ref(10)   1     client   0   client  */
    Server * t_client = new Client(1,1,1);
    /* No service time for station t_client */


    /* Servers */

    /* client   mult(4)   1     FCFS          (Conway_Multi_Server) */
    Server * p_client = new Conway_Multi_Server(4,1,1,1);
    p_client->setService(1,1,1,1).setVisits(1,1,1,1);


    /* Station names */

    cout << "Clients:" << endl;
    station[1]	= t_client;	cout << "1: client   ref(10)   1     client   0   client " << endl;
    cout << endl << "Servers:" << endl;
    station[2]	= p_client;	cout << "2: client   mult(4)   1     FCFS          (Conway_Multi_Server)" << endl;
    cout << endl;


    /* Solution */

    MVA::boundsLimit = 8;
    Linearizer model( station, customers, thinkTime, priority );
    model.solve();
    cout << model << endl;
    return 0;
}
