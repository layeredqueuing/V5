/* -*- C++ -*-
 * synmodel.h	-- Greg Franks
 *
 * $Id: synmodel.h 13676 2020-07-10 15:46:20Z greg $
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
	
    const char * const submodelType() const { return "Synch Submodel"; }

    virtual SynchSubmodel& initClients( const Model& ) { return *this; }
    virtual SynchSubmodel& initServers( const Model& );
    virtual SynchSubmodel& initInterlock() { return *this; }
    virtual SynchSubmodel& build() { return *this; }
		
    virtual SynchSubmodel& solve( long, MVACount&, const double );
	
    virtual ostream& print( ostream& ) const;

private:
    ostream& printSyncModel( ostream& output ) const;
};

#endif
