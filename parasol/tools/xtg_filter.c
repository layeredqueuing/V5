/* @(#)xtg_filter.c	1.2 16:21:26 8/23/95 */
/************************************************************************/
/*	xtg_filter.c - PARASOL trace to XTG event file filter program.	*/
/*									*/
/*	Copyright (C) 1995 School of Computer Science, 			*/
/*		Carleton University, Ottawa, Ont., Canada		*/
/*		Written by Patrick Morin				*/
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
/*	Created: 01/06/95 (PRM)						*/
/*									*/
/************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/*
 * Constants and macro definitions 
 */
enum { TASK, SEMAPHORE, LOCK, SPECIAL }; /* MLog object types */
  
/*
 * Structures and type definitions
 */
typedef struct tag_msg_def {
  char *template;          /* Message template */
  char *eid;               /* Event ID */
  int type;                /* Object type */
  void *handler;           /* Handler function */
} msg_def;

typedef int (*t_handler) (msg_def *mdp, const char *text, char *od);
typedef char task_name[40];

/*
 * Forward function declarations 
 */
int parse_command_line(int argc, char *argv[], FILE **in, FILE **out, 
		       int *discrete_flag);
char *strip_header (char *line, double *ts, int *nid, int *tid, char **tname);
msg_def *get_msg_def (const char *text);
int match (const char *template, const char *line);
void construct_event (char *event, const char *eid, double ts, const char *oid,
		      const char *od);
char *tid2oid (int tid);
char *sid2oid (int sid);
char *lid2oid (int lid);

int t_default (msg_def *mdp, const char *text, char *od);
int t_dummy (msg_def *mdp, const char *text, char *od);
int t_error (msg_def *mdp, const char *text, char *od);
int t_created (msg_def *mdp, const char *text, char *od);
int t_sending (msg_def *mdp, const char *text, char *od);
int t_receives (msg_def *mdp, const char *text, char *od);
int t_blocked_on_sema (msg_def *mdp, const char *text, char *od);
int t_spinning (msg_def *mdp, const char *text, char *od);

char *get_task_name(int tid);
void set_task_name(int tid, char *name);

/*
 * The following is a table of trace messages, their event identifiers and 
 * their handling functions. Messages handled by t_dummy are simply ignored
 * and create no output in the output file. Messages handled by t_default
 * don't create an <other-data> portion in the <event-entry>. See the XTG
 * User's Guide Section 4.2 for more details on the event file format. 
 */
msg_def messages[] = {
  { "removes port %d from port set %d", "TASK_REMOVING_PORT", TASK, t_dummy },
  { "inserts port %d in port set %d", "TASK_INSERTING_PORT", TASK, t_dummy },
  { "priority adjusted to %d", "TASK_PRIORITY_ADJUSTED", TASK, t_default },
  { "created (suspended)", "TASK_CREATED", TASK, t_created },
  { "dead", "TASK_DEAD", TASK, t_default },
  { "migrating to node %d", "TASK_MIGRATING", TASK, t_default },
  { "sleeping", "TASK_SLEEPING", TASK, t_default },
  { "suspended", "TASK_SUSPENDED", TASK, t_default },
  { "sending message %d to task %d via port %d and bus %d", "TASK_SENDING", TASK, t_sending },
  { "sending message %d to shared port %d via bus %d", "TASK_SENDING", TASK, t_sending },
  { "sending message %d to task %d via port %d and link %d", "TASK_SENDING", TASK, t_sending },
  { "sending message %d to shared port %d via link %d", "TASK_SENDING", TASK, t_sending },
  { "sending message %d to shared port %d", "TASK_SENDING", TASK, t_sending },
  { "passes port %d to task %d", "TASK_PASSING_PORT", TASK, t_default },
  { "sending message %d to task %d via port %d", "TASK_SENDING", TASK, t_sending },
  { "spinning on lock %d", "TASK_SPINNING", SPECIAL, t_spinning },
  { "locking lock %d", "TASK_LOCKING", SPECIAL, t_default },
  { "resetting semaphore %d to value %d", "SEMAPHORE_RESETTING", SEMAPHORE, NULL },
  { "signalling semaphore %d", "SEMAPHORE_SIGNALLING", SEMAPHORE, NULL },
  { "unlocking lock %d", "SPINLOCK_TASK_UNLOCKING", LOCK, t_default },
  { "waiting on semaphore %d", "SEMAPHORE_TASK_WAITING", SEMAPHORE, NULL },
  { "blocked on semaphore %d", "TASK_BLOCKED_ON_SEMAPHORE", TASK, t_blocked_on_sema },
  { "suspended", "TASK_SUSPENDED", TASK, t_default },
  { "executing", "TASK_EXECUTING", TASK, t_default },
  { "ready", "TASK_READY", TASK, t_default },
  { "times out on port %d", "TASK_TIMED_OUT", TASK, t_default },
  { "receives message %d on port %d", "TASK_RECEIVES", TASK, t_receives },
  { "receives message %d on shared port %d", "TASK_RECEIVES", TASK, t_receives },
  { "receiving (blocked) on port %d", "TASK_BLOCKED_ON_PORT", TASK, t_default },
  { "", "TASK_UNKNOWN_TRACE_MESSAGE", TASK, t_error }  
};

int nnames = 0;
task_name *names = NULL;

/*
 * Program entry point 
 */
int main (int argc, char *argv[])
{
  FILE *in;                       /* Input file */
  FILE *out;                      /* Output file */
  static char linein[128];        /* Input line */
  static char lineout[128];       /* Output line */
  static char od[80];             /* <other-data> */
  char *tname;                    /* Task name */
  double ts;                      /* Time stamp */
  int nid, tid, sid, lid;         /* Task and node ids */
  char *text;                     /* Trace message body */
  int result;                     /* Handler result */
  msg_def *mdp;                   /* Message info ptr */
  int value;                      /* Semaphore value */
  int time = 0;                   /* Discrete time */
  int discrete_flag;              /* Do we use discrete time? */

  if (!parse_command_line(argc, argv, &in, &out, &discrete_flag))
    return 1;
 
  while (!feof(in)) {
    fgets(linein, sizeof(linein), in);
    if (!memcmp (linein, "Time: ", 6)) {
      *strrchr(linein, '.') = '\0';
      text = strip_header (linein, &ts, &nid, &tid, &tname);
      mdp = get_msg_def(text);
      if (discrete_flag)
	ts = time++;
      switch (mdp->type) {
      case TASK:
	if ((result = ((t_handler)mdp->handler)(mdp, text, od))) {
	  if (result == 3)
	    set_task_name (tid, tname);
	  construct_event(lineout, mdp->eid, ts, tid2oid(tid), od);
	  fprintf(out,"%s\n",lineout);
	  if (result == 2) {
	    construct_event(lineout, "TASK_DUMMY", ts, tid2oid(tid), "");
	    fprintf(out, "%s\n", lineout);
	  }
	} 
	break;

      case SEMAPHORE:
	if (sscanf (text, mdp->template, &sid, &value) == 2)
	    sprintf (od, "value %d", value);
	else
	  od[0] = '\0';
	sprintf (&od[strlen(od)], " tid %s%d", get_task_name(tid), tid);
	construct_event(lineout, mdp->eid, ts, sid2oid(sid), od);
	fprintf (out,"%s\n", lineout);
	construct_event(lineout, "TASK_DUMMY", ts, tid2oid(tid), "");
	fprintf (out,"%s\n", lineout);
	break;

      case LOCK:
	sscanf (text, mdp->template, &lid);
	sprintf (od, "tid %s%d", get_task_name(tid), tid);
	construct_event(lineout, mdp->eid, ts, lid2oid(lid), od);
	fprintf (out,"%s\n", lineout);
	construct_event(lineout, "TASK_DUMMY", ts, tid2oid(tid), "");
	fprintf (out, "%s\n", lineout);
	break;

      case SPECIAL:
	sscanf (text, mdp->template, &lid);
	sprintf (od, "tid %s%d", get_task_name(tid), tid);
	construct_event(lineout, mdp->eid, ts, lid2oid(lid), od);
	fprintf (out, "%s\n", lineout);
	sprintf (od, "lid %d", lid);
	construct_event(lineout, mdp->eid, ts, tid2oid(tid), od);
	fprintf (out, "%s\n", lineout);
	construct_event(lineout, "TASK_DUMMY", ts, tid2oid(tid), "");
	fprintf (out, "%s\n", lineout);
	break;

      default:
	fprintf (stderr, "\nInvalid line\n");
	break;
      }
    }
  }
  return 0;
}

/*
 * parses the command line and returns:
 * in     - The input file
 * out    - The output file
 * discrete_flag - A flag indicating whether or not to use discrete time
 */
int parse_command_line(int argc, char *argv[], FILE **in, FILE **out, 
		       int *discrete_flag)
{
  int i, error = 0;

  *in = stdin;
  *out = stdout;
  *discrete_flag = 0;

  error = argc > 3;
  for (i = 1; !error && i < argc; i++) {
    if (argv[i][0] == '-') { 
      if (argv[i][1] == 'd')
	*discrete_flag = 1;
      else
	error = 1;
    }
    else {
      if (!(*in = fopen(argv[i], "r"))) {
	fprintf (stderr, "Unable to open input file %s\n", argv[i]);
	error = 1;
      }
    }
  }
  if (error)
    fprintf (stderr, "Usage: %s [-d] [infile]\n", argv[0]);
  return !error;
}
      
/*
 * Extracts the time stamp, node id, task id and task name from the header of
 * a PARASOL trace message and returns a pointer to the rest of the message
 */
char *strip_header (char *line, double *ts, int *nid, int *tid, char **tname)
{
  int i;
  char *temp, *temp2;

  sscanf (line, "Time: %lG; Node: %d; Task %d", ts, nid, tid);
  for (i = 0, temp = line; i < 6; i++)
    temp = strchr(temp, ' ') + 1;
  while (*temp == ' ') temp++;
  if (*temp == '(') {
    *(temp2 = strchr (temp, ')')) = '\0';
    *tname = temp + 1;
    temp = temp2 + 2;
    for (temp2 = *tname; *temp2; temp2++)
      if (isspace(*temp2))
        *temp2 = '_';
  }
  else 
    *tname = "";
  return temp;
}

/*
 * Uses the messages table to find the function which should be used to filter
 * the given line. 
 */
msg_def *get_msg_def (const char *text)
{
  msg_def *mdp;

  for (mdp = messages; strlen (mdp->template); mdp++) 
    if (match (mdp->template, text))
      break;

  return mdp;
}

/*
 * Matches an input line to a template. Returns true if the match was 
 * successful, otherwise returns false.
 * Note: This will have problems if a task name contains a ')' character.
 */
int match (const char *template, const char *text) 
{
  int li, ti;

  for (li = ti = 0; template[ti] && text[li]; li++, ti++) {
    if (template[ti] == '%' && template[ti+1] == 'd') {
      ti++;
      if (!isdigit(text[li])) 
	break;
      while (isdigit(text[li+1])) 
	li++;
    }
    else if (template[ti] != text[li]) 
      break;
  }
  return template[ti] == '\0' && text[li] == '\0';
}

/*
 * The default trace message handler, by default messages have no <other-data>
 * (od) section.
 */
int t_default (msg_def *mdp, const char *text, char *od)
{
  od[0] = '\0';
  return 1;
}

/*
 * The dummy trace message handler, return 0 indicating that this message 
 * shouldn't produce an event in the event file.
 */
int t_dummy (msg_def *mdp, const char *text, char *od)
{
  return 0;
}

/*
 * The error trace message handler, Print a warning on stderr saying that an
 * unidentified trace message was encountered.  Return 0 indicating that this
 * message should produce an event in the event file. 
 */
int t_error (msg_def *mdp, const char *text, char *od)
{
  fprintf (stderr, "\nWarning: Unrecognized trace message \"%s\"\n", text);
  return 0;
}

/*
 * Handles trace messages of the form:
 * "created"
 */
int t_created (msg_def *mdp, const char *text, char *od)
{
  od[0] = '\0';
  return 3;
}

/*
 * Handles trace messages of the form:
 * sending message #d to task x via port y [and link z]|[and bus z]
 */
int t_sending (msg_def *mdp, const char *text, char *od)
{
  int receiver, port, link, mid;

  sscanf (text, mdp->template, &mid, &receiver, &port, &link);
  sprintf (od, "mid %d", mid);
  return 1;
}

/*
 * Handles PARASOL trace messages of the form:
 * "receives message #%d on port %d"
 */
int t_receives (msg_def *mdp, const char *text, char *od)
{
  int mid, dummy1, dummy2, dummy3;  

  sscanf (text, mdp->template, &mid, &dummy1, &dummy2, &dummy3);
  sprintf (od, "mid %d", mid);
  return 2;
}

/*
 * Handles all PARASOL trace messages dealing with semaphores
 */
int t_blocked_on_sema (msg_def *mdp, const char *text, char *od)
{
  int sid;

  sscanf (text,mdp->template, &sid);
  sprintf (od, "sid %d", sid);
  return 1;
}

/*
 * Handles PARASOL trace messages of the form:
 * "spinning on lock %d"
 */
int t_spinning (msg_def *mdp, const char *text, char *od)
{
  int lid;

  sscanf (text,mdp->template,&lid);
  sprintf (od, "lid %d", lid);
  return 1;
}

/*
 * Constructs an event string as described in the XTG User's guide. If we 
 * make changes to the time format they will be localized in this function.
 */
void construct_event (char *event, const char *eid, double ts, const char *oid,
  		      const char *od)
{
  sprintf (event, "%s %d %d %s %s", eid, (int)ts, (int)((ts - (int) ts)*1.0e6),
	   oid, od);
}

/* 
 * Converts a thread id to an object id string. 
 * Warning: The string returned by tid2oid() gets overwritten by the next
 *          call to tid2oid().
 */
char *tid2oid (int tid)
{
  static char oid[40];

  sprintf (oid, "TASK %s_%d \"\"", get_task_name(tid), tid);
  return oid;
}

/*
 * Converts a semaphore id to an object id string
 * Warning: See above
 */
char *sid2oid (int sid)
{
  static char oid[40];

  sprintf (oid, "SEMAPHORE %d \"\"", sid);
  return oid;
} 

/*
 * Converts a spinlock id to an object id string
 * Warning: See above
 */
char *lid2oid (int lid)
{
  static char oid[40];

  sprintf (oid, "SPINLOCK %d \"\"", lid);
  return oid;
}

/*
 * Returns the name of the specified task in the task table 
 */
char *get_task_name(int tid) 
{
  if (tid >= nnames) {
    fprintf(stderr, "\nTask referenced before it was created, aborting\n");
    exit(1);
  }
  return names[tid];
}

/*
 * Sets the name of the specified task to the given name in the task name
 * table
 */
void set_task_name(int tid, char *name)
{
  task_name *temp;

  if (tid >= nnames) {
    temp = malloc (sizeof(task_name)*(tid+1)*2);
    if (!temp) {
      fprintf(stderr, "\nMemory allocation error, aborting\n");
      exit(1);
    }
    if (nnames) {
      memcpy (temp, names, sizeof(task_name)*nnames);
      free (names);
    }
    names = temp;
    nnames = (tid+1)*2;
  }
  strncpy(names[tid], name, sizeof(task_name));
}
