/* -*- C++ -*-
 * synmodel.h	-- Greg Franks
 *
 * $Id$
 */

#ifndef _SYNMODEL_H
#define _SYNMODEL_H

#include "submodel.h"

/* -------------- Special Submodel for Synchronization  --------------- */

class SynchSubmodel : public Submodel
{
public:
    SynchSubmodel( const unsigned, const Model * );
    virtual ~SynchSubmodel();
	
    virtual void initClients( const Model& );
    virtual void initServers( const Model& );
    virtual void initInterlock();
    virtual void build();
		
    virtual SynchSubmodel& solve( long, MVACount&, const double );
	
    virtual ostream& print( ostream& ) const;

private:
    ostream& printSyncModel( ostream& output ) const;
};

#endif
