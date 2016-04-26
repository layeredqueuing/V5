// $Id: carrier.h 9042 2009-10-14 01:31:21Z greg $
//=======================================================================
//	carrier.h - PS_Carrier, PS_Link and PS_Bus class declarations.
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
#ifndef __CARRIER_H
#define __CARRIER_H

#ifndef __PARA_ENTITY_H
#include <para_entity.h>
#endif //__PARA_ENTITY_H

#ifndef __NODE_H
#include <node.h>
#endif //__NODE_H

#ifndef __PORT_H
#include <port.h>
#endif //__PORT_H

//=======================================================================
// class:	Carrier
// description:	Encapsulates Parasol message passing mediums.
//=======================================================================
class PS_AbstractCarrier : public PS_ParasolEntity {
protected:
	PS_AbstractCarrier(long cid) : PS_ParasolEntity(cid) {};
 
public:
	virtual SYSCALL Send(const PS_Port& port, long type, int size, char *text,
	    long ap) const = 0;
}; 


//=======================================================================
// class:	PS_Link
// description:	Encapsulates Parasol links.
//=======================================================================
class PS_Link : public PS_AbstractCarrier {
public:
	PS_Link(const char *name, const PS_AbstractNode& src, 
	    const PS_AbstractNode& dest, double rate, long stat_flag);
	virtual SYSCALL Send(const PS_Port& port, long type, int size, char *text,
	    long ap) const;
};

//=======================================================================
// class:	PS_Bus
// description:	Encapsulates Parasol buses.
//=======================================================================
class PS_Bus : public PS_AbstractCarrier {
private:
	static	long	*node_ids;

	long *Nodes2IDs(int nnodes, const PS_AbstractNode **nodes);
	
public:
	PS_Bus(const char *name, long nnodes, const PS_AbstractNode **nodes, 
	    double rate, long discipline, int stat_flag);
	virtual SYSCALL Send(const PS_Port& port, long type, int size, 
	    char *text, long ap) const;
};


#endif //__CARRIER_H
