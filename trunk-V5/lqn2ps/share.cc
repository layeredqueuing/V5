/* share.cc	-- Greg Franks Tue Nov  4 2008
 * $HeadURL$
 *
 * ------------------------------------------------------------------------
 * $Id$
 * ------------------------------------------------------------------------
 */


#include "share.h"
#include <algorithm>
#include <lqio/error.h>
#include <lqio/glblerr.h>
#include <lqio/input.h>
#include <lqio/dom_group.h>
#include <lqio/dom_processor.h>
#include "processor.h"
#include "task.h"

set<Share *,ltShare> share;

/* ---------------------- Overloaded Operators ------------------------ */

ostream&
operator<<( ostream& output, const Share& self )
{
    switch( Flags::print[OUTPUT_FORMAT].value.i ) {
    case FORMAT_SRVN:
	self.print( output );
	break;
#if defined(TXT_OUTPUT)
    case FORMAT_TXT:
	break;
#endif
#if defined(QNAP_OUTPUT)
    case FORMAT_QNAP:
	break;
#endif
    case FORMAT_XML:
	break;
    default:
	self.draw( output );
	break;
    }

    return output;
}

Share::Share( const LQIO::DOM::Group* dom, const Processor * aProcessor )
    : _documentObject( dom ), myProcessor(aProcessor)
{
    if ( aProcessor ) {
	const_cast<Processor *>(aProcessor)->addShare(this);
    }
}

ostream& 
Share::draw( ostream& output ) const
{
    return output;
}


ostream& 
Share::print( ostream& output ) const
{
    output << "  g " << name()
	   << " " << share()
	   << " " << myProcessor->name() 
	   << endl;
    return output;
}

/* static */
void 
Share::create( LQIO::DOM::Group* domGroup )
{
    const std::string& group_name = domGroup->getName();
    const std::string& processor_name = domGroup->getProcessor()->getName();
    Processor* aProcessor = Processor::find(processor_name.c_str());

    if ( !aProcessor ) return;

    if ( find_if( ::share.begin(), ::share.end(), eqShareStr( group_name.c_str() ) ) != ::share.end() ) {
	LQIO::input_error2( LQIO::ERR_DUPLICATE_SYMBOL, "Group", group_name.c_str() );
	return;
    } 
    if ( aProcessor->scheduling() != SCHEDULE_CFS ) {
	LQIO::input_error2( LQIO::WRN_NON_CFS_PROCESSOR, group_name.c_str(), processor_name.c_str() );
    }

    Share * aShare = new Share( domGroup, aProcessor );
    ::share.insert(aShare);
}



Share * 
Share::find( const string& name )
{
    set<Share *,ltShare>::const_iterator nextShare = find_if( ::share.begin(), ::share.end(), eqShareStr( name ) );
    if ( nextShare == ::share.end() ) {
	return 0;
    } else {
	return *nextShare;
    }
}
