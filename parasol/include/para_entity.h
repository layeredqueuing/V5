// $Id: para_entity.h 9042 2009-10-14 01:31:21Z greg $
//=======================================================================
//	para_entity.h - PS_ParasolEntity class declaration.
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
#ifndef __PARA_ENTITY_H
#define __PARA_ENTITY_H

#ifndef __PARASOL_H
#include <parasol.h>
#endif //__PARASOL_H

//=======================================================================
// class:	PS_ParasolEntity
// description:	The class from which all other PARASOL classes derive.
//=======================================================================
class PS_ParasolEntity {
private:
	long	parasol_id;		// PARASOL semaphore id

	void id(long id)
	    { parasol_id = id; };

protected:
	PS_ParasolEntity(long eid) { id(eid); };
	void Abort(const char *message) const;

public:
	long id() const
	    { return parasol_id; };
};

#endif //__PARA_ENTITY_H
