// $Id: node.cc 9042 2009-10-14 01:31:21Z greg $
//=======================================================================
//	node.cc - PS_AbstractNode, PS_UserNode and PS_SystemNode class
//		  definitions.
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
#include <node.h>
 
//=======================================================================
// function:	PS_UserNode:: PS_UserNode(const char*,int,double,double,
//		    int,int) 
// description: PS_UserNode constructor.
//=======================================================================
PS_UserNode:: PS_UserNode(const char *name, long ncpus, double speed, 
    double quantum, long discipline, int stat_flags)
    : PS_AbstractNode(ps_build_node(name, ncpus, speed, quantum, discipline,
    stat_flags))

{
	if (id() == SYSERR)
		Abort(" PS_UserNode constructor failed");
}


