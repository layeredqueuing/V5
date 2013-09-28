/* $Id */
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
		if ((nids[i] = ps_build_node("", 1, 1.0, 0.0, FCFS, FALSE))
		    == SYSERR)
			ERRABORT("ps_build_node failed");
		if ((tids[i] = ps_create("", nids[i], ANY_HOST, task, 1))
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

	if (ps_build_node("", 0, 1.0, 0.0, FCFS, FALSE) != SYSERR)
		ERRREPORT("ps_build_node ncpus = 0 succeeded");
	if (ps_build_node("", 1, 0.0, 0.0, FCFS, FALSE) != SYSERR)
		ERRREPORT("ps_build_node speed = 0.0 succeeded");
	if (ps_build_node("", 1, 1.0, -1.0, FCFS, FALSE) != SYSERR)
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

	if (ps_create("", -1, ANY_HOST, task, 1) != SYSERR)
		ERRREPORT("ps_create node = -1 succeeded");
	if (ps_create("", nids[0], 2, task, 1) != SYSERR)
		ERRREPORT("ps_create cpu = 2 succeeded");

/* ps_create2								*/

	if (ps_create2("", -1, ANY_HOST, task, 1, 100000, 0 ) != SYSERR)
		ERRREPORT("ps_create node = -1 succeeded");
	if (ps_create2("", nids[0], 2, task, 1, 100000, 0 ) != SYSERR)
		ERRREPORT("ps_create cpu = 2 succeeded");
	if (ps_create2("", nids[0], ANY_HOST, task, 1, 0, 0 ) != SYSERR)
		ERRREPORT("ps_create2 stacksize = 0 succeeded");

/* ps_kill								*/

	if (ps_kill(-1) != SYSERR)
		ERRREPORT("ps_kill task = -1 succeeded");

/* ps_resume								*/

	if (ps_resume(-1) != SYSERR)
		ERRREPORT("ps_resume task = -1 succeeded");

/* ps_suspend								*/

	if (ps_suspend(-1) != SYSERR)
		ERRREPORT("ps_suspend task = -1 succeeded");

/* ps_sleep								*/

/*	if (ps_sleep(-1.0) != SYSERR)
		ERRREPORT("ps_sleep duration = -1.0 succeeded");	*/

/* ps_awaken								*/

	if (ps_awaken(-1) != SYSERR)
		ERRREPORT("ps_awaken task = -1 succeeded");
		
/* ps_compute								*/

	if (ps_compute(-1.0) != SYSERR)
		ERRREPORT("ps_compute duration = -1.0 succeeded");

/* ps_sync								*/

	if (ps_sync(-1.0) != SYSERR)
		ERRREPORT("ps_sync duration = -1.0 succeeded");

/* ps_hold								*/

	if (ps_hold(-1.0) != SYSERR)
		ERRREPORT("ps_hold duration = -1.0 succeeded");

/* ps_migrate								*/
	
	if (ps_migrate(-1, nids[0], 0) != SYSERR)
		ERRREPORT("ps_migrate task = -1 succeeded");
	if (ps_migrate(tids[1], -1, 0) != SYSERR)
		ERRREPORT("ps_migrate node = -1 succeeded");
	if (ps_migrate(tids[1], nids[0], 2) != SYSERR)
		ERRREPORT("ps_migrate cpu = 2 succeeded");

/* ps_adjust_priority							*/

	if (ps_adjust_priority(-1, 10) != SYSERR)
		ERRREPORT("ps_adjust_priority task = -1 succeeded");

/* ps_buses								*/

	if (ps_buses(-1, &count, buf) != SYSERR)
		ERRREPORT("ps_buses task = -1 succeeded");

/* ps_send_links							*/
	
	if (ps_send_links(-1, &count, buf) != SYSERR)
		ERRREPORT("ps_send_links task = -1 succeeded");

/* ps_receive_links							*/
	
	if (ps_receive_links(-1, &count, buf) != SYSERR)
		ERRREPORT("ps_receive_links task = -1 succeeded");

/* ps_allocate_port							*/

	if (ps_allocate_port("", -1) != SYSERR)
		ERRREPORT("ps_allocate_port task = -1 succeeded");

/* ps_release_port							*/

	if (ps_release_port(-1) != SYSERR)
		ERRREPORT("ps_release_port port = -1 succeeded");
	if (ps_release_port(ps_my_std_port) != SYSERR)
		ERRREPORT("ps_release_port port = ps_my_std_port succeeded");

/* ps_release_shared_port						*/

	if (ps_release_shared_port(-1) != SYSERR)
		ERRREPORT("ps_release_shared_port port = -1 succeeded");
	if (ps_release_shared_port(pid) != SYSERR)
		ERRREPORT("ps_release_shared_port invalid pid succeeded");

/* ps_pass_port 							*/

	if (ps_pass_port(-1, tids[0]) != SYSERR)
		ERRREPORT("ps_pass_port port = -1 succeeded");
	if (ps_pass_port(spid, tids[0]) != SYSERR)
		ERRREPORT("ps_pass_port port = spid succeeded");
	if (ps_pass_port(psid, tids[0]) != SYSERR)
		ERRREPORT("ps_pass_port port = psid succeeded");
	if (ps_pass_port(ps_my_std_port, tids[0]) != SYSERR)
		ERRREPORT("ps_pass_port port = ps_my_std_port succeeded");
	if (ps_pass_port(pid, -1) != SYSERR) 
		ERRREPORT("ps_pass_port task = -1 succeeded");

/* ps_join_port_set							*/

	if (ps_join_port_set(-1, pid) != SYSERR)
		ERRREPORT("ps_join_port_set port_set = -1 succeeded");
	if (ps_join_port_set(pid, pid) != SYSERR)
		ERRREPORT("ps_join_port_set port_set = pid succeeded");
	if (ps_join_port_set(psid, -1) != SYSERR)
		ERRREPORT("ps_join_port_set port_set = -1 succeeded");
	if (ps_join_port_set(psid, ps_my_std_port) != SYSERR)
		ERRREPORT("ps_join_port_set port = ps_my_std_port succeeded");
	if (ps_join_port_set(psid, psid) != SYSERR)
		ERRREPORT("ps_join_port_set port = psid succeeded");
	if (ps_join_port_set(psid, spid) != SYSERR)
		ERRREPORT("ps_join_port_set port = spid succeeded");

/* ps_leave_port_set							*/
	
 	if (ps_leave_port_set(-1, pid) != SYSERR)
		ERRREPORT("ps_leave_port_set port_set = -1 succeeded");
	if (ps_leave_port_set(psid, -1) != SYSERR)
		ERRREPORT("ps_leave_port_set port = -1 succeeded");
	if (ps_leave_port_set(psid, pid) != SYSERR)
		ERRREPORT("ps_leave_port_set port is not member succeeded");

/* ps_send								*/

	if (ps_send(-1, 0, "", NULL_PORT) != SYSERR)
		ERRREPORT("ps_send port = -1 succeeded");

/* ps_resend								*/

	if (ps_resend(-1, 0, 0.0, "", NULL_PORT) != SYSERR)
		ERRREPORT("ps_resend port = -1 succeeded");

/* ps_bus_send								*/

	if (ps_bus_send(-1, ps_std_port(tids[0]), 0, 100, "", NULL_PORT) 
	    != SYSERR)
		ERRREPORT("ps_bus_send bus = -1 succeeded");
	if (ps_bus_send(bid, ps_std_port(tids[0]), 0, 100, "", NULL_PORT) 
	    != SYSERR)
		ERRREPORT("ps_bus_send bus is not connected succeeded");

/* ps_link_send								*/

	if (ps_link_send(-1, ps_std_port(tids[0]), 0, 100, "", NULL_PORT) 
	    != SYSERR)
		ERRREPORT("ps_link_send link = -1 succeeded");
	if (ps_link_send(lid2, ps_std_port(tids[1]), 0, 100, "", NULL_PORT) 
	    != SYSERR)
		ERRREPORT("ps_link_send link is not from me succeeded");
	if (ps_link_send(lid, -1, 0, 100, "", NULL_PORT) != SYSERR)
		ERRREPORT("ps_link_send port = -1 succeeded");
	if (ps_link_send(lid, ps_std_port(tids[1]), 0, 100, "", NULL_PORT) 
	    != SYSERR)
		ERRREPORT("ps_link_send link is not to him succeeded");
	if (ps_link_send(lid, ps_std_port(tids[0]), 0, -1, "", NULL_PORT)
	    != SYSERR)
		ERRREPORT("ps_link_send size = -1 succeeded");

/* ps_receive								*/

	if (ps_receive(-1, NEVER, &type, &ts, &text, &ack_port) != SYSERR)
		ERRREPORT("ps_receive port = -1 succeeded");
	if (ps_receive(ps_std_port(tids[0]), NEVER, &type, &ts, &text, 
	    &ack_port) != SYSERR)
		ERRREPORT("ps_receive port is not mine succeeded");

/* ps_receive_last							*/

	if (ps_receive_last(-1, NEVER, &type, &ts, &text, &ack_port) != SYSERR)
		ERRREPORT("ps_receive_last port = -1 succeeded");
	if (ps_receive_last(ps_std_port(tids[0]), NEVER, &type, &ts, &text, 
	    &ack_port) != SYSERR)
		ERRREPORT("ps_receive_last port is not mine succeeded");

/* ps_receive_random							*/

	if (ps_receive_random(-1, NEVER, &type, &ts, &text, &ack_port) 
	    != SYSERR)
		ERRREPORT("ps_receive_random port = -1 succeeded");
	if (ps_receive_random(ps_std_port(tids[0]), NEVER, &type, &ts, &text, 
	    &ack_port) != SYSERR)
		ERRREPORT("ps_receive_random port is not mine succeeded");

/* ps_receive_shared							*/

	if (ps_receive(-1, NEVER, &type, &ts, &text, &ack_port) != SYSERR)
		ERRREPORT("ps_receive port = -1 succeeded");
	if (ps_receive(spid, NEVER, &type, &ts, &text, 
	    &ack_port) != SYSERR)
		ERRREPORT("ps_receive port is not mine succeeded");

/* ps_build_buffer_pool							*/

	if (ps_build_buffer_pool(0) != SYSERR)
		ERRREPORT("ps_build_buffer_pool size = 0 succeeded");

/* ps_free_buffer							*/
/* This would just be begging for a segmentation fault			*/

/* ps_get_buffer							*/

	if ((int)ps_get_buffer(-1) != SYSERR)
		ERRREPORT("ps_get_buffer pool = -1 succeeded");

/* ps_buffer_size							*/
/* See ps_free_buffer							*/

/* ps_lock								*/
	
	if (ps_lock(-1) != SYSERR)
		ERRREPORT("ps_lock lock = -1 succeeded");
	
/* ps_unlock								*/

	if (ps_unlock(-1) != SYSERR)
		ERRREPORT("ps_unlock lock = -1 succeeded");

/* ps_reset_semaphore							*/

	if (ps_reset_semaphore(-1, 5) != SYSERR)
		ERRREPORT("ps_reset_semaphore semaphore = -1 succeeded");
	if (ps_reset_semaphore(0, -1) != SYSERR)
		ERRREPORT("ps_reset_semaphore value = -1 succeeded");

/* ps_wait_semaphore							*/

	if (ps_wait_semaphore(-1) != SYSERR)
		ERRREPORT("ps_wait_semaphore semaphore = -1 succeeded");
	
/* ps_signal_semaphore							*/

	if (ps_signal_semaphore(-1) != SYSERR)
		ERRREPORT("ps_signal_semaphore semaphore = -1 succeeded");

/* ps_open_stat 							*/

	if (ps_open_stat("", -392) != SYSERR)
		ERRREPORT("ps_open_stat type = -392 succeeded");
	
/* ps_reset_stat							*/

	if (ps_reset_stat(15) != SYSERR)
		ERRREPORT("ps_reset_stat stat = 15 succeeded");

/* ps_record_stat							*/

	if (ps_record_stat(15, 0.0) != SYSERR)
		ERRREPORT("ps_record_stat stat = 15 succeeeded");

/* ps_get_stat								*/

	if (ps_get_stat(15, &mean, &other) != SYSERR)
		ERRREPORT("ps_get_stat stat = 15 succeeded");

/* ps_block_stats							*/
	
	if (ps_block_stats(0, 0.0) != SYSERR)		
		ERRREPORT("ps_block_stats nblocks = 0 succeeded");
	if (ps_block_stats(5, -1.0) != SYSERR)
		ERRREPORT("ps_block_stats delay = -1.0 succeeded");
	
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
