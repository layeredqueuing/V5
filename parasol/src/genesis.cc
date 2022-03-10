// $Id: genesis.cc 15456 2022-03-09 15:06:35Z greg $
//=======================================================================
//	genesis.cc - The simulation startup code.
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
#include <parasol/genesis.h>

long PS_GenesisTask::init = FALSE;
long PS_ParasolNode::init = FALSE;
long PS_Ether::init = FALSE;

// Global variables.
PS_ParasolNode 	PS_parasol_node;
PS_GenesisTask 	PS_genesis_task;
PS_Ether	PS_ether;

//=======================================================================
// function:	PS_GenesisTask::PS_GenesisTask()
// description:	PS_GenesisTask constructor, ensures that there is only ever  
//		one instance.
//=======================================================================
PS_GenesisTask::PS_GenesisTask(long tid) : PS_SystemTask(tid)
{
	if (init)
		Abort("Attempt to create a second instance of PS_GenesisTask");
	init = TRUE;
}
 
//=======================================================================
// function:	PS_ParasolNode::PS_ParasolNode()
// description:	PS_ParasolNode constructor, ensures that there is only ever  
//		one instance.
//=======================================================================
PS_ParasolNode::PS_ParasolNode() : PS_SystemNode(0)
{
	if (init)
		Abort("Attempt to create a second instance of PS_ParasolNode");
	init = TRUE;
}

//=======================================================================
// function:	PS_Ether::PS_Ether()
// description:	PS_Ether constructor, ensures that there is only ever one
//		instance.
//=======================================================================
PS_Ether::PS_Ether() : PS_AbstractCarrier(-1)
{
	if (init)
		Abort("Attemp to create a second instance of PS_Ether");
	init = TRUE;
}

//=======================================================================
// function:	long PS_Ether::Send(int,int,int,char*,int)
// description:	Sends a message across the 'ether'.
//=======================================================================
SYSCALL PS_Ether::Send(const PS_Port& port, long type, int size, char *text,
    long other) const
{
	(void)size;
	return ps_send(port.id(), type, text, other);
}

//=======================================================================
// function:	void ps_genesis(void)
// description:	The simulation entry point.
//=======================================================================
void ps_genesis(void *)
{
	PS_genesis_task = PS_GenesisTask(ps_myself);	// Setup genesis Task
 	PS_genesis_task.code( 0 );				// Call user code
	ps_suspend(ps_myself);			// Only needed for NeXTStep
} 
