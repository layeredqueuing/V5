// $Id: task.h 9210 2010-02-24 18:59:24Z greg $
//=======================================================================
//	task.h - PS_AbstractTask, PS_UserTask and PS_SystemTask class
//		 declarations.
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
//  Add group field for task , Aug, 2008
//=======================================================================
#ifndef __TASK_H
#define __TASK_H

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
// class:	PS_AbstractTask
// description:	The base class from which other Tasks inherit.
//=======================================================================
class PS_AbstractTask : public PS_ParasolEntity {
private:
	PS_Port std_port;

protected:
	PS_AbstractTask() : PS_ParasolEntity(-1), std_port(-1) {};
	PS_AbstractTask(long tid) : PS_ParasolEntity(tid), std_port(ps_std_port(tid))
	    {};

    	SYSCALL Sleep(double duration)
	    { return (ps_myself == id()) ? ps_sleep(duration) : SYSERR; };
	SYSCALL Compute(double delta)
	    { return (ps_myself == id()) ? ps_compute(delta) : SYSERR; };
	SYSCALL Sync(double delta)
	    { return (ps_myself == id()) ? ps_sync(delta) : SYSERR; };
	SYSCALL Hold(double delta)
	    { return (ps_myself == id()) ? ps_hold(delta) : SYSERR; };
	SYSCALL Headroom()
	    { return (ps_myself == id()) ? ps_headroom() : SYSERR; };
	SYSCALL PassPort(const PS_Port& port, const PS_AbstractTask& task)
	    { return (ps_owner(port.id()) == id()) 
	    ? ps_pass_port(port.id(), task.id()) : SYSERR; };

	virtual void code( void * ) = 0;

public:
 	const PS_Port& StandardPort() const
	    { return std_port; };
	long Priority() const
	    { return ps_task_priority(id()); };
	const char *Name() const
	    { return ps_task_name(id()); };
	long Host() const
	    { return ps_task_host(id()); };
	long State() const
	    { return ps_task_state(id()); };

	SYSCALL Suspend()
	    { return ps_suspend(id()); };
	SYSCALL Resume()
	    { return ps_resume(id()); };
	SYSCALL Awaken()
	    { return ps_awaken(id()); };
	SYSCALL AdjustPriority(long priority)
	    { return ps_adjust_priority(id(), priority); };
	SYSCALL Migrate(const PS_AbstractNode& node, long cpu) 
	    { return ps_migrate(id(), node.id(), cpu); };
};

//=======================================================================
// class:	PS_UserTask
// description:	The base class from which user Tasks inherit.
//=======================================================================
class PS_UserTask : public PS_AbstractTask {
private:
	SYSCALL Kill() { return ps_kill(id()); };

protected:
	virtual ~PS_UserTask();
	PS_UserTask(const char *name, const PS_AbstractNode& node, long cpu,
	    long priority,long group=0);

	friend void cpp_wrapper(void *);
};

//=======================================================================
// class:	PS_SystemTask
// description:	The base class from which system tasks (such as genesis)
//		inherit.
//=======================================================================
class PS_SystemTask : public PS_AbstractTask {	
protected:
	PS_SystemTask() {};	// Do nothing, real work gets done later
	PS_SystemTask(long tid) : PS_AbstractTask(tid) {};
};

#endif //__TASK_H
