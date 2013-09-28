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
 * $Id$
 */

#include <iostream>
#include <iomanip>
#include <sstream>
#include <parasol.h>
#include <assert.h>
#include <lqio/input.h>
#include <lqio/error.h>
#include "lqsim.h"
#include "entry.h"
#include "message.h"
#include "task.h"
#include "instance.h"


/*
 * This function tags a message with the appropriate fields.
 */

Message * 
Message::init( const Entry * ep, tar_t * src )
{
    time_stamp   = ps_now; /* Tag send time.	*/
    client       = ep;
    reply_port   = -1;
    intermediate = 0;
    source       = src;
    next         = 0;

    return this;
}


Message * 
Message::alloc( const Entry * ep, tar_t * src )
{
    Entry * dst = src->entry;
    Task * cp = dst->task();
    Message * msg = cp->free_messages;

    if ( msg ) {
	cp->free_messages = msg->next;
	msg->init( ep, src );
    }
    return msg;
}

void
Message::free( Message * msg )
{
    tar_t * tp = msg->source;
    Entry * dst = tp->entry;
    Task * cp = dst->task();
    double delta = ps_now - msg->time_stamp;

    assert ( msg->reply_port == -1 );
    ps_record_stat( tp->r_delay.raw, delta );
    ps_record_stat( tp->r_delay_sqr.raw, square( delta ) );

    msg->next = cp->free_messages;
    cp->free_messages = msg;
}
