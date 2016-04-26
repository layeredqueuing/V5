// $Id: port.cc 9042 2009-10-14 01:31:21Z greg $
//=======================================================================
//	port.cc - PS_Port, PS_SharedPort and PS_PortSet class definitions.
//
//	Copyright (C) 1995 School of Computer Science,
//		Carleton University, Ottawa, Ont., Canada
//		Written by Patrick Morin
//
//  This library is free software; you can redistribute it and/or
//  modify it under the terms of the GNU Library General Public
//  License as published by the Free Software Foundation; either
//  version 2 of the License, or (at your option) any later version.
//
//  This library is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//  Library General Public License for more details.
//
//  You should have received a copy of the GNU Library General Public
//  License along with this library; if not, write to the Free
//  Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//
//  The author may be reached at morin@scs.carleton.ca or through
//  the School of Computer Science at Carleton University.
//
//	Created: 27/06/95 (PRM)
//
//=======================================================================
#include "port.h"

#include <stdlib.h>
#include <port.h>
 
//=======================================================================
// function:	long PS_Port::~Port()
// description:	The port destructor.  Frees the port if it was allocated,
//		otherwise does nothing.
//=======================================================================
PS_Port::~PS_Port()
{
	if (allocated)
		ps_release_port(id());
}

//=======================================================================
// function:	long PS_Port::Receive(double,int*,double*,char**,int*)
// description:	Receives a message from the receiver.
//=======================================================================
SYSCALL PS_Port::Receive(double timeout, long *type, double *ts, char **text,
    long *other) const
{
	return ps_receive(id(), timeout, type, ts, text, other);
}
 
//=======================================================================
// function:	long PS_Port::ReceiveLast(double,int*,double*,char**,int*)
// description:	Receives a message from the receiver.
//=======================================================================
SYSCALL PS_Port::ReceiveLast(double timeout, long *type, double *ts, char **text,
    long *other) const
{
	return ps_receive_last(id(), timeout, type, ts, text, other);
}

//=======================================================================
// function:	long PS_Port::ReceiveRandom(double,int*,double*,char**,int*)
// description:	Receives a message from the receiver.
//=======================================================================
SYSCALL PS_Port::ReceiveRandom(double timeout, long *type, double *ts, char **text,
    long *other) const
{
	return ps_receive_random(id(), timeout, type, ts, text, other);
}
 
//=======================================================================
// function:	long  PS_SharedPort::~ PS_SharedPort()
// description:	The  PS_SharedPort destructor.  Frees the port if it was 
//		allocated, otherwise does nothing.
//=======================================================================
 PS_SharedPort::~ PS_SharedPort()
{
	if (allocated)
		ps_release_shared_port(id());
}

//=======================================================================
// function:	long  PS_SharedPort::Receive(double,int*,double*,char**,int*)
// description:	Receives a message from the receiver.
//=======================================================================
SYSCALL  PS_SharedPort::Receive(double timeout, long *type, double *ts, char **text,
    long *other) const
{
	return ps_receive_shared(id(), timeout, type, ts, text, other);
}
 
//=======================================================================
// function:	long  PS_SharedPort::ReceiveLast(double,int*,double*,char**,int*)
// description:	Receives the last message sent to the receiver.
//=======================================================================
SYSCALL  PS_SharedPort::ReceiveLast(double timeout, long *type, double *ts,
    char **text, long *other) const
{
	(void)timeout;
	(void)type;
	(void)ts;
	(void)text;
	(void)other;
	Abort(" PS_SharedPort::ReceiveLast() is an illegal operation");
	return 1;  	// Prevents compiler warning messages
}

//=======================================================================
// function:	long  PS_SharedPort::ReceiveRandom(double,int*,double*,char**,int*)
// description:	Receives a message at random from the receiver.
//=======================================================================
SYSCALL  PS_SharedPort::ReceiveRandom(double timeout, long *type, double *ts,
    char **text, long *other) const
{
	(void)timeout;
	(void)type;
	(void)ts;
	(void)text;
	(void)other;
	Abort(" PS_SharedPort::ReceiveRandom() is an illegal operation");
	return 1;  	// Prevents compiler warning messages
}
