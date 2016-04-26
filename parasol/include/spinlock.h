// $Id: spinlock.h 9042 2009-10-14 01:31:21Z greg $
//=======================================================================
//	spinlock.h - PS_Spinlock class declaration.
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
#ifndef __SPINLOCK_H
#define __SPINLOCK_H

#ifndef __PARA_ENTITY_H
#include <para_entity.h>
#endif //__PARA_ENTITY_H

//=======================================================================
// class:	PS_Spinlock
// description:	Encapsulates PARASOL spinlocks.
//=======================================================================
class PS_Spinlock : public PS_ParasolEntity {
private:
	static	long 	nextid;		// Next semaphore id

public:
	PS_Spinlock() : PS_ParasolEntity(nextid++) {};
 	SYSCALL Lock() const
	    { return ps_lock(id()); };
	SYSCALL Unlock() const
	    { return ps_unlock(id()); };
};

#endif //__SPINLOCK_H 
