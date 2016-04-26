// $Id: carrier.cc 9042 2009-10-14 01:31:21Z greg $
//=======================================================================
//	carrier.cc - PS_Carrier, PS_Link and PS_Bus class definitions.
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
#include <stdlib.h>
#include <carrier.h>

long *PS_Bus::node_ids;	// Used by PS_Bus::PS_Bus() and PS_Bus::Nodes2IDs()
 
//=======================================================================
// function:	PS_Link::PS_Link(const char*,const AbstractNode&, 
//		    const AbstractNode& dest,double,int)
// description: PS_Link constructor.
//=======================================================================
PS_Link::PS_Link(const char *name, const PS_AbstractNode& src, 
    const PS_AbstractNode& dest, double rate, long stat_flag)
    : PS_AbstractCarrier(ps_build_link(name, src.id(), dest.id(), rate, stat_flag))
{
	if (id() == SYSERR)
		Abort("PS_Bus constructor failed");
}

//=======================================================================
// function:	long PS_Link::Send(const PS_Port&,int,int,char*,int)
// description:	Sends a message across a link.
//=======================================================================
SYSCALL PS_Link::Send(const PS_Port& port, long type, int size, char *text,
    long ap) const
{
	return ps_link_send(id(), port.id(), type, size, text, ap);
}

//=======================================================================
// function:	PS_Bus::PS_Bus(const char*,int,const AbstractNode**,double,int,int)
// description:	Sends a message across a bus.
//=======================================================================
PS_Bus::PS_Bus(const char *name, long nnodes, const PS_AbstractNode **nodes,
    double rate, long discipline, int stat_flag) 
    : PS_AbstractCarrier(ps_build_bus(name, nnodes, Nodes2IDs(nnodes, nodes),
    rate, discipline, stat_flag))
{
	delete node_ids;
	node_ids = NULL;
	if (id() == SYSERR)
		Abort("PS_Bus constructor failed");
}

//=======================================================================
// function:	PS_Bus::Nodes2IDs(int, const AbstractNode**)
// description:	Sends a message across a bus.
//=======================================================================
long *PS_Bus::Nodes2IDs(int nnodes, const PS_AbstractNode **nodes)
{
	long	i;

	node_ids = new long[nnodes];
	if (!node_ids)
		Abort("Insufficient Memory");
 	for (i = 0; i < nnodes; i++)
		node_ids[i] = nodes[i]->id();
	return node_ids;
}
 
//=======================================================================
// function:	long PS_Bus::Send(int,int,intchar*,int)
// description:	Sends a message across a link.
//=======================================================================
SYSCALL PS_Bus::Send(const PS_Port& port, long type, int size, char *text,
    long ap) const
{
	return ps_bus_send(id(), port.id(), type, size, text, ap);
}
