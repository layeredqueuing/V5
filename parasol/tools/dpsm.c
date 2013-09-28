/* @(#)dpsm.c	1.4 17:07:53 8/30/95 */
/************************************************************************/
/*	dpsm.c - Master program for Distributed Parasol Simulation 	*/
/*		 Manager source file.					*/
/*									*/
/*	Copyright (C) 1995 School of Computer Science, 			*/
/*		Carleton University, Ottawa, Ont., Canada		*/
/*		Written by Patrick Morin				*/
/*		Based on ps_dist.c by Robert Davison.			*/
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
#include <stdlib.h>
#include <errno.h>
#include <math.h>
#include <string.h>
#include <signal.h>
#include <ctype.h>
#include "pvm3.h"
#include "dpsm.h"

/************************************************************************/
/*	Constants, macros and enumerations				*/
/************************************************************************/

#define MAXSTATNAME 50				/* max statistic name	*/
#define MAXSTATS 10				/* max # of requests	*/
#define MAXINPUTS 20				/* max # of inputs	*/
#define DEFAULT_ESTIMATE_REPS 5			/* estimate on reps	*/
#define SLAVENAME "dpss"			/* name of slave prog.	*/

/* The following macro is called whenever a PVM function returns an	*/
/* error code which it shouldn't have.  For a small performance boost 	*/
/* and reduction in program text size you could define it as {}		*/
#define DPSM_ABORT() { pvm_perror(__FUNCTION__); pvm_exit(); exit(1); }

enum {						/* slave states		*/
	IDLE = -20,				/* idle			*/
	FREE, 					/* unused		*/
	BROKEN 					/* node is down		*/
};

/************************************************************************/
/*	Type definitions and structures					*/
/************************************************************************/

typedef struct {
	int		confidence;		/* Confidence level	*/
	double		accuracy;		/* desired accuracy	*/
	double		interval;		/* interval width	*/
	char		name[MAXSTATNAME];	/* statistic name	*/
	double		sum;			/* sum			*/
	double		sum_sq;			/* sum of squares	*/
	int		nreps;			/* number of repetitions*/
} request_t;					/* request type		*/

typedef struct {
	char		host[50];		/* host id		*/
	char		symbol[10];		/* legend symbol	*/
	int		tid;			/* slave task id	*/
	int		state;			/* BUSY/IDLE/batch #	*/
	double		results[MAXSTATS];	/* statistic values	*/
	int		checked[MAXSTATS];	/* TRUE/FALSE		*/
} slave_t;					/* slave type		*/

typedef struct {
	char		prog_name[MAXPROGNAME];	/* program name		*/
	long		rnd_seed;		/* rnd number seed	*/
	double		duration;		/* simulation duration	*/
	char		input[MAXINPUT];	/* program input	*/
	int		nrequests;		/* number of requests	*/
	request_t	requests[MAXSTATS];	/* requests		*/
	int		nslaves;		/* slaves allocated	*/
	int		est_reps_left;		/* estimated reps to do	*/
} job_t;					/* job type		*/

/************************************************************************/
/*	Forward function declarations					*/
/************************************************************************/

void	setup_network(void);
int	find_job(void);
void	assign_slave_to_job(int job, int slave);
int	finished(void);
int	any_slaves(void);
int	lookup_slave(int slave_tid);
void	handle_output_message(int slave);
void	handle_endoutput_message(int slave);
void	handle_hung_message(int slave);
void	cleanup_job(int job);
void	report_results(int job);
void	print_legend(void);
void	pad_string(char *dest, char *src, int length);
int	estimate_reps_left(request_t *rp);
double	calc_t(int confidence, int size);
void	read_input_file (char *filename);
int	read_model_name(FILE *fp, char *model);
void	read_requests(FILE *fp, int *nrequests, request_t *requests);
void	read_experiments(FILE *fp, int *nexperiments,
	    char parameters[][MAXINPUT], double *durations);
int	get_input_line(FILE *fp, char *line);
void	sigint_isr(void);


/************************************************************************/
/*	Global Variables						*/
/************************************************************************/

int	my_tid;					/* my task id		*/
int	njobs;					/* number of jobs	*/
job_t	*jobs;					/* jobs			*/
int	nslaves;				/* number of slaves	*/
slave_t	*slaves;				/* slaves		*/

/************************************************************************/
/*	Function Definitions						*/
/************************************************************************/

int	main(

/* The dpsm program entry point. 					*/

	int	argc,				/* argument count	*/
	char 	*argv[]				/* argument values	*/
)
{
	int	i;				/* loop index		*/
	int	bytes;				/* message size		*/
	int	type;				/* message type		*/
	int	src;				/* message source	*/
	int	bid;				/* receive buffer id	*/
	int	job;				/* job number		*/
	int	sid;				/* slave number		*/

	if (argc != 2) {
		fprintf(stderr, "\nUsage: %s <simfile>\n", argv[0]);
		exit(1);
	}

	read_input_file(argv[1]);
 	setup_network();
	print_legend();
	signal(SIGINT, sigint_isr);
	for(i = 0; i < nslaves; i++) 
		assign_slave_to_job(find_job(), i);

	while(!finished()) {
		if(!any_slaves()) {
			fprintf(stderr, "\nALL slaves have failed, aborting\n");
			pvm_exit();
			exit(1);
		}
		if((bid = pvm_recv(-1, -1)) < 0)
			DPSM_ABORT();
		if(pvm_bufinfo(bid, &bytes, &type, &src) < 0)
			DPSM_ABORT();
		if(pvm_upkint(&job, 1, 1) < 0) 
			DPSM_ABORT();
		if ((sid = lookup_slave(src)) >= 0 
		   && slaves[sid].state == job) {
			switch(type) {
			case OUTPUT:
 				handle_output_message(sid);
				break;

			case ENDOUTPUT:
				handle_endoutput_message(sid);
				break;

			case HUNG:
				handle_hung_message(sid);
				break;

			default:
				fprintf(stderr, "\nInvalid Message Type\n");
				pvm_exit();
				exit(1);
				break;
			}
		}
 	}
	pvm_exit();
	return 0;
}

/************************************************************************/

void	setup_network(void)

/* Analyzes the pvm configuration and allocates an appropriate number	*/
/* of slaves to each machine.						*/

{
	int			nhosts;		/* number of hosts	*/
	int			narch;		/* number of arches	*/
	struct pvmhostinfo	*hostp;		/* host ptr.		*/
	int			i, j;		/* loop indices		*/
	int			temp;		/* working storage	*/
	char			c1, c2;		/* symbol components	*/
	slave_t			*sp;

	if ((my_tid = pvm_mytid()) < 0)
		DPSM_ABORT();

	if (pvm_config(&nhosts, &narch, &hostp) < 0) 
		DPSM_ABORT();

	for (i = 0, nslaves = 0; i < nhosts; i++) {
		temp = (temp = hostp[i].hi_speed / 1000) ? temp : 1;
		nslaves += temp;
	}
	if (!(slaves = (slave_t*)malloc(nslaves * sizeof(slave_t)))) {
		perror("setup_network");
		pvm_exit();
		exit(1);
	}
	sp = slaves;
	for (i = 0; i < nhosts; i++) {
		temp = (temp = hostp[i].hi_speed / 1000) ? temp : 1;
		for (j = 0; j < temp; j++) {
			sp->state = FREE;
			strcpy(sp->host, hostp[i].hi_name);
			sp++;
		}
	}
	/* Assign symbols of the form XY to all the slaves, so that	*/
	/* we can show where the results are coming from when they 	*/
	/* arrive.							*/
	for (i = 0, c1 = 'A', c2 = 'A'; i < nslaves; i++) {
		sprintf(slaves[i].symbol, "%c%c", c2, c1);
		if(c1++ == 'Z') {
			c1 = 'A';
			if(c2++ == 'Z')
				c2 = 'A';
		}
	}
}

/************************************************************************/

int	find_job(void)

/* Looks for a job in need of processing.  Look for a job such that the	*/
/* estimated number of repetitions needed is greater than the number	*/
/* of slaves currently servicing that job. If we can't find one then 	*/
/* just pick the job most in need of processing.			*/

{
	int	i, max, maxi;

	for (i = 0; i < njobs; i++) 
		if (jobs[i].est_reps_left > jobs[i].nslaves)
			return i;

	maxi = -1;
	for (i = 0; i < njobs; i++) {
		if ((jobs[i].est_reps_left > 0) 
		    && (maxi == -1 
		    || jobs[i].est_reps_left - jobs[i].nslaves > max)) {
			maxi = i;
			max = jobs[i].est_reps_left - jobs[i].nslaves;
		}
	}
	return maxi;				/* nothing to do!	*/
}

/************************************************************************/

void	assign_slave_to_job(

/* Assigns the specified slave to the specified job.			*/

	int	job,				/* job number		*/
	int	slave				/* slave number		*/
)
{
	slave_t		*sp;			/* slave ptr.		*/
	job_t		*jp;			/* job ptr.		*/
	int		i;			/* index variable	*/
	char		string[20];		/* temp string		*/

	sp = &slaves[slave];
	jp = &jobs[job];

	if (sp->state != FREE && sp->state != IDLE) {
		fprintf(stderr, "\nAttempting to assign a busy slave\n");
		pvm_exit();
		exit(1);
	}
	if (sp->state == FREE) {
		sp->state = job;
		if (pvm_spawn(SLAVENAME, NULL, PvmTaskHost, sp->host, 1,
		    &sp->tid) != 1) {
			sp->state = BROKEN;
			return;
		}
	}
 	sp->state = job;
	for (i = 0; i < jp->nrequests; i++)
		sp->checked[i] = FALSE;

	if (pvm_initsend(PvmDataDefault) < 0)
		DPSM_ABORT();

 	/* Make a request message of the form:				*/
	/* | job# | prog_name | random_seed | duration | input |	*/
 	if(pvm_pkint(&job, 1, 1) < 0)
		DPSM_ABORT()
	if(pvm_pkstr(jp->prog_name) < 0)
		DPSM_ABORT();
	sprintf(string, "%ld", jp->rnd_seed++);
	if(pvm_pkstr(string) < 0)
		DPSM_ABORT();
	sprintf(string, "%g", jp->duration);
 	if(pvm_pkstr(string) < 0)
		DPSM_ABORT();
	if(pvm_pkstr(jp->input) < 0)
		DPSM_ABORT();

	if(pvm_send(sp->tid, REQUEST) < 0) {
		sp->state = BROKEN;
		return;
	}
 
	jp->nslaves++;
}

/************************************************************************/

int	finished(void)

/* Returns true if all the confidence interval goals have been 		*/
/* satisfied.								*/

{
	int	i;

	for (i = 0; i < njobs; i++)
		if (jobs[i].est_reps_left > 0)
			return FALSE;

	return TRUE;
}

/************************************************************************/

int	any_slaves(void)

/* Returns TRUE if there are ANY slaves left which are not in the 	*/
/* BROKEN state, otherwise returns FALSE.				*/

{
	int	i;
	
	for (i = 0; i < nslaves; i++)
		if (slaves[i].state != BROKEN)
			return TRUE;

	return FALSE;
}

/************************************************************************/

int	lookup_slave(

/* Returns the slave number given a slave's tid, or -1 if the tid does	*/
/* not correspond to a slave.						*/

	int	slave_tid			/* tid of slave		*/
)
{
	int	i;

	for (i = 0; i < nslaves; i++) 
		if (slaves[i].tid == slave_tid)
			return i;

	return -1;
}

/************************************************************************/

void	handle_output_message(

/* Handles output messages.  Checks if the message contains any statis-	*/
/* tics values, and if so uses it. Otherwise discards the message.	*/

	int	slave				/* message origin	*/	
)
{
	job_t		*jp;
	slave_t		*sp;
	int		i;
	char		dummy[20];
	char		line[MAXLINE];

	sp = &slaves[slave];
	jp = &jobs[sp->state];
	if(pvm_upkstr(line) < 0)
		DPSM_ABORT();
	
	for (i = 0; i < jp->nrequests; i++) {
		if (!strncasecmp(line, jp->requests[i].name,
		    strlen(jp->requests[i].name))) {
			if (sp->checked[i]) {
				/* This is a problem!!!			*/
			}
			sscanf(line+39, "%s %lf", dummy, &sp->results[i]);
			sp->checked[i] = TRUE;
			break;
		}
	}
}


/************************************************************************/

void	handle_endoutput_message(

/* Handles ENDOUTPUT messages.  Includes the results obtained by the	*/
/* slave in the CI calculations for the job the slave was working on.	*/
/* also allocates some more work to the slave.				*/

	int	slave				/* slave number		*/
)
{
	slave_t		*sp;
	job_t		*jp;
	request_t	*rp;
	double		std_dev;
	int		job;
	int		i, temp;
	int		max_estimate;

	sp = &slaves[slave];
	job = sp->state;
	jp = &jobs[job];

	printf("[%s-%d] ", sp->symbol, job+1);
	fflush(stdout);

	/* Make sure we have results for all the requested statistics.	*/
	for (i = 0; i < jp->nrequests; i++) {
		if(!sp->checked[i]) {
			fprintf(stderr, "\nNon-existent statistic: %s, aborting experiment #%d\n",
			   jp->requests[i].name, job);
			jp->est_reps_left = 0;
		}
	}
	if (jp->est_reps_left == 0) {
		cleanup_job(job);
		return;
	}

	/* Include the results for this run in CI calculations and 	*/
	/* estimate the number of reps needed to achieve the requested	*/
	/* accuracy.							*/
	max_estimate = 0;
	for (i = 0; i < jp->nrequests; i++) {
		rp = &jp->requests[i];
 		rp->sum += sp->results[i];
		rp->sum_sq += sp->results[i] * sp->results[i];
		if (++rp->nreps >= 5) {
			std_dev = sqrt((rp->sum_sq - rp->sum * rp->sum /
			    rp->nreps) / (rp->nreps * (rp->nreps - 1)));
			rp->interval = std_dev * calc_t(rp->confidence,
			    rp->nreps);
			temp = estimate_reps_left(rp);
		}
		else
			temp = 5;
		max_estimate = max_estimate < temp ? temp : max_estimate;
	}
	jp->est_reps_left = max_estimate;
 	jp->nslaves--;

	/* Assign the slave another job.				*/
	if ((temp = find_job()) >= 0) {
		sp->state = IDLE;
		assign_slave_to_job(temp, slave);
	}

	/* Check if this job is complete, if so reassign all the slaves	*/
	/* working on it to other jobs.					*/
	if (jp->est_reps_left == 0) {
		cleanup_job(job);
		report_results(job);
	}
}

/************************************************************************/

void	handle_hung_message(

/* Handles a HUNG message by aborting the job that is hung.  HUNG 	*/
/* messages are sent by slaves when the simulation becomes blocked for	*/
/* one reason or another.						*/

	int	slave
)
{
	slave_t		*sp;
	job_t		*jp;
	int		job;

	sp = &slaves[slave];
	job = sp->state;
	jp = &jobs[job];

	fprintf(stderr, "\nExperiment #%d blocked, is input correct?\n",
	    job + 1);
	jp->est_reps_left = 0;
	cleanup_job(job);
}

/************************************************************************/

void	cleanup_job(

/* Cleans up a job by aborting all the simulations that are currently	*/
/* being run to support that job.  Assigns all the slaves that were	*/
/* working on the job new jobs.						*/

	int	job				/* job number		*/
)
{
	int	i, temp;

	for(i = 0; i < nslaves; i++) {
		if (slaves[i].state == job) {
			if (pvm_initsend(PvmDataDefault) < 0) 
				DPSM_ABORT();
			if (pvm_pkint(&slaves[i].state, 1, 1) < 0)
				DPSM_ABORT();
			if (pvm_send(slaves[i].tid, ABORT) < 0)
				slaves[i].state = BROKEN;
			else {
				slaves[i].state = IDLE;
				if ((temp = find_job()) >= 0)
					assign_slave_to_job(temp, i);
				else {
					if (pvm_initsend(PvmDataDefault) < 0)
						DPSM_ABORT();
					pvm_send(slaves[i].tid, TERMINATE);
					slaves[i].state = FREE;
				}
			}
		}
	}
	jobs[job].nslaves = 0;
}

/************************************************************************/

void	report_results(

/* Reports the results obtained for the specified job.			*/

	int	job				/* job number		*/
)
{
	int		i;
	job_t		*jp;
	request_t	*rp;
	double		mean;
	char		name[40];

	jp = &jobs[job];
	printf("\n**********************************************************");
	printf("********************");
	printf("\nExperiment#: %d\nModel: %s\nDuration: %g\nInput:\n%s", job+1,
	    jp->prog_name, jp->duration, jp->input);
	printf("Number of replications performed: %d", jp->requests[0].nreps);
	printf("\nName\t\t\t\tMean\t\tConfidence Interval");
	for (i = 0; i < jp->nrequests; i++) {
		rp = &jp->requests[i];
		pad_string(name, rp->name, 31);
		mean = rp->sum / rp->nreps;
		if (mean) {
			printf("\n%s %f    %f @%d%% = %3.1f%% accuracy",
			    name, mean, rp->interval, rp->confidence,
			    rp->interval*100/mean);
		}
		else {
			printf("\n%s %f    %f @%d%% = %3.1f%% accuracy",
			    name, mean, rp->interval, rp->confidence, 0.0);
		}
	}
	printf("\n**********************************************************");
	printf("********************\n");
}

/************************************************************************/

void print_legend(void)

/* Prints the legend associating slaves with symbols.			*/

{
	int	i;				/* loop index		*/

	printf("\n**********************************************************");
	printf("********************");

	printf("\nLegend\n-------");
	for(i = 0; i < nslaves; i++)
		printf("\n[%s] - %s", slaves[i].symbol, slaves[i].host);

	printf("\n**********************************************************");
	printf("********************\n");
}

/************************************************************************/

void	pad_string(

/* Pads the src string with blanks to the length specified by length.	*/
/* The padded string is stored in dest.					*/

	char	*dest, 				/* destination string	*/
	char	*src, 				/* source string	*/
	int	length				/* length to pad to	*/
)
{
	static char     blanks[] = "                                      ";

	strncpy(dest, src, length);
	dest[length] = '\0';
	length -= strlen(src);
	if (length > 0)
		strncat(dest, blanks, length);
}

/************************************************************************/

int	estimate_reps_left(

/* Returns an estimate of the number of repetitions needed to achieve	*/
/* the confidence interval accuracy of the given request not including	*/
/* repetitions that have already been performed.			*/

	request_t 	*rp			/* request ptr.		*/
)
{
	if (rp->sum == 0.0) 
		return 0;

	return ceil(rp->nreps * pow(rp->interval/(rp->sum/rp->nreps)
	    /rp->accuracy, 2.0)) - rp->nreps; 
}
	
/************************************************************************/

double	calc_t(

/* Returns the student-t value for hte given confidence level and 	*/
/* sample size.  confidence must be one of 90, 95, or 99.		*/

	int	confidence,			/* confidence level	*/
	int	size				/* sample size		*/
)
{
	int             column;
	int             row;
	double          t_value;

	static double   t_table[3][34] = {	
	{	6.314, 2.920, 2.353, 2.132, 2.015, 1.943, 1.895, 1.860,
		1.833, 1.812, 1.796, 1.782, 1.771, 1.761, 1.753, 1.746,
		1.740, 1.734, 1.729, 1.725, 1.721, 1.717, 1.714, 1.711,
		1.708, 1.706, 1.703, 1.701, 1.699, 1.697, 1.684, 1.671,
		1.658, 1.645},
	{	12.706, 4.303, 3.182, 2.776, 2.571, 2.447, 2.365, 2.306,
		2.262, 2.228, 2.201, 2.179, 2.160, 2.145, 2.131, 2.120,
		2.110, 2.101, 2.093, 2.086, 2.080, 2.074, 2.069, 2.064,
		2.060, 2.056, 2.052, 2.048, 2.045, 2.042, 2.021, 2.000,
		1.980, 1.960},
	{	63.657, 9.925, 5.841, 4.604, 4.032, 3.707, 3.499, 3.355,
		3.250, 3.169, 3.106, 3.055, 3.012, 2.977, 2.947, 2.921,
		2.898, 2.878, 2.861, 2.845, 2.831, 2.819, 2.807, 2.797,
		2.787, 2.779, 2.771, 2.763, 2.756, 2.750, 2.704, 2.660,
		2.617, 2.576} };

	if (confidence == 90)
		row = 0;
	else if (confidence == 95)
		row = 1;
	else
		row = 2;

	column = size - 2;
	if (column < 30)
		t_value = t_table[row][column];
	else if (column < 40)
		t_value = t_table[row][29] + (t_table[row][30] - t_table[row][29])
			* (column - 29) / 10.0;
	else if (column < 60)
		t_value = t_table[row][30] + (t_table[row][31] - t_table[row][30])
			* (column - 39) / 20.0;
	else if (column < 120)
		t_value = t_table[row][31] + (t_table[row][32] - t_table[row][31])
			* (column - 59) / 60.0;
	else
		t_value = t_table[row][33];

	return (t_value);
}

/************************************************************************/

void	read_input_file (

/* Reads an input file and fills out the jobs and njobs global variables*/
/* accordingly.								*/

	char	*filename			/* input file name	*/
)
{
	FILE		*fp;
	job_t		*jp, *tjobs;
	char		model[MAXPROGNAME];
	int		nrequests;
	request_t	requests[MAXSTATS];
	int		nexperiments;
	char		parameters[MAXINPUTS][MAXINPUT];
	double		durations[MAXINPUTS];
	int		i;

	if (!(fp = fopen(filename, "r"))) {
		perror(filename);
		exit(1);
	}
	while (read_model_name(fp, model)) {
		read_requests(fp, &nrequests, requests);

		read_experiments(fp, &nexperiments, parameters, durations);

		tjobs = (job_t*)malloc(sizeof(job_t)*(njobs+nexperiments));
		if(njobs) {
			memcpy(tjobs, jobs, sizeof(job_t)*njobs);
			free(jobs);
 		}
		for (i = 0; i < nexperiments; i++) {
			jp = &tjobs[i+njobs];
			strcpy(jp->prog_name, model);
			jp->rnd_seed = 1;
			jp->duration = durations[i];
			strcpy(jp->input, parameters[i]);
			jp->nrequests = nrequests;
			memcpy(jp->requests, requests, 
			    sizeof(request_t)*nrequests);
			jp->nslaves = 0;
			jp->est_reps_left = DEFAULT_ESTIMATE_REPS;
 		}
		njobs += nexperiments;
		jobs = tjobs;
	}
	if(njobs == 0) {
		fprintf(stderr, "\nThere must be at least 1 experiment\n");
		exit(1);
	}
	fclose(fp);	
}

/************************************************************************/

int	read_model_name(

/* Read a model name from the open input file fp.  Model names specs.	*/
/* are of the form 'MODEL: <model_name>'.  Returns 1 on success of 0 on	*/
/* EOF									*/

	FILE	*fp, 				/* input file ptr	*/
	char	*model				/* model name		*/
)
{
	char line[80];
	char dummy[40];

	if(get_input_line(fp, line) == 0)
		return 0;

	if (sscanf(line, "%s %s", dummy, model) != 2 
	    || strcmp(dummy, "MODEL:")) {
		fprintf(stderr, "\nModel specification expected: %s\n", line);
		exit(1);
	}
	return 1;
}

/************************************************************************/

void	read_requests(

/* Reads the requests section of the open input file fp.  See docs for	*/
/* the format of this section.						*/

	FILE		*fp,			/* input file ptr.	*/ 
	int		*nrequests,		/* number of requests	*/
	request_t	*requests		/* requests		*/
)
{
	char		line[80];
	char		*name, *temp;
	request_t	*rp;
	int		confidence;
	double		accuracy;

 	if (get_input_line(fp, line) == 0) {
		fprintf(stderr, "\nUnexpected end of input file\n");
		exit(1);
	}
	if (strncmp(line, "STATISTICS", strlen("STATISTICS"))) {
		fprintf(stderr, "\n\"STATISTCS:\" expected: %s\n", line);
		exit(1);
	}

	*nrequests = 0;
	while (TRUE) {
		rp = &requests[*nrequests];
	 	if (get_input_line(fp, line) == 0) {
			fprintf(stderr, "\nUnexpected end of input file\n");
			exit(1);
		}
		name = strchr(line, '\"');
		if (name == NULL) {
			if (!strncmp(line, "ENDSTATISTICS",
			    strlen("ENDSTATISTICS")))
				break;
			else {
				fprintf(stderr, "\nInvalid Line: %s\n", line);
				exit(1);
			}
		}
		name++;
		temp = strchr(name, '\"');
		if (temp == NULL) {
			fprintf(stderr, "\nMismatched Quote: %s\n", line);
			exit(1);
		}
		*temp++ = '\0';
		if (sscanf(temp, "%d%lf", &confidence, &accuracy) != 2) {
			fprintf(stderr, "\nInvalid STATISTICS entry: %s", line);
			exit(1);
		}
		if (confidence != 90 && confidence != 95 && confidence != 99) {
			fprintf(stderr, 
			    "\nValid confidence levels are 90, 95 & 99: %s\n",
			    line);
			exit(1);
		}
		if (accuracy < 0.0) {
			fprintf(stderr, "\nNegative accuracy not allowed: %s\n",
			    line);
			exit(1);
		}
		strcpy (rp->name, name);
		rp->confidence = confidence;
		rp->accuracy = accuracy/100.0;
		rp->sum = rp->sum_sq = 0.0;
		rp->nreps = 0;
		++*nrequests;
	}
	if (*nrequests == 0) {
		fprintf(stderr, 
		    "\nSTATISTICS section must have at least 1 entry\n");
		exit(1);
	}
}

/************************************************************************/

void	read_experiments(

/* Read an experiments section of the open input file fp.  See the docs	*/
/* for the format of this section.					*/

	FILE	*fp,				/* input file ptr.	*/
	int	*nexperiments,			/* number of inputs	*/
	char	parameters[][MAXINPUT],		/* simualtion params.	*/
	double	*durations			/* simulation durations	*/
)
{
	char		line[80];
	char		dummy[40];
	double		duration;
  
	if(get_input_line(fp, line) == 0) {
		fprintf(stderr, "\nUnexpected end of input file\n");
		exit(1);
	}
	if (strncmp(line, "EXPERIMENTS", strlen("EXPERIMENTS"))) {
		fprintf(stderr, "\n\"EXPERIMENTS\" expected: %s\n", line);
		exit(1);
	}
	if(get_input_line(fp, line) == 0) {
		fprintf(stderr, "\nUnexpected end of input file\n");
		exit(1);
	}
 	if (sscanf(line, "%s %lf", dummy, &duration) != 2 
	    || strcmp(dummy, "DURATION:")) {
		fprintf(stderr, "\nDuration expected: %s\n", line);
		exit(1);
	}
	*nexperiments = 0;
	while(TRUE) {
	 	if (get_input_line(fp, line) == 0) {
			fprintf(stderr, "\nUnexpected end of input file\n");
			exit(1);
		}
		if (!strncmp(line, "ENDEXPERIMENTS", strlen("ENDEXPERIMENTS")))
			break;
		if (!strncmp(line, "DURATION:", strlen("DURATION:"))) {
			if(sscanf(line, "DURATION: %lf", &duration) != 1) {
				fprintf(stderr, 
				    "\nInvalid duration specification\n");
				exit(1);
			}
			continue;
		}
		if (strncmp(line, "PARAMETERS", strlen("PARAMETERS"))) {
			fprintf(stderr, "\n\"PARAMETERS\" expected: %s\n", 
			    line);
			exit(1);
		}

		parameters[*nexperiments][0] = '\0';
		durations[*nexperiments] = duration;
		/* FIXME: This has O(n^2) running time!! 		*/
		while(TRUE) {
	 		if(fgets(line, sizeof(line), fp) == NULL) {
				fprintf(stderr, 
				    "\nUnexpected end of input file\n");
				exit(1);
			}
			if (strstr(line, "ENDPARAMETERS")) 
 				break;
			strcat(parameters[*nexperiments], line);
		}
		++*nexperiments;
	}
	if (*nexperiments == 0) {
		fprintf(stderr, 
		    "\nEXPERIMENTS section must have at least 1 entry\n");
		exit(1);
	}
}

/************************************************************************/

int	get_input_line(

/* Reads a line from the input file fp, ignoring any lines that contain	*/
/* only whitespace.  It also removes anything following the # character	*/
/* and any leading whitespace.						*/

	FILE	*fp,				/* input file ptr.	*/
	char	*line				/* input line		*/
)
{
	int	i, j;				/* loop indices		*/

	while(TRUE) {
		if (fgets(line, 80, fp) == NULL)
			return 0;
		for(i = 0; line[i]; i++)
			if (line[i] == '#') 
				line[i] = line[i+1] = '\0';
		for(i = 0; isspace(line[i]); i++);
		if(i != 0) {
 			for(j = 0; line[i+j]; j++)
				line[j] = line[i+j];
			line[j] = '\0';
		}
		if(line[0] != '\0')
			return 1;
	}
}

/************************************************************************/

void	sigint_isr(void)

/* The SIGINT interrupt service routine.  This is installed so that if	*/
/* the user aborts the program with ^C we can cleanup the slaves.	*/

{
	int	i;

	fprintf(stderr, "\nCleaning Up...");
	for (i = 0; i < nslaves; i++) {
		if (slaves[i].state >= 0) {
			pvm_initsend(PvmDataDefault);
			pvm_pkint(&slaves[i].state, 1, 1);
			pvm_send(slaves[i].tid, ABORT);
		}
		if (slaves[i].state != FREE && slaves[i].state != BROKEN) {
			pvm_initsend(PvmDataDefault);
			pvm_send(slaves[i].tid, TERMINATE);
		}
	}
	fprintf(stderr, "\nAll experiments aborted\n");
	pvm_exit();
	exit(1);
}

/************************************************************************/


