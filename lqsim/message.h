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
 * $Id: message.h 11378 2013-04-21 03:44:25Z greg $
 */

#ifndef MESSAGE_H
#define MESSAGE_H

class Activity;
class Entry;
class Task;
class tar_t;

class Message {
public:
    static Message * alloc( const Entry * ep, tar_t * tp );
    static void free( Message * msg );

    Message( const Entry * e=0, tar_t  * tp=0 ) { init( e, tp ); }
    Message * init( const Entry * e, tar_t * tp );
    
    Activity * activity;		/* Activity to run.		*/
    double time_stamp;			/* Time stamp of message send.	*/
    const Entry * client;		/* Pointer to sending entry.    */
    int reply_port;			/* Place to send reply.		*/
    const Entry * intermediate;		/* Pointer to intermediate entry*/
    tar_t * source;			/* Index of appropriate stat.	*/
    Message * next;			/* Pointer to next message.	*/

private:
};

#endif
