// $Id: para_entity.cc 17453 2024-11-10 12:08:26Z greg $
//=======================================================================
//	para_entity.cc - PS_ParasolEntity class definition.
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
#include <parasol/para_entity.h>
 
//=======================================================================
// function:	void PS_ParasolEntity::Abort(const char*) 
// description: Prints a message and aborts the simulation.
//=======================================================================
void PS_ParasolEntity::Abort(const char *message) const
{
	ps_abort(message);
}
