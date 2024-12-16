/* -*- c++ -*-
 * Messages sent between tasks.
 *
 * Copyright the Real-Time and Distributed Systems Group,
 * Department of Systems and Computer Engineering,
 * Carleton University, Ottawa, Ontario, Canada. K1S 5B6
 *
 * May 1996.
 * November 2024
 *
 * ------------------------------------------------------------------------
 * $Id: message.h 17499 2024-11-27 14:14:11Z greg $
 * ------------------------------------------------------------------------
 */

#ifndef LQSIM_MESSAGE_H
#define LQSIM_MESSAGE_H

class Activity;
class Entry;
class Call;

class Message {
public:
    Message( const Entry * e=nullptr, Call * tp=nullptr );
    Message * init( const Entry * ep, Call * src );
    
public:
    const Entry * client;		/* Pointer to sending entry.    */
    Call * target;			/* Index of appropriate stat.	*/
    Activity * activity;		/* Activity to run.		*/
    double time_stamp;			/* Time stamp of message send.	*/
    int reply_port;			/* Place to send reply.		*/
    const Entry * intermediate;		/* Pointer to intermediate entry*/
};
#endif
