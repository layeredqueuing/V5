// $Id: statistic.h 9042 2009-10-14 01:31:21Z greg $
//=======================================================================
//	statistic.h - PS_AbstractStatistic, PS_RateStatistic, 
//		     PS_VariableStatistic and PS_SampleStatistic class
//		     declarations.
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
#ifndef __STATISTIC_H
#define __STATISTIC_H

#ifndef __PARA_ENTITY_H
#include <para_entity.h>
#endif //__PARA_ENTITY_H

//=======================================================================
// class:	Statistic
// description:	Encapsulates PARASOL statistics.
//=======================================================================
class PS_AbstractStatistic : public PS_ParasolEntity {
private:

protected:
	PS_AbstractStatistic(const char *name, long type);
	double Other() const
	    { double mean, other; ps_get_stat (id(), &mean, &other);
	    return other; };

public:
	long Reset()
	    { return ps_reset_stat(id()); };
	double Mean() const
	    { double mean, other; ps_get_stat (id(), &mean, &other); 
	    return mean; };
};

//=======================================================================
// class:	PS_SampleStatistic
// description:	Encapsulates PARASOL SAMPLE statistics.
//=======================================================================
class PS_SampleStatistic : public PS_AbstractStatistic {
public:
	PS_SampleStatistic(const char *name) : PS_AbstractStatistic(name, SAMPLE) {};
	SYSCALL Record(double value)
	    { return ps_record_stat(id(), value); };
	long Observations() const
	    { return (long)Other(); };
};

//=======================================================================
// class:	PS_VariableStatistic
// description:	Encapsulates PARASOL VARIABLE statistics.
//=======================================================================
class PS_VariableStatistic : public PS_AbstractStatistic {
public:
	PS_VariableStatistic(const char *name) 
	    : PS_AbstractStatistic(name, VARIABLE) {};
	SYSCALL Record(double value)
	    { return ps_record_stat(id(), value); };
	double TimePeriod() const
	    { return Other(); };
};

//=======================================================================
// class:	PS_RateStatistic
// description:	Encapsulates PARASOL RATE statistics.
//=======================================================================
class PS_RateStatistic : public PS_AbstractStatistic {
public:
	PS_RateStatistic(const char *name) 
	    : PS_AbstractStatistic(name, RATE) {};
	SYSCALL Record()
	    { return ps_record_rate_stat(id()); };
	double Observations() const
	    { return (long)Other(); };
};


#endif //__STATISTIC_H
