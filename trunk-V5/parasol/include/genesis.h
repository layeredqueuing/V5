// $Id$
//=======================================================================
//	genesis.h - The simulation startup code header file.
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
#ifndef __GENESIS_H
#define __GENESIS_H

#ifndef __TASK_H
#include <task.h>
#endif //__TASK_H

#ifndef __NODE_H
#include <node.h>
#endif //__NODE_H

#ifndef __CARRIER_H
#include <carrier.h>
#endif //__CARRIER_H

//=======================================================================
// class:	PS_GenesisTask
// description:	Represents a ps_genesis task.  There is only one instance
//		of this class and it's called 'genesis'.
//=======================================================================
class PS_GenesisTask : public PS_SystemTask {
private:
	static	long 	init;

	virtual void code( void * );

public:
	PS_GenesisTask() {};
	PS_GenesisTask(long tid);
	friend void ps_genesis(void *);
};

//=======================================================================
// class:	PS_ParasolNode
// description:	Represents a Parasol base node. There is only one instance
//		of this class and it's called 'base_node'.
//=======================================================================
class PS_ParasolNode : public PS_SystemNode {
private:
	static	long	init;

public:
	PS_ParasolNode();
};

//=======================================================================
// class:	PS_Ether
// description:	Represents a zero delay/infinite rate carrier.  There is
//		only one instance of this class and it's called 'ether'.
//=======================================================================
class PS_Ether : public PS_AbstractCarrier {
private:
	static	long	init;

public:
	PS_Ether();
	virtual SYSCALL Send(const PS_Port& port, long type, int size,
	    char *text, long other) const;
	SYSCALL Resend(const PS_Port& port, long type, int size, double ts,
	    char *text, long other) const
	    { return ps_resend(port.id(), type, ts, text, other); };
	long Broadcast(int type, int size, char *text, int other) const
	    { return ps_broadcast(type, text, other); };
};

extern PS_ParasolNode	PS_parasol_node;	// The base node (node 0)
extern PS_GenesisTask	PS_genesis_task;	// The genesis task
extern PS_Ether		PS_ether;		// The 'ether' carrier

#endif //__GENESIS_H
