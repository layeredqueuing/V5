// $Id: node.h 9042 2009-10-14 01:31:21Z greg $
//=======================================================================
//	node.h - PS_AbstractNode, PS_UserNode and PS_SystemNode class
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
//
//=======================================================================
#ifndef __NODE_H
#define __NODE_H

#ifndef __PARASOL_ENTITY_H
#include <para_entity.h>
#endif //__PARASOL_ENTITY_H

//=======================================================================
// class:	PS_AbstractNode
// description:	The base class from which other node classes inherit.
//=======================================================================
class PS_AbstractNode : public PS_ParasolEntity {
protected:
	PS_AbstractNode(long nid) : PS_ParasolEntity(nid) {};

public:
	long ReadyToRun() const
	    { return ps_ready_queue(id(), 0, NULL); };
	long IdleCPUs() const
	    { return ps_idle_cpu(id()); };
};

//=======================================================================
// class:	PS_UserNode
// description:	The base class from which user nodes inherit.
//=======================================================================
class PS_UserNode : public PS_AbstractNode {
public:
	PS_UserNode(const char *name, long ncpus, double speed, double quantum, 
	    long discipline, int stat_flags);
};

//=======================================================================
// class:	PS_SystemNode
// description:	The base class from which system nodes inherit. //=======================================================================
class PS_SystemNode : public PS_AbstractNode {
protected:
	PS_SystemNode(long nid) : PS_AbstractNode(nid) {};
};

#endif //__NODE_H
