// $Id: task.cc 9210 2010-02-24 18:59:24Z greg $
//=======================================================================
//	task.cc - PS_AbstractTask, PS_UserTask and PS_SystemTask class
//		     definitions.
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
#include <task.h>
 
//=======================================================================
// function:	cpp_wrapper
// description:	Helps PS_UserTask::PS_UserTask(const Node&,int,int)
//=======================================================================
void cpp_wrapper(void *)
{
	long	type, ack_port;
	double	ts;
	PS_UserTask 	*task;

	ps_receive(ps_my_std_port, NEVER, &type, &ts, (char**)&task, 
	    &ack_port);
	task->code( 0 );
	ps_kill(ps_myself);	// Only necessary for NeXTStep
}

//=======================================================================
// function:	PS_UserTask::PS_UserTask(char*,PS_AbstractNode&,int,int)
// description:	The PS_AbstractTask constructor.
//=======================================================================
PS_UserTask::PS_UserTask(const char *name, const PS_AbstractNode& node,
			 long cpu, long priority, long group ) 
    : PS_AbstractTask(ps_create_group(name, node.id(), cpu, cpp_wrapper, priority, group))
{
	if (id() == SYSERR)
		Abort("PS_UserTask constructor failed");
	if (ps_send(StandardPort().id(), 0, (char*)this, NULL_PORT) == SYSERR)
		Abort("Unable to send initialization message");
}

//=======================================================================
// function:	PS_UserTask::~PS_UserTask()
// description:	The PS_AbstractTask constructor.
//=======================================================================
PS_UserTask::~PS_UserTask()
{
 	Kill();
}

