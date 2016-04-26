// $Id: port.h 9042 2009-10-14 01:31:21Z greg $
//=======================================================================
//	port.h - PS_Port, PS_SharedPort and PS_PortSet class declarations.
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
#ifndef __PORT_H
#define __PORT_H

#ifndef __PARA_ENTITY_H
#include <para_entity.h>
#endif //__PARA_ENTITY_H

//=======================================================================
// class:	Port
// description:	Encapsulates PARASOL ports.
//=======================================================================
class PS_Port : public PS_ParasolEntity {
protected:
	long	allocated;	// was allocated with ps_allocate_port?

public:
	PS_Port(const char *name) 
	    : PS_ParasolEntity(ps_allocate_port(name, ps_myself))
	    { allocated = TRUE; };
	PS_Port(long pid) : PS_ParasolEntity(pid) { allocated = FALSE; };
	virtual ~PS_Port();
	PS_Port(const PS_Port& port) : PS_ParasolEntity(port.id()) 
	    { allocated = FALSE; };

	virtual SYSCALL Receive(double timeout, long *type, double *ts,
	    char **text, long *other) const;
 	virtual SYSCALL ReceiveLast(double timeout, long *type, double *ts, 
	    char **text, long *other) const;
 	virtual SYSCALL ReceiveRandom(double timeout, long *type, double *ts, 
	    char **text, long *other) const;
};


//=======================================================================
// class:	PS_SharedPort
// description:	Encapsulates PARASOL shared ports.
//=======================================================================
class PS_SharedPort : public PS_Port {
public:
	PS_SharedPort(const char *name)
	    : PS_Port(ps_allocate_shared_port(name))
	    { allocated = TRUE; };
 	virtual ~PS_SharedPort();
 	virtual SYSCALL Receive(double timeout, long *type, double *ts,
	    char **text, long *other) const;
 	virtual SYSCALL ReceiveLast(double timeout, long *type, double *ts, 
	    char **text, long *other) const;
 	virtual SYSCALL ReceiveRandom(double timeout, long *type, double *ts,
	    char **text, long *other) const;
 };

//=======================================================================
// class:	PS_PortSet
// description:	Encapsulates PARASOL port sets.
//=======================================================================
class PS_PortSet : public PS_Port {
public:
	PS_PortSet(const char *name)
	    : PS_Port(ps_allocate_port_set(name))
	    { allocated = TRUE; };
 
	SYSCALL Add(const PS_Port& port) 
	    { return ps_join_port_set(id(), port.id()); };
	SYSCALL Remove(const PS_Port& port)
	    { return ps_leave_port_set(id(), port.id()); };
};

 
#endif //__PORT_H 
