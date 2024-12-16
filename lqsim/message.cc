/************************************************************************/
/* Copyright the Real-Time and Distributed Systems Group,		*/
/* Department of Systems and Computer Engineering,			*/
/* Carleton University, Ottawa, Ontario, Canada. K1S 5B6		*/
/* 									*/
/* May 1996								*/
/* June 2009								*/
/************************************************************************/

/*
 * Input output processing.
 *
 * $Id: message.cc 17505 2024-12-03 23:16:26Z greg $
 */

#include "message.h"
#include "processor.h"


Message::Message( const Entry * e, Call * tp )
    : client(e), target(tp), activity(nullptr), time_stamp(Processor::now()), reply_port(-1), intermediate(nullptr)
{
}

/*
 * This function tags a message with the appropriate fields.
 */

Message * 
Message::init( const Entry * ep, Call * destination )
{
    time_stamp   = Processor::now(); /* Tag send time.	*/
    client       = ep;
    reply_port   = -1;
    intermediate = nullptr;
    target       = destination;

    return this;
}
