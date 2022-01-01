/* -*- c++ -*- */

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
 * $Id: message.h 15317 2022-01-01 16:44:56Z greg $
 */

#ifndef MESSAGE_H
#define MESSAGE_H

class Activity;
class Entry;
class Task;
class tar_t;

class Message {
public:
    Message( const Entry * e=nullptr, tar_t  * tp=nullptr ) { init( e, tp ); }
    Message * init( const Entry * e, tar_t * tp );
    
    Activity * activity;		/* Activity to run.		*/
    double time_stamp;			/* Time stamp of message send.	*/
    const Entry * client;		/* Pointer to sending entry.    */
    int reply_port;			/* Place to send reply.		*/
    const Entry * intermediate;		/* Pointer to intermediate entry*/
    tar_t * target;			/* Index of appropriate stat.	*/
};

#endif
