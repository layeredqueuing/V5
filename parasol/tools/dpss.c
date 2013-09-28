/* @(#)dpss.c	1.3 08:09:31 8/29/95 */
/************************************************************************/
/*	dpss.c - Slave program for Distributed Parasol Simulation 	*/
/*		 Manager source file.					*/
/*									*/
/*	Copyright (C) 1995 School of Computer Science, 			*/
/*		Carleton University, Ottawa, Ont., Canada		*/
/*		Written by Patrick Morin				*/
/*		Based on ps_slave.c by Robert Davison.			*/
/*									*/
/*  This program is free software; you can redistribute it and/or modify*/
/*  it under the terms of the GNU General Public License as published by*/
/*  the Free Software Foundation; either version 2, or (at your option)	*/
/*  any later version.							*/
/*									*/
/*  This program is distributed in the hope that it will be useful,	*/
/*  but WITHOUT ANY WARRANTY; without even the implied warranty of	*/
/*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the	*/
/*  GNU General Public License for more details.			*/
/*									*/
/*  You should have received a copy of the GNU General Public License	*/
/*  along with this program; if not, write to the Free Software		*/
/*  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.		*/
/*									*/
/*  The author may be reached at morin@scs.carleton.ca or through	*/
/*  the School of Computer Science at Carleton University.		*/
/*									*/
/*	Created: 11/08/95 (PRM)						*/
/*									*/
/************************************************************************/
#include <stdio.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/resource.h>
#include <signal.h>
#include "pvm3.h"
#include "dpsm.h"

/* The priority level at which the simulation is to be run.		*/
#define SIM_PRIORITY 4

/* The child processes have 30 seconds to get up and running.		*/
#define STARTUPTIME 30

/* The following macro is called whenever a PVM function returns an	*/
/* error code which it shouldn't have.  For a small performance boost 	*/
/* and reduction in program text size you could define it as {}		*/
#define DPSS_ABORT() { pvm_perror(__FUNCTION__); pvm_exit(); exit(1); }

void	accept_request(void);
void	control_simulation(int job, int childpid, char *input, int pipes[2][2]);
void	exec_simulation(char *command_line[], int pipes[2][2]);
int	isidle(int pid);

int	my_tid;					/* my task id		*/
int	master_tid;				/* master task id	*/

/************************************************************************/

int main(void)

/* The dpss program entry point, simply sits in an infinite loop 	*/
/* accepting requests from the master.  When this task is no longer 	*/
/* needed the master will kill it.					*/

{
 	if ((my_tid = pvm_mytid()) < 0)
		DPSS_ABORT();
	if ((master_tid = pvm_parent()) < 0)
		DPSS_ABORT();
 
	while(TRUE)
		accept_request();
}

/************************************************************************/

void accept_request(void)

/* Accepts a request to execute a simulation from the master task and	*/
/* satisfies that request.						*/

{
	int	pipes[2][2];
 	int	pid;
	int	job;
 	char	*command_line[4];
	char	s1[120], s2[20], s3[20];
	char	input[MAXINPUT];
	int 	bid;
	int	bytes, type, src;

	if ((bid = pvm_recv(master_tid, -1)) < 0)
		DPSS_ABORT();
 	if(pvm_bufinfo(bid, &bytes, &type, &src) < 0)
		DPSS_ABORT();

	switch (type) {
	case TERMINATE:
		pvm_exit();
		exit(0);
		break;

	case ABORT:
		break;

	case REQUEST:
 		command_line[3] = (char*)0;
		if (pvm_upkint(&job, 1, 1) < 0)
			DPSS_ABORT();
		sprintf(command_line[0] = s1, "%s/bin/%s/", getenv("PVM_ROOT"),
		    getenv("PVM_ARCH"));
		if (pvm_upkstr(&s1[strlen(s1)]) < 0)
			DPSS_ABORT();
		if (pvm_upkstr(command_line[1] = s2) < 0)
			DPSS_ABORT();
 		if (pvm_upkstr(command_line[2] = s3) < 0)
			DPSS_ABORT();
 		if (pvm_upkstr(input) < 0)
			DPSS_ABORT();
	
		pipe(pipes[0]);
		pipe(pipes[1]);

		pid = fork();
		if (pid > 0) {
			control_simulation(job, pid, input, pipes);
			while( wait((int*)0) != pid )
				if( wait((int*)0) == -1 ) break;
		}
		else
			exec_simulation(command_line, pipes);
		break;

	default:
		fprintf(stderr, "Unknown Message Type\n");
		pvm_exit();
		exit(1);
		break;
	}
}

/************************************************************************/

void control_simulation(

/* Executed by the parent after the fork() call.  Sends information to	*/
/* child's stdin and reads information from the child's stdout sending	*/
/* it to the master task line-by-line.  This function can return in one	*/
/* of 3 ways: (1) The child process runs to completion, (2) The child	*/
/* process hangs, (3) The master sends an ABORT message.		*/

	int	job,				/* master job id	*/
	int	childpid,			/* child process id	*/
	char 	*input,				/* stdin for child	*/ 
	int 	pipes[2][2]			/* redirectors		*/
)
{
	char	line[MAXLINE];			/* current line		*/
	int	i, n;				/* index & count	*/
	fd_set	fds;				/* file descriptor set	*/
	struct	timeval	tv;			/* time value		*/
	int	width;				/* size of fd table	*/
	int	temp;				/* temporary storage	*/
	int	bid;				/* buffer id		*/
	int	timer;				/* timing device	*/

	width = getdtablesize();
  	/* Fix up the pipes and send the input to the child via 	*/
	/* pipes[0].							*/
	close(pipes[0][0]);
	close(pipes[1][1]);
	write(pipes[0][1], input, strlen(input));

	/* Send the output to the master one line at a time		*/
	do {
		i = 0; 
		do {
			/* This bit of code makes sure we don't block	*/
			/* too long on the pipe, so that we can listen	*/
			/* for ABORT messages from the master.	At the	*/
			/* same time we're trying to make sure that the	*/
			/* child doesn't hang.				*/
			timer = 0;
 			do {
				if(++timer > STARTUPTIME) {
					timer = 0;
				    	if(isidle(childpid)) {
						close(pipes[0][1]);
						close(pipes[1][0]);
						kill(childpid, SIGTERM);
 						if(pvm_initsend(PvmDataDefault) 
						    < 0)
							DPSS_ABORT();
						if(pvm_pkint(&job, 1, 1) < 0)
							DPSS_ABORT();
						if(pvm_send(master_tid, HUNG) 
						    < 0)
							DPSS_ABORT();
						return;
					}
				}
				if((bid = pvm_nrecv(master_tid, ABORT)) < 0)
					DPSS_ABORT();
				if(bid > 0) {
					if (pvm_upkint(&temp, 1, 1) < 0)
						DPSS_ABORT();
					if (temp == job) {
						close(pipes[0][1]);
						close(pipes[1][0]);
						kill(childpid, SIGTERM);
						return;
					}
				}
				tv.tv_sec = 1;
				tv.tv_usec = 0;
				FD_ZERO(&fds);
				FD_SET(pipes[1][0], &fds);
			} while(select(width, &fds, NULL, NULL, &tv) == 0);
			n = read(pipes[1][0], &line[i], 1);
 		} while(n > 0 && line[i++] != '\n' && i < MAXLINE);
		line[i-1] = '\0';

		if(pvm_initsend(PvmDataDefault) < 0)
			DPSS_ABORT();
		if(pvm_pkint(&job, 1, 1) < 0)
			DPSS_ABORT();
		if(pvm_pkstr(line) < 0)
			DPSS_ABORT(); 
		if(pvm_send(master_tid, OUTPUT) < 0)
			DPSS_ABORT();
 	} while(n > 0);

	/* Notify master that simulation is complete. No more results.	*/
	if(pvm_initsend(PvmDataDefault) < 0)
		DPSS_ABORT();
	if(pvm_pkint(&job, 1, 1) < 0)
		DPSS_ABORT();
	if(pvm_send(master_tid, ENDOUTPUT) < 0)
		DPSS_ABORT();

	close(pipes[0][1]);
	close(pipes[1][0]);
}

/************************************************************************/

void exec_simulation(
	
/* Executed by the child after the fork() call.  Redirects stdin and	*/
/* stdout then exec's the PARASOL simulation program.			*/

	char 	*command_line[], 		/* command_line		*/
	int 	pipes[2][2]			/* redirectors		*/
)
{
	/* Redirect stdout and stdin through the pipes			*/
	close(0);
	dup(pipes[0][0]);
 	close(1);
 	dup(pipes[1][1]);

 	close(pipes[0][0]);
	close(pipes[1][1]);
	close(pipes[0][1]);
	close(pipes[1][0]);

	/* 'nice' ourselves						*/
 	setpriority(PRIO_PROCESS, getpid(), SIM_PRIORITY); 
 
	/* Fire up the simulation					*/
 	execv(command_line[0], command_line);
	fprintf(stderr,"%s\n", command_line[0]);
 	perror("exec_simulation");
	exit(1);
}

/************************************************************************/

int isidle(

/* Returns true if the process has been sleeping for more than about	*/
/* 20 seconds.  See ps(1) for more details.  This is done by executing	*/
/* 'ps' and putting the results into a file.  There must be a nicer	*/
/* way to do this.							*/

	int	pid
)
{
	FILE	*fp;
	char	command[50];
	char	filename[20];
	char	line[200];
	char	terminal[10];
	char	status[10];
	char	*temp;
	int	tpid;

	/* Execute 'ps' command.					*/
	sprintf(filename, "/tmp/ps.%d", getpid());
	sprintf(command, "ps %d > %s", pid, filename);
	system(command);

	/* Read results.						*/
	if((fp = fopen(filename, "r")) == NULL)
		return(FALSE);
	fgets(line, sizeof(line), fp);
	temp = fgets(line, sizeof(line), fp);
	fclose(fp);

	/* Is the process (I)dle?					*/
	sprintf(command, "rm -f %s", filename);	
	if(temp == NULL)
		return(FALSE);
	if(sscanf(line, "%d %s %s", &tpid, terminal, status) != 3)
		return(FALSE);
	if(status[0] == 'I')
		return(TRUE);

	return(FALSE);
}

