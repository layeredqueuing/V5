/* $Id$ */
/************************************************************************/
/* invalid.c:	PARASOL test program					*/
/*									*/
/* Created: 	07/06/95 (PRM)						*/
/*									*/
/* Description:	Tests PARASOL functions with invalid parameters		*/
/*									*/
/************************************************************************/

#include <parasol.h>
#include "test.h"

#define NTASKS 5

long 	nids[NTASKS];
long	tids[NTASKS];
long	bid;

void ps_genesis (void * arg)
{
	long	i, temp, pid, count, buf[NTASKS], spid, psid, lid, lid2;
	long	type, ack_port;
	double	ts, mean, other;
	char	*text;
	void	task(void *);

/* Build some nodes and tasks so we can use their ids in calls to other	*/
/* functions.								*/

	for (i = 0; i < NTASKS; i++) {
		if ((nids[i] = ps_build_node("", 1, 1.0, 0.0, CFS, FALSE))
		    == SYSERR)
			ERRABORT("ps_build_node failed");
		char name[10] = "t19-x";
		name[4] = i + '0';
		if ((tids[i] = ps_create_group(name, nids[i], ANY_HOST, task, 1,0))
		    == SYSERR)
			ERRABORT("ps_create failed");
		if (ps_resume(tids[i]) == SYSERR)
			ERRABORT("ps_resume failed");
	}
	if ((pid = ps_allocate_port("", ps_myself)) == SYSERR)
		ERRABORT("ps_allocate_port failed");
	if ((spid = ps_allocate_shared_port("")) == SYSERR)
		ERRABORT("ps_allocate_shared_port failed");
	if ((psid = ps_allocate_port_set("")) == SYSERR)
		ERRABORT("ps_allocate_port_set failed");
	if ((bid = ps_build_bus("", NTASKS, nids, 100, FCFS, FALSE)) == SYSERR)
		ERRABORT("ps_build_bus failed");
	if ((lid = ps_build_link("", 0, nids[0], 100, FALSE)) == SYSERR)
		ERRABORT("ps_build_link failed");
	if ((lid2 = ps_build_link("", nids[0], nids[1], 100, FALSE)) == SYSERR)
		ERRABORT("ps_build_link failed");

/* ps_build_node							*/

	if (ps_build_node("", 0, 1.0, 0.0, CFS, FALSE) != SYSERR)
		ERRREPORT("ps_build_node ncpus = 0 succeeded");
	if (ps_build_node("", 1, 0.0, 0.0, CFS, FALSE) != SYSERR)
		ERRREPORT("ps_build_node speed = 0.0 succeeded");
	if (ps_build_node("", 1, 1.0, -1.0, CFS, FALSE) != SYSERR)
		ERRREPORT("ps_build_node quantum = 1.0 succeeded");
	if (ps_build_node("", 1, 1.0, 0.0, 22000, FALSE) != SYSERR)
		ERRREPORT("ps_build_node discipline = 22000 succeeded");

/* ps_build_bus								*/

	if (ps_build_bus("", 0, nids, 100, FCFS, FALSE) != SYSERR)
		ERRREPORT("ps_build_bus nnodes = 0 succeeded");
	temp = nids[0];
	nids[0] = -1;
	if (ps_build_bus("", NTASKS, nids, 100, FCFS, FALSE) != SYSERR)
		ERRREPORT("ps_build_bus with invalid node_ids succeeded");
	nids[0] = temp;
	if (ps_build_bus("", NTASKS, nids, 0, FCFS, FALSE) != SYSERR)
		ERRREPORT("ps_build_bus rate = 0 succeeded");
	if (ps_build_bus("", NTASKS, nids, 100, 22000, FALSE) != SYSERR)
		ERRREPORT("ps_build_bus discipline = 22000 succeeded");

/* ps_build_link							*/

	if (ps_build_link("", -1, nids[1], 100, FALSE) != SYSERR)
		ERRREPORT("ps_build_link source = -1 succeeded");
	if (ps_build_link("", nids[0], -1, 100, FALSE) != SYSERR)
		ERRREPORT("ps_build_link destination = -1 succeeded");
	if (ps_build_link("", nids[0], nids[1], 0, FALSE) != SYSERR)
		ERRREPORT("ps_build_link rate = 0 succeeded");

/* ps_idle_cpu								*/

	if (ps_idle_cpu(-1) != SYSERR)
		ERRREPORT("ps_idle_cpu node = -1 succeeded");

/* ps_ready_queue							*/
	
	if (ps_ready_queue(-1,0,NULL) != SYSERR)
		ERRREPORT("ps_ready_queue node = -1 succeeded");

/* ps_create								*/

	if (ps_create_group("t19-6", -1, ANY_HOST, task, 1,0) != SYSERR)
		ERRREPORT("ps_create node = -1 succeeded");
	if (ps_create_group("t19-7", nids[0], 2, task, 1,0) != SYSERR)
		ERRREPORT("ps_create cpu = 2 succeeded");
	if (ps_create_group("t19-8", nids[2], ANY_HOST, task, 1,0) != SYSERR)
		ERRREPORT("ps_create cpu = 2 succeeded");

/* ps_create2								*/

	if (ps_create2("t19-9", -1, ANY_HOST, task, 1, 100000,0) != SYSERR)
		ERRREPORT("ps_create node = -1 succeeded");
	if (ps_create2("t19-10", nids[0], 2, task, 1, 100000,0) != SYSERR)
		ERRREPORT("ps_create cpu = 2 succeeded");
	if (ps_create2("t19-11", nids[0], ANY_HOST, task, 1, 0,0) != SYSERR)
		ERRREPORT("ps_create2 stacksize = 0 succeeded");

	if (ps_create_group("t19-12", nids[2], ANY_HOST, task, 1,9) != SYSERR)
		ERRREPORT("ps_create group = 9 succeeded");
	if (ps_create_group("t19-13", nids[2], ANY_HOST, task, 1,9) != SYSERR)
		ERRREPORT("ps_create group = -1 succeeded");

/* ps_kill								*/

/*	if (ps_kill(-1) != SYSERR)
		ERRREPORT("ps_kill task = -1 succeeded");

/* ps_resume								*/

/*	if (ps_resume(-1) != SYSERR)
		ERRREPORT("ps_resume task = -1 succeeded");

/* ps_suspend								*/

/*	if (ps_suspend(-1) != SYSERR)
		ERRREPORT("ps_suspend task = -1 succeeded");

/* ps_sleep								*/

/*	if (ps_sleep(-1.0) != SYSERR)
		ERRREPORT("ps_sleep duration = -1.0 succeeded");	*/

/* ps_awaken								*/

/*	if (ps_awaken(-1) != SYSERR)
		ERRREPORT("ps_awaken task = -1 succeeded");
		
/* ps_compute								*/

/*	if (ps_compute(-1.0) != SYSERR)
		ERRREPORT("ps_compute duration = -1.0 succeeded");
*/


	ps_sleep(5.0);

	printf ("\nAll tests completed!!\n");

	ps_suspend(ps_myself);
 }



void task(void * arg)
{
	static long 	pid;
	long		psid;

	if (ps_myself == tids[0]) {

/* ps_kill								*/

		if (ps_kill (tids[1]) != SYSERR)
			ERRREPORT("ps_kill killed a sibling");

 /* ps_migrate								*/

		if (ps_migrate(tids[1], nids[0], 0) != SYSERR)
			ERRREPORT("ps_migrate migrated a sibling");

/* ps_adjust_priority							*/

		if (ps_adjust_priority(tids[1], 10) != SYSERR)
			ERRREPORT("ps_adjust_priority adjusted a siblings priority");

/* ps_pass_port								*/

		ps_sleep(0.1);
		if (ps_pass_port(pid, ps_myself) != SYSERR)
			ERRREPORT("ps_pass_port passed a sibling's port");

/* ps_join_port_set							*/

		if ((psid = ps_allocate_port_set("")) == SYSERR)
			ERRABORT("ps_allocate_port_set failed");
		if (ps_join_port_set (psid, pid) != SYSERR)
			ERRREPORT("ps_join_port_set took a sibling's port");

/* ps_bus_send								*/
		
		if (ps_bus_send(bid, -1, 0, 100, "", NULL_PORT) != SYSERR)
			ERRREPORT("ps_bus_send port = -1 succeeded");
		if (ps_bus_send(bid, ps_std_port(tids[1]), 0, -1, "", NULL_PORT)
		    != SYSERR)
			ERRREPORT("ps_bus_send size = -1 succeeded");
		printf("Subtests completed\n");
	}

	if (ps_myself == tids[1]) {
		if ((pid = ps_allocate_port("", ps_myself)) == SYSERR)
			ERRABORT("ps_allocate_port failed");
	}
	ps_suspend(ps_myself);
}
