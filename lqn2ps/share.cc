/* share.cc	-- Greg Franks Tue Nov  4 2008
 * $HeadURL: http://rads-svn.sce.carleton.ca:8080/svn/lqn/trunk-V5/lqn2ps/share.cc $
 *
 * ------------------------------------------------------------------------
 * $Id: share.cc 14134 2020-11-25 18:12:05Z greg $
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

std::set<Share *,LT<Share> > Share::__share;

Share::Share( const LQIO::DOM::Group* dom, const Processor * aProcessor )
    : _documentObject( dom ), _processor(aProcessor)
{
    if ( aProcessor ) {
	const_cast<Processor *>(aProcessor)->addShare(this);
    }
}

std::ostream& 
Share::print( std::ostream& output ) const
{
    output << "  g " << name()
	   << " " << share()
	   << " " << _processor->name() 
	   << std::endl;
    return output;
}

/* static */
void 
Share::create( const std::pair<std::string,LQIO::DOM::Group *>& p )
{
    const std::string& group_name = p.first;
    const LQIO::DOM::Group* domGroup = p.second;
    const std::string& processor_name = domGroup->getProcessor()->getName();
    Processor* aProcessor = Processor::find( processor_name );

    if ( !aProcessor ) return;

    if ( find_if( __share.begin(), __share.end(), EQStr<Share>( group_name ) ) != __share.end() ) {
	LQIO::input_error2( LQIO::ERR_DUPLICATE_SYMBOL, "Group", group_name.c_str() );
	return;
    } 
    if ( aProcessor->scheduling() != SCHEDULE_CFS ) {
	LQIO::input_error2( LQIO::WRN_NON_CFS_PROCESSOR, group_name.c_str(), processor_name.c_str() );
    }

    Share * aShare = new Share( domGroup, aProcessor );
    __share.insert(aShare);
}



Share * 
Share::find( const std::string& name )
{
    std::set<Share *>::const_iterator share = find_if( __share.begin(), __share.end(), EQStr<Share>( name ) );
    return share != __share.end() ? *share : 0;
}
