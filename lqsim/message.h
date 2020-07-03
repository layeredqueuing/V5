/************************************************************************/
/* Copyright the Real-Time and Distributed Systems Group,		*/
/* Department of Systems and Computer Engineering,			*/
/* Carleton University, Ottawa, Ontario, Canada. K1S 5B6		*/
/* 									*/
/* May 1996.								*/
/************************************************************************/

/*
 * Global vars for simulation.
 *
 * $Id: message.h 13353 2018-06-25 20:27:13Z greg $
 */

#ifndef MESSAGE_H
#define MESSAGE_H

class Activity;
class Entry;
class Task;
class tar_t;

class Message {
public:
    Message( const Entry * e=NULL, tar_t  * tp=NULL ) { init( e, tp ); }
    Message * init( const Entry * e, tar_t * tp );
    
    Activity * activity;		/* Activity to run.		*/
    double time_stamp;			/* Time stamp of message send.	*/
    const Entry * client;		/* Pointer to sending entry.    */
    int reply_port;			/* Place to send reply.		*/
    const Entry * intermediate;		/* Pointer to intermediate entry*/
    tar_t * target;			/* Index of appropriate stat.	*/
};

#endif
