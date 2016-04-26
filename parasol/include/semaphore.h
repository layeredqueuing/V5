// $Id: semaphore.h 9042 2009-10-14 01:31:21Z greg $
//=======================================================================
//	semaphore.h - PS_Semaphore class declaration.
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
#ifndef __SEMAPHORE_H
#define __SEMAPHORE_H

#ifndef __PARA_ENTITY_H
#include <para_entity.h>
#endif //__PARA_ENTITY_H

//=======================================================================
// class:	PS_Semaphore
// description:	Encapsulates PARASOL semaphores.
//=======================================================================
class PS_Semaphore : public PS_ParasolEntity {
private:
	static	long 	nextid;		// Next semaphore id

public:
	PS_Semaphore(long value = 1) : PS_ParasolEntity(nextid++) { Reset(value); };
	SYSCALL Wait() const { return ps_wait_semaphore(id()); };
	SYSCALL Signal() const { return ps_signal_semaphore(id()); };
	SYSCALL Reset(long value) const 
	    { return ps_reset_semaphore(id(), value); };
};

#endif //__SEMAPHORE_H
