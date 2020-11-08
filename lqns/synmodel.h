/* -*- C++ -*-
 * synmodel.h	-- Greg Franks
 *
 * $Id: synmodel.h 14052 2020-11-08 03:04:43Z greg $
 */

#ifndef _SYNMODEL_H
#define _SYNMODEL_H

#include "submodel.h"

/* -------------- Special Submodel for Synchronization  --------------- */

class SynchSubmodel : public Submodel
{
public:
    SynchSubmodel( const unsigned n ) : Submodel( n ) {}
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
